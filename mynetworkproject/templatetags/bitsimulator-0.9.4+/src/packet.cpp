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

#include <set>
#include <cassert>
#include "utils.h"
#include "packet.h"
#include "scheduler.h"

mt19937 BinaryPayload::payloadRNG;

//===========================================================================================================
//
//          Packet  (class)
//
//===========================================================================================================

int Packet::nextId = 0;

Packet::Packet(PacketType _type, int _size, int _srcId, int _dstId, int _port, int _flowId, int _flowSequenceNumber) :
  Packet (_type, _size, _srcId, _dstId, _port, _flowId, _flowSequenceNumber, -1, -1)
{
}





Packet::Packet(PacketType _type, int _size, int _srcId, int _dstId, int _port, int _flowId, int _flowSequenceNumber,int _anchorID, int _anchorDist) :
  type(_type), size(_size), srcId(_srcId), dstId(_dstId), port(_port), flowId(_flowId), flowSequenceNumber(_flowSequenceNumber) {

  assert(size >= 1);
  packetId = nextId++;
  beta = (int)ScenarioParameters::getDefaultBeta();
  srcSequenceNumber = -1;
  transmitterId = -1;
  retransmission = 0;
  collisioned = false;
  parasite = false;
  payload = PayloadPtr(new BinaryPayload(_size));

  anchorID = _anchorID;
  anchorDist = _anchorDist;

  src_SLRX = -1;
  src_SLRY = -1;
  src_SLRZ = -1;

  dst_SLRX = -1;
  dst_SLRY = -1;
  dst_SLRZ = -1;

  tx_SLRX = -1;
  tx_SLRY = -1;
  tx_SLRZ = -1;

  creationTime = -1;

  deviation = 1;
  needAck = false;
  retransmission = 0;

  disableBackoff = false;
  
  communicationRange = -1;
}

PacketPtr Packet::clone() {
  PacketPtr clone = std::make_shared<Packet>(type, size, srcId, dstId, port, flowId, flowSequenceNumber);


  clone->beta = beta;
  clone->srcSequenceNumber = srcSequenceNumber;

  clone->transmitterId = transmitterId;
  clone->retransmission = retransmission;

  clone->payload = payload;

  clone->anchorID = anchorID;
  clone->anchorDist = anchorDist;

  clone->src_SLRX = src_SLRX;
  clone->src_SLRY = src_SLRY;
  clone->src_SLRZ = src_SLRZ;

  clone->dst_SLRX = dst_SLRX;
  clone->dst_SLRY = dst_SLRY;
  clone->dst_SLRZ = dst_SLRZ;

  clone->tx_SLRX = tx_SLRX;
  clone->tx_SLRY = tx_SLRY;
  clone->tx_SLRZ = tx_SLRZ;

  clone->collisioned = false;
  clone->parasite = false;

  clone->creationTime = creationTime;

  clone->deviation = deviation;

  clone->needAck = needAck;
  clone->retransmission = retransmission;

  clone->disableBackoff = disableBackoff;

  clone->hopCount = hopCount;
  
  clone->communicationRange = communicationRange;

  return clone;
}

bool Packet::checkAndTagCollision(simulationTime_t _p1StartTime, PacketPtr _p2, simulationTime_t _p2StartTime) {
  bool EugenVersion = false;
  unsigned int maxError = 0; // max number of corrupted bits for which the packet is still considered intact (using error correction code)

  if ( EugenVersion ) {
    int beta1 = beta;
    simulationTime_t i;
    simulationTime_t zeroPos = -1;
    simulationTime_t tp = ScenarioParameters::getPulseDuration ();

    //     cout << "Eugen p1StartTime: " << _p1StartTime << " txID: " << transmitterId << " p2StartTime: " << _p2StartTime << " txId: " << _p2->transmitterId << endl;

    if ( !(_p2->collisioned && collisioned) ) {
      //      cout << "Eugen dedans p1StartTime: " << _p1StartTime << " txID: " << transmitterId << " p2StartTime: " << _p2StartTime << " txId: " << _p2->transmitterId << endl;
      int beta2 = _p2->beta;
      simulationTime_t startBit = max (_p1StartTime, _p2StartTime);
      simulationTime_t stopBit = min (Scheduler::now(), _p2StartTime + _p2->size * tp * beta2);
      /* for testing purposes
         cout << endl << " startBit : " <<  startBit << " stopBit " << stopBit << endl;
         cout << " _p1StartTime : " <<  _p1StartTime << " Scheduler::now() : \t" << Scheduler::now() << endl;
         cout << " _p2StartTime : " <<  _p2StartTime << " _p2SEndTime  : \t" <<  _p2StartTime + _p2->size * tp * beta2 << endl<< endl;
      */
      long int nbBits = (stopBit - startBit)/(beta1*tp);  // +1 if last (virtual) bit does not exist
      //         cout << "nbBits: " << nbBits << " startBit: " << startBit << " stopBit: " << stopBit << endl;
      //        cout << "beta1: " << beta1 << " beta2: " << beta2 << endl;
      simulationTime_t offset = _p2StartTime - _p1StartTime;
      // make it positive
      offset %= beta2*tp;
      if (offset < 0)
        offset += beta2*tp;

      simulationTime_t diff0 = offset % (beta2*tp);

      //         cout << "Now:" << Scheduler::now() << endl;
      //         cout << "offset: " << offset << " diff0: " << diff0 << endl;

      for (i=0 ; i < nbBits ; i++){
        long int diff = ( offset + i*beta1*tp ) % (beta2*tp);
        if (i > 0 && diff == diff0) {  // found a loop
          break;
        }
        if ( diff < tp ) zeroPos = i;  // test with bit on left
        if ( diff > beta2*tp - tp && i+1 < nbBits ) zeroPos = i + 1;  // test with bit on right && bit inside the window
      }// end for

      /*
        if (zeroPos != -1 ) {
        cout << " collision on tested bit number " << zeroPos << " on time : " << startBit+( zeroPos*tp*beta1) << endl;
        }
        cout << "eugen now=" << Scheduler::now() << ", p1StartTime: " << _p1StartTime << ", p2StartTime: " << _p2StartTime << ", beta1: " << beta << ", beta2: " << beta2 << ", i: " << i << ", zeroPos: " << zeroPos << ", nbBits: " << nbBits << endl << endl;*/

      /***** Reversed test ****/
      //         nbBits = (stopBit - startBit)/(beta2*tp);  // +1 if last (virtual) bit does not exist
      //         offset = _p1StartTime - _p2StartTime;
      //         // make it positive
      //         offset %= beta1*tp;
      //         if (offset < 0)
      //           offset += beta1*tp;
      //
      //         diff0 = offset % (beta1*tp);
      //         for (i=0 ; i < nbBits ; i++){
      //             long int diff = ( offset + i*beta2*tp ) % (beta1*tp);
      //             if (i > 0 && diff == diff0) {  // found a loop
      // //                 cout << "thierry break " << i << endl;
      //                 break;
      //             }
      //                 if ( diff < tp ) zeroPos2 = i;  // test with bit on left
      //                 if ( diff > beta1*tp - tp && i+1 < nbBits ) zeroPos2 = i + 1;  // test with bit on right && bit inside the window
      //         }// end for
      //        if (zeroPos2 != -1 ) {
      //           cout << " collision on tested bit number " << zeroPos2 << " on time : " << startBit+( zeroPos2*tp*beta2) << endl;
      //        }
      //         cout << "thierry now=" << Scheduler::now() << ", p1StartTime: " << _p1StartTime << ", p2StartTime: " << _p2StartTime << ", beta1: " << beta << ", beta2: " << beta2 << ", i: " << i << ", zeroPos: " << zeroPos2 << ", nbBits: " << nbBits << endl;
      /***** Reversed test ****/

      if (zeroPos != -1) {
        collisioned = true;
        _p2->collisioned = true;
      }
    }// end if
  } else {
    // Dominique
    simulationTime_t tp = ScenarioParameters::getPulseDuration();

    simulationTime_t p1Start = _p1StartTime;
    simulationTime_t ts1 = beta * tp;
    int p1Size = size;
    simulationTime_t p1Duration = (p1Size-1) * ts1 + tp;
    simulationTime_t p1End = p1Start + p1Duration;
    simulationTime_t b1Start = p1Start;
    simulationTime_t b1End = b1Start + tp;

    simulationTime_t p2Start = _p2StartTime;
    simulationTime_t ts2 = _p2->beta * tp;
    int p2Size = _p2->size;
    simulationTime_t p2Duration = (p2Size-1) * ts2 + tp;
    simulationTime_t p2End = p2Start + p2Duration;
    simulationTime_t b2Start = p2Start;
    simulationTime_t b2End = p2Start + tp;

    int p1Count = 0;
    int p2Count = 0;

    bool progress;

    if ( !(p2End < p1Start || p1End < p2Start )) {  // packets are overlapping at least a little bit

      simulationTime_t diff;
      diff = p1Start - p2Start;
      if (diff < 0) {
        // p1 start first
        p1Count = (int)(diff / ts1);
        b1Start += p1Count * ts1;
        b1End += p1Count * ts1;
        //if (p1Count > 0) cout << "+";
      } else {
        // p2 start first
        p2Count = (int)abs(diff / ts2);
        b2Start += p2Count * ts2;
        b2End += p2Count * ts2;
        //if (p2Count > 0) cout << "*";
      }

      simulationTime_t offset = p1Start - p2Start;
      long pgcdTs = MathUtilities::gcd(ts1, ts2);
      //long ppcmTs = MathUtilities::lcm(ts1, ts2);
      simulationTime_t diff2 = ((offset % pgcdTs) + pgcdTs) % pgcdTs;

      if ( diff2 < tp || diff2 > (pgcdTs-tp)) {
        while ( p1Count < p1Size && p2Count < p2Size ) {
          progress = false;
          if (b1End <= b2Start) {
            p1Count++;
            b1Start += ts1;
            b1End += ts1;
            progress = true;
          }
          if (b2End <= b1Start) {
            p2Count++;
            b2Start += ts2;
            b2End += ts2;
            progress = true;
          }
          if ( !progress ) {
            //      cout << "collision bit:" << p1Count << " b1Start:" << b1Start << " b1End:" << b1End << "  bit:" << p2Count << " b2Start:" << b2Start << " b2End:" << b2End << endl;

            if (payload->getVal(p1Count) == false && _p2->payload->getVal(p2Count) == true)
              modifiedBitsPositions.insert(p1Count);
            if (payload->getVal(p1Count) == true && _p2->payload->getVal(p2Count) == false)
              _p2->modifiedBitsPositions.insert(p2Count);

            //break;
            p1Count++;
            p2Count++;
            b1Start += ts1;
            b1End += ts1;
            b2Start += ts2;
            b2End += ts2;
          }
        }
      }
    }
  }

  if (modifiedBitsPositions.size() > maxError)
    collisioned = true;
  // p2 check is mandatory, because p2 is not checked for collisions if there is no concurrent packet at its reception
  if (_p2->modifiedBitsPositions.size() > maxError)
    _p2->collisioned = true;
  return collisioned;
}
