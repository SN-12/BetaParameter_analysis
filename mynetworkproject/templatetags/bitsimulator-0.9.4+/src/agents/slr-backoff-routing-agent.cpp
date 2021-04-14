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
#include "slr-backoff-routing-agent.h"
#include "scheduler.h"
#include "utils.h"
#include "node.h"
#include "world.h"
#include "events.h"

using namespace std;


/*
SLRBackoffRoutingAgent3 seems to have been added for BitSimulator article,
cf. commit d2dbed
-> compared to without 3, it:
- uses slr with backoff from backoff flooding (counter-based), depending on deden result, whereas without 3 uses a (fixed, given ?) backoff
- only some of the nodes forward data packets, depending on deden result
- only some of the nodes (using beaconRedundancy) participate to slr setup phase

------------

here are main differences (using diff) between SLRBackoffRoutingAgent and SLRBackoffRoutingAgent3 (bottom is 3):

21 jos are SLRUpdate(p)
21 sus pachetul e -1, jos e anchorID*1000

26 sus in rcvpktfromNetw calculeaza totalcollisions et totalaltered
27 jos logheaza routingRCV

29 jos se utilizeaza wantedRedundancy (beaconRedundancy si redundancy)

44 jos trateaza slr zone, slr node, slr broadcast reception

50 jos comentariu: se va utiliza backoff flooding

59 sus se utilizeaza ScenarioParameters::getSlrPathWidth()

76 sus SLRDestinationReach, jos SLRNodeReach si SLRZoneReach
*/

//==============================================================================
//
//          SLRBackoffRoutingAgent3  (class)
//
//==============================================================================


mt19937_64 *SLRBackoffRoutingAgent3::forwardingRNG =  new mt19937_64(0);
ofstream SLRBackoffRoutingAgent3::SLRPositionsFile;

int SLRBackoffRoutingAgent3::currentAnchorID = 0;
int SLRBackoffRoutingAgent3::slrBeaconCounter = 0;
int SLRBackoffRoutingAgent3::totalPacketsReceived = 0;
int SLRBackoffRoutingAgent3::totalPacketsCollisions = 0;
int SLRBackoffRoutingAgent3::totalAlteredBits = 0;

SLRBackoffRoutingAgent3::SLRBackoffRoutingAgent3(Node *_hostNode) : RoutingAgent(_hostNode) {
  type = RoutingAgentType::SLR_BACKOFF;

  SLRCoordX = -1;
  SLRCoordY = -1;
  SLRCoordZ = -1;

  anchorID = -1;
  initialisationStarted = false;
  dedenStarted = false;

  dataBackoffMultiplier = ScenarioParameters::getSlrBackoffMultiplier();
  beaconBackoffMultiplier = ScenarioParameters::getSlrBackoffBeaconMultiplier();

  redundancy = ScenarioParameters::getSlrBackoffredundancy();
  beaconRedundancy = ScenarioParameters::getSlrBeaconBackoffredundancy();


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
    Scheduler::getScheduler().schedule(new SLRBackoffInitialisationGenerationEvent3(t,this));
  }
}

SLRBackoffRoutingAgent3::~SLRBackoffRoutingAgent3() {
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

    //        filename = ScenarioParameters::getScenarioDirectory()+"/reachability_cost.data";
    //       ofstream file2(filename,std::ofstream::app);
    //       file2 << forwardedDataPackets << " " ;
    //       file2.close();
  }
}

void SLRBackoffRoutingAgent3::initializeAgent() {
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



void SLRBackoffRoutingAgent3::processPacketGenerationEvent() {
  cout << " anchor " << anchorID << " on node " << hostNode->getId() << " sent a beacon at " << Scheduler::now() << endl;
  // SLR beacons have a -1 flow id
  PacketPtr p(new Packet(PacketType::SLR_BEACON, 100, hostNode->getId(), -1, -1,anchorID*1000, 0,anchorID,1));
  SLRUpdate(p);
  hostNode->getRoutingAgent()->receivePacketFromApplication(p);
}


void SLRBackoffRoutingAgent3::receivePacketFromNetwork(PacketPtr _packet) {
  LogSystem::EventsLogOutput.log( LogSystem::routingRCV, Scheduler::now(),hostNode->getId(),(int) _packet->type,_packet->flowId,_packet->flowSequenceNumber, _packet->modifiedBitsPositions.size());

  // Initialisation

  //     if (_packet->type == PacketType::DATA){
  //       string filename = ScenarioParameters::getScenarioDirectory()+"/collisions.data";
  //       ofstream file(filename,std::ofstream::app);
  //      file << ScenarioParameters::getDefaultBeta() << " " << ScenarioParameters::getSlrBackoffredundancy() << " " << hostNode->getId() <<" " << _packet->flowId << " " << _packet->flowSequenceNumber << " " << _packet->modifiedBitsPositions.size() << endl;;
  //   }

  LogSystem::EventsLogOutput.log( LogSystem::routingRCV, Scheduler::now(),hostNode->getId(),(int) _packet->type,_packet->flowId,_packet->flowSequenceNumber, _packet->modifiedBitsPositions.size());

  bool update  = false;
  bool forward = false;
  bool directForward = false;

  int wantedRedundancy;
  if (_packet->type == PacketType::DATA) {
    wantedRedundancy = redundancy;
  }
  else if (_packet->type == PacketType::SLR_BEACON){
    wantedRedundancy = beaconRedundancy;
  }

  if (_packet->type == PacketType::DATA || _packet->type == PacketType::SLR_BEACON){
//     cout <<" node " << hostNode->getId() << " receives " << _packet->flowId << " : " << _packet->flowSequenceNumber << endl;
//     cout << " it wait for packet : " << endl;
//     for ( auto it = waitingPacket.begin(); it!=waitingPacket.end();it++){
//       PacketPtr testedPacket = it->second.p;
//       int testedPacketCounter = it->second.counter;
//       cout << testedPacket->flowId << " : " << testedPacket->flowSequenceNumber << " ! " <<it->second.counter  << endl;

//     }

    for ( auto it = waitingPacket.begin(); it!=waitingPacket.end();it++){
      PacketPtr testedPacket = it->second.p;
      int testedPacketCounter = it->second.counter;
      if ( (testedPacket->flowId == _packet->flowId) && (testedPacket->flowSequenceNumber == _packet->flowSequenceNumber) ){ // if the received packet is in waiting state
        if (testedPacketCounter >= wantedRedundancy ){
//          if ( implicitAck(_packet)){
            // //               cout << " received packet " << _packet->flowId << "(" << _packet->flowSequenceNumber << ")" << endl;
            // //               cout << " testedPacket    "<< testedPacket->flowId << "(" << testedPacket   ->flowSequenceNumber << ")" << endl;
            // //               getchar();
            waitingPacket.erase(it);
//             cout << "annulation " << endl; getchar();
//                         LogSystem::EventsLogOutput.log( LogSystem::memoryTrace, Scheduler::now(),hostNode->getId(),"-",it->second.p->flowId,it->second.p->flowSequenceNumber);
            // #Tuto LogSystem


            //                 if (_packet->type == PacketType::DATA ){
            //                   string filename = ScenarioParameters::getScenarioDirectory()+"/memoryTrace.data";
            //                   ofstream memoryTrace(filename,std::ofstream::app);
            //                   memoryTrace << ScenarioParameters::getDefaultBeta() <<" " << hostNode->getId() <<  " - "<< Scheduler::now() << endl;
            //                   memoryTrace.close();
            //                 }
//         } // FIN is implicitAck
//          else { // Le packet n'est pas un implicitAck
//
//          }// else fin implicitAck
        }
        else {
          it->second.counter++;
        }
        return;
      }
    }
  }


  //Forwarding decision

  //SLR Beacons are forwarded with the "normal" forwarding system
  if (_packet->type == PacketType::SLR_BEACON ){
    update = SLRUpdate(_packet);
    if (update) {
      forward = true; // Pb avec le forward des beacon de la seconde ancre ...
    }
    if ( anchorID == 1 &&  _packet->anchorID != anchorID && !initialisationStarted ){
      initialisationStarted = true;
      simulationTime_t t = Scheduler::now()+20000000*anchorID;
      Scheduler::getScheduler().schedule(new SLRBackoffInitialisationGenerationEvent3(t,this));
    }
  }

  // Density probes are never forwarded
  else if ( _packet->type == PacketType::D1_DENSITY_PROBE) {
    hostNode->dispatchPacketToApplication(_packet);
    return;
  }

  // Densit inits are "directly forwarded"
  else if ( _packet->type == PacketType::D1_DENSITY_INIT && !dedenStarted ) {
    hostNode->dispatchPacketToApplication(_packet);
    dedenStarted=true;
    directForward=true;
  }

  else if ( _packet->type == PacketType::DATA) {
    int flowId = _packet->flowId;
    int sequenceNumber = _packet->flowSequenceNumber;
    //     bool alreadySeen;

    // If we want an SLR ZONE reception
    //     if (SLRZoneReach(_packet){
    //       hostNode->dispatchPacketToApplication(_packet);
    //     }

    //If we want an SLR NODE reception
    if (SLRNodeReach(_packet)){
      hostNode->dispatchPacketToApplication(_packet);
      LogSystem::EventsLogOutput.log( LogSystem::dstReach, Scheduler::now(),hostNode->getId(),(int)_packet->type,_packet->flowId,_packet->flowSequenceNumber);
    }

    // If we want an SLR BROADCAST reception
    if (_packet->dstId ==  -1 ){
      hostNode->dispatchPacketToApplication(_packet);
//             LogSystem::EventsLogOutput.log( LogSystem::dstReach, Scheduler::now(),hostNode->getId(),(int)_packet->type,_packet->flowId,_packet->flowSequenceNumber);
      LogSystem::EventsLogOutput.log(LogSystem::slrBroadcastReach, Scheduler::now(),hostNode->getId(),(int)_packet->type,_packet->flowId,_packet->flowSequenceNumber,SLRCoordX,SLRCoordY,SLRCoordZ);


      forward=true;
    }

    auto it = alreadyForwardedPackets.find(flowId);
    if ( it != alreadyForwardedPackets.end() ) { //Already existing flow
      auto iit = it->second.find(sequenceNumber);
      if ( iit != it->second.end() ) { // Packet already seen
        //         cout << "Already seen packet " << sequenceNumber << "(" << flowId << ") on node " << hostNode->getId() << " at " << Scheduler::now() << endl;
        forward=false;
      }
      else { // new packet from an existing flow
        it->second.insert(sequenceNumber);
        forward=true;
      }
    }
    else { //New flow
      set<int> newSet;
      newSet.insert(sequenceNumber);
      alreadyForwardedPackets.insert(pair<int,set<int>>(flowId,newSet));
      forward=true;
    } //END CHECKING FLOW


    if ( forward && (SLRForward(_packet) || _packet->dstId == -1) /*&& isCloser(_packet)*/){
      forward=true;
    }
    else{
      forward=false;
    } // END CHECKING SLR
  }

  // Here we know the packet have to be forwarded using the normal forwarding scheme, the "backoff flooding"
  if ( forward ) {
    PacketPtr newPacket(_packet->clone());
    if ( update ){
      newPacket->anchorDist++;
    }

    newPacket->tx_SLRX = SLRCoordX;
    newPacket->tx_SLRY = SLRCoordY;
    newPacket->tx_SLRZ = SLRCoordZ;

    time_t effectiveBackoffWindow =2*((Node::getPulseDuration() * _packet->beta * (_packet->size-1) + Node::getPulseDuration()) + ScenarioParameters::getCommunicationRange()/300 );

    if ( hostNode->getEstimatedNeighbours() != -1 ) {
      if (_packet->type == PacketType::DATA ){
        effectiveBackoffWindow*=dataBackoffMultiplier*hostNode->getEstimatedNeighbours();
        //           effectiveBackoffWindow*=100;
      } else if(_packet->type == PacketType::SLR_BEACON)
        effectiveBackoffWindow*=beaconBackoffMultiplier*hostNode->getEstimatedNeighbours();
    }


    uniform_int_distribution<time_t> distrib(0, effectiveBackoffWindow);
    time_t backoffTime = distrib(*forwardingRNG);

    SLRbackoffRoutingCounter info;
    info.p = newPacket;
    info.counter=1;
    waitingPacket.insert(pair<int,SLRbackoffRoutingCounter>(newPacket->packetId,info));
    Scheduler::getScheduler().schedule(new SLRbackoffSendingEvent3(Scheduler::now() + backoffTime, hostNode,newPacket->packetId));
//         LogSystem::EventsLogOutput.log( LogSystem::memoryTrace, Scheduler::now(),hostNode->getId(),"+",_packet->flowId,_packet->flowSequenceNumber);

    //             if (newPacket->type == PacketType::DATA ){
    //         string filename = ScenarioParameters::getScenarioDirectory()+"/memoryTrace.data";
    //         ofstream memoryTrace(filename,std::ofstream::app);
    //         memoryTrace << ScenarioParameters::getDefaultBeta() <<" " << hostNode->getId() <<  " + "<< Scheduler::now() << endl;
    //         memoryTrace.close();
    //       }

  }

  // Here we know the packet have to be directly forwarded
  else if ( directForward ){
    PacketPtr newPacket(_packet->clone());

    newPacket->tx_SLRX = SLRCoordX;
    newPacket->tx_SLRY = SLRCoordY;
    newPacket->tx_SLRZ = SLRCoordZ;

    hostNode->enqueueOutgoingPacket(newPacket);
  }
}


bool SLRBackoffRoutingAgent3::SLRForward(PacketPtr _packet, int m){
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


bool SLRBackoffRoutingAgent3::SLRForward(PacketPtr _packet){
  return SLRForward(_packet,1);
}


void SLRBackoffRoutingAgent3::receivePacketFromApplication(PacketPtr _packet) {
  _packet->setSrcSequenceNumber(hostNode->getNextSrcSequenceNumber());

  _packet->src_SLRX = SLRCoordX;
  _packet->src_SLRY = SLRCoordY;
  _packet->src_SLRZ = SLRCoordZ;

  _packet->tx_SLRX = SLRCoordX;
  _packet->tx_SLRY = SLRCoordY;
  _packet->tx_SLRZ = SLRCoordZ;


  //TODO : how to find values here ?
  // Now the node just knows SLR address of any node by magic
  if (_packet->type == PacketType::DATA ){
    if (_packet->dstId != -1){

      Node* dst = World::getNode(_packet->dstId);
      SLRBackoffRoutingAgent3* dstAgent = dynamic_cast<SLRBackoffRoutingAgent3*> (dst->getRoutingAgent());

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
      // Here we use -2 as diffusion address to no mistake for the "no-adress"
      _packet->dst_SLRX = -2;
      _packet->dst_SLRY = -2;
      _packet->dst_SLRZ = -2;
    }
  }

  hostNode->enqueueOutgoingPacket(_packet);
}


bool SLRBackoffRoutingAgent3::isCloser(PacketPtr _packet){
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

  if ( dstX == -2 && dstY == -2 && dstZ == -2){
    int dist,disttx;
    dist = abs(X - srcX) + abs(Y - srcY) ;
    disttx = abs(txX - srcX) + abs(txY - srcY) ;

    if (dist > disttx){
      res = true;
    }

    return res;

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

  //  if (res) cout << " isCloser on node " << hostNode->getId() << " res = " << res << endl;

  //   printf(" dstX %d dstY %d dstZ %d \n", dstX,dstY,dstZ);
  //   printf(" srcX %d srcY %d srcZ %d \n", srcX,srcY,srcZ);
  //   printf("  txX %d  txY %d  txZ %d \n", txX,txY,txZ);
  //   printf("  X   %d  Y   %d  Z   %d \n", X,Y,Z);

  return res;
}

bool SLRBackoffRoutingAgent3::implicitAck(PacketPtr _packet){
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


  if (dstX != -2 && dstY != -2 && dstZ != -2) {  // If we're NOT on a broadcasting flow

    //   if ( X == txX && Y == txY && Z == txZ ){
    //    res = true;
    //   }
    //

    if ( txX == dstX && txY == dstY && txZ == dstZ   ) {
      res = true;
      //     return true;
    }


    if ( (srcX < dstX) && (X < txX) ) {
      res = true;
      //     return true;
    }
    if ( (srcX > dstX) && (X > txX) ) {
      res = true;
      //     return true;
    }

    if ( (srcY < dstY) && (Y < txY) ) {
      res = true;
      //     return true;
    }
    if ( (srcY > dstY) && (Y > txY) ) {
      res = true;
      //     return true;
    }

    if ( (srcZ < dstZ) && (Z < txZ) ) {
      res = true;
      //     return true;
    }
    if ( (srcZ > dstZ) && (Z > txZ) ) {
      res = true;
      //     return true;
    }
  } else { // we're on a broadcasting flow
    int distF; // distance between src and forwarder
    int dist; // distance between src and receiving node

    distF = abs(srcX-txX)+abs(srcY-txY)+abs(srcZ-txZ);
    dist  = abs(srcX-X)+abs(srcY-Y)+abs(srcZ-Z);

    res = distF > dist;
  }

  return res;
}


bool SLRBackoffRoutingAgent3::SLRNodeReach(PacketPtr _packet){
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

bool SLRBackoffRoutingAgent3::SLRZoneReach(PacketPtr _packet){
  if (_packet->dst_SLRX == SLRCoordX && _packet->dst_SLRY == SLRCoordY && _packet->dst_SLRZ == SLRCoordZ ) {
    cout << " DESTINATION REACHED on node " << hostNode->getId() << "(" << SLRCoordX << "," << SLRCoordY << "," << SLRCoordZ << ")" << " packet (flow) " << _packet->flowSequenceNumber << "(" << _packet->flowId << ")"  << endl;
    //     getchar();
    return true;
  }
  return false;
}

bool SLRBackoffRoutingAgent3::SLRUpdate(PacketPtr _packet){
  int anchorId = _packet->anchorID;
  int anchorDist =  _packet->anchorDist;
  bool update=false;

  SLRBackoffRoutingAgent3* routing = static_cast<SLRBackoffRoutingAgent3*> (hostNode->getRoutingAgent());

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

void SLRBackoffRoutingAgent3::processSLRBackoffSending(int packetId){
  auto it = waitingPacket.find(packetId);

  if ( it != waitingPacket.end() ){// On envoi le paquet uniquement s'il est toujours en attente
    PacketPtr forwardingPacket = it->second.p;
    hostNode->enqueueOutgoingPacket(forwardingPacket);
    LogSystem::EventsLogOutput.log( LogSystem::routingSND, Scheduler::now(),hostNode->getId(),(int) it->second.p->type,it->second.p->flowId,it->second.p->flowSequenceNumber);
//         LogSystem::EventsLogOutput.log( LogSystem::memoryTrace, Scheduler::now(),hostNode->getId(),"-",it->second.p->flowId,it->second.p->flowSequenceNumber);

    waitingPacket.erase(it);
  }
}

//==============================================================================
//
//          SLRBackoffInitialisationGenerationEvent3  (class)
//
//==============================================================================

SLRBackoffInitialisationGenerationEvent3::SLRBackoffInitialisationGenerationEvent3(simulationTime_t _t, SLRBackoffRoutingAgent3 *_gen) : Event(_t){
  eventType = EventType::SLR_INITIALISATION_PACKET_GENERATION;
  concernedGenerator = _gen;
}

const string SLRBackoffInitialisationGenerationEvent3::getEventName(){
  return "SLRInitialisationGeneration Event";
}

void SLRBackoffInitialisationGenerationEvent3::consume() {
  concernedGenerator->processPacketGenerationEvent();
}




//==============================================================================
//
//          SLRbackoffSendingEvent3  (class)
//
//==============================================================================

SLRbackoffSendingEvent3::SLRbackoffSendingEvent3(simulationTime_t _t, Node* _node,int _packetId) : Event(_t){
  eventType = EventType::SLR_BACKOFF_SENDING_EVENT;
  node = _node;
  packetId = _packetId;
  //         packet = _packet;
}

const string SLRbackoffSendingEvent3::getEventName(){
  return "backoff Sending Event";
}

void SLRbackoffSendingEvent3::consume() {
  node->getRoutingAgent()->processSLRBackoffSending(packetId);
}
