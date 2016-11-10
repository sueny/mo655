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
//#include <sstream>

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

NS_LOG_COMPONENT_DEFINE ("RajadaWithoutMobilityProgram");

int 
main (int argc, char *argv[])
{
	uint32_t qtddExec = 5/5;
	uint32_t repeticao = 2;

	bool verbose = true;
	uint32_t nServer = 0;
	float tempoExecucao = 10.0;

	bool tracing = false;


	for (uint32_t z = 1; z <= qtddExec; z++) {

		uint32_t nWifi = z * 5;
		Time delaySumNodes[nWifi*2];

		for (uint32_t k = 1; k <= repeticao; k++) {


			CommandLine cmd;
			cmd.AddValue ("nServer", "Number of server", nServer);
			cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
			cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
			cmd.AddValue ("tracing", "Enable pcap tracing", tracing);

			cmd.Parse (argc,argv);

			Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1440));

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
				LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
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


			OnOffHelper onOffHelper ("ns3::TcpSocketFactory", p2pInterfaces.GetAddress (1));
			onOffHelper.SetAttribute ("OnTime", StringValue
					("ns3::NormalRandomVariable[Mean=5.0|Variance=1.0|Bound=10.0]"));
			onOffHelper.SetAttribute ("OffTime", StringValue
					("ns3::NormalRandomVariable[Mean=7.0|Variance=1.0|Bound=10.0]"));
			onOffHelper.SetAttribute ("DataRate",StringValue ("1Mbps"));
			onOffHelper.SetAttribute ("PacketSize", UintegerValue (1426));

			/*
			 * client = ns.applications.OnOffHelper("ns3::TcpSocketFactory", csmaInterfaces.GetAddress(nCsma))
    client.SetAttribute("PacketSize", ns.core.UintegerValue(1440)) # +60 header
    client.SetAttribute("MaxBytes", ns.core.UintegerValue(0))
    client.SetAttribute("DataRate", ns3.DataRateValue(ns3.DataRate("1Mbps")))
    client.SetAttribute("OnTime", ns.core.StringValue("ns3::NormalRandomVariable[Mean=5.|Variance=1.|Bound=10.]"))
client.SetAttribute("OffTime", ns.core.StringValue("ns3::NormalRandomVariable[Mean=7.|Variance=1.|Bound=10.]")
			 */


			ApplicationContainer serverApps;
			ApplicationContainer clientApps;

			for (uint32_t i = 0; i < nWifi; i++) {
				AddressValue sinkAddress (InetSocketAddress (p2pInterfaces.GetAddress (1), 9+i));
				PacketSinkHelper  echoServer ("ns3::TcpSocketFactory", InetSocketAddress (p2pInterfaces.GetAddress (1), 9+i));
				serverApps.Add(echoServer.Install (serverNode.Get (0)));

				onOffHelper.SetAttribute("Remote", sinkAddress);
				clientApps.Add(onOffHelper.Install (wifiStaNodes.Get (i)));
			}

			clientApps.Start (Seconds (2.0));
			clientApps.Stop (Seconds (tempoExecucao));

			serverApps.Start (Seconds (1.0));
			serverApps.Stop (Seconds (tempoExecucao));

			Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

			//Install flow monitor in all nodes
			Ptr<FlowMonitor> flowMonitor;
			FlowMonitorHelper flowHelper;
			flowMonitor = flowHelper.InstallAll();

			//Run simulation
			Simulator::Stop (Seconds (tempoExecucao));

			/*
			if (tracing == true)
			{
				pointToPoint.EnableAsciiAll ("third");
				phy.EnableAscii ("third", apDevices.Get (0));
				// csma.EnablePcap ("third", csmaDevices.Get (0), true);
			}*/

			Simulator::Run ();

			std::ostringstream oss;
			oss << "mo655/simulation/rajadaNoMobility" << "-" << nWifi << "-" << k << ".xml";
			std::cout << oss.str();

			flowMonitor->SerializeToXmlFile(oss.str(), true, true);

			flowMonitor->CheckForLostPackets();
			Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
			FlowMonitor::FlowStatsContainer stats = flowMonitor->GetFlowStats ();
			for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
			{
				// first 2 FlowIds are for ECHO apps, we don't want to display them
				//
				// Duration for throughput measurement is 9.0 seconds, since
				//   StartTime of the OnOffApplication is at about "second 1"
				// and
				//   Simulator::Stops at "second 10".
				//if (i->first > 2)
				//{
					Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
					std::cout << "\n";
					std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
					std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
					std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
					std::cout << "  TxOffered:  " << i->second.txBytes * 8.0 / 9.0 / 1000 / 1000  << " Mbps\n";
					std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
					std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
					std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / 9.0 / 1000 / 1000  << " Mbps\n";

					std::cout << "  timeFirstTxPacket: " << i->second.timeFirstTxPacket  << " \n";
					std::cout << "  timeFirstRxPacket: " << i->second.timeFirstRxPacket  << " \n";
					std::cout << "  timeLastTxPacket: " << i->second.timeLastTxPacket  << " \n";
					std::cout << "  timeLastRxPacket: " << i->second.timeLastRxPacket  << " \n";
					std::cout << "  DelaySum: " << i->second.delaySum  << " \n";
					if(k==1){
						delaySumNodes[i->first-1]  = i->second.delaySum;
					} else {
						delaySumNodes[i->first-1]  += i->second.delaySum;
					}
					std::cout << "  JitterSum: " << i->second.jitterSum  << " \n";
					std::cout << "  lastDelay: " << i->second.lastDelay  << " \n";
					std::cout << "  txBytes: " << i->second.txBytes  << " \n";
					std::cout << "  rxBytes: " << i->second.rxBytes  << " \n";
					std::cout << "  txPackets: " << i->second.txPackets  << " \n";
					std::cout << "  rxPackets: " << i->second.rxPackets  << " \n";
					std::cout << "  lostPackets: " << i->second.lostPackets  << " \n";
					std::cout << "  timesForwarded: " << i->second.timesForwarded  << " \n";
				//}
			}


			Simulator::Destroy ();
		}//fim das repetições

		std::cout << "\n\n";
		std::cout << "Número de nós do wifi: " << nWifi << " \n";
		std::cout << "Quantidade de repetições: " << repeticao << " \n";
		for(uint32_t j = 0; j < nWifi*2; j++) {
			std::cout << " Soma Delay: " << delaySumNodes[j]  << " \n";
			std::cout << " Média Delay: " << delaySumNodes[j]/repeticao  << " \n";
		}

	}



	return 0;
}
