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


#include <iostream>
#include <cassert>
#include "slr-routing-agent.h"
#include "scheduler.h"
#include "utils.h"
#include "node.h"
#include "world.h"
#include "events.h"

using namespace std;


using SLRInitialisationGenerationEvent = CallMethodEvent<SLRRoutingAgent,
  &SLRRoutingAgent::processPacketGenerationEvent,
  EventType::SLR_INITIALISATION_PACKET_GENERATION>;



//==============================================================================
//
//          SLRRoutingAgent  (class)
//
//==============================================================================

ofstream SLRRoutingAgent::SLRPositionsFile;
mt19937_64 *SLRRoutingAgent::forwardingRNG =  new mt19937_64(1);
int SLRRoutingAgent::currentAnchorID = 0;
int SLRRoutingAgent::forwardedSLRBeacons = 0;


SLRRoutingAgent::SLRRoutingAgent(Node *_hostNode) : RoutingAgent(_hostNode) {
  type = RoutingAgentType::SLR;

  SLRCoordX = -1;
  SLRCoordY = -1;
  SLRCoordZ = -1;

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
    Scheduler::getScheduler().schedule(new SLRInitialisationGenerationEvent(t,this));
  }
}

SLRRoutingAgent::~SLRRoutingAgent() {
  if ( SLRPositionsFile.is_open() ) {
    SLRPositionsFile << hostNode->getId() << " " << SLRCoordX << " " << SLRCoordY << " " << SLRCoordZ << endl;
  }

  //     if (SLRCoordX == -1 || SLRCoordY == -1 || SLRCoordZ == -1 ){
  //      cout << " Destructor on node " << hostNode->getId() << " SLR coord X : " << SLRCoordX << " Y : " << SLRCoordY << " Z : " << SLRCoordZ << endl;
  //     }

  aliveAgents--;
  if ( aliveAgents == 0 ) {
    //      TODO : add global information
    cout << " SLR Routing:: forwarded packets: " << forwardedDataPackets << endl;
    cout << " SLR Routing:: forwarded beacons: " <<  forwardedSLRBeacons << endl;
  }
  //    if ( SLRCoordX == -1 || SLRCoordY == -1  || SLRCoordZ == -1 ){
  //// if ( hostNode->getId() < 30  ){
  //        cout << "SLRRoutingAgent destructor on Node " << hostNode->getId() ;
  //        cout << " SLR coord " << SLRCoordX << " " << SLRCoordY << " " << SLRCoordZ ;
  //        cout << endl;
  //    }
}

void SLRRoutingAgent::initializeAgent() {
  string filename;
  string extension = ScenarioParameters::getDefaultExtension();
  string separator = "";
  if ( ScenarioParameters::getOutputBaseName().length() > 0 ) {
    separator = "-";
  }
  filename = ScenarioParameters::getScenarioDirectory() + "/" + ScenarioParameters::getOutputBaseName() + separator + "SLRPositions" + extension;
  SLRPositionsFile.open(filename);
  if (!SLRPositionsFile) {
    cerr << "*** ERROR *** While opening SLR positions file " << filename << endl;
    exit(EXIT_FAILURE);
  }
}



void SLRRoutingAgent::receivePacketFromNetwork(PacketPtr _packet) {
  LogSystem::EventsLogOutput.log( LogSystem::routingRCV, Scheduler::now(),hostNode->getId(),(int) _packet->type,_packet->flowId,_packet->flowSequenceNumber, _packet->modifiedBitsPositions.size());

  //          cout << Scheduler::getScheduler().now() << " SLRRoutingAgent::receivePacket on Node " << hostNode->getId();
  //         cout << " srcId:" << _packet->srcId << " srcSeq:" << _packet->srcSequenceNumber;
  //         cout << " dstId:" << _packet->dstId << " port:" << _packet->port ;
  //         cout << " size:" << _packet->size;
  //         cout << " flowId:" << _packet->flowId << " flowSeq:" << _packet->flowSequenceNumber;
  //            cout << endl;

  //                   cout << " Forwarding packet type " << (int)_packet->type << " at " << Scheduler::now() <<  endl;
  //                   if ((int)_packet->type == 4) getchar();

  bool alreadySeen = false;
  bool update = false;
  bool forward = false;
  auto it =alreadyForwardedPackets.find(_packet->flowId);
  if (it != alreadyForwardedPackets.end() ){ // Flow already followed
    if ( it->second >= _packet->flowSequenceNumber){ // The last seen packet from is flow is newer than the received one
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
    LogSystem::EventsLogOutput.log( LogSystem::routingSND, Scheduler::now(),hostNode->getId(),(int) newPacket->type,newPacket->flowId,newPacket->flowSequenceNumber);
    return;
  }
  if ( _packet->type == PacketType::D1_DENSITY_PROBE) {
    hostNode->dispatchPacketToApplication(_packet);
    return;
  }
  if (_packet->type == PacketType::SLR_BEACON ){
    update = SLRUpdate(_packet);
    if (update) forward = true;
    if ( anchorID != -1   &&  _packet->anchorID != anchorID && !initialisationStarted ){
      initialisationStarted = true;
      simulationTime_t t = Scheduler::now()+100000000*anchorID;
      Scheduler::getScheduler().schedule(new SLRInitialisationGenerationEvent(t,this));
    }
  }

  if (_packet->type == PacketType::DATA && SLRDestinationReach(_packet)) {
    hostNode->dispatchPacketToApplication(_packet);
//         return;
  }

  if ( (!alreadySeen && _packet->type == PacketType::DATA)){
    //                     cout << "dst" << "[" << _packet->dst_SLRX << "," << _packet->dst_SLRY << "," << _packet->dst_SLRZ << "] ";
    //                     cout << "fwd" << "[" << _packet->tx_SLRX << "," << _packet->tx_SLRY << "," << _packet->tx_SLRZ << "] ";
    //                     cout << "src" << "[" << _packet->src_SLRX << "," << _packet->src_SLRY << "," << _packet->src_SLRZ << "] ";
    //                     cout << "me " << "[" << SLRCoordX << "," << SLRCoordY << "," << SLRCoordZ << "] ";
    //                     cout << endl;
    //                     getchar();
    if ( SLRForward(_packet) && isCloser(_packet) ){
      forward = true;
      auto it =alreadyForwardedPackets.find(_packet->flowId);
      if (it != alreadyForwardedPackets.end()){
        it->second = _packet->flowSequenceNumber ;
      }
      else{
        alreadyForwardedPackets.insert(pair<int,int>(_packet->flowId,_packet->flowSequenceNumber));
      }
      //                      cout << " forwarding packet " <<  _packet->flowSequenceNumber << " on node " << hostNode->getId() << endl;
    }
  }

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
    if (retransmit) {
      PacketPtr newPacket(_packet->clone());
      newPacket->tx_SLRX = SLRCoordX;
      newPacket->tx_SLRY = SLRCoordY;
      newPacket->tx_SLRZ = SLRCoordZ;
      if ( update ) {newPacket->anchorDist++;}
      hostNode->enqueueOutgoingPacket(newPacket);
      LogSystem::EventsLogOutput.log( LogSystem::routingSND, Scheduler::now(),hostNode->getId(),(int) newPacket->type,newPacket->flowId,newPacket->flowSequenceNumber);

      if (newPacket->type == PacketType::DATA) {
        forwardedDataPackets++;
      }
      else if (newPacket->type == PacketType::SLR_BEACON){
        forwardedSLRBeacons++;
      }
    }
  }
}

// TODO: Delta functions ignore their Z parameter. SLR May not work in 3D !


bool SLRRoutingAgent::isCloser(PacketPtr _packet){
  bool res = false;

  int dstX,dstY,dstZ;
  int srcX,srcY,srcZ;
  int txX,txY,txZ;
  int X,Y,Z;

  dstX = _packet->dst_SLRX;
  dstY = _packet->dst_SLRY;
  if ( _packet->dst_SLRZ == -1 && _packet->dst_SLRX != -1 && _packet->dst_SLRY != -1 ){
    dstZ = _packet->dst_SLRX;
  }
  else {
    dstZ = _packet->dst_SLRZ;
  }

  srcX = _packet->src_SLRX;
  srcY = _packet->src_SLRY;
  if ( _packet->src_SLRZ == -1 && _packet->src_SLRX != -1 && _packet->src_SLRY != -1 ){
    srcZ = _packet->src_SLRX;
  }
  else {
    srcZ = _packet->src_SLRZ;
  }

  txX = _packet->tx_SLRX;
  txY = _packet->tx_SLRY;
  if ( _packet->tx_SLRZ == -1 && _packet->tx_SLRX != -1 && _packet->tx_SLRY != -1 ){
    txZ = _packet->tx_SLRX;
  }
  else {
    txZ = _packet->tx_SLRZ;
  }

  X = SLRCoordX;
  Y = SLRCoordY;

  if ( SLRCoordZ == -1 && SLRCoordX != -1 && SLRCoordY != -1 ){
    Z=X;
  }
  else {
    Z = SLRCoordZ;
  }

  if ( dstX == -1 && dstY == -1 && dstZ == -1){
    int dist,disttx;
    dist = abs(X - srcX) + abs(Y - srcY) ;
    disttx = abs(txX - srcX) + abs(txY - srcY) ;

    if (dist > disttx){
      res = true;
    }

//       if (!res){
//       printf(" srcX %d srcY %d srcZ %d \n", srcX,srcY,srcZ);
//       printf("  txX %d  txY %d  txZ %d \n", txX,txY,txZ);
//       printf("  X   %d  Y   %d  Z   %d \n", X,Y,Z);
//       printf("dist    %d \n",dist);
//       printf("disttx  %d \n",disttx);
//       printf("%d \n",res);
//       getchar();
//       }
//       res = true;
    return res;
  }

  if ( (X == dstX) && (Y=dstY) && (Z==dstZ) ){
    res = true;
    //     return true;
  }

  if ( (srcX <= dstX) && (X > txX) ) {
    res = true;
    //     return true;
  }
  if ( (srcX > dstX) && (X < txX) ) {
    res = true;
    //     return true;
  }

  if ( (srcY < dstY) && (Y > txY) ) {
    res = true;
    //     return true;
  }
  if ( (srcY > dstY) && (Y < txY) ) {
    res = true;
    //     return true;
  }

  if ( (srcZ < dstZ) && (Z > txZ) ) {
    res = true;
    //     return true;
  }
  if ( (srcZ > dstZ) && (Z < txZ) ) {
    res = true;
    //     return true;
  }

  //  if (res) cout << " isCloser on node " << hostNode->getId() << " res = " << res << endl;

  //   printf(" dstX %d dstY %d dstZ %d \n", dstX,dstY,dstZ);
  //   printf(" srcX %d srcY %d srcZ %d \n", srcX,srcY,srcZ);
  //   printf("  txX %d  txY %d  txZ %d \n", txX,txY,txZ);
  //   printf("  X   %d  Y   %d  Z   %d \n", X,Y,Z);
  //
  //   getchar();

  return res;
}


bool SLRRoutingAgent::SLRForward(PacketPtr _packet, int m )
{

  int dstX,dstY,dstZ;
  int srcX,srcY,srcZ;
  int X,Y,Z;

  dstX = _packet->dst_SLRX;
  dstY = _packet->dst_SLRY;
  if ( _packet->dst_SLRZ == -1 && _packet->dst_SLRX != -1 && _packet->dst_SLRY != -1 ){
    dstZ = _packet->dst_SLRX;
  }
  else {
    dstZ = _packet->dst_SLRZ;
  }

  srcX = _packet->src_SLRX;
  srcY = _packet->src_SLRY;

  if ( _packet->src_SLRZ == -1 && _packet->src_SLRX != -1 && _packet->src_SLRY != -1 ){
    srcZ = _packet->src_SLRX;
  }
  else {
    srcZ = _packet->src_SLRZ;
  }

  X = SLRCoordX;
  Y = SLRCoordY;

  if ( SLRCoordZ == -1 && SLRCoordX != -1 && SLRCoordY != -1 ){
    Z=X;
  }
  else {
    Z = SLRCoordZ;
  }

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
  bool onTheLine= ((deltaA*deltaAX <=0 || deltaA*deltaAY <= 0 || deltaA*deltaAZ <= 0) && (deltaB*deltaBX <=0 || deltaB*deltaBY <= 0 || deltaB*deltaBZ <= 0) );
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


bool SLRRoutingAgent::SLRForward(PacketPtr _packet)
{
  //The default parameter is 1;
  return SLRForward(_packet,ScenarioParameters::getSlrPathWidth());
}


bool SLRRoutingAgent::SLRDestinationReach(PacketPtr _packet)
{

  if (_packet->dstId == -1 ){
//       cout << " DESTINATION REACHED on node " << hostNode->getId() << "(" << SLRCoordX << "," << SLRCoordY << "," << SLRCoordZ << ")" << " packet (flow) " << _packet->flowSequenceNumber << "(" << _packet->flowId << ")"  << endl;
    LogSystem::EventsLogOutput.log(LogSystem::slrBroadcastReach, Scheduler::now(),hostNode->getId(),(int)_packet->type,_packet->flowId,_packet->flowSequenceNumber,SLRCoordX,SLRCoordY,SLRCoordZ);
  }
  //   if (hostNode->getId() == _packet->dstId) {
  //         cout << " DESTINATION REACHED on node " << hostNode->getId() << "(" << SLRCoordX << "," << SLRCoordY << "," << SLRCoordZ << ")" << " packet (flow) " << _packet->flowSequenceNumber << "(" << _packet->flowId << ")"  << endl;
  // //         getchar();
  //       return true;
  //   }

  if (_packet->dst_SLRX == SLRCoordX && _packet->dst_SLRY == SLRCoordY && _packet->dst_SLRZ == SLRCoordZ ) {
    cout << " DESTINATION REACHED on node " << hostNode->getId() << "(" << SLRCoordX << "," << SLRCoordY << "," << SLRCoordZ << ")" << " packet (flow) " << _packet->flowSequenceNumber << "(" << _packet->flowId << ")"  << endl;
    LogSystem::EventsLogOutput.log( LogSystem::dstReach, Scheduler::now(),hostNode->getId(),(int)_packet->type,_packet->flowId,_packet->flowSequenceNumber,_packet->type);

    //     getchar();
    return true;
  }
  return false;
}

void SLRRoutingAgent::receivePacketFromApplication(PacketPtr _packet) {
  _packet->setSrcSequenceNumber(hostNode->getNextSrcSequenceNumber());

  _packet->src_SLRX = SLRCoordX;
  _packet->src_SLRY = SLRCoordY;
  _packet->src_SLRZ = SLRCoordZ;

  _packet->tx_SLRX = SLRCoordX;
  _packet->tx_SLRY = SLRCoordY;
  _packet->tx_SLRZ = SLRCoordZ;

  //TODO : how to find values here?
  if (_packet->type == PacketType::DATA && _packet->dstId != -1 ){
    Node* dst = World::getNode(_packet->dstId);
    SLRRoutingAgent* dstAgent = dynamic_cast<SLRRoutingAgent*> (dst->getRoutingAgent());

    if ( dstAgent == nullptr ) {
      cerr << " Error in setting SLR destination to a packet. The targeted destination may not run SLR " << endl;

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

bool SLRRoutingAgent::SLRUpdate(PacketPtr _packet)
{
  int anchorId = _packet->anchorID;
  int anchorDist =  _packet->anchorDist;
  bool update=false;

  SLRRoutingAgent* routing = static_cast<SLRRoutingAgent*> (hostNode->getRoutingAgent());
  //   cout << " SLR update on node " << hostNode->getId() << " by anchor " << anchorId << endl;

  switch ( anchorId )
  {
  case 0:
    if (anchorDist < routing->getSLRX() || routing->getSLRX() == -1){
      routing->setSLRX(anchorDist);
//                 routing->setSLRZ(anchorDist);
      update=true;
    }
    break;
  case 1:
    if (anchorDist < routing->getSLRY() || routing->getSLRY() == -1){
      routing->setSLRY(anchorDist);
      update=true;
    }
    break;
  case 2:
    if (anchorDist < routing->getSLRZ() || routing->getSLRZ() == -1){
      routing->setSLRZ(anchorDist);
      update=true;
    }
    break;
  default:
    cerr << " *** ERROR *** SLR_BEACON received from unknown anchor " << anchorId << endl;
    exit (EXIT_FAILURE);
  }

  return update;
}

void SLRRoutingAgent::processPacketGenerationEvent() {
  cout << " anchor " << anchorID << " on node " << hostNode->getId() << " sent a beacon" << endl;
  // SLR beacons have a -1 flow id
  PacketPtr p(new Packet(PacketType::SLR_BEACON, 100, hostNode->getId(), -1, -1,-1, 0,anchorID,1));
  SLRUpdate(p);
  hostNode->getRoutingAgent()->receivePacketFromApplication(p);
}

