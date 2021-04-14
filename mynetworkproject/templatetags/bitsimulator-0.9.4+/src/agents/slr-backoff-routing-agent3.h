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

#ifndef SLRBACKOFF_H_
#define SLRBACKOFF_H_

#include "agents/routing-agent.h"

//==============================================================================
//
//          SLRBackoffRoutingAgent  (class)
//
//==============================================================================

class SLRBackoffRoutingAgent : public RoutingAgent {

protected:
  static ofstream SLRPositionsFile;

  bool SLRForward(PacketPtr _packet);
  bool SLRForward(PacketPtr _packet, int m);
  bool SLRDestinationReach(PacketPtr _packet);
  bool isCloser(PacketPtr _packet);
  bool implicitAck(PacketPtr _packet);
  bool SLRUpdate(PacketPtr _packet);

  int SLRCoordX; // Anchor 0;
  int SLRCoordY; // Anchor 1;
  int SLRCoordZ; // Anchor 2;

  static int currentAnchorID;
  static int slrBeaconCounter;

  static int totalPacketsReceived;
  static int totalPacketsCollisions;
  static int totalAlteredBits;

  static mt19937_64 *forwardingRNG;
  bool initialisationStarted;

  //         map<int,int> alreadySeenPackets;

  struct SLRbackoffRoutingCounter {
    PacketPtr p;
    int counter;
  };

  map<int,SLRbackoffRoutingCounter> waitingPacket;
  float dataBackoffMultiplier;
  float beaconBackoffMultiplier;
  int redundancy;
  int beaconRedundancy;

  void processSLRBackoffSending(int packetId);

public:
  SLRBackoffRoutingAgent(Node *_hostNode);
  virtual ~SLRBackoffRoutingAgent();

  static void initializeAgent();

  virtual void receivePacketFromNetwork(PacketPtr _packet);
  virtual void receivePacketFromApplication(PacketPtr _packet);

  map<int,set<int>> alreadyForwardedPackets;
  int anchorID;

  int getSLRX() { return(SLRCoordX); }
  int getSLRY() { return(SLRCoordY); }
  int getSLRZ() { return(SLRCoordZ); }

  void setSLRX(int val) { SLRCoordX = val; }
  void setSLRY(int val) { SLRCoordY = val; }
  void setSLRZ(int val) { SLRCoordZ = val; }
  void processPacketGenerationEvent();
};


//===========================================================================================================
//
//          SLRBackoffInitialisationGenerationEvent  (class)
//
//===========================================================================================================

class SLRBackoffInitialisationGenerationEvent: public Event {
private:
  SLRBackoffRoutingAgent *concernedGenerator;
public:
  SLRBackoffInitialisationGenerationEvent(simulationTime_t _t, SLRBackoffRoutingAgent* _gen);

  const string getEventName();
  void consume();
};


//===========================================================================================================
//
//          SLRbackoffSendingEvent  (class)
//
//===========================================================================================================

class SLRbackoffSendingEvent: public Event {
private:

public:
  SLRbackoffSendingEvent(simulationTime_t _t, Node* _node, int packetId);

  const string getEventName();
  void consume();
  Node* node;
  int packetId;
};

#endif /* SLRBACKOFF_H_ */
