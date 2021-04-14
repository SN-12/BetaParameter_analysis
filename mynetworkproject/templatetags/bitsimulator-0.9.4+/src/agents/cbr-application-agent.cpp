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
#include "datasink-application-agent.h"
#include "cbr-application-agent.h"


using CBRPacketGenerationEvent = CallMethodEvent<CBRApplicationAgent,
  &CBRApplicationAgent::processPacketGenerationEvent,
  EventType::CBR_PACKET_GENERATION>;

//==============================================================================
//
//          CBRApplicationAgent  (class)
//
//==============================================================================

CBRApplicationAgent::CBRApplicationAgent(Node *_hostNode, int _flowId, int _size, int _dstId, int _port, PacketType _packetType, simulationTime_t _interval, int _repetitions, int _beta) : ApplicationAgent(_hostNode) {
  flowId = _flowId;
  size = _size;
  dstId = _dstId;
  port = _port;
  packetType = _packetType;
  interval = _interval;
  repetitions = _repetitions;
  beta = _beta;

  flowSequenceNumber = 0;
}

CBRApplicationAgent::~CBRApplicationAgent() {
}

void CBRApplicationAgent::processPacketGenerationEvent() {
  if (repetitions != -1 && flowSequenceNumber >= repetitions) {
    return;
  }

  PacketPtr p(new Packet(PacketType::DATA, size, hostNode->getId(), dstId, port, flowId, flowSequenceNumber));
  p->setBeta(beta);
  p->needAck = true;
  p->creationTime = Scheduler::now();

  if ( hostNode->getRoutingAgent()->type == RoutingAgentType::BACKOFF_FLOODING || hostNode->getRoutingAgent()->type == RoutingAgentType::SLR_BACKOFF) {
    p->disableBackoff = true;
  }

  hostNode->getRoutingAgent()->receivePacketFromApplication(p);

  simulationTime_t t = Scheduler::now() + interval;
  Scheduler::getScheduler().schedule(new CBRPacketGenerationEvent(t,this));

  flowSequenceNumber++;
}

void CBRApplicationAgent::initializeAgent() {
  int flowId;
  int srcId;
  int dstId;
  int port;
  int packetSize;
  simulationTime_t interval;
  int repetitions;
  simulationTime_t startTime;
  int beta;

  tinyxml2::XMLElement *XMLRootNode = ScenarioParameters::getXMLRootNode();
  tinyxml2::XMLElement *applicationAgentsConfigElement = XMLRootNode->FirstChildElement("applicationAgentsConfig");
  if (applicationAgentsConfigElement == nullptr) {
    cout << "*** INFO *** CBRApplicationAgent initialization: \"applicationAgentsConfig\" section no found in " << ScenarioParameters::getConfigurationFileName() << "...  skipping" << endl;
  } else {
    tinyxml2::XMLElement *CBRGeneratorElement = applicationAgentsConfigElement->FirstChildElement("CBRGenerator");
    if (CBRGeneratorElement == nullptr) {
//            cout << "*** INFO *** CBRApplicationAgent initialization: \"CBRGenerator\" section not found in " << ScenarioParameters::getConfigurationFileName() << "...  skipping" << endl;
    } else {
      tinyxml2::XMLElement *flow = CBRGeneratorElement->FirstChildElement("flow");
      cout << "\033[36;1mConstant Bit Rate (CBR) Flows: \033[0m" << endl;

      while (flow != nullptr) {
        ScenarioParameters::queryIntAttr(flow, "flowId", flowId, true, nullptr, 0, "no \"flowId\" attribute in element <flow>");
        ScenarioParameters::queryIntAttr(flow, "srcId", srcId, true, nullptr, 0, "no \"srcId\" attribute in element <flow>");
        ScenarioParameters::queryIntAttr(flow, "dstId", dstId, true, nullptr, 0, "no \"dstId\" attribute in element <flow>");
        if (srcId >= Node::getNextId() || dstId >= Node::getNextId()) {
          cerr << "*** ERROR *** source or destination id out of bounds for CBR " << flowId << endl;
          exit(EXIT_FAILURE);
        }
        ScenarioParameters::queryIntAttr(flow, "port", port, true, nullptr, 0, "no \"port\" attribute in element <flow>");
        ScenarioParameters::queryIntAttr(flow, "packetSize", packetSize, true, nullptr, 0, "no \"packetSize\" attribute in element <flow>");
        ScenarioParameters::queryLongAttr(flow, "interval_fs", "interval_ns", interval, true, nullptr, 0, "no \"interval_fs\" nor \"interval_ns\" attribute in element <flow>");
        ScenarioParameters::queryIntAttr(flow, "repetitions", repetitions, true, nullptr, 0, "no \"repetitions\" attribute in element <flow>");
        ScenarioParameters::queryLongAttr(flow, "startTime_fs", "startTime_ns", startTime, true, nullptr, 0, "no \"startTime_fs\" nor \"startTime_ns\" attribute in element <flow>");
        ScenarioParameters::queryIntAttr(flow, "beta", beta, true, nullptr, 0, "no \"beta\" attribute in element <flow>");
        if (beta == 0)  // default value above
          beta = ScenarioParameters::getDefaultBeta();
        cout << "  flow " << flowId << "  [src:" << srcId << ", dst:" << dstId << ", packetSize:" << packetSize << ", interval:" << interval << ", repetitions:" << repetitions << ", startTime:" << startTime << ", beta:" << beta << "]" << endl;

        if (ScenarioParameters::getSleep() && beta != ScenarioParameters::getDefaultBeta()) {
          cerr << "*** ERROR *** Sleep is enabled and beta of this flow is not equal to defaultBeta; sleep works only when beta of all flows are the same" << endl;
          exit(EXIT_FAILURE);
        }

        Node *srcNode = World::getNode(srcId);
        Node *dstNode;

        CBRApplicationAgent *cbr = new CBRApplicationAgent(srcNode,
          flowId,
          packetSize,
          dstId,
          port,
          PacketType::DATA,
          interval,
          repetitions,
          beta);
        srcNode->attachApplicationAgent(cbr);

        if (dstId != -1) {
          // Create a DataSink for this unicast CBR
          dstNode = World::getNode(dstId);
          DataSinkApplicationAgent *sink = new DataSinkApplicationAgent(dstNode);
          dstNode->attachServerApplicationAgent(sink,port);
        } else {
          // Create DataSink for this broadcast CBR on all nodes !
          // For now, be careful to not create multiple broadcast CBRs !
          vector<Node*>::iterator it;
          it = World::getFirstNodeIterator();
          while ( it != World::getEndNodeIterator() ) {
            dstNode = *it;
            DataSinkApplicationAgent *sink = new DataSinkApplicationAgent(dstNode);
            dstNode->attachServerApplicationAgent(sink,port);
            it++;
          }

        }

        Scheduler::getScheduler().schedule(new CBRPacketGenerationEvent(Scheduler::getScheduler().now() + startTime, cbr));
        flow = flow->NextSiblingElement("flow");
      }
      cout << endl;
    }
  }
}
