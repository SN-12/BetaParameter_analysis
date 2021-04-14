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

#ifndef AGENTS_DATASINK_APPLICATION_AGENT_H_
#define AGENTS_DATASINK_APPLICATION_AGENT_H_

#include "server-application-agent.h"


//===========================================================================================================
//
//          DataSinkApplicationAgent  (class)
//
//===========================================================================================================

class DataSinkApplicationAgent : public ServerApplicationAgent {
protected:
  int nbPacketsReceived;
  int nbPacketsCollisions;
  int receivedCorruptedBits;

  static int totalPacketsReceived;
  static int totalPacketsCollisions;
  static int totalCorruptedBits;

  simulationTime_t loggingInterval;

  int packetsReceivedDuringInterval;
  int bitsReceivedDuringInterval;

  vector<pair<int,int>> idPacketReceived;
  map <int,simulationTime_t> packetsDelay;

  static map <int,int> globalIdReceived;
  static int aliveAgents;
  static int maxAliveAgents;

  static set<int> reachability;
  static map<int,int> histoBitsCollisions;

  //  static int nodeCompletion;
  //  static simulationTime_t completionTime;

public:
  DataSinkApplicationAgent(Node *_hostNode);
  virtual ~DataSinkApplicationAgent();

  virtual void receivePacket(PacketPtr _packet);

  void startLogging(simulationTime_t _interval);
  void stopLogging();
  void processDataSinkLogEvent();
};

#endif /* AGENTS_DATASINK_APPLICATION_AGENT_H_ */
