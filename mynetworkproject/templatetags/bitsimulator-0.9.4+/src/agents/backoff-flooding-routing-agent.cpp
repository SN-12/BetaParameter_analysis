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

#include "scheduler.h"
#include "backoff-flooding-routing-agent.h"


//==============================================================================
//
//          BackoffFloodingRoutingAgent    (class)
//
//==============================================================================
set<int> BackoffFloodingRoutingAgent::reachability = set<int>();
mt19937_64 *BackoffFloodingRoutingAgent::forwardingRNG;
int BackoffFloodingRoutingAgent::redundancy;
float BackoffFloodingRoutingAgent::dataBackoffMultiplier;


BackoffFloodingRoutingAgent::BackoffFloodingRoutingAgent (Node *_hostNode) : RoutingAgent(_hostNode) {
  type = RoutingAgentType::BACKOFF_FLOODING;
  backoffWindow = 2 *(ScenarioParameters::getCommunicationRange()/300);
  initialisationStarted = false;
}

BackoffFloodingRoutingAgent::~BackoffFloodingRoutingAgent () {
  aliveAgents--;

  if (aliveAgents == 0) {
    //        TODO : Info globale backoff Flooding
    cout << " Backoff flooding:: number of nodes reached : " << reachability.size() << endl;
    cout << " Backoff flooding:: forwarded packets       : " << forwardedDataPackets << endl;

    //     string filename = ScenarioParameters::getScenarioDirectory()+"/reachability_cost.data";
    //     ofstream file(filename,std::ofstream::app);
    //     file << forwardedDataPackets << " " ;
    //     file.close();
  }
}

void BackoffFloodingRoutingAgent::initializeAgent() {
  forwardingRNG =  new mt19937_64(ScenarioParameters::getBackoffFloodingRNGSeed());
  redundancy = ScenarioParameters::getSlrBackoffredundancy();
  dataBackoffMultiplier = ScenarioParameters::getSlrBackoffMultiplier();
}

void BackoffFloodingRoutingAgent::receivePacketFromNetwork(PacketPtr _packet){
  LogSystem::EventsLogOutput.log( LogSystem::routingRCV, Scheduler::now(),hostNode->getId(),(int) _packet->type,_packet->flowId,_packet->flowSequenceNumber, _packet->modifiedBitsPositions.size());
  //   bool insertedPackets = false;
  //   bool erasedPackets = false;

  if ( _packet->modifiedBitsPositions.size() > 0 ) {
    return;
  }

  if (_packet->dstId == hostNode->getId() ) {
    hostNode->dispatchPacketToApplication(_packet);
    reachability.insert(hostNode->getId());
    LogSystem::EventsLogOutput.log( LogSystem::dstReach, Scheduler::now(),hostNode->getId(),(int)_packet->type,_packet->flowId,_packet->flowSequenceNumber);
    return;
  }

  if ( _packet->type == PacketType::D1_DENSITY_PROBE) {  //PROBE is local
    hostNode->dispatchPacketToApplication(_packet);
    return;
  }

  if (  _packet->type == PacketType::D1_DENSITY_INIT && !initialisationStarted ) { //INIT is global
    initialisationStarted = true;
    hostNode->dispatchPacketToApplication(_packet);
    PacketPtr newPacket(_packet->clone());
    hostNode->enqueueOutgoingPacket(newPacket);
    return;
  }

  if (_packet->dstId == -1 ) {
    hostNode->dispatchPacketToApplication(_packet);
    LogSystem::EventsLogOutput.log( LogSystem::dstReach, Scheduler::now(),hostNode->getId(),(int)_packet->type,_packet->flowId,_packet->flowSequenceNumber);
    //       reachability.insert(hostNode->getId());
    //       return;
  }


  // If I already saw this packet REDUNDANCY or more times, I prevent any retransmission already scheduled
  //
  // Also further copies of this packet will not be retransmitted either, as
  // alreadySeen list should prevent it.
  //
  for (auto it = waitingPacket.begin(); it!=waitingPacket.end();it++){
    if ( (it->second.p)->flowId == _packet->flowId && (it->second.p)->flowSequenceNumber == _packet->flowSequenceNumber) {
      if (_packet->type == PacketType::DATA && it->second.counter >= 2){ //redundancy
        it = waitingPacket.erase(it);
        // erasedPackets=true;
        // LogSystem::EventsLogOutput.log( LogSystem::memoryTrace, Scheduler::now(),hostNode->getId(),"-",_packet->flowId,_packet->flowSequenceNumber);
      } else {
        it->second.counter++;
      }
      break;
    }
  }
  simulationTime_t effectiveBackoffWindow = 2*((Node::getPulseDuration() * _packet->beta * (_packet->size-1) + Node::getPulseDuration()) + ScenarioParameters::getCommunicationRange()/300 );

  if (hostNode->getEstimatedNeighbours() != -1){
    effectiveBackoffWindow *= hostNode->getEstimatedNeighbours();
  }

  if (_packet->type ==PacketType::DATA ){
    effectiveBackoffWindow *= dataBackoffMultiplier;
  }
  uniform_int_distribution<time_t> distrib(0, effectiveBackoffWindow);
  simulationTime_t backoffTime = distrib(*forwardingRNG);

  auto srcIt = alreadySeenPackets.find(_packet->flowId);
  if (srcIt != alreadySeenPackets.end()) {
    // I already received a packet from this source
    if (_packet->flowSequenceNumber > srcIt->second && _packet->type == PacketType::DATA) {
      // The source sequence number of this packet is higher than the highest already seen from this source.
      // I will update my data and forward this packet

      srcIt->second = _packet->flowSequenceNumber;

      PacketPtr packetClone(_packet->clone());
      struct backoffRoutingCounter info;
      info.p = packetClone;
      info.counter = 1;
      waitingPacket.insert(pair<int,backoffRoutingCounter>(packetClone->packetId,info));
      //             insertedPackets = true;
      Scheduler::getScheduler().schedule(new backoffSendingEvent(Scheduler::now() + backoffTime, hostNode,packetClone->packetId));
//             LogSystem::EventsLogOutput.log( LogSystem::memoryTrace, Scheduler::now(),hostNode->getId(),"+",_packet->flowId,_packet->flowSequenceNumber);
    }
  } else {
    // First time I receive a packet from this source
    // I will forward this packet
    //
    if (_packet->type == PacketType::DATA) {
      alreadySeenPackets.insert(pair<int,int>(_packet->flowId, _packet->flowSequenceNumber));
      PacketPtr packetClone(_packet->clone());
      struct backoffRoutingCounter info;
      info.p = packetClone;
      info.counter = 1;
      waitingPacket.insert(pair<int,backoffRoutingCounter>(packetClone->packetId,info));
      //             insertedPackets=true;
      //             cout << " node id " << hostNode->getId() << endl;
      Scheduler::getScheduler().schedule(new backoffSendingEvent(Scheduler::now() + backoffTime, hostNode,packetClone->packetId));
//             LogSystem::EventsLogOutput.log( LogSystem::memoryTrace, Scheduler::now(),hostNode->getId(),"+",_packet->flowId,_packet->flowSequenceNumber);
    }
  }


  //     if (insertedPackets){
  //       string filename = ScenarioParameters::getScenarioDirectory()+"/memoryTrace.data";
  //       ofstream memoryTrace(filename,std::ofstream::app);
  //       memoryTrace << ScenarioParameters::getDefaultBeta() <<" "<<hostNode->getId() <<  " + "<< Scheduler::now() << endl;
  //       memoryTrace.close();
  //     }
  //     if (erasedPackets){
  //       string filename = ScenarioParameters::getScenarioDirectory()+"/memoryTrace.data";
  //       ofstream memoryTrace(filename,std::ofstream::app);
  //       memoryTrace << ScenarioParameters::getDefaultBeta() <<" " << hostNode->getId() <<  " - "<< Scheduler::now() << endl;
  //       memoryTrace.close();
  //     }
}


void BackoffFloodingRoutingAgent::receivePacketFromApplication(PacketPtr _packet) {
  _packet->setSrcSequenceNumber(hostNode->getNextSrcSequenceNumber());
  hostNode->enqueueOutgoingPacket(_packet);

  if (_packet->type == PacketType::D1_DENSITY_INIT){
  }

  auto srcIt = alreadySeenPackets.find(_packet->srcId);   // source must not resend the packet
  if (srcIt != alreadySeenPackets.end()  && _packet->type == PacketType::DATA) {
    srcIt->second = _packet->srcSequenceNumber;
  } else if (_packet->type == PacketType::DATA){
    alreadySeenPackets.insert(pair<int,int>(_packet->srcId, _packet->srcSequenceNumber));
  }
}


void BackoffFloodingRoutingAgent::processBackoffSending(int packetId){
  auto it = waitingPacket.find(packetId);
  if ( it != waitingPacket.end() && it->second.p->type == PacketType::DATA ) {
    //
    // if the packet is still in the waiting list
    // i.e. I have not see more than REDUNDANCY copies
    // then I send it now
    //
    //cout << " ++ forward seq " << it->second.p->flowSequenceNumber << " by node " << hostNode->getId()<< " at " << Scheduler::getScheduler().now() << endl;
    hostNode->enqueueOutgoingPacket(it->second.p);
    LogSystem::EventsLogOutput.log( LogSystem::routingSND, Scheduler::now(),hostNode->getId(),(int) it->second.p->type,it->second.p->flowId,it->second.p->flowSequenceNumber);
//         LogSystem::EventsLogOutput.log( LogSystem::memoryTrace, Scheduler::now(),hostNode->getId(),"-",it->second.p->flowId,it->second.p->flowSequenceNumber);

    forwardedDataPackets++;
    waitingPacket.erase(it);
    //       string filename = ScenarioParameters::getScenarioDirectory()+"/dataSent.data";
    //       ofstream dataSent(filename,std::ofstream::app);
    //       dataSent<< ScenarioParameters::getDefaultBeta() << " " <<hostNode->getId() << " "<< Scheduler::now() << endl;
    //       dataSent.close();
    //
    //
    //       string filename2 = ScenarioParameters::getScenarioDirectory()+"/memoryTrace.data";
    //       ofstream memoryTrace(filename2,std::ofstream::app);
    //       memoryTrace << ScenarioParameters::getDefaultBeta() <<" " << hostNode->getId() <<  " - "<< Scheduler::now() << endl;
    //       memoryTrace.close();
  }
}



//==============================================================================
//
//          backoffSendingEvent  (class)
//
//==============================================================================

backoffSendingEvent::backoffSendingEvent(simulationTime_t _t, Node* _node,int _packetId) : Event(_t){

  eventType = EventType::BACKOFF_SENDING_EVENT;
  node = _node;
  packetId = _packetId;
  //         packet = _packet;
}

const string backoffSendingEvent::getEventName(){
  return "backoff Sending Event";
}

void backoffSendingEvent::consume() {
  //           cout<< " backoffSendingEvent::consume() BEGIN  on node "<< node->getId() << " for packet " << packetId << endl;
  node->getRoutingAgent()->processBackoffSending(packetId);
}
