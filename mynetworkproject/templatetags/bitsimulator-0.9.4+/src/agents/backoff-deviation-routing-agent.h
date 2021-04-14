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

#ifndef AGENTS_BACKOFF_DEVIATION_ROUTING_AGENT_H_
#define AGENTS_BACKOFF_DEVIATION_ROUTING_AGENT_H_

#include <vector>
#include <map>
#include <utility>

#include "events.h"
#include "routing-agent.h"


/*
 * BackoffDeviationRoutingAgent
 *
 * This Agent combines SLR Backoff Flooding (from SLRBackoffRoutingAgent3) with
 * a dynamic congestion detection and deviation algorithm.
 */
class BackoffDeviationRoutingAgent : public RoutingAgent {
public:
  static void initializeAgent();
  int slrx;
  int slry;
  int slrz;

private:
  static ofstream slrPositionsFile;
  static mt19937_64 forwardDelayRnd;

  static int redundancy;
  static int deviateThresh;
  static int convergeThresh;

  bool iAmAnchor;
  bool beaconPhaseStarted;

  // store packetID -> (packet, backoffCount)
  map<int, std::pair<PacketPtr, int>> delayedPackets;

  // note: this is rather unrealistic, in that it stores much more
  // than probably possible for nanobots.
  vector<std::pair<int,int>> alreadyForwardedPackets;

  bool slrUpdate(PacketPtr packet);
  bool shouldSlrForward(PacketPtr packet, int m);

  void resendPacket(PacketPtr packet, bool inBeaconPhase);
  bool alreadyForwarding(PacketPtr packet);
  bool areWeCloserThanPacketSender(PacketPtr packet);

public:
  BackoffDeviationRoutingAgent(Node* host);
  ~BackoffDeviationRoutingAgent();

  void receivePacketFromNetwork(PacketPtr packet);
  void receivePacketFromApplication(PacketPtr packet);

  void StartPropagationMessage();
  void sendPacketDelayed(int packetId);
};


#endif /* AGENTS_BACKOFF_DEVIATION_ROUTING_AGENT_H_ */
