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
#include "scheduler.h"
#include "world.h"
#include "utils.h"
#include "backoff-deviation-routing-agent.h"


int PACKETSEND =0;


//==============================================================================
// BackoffDeviationRoutingAgent


using StartPropagationPhaseEvent = CallMethodEvent<BackoffDeviationRoutingAgent,
  &BackoffDeviationRoutingAgent::StartPropagationMessage,
  EventType::BACKOFF_DEVIATION_PROPAGATION_PHASE_EVENT>;

using BackoffDeviationSendEvent = CallMethodArgEvent<BackoffDeviationRoutingAgent, int,
  &BackoffDeviationRoutingAgent::sendPacketDelayed,
  EventType::BACKOFF_DEVIATION_DELAYED_SEND_EVENT>;


mt19937_64 BackoffDeviationRoutingAgent::forwardDelayRnd(9004);
ofstream BackoffDeviationRoutingAgent::slrPositionsFile;
int BackoffDeviationRoutingAgent::redundancy(1);
int BackoffDeviationRoutingAgent::deviateThresh(0.5);
int BackoffDeviationRoutingAgent::convergeThresh(0.5);


void BackoffDeviationRoutingAgent::initializeAgent() {
  if (ScenarioParameters::getRoutingAgentName() != "BackoffDeviation") {
    return;
  }

  // open the file to write stuff into.
  string extension = ScenarioParameters::getDefaultExtension();
	string separator = "";
	if ( ScenarioParameters::getOutputBaseName().length() > 0 ) {
		separator = "-";
	}
	string filename = ScenarioParameters::getScenarioDirectory() + "/" + ScenarioParameters::getOutputBaseName() + separator + "SLRPositions" + extension;

  slrPositionsFile.open(filename);

  if (!slrPositionsFile) {
    cout << "*** ERROR *** Could not open slrPositionsFile for "
      "BackoffDeviationRouting. Error code: " << strerror(errno) << "\n";
  }

  // load configuration parameters.
  auto xml = ScenarioParameters::getXMLRootNode();
  auto rac = xml->FirstChildElement("routingAgentsConfig");
  auto routing = rac->FirstChildElement("BackoffDeviation");
  ScenarioParameters::queryIntAttr(routing, "redundancy",
    BackoffDeviationRoutingAgent::redundancy, false, nullptr, 1, "");
  ScenarioParameters::queryIntAttr(routing, "deviateThresholdPercent",
    BackoffDeviationRoutingAgent::deviateThresh, false, nullptr, 50, "");
  ScenarioParameters::queryIntAttr(routing, "convergeThresholdPercent",
    BackoffDeviationRoutingAgent::convergeThresh, false, nullptr, 50, "");
}


BackoffDeviationRoutingAgent::BackoffDeviationRoutingAgent(Node* host)
  : RoutingAgent(host), iAmAnchor(false), beaconPhaseStarted(false) {
  slrx = std::numeric_limits<int>::max();
  slry = std::numeric_limits<int>::max();
  slrz = std::numeric_limits<int>::max();

  // are we an anchor? If so, schedule propagation phase.
  for (const auto& nodeInfo : ScenarioParameters::getVectNodeInfo()) {
    if (nodeInfo.isAnchor && nodeInfo.id == host->getId()) {
      iAmAnchor = true;
    }
  }

  if (iAmAnchor && host->getId() == 0) {
    auto startTime = Scheduler::now() + 4e11;
    Scheduler::getScheduler().schedule(
      new StartPropagationPhaseEvent(startTime, this));
    beaconPhaseStarted = true;
  }
}

BackoffDeviationRoutingAgent::~BackoffDeviationRoutingAgent() {
  if (slrPositionsFile) {
    slrPositionsFile << hostNode->getId() << " " << slrx << " " << slry <<
      " " << slrz << endl;
  }


  cout << " PACKETSEND " << PACKETSEND << endl;
}


void BackoffDeviationRoutingAgent::StartPropagationMessage() {
  cout << Scheduler::now() << " Start BackoffDeviationRoutingAgent propagation "
    "phase on node " << hostNode->getId() << "\n";

  PacketPtr propPacket(new Packet(PacketType::SLR_BEACON,
      100, // size
      hostNode->getId(), -1, // source, destination
      -1, // port
      9000 + hostNode->getId(), // flowId
      0, // sequenceNr
      hostNode->getId(), // anchor Id; HCR propagation: Sending HC
      1)); // anchor distance; HCR propagation: Current HC
  slrUpdate(propPacket);
  receivePacketFromApplication(propPacket);
}


void BackoffDeviationRoutingAgent::receivePacketFromApplication(PacketPtr packet) {
  // set the per-node sequence number.
  packet->setSrcSequenceNumber(hostNode->getNextSrcSequenceNumber());

  packet->src_SLRX = slrx;
  packet->src_SLRY = slry;
  packet->src_SLRZ = slrz;
  packet->tx_SLRX = slrx;
  packet->tx_SLRY = slry;
  packet->tx_SLRZ = slrz;
  packet->deviation = 1;

  if (packet->type == PacketType::DATA) {
    // dst -1 -> Broadcast; if not broadcast, get target slr coordinate
    // note: we are cheating a little here, but it is probably okay.
    if (packet->dstId != -1) {
      auto target = World::getNode(packet->dstId);
      auto targetAgent = dynamic_cast<BackoffDeviationRoutingAgent*>(
        target->getRoutingAgent());

      if (targetAgent == nullptr) {
        cerr << "*** ERROR *** BackoffDeviationRoutingAgent routing to "
          "node without agent.\n";
        exit(1);
      }

      packet->dst_SLRX = targetAgent->slrx;
      packet->dst_SLRY = targetAgent->slry;
      packet->dst_SLRZ = targetAgent->slrz;
    }
  }

  hostNode->enqueueOutgoingPacket(packet);
}





void BackoffDeviationRoutingAgent::receivePacketFromNetwork(PacketPtr packet) {
  LogSystem::EventsLogOutput.log(LogSystem::routingRCV,
    Scheduler::now(), hostNode->getId(),
    (int)packet->type,
    packet->flowId,
    packet->flowSequenceNumber,
    packet->modifiedBitsPositions.size());

  // if we already track that packet, do not consider it further.
  for (auto& upcoming : delayedPackets) {
    if (upcoming.second.first->flowId == packet->flowId &&
      upcoming.second.first->flowSequenceNumber == packet->flowSequenceNumber) {
      if (upcoming.second.second >= redundancy) {
        delayedPackets.erase(upcoming.first);
      }
      else {
        // increase the counter.
        upcoming.second.second += 1;
      }
      return;
    }
  }

  // actual hcr propagation / slr beacon phase.
  if (packet->type == PacketType::SLR_BEACON) {
    if (slrUpdate(packet)) {
      resendPacket(packet, true);
    }

    if (iAmAnchor && !beaconPhaseStarted) {
      // if there is a propagation phase going on, let's participate.
      auto startTime = Scheduler::now() + 3e8 * hostNode->getId();
      Scheduler::getScheduler().schedule(
        new StartPropagationPhaseEvent(startTime, this));
      beaconPhaseStarted = true;
    }
  }
  else if (packet->type == PacketType::DATA) {
    // if we arrive at the destination node by id (investigate: unrealistic!)
    if (packet->dstId == hostNode->getId()) {
      // cout << "*** BackoffDeviationRoutingAgent: Reached target for flow " <<
      //   packet->flowId << ".\n";
      LogSystem::EventsLogOutput.log(LogSystem::dstReach,
        Scheduler::now(),
        hostNode->getId(),
        (int)packet->type,
        packet->flowId,
        packet->flowSequenceNumber);

      hostNode->dispatchPacketToApplication(packet);
      if ( packet->flowId == 42 ) {
        cout << " DESTINATION REACHED on node " << hostNode->getId() << "(" << slrx << "," << slry<< "," << slrz<< ")" << " packet (flow) " << packet->flowSequenceNumber << "(" << packet->flowId << ")"  << endl;
      }
    }

    // if broadcast
    if (packet->dstId == -1) {
      // no broadcast for data-packets supported here.
    }


    // check the congestion to deviate if required.
    int mcr = ScenarioParameters::getMaxConcurrentReceptions();
    int receptCount = hostNode->getConcurrentReceptionCount();
    float congestionLevel = (float)receptCount / mcr;
    if (100 * congestionLevel > deviateThresh) {
      packet->deviation++;
      // cout << "deviating with " << congestionLevel << " to thresh " << deviateThresh << ". new level: " << packet->deviation << "\n";
    }
    else if (100 * congestionLevel < convergeThresh && packet->deviation > 1) {
      packet->deviation--;
    }

    // check slr forwarding.
    //bool slrForward = shouldSlrForward(packet, packet->deviation);
    bool slrForward = shouldSlrForward(packet, packet->deviation) && !shouldSlrForward(packet, (packet->deviation)-1) ;

    auto v = alreadyForwardedPackets;
    std::pair<int,int> flowIdent(packet->flowId, packet->flowSequenceNumber);
    bool alreadyForwarded = std::find(v.begin(), v.end(), flowIdent) != v.end();
    if (!alreadyForwarded) {
      alreadyForwardedPackets.push_back(flowIdent);
    }

    bool forward = slrForward && !alreadyForwarded;

    if (forward) {
      resendPacket(packet, false);
    }
  }
}


bool BackoffDeviationRoutingAgent::alreadyForwarding(PacketPtr packet) {
  for (const auto& d : delayedPackets) {
    if (d.second.first->flowId == packet->flowId &&
      d.second.first->flowSequenceNumber == packet->flowSequenceNumber) {
      return true;
    }
  }

  return false;
}


void BackoffDeviationRoutingAgent::resendPacket(PacketPtr packet, bool inBeaconPhase) {
  PacketPtr fwdPacket = packet->clone();
  fwdPacket->tx_SLRX = slrx;
  fwdPacket->tx_SLRY = slry;
  fwdPacket->tx_SLRZ = slrz;

  if (inBeaconPhase) {
    fwdPacket->anchorDist += 1;
  }

  // the maximum delay: 2 times sending each message pulse, plus the maximum
  // time the propagation takes.

  unsigned int backoffWindow = hostNode->getNeighboursCount() * 2 * (
    (Node::getPulseDuration() * (1 + packet->beta * (packet->size - 1))) +
    ScenarioParameters::getCommunicationRange() / PROPAGATIONSPEED);

  uniform_int_distribution<unsigned int> backoffDist(0, backoffWindow);
  simulationTime_t backoffDelay = backoffDist(forwardDelayRnd);

  delayedPackets[fwdPacket->packetId] = { fwdPacket, 1 };
  Scheduler::getScheduler().schedule(new BackoffDeviationSendEvent(
      Scheduler::now() + backoffDelay, this, fwdPacket->packetId));
}


void BackoffDeviationRoutingAgent::sendPacketDelayed(int packetId) {
  auto fwdPacketR = delayedPackets.find(packetId);
  if (fwdPacketR != delayedPackets.end()) {
    auto fwdPacket = fwdPacketR->second.first;
    hostNode->enqueueOutgoingPacket(fwdPacket);
    delayedPackets.erase(packetId);
  }
}


bool BackoffDeviationRoutingAgent::slrUpdate(PacketPtr packet) {
  if (packet->anchorID == 0 && slrx > packet->anchorDist) {
    slrx = packet->anchorDist;
    return true;
  }
  else if (packet->anchorID == 1 && slry > packet->anchorDist) {
    slry = packet->anchorDist;
    return true;
  }
  else if (packet->anchorID == 2 && slrz > packet->anchorDist) {
    slrz = packet->anchorDist;
    return true;
  }

  return false;
}


bool BackoffDeviationRoutingAgent::areWeCloserThanPacketSender(PacketPtr packet) {
  int dstX,dstY,dstZ;
  int txX,txY,txZ;
  int X,Y,Z;

  bool res = false;

  dstX = packet->dst_SLRX;
  dstY = packet->dst_SLRY;
  dstZ = packet->dst_SLRZ;
  txX = packet->tx_SLRX;
  txY = packet->tx_SLRY;
  txZ = packet->tx_SLRZ;

  X = slrx;
  Y = slry;
  Z = slrz;

  // one. if the packet sender is closer in one single dimension,
  // we are not closer.
  if ((std::abs(dstX - txX) < std::abs(dstX - X)) ||
    (std::abs(dstY - txY) < std::abs(dstY - Y)) ||
    (std::abs(dstZ - txZ) < std::abs(dstZ - Z))) {
    res = false;
  }
  // two. if the packet sender is in the same zone, we are not closer.
  else if (txX == X && txY == Y && txZ == Z) {
    res = false;
  }

  // three. anybody left must be equal or closer on all dimensions, and
  // actually closer in at least one of them.
  else {
    res = true;
  }

  // cout << " isCloser on node " << hostNode->getId() << " res = " <<
  //     res << ", case: " << c << endl;
  // printf(" dstX %d dstY %d dstZ %d \n", dstX,dstY,dstZ);
  // printf(" srcX %d srcY %d srcZ %d \n", srcX,srcY,srcZ);
  // printf("  txX %d  txY %d  txZ %d \n", txX,txY,txZ);
  // printf("  X   %d  Y   %d  Z   %d \n", X,Y,Z);

  return res;
}


bool BackoffDeviationRoutingAgent::shouldSlrForward(PacketPtr packet, int m) {
  // copied from SLRBackoffRoutingAgent3::SLRForward
  int dstX,dstY,dstZ;
  int srcX,srcY,srcZ;
  // int txX,txY,txZ;
  int X,Y,Z;

  dstX = packet->dst_SLRX;
  dstY = packet->dst_SLRY;
  dstZ = packet->dst_SLRZ;

  srcX = packet->src_SLRX;
  srcY = packet->src_SLRY;
  srcZ = packet->src_SLRZ;

  X = slrx;
  Y = slry;
  Z = slrz;

  int deltaA = DeltaA(dstX, dstY, dstZ, srcX, srcY, srcZ, X, Y, Z);
  int deltaB = DeltaB(dstX, dstY, dstZ, srcX, srcY, srcZ, X, Y, Z);

  int deltaAX = DeltaA(dstX, dstY, dstZ, srcX, srcY, srcZ, X - m, Y, Z);
  int deltaAY = DeltaA(dstX, dstY, dstZ, srcX, srcY, srcZ, X, Y - m, Z);
  int deltaAZ = DeltaA(dstX, dstY, dstZ, srcX, srcY, srcZ, X - m, Y - m, Z);

  int deltaBX = DeltaB(dstX, dstY, dstZ, srcX, srcY, srcZ, X - m, Y, Z);
  int deltaBY = DeltaB(dstX, dstY, dstZ, srcX, srcY, srcZ, X, Y, Z - m);
  int deltaBZ = DeltaB(dstX, dstY, dstZ, srcX, srcY, srcZ, X - m, Y, Z - m);

  //Node is on the line between src and dst
  bool onTheLine = (
    (deltaA * deltaAX <= 0 ||
      deltaA * deltaAY <= 0 ||
      deltaA * deltaAZ <= 0) &&
    (deltaB * deltaBX <= 0 ||
      deltaB * deltaBY <= 0 ||
      deltaB * deltaBZ <= 0));

  bool onSegment = (
    ((X - dstX) * (X - srcX) <= 0) &&
    ((Y - dstY) * (Y - srcY) <= 0) &&
    ((Z - dstZ) * (Z - srcZ) <= 0));

  return onTheLine && onSegment;
}
