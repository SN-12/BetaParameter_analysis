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

#ifndef WORLD_H_
#define WORLD_H_

#include <map>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <functional>
#include <mutex>
#include "utils.h"
#include "node.h"

enum class DrawingType {
  RECEIVE,
  SEND,
  COLLISION,
  IGNORE,
  OTHER
};

class PointInfo {
public:
  short int x,y;
  unsigned char r,g,b;
  simulationTime_t date;

  PointInfo(short int _x, short int _y, unsigned char _r, unsigned char _g, unsigned char _b, simulationTime_t _date) {
    x = _x;
    y = _y;
    r = _r;
    g = _g;
    b = _b;
    date = _date;
  }
};

//==============================================================================
//
//          World  (class)
//
//==============================================================================

class World {
private:
  static World *myWorld;
  distance_t sizeX, sizeY, sizeZ;

  vector<Node*> vectNodes;

  // grid that can be used instead of neighbours list in each Node
  vector<Node*> ***ptrNodes3D;
  int gridXSize, gridYSize, gridZSize;

  multimap<simulationTime_t, PointInfo> pointInfoMap;
  vector<PointInfo> vectPointInfo;
  vector<PointInfo> toDrawVectPointInfo;
  multimap<simulationTime_t, PointInfo> toDrawPointInfoMap;
  mutex mutexVisualization;
  bool endVisualization;

  static mt19937_64 *shadowingCommunicationRangeRandomGenerator;
  static normal_distribution<double> shadowingCommunicationRangeDistribution;
  World();

public:
  ~World();

  static World *getWorld() { return myWorld; }
  static void initWorld();
  void initNodes();
  void printWorldInfo();
  static void initAgents();
  void initSDL();
  static Node *getNode(int _id) { return myWorld->vectNodes[_id]; }
  static vector<Node*>::iterator getFirstNodeIterator() { return myWorld->vectNodes.begin(); }
  static vector<Node*>::iterator getEndNodeIterator() { return myWorld->vectNodes.end(); }
  static void drawNode(int _id, DrawingType _type);
  void visualizationLoop(World *_world);
  void sendPacketToNeighbours(Node *_srcNode, PacketPtr _p, simulationTime_t _delayBeforeTransmission);

  void foreachNodeNear(distance_t x, distance_t y, distance_t z,
    distance_t distance, std::function<void(Node*)> operation);

  void destroyWorld();
};

#endif /* WORLD_H_ */
