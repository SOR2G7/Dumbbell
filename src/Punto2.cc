#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/flow-monitor-helper.h"
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
  //TcpNewReno y OnOffApp
  Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpNewReno"));
  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue(50));

  //Numero de nodos de hoja del lado izquierdo
  uint32_t leftLeaf = 2;

  //Numero de nodos de hoja del lado derecho
  uint32_t rightLeaf = 2;

  //PointToPoint lado izquierdo
  PointToPointHelper pointToPointLeftLeaf;
  pointToPointLeftLeaf.SetDeviceAttribute("DataRate", StringValue ("100Mbps"));
  pointToPointLeftLeaf.SetChannelAttribute("Delay", StringValue ("100ms"));
  pointToPointLeftLeaf.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("5p")); //Reduzco tamaño maximo de recepcion

  //PointToPoint lado derecho
  PointToPointHelper pointToPointRightLeaf;
  pointToPointRightLeaf.SetDeviceAttribute("DataRate", StringValue ("100Mbps"));
  pointToPointRightLeaf.SetChannelAttribute("Delay", StringValue ("100ms"));
  pointToPointRightLeaf.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("5p")); //Reduzco tamaño maximo de recepcion

  //PointToPoint router central
  PointToPointHelper pointToPointRouterCentral;
  //Reduzco el DataRate de los routers centrales para que sature el canal
  pointToPointRouterCentral.SetDeviceAttribute  ("DataRate", StringValue ("100Kbps"));
  pointToPointRouterCentral.SetChannelAttribute ("Delay", StringValue ("100ms"));
  pointToPointRouterCentral.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("5p")); //Reduzco tamaño maximo de recepcion
 
  //Creo un dumbbell topology con la libreria Helper de ns3
  //Doc en la fuente del informe
  PointToPointDumbbellHelper dumbbell(
                                leftLeaf, pointToPointLeftLeaf,
                                rightLeaf, pointToPointRightLeaf,
                                pointToPointRouterCentral);

  //Instalo el stack
  InternetStackHelper stack;
  dumbbell.InstallStack(stack);
 
  //Asigno direcciones de IP a cada nodo
  //10.1.1.0 -> nodos izquierdos
  //10.2.1.0 -> nodos derechos
  //10.3.1.0 -> nodos centrales
  dumbbell.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"),
                                Ipv4AddressHelper ("10.2.1.0", "255.255.255.0"),
                                Ipv4AddressHelper ("10.3.1.0", "255.255.255.0"));

  //Instalo on/off a los nodos
  //Configuracion para UDP
  int portUDP=1000;
  OnOffHelper onOffHelperUDP ("ns3::UdpSocketFactory", Address ());
  Address sinkLocalAddresssUDP(InetSocketAddress (Ipv4Address::GetAny (), portUDP));
  PacketSinkHelper sinkUDP ("ns3::UdpSocketFactory", sinkLocalAddresssUDP);
 
  //Configuracion para TCP
  //creo un on/off helper para TCP
  int portTCP=1001;
  OnOffHelper onOffHelperTCP ("ns3::TcpSocketFactory", Address ());
  Address sinkLocalAddresssTCP(InetSocketAddress (Ipv4Address::GetAny (), portTCP));
  PacketSinkHelper sinkTCP ("ns3::TcpSocketFactory", sinkLocalAddresssTCP);

  //Container de apps
  ApplicationContainer clientApps;

  //Ciclo los nodos y defino cual es TCP y cual es UDP
  for(uint32_t i=0; i< dumbbell.LeftCount(); i++) {
    /*
    if(i==1) {
      //Nodo con UDP
      AddressValue remoteAddressUDP(InetSocketAddress(dumbbell.GetRightIpv4Address(i), portUDP));
      onOffHelperUDP.SetAttribute("Remote", remoteAddressUDP);
      clientApps.Add(onOffHelperUDP.Install(dumbbell.GetLeft (i)));
      clientApps=sinkUDP.Install(dumbbell.GetRight(i));
    } else {
    */
      //Nodo con TCP
      AddressValue remoteAddressTCP (InetSocketAddress(dumbbell.GetRightIpv4Address(i), portTCP));
      onOffHelperTCP.SetAttribute("Remote", remoteAddressTCP);
      clientApps.Add(onOffHelperTCP.Install(dumbbell.GetLeft(i)));
      clientApps=sinkTCP.Install(dumbbell.GetRight(i));
    /*
    }
    */
  }

  //Start after sink y stop before sink
  clientApps.Start(Seconds(0.0));
  clientApps.Stop(Seconds(100.0));

  //Establece el cuadro delimitador para la animacion
  dumbbell.BoundingBox(1, 1, 100, 100);

  //Archivo XML para NetAnim
  AnimationInterface anim("TCP.xml");
 
  //Configura la simulacion real
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();
 
  //Stop simulador
  Simulator::Stop(Seconds(100));

  // crear archivos para analizar con wireshark
  pointToPointRouterCentral.EnablePcapAll("TCP"); //filename without .pcap extention
  
  AsciiTraceHelper asciiTraceHelper;
  // Para cada nodo en la topología dumbbell
  for (uint32_t i = 0; i < dumbbell.LeftCount() + dumbbell.RightCount() + 2; i++) {
  // Crea un nuevo stream para cada nodo
  	Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream("TCPcwnd_nodo" + std::to_string(i) + ".txt");
  	// Rastrea la ventana de congestión para el nodo i
  	Simulator::Schedule(Seconds(0.00001), &TraceCwnd, i, 0, MakeBoundCallback (&CwndChange, stream));
  }

  //Necesario para ver las estadisticas como paquetes perdidos
  // Flow monitor
  //Ptr<FlowMonitor> flowMonitor;
  //FlowMonitorHelper flowHelper;
  //flowMonitor = flowHelper.InstallAll();

  //Run simulador
  Simulator::Run();

  //Genero el xml para las estadisticas
  //flowMonitor->SerializeToXmlFile("FlowMonitorDumbbellPunto2.xml", true, true);

  //Destroy simulador
  Simulator::Destroy();
  return 0;
}
