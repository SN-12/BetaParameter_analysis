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

#ifndef UTILS_H_
#define UTILS_H_

#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include <map>
#include <random>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <set>
#include <map>
#include <exception>

#include <tclap/CmdLine.h>
#include <tinyxml2.h>

#include "output.h"

using namespace std;

typedef long int distance_t;
typedef long int simulationTime_t;

class MathUtilities {
public:
  static long gcd(long _m, long _n) {
    long m = _m;
    long n = _n;
    long tmp;
    while(m) {
      tmp = m; m = n % m; n = tmp;
    }
    return n;
  }

  static long lcm(long m, long n) {
    return m / gcd(m, n) * n;
  }
};

enum class NodesAreaShape {
    UNKNOWN,
    INVALID,
    RECTANGLE,
    RECTANGLE_HOLE,
    ELLIPSE
};

enum class NodesAreaDistribution {
    UNKNOWN,
    NONE,
    UNIFORM,
    NORMAL
};

class NodesArea;

//===========================================================================================================
//
//          NodeInfo  (class)
//
//===========================================================================================================

class NodeInfo {
public:
  int id;
  distance_t posX, posY, posZ;
  bool isAnchor;
  simulationTime_t beaconStartTime;

  NodeInfo(int _id, distance_t _x, distance_t _y, distance_t _z, bool _anchor, simulationTime_t _start) {
    id = _id;
    posX = _x;
    posY = _y;
    posZ = _z;
    isAnchor = _anchor;
    beaconStartTime = _start;
  }
};

//===========================================================================================================
//
//          ScenarioParameters  (class)
//
//===========================================================================================================

class ScenarioParameters {
private:
  static ScenarioParameters *scenarioParameters;

  tinyxml2::XMLDocument doc;
  tinyxml2::XMLElement *XMLRootNode;

  // General informations
  string scenarioName;
  TCLAP::ValueArg<std::string> *scenarioNameParam;
  string scenarioDescription;
  TCLAP::ValueArg<std::string> *scenarioDescriptionParam;

  // XML scenario file
  string scenarioDirectory;
  string configurationFileName;
  string configurationFileCompleteName;

  // World
  distance_t worldXSize;
  TCLAP::ValueArg<long> *worldXSizeParam;
  distance_t worldYSize;
  TCLAP::ValueArg<long> *worldYSizeParam;
  distance_t worldZSize;
  TCLAP::ValueArg<long> *worldZSizeParam;

  vector<NodeInfo> vectNodeInfo;
  int genericNodesNumber;
  TCLAP::ValueArg<int> *genericNodesNumberParam;
  int genericNodesRNGSeed;
  TCLAP::ValueArg<int> *genericNodesRNGSeedParam;

  int binaryPayloadRNGSeed;
  TCLAP::ValueArg<int> *binaryPayloadRNGSeedParam;

  bool doNotUseNeighboursList;

  // Agents
  string routingAgentName;
  TCLAP::ValueArg<string> *routingAgentNameParam;

  // nodes
  simulationTime_t nodeStartupTime;
  TCLAP::ValueArg<long> *nodeStartupTimeParam;
  int backoffRNGSeed;
  TCLAP::ValueArg<int> *backoffRNGSeedParam;
  int defaultBackoffWindowWidth;
  TCLAP::ValueArg<int> *defaultBackoffWindowWidthParam;
  //  simulationTime_t defaultBeta;
  //         TCLAP::ValueArg<long> *defaultBetaParam;
  int defaultBeta;
  TCLAP::ValueArg<int> *defaultBetaParam;

  int nodePositionNoise;
  TCLAP::ValueArg<long> *nodePositionNoiseParam;

  simulationTime_t pulseDuration;
  distance_t communicationRange;
  distance_t communicationRangeSmall;
  distance_t communicationRangeStandardDeviation;
  unsigned int maxConcurrentReceptions;
  simulationTime_t minimumIntervalBetweenSends;
  simulationTime_t minimumIntervalBetweenReceiveAndSend;
  bool acceptCollisionedPackets;

  // log system
  map<string,logSystemInfo_t> mapOutputFiles;
  string outputBaseName;
  TCLAP::ValueArg<string> *outputBaseNameParam;

  // graphic mode
  bool graphicMode;

  // D1DensityEstimator agent
  float D1GrowRate;
  TCLAP::ValueArg<float> *D1GrowRateParam;
  float D1ErrorMax;
  TCLAP::ValueArg<float> *D1ErrorMaxParam;
  int D1MaxTic;
  TCLAP::ValueArg<int> *D1MaxTicParam;
  int D1MaxRound;
  TCLAP::ValueArg<int> *D1MaxRoundParam;
  int D1RemainingFreeRounds;
  TCLAP::ValueArg<int>*D1RemainingFreeRoundsParam;
  int D1MinPacketsForTermination;
  TCLAP::ValueArg<int> *D1MinPacketsForTerminationParam;
  bool D1DoNotSimulateInitFlood;
  TCLAP::SwitchArg *D1DoNotSimulateInitFloodParam;

  //slr
  int slrPathWidth;
  TCLAP::ValueArg<int> *slrPathWidthParam;

  //hcd
  int hcdPathWidth;
  TCLAP::ValueArg<int> *hcdPathWidthParam;

  //slrBackoff
  int slrBackoffRedundancy;
  TCLAP::ValueArg<int> *slrBackoffRedundancyParam;
  int slrBeaconBackoffRedundancy;
  TCLAP::ValueArg<int> *slrBeaconBackoffRedundancyParam;
  float slrBackoffMultiplier;
  TCLAP::ValueArg<float> *slrBackoffMultiplierParam;
  float slrBackoffBeaconMultiplier;
  TCLAP::ValueArg<float> *slrBackoffBeaconMultiplierParam;
  int backoffFloodingRNGSeed;
  TCLAP::ValueArg<int> *backoffFloodingRNGSeedParam;

  //advanced LogSystem
  bool logAtNodeLevel; // Activate node level log
  TCLAP::SwitchArg *logAtNodeLevelParam;
  bool logAtRoutingLevel; // Activate routing level log
  TCLAP::SwitchArg *logAtRoutingLevelParam;

  //simulation mode
  bool allowMultipleSend;
  TCLAP::SwitchArg *allowMultipleSendParam;

  //sleeping system
  bool sleepIsEnabled;
  int awakenDuration;
  TCLAP::ValueArg<int> *awakenDurationParam;
  int awakenNodes;
  TCLAP::ValueArg<int> *awakenNodesParam;
  int sleepRNGSeed;
  TCLAP::ValueArg<int> *sleepRNGSeedParam;

  //activate DEDeN
  bool dedenIsEnabled;
  TCLAP::SwitchArg *dedenParam;
  int dedenRNGSeed;
  TCLAP::ValueArg<int> *dedenRNGSeedParam;


  // ##COMMANDLINE NEW OPTIONS


  simulationTime_t stepDuration;
  simulationTime_t initialTimeSkip;
  int nodeZoom;
  int chronoNodeId;
  string complementaryNodeFloatInfoFileName;
  vector <int>othersNodesToDisplayVector;
  bool precomputeSteps;
  
  NodesArea *rootNodesArea;

  ScenarioParameters(int argc, char **argv, int i);

public:
  static void initialize(int argc, char **argv, int i);

  static bool queryStringElement(tinyxml2::XMLElement *_parentElement, string _elementName, string &_result, bool _required, TCLAP::ValueArg<std::string> *_TCLAPParam, string _errorMessage);
  static bool queryStringAttr(tinyxml2::XMLElement *_parentElement, string _attributeName, string &_result, bool _required, TCLAP::ValueArg<string> *_TCLAPParam, string _defaultValue, string _message);
  static bool queryIntAttr(tinyxml2::XMLElement *_element, string _attributeName, int &_result, bool _required, TCLAP::ValueArg<int> *_TCLAPParam, int _defaultValue, string _message);
  static bool queryLongAttr(tinyxml2::XMLElement *_element, string _attributeName_nm, string _attributeName_mm, long &_result, bool _required, TCLAP::ValueArg<long> *_TCLAPParam, long _defaultValue, string _errorMessage);

  void parseRoutingElement(tinyxml2::XMLElement *_parent);
  void parseWorldElement(tinyxml2::XMLElement *_parent);
  void parseLogSystemElement(tinyxml2::XMLElement *_parent);
  void displayParameters();


  string queryStringAttribute(tinyxml2::XMLElement *_element, string _attributeName);
  int queryIntAttribute(tinyxml2::XMLElement *_element, string _elementName);
  string queryStringElement(tinyxml2::XMLElement *_parentElement, string _elementName);
  static long queryLongAttribute(tinyxml2::XMLElement *_element, string _attributeName_nm, string _attributeName_mm);

  static tinyxml2::XMLElement *getXMLRootNode(){ return(scenarioParameters->XMLRootNode); }

  static string getScenarioDirectory() { return(scenarioParameters->scenarioDirectory); }
  static string getConfigurationFileName() { return(scenarioParameters->configurationFileName); }
  static distance_t getWorldXSize() { return(scenarioParameters->worldXSize); }
  static distance_t getWorldYSize() { return(scenarioParameters->worldYSize); }
  static distance_t getWorldZSize() { return(scenarioParameters->worldZSize); }
  static vector<NodeInfo> getVectNodeInfo() { return(scenarioParameters->vectNodeInfo); }
  static map<string,logSystemInfo_t> getMapOutPutFiles() { return(scenarioParameters->mapOutputFiles); }
  static string getOutputBaseName() { return(scenarioParameters->outputBaseName); }
  static int getGenericNodesNumber() { return(scenarioParameters->genericNodesNumber); }
  static int getGenericNodesRNGSeed() { return(scenarioParameters->genericNodesRNGSeed); }
  static distance_t getNodeStartupTime() { return(scenarioParameters->nodeStartupTime); }
  static int getBackoffRNGSeed() { return(scenarioParameters->backoffRNGSeed); }
  static int getdefaultBackoffWindowWidth() { return(scenarioParameters->defaultBackoffWindowWidth); }
  static simulationTime_t getDefaultBeta() { return(scenarioParameters->defaultBeta); }
  static simulationTime_t getPulseDuration() { return(scenarioParameters->pulseDuration); }
  static distance_t getCommunicationRange() { return(scenarioParameters->communicationRange); }
  static distance_t getCommunicationRangeSmall() { return(scenarioParameters->communicationRangeSmall); }
  static distance_t getCommunicationRangeStandardDeviation() { return(scenarioParameters->communicationRangeStandardDeviation); }
  static unsigned int getMaxConcurrentReceptions() { return(scenarioParameters->maxConcurrentReceptions); }
  static simulationTime_t getMinimumIntervalBetweenSends() { return(scenarioParameters->minimumIntervalBetweenSends); }
  static simulationTime_t getMinimumIntervalBetweenReceiveAndSend() { return(scenarioParameters->minimumIntervalBetweenReceiveAndSend); }
  static bool getGraphicMode() { return(scenarioParameters->graphicMode); }
  static string getRoutingAgentName() { return(scenarioParameters->routingAgentName); }
  static bool getDoNotUseNeighboursList() { return(scenarioParameters->doNotUseNeighboursList); }
  static bool getAcceptCollisionedPackets() { return(scenarioParameters->acceptCollisionedPackets); }
  static int getBinaryPayloadRNGSeed() { return(scenarioParameters->binaryPayloadRNGSeed); }

  static distance_t getNodePositionNoise() {return(scenarioParameters->nodePositionNoise);}

  static float getD1GrowRate() { return(scenarioParameters->D1GrowRate); }
  static float getD1ErrorMax() { return(scenarioParameters->D1ErrorMax); }
  static int getD1MaxTic() { return(scenarioParameters->D1MaxTic); }
  static int getD1MaxRound() { return(scenarioParameters->D1MaxRound); }
  static int getD1RemainingFreeRounds() { return(scenarioParameters->D1RemainingFreeRounds); }
  static int getD1MinPacketsForTermination() { return(scenarioParameters->D1MinPacketsForTermination); }
  static bool getD1DoNotSimulateInitFlood() { return(scenarioParameters->D1DoNotSimulateInitFlood); }

  //slr
  static int getSlrPathWidth() { return(scenarioParameters->slrPathWidth); }

  //hcd
  static int getHcdPathWidth() { return(scenarioParameters->hcdPathWidth); }

  //slrBackoff
  static int getSlrBackoffredundancy() { return(scenarioParameters->slrBackoffRedundancy); }
  static int getSlrBeaconBackoffredundancy() { return(scenarioParameters->slrBeaconBackoffRedundancy); }

  static float getSlrBackoffMultiplier(){ return scenarioParameters->slrBackoffMultiplier;}
  static float getSlrBackoffBeaconMultiplier(){ return scenarioParameters->slrBackoffBeaconMultiplier;}

  static int getBackoffFloodingRNGSeed() { return(scenarioParameters->backoffFloodingRNGSeed); }

  // advanced LogSystem
  static bool getLogAtNodeLevel() { return scenarioParameters->logAtNodeLevel; }
  static bool getLogAtRoutingLevel() { return scenarioParameters->logAtRoutingLevel; }

  //sleeping system
  static bool getSleep() { return scenarioParameters->sleepIsEnabled; }
  static int getAwakenDuration () { return scenarioParameters->awakenDuration; }
  static int getAwakenNodes () {return scenarioParameters->awakenNodes; }
  static int getSleepRNGSeed () { return scenarioParameters->sleepRNGSeed; }

  //Activate DEDEN
  static bool getDeden() { return scenarioParameters->dedenIsEnabled; }
  static int getDedenRNGSeed() {return scenarioParameters->dedenRNGSeed;}

  static long getStepDuration() { return(scenarioParameters->stepDuration); }
  static long getInitialTimeSkip() { return(scenarioParameters->initialTimeSkip); }
  static int getChronoNodeId() { return(scenarioParameters->chronoNodeId); }
  static int getNodeZoom() { return(scenarioParameters->nodeZoom); }
  static string getComplementaryNodeFloatInfoFileName() { return(scenarioParameters->complementaryNodeFloatInfoFileName); }
  static vector<int> getOthersNodesToDisplay() { return(scenarioParameters->othersNodesToDisplayVector); }
  static bool getPrecomputeSteps() { return scenarioParameters->precomputeSteps; }

  static string getDefaultExtension() {return ".log";}

  static NodesArea *getRootNodesArea() { return scenarioParameters->rootNodesArea; }
};


//==============================================================================
//
//          Function for all kind of SLR
//
//==============================================================================

int DeltaA(int dstX, int dstY, int dstZ,int srcX,int srcY,int srcZ,int X,int Y, int Z );


int DeltaB(int dstX, int dstY, int dstZ,int srcX,int srcY,int srcZ,int X,int Y, int Z );

//==============================================================================
//
//          NodesArea  (class)
//
//==============================================================================

class NodePosition {
public:
    distance_t x,y,z;
    NodePosition(distance_t _x, distance_t _y, distance_t _z): x(_x), y(_y), z(_z) {}
};

class NodesArea {
public:
        NodesAreaShape shape;
        NodesAreaDistribution distribution;
        int nodesCount;
        int positionRNGSeed;

        NodesArea();

        void setShapeRectangle( distance_t _x, distance_t _y, distance_t _z, distance_t _sizeX, distance_t _sizeY, distance_t _sizeZ );
        void addAreaFromXMLElement( tinyxml2::XMLElement *_xmlElement );

        void print( string _shift);
        vector<NodePosition> getNodesPositionsVector();

private:
        distance_t x,y,z;                   // global coordinates (relative to the whole world)
        distance_t localX, localY, localZ;  // local coordinates (relative to the parent NodesArea)

        distance_t sizeX, sizeY, sizeZ;

        mt19937_64 *positionsRandomGenerator;

        NodesArea *parent;
        vector<NodesArea*> children;

        vector<NodePosition> nodes;
};

#endif /* UTILS_H_ */
