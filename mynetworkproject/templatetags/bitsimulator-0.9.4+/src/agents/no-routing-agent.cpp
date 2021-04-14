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
#include "no-routing-agent.h"
using namespace std;

//==============================================================================
//
//          NoRoutingAgent  (class)
//
//==============================================================================

NoRoutingAgent::NoRoutingAgent(Node *_hostNode) : RoutingAgent(_hostNode) {
  type = RoutingAgentType::NO_ROUTING;
}

void NoRoutingAgent::receivePacketFromNetwork(PacketPtr _packet) {
  // cout << Scheduler::getScheduler().now() << " NoRoutingAgent::receivePacket on Node " << hostNode->getId();
  // cout << " srcId:" << _packet->srcId << " srcSeq:" << _packet->srcSequenceNumber;
  // cout << " dstId:" << _packet->dstId << " port:" << _packet->port ;
  // cout << " size:" << _packet->size;
  // cout << " flowId:" << _packet->flowId << " flowSeq:" << _packet->flowSequenceNumber << endl;
  hostNode->dispatchPacketToApplication(_packet);
}
