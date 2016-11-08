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
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include <sstream>

// Default Network Topology
//
// Number of wifi or csma nodes can be increased up to 250
//                          |
//                 Rank 0   |   Rank 1
// -------------------------|----------------------------
//   Wifi 10.1.2.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n2   n3   n4   n0 -------------- n1  Server 10.1.1.2
//                   point-to-point  
//                                   
//                                    

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample");

int 
main (int argc, char *argv[])
{
	uint32_t exec = 1;
	uint32_t qtddExec = 40/5;
	uint32_t repeticao = 10;


	uint64_t maxPackets = 15690;
	double timeInterval = 0.3824;
	uint64_t packetSize = 450;

	bool verbose = true;
	uint32_t nServer = 0;
	float tempoExecucao = 5.0;

	bool tracing = false;

	CommandLine cmd;
	cmd.AddValue ("nServer", "Number of \"extra\" CSMA nodes/devices", nServer);
	cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
	cmd.AddValue ("tracing", "Enable pcap tracing", tracing);

	cmd.Parse (argc,argv);

	for (uint32_t i = exec; i <= qtddExec; i++) {
		for (uint32_t k = 1; k <= repeticao; k++) {

			uint32_t nWifi = i* 5;
			cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);

			// Check for valid number of csma or wifi nodes
			// 250 should be enough, otherwise IP addresses
			// soon become an issue
			if (nWifi > 250 || nServer > 250)
			{
				std::cout << "Too many wifi or csma nodes, no more than 250 each." << std::endl;
				return 1;
			}

			if (verbose)
			{
				LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
				LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
			}

			NodeContainer p2pNodes;
			p2pNodes.Create (2);

			PointToPointHelper pointToPoint;
			pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
			pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

			NetDeviceContainer p2pDevices;
			p2pDevices = pointToPoint.Install (p2pNodes);

			NodeContainer serverNode;
			serverNode.Add (p2pNodes.Get (1));


			NodeContainer wifiStaNodes;
			wifiStaNodes.Create (nWifi);
			NodeContainer wifiApNode = p2pNodes.Get (0);

			YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
			YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
			phy.SetChannel (channel.Create ());

			WifiHelper wifi;
			wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

			WifiMacHelper mac;
			Ssid ssid = Ssid ("ns-3-ssid");
			mac.SetType ("ns3::StaWifiMac",
					"Ssid", SsidValue (ssid),
					"ActiveProbing", BooleanValue (false));

			NetDeviceContainer staDevices;
			staDevices = wifi.Install (phy, mac, wifiStaNodes);

			mac.SetType ("ns3::ApWifiMac",
					"Ssid", SsidValue (ssid));

			NetDeviceContainer apDevices;
			apDevices = wifi.Install (phy, mac, wifiApNode);

			MobilityHelper mobility;

			mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
					"MinX", DoubleValue (0.0),
					"MinY", DoubleValue (0.0),
					"DeltaX", DoubleValue (5.0),
					"DeltaY", DoubleValue (10.0),
					"GridWidth", UintegerValue (3),
					"LayoutType", StringValue ("RowFirst"));

			mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
			mobility.Install (wifiStaNodes);
			mobility.Install (wifiApNode);

			InternetStackHelper stack;
			stack.Install (serverNode);
			stack.Install (wifiApNode);
			stack.Install (wifiStaNodes);

			Ipv4AddressHelper address;

			address.SetBase ("10.1.1.0", "255.255.255.0");
			Ipv4InterfaceContainer p2pInterfaces;
			p2pInterfaces = address.Assign (p2pDevices);

			address.SetBase ("10.1.2.0", "255.255.255.0");
			address.Assign (staDevices);
			address.Assign (apDevices);

			PacketSinkHelper  echoServer ("ns3::UdpSocketFactory", InetSocketAddress (p2pInterfaces.GetAddress (1), 9));

			ApplicationContainer serverApps = echoServer.Install (serverNode.Get (0));
			serverApps.Start (Seconds (1.0));
			serverApps.Stop (Seconds (tempoExecucao));


			UdpEchoClientHelper echoClient (p2pInterfaces.GetAddress (1), 9);
			echoClient.SetAttribute ("MaxPackets", UintegerValue (maxPackets));
			echoClient.SetAttribute ("Interval", TimeValue (Seconds (timeInterval)));
			echoClient.SetAttribute ("PacketSize", UintegerValue (packetSize));

			ApplicationContainer clientApps;
			for (uint32_t i = 0; i < nWifi; i++) {
				clientApps.Add(echoClient.Install (wifiStaNodes.Get (i)));
			}
			clientApps.Start (Seconds (2.0));
			clientApps.Stop (Seconds (tempoExecucao));


			Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

			Ptr<FlowMonitor> flowMonitor;
			FlowMonitorHelper flowHelper;
			flowMonitor = flowHelper.InstallAll();

			Simulator::Stop (Seconds (tempoExecucao));

			if (tracing == true)
			{
				pointToPoint.EnablePcapAll ("third");
				phy.EnablePcap ("third", apDevices.Get (0));
				// csma.EnablePcap ("third", csmaDevices.Get (0), true);
			}

			Simulator::Run ();

			std::ostringstream oss;
			oss << "mo655/simulation/cbrNoMobility" << "-" << nWifi << "-" << k << ".xml";
			std::cout << oss.str();

			flowMonitor->SerializeToXmlFile(oss.str(), true, true);

			Simulator::Destroy ();
		}
	}

	return 0;
}
