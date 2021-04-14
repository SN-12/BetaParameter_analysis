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

#include <cassert>
#include <set>
#include "utils.h"
#include "node.h"
#include "scheduler.h"
#include "world.h"

#include "agents/deden-agent.h"
#include "agents/no-routing-agent.h"
#include "agents/manual-routing-agent.h"
#include "agents/pure-flooding-routing-agent.h"
#include "agents/proba-flooding-routing-agent.h"
#include "agents/confidence-routing-agent.h"
#include "agents/backoff-deviation-routing-agent.h"
#include "agents/hcd-routing-agent.h"
#include "agents/pure-flooding-ring-routing-agent.h"
#include "agents/backoff-flooding-routing-agent.h"
#include "agents/backoff-flooding-ring-routing-agent.h"
#include "agents/proba-flooding-ring-routing-agent.h"
#include "agents/slr-routing-agent.h"
#include "agents/slr-backoff-routing-agent.h"
#include "agents/slr-backoff-routing-agent3.h"
#include "agents/slr-ring-routing-agent.h"
#include "agents/slr-deviation-routing-agent.h"

using NodeLogEvent = CallMethodEvent<Node, &Node::processNodeLogEvent,
  EventType::NODE_LOG>;

//==============================================================================
//
//          IntervalInfoLog  (class)
//
//==============================================================================

IntervalInfoLog::IntervalInfoLog() {
  reset();
}

void IntervalInfoLog::reset() {
  totalPacketsReceived = 0;
  packetsCorrectlyReceived = 0;
  packetsCollisioned = 0;
  packetsIgnored = 0;
  totalBitsReceived = 0;
  bitsCorrectlyReceived = 0;
  bitsCollisioned = 0;
  bitsIgnored = 0;
}

//==============================================================================
//
//          Node  (class)
//
//==============================================================================

int Node::nextId = 0;
mt19937_64 *Node::backoffRandomGenerator = nullptr;
simulationTime_t Node::pulseDuration;
uniform_int_distribution<distance_t> Node::defaultBackoffDistribution;
map<PacketType,backoffHelper_t> Node::specificBackoffsMap;

Node::Node(int _id, distance_t _x, distance_t _y, distance_t _z) {
  assert (_id == nextId);
  id = _id;
  nextId++;
  x = _x;
  y = _y;
  z = _z;

  currentBeta = ScenarioParameters::getDefaultBeta();
  nodeSequenceNumber = 0;
  estimatedNeighbours = -1;

  routingAgent = nullptr;

  currentTransmitedPacketStartTime = -1;
  lastEndSend = -ScenarioParameters::getMinimumIntervalBetweenSends();
  lastEndReceive = -ScenarioParameters::getMinimumIntervalBetweenReceiveAndSend();

  LogSystem::NodeInfo << "Instantiating node " << id << endl;

  neighboursCount = -1;

  intervalInfoLog = nullptr;
  endLoggingTime = 0;
  loggingInterval = 0;
  nodeLogFile = nullptr;
}

Node::~Node() {
  delete routingAgent;
  for (auto it=serverApplicationsMap.begin(); it!=serverApplicationsMap.end(); it++) {
    delete it->second;
  }
  for (auto it=vectApplications.begin(); it!=vectApplications.end(); it++) {
    delete *it;
  }
}

void Node::setNeighboursCount(int count) {
  neighboursCount = count;
}

int Node::getNeighboursCount() {
  if ( ScenarioParameters::getDoNotUseNeighboursList() ) {
    assert (neighboursCount != -1);
    return neighboursCount;
  } else
    return (int)neighboursMap.size();
}

void Node::attachRoutingAgent(RoutingAgent *_routingAgent) {
  if (routingAgent != nullptr) {
    cerr << "*** ERROR *** Routing agent already attached to node " << id << endl;
    exit(EXIT_FAILURE);
  }
  routingAgent = _routingAgent;
}

void Node::attachApplicationAgent(ApplicationAgent *_applicationAgent) {
  vectApplications.push_back(_applicationAgent);
}

bool Node::attachServerApplicationAgent(ServerApplicationAgent *_serverApplicationAgent, int _port) {
  //cout << "Attaching ServerApplicationAgent on node " << id << "  port:" << _port << endl;
  return serverApplicationsMap.insert(pair<int, ServerApplicationAgent*>(_port,_serverApplicationAgent)).second;
}

void Node::dispatchPacketToApplication(PacketPtr _packet) {
  auto it = serverApplicationsMap.find(_packet->port);
  if (it != serverApplicationsMap.end()) {
    // forwarding packet to bound server application
    it->second->receivePacket(_packet);
  } else {
    //cout << "Node " << id << " Packet dropped (no application bound to port " << _packet->port << ")" << endl;
  }
}

void Node::enqueueOutgoingPacket(PacketPtr _p) {
  //cerr << " ENQUEUE packet buffer size of node " << id << " is " << outputPacketBuffer.size() << " new packet is " << p->sequence << " beta " << p->beta<< endl;

  // TODO ? simultaneous emission of several packets

  _p->transmitterId = id;

  outputPacketBuffer.push(_p);

  if (outputPacketBuffer.size() == 1){
    Scheduler::getScheduler().schedule(new StartSendPacketEvent(Scheduler::now(), this, outputPacketBuffer.front()));
  }

  //cout << "AFTER ENQUEUE packet buffer size of node " << id << " is " << outputPacketBuffer.size() << " new packet is " << p << endl;
}

bool Node::detectPacketCollision(PacketPtr _p, simulationTime_t _p1StartTime) {
  //cout << "Detect collision on node dpc" << id << endl;

  //
  // A packet being transmitted can collide with an incoming one
  //
  if (currentTransmitedPacketStartTime != -1 && !outputPacketBuffer.empty())  {
    _p->checkAndTagCollision(_p1StartTime, outputPacketBuffer.front(), currentTransmitedPacketStartTime);
  }


  //
  // The current packet has to be checked against AT LEAST all the packets in
  // the reception buffer, as it may cause a collision with some of them.
  // In case of collision the affected packets will get a mark that will prevent their reception
  //
  for ( map<PacketPtr,simulationTime_t>::iterator it = receptionBuffer.begin(); it != receptionBuffer.end(); it++) {
    if (_p->packetId != it->first->packetId) {
      _p->checkAndTagCollision(_p1StartTime, it->first, it->second);
    }
  }

  //
  // If the current packet is not a parasite (see maxConcurrentReceptions parameter)
  // it has to be checked also against all current parasites, that may collision with it.
  //
  if (!_p->parasite) {
    for ( map<PacketPtr,simulationTime_t>::iterator it = parasiteReceptionBuffer.begin(); it != parasiteReceptionBuffer.end(); it++) {
      if (_p->packetId != it->first->packetId) {
        _p->checkAndTagCollision(_p1StartTime, it->first, it->second);
      }
    }
  }
  return _p->collisioned;
}

// Node is always awake
bool Node::isAwake (simulationTime_t /* receptionTime */) {
  return true;
}

void Node::processStartSendEvent(PacketPtr packet) {
  // random beta
  //uniform_int_distribution<simulationTime_t> defaultBackoffDistribution(1000,5000);
  //packet->beta = defaultBackoffDistribution(*backoffRandomGenerator);

  simulationTime_t packetDuration = (Node::getPulseDuration() * packet->beta) * (packet->size-1) + Node::getPulseDuration();
  //cout << "packetDuration: " << packetDuration << " beta: " << packet->beta << " tp: " << Node::getPulseDuration() << endl;

  simulationTime_t guardInterval = 0;
  simulationTime_t guardIntervalReceive = 0;
  simulationTime_t diffSend, diffReceive;
  diffSend = Scheduler::now() - lastEndSend;
  if (diffSend < ScenarioParameters::getMinimumIntervalBetweenSends()) guardInterval = ScenarioParameters::getMinimumIntervalBetweenSends() - diffSend;
  diffReceive = Scheduler::now() - lastEndReceive;
  if (diffReceive < ScenarioParameters::getMinimumIntervalBetweenReceiveAndSend()) guardIntervalReceive = ScenarioParameters::getMinimumIntervalBetweenReceiveAndSend() - diffReceive;
  if (guardInterval < guardIntervalReceive) guardInterval = guardIntervalReceive;

  simulationTime_t receptionTime;
  simulationTime_t delayBeforeTransmission = guardInterval;

  if ( !packet->disableBackoff || id == packet->srcId ) {
    delayBeforeTransmission += getNewBackoff(packet->type);
    if (packet->type == PacketType::D1_DENSITY_PROBE) delayBeforeTransmission += 1000000000;
  }

  // simulationTime_t delayBeforeTransmission = 0;

  if (ScenarioParameters::getDoNotUseNeighboursList())
    World::getWorld()->sendPacketToNeighbours(this, packet, delayBeforeTransmission);
  else
    for (auto it = neighboursMap.begin(); it != neighboursMap.end() ; it ++) {
      if ((packet->type == PacketType::SLR_BEACON && it->first <= ScenarioParameters::getCommunicationRangeSmall()) || packet->type != PacketType::SLR_BEACON) {
        receptionTime = Scheduler::now() + delayBeforeTransmission + it->first/PROPAGATIONSPEED;
        if (it->second->isAwake (receptionTime)) {
          //PacketPtr packetClone = PacketPtr(new Packet(packet));
          PacketPtr packetClone = PacketPtr(packet->clone());
          Scheduler::getScheduler().schedule(new StartReceivePacketEvent(receptionTime, it->second, packetClone));
        }
      }
  }

  currentTransmitedPacketStartTime = Scheduler::now() + delayBeforeTransmission;
  Scheduler::getScheduler().schedule(new EndSendPacketEvent(Scheduler::now() + delayBeforeTransmission + packetDuration, this, packet));

  //     LogSystem::EventsLogOutput.log( LogSystem::channelUsage, currentTransmitedPacketStartTime,_event->packet->type,_event->packet->flowId,_event->packet->flowSequenceNumber,_event->packet->beta, "+" );
  //     LogSystem::EventsLogOutput.log( LogSystem::channelUsage, Scheduler::now() + delayBeforeTransmission + packetDuration +(ScenarioParameters::getCommunicationRange()/300) ,_event->packet->type,_event->packet->flowId,_event->packet->flowSequenceNumber,_event->packet->beta, "-" );
}

void Node::processEndSendPacket(PacketPtr packet) {
  //
  // The outgoing packet may have caused collision on still incoming packets, we have to check all of them
  //
  for ( map<PacketPtr,simulationTime_t>::iterator it = receptionBuffer.begin(); it != receptionBuffer.end(); it++) {
    if (outputPacketBuffer.front()->packetId != it->first->packetId) {
      outputPacketBuffer.front()->checkAndTagCollision(currentTransmitedPacketStartTime, it->first, it->second);
    }
  }

  //
  // remove the current packet from the outgoing buffer
  // then schedule the transmission of the following one (if exists)
  //
  outputPacketBuffer.pop();
  currentTransmitedPacketStartTime = -1;

  //if ( ScenarioParameters::getLogAtNodeLevel() ) {
  LogSystem::EventsLogOutput.log( LogSystem::sentEventLog, Scheduler::now(), id, packet->transmitterId, packet->beta, packet->size,packet->type,packet->flowId,packet->flowSequenceNumber);

  //SLRBackoffRoutingAgent3* slrBackoff = dynamic_cast<SLRBackoffRoutingAgent3*>(routingAgent);
  //bool onLine = slrBackoff->SLRForward(packet);
  //fprintf(LogSystem::EventsLogC,"s %d %d %d %ld %d %d %d %d %d %d %d\n", id, packet->srcSequenceNumber, packet->packetId, Scheduler::now(), packet->flowId, packet->transmitterId, packet->size, packet->beta, packet->flowId, packet->flowSequenceNumber, onLine);
  //  }
  if (ScenarioParameters::getGraphicMode()) {
    World::drawNode(packet->transmitterId, DrawingType::SEND);
  }
  if (outputPacketBuffer.size() > 0) {
    Scheduler::getScheduler().schedule(new StartSendPacketEvent(Scheduler::now(), this, outputPacketBuffer.front()));
  }
  lastEndSend = Scheduler::now();

  //if (packet->type == PacketType::DATA && id != packet->srcId){
  //    string filename = ScenarioParameters::getScenarioDirectory()+"/memoryTrace.data";
  //    ofstream memoryTrace(filename,std::ofstream::app);
  //    memoryTrace << ScenarioParameters::getDefaultBeta() <<" " << id <<  " - "<< Scheduler::now() << endl;
  //    memoryTrace.close();
  //}
}

void Node::processStartReceivePacketEvent(StartReceivePacketEvent *_event) {
  PacketPtr packet = _event->packet;
  simulationTime_t packetDuration =  (Node::getPulseDuration() * packet->beta) * (packet->size-1) + Node::getPulseDuration();
  simulationTime_t endReceptionTime = Scheduler::now() + packetDuration;

  if (receptionBuffer.size() < ScenarioParameters::getMaxConcurrentReceptions() ) {
    //   fprintf(LogSystem::EventsLogC,"a %d %d %d %ld %d %d %d %d\n", id, packet->srcSequenceNumber, packet->packetId, Scheduler::now(), packet->flowId, packet->transmitterId, packet->size, packet->beta);
    receptionBuffer.insert(pair<PacketPtr,simulationTime_t>(packet, _event->date));
  } else {
    //   fprintf(LogSystem::EventsLogC,"b %d %d %d %ld %d %d %d %d\n", id, packet->srcSequenceNumber, packet->packetId, Scheduler::now(), packet->flowId, packet->transmitterId, packet->size, packet->beta);
    packet->parasite = true;
    parasiteReceptionBuffer.insert(pair<PacketPtr,simulationTime_t>(packet, _event->date));
  }
  Scheduler::getScheduler().schedule(new EndReceivePacketEvent(endReceptionTime, this, packet, _event->date));
}

void Node::processEndReceivePacketEvent(EndReceivePacketEvent *_event) {
  PacketPtr packet = _event->packet;

  lastEndReceive = Scheduler::now();

  if (!detectPacketCollision(packet, _event->receptionStartTime)) {
    //
    // A packet arrived, it is not altered (at least not enough to prevent decoding)
    //
    if (!packet->parasite) {
      //
      // The packet has arrived correctly and can be processed (maxConcurrentReception currently not reached)
      //
      // This a "Received" packet.
      //
      //       if ( ScenarioParameters::getLogAtNodeLevel() ) {
      LogSystem::EventsLogOutput.log( LogSystem::receptionEventLog, Scheduler::now(), id, packet->transmitterId, packet->beta, packet->size, packet->flowId, packet->flowSequenceNumber );
      //SLRBackoffRoutingAgent3* slrBackoff = dynamic_cast<SLRBackoffRoutingAgent3*>(routingAgent);
      //bool onLine = slrBackoff->SLRForward(_event->packet);
      //fprintf(LogSystem::EventsLogC,"r %d %d %d %ld %d %d %d %d %d %d %d\n", id, _event->packet->srcSequenceNumber, _event->packet->packetId, Scheduler::now(), _event->packet->flowId, _event->packet->transmitterId, _event->packet->size, _event->packet->beta, _event->packet->flowId, _event->packet->flowSequenceNumber, onLine);
      //       }
      if (ScenarioParameters::getGraphicMode()) {
        World::drawNode(id, DrawingType::RECEIVE);
      }
      if (intervalInfoLog) {
        intervalInfoLog->packetsCorrectlyReceived++;
        intervalInfoLog->bitsCorrectlyReceived += packet->size;
      }
      // forward to routing agent
      routingAgent->receivePacketFromNetwork(packet);
    } else {
      //
      // The received packet was not collisionned, but can not be processed (maxConcurrentReception currently reached)
      //
      // This is an "Ignored" packet
      //
      if ( ScenarioParameters::getLogAtNodeLevel() ) {
        LogSystem::EventsLogOutput.log( LogSystem::ignoredEventLog, Scheduler::now(), id, _event->packet->transmitterId, _event->packet->beta, _event->packet->size,_event->packet->type ,packet->flowId, packet->flowSequenceNumber);
      }

      if (ScenarioParameters::getGraphicMode()) {
        World::drawNode(id, DrawingType::IGNORE);
      }
      if (intervalInfoLog) {
        intervalInfoLog->packetsIgnored++;
        intervalInfoLog->bitsIgnored += packet->size;
      }
    }
  } else {
    //
    // A packet arrived, but too many bits have been altered. It is considered collisionned
    //
    if (!packet->parasite) {
      //
      // The collisionned packet should have been processed (maxConcurrentReception was not reached)
      //
      // This is a "Collisionned" packet
      //
      if ( ScenarioParameters::getLogAtNodeLevel() ) {
        LogSystem::EventsLogOutput.log( LogSystem::collisionEventLog, Scheduler::now(), id, _event->packet->transmitterId, _event->packet->beta, _event->packet->size,_event->packet->type ,packet->flowId, packet->flowSequenceNumber);
        //SLRBackoffRoutingAgent3* slrBackoff = dynamic_cast<SLRBackoffRoutingAgent3*>(routingAgent);
        //bool onLine = slrBackoff->SLRForward(_event->packet);
        //fprintf(LogSystem::EventsLogC,"c %d %d %d %ld %d %d %d %d %d %d %d\n", id, _event->packet->srcSequenceNumber, _event->packet->packetId, Scheduler::now(), _event->packet->flowId, _event->packet->transmitterId, _event->packet->size, _event->packet->beta, _event->packet->flowId, _event->packet->flowSequenceNumber, onLine);
      }

      if (ScenarioParameters::getGraphicMode()) {
        World::drawNode(id, DrawingType::COLLISION);
      }
      if (intervalInfoLog) {
        intervalInfoLog->packetsCollisioned++;
        intervalInfoLog->bitsCollisioned += packet->size;
      }
      if (ScenarioParameters::getAcceptCollisionedPackets()) {
        routingAgent->receivePacketFromNetwork(packet);
      }
      //   cout << "Node:" << id << " srcId:" << packet->srcId << " modified bits: ";
      //   for (auto it=packet->modifiedBitsPositions.begin(); it != packet->modifiedBitsPositions.end(); it++) {
      //    cout << (*it) << " ";
      //   }
      //   cout << endl;

    } else {
      //
      // A packet arrived but was collisionned. This packet could not have been processed anyway (maxConcurrentReception reached)
      // This packet is a parasite and will be ignored, but will cause at least one current transmission to fail later
      //
      // This is an "Ignored" packet
      //
      if ( ScenarioParameters::getLogAtNodeLevel() ) {
        LogSystem::EventsLogOutput.log( LogSystem::ignoredEventLog, Scheduler::now(), id, _event->packet->transmitterId, _event->packet->beta, _event->packet->size,_event->packet->type,packet->flowId, packet->flowSequenceNumber);
        //SLRBackoffRoutingAgent3* slrBackoff = static_cast<SLRBackoffRoutingAgent3*>(routingAgent);
        //bool onLine = slrBackoff->SLRForward(_event->packet);
        //fprintf(LogSystem::EventsLogC,"i %d %d %d %ld %d %d %d %d %d %d %d\n", id, _event->packet->srcSequenceNumber, _event->packet->packetId, Scheduler::now(), _event->packet->flowId, _event->packet->transmitterId, _event->packet->size, _event->packet->beta, _event->packet->flowId, _event->packet->flowSequenceNumber, onLine);
      }

      if (ScenarioParameters::getGraphicMode()) {
        World::drawNode(id, DrawingType::IGNORE);
      }
      //if (intervalInfoLog != nullptr) intervalInfoLog->packetsIgnored++;
    }
  }

  if (intervalInfoLog) {
    intervalInfoLog->totalPacketsReceived++;
    intervalInfoLog->totalBitsReceived += packet->size;
    intervalInfoLog->bitsCollisioned += (int)packet->modifiedBitsPositions.size();
  }
  if (packet->parasite) {
    parasiteReceptionBuffer.erase(packet);
  } else {
    receptionBuffer.erase(packet);
  }
  //cout << "NODE " << id << "processEndReceive  now:" << Scheduler::now() << "  _event->date:" << _event->date << "  _event->receptionStartTime:" << _event->receptionStartTime << endl;
}

void Node::startLogging(simulationTime_t _interval, simulationTime_t _endTime) {
  cout << "INTERVAL LOG starting" << endl;
  string fileName = ScenarioParameters::getScenarioDirectory() + ScenarioParameters::getOutputBaseName()+"-node-"+to_string(id)+".log";
  nodeLogFile = fopen(fileName.c_str(), "w");
  loggingInterval = _interval;
  endLoggingTime = _endTime;
  intervalInfoLog = IntervalInfoLogPtr(new IntervalInfoLog());
  intervalInfoLog->bitsIgnored++;
  if ( loggingInterval > 0 && Scheduler::now() <= endLoggingTime) {
    Scheduler::getScheduler().schedule(new NodeLogEvent(Scheduler::now() + loggingInterval, this));
  }
}

void Node::processNodeLogEvent() {
  if (intervalInfoLog != nullptr) {
    double channelUsage = 100 * intervalInfoLog->totalBitsReceived / ((double)loggingInterval / (double)ScenarioParameters::getPulseDuration());
    //        cout << "Interval log on node " << id<< ":" << endl;
    //        cout << "    " << intervalInfoLog->totalPacketsReceived << " packets total / ";
    //        cout << intervalInfoLog->totalBitsReceived << " bits total / ";
    //        cout << channelUsage << "% channel usage" << endl;
    //        cout << "    " << intervalInfoLog->packetsCorrectlyReceived << " corrects (" << ((double)intervalInfoLog->packetsCorrectlyReceived / intervalInfoLog->totalPacketsReceived) * 100 << "%)";
    //        cout << "    " << intervalInfoLog->packetsCollisioned << " collisioned (" <<((double)intervalInfoLog->packetsCollisioned / intervalInfoLog->totalPacketsReceived) * 100 << "%)";
    //        cout << "    " << intervalInfoLog->packetsIgnored << " ignored (" <<((double)intervalInfoLog->packetsIgnored / intervalInfoLog->totalPacketsReceived) * 100 << "%)";
    //        cout << endl;
    fprintf(nodeLogFile,"interval: %ld totalPackets: %d totalBits: %d channelUsage: %lf okPcks: %d okBits: %d cPkts: %d cBits: %d iPkt: %d iBits: %d\n",
      loggingInterval,
      intervalInfoLog->totalPacketsReceived,
      intervalInfoLog->totalBitsReceived,
      channelUsage,
      intervalInfoLog->packetsCorrectlyReceived,
      intervalInfoLog->bitsCorrectlyReceived,
      intervalInfoLog->packetsCollisioned,
      intervalInfoLog->bitsCollisioned,
      intervalInfoLog->packetsIgnored,
      intervalInfoLog->bitsIgnored);
    if ( loggingInterval > 0 && Scheduler::now() <= endLoggingTime) {
      Scheduler::getScheduler().schedule(new NodeLogEvent(Scheduler::now() + loggingInterval, this));
    } else {
      fclose(nodeLogFile);
    }
    intervalInfoLog->reset();
  }
}

void Node::startupCode() {
  // attach the routing agent
  string agent = ScenarioParameters::getRoutingAgentName();
  if (agent.compare("PureFloodingRouting") == 0)
    attachRoutingAgent(new PureFloodingRoutingAgent(this));
  else if (agent.compare("NoRouting") == 0)
    attachRoutingAgent(new NoRoutingAgent(this));
  else if (agent.compare("ManualRouting") == 0)
    attachRoutingAgent(new ManualRoutingAgent(this));
  else if (agent.compare("SLRRouting") == 0)
    attachRoutingAgent(new SLRRoutingAgent(this));
  else if (agent.compare("DeviationRouting") == 0)
    attachRoutingAgent(new DeviationRoutingAgent(this));
  else if (agent.compare("BackoffDeviationRouting") == 0)
    attachRoutingAgent(new BackoffDeviationRoutingAgent(this));
  else if (agent.compare("BackoffFloodingRouting") == 0)
    attachRoutingAgent(new BackoffFloodingRoutingAgent(this));
  else if (agent.compare("SLRBackoffRouting") == 0)
    attachRoutingAgent(new SLRBackoffRoutingAgent(this));
  else if (agent.compare("SLRBackoffRouting3") == 0)
    attachRoutingAgent(new SLRBackoffRoutingAgent3(this));
  else if (agent.compare("ProbaFloodingRouting") == 0)
    attachRoutingAgent(new ProbaFloodingRoutingAgent(this));
  else if (agent.compare("ConfidenceRouting") == 0)
    attachRoutingAgent(new ConfidenceRoutingAgent(this));
  else if (agent.compare("HCDRouting") == 0)
    attachRoutingAgent(new HCDRoutingAgent(this));
  else if (agent.compare("PureFloodingRingRouting") == 0)
    attachRoutingAgent(new PureFloodingRingRoutingAgent(this));
  else if (agent.compare("BackoffFloodingRingRouting") == 0)
    attachRoutingAgent(new BackoffFloodingRingRoutingAgent(this));
  else if (agent.compare("ProbaFloodingRingRouting") == 0)
    attachRoutingAgent(new ProbaFloodingRingRoutingAgent(this));
  else if (agent.compare("SLRRingRouting") == 0)
    attachRoutingAgent(new SLRRingRoutingAgent(this));
  else {
    cerr << "*** ERROR *** No valid routing agent specified" << endl;
    exit (EXIT_FAILURE);
  }

  /****************************************************************************/
  //                          TO USE DEDEN                                     /
  /****************************************************************************/

  if (ScenarioParameters::getDeden()) {
    D11DensityEstimatorAgent *D11DensityEstimator = new D11DensityEstimatorAgent(this, 1, 4000, PacketType::D1_DENSITY_PROBE, 10000000000, 1,2);
    attachServerApplicationAgent(D11DensityEstimator, 4000);

    if ( ScenarioParameters::getD1DoNotSimulateInitFlood() ) {
      simulationTime_t t = Scheduler::now();
      Scheduler::getScheduler().schedule(new D11DensityEstimatorGenerationEvent(t,D11DensityEstimator));
    } else {
      if (id == 0) {
        cout << "Node 0 initialization ..." << endl;
        PacketPtr p(new Packet(PacketType::D1_DENSITY_INIT, 40, id, -1, 4000, 1, 0));
        routingAgent->receivePacketFromApplication(p);

        simulationTime_t t = Scheduler::now();
        Scheduler::getScheduler().schedule(new D11DensityEstimatorGenerationEvent(t,D11DensityEstimator));
      }
    }
  }
}


//==============================================================================
//
//          SleepingNode  (class)
//
//==============================================================================

class SleepingNode;
mt19937_64 *SleepingNode::RNG =  new mt19937_64(0);
simulationTime_t SleepingNode::ts;

SleepingNode::SleepingNode(int _id, distance_t _x, distance_t _y, distance_t _z) : Node(_id,_x,_y, _z) {
  initialized = 0;
  if ( id == 0 ){
    RNG = new mt19937_64(ScenarioParameters::getSleepRNGSeed());
    ts = currentBeta * pulseDuration;
  }
}

// schedule only if SleepingNode is awake at reception
bool SleepingNode::isAwake (simulationTime_t receptionTime) {
  // the second term of the or condition deals with the case where the awake
  // duration spreads over 2 consecutive ts, i.e. awakenStart+awakenDuration > ts
  bool receiverAwake = (receptionTime%ts >= awakenStart && receptionTime%ts < awakenStart + awakenDuration)
    || receptionTime%ts < awakenStart + awakenDuration - ts;
  if (!receiverAwake && initialized != 0)
    return false;
  return true;
}

void SleepingNode::processSleepingSetUpEvent() {
  int min = 300;  // it makes no sense to stay awake for a lesser duration
  initialized = 1;

  if (ScenarioParameters::getAwakenNodes()){
    // the average number of awake neighbours was specified, transform it in absolute duration
    awakenDuration = ts * ScenarioParameters::getAwakenNodes() / estimatedNeighbours;
    if (awakenDuration > ts) awakenDuration = ts;
  } else
    awakenDuration = ScenarioParameters::getAwakenDuration();  // absolute value

  if (awakenDuration < min || awakenDuration > ts){
    cerr << " SLEEPING node :: awakenDuration " << awakenDuration << " must be in [" << min << "," << ts << "]" << endl;
    exit(-1);
  }

  uniform_int_distribution<time_t> distrib(0, ts);
  awakenStart = distrib(*RNG); // NODES WILL START SEPARATELY
}


//==============================================================================
//
//          StartReceivePacketEvent  (class)
//
//==============================================================================

StartReceivePacketEvent::StartReceivePacketEvent(simulationTime_t _t, Node* _n, PacketPtr _p): Event(_t) {
  node = _n;
  packet = _p;
  eventType = EventType::START_RECEIVE_PACKET;
}

const string StartReceivePacketEvent::getEventName(){
  return "StartReceivePacketEvent Event";
}

void StartReceivePacketEvent::consume() {
  //  cerr << "Consuming StartRcvPacketEvent on node " << node->getId() << " At date " << date << " of packet  "<< packet->sequence<<  endl;getchar();
  node->processStartReceivePacketEvent(this);
}

//==============================================================================
//
//          EndReceivePacketEvent  (class)
//
//==============================================================================

EndReceivePacketEvent::EndReceivePacketEvent(simulationTime_t _t, Node *_n, PacketPtr _p, simulationTime_t _receptionStartTime): Event(_t) {
  node = _n;
  packet = _p;
  receptionStartTime = _receptionStartTime;
  eventType = EventType::END_RECEIVE_PACKET;
}

const string EndReceivePacketEvent::getEventName(){
  return "EndReceivePacketEvent Event on node " + to_string(node->getId());
}

void EndReceivePacketEvent::consume() {
  node->processEndReceivePacketEvent(this);
}
