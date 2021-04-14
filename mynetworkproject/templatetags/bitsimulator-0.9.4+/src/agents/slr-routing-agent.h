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

#ifndef AGENTS_SLR_ROUTING_AGENT_H_
#define AGENTS_SLR_ROUTING_AGENT_H_

#include "routing-agent.h"


//===========================================================================================================
//
//          SLRRoutingAgent  (class)
//
//===========================================================================================================

class SLRRoutingAgent: public RoutingAgent {

protected:
  static ofstream SLRPositionsFile;

  bool SLRForward(PacketPtr _packet);
  bool SLRForward(PacketPtr _packet, int m);
  bool SLRDestinationReach(PacketPtr _packet);
  bool isCloser(PacketPtr _packet);
  bool SLRUpdate(PacketPtr _packet);


  int SLRCoordX; // Anchor 0;
  int SLRCoordY; // Anchor 1;
  int SLRCoordZ; // Anchor 2;

  static int currentAnchorID;

  static mt19937_64 *forwardingRNG;
  bool initialisationStarted;

  static int forwardedSLRBeacons;


public:
  SLRRoutingAgent(Node *_hostNode);
  virtual ~SLRRoutingAgent();

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
};


#endif /* AGENTS_SLR_ROUTING_AGENT_H_ */
