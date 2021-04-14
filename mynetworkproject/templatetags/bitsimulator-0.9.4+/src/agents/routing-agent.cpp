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

#include "routing-agent.h"


//===========================================================================================================
//
//          RoutingAgent  (class)
//
//===========================================================================================================

int RoutingAgent::aliveAgents = 0;
int RoutingAgent::forwardedDataPackets = 0;

RoutingAgent::RoutingAgent(Node *_hostNode): hostNode(_hostNode) {
  type = RoutingAgentType::GENERIC;
  aliveAgents++;
}

RoutingAgent::~RoutingAgent() {
  //     aliveAgents--;
}

void RoutingAgent::receivePacketFromApplication(PacketPtr _packet) {
  _packet->setSrcSequenceNumber(hostNode->getNextSrcSequenceNumber());
  hostNode->enqueueOutgoingPacket(_packet);
}

void RoutingAgent::processAckTimedOut(PacketPtr /* _packet */) {
  cout << " Routing Agent processAckTimedOut" << endl; /*getchar();*/
}


void RoutingAgent::processBackoffSending(int /* packetId */)
{
  cout << " Routing Agent processBackoffSending" << endl;
}

void RoutingAgent::processSLRBackoffSending(int /* packetId */)
{
  cout << " Routing Agent processSLRBackoffSending" << endl;
}
