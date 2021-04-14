/*
 * Copyright (C) 2017-2019 Dominique Dhoutaut, Thierry Arrabal, Eugen Dedu.
 *
 * This file is part of BitSimulator.
 *
 * BitSimulator is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * BitSimulator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with BitSimulator.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <cassert>
#include "scheduler.h"
#include "world.h"
#include "hcd-routing-agent.h"

//==============================================================================
//
//          HCDRoutingAgent  (class)
//
//==============================================================================

ofstream HCDRoutingAgent::HCDPositionsFile;
mt19937_64 *HCDRoutingAgent::forwardingRNG =  new mt19937_64(1);
int HCDRoutingAgent::currentAnchorID = 0;
int HCDRoutingAgent::forwardedHCDBeacons = 0;


using HCDInitialisationGenerationEvent = CallMethodEvent<HCDRoutingAgent,
  &HCDRoutingAgent::processPacketGenerationEvent,
  EventType::HCD_INITIALISATION_PACKET_GENERATION>;


HCDRoutingAgent::HCDRoutingAgent(Node *_hostNode) : RoutingAgent(_hostNode) {
  //cout << "HCDRoutingAgent constructor on Node " << hostNode->getId() << endl;
  type = RoutingAgentType::HCD;

  HCDCoordX = -1;
  HCDCoordY = -1;
  HCDCoordZ = -1;

  anchorID = -1;
  initialisationStarted = false;

  vector<NodeInfo> vect = ScenarioParameters::getVectNodeInfo();
  for ( auto it = vect.begin(); it != vect.end(); it++ ) {
    if ( it->isAnchor && it->id == hostNode->getId() ) {
      anchorID = currentAnchorID;
      currentAnchorID++;
    }
  }
  if ( anchorID == 0 ){
    simulationTime_t t = Scheduler::now()+200000000000;
    initialisationStarted = true;
    Scheduler::getScheduler().schedule(new HCDInitialisationGenerationEvent(t, this));
  }
}

HCDRoutingAgent::~HCDRoutingAgent() {
  if ( HCDPositionsFile.is_open() ) {
    HCDPositionsFile << hostNode->getId() << " " << HCDCoordX << " " << HCDCoordY << " " << HCDCoordZ << endl;
  }

  //     if (SLRCoordX == -1 || SLRCoordY == -1 || SLRCoordZ == -1 ){
  //      cout << " Destructor on node " << hostNode->getId() << " HCD coord X : " << HCDCoordX << " Y : " << HCDCoordY << " Z : " << HCDCoordZ << endl;
  //     }

  aliveAgents--;
  if ( aliveAgents == 0 ) {
    //      TODO : add global information
    cout << " HCD Routing:: forwarded packets : " << forwardedDataPackets << endl;
    cout << " HCD Routing:: forwarded beacons : " <<  forwardedHCDBeacons << endl;
  }
  //    if ( SLRCoordX == -1 || SLRCoordY == -1  || SLRCoordZ == -1 ){
  //// if ( hostNode->getId() < 30  ){
  //        cout << "HCDRoutingAgent destructor on Node " << hostNode->getId() ;
  //        cout << " HCD coord " << HCDCoordX << " " << HCDCoordY << " " << HCDCoordZ ;
  //        cout << endl;
  //    }
}

void HCDRoutingAgent::initializeAgent() {
  string filename;
  string extension;

  tinyxml2::XMLElement *XMLRootNode = ScenarioParameters::getXMLRootNode();

  tinyxml2::XMLElement *routingAgentConfigElement = XMLRootNode->FirstChildElement("routingAgentsConfig");
  assert (routingAgentConfigElement != nullptr);

  tinyxml2::XMLElement *HCDRoutingElement = routingAgentConfigElement->FirstChildElement("HCDRouting");
  assert (HCDRoutingElement != nullptr);
  ScenarioParameters::queryStringAttr(HCDRoutingElement, "HCDPositionFileExtension", extension, false, nullptr, ".HCDpos", "");

  filename = ScenarioParameters::getScenarioDirectory() + "/" + ScenarioParameters::getOutputBaseName() + "-" + "HCDPositions" + extension;
  HCDPositionsFile.open(filename);
  if (!HCDPositionsFile) {
    cerr << "*** ERROR *** While opening HCD positions file " << filename << endl;
    exit(EXIT_FAILURE);
  }
}

void HCDRoutingAgent::receivePacketFromNetwork(PacketPtr _packet) {

  LogSystem::EventsLogOutput.log( LogSystem::routingRCV, Scheduler::now(),hostNode->getId(),(int) _packet->type,_packet->flowId,_packet->flowSequenceNumber, _packet->modifiedBitsPositions.size());

  cout << Scheduler::getScheduler().now() << " HCDRoutingAgent::receivePacket on Node " << hostNode->getId();
  cout << " srcId:" << _packet->srcId << " srcSeq:" << _packet->srcSequenceNumber;
  cout << " dstId:" << _packet->dstId << " port:" << _packet->port ;
  cout << " size:" << _packet->size;
  cout << " flowId:" << _packet->flowId << " flowSeq:" << _packet->flowSequenceNumber;
  cout << endl;
  cout << "src coords: (" << _packet->src_SLRX << ", " << _packet->src_SLRY << ", " << _packet->src_SLRZ << ")";
  cout << endl;
  cout << "dst coords: (" << _packet->dst_SLRX << ", " << _packet->dst_SLRY << ", " << _packet->dst_SLRZ << ")";
  cout << endl;
  cout << "tx coords: (" << _packet->tx_SLRX << ", " << _packet->tx_SLRY << ", " << _packet->tx_SLRZ << ")";


  //                   cout << " Forwarding packet type " << (int)_packet->type << " at " << Scheduler::now() <<  endl;
  //                   if ((int)_packet->type == 4) getchar();

  bool alreadySeen = false;
  bool update = false;
  bool forward = false;
  auto it =alreadyForwardedPackets.find(_packet->flowId);
  if ( it != alreadyForwardedPackets.end() ){ // Flow already followed
    if ( it->second >= _packet->flowSequenceNumber ){ // The last seen packet from is flow is newer than the received one
      alreadySeen = true;
    }
  }

  //                 if (_packet->needAck && isAck(_packet)){

  if (  _packet->type == PacketType::D1_DENSITY_INIT && !alreadySeen ) {
    hostNode->dispatchPacketToApplication(_packet);
    //PacketPtr newPacket(new Packet(_packet));
    PacketPtr newPacket(_packet->clone());
    alreadyForwardedPackets.insert(pair<int,int>(_packet->flowId,_packet->flowSequenceNumber));
    hostNode->enqueueOutgoingPacket(newPacket);
    LogSystem::EventsLogOutput.log( LogSystem::routingSND, Scheduler::now(),hostNode->getId(),(int) newPacket->type);

    return;
  }
  if ( _packet->type == PacketType::D1_DENSITY_PROBE) {
    hostNode->dispatchPacketToApplication(_packet);
    return;
  }

  //
  if (_packet->type == PacketType::HCD_BEACON ){

    update = HCDUpdate(_packet);
    if (update) forward = true;
    if ( anchorID != -1   &&  _packet->anchorID != anchorID && !initialisationStarted ){

      initialisationStarted = true;
      simulationTime_t t = Scheduler::now()+100000000*anchorID;
      Scheduler::getScheduler().schedule(new HCDInitialisationGenerationEvent(t,this));
    }
  }

  // We are the destination, so we forward the incoming packet to the application layer
  if (_packet->type == PacketType::DATA && HCDDestinationReach(_packet)) {
    hostNode->dispatchPacketToApplication(_packet);
//         return;
  }

  if ( (!alreadySeen && _packet->type == PacketType::DATA) ){
    //                     cout << "dst" << "[" << _packet->dst_SLRX << "," << _packet->dst_SLRY << "," << _packet->dst_SLRZ << "] ";
    //                     cout << "fwd" << "[" << _packet->tx_SLRX << "," << _packet->tx_SLRY << "," << _packet->tx_SLRZ << "] ";
    //                     cout << "src" << "[" << _packet->src_SLRX << "," << _packet->src_SLRY << "," << _packet->src_SLRZ << "] ";
    //                     cout << "me " << "[" << SLRCoordX << "," << SLRCoordY << "," << SLRCoordZ << "] ";
    //                     cout << endl;
    //                     getchar();
    if ( HCDForward(_packet) && isCloser(_packet) ){
      forward = true;
      auto it =alreadyForwardedPackets.find(_packet->flowId);
      if (it != alreadyForwardedPackets.end()){
        it->second = _packet->flowSequenceNumber ;
      } else {
        alreadyForwardedPackets.insert(pair<int,int>(_packet->flowId,_packet->flowSequenceNumber));
      }
      //                      cout << " forwarding packet " <<  _packet->flowSequenceNumber << " on node " << hostNode->getId() << endl;

    }
  }

  // We want to forward the packet with our current hc
  if ( forward ) {
    uniform_real_distribution<float> distrib(0, 1);
    //                   float forwardingProbabilityNumber = distrib(*forwardingRNG);
    //
    //                   int wantedForwarder = 40;
    //                   if ( _packet->type == PacketType::DATA) wantedForwarder = 15;

    //                   float forwadingProbability;
    //                   if ( hostNode->getEstimatedNeibhbours() == -1 ){
    //                     forwadingProbability = 1;
    //                   }
    //                   else {
    //                     forwadingProbability = ((float)wantedForwarder)/(float)hostNode->getEstimatedNeibhbours();
    //                   }

    bool retransmit = true;
    //                   bool retransmit = forwardingProbabilityNumber < forwadingProbability ;
    if ( retransmit ) {
      PacketPtr newPacket(_packet->clone());
      newPacket->tx_SLRX = HCDCoordX;
      newPacket->tx_SLRY = HCDCoordY;
      newPacket->tx_SLRZ = HCDCoordZ;
      if ( update ) {
        newPacket->anchorDist++;
      }
      hostNode->enqueueOutgoingPacket(newPacket);
      LogSystem::EventsLogOutput.log( LogSystem::routingSND, Scheduler::now(),hostNode->getId(),(int) newPacket->type);

      if (newPacket->type == PacketType::DATA) {
        forwardedDataPackets++;
      }
      else if (newPacket->type == PacketType::HCD_BEACON){
        forwardedHCDBeacons++;
      }
    }
  }
}

// TODO: Delta functions ignore their Z parameter. SLR May not work in 3D !
bool HCDRoutingAgent::isCloser(PacketPtr _packet){

  int dstX,dstY,dstZ;
  int srcX,srcY,srcZ;
  int txX,txY,txZ;
  int X,Y,Z;

  bool res = false;

  dstX = _packet->dst_SLRX;
  dstY = _packet->dst_SLRY;
  dstZ = _packet->dst_SLRZ;

  srcX = _packet->src_SLRX;
  srcY = _packet->src_SLRY;
  srcZ = _packet->src_SLRZ;

  txX = _packet->tx_SLRX;
  txY = _packet->tx_SLRY;
  txZ = _packet->tx_SLRZ;

  X = HCDCoordX;
  Y = HCDCoordY;
  Z = HCDCoordZ;

  if ( dstX == -1 && dstY == -1 && dstZ == -1){
    int dist,disttx;
    dist = abs(X - srcX) + abs(Y - srcY) + abs(Z - srcZ);
    disttx = abs(txX - srcX) + abs(txY - srcY) + abs(txZ - srcZ) ;

    if (dist > disttx){
      res = true;
    }

    if (!res){
      printf(" srcX %d srcY %d srcZ %d \n", srcX, srcY, srcZ);
      printf("  txX %d  txY %d  txZ %d \n", txX, txY, txZ);
      printf("  X   %d  Y   %d  Z   %d \n", X, Y, Z);
      printf("dist    %d \n",dist);
      printf("disttx  %d \n",disttx);
      printf("%d \n",res);
      getchar();
    }
//       res = true;
    return res;

  }

  if ( (X == dstX) && (Y=dstY) && (Z==dstZ) ){
    res = true;
  }

  if ( (srcX < dstX) && (X > txX) ) {
    res = true;
  }
  if ( (srcX > dstX) && (X < txX) ) {
    res = true;
  }

  if ( (srcY < dstY) && (Y > txY) ) {
    res = true;
  }
  if ( (srcY > dstY) && (Y < txY) ) {
    res = true;
  }

  if ( (srcZ < dstZ) && (Z > txZ) ) {
    res = true;
  }
  if ( (srcZ > dstZ) && (Z < txZ) ) {
    res = true;
  }

  if (res) {
    cout << " isCloser on node " << hostNode->getId() << " res = " << res << endl;
  }

  printf(" dstX %d dstY %d dstZ %d \n", dstX, dstY, dstZ);
  printf(" srcX %d srcY %d srcZ %d \n", srcX, srcY, srcZ);
  printf("  txX %d  txY %d  txZ %d \n", txX, txY, txZ);
  printf("  X   %d  Y   %d  Z   %d \n", X, Y, Z);

  getchar();


  return res;
}

bool HCDRoutingAgent::HCDForward(PacketPtr _packet, int m )
{

  int dstX,dstY,dstZ;
  int srcX,srcY,srcZ;
  int X,Y,Z;

  dstX = _packet->dst_SLRX;
  dstY = _packet->dst_SLRY;
  dstZ = _packet->dst_SLRZ;

  srcX = _packet->src_SLRX;
  srcY = _packet->src_SLRY;
  srcZ = _packet->src_SLRZ;

  X = HCDCoordX;
  Y = HCDCoordY;
  Z = HCDCoordZ;

  if (dstX == -1 && dstY == -1 && dstZ == -1) {
    return true;
  }


  int deltaA = DeltaA(dstX,dstY,dstZ,srcX,srcY,srcZ, X,Y,Z);
  int deltaB = DeltaB(dstX,dstY,dstZ,srcX,srcY,srcZ, X,Y,Z);

  int deltaAX = DeltaA(dstX,dstY,dstZ,srcX,srcY,srcZ, X-m,Y,Z);
  int deltaAY = DeltaA(dstX,dstY,dstZ,srcX,srcY,srcZ, X,Y-m,Z);
  int deltaAZ = DeltaA(dstX,dstY,dstZ,srcX,srcY,srcZ, X-m,Y-m,Z);

  int deltaBX = DeltaB(dstX,dstY,dstZ,srcX,srcY,srcZ, X-m,Y,Z);
  int deltaBY = DeltaB(dstX,dstY,dstZ,srcX,srcY,srcZ, X,Y,Z-m);
  int deltaBZ = DeltaB(dstX,dstY,dstZ,srcX,srcY,srcZ, X-m,Y,Z-m);

  //Node is on the line between src and dst
  bool onTheLine= (
    (deltaA*deltaAX <= 0 || deltaA*deltaAY <= 0 || deltaA*deltaAZ <= 0)
    &&  (deltaB*deltaBX <= 0 || deltaB*deltaBY <= 0 || deltaB*deltaBZ <= 0) );
  bool onSegment = ( ((X-dstX)*(X-srcX) <= 0)  && ((Y-dstY)*(Y-srcY) <= 0) && ((Z-dstZ)*(Z-srcZ) <= 0) );

  //    cout << " Routin on node " << hostNode->getId() << " SLR : " << onTheLine << " " << onSegment << endl;
  /*
    printf(" dstX %d dstY %d dstZ %d \n", dstX,dstY,dstZ);
    printf(" srcX %d srcY %d srcZ %d \n", srcX,srcY,srcZ);
    printf("  txX %d  txY %d  txZ %d \n", txX,txY,txZ);
    printf("    X %d    Y %d Z    %d \n", X,Y,Z);
    getchar();
  */
  return onTheLine && onSegment;

}


bool HCDRoutingAgent::HCDForward(PacketPtr _packet)
{
  //The default parameter is 1;
  return HCDForward(_packet,ScenarioParameters::getHcdPathWidth());
}


bool HCDRoutingAgent::HCDDestinationReach(PacketPtr _packet)
{

  if (_packet->dstId == -1 ){
//       cout << " DESTINATION REACHED on node " << hostNode->getId() << "(" << SLRCoordX << "," << SLRCoordY << "," << SLRCoordZ << ")" << " packet (flow) " << _packet->flowSequenceNumber << "(" << _packet->flowId << ")"  << endl;
    LogSystem::EventsLogOutput.log(LogSystem::hcdBroadcastReach, Scheduler::now(),hostNode->getId(),(int)_packet->type,_packet->flowId,_packet->flowSequenceNumber,HCDCoordX,HCDCoordY,HCDCoordZ);
  }
  //   if (hostNode->getId() == _packet->dstId) {
  //         cout << " DESTINATION REACHED on node " << hostNode->getId() << "(" << SLRCoordX << "," << SLRCoordY << "," << SLRCoordZ << ")" << " packet (flow) " << _packet->flowSequenceNumber << "(" << _packet->flowId << ")"  << endl;
  // //         getchar();
  //       return true;
  //   }

  if (_packet->dst_SLRX == HCDCoordX && _packet->dst_SLRY == HCDCoordY && _packet->dst_SLRZ == HCDCoordZ ) {
    cout << " DESTINATION REACHED on node " << hostNode->getId() << "(" << HCDCoordX << "," << HCDCoordY << "," << HCDCoordZ << ")" << " packet (flow) " << _packet->flowSequenceNumber << "(" << _packet->flowId << ")"  << endl;
    LogSystem::EventsLogOutput.log( LogSystem::dstReach, Scheduler::now(),hostNode->getId(),(int)_packet->type,_packet->flowId,_packet->flowSequenceNumber,_packet->type);

    //     getchar();
    return true;
  }

  return false;
}

void HCDRoutingAgent::receivePacketFromApplication(PacketPtr _packet) {
  _packet->setSrcSequenceNumber(hostNode->getNextSrcSequenceNumber());

  _packet->src_SLRX = HCDCoordX;
  _packet->src_SLRY = HCDCoordY;
  _packet->src_SLRZ = HCDCoordZ;

  _packet->tx_SLRX = HCDCoordX;
  _packet->tx_SLRY = HCDCoordY;
  _packet->tx_SLRZ = HCDCoordZ;

  //TODO : how to find values here?
  if (_packet->type == PacketType::DATA && _packet->dstId != -1 ){
    Node* dst = World::getNode(_packet->dstId);
    HCDRoutingAgent* dstAgent = dynamic_cast<HCDRoutingAgent*> (dst->getRoutingAgent());

    if ( dstAgent == nullptr ) {
      cerr << " Error in setting HCD destination to a packet. The targeted destination may not run HCD " << endl;

    }
    else {

      _packet->dst_SLRX = dstAgent->getSLRX();
      _packet->dst_SLRY = dstAgent->getSLRY();
      _packet->dst_SLRZ = dstAgent->getSLRZ();
    }
  }
  else if (_packet->type == PacketType::DATA  && _packet->dstId == -1 ){
    _packet->dst_SLRX = -1;
    _packet->dst_SLRY = -1;
    _packet->dst_SLRZ = -1;
  }
  hostNode->enqueueOutgoingPacket(_packet);
}

bool HCDRoutingAgent::HCDUpdate(PacketPtr _packet)
{
  int anchorId = _packet->anchorID;
  int anchorDist =  _packet->anchorDist;
  bool update=false;

  HCDRoutingAgent* routing = static_cast<HCDRoutingAgent*> (hostNode->getRoutingAgent());
  //   cout << " SLR update on node " << hostNode->getId() << " by anchor " << anchorId << endl;

  switch ( anchorId )
  {
  case 0:
    if (anchorDist < routing->getSLRX() || routing->getSLRX() == -1  ){
      routing->setSLRX(anchorDist);
      routing->setSLRZ(anchorDist);
      update=true;
    }
    break;
  case 1:
    if (anchorDist < routing->getSLRY() || routing->getSLRY() == -1  ){
      routing->setSLRY(anchorDist);
      update=true;
    }
    break;
  case 2:
    if (anchorDist < routing->getSLRZ() || routing->getSLRZ() == -1  ){
      routing->setSLRZ(anchorDist);
      update=true;
    }
    break;
  default:
    cout << " WARNING : receiving HCD_BEACON from unknown anchor. Anchor Id is : " << anchorId << endl;
  }

  return update;
}

void HCDRoutingAgent::processPacketGenerationEvent() {
  cout << " anchor " << anchorID << " on node " << hostNode->getId() << " starts sending "<< endl;
  // SLR beacons have a -1 flow id
  PacketPtr p(new Packet(PacketType::HCD_BEACON, 100, hostNode->getId(), -1, -1,-1, 0,anchorID,1));
  HCDUpdate(p);
  hostNode->getRoutingAgent()->receivePacketFromApplication(p);
}
