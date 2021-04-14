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

#ifndef AGENTS_SLR_BACKOFF_ROUTING_AGENT3_H_
#define AGENTS_SLR_BACKOFF_ROUTING_AGENT3_H_

#include "routing-agent.h"


//===========================================================================================================
//
//          SLRBackoffRoutingAgent3  (class)
//
//===========================================================================================================


class SLRBackoffRoutingAgent3 : public RoutingAgent {

protected:
  static ofstream SLRPositionsFile;

  //True if the receiving node is locate on the SLR curbe of width m
  bool SLRForward(PacketPtr _packet, int m);


  //True if the packet has reached the destination node
  bool SLRNodeReach(PacketPtr _packet);
  //True if the packet has reached the destination zone
  bool SLRZoneReach(PacketPtr _packet);

  //True if the current node isCloser from the destination than the last forwarder
  bool isCloser(PacketPtr _packet);

  //True if the received packet can ack a previous send packet
  bool implicitAck(PacketPtr _packet);

  //Perfom an SLR update when receiving an SLR beacon if necessary and return true if it has been done.
  bool SLRUpdate(PacketPtr _packet);

  int SLRCoordX; // Anchor 0;
  int SLRCoordY; // Anchor 1;
  int SLRCoordZ; // Anchor 2;

  bool dedenStarted;

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
  SLRBackoffRoutingAgent3(Node *_hostNode);
  virtual ~SLRBackoffRoutingAgent3();

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

  //True if the receiving node is locate on the SLR curbe of width 1
  // Call the previous function
  bool SLRForward(PacketPtr _packet);

};


//===========================================================================================================
//
//          SLRBackoffInitialisationGenerationEvent3  (class)
//
//===========================================================================================================

class SLRBackoffInitialisationGenerationEvent3: public Event {
private:
  SLRBackoffRoutingAgent3 *concernedGenerator;
public:
  SLRBackoffInitialisationGenerationEvent3(simulationTime_t _t, SLRBackoffRoutingAgent3* _gen);

  const string getEventName();
  void consume();
};


//===========================================================================================================
//
//          SLRbackoffSendingEvent3  (class)
//
//===========================================================================================================

class SLRbackoffSendingEvent3: public Event {
private:

public:
  SLRbackoffSendingEvent3(simulationTime_t _t, Node* _node, int packetId);

  const string getEventName();
  void consume();
  Node* node;
  int packetId;
};


#endif /* AGENTS_SLR_BACKOFF_ROUTING_AGENT3_H_ */
