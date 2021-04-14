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

#ifndef DEDEN_H_
#define DEDEN_H_

#include "events.h"
#include "server-application-agent.h"

class D1DensityEstimatorAgent;
class D11DensityEstimatorAgent;
class D2DensityEstimatorAgent;


//==============================================================================
//
//          D1DensityEstimatorGenerationEvent  (class)
//
//==============================================================================

class D1DensityEstimatorGenerationEvent: public Event {
private:
  D1DensityEstimatorAgent *concernedGenerator;
public:
  D1DensityEstimatorGenerationEvent(simulationTime_t _t, D1DensityEstimatorAgent *_gen);
  ~D1DensityEstimatorGenerationEvent();

  void consume();
  const string getEventName();
};

//==============================================================================
//
//          D1DensityEstimatorAgent  (class)
//
//==============================================================================

typedef struct {
  int nodeId;
  long posX, posY, posZ;
  int neighbours;
  double estimatedNeighbours;
  int packetsReceived;
  int packetsSent;
} NeighboursInfo_t;

class D1DensityEstimatorAgent : public ServerApplicationAgent {
protected:
  int flowId;
  int port;
  PacketType packetType;
  simulationTime_t interval;
  int limit;          // number of packets to send, -1 for infinite

  int flowSequenceNumber;  // number of packets already sent by this generator
  double sendingProba;

  static mt19937_64 *sendingRandomGenerator;

  int roundNumber;
  int maxRound;
  int ticInRound;
  int maxTic;
  float growRate;
  double probability;
  int minPacketsForTermination;
  vector<int> vectProbesReceived;
  vector<double> vectEstimated;
  vector<int>vectProbesSent;

  double estimated;
  int packetsReceived, packetsSent;

  static vector<NeighboursInfo_t> vectNeighboursInfo;
  static int aliveAgents;

  bool alreadyWorking;
  int remainingFreeRounds;

  static vector<int> vectEstimationErrorDistribution;

  static ofstream estimationErrorFile;

public:
  D1DensityEstimatorAgent(Node *_hostNode, int _flowId, int _port, PacketType _packetType, simulationTime_t _interval, int _repetition, int _seed);
  virtual ~D1DensityEstimatorAgent();

  static void initialize();
  static mt19937_64 *getSendingRNG() { return(sendingRandomGenerator); }

  virtual void receivePacket(PacketPtr _packet);
  void processPacketGenerationEvent();
};



//===========================================================================================================
//
//          D11DensityEstimatorGenerationEvent  (class)
//
//===========================================================================================================

class D11DensityEstimatorGenerationEvent: public Event {
private:
  D11DensityEstimatorAgent *concernedGenerator;
public:
  D11DensityEstimatorGenerationEvent(simulationTime_t _t, D11DensityEstimatorAgent *_gen);
  ~D11DensityEstimatorGenerationEvent();

  void consume();
  const string getEventName();
};


//===========================================================================================================
//
//          D11DensityEstimatorAgent  (class)
//
//===========================================================================================================

// typedef struct {
//         int nodeId;
//         long posX, posY, posZ;
//         int neighbours;
//         double estimatedNeighbours;
//         int packetsReceived;
//         int packetsSent;
// } NeighboursInfo_t;

class D11DensityEstimatorAgent : public ServerApplicationAgent {
protected:
  int flowId;
  int port;
  PacketType packetType;
  simulationTime_t interval;
  int limit;          // number of packets to send, -1 for infinite

  int flowSequenceNumber;  // number of packets already sent by this generator
  double sendingProba;

  static mt19937_64 *sendingRandomGenerator;

  int roundNumber;
  int maxRound;
  int ticInRound;
  int maxTic;
  float growRate;
  double probability;
  int minPacketsForTermination;
  vector<int> vectProbesReceived;
  vector<double> vectEstimated;
  vector<int>vectProbesSent;

  double estimated;
  int packetsReceived, packetsSent;

  static vector<NeighboursInfo_t> vectNeighboursInfo;
  static int aliveAgents;

  bool alreadyWorking;
  int remainingFreeRounds;

  static vector<int> vectEstimationErrorDistribution;
        
  static ofstream estimationErrorFile;

public:
  
  static int computedMaxRound;

  static map <double,int> probaThrsldGrowthRate2Error5;
  static map <double,int> probaThrsldGrowthRate2Error10;
  static map <double,int> probaThrsldGrowthRate2Error20;
  static map <double,int> probaThrsldGrowthRate2Error30;
  static map <double,int> probaThrsldGrowthRate1_6Error5;
  static map <double,int> probaThrsldGrowthRate1_6Error10;
  static map <double,int> probaThrsldGrowthRate1_6Error20;
  static map <double,int> probaThrsldGrowthRate1_6Error30;
  static map <double,int> probaThrsldGrowthRate1_2Error5;
  static map <double,int> probaThrsldGrowthRate1_2Error10;
  static map <double,int> probaThrsldGrowthRate1_2Error20;
  static map <double,int> probaThrsldGrowthRate1_2Error30;


  map <double,int> probaThrsld;
  map<double,int>::iterator current;
        
  D11DensityEstimatorAgent(Node *_hostNode, int _flowId, int _port, PacketType _packetType, simulationTime_t _interval, int _repetition, int _seed);
  virtual ~D11DensityEstimatorAgent();
       

  static void initialize();
  static mt19937_64 *getSendingRNG() { return(sendingRandomGenerator); }

  virtual void receivePacket(PacketPtr _packet);
  void processPacketGenerationEvent();
};



//===========================================================================================================
//
//          D2DensityEstimatorProbePacket  (class)
//
//===========================================================================================================

class D2DensityEstimatorProbePacket : public Packet {
public:
  double probability;

  D2DensityEstimatorProbePacket(int _size, int _srcId, int _dstId, int _port, int _flowId, int _flowSequenceNumber, double _probability);
  virtual ~D2DensityEstimatorProbePacket() { }
  virtual PacketPtr clone();
};

//===========================================================================================================
//
//          D2DensityEstimatorGenerationEvent  (class)
//
//===========================================================================================================

class D2DensityEstimatorGenerationEvent: public Event {
private:
  D2DensityEstimatorAgent *concernedGenerator;
public:
  D2DensityEstimatorGenerationEvent(simulationTime_t _t, D2DensityEstimatorAgent *_gen);
  ~D2DensityEstimatorGenerationEvent();

  void consume();
  const string getEventName();
};

//===========================================================================================================
//
//          D2DensityEstimatorAgent  (class)
//
//===========================================================================================================

class D2DensityEstimatorAgent : public ServerApplicationAgent {
protected:
  int flowId;
  int port;

  int flowSequenceNumber;  // number of packets already sent by this generator

  static int initPacketSize;
  static int probePacketSize;
  static simulationTime_t ticDuration;
  static int maxRound;
  static int maxTic;
  static double growRate;
  static int minPacketsForTermination;

  static mt19937 *sendingRandomGenerator;
  static uniform_real_distribution<float> sendingDistribution;

  static vector<NeighboursInfo_t> vectNeighboursInfo;

  static long totalProbesSent;
  static long totalProbesReceived;

  static vector<int> vectEstimationErrorDistribution;

  static int nbAgentsAlive;

  bool alreadyWorking;
  int currentRound;
  int currentTic;
  int currentFlowSequenceNumber;
  double probability;
  int remainingFreeRounds;

  double finalEstimation;

  double roundEstimation;
  int probesReceivedThisRound;
  int probesReceived;
  int probesSent;

  vector<int> vectProbesReceived;
  vector<double> vectEstimated;
  vector<int>vectProbesSent;

public:
  D2DensityEstimatorAgent(Node *_hostNode, int _flowId, int _port);
  virtual ~D2DensityEstimatorAgent();

  static void initializeAgent();

  virtual void receivePacket(PacketPtr _packet);
  void processPacketGenerationEvent();
};


//===========================================================================================================
//
//          sleepingSetUpEvent  (class)
//
//===========================================================================================================

class sleepingSetUpEvent : public Event {
public:
  sleepingSetUpEvent(simulationTime_t _t, Node* _node);

  const string getEventName();
  void consume();
  Node* node;
};

#endif /* DEDEN_H_ */
