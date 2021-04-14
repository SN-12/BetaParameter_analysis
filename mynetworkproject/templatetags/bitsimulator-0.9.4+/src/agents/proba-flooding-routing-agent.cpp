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
#include "scheduler.h"
#include "proba-flooding-routing-agent.h"

//==============================================================================
//
//          ProbaFloodingRoutingAgent  (class)
//
//==============================================================================

mt19937_64 *ProbaFloodingRoutingAgent::forwardingRNG =  new mt19937_64(2);
set<int> ProbaFloodingRoutingAgent::reachability = set<int>();

ProbaFloodingRoutingAgent::ProbaFloodingRoutingAgent(Node *_hostNode) : RoutingAgent(_hostNode) {
  type = RoutingAgentType::PROBA_FLOODING;
  //      aliveAgents++;
}

ProbaFloodingRoutingAgent::~ProbaFloodingRoutingAgent() {
  aliveAgents--;
  //   cout << " aliveAgents " << aliveAgents << endl;

  if ( aliveAgents == 0 ){
    //           TODO : add global information
    cout << " Proba flooding:: number of nodes reached : " << reachability.size() << endl;
    cout << " Proba flooding:: forwarded packets       : " << forwardedDataPackets << endl;
  }
}

void ProbaFloodingRoutingAgent::receivePacketFromNetwork(PacketPtr _packet) {
  //      cout << Scheduler::getScheduler().now() << " SimpleFloodingAgent::receivePacket on Node " << hostNode->getId();
  //      cout << " srcId:" << _packet->srcId << " srcSeq:" << _packet->srcSequenceNumber;
  //      cout << " dstId:" << _packet->dstId << " port:" << _packet->port ;
  //      cout << " size:" << _packet->size;
  //      cout << " flowId:" << _packet->flowId << " flowSeq:" << _packet->flowSequenceNumber << endl;
  //      cout << "SimpleFloodingAgent on node " << hostNode->getId() << " received srcSeq " << _packet->srcSequenceNumber << " de " << _packet->transmitterId << endl;
  //         getchar();

  // source must not resend the packet
  if ( _packet->srcId == hostNode->getId())
    return;

  bool fw = false;
  auto srcIt = alreadySeenPackets.find(_packet->srcId);
  if (srcIt != alreadySeenPackets.end()) {
    // already seen
    if (_packet->srcSequenceNumber > srcIt->second) {
      // the source sequence number of this packet is higher than the highest already seen from this source
      srcIt->second = _packet->srcSequenceNumber;
      fw = true;
    }
  } else {
    // seen for the first time
    alreadySeenPackets.insert(pair<int,int>(_packet->srcId, _packet->srcSequenceNumber));
    fw = true;
  }

  uniform_real_distribution<float> distrib(0, 1);
  float forwardingProbabilityNumber = distrib(*forwardingRNG);

//  int wantedForwarder = ScenarioParameters::getSlrBackoffredundancy();
//  cout << " wantedForwarder       =" << wantedForwarder << endl;

  float forwadingProbability;
//  if (hostNode->getEstimatedNeibhbours() == -1)
//    forwadingProbability = 1;
//  else {
//    forwadingProbability = ((float)wantedForwarder)/(float)hostNode->getEstimatedNeibhbours(); //avec deden
//  }
    forwadingProbability = 0.2;
//  cout << " forwadingProbability       =" << forwadingProbability << endl;

  if (fw && forwardingProbabilityNumber < forwadingProbability) {
    //PacketPtr ppacket(new Packet(_packet));
    PacketPtr packetClone(_packet->clone());
    hostNode->enqueueOutgoingPacket(packetClone);
    if (packetClone->type == PacketType::DATA)
      forwardedDataPackets++;
  }

  // packet arrives to destination (the destination can be unicast, zone
  // (SLR), or any node if broadcast to all network)
  // note that all the copies are sent upwards to application, it is up to
  // it to process them only once
  // note also that the destination too forwards the packet, useful in SLR
  // for ex., where all the zone should receive the packet
  if (_packet->dstId == hostNode->getId() || _packet->dstId == -1) {
    hostNode->dispatchPacketToApplication(_packet);
    reachability.insert(hostNode->getId());
    LogSystem::EventsLogOutput.log( LogSystem::dstReach, Scheduler::now(),hostNode->getId(),(int)_packet->type,_packet->flowId,_packet->flowSequenceNumber);
  }
}
