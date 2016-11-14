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
#include "ns3/netanim-module.h"
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
// executar comando : ./waf --run mo655rajada1 > result.txt


using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("RajadaWithoutMobilityProgram");


void printEstatistica(string nomeVariavel, double soma, uint32_t repeticao, double dp) {
	std::cout << " \tMédia Repetições " << nomeVariavel <<  ": " << soma/repeticao << " \t Desvio padrão: " << dp << "  \n";
	//std::cout << " \tSoma " << nomeVariavel <<  ": " << soma  << " \t Média Repetições: " << soma/repeticao << " \t Desvio padrão: " << dp << "  \n";
}

double calcDesvioPadrao(uint32_t tamanho, double* valorDoNo, double media) {
	double acumSum = 0.0;

	for(uint32_t l = 0; l < tamanho; l++) {
		acumSum +=  pow ((valorDoNo[l] - media), 2.0);
	}
	return sqrt (acumSum/(tamanho-1));
}

double calcDesvioPadrao(uint32_t tamanho, uint64_t* valorDoNo, double media) {
	double acumSum = 0.0;

	for(uint32_t l = 0; l < tamanho; l++) {
		acumSum +=  pow ((valorDoNo[l] - media), 2.0);
	}
	return sqrt (acumSum/(tamanho-1));
}


int 
main (int argc, char *argv[])
{
	uint32_t qtddExec = 40/5;
	uint32_t repeticao = 1;

	bool verbose = true;
	uint32_t nServer = 0;
	float tempoExecucao = 10.0;

	bool tracing = false;


	for (uint32_t z = 1; z <= qtddExec; z++) {

		uint32_t nWifi = z * 5;

		Ipv4Address source[nWifi*2];
		Ipv4Address destination[nWifi*2];

		Time timeFirstTxPacketMR[nWifi*2];
		Time timeFirstRxPacketMR[nWifi*2];
		Time timeLastTxPacketMR[nWifi*2];
		Time timeLastRxPacketMR[nWifi*2];
		Time delaySumMR[nWifi*2];
		Time jitterSumMR[nWifi*2];
		Time lastDelayMR[nWifi*2];

		uint64_t txBytesMR[nWifi*2];
		uint64_t rxBytesMR[nWifi*2];
		uint64_t txPacketsMR[nWifi*2];
		uint64_t rxPacketsMR[nWifi*2];
		uint64_t lostPacketsMR[nWifi*2];

		for(uint32_t j = 0; j < nWifi*2; j++) {
			txBytesMR[j] = 0;
			rxBytesMR[j] = 0;
			txPacketsMR[j] = 0;
			rxPacketsMR[j] = 0;
			lostPacketsMR[j] = 0;
		}

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

			mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
												mobility.Install (serverNode);

			mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
									"MinX", DoubleValue (20.0),
									"MinY", DoubleValue (0.0),
									"DeltaX", DoubleValue (1.0),
									"DeltaY", DoubleValue (1.0),
									"GridWidth", UintegerValue (1),
									"LayoutType", StringValue ("RowFirst"));
			mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
			mobility.Install (wifiApNode);


			mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
									"MinX", DoubleValue (10.0),
									"MinY", DoubleValue (2.0),
									"DeltaX", DoubleValue (5.0),
									"DeltaY", DoubleValue (2.0),
									"GridWidth", UintegerValue (5),
									"LayoutType", StringValue ("RowFirst"));
			mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
			mobility.Install (wifiStaNodes);


			InternetStackHelper stack;
			stack.Install (serverNode);
			stack.Install (wifiApNode);
			stack.Install (wifiStaNodes);

			Ipv4AddressHelper address;

			address.SetBase ("10.0.0.0", "255.255.255.0");
			Ipv4InterfaceContainer p2pInterfaces;
			p2pInterfaces = address.Assign (p2pDevices);

			address.SetBase ("192.168.0.0", "255.255.255.0");
			address.Assign (staDevices);
			address.Assign (apDevices);



			OnOffHelper onOffHelper ("ns3::TcpSocketFactory", p2pInterfaces.GetAddress (1));
			onOffHelper.SetAttribute ("OnTime", StringValue
					("ns3::NormalRandomVariable[Mean=100.0|Variance=1.0|Bound=1.0]"));
			onOffHelper.SetAttribute ("OffTime", StringValue
					("ns3::NormalRandomVariable[Mean=1.0|Variance=1.0|Bound=1.0]"));
			onOffHelper.SetAttribute ("DataRate",StringValue ("1Mbps"));
			onOffHelper.SetAttribute ("PacketSize", UintegerValue (1426));


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
			AnimationInterface anim ("sim/rajadaNoMobility/animation.xml");

			std::ostringstream oss;
			oss << "sim/rajadaNoMobility/"  << nWifi << "-" << k << ".xml";
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


				/*
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
				std::cout << "  JitterSum: " << i->second.jitterSum  << " \n";
				std::cout << "  lastDelay: " << i->second.lastDelay  << " \n";
				std::cout << "  txBytes: " << i->second.txBytes  << " \n";
				std::cout << "  rxBytes: " << i->second.rxBytes  << " \n";
				std::cout << "  txPackets: " << i->second.txPackets  << " \n";
				std::cout << "  rxPackets: " << i->second.rxPackets  << " \n";
				std::cout << "  lostPackets: " << i->second.lostPackets  << " \n";
				std::cout << "  timesForwarded: " << i->second.timesForwarded  << " \n";*/

				if(k==1){
					/*
					std::ostringstream sources;
					sources << t.sourceAddress;

					std::ostringstream destinations;
					destinations << t.destinationAddress;

					source[i->first-1] = sources.str();
					destination[i->first-1] = destinations.str();*/

					source[i->first-1] = t.sourceAddress;
					destination[i->first-1] = t.destinationAddress;

					timeFirstTxPacketMR[i->first-1]  = i->second.timeFirstTxPacket;
					timeFirstRxPacketMR[i->first-1]  = i->second.timeFirstRxPacket;
					timeLastTxPacketMR[i->first-1]  = i->second.timeLastTxPacket;
					timeLastRxPacketMR[i->first-1]  = i->second.timeLastRxPacket;
					delaySumMR[i->first-1]  = i->second.delaySum;
					jitterSumMR[i->first-1]  = i->second.jitterSum;
					lastDelayMR[i->first-1]  = i->second.lastDelay;
				} else {
					timeFirstTxPacketMR[i->first-1] += i->second.timeFirstTxPacket;
					timeFirstRxPacketMR[i->first-1] += i->second.timeFirstRxPacket;
					timeLastTxPacketMR[i->first-1] += i->second.timeLastTxPacket;
					timeLastRxPacketMR[i->first-1] += i->second.timeLastRxPacket;
					delaySumMR[i->first-1] += i->second.delaySum;
					jitterSumMR[i->first-1] += i->second.jitterSum;
					lastDelayMR[i->first-1] += i->second.lastDelay;
				}

				txBytesMR[i->first-1] += i->second.txBytes;
				rxBytesMR[i->first-1] += i->second.rxBytes;
				txPacketsMR[i->first-1] += i->second.txPackets;
				rxPacketsMR[i->first-1] += i->second.rxPackets;
				lostPacketsMR[i->first-1] += i->second.lostPackets;
				//}
			}


			Simulator::Destroy ();
		}//fim das repetições

		std::cout << "\n\n";
		std::cout << "Número de nós do wifi: " << nWifi << " \n";
		std::cout << "Quantidade de repetições: " << repeticao << " \n";

		for(uint32_t j = 0; j < nWifi*2; j++) {

			std::cout << "\n\n\n Flow: " << j+1;
			std::cout << " \tSource: " << source[j]  << " \t Destination: " << destination[j] << "  \n";
			std::cout << " \tSoma timeFirstTxPacket: " << timeFirstTxPacketMR[j]  << " \t Média Repetições: " << timeFirstTxPacketMR[j]/repeticao << "  \n";
			std::cout << " \tSoma timeFirstRxPacket: " << timeFirstRxPacketMR[j]  << " \t Média Repetições: " << timeFirstRxPacketMR[j]/repeticao << "  \n";
			std::cout << " \tSoma timeLastTxPacket: " << timeLastTxPacketMR[j]  << " \t Média Repetições: " << timeLastTxPacketMR[j]/repeticao << "  \n";
			std::cout << " \tSoma timeLastRxPacket: " << timeLastRxPacketMR[j]  << " \t Média Repetições: " << timeLastRxPacketMR[j]/repeticao << "  \n";
			std::cout << " \tSoma delaySum: " << delaySumMR[j]  << " \t Média Repetições: " << delaySumMR[j]/repeticao << "  \n";
			std::cout << " \tSoma jitterSum: " << jitterSumMR[j]  << " \t Média Repetições: " << jitterSumMR[j]/repeticao << "  \n";
			std::cout << " \tSoma lastDelay: " << lastDelayMR[j]  << " \t Média Repetições: " << lastDelayMR[j]/repeticao << "  \n";
			std::cout << " \tSoma txBytes: " << txBytesMR[j]  << " \t Média Repetições: " << txBytesMR[j]/repeticao << "  \n";
			std::cout << " \tSoma rxBytes: " << rxBytesMR[j]  << " \t Média Repetições: " << rxBytesMR[j]/repeticao << "  \n";
			std::cout << " \tSoma txPackets: " << txPacketsMR[j]  << " \t Média Repetições: " << txPacketsMR[j]/repeticao << "  \n";
			std::cout << " \tSoma rxPackets: " << rxPacketsMR[j]  << " \t Média Repetições: " << rxPacketsMR[j]/repeticao << "  \n";
			std::cout << " \tSoma lostPackets: " << lostPacketsMR[j]  << " \t Média Repetições: " << lostPacketsMR[j]/repeticao << "  \n";
			std::cout << "\n";

			std::cout << " \tCálculos importantes:";
			std::cout << " \tMean delay:  " << (delaySumMR[j]/repeticao)/(rxPacketsMR[j]/repeticao) << "  \n";
			std::cout << " \tMean jitter:  " << (jitterSumMR[j]/repeticao)/((rxPacketsMR[j]/repeticao)-1) << "  \n";
			std::cout << " \tMean transmitted packet size (byte):  " << (txBytesMR[j]/repeticao)/(txPacketsMR[j]/repeticao) << "  \n";
			std::cout << " \tMean received packet size (byte):  " << (rxBytesMR[j]/repeticao)/(rxPacketsMR[j]/repeticao) << "  \n";
			std::cout << " \tMean transmitted bitrate (bit/s):  " << (8 * txBytesMR[j]/repeticao)/((timeLastTxPacketMR[j]/repeticao)-(timeFirstTxPacketMR[j]/repeticao)) << "  \n";
			std::cout << " \tMean received bitrate (bit/s):  " << (8 * rxBytesMR[j]/repeticao)/((timeLastRxPacketMR[j]/repeticao)-(timeFirstRxPacketMR[j]/repeticao)) << "  \n";
			std::cout << " \tMean packet loss ratio:  " << (lostPacketsMR[j]/repeticao)/((rxPacketsMR[j]/repeticao)+(lostPacketsMR[j]/repeticao)) << "  \n";

		}

	}



	return 0;
}
