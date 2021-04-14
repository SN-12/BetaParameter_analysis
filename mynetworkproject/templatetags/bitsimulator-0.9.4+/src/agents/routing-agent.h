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

#ifndef AGENTS_ROUTING_AGENT_H_
#define AGENTS_ROUTING_AGENT_H_

#include "node.h"


enum class RoutingAgentType {
  GENERIC,
  NO_ROUTING,
  PURE_FLOODING,
  BACKOFF_FLOODING,
  SLR,
  HCD,
  SLR_BACKOFF,
  PROBA_FLOODING,
  SLEEPING,
  CONFIDENCE,
  PURE_FLOODING_RING,
  BACKOFF_FLOODING_RING,
  PROBA_FLOODING_RING,
  SLR_RING
};


//==============================================================================
//
//          RoutingAgent  (class)
//
//==============================================================================

class RoutingAgent {
protected:
  Node *hostNode;
  static int aliveAgents;
  static int forwardedDataPackets;
public:
  RoutingAgentType type;
  static void initializeAgent() { cout << "Initializing RoutingAgent" << endl; }
  RoutingAgent(Node *_hostNode);
  virtual ~RoutingAgent();

  virtual void receivePacketFromNetwork(PacketPtr _packet) = 0;
  virtual void receivePacketFromApplication(PacketPtr _packet);
  virtual void processAckTimedOut(PacketPtr _packet);
  virtual void processBackoffSending(int packetId);
  virtual void processSLRBackoffSending(int packetId);
};

#endif /* AGENTS_ROUTING_AGENT_H_ */
