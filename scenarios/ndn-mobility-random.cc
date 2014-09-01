/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Copyright (c) 2014 Waseda University
 * Author: Jairo Eduardo Lopez <jairo@ruri.waseda.jp>
 *
 * ndn-mobility-random.cc
 *  Random walk Wifi Mobile scenario for ndnSIM
 *
 * Special thanks to University of Washington for initial templates
 */

// Standard C++ modules
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iterator>
#include <iostream>
#include <string>
#include <sys/time.h>
#include <vector>

// Random modules
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/variate_generator.hpp>

// ns3 modules
#include <ns3-dev/ns3/applications-module.h>
#include <ns3-dev/ns3/bridge-helper.h>
#include <ns3-dev/ns3/csma-module.h>
#include <ns3-dev/ns3/core-module.h>
#include <ns3-dev/ns3/mobility-module.h>
#include <ns3-dev/ns3/network-module.h>
#include <ns3-dev/ns3/point-to-point-module.h>
#include <ns3-dev/ns3/wifi-module.h>

// ndnSIM modules
#include <ns3-dev/ns3/ndnSIM-module.h>
#include <ns3-dev/ns3/ndnSIM/utils/tracers/ipv4-rate-l3-tracer.h>
#include <ns3-dev/ns3/ndnSIM/utils/tracers/ipv4-seqs-app-tracer.h>

using namespace ns3;
using namespace boost;
using namespace std;

namespace br = boost::random;

typedef struct timeval TIMER_TYPE;
#define TIMER_NOW(_t) gettimeofday (&_t,NULL);
#define TIMER_SECONDS(_t) ((double)(_t).tv_sec + (_t).tv_usec*1e-6)
#define TIMER_DIFF(_t1, _t2) (TIMER_SECONDS (_t1)-TIMER_SECONDS (_t2))

char scenario[250] = "NDNMobilityRandom";

NS_LOG_COMPONENT_DEFINE (scenario);

// Number generator
br::mt19937_64 gen;

// Obtains a random number from a uniform distribution between min and max.
// Must seed number generator to ensure randomness at runtime.
int obtain_Num(int min, int max) {
    br::uniform_int_distribution<> dist(min, max);
    return dist(gen);
}

std::vector<Ptr<Node> > getVector(NodeContainer node) {

	uint32_t size = node.GetN ();

	std::vector<Ptr<Node> > nodemutable;

	// Copy the Node pointers into a mutable vector
	for (uint32_t i = 0; i < size; i++) {
		nodemutable.push_back (node.Get(i));
	}

	NS_LOG_INFO ("getVector: returning Node vector");

	return nodemutable;
}

// Randomly picks toAsig nodes from a vector that has nodesAvailable in size
std::vector<Ptr<Node> > assignNodes(std::vector<Ptr<Node> > nodes, int toAsig, int nodesAvailable) {

	char buffer[250];

	sprintf(buffer, "assignNodes: to assign %d, left %d", toAsig, nodesAvailable);

	NS_LOG_INFO (buffer);

	std::vector<Ptr<Node> > assignedNodes;

	uint32_t assignMin = nodesAvailable - toAsig;

	// Apply Fisher-Yates shuffle
	for (uint32_t i = nodesAvailable; i > assignMin; i--)
	{
		// Get a random number
		int toSwap = obtain_Num (0, i);
		// Push into the client container
		assignedNodes.push_back (nodes[toSwap]);
		// Swap the obtained number with the last element
		std::swap (nodes[toSwap], nodes[i]);
	}

	return assignedNodes;
}

// Obtains a random list of num_clients clients and num_servers servers from a NodeContainer
tuple<std::vector<Ptr<Node> >, std::vector<Ptr<Node> > > assignClientsandServers(NodeContainer nodes, int num_clients, int num_servers) {

	char buffer[250];

	// Get the number of nodes in the simulation
	uint32_t size = nodes.GetN ();

	sprintf(buffer, "assignClientsandServers, we have %d nodes, will assign %d clients and %d servers", size, num_clients, num_servers);

	NS_LOG_INFO (buffer);

	// Check that we haven't asked for a scenario where we don't have enough Nodes to fulfill
	// the requirements
	if (num_clients + num_servers > size) {
		NS_LOG_INFO("assignClientsandServer, required number bigger than container size!");
		return tuple<std::vector<Ptr<Node> >, std::vector<Ptr<Node> > > ();
	}

	std::vector<Ptr<Node> > nodemutable = getVector(nodes);

	std::vector<Ptr<Node> > ClientContainer = assignNodes(nodemutable, num_clients, size-1);

	std::vector<Ptr<Node> > ServerContainer = assignNodes(nodemutable, num_servers, size-1-num_clients);

	return tuple<std::vector<Ptr<Node> >, std::vector<Ptr<Node> > > (ClientContainer, ServerContainer);
}

// Returns a randomly picked num of Nodes from nodes Container
std::vector<Ptr<Node> > assignWithinContainer (NodeContainer nodes, int num)
{
	char buffer[250];

	// Get the number of nodes in the simulation
	uint32_t size = nodes.GetN ();

	sprintf(buffer, "assignWithinContainer, we have %d nodes, will assign %d", size, num);

	NS_LOG_INFO (buffer);

	if (num > size) {
		NS_LOG_INFO("assignWithinContainer, required number bigger than container size!");
		return std::vector<Ptr<Node> >();
	}

	std::vector<Ptr<Node> > nodemutable = getVector(nodes);

	return assignNodes(nodemutable, num, size-1);

}

// Function to get a complete Random setup
tuple<std::vector<Ptr<Node> >, std::vector<Ptr<Node> > > assignCompleteRandom(int num_clients, int num_servers) {

	// Obtain all the node used in the simulation
	NodeContainer global = NodeContainer::GetGlobal ();

	return assignClientsandServers(global, num_clients, num_servers);
}

int main (int argc, char *argv[])
{
	// These are our scenario arguments
	uint32_t sectors = 9;                       // Number of wireless sectors
	uint32_t aps = 6;					        // Number of wireless access nodes in a sector
	uint32_t mobile = 1;				        // Number of mobile terminals
	uint32_t servers = 1;				        // Number of servers in the network
	uint32_t wnodes = aps * sectors;            // Number of nodes in the network
	uint32_t xaxis = 100;                       // Size of the X axis
	uint32_t yaxis = 100;                       // Size of the Y axis
	int posCC = -1;                             // Establish which node will be client
	double sec = 0.0;                           // Movement start
	double waitint = 1.0;                       // Wait at AP
	double travelTime = 3.0;                    // Travel time within APs
	bool traceFiles = false;                    // Tells to run the simulation with traceFiles
	bool smart = false;                         // Tells to run the simulation with SmartFlooding
	bool bestr = false;                         // Tells to run the simulation with BestRoute
	bool walk = true;                           // Do random walk at walking speed
	bool car = false;                           // Do random walk at car speed
	char results[250] = "results";              // Directory to place results
	char posFile[250] = "rand-hex.txt";          // File including the positioning of the nodes

	// Variable for buffer
	char buffer[250];

	CommandLine cmd;
	cmd.AddValue ("mobile", "Number of mobile terminals in simulation", mobile);
	cmd.AddValue ("servers", "Number of servers in the simulation", servers);
	cmd.AddValue ("results", "Directory to place results", results);
	cmd.AddValue ("start", "Starting second", sec);
	cmd.AddValue ("waitint", "Wait interval between APs", waitint);
	cmd.AddValue ("travel", "Travel time between APs", travelTime);
	cmd.AddValue ("pos", "Position ", posCC);
	cmd.AddValue ("trace", "Enable trace files", traceFiles);
	cmd.AddValue ("smart", "Enable SmartFlooding forwarding", smart);
	cmd.AddValue ("bestr", "Enable BestRoute forwarding", bestr);
	cmd.AddValue ("posfile", "File containing positioning information", posFile);
	cmd.AddValue ("walk", "Enable random walk at walking speed", walk);
	cmd.AddValue ("car", "Enable random walk at car speed", car);
	cmd.Parse (argc,argv);

	vector<double> centralXpos;
	vector<double> centralYpos;

	vector<double> wirelessXpos;
	vector<double> wirelessYpos;

	NS_LOG_INFO ("Attempting to read positions file");

	// Attempt to read the file with the position data
	ifstream file;

	file.open (posFile);

	if (!file.is_open ()) {
		cerr << "ERROR: Error opening file -> " << posFile << endl;
		cerr << "ERROR: Please check position file before running simulation!" << endl;
		return 1;
	}
	else {
		// Attempt to read everything

		string line;

		// Get the size of the area to simulate
		NS_LOG_INFO ("Reading X and Y axis");

		getline(file, line);
		istringstream xss(line);
		xss >> xaxis;

		getline(file, line);
		istringstream yss(line);
		yss >> yaxis;

		// Get the number of central nodes
		NS_LOG_INFO ("Reading number of sectors");

		getline(file, line);
		istringstream sss(line);
		sss >> sectors;

		double tmpX;
		double tmpY;

		NS_LOG_INFO ("Reading sector positions");

		// Read the position for each sector, save into vector
		for (int i = 0; i < sectors; i++) {
			getline(file, line, ',');
			istringstream dxs(line);
			dxs >> tmpX;

			centralXpos.push_back(tmpX);

			getline(file, line);
			istringstream dys(line);
			dys >> tmpY;

			centralYpos.push_back(tmpY);
		}

		// Get the number of wireless nodes per sector
		NS_LOG_INFO ("Reading number of wireless nodes per sector");
		getline(file, line);
		istringstream apss(line);
		apss >> aps;

		// Get the total number of wireless nodes in simulation
		NS_LOG_INFO ("Reading number of wireless nodes in the simulation");
		getline(file, line);
		istringstream wnss(line);
		wnss >> wnodes;

		// Read the position for each wireless node, save into vectors
		NS_LOG_INFO ("Reading wireless node positions");
		for (int i = 0; i < wnodes; i++) {
			getline(file, line, ',');
			istringstream dxs(line);
			dxs >> tmpX;

			wirelessXpos.push_back(tmpX);

			getline(file, line);
			istringstream dys(line);
			dys >> tmpY;

			wirelessYpos.push_back(tmpY);
		}
	}

	// Node definitions for mobile terminals (consumers)
	NodeContainer mobileTerminalContainer;
	mobileTerminalContainer.Create(mobile);

	uint32_t mtId = mobileTerminalContainer.Get (0)->GetId();

	// Central Nodes
	NodeContainer centralContainer;
	centralContainer.Create (sectors);

	// Wireless access Nodes
	NodeContainer wirelessContainer;
	wirelessContainer.Create(wnodes);

	// Separate the wireless nodes into sector specific containers
	std::vector<NodeContainer> sectorNodes;

	for (int i = 0; i < sectors; i++)
	{
		NodeContainer wireless;
		for (int j = i*aps; j < aps + i*aps; j++)
		{
			wireless.Add(wirelessContainer.Get (j));
		}
		sectorNodes.push_back(wireless);
	}

	// Find out how many first level nodes we will have
	// The +1 is for the server which will be attached to the first level nodes
	int first = (sectors / 3) + 1;

	// First level Nodes
	NodeContainer firstLevel;
	firstLevel.Create (first);

	// Container for all NDN capable nodes
	NodeContainer allNdnNodes;
	allNdnNodes.Add (centralContainer);
	allNdnNodes.Add (wirelessContainer);
	allNdnNodes.Add (firstLevel);

	// Container for all NDN capable nodes not in first level
	NodeContainer lowerNdnNodes;
	lowerNdnNodes.Add (centralContainer);
	lowerNdnNodes.Add (wirelessContainer);

	// Container for server (producer) nodes
	NodeContainer serverNodes;
	serverNodes.Create (servers);

	// Container for all nodes without NDN specific capabilities
	NodeContainer allUserNodes;
	allUserNodes.Add (mobileTerminalContainer);
	allUserNodes.Add (serverNodes);

	// Make sure to seed our random
	gen.seed (std::time (0) + (long long)getpid () << 32);

	NS_LOG_INFO ("Placing Central nodes");
	MobilityHelper centralStations;

	Ptr<ListPositionAllocator> initialCenter = CreateObject<ListPositionAllocator> ();

	for (int i = 0; i < sectors; i++)
	{
		Vector pos (centralXpos[i], centralYpos[i], 0.0);
		initialCenter->Add (pos);
	}

	centralStations.SetPositionAllocator(initialCenter);
	centralStations.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	centralStations.Install(centralContainer);

	NS_LOG_INFO ("Placing wireless access nodes");
	MobilityHelper wirelessStations;

	Ptr<ListPositionAllocator> initialWireless = CreateObject<ListPositionAllocator> ();

	for (int i = 0; i < wnodes; i++)
	{
		Vector pos (wirelessXpos[i], wirelessYpos[i], 0.0);
		initialWireless->Add (pos);
	}

	wirelessStations.SetPositionAllocator(initialWireless);
	wirelessStations.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	wirelessStations.Install(wirelessContainer);

	NS_LOG_INFO ("Placing mobile node");
	MobilityHelper mobileStations;

	Ptr<ListPositionAllocator> initialMobile = CreateObject<ListPositionAllocator> ();

	initialMobile->Add(Vector(0.0, 0.0, 0.0));

	sprintf(buffer, "0|%d|0|%d", xaxis, yaxis);

	string bounds = string(buffer);

	if (! (car || walk))
	{
		cerr << "ERROR: Must choose a speed for random walk!" << endl;
		return 1;
	}

	if (car)
	{
		NS_LOG_INFO("Random walk at car speed - 18.5m/s");
		sprintf(buffer, "ns3::ConstantRandomVariable[Constant=%f]", 18.5);
	} else if (walk)
	{
		NS_LOG_INFO("Random walk at human walking speed - 1.4m/s");
		sprintf(buffer, "ns3::ConstantRandomVariable[Constant=%f]", 1.4);
	}

	string speed = string(buffer);

	mobileStations.SetPositionAllocator(initialMobile);
	mobileStations.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
	                             "Mode", StringValue ("Distance"),
	                             "Distance", StringValue ("500"),
	                             "Speed", StringValue (speed),
	                             "Bounds", StringValue (bounds));

	mobileStations.Install(mobileTerminalContainer);


	// Connect Wireless Nodes to central nodes
	// Because the simulation is using Wifi, PtP connections are 100Mbps
	// with 5ms delay
	NS_LOG_INFO("Connecting Central nodes to wireless access nodes");

	vector <NetDeviceContainer> ptpWLANCenterDevices;

	PointToPointHelper p2p_100mbps5ms;
	p2p_100mbps5ms.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
	p2p_100mbps5ms.SetChannelAttribute ("Delay", StringValue ("5ms"));

	for (int i = 0; i < sectors; i++)
	{
		NetDeviceContainer ptpWirelessCenterDevices;

		for (int j = 0; j < aps; j++)
		{
			ptpWirelessCenterDevices.Add (p2p_100mbps5ms.Install (centralContainer.Get (i), sectorNodes[i].Get (j) ));
		}

		ptpWLANCenterDevices.push_back (ptpWirelessCenterDevices);
	}

	// Connect the server to the lone core node
	NetDeviceContainer ptpServerlowerNdnDevices;
	ptpServerlowerNdnDevices.Add (p2p_100mbps5ms.Install (serverNodes.Get (0), firstLevel.Get (first-1)));

	// Connect the center nodes to first level nodes
	NS_LOG_INFO("Connecting Central Nodes amongst themselves");
	vector <NetDeviceContainer> ptpCenterFirstDevices;

	PointToPointHelper p2p_1Gbps2ms;
	p2p_1Gbps2ms.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
	p2p_1Gbps2ms.SetChannelAttribute ("Delay", StringValue ("2ms"));

	for (int i = 0; i < first-1; i++)
	{
		NetDeviceContainer ptpCenterDevices;

		for (int j = i; j < sectors; j+=(first-1))
		{
			ptpCenterDevices.Add (p2p_1Gbps2ms.Install (firstLevel.Get (i), centralContainer.Get (j)));
		}

		ptpCenterFirstDevices.push_back (ptpCenterDevices);
	}

	// Connect the first level nodes amongst themselves
	NS_LOG_INFO("Connecting First level nodes amongst themselves");
	NetDeviceContainer ptpFirstFirstDevices;

	for (int i = 0; i < first; i++)
	{
		for (int j = i+1; j < first; j++)
		{
			ptpFirstFirstDevices.Add (p2p_1Gbps2ms.Install (firstLevel.Get (i), firstLevel.Get (j)));
		}
	}

	NS_LOG_INFO ("Creating Wireless cards");

	// Use the Wifi Helper to define the wireless interfaces for APs
	WifiHelper wifi = WifiHelper::Default ();
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager");

	YansWifiChannelHelper wifiChannel;
	wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
	wifiChannel.AddPropagationLoss ("ns3::ThreeLogDistancePropagationLossModel");
	wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel");

	// All interfaces are placed on the same channel. Makes AP changes easy. Might
	// have to be reconsidered for multiple mobile nodes
	YansWifiPhyHelper wifiPhyHelper = YansWifiPhyHelper::Default ();
	wifiPhyHelper.SetChannel (wifiChannel.Create ());
	wifiPhyHelper.Set("TxPowerStart", DoubleValue(5));
	wifiPhyHelper.Set("TxPowerEnd", DoubleValue(5));

	// Add a simple no QoS based card to the Wifi interfaces
	NqosWifiMacHelper wifiMacHelper = NqosWifiMacHelper::Default ();

	// Create SSIDs for all the APs
	std::vector<Ssid> ssidV;

	NS_LOG_INFO ("Creating ssids for wireless cards");

	for (int i = 0; i < wnodes; i++)
	{
		ssidV.push_back (Ssid ("ap-" + boost::lexical_cast<std::string>(i)));
	}

	NS_LOG_INFO ("Assigning mobile terminal wireless cards");

	NS_LOG_INFO ("Assigning AP wireless cards");
	std::vector<NetDeviceContainer> wifiAPNetDevices;
	for (int i = 0; i < wnodes; i++)
	{
		wifiMacHelper.SetType ("ns3::ApWifiMac",
						   "Ssid", SsidValue (ssidV[0]),
						   "BeaconGeneration", BooleanValue (true),
						   "BeaconInterval", TimeValue (Seconds (0.1)));

		wifiAPNetDevices.push_back (wifi.Install (wifiPhyHelper, wifiMacHelper, wirelessContainer.Get (i)));
	}

	// Create a Wifi station type MAC
	wifiMacHelper.SetType("ns3::StaWifiMac",
			"Ssid", SsidValue (ssidV[0]),
			"ActiveProbing", BooleanValue (true));

	NetDeviceContainer wifiMTNetDevices = wifi.Install (wifiPhyHelper, wifiMacHelper, mobileTerminalContainer);


	char routeType[250];

	// Now install content stores and the rest on the middle node. Leave
	// out clients and the mobile node
	NS_LOG_INFO ("Installing NDN stack on routers");
	ndn::StackHelper ndnHelperRouters;

	// Decide what Forwarding strategy to use depending on user command line input
	if (smart) {
		sprintf(routeType, "%s", "smart");
		NS_LOG_INFO ("NDN Utilizing SmartFlooding");
		ndnHelperRouters.SetForwardingStrategy ("ns3::ndn::fw::SmartFlooding::PerOutFaceLimits", "Limit", "ns3::ndn::Limits::Window");
	} else if (bestr) {
		sprintf(routeType, "%s", "bestr");
		NS_LOG_INFO ("NDN Utilizing BestRoute");
		ndnHelperRouters.SetForwardingStrategy ("ns3::ndn::fw::BestRoute::PerOutFaceLimits", "Limit", "ns3::ndn::Limits::Window");
	} else {
		sprintf(routeType, "%s", "flood");
		NS_LOG_INFO ("NDN Utilizing Flooding");
		ndnHelperRouters.SetForwardingStrategy ("ns3::ndn::fw::Flooding::PerOutFaceLimits", "Limit", "ns3::ndn::Limits::Window");
	}

	// Set the Content Stores
	ndnHelperRouters.SetContentStore ("ns3::ndn::cs::Freshness::Lru", "MaxSize", "1000");
	ndnHelperRouters.SetDefaultRoutes (true);
	// Install on ICN capable routers
	ndnHelperRouters.Install (allNdnNodes);

	// Create a NDN stack for the clients and mobile node
	ndn::StackHelper ndnHelperUsers;
	// These nodes have only one interface, so BestRoute forwarding makes sense
	ndnHelperUsers.SetForwardingStrategy ("ns3::ndn::fw::BestRoute");
	// No Content Stores are installed on these machines
	ndnHelperUsers.SetContentStore ("ns3::ndn::cs::Nocache");
	ndnHelperUsers.SetDefaultRoutes (true);
	ndnHelperUsers.Install (allUserNodes);

	NS_LOG_INFO ("Installing Producer Application");
	// Create the producer on the mobile node
	ndn::AppHelper producerHelper ("ns3::ndn::Producer");
	producerHelper.SetPrefix ("/waseda/sato");
	producerHelper.SetAttribute("StopTime", TimeValue (Seconds(sec)));
	producerHelper.Install (serverNodes);

	NS_LOG_INFO ("Installing Consumer Application");
	// Create the consumer on the randomly selected node
	ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
	consumerHelper.SetPrefix ("/waseda/sato");
	consumerHelper.SetAttribute ("Frequency", DoubleValue (10.0));
	consumerHelper.SetAttribute("StartTime", TimeValue (Seconds(travelTime /2)));
	consumerHelper.SetAttribute("StopTime", TimeValue (Seconds(sec-1)));
	consumerHelper.Install (mobileTerminalContainer);

	sprintf(buffer, "Ending time! %f", sec-1);
	NS_LOG_INFO(buffer);

	// If the variable is set, print the trace files
	if (traceFiles) {
		// Filename
		char filename[250];

		// File ID
		char fileId[250];

		// Create the file identifier
		sprintf(fileId, "%s-%02d-%03d-%03d.txt", routeType, mobile, servers, wnodes);

		// Print server nodes to file
		sprintf(filename, "%s/%s-servers-%d", results, scenario, fileId);

	/*	NS_LOG_INFO ("Printing node files");
		std::ofstream serverFile;
		serverFile.open (filename);
		for (int i = 0; i < serverNodeIds.size(); i++) {
			serverFile << serverNodeIds[i] << std::endl;
		}
		serverFile.close();*/

//		sprintf(filename, "%s/%s-clients-%s", results, scenario, fileId);
//
//		std::ofstream clientFile;
//		clientFile.open (filename);
//		for (int i = 0; i < clientNodeIds.size(); i++) {
//			clientFile << clientNodeIds[i] << std::endl;
//		}
//		clientFile.close();

		NS_LOG_INFO ("Installing tracers");
		// NDN Aggregate tracer
		sprintf (filename, "%s/%s-aggregate-trace-%s", results, scenario, fileId);
		ndn::L3AggregateTracer::InstallAll(filename, Seconds (1.0));

		// NDN L3 tracer
		sprintf (filename, "%s/%s-rate-trace-%s", results, scenario, fileId);
		ndn::L3RateTracer::InstallAll (filename, Seconds (1.0));

		// NDN App Tracer
		sprintf (filename, "%s/%s-app-delays-%s", results, scenario, fileId);
		ndn::AppDelayTracer::InstallAll (filename);

		// L2 Drop rate tracer
		sprintf (filename, "%s/%s-drop-trace-%s", results, scenario, fileId);
		L2RateTracer::InstallAll (filename, Seconds (0.5));

		// Content Store tracer
		sprintf (filename, "%s/%s-cs-trace-%s", results, scenario, fileId);
		ndn::CsTracer::InstallAll (filename, Seconds (1));
	}
//
//	NS_LOG_INFO ("Scheduling events - Getting objects");
//
//	char configbuf[250];
//	// This causes the device in mtId to change the SSID, forcing AP change
//	sprintf(configbuf, "/NodeList/%d/DeviceList/0/$ns3::WifiNetDevice/Mac/Ssid", mtId);
//
//	// Schedule AP Changes
//	double apsec = 0.0;
//
//	NS_LOG_INFO ("Scheduling events - Installing events");
//	for (int j = 0; j < aps; j++)
//	{
//		sprintf(buffer, "Setting mobile node to AP %i at %2f seconds", j, apsec);
//		NS_LOG_INFO (buffer);
//
//		Simulator::Schedule (Seconds(apsec), Config::Set, configbuf, SsidValue (ssidV[j]));
//
//		apsec += waitint + travelTime;
//	}
//
	NS_LOG_INFO ("Ready for execution!");

	Simulator::Stop (Seconds (28.0));
	Simulator::Run ();
	Simulator::Destroy ();
}


