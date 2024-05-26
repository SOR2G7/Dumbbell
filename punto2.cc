#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-layout-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("dumbbell-topology");

int main (int argc, char *argv[])
{
  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue(50));

  //https://www.nsnam.org/docs/release/3.16/doxygen/classns3_1_1_point_to_point_dumbbell_helper.html#a0ef17ad7a35cf814ba949ea91ab38279

  uint32_t left = 3; // nodos izquierdos
  uint32_t right = 3; // nodos derechos

  //p2p izquierdo
  PointToPointHelper p2pLeft;
  p2pLeft.SetDeviceAttribute("DataRate", StringValue ("100Kbps"));
  p2pLeft.SetChannelAttribute("Delay", StringValue ("10ms"));

  //p2p derecho
  PointToPointHelper p2pRight;
  p2pRight.SetDeviceAttribute("DataRate", StringValue ("100Kbps"));
  p2pRight.SetChannelAttribute("Delay", StringValue ("10ms"));

  //p2p router
  PointToPointHelper p2pRouter;
  p2pRouter.SetDeviceAttribute("DataRate", StringValue ("200Kbps"));
  p2pRouter.SetChannelAttribute("Delay", StringValue ("10ms"));
 
  //Dumbbell topology con Helper
  PointToPointDumbbellHelper dumbbell(left, p2pLeft, right, p2pRight, p2pRouter);

  //Instalo el stack
  InternetStackHelper stack;
  dumbbell.InstallStack(stack);
 
  //Asigno direcciones IP
  dumbbell.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"), //nodos izquierdos
                                Ipv4AddressHelper ("10.2.1.0", "255.255.255.0"), //nodos derechos
                                Ipv4AddressHelper ("10.3.1.0", "255.255.255.0")); //routers
  
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

  //Ciclo los nodos de la izquierda y defino cual es TCP y cual es UDP
  for(uint32_t i=0; i< dumbbell.LeftCount(); i++) {
    if(i==1) {
      // Nodo con UDP (apagado)
      AddressValue remoteAddressUDP(InetSocketAddress(dumbbell.GetRightIpv4Address(i), portUDP));
      onOffHelperUDP.SetAttribute("Remote", remoteAddressUDP);
      // No se inicia la aplicación UDP
    } else {
      //Nodo con TCP
      AddressValue remoteAddressTCP (InetSocketAddress(dumbbell.GetRightIpv4Address(i), portTCP));
      onOffHelperTCP.SetAttribute("Remote", remoteAddressTCP);
      clientApps.Add(onOffHelperTCP.Install(dumbbell.GetLeft(i)));
      clientApps=sinkTCP.Install(dumbbell.GetRight(i));
    }
  }
 
  //Start after sink y stop before sink
  clientApps.Start(Seconds(0.0));
  clientApps.Stop(Seconds(10.0));

  //Establece el cuadro delimitador para la animacion
  dumbbell.BoundingBox(1, 1, 100, 100);

  //Archivo XML para NetAnim
  AnimationInterface anim("animation.xml");
 
  //Rellena el ruteo
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();
  Simulator::Stop(Seconds(10));
  
  // .pcap
  p2pRouter.EnablePcapAll("1router");

  Simulator::Run();
  Simulator::Destroy();
  return 0;
}