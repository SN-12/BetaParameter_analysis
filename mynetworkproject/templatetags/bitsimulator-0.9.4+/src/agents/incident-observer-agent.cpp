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
#include "scheduler.h"

#include "routing-agent.h"
#include "gateway-server-agent.h"
#include "incident-observer-agent.h"


double IncidentShape::getValueAt(simulationTime_t time, double dist) const {
  double intensity = 0;

  if (function == Function::Linear) {
    intensity = startValue + growthPerFs * time;
  }
  else if (function == Function::Sine) {
    intensity = amplitude * std::sin(2 * 3.1415926 * time / fsPerCycle);
  }
  else { // function == Function::Constant
    if (time >= startTime) {
      intensity = constantValue;
    }
  }

  // std::cout << "valueat: " << intensity << "\n";

  // divide by distance squared in metres.
  return dist == 0 ? intensity : intensity / (dist * dist);
}

double IncidentShape::getMaxDistance() const {
  double maxValue = 0.0;
  if (function == Function::Sine) {
    maxValue = amplitude;
  }
  else if (function == Function::Linear) {
    // select an arbitrary maximum (after 1 second), best we can do.
    maxValue = getValueAt(1e15, 0);
  }
  else {
    maxValue = constantValue;
  }

  double distInM = std::sqrt((double)maxValue);

  // convert metre to nanometre.
  return distInM * 1e9;
}


void IncidentObserverAgent::initializeAgent() {
  tinyxml2::XMLElement *XMLRootNode = ScenarioParameters::getXMLRootNode();

  tinyxml2::XMLElement *applicationAgentsConfigElement =
    XMLRootNode->FirstChildElement("applicationAgentsConfig");
  if (applicationAgentsConfigElement == nullptr) {
    return;
  }

  tinyxml2::XMLElement *observerEl =
    applicationAgentsConfigElement->FirstChildElement("IncidentObserverAgent");
  if (observerEl == nullptr) {
    return;
  }

  std::vector<shared_ptr<IncidentShape>> shapes;

  int incidentCount = 0;
  for (tinyxml2::XMLElement* child =
         observerEl->FirstChildElement("Incident");
       child != nullptr;
       child = child->NextSiblingElement("Incident")) {

    shared_ptr<IncidentShape> shape = std::make_shared<IncidentShape>();

    unsigned int input;
    tinyxml2::XMLError r = child->QueryUnsignedAttribute("x_nm", &input);
    if (r != tinyxml2::XML_SUCCESS) {
      cerr << "*** ERROR *** Incident does not have a \"x_nm\" attribute.\n";
      continue;
    }
    shape->x = input;

    r = child->QueryUnsignedAttribute("y_nm", &input);
    if (r != tinyxml2::XML_SUCCESS) {
      cerr << "*** ERROR *** Incident does not have a \"y_nm\" attribute.\n";
      continue;
    }
    shape->y = input;

    r = child->QueryUnsignedAttribute("z_nm", &input);
    if (r != tinyxml2::XML_SUCCESS) {
      cerr << "*** ERROR *** Incident does not have a \"z_nm\" attribute.\n";
      continue;
    }
    shape->z = input;

    const char* funcStr = child->Attribute("function");
    if (funcStr == nullptr) {
      cerr << "*** ERROR *** Incident does not have a \"function\" "
        "attribute.\n";
      continue;
    }

    std::string function = std::string(funcStr);
    if (function == "Constant") {
      shape->function = IncidentShape::Function::Constant;

      r = child->QueryDoubleAttribute("constantValue", &shape->constantValue);
      if (r != tinyxml2::XML_SUCCESS) {
        cerr << "*** ERROR *** Incident with constant function does not have "
          "a \"constantValue\" attribute.\n";
        continue;
      }

      r = child->QueryDoubleAttribute("startTime", &shape->startTime);
      if (r != tinyxml2::XML_SUCCESS) {
        cerr << "*** ERROR *** Incident with constant function does not have "
          "a \"startTime\" attribute.\n";
        continue;
      }
    }
    else if (function == "Linear") {
      shape->function = IncidentShape::Function::Linear;

      r = child->QueryDoubleAttribute("startValue", &shape->startValue);
      if (r != tinyxml2::XML_SUCCESS) {
        cerr << "*** ERROR *** Incident with linear function does not have "
          "a \"startValue\" attribute.\n";
        continue;
      }

      r = child->QueryDoubleAttribute("growthPerFs", &shape->growthPerFs);
      if (r != tinyxml2::XML_SUCCESS) {
        cerr << "*** ERROR *** Incident with linear function does not have "
          "a \"growthPerFs\" attribute.\n";
        continue;
      }
    }
    else if (function == "Sine") {
      shape->function = IncidentShape::Function::Sine;

      r = child->QueryDoubleAttribute("fsPerCycle", &shape->fsPerCycle);
      if (r != tinyxml2::XML_SUCCESS) {
        cerr << "*** ERROR *** Incident with sine function does not have "
          "a \"fsPerCycle\" attribute.\n";
        continue;
      }

      r = child->QueryDoubleAttribute("amplitude", &shape->amplitude);
      if (r != tinyxml2::XML_SUCCESS) {
        cerr << "*** ERROR *** Incident with sine function does not have "
          "a \"amplitude\" attribute.\n";
        continue;
      }
    }
    else {
      cerr << "*** ERROR *** Invalid value in Incident for function:"
        "\"" << function << "\". Expected one of \"Constant\", \"Linear\" "
        "or \"Sine\".\n";
      continue;
    }

    shapes.push_back(shape);
    incidentCount += 1;
  }

  if (incidentCount == 0) {
    cerr << "*** ERROR *** Defined IncidentObserverAgent, but no valid "
      "Incident has been defined. No Agent will be created.\n";
    return;
  }

  mt19937 rnd(174);

  // okay, we have at least one valid incident definition now.
  for (shared_ptr<const IncidentShape> shape : shapes) {


    int nearbyNodes = 0;

    // attach an agent to each node in range of the incident.
    World::getWorld()->foreachNodeNear(shape->x, shape->y, shape->z,
      shape->getMaxDistance(), [&shape, &rnd, &nearbyNodes] (Node* node) {
        IncidentObserverAgent* agent = new IncidentObserverAgent(node, shape,
          rnd);
        node->attachApplicationAgent(agent);
        nearbyNodes += 1;
      });

    cout << "New Incident at (" << shape->x << "," << shape->y << "," <<
      shape->z << "), up to distance " << shape->getMaxDistance() <<
      ". Possible observers: " << nearbyNodes << "\n";

  }
}

IncidentObserverAgent::IncidentObserverAgent(Node *_hostNode,
  shared_ptr<const IncidentShape> shape, mt19937& rnd)
  : ApplicationAgent(_hostNode), incident(shape) {
  distanceToIncident = hostNode->distance(incident->x, incident->y,
    incident->z);
  scheduleObservation(startupTime + (rnd() % measureInterval));
}

void IncidentObserverAgent::scheduleObservation(simulationTime_t at) {
  simulationTime_t t = Scheduler::now() + at;
  Scheduler::getScheduler().schedule(new IncidentObservationEvent(t, this));
}

void IncidentObserverAgent::observeIncident() {
  // be sure to try again next time.
  observations += 1;
  if (observations < maxObservations) {
    scheduleObservation();
  }

  double incidentValue = incident->getValueAt(Scheduler::now(),
    distanceToIncident);
  if (incidentValue < IncidentShape::detectThreshold) {
    // not detected by this agent.
    return;
  }

  // create a new packet to let everyone know.
  PacketPtr packet(new Packet(PacketType::DATA, 4, hostNode->getId(), -1,
      GatewayServerAgent::Port, 1, 0));
  packet->creationTime = Scheduler::now();
  hostNode->getRoutingAgent()->receivePacketFromApplication(packet);

  // todo: write the actual value into the packet payload.
}


