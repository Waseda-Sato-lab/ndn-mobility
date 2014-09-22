/* -*- Mode:C++; c-file-style:"gnu" -*- */
/*
 * ccn-mobility.cc
 *  Simple Wifi Mobile scenario for ndnSIM
 *
 * Copyright (c) 2014 Waseda University, Sato Laboratory
 * Author: Jairo Eduardo Lopez <jairo@ruri.waseda.jp>
 *
 * Special thanks to University of Washington for initial templates
 *
 *  ccn-mobility is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  ccn-mobility is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero Public License for more details.
 *
 *  You should have received a copy of the GNU Affero Public License
 *  along with ccn-mobility.  If not, see <http://www.gnu.org/licenses/>.
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

namespace br = boost::random;

typedef struct timeval TIMER_TYPE;
#define TIMER_NOW(_t) gettimeofday (&_t,NULL);
#define TIMER_SECONDS(_t) ((double)(_t).tv_sec + (_t).tv_usec*1e-6)
#define TIMER_DIFF(_t1, _t2) (TIMER_SECONDS (_t1)-TIMER_SECONDS (_t2))

char scenario[250] = "CCNMobility";

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
	uint32_t aps = 6;					// Number of clients in the network
	uint32_t mobile = 1;				// Number of mobile terminals
	uint32_t clients = 1;				// Number of clients in the network
	uint32_t nodes = 12;				// Number of nodes in the network
	int posCC = -1;						// Establish which node will be client
	double sec = 0.0;					// Movement start
	double waitint = 1.0;				// Wait at AP
	double travelTime = 3.0;			// Travel time within APs
	bool traceFiles = false;			// Tells to run the simulation with traceFiles
	bool smart = false;					// Tells to run the simulation with SmartFlooding
	bool bestr = false;					// Tells to run the simulation with BestRoute
	char results[250] = "results";      // Directory to place results

	// Variable for buffer
	char buffer[250];

	CommandLine cmd;
	cmd.AddValue ("aps", "Number of APs", aps);
	cmd.AddValue ("mobile", "Number of mobile terminals", mobile);
	cmd.AddValue ("clients", "Number of clients", clients);
	cmd.AddValue ("network", "Number of network nodes", nodes);
	cmd.AddValue ("results", "Directory to place results", results);
	cmd.AddValue ("start", "Starting second", sec);
	cmd.AddValue ("waitint", "Wait interval between APs", waitint);
	cmd.AddValue ("travel", "Travel time between APs", travelTime);
	cmd.AddValue ("pos", "Position ", posCC);
	cmd.AddValue ("trace", "Enable trace files", traceFiles);
	cmd.AddValue ("smart", "Enable SmartFlooding forwarding", smart);
	cmd.AddValue ("bestr", "Enable BestRoute forwarding", bestr);
	cmd.Parse (argc,argv);

	// Node definitions for mobile terminals
	NodeContainer mobileTerminalContainer;
	mobileTerminalContainer.Create(mobile);

	uint32_t mtId = mobileTerminalContainer.Get (0)->GetId();

	// Nodes for APs
	NodeContainer apsContainer;
	apsContainer.Create (aps);

	// LAN nodes
	NodeContainer networkNodes;
	networkNodes.Create (nodes);

	int desiredConnections = 2;
	int lvl3nodes = aps / desiredConnections;
	int lansize = nodes / lvl3nodes;

	// Second level nodes, after APs
	NodeContainer lanrouterNodes;
	lanrouterNodes.Create (lvl3nodes);

	// Container for all routers
	NodeContainer allRouters;
	allRouters.Add (apsContainer);
	allRouters.Add (lanrouterNodes);

	// Container for all user nodes
	NodeContainer allUserNodes;
	allUserNodes.Add (mobileTerminalContainer);
	allUserNodes.Add (networkNodes);

	// Make sure to seed our random
	gen.seed(std::time(0) + (long long)getpid() << 32);

	// With the network assigned, time to randomly obtain clients and servers
	NS_LOG_INFO ("Obtaining the clients and servers");

	NodeContainer clientNodes;
	std::vector<uint32_t> clientNodeIds;

	if (posCC > -1 && posCC < networkNodes.GetN())
	{
		Ptr<Node> tmp = networkNodes.Get (posCC);
		clientNodes.Add (tmp);
		clientNodeIds.push_back(tmp->GetId());

	} else 	{
		// Obtain the random lists of server and clients
		std::vector<Ptr<Node> > clientVector = assignWithinContainer(networkNodes, clients);

		// We have to manually introduce the Ptr<Node> to the NodeContainers
		// We do this to make them easier to control later
		for (uint32_t i = 0; i < clients ; i++)
		{
			Ptr<Node> tmp = clientVector[i];

			uint32_t nodeNum = tmp->GetId();

			sprintf (buffer, "Adding client node: %d", nodeNum);
			NS_LOG_INFO (buffer);

			clientNodes.Add(tmp);
			clientNodeIds.push_back(nodeNum);
		}
	}

	NS_LOG_INFO ("Placing APs");

	// Mobility definition for APs
	MobilityHelper mobilityStations;

	mobilityStations.SetPositionAllocator ("ns3::GridPositionAllocator",
			"MinX", DoubleValue (0.0),
			"MinY", DoubleValue (0.0),
			"DeltaX", DoubleValue (30.0),
			"DeltaY", DoubleValue (0.0),
			"GridWidth", UintegerValue (apsContainer.GetN ()),
			"LayoutType", StringValue ("RowFirst"));
	mobilityStations.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobilityStations.Install (apsContainer);

	// Mobility definition for Level 1
	MobilityHelper mobilitylvl1;

	mobilitylvl1.SetPositionAllocator ("ns3::GridPositionAllocator",
			"MinX", DoubleValue (15.0),
			"MinY", DoubleValue (30.0),
			"DeltaX", DoubleValue (60.0),
			"DeltaY", DoubleValue (0.0),
			"GridWidth", UintegerValue (lanrouterNodes.GetN ()),
			"LayoutType", StringValue ("RowFirst"));
	mobilitylvl1.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobilitylvl1.Install (lanrouterNodes);

	NS_LOG_INFO ("Placing mobile terminals");

	// Place the mobile terminal above the APs and use the waypoint model
	MobilityHelper mobilityTerminals;

	Vector diff = Vector (0.0, -20.0, 0.0);

	Vector pos;

	Ptr<ListPositionAllocator> initialAlloc = CreateObject<ListPositionAllocator> ();

	Ptr<MobilityModel> mob = apsContainer.Get (0)->GetObject<MobilityModel>();

	pos = mob->GetPosition();

	initialAlloc->Add (Vector (pos.x + diff.x, pos.y + diff.y, pos.z + pos.z));
	mobilityTerminals.SetPositionAllocator(initialAlloc);
	mobilityTerminals.SetMobilityModel("ns3::WaypointMobilityModel");
	mobilityTerminals.Install(mobileTerminalContainer);

	Ptr<WaypointMobilityModel> staWaypointMobility = DynamicCast<WaypointMobilityModel>(mobileTerminalContainer.Get (0)->GetObject<MobilityModel> ());

	sprintf(buffer, "Assigning waypoints - start: %f, pause: %f, travel: %f", sec, waitint, travelTime);

	NS_LOG_INFO (buffer);

	// Assign the waypoints for the mobile terminal
	for (int j = 0; j < aps; j++)
	{
		mob = apsContainer.Get (j)->GetObject<MobilityModel>();

		Vector tmp = mob->GetPosition ();

		Vector wayP = Vector (tmp.x + diff.x, tmp.y + diff.y, tmp.z + diff.z);

		staWaypointMobility->AddWaypoint (Waypoint(Seconds(sec), wayP));
		staWaypointMobility->AddWaypoint (Waypoint(Seconds(sec + waitint), wayP));

		sec += waitint + travelTime;
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

	for (int i = 0; i < aps; i++)
	{
		ssidV.push_back (Ssid ("ap-" + boost::lexical_cast<std::string>(i)));
	}

	NS_LOG_INFO ("Assigning mobile terminal wireless cards");

	// Create a Wifi station type MAC
	wifiMacHelper.SetType("ns3::StaWifiMac",
			"ActiveProbing", BooleanValue (true));

	NetDeviceContainer wifiMTNetDevices = wifi.Install (wifiPhyHelper, wifiMacHelper, mobileTerminalContainer);

	NS_LOG_INFO ("Assigning AP wireless cards");
	std::vector<NetDeviceContainer> wifiAPNetDevices;
	for (int i = 0; i < aps; i++)
	{
		wifiMacHelper.SetType ("ns3::ApWifiMac",
	                       "Ssid", SsidValue (ssidV[i]),
	                       "BeaconGeneration", BooleanValue (true),
	                       "BeaconInterval", TimeValue (Seconds (0.1)));

		wifiAPNetDevices.push_back (wifi.Install (wifiPhyHelper, wifiMacHelper, apsContainer.Get (i)));
	}

	NS_LOG_INFO ("Creating Ptp connections");
	NetDeviceContainer p2pAPDevices;

	// Connect every 2 APs to each other
	PointToPointHelper p2p_1gb5ms;
	p2p_1gb5ms.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
	p2p_1gb5ms.SetChannelAttribute ("Delay", StringValue ("5ms"));

	for (int j = 0; j < aps; j+=2)
	{
		p2pAPDevices.Add (
				p2p_1gb5ms.Install (apsContainer.Get (j), apsContainer.Get (j+1)));
	}

	// Connect APs to Lan routers
	NetDeviceContainer p2pAPLanDevices;

	for (int j = 0; j < aps; j+=2)
	{
		int k = j / 2;
		p2pAPLanDevices.Add (
				p2p_1gb5ms.Install (apsContainer. Get(j), lanrouterNodes.Get (k)));
		p2pAPLanDevices.Add (
				p2p_1gb5ms.Install (apsContainer. Get(j+1), lanrouterNodes.Get (k)));
	}

	// Connect second level nodes among themselves
	NetDeviceContainer p2pLanRouterDevices;

	for (int j = 0; j < lvl3nodes-1; j++)
	{
		p2pLanRouterDevices.Add (
				p2p_1gb5ms.Install (lanrouterNodes.Get (j), lanrouterNodes.Get (j+1)));
	}

	NS_LOG_INFO ("Creating LAN connections");
	std::vector<NodeContainer> lans;

	int step = 0;

	for (int i = 0; i < lvl3nodes; i++)
	{
		NodeContainer tmp;
		tmp.Add (lanrouterNodes.Get (i));

		for (int k = 0; k < lansize; k++)
		{
			tmp.Add (networkNodes.Get (k + step));
		}

		step += lansize;
		lans.push_back (tmp);
	}

	std::vector<CsmaHelper> csmaV;

	for (int j = 0; j < lvl3nodes; j++)
	{
		CsmaHelper csma;
		csma.SetChannelAttribute("Delay", StringValue("2ms"));
		csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));

		csmaV.push_back (csma);
	}
	CsmaHelper csma;

	std::vector<NetDeviceContainer> lanDevices;

	for (int j = 0; j < lvl3nodes; j++)
	{
		lanDevices.push_back (csmaV[j].Install (lans[j]));
	}

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
	ndnHelperRouters.Install (allRouters);

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
	producerHelper.Install (mobileTerminalContainer);

	NS_LOG_INFO ("Installing Consumer Application");
	// Create the consumer on the randomly selected node
	ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
	consumerHelper.SetPrefix ("/waseda/sato");
	consumerHelper.SetAttribute ("Frequency", DoubleValue (10.0));
	consumerHelper.SetAttribute("StartTime", TimeValue (Seconds(travelTime /2)));
	consumerHelper.SetAttribute("StopTime", TimeValue (Seconds(sec-1)));
	consumerHelper.Install (clientNodes);

	sprintf(buffer, "Ending time! %f", sec-1);
	NS_LOG_INFO(buffer);

	// If the variable is set, print the trace files
	if (traceFiles) {
		// Filename
		char filename[250];

		// File ID
		char fileId[250];

		// Create the file identifier
		sprintf(fileId, "%s-%02d-%03d-%03d.txt", routeType, mobile, clients, nodes);

		// Print server nodes to file
		sprintf(filename, "%s/%s-servers-%s", results, scenario, fileId);

	/*	NS_LOG_INFO ("Printing node files");
		std::ofstream serverFile;
		serverFile.open (filename);
		for (int i = 0; i < serverNodeIds.size(); i++) {
			serverFile << serverNodeIds[i] << std::endl;
		}
		serverFile.close();*/

		sprintf(filename, "%s/%s-clients-%s", results, scenario, fileId);

		std::ofstream clientFile;
		clientFile.open (filename);
		for (int i = 0; i < clientNodeIds.size(); i++) {
			clientFile << clientNodeIds[i] << std::endl;
		}
		clientFile.close();

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

	NS_LOG_INFO ("Scheduling events - Getting objects");

	char configbuf[250];
	// This causes the device in mtId to change the SSID, forcing AP change
	sprintf(configbuf, "/NodeList/%d/DeviceList/0/$ns3::WifiNetDevice/Mac/Ssid", mtId);

	// Schedule AP Changes
	double apsec = 0.0;

	NS_LOG_INFO ("Scheduling events - Installing events");
	for (int j = 0; j < aps; j++)
	{
		sprintf(buffer, "Setting mobile node to AP %i at %2f seconds", j, apsec);
		NS_LOG_INFO (buffer);

		Simulator::Schedule (Seconds(apsec), Config::Set, configbuf, SsidValue (ssidV[j]));

		apsec += waitint + travelTime;
	}

	NS_LOG_INFO ("Ready for execution!");

	Simulator::Stop (Seconds (28.0));
	Simulator::Run ();
	Simulator::Destroy ();
}


