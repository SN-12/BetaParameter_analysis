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

#include <config.h>
#include <iostream>
#include <vector>
#include <list>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <assert.h>
#include <limits>
#include "utils.h"
#include "renderer.h"
#include <array>

using namespace std;

class GraphicNode {
public:
  unsigned int id;
  long x,y,z;
  int corX, corY, corZ;

  bool wasActive;
  bool hasReceived;
  bool hasTransmitted;
  bool hadCollision;
  bool hadIgnored;

  int nbReceptions;
  int nbTransmissions;
  int nbCollisions;
  int nbIgnored;

  float complementaryFloatValue;
  int complementaryRed, complementaryGreen, complementaryBlue;

  GraphicNode(int _id, long _x, long _y, long _z, int _corX, int _corY, int _corZ) {
    id = _id;
    x = _x;
    y = _y;
    z = _z;
    corX = _corX;
    corY = _corY;
    corZ = _corZ;

    wasActive = false;
    hasReceived = false;
    hasTransmitted = false;
    hadCollision = false;
    hadIgnored = false;

    nbReceptions = 0;
    nbTransmissions = 0;
    nbCollisions = 0;
    nbIgnored = 0;

    complementaryFloatValue = 0;
    complementaryRed = 0;
    complementaryGreen = 0;
    complementaryBlue = 0;
  }
};

namespace DisplayProperties {
  int windowWidth = 1220;
  int windowHeight = 800;
  float moveSpeed = 10;

  simulationTime_t currentTime = 0;
  simulationTime_t initialTimeSkip = 0;
  simulationTime_t stepDuration = 100;
  simulationTime_t normalStepDuration = 100;

  int nodeZoom = 2;
  double zoom = 1000;
  float offsetX = 0;
  float offsetY = 0;
  long offsetXCenterMap = 0;
  long offsetYCenterMap = 0;
  long offsetXCenter = 450;
  long offsetYCenter = 450;

  SDL_Renderer *renderer;

  bool showAllNodes = false;

  bool drawReceptions = true;
  bool drawTransmissions = true;
  bool drawCollisions = true;
  bool drawIgnored = true;
  bool drawAllSLRZones = true;
  bool drawComplementaryFloat = false;

  bool useSLRZones = false;
  bool hasComplementaryFloat = false;

  TTF_Font* font;

  constexpr int histoElemNumber = 100;
  int histoWidth = 600;
  int histoHeight = 120;

  bool chronoShowCollisions = true;
  bool chronoDisplayAllNodes = false;
};

typedef struct {
  int eventId;
  //char type;
  string type;
  simulationTime_t startDate;
  simulationTime_t endDate;
  int beta;
  int size;
  int transmitterId;
} EventInfo_t;

namespace Data {
  FILE *positionsFile;
  FILE *SLRPositionsFile;
  FILE *eventsFile;
  FILE *complementaryNodeFloatInfoFile;

  LogOutput output;

  vector<GraphicNode> vectGraphicNode;

  vector<int> histoTotal;
  vector<int> histoTransmissions;
  vector<int> histoReceptions;
  vector<int> histoCollisions;
  vector<int> histoIgnored;

  int nbIgnored, nbCollisions, nbReceptions, nbTransmissions, nbEventsTotal;

  vector<EventInfo_t> vectEventInfo;
  set<int> othersNodesToDisplaySet;
  vector<int> othersNodesToDisplayVector;
  map<int,int> verticalPositionByNodeIdMap;

}


// data structures for holding precomputed visualisation data.
using histData = array<int, DisplayProperties::histoElemNumber>;
using NodeVect = vector<GraphicNode>;

struct StepData {
  simulationTime_t startTime;
  simulationTime_t stepDuration;
  int nbEvents;

  vector<GraphicNode> vectGraphicNode;

  histData histoTotal;
  histData histoTransmissions;
  histData histoReceptions;
  histData histoCollisions;
  histData histoIgnored;

  int nbIgnored;
  int nbCollisions;
  int nbReceptions;
  int nbTransmissions;
  int nbEventsTotal;
};


void initialiseVisualizer(NodeVect &vectGraphicNode) {
  DisplayProperties::stepDuration = ScenarioParameters::getStepDuration();
  DisplayProperties::normalStepDuration = ScenarioParameters::getStepDuration();
  DisplayProperties::initialTimeSkip = ScenarioParameters::getInitialTimeSkip();
  DisplayProperties::nodeZoom = ScenarioParameters::getNodeZoom();

	string separator = "";
	if ( ScenarioParameters::getOutputBaseName().length() > 0 ) {
		separator = "-";
	}
	string positionsFileName = ScenarioParameters::getScenarioDirectory() + "/" + ScenarioParameters::getOutputBaseName() + separator + "positions" + ScenarioParameters::getDefaultExtension();
//    string eventsFileName = ScenarioParameters::getScenarioDirectory() + "/" + ScenarioParameters::getOutputBaseName() + ScenarioParameters::getMapOutPutFiles().find("EventsLog")->second.extension;
	string eventsFileName = ScenarioParameters::getScenarioDirectory() + "/" + ScenarioParameters::getOutputBaseName() + separator + "events" + ScenarioParameters::getDefaultExtension();
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  int nbFields = 0;

  unsigned int id;
  distance_t posX, posY, posZ;
  int corX, corY, corZ;

  double ratio = ScenarioParameters::getWorldZSize() / ScenarioParameters::getWorldXSize();
  DisplayProperties::windowHeight = (DisplayProperties::windowWidth - DisplayProperties::histoWidth)*ratio + DisplayProperties::histoHeight;
  if (DisplayProperties::windowHeight > 1300) {
    DisplayProperties::windowHeight = 1300;
  }
  if (DisplayProperties::windowWidth > 2000) {
    DisplayProperties::windowWidth = 2000;
  }

  cout << "worldXSize: " << ScenarioParameters::getWorldXSize() << " worldZSize: " << ScenarioParameters::getWorldZSize() << endl;
  cout << "ratio: " << (ScenarioParameters::getWorldZSize() /
    ScenarioParameters::getWorldXSize()) << endl;
  cout << "windowWidth: " <<  DisplayProperties::windowWidth << " windowHeight: " << DisplayProperties::windowHeight << endl;
  cout << endl;

  cout << "Opening position file: " << positionsFileName << endl;
  cout << "... ";
  Data::positionsFile = fopen(positionsFileName.c_str(),"r");
  if (!Data::positionsFile) {
    cout << "Error while opening position file " << positionsFileName << endl;
    exit(EXIT_FAILURE);
  }
  cout << "ok" << endl;

  cout << "Loading positions" << endl;
  cout << "... ";
  while ((read = getline(&line, &len, Data::positionsFile)) != -1) {
    nbFields = sscanf(line,"%d%d%d%d%ld%ld%ld", &id, &corX, &corY, &corZ, &posX, &posY, &posZ);
    if (nbFields == 7) {
      vectGraphicNode.push_back(GraphicNode(id, posX, posY, posZ, corX, corY, corZ));
    } else {
      cout << "*** WARNING *** parse error on line: " << line << endl;
    }
  }
  cout << vectGraphicNode.size() << " nodes loaded"<< endl;

  cout << "Looking for SLR positions file" << endl;
  cout << "... ";

  tinyxml2::XMLElement *XMLRootNode = ScenarioParameters::getXMLRootNode();

  tinyxml2::XMLElement *routingAgentConfigElement = XMLRootNode->FirstChildElement("routingAgentsConfig");
  if (routingAgentConfigElement == nullptr) {
    cout << "*** INFO *** SLRRoutingAgent initialization: \"routingAgentsConfig\" section no found in " << ScenarioParameters::getConfigurationFileName() << "...  skipping" << endl;
  } else {
    tinyxml2::XMLElement *SLRRoutingElement = routingAgentConfigElement->FirstChildElement("SLRRouting");
    if (SLRRoutingElement == nullptr) {
      cout << "*** INFO *** SLRRoutingAgent initialization: \"SLRRouting\" section not found in " << ScenarioParameters::getConfigurationFileName() << "...  skipping" << endl;
    }
  }

	string SLRPositionsFileName = ScenarioParameters::getScenarioDirectory() + "/" + ScenarioParameters::getOutputBaseName() + separator + "SLRPositions" + ScenarioParameters::getDefaultExtension();
  Data::SLRPositionsFile = fopen(SLRPositionsFileName.c_str(),"r");
  if (!Data::SLRPositionsFile) {
    cout << SLRPositionsFileName << " not found" << endl;
  } else {
    cout << SLRPositionsFileName << " found" << endl;
    DisplayProperties::useSLRZones = true;
    int slrNodesCount = 0;
    while ((read = getline(&line, &len, Data::SLRPositionsFile)) != -1) {
      nbFields = sscanf(line,"%d%d%d%d", &id, &corX, &corY, &corZ);
      if (nbFields == 4) {
        if (id < vectGraphicNode.size() && vectGraphicNode[id].id == id) {
          vectGraphicNode[id].corX = corX;
          vectGraphicNode[id].corY = corY;
          vectGraphicNode[id].corZ = corZ;
          slrNodesCount++;
        } else {
          cout << "*** WARNING *** node identifier inconsistent between positions and SLR positions files" << endl;
        }
      } else {
        cout << "*** WARNING *** parse error on line: " << line << endl;
      }
    }
    cout << slrNodesCount << " SLR positions loaded for " << vectGraphicNode.size() << " nodes" << endl;
  }

  string complementaryNodeFloatInfoFileName = ScenarioParameters::getScenarioDirectory() + "/" + ScenarioParameters::getComplementaryNodeFloatInfoFileName();
  float value;
  if (!ScenarioParameters::getComplementaryNodeFloatInfoFileName().empty()) {
    Data::complementaryNodeFloatInfoFile = fopen(complementaryNodeFloatInfoFileName.c_str(), "r");
    if (!Data::complementaryNodeFloatInfoFile) {
      cout << "*** ERROR *** Could not open complementary node float info file " << complementaryNodeFloatInfoFileName << endl;
      exit(EXIT_FAILURE);
    }
    cout << "Loading complementary float informations from " << complementaryNodeFloatInfoFileName << endl;
    DisplayProperties::hasComplementaryFloat = true;
    int slrNodesCount = 0;
    float minVal = numeric_limits<float>::max();
    float maxVal = numeric_limits<float>::min();
    while ((read = getline(&line, &len, Data::complementaryNodeFloatInfoFile)) != -1) {
      nbFields = sscanf(line,"%d%f", &id, &value);
      if (nbFields == 2) {
        if (id < vectGraphicNode.size() && vectGraphicNode[id].id == id) {
          vectGraphicNode[id].complementaryFloatValue = value;
          if ( value < minVal ) minVal = value;
          if ( value > maxVal ) maxVal = value;
          slrNodesCount++;
        } else {
          cout << "*** WARNING *** node identifier inconsistent between positions and SLR positions files" << endl;
        }
      } else {
        cout << "*** WARNING *** parse error on line: " << line << endl;
      }
    }
    cout << "min: " << minVal  << "  max: " << maxVal << endl;
    float ratio = 255/(maxVal-minVal);
    for (auto it = vectGraphicNode.begin(); it != vectGraphicNode.end(); it++) {
      if ( it->complementaryFloatValue == 0) {
        it->complementaryRed = 0;
        it->complementaryGreen = 255;
        it->complementaryBlue = 0;
      } else if ( it->complementaryFloatValue < 0 ) {
        it->complementaryRed = -it->complementaryFloatValue * ratio;
        it->complementaryGreen = 255 + it->complementaryFloatValue * ratio;
        it->complementaryBlue = 0;
      } else {
        it->complementaryRed = 0;
        it->complementaryGreen = 255 - it->complementaryFloatValue * ratio;
        it->complementaryBlue = it->complementaryFloatValue * ratio;;
      }
      //it->complementaryRed = (it->complementaryFloatValue - minVal) * ratio;
      //cout << "value: " << it->complementaryFloatValue << "  red:" << it->complementaryRed << endl;
      //it->complementaryGreen = 0;
      //it->complementaryBlue = 0;
    }
    cout << slrNodesCount << " complementary float informations loaded for " << vectGraphicNode.size() << " nodes" << endl;

  }

  cout << "Opening events file: " << eventsFileName << endl;
  cout << "... ";
  Data::eventsFile = fopen(eventsFileName.c_str(),"r");
  if (!Data::eventsFile) {
    cout << "Error while opening position file " << eventsFileName << endl;
    exit(EXIT_FAILURE);
  }
  cout << "ok" << endl;

  Data::output.open(eventsFileName);
}


void computeWindowSize() {
  int maxWindowWidth = 1220;
  int maxWindowHeight = 800;
  SDL_DisplayMode dm;
  if (SDL_GetDesktopDisplayMode(0, &dm) != 0) {
    std::cout << "SDL_GetDesktopDisplayMode failed: " << SDL_GetError() <<
      "\n";
  }
  else {
    maxWindowWidth = dm.w * 0.8;
    maxWindowHeight = dm.h * 0.8;
  }

  double ratio = ScenarioParameters::getWorldZSize() /
    ScenarioParameters::getWorldXSize();

  int maxRenderWidth = maxWindowWidth - DisplayProperties::histoWidth;
  int maxRenderHeight = maxWindowHeight - DisplayProperties::histoHeight;

  int renderWidth = maxRenderWidth;
  int renderHeight = maxRenderHeight;

  if (maxRenderWidth * ratio > maxRenderHeight) {
    renderWidth = maxRenderHeight / ratio;
    DisplayProperties::windowWidth =
      std::max(renderWidth + DisplayProperties::histoWidth,
        2 * DisplayProperties::histoWidth);
    DisplayProperties::windowHeight = maxWindowHeight;
  }
  else {
    renderHeight = maxRenderWidth * ratio;
    DisplayProperties::windowWidth = maxWindowWidth;
    DisplayProperties::windowHeight =
      std::max(renderHeight + DisplayProperties::histoHeight,
        4 * DisplayProperties::histoHeight);
  }

  // initial node positions.
  DisplayProperties::offsetXCenter =
    (DisplayProperties::windowWidth - DisplayProperties::histoWidth) / 2;
  DisplayProperties::offsetYCenter =
    DisplayProperties::histoHeight + 40 +
    (DisplayProperties::windowHeight - DisplayProperties::histoHeight - 40) / 2;
  DisplayProperties::offsetXCenterMap =
    -ScenarioParameters::getWorldXSize() / 2;
  DisplayProperties::offsetYCenterMap =
    -ScenarioParameters::getWorldZSize() / 2;

  //DisplayProperties::zoom = 8000000000/ScenarioParameters::getWorldXSize();
  DisplayProperties::zoom = std::max(
    (double)ScenarioParameters::getWorldXSize() /
    (DisplayProperties::windowWidth - DisplayProperties::histoWidth),
    (double)ScenarioParameters::getWorldZSize() /
    (DisplayProperties::windowHeight - DisplayProperties::histoHeight - 40))
    * 1.1;


}


void initialiseSDL() {
  if (SDL_Init(SDL_INIT_VIDEO) != 0){
    std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
    exit(EXIT_FAILURE);
  }

  computeWindowSize();

  SDL_Window *win = SDL_CreateWindow("VisualTracer " VERSION, 100, 100, DisplayProperties::windowWidth, DisplayProperties::windowHeight, SDL_WINDOW_SHOWN);
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

  if ( TTF_Init() == -1 ) {
    std::cout << " Failed to initialize TTF: " << SDL_GetError() << std::endl;
    SDL_Quit();
  }

  // liberation is for Linux (/truetype in debian, / for CentOS/RedHat), arial is for macOS, and font is to allow user to use a font
  const char *fonts[] = {"/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf", "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", "/usr/share/fonts/liberation/LiberationSans-Regular.ttf", "/Library/Fonts/Arial.ttf", "font.ttf"};
  for (int i=0 ; i<5 ; i++) {
    DisplayProperties::font = TTF_OpenFont( fonts[i], 20 );
    if (DisplayProperties::font)  // found it
      break;
  }

  if (! DisplayProperties::font) {
    cerr << "*** ERROR ***  no font file found (LiberationSans-Regular, Arial), please create a file called font.ttf in the current directory" << endl;
    exit(EXIT_FAILURE);
  }

  SDL_SetRenderDrawColor( DisplayProperties::renderer, 0, 0, 0, 255 );
  SDL_RenderClear( DisplayProperties::renderer );
}

void drawNodes(const StepData& step) {
  long x,y;
  int slrRed, slrGreen, slrBlue;

  if (DisplayProperties::useSLRZones) {
    for (auto it=step.vectGraphicNode.begin(); it!=step.vectGraphicNode.end(); it++) {
      slrRed = 150 + (it->corX % 2) * 50;
      slrGreen = 150 + (it->corY % 2) * 50;
      slrBlue = 150 + (it->corZ % 2) * 50;

      int slrRadius = 2 * DisplayProperties::nodeZoom;

      x = DisplayProperties::offsetXCenter + (DisplayProperties::offsetX + it->x + DisplayProperties::offsetXCenterMap)/DisplayProperties::zoom;
      y = DisplayProperties::offsetYCenter + (DisplayProperties::offsetY + it->z + DisplayProperties::offsetYCenterMap)/DisplayProperties::zoom;

      if (DisplayProperties::drawAllSLRZones || DisplayProperties::showAllNodes) {
        filledCircleRGBA(DisplayProperties::renderer, x, y, slrRadius, slrRed, slrGreen, slrBlue, 255);
      } else {
        if (it->hasReceived && DisplayProperties::drawReceptions) filledCircleRGBA(DisplayProperties::renderer, x, y, slrRadius, slrRed, slrGreen, slrBlue, 255);
        if (it->hadCollision && DisplayProperties::drawCollisions) filledCircleRGBA(DisplayProperties::renderer, x, y, slrRadius, slrRed, slrGreen, slrBlue, 255);
        if (it->hasTransmitted && DisplayProperties::drawTransmissions) filledCircleRGBA(DisplayProperties::renderer, x, y, slrRadius, slrRed, slrGreen, slrBlue, 255);
        if (it->hadIgnored && DisplayProperties::drawIgnored) filledCircleRGBA(DisplayProperties::renderer, x, y, slrRadius, slrRed, slrGreen, slrBlue, 255);
      }
    }
  }

  if ( DisplayProperties::hasComplementaryFloat && DisplayProperties::drawComplementaryFloat ) {
    int complementaryRadius = 2 * DisplayProperties::nodeZoom;

    for (auto it=step.vectGraphicNode.begin(); it!=step.vectGraphicNode.end(); it++) {
      x = DisplayProperties::offsetXCenter + (DisplayProperties::offsetX + it->x + DisplayProperties::offsetXCenterMap)/DisplayProperties::zoom;
      y = DisplayProperties::offsetYCenter + (DisplayProperties::offsetY + it->z + DisplayProperties::offsetYCenterMap)/DisplayProperties::zoom;

      filledCircleRGBA(DisplayProperties::renderer, x, y, complementaryRadius, it->complementaryRed, it->complementaryGreen, it->complementaryBlue, 255);
    }
  }

  for (auto it=step.vectGraphicNode.begin(); it!=step.vectGraphicNode.end(); it++) {
    int nodeRadius = 1 * DisplayProperties::nodeZoom;

    if (DisplayProperties::showAllNodes) {
      SDL_SetRenderDrawColor( DisplayProperties::renderer, 200, 200, 200, 255 );
    }

    x = DisplayProperties::offsetXCenter + (DisplayProperties::offsetX + it->x + DisplayProperties::offsetXCenterMap)/DisplayProperties::zoom;
    y = DisplayProperties::offsetYCenter + (DisplayProperties::offsetY + it->z + DisplayProperties::offsetYCenterMap)/DisplayProperties::zoom;

    if (DisplayProperties::showAllNodes) {
      filledCircleRGBA(DisplayProperties::renderer, x, y, nodeRadius, 200, 200, 200, 255);
    }
    //      if (it->wasActive) {
    //          filledCircleRGBA(DisplayProperties::renderer, x, y, 1*DisplayProperties::nodeZoom, 255, 0, 0, 255);
    //      }
    if (it->hasReceived && DisplayProperties::drawReceptions) {
      filledCircleRGBA(DisplayProperties::renderer, x, y, nodeRadius, 0, 255, 0, 255);
    }

    if (it->hadCollision && DisplayProperties::drawCollisions) {
      filledCircleRGBA(DisplayProperties::renderer, x, y, nodeRadius, 255, 0, 0, 255);
    }
    if (it->hasTransmitted && DisplayProperties::drawTransmissions) {
      filledCircleRGBA(DisplayProperties::renderer, x, y, nodeRadius, 0, 0, 255, 255);
    }
    if (it->hadIgnored && DisplayProperties::drawIgnored) {
      filledCircleRGBA(DisplayProperties::renderer, x, y, nodeRadius, 255, 255, 0, 255);
    }
  }
}

void clearStepData(StepData& data) {
  cout << "clearing step data ...";
  for (auto it=data.vectGraphicNode.begin(); it!=data.vectGraphicNode.end(); it++) {
    it->wasActive = false;

    it->nbReceptions = 0;
    it->nbTransmissions = 0;
    it->nbCollisions = 0;
    it->nbIgnored = 0;

    it->hasReceived = false;
    it->hasTransmitted = false;
    it->hadCollision = false;
    it->hadIgnored = false;
  }

  data.histoTotal.fill(0);
  data.histoReceptions.fill(0);
  data.histoTransmissions.fill(0);
  data.histoCollisions.fill(0);
  data.histoIgnored.fill(0);

  data.nbTransmissions = 0;
  data.nbReceptions = 0;
  data.nbCollisions = 0;
  data.nbIgnored = 0;
  data.nbEventsTotal = 0;

  cout << " done" << endl;
}

void injectStepData(StepData& data, string _type, int _id, long _time) {
  float histoIndex;

  //cout << "_time: " << _time << endl;
  histoIndex = (_time - data.startTime) / (data.stepDuration / DisplayProperties::histoElemNumber);
  //     cout << "histoIndex: " << histoIndex << endl;
  //  cout << histoIndex << " = (" << _time << " - " << DisplayProperties::currentTime << ") / " << DisplayProperties::stepDuration << endl;
  assert(histoIndex >= 0 && histoIndex < DisplayProperties::histoElemNumber);
  if ( _type.compare("r") == 0 ) {
    data.vectGraphicNode[_id].nbReceptions++;
    data.vectGraphicNode[_id].wasActive = true;
    data.vectGraphicNode[_id].hasReceived = true;
    data.histoTotal[histoIndex]++;
    data.histoReceptions[histoIndex]++;
    data.nbReceptions++;
    data.nbEventsTotal++;
  } else if ( _type.compare("c") == 0 ) {
    data.vectGraphicNode[_id].nbCollisions++;
    data.vectGraphicNode[_id].wasActive = true;
    data.vectGraphicNode[_id].hadCollision = true;
    data.histoTotal[histoIndex]++;
    data.histoCollisions[histoIndex]++;
    data.nbCollisions++;
    data.nbEventsTotal++;
  } else if ( _type.compare("s") == 0 ) {
    data.vectGraphicNode[_id].nbTransmissions++;
    data.vectGraphicNode[_id].wasActive = true;
    data.vectGraphicNode[_id].hasTransmitted = true;
    data.histoTotal[histoIndex]++;
    data.histoTransmissions[histoIndex]++;
    data.nbTransmissions++;
    data.nbEventsTotal++;
  } else if ( _type.compare("i") == 0 ) {
    data.vectGraphicNode[_id].nbIgnored++;
    data.vectGraphicNode[_id].wasActive = true;
    data.vectGraphicNode[_id].hadIgnored = true;
    data.histoTotal[histoIndex]++;
    data.histoIgnored[histoIndex]++;
    data.nbIgnored++;
    data.nbEventsTotal++;
  } else if ( _type.compare("t") == 0 ) {
    data.histoTotal[histoIndex]++;
    data.nbEventsTotal++;
  } else {
    cout << "Unknown event type: " << _type << endl;
  }
}


simulationTime_t getStepStartTime(StepData& data, simulationTime_t firstEvent) {
  if (firstEvent >= data.startTime + data.stepDuration) {
    // get the first regular increment that will include firstEvent.
    simulationTime_t delta = firstEvent - data.startTime;
    return data.startTime + delta - (delta % data.stepDuration);
  }
  else {
    return data.startTime;
  }
}


void loadStep(StepData& data) {
  static int id;
  static long time=-1;
  static string type;

  LogOutput::LineType res;
  static bool dataAvailable = false;

  data.nbEvents = 0;

  if (data.startTime == 0 && DisplayProperties::initialTimeSkip != 0) {
    data.stepDuration = DisplayProperties::initialTimeSkip;
  } else {
    data.stepDuration = DisplayProperties::normalStepDuration;
  }

  std::cout << "loadStep [" << data.startTime << ", " << data.stepDuration << "]\n";

  while (dataAvailable || time < data.startTime + data.stepDuration) {
    if (dataAvailable) {
      // if the event is too late for this step...
      if (time >= data.startTime + data.stepDuration) {
        if (data.nbEvents == 0) {
          // well, just move the step.
          data.startTime = getStepStartTime(data, time);
        }
        else {
          // unless, of course, it already has events.
          break;
        }
      }

      // save the last loaded line of data.
      injectStepData(data, type, id, time);
      time = -1;
      data.nbEvents += 1;
      dataAvailable = false;
    }

    res = Data::output.readNextLine();
    if (res == LogOutput::LineType::END_OF_FILE) {
      dataAvailable = false;
      break;
    }
    else if (res == LogOutput::LineType::DATA) {
      type = Data::output.currentLineFormatKey();
      if (type.compare("r") == 0 || type.compare("s") == 0 ||
        type.compare("c") == 0 || type.compare("i") == 0 ) {

        time = Data::output.getLogItemLongValue("time");
        id = Data::output.getLogItemIntValue("nodeID");
        dataAvailable = true;
      }
      else {
        // Just read a commented line in the log file.
        // std::cout << "unknown line type: " << type << ", skipping.\n";
      }
    }
  }

  std::cout << data.nbEvents << " events loaded.\n";
  return;
}

bool advanceStep(StepData& data, simulationTime_t startTime) {
  static int nbEventsTotal = 0;

  clearStepData(data);
  data.startTime = startTime;
  loadStep(data);

  nbEventsTotal += data.nbEvents;

  return data.nbEvents != 0;
}

void drawText(int _xPos, int _yPos, string _text, SDL_Color _color) {
  SDL_Rect solidRect;
  SDL_Surface *surface;
  SDL_Texture *texture;

  surface = TTF_RenderText_Solid(DisplayProperties::font, _text.c_str(), { _color.r, _color.g, _color.b, _color.a });
  texture = SDL_CreateTextureFromSurface( DisplayProperties::renderer, surface );
  SDL_QueryTexture( texture, NULL, NULL, &solidRect.w, &solidRect.h );
  solidRect.x = _xPos;
  solidRect.y = _yPos;
  SDL_FreeSurface( surface );
  SDL_RenderCopy(DisplayProperties::renderer, texture, NULL, &solidRect);
  SDL_DestroyTexture( texture);
}

void drawDrawingModesStates() {
  SDL_Color color;

  if (DisplayProperties::drawTransmissions) color = { 0, 0, 255, 255 }; else color = { 100, 100, 100, 255 };
  drawText(20, 0, "(s)end", color);

  if (DisplayProperties::drawReceptions) color = { 0, 255, 0, 255 }; else color = { 100, 100, 100, 255 };
  drawText(170, 0, "(r)eceive", color);

  if (DisplayProperties::drawCollisions) color = { 255, 0, 0, 255 }; else color = { 100, 100, 100, 255 };
  drawText(320, 0, "(c)ollisions", color);

  if (DisplayProperties::drawIgnored) color = { 255, 255, 0, 255 }; else color = { 100, 100, 100, 255 };
  drawText(470, 0, "(i)gnore", color);

  if (DisplayProperties::drawAllSLRZones) color = { 255, 255, 255, 255 }; else color = { 100, 100, 100, 255 };
  drawText(620, 0, "all S(L)R", color);

  if (DisplayProperties::drawComplementaryFloat) color = { 255, 255, 255, 255 }; else color = { 100, 100, 100, 255 };
  drawText(770, 0, "co(m)plementary float", color);

}

void drawCumulativeHisto(const histData &_vectTransmissions, const histData&_vectReceptions, const histData&_vectCollisions, const histData&_vectIgnored, const histData&_vectTotal,
  int _xRect, int _yRect, int _nbEvents) {
  double verticalCoeff, horizontalCoeff;
  int maxTotal = 0;
  SDL_Rect rectangle;
  int hTransmissions, hReceptions, hCollisions, hIgnored;

  int nbElem = _vectTotal.size();

  for (int i=0; i<nbElem; i++) {
    if (_vectTotal[i] > maxTotal) maxTotal = _vectTotal[i];
  }

  verticalCoeff = (double)DisplayProperties::histoHeight / maxTotal;
  if (maxTotal == 0) verticalCoeff = 1;
  horizontalCoeff = (double)DisplayProperties::histoWidth / DisplayProperties::histoElemNumber;

  SDL_SetRenderDrawColor(DisplayProperties::renderer, 255, 255, 255, 255);
  rectangle.x = _xRect;
  rectangle.y = _yRect;
  rectangle.w = DisplayProperties::histoWidth;
  rectangle.h = DisplayProperties::histoHeight;
  SDL_RenderDrawRect( DisplayProperties::renderer, &rectangle );

  for (int i=0; i<nbElem; i++) {
    rectangle.x = _xRect + i * horizontalCoeff;

    SDL_SetRenderDrawColor(DisplayProperties::renderer, 0, 0, 180, 255);
    rectangle.y = _yRect + DisplayProperties::histoHeight;
    rectangle.w = horizontalCoeff;
    hTransmissions = -_vectTransmissions[i] * verticalCoeff;
    if (hTransmissions < 0 && hTransmissions > -1) hTransmissions = -1;
    rectangle.h = hTransmissions;
    SDL_RenderFillRect( DisplayProperties::renderer, &rectangle );

    SDL_SetRenderDrawColor(DisplayProperties::renderer, 0, 180, 0, 255);
    rectangle.y = _yRect + DisplayProperties::histoHeight + hTransmissions;
    rectangle.w = horizontalCoeff;
    hReceptions = -_vectReceptions[i] * verticalCoeff;
    if (hReceptions < 0 && hReceptions > -1) hReceptions = -1;
    rectangle.h = hReceptions;
    SDL_RenderFillRect( DisplayProperties::renderer, &rectangle );

    SDL_SetRenderDrawColor(DisplayProperties::renderer, 180, 0, 0, 255);
    rectangle.y = _yRect + DisplayProperties::histoHeight + hTransmissions + hReceptions;
    rectangle.w = horizontalCoeff;
    hCollisions = -_vectCollisions[i] * verticalCoeff;
    if (hCollisions < 0 && hCollisions > -1) hCollisions = -1;
    rectangle.h = hCollisions;
    SDL_RenderFillRect( DisplayProperties::renderer, &rectangle );

    SDL_SetRenderDrawColor(DisplayProperties::renderer, 180, 180, 0, 255);
    rectangle.y = _yRect + DisplayProperties::histoHeight + hTransmissions + hReceptions + hCollisions;
    rectangle.w = horizontalCoeff;
    hIgnored = -_vectIgnored[i] * verticalCoeff;
    if (hIgnored < 0 && hIgnored > -1) hIgnored = -1;
    rectangle.h = hIgnored;
    SDL_RenderFillRect( DisplayProperties::renderer, &rectangle );
  }

  drawText(_xRect + 4, _yRect + 4, to_string(maxTotal).c_str(), { 255, 255, 255, 255 });
  drawText(_xRect + 4, _yRect + DisplayProperties::histoHeight - 24, "0", { 255, 255, 255, 255 });
  drawText(_xRect + DisplayProperties::histoWidth - 120, _yRect + DisplayProperties::histoHeight / 2, to_string(_nbEvents), { 255, 255, 255, 255 } );
}

void drawHisto(const histData &_vect, int _xRect, int _yRect, SDL_Color _color, int _nbEvents) {
  double verticalCoeff, horizontalCoeff;
  int maxTotal=0;
  SDL_Rect rectangle;

  for (auto it=_vect.begin(); it!=_vect.end(); it++) {
    if ((*it) > maxTotal) maxTotal = (*it);
  }

  verticalCoeff = (double)DisplayProperties::histoHeight / maxTotal;
  if (maxTotal == 0) verticalCoeff = 1;
  horizontalCoeff = (double)DisplayProperties::histoWidth / DisplayProperties::histoElemNumber;

  SDL_SetRenderDrawColor(DisplayProperties::renderer, 255, 255, 255, 255);
  rectangle.x = _xRect;
  rectangle.y = _yRect;
  rectangle.w = DisplayProperties::histoWidth;
  rectangle.h = DisplayProperties::histoHeight;
  SDL_RenderDrawRect( DisplayProperties::renderer, &rectangle );

  rectangle.w = horizontalCoeff;
  int i = 0;
  for (auto it=_vect.begin(); it!=_vect.end(); it++) {
    SDL_SetRenderDrawColor(DisplayProperties::renderer, _color.r, _color.g, _color.b, _color.a);
    rectangle.x = _xRect + i * horizontalCoeff;
    rectangle.y = _yRect + DisplayProperties::histoHeight;
    rectangle.h = -(*it)*verticalCoeff;
    SDL_RenderFillRect(DisplayProperties::renderer, &rectangle);
    i++;
  }

  drawText(_xRect + 4, _yRect + 4, to_string(maxTotal).c_str(), { 255, 255, 255, 255 });
  drawText(_xRect + 4, _yRect + DisplayProperties::histoHeight - 24, "0", { 255, 255, 255, 255 });
  drawText(_xRect + DisplayProperties::histoWidth - 120, _yRect + DisplayProperties::histoHeight / 2, to_string(_nbEvents), { 255, 255, 255, 255 } );
}

void renderStep(int stepNum, const StepData& data) {
  cout << "rendering step " << stepNum << " [ " << data.startTime << " , " << data.startTime + data.stepDuration << " ], (" << data.nbEvents << " events)" << endl;
  SDL_SetRenderDrawColor( DisplayProperties::renderer, 0, 0, 0, 255 );
  SDL_RenderClear( DisplayProperties::renderer );
  drawNodes(data);
  drawDrawingModesStates();
  drawHisto(data.histoIgnored,DisplayProperties::windowWidth-DisplayProperties::histoWidth,40+(20+DisplayProperties::histoHeight)*0, {180,180,0,255}, data.nbIgnored);
  drawHisto(data.histoCollisions,DisplayProperties::windowWidth-DisplayProperties::histoWidth,40+(20+DisplayProperties::histoHeight)*1, {180,0,0,255}, data.nbCollisions);
  drawHisto(data.histoReceptions,DisplayProperties::windowWidth-DisplayProperties::histoWidth,40+(20+DisplayProperties::histoHeight)*2, {0,180,0,255}, data.nbReceptions);
  drawHisto(data.histoTransmissions,DisplayProperties::windowWidth-DisplayProperties::histoWidth,40+(20+DisplayProperties::histoHeight)*3, {0,0,180,255}, data.nbTransmissions);

  drawCumulativeHisto(data.histoTransmissions, data.histoReceptions, data.histoCollisions, data.histoIgnored, data.histoTotal, 0, 40, data.nbEventsTotal);
  SDL_RenderPresent(DisplayProperties::renderer);
}

list<StepData> precomputeMapData(NodeVect& nodes) {
  // precompute step data.
  list<StepData> data;

  bool hasMoreData = true;
  simulationTime_t currentTime = 0;
  while (hasMoreData) {
    StepData step;
    step.vectGraphicNode = nodes;

    hasMoreData = advanceStep(step, currentTime);
    currentTime = step.startTime + step.stepDuration;


    if (hasMoreData) {
      data.push_back(step);
      if (data.size() % 10 == 0) {
        std::cout << "loaded " << data.size() << " steps...\n";
      }
    }
  }

  std::cout << "finished loading " << data.size() << " steps.\n";

  return data;
}

bool processSDLEvent(long unsigned int& currentStep,
  list<StepData>::iterator& iterator, list<StepData>& data) {

  SDL_Event evt;
  bool render = false;

  SDL_WaitEvent(&evt);

  if(evt.type == SDL_KEYDOWN) {
    if (evt.key.keysym.sym == SDLK_SPACE ||
      evt.key.keysym.sym == SDLK_g) {
      if (!ScenarioParameters::getPrecomputeSteps()) {
        // advance by one more step.
        bool hasMoreData = advanceStep(*iterator,
          iterator->startTime + iterator->stepDuration);
        if (hasMoreData) {
          ++currentStep;
          render = true;
        }
      }
      else {
        if (iterator != --data.end()) {
          ++iterator;
          ++currentStep;
          render = true;
        }
      }
    }
    if (evt.key.keysym.sym == SDLK_f &&
      ScenarioParameters::getPrecomputeSteps()) {
      if (iterator != data.begin()) {
        --iterator;
        --currentStep;
        render = true;
      }
    }
    if (evt.key.keysym.sym == SDLK_a) {
      DisplayProperties::zoom *= 1.1;
      render = true;
    }
    if (evt.key.keysym.sym == SDLK_z) {
      DisplayProperties::zoom /= 1.1;
      render = true;
    }
    if (evt.key.keysym.sym == SDLK_s) {
      DisplayProperties::drawTransmissions = !DisplayProperties::drawTransmissions;
      render = true;
    }
    if (evt.key.keysym.sym == SDLK_r) {
      DisplayProperties::drawReceptions = !DisplayProperties::drawReceptions;
      render = true;
    }
    if (evt.key.keysym.sym == SDLK_c) {
      DisplayProperties::drawCollisions = !DisplayProperties::drawCollisions;
      render = true;
    }
    if (evt.key.keysym.sym == SDLK_i) {
      DisplayProperties::drawIgnored = !DisplayProperties::drawIgnored;
      render = true;
    }
    if (evt.key.keysym.sym == SDLK_l) {
      DisplayProperties::drawAllSLRZones = !DisplayProperties::drawAllSLRZones;
      render = true;
    }
    if (evt.key.keysym.sym == SDLK_m) {
      DisplayProperties::drawComplementaryFloat = !DisplayProperties::drawComplementaryFloat;
      render = true;
    }
    if (evt.key.keysym.sym == SDLK_LEFT) {
      DisplayProperties::offsetX += DisplayProperties::moveSpeed * DisplayProperties::zoom;
      render = true;
    }
    if (evt.key.keysym.sym == SDLK_RIGHT) {
      DisplayProperties::offsetX -= DisplayProperties::moveSpeed * DisplayProperties::zoom;
      render = true;
    }
    if (evt.key.keysym.sym == SDLK_UP) {
      DisplayProperties::offsetY += DisplayProperties::moveSpeed * DisplayProperties::zoom;
      render = true;
    }
    if (evt.key.keysym.sym == SDLK_DOWN) {
      DisplayProperties::offsetY -= DisplayProperties::moveSpeed * DisplayProperties::zoom;
      render = true;
    }
    if (evt.key.keysym.sym == SDLK_q && evt.key.keysym.mod && KMOD_LCTRL != 0) {
      SDL_Quit();
      exit(0);
    }
  }
  if(evt.type == SDL_QUIT) {
    SDL_Quit();
    exit(0);
  }

  // if there are further events, process immediately.
  if (SDL_PollEvent(NULL) == 1) {
    render = render || processSDLEvent(currentStep, iterator, data// , cont
      );
  }

  return render;
}

void mainSDLLoopMap(NodeVect& nodes) {
  list<StepData> data;
  if (ScenarioParameters::getPrecomputeSteps()) {
    data = precomputeMapData(nodes);
  }
  else {
    // load just one step.
    StepData step;
    step.vectGraphicNode = nodes;
    advanceStep(step, 0);
    data.push_back(step);
  }

  initialiseSDL();

  bool programrunning = true;
  long unsigned int currentStep = 0;
  auto iterator = data.begin();

  renderStep(currentStep, *iterator);
  while (programrunning) {
    bool render = processSDLEvent(currentStep, iterator, data);

    if (render) {
      renderStep(currentStep, *iterator);
    }
  }
}

void glRenderStaticStep(Renderer& renderer, const StepData& firstStep) {
  DisplayProperties::showAllNodes = true;

  if (DisplayProperties::drawAllSLRZones && DisplayProperties::useSLRZones) {
    float slrRed, slrGreen, slrBlue;
    for (auto node : firstStep.vectGraphicNode) {
      slrRed = 0.6 + (node.corX % 2) * 0.2;
      slrGreen = 0.6 + (node.corY % 2) * 0.2;
      slrBlue = 0.6 + (node.corZ % 2) * 0.2;

      renderer.addStaticPoint(Point
        {(float)node.x, (float)node.y, (float)node.z,
            slrRed, slrGreen, slrBlue, 1});
    }
  }
  else if (DisplayProperties::showAllNodes) {
    for (auto node : firstStep.vectGraphicNode) {
      renderer.addStaticPoint(Point
        {(float)node.x, (float)node.y, (float)node.z, 0.4, 0.4, 0.4, 1});
    }
  }


  if ( DisplayProperties::hasComplementaryFloat &&
    DisplayProperties::drawComplementaryFloat ) {
    for (auto node : firstStep.vectGraphicNode) {
      renderer.addStaticPoint(Point
        {(float)node.x, (float)node.y, (float)node.z,
            (float)node.complementaryRed / 255,
            (float)node.complementaryGreen / 255,
            (float)node.complementaryBlue / 255, 0.5 });
    }
  }

}

void glDrawNodes(Renderer& renderer, const StepData& step) {
  for (auto node : step.vectGraphicNode) {
    if (node.hasReceived && DisplayProperties::drawReceptions) {
      renderer.addDynamicPoint((float)node.x, (float)node.y, (float)node.z,
        0, 1, 0, 0.4);
    }

    if (node.hadCollision && DisplayProperties::drawCollisions) {
      renderer.addDynamicPoint((float)node.x, (float)node.y, (float)node.z,
        1, 0, 0, 0.4);
    }
    if (node.hasTransmitted && DisplayProperties::drawTransmissions) {
      renderer.addDynamicPoint((float)node.x, (float)node.y, (float)node.z,
        0, 0, 1, 0.4);
    }
    if (node.hadIgnored && DisplayProperties::drawIgnored) {
      renderer.addDynamicPoint((float)node.x, (float)node.y, (float)node.z,
        1, 1, 0, 0.4);
    }
  }
}

void glDrawDrawingModesStates(Renderer& render) {
  auto size = render.getWindowSize();
  int x = -size.first;
  int y = size.second - 50;

  // render.addText does not support color, yet.
  render.addText(x + 20, y, DisplayProperties::drawTransmissions
    ? "(s)end" : " s end");

  render.addText(x + 170, y, DisplayProperties::drawReceptions
    ? "(r)eceive" : " r eceive");

  render.addText(x + 390, y, DisplayProperties::drawCollisions
    ? "(c)ollisions" : " c ollisions");

  render.addText(x + 640, y, DisplayProperties::drawIgnored
    ? "ig(n)ore" : "ig n ore");

  render.addText(x + 840, y, DisplayProperties::drawAllSLRZones
    ? "SLR z(o)nes" : "SLR z o nes");

  render.addText(x + 1150, y, DisplayProperties::drawComplementaryFloat
    ? "co(m)plementary float" : "co m plementary float");
}

void glRenderStep(Renderer& render, int stepNum, const StepData& data) {
  cout << "rendering step " << stepNum << " [ " << data.startTime << " , " << data.startTime + data.stepDuration << " ], (" << data.nbEvents << " events)" << endl;

  render.clearDynamicPoints();
  glDrawNodes(render, data);

  // todo: render histograms.
}

bool glProcessSDLEvent(Renderer& renderer, long unsigned int& currentStep,
  list<StepData>::iterator& iterator, list<StepData>& data) {
  SDL_Event evt;
  bool reprocess = false;
  while (SDL_PollEvent(&evt)) {
    if(evt.type == SDL_KEYDOWN) {
      if (evt.key.keysym.sym == SDLK_SPACE ||
        evt.key.keysym.sym == SDLK_g) {
        if (!ScenarioParameters::getPrecomputeSteps()) {
          // advance by one more step.
          bool hasMoreData = advanceStep(*iterator,
            iterator->startTime + iterator->stepDuration);
          if (hasMoreData) {
            ++currentStep;
            reprocess = true;
          }
        }
        else {
          if (iterator != --data.end()) {
            ++iterator;
            ++currentStep;
            reprocess = true;
          }
        }
      }
      if (evt.key.keysym.sym == SDLK_f && ScenarioParameters::getPrecomputeSteps()) {
        if (iterator != data.begin()) {
          --iterator;
          --currentStep;
          reprocess = true;
        }
      }
      if (evt.key.keysym.sym == SDLK_a) {
        int zoom = 40000;
        if (evt.key.keysym.mod & KMOD_SHIFT) {
          zoom *= 10;
        }
        renderer.translate(0, 0, zoom);
      }
      if (evt.key.keysym.sym == SDLK_z) {
        int zoom = 40000;
        if (evt.key.keysym.mod & KMOD_SHIFT) {
          zoom *= 10;
        }
        renderer.translate(0, 0, -zoom);
      }
      if (evt.key.keysym.sym == SDLK_j) {
        renderer.rotate(0, 5, 0);
      }
      if (evt.key.keysym.sym == SDLK_l) {
        renderer.rotate(0, -5, 0);
      }
      if (evt.key.keysym.sym == SDLK_k) {
        renderer.rotate(0, 0, -5);
      }
      if (evt.key.keysym.sym == SDLK_i) {
        renderer.rotate(0, 0, 5);
      }
      if (evt.key.keysym.sym == SDLK_s) {
        DisplayProperties::drawTransmissions = !DisplayProperties::drawTransmissions;
        reprocess = true;
      }
      if (evt.key.keysym.sym == SDLK_r) {
        DisplayProperties::drawReceptions = !DisplayProperties::drawReceptions;
        reprocess = true;
      }
      if (evt.key.keysym.sym == SDLK_c) {
        DisplayProperties::drawCollisions = !DisplayProperties::drawCollisions;
        reprocess = true;
      }
      if (evt.key.keysym.sym == SDLK_n) {
        DisplayProperties::drawIgnored = !DisplayProperties::drawIgnored;
        reprocess = true;
      }
      if (evt.key.keysym.sym == SDLK_o) {
        DisplayProperties::drawAllSLRZones =
          !DisplayProperties::drawAllSLRZones;
        reprocess = true;
      }
      if (evt.key.keysym.sym == SDLK_m) {
        DisplayProperties::drawComplementaryFloat =
          !DisplayProperties::drawComplementaryFloat;
        reprocess = true;
      }
      if (evt.key.keysym.sym == SDLK_LEFT) {
        int mov = 100000;
        if (evt.key.keysym.mod & KMOD_SHIFT) {
          mov *= 10;
        }
        renderer.translate(mov, 0, 0);
      }
      if (evt.key.keysym.sym == SDLK_RIGHT) {
        int mov = 100000;
        if (evt.key.keysym.mod & KMOD_SHIFT) {
          mov *= 10;
        }
        renderer.translate(-mov, 0, 0);
      }
      if (evt.key.keysym.sym == SDLK_UP) {
        int mov = 100000;
        if (evt.key.keysym.mod & KMOD_SHIFT) {
          mov *= 10;
        }
        renderer.translate(0, -mov, 0);
      }
      if (evt.key.keysym.sym == SDLK_DOWN) {
        int mov = 100000;
        if (evt.key.keysym.mod & KMOD_SHIFT) {
          mov *= 10;
        }
        renderer.translate(0, mov, 0);
      }
      if (evt.key.keysym.sym == SDLK_q && evt.key.keysym.mod &&
        KMOD_LCTRL != 0) {
        SDL_Quit();
        exit(0);
      }
    }
    else if (evt.type == SDL_MOUSEMOTION) {
      if (evt.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
        renderer.rotate(0, -evt.motion.xrel / 10.0, -evt.motion.yrel / 10.0);
      }
    }
    else if (evt.type == SDL_QUIT) {
      SDL_Quit();
      exit(0);
    }
  }

  return reprocess;
}

void glRecenterView(Renderer& renderer, const vector<GraphicNode>& nodes) {
  long int minx = 0;
  long int miny = 0;
  long int minz = 0;
  long int maxx = 0;
  long int maxy = 0;
  long int maxz = 0;

  for (auto node : nodes) {
    if (node.x < minx) {
      minx = node.x;
    }
    if (node.x > maxx) {
      maxx = node.x;
    }
    if (node.y < miny) {
      miny = node.y;
    }
    if (node.y > maxy) {
      maxy = node.y;
    }
    if (node.z < minz) {
      minz = node.z;
    }
    if (node.z > maxz) {
      maxz = node.z;
    }
  }

  long int medx = minx + ((maxx - minx) / 2);
  long int medy = miny + ((maxy - miny) / 2);
  long int medz = minz + ((maxz - minz) / 2);
  renderer.resetView(medx, medy, medz, maxx - minx, maxy - miny);
}

void mainGLLoopMap(NodeVect& nodes) {
  list<StepData> data;
  if (ScenarioParameters::getPrecomputeSteps()) {
    data = precomputeMapData(nodes);
  }
  else {
    // load just one step.
    StepData step;
    step.vectGraphicNode = nodes;
    advanceStep(step, 0);
    data.push_back(step);
  }

  Renderer renderer;
  std::string title("VisualTracer " VERSION);
  renderer.createWindow(title);

  bool programrunning = true;
  long unsigned int currentStep = 0;
  auto iterator = data.begin();

  glRenderStaticStep(renderer, *iterator);
  glRecenterView(renderer, iterator->vectGraphicNode);

  glRenderStep(renderer, currentStep, *iterator);
  while (programrunning) {
    bool reprocess = glProcessSDLEvent(renderer, currentStep, iterator, data);
    if (reprocess) {
      glRenderStep(renderer, currentStep, *iterator);
    }
    glDrawDrawingModesStates(renderer);
    renderer.redraw();
  }
}

void loadNodeEvents() {
  //char *line;
  //size_t len = 512;

  //int nbFields;
  int totalEventsCount = 0;
  int nodeEventsCount = 0;

  //char type;
  string type;
  int nodeId;
  //int srcSeqNumber;
  //int packetId;
  simulationTime_t time;
  //int flowId;
  int transmitterId;
  int size;
  int beta;

  int eventId = 0;

  LogOutput::LineType res;

  simulationTime_t packetDuration;

  //line = (char *)malloc(512);

  simulationTime_t tp = ScenarioParameters::getPulseDuration();

  int selectedNodeId = ScenarioParameters::getChronoNodeId();

  int position = 0;
  Data::verticalPositionByNodeIdMap.insert(pair<int,int>(selectedNodeId,position++));

  Data::othersNodesToDisplayVector = ScenarioParameters::getOthersNodesToDisplay();
  if ( Data::othersNodesToDisplayVector.empty() ) {
    DisplayProperties::chronoDisplayAllNodes = true;
  }
  vector<int>::iterator iontd;
  for (iontd = Data::othersNodesToDisplayVector.begin(); iontd != Data::othersNodesToDisplayVector.end(); iontd++) {
    Data::othersNodesToDisplaySet.insert(*iontd);
    Data::verticalPositionByNodeIdMap.insert(pair<int,int>(*iontd, position++));
  }


  while ( (res = Data::output.readNextLine()) != LogOutput::LineType::END_OF_FILE ) {
    if ( res == LogOutput::LineType::DATA ) {
      type = Data::output.currentLineFormatKey();

      type = Data::output.currentLineFormatKey();
      if ( type.compare("r") == 0 ||
        type.compare("s") == 0 ||
        type.compare("c") == 0 ||
        type.compare("i") == 0 ) {


        nodeId = Data::output.getLogItemIntValue("nodeID");
        transmitterId = Data::output.getLogItemIntValue("transmitterID");
        time = Data::output.getLogItemLongValue("time");
        beta = Data::output.getLogItemIntValue("beta");
        size = Data::output.getLogItemIntValue("size");
        if ( DisplayProperties::chronoDisplayAllNodes &&
          transmitterId != selectedNodeId &&
          nodeId != selectedNodeId &&
          Data::othersNodesToDisplaySet.find(transmitterId) == Data::othersNodesToDisplaySet.end() ) {
          Data::othersNodesToDisplaySet.insert(transmitterId);
          Data::othersNodesToDisplayVector.push_back(transmitterId);
          Data::verticalPositionByNodeIdMap.insert(pair<int,int>(transmitterId, position++));
        }
        if ( nodeId == selectedNodeId ) {
          //if (nodeId == selectedNodeId || Data::othersNodesToDisplaySet.find(nodeId) != Data::othersNodesToDisplaySet.end() ) {
          //    type
          //    simulationTime_t startDate;
          //    simulationTime_t endDate;
          //    int beta;
          //    int size;
          //    int transmitterId;

          //cout << "type : " << type << endl;
          packetDuration = (size-1)*tp*beta+tp;
          Data::vectEventInfo.push_back({eventId++, type, time-packetDuration, time, beta, size, transmitterId});

          nodeEventsCount++;
        }
      }
      totalEventsCount++;
    }
  }

  //    while ( getline(&line, &len, Data::eventsFile) > 0 ) {
  //        nbFields = sscanf(line,"%c%d%d%d%ld%d%d%d%d",&type, &nodeId, &srcSeqNumber, &packetId, &time, &flowId, &transmitterId, &size, &beta);
  //        if (nbFields != 9) {
  //            cout << "Warning" << endl;
  //            cout << line << endl;
  //        }
  //        if (nodeId == selectedNodeId) {
  //            //    type
  //            //    simulationTime_t startDate;
  //            //    simulationTime_t endDate;
  //            //    int beta;
  //            //    int size;
  //            //    int transmitterId;
  //
  //            //cout << "type : " << type << endl;
  //            packetDuration = (size-1)*tp*beta+tp;
  //            Data::vectEventInfo.push_back({eventId++, type, time-packetDuration, time, beta, size, transmitterId});
  //
  //            nodeEventsCount++;
  //        }
  //        totalEventsCount++;
  //    }
  cout << nodeEventsCount << " events for selected node(s)" << endl;
  cout << totalEventsCount << " events in total" << endl;
  cout << "Point of view: node " << selectedNodeId << endl;
  cout << "Others nodes to display: ";
  for (set<int>::iterator it=Data::othersNodesToDisplaySet.begin(); it != Data::othersNodesToDisplaySet.end(); it++ ) {
    cout << " " << *it;
  }
  cout << endl;
  DisplayProperties::currentTime = ScenarioParameters::getInitialTimeSkip();
}

void drawEvent(EventInfo_t _event) {
  SDL_Rect rectangle;
  int r,g,b;

  int yBase = Data::verticalPositionByNodeIdMap.find( _event.transmitterId )->second * 25;

  static int col = 100;

  if ( _event.type.compare("c") == 0 ) {
    r = 255;
    g = col % 155;
    b = col % 155;
  } else if ( _event.type.compare("r") == 0 ) {
    r = col % 155;
    g = 255;
    b = col % 155;
  } else if ( _event.type.compare("i") == 0 ) {
    r = 255;
    g = 255;
    b = col % 155;
  } else if ( _event.type.compare("s") == 0 ) {
    r = col % 155;
    g = col % 155;
    b = 255;
  } else {
    //cout << "other event type ??? " << _event.type << endl;
    r = 0;
    g = 255;
    b = 0;
  }
  SDL_SetRenderDrawColor(DisplayProperties::renderer, r, g, b, 255);
  rectangle.x = (_event.startDate-DisplayProperties::currentTime) * DisplayProperties::zoom;
  rectangle.y = 2+yBase;
  rectangle.w = (_event.endDate-_event.startDate) * DisplayProperties::zoom;
  rectangle.h = 16;
  SDL_RenderDrawRect( DisplayProperties::renderer, &rectangle );

  simulationTime_t tp = ScenarioParameters::getPulseDuration();
  simulationTime_t ts1 = tp * _event.beta;

  for (simulationTime_t i=0; i<_event.size; i++) {
    rectangle.x = ((_event.startDate-DisplayProperties::currentTime)  + i * ts1 )* DisplayProperties::zoom;
    rectangle.y = 6+yBase;
    rectangle.w = tp * DisplayProperties::zoom;
    if (rectangle.w < 1) rectangle.w = 1;

    rectangle.h = 10;
    SDL_RenderFillRect( DisplayProperties::renderer, &rectangle );
  }

  simulationTime_t p1Start = _event.startDate;
  simulationTime_t p1End = _event.endDate;
  simulationTime_t b1Start = _event.startDate;
  simulationTime_t b1End = b1Start + tp;

  simulationTime_t p2Start;
  simulationTime_t p2End;
  simulationTime_t b2Start;
  simulationTime_t b2End;
  simulationTime_t ts2;

  int p1Size = _event.size;
  int p2Size;

  if ( DisplayProperties::chronoShowCollisions ) {
    for (auto it=Data::vectEventInfo.begin(); it != Data::vectEventInfo.end(); it++) { // for each packet
      if ( it->eventId != _event.eventId ) { // not myself

        p1Start = _event.startDate;
        p1End = _event.endDate;
        b1Start = p1Start;
        b1End = b1Start + tp;

        p2Start = it->startDate;
        p2End = it->endDate;
        b2Start = it->startDate;
        b2End = b2Start + tp;
        p2Size = it->size;
        ts2 = tp * it->beta;

        int p1Count = 0;
        int p2Count = 0;

        bool progress;

        if ( !(p2End < p1Start || p1End < p2Start )) {  // packets are overlapping at least a little bit
          SDL_RenderDrawLine(DisplayProperties::renderer,
            ((_event.startDate-DisplayProperties::currentTime)  + p1Count * ts1 )* DisplayProperties::zoom,
            yBase,
            ((_event.startDate-DisplayProperties::currentTime)  + p1Count * ts1 )* DisplayProperties::zoom,
            yBase+10);

          while ( p1Count < p1Size && p2Count < p2Size ) {
            progress = false;
            if (b1End < b2Start) {
              p1Count++;
              b1Start += ts1;
              b1End += ts1;
              progress = true;
            }
            if (b2End < b1Start) {
              p2Count++;
              b2Start += ts2;
              b2End += ts2;
              progress = true;
            }
            if ( !progress ) {
              SDL_RenderDrawLine(DisplayProperties::renderer,
                ((_event.startDate-DisplayProperties::currentTime)  + p1Count * ts1 )* DisplayProperties::zoom,
                20,
                ((_event.startDate-DisplayProperties::currentTime)  + p1Count * ts1 )* DisplayProperties::zoom,
                200);
              //cout << "collision bit:" << p1Count << " b1Start:" << b1Start << " b1End:" << b1End << "  bit:" << p2Count << " b2Start:" << b2Start << " b2End:" << b2End << endl;
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
  }

  col += 100;
}

void drawChronogram() {
  simulationTime_t viewStart, viewEnd;

  viewStart = DisplayProperties::currentTime;
  viewEnd = DisplayProperties::currentTime + DisplayProperties::stepDuration;

  DisplayProperties::zoom = DisplayProperties::windowWidth / double(viewEnd-viewStart);

  SDL_SetRenderDrawColor(DisplayProperties::renderer, 0, 0, 0, 255);
  SDL_RenderClear( DisplayProperties::renderer );

  cout << "Drawing [ " << viewStart << "; " << viewEnd << " ]  zoom:" << DisplayProperties::zoom << endl;

  /*
   * Draw labels (node numbers) on the left side
   */
  SDL_Color color;
  color = { 255, 255, 255, 255 };
  int yBase = 0;
  char lineLabel[256];
  sprintf(lineLabel,"%s%d", "Node ", ScenarioParameters::getChronoNodeId() );
  drawText(20, yBase, lineLabel, color);

  vector<int>::iterator iontd;
  for (iontd = Data::othersNodesToDisplayVector.begin(); iontd != Data::othersNodesToDisplayVector.end(); iontd++) {
    yBase += 25;
    sprintf(lineLabel,"%s%d", "Node ", *iontd );
    drawText(20, yBase, lineLabel, color);
  }

  /*
   * Draw events themselves
   */
  for (auto it=Data::vectEventInfo.begin(); it!=Data::vectEventInfo.end(); it++) {
    if ( (it->startDate <= viewStart && it->endDate >= viewEnd) ||
      (it->startDate >= viewStart && it->startDate <= viewEnd) ||
      (it->endDate >= viewStart && it->endDate <= viewEnd)) {
      drawEvent((*it));
    }
  }
  SDL_RenderPresent(DisplayProperties::renderer);
}

void mainSDLLoopChrono() {
  SDL_Event evt;
  bool programrunning = true;

  cout << "width: " << DisplayProperties::windowWidth  << endl;
  drawChronogram();
  while(programrunning) {
    SDL_WaitEvent(&evt);
    if(evt.type == SDL_KEYDOWN) {
      if (evt.key.keysym.sym == SDLK_SPACE) {
        DisplayProperties::currentTime += DisplayProperties::stepDuration/4;
        drawChronogram();
      }
      if (evt.key.keysym.sym == SDLK_z) {
        DisplayProperties::currentTime +=  DisplayProperties::stepDuration * 0.1;
        DisplayProperties::stepDuration *=0.8;

        cout << "step change:" << DisplayProperties::stepDuration << endl;
        drawChronogram();
      }
      if (evt.key.keysym.sym == SDLK_a) {
        DisplayProperties::stepDuration *=1.25;
        DisplayProperties::currentTime -=  DisplayProperties::stepDuration * 0.1;
        drawChronogram();
      }

      if (evt.key.keysym.sym == SDLK_LEFT) {
        DisplayProperties::currentTime -=  DisplayProperties::stepDuration / 4;
        drawChronogram();
      }
      if (evt.key.keysym.sym == SDLK_RIGHT) {
        DisplayProperties::currentTime +=  DisplayProperties::stepDuration / 4;
        drawChronogram();
      }

      if (evt.key.keysym.sym == SDLK_c) {
        DisplayProperties::chronoShowCollisions = !DisplayProperties::chronoShowCollisions;
        drawChronogram();
      }

      if (evt.key.keysym.sym == SDLK_q && evt.key.keysym.mod && KMOD_LCTRL != 0) {
        SDL_Quit();
        exit(0);
      }
    }
    if(evt.type == SDL_QUIT) {
      SDL_Quit();
      exit(0);
    }
  }
}

int main(int argc, char **argv) {
  cout << "VisualTracer " VERSION << endl;

  ScenarioParameters::initialize(argc, argv, 1);

  if ( ScenarioParameters::getChronoNodeId() != -1 ){
    initialiseVisualizer(Data::vectGraphicNode);
    initialiseSDL();
    loadNodeEvents();
    mainSDLLoopChrono();
  } else if (true) { // currently only manual switch until settings.h arrives.
    NodeVect nodes;
    initialiseVisualizer(nodes);
    mainSDLLoopMap(nodes);
  }
  else {
    NodeVect nodes;
    initialiseVisualizer(nodes);
    mainGLLoopMap(nodes);
  }

  return EXIT_SUCCESS;
}
