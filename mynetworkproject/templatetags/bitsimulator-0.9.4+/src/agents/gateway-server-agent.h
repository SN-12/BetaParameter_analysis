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

#ifndef AGENTS_GATEWAY_SERVER_AGENT_H_
#define AGENTS_GATEWAY_SERVER_AGENT_H_

#include "server-application-agent.h"


// A Gateway to interface to a hypothetically connected IoT or Internet.

class GatewayServerAgent : public ServerApplicationAgent {
public:
  static void initializeAgent();
  static constexpr int Port = 9042; // over 9000!

  GatewayServerAgent(Node* hostNode);

  void receivePacket(PacketPtr packet);
};


#endif /* AGENTS_GATEWAY_SERVER_AGENT_H_ */
