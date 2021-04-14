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

#ifndef AGENTS_SLR_DEVIATION_ROUTING_AGENT_H_
#define AGENTS_SLR_DEVIATION_ROUTING_AGENT_H_

#include "routing-agent.h"


//===========================================================================================================
//
//          DeviationRoutingAgent  (class)
//
//===========================================================================================================

class DeviationRoutingAgent: public RoutingAgent {

protected:
  static ofstream SLRPositionsFile;

  bool SLRForward(PacketPtr _packet);
  bool SLRForward(PacketPtr _packet, int m);
  bool SLRDestinationReach(PacketPtr _packet);
  bool isCloser(PacketPtr _packet);
  bool SLRUpdate(PacketPtr _packet);
  bool deviationForward(PacketPtr _packet);


  int SLRCoordX; // Anchor 0;
  int SLRCoordY; // Anchor 1;
  int SLRCoordZ; // Anchor 2;

  static int currentAnchorID;

  static mt19937_64 *forwardingRNG;
  bool initialisationStarted;

  bool isAck (PacketPtr _packet);

  bool deviation;

  // for backoff flooding

  int redundancy;
  int beaconRedundancy;

  struct SLRbackoffRoutingCounter {
    PacketPtr p;
    int counter;
  };

  map<int,SLRbackoffRoutingCounter> backoffWaitingPacket;

  bool dedenStarted = false;

public:
  DeviationRoutingAgent(Node *_hostNode);
  virtual ~DeviationRoutingAgent();

  static void initializeAgent();

  virtual void receivePacketFromNetwork(PacketPtr _packet);
  virtual void receivePacketFromApplication(PacketPtr _packet);

  map<int,int> alreadyForwardedPackets;
  int anchorID;

  int getSLRX() { return(SLRCoordX); }
  int getSLRY() { return(SLRCoordY); }
  int getSLRZ() { return(SLRCoordZ); }

  void setSLRX(int val) { SLRCoordX = val; }
  void setSLRY(int val) { SLRCoordY = val; }
  void setSLRZ(int val) { SLRCoordZ = val; }
  void processPacketGenerationEvent();

  void processSLRBackoffSending(int packetId);
};

//===========================================================================================================
//
//          SLRbackoffSendingEvent2  (class)
//
//===========================================================================================================

class SLRbackoffSendingEvent2: public Event {
private:

public:
  SLRbackoffSendingEvent2(simulationTime_t _t, Node* _node, int packetId);

  const string getEventName();
  void consume();
  Node* node;
  int packetId;
};



#endif /* AGENTS_SLR_DEVIATION_ROUTING_AGENT_H_ */
