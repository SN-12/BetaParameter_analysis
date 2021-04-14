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

#include <map>
#include "deden-agent.h"
#include "scheduler.h"
#include "utils.h"
#include "node.h"
#include "world.h"
#include "routing-agent.h"

//===========================================================================================================
//
//          D1DensityEstimatorGenerationEvent  (class)
//
//===========================================================================================================

D1DensityEstimatorGenerationEvent::D1DensityEstimatorGenerationEvent(simulationTime_t _t, D1DensityEstimatorAgent *_gen) : Event(_t){
  eventType = EventType::DENSITY_ESTIMATOR_PACKET_GENERATION;
  concernedGenerator = _gen;
}

D1DensityEstimatorGenerationEvent::~D1DensityEstimatorGenerationEvent(){
}

void D1DensityEstimatorGenerationEvent::consume() {
  concernedGenerator->processPacketGenerationEvent();
}

const string D1DensityEstimatorGenerationEvent::getEventName() {
  return("D1DensityEstimatorGenerationEvent Event");
}

//===========================================================================================================
//
//          D1DensityEstimatorAgent  (class)
//
//===========================================================================================================

mt19937_64 *D1DensityEstimatorAgent::sendingRandomGenerator = nullptr;
vector<NeighboursInfo_t> D1DensityEstimatorAgent::vectNeighboursInfo;
int D1DensityEstimatorAgent::aliveAgents = 0;
vector<int> D1DensityEstimatorAgent::vectEstimationErrorDistribution;
ofstream D1DensityEstimatorAgent::estimationErrorFile;

D1DensityEstimatorAgent::D1DensityEstimatorAgent(Node *_hostNode, int _flowId, int _port, PacketType _packetType, simulationTime_t _interval, int _repetition, int _seed) : ServerApplicationAgent(_hostNode) {
  //cout << "D1DensityEstimatorAgent constructor" << endl;

  static bool first = true;
  if (first){
    sendingRandomGenerator = new mt19937_64(_seed);
    first = false;
  }

  aliveAgents++;

  flowId = _flowId;
  port = _port;
  packetType = _packetType;
  interval = _interval;
  limit = _repetition;

  flowSequenceNumber = 0;
  sendingProba = 0.0;

  roundNumber = 0;
  maxRound = ScenarioParameters::getD1MaxRound();
  ticInRound = 0;
  maxTic = ScenarioParameters::getD1MaxTic();
  growRate = ScenarioParameters::getD1GrowRate();
  probability = 1.0 / pow(growRate,maxRound-1);
  minPacketsForTermination = ScenarioParameters::getD1MinPacketsForTermination();

  vectProbesReceived = vector<int>(maxRound,0);
  vectProbesSent = vector<int>(maxRound,0);
  vectEstimated = vector<double>(maxRound,0);

  estimated = -1;
  packetsReceived = 0;
  packetsSent = 0;

  alreadyWorking = false;
  remainingFreeRounds = 3;

  vectEstimationErrorDistribution = vector<int>(100,0);
}

D1DensityEstimatorAgent::~D1DensityEstimatorAgent() {
  aliveAgents--;

  //  if (hostNode->getId() < 20) {
  //   cout << "\033[36;1mNode " << hostNode->getId() << " [real:" << hostNode->getNeighboursCount() << ", estimated:" << estimated << "]: \033[0m";
  //   for (int i = 0; i<maxRound; i++) cout << vectProbesSent[i] << "/" << vectProbesReceived[i] << "(" << vectEstimated[i] << ") ";
  //   cout << "rfr:" << remainingFreeRounds;
  //   cout << endl;
  //  }

  vectNeighboursInfo.push_back({hostNode->getId(), hostNode->getXPos(), hostNode->getYPos(), hostNode->getZPos(), hostNode->getNeighboursCount(), estimated, packetsReceived, packetsSent});

  double averageNeighboursReal;
  double averageNeighboursEstimated;
  int totalNeighboursReal = 0;
  double totalNeighboursEstimated = 0;

  double totalDifferenceEstimated = 0;
  double averageDifferenceEstimated = 0;
  double maxDifferenceBelowAverage = 0;
  double maxDifferenceOverAverage = 0;

  int totalPacketsSent = 0;

  double totalDiff = 0;
  double averageDiff = 0;
  double maxDiffBelowAverage = 0;
  double maxDiffAboveAverage = 0;

  int nodesCount = (int)vectNeighboursInfo.size()-1;

  estimationErrorFile << hostNode->getId() << " " << (estimated - (float)hostNode->getNeighboursCount()) / (float)hostNode->getNeighboursCount() << endl;

  //cout << "Destruction Node " << hostNode->getId() << " sent:" << packetsSent << endl;

  if (aliveAgents == 0) {
    cout << "Agents having exported info: " << vectNeighboursInfo.size() << endl;
    for (size_t i=0; i<vectNeighboursInfo.size(); i++) {
      totalNeighboursReal += vectNeighboursInfo[i].neighbours;
      totalNeighboursEstimated += vectNeighboursInfo[i].estimatedNeighbours;
      totalPacketsSent += vectNeighboursInfo[i].packetsSent;
    }

    averageNeighboursReal = (float)totalNeighboursReal / (float)nodesCount;
    averageNeighboursEstimated = totalNeighboursEstimated / (float)nodesCount;

    //  distance_t worldXSize = ScenarioParameters::getWorldXSize();
    //distance_t worldYSize = ScenarioParameters::getWorldYSize();
    //  distance_t worldZSize = ScenarioParameters::getWorldZSize();
    //   distance_t commRange =ScenarioParameters::getCommunicationRange();
    //   distance_t posX;
    //distance_t posY;
    //   distance_t posZ;

    for (size_t i=1; i<vectNeighboursInfo.size(); i++) {
      //       posX = vectNeighboursInfo[i].posX;
      //      posY = vectNeighboursInfo[i].posY;
      //       posZ = vectNeighboursInfo[i].posZ;
      //if ( posX > commRange && posX < worldXSize-commRange && posZ > commRange && posZ < worldZSize-commRange){
                        
      double diff, diff2;
      diff = vectNeighboursInfo[i].estimatedNeighbours - averageNeighboursEstimated;
      totalDifferenceEstimated += abs(diff);
      if (diff < maxDifferenceBelowAverage) maxDifferenceBelowAverage = diff;
      if (diff > maxDifferenceOverAverage) maxDifferenceOverAverage = diff;

      diff2 = vectNeighboursInfo[i].estimatedNeighbours - vectNeighboursInfo[i].neighbours;
      totalDiff += abs(diff2);
      if (diff2 < maxDiffBelowAverage) maxDiffBelowAverage = diff2;
      if (diff2 > maxDiffAboveAverage) maxDiffAboveAverage = diff2;

      int index;
      index = 50 + (int)(diff2 / 5);
      if (index < 0) index = 0;
      if (index > 99) index = 99;
      vectEstimationErrorDistribution[index]++;
      //}
    }

    averageDifferenceEstimated = totalDifferenceEstimated / (float)nodesCount;
    averageDiff = totalDiff / (float)nodesCount;

    LogSystem::SummarizeLog << "averageNeighboursReal:             " << averageNeighboursReal << endl;
    LogSystem::SummarizeLog << "averageNeighboursEstimated:        " << averageNeighboursEstimated << " ( " << (1-(averageNeighboursEstimated/averageNeighboursReal))*100 << "% )" <<endl;
    LogSystem::SummarizeLog << endl;
    LogSystem::SummarizeLog << "averageDifferenceEstimated:        " << averageDifferenceEstimated << endl;
    LogSystem::SummarizeLog << "maxDifferenceBelowAverage:         " << maxDifferenceBelowAverage << endl;
    LogSystem::SummarizeLog << "maxDifferenceOverAverage:          " << maxDifferenceOverAverage << endl;
    LogSystem::SummarizeLog << endl;
    LogSystem::SummarizeLog << "averageDiff:                       " << averageDiff << endl;
    LogSystem::SummarizeLog << "maxDiffBelowAverage:               " << maxDiffBelowAverage << endl;
    LogSystem::SummarizeLog << "maxDiffAboveAverage:               " << maxDiffAboveAverage << endl;
    LogSystem::SummarizeLog << endl;
    LogSystem::SummarizeLog << "totalPacketsSent:                   " << totalPacketsSent << endl;
    LogSystem::SummarizeLog << endl;

    cout << "writing estimation error distribution file" << endl;
    for (size_t i=1; i<vectNeighboursInfo.size(); i++) {
      //fprintf(LogSystem::EstimationLogC, "%f\n", vectNeighboursInfo[i].neighbours - vectNeighboursInfo[i].estimatedNeighbours);
      LogSystem::EstimationLog << vectNeighboursInfo[i].estimatedNeighbours;
      LogSystem::EstimationLog << " " << vectNeighboursInfo[i].neighbours;
      LogSystem::EstimationLog << " " << vectNeighboursInfo[i].neighbours - vectNeighboursInfo[i].estimatedNeighbours;
      LogSystem::EstimationLog << " " << (vectNeighboursInfo[i].neighbours / vectNeighboursInfo[i].estimatedNeighbours);
      LogSystem::EstimationLog <<  endl;
    }

    //  ofstream estimationErrorFile;
    //        string estimationErrorFileName = ScenarioParameters::getScenarioDirectory()+"/estimationErrorHisto.txt";
    //  estimationErrorFile.open(estimationErrorFileName);
    //        if ( !estimationErrorFile ) {
    //         estimationErrorFile << "*** ERROR *** Opening estimationErrorFile file failed: " << estimationErrorFileName << endl;
    //        } else {
    //         cout << "writing estimation error distribution to " << estimationErrorFileName << endl;
    //         for (size_t i=1; i<vectNeighboursInfo.size(); i++) {
    //          estimationErrorFile << vectNeighboursInfo[i].neighbours - vectNeighboursInfo[i].estimatedNeighbours << endl;
    //         }
    ////
    ////         for (size_t i=0; i<vectEstimationErrorDistribution.size(); i++) {
    ////          estimationErrorFile << vectEstimationErrorDistribution[i] << endl;
    ////         }
    //         estimationErrorFile.close();
    //        }

    estimationErrorFile.close();
  }
}

void D1DensityEstimatorAgent::initialize() {
  Node::registerSpecificBackoff(PacketType::D1_DENSITY_INIT, 0, 1000000);
  Node::registerSpecificBackoff(PacketType::D1_DENSITY_PROBE, 0, 900000000);
  //Node::registerSpecificBackoff(PacketType::D1_DENSITY_PROBE, 0, 0);
	
	string separator = "";
	if ( ScenarioParameters::getOutputBaseName().length() > 0 ) {
		separator = "-";
	}
  string estimationErrorFileName = ScenarioParameters::getScenarioDirectory() + "/" + ScenarioParameters::getOutputBaseName() + separator + "densityError" + ScenarioParameters::getDefaultExtension();
  estimationErrorFile.open(estimationErrorFileName, ofstream::out);
}

void D1DensityEstimatorAgent::processPacketGenerationEvent() {
  alreadyWorking = true;
  //if (hostNode->getId() == 10) cout << "Node " << hostNode->getId() << " tic time:" << Scheduler::now() << " round:" << roundNumber << " tic:" << ticInRound << " proba:" << probability << endl;
  if (ticInRound >= maxTic) {
    vectEstimated[roundNumber] = (vectProbesReceived[roundNumber] / probability) / maxTic;
    if (estimated == -1) {
      if ( vectProbesReceived[roundNumber] > minPacketsForTermination) {
        estimated = vectEstimated[roundNumber];
        hostNode->setEstimatedNeighbours((int)estimated);
      }
    } else {
      remainingFreeRounds--;
    }
    if (roundNumber == maxRound-1) {
      //estimated = vectEstimated[roundNumber];
      estimated = vectProbesReceived[roundNumber] / maxTic;
    }

    roundNumber++;
    if (hostNode->getId() == 22) {
      cout << "NODE 22 changing to round:" << roundNumber << endl;
    }

    ticInRound = 0;
    probability = probability * growRate;
    if (hostNode->getId() == 10) LogSystem::SummarizeLog << "Node " << hostNode->getId() << " at " << Scheduler::now() << " changing to round " << roundNumber << " probability: " << probability << endl;
  }

  if (roundNumber < maxRound && remainingFreeRounds > 0) {
    uniform_real_distribution<float> sendingRNG(0,1);
    float randomValue = sendingRNG(*(D1DensityEstimatorAgent::getSendingRNG()));

    if (randomValue <= probability) {
      packetsSent++;
      vectProbesSent[roundNumber]++;
      PacketPtr p(new Packet(PacketType::D1_DENSITY_PROBE, 10, hostNode->getId(), -1, port, flowId, flowSequenceNumber));
      hostNode->getRoutingAgent()->receivePacketFromApplication(p);
      if (hostNode->getId() == 10)
        cout << "\033[33;1m  Node 10 sent packet at round " << roundNumber <<  "\033[0m" << endl;
      if (hostNode->getId() == 11)
        cout << "\033[33;1m  Node 11 sent packet at round " << roundNumber <<  "\033[0m" << endl;
    }

    simulationTime_t t = Scheduler::now() + interval;
    Scheduler::getScheduler().schedule(new D1DensityEstimatorGenerationEvent(t,this));
  }

  ticInRound++;
}

void D1DensityEstimatorAgent::receivePacket(PacketPtr _packet) {
  // cout << " srcId:" << _packet->srcId << " srcSeq:" << _packet->srcSequenceNumber;
  // cout << " dstId:" << _packet->dstId << " port:" << _packet->port ;
  // cout << " size:" << _packet->size << endl;
  // cout << " flow:" << _packet->flowId << " flowId:" << _packet->flowSequenceNumber << endl;

  if (_packet->type == PacketType::D1_DENSITY_INIT && !alreadyWorking) {
    //cout << "Node " << hostNode->getId() << " initialization neighbours density estimation" << endl;
    alreadyWorking = true;
    simulationTime_t t = Scheduler::now();
    Scheduler::getScheduler().schedule(new D1DensityEstimatorGenerationEvent(t,this));
  }

  int size = (int)vectProbesReceived.size();
  if (_packet->type == PacketType::D1_DENSITY_PROBE && roundNumber < size) {
    vectProbesReceived[roundNumber]++;
    packetsReceived++;
  }
}

//===========================================================================================================
//
//          D11DensityEstimatorGenerationEvent  (class)
//
//===========================================================================================================

D11DensityEstimatorGenerationEvent::D11DensityEstimatorGenerationEvent(simulationTime_t _t, D11DensityEstimatorAgent *_gen) : Event(_t){
  eventType = EventType::DENSITY_ESTIMATOR_PACKET_GENERATION;
  concernedGenerator = _gen;
}

D11DensityEstimatorGenerationEvent::~D11DensityEstimatorGenerationEvent(){
}

void D11DensityEstimatorGenerationEvent::consume() {
  concernedGenerator->processPacketGenerationEvent();
}

const string D11DensityEstimatorGenerationEvent::getEventName() {
  return("D11DensityEstimatorGenerationEvent Event");
}

//===========================================================================================================
//
//          D11DensityEstimatorAgent  (class)
//
//===========================================================================================================

int D11DensityEstimatorAgent::computedMaxRound = 0;
mt19937_64 *D11DensityEstimatorAgent::sendingRandomGenerator = nullptr;
vector<NeighboursInfo_t> D11DensityEstimatorAgent::vectNeighboursInfo;
int D11DensityEstimatorAgent::aliveAgents = 0;
vector<int> D11DensityEstimatorAgent::vectEstimationErrorDistribution;
ofstream D11DensityEstimatorAgent::estimationErrorFile;

map <double,int> D11DensityEstimatorAgent::probaThrsldGrowthRate2Error5;
map <double,int> D11DensityEstimatorAgent::probaThrsldGrowthRate2Error10;
map <double,int> D11DensityEstimatorAgent::probaThrsldGrowthRate2Error20;
map <double,int> D11DensityEstimatorAgent::probaThrsldGrowthRate2Error30;
map <double,int> D11DensityEstimatorAgent::probaThrsldGrowthRate1_6Error5;
map <double,int> D11DensityEstimatorAgent::probaThrsldGrowthRate1_6Error10;
map <double,int> D11DensityEstimatorAgent::probaThrsldGrowthRate1_6Error20;
map <double,int> D11DensityEstimatorAgent::probaThrsldGrowthRate1_6Error30;
map <double,int> D11DensityEstimatorAgent::probaThrsldGrowthRate1_2Error5;
map <double,int> D11DensityEstimatorAgent::probaThrsldGrowthRate1_2Error10;
map <double,int> D11DensityEstimatorAgent::probaThrsldGrowthRate1_2Error20;
map <double,int> D11DensityEstimatorAgent::probaThrsldGrowthRate1_2Error30;

D11DensityEstimatorAgent::D11DensityEstimatorAgent(Node *_hostNode, int _flowId, int _port, PacketType _packetType, simulationTime_t _interval, int _repetition, int _seed) : ServerApplicationAgent(_hostNode) {
  //cout << "D11DensityEstimatorAgent constructor" << endl;

  static bool first = true;
  if (first){
    sendingRandomGenerator = new mt19937_64(_seed);
    first = false;
  }

  aliveAgents++;

  flowId = _flowId;
  port = _port;
  packetType = _packetType;
  interval = _interval;
  limit = _repetition;

  flowSequenceNumber = 0;
  sendingProba = 0.0;

  roundNumber = 0;
  //maxRound = ScenarioParameters::getD1MaxRound();
  maxRound = computedMaxRound;
  ticInRound = 0;
  maxTic = ScenarioParameters::getD1MaxTic();
  growRate = ScenarioParameters::getD1GrowRate();
  probability = 1.0 / pow(growRate,maxRound-1);
  minPacketsForTermination = ScenarioParameters::getD1MinPacketsForTermination();

  vectProbesReceived = vector<int>(maxRound,0);
  vectProbesSent = vector<int>(maxRound,0);
  vectEstimated = vector<double>(maxRound,0);

  estimated = -1;
  packetsReceived = 0;
  packetsSent = 0;

  alreadyWorking = false;
  remainingFreeRounds = ScenarioParameters::getD1RemainingFreeRounds();

  vectEstimationErrorDistribution = vector<int>(100,0);

  if ( ScenarioParameters::getD1GrowRate() == (float)2) {
    if ( ScenarioParameters::getD1ErrorMax() == 30 ) {
      current = probaThrsldGrowthRate2Error30.begin();
      computedMaxRound = probaThrsldGrowthRate2Error30.size();
    } else if ( ScenarioParameters::getD1ErrorMax() == 20 ) {
      current = probaThrsldGrowthRate2Error20.begin();
      computedMaxRound = probaThrsldGrowthRate2Error20.size();
    } else if ( ScenarioParameters::getD1ErrorMax() == 10 ) {
      current = probaThrsldGrowthRate2Error10.begin();
      computedMaxRound = probaThrsldGrowthRate2Error10.size();
    } else if ( ScenarioParameters::getD1ErrorMax() == 5 ) {
      current = probaThrsldGrowthRate2Error5.begin();
      computedMaxRound = probaThrsldGrowthRate2Error5.size();
    } else {
      cout << "*** ERROR: invalid D1ErrorMax:" << ScenarioParameters::getD1ErrorMax() << endl;
    }
  } else if ( ScenarioParameters::getD1GrowRate() == (float)1.6 ) {
    if ( ScenarioParameters::getD1ErrorMax() == 30 ) {
      current = probaThrsldGrowthRate1_6Error30.begin();
      computedMaxRound = probaThrsldGrowthRate1_6Error30.size();
    } else if ( ScenarioParameters::getD1ErrorMax() == 20 ) {
      current = probaThrsldGrowthRate1_6Error20.begin();
      computedMaxRound = probaThrsldGrowthRate1_6Error20.size();
    } else if ( ScenarioParameters::getD1ErrorMax() == 10 ) {
      current = probaThrsldGrowthRate1_6Error10.begin();
      computedMaxRound = probaThrsldGrowthRate1_6Error10.size();
    } else if ( ScenarioParameters::getD1ErrorMax() == 5 ) {
      current = probaThrsldGrowthRate1_6Error5.begin();
      computedMaxRound = probaThrsldGrowthRate1_6Error5.size();
    } else {
      cout << "*** ERROR: invalid D1ErrorMax:" << ScenarioParameters::getD1ErrorMax() << endl;
    }
  } else if ( ScenarioParameters::getD1GrowRate() == (float)1.2 ) {
    if ( ScenarioParameters::getD1ErrorMax() == 30 ) {
      current = probaThrsldGrowthRate1_2Error30.begin();
      computedMaxRound = probaThrsldGrowthRate1_2Error30.size();
    } else if ( ScenarioParameters::getD1ErrorMax() == 20 ) {
      current = probaThrsldGrowthRate1_2Error20.begin();
      computedMaxRound = probaThrsldGrowthRate1_2Error20.size();
    } else if ( ScenarioParameters::getD1ErrorMax() == 10 ) {
      current = probaThrsldGrowthRate1_2Error10.begin();
      computedMaxRound = probaThrsldGrowthRate1_2Error10.size();
    } else if ( ScenarioParameters::getD1ErrorMax() == 5 ) {
      current = probaThrsldGrowthRate1_2Error5.begin();
      computedMaxRound = probaThrsldGrowthRate1_2Error5.size();
    } else {
      cout << "*** ERROR: invalid D1ErrorMax:" << ScenarioParameters::getD1ErrorMax() << endl;
    }
  } else {
    cout << "*** ERROR: invalid D1GrowthRate:" << ScenarioParameters::getD1GrowRate() << endl;
    exit(EXIT_FAILURE);
  }

  probability=current->first;
  minPacketsForTermination=current->second;
}

D11DensityEstimatorAgent::~D11DensityEstimatorAgent() {
  aliveAgents--;

  //         if (hostNode->getId() < 20) {
  // //         if ( hostNode->getNeighboursCount() != estimated) {
  //                 cout << "\033[36;1mNode " << hostNode->getId() << " [real:" << hostNode->getNeighboursCount() << ", estimated:" << estimated << "]: \033[0m";
  //                 for (int i = 0; i<maxRound; i++) cout << vectProbesSent[i] << "/" << vectProbesReceived[i] << "(" << vectEstimated[i] << ") ";
  //                 cout << "rfr:" << remainingFreeRounds;
  //                 cout << endl;
  //         }

  vectNeighboursInfo.push_back({hostNode->getId(), hostNode->getXPos(), hostNode->getYPos(), hostNode->getZPos(), hostNode->getNeighboursCount(), estimated, packetsReceived, packetsSent});

  double averageNeighboursReal;
  double averageNeighboursEstimated;
  int totalNeighboursReal = 0;
  double totalNeighboursEstimated = 0;

  double totalDifferenceEstimated = 0;
  double averageDifferenceEstimated = 0;
  double maxDifferenceBelowAverage = 0;
  double maxDifferenceOverAverage = 0;

  int totalPacketsSent = 0;

  double totalDiff = 0;
  double averageDiff = 0;
  double maxDiffBelowAverage = 0;
  double maxDiffAboveAverage = 0;

  int nodesCount = (int)vectNeighboursInfo.size()-1;

  estimationErrorFile << hostNode->getId() << " " << (estimated - (float)hostNode->getNeighboursCount()) / (float)hostNode->getNeighboursCount() << " " << hostNode->getNeighboursCount() << " " << estimated << " " << packetsSent <<  endl;

  //cout << "Destruction Node " << hostNode->getId() << " sent:" << packetsSent << endl;

  if (aliveAgents == 0) {
    cout << "Agents having exported info: " << vectNeighboursInfo.size() << endl;
    for (size_t i=0; i<vectNeighboursInfo.size(); i++) {
      totalNeighboursReal += vectNeighboursInfo[i].neighbours;
      totalNeighboursEstimated += vectNeighboursInfo[i].estimatedNeighbours;
      totalPacketsSent += vectNeighboursInfo[i].packetsSent;
    }

    averageNeighboursReal = (float)totalNeighboursReal / (float)nodesCount;
    averageNeighboursEstimated = totalNeighboursEstimated / (float)nodesCount;

    //                 distance_t worldXSize = ScenarioParameters::getWorldXSize();
    //distance_t worldYSize = ScenarioParameters::getWorldYSize();
    //                 distance_t worldZSize = ScenarioParameters::getWorldZSize();
    //                 distance_t commRange =ScenarioParameters::getCommunicationRange();
    //                 distance_t posX;
    //distance_t posY;
    //                 distance_t posZ;

    for (size_t i=1; i<vectNeighboursInfo.size(); i++) {
      //                     posX = vectNeighboursInfo[i].posX;
      //                  posY = vectNeighboursInfo[i].posY;
      //                     posZ = vectNeighboursInfo[i].posZ;
      //if ( posX > commRange && posX < worldXSize-commRange && posZ > commRange && posZ < worldZSize-commRange){
                        
      double diff, diff2;
      diff = vectNeighboursInfo[i].estimatedNeighbours - averageNeighboursEstimated;
      totalDifferenceEstimated += abs(diff);
      if (diff < maxDifferenceBelowAverage) maxDifferenceBelowAverage = diff;
      if (diff > maxDifferenceOverAverage) maxDifferenceOverAverage = diff;

      diff2 = vectNeighboursInfo[i].estimatedNeighbours - vectNeighboursInfo[i].neighbours;
      totalDiff += abs(diff2);
      if (diff2 < maxDiffBelowAverage) maxDiffBelowAverage = diff2;
      if (diff2 > maxDiffAboveAverage) maxDiffAboveAverage = diff2;

      int index;
      index = 50 + (int)(diff2 / 5);
      if (index < 0) index = 0;
      if (index > 99) index = 99;
      vectEstimationErrorDistribution[index]++;
      //}
    }

    averageDifferenceEstimated = totalDifferenceEstimated / (float)nodesCount;
    averageDiff = totalDiff / (float)nodesCount;

    LogSystem::SummarizeLog << "averageNeighboursReal:             " << averageNeighboursReal << endl;
    LogSystem::SummarizeLog << "averageNeighboursEstimated:        " << averageNeighboursEstimated << " ( " << (1-(averageNeighboursEstimated/averageNeighboursReal))*100 << "% )" <<endl;
    LogSystem::SummarizeLog << endl;
    LogSystem::SummarizeLog << "averageDifferenceEstimated:        " << averageDifferenceEstimated << endl;
    LogSystem::SummarizeLog << "maxDifferenceBelowAverage:         " << maxDifferenceBelowAverage << endl;
    LogSystem::SummarizeLog << "maxDifferenceOverAverage:          " << maxDifferenceOverAverage << endl;
    LogSystem::SummarizeLog << endl;
    LogSystem::SummarizeLog << "averageDiff:                       " << averageDiff << endl;
    LogSystem::SummarizeLog << "maxDiffBelowAverage:               " << maxDiffBelowAverage << endl;
    LogSystem::SummarizeLog << "maxDiffAboveAverage:               " << maxDiffAboveAverage << endl;
    LogSystem::SummarizeLog << endl;
    LogSystem::SummarizeLog << "totalPacketsSent:                   " << totalPacketsSent << endl;
    LogSystem::SummarizeLog << endl;

                
    //                  string filename = ScenarioParameters::getScenarioDirectory()+"/pouet.data";
    //                  ofstream output(filename,std::ofstream::app); 
    //                  
    //                 output << "averageNeighboursReal:             " << averageNeighboursReal << endl;
    //                 output << "averageNeighboursEstimated:        " << averageNeighboursEstimated << " ( " << (1-(averageNeighboursEstimated/averageNeighboursReal))*100 << "% )" <<endl;
    //                 output << endl;
    //                 output << "averageDifferenceEstimated:        " << averageDifferenceEstimated << endl;
    //                 output << "maxDifferenceBelowAverage:         " << maxDifferenceBelowAverage << endl;
    //                 output << "maxDifferenceOverAverage:          " << maxDifferenceOverAverage << endl;
    //                 output<< endl;
    //                 output << "averageDiff:                       " << averageDiff << endl;
    //                 output << "maxDiffBelowAverage:               " << maxDiffBelowAverage << endl;
    //                 output << "maxDiffAboveAverage:               " << maxDiffAboveAverage << endl;
    //                 output << endl;
    //                 output<< "totalPacketsSent:                   " << totalPacketsSent << endl;
    //                 output  << endl;
                
                
                
    cout << "writing estimation error distribution file" << endl;
    for (size_t i=1; i<vectNeighboursInfo.size(); i++) {
      //fprintf(LogSystem::EstimationLogC, "%f\n", vectNeighboursInfo[i].neighbours - vectNeighboursInfo[i].estimatedNeighbours);
      LogSystem::EstimationLog << vectNeighboursInfo[i].estimatedNeighbours;
      LogSystem::EstimationLog << " " << vectNeighboursInfo[i].neighbours;
      LogSystem::EstimationLog << " " << vectNeighboursInfo[i].neighbours - vectNeighboursInfo[i].estimatedNeighbours;
      LogSystem::EstimationLog << " " << (vectNeighboursInfo[i].neighbours / vectNeighboursInfo[i].estimatedNeighbours);
      LogSystem::EstimationLog <<  endl;
    }

    //              ofstream estimationErrorFile;
    //        string estimationErrorFileName = ScenarioParameters::getScenarioDirectory()+"/estimationErrorHisto.txt";
    //              estimationErrorFile.open(estimationErrorFileName);
    //        if ( !estimationErrorFile ) {
    //              estimationErrorFile << "*** ERROR *** Opening estimationErrorFile file failed: " << estimationErrorFileName << endl;
    //        } else {
    //              cout << "writing estimation error distribution to " << estimationErrorFileName << endl;
    //              for (size_t i=1; i<vectNeighboursInfo.size(); i++) {
    //                      estimationErrorFile << vectNeighboursInfo[i].neighbours - vectNeighboursInfo[i].estimatedNeighbours << endl;
    //              }
    ////
    ////            for (size_t i=0; i<vectEstimationErrorDistribution.size(); i++) {
    ////                    estimationErrorFile << vectEstimationErrorDistribution[i] << endl;
    ////            }
    //              estimationErrorFile.close();
    //        }

    estimationErrorFile.close();
  }
}

void D11DensityEstimatorAgent::initialize() {
  Node::registerSpecificBackoff(PacketType::D1_DENSITY_INIT, 0, 1000);
  Node::registerSpecificBackoff(PacketType::D1_DENSITY_PROBE, 0, 900000000);
  //Node::registerSpecificBackoff(PacketType::D1_DENSITY_PROBE, 0, 0);
    
  string separator = "";
	if ( ScenarioParameters::getOutputBaseName().length() > 0 ) {
		separator = "-";
	}
  string estimationErrorFileName = ScenarioParameters::getScenarioDirectory() + "/" + ScenarioParameters::getOutputBaseName() + separator + "densityError" + ScenarioParameters::getDefaultExtension();

  estimationErrorFile.open(estimationErrorFileName, ofstream::out);

  //
  // growthRate 2
  //  5% error threshold
  //
  probaThrsldGrowthRate2Error5.insert(pair<double,int>(0.000003814697265625,1542));
  probaThrsldGrowthRate2Error5.insert(pair<double,int>(0.00000762939453125,1542));
  probaThrsldGrowthRate2Error5.insert(pair<double,int>(0.0000152587890625,1542));
  probaThrsldGrowthRate2Error5.insert(pair<double,int>(0.000030517578125,1542));
  probaThrsldGrowthRate2Error5.insert(pair<double,int>(0.00006103515625,1542));
  probaThrsldGrowthRate2Error5.insert(pair<double,int>(0.0001220703125,1542));
  probaThrsldGrowthRate2Error5.insert(pair<double,int>(0.000244140625,1542));
  probaThrsldGrowthRate2Error5.insert(pair<double,int>(0.00048828125,1542));
  probaThrsldGrowthRate2Error5.insert(pair<double,int>(0.0009765625,1542));
  probaThrsldGrowthRate2Error5.insert(pair<double,int>(0.001953125,1540));
  probaThrsldGrowthRate2Error5.insert(pair<double,int>(0.00390625,1535));
  probaThrsldGrowthRate2Error5.insert(pair<double,int>(0.0078125,1530));
  probaThrsldGrowthRate2Error5.insert(pair<double,int>(0.015625,1520));
  probaThrsldGrowthRate2Error5.insert(pair<double,int>(0.03125,1500));
  probaThrsldGrowthRate2Error5.insert(pair<double,int>(0.0625,1450));
  probaThrsldGrowthRate2Error5.insert(pair<double,int>(0.125,1350));
  probaThrsldGrowthRate2Error5.insert(pair<double,int>(0.25,1150));
  probaThrsldGrowthRate2Error5.insert(pair<double,int>(0.5,760));
  probaThrsldGrowthRate2Error5.insert(pair<double,int>(1,-1));

  //
  // growthRate 2
  //  10% error threshold
  //
  probaThrsldGrowthRate2Error10.insert(pair<double,int>(0.000003814697265625,387));
  probaThrsldGrowthRate2Error10.insert(pair<double,int>(0.00000762939453125,387));
  probaThrsldGrowthRate2Error10.insert(pair<double,int>(0.0000152587890625,387));
  probaThrsldGrowthRate2Error10.insert(pair<double,int>(0.000030517578125,386));
  probaThrsldGrowthRate2Error10.insert(pair<double,int>(0.00006103515625,386));
  probaThrsldGrowthRate2Error10.insert(pair<double,int>(0.0001220703125,386));
  probaThrsldGrowthRate2Error10.insert(pair<double,int>(0.000244140625,386));
  probaThrsldGrowthRate2Error10.insert(pair<double,int>(0.00048828125,386));
  probaThrsldGrowthRate2Error10.insert(pair<double,int>(0.0009765625,386));
  probaThrsldGrowthRate2Error10.insert(pair<double,int>(0.001953125,386));
  probaThrsldGrowthRate2Error10.insert(pair<double,int>(0.00390625,385));
  probaThrsldGrowthRate2Error10.insert(pair<double,int>(0.0078125,384));
  probaThrsldGrowthRate2Error10.insert(pair<double,int>(0.015625,380));
  probaThrsldGrowthRate2Error10.insert(pair<double,int>(0.03125,375));
  probaThrsldGrowthRate2Error10.insert(pair<double,int>(0.0625,362));
  probaThrsldGrowthRate2Error10.insert(pair<double,int>(0.125,338));
  probaThrsldGrowthRate2Error10.insert(pair<double,int>(0.25,288));
  probaThrsldGrowthRate2Error10.insert(pair<double,int>(0.5,190));
  probaThrsldGrowthRate2Error10.insert(pair<double,int>(1,-1));

  //
  // growthRate 2
  //  20% error threshold
  //
  probaThrsldGrowthRate2Error20.insert(pair<double,int>(0.000003814697265625,99));
  probaThrsldGrowthRate2Error20.insert(pair<double,int>(0.00000762939453125,99));
  probaThrsldGrowthRate2Error20.insert(pair<double,int>(0.0000152587890625,99));
  probaThrsldGrowthRate2Error20.insert(pair<double,int>(0.000030517578125,99));
  probaThrsldGrowthRate2Error20.insert(pair<double,int>(0.00006103515625,99));
  probaThrsldGrowthRate2Error20.insert(pair<double,int>(0.0001220703125,99));
  probaThrsldGrowthRate2Error20.insert(pair<double,int>(0.000244140625,99));
  probaThrsldGrowthRate2Error20.insert(pair<double,int>(0.00048828125,99));
  probaThrsldGrowthRate2Error20.insert(pair<double,int>(0.0009765625,98));
  probaThrsldGrowthRate2Error20.insert(pair<double,int>(0.001953125,98));
  probaThrsldGrowthRate2Error20.insert(pair<double,int>(0.00390625,98));
  probaThrsldGrowthRate2Error20.insert(pair<double,int>(0.0078125,98));
  probaThrsldGrowthRate2Error20.insert(pair<double,int>(0.015625,97));
  probaThrsldGrowthRate2Error20.insert(pair<double,int>(0.03125,95));
  probaThrsldGrowthRate2Error20.insert(pair<double,int>(0.0625,92));
  probaThrsldGrowthRate2Error20.insert(pair<double,int>(0.125,86));
  probaThrsldGrowthRate2Error20.insert(pair<double,int>(0.25,73));
  probaThrsldGrowthRate2Error20.insert(pair<double,int>(0.5,48));
  probaThrsldGrowthRate2Error20.insert(pair<double,int>(1,-1));

  //
  // growthRate 2
  //  30% error threshold
  //
  probaThrsldGrowthRate2Error30.insert(pair<double,int>(0.000003814697265625,45));
  probaThrsldGrowthRate2Error30.insert(pair<double,int>(0.00000762939453125,45));
  probaThrsldGrowthRate2Error30.insert(pair<double,int>(0.0000152587890625,45));
  probaThrsldGrowthRate2Error30.insert(pair<double,int>(0.000030517578125,45));
  probaThrsldGrowthRate2Error30.insert(pair<double,int>(0.00006103515625,45));
  probaThrsldGrowthRate2Error30.insert(pair<double,int>(0.0001220703125,45));
  probaThrsldGrowthRate2Error30.insert(pair<double,int>(0.000244140625,45));
  probaThrsldGrowthRate2Error30.insert(pair<double,int>(0.00048828125,45));
  probaThrsldGrowthRate2Error30.insert(pair<double,int>(0.0009765625,45));
  probaThrsldGrowthRate2Error30.insert(pair<double,int>(0.001953125,45));
  probaThrsldGrowthRate2Error30.insert(pair<double,int>(0.00390625,45));
  probaThrsldGrowthRate2Error30.insert(pair<double,int>(0.0078125,45));
  probaThrsldGrowthRate2Error30.insert(pair<double,int>(0.015625,45));
  probaThrsldGrowthRate2Error30.insert(pair<double,int>(0.03125,44));
  probaThrsldGrowthRate2Error30.insert(pair<double,int>(0.0625,42));
  probaThrsldGrowthRate2Error30.insert(pair<double,int>(0.125,40));
  probaThrsldGrowthRate2Error30.insert(pair<double,int>(0.25,34));
  probaThrsldGrowthRate2Error30.insert(pair<double,int>(0.5,22));
  probaThrsldGrowthRate2Error30.insert(pair<double,int>(1,-1));

  //      
  // growthRate 1.6
  //  5% error threshold
  //
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.00000308148791101958, 1548));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.00000493038065763132, 1548));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.00000788860905221012, 1548));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.0000126217744835362, 1548));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.0000201948391736579, 1457));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.0000323117426778526, 1547));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.0000516987882845642, 1546));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.0000827180612553028, 1546));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.000132348898008, 1546));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.000211758236814, 1546));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.000338813178902, 1546));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.000542101086243, 1545));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.000867361737988, 1545));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.0013877788, 1545));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.002220446, 1544));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.0035527137, 1544));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.0056843419, 1543));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.009094947, 1542 ));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.0145519152, 1540));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.0232830644, 1535));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.037252903, 1530));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.0596046448, 1520));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.0953674316, 1500));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.1525878906, 1450));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.244140625, 1342 ));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.390625, 1150));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(0.625, 760));
  probaThrsldGrowthRate1_6Error5.insert(pair<double,int>(1, -1));

  //
  // growthRate 1.6
  //  10% error threshold
  //
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.00000308148791101958, 387));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.00000493038065763132, 387));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.00000788860905221012, 387));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.0000126217744835362, 387));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.0000201948391736579, 387));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.0000323117426778526, 387));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.0000516987882845642, 387));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.0000827180612553028, 387));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.000132348898008, 387));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.000211758236814, 387));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.000338813178902, 387));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.000542101086243, 387));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.000867361737988, 387));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.0013877788, 386));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.002220446, 386));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.0035527137, 385));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.0056843419, 384));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.009094947, 383 ));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.0145519152, 381));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.0232830644, 377));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.037252903, 372));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.0596046448, 364));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.0953674316, 350));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.1525878906, 327));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.244140625, 291 ));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.390625, 235));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(0.625, 139));
  probaThrsldGrowthRate1_6Error10.insert(pair<double,int>(1, -1));

  //
  // growthRate 1.6
  //  20% error threshold
  //
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.00000308148791101958, 99));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.00000493038065763132, 99));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.00000788860905221012, 99));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.0000126217744835362, 99));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.0000201948391736579, 99));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.0000323117426778526, 99));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.0000516987882845642, 99));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.0000827180612553028, 99));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.000132348898008, 99));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.000211758236814, 99));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.000338813178902, 99));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.000542101086243, 99));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.000867361737988, 98));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.0013877788, 98));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.002220446, 98));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.0035527137, 98));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.0056843419, 98));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.009094947, 98 ));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.0145519152, 97));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.0232830644, 96));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.037252903, 95));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.0596046448, 93));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.0953674316, 89));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.1525878906, 84));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.244140625, 74 ));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.390625, 59));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(0.625, 35));
  probaThrsldGrowthRate1_6Error20.insert(pair<double,int>(1, -1));

  //
  // growthRate 1.6
  //  30% error threshold
  //
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.00000308148791101958, 45));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.00000493038065763132, 45));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.00000788860905221012, 45));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.0000126217744835362, 45));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.0000201948391736579, 45));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.0000323117426778526, 45));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.0000516987882845642, 45));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.0000827180612553028, 45));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.000132348898008, 45));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.000211758236814, 45));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.000338813178902, 45));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.000542101086243, 45));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.000867361737988, 45));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.0013877788, 45));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.002220446, 45));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.0035527137, 45));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.0056843419, 45));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.009094947, 45 ));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.0145519152, 45));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.0232830644, 44));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.037252903, 44));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.0596046448, 43));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.0953674316, 41));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.1525878906, 38));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.244140625, 35 ));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.390625, 28));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(0.625, 17));
  probaThrsldGrowthRate1_6Error30.insert(pair<double,int>(1, -1));

  // growthRate 1.2
  //  5% error threshold
  //
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.0014108315,1550));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.0016929978,1549));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.0020315973,1549));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.0024379168,1548));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.0029255002,1547));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.0035106002,1546));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.0042127202,1545));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.0050552643,1544));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.0060663171,1542));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.0072795806,1541));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.0087354967,1540));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.010482596,1539));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.0125791152,1537));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.0150949383,1535));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.0181139259,1530));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.0217367111,1525));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.0260840533,1520));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.031300864,1510));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.0375610368,1500));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.0450732441,1490));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.0540878929,1480));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.0649054715,1460));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.0778865658,1440));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.093463879,1420));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.1121566548,1380));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.1345879857,1350));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.1615055829,1300));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.1938066995,1250));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.2325680394,1180));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.2790816472,1100));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.3348979767,1019));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.401877572,917));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.4822530864,791));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.5787037037,640));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.6944444444,471));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(0.8333333333,235));
  probaThrsldGrowthRate1_2Error5.insert(pair<double,int>(1.0,-1));

  // growthRate 1.2
  //  10% error threshold
  //
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.0014108315, 386));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.0016929978, 386));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.0020315973, 386));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.0024379168, 386));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.0029255002, 385));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.0035106002, 385));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.0042127202, 385));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.0050552643, 385));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.0060663171, 384));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.0072795806, 384));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.0087354967, 383));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.010482596, 382));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.0125791152, 382));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.0150949383, 381));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.0181139259, 379));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.0217367111, 378));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.0260840533, 376));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.031300864,  375));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.0375610368, 372));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.0450732441, 369));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.0540878929, 366));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.0649054715, 366));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.0778865658, 356));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.093463879, 350));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.1121566548, 343));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.1345879857, 333));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.1615055829, 324));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.1938066995, 311));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.2325680394, 296));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.2790816472, 277));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.3348979767, 255));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.401877572, 230));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.4822530864, 198));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.5787037037, 157));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.6944444444, 112));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(0.8333333333, 60));
  probaThrsldGrowthRate1_2Error10.insert(pair<double,int>(1, -1));

  // growthRate 1.2
  //  20% error threshold
  //
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.0014108315, 98));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.0016929978, 98));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.0020315973, 98));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.0024379168, 98));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.0029255002, 98));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.0035106002, 98));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.0042127202, 98));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.0050552643, 98));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.0060663171, 98));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.0072795806, 98));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.0087354967, 98));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.010482596, 98));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.0125791152, 97));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.0150949383, 97));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.0181139259, 97));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.0217367111, 96));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.0260840533, 96));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.031300864,  95));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.0375610368, 95));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.0450732441, 94));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.0540878929, 93));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.0649054715, 92));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.0778865658, 91));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.093463879, 89));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.1121566548, 87));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.1345879857, 85));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.1615055829, 83));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.1938066995, 79));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.2325680394, 75));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.2790816472, 72));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.3348979767, 64));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.401877572, 59));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.4822530864, 51));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.5787037037, 42));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.6944444444, 32));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(0.8333333333, 17));
  probaThrsldGrowthRate1_2Error20.insert(pair<double,int>(1, -1));

  // growthRate 1.2
  //  30% error threshold
  //
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.0014108315, 45));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.0016929978, 45));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.0020315973, 45));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.0024379168, 45));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.0029255002, 45));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.0035106002, 45));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.0042127202, 45));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.0050552643, 45));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.0060663171, 45));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.0072795806, 45));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.0087354967, 45));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.010482596, 45));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.0125791152, 45));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.0150949383, 45));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.0181139259, 44));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.0217367111, 44));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.0260840533, 44));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.031300864,  44));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.0375610368, 44));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.0450732441, 43));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.0540878929, 43));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.0649054715, 42));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.0778865658, 42));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.093463879, 41));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.1121566548, 41));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.1345879857, 40));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.1615055829, 38));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.1938066995, 37));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.2325680394, 35));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.2790816472, 33));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.3348979767, 31));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.401877572, 27));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.4822530864, 25));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.5787037037, 20));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.6944444444, 14));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(0.8333333333, 10));
  probaThrsldGrowthRate1_2Error30.insert(pair<double,int>(1, -1));
}

void D11DensityEstimatorAgent::processPacketGenerationEvent() {
  alreadyWorking = true;
  //int nodeDisplayed = 402;
  //if (hostNode->getId() == 10) cout << "Node " << hostNode->getId() << " tic time:" << Scheduler::now() << " round:" << roundNumber << " tic:" << ticInRound << " proba:" << probability << endl;
  if (ticInRound >= maxTic) {
    vectEstimated[roundNumber] = (vectProbesReceived[roundNumber] / probability) / maxTic;

    //                if (hostNode->getId() == nodeDisplayed) {
    //                    cout << ">>> node " << nodeDisplayed << " tin:" << ticInRound << " prob:" << probability << " round:" << roundNumber << " received:" <<  vectProbesReceived[roundNumber] << " estimation:" << vectEstimated[roundNumber] << " real:" << hostNode->getNeighboursCount() << endl;
    //                }

    if (estimated == -1) {
      if ( vectProbesReceived[roundNumber] > minPacketsForTermination) {
        estimated = vectEstimated[roundNumber];
        //                                if (hostNode->getId() == nodeDisplayed) {
        //                                    cout << "=== node " << nodeDisplayed << " final estimation: " << estimated << " at round " << roundNumber << endl;
        //                                }
        hostNode->setEstimatedNeighbours((int)estimated);
        // Uncomment to plug deden to the sleeping node system
        if(SleepingNode* sleepingHostNode = dynamic_cast<SleepingNode*>(hostNode) ){
          simulationTime_t t = 500000000000;
          Scheduler::getScheduler().schedule(new sleepingSetUpEvent(t,sleepingHostNode));
        }
      }
    } //else {
    //      remainingFreeRounds--;
    //}
    if ( roundNumber == maxRound-1 && estimated == -1 ) {
      //estimated = vectEstimated[roundNumber];
      estimated = vectProbesReceived[roundNumber] / maxTic;
    }
    roundNumber++;
    ticInRound = 0;
    current++;
    probability=current->first;
    minPacketsForTermination=current->second;
    //                 if (hostNode->getId() == 22) LogSystem::SummarizeLog << "Node " << hostNode->getId() << " at " << Scheduler::now() << " changing to round " << roundNumber << " probability: " << current->first << endl;
  }
  //if (roundNumber < maxRound && (remainingFreeRounds > 0) ) {
  if (roundNumber < maxRound && (estimated == -1 || remainingFreeRounds > 0) ) {
    uniform_real_distribution<float> sendingRNG(0,1);
    float randomValue = sendingRNG(*(D11DensityEstimatorAgent::getSendingRNG()));

    //                if (hostNode->getId() == nodeDisplayed) {
    //                    cout << ">>> node " << nodeDisplayed << " participates in round: " << roundNumber << " (maxRound:" << maxRound << " estimated:" << estimated << " rfr:" << remainingFreeRounds << ")" << endl;
    //                }

    if (randomValue <= probability) {
      packetsSent++;
      vectProbesSent[roundNumber]++;
      PacketPtr p(new Packet(PacketType::D1_DENSITY_PROBE, 10, hostNode->getId(), -1, port, flowId, flowSequenceNumber));
      hostNode->getRoutingAgent()->receivePacketFromApplication(p);
      //                         if (hostNode->getId() == 10)
      //                             cout << "\033[33;1m  Node 10 sent packet at round " << roundNumber <<  "\033[0m" << endl;
      //                         if (hostNode->getId() == 11)
      //                             cout << "\033[33;1m  Node 11 sent packet at round " << roundNumber <<  "\033[0m" << endl;
    }
    if (estimated != -1) remainingFreeRounds--;
    simulationTime_t t = Scheduler::now() + interval;
    Scheduler::getScheduler().schedule(new D11DensityEstimatorGenerationEvent(t,this));
  }
        
  if ( remainingFreeRounds == 0 ){
    //simulationTime_t t = 200600000000;
    if(SleepingNode* sleepingHostNode = dynamic_cast<SleepingNode*>(hostNode) ){
      simulationTime_t t = 500000000000;
      Scheduler::getScheduler().schedule(new sleepingSetUpEvent(t,sleepingHostNode));
    }
        
  }

  ticInRound++;

}

void D11DensityEstimatorAgent::receivePacket(PacketPtr _packet) {
  //      cout << " srcId:" << _packet->srcId << " srcSeq:" << _packet->srcSequenceNumber;
  //      cout << " dstId:" << _packet->dstId << " port:" << _packet->port ;
  //      cout << " size:" << _packet->size << endl;
  //      cout << " flow:" << _packet->flowId << " flowId:" << _packet->flowSequenceNumber << endl;

  if (_packet->type == PacketType::D1_DENSITY_INIT && !alreadyWorking) {
    //cout << "Node " << hostNode->getId() << " initialization neighbours density estimation" << endl;
    alreadyWorking = true;
    simulationTime_t t = Scheduler::now();
    Scheduler::getScheduler().schedule(new D11DensityEstimatorGenerationEvent(t,this));
  }

  int size = (int)vectProbesReceived.size();
  if (_packet->type == PacketType::D1_DENSITY_PROBE && roundNumber < size) {
    vectProbesReceived[roundNumber]++;
    packetsReceived++;
    //                if ( hostNode->getId() == 22 && probability == 1) {
    //                    cout << "+++ pkts:" << vectProbesReceived[roundNumber] << " src:" << _packet->srcId << endl;
    //                }
  }
}


//===========================================================================================================
//
//          D2DensityEstimatorGenerationEvent  (class)
//
//===========================================================================================================

D2DensityEstimatorGenerationEvent::D2DensityEstimatorGenerationEvent(simulationTime_t _t, D2DensityEstimatorAgent *_gen) : Event(_t){
  eventType = EventType::DENSITY_ESTIMATOR_PACKET_GENERATION;
  concernedGenerator = _gen;
}

D2DensityEstimatorGenerationEvent::~D2DensityEstimatorGenerationEvent(){
}

void D2DensityEstimatorGenerationEvent::consume() {
  concernedGenerator->processPacketGenerationEvent();
}

const string D2DensityEstimatorGenerationEvent::getEventName() {
  return("D12ensityEstimatorGenerationEvent Event");
}

//===========================================================================================================
//
//          D2DensityEstimatorProbePacket  (class)
//
//===========================================================================================================

D2DensityEstimatorProbePacket::D2DensityEstimatorProbePacket(int _size, int _srcId, int _dstId, int _port, int _flowId, int _flowSequenceNumber, double _probability) : Packet(PacketType::D2_DENSITY_PROBE, _size,  _srcId, _dstId, _port, _flowId, _flowSequenceNumber) {
  probability = _probability;
}

PacketPtr D2DensityEstimatorProbePacket::clone() {
  PacketPtr clone = std::make_shared<D2DensityEstimatorProbePacket>(size, srcId, dstId, port, flowId, flowSequenceNumber, probability);
  return(clone);
}

//===========================================================================================================
//
//          D2DensityEstimatorAgent  (class)
//
//===========================================================================================================

int D2DensityEstimatorAgent::initPacketSize = 40;
int D2DensityEstimatorAgent::probePacketSize = 40;
simulationTime_t D2DensityEstimatorAgent::ticDuration = 10000000;
int D2DensityEstimatorAgent::maxRound = 20;
int D2DensityEstimatorAgent::maxTic = 5;
double D2DensityEstimatorAgent::growRate = 2;
int D2DensityEstimatorAgent::minPacketsForTermination = 100;

mt19937 *D2DensityEstimatorAgent::sendingRandomGenerator = nullptr;
uniform_real_distribution<float> D2DensityEstimatorAgent::sendingDistribution(0,1);

long D2DensityEstimatorAgent::totalProbesSent = 0;
long D2DensityEstimatorAgent::totalProbesReceived = 0;

int D2DensityEstimatorAgent::nbAgentsAlive = 0;

vector<NeighboursInfo_t> D2DensityEstimatorAgent::vectNeighboursInfo;
vector<int> D2DensityEstimatorAgent::vectEstimationErrorDistribution;

D2DensityEstimatorAgent::D2DensityEstimatorAgent(Node *_hostNode, int _flowId, int _port) : ServerApplicationAgent(_hostNode) {
  port = _port;
  flowId = _flowId;
  flowSequenceNumber = 0;

  alreadyWorking = false;
  currentRound = 0;
  currentTic = 0;
  currentFlowSequenceNumber = 0;
  probability = 1.0 / pow(growRate,maxRound);
  if (hostNode->getId() == 0) cout << "initial probability: " << probability << endl;

  roundEstimation = 0;
  probesReceivedThisRound = 0;
  remainingFreeRounds = 3;
  finalEstimation = -1;
  probesReceived = 0;
  probesSent = 0;

  vectProbesReceived = vector<int>(maxRound,0);
  vectProbesSent = vector<int>(maxRound,0);
  vectEstimated = vector<double>(maxRound,0);

  vectEstimationErrorDistribution = vector<int>(100,0);

  nbAgentsAlive++;
}

D2DensityEstimatorAgent::~D2DensityEstimatorAgent() {
  nbAgentsAlive--;
  if (hostNode->getId() < 20) {
    cout << "\033[36;1mNode " << hostNode->getId() << " [real:" << hostNode->getNeighboursCount() << ", estimated:" << finalEstimation << "]: \033[0m";
    for (int i = 0; i<maxRound; i++) cout << vectProbesSent[i] << "/" << vectProbesReceived[i] << "(" << vectEstimated[i] << ") ";
    cout << "rfr:" << remainingFreeRounds;
    cout << endl;
  }

  vectNeighboursInfo.push_back({hostNode->getId(), hostNode->getXPos(), hostNode->getYPos(), hostNode->getZPos(), hostNode->getNeighboursCount(), finalEstimation, probesReceived, probesSent});

  double averageNeighboursReal;
  double averageNeighboursEstimated;
  int totalNeighboursReal = 0;
  double totalNeighboursEstimated = 0;

  double totalDifferenceEstimated = 0;
  double averageDifferenceEstimated = 0;
  double maxDifferenceBelowAverage = 0;
  double maxDifferenceOverAverage = 0;

  int totalPacketsSent = 0;

  double totalDiff = 0;
  double averageDiff = 0;
  double maxDiffBelowAverage = 0;
  double maxDiffAboveAverage = 0;

  int nodesCount = (int)vectNeighboursInfo.size()-1;

  if (nbAgentsAlive == 0) {
    cout << "Total number of probes sent:     " << totalProbesSent << endl;
    cout << "Total number of probes received: " << totalProbesReceived << endl;
    cout << "ratio:                           " << (double)totalProbesReceived / (double)totalProbesSent << endl;
    cout << endl;

    cout << "Agents having exported info: " << vectNeighboursInfo.size() << endl;
    for (size_t i=1; i<vectNeighboursInfo.size(); i++) {
      totalNeighboursReal += vectNeighboursInfo[i].neighbours;
      totalNeighboursEstimated += vectNeighboursInfo[i].estimatedNeighbours;
      totalPacketsSent += vectNeighboursInfo[i].packetsSent;
    }

    averageNeighboursReal = (double)totalNeighboursReal / (double)nodesCount;
    averageNeighboursEstimated = totalNeighboursEstimated / (double)nodesCount;



    for (size_t i=1; i<vectNeighboursInfo.size(); i++) {
      double diff, diff2;
      diff = averageNeighboursEstimated - vectNeighboursInfo[i].estimatedNeighbours;
      totalDifferenceEstimated += abs(diff);
      if (diff < maxDifferenceBelowAverage) maxDifferenceBelowAverage = diff;
      if (diff > maxDifferenceOverAverage) maxDifferenceOverAverage = diff;

      diff2 = vectNeighboursInfo[i].neighbours - vectNeighboursInfo[i].estimatedNeighbours;
      totalDiff += abs(diff2);
      if (diff2 < maxDiffBelowAverage) maxDiffBelowAverage = diff2;
      if (diff2 > maxDiffAboveAverage) maxDiffAboveAverage = diff2;

      int index;
      index = 50 + (int)(diff2 / 5);
      if (index < 0) index = 0;
      if (index > 99) index = 99;
      vectEstimationErrorDistribution[index]++;
    }

    averageDifferenceEstimated = totalDifferenceEstimated / (float)nodesCount;
    averageDiff = totalDiff / (float)nodesCount;

    LogSystem::SummarizeLog << "nodesCount:                        " << nodesCount << endl;
    LogSystem::SummarizeLog << "averageNeighboursReal:             " << averageNeighboursReal << endl;
    LogSystem::SummarizeLog << "averageNeighboursEstimated:        " << averageNeighboursEstimated << " ( " << (1-(averageNeighboursEstimated/averageNeighboursReal))*100 << "% )" <<endl;
    LogSystem::SummarizeLog << endl;
    LogSystem::SummarizeLog << "averageDifferenceEstimated:        " << averageDifferenceEstimated << endl;
    LogSystem::SummarizeLog << "maxDifferenceBelowAverage:         " << maxDifferenceBelowAverage << endl;
    LogSystem::SummarizeLog << "maxDifferenceOverAverage:          " << maxDifferenceOverAverage << endl;
    LogSystem::SummarizeLog << endl;
    LogSystem::SummarizeLog << "averageDiff:                       " << averageDiff << endl;
    LogSystem::SummarizeLog << "maxDiffBelowAverage:               " << maxDiffBelowAverage << endl;
    LogSystem::SummarizeLog << "maxDiffAboveAverage:               " << maxDiffAboveAverage << endl;
    LogSystem::SummarizeLog << endl;
    LogSystem::SummarizeLog << "totalPacketsSent:                   " << totalPacketsSent << endl;
    LogSystem::SummarizeLog << endl;
  }
}

void D2DensityEstimatorAgent::initializeAgent() {
  Node::registerSpecificBackoff(PacketType::D2_DENSITY_INIT, 0, 100000);
  Node::registerSpecificBackoff(PacketType::D2_DENSITY_PROBE, 0, 4000000000);

  initPacketSize = 40;
  probePacketSize = 40;
  ticDuration = 4000000000;
  maxRound = 20;
  maxTic = 10;
  growRate = 1.8;
  minPacketsForTermination = 300;

  sendingRandomGenerator = new mt19937(0);
}

void D2DensityEstimatorAgent::processPacketGenerationEvent() {
  if (currentTic >= maxTic) {
    roundEstimation /= maxTic;
    vectEstimated[currentRound] = roundEstimation;
    vectProbesReceived[currentRound] = probesReceivedThisRound;
    if (finalEstimation == -1) {
      if ( vectProbesReceived[currentRound] > minPacketsForTermination) {
        finalEstimation = vectEstimated[currentRound];
        //cout << "Final estimation: " << finalEstimation << endl;
        //hostNode->setEstimatedNeighbours(estimated);
      }
    } else {
      remainingFreeRounds--;
    }

    if (currentRound == maxRound-1) {
      finalEstimation = vectEstimated[currentRound];
    }

    if (hostNode->getId() == 0) {
      cout << "Density Estimator on node " << hostNode->getId() << " at round " << currentRound;
      cout << " probability: " << probability << " probesReceivedThisRound: " << probesReceivedThisRound ;
      cout << " estimation: " << roundEstimation << endl;
    }

    currentRound++;
    currentTic = 0;
    probability *= growRate;
    probesReceivedThisRound = 0;
    roundEstimation = 0;
  }

  if (currentRound < maxRound && remainingFreeRounds > 0) {
    float randomValue = sendingDistribution(*sendingRandomGenerator);
    if (randomValue <= probability) {
      PacketPtr packet(new D2DensityEstimatorProbePacket(probePacketSize, hostNode->getId(), -1, port, 42, currentFlowSequenceNumber, probability));
      shared_ptr<D2DensityEstimatorProbePacket> p2 =  static_pointer_cast<D2DensityEstimatorProbePacket>(packet);
      //cout << "envoi: " << p2->probability << endl;
      hostNode->getRoutingAgent()->receivePacketFromApplication(packet);
      vectProbesSent[currentRound]++;
      totalProbesSent++;
      probesSent++;
      currentFlowSequenceNumber++;
    }
    simulationTime_t t = Scheduler::now() + ticDuration;
    Scheduler::getScheduler().schedule(new D2DensityEstimatorGenerationEvent(t,this));
  }

  currentTic++;
}

void D2DensityEstimatorAgent::receivePacket(PacketPtr _packet) {
  if (_packet->type == PacketType::D2_DENSITY_INIT && !alreadyWorking) {
    //cout << "Node " << hostNode->getId() << " initialization of neighbours density estimation" << endl;
    alreadyWorking = true;
    simulationTime_t t = Scheduler::now() + ticDuration;
    Scheduler::getScheduler().schedule(new D2DensityEstimatorGenerationEvent(t,this));
  }
  if (_packet->type == PacketType::D2_DENSITY_PROBE) {
    shared_ptr<D2DensityEstimatorProbePacket> packet =  static_pointer_cast<D2DensityEstimatorProbePacket>(_packet);
    if (hostNode->getId() == 0) {
      //cout << "  reception: " << packet->probability << endl;
    }
    roundEstimation += 1 / packet->probability;
    totalProbesReceived++;
    probesReceived++;
    probesReceivedThisRound++;
  }
}



//==============================================================================
//
//          sleepingSetUpEvent  (class)
//
//==============================================================================


sleepingSetUpEvent::sleepingSetUpEvent(simulationTime_t _t, Node* _node)
  : Event(_t) {
  eventType = EventType::SLEEPING_SETUP_EVENT;
  node = _node;
}

const string sleepingSetUpEvent::getEventName() {
  return "sleeping setup Event";
}

void sleepingSetUpEvent::consume() {
  if (SleepingNode* cnode = dynamic_cast<SleepingNode*>(node))
    cnode->processSleepingSetUpEvent();
  else
    cerr << " Bad usage of a sleeping node : set up not possible " << endl;
}
