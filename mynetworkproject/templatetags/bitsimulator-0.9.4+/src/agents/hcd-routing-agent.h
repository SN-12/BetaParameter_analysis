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

#ifndef AGENTS_HCD_ROUTING_AGENT_H_
#define AGENTS_HCD_ROUTING_AGENT_H_

#include "routing-agent.h"


//===========================================================================================================
//
//          HCDRouting  (class)
//
//===========================================================================================================

class HCDRoutingAgent: public RoutingAgent {

protected:
  static ofstream HCDPositionsFile;

  bool HCDForward(PacketPtr _packet);
  bool HCDForward(PacketPtr _packet, int m);
  bool HCDDestinationReach(PacketPtr _packet);
  bool isCloser(PacketPtr _packet);
  bool HCDUpdate(PacketPtr _packet);

  int HCDCoordX; // Anchor 0;
  int HCDCoordY; // Anchor 1;
  int HCDCoordZ; // Anchor 2;

  static int currentAnchorID;

  static mt19937_64 *forwardingRNG;
  bool initialisationStarted;

  static int forwardedHCDBeacons;


public:
  HCDRoutingAgent(Node *_hostNode);
  virtual ~HCDRoutingAgent();

  static void initializeAgent();

  virtual void receivePacketFromNetwork(PacketPtr _packet);
  virtual void receivePacketFromApplication(PacketPtr _packet);

  map<int,int> alreadyForwardedPackets;
  int anchorID;

  int getSLRX() { return(HCDCoordX); }
  int getSLRY() { return(HCDCoordY); }
  int getSLRZ() { return(HCDCoordZ); }

  void setSLRX(int val) { HCDCoordX = val; }
  void setSLRY(int val) { HCDCoordY = val; }
  void setSLRZ(int val) { HCDCoordZ = val; }
  void processPacketGenerationEvent();

};


#endif /* AGENTS_HCD_ROUTING_AGENT_H_ */
