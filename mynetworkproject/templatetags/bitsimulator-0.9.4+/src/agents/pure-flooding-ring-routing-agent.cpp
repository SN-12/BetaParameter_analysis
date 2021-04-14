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
#include "pure-flooding-ring-routing-agent.h"


//==============================================================================
//
//          PureFloodingRingRoutingAgent  (class)
//
//==============================================================================

set<int> PureFloodingRingRoutingAgent::reachability = set<int>();

PureFloodingRingRoutingAgent::PureFloodingRingRoutingAgent(Node *_hostNode) : RoutingAgent(_hostNode) {
  type = RoutingAgentType::PURE_FLOODING_RING;
  //      aliveAgents++;
  alreadySent1and2 = false;
  range1 = 800000;
  range2 = 700000;
}

PureFloodingRingRoutingAgent::~PureFloodingRingRoutingAgent() {
  aliveAgents--;
  //   cout << " aliveAgents " << aliveAgents << endl;

  if ( aliveAgents == 0 ){
    //           TODO : add global information
    cout << " Pure flooding with ring:: number of nodes reached : " << reachability.size() << endl;
    cout << " Pure flooding with ring:: forwarded packets       : " << forwardedDataPackets << endl;

    //           string filename = ScenarioParameters::getScenarioDirectory()+"/reachability_cost.data";
    //           ofstream file(filename,std::ofstream::app);
    //           file << forwardedDataPackets << " " ;
    //           file.close();
  }
}




void PureFloodingRingRoutingAgent::receivePacketFromApplication(PacketPtr _packet) { //on source

	if (!alreadySent1and2) { //1 and 2 before the very first data transmission
		PacketPtr control1(new Packet(PacketType::CONTROL_1, 101,hostNode->getId(),-1,-1,-1,-1));
		//control1->setSrcSequenceNumber(hostNode->getNextSrcSequenceNumber()); 
		PacketPtr control2(new Packet(PacketType::CONTROL_2, 102,hostNode->getId(),-1,-1,-1,-1));
		//control2->setSrcSequenceNumber(hostNode->getNextSrcSequenceNumber());

		control1->setCommunicationRange(range1);
  		hostNode->enqueueOutgoingPacket(control1);

		control2->setCommunicationRange(range2);
		hostNode->enqueueOutgoingPacket(control2);
		
		alreadySent1and2 = true;	
	}

	_packet->setSrcSequenceNumber(hostNode->getNextSrcSequenceNumber());
	hostNode->enqueueOutgoingPacket(_packet);   
}


void PureFloodingRingRoutingAgent::receivePacketFromNetwork(PacketPtr _packet) {
  //     LogSystem::EventsLogOutput.log( LogSystem::routingRCV, Scheduler::now(),hostNode->getId(),(int) _packet->type,_packet->flowId,_packet->flowSequenceNumber, _packet->modifiedBitsPositions.size());

  //      cout << Scheduler::getScheduler().now() << " PureFloodingRingRoutingAgent::receivePacket on Node " << hostNode->getId();
  //      cout << " srcId:" << _packet->srcId << " srcSeq:" << _packet->srcSequenceNumber;
  //      cout << " dstId:" << _packet->dstId << " port:" << _packet->port ;
  //      cout << " size:" << _packet->size;
  //      cout << " flowId:" << _packet->flowId << " flowSeq:" << _packet->flowSequenceNumber << endl;
  //      cout << "PureFloodingRingRoutingAgent on node " << hostNode->getId() << " received srcSeq " << _packet->srcSequenceNumber << " de " << _packet->transmitterId << endl;
  //         getchar();

  // source must not resend the packet
  if ( _packet->srcId == hostNode->getId())
    return;

  bool fw = false;
  auto srcIt = alreadySeenPackets.find(_packet->srcId);
  if (srcIt != alreadySeenPackets.end()) {
    // already seen
 
    if (_packet->srcSequenceNumber > srcIt->second && _packet->type == PacketType::DATA) {
      // the source sequence number of this packet is higher than the highest already seen from this source
      srcIt->second = _packet->srcSequenceNumber;
      fw = true;
    }
  } else {

    if (_packet->type == PacketType::DATA) {
      // seen for the first time
      alreadySeenPackets.insert(pair<int,int>(_packet->srcId, _packet->srcSequenceNumber));
      fw = true;
    }
  }

  if (_packet->type == PacketType::CONTROL_1 || _packet->type == PacketType::CONTROL_2) {
	fw = true;
}

  if (fw) {
    forwardPacketIfOnRing(_packet);
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

void PureFloodingRingRoutingAgent::forwardPacketIfOnRing( PacketPtr _packet ) {

	auto nodeIt = ring.find( _packet->transmitterId );

	if (nodeIt == ring.end()) {
		// first time we see a packet coming from this node	
		Control12_t control;
		control.control1 = false;
		control.control2 = false;

		if ( _packet->type == PacketType::CONTROL_1 ) control.control1 = true;
		if ( _packet->type == PacketType::CONTROL_2 ) control.control2 = true;

		ring.insert(pair<int,Control12_t>(_packet->transmitterId, control));
		// Even if it is a DATA packet, we don't forward it as we did not previously see CONTROL1
	} else {
		// we have already seen a packet coming from this node
		if ( _packet->type == PacketType::CONTROL_1 ) {
			nodeIt->second.control1 = true;
		} else if ( _packet->type == PacketType::CONTROL_2 ) {
			nodeIt->second.control2 = true;
		}else if ( nodeIt->second.control1 && !nodeIt->second.control2 ) {
			if (!alreadySent1and2) { //1 and 2 before the very first data transmission
				PacketPtr control1(new Packet(PacketType::CONTROL_1, 101,hostNode->getId(),-1,-1,-1,-1));   
				PacketPtr control2(new Packet(PacketType::CONTROL_2, 102,hostNode->getId(),-1,-1,-1,-1));

				control1->setCommunicationRange(range1);
  				hostNode->enqueueOutgoingPacket(control1);

				control2->setCommunicationRange(range2);
				hostNode->enqueueOutgoingPacket(control2);
			
				alreadySent1and2 = true;	
			}
			
			PacketPtr packetClone(_packet->clone());
			hostNode->enqueueOutgoingPacket(packetClone);
			if (packetClone->type == PacketType::DATA ){
			       	//cout << " je forward de la data " << endl;
			       	forwardedDataPackets++;
			}		
		}
	}
//	cerr << "eugen " << this << " " << _packet->transmitterId << " " << nodeIt->second.control1 << "," << nodeIt->second.control2 << endl;
}
