#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-layout-module.h"
#include <iostream>

using namespace std;
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Dumbbell");

//Funciones para tamaño de ventana
void TraceCwnd (uint32_t node, uint32_t cwndWindow,
            Callback <void, uint32_t, uint32_t> CwndTrace)
{
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string (node) + "/$ns3::TcpL4Protocol/SocketList/" + std::to_string (cwndWindow) + "/CongestionWindow", CwndTrace);
}

static void CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  //NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << std::endl;
}


int main (int argc, char *argv[])
{
  Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpNewReno"));
  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue(50));

  uint32_t left = 3; // nodos izquierdos
  uint32_t right = 3; // nodos derechos

  //PointToPoint izquierdos, derechos y routers
  PointToPointHelper p2pLeft;
  p2pLeft.SetDeviceAttribute("DataRate", StringValue ("50Kbps"));
  p2pLeft.SetChannelAttribute("Delay", StringValue ("100ms"));

  PointToPointHelper p2pRight;
  p2pRight.SetDeviceAttribute("DataRate", StringValue ("50Kbps"));
  p2pRight.SetChannelAttribute("Delay", StringValue ("100ms"));

  PointToPointHelper p2pRouter;
  p2pRouter.SetDeviceAttribute("DataRate", StringValue ("200Kbps"));
  p2pRouter.SetChannelAttribute("Delay", StringValue ("100ms"));
 
  //Creo la topología Dumbbell con DumbbellHelper
  PointToPointDumbbellHelper dumbbell(left, p2pLeft, right, p2pRight, p2pRouter);

  //Instalo el stack
  InternetStackHelper stack;
  dumbbell.InstallStack(stack);
 
  //Asigno direcciones de IP a cada nodo
  dumbbell.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"),  //10.1.1.0 nodos izquierdos
                                Ipv4AddressHelper ("10.2.1.0", "255.255.255.0"),  //10.2.1.0 nodos derechos
                                Ipv4AddressHelper ("10.3.1.0", "255.255.255.0")); //10.3.1.0 routers

  //Instalo los on/off a los nodos UDP y TCP
  int portUDP=1000;
  OnOffHelper onOffHelperUDP ("ns3::UdpSocketFactory", Address ());
  Address sinkLocalAddresssUDP(InetSocketAddress (Ipv4Address::GetAny (), portUDP));
  PacketSinkHelper sinkUDP ("ns3::UdpSocketFactory", sinkLocalAddresssUDP);
 
  int portTCP=1001;
  OnOffHelper onOffHelperTCP ("ns3::TcpSocketFactory", Address ());
  Address sinkLocalAddresssTCP(InetSocketAddress (Ipv4Address::GetAny (), portTCP));
  PacketSinkHelper sinkTCP ("ns3::TcpSocketFactory", sinkLocalAddresssTCP);

  //Container de apps
  ApplicationContainer clientApps;

  //Ciclo los nodos de la izquierda y defino cual es UDP y cual es TCP
  for(uint32_t i=0; i< dumbbell.LeftCount(); i++) {
    if(i==1) {
      //Nodo con UDP
      AddressValue remoteAddressUDP(InetSocketAddress(dumbbell.GetRightIpv4Address(i), portUDP));
      onOffHelperUDP.SetAttribute("Remote", remoteAddressUDP);
      clientApps.Add(onOffHelperUDP.Install(dumbbell.GetLeft (i)));
      clientApps=sinkUDP.Install(dumbbell.GetRight(i));
    } else {
      //Nodo con TCP
      AddressValue remoteAddressTCP (InetSocketAddress(dumbbell.GetRightIpv4Address(i), portTCP));
      onOffHelperTCP.SetAttribute("Remote", remoteAddressTCP);
      clientApps.Add(onOffHelperTCP.Install(dumbbell.GetLeft(i)));
      clientApps=sinkTCP.Install(dumbbell.GetRight(i));
    }
  }
  //Arrancamos
  clientApps.Start(Seconds(0.0));
  clientApps.Stop(Seconds(10.0));

  dumbbell.BoundingBox(1, 1, 100, 100);
  AnimationInterface anim("Dumbbell.xml"); //Para NetAnim

  //Generamos sims
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();
  Simulator::Stop(Seconds(10));  
  
  //.pcap para analizar con wireshark
  p2pRouter.EnablePcapAll("Dumbbell");
  
  AsciiTraceHelper asciiTraceHelper;
  //Para cada nodo en la topología dumbbell
  for (uint32_t i = 0; i < dumbbell.LeftCount() + dumbbell.RightCount() + 2; i++) {
  //Crea un nuevo stream para cada nodo
  	Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream("Dumbbellcwnd_nodo" + std::to_string(i) + ".txt");
  	//Rastrea la ventana de congestión para el nodo i
  	Simulator::Schedule(Seconds(0.00001), &TraceCwnd, i, 0, MakeBoundCallback (&CwndChange, stream));
  }
  Simulator::Run();

  //Matamos sims
  Simulator::Destroy();

  return 0;
}
