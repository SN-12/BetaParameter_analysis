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
#include "pure-flooding-routing-agent.h"


//==============================================================================
//
//          PureFloodingRoutingAgent  (class)
//
//==============================================================================

set<int> PureFloodingRoutingAgent::reachability = set<int>();

PureFloodingRoutingAgent::PureFloodingRoutingAgent(Node *_hostNode) : RoutingAgent(_hostNode) {
  type = RoutingAgentType::PURE_FLOODING;
  //      aliveAgents++;
}

PureFloodingRoutingAgent::~PureFloodingRoutingAgent() {
  aliveAgents--;
  //   cout << " aliveAgents " << aliveAgents << endl;

  if ( aliveAgents == 0 ){
    //           TODO : add global information
    cout << " Pure flooding:: number of nodes reached : " << reachability.size() << endl;
    cout << " Pure flooding:: forwarded packets       : " << forwardedDataPackets << endl;

    //           string filename = ScenarioParameters::getScenarioDirectory()+"/reachability_cost.data";
    //           ofstream file(filename,std::ofstream::app);
    //           file << forwardedDataPackets << " " ;
    //           file.close();
  }
}

void PureFloodingRoutingAgent::receivePacketFromNetwork(PacketPtr _packet) {
  //     LogSystem::EventsLogOutput.log( LogSystem::routingRCV, Scheduler::now(),hostNode->getId(),(int) _packet->type,_packet->flowId,_packet->flowSequenceNumber, _packet->modifiedBitsPositions.size());

  //      cout << Scheduler::getScheduler().now() << " PureFloodingRoutingAgent::receivePacket on Node " << hostNode->getId();
  //      cout << " srcId:" << _packet->srcId << " srcSeq:" << _packet->srcSequenceNumber;
  //      cout << " dstId:" << _packet->dstId << " port:" << _packet->port ;
  //      cout << " size:" << _packet->size;
  //      cout << " flowId:" << _packet->flowId << " flowSeq:" << _packet->flowSequenceNumber << endl;
  //      cout << "PureFloodingRoutingAgent on node " << hostNode->getId() << " received srcSeq " << _packet->srcSequenceNumber << " de " << _packet->transmitterId << endl;
  //         getchar();

  // source must not resend the packet
  if ( _packet->srcId == hostNode->getId())
    return;

  bool fw = false;
  auto srcIt = alreadySeenPackets.find(_packet->srcId);
  if (srcIt != alreadySeenPackets.end()) {
    // already seen
    // DEDeN probes are not forwarded
    if (_packet->srcSequenceNumber > srcIt->second && _packet->type != PacketType::D1_DENSITY_PROBE) {
      // the source sequence number of this packet is higher than the highest already seen from this source
      srcIt->second = _packet->srcSequenceNumber;
      fw = true;
    }
  } else {
    // DEDeN probes are not forwarded
    if (_packet->type != PacketType::D1_DENSITY_PROBE) {
      // seen for the first time
      alreadySeenPackets.insert(pair<int,int>(_packet->srcId, _packet->srcSequenceNumber));
      fw = true;
    }
  }

  if (fw) {
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
