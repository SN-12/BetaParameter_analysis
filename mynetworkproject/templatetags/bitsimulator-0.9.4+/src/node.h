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

#ifndef NODE_H_
#define NODE_H_

#include <iostream>
#include <queue>
#include "utils.h"
#include "packet.h"
#include "events.h"

#define PROPAGATIONSPEED 300

class RoutingAgent;
class ApplicationAgent;
class ServerApplicationAgent;
class StartSendPacketEvent;
class EndSendPacketEvent;
class StartReceivePacketEvent;
class EndReceivePacketEvent;
class Event;

using namespace std;

//==============================================================================
//
//          Node  (class)
//
//==============================================================================

typedef struct {
  simulationTime_t min;
  simulationTime_t max;
  uniform_int_distribution<simulationTime_t> backoffDistribution;
} backoffHelper_t;

class IntervalInfoLog{
public:
  int totalPacketsReceived;
  int packetsCorrectlyReceived;
  int packetsCollisioned;
  int packetsIgnored;
  int totalBitsReceived;
  int bitsCorrectlyReceived;
  int bitsCollisioned;
  int bitsIgnored;

  IntervalInfoLog();
  void reset();
};

typedef shared_ptr<IntervalInfoLog> IntervalInfoLogPtr;

class Node {
protected:
  static int nextId;
  static simulationTime_t pulseDuration;

  static mt19937_64 *backoffRandomGenerator;
  static uniform_int_distribution<distance_t> defaultBackoffDistribution;
  static map<PacketType,backoffHelper_t> specificBackoffsMap;

  int id;

  distance_t x,y,z;

  RoutingAgent *routingAgent;
  map<int,ServerApplicationAgent*> serverApplicationsMap;
  vector<ApplicationAgent*> vectApplications;

  queue<PacketPtr> outputPacketBuffer;
  simulationTime_t currentTransmitedPacketStartTime;
  simulationTime_t lastEndSend;
  simulationTime_t lastEndReceive;
  simulationTime_t currentBeta;
  int nodeSequenceNumber;

  multimap<distance_t ,Node*> neighboursMap;
  int estimatedNeighbours;

  map<PacketPtr, simulationTime_t> receptionBuffer;
  map<PacketPtr, simulationTime_t> parasiteReceptionBuffer;

  int neighboursCount;

  IntervalInfoLogPtr intervalInfoLog;
  simulationTime_t endLoggingTime;
  simulationTime_t loggingInterval;
  FILE *nodeLogFile;

  distance_t communicationRange;
  distance_t communicationRangeStandardDeviation;

public:
  Node(int _id, distance_t _x, distance_t _y, distance_t _z);
  virtual ~Node();

  static int getNextId() { return(nextId); }

  static void initBackoffRandomGenerator(int _seed) {
    backoffRandomGenerator = new mt19937_64(_seed);
    defaultBackoffDistribution = uniform_int_distribution<distance_t>(0,ScenarioParameters::getdefaultBackoffWindowWidth());
  }
  static mt19937_64 *getBackoffRNG() { return(backoffRandomGenerator); }
  static void setPulseDuration(simulationTime_t _duration) { pulseDuration = _duration; }
  static void registerSpecificBackoff(PacketType _type, simulationTime_t _min, simulationTime_t _max) {
    map<PacketType, backoffHelper_t>::iterator it;
    if ( (it = specificBackoffsMap.find(_type)) == specificBackoffsMap.end() ) {
      cout << "Registering a specific backoff for packet type " << (int)_type << " : [ " << _min << ", " << _max << " ]" << endl;
      specificBackoffsMap.insert(pair<PacketType, backoffHelper_t>(_type, {_min, _max, uniform_int_distribution<distance_t>(_min, _max)}));
    } else {
      cout << "Updating the specific backoff for packet type " << (int)_type << " : [ " << _min << ", " << _max << " ]" << endl;
      it->second.min = _min;
      it->second.max = _max;
      it->second.backoffDistribution = uniform_int_distribution<distance_t>(_min, _max);
    }
  }
  static void unregisterSpecificBackoff(PacketType _type) {
    cout << "Unregistering the specific backoff for packet type " << (int)_type << endl;
    specificBackoffsMap.erase(_type);
  }
  static simulationTime_t getNewBackoff(PacketType _type) {
    map<PacketType, backoffHelper_t>::iterator it;
    it = specificBackoffsMap.find(_type);
    if (it != specificBackoffsMap.end()) {
      return it->second.backoffDistribution(*backoffRandomGenerator);
    } else {
      return defaultBackoffDistribution(*backoffRandomGenerator);
    }
  }

  static simulationTime_t getPulseDuration() { return pulseDuration; }

  distance_t distance(Node *_n) { return((distance_t)(sqrt( pow(x-_n->x,2) + pow(y-_n->y,2) + pow(z-_n->z,2)))); }
  distance_t distance(distance_t dx, distance_t dy, distance_t dz) {
    return (distance_t)(
      sqrt( pow(dx - x, 2) + pow(dy - y, 2) + pow(dz - z, 2)));
  }

  void addNeighbour(distance_t _distance, Node *_node) { neighboursMap.insert(pair<distance_t, Node*>(_distance, _node)); }
  int getNeighboursCount();
  void setNeighboursCount(int count);

  int getId() { return id; }
  simulationTime_t getBeta() { return currentBeta; }
  int getNextSrcSequenceNumber() { return nodeSequenceNumber++; }
  int getEstimatedNeighbours () {return estimatedNeighbours; }
  void setEstimatedNeighbours (int e ) {estimatedNeighbours = e; }

  distance_t getXPos() { return(x); }
  distance_t getYPos() { return(y); }
  distance_t getZPos() { return(z); }

  void attachRoutingAgent(RoutingAgent *_routingAgent);
  void attachApplicationAgent(ApplicationAgent *_applicationAgent);
  bool attachServerApplicationAgent(ServerApplicationAgent *_serverApplicationAgent, int _port);
  void dispatchPacketToApplication(PacketPtr _packet);
  RoutingAgent *getRoutingAgent() { return routingAgent; };

  void enqueueOutgoingPacket(PacketPtr p);
  void popPacketFromOutputPacketBuffer() { outputPacketBuffer.pop(); }
  unsigned long int outputPacketBufferSize() { return outputPacketBuffer.size(); }

  bool detectPacketCollision(PacketPtr _p, simulationTime_t _p1StartTime);

  distance_t getCommunicationRange() {
    return this->communicationRange;
  }
  void setCommunicationRange( distance_t _cr ) {
    if ( _cr > ScenarioParameters::getCommunicationRange() ) {
      cout << "*** ERROR, trying to set communicationRange of this Node beyond the communicationRange globally defined" << endl;
      exit (EXIT_FAILURE);
    }

    if ( _cr != ScenarioParameters::getCommunicationRange() && !ScenarioParameters::getDoNotUseNeighboursList() ) {
      cout << "*** ERROR, You cannot change the communication range at runtime if you are using the neighbours list" << endl;
      exit (EXIT_FAILURE);
    }

    communicationRange = _cr;
  }

  void setCommunicationRangeStandardDeviation( distance_t _crsd ) {
    this->communicationRangeStandardDeviation = _crsd;
  }
  distance_t getCommunicationRangeStandardDeviation() {
    return this->communicationRangeStandardDeviation;
  }

protected:
  virtual void processStartSendEvent(PacketPtr packet);
  virtual void processEndSendPacket(PacketPtr packet);
public:
  virtual bool isAwake(simulationTime_t receptionTime);
  virtual void processStartReceivePacketEvent(StartReceivePacketEvent *_event);
  virtual void processEndReceivePacketEvent(EndReceivePacketEvent *_event);

  void startLogging(simulationTime_t _interval, simulationTime_t _endTime);
  void processNodeLogEvent();

  virtual void startupCode();

  inline unsigned int getConcurrentReceptionCount() {
    return receptionBuffer.size();
  }

  // event types
  using StartSendPacketEvent = CallMethodArgEvent<Node, PacketPtr,
    &Node::processStartSendEvent,
    EventType::START_SEND_PACKET>;

  using EndSendPacketEvent = CallMethodArgEvent<Node, PacketPtr,
    &Node::processEndSendPacket,
    EventType::END_SEND_PACKET>;
};


using NodeStartupEvent = CallMethodEvent<Node, &Node::startupCode,
  EventType::NODE_STARTUP>;


//==============================================================================
//
//          SleepingNode  (class)
//
//==============================================================================

class SleepingNode : public Node {
protected:
  static mt19937_64 *RNG;
  static simulationTime_t ts;
  int initialized;

public:
  SleepingNode(int _id, distance_t _x, distance_t _y, distance_t _z);
  void processSleepingSetUpEvent();

  int awakenStart;
  int awakenDuration;
  virtual bool isAwake(simulationTime_t receptionTime);
};

//==============================================================================
//
//          StartReceivePacketEvent  (class)
//
//==============================================================================

class StartReceivePacketEvent : public Event {
protected:
  Node *node;

public:
  PacketPtr packet;

  StartReceivePacketEvent(simulationTime_t _t, Node *_node, PacketPtr _p);
  const std::string getEventName();
  void consume();
};

//==============================================================================
//
//          EndReceivePacketEvent  (class)
//
//==============================================================================

class EndReceivePacketEvent : public Event {
protected:
  Node *node;

public:
  PacketPtr packet;
  simulationTime_t receptionStartTime;

  EndReceivePacketEvent(simulationTime_t _t, Node *_n, PacketPtr _p, simulationTime_t receptionStartTime);
  const std::string getEventName();
  void consume();
};


#endif /* NODE_H_ */
