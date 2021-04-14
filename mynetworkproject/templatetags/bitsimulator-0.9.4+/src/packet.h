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

#ifndef PACKET_H_
#define PACKET_H_

#include <unordered_set>

// why Class and not just enum?
enum class PacketType {
  DATA,                   //0
  SLR_BEACON,             //1
  DENSITY_PROBE,          //2
  D1_DENSITY_INIT,        //3
  D1_DENSITY_PROBE,       //4
  D2_DENSITY_INIT,        //5
  D2_DENSITY_PROBE,       //6
  DEVIATION_ACK,         //7
  HCD_BEACON,		 //8
  CONTROL_1,	          //9
  CONTROL_2              //10	            
};

class DataSinkApplicationAgent;

//===========================================================================================================
//
//          BinaryPayload  (class)
//
//===========================================================================================================

class BinaryPayload {
protected:
  uint32_t *data;
  uint32_t marsagliaState;;
  static mt19937 payloadRNG;

public:
  BinaryPayload(int _size) {
    int intCount = _size / 32;
    int reminder = _size % 32;
    if (reminder > 0) intCount++;

    marsagliaState = payloadRNG();

    data = new uint32_t[intCount];
    for (int i=0; i<intCount; i++) {
      marsagliaState ^= marsagliaState << 13;
      marsagliaState ^= marsagliaState >> 17;
      marsagliaState ^= marsagliaState << 5;
      data[i] = marsagliaState;
    }
  }
  ~BinaryPayload() {
    delete(data);
  }

  static void initialize(int _seed) {
    payloadRNG = mt19937(_seed);
  }

  bool getVal(int _i) {
    int index = _i / 32;
    int shift = _i % 32;
    return(data[index] & (2147483648 >> shift));
  }
  void set(int _i) {
    int index = _i / 32;
    int shift = _i % 32;
    data[index] |= (uint32_t)(2147483648 >> shift);
  }
  void unset(int _i) {
    int index = _i / 32;
    int shift = _i % 32;
    data[index] &=  (2147483647 << (32 - shift)) | (2147483647 >> shift);
  }
};

typedef shared_ptr<BinaryPayload> PayloadPtr;

//===========================================================================================================
//
//          Packet  (class)
//
//===========================================================================================================

class Packet;
typedef shared_ptr<Packet> PacketPtr;

class Packet {
protected:
  static int nextId;

public:
  int packetId;
  PacketType type;
  int size;
  int beta;
  int srcId;
  int srcSequenceNumber;
  int dstId;
  int port;

  int transmitterId;
  int flowId;
  int flowSequenceNumber;
  int retransmission;

  bool collisioned;
  bool parasite;

  int deviation;

  simulationTime_t creationTime;

  PayloadPtr payload;
  unordered_set<int> modifiedBitsPositions;

  int anchorID;
  int anchorDist;
  int hopCount = 0;

  int src_SLRX;
  int src_SLRY;
  int src_SLRZ;

  int dst_SLRX;
  int dst_SLRY;
  int dst_SLRZ;

  int tx_SLRX;
  int tx_SLRY;
  int tx_SLRZ;

  bool needAck;

  bool disableBackoff;

  distance_t communicationRange;

  Packet(PacketType _type, int _size, int _srcId, int _dstId, int _port, int _flowId, int _flowSequenceNumber);
  Packet(PacketType _type, int _size, int _srcId, int _dstId, int _port, int _flowId, int _flowSequenceNumber, int _achorID, int _achorDist);


  virtual ~Packet() { }
  virtual PacketPtr clone();

  void setBeta(int _beta) { beta = _beta; }
  void setSrcSequenceNumber(int _srcSequenceNumber) { srcSequenceNumber = _srcSequenceNumber; }
  void setTransmitterId(int _transmitterId) { transmitterId = _transmitterId; }

  bool checkAndTagCollision(simulationTime_t _p1StartTime, PacketPtr _p2, simulationTime_t _p2StartTime);

  void setCommunicationRange( distance_t _range ) {
    if ( _range > ScenarioParameters::getCommunicationRange() ) {
        cout << "*** ERROR, trying to set communicationRange of this Packet beyond the communicationRange globally defined" << endl;
        exit(EXIT_FAILURE);
    }

    if ( _range != ScenarioParameters::getCommunicationRange() && !ScenarioParameters::getDoNotUseNeighboursList() ) {
        cout << "*** ERROR, You cannot change the communication range at runtime if you are using the neighbours list" << endl;
        exit(EXIT_FAILURE);
    }

    communicationRange = _range;
  }
};

#endif /* PACKET_H_ */
