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
#include "slr-backoff-routing-agent3.h"
#include "scheduler.h"
#include "utils.h"
#include "node.h"
#include "world.h"
#include "events.h"

using namespace std;

//==============================================================================
//
//          SLRBackoffRoutingAgent  (class)
//
//==============================================================================


mt19937_64 *SLRBackoffRoutingAgent::forwardingRNG =  new mt19937_64(0);
ofstream SLRBackoffRoutingAgent::SLRPositionsFile;

int SLRBackoffRoutingAgent::currentAnchorID = 0;
int SLRBackoffRoutingAgent::slrBeaconCounter = 0;
int SLRBackoffRoutingAgent::totalPacketsReceived = 0;
int SLRBackoffRoutingAgent::totalPacketsCollisions = 0;
int SLRBackoffRoutingAgent::totalAlteredBits = 0;

SLRBackoffRoutingAgent::SLRBackoffRoutingAgent(Node *_hostNode) : RoutingAgent(_hostNode) {
  type = RoutingAgentType::SLR_BACKOFF;

  SLRCoordX = -1;
  SLRCoordY = -1;
  SLRCoordZ = -1;

  anchorID = -1;
  initialisationStarted = false;

  dataBackoffMultiplier = ScenarioParameters::getSlrBackoffMultiplier();
  beaconBackoffMultiplier = ScenarioParameters::getSlrBackoffBeaconMultiplier();

  redundancy = ScenarioParameters::getSlrBackoffredundancy();
  beaconRedundancy = ScenarioParameters::getSlrBeaconBackoffredundancy();

  //     redundancy = 10;
  //     beaconRedundancy = 20;

  vector<NodeInfo> vect = ScenarioParameters::getVectNodeInfo();
  for ( auto it = vect.begin(); it != vect.end(); it++ ) {
    if ( it->isAnchor && it->id == hostNode->getId() ) {
      anchorID = currentAnchorID;
      currentAnchorID++;
    }
  }
  if ( anchorID == 0 ){
    simulationTime_t t = Scheduler::now()+400000000000;
    //         simulationTime_t t = Scheduler::now();
    initialisationStarted = true;
    Scheduler::getScheduler().schedule(new SLRBackoffInitialisationGenerationEvent(t,this));
  }
}

SLRBackoffRoutingAgent::~SLRBackoffRoutingAgent() {
  if ( SLRPositionsFile.is_open() ) {
    SLRPositionsFile << hostNode->getId() << " " << SLRCoordX << " " << SLRCoordY << " " << SLRCoordZ << endl;
  }

  aliveAgents--;

  if ( aliveAgents == 0 ) {
    cout << " SLR Backoff Routing:: forwarded packets : " << forwardedDataPackets << endl;
    cout << " SLR Backoff Routing:: forwarded beacons : " << slrBeaconCounter  << endl;

    cout << " SLR Backoff Routing:: Total packet received   : " << totalPacketsReceived << endl;
    cout << " SLR Backoff Routing:: Total packet collisions : " << totalPacketsCollisions<< endl;
    cout << " SLR Backoff Routing:: Total bit altered       : " << totalAlteredBits << endl << endl;
  }
}

void SLRBackoffRoutingAgent::initializeAgent() {
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



void SLRBackoffRoutingAgent::processPacketGenerationEvent() {
  cout << " anchor " << anchorID << " on node " << hostNode->getId() << " sent a beacon" << endl;

  PacketPtr p(new Packet(PacketType::SLR_BEACON, 100, hostNode->getId(), -1, -1,-1, 0,anchorID,1));
  hostNode->getRoutingAgent()->receivePacketFromApplication(p);
}


void SLRBackoffRoutingAgent::receivePacketFromNetwork(PacketPtr _packet) {
  LogSystem::EventsLogOutput.log( LogSystem::routingRCV, Scheduler::now(),hostNode->getId(),(int) _packet->type,_packet->flowId,_packet->flowSequenceNumber, _packet->modifiedBitsPositions.size());


  //     if ( _packet->type == PacketType::DATA ) {
//         cout << Scheduler::getScheduler().now() << " SLRRoutingAgent::receivePacket on Node " << hostNode->getId();
//         cout << " srcId:" << _packet->srcId << " srcSeq:" << _packet->srcSequenceNumber;
//         cout << " dstId:" << _packet->dstId << " port:" << _packet->port ;
//         cout << " size:" << _packet->size;
//         cout << " flowId:" << _packet->flowId << " flowSeq:" << _packet->flowSequenceNumber;
//         cout << endl;
//     }

  totalPacketsReceived++;
  int nbAlteredBits = _packet->modifiedBitsPositions.size();
  if (nbAlteredBits > 0) {
    //        cout << "nbAlteredBits: " << nbAlteredBits << " on node " << hostNode->getId() << endl;
    totalPacketsCollisions++;
    totalAlteredBits += nbAlteredBits ;
  }

  //        if ( _packet->modifiedBitsPositions.size() > 0 ) {
  //         return;
  //        }
  //


  bool alreadySeen = false;
  bool update = false;
  bool forward = false;
  auto it =alreadyForwardedPackets.find(_packet->flowId);
  if (it != alreadyForwardedPackets.end() ){ // Flow already followed
    if ( it->second.find(_packet->flowSequenceNumber) != it->second.end() ) {
      //                     if ( it->second >= _packet->flowSequenceNumber){
      alreadySeen = true;
    }
  }

  for (auto it = waitingPacket.begin(); it!=waitingPacket.end();it++){
    if ( (it->second.p)->flowId == _packet->flowId && (it->second.p)->flowSequenceNumber == _packet->flowSequenceNumber) {
      if ( _packet->type == PacketType::SLR_BEACON && (it->second.counter) >= beaconRedundancy ) {
        it=waitingPacket.erase(it);
      }
      else if (  _packet->type == PacketType::DATA && (it->second.counter) >= redundancy){
        if ( implicitAck(_packet)) {
          //                             if ( _packet->flowSequenceNumber != (it->second.p)->flowSequenceNumber || _packet->flowId != (it->second.p)->flowId ){
          //                               cout << " packet " << _packet->flowSequenceNumber << "(" << _packet->flowId << ") ACK packet " << (it->second.p)->flowSequenceNumber << "(" << (it->second.p)->flowId << ")" << endl; getchar();
          //                             }
          it=waitingPacket.erase(it);
        }

      } else {
        it->second.counter++;
      }
      break;
    }
    return;
  }

  if (  _packet->type == PacketType::D1_DENSITY_INIT && !alreadySeen ) {
    hostNode->dispatchPacketToApplication(_packet);
    //PacketPtr newPacket(new Packet(_packet));
    PacketPtr newPacket(_packet->clone());

    auto it = alreadyForwardedPackets.find(_packet->flowId);
    if ( it != alreadyForwardedPackets.end() ) {
      it->second.insert(_packet->flowSequenceNumber);
    }
    else{

      pair<int,set<int>> toto = pair<int,set<int>>(_packet->flowId,set<int>());
      alreadyForwardedPackets.insert(toto);
      //                     alreadyForwardedPackets.insert(pair<int,int>(_packet->flowId,_packet->flowSequenceNumber));
    }
    hostNode->enqueueOutgoingPacket(newPacket);
    return;
  }

  if ( _packet->type == PacketType::D1_DENSITY_PROBE) {
    hostNode->dispatchPacketToApplication(_packet);
    return;
  }

  if (_packet->type == PacketType::SLR_BEACON ){
    update = SLRUpdate(_packet);
    if (update) {
      forward = true;
    }
    if ( anchorID == 1 &&  _packet->anchorID != anchorID && !initialisationStarted ){

      initialisationStarted = true;
      simulationTime_t t = Scheduler::now()+20000000*anchorID;
      Scheduler::getScheduler().schedule(new SLRBackoffInitialisationGenerationEvent(t,this));
    }
  }

  if (_packet->type == PacketType::DATA && (SLRDestinationReach(_packet) || _packet->dstId ==  -1) ) {
    LogSystem::EventsLogOutput.log( LogSystem::dstReach, Scheduler::now(),hostNode->getId(),(int)_packet->type,_packet->flowId,_packet->flowSequenceNumber);
    hostNode->dispatchPacketToApplication(_packet);
    return;
  }
  if (_packet->type == PacketType::DATA && _packet->dst_SLRX == -2 && _packet->dst_SLRY == -2 && _packet->dst_SLRZ == -2  ) {
    hostNode->dispatchPacketToApplication(_packet);

  }

  if ( (!alreadySeen && _packet->type == PacketType::DATA)){
    //                     cout << " Node : " << hostNode->getId() << " packet : " << _packet->flowSequenceNumber <<  " src (" << _packet->src_SLRX << "," << _packet->src_SLRY << "," << _packet->src_SLRZ << ") dst ( " << _packet->dst_SLRX << "," << _packet->dst_SLRY << "," << _packet->dst_SLRZ << ") cur (" << SLRCoordX << "," << SLRCoordY  << "," << SLRCoordZ << ")";
    //                     cout << " at " << Scheduler::now() <<  " forwarding decision : " << (int) SLRForward(_packet) << endl;


    //                   if (_packet->flowId == 0 && (SLRForward(_packet) || isCloser(_packet)  )) {
    //                       cout << " Packet : " << _packet->flowSequenceNumber << "(" << _packet->flowId << ") forward : " << SLRForward(_packet) << "(" << isCloser(_packet) << ") SLR zone  " << SLRCoordX << ":" << SLRCoordY << ":" << SLRCoordZ << endl;
    //                             if (SLRForward(_packet) && isCloser(_packet)){
    //                               cout << " YOUPIIII" << endl;
    //                             }
    // //                         getchar();
    //                   }

    if ( SLRForward(_packet) && isCloser(_packet)){
      forward = true;
      auto it =alreadyForwardedPackets.find(_packet->flowId);
      if (it != alreadyForwardedPackets.end()){
        it->second.insert(_packet->flowSequenceNumber);
      }
      else{
        pair<int,set<int>> toto = pair<int,set<int>>(_packet->flowId,set<int>());
        alreadyForwardedPackets.insert(toto);
        //                      alreadyForwardedPackets.insert(pair<int,int>(_packet->flowId,_packet->flowSequenceNumber));
      }
      //                      else{
      //                          alreadyForwardedPackets.insert(pair<int,int>(_packet->flowId,_packet->flowSequenceNumber));
      //                      }

    }
  }

  if ( forward ) {

    //                   if (_packet->type == PacketType::SLR_BEACON ) {
    //                     PacketPtr newPacket(_packet->clone());
    //                     if ( update ){
    //                       newPacket->anchorDist++;
    //                     }
    //                     alreadyForwardedPackets.insert(pair<int,int>(_packet->flowId,_packet->flowSequenceNumber));
    //                     uniform_real_distribution<float> slrBeacondistrib(0, 1);
    //                     float forwardingProbabilityNumber = slrBeacondistrib(*forwardingRNG);
    //                     int wantedForwarder = 30;
    //                      float forwadingProbability;
    //                       if ( hostNode->getEstimatedNeighbours() == -1 ){
    //                         forwadingProbability = 1;
    //                       }
    //                       else {
    //                         forwadingProbability = ((float)wantedForwarder)/(float)hostNode->getEstimatedNeighbours();
    //                       }
    //                     bool retransmit = forwardingProbabilityNumber < forwadingProbability ;
    //                     if ( retransmit ){
    //                       hostNode->enqueueOutgoingPacket(newPacket);
    //                       slrBeaconCounter++;
    //                     }
    //                     return;
    //                   }
    //

    time_t effectiveBackoffWindow =2*((Node::getPulseDuration() * _packet->beta * (_packet->size-1) + Node::getPulseDuration()) + ScenarioParameters::getCommunicationRange()/300 );
    if ( hostNode->getEstimatedNeighbours() != -1 ) {
      if (_packet->type == PacketType::DATA ){
        effectiveBackoffWindow*=dataBackoffMultiplier*hostNode->getEstimatedNeighbours();
      }
      else if(_packet->type == PacketType::SLR_BEACON){
        effectiveBackoffWindow*=beaconBackoffMultiplier*hostNode->getEstimatedNeighbours();
      }
    }
    uniform_int_distribution<time_t> distrib(0, effectiveBackoffWindow);
    time_t backoffTime = distrib(*forwardingRNG);

    PacketPtr newPacket(_packet->clone());
    if ( update ){
      newPacket->anchorDist++;
    }

    if ( _packet->type == PacketType::DATA) {
      newPacket->hopCount++;
    }

    newPacket->tx_SLRX = SLRCoordX;
    newPacket->tx_SLRY = SLRCoordY;
    newPacket->tx_SLRZ = SLRCoordZ;

    struct SLRbackoffRoutingCounter info;
    info.p = newPacket;
    info.counter = 1;
    waitingPacket.insert(pair<int,SLRbackoffRoutingCounter>(newPacket->packetId,info));
    Scheduler::getScheduler().schedule(new SLRbackoffSendingEvent(Scheduler::now() + backoffTime, hostNode,newPacket->packetId));
  }
}


bool SLRBackoffRoutingAgent::SLRForward(PacketPtr _packet, int m)
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
  /*
    if ( dstX == X &&  dstY == Y && dstZ == Z ){
    return false;
    }
  */
  if ( dstX == -2 &&  dstY == -2 && dstZ == -2 ){
    return true;
  }

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

  //    cout << " Routing on node " << hostNode->getId() << " SLR : " << onTheLine << " " << onSegment << endl;
  /*
    printf(" dstX %d dstY %d dstZ %d \n", dstX,dstY,dstZ);
    printf(" srcX %d srcY %d srcZ %d \n", srcX,srcY,srcZ);
    printf("  txX %d  txY %d  txZ %d \n", txX,txY,txZ);
    printf("    X %d    Y %d Z    %d \n", X,Y,Z);
    getchar();
  */
  return onTheLine && onSegment;
}


bool SLRBackoffRoutingAgent::SLRForward(PacketPtr _packet)
{
  return SLRForward(_packet,ScenarioParameters::getSlrPathWidth());
}


void SLRBackoffRoutingAgent::receivePacketFromApplication(PacketPtr _packet) {
  _packet->setSrcSequenceNumber(hostNode->getNextSrcSequenceNumber());

  _packet->src_SLRX = SLRCoordX;
  _packet->src_SLRY = SLRCoordY;
  _packet->src_SLRZ = SLRCoordZ;

  _packet->tx_SLRX = SLRCoordX;
  _packet->tx_SLRY = SLRCoordY;
  _packet->tx_SLRZ = SLRCoordZ;


  //TODO : how to find values here ?
  if (_packet->type == PacketType::DATA ){
    if (_packet->dstId != -1){

      Node* dst = World::getNode(_packet->dstId);
      SLRBackoffRoutingAgent* dstAgent = dynamic_cast<SLRBackoffRoutingAgent*> (dst->getRoutingAgent());

      if ( dstAgent == nullptr ) {
        cerr << " Error in setting SLR destination to a packet. The targeted destination may not run SLR " << endl;
      }
      else {

        _packet->dst_SLRX = dstAgent->getSLRX();
        _packet->dst_SLRY = dstAgent->getSLRY();
        _packet->dst_SLRZ = dstAgent->getSLRZ();
      }
    }
    else {
      // Here e use -2 as diffusion adress to no mistake for the "no-adress"
      _packet->dst_SLRX = -2;
      _packet->dst_SLRY = -2;
      _packet->dst_SLRZ = -2;
    }
  }

  //          if (_packet->type == PacketType::DATA ) {cout << " J'envoie un packet at  " << Scheduler::now() << endl; getchar(); }
  hostNode->enqueueOutgoingPacket(_packet);
}


bool SLRBackoffRoutingAgent::isCloser(PacketPtr _packet) {
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

  //   if ( (X == dstX) && (Y=dstY) && (Z==dstZ) ){
  //    res = true;
  // //     return true;
  //   }

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

  //  if (res) cout << " isCloser on node " << hostNode->getId() << " res = " << res << endl;

  //   printf(" dstX %d dstY %d dstZ %d \n", dstX,dstY,dstZ);
  //   printf(" srcX %d srcY %d srcZ %d \n", srcX,srcY,srcZ);
  //   printf("  txX %d  txY %d  txZ %d \n", txX,txY,txZ);
  //   printf("  X   %d  Y   %d  Z   %d \n", X,Y,Z);
  //
  //   getchar();
  //

  return res;
}

bool SLRBackoffRoutingAgent::implicitAck(PacketPtr _packet)
{

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

  if ( X == txX && Y == txY && Z == txZ ){
    res = true;
    //     return true;
  }


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


bool SLRBackoffRoutingAgent::SLRDestinationReach(PacketPtr _packet)
{
  //   TODO : DO we want a zone reception or a node reception ?

  if (hostNode->getId() == _packet->dstId) {
    //         cout << " DESTINATION REACHED on node " << hostNode->getId() << "(" << SLRCoordX << "," << SLRCoordY << "," << SLRCoordZ << ")" << " packet (flow) " << _packet->flowSequenceNumber << "(" << _packet->flowId << ")"  << endl;
    cout << " DESTINATION REACHED on node " << hostNode->getId() << "(" << SLRCoordX << "," << SLRCoordY << "," << SLRCoordZ << ")" << " packet (flow) " << _packet->flowSequenceNumber << "(" << _packet->flowId << ") from node " << _packet->transmitterId << "(" << _packet->tx_SLRX << "," << _packet->tx_SLRY << "," << _packet->tx_SLRZ << ")" <<endl;
    //                  getchar();
    return true;
  }

  //   if (_packet->dst_SLRX == SLRCoordX && _packet->dst_SLRY == SLRCoordY && _packet->dst_SLRZ == SLRCoordZ ) {
  //     cout << " DESTINATION REACHED on node " << hostNode->getId() << "(" << SLRCoordX << "," << SLRCoordY << "," << SLRCoordZ << ")" << " packet (flow) " << _packet->flowSequenceNumber << "(" << _packet->flowId << ")"  << endl;
  // //     getchar();
  //       return true;
  //   }
  //
  return false;
}


bool SLRBackoffRoutingAgent::SLRUpdate(PacketPtr _packet)
{
  int anchorId = _packet->anchorID;
  int anchorDist =  _packet->anchorDist;
  bool update=false;

  SLRBackoffRoutingAgent* routing = static_cast<SLRBackoffRoutingAgent*> (hostNode->getRoutingAgent());


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
    //          case 2:
    //               if (anchorDist < routing->getSLRZ() || routing->getSLRZ() == -1  ){
    //                 routing->setSLRZ(anchorDist);
    //                 update=true;
    //               }
    break;
  default:
    cout << " WARNING : receiving SLR_BEACON from unknown anchor " << endl;
  }

  return update;
}

void SLRBackoffRoutingAgent::processSLRBackoffSending(int packetId){
  auto it = waitingPacket.find(packetId);
  //     if ( it != waitingPacket.end() && it->second.p->type == PacketType::DATA ) {
  if ( it != waitingPacket.end() ) {
    //
    // if the packet is still in the waiting list
    // i.e. I have not see more than REDUNDANCY copies
    // then I send it now
    //
    //cout << " ++ forward seq " << it->second.p->flowSequenceNumber << " by node " << hostNode->getId()<< " at " << Scheduler::getScheduler().now() << endl;
    hostNode->enqueueOutgoingPacket(it->second.p);
    LogSystem::EventsLogOutput.log( LogSystem::routingSND, Scheduler::now(),hostNode->getId(),(int) it->second.p->type,it->second.p->flowId,it->second.p->flowSequenceNumber);

    if (it->second.p->type == PacketType::DATA){
      forwardedDataPackets++;
    }
    else if (it->second.p->type == PacketType::SLR_BEACON){
      slrBeaconCounter++;
    }

    waitingPacket.erase(it);
  }
}

//==============================================================================
//
//          SLRBackoffInitialisationGenerationEvent  (class)
//
//==============================================================================

SLRBackoffInitialisationGenerationEvent::SLRBackoffInitialisationGenerationEvent(simulationTime_t _t, SLRBackoffRoutingAgent *_gen) : Event(_t){

  eventType = EventType::SLR_INITIALISATION_PACKET_GENERATION;
  concernedGenerator = _gen;
}

const string SLRBackoffInitialisationGenerationEvent::getEventName(){
  return "SLRInitialisationGeneration Event";
}

void SLRBackoffInitialisationGenerationEvent::consume() {
  concernedGenerator->processPacketGenerationEvent();
}


//==============================================================================
//
//          SLRbackoffSendingEvent  (class)
//
//==============================================================================

SLRbackoffSendingEvent::SLRbackoffSendingEvent(simulationTime_t _t, Node* _node,int _packetId) : Event(_t){
  eventType = EventType::SLR_BACKOFF_SENDING_EVENT;
  node = _node;
  packetId = _packetId;
  //         packet = _packet;
}

const string SLRbackoffSendingEvent::getEventName(){
  return "backoff Sending Event";
}

void SLRbackoffSendingEvent::consume() {
  node->getRoutingAgent()->processSLRBackoffSending(packetId);
}
