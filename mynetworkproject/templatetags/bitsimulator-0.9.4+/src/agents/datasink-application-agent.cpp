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

#include "scheduler.h"
#include "datasink-application-agent.h"
#include "events.h"

using DataSinkLogEvent = CallMethodEvent<DataSinkApplicationAgent,
  &DataSinkApplicationAgent::processDataSinkLogEvent,
  EventType::DATA_SINK_LOG>;


//===========================================================================================================
//
//          DataSinkApplicationAgent  (class)
//
//===========================================================================================================

int DataSinkApplicationAgent::totalPacketsReceived = 0;
int DataSinkApplicationAgent::totalPacketsCollisions = 0;
int DataSinkApplicationAgent::totalCorruptedBits = 0;

int DataSinkApplicationAgent::aliveAgents = 0;
map<int,int> DataSinkApplicationAgent::globalIdReceived = map<int,int>();
set<int> DataSinkApplicationAgent::reachability= set<int>();
int DataSinkApplicationAgent::maxAliveAgents =0;
map<int,int> DataSinkApplicationAgent::histoBitsCollisions = map<int,int>();

DataSinkApplicationAgent::DataSinkApplicationAgent(Node *_hostNode) : ServerApplicationAgent(_hostNode) {
  //  cout << "DataSinkApplicationAgent constructor on node " << _hostNode->getId() << endl;
  nbPacketsReceived = 0;
  loggingInterval = 0;

  packetsReceivedDuringInterval = 0;
  bitsReceivedDuringInterval = 0;

  aliveAgents++;
  maxAliveAgents++;

  // if (hostNode->getId() == 3) {
  //     startLogging(100000000);
  // }
}

DataSinkApplicationAgent::~DataSinkApplicationAgent() {
//     cout << "DataSinkApplicationAgent destructor on node " << hostNode->getId() << " id received "<< idPacketReceived.size() << " number of receptions " << nbPacketsReceived << endl;
  //  cout << "DataSinkApplicationAgent destructor on node " << hostNode->getId() << " " << nbPacketsReceived << " packets have been received  total:" << totalPacketsReceived << endl;

  aliveAgents--;
  //         cout << " aliveAgents " << aliveAgents << endl;

  globalIdReceived.insert(pair<int,int>(hostNode->getId(),idPacketReceived.size()));

  if (aliveAgents == 0 ){
    // file << endl;
    //      string filename = ScenarioParameters::getScenarioDirectory()+"/reachability_cost.data";
    //       ofstream file(filename,std::ofstream::app);
    //
    //       file << ScenarioParameters::getSlrBackoffredundancy() << " " << reachability.size() << " ";
    //        file << ScenarioParameters::getGenericNodesRNGSeed() << " " << reachability.size() << " ";

    cout << " Total packets received: " << totalPacketsReceived << endl;
    cout << " Total packets collided: " << totalPacketsCollisions<< endl;
    cout << " Total bits corrupted  : " << totalCorruptedBits << endl;

    //       file << ScenarioParameters::getGenericNodesRNGSeed() << "\t" << ScenarioParameters::getDefaultBeta() << "\t" << totalPacketsReceived << "\t" << totalPacketsCollisions << "\t" << totalCorruptedBits;
    //       file << endl;
    //       file.close();
  }
}

void DataSinkApplicationAgent::receivePacket(PacketPtr _packet) {
  nbPacketsReceived++;
  totalPacketsReceived++;
  //  packetsReceivedDuringIdatanterval++;
  bitsReceivedDuringInterval += _packet->size;

  bool add = true;
  for (auto it = idPacketReceived.begin(); it != idPacketReceived.end() ; it++) {
    if (it->first == _packet->flowId && it->second == _packet->flowSequenceNumber) {
      add = false;
      break;
    }
  }

  if (add) {
    idPacketReceived.push_back(pair<int,int>(_packet->flowId,_packet->flowSequenceNumber));
    packetsDelay.insert(pair<int,simulationTime_t>(_packet->flowSequenceNumber,Scheduler::now()-_packet->creationTime));
    reachability.insert(hostNode->getId());
  }

  if (_packet->type == PacketType::DATA) {
    int nbCorruptedBits = _packet->modifiedBitsPositions.size();
    if (nbCorruptedBits > 0) {
      //      cout << "nbCorruptedBits: " << nbCorruptedBits << " on node " << hostNode->getId() << endl;
      nbPacketsCollisions++;
      totalPacketsCollisions++;
      receivedCorruptedBits += nbCorruptedBits ;
      totalCorruptedBits += nbCorruptedBits ;
    }

    auto it = histoBitsCollisions.find(nbCorruptedBits);
    if ( it != histoBitsCollisions.end() ){
      histoBitsCollisions[nbCorruptedBits]++;
    }
    else{
      histoBitsCollisions.insert(pair<int,int>(nbCorruptedBits,1));
    }
  }

  //  if (_packet->flowId == 1) {
  //   cout << _packet->payload->getVal(0) << _packet->payload->getVal(1) << _packet->payload->getVal(2) << _packet->payload->getVal(3) << _packet->payload->getVal(4) << endl;
  //  }
}

void DataSinkApplicationAgent:: startLogging(simulationTime_t _interval) {
  loggingInterval = _interval;
  Scheduler::getScheduler().schedule(new DataSinkLogEvent(Scheduler::now() + loggingInterval, this));
}

void DataSinkApplicationAgent::stopLogging() {
  loggingInterval = 0;
}

void DataSinkApplicationAgent::processDataSinkLogEvent() {
  //     double channelUsage = 100*bitsReceivedDuringInterval / ((double)loggingInterval / (double)ScenarioParameters::getPulseDuration());
  //     cout << "Sink log on node " << hostNode->getId() << " " << packetsReceivedDuringInterval << " packets / ";
  //     cout << bitsReceivedDuringInterval << " bits / ";
  //     cout << channelUsage << "% channel usage" << endl;
  if ( loggingInterval > 0 ) {
    Scheduler::getScheduler().schedule(new DataSinkLogEvent(Scheduler::now() + loggingInterval, this));
  }
  if (Scheduler::now() > 1000000000) loggingInterval = 0;
  packetsReceivedDuringInterval = 0;
  bitsReceivedDuringInterval = 0;
}
