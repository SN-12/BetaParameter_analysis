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

#include "scheduler.h"

#include "confidence-routing-agent.h"


// confidence routing.

ConfidenceRoutingAgent::ConfidenceRoutingAgent(Node* hostNode)
  : RoutingAgent(hostNode) {
  type = RoutingAgentType::CONFIDENCE;
}

void ConfidenceRoutingAgent::receivePacketFromNetwork(PacketPtr packet) {
  // let the application know.
  hostNode->dispatchPacketToApplication(packet);

  trySendPacket(packet);
}


void ConfidenceRoutingAgent::receivePacketFromApplication(PacketPtr packet) {
  trySendPacket(packet);
}



void ConfidenceRoutingAgent::trySendPacket(PacketPtr packet) {
  if (overall <= 0) {
    // cout << "exhausted overall.\n";
    return;
  }

  if (recup) {
    if (Scheduler::now() > recupDuration + recupStart) {
      recup = false;
      forwards = 2;
    }
    else {
      // cout << "nah, not finished recuping.\n";
      return;
    }
  }


  if (forwards <= 0) {
    recupStart = Scheduler::now();
    recup = true;
    // cout << "too much forwarding, recuping.\n";
    return;
  }

  forwards--;
  overall--;

  // cout << "sending.\n";

  // just for fun, forward everything.
  hostNode->enqueueOutgoingPacket(packet);
}
