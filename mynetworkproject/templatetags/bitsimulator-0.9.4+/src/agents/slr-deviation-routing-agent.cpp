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
#include "slr-deviation-routing-agent.h"
#include "scheduler.h"
#include "utils.h"
#include "node.h"
#include "world.h"
#include "events.h"

using namespace std;



using DeviationInitialisationGenerationEvent = CallMethodEvent<
  DeviationRoutingAgent,
  &DeviationRoutingAgent::processPacketGenerationEvent,
  EventType::SLR_INITIALISATION_PACKET_GENERATION>;



//==============================================================================
//
//          DeviationRoutingAgent  (class)
//
//==============================================================================

ofstream DeviationRoutingAgent::SLRPositionsFile;
mt19937_64 *DeviationRoutingAgent::forwardingRNG =  new mt19937_64(0);
int DeviationRoutingAgent::currentAnchorID = 0;


DeviationRoutingAgent::DeviationRoutingAgent(Node *_hostNode) : RoutingAgent(_hostNode) {
  SLRCoordX = -1;
  SLRCoordY = -1;
  SLRCoordZ = -1;

  anchorID = -1;
  initialisationStarted = false;

  deviation = false;

  time_t effectiveBackoffWindow =2*((Node::getPulseDuration() * 1000* (999) + Node::getPulseDuration()) + ScenarioParameters::getCommunicationRange()/300 );
  if ( hostNode->getNeighboursCount() != -1 ) {
//               effectiveBackoffWindow*=hostNode->getEstimatedNeighbours();
    effectiveBackoffWindow*=hostNode->getNeighboursCount();
  }

  vector<NodeInfo> vect = ScenarioParameters::getVectNodeInfo();
  for ( auto it = vect.begin(); it != vect.end(); it++ ) {
    if ( it->isAnchor && it->id == hostNode->getId() ) {
      anchorID = currentAnchorID;
      currentAnchorID++;
    }
  }
  if ( anchorID == 0 ){
    simulationTime_t t = Scheduler::now()+400000000000;
    initialisationStarted = true;
    Scheduler::getScheduler().schedule(new DeviationInitialisationGenerationEvent(t,this));
  }

  redundancy = ScenarioParameters::getSlrBackoffredundancy();
  beaconRedundancy = ScenarioParameters::getSlrBeaconBackoffredundancy();

}

DeviationRoutingAgent::~DeviationRoutingAgent() {
  if ( SLRPositionsFile.is_open() ) {
    SLRPositionsFile << hostNode->getId() << " " << SLRCoordX << " " << SLRCoordY << " " << SLRCoordZ << endl;
  }
}

void DeviationRoutingAgent::initializeAgent() {
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

  forwardingRNG = new mt19937_64(ScenarioParameters::getBackoffFloodingRNGSeed());
}


void DeviationRoutingAgent::receivePacketFromNetwork(PacketPtr _packet) { // Same as SLR
  LogSystem::EventsLogOutput.log( LogSystem::routingRCV, Scheduler::now(),hostNode->getId(),(int) _packet->type,_packet->flowId,_packet->flowSequenceNumber, _packet->modifiedBitsPositions.size());

  bool alreadySeen = false;
  bool update = false;
  bool forward = false;
  auto it =alreadyForwardedPackets.find(_packet->flowId);
  if (it != alreadyForwardedPackets.end() ){ // Flow already followed
    if ( it->second >= _packet->flowSequenceNumber){ // The last seen packet from is flow is newer than the received one
      alreadySeen = true;
    }
  }

  int wantedRedundancy;
  if (_packet->type == PacketType::DATA) {
    wantedRedundancy = redundancy;
  }

  // bool dev = false;
  // if (_packet->deviation > 1 ) dev = true;

  if (_packet->type == PacketType::DATA){
    for ( auto it = backoffWaitingPacket.begin(); it!=backoffWaitingPacket.end();it++){
      PacketPtr testedPacket = it->second.p;
      int testedPacketCounter = it->second.counter;
      if ( (testedPacket->flowId == _packet->flowId) && (testedPacket->flowSequenceNumber == _packet->flowSequenceNumber) ){ // if the received packet is in waiting state
        if (testedPacketCounter >= wantedRedundancy ){
          if ( isAck(_packet)){
            backoffWaitingPacket.erase(it);
          }
          else { // Packet is not an implicitAck
          }
        }
        else {
          it->second.counter++;
        }
        return;
      }
    }
  }

  if (  _packet->type == PacketType::D1_DENSITY_INIT && !alreadySeen ) {
    hostNode->dispatchPacketToApplication(_packet);
    PacketPtr newPacket(_packet->clone());
    alreadyForwardedPackets.insert(pair<int,int>(_packet->flowId,_packet->flowSequenceNumber));
    hostNode->enqueueOutgoingPacket(newPacket);
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
      Scheduler::getScheduler().schedule(new DeviationInitialisationGenerationEvent(t,this));
    }
  }

  if (_packet->type == PacketType::DATA && SLRDestinationReach(_packet)) {
    hostNode->dispatchPacketToApplication(_packet);
    LogSystem::EventsLogOutput.log( LogSystem::dstReach, Scheduler::now(),hostNode->getId(),(int)_packet->type,_packet->flowId,_packet->flowSequenceNumber);
    return;
  }

  if ( (!alreadySeen && _packet->type == PacketType::DATA)){
    if ( deviationForward(_packet) && isCloser(_packet) ){
      forward = true;
      auto it =alreadyForwardedPackets.find(_packet->flowId);
      if (it != alreadyForwardedPackets.end()){
        it->second = _packet->flowSequenceNumber ;
      }
      else{
        alreadyForwardedPackets.insert(pair<int,int>(_packet->flowId,_packet->flowSequenceNumber));
      }
    }
  }

  if ( forward ) {
    PacketPtr newPacket(_packet->clone());
    newPacket->tx_SLRX = SLRCoordX;
    newPacket->tx_SLRY = SLRCoordY;
    newPacket->tx_SLRZ = SLRCoordZ;

    if ( update ) {
      newPacket->anchorDist++;
      hostNode->enqueueOutgoingPacket(newPacket);
      return;
    }

    time_t effectiveBackoffWindow =2*((Node::getPulseDuration() * _packet->beta * (_packet->size-1) + Node::getPulseDuration()) + ScenarioParameters::getCommunicationRange()/300 );
    if ( hostNode->getEstimatedNeighbours() != -1 ) {
      effectiveBackoffWindow*=hostNode->getEstimatedNeighbours();
    }

    uniform_int_distribution<time_t> distrib(0, effectiveBackoffWindow);
    time_t backoffTime = distrib(*forwardingRNG);

    int mcr = ScenarioParameters::getMaxConcurrentReceptions();
    int receptCount = hostNode->getConcurrentReceptionCount();
    float congestionLevel = (float)receptCount / mcr;
    int deviateThresh = 50;
    int convergeThresh = 50;
    if (100 * congestionLevel > deviateThresh) {
      newPacket->deviation++;
      // cout << "deviating with " << congestionLevel << " to thresh " << deviateThresh << ". new level: " << packet->deviation << "\n";
    }
    else if (100 * congestionLevel < convergeThresh && newPacket->deviation > 1) {
      newPacket->deviation--;
      newPacket->src_SLRX = SLRCoordX;
      newPacket->src_SLRY = SLRCoordY;
      newPacket->src_SLRZ = SLRCoordZ;
    }

    struct SLRbackoffRoutingCounter info;
    info.p = newPacket;
    info.counter = 1;
    backoffWaitingPacket.insert(pair<int,SLRbackoffRoutingCounter>(newPacket->packetId,info));

    Scheduler::getScheduler().schedule(new SLRbackoffSendingEvent2(Scheduler::now() + backoffTime, hostNode,newPacket->packetId));
//             deviation = true;
  }
}


bool DeviationRoutingAgent::isCloser(PacketPtr _packet){

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

  X = SLRCoordX;
  Y = SLRCoordY;
  Z = SLRCoordZ;

  //   if ( X == srcX && Y == srcY && Z == srcZ ){
  //    res = true;
  // //     return true;
  //   }

//     if ( (X == dstX) && (Y=dstY) && (Z==dstZ) ){
//         res = true;
//         //     return true;
//     }

  if ( (srcX < dstX) && (X > txX) ) {
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

  return res;
}


bool DeviationRoutingAgent::deviationForward(PacketPtr _packet)
{
//   if (_packet->deviation == 1){
//       return SLRForward(_packet,1);
//   }
//   else { //deviation > 1
//
//     return (SLRForward(_packet,_packet->deviation) && !SLRForward(_packet,(_packet->deviation)-1));
//
//   }
  return SLRForward(_packet,_packet->deviation);
}


bool DeviationRoutingAgent::SLRForward(PacketPtr _packet, int m )
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

  X = SLRCoordX;
  Y = SLRCoordY;
  Z = SLRCoordZ;


  //TODO : test here if coordinates are -1 ?

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

  return onTheLine && onSegment;
}


bool DeviationRoutingAgent::SLRForward(PacketPtr _packet)
{
  return SLRForward(_packet,_packet->deviation);
}


bool DeviationRoutingAgent::SLRDestinationReach(PacketPtr _packet)
{
  //   TODO : DO we want a zone reception or a node reception ?
//     if (hostNode->getId() == _packet->dstId) {
//         if ( _packet->flowId == 7 ) {
//           cout << " DESTINATION REACHED on node " << hostNode->getId() << "(" << SLRCoordX << "," << SLRCoordY << "," << SLRCoordZ << ")" << " packet (flow) " << _packet->flowSequenceNumber << "(" << _packet->flowId << ")"  << endl;
//         }
//         return true;
//     }

  if (_packet->dst_SLRX == SLRCoordX && _packet->dst_SLRY == SLRCoordY && _packet->dst_SLRZ == SLRCoordZ ) {
    cout << " DESTINATION REACHED on node " << hostNode->getId() << "(" << SLRCoordX << "," << SLRCoordY << "," << SLRCoordZ << ")" << " packet (flow) " << _packet->flowSequenceNumber << "(" << _packet->flowId << ")"  << endl;
    return true;
  }
  return false;
}

void DeviationRoutingAgent::receivePacketFromApplication(PacketPtr _packet) {
  _packet->setSrcSequenceNumber(hostNode->getNextSrcSequenceNumber());

  _packet->src_SLRX = SLRCoordX;
  _packet->src_SLRY = SLRCoordY;
  _packet->src_SLRZ = SLRCoordZ;

  _packet->tx_SLRX = SLRCoordX;
  _packet->tx_SLRY = SLRCoordY;
  _packet->tx_SLRZ = SLRCoordZ;

  if (_packet->type == PacketType::DATA && _packet->dstId != -1 ){
    Node* dst = World::getNode(_packet->dstId);
    DeviationRoutingAgent* dstAgent = dynamic_cast<DeviationRoutingAgent*> (dst->getRoutingAgent());

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

// // //     if (_packet->needAck ){
// // //         waintingPacket[_packet->flowId]=_packet->flowSequenceNumber;
// // //         simulationTime_t t = Scheduler::now()+ACK_TIMEOUT;
// // //         Scheduler::getScheduler().schedule(new AckTimedOutEvent(t,hostNode,_packet));
// // //     }

  hostNode->enqueueOutgoingPacket(_packet);
}

bool DeviationRoutingAgent::SLRUpdate(PacketPtr _packet)
{
  int anchorId = _packet->anchorID;
  int anchorDist =  _packet->anchorDist;
  bool update=false;

  //   cout << " SLR update on node " << hostNode->getId() << " by anchor " << anchorId << endl;

  switch ( anchorId )
  {
  case 0:
    if (anchorDist < getSLRX() || getSLRX() == -1  ){
      setSLRX(anchorDist);
      setSLRZ(anchorDist);
      update=true;
    }
    break;
  case 1:
    if (anchorDist < getSLRY() || getSLRY() == -1  ){
      setSLRY(anchorDist);
      update=true;
    }
    break;
    //          case 2:
    //               if (anchorDist < routing->getSLRZ() || routing->getSLRZ() == -1  ){
    //                 routing->setSLRZ(anchorDist);
    //                 update=true;
    //               }
    //          break;
  default:
    cout << " WARNING : receiving SLR_BEACON from unknown anchor " << endl;
  }

  return update;
}

void DeviationRoutingAgent::processPacketGenerationEvent() {
  cout << " anchor " << anchorID << " on node " << hostNode->getId() << " sent a beacon" << endl;

  PacketPtr p(new Packet(PacketType::SLR_BEACON, 100, hostNode->getId(), -1, -1,-1, 0,anchorID,1));
  SLRUpdate(p);
  hostNode->getRoutingAgent()->receivePacketFromApplication(p);
}

bool DeviationRoutingAgent::isAck(PacketPtr _packet) // The packet is coming from "further"
{
  int X = SLRCoordX;
  int Y = SLRCoordY;
  int Z = SLRCoordZ;

  int txX = _packet->tx_SLRX;
  int txY = _packet->tx_SLRY;
  int txZ = _packet->tx_SLRZ;

  int srcX = _packet->src_SLRX;
  int srcY = _packet->src_SLRY;
  int srcZ = _packet->src_SLRZ;

  int dstX = _packet->dst_SLRX;
  int dstY = _packet->dst_SLRY;
  int dstZ = _packet->dst_SLRZ;

//     if (SLRForward(_packet, _packet->deviation)){

  if ( (X == dstX) && (Y == dstY) && (Z == dstZ) ){
    return true;
  }

  if ( (srcX < dstX) && (X < txX) ) {
    return true;
  }
  if ( (srcX > dstX) && (X > txX) ) {
    return true;
  }

  if ( (srcY < dstY) && (Y < txY) ) {
    return true;
  }
  if ( (srcY > dstY) && (Y > txY) ) {
    return true;
  }

  if ( (srcZ < dstZ) && (Z < txZ) ) {
    return true;
  }
  if ( (srcZ > dstZ) && (Z > txZ) ) {
    return true;
  }


//     }

  return false;
}

void DeviationRoutingAgent::processSLRBackoffSending(int packetId){
  auto it = backoffWaitingPacket.find(packetId);
  if ( it != backoffWaitingPacket.end() ) {
    //
    // if the packet is still in the waiting list
    // i.e. I have not see more than REDUNDANCY copies
    // then I send it now
    PacketPtr p = it->second.p;
    hostNode->enqueueOutgoingPacket(p);
    LogSystem::EventsLogOutput.log( LogSystem::routingSND, Scheduler::now(),hostNode->getId(),(int) it->second.p->type,it->second.p->flowId,it->second.p->flowSequenceNumber);
    backoffWaitingPacket.erase(it);
  }
}



//===========================================================================================================
//
//          DeviationRoutingAgent  (class)
//
//===========================================================================================================
/*
  #define DEVIATION_VALUE 4

  DeviationRoutingAgent::DeviationRoutingAgent(Node *_hostNode) : SLRRoutingAgent(_hostNode) {

  }


  bool DeviationRoutingAgent::SLRForward(PacketPtr _packet)
  {

  if ( needDeviation(_packet) ) {
  deviation(_packet);
  }

  if (_packet->deviation == 0 ) {
  return SLRRoutingAgent::SLRForward(_packet);
  }

  else if (_packet->deviation > 0 ) {
  if ( _packet->deviation < DEVIATION_VALUE) {
  _packet->deviation ++;
  cout << " packet " << _packet->flowSequenceNumber << " devitation increased to " << _packet->deviation << " on node " << hostNode->getId() << endl;
  }
  else if ( _packet->deviation == DEVIATION_VALUE) {
  _packet->deviation = -DEVIATION_VALUE;
  }
  else {
  cerr << " Deviation value has reached an illegal value here " << endl;
  }
  }
  else if (_packet->deviation < 0) {
  if  ( SLRRoutingAgent::SLRForward(_packet) ) {
  return true;
  }
  else if (_packet->deviation > -DEVIATION_VALUE ){
  _packet->deviation --;
  }
  else {
  cerr << " Deviation value has reached an illegal value here " << endl;
  }

  }

  return   ActiveDeviationForward(_packet);

  }



  bool DeviationRoutingAgent::ActiveDeviationForward(PacketPtr _packet)
  {
  int deviation = _packet->deviation;
  bool res = false;
  bool res1 = false;
  bool res2 = false;


  if (_packet->flowSequenceNumber%2 == 0){
  //        res = (!SLRRoutingAgent::SLRForward(_packet,deviation-1) && (SLRRoutingAgent::SLRForward(_packet,deviation)));
  res1 = (!SLRRoutingAgent::SLRForward(_packet,deviation-1));
  res2 = (SLRRoutingAgent::SLRForward(_packet,deviation));
  }
  else{
  //         res = (!SLRRoutingAgent::SLRForward(_packet,-(deviation-1)) && SLRRoutingAgent::SLRForward(_packet,-deviation));
  res1=(!SLRRoutingAgent::SLRForward(_packet,-(deviation-1)));
  res2 = SLRRoutingAgent::SLRForward(_packet,-deviation);
  }

  res = res1 && res2;
  if ( res )  {
  cout << " forwading thx to active deviation "<< deviation << " " << res1 << " " << res2 << " isCloser " << isCloser(_packet)  << endl; getchar();
  }
  else {
  cout << " NOT forwarding "  << " deviation " << deviation << " " << res1 << " " << res2 << " isCloser " << isCloser(_packet) <<endl;

  }

  //   return res;
  }


  void DeviationRoutingAgent::deviation(PacketPtr _packet){
  _packet->deviation = 1;
  }

  bool DeviationRoutingAgent::needDeviation(PacketPtr _packet)
  {
  // TODO : This commented part was for deviation by flow : auto deviation
  // cout << " test deviation packet "<< _packet->flowSequenceNumber << " beta " << _packet->beta  << endl;

  //   map<int,int>::iterator it = deviationFlow.find(packet->flowId);
  //
  //   if (it  != deviationFlow.end()){
  //    int flowDeviation = it->second;
  //    if (flowDeviation > 0){
  //     it->second--;
  //     if (it->second == 0) {
  //       deviationFlow.erase(it);
  // //       cerr << " Delete deviation of flow " << packet->flowId << " on node " << id << endl;
  //       return false;
  //     }
  //     cerr << " Deviation because of deviationFlow = " <<it->second <<  endl;
  //     return true;
  //    }
  //   }

  if ( _packet->deviation > 0 ) return false; // If a packet is already deviated, we don't deviate it again (temporary)

  auto it = reccordedBeta.find(_packet->flowId);




  if (it != reccordedBeta.end() && _packet->beta < it->second){
  cout << " Deviation because of beta " << endl;
  return true;
  }

  return false;
  }*/




//===========================================================================================================
//
//          SLRbackoffSendingEvent2  (class)
//
//===========================================================================================================

SLRbackoffSendingEvent2::SLRbackoffSendingEvent2(simulationTime_t _t, Node* _node,int _packetId) : Event(_t){
  eventType = EventType::SLR_BACKOFF_SENDING_EVENT;
  node = _node;
  packetId = _packetId;
  //         packet = _packet;
}

const string SLRbackoffSendingEvent2::getEventName(){
  return("backoff Sending Event");
}

void SLRbackoffSendingEvent2::consume() {
  //           cout<< " backoffSendingEvent::consume() BEGIN  on node "<< node->getId() << " for packet " << packetId << endl;
  node->getRoutingAgent()->processSLRBackoffSending(packetId);
}
