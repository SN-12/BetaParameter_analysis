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

#include "world.h"
#include "gateway-server-agent.h"


// Gateway servers.

void GatewayServerAgent::initializeAgent() {
  tinyxml2::XMLElement *XMLRootNode = ScenarioParameters::getXMLRootNode();

  tinyxml2::XMLElement *applicationAgentsConfigElement =
    XMLRootNode->FirstChildElement("applicationAgentsConfig");
  if (applicationAgentsConfigElement == nullptr) {
    return;
  }

  tinyxml2::XMLElement *gatewayEl =
    applicationAgentsConfigElement->FirstChildElement("GatewayServerAgent");
  if (gatewayEl == nullptr) {
    return;
  }

  for (tinyxml2::XMLElement* child =
         gatewayEl->FirstChildElement("Gateway");
       child != nullptr;
       child = child->NextSiblingElement("Gateway")) {

    int nodeId = 0;
    tinyxml2::XMLError r = child->QueryIntAttribute("nodeId", &nodeId);
    if (r != tinyxml2::XML_SUCCESS) {
      cerr << "*** ERROR *** Gateway does not have a \"nodeId\" attribute.\n";
      continue;
    }

    // attach gateway to the specified node.
    Node* node = World::getNode(nodeId);
    node->attachServerApplicationAgent(new GatewayServerAgent(node),
      GatewayServerAgent::Port);
  }
}


GatewayServerAgent::GatewayServerAgent(Node* hostNode)
  : ServerApplicationAgent(hostNode) {
}




void GatewayServerAgent::receivePacket(PacketPtr /*packet*/) {
  cout << "GatewayServer received a packet.\n";
}


