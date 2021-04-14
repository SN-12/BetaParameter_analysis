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

#ifndef AGENTS_BACKOFF_FLOODING_ROUTING_AGENT_H_
#define AGENTS_BACKOFF_FLOODING_ROUTING_AGENT_H_

#include <random>
#include "routing-agent.h"


//==============================================================================
//
//          BackoffFloodingRoutingAgent  (class)
//
//==============================================================================


class BackoffFloodingRoutingAgent  : public RoutingAgent {
protected:
  static mt19937_64 *forwardingRNG;
  map<int,int> alreadySeenPackets;
  //         time_t backoffWindow = 5000000000 ;
  time_t backoffWindow;
  bool initialisationStarted;

  struct backoffRoutingCounter {
    PacketPtr p;
    int counter;
  };

  map<int,backoffRoutingCounter> waitingPacket;
  static set<int> reachability;

  static float dataBackoffMultiplier;
  static int redundancy;

public:
  BackoffFloodingRoutingAgent  (Node *_hostNode);
  virtual ~BackoffFloodingRoutingAgent  ();

  static void initializeAgent();

  virtual void receivePacketFromNetwork(PacketPtr _packet);
  virtual void receivePacketFromApplication(PacketPtr _packet);

  virtual void processBackoffSending(int packetId);

  //         void processPacketGenerationEvent();
};



//==============================================================================
//
//          backoffSendingEvent  (class)
//
//==============================================================================

class backoffSendingEvent : public Event {
public:
  backoffSendingEvent(simulationTime_t _t, Node* _node, int packetId);

  const std::string getEventName();
  void consume();
  Node* node;
  int packetId;
};

#endif /* AGENTS_BACKOFF_FLOODING_ROUTING_AGENT_H_ */
