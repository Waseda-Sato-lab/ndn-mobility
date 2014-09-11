/* -*- Mode:C++; c-file-style:"gnu"; -*- */
/*
 * ndn-mobility-random.cc
 *  Random walk Wifi Mobile scenario for ndnSIM
 *
 * Copyright (c) 2014 Waseda University, Sato Laboratory
 * Author: Jairo Eduardo Lopez <jairo@ruri.waseda.jp>
 *
 * Special thanks to University of Washington for initial templates
 *
 *  ndn-mobility-random is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  ndn-mobility-random is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero Public License for more details.
 *
 *  You should have received a copy of the GNU Affero Public License
 *  along with ndn-mobility-random.  If not, see <http://www.gnu.org/licenses/>.
 */

// Standard C++ modules
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iterator>
#include <iostream>
#include <map>
#include <string>
#include <sys/time.h>
#include <vector>

// Random modules
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/uniform_real_distribution.hpp>
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
int obtain_Num(int min, int max)
{
	br::uniform_int_distribution<> dist(min, max);
	return dist(gen);
}

// Obtain a random double from a uniform distribution between min and max.
// Must seed number generator to ensure randomness at runtime.
double obtain_Num(double min, double max)
{
	br::uniform_real_distribution<> dist(min, max);
	return dist(gen);
}

// Function to change the SSID of a Node, depending on distance
void SetSSIDviaDistance(uint32_t mtId, Ptr<MobilityModel> node, std::map<std::string, Ptr<MobilityModel> > aps)
{
	char configbuf[250];
	char buffer[250];

	// This causes the device in mtId to change the SSID, forcing AP change
	sprintf(configbuf, "/NodeList/%d/DeviceList/0/$ns3::WifiNetDevice/Mac/Ssid", mtId);

	std::map<double, std::string> SsidDistance;

	// Iterate through the map of seen Ssids
	for (std::map<std::string, Ptr<MobilityModel> >::iterator ii=aps.begin(); ii!=aps.end(); ++ii)
	{
		// Calculate the distance from the AP to the node and save into the map
		SsidDistance[node->GetDistanceFrom((*ii).second)] = (*ii).first;
	}

	double distance = SsidDistance.begin()->first;
	std::string ssid(SsidDistance.begin()->second);

	sprintf(buffer, "Change to SSID %s at distance of %f", ssid.c_str(), distance);

	NS_LOG_INFO(buffer);

	// Because the map sorts by std:less, the first position has the lowest distance
	Config::Set(configbuf, SsidValue(ssid));

	// Empty the maps
	SsidDistance.clear();
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
	double sec = 0.0;                           // Movement start
	bool traceFiles = false;                    // Tells to run the simulation with traceFiles
	bool smart = false;                         // Tells to run the simulation with SmartFlooding
	bool bestr = false;                         // Tells to run the simulation with BestRoute
	bool walk = true;                           // Do random walk at walking speed
	bool car = false;                           // Do random walk at car speed
	char results[250] = "results";              // Directory to place results
	char posFile[250] = "rand-hex.txt";         // File including the positioning of the nodes
	double endTime = 800;                       // Number of seconds to run the simulation

	// Variable for buffer
	char buffer[250];

	CommandLine cmd;
	cmd.AddValue ("mobile", "Number of mobile terminals in simulation", mobile);
	cmd.AddValue ("servers", "Number of servers in the simulation", servers);
	cmd.AddValue ("results", "Directory to place results", results);
	cmd.AddValue ("start", "Starting second", sec);
	cmd.AddValue ("trace", "Enable trace files", traceFiles);
	cmd.AddValue ("smart", "Enable SmartFlooding forwarding", smart);
	cmd.AddValue ("bestr", "Enable BestRoute forwarding", bestr);
	cmd.AddValue ("posfile", "File containing positioning information", posFile);
	cmd.AddValue ("walk", "Enable random walk at walking speed", walk);
	cmd.AddValue ("car", "Enable random walk at car speed", car);
	cmd.AddValue ("endTime", "How long the simulation will last", endTime);
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

	NS_LOG_INFO ("Creating nodes");
	// Node definitions for mobile terminals (consumers)
	NodeContainer mobileTerminalContainer;
	mobileTerminalContainer.Create(mobile);

	std::vector<uint32_t> mobileNodeIds;

	// Save all the mobile Node IDs
	for (int i = 0; i < mobile; i++)
	{
		mobileNodeIds.push_back(mobileTerminalContainer.Get (i)->GetId ());
	}

	// Central Nodes
	NodeContainer centralContainer;
	centralContainer.Create (sectors);

	// Wireless access Nodes
	NodeContainer wirelessContainer;
	wirelessContainer.Create (wnodes);

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

	std::vector<uint32_t> serverNodeIds;

	// Save all the mobile Node IDs
	for (int i = 0; i < servers; i++)
	{
		serverNodeIds.push_back(serverNodes.Get (i)->GetId ());
	}

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

	for (int i = 0; i < mobile; i++)
	{
		int side = obtain_Num(0,3);
		double tmp;

		switch (side)
		{
		case 0:
			initialMobile->Add(Vector(0.0, obtain_Num(0.0, (double)yaxis), 0.0));
			break;
		case 1:
			initialMobile->Add(Vector(obtain_Num(0.0, (double)xaxis), 0.0, 0.0));
			break;
		case 2:
			initialMobile->Add(Vector((double)xaxis, obtain_Num(0.0, (double)yaxis), 0.0));
			break;
		case 3:
			initialMobile->Add(Vector(obtain_Num(0.0, (double)xaxis), (double)yaxis, 0.0));
			break;
		}
	}

	sprintf(buffer, "0|%d|0|%d", xaxis, yaxis);

	string bounds = string(buffer);

	if (! (car || walk))
	{
		cerr << "ERROR: Must choose a speed for random walk!" << endl;
		return 1;
	}

	double carSpeed = 18.5;
	double walkSpeed = 1.4;

	double finalspeed;

	if (car)
	{
		NS_LOG_INFO("Random walk at car speed - 18.5m/s");
		sprintf(buffer, "ns3::ConstantRandomVariable[Constant=%f]", carSpeed);

		finalspeed = carSpeed;
	} else if (walk)
	{
		NS_LOG_INFO("Random walk at human walking speed - 1.4m/s");
		sprintf(buffer, "ns3::ConstantRandomVariable[Constant=%f]", walkSpeed);

		finalspeed = walkSpeed;
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
	wifiPhyHelper.Set("TxPowerStart", DoubleValue(16.0206));
	wifiPhyHelper.Set("TxPowerEnd", DoubleValue(1));

	// Add a simple no QoS based card to the Wifi interfaces
	NqosWifiMacHelper wifiMacHelper = NqosWifiMacHelper::Default ();

	// Create SSIDs for all the APs
	std::vector<Ssid> ssidV;

	NS_LOG_INFO ("Creating ssids for wireless cards");

	// We store the Wifi AP mobility models in a map, ordered by the ssid string. Will be easier to manage when
	// calling the modified StaMApWifiMac
	std::map<std::string, Ptr<MobilityModel> > apTerminalMobility;

	for (int i = 0; i < wnodes; i++)
	{
		// Temporary string containing our SSID
		std::string ssidtmp("ap-" + boost::lexical_cast<std::string>(i));

		// Push the newly created SSID into a vector
		ssidV.push_back (Ssid (ssidtmp));

		// Get the mobility model for wnode i
		Ptr<MobilityModel> tmp = (wirelessContainer.Get (i))->GetObject<MobilityModel> ();

		// Store the information into our map
		apTerminalMobility[ssidtmp] = tmp;
	}

	NS_LOG_INFO ("Assigning mobile terminal wireless cards");

	NS_LOG_INFO ("Assigning AP wireless cards");
	std::vector<NetDeviceContainer> wifiAPNetDevices;
	for (int i = 0; i < wnodes; i++)
	{
		wifiMacHelper.SetType ("ns3::ApWifiMac",
						   "Ssid", SsidValue (ssidV[i]),
						   "BeaconGeneration", BooleanValue (true),
						   "BeaconInterval", TimeValue (Seconds (0.1)));

		wifiAPNetDevices.push_back (wifi.Install (wifiPhyHelper, wifiMacHelper, wirelessContainer.Get (i)));
	}

	// Create a Wifi station with a modified Station MAC.
	wifiMacHelper.SetType("ns3::StaWifiMac",
			"Ssid", SsidValue (ssidV[0]),
			"ActiveProbing", BooleanValue (true));

	NetDeviceContainer wifiMTNetDevices = wifi.Install (wifiPhyHelper, wifiMacHelper, mobileTerminalContainer);

	// Using the same calculation from the Yans-wifi-Channel, we obtain the Mobility Models for the
	// mobile node as well as all the Wifi capable nodes
	Ptr<MobilityModel> mobileTerminalMobility = (mobileTerminalContainer.Get (0))->GetObject<MobilityModel> ();

	std::vector<Ptr<MobilityModel> > mobileTerminalsMobility;

	// Get the list of mobile node mobility models
	for (int i = 0; i < mobile; i++)
	{
		mobileTerminalsMobility.push_back((mobileTerminalContainer.Get (i))->GetObject<MobilityModel> ());
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
	producerHelper.SetAttribute("StopTime", TimeValue (Seconds(endTime-1)));
	producerHelper.Install (serverNodes);

	NS_LOG_INFO ("Installing Consumer Application");
	// Create the consumer on the randomly selected node
	ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
	consumerHelper.SetPrefix ("/waseda/sato");
	consumerHelper.SetAttribute ("Frequency", DoubleValue (10.0));
	consumerHelper.SetAttribute("StartTime", TimeValue (Seconds(1)));
	consumerHelper.SetAttribute("StopTime", TimeValue (Seconds(endTime-1)));
	consumerHelper.Install (mobileTerminalContainer);

	sprintf(buffer, "Ending time! %f", endTime);
	NS_LOG_INFO(buffer);

	// If the variable is set, print the trace files
	if (traceFiles) {
		// Filename
		char filename[250];

		// File ID
		char fileId[250];

		// Create the file identifier
		sprintf(fileId, "%s-%02d-%03d-%03d.txt", routeType, mobile, servers, wnodes);

		sprintf(filename, "%s/%s-clients-%s", results, scenario, fileId);

		std::ofstream clientFile;

		clientFile.open (filename);
		for (int i = 0; i < mobileNodeIds.size(); i++)
		{
			clientFile << mobileNodeIds[i] << std::endl;
		}

		clientFile.close();

		// Print server nodes to file
		sprintf(filename, "%s/%s-servers-%s", results, scenario, fileId);

		std::ofstream serverFile;

		serverFile.open (filename);
		for (int i = 0; i < serverNodeIds.size(); i++)
		{
			serverFile << serverNodeIds[i] << std::endl;
		}

		serverFile.close();

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

	NS_LOG_INFO ("Scheduling events - SSID changes");

	// Schedule AP Changes
	double apsec = 0.0;
	// How often should the AP check it's distance
	double checkTime = 100.0 / finalspeed;
	double j = apsec;

	while ( j < endTime)
	{
		sprintf(buffer, "Running event at %f", j);
		NS_LOG_INFO(buffer);

		for (int i = 0; i < mobile; i++)
		{
			Simulator::Schedule (Seconds(j), &SetSSIDviaDistance, mobileNodeIds[i], mobileTerminalsMobility[i], apTerminalMobility);
		}

		j += checkTime;
	}

	NS_LOG_INFO ("Ready for execution!");

	Simulator::Stop (Seconds (endTime));
	Simulator::Run ();
	Simulator::Destroy ();
}


