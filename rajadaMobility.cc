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
//
// Obs:
// Resultados exibidos em escala de segundos
// executar comando : ./waf --run nomeDoArquivo > result.txt



using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("RajadaWithMobilityProgram");


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
	uint32_t repeticao = 10;

	bool verbose = true;
	uint32_t nServer = 0;
	float tempoExecucao = 100.0;

	bool tracing = false;


	for (uint32_t z = 1; z <= qtddExec; z++) {

		uint32_t nWifi = z * 5;

		Ipv4Address source[nWifi*2];
		Ipv4Address destination[nWifi*2];

		/*Variáveis que acumulam a soma para depois calcular a média*/
		double timeFirstTxPacketMR[nWifi*2];
		double timeFirstRxPacketMR[nWifi*2];
		double timeLastTxPacketMR[nWifi*2];
		double timeLastRxPacketMR[nWifi*2];
		double delaySumMR[nWifi*2];
		double jitterSumMR[nWifi*2];
		double lastDelayMR[nWifi*2];

		uint64_t txBytesMR[nWifi*2];
		uint64_t rxBytesMR[nWifi*2];
		uint64_t txPacketsMR[nWifi*2];
		uint64_t rxPacketsMR[nWifi*2];
		uint64_t lostPacketsMR[nWifi*2];

		for(uint32_t j = 0; j < nWifi*2; j++) {
			timeFirstTxPacketMR[j] = 0.0;
			timeFirstRxPacketMR[j] = 0.0;
			timeLastTxPacketMR[j] = 0.0;
			timeLastRxPacketMR[j] = 0.0;
			delaySumMR[j] = 0.0;
			jitterSumMR[j] = 0.0;
			lastDelayMR[j] = 0.0;
			txBytesMR[j] = 0;
			rxBytesMR[j] = 0;
			txPacketsMR[j] = 0;
			rxPacketsMR[j] = 0;
			lostPacketsMR[j] = 0;
		}


		/*Variáveis para armazenar o valor de cada NÓ para depois calcular o desvio padrão*/
		double timeFirstTxPacketNO[nWifi*2][repeticao];
		double timeFirstRxPacketNO[nWifi*2][repeticao];
		double timeLastTxPacketNO[nWifi*2][repeticao];
		double timeLastRxPacketNO[nWifi*2][repeticao];
		double delaySumNO[nWifi*2][repeticao];
		double jitterSumNO[nWifi*2][repeticao];
		double lastDelayNO[nWifi*2][repeticao];

		uint64_t txBytesNO[nWifi*2][repeticao];
		uint64_t rxBytesNO[nWifi*2][repeticao];
		uint64_t txPacketsNO[nWifi*2][repeticao];
		uint64_t rxPacketsNO[nWifi*2][repeticao];
		uint64_t lostPacketsNO[nWifi*2][repeticao];


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

			/*if (verbose)
			{
				LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
				LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
			}*/

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

			mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
					"Bounds", RectangleValue (Rectangle (0, 40, 0, 40))
			);
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
			address.Assign (apDevices);
			address.Assign (staDevices);




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
				AddressValue sinkAddress (InetSocketAddress (p2pInterfaces.GetAddress (1), 21+i));
				PacketSinkHelper  echoServer ("ns3::TcpSocketFactory", InetSocketAddress (p2pInterfaces.GetAddress (1), 21+i));
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
			AnimationInterface anim ("sim/rajadaMobility/animation.xml");

			std::ostringstream oss;
			oss << "sim/rajadaMobility/"  << nWifi << "-" << k << ".xml";
			std::cout << oss.str();

			flowMonitor->SerializeToXmlFile(oss.str(), true, true);

			flowMonitor->CheckForLostPackets();
			Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
			FlowMonitor::FlowStatsContainer stats = flowMonitor->GetFlowStats ();
			for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
			{
				Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);

				if(k==1){
					source[i->first-1] = t.sourceAddress;
					destination[i->first-1] = t.destinationAddress;
				}


				/*Soma os valores de cada repetição de cada nó para depois calcular a média*/
				timeFirstTxPacketMR[i->first-1] += i->second.timeFirstTxPacket.GetSeconds();
				timeFirstRxPacketMR[i->first-1] += i->second.timeFirstRxPacket.GetSeconds();
				timeLastTxPacketMR[i->first-1] += i->second.timeLastTxPacket.GetSeconds();
				timeLastRxPacketMR[i->first-1] += i->second.timeLastRxPacket.GetSeconds();
				delaySumMR[i->first-1] += i->second.delaySum.GetSeconds();
				jitterSumMR[i->first-1] += i->second.jitterSum.GetSeconds();
				lastDelayMR[i->first-1] += i->second.lastDelay.GetSeconds();
				txBytesMR[i->first-1] += i->second.txBytes;
				rxBytesMR[i->first-1] += i->second.rxBytes;
				txPacketsMR[i->first-1] += i->second.txPackets;
				rxPacketsMR[i->first-1] += i->second.rxPackets;
				lostPacketsMR[i->first-1] += i->second.lostPackets;

				/*Guarda o valor de cada nó para depois calcular o desvio padrão*/
				timeFirstTxPacketNO[i->first-1][k-1] = i->second.timeFirstTxPacket.GetSeconds();
				timeFirstRxPacketNO[i->first-1][k-1] = i->second.timeFirstRxPacket.GetSeconds();
				timeLastTxPacketNO[i->first-1][k-1] = i->second.timeLastTxPacket.GetSeconds();
				timeLastRxPacketNO[i->first-1][k-1] = i->second.timeLastRxPacket.GetSeconds();
				delaySumNO[i->first-1][k-1] = i->second.delaySum.GetSeconds();
				jitterSumNO[i->first-1][k-1] = i->second.jitterSum.GetSeconds();
				lastDelayNO[i->first-1][k-1] = i->second.lastDelay.GetSeconds();
				txBytesNO[i->first-1][k-1] = i->second.txBytes;
				rxBytesNO[i->first-1][k-1] = i->second.rxBytes;
				txPacketsNO[i->first-1][k-1] = i->second.txPackets;
				rxPacketsNO[i->first-1][k-1] = i->second.rxPackets;
				lostPacketsNO[i->first-1][k-1] = i->second.lostPackets;
			}


			Simulator::Destroy ();
		}//fim das repetições

		std::cout << "\n\n";
		std::cout << "Número de nós do wifi: " << nWifi << " \n";
		std::cout << "Quantidade de repetições: " << repeticao << " \n";

		for(uint32_t j = 0; j < nWifi*2; j++) {

			std::cout << "\n\n\n Flow: " << j+1;
			std::cout << " \tSource: " << source[j]  << " \t Destination: " << destination[j] << "  \n";

			double dp = 0.0;

			dp = calcDesvioPadrao(repeticao, timeFirstTxPacketNO[j], timeFirstTxPacketMR[j]/repeticao);
			printEstatistica("timeFirstTxPacket", timeFirstTxPacketMR[j], repeticao, dp);

			dp = calcDesvioPadrao(repeticao, timeFirstRxPacketNO[j], timeFirstRxPacketMR[j]/repeticao);
			printEstatistica("timeFirstRxPacket", timeFirstRxPacketMR[j], repeticao, dp);

			dp = calcDesvioPadrao(repeticao, timeLastTxPacketNO[j], timeLastTxPacketMR[j]/repeticao);
			printEstatistica("timeLastTxPacket", timeLastTxPacketMR[j], repeticao, dp);

			dp = calcDesvioPadrao(repeticao, timeLastRxPacketNO[j], timeLastRxPacketMR[j]/repeticao);
			printEstatistica("timeLastRxPacket", timeLastRxPacketMR[j], repeticao, dp);

			dp = calcDesvioPadrao(repeticao, delaySumNO[j], delaySumMR[j]/repeticao);
			printEstatistica("delaySum", delaySumMR[j], repeticao, dp);

			dp = calcDesvioPadrao(repeticao, jitterSumNO[j], jitterSumMR[j]/repeticao);
			printEstatistica("jitterSum", jitterSumMR[j], repeticao, dp);

			dp = calcDesvioPadrao(repeticao, lastDelayNO[j], lastDelayMR[j]/repeticao);
			printEstatistica("lastDelay", lastDelayMR[j], repeticao, dp);

			dp = calcDesvioPadrao(repeticao, txBytesNO[j], txBytesMR[j]/repeticao);
			printEstatistica("txBytes", txBytesMR[j], repeticao, dp);

			dp = calcDesvioPadrao(repeticao, rxBytesNO[j], rxBytesMR[j]/repeticao);
			printEstatistica("rxBytes", rxBytesMR[j], repeticao, dp);

			dp = calcDesvioPadrao(repeticao, txPacketsNO[j], txPacketsMR[j]/repeticao);
			printEstatistica("txPackets", txPacketsMR[j], repeticao, dp);

			dp = calcDesvioPadrao(repeticao, rxPacketsNO[j], rxPacketsMR[j]/repeticao);
			printEstatistica("rxPackets", rxPacketsMR[j], repeticao, dp);

			dp = calcDesvioPadrao(repeticao, lostPacketsNO[j], lostPacketsMR[j]/repeticao);
			printEstatistica("lostPackets", lostPacketsMR[j], repeticao, dp);

			std::cout << "\n";
			std::cout << " \tCálculos importantes:";
			std::cout << "\n";


			double aux[repeticao];
			double media;

			for(uint32_t l = 0; l < repeticao; l++) {
				aux[l] = delaySumNO[j][l]/rxPacketsNO[j][l];
			}
			media = (delaySumMR[j]/repeticao)/(rxPacketsMR[j]/repeticao);
			dp = calcDesvioPadrao(repeticao, aux, media);
			std::cout << " \tMean delay:  " << media << " \tDesvioPadrão: " << dp <<  " \n";

			for(uint32_t l = 0; l < repeticao; l++) {
				aux[l] = (jitterSumNO[j][l])/(rxPacketsNO[j][l]-1);
			}
			media = (jitterSumMR[j]/repeticao)/((rxPacketsMR[j]/repeticao)-1);
			dp = calcDesvioPadrao(repeticao, aux, media);
			std::cout << " \tMean jitter:  " << media << " \tDesvioPadrão: " << dp <<  " \n";

			for(uint32_t l = 0; l < repeticao; l++) {
				aux[l] = txBytesNO[j][l]/txPacketsNO[j][l];
			}
			media = (txBytesMR[j]/repeticao)/(txPacketsMR[j]/repeticao);
			dp = calcDesvioPadrao(repeticao, aux, media);
			std::cout << " \tMean transmitted packet size (byte):  " << media << " \tDesvioPadrão: " << dp <<  " \n";

			for(uint32_t l = 0; l < repeticao; l++) {
				aux[l] = rxBytesNO[j][l]/rxPacketsNO[j][l];
			}
			media = (rxBytesMR[j]/repeticao)/(rxPacketsMR[j]/repeticao);
			dp = calcDesvioPadrao(repeticao, aux, media);
			std::cout << " \tMean received packet size (byte):  " << media << " \tDesvioPadrão: " << dp <<  " \n";

			for(uint32_t l = 0; l < repeticao; l++) {
				aux[l] = ((8 * txBytesNO[j][l])/(timeLastTxPacketNO[j][l]-timeFirstTxPacketNO[j][l]));
			}
			media = ((8 * txBytesMR[j]/repeticao)/((timeLastTxPacketMR[j]/repeticao)-(timeFirstTxPacketMR[j]/repeticao)));
			dp = calcDesvioPadrao(repeticao, aux, media);
			std::cout << " \tMean transmitted bitrate (bit/s):  " << media << " \tDesvioPadrão: " << dp <<  " \n";

			for(uint32_t l = 0; l < repeticao; l++) {
				aux[l] = ((8 * rxBytesNO[j][l])/(timeLastRxPacketNO[j][l]-timeFirstRxPacketNO[j][l]));
			}
			media = ((8 * rxBytesMR[j]/repeticao)/((timeLastRxPacketMR[j]/repeticao)-(timeFirstRxPacketMR[j]/repeticao)));
			dp = calcDesvioPadrao(repeticao, aux, media);
			std::cout << " \tMean received bitrate (bit/s):  " << media << " \tDesvioPadrão: " << dp <<  " \n";

			for(uint32_t l = 0; l < repeticao; l++) {
				aux[l] = lostPacketsNO[j][l]/(rxPacketsNO[j][l]+lostPacketsNO[j][l]);
			}
			media = (lostPacketsMR[j]/repeticao)/((rxPacketsMR[j]/repeticao)+(lostPacketsMR[j]/repeticao));
			dp = calcDesvioPadrao(repeticao, aux, media);
			std::cout << " \tMean packet loss ratio:  " << media << " \tDesvioPadrão: " << dp <<  " \n";

		}

	}



	return 0;
}
