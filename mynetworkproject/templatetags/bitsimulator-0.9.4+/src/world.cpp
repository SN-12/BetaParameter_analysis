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
#include "utils.h"
#include "scheduler.h"
#include "world.h"

#include "agents/deden-agent.h"
#include "agents/incident-observer-agent.h"
#include "agents/pure-flooding-routing-agent.h"
#include "agents/pure-flooding-ring-routing-agent.h"
#include "agents/manual-routing-agent.h"
#include "agents/gateway-server-agent.h"
#include "agents/backoff-deviation-routing-agent.h"
#include "agents/hcd-routing-agent.h"
#include "agents/cbr-application-agent.h"
#include "agents/backoff-flooding-routing-agent.h"
#include "agents/backoff-flooding-ring-routing-agent.h"
#include "agents/proba-flooding-routing-agent.h"
#include "agents/proba-flooding-ring-routing-agent.h"
#include "agents/slr-routing-agent.h"
#include "agents/slr-backoff-routing-agent.h"
#include "agents/slr-backoff-routing-agent3.h"
#include "agents/slr-ring-routing-agent.h"
#include "agents/slr-deviation-routing-agent.h"

namespace DisplayProperties {
  int windowWidth = 800;
  int windowHeight = 800;

  simulationTime_t currentTime = 0;
  simulationTime_t stepDuration = 100;

  int nodeZoom = 3;
  float zoom = 1000;
  float offsetX = 0;
  float offsetY = 0;

  SDL_Renderer *renderer;

  bool showAllNodes = false;
};

//==============================================================================
//
//          World  (class)
//
//==============================================================================

World *World::myWorld = nullptr;
mt19937_64 *World::shadowingCommunicationRangeRandomGenerator = nullptr;
normal_distribution<double> World::shadowingCommunicationRangeDistribution;

World::World() {
  cout << "Creating World ..." << endl;

  sizeX = ScenarioParameters::getWorldXSize();
  sizeY = ScenarioParameters::getWorldYSize();
  sizeZ = ScenarioParameters::getWorldZSize();
  cout << "  World size [ " << sizeX << ", " << sizeY << ", " << sizeZ << " ]" << endl;

  ptrNodes3D = nullptr;

  shadowingCommunicationRangeRandomGenerator = new mt19937_64( 42 );
  World::shadowingCommunicationRangeDistribution = normal_distribution<double>(0.0,ScenarioParameters::getCommunicationRangeStandardDeviation());

  initNodes();
  printWorldInfo();
  initSDL();
}

World::~World() {
  if (ptrNodes3D != nullptr) {
    for(int i=0; i<gridXSize; i++) {
      for(int j=0; j<gridYSize; j++) {
        delete[] ptrNodes3D[i][j];
      }
      delete[] ptrNodes3D[i];
    }
    delete ptrNodes3D;
  }
}

void World::initWorld() {
  myWorld = new World();
}

void World::initNodes() {
  Node* newNode;
  bool sleep = ScenarioParameters::getSleep();
  Node::initBackoffRandomGenerator(ScenarioParameters::getBackoffRNGSeed());
  Node::setPulseDuration(ScenarioParameters::getPulseDuration());

  //
  // instantiating the manual and anchor nodes
  //
  vector<NodeInfo> vectorNodeInfo = ScenarioParameters::getVectNodeInfo();
  mt19937 rng(ScenarioParameters::getGenericNodesRNGSeed());
  distance_t maxNoise = ScenarioParameters::getNodePositionNoise();
  uniform_int_distribution<distance_t> noiseDistribution(-maxNoise,maxNoise);

  for (auto it=vectorNodeInfo.begin(); it!=vectorNodeInfo.end(); it++) {
    distance_t posX = it->posX;
    distance_t posY = it->posY;
    distance_t posZ = it->posZ;

    cout << "    " << (it->isAnchor ? "anchor " : "manual ") << it->id << endl;

    // check that nodes are inside the world
    if (posX < 0 || posX > ScenarioParameters::getWorldXSize()
        || posY < 0 || posY > ScenarioParameters::getWorldYSize()
        || posZ < 0 || posZ > ScenarioParameters::getWorldZSize()) {
      cerr << "*** ERROR *** Node " << it->id << " is placed outside the world (" << posX << "," << posY << "," << posZ << ")" << endl;
      exit(EXIT_FAILURE);
    }

    if (ScenarioParameters::getNodePositionNoise() != 0 && !it->isAnchor) {
      posX += noiseDistribution (rng);
      posY += noiseDistribution (rng);
      posZ += noiseDistribution (rng);

      if (posX > ScenarioParameters::getWorldXSize()) posX = ScenarioParameters::getWorldXSize();
      if (posX < 0) posX = 0;
      if (posY > ScenarioParameters::getWorldYSize()) posY = ScenarioParameters::getWorldYSize();
      if (posY < 0) posY = 0;
      if (posZ > ScenarioParameters::getWorldZSize()) posZ = ScenarioParameters::getWorldZSize();
      if (posZ < 0) posZ = 0;
    }

    if (sleep)
      newNode = new SleepingNode(it->id, posX, posY, posZ);
    else
      newNode = new Node(it->id, posX, posY, posZ);
    vectNodes.push_back(newNode);
  }

  //
  // instantiating the random generic nodes
  //

  mt19937_64 positionsRandomGenerator(ScenarioParameters::getGenericNodesRNGSeed());
  uniform_int_distribution<distance_t> posXUniformDistribution(0, ScenarioParameters::getWorldXSize());
  uniform_int_distribution<distance_t> posYUniformDistribution(0, ScenarioParameters::getWorldYSize());
  uniform_int_distribution<distance_t> posZUniformDistribution(0, ScenarioParameters::getWorldZSize());

  int generatedNodesCount = 0;
  vector<NodePosition> nodesPositionsVector = ScenarioParameters::getRootNodesArea()->getNodesPositionsVector();

  for (auto it=nodesPositionsVector.begin(); it<nodesPositionsVector.end(); it++) {
    if (sleep)
      newNode = new SleepingNode(Node::getNextId(), it->x, it->y, it->z);
    else
      newNode = new Node(Node::getNextId(), it->x, it->y, it->z);
    vectNodes.push_back(newNode);
    generatedNodesCount++;
  }

  cout << "  random nodes generated: " << generatedNodesCount << endl;

  for (auto it=vectNodes.begin(); it!= vectNodes.end(); it++)
    Scheduler::getScheduler().schedule(new NodeStartupEvent(ScenarioParameters::getNodeStartupTime(), *it));

  std::ofstream positionsFile;
  string extension = ScenarioParameters::getDefaultExtension();
  string filename;
  string separator = "";
  if (ScenarioParameters::getOutputBaseName().length() > 0)
    separator = "-";
  filename = ScenarioParameters::getScenarioDirectory() + "/" + ScenarioParameters::getOutputBaseName() + separator + "positions" + extension;
  positionsFile.open(filename);
  if (!positionsFile) {
    cerr << "*** ERROR *** While opening position file " << filename << endl;
    exit(EXIT_FAILURE);
  }
  for (auto it=vectNodes.begin(); it!= vectNodes.end(); it++)
    positionsFile << (*it)->getId() << " 0 0 0 " << (*it)->getXPos() << " " << (*it)->getYPos() << " " << (*it)->getZPos() << endl;

  //
  // affect initial value to communication range and standard deviation to each node
  //
  for (auto currentNodeIt = vectNodes.begin(); currentNodeIt != vectNodes.end(); currentNodeIt++) {
    (*currentNodeIt)->setCommunicationRange( ScenarioParameters::getCommunicationRange() );
    (*currentNodeIt)->setCommunicationRangeStandardDeviation( ScenarioParameters::getCommunicationRangeStandardDeviation() );
  }

  //
  // neighborhood construction
  //
  int counter=0;
  distance_t distance;

  if ( ScenarioParameters::getDoNotUseNeighboursList() ) {
    gridXSize = (int)ceil(ScenarioParameters::getWorldXSize() / ScenarioParameters::getCommunicationRange() ) + 1;
    if (gridXSize == 0) gridXSize = 1;
    gridYSize = (int)ceil(ScenarioParameters::getWorldYSize() / ScenarioParameters::getCommunicationRange() ) + 1;
    if (gridYSize) gridYSize = 1;
    gridZSize = (int)ceil(ScenarioParameters::getWorldZSize() / ScenarioParameters::getCommunicationRange() ) + 1;
    if (gridZSize == 0) gridZSize = 1;

    ptrNodes3D = new vector<Node*>**[gridXSize];

    for(int i=0; i<gridXSize; i++) {
      ptrNodes3D[i] = new vector<Node*>*[gridYSize];
      for(int j=0; j<gridYSize; j++)
        ptrNodes3D[i][j] = new vector<Node*>[gridZSize];
    }

    cout << "  neighbours grid size: "<< gridXSize << " " << gridYSize << " " << gridZSize << endl;
    int xn, yn, zn;
    for (auto currentNodeIt = vectNodes.begin(); currentNodeIt != vectNodes.end(); currentNodeIt++) {
      xn = (int)floor( (*currentNodeIt)->getXPos() / (ScenarioParameters::getCommunicationRange()) );
      yn = (int)floor( (*currentNodeIt)->getYPos() / (ScenarioParameters::getCommunicationRange()) );
      zn = (int)floor( (*currentNodeIt)->getZPos() / (ScenarioParameters::getCommunicationRange()) );
      ptrNodes3D[xn][yn][zn].push_back(*currentNodeIt);
    }

    for (auto _node = vectNodes.begin(); _node != vectNodes.end(); _node++) {
      int count = 0;

      xn = (int)floor((*_node)->getXPos() / (ScenarioParameters::getCommunicationRange()) );
      yn = (int)floor((*_node)->getYPos() / (ScenarioParameters::getCommunicationRange()) );
      zn = (int)floor((*_node)->getZPos() / (ScenarioParameters::getCommunicationRange()) );

      for (int d=xn-1; d<=xn+1; d++) {
        for (int e=yn-1; e<=yn+1; e++) {
          for (int f=zn-1; f<=zn+1; f++) {
            if (d >= 0 && d < gridXSize && e >= 0 && e < gridYSize && f >= 0 && f < gridZSize) {
              for (auto it = ptrNodes3D[d][e][f].begin(); it != ptrNodes3D[d][e][f].end(); it++) {
                if ((*_node)->getId() != (*it)->getId() && (*_node)->distance(*it) <= (*_node)->getCommunicationRange())
                  count++;
              }
            }
          }
        }
      }
      (*_node)->setNeighboursCount (count);
    }
  } else { // use neighbours list ... use (a lot) of memory in high density scenarios, but fast
    //ofstream neighboursFile;
    FILE *neighboursFile;
    string filename;
    string separator = "";
    if (ScenarioParameters::getOutputBaseName().length() > 0)
      separator = "-";
    filename = ScenarioParameters::getScenarioDirectory() + "/" + ScenarioParameters::getOutputBaseName() + separator + "neighboursPositions" + extension;
    neighboursFile = fopen(filename.c_str(), "w");
    if ( !neighboursFile ) {
      cout << "*** ERROR *** Opening neighborhood file failed: " << filename << endl;
      exit(-1);
    }

    //
    // slower but simpler neighbours search
    //

    bool slowButSure = false;
    if (slowButSure) {
      for (auto currentNodeIt = vectNodes.begin(); currentNodeIt != vectNodes.end(); currentNodeIt++) {
        counter++;
        //neighboursFile << (*currentNodeIt)->getId() << " ";
        fprintf(neighboursFile,"%d ", (*currentNodeIt)->getId());
        for (auto it = vectNodes.begin(); it != vectNodes.end(); it++) {
          distance = (*currentNodeIt)->distance(*it);
          if (currentNodeIt != it && distance <= ScenarioParameters::getCommunicationRange() ) {
            (*currentNodeIt)->addNeighbour(distance,*it);
            //neighboursFile << distance << " " << (*it)->getId() << " ";
            fprintf(neighboursFile,"%d ",  (*it)->getId() );
          }
        }
        //neighboursFile << endl;
        fprintf(neighboursFile,"\n");
      }
      cout << "  " << counter << " nodes processed" << endl;
    } else {
      int xs = (int)ceil(ScenarioParameters::getWorldXSize() / ScenarioParameters::getCommunicationRange() ) + 1;
      if (xs == 0) xs = 1;
      int ys = (int)ceil(ScenarioParameters::getWorldYSize() / ScenarioParameters::getCommunicationRange() ) + 1;
      if (ys == 0) ys = 1;
      int zs = (int)ceil(ScenarioParameters::getWorldZSize() / ScenarioParameters::getCommunicationRange() ) + 1;
      if (zs == 0) zs = 1;

      int xn, yn, zn;
      vector<Node*> ***ptr3D=NULL;
      ptr3D=new vector<Node*>**[xs];

      for(int i=0; i<xs; i++) {
        ptr3D[i] = new vector<Node*>*[ys];
        for(int j=0; j<ys; j++)
          ptr3D[i][j] = new vector<Node*>[zs];
      }

      cout << "  fast neighbours search grid size: "<< xs << " " << ys << " " << zs << endl;
      for (auto currentNodeIt = vectNodes.begin(); currentNodeIt != vectNodes.end(); currentNodeIt++) {
        xn = (int)floor( (*currentNodeIt)->getXPos() / (ScenarioParameters::getCommunicationRange()) );
        yn = (int)floor( (*currentNodeIt)->getYPos() / (ScenarioParameters::getCommunicationRange()) );
        zn = (int)floor( (*currentNodeIt)->getZPos() / (ScenarioParameters::getCommunicationRange()) );

        if (xn > xs || yn > ys || zn > zs) {
          cout << "*** ERROR ** invalid coordinates: (" <<
            (*currentNodeIt)->getXPos() << "," <<
            (*currentNodeIt)->getYPos() << "," <<
            (*currentNodeIt)->getZPos() << ") in World size: (" <<
            ScenarioParameters::getWorldXSize() << "," <<
            ScenarioParameters::getWorldYSize() << "," <<
            ScenarioParameters::getWorldZSize() << ")\n";
          exit(EXIT_FAILURE);
        }

        ptr3D[xn][yn][zn].push_back((*currentNodeIt));
      }

      for (auto currentNodeIt = vectNodes.begin(); currentNodeIt != vectNodes.end(); currentNodeIt++) {
        counter++;
        //neighboursFile << (*currentNodeIt)->getId() << " ";
        fprintf(neighboursFile,"%d ", (*currentNodeIt)->getId());

        xn = (int)floor((*currentNodeIt)->getXPos() / (ScenarioParameters::getCommunicationRange()) );
        yn = (int)floor((*currentNodeIt)->getYPos() / (ScenarioParameters::getCommunicationRange()) );
        zn = (int)floor((*currentNodeIt)->getZPos() / (ScenarioParameters::getCommunicationRange()) );

        for (int d=xn-1; d<=xn+1; d++) {
          for (int e=yn-1; e<=yn+1; e++) {
            for (int f=zn-1; f<=zn+1; f++) {
              if (d >= 0 && d < xs && e >= 0 && e < ys && f >= 0 && f < zs) {
                for (auto it = ptr3D[d][e][f].begin(); it != ptr3D[d][e][f].end(); it++) {
                  distance = (*currentNodeIt)->distance(*it);
                  if ((*currentNodeIt)->getId() != (*it)->getId() && distance <= ScenarioParameters::getCommunicationRange() ) {
                    (*currentNodeIt)->addNeighbour(distance,*it);
                    //neighboursFile << distance << " " << (*it)->getId() << " ";
                    fprintf(neighboursFile, "%ld %d ", distance, (*it)->getId());
                  }
                }
              }
            }
          }
        }
        //neighboursFile << endl;
        fprintf(neighboursFile,"\n");
      }

      cout << "  " << counter << " nodes processed" << endl;

      for(int i=0; i<xs; i++) {
        for(int j=0; j<ys; j++)
          delete[] ptr3D[i][j];
        delete[] ptr3D[i];
      }
      delete ptr3D;
    }
  }
  // vectNodes[3]->drawLocalView(2000000000, 2200000000);
  // vectNodes[2]->drawLocalView(2000000000, 2200000000);
}

void World::printWorldInfo () {
  int count = 0;
  int maxi = 0;
  int mini = INT_MAX;
  int nodeCount = 0;
  int countTotal = 0;

  // print network density
  for (auto node = vectNodes.begin(); node != vectNodes.end(); node++) {
    nodeCount++;
    count = (*node)->getNeighboursCount();
    if (maxi < count) maxi = count;
    if (mini > count) mini = count;
    countTotal += count;
  }
  if (nodeCount == 0) maxi = 0;  // particular case of 0 nodes
  cout << "  min/avg/max number of neighbours: " << mini << "/" << countTotal / nodeCount << "/" << maxi << endl;

  // print size of world in terms of hops
  printf ("  nb of hops on x/y/z: %.1f/%.1f/%.1f\n",
          ScenarioParameters::getWorldXSize() / (float)ScenarioParameters::getCommunicationRange(),
          ScenarioParameters::getWorldYSize() / (float)ScenarioParameters::getCommunicationRange(),
          ScenarioParameters::getWorldZSize() / (float)ScenarioParameters::getCommunicationRange());
}

void World::initAgents() {
  // TODO : Initialise only the agent used

  ManualRoutingAgent::initializeAgent();
  SLRRoutingAgent::initializeAgent();
  SLRRingRoutingAgent::initializeAgent();
  SLRBackoffRoutingAgent::initializeAgent();
  SLRBackoffRoutingAgent3::initializeAgent();
  CBRApplicationAgent::initializeAgent();
  //D1DensityEstimatorAgent::initialize();
  D11DensityEstimatorAgent::initialize();
  D2DensityEstimatorAgent::initializeAgent();
  BackoffFloodingRoutingAgent::initializeAgent();
  DeviationRoutingAgent::initializeAgent();
  BackoffDeviationRoutingAgent::initializeAgent();
  BackoffFloodingRingRoutingAgent::initializeAgent();
  IncidentObserverAgent::initializeAgent();
  GatewayServerAgent::initializeAgent();
}

void World::initSDL() {
  endVisualization = false;

  if (ScenarioParameters::getGraphicMode()) {
    float ratio = (float)ScenarioParameters::getWorldXSize() / (float)ScenarioParameters::getWorldZSize();
    DisplayProperties::windowHeight = (int)((float)DisplayProperties::windowWidth*ratio);
    DisplayProperties::zoom = (float)ScenarioParameters::getWorldXSize() / (float)DisplayProperties::windowWidth;

    if ( DisplayProperties::windowWidth < 100 ) DisplayProperties::windowWidth = 200;
    if ( DisplayProperties::windowHeight < 100 ) DisplayProperties::windowHeight = 200;

    if (SDL_Init(SDL_INIT_VIDEO) != 0){
      std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
      exit(EXIT_FAILURE);
    }

    SDL_Window *win = SDL_CreateWindow("BitSimulator " VERSION, 500, 100, DisplayProperties::windowWidth, DisplayProperties::windowHeight, SDL_WINDOW_SHOWN);
    if (win == nullptr){
      std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
      SDL_Quit();
      exit(EXIT_FAILURE);
    }

    DisplayProperties::renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (DisplayProperties::renderer == nullptr){
      SDL_DestroyWindow(win);
      std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
      SDL_Quit();
      exit(EXIT_FAILURE);
    }

    SDL_Texture *tex = SDL_CreateTexture(DisplayProperties::renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
      DisplayProperties::windowWidth, DisplayProperties::windowHeight);
    if (tex == nullptr){
      SDL_DestroyRenderer(DisplayProperties::renderer);
      SDL_DestroyWindow(win);
      std::cout << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
      SDL_Quit();
      exit(EXIT_FAILURE);
    }
  }
}

void World::visualizationLoop(World *world) {
  SDL_SetRenderDrawColor( DisplayProperties::renderer, 0, 0, 0, 255 );
  SDL_RenderClear( DisplayProperties::renderer );

  SDL_Event evt;
  bool programrunning = true;
  simulationTime_t now;
  simulationTime_t before = 0;

  while (programrunning) {
    SDL_WaitEventTimeout(&evt,500);
    if ((evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_q && evt.key.keysym.mod && KMOD_LCTRL != 0)
        || evt.type == SDL_QUIT
        || endVisualization) {
      programrunning = false;
    }

    now = Scheduler::now();
    if (now != before) {
      multimap<simulationTime_t, PointInfo>::iterator first;
      SDL_SetRenderDrawColor( DisplayProperties::renderer, 0, 0, 0, 255 );
      SDL_RenderClear( DisplayProperties::renderer );

      //         while ( (first=world->toDrawPointInfoMap.begin()) != world->toDrawPointInfoMap.end() && first->first < (now - 10000)) {
      //          world->toDrawPointInfoMap.erase(first);
      //         }
      world->toDrawPointInfoMap.clear();

      world->mutexVisualization.lock();

      for (auto it=world->vectPointInfo.begin(); it!=world->vectPointInfo.end(); it++) {
        //if (it->date >= (now-10000)) {
        world->toDrawPointInfoMap.insert(pair<simulationTime_t, PointInfo>(it->date, *it));
        //}
      }
      world->vectPointInfo.clear();
      world->mutexVisualization.unlock();

      //         while ( (first=world->toDrawPointInfoMap.begin()) != world->toDrawPointInfoMap.end() && first->first < (now - 10000)) {
      //          world->toDrawPointInfoMap.erase(first);
      //         }

      for (auto it=world->toDrawPointInfoMap.begin(); it!=world->toDrawPointInfoMap.end(); it++ ) {
        // //SDL_SetRenderDrawColor(DisplayProperties::renderer, r, g, b, 255);
        // //SDL_RenderDrawPoint(DisplayProperties::renderer,x,y);
        filledCircleRGBA(DisplayProperties::renderer, it->second.x, it->second.y, (short int)(1*DisplayProperties::nodeZoom), it->second.r, it->second.g, it->second.b, 255);
      }

      SDL_RenderPresent(DisplayProperties::renderer);
    }
    before = now;
  }
  Scheduler::endSimulation();
}


void World::drawNode(int _id,  DrawingType _type) {
  //if ( Scheduler::now() < 100000000000000 ) return;
  unsigned char r,g,b;
  switch (_type) {
  case DrawingType::SEND:
    r=0;
    g=0;
    b=255;
    break;
  case DrawingType::RECEIVE:
    r=255;
    g=0;
    b=0;
    break;
  case DrawingType::COLLISION:
    r=0;
    g=255;
    b=0;
    break;
  case DrawingType::IGNORE:
    r=255;
    g=255;
    b=0;
    break;
  default:  // OTHER
    r=255;
    g=255;
    b=255;
    break;
  }
  short int x = (short int)((double)myWorld->vectNodes[_id]->getXPos() / DisplayProperties::zoom);
  short int y = (short int)((double)myWorld->vectNodes[_id]->getZPos() / DisplayProperties::zoom);

  myWorld->mutexVisualization.lock();
  myWorld->vectPointInfo.push_back(PointInfo(x, y, r, g, b,Scheduler::now()));
  myWorld->mutexVisualization.unlock();
}

void World::sendPacketToNeighbours(Node *_srcNode, PacketPtr _p, simulationTime_t _delayBeforeTransmission) {
  distance_t distance;
  simulationTime_t receptionTime;

  distance_t communicationRange, effectiveCommunicationRange;

  if (_p->communicationRange == -1)
    communicationRange = _srcNode->getCommunicationRange();
  else {
    assert (_p->communicationRange <= _srcNode->getCommunicationRange());
    communicationRange = _p->communicationRange;
  }

  distance_t standardDeviation = ScenarioParameters::getCommunicationRangeStandardDeviation();

  int xn, yn, zn;
  xn = (int)floor(_srcNode->getXPos() / (ScenarioParameters::getCommunicationRange()) );
  yn = (int)floor(_srcNode->getYPos() / (ScenarioParameters::getCommunicationRange()) );
  zn = (int)floor(_srcNode->getZPos() / (ScenarioParameters::getCommunicationRange()) );

  for (int d=xn-1; d<=xn+1; d++) {
    for (int e=yn-1; e<=yn+1; e++) {
      for (int f=zn-1; f<=zn+1; f++) {
        if (d >= 0 && d < gridXSize && e >= 0 && e < gridYSize && f >= 0 && f < gridZSize) {
          for (auto it = ptrNodes3D[d][e][f].begin(); it != ptrNodes3D[d][e][f].end(); it++) {
            distance = _srcNode->distance(*it);

            if (standardDeviation == 0)
              effectiveCommunicationRange = communicationRange;
            else {
              distance_t shadowingRange = shadowingCommunicationRangeDistribution(*shadowingCommunicationRangeRandomGenerator);
              if ( shadowingRange > standardDeviation*3 ) shadowingRange=standardDeviation*3;
              if ( shadowingRange < -standardDeviation*3 ) shadowingRange=-standardDeviation*3;
              effectiveCommunicationRange = communicationRange - standardDeviation*3 + shadowingRange;
              if (effectiveCommunicationRange < 0) effectiveCommunicationRange = 0;
            }

            if ((_p->type != PacketType::SLR_BEACON &&_srcNode->getId() != (*it)->getId() && distance <= effectiveCommunicationRange)
                || (_p->type == PacketType::SLR_BEACON && distance <= ScenarioParameters::getCommunicationRangeSmall())) {
              receptionTime = Scheduler::now() + _delayBeforeTransmission + distance/PROPAGATIONSPEED;
              if ((*it)->isAwake (receptionTime)) {
                //PacketPtr packetClone = PacketPtr(new Packet(_p));
                PacketPtr packetClone = PacketPtr(_p->clone());
                Scheduler::getScheduler().schedule(new StartReceivePacketEvent(receptionTime, *it, packetClone));
              }
            }
          }
        }
      }
    }
  }
}

void World::foreachNodeNear(distance_t fx, distance_t fy, distance_t fz,
  distance_t distance, std::function<void(Node*)> operation) {

  // okay, we can't use the lookup
  if (ptrNodes3D == nullptr) {
    // well, test each node separately
    for (Node* node : vectNodes) {
      if (node->distance(fx, fy, fz) <= distance) {
        operation(node);
      }
    }
  } else{
    // each grid cell is ScenarioParam
    double cellSize = ScenarioParameters::getCommunicationRange();

    int xs = (int)ceil(ScenarioParameters::getWorldXSize() / ScenarioParameters::getCommunicationRange() ) + 1;
    int ys = (int)ceil(ScenarioParameters::getWorldYSize() / ScenarioParameters::getCommunicationRange() ) + 1;
    int zs = (int)ceil(ScenarioParameters::getWorldZSize() / ScenarioParameters::getCommunicationRange() ) + 1;

    // get the number of cells to check.
    int length = std::ceil(distance / cellSize);

    int xStart = std::max(std::floor(fx / cellSize) - length, 0.0);
    int xEnd = std::min(xStart + 2 * length + 1, xs);
    int yStart = std::max(std::floor(fy / cellSize) - length, 0.0);
    int yEnd = std::min(yStart + 2 * length + 1, ys);
    int zStart = std::max(std::floor(fz / cellSize) - length, 0.0);
    int zEnd = std::min(zStart + 2 * length + 1, zs);

    for (int x = xStart; x <= xEnd; x++)
      for (int y = yStart; y <= yEnd; y++)
        for (int z = zStart; z <= zEnd; z++)
          for (Node* node : ptrNodes3D[x][y][z])
            if (node->distance(fx, fy, fz) <= distance)
              operation(node);
  }
}

void World::destroyWorld() {
  cout << "Destroying World ..." << endl;
  for (auto it=vectNodes.begin(); it!=vectNodes.end(); it++)
    delete *it;
  if (ScenarioParameters::getGraphicMode() )
    endVisualization = true;
}
