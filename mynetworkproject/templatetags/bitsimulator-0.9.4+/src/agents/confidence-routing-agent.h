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

#ifndef AGENTS_CONFIDENCE_AGENT_H_
#define AGENTS_CONFIDENCE_AGENT_H_

#include "routing-agent.h"


// Routing based on confidence in messages.

class ConfidenceRoutingAgent : public RoutingAgent {
private:
  int forwards = 2;
  bool recup = false;
  simulationTime_t recupStart = -1;
  static constexpr simulationTime_t recupDuration = 100000000; // 100 ns

  int overall = 200;

  void trySendPacket(PacketPtr packet);

public:
  ConfidenceRoutingAgent(Node* hostNode);

  void receivePacketFromNetwork(PacketPtr packet);
  void receivePacketFromApplication(PacketPtr packet);
};

#endif /* AGENTS_CONFIDENCE_AGENT_H_ */
