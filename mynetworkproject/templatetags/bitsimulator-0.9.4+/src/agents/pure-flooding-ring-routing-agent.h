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

#ifndef AGENTS_PURE_FLOODING_RING_ROUTING_AGENT_H_
#define AGENTS_PURE_FLOODING_RING_ROUTING_AGENT_H_

#include <random>
#include "routing-agent.h"


//===========================================================================================================
//
//          PureFloodingRingRoutingAgent  (class)
//
//===========================================================================================================
typedef struct {
	bool control1;
	bool control2;
} Control12_t;
class PureFloodingRingRoutingAgent : public RoutingAgent {
protected:
  map<int,int> alreadySeenPackets;
  map<int, Control12_t> ring;     //transmitterId (hostnode), received control1 or not,received control2 or not    //int, bool, bool
                                //in multimap we insert key,value1 then key,value2... which we does not want, we need this order key,value1,value2
                                //a map with list of values is better i think
 


  static mt19937_64 *forwardingRNG;
  static set<int> reachability;

  bool alreadySent1and2;
  distance_t range1, range2;


public:
  PureFloodingRingRoutingAgent(Node *_hostNode);
  virtual ~PureFloodingRingRoutingAgent();

  static void initializeAgent();

  virtual void receivePacketFromNetwork(PacketPtr _packet);
  virtual void receivePacketFromApplication(PacketPtr _packet);
  void forwardPacketIfOnRing( PacketPtr _packet );
};

#endif /* AGENTS_PURE_FLOODING_RING_ROUTING_AGENT_H_ */
