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

#ifndef AGENTS_SERVER_APPLICATION_AGENT_H_
#define AGENTS_SERVER_APPLICATION_AGENT_H_

#include "application-agent.h"


//===========================================================================================================
//
//          ServerApplicationAgent  (class)
//
//===========================================================================================================

class ServerApplicationAgent : public ApplicationAgent {
public:
  ServerApplicationAgent(Node *_hostNode);
  virtual ~ServerApplicationAgent();

  virtual void receivePacket(PacketPtr _packet) = 0;
};


#endif /* AGENTS_SERVER_APPLICATION_AGENT_H_ */
