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

#ifndef AGENTS_CBR_APPLICATION_AGENT_H_
#define AGENTS_CBR_APPLICATION_AGENT_H_

#include "application-agent.h"


//==============================================================================
//
//          CBRApplicationAgent  (class)
//
//==============================================================================

class CBRApplicationAgent : public ApplicationAgent {
protected:
  int flowId;
  int size;
  int dstId;
  int port;
  PacketType packetType;
  simulationTime_t interval;
  int repetitions;         // number of packets to send, -1 for infinite
  int flowSequenceNumber;  // number of packets already sent by this generator
  int beta;

public:
  CBRApplicationAgent(Node *_hostNode, int _flowId, int _size, int _dstId, int _port, PacketType _packetType, simulationTime_t _interval, int _repetitions, int _beta);
  virtual ~CBRApplicationAgent();

  static void initializeAgent();

  void processPacketGenerationEvent();
};

#endif /* AGENTS_CBR_APPLICATION_AGENT_H_ */
