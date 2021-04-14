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

#include <iostream>
#include <cassert>
#include "manual-routing-agent.h"
using namespace std;


//==============================================================================
//
//          ManualRoutingAgent  (class)
//
//==============================================================================

multimap<int, int> ManualRoutingAgent::forwardingRulesMap;

ManualRoutingAgent::ManualRoutingAgent(Node *_hostNode) : RoutingAgent(_hostNode) {
  for ( auto it = forwardingRulesMap.find(_hostNode->getId()); it != forwardingRulesMap.end() && it->first == _hostNode->getId(); it++ ) {
    cout << "ManualRouingAgent on node " << _hostNode->getId() << "  add forwarding for flow " << it->second << endl;
  }
}

void ManualRoutingAgent::initializeAgent() {
  tinyxml2::XMLElement *manualRoutingElement = ScenarioParameters::getXMLRootNode()->FirstChildElement("routingAgentsConfig")->FirstChildElement("ManualRouting");
  if (manualRoutingElement != nullptr) {
    tinyxml2::XMLElement *rule = manualRoutingElement->FirstChildElement("rule");
    while (rule != nullptr) {
      int nodeId, flowId;
      ScenarioParameters::queryIntAttr(rule, "node", nodeId, true, nullptr, 0, "no \"rule\" attribute in element <manualRouting>");
      ScenarioParameters::queryIntAttr(rule, "forwardFlow", flowId, true, nullptr, 0, "no \"flowId\" attribute in element <manualRouting>");
      //cout << "RULE  node:" << nodeId << "  flow:" << flowId << endl;
      forwardingRulesMap.insert(pair<int,int>(nodeId, flowId));
      rule = rule->NextSiblingElement("rule");
    }
  }
}

void ManualRoutingAgent::receivePacketFromNetwork(PacketPtr _packet) {
  //    cout << Scheduler::getScheduler().now() << " NoRoutingAgent::receivePacket on Node " << hostNode->getId();
  //    cout << " srcId:" << _packet->srcId << " srcSeq:" << _packet->srcSequenceNumber;
  //    cout << " dstId:" << _packet->dstId << " port:" << _packet->port ;
  //    cout << " size:" << _packet->size;
  //    cout << " flowId:" << _packet->flowId << " flowSeq:" << _packet->flowSequenceNumber << endl;

  hostNode->dispatchPacketToApplication(_packet);
}
