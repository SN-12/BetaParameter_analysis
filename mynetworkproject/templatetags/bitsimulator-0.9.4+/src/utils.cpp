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

#include "utils.h"
#include <cstdarg>
#include <cassert>

//==============================================================================
//
//          ScenarioParameters  (class)
//
//==============================================================================

ScenarioParameters *ScenarioParameters::scenarioParameters = nullptr;

ScenarioParameters::ScenarioParameters(int argc, char **argv, int program) {
  int xmlLoadReturnCode;

  scenarioParameters = this;

  try {
    string text;
    if (program == 0)
      text = "BitSimulator is a fast wireless nanonetwork bit-level simulator for routing and transport levels.";
    else
      text = "VisualTracer is a program to visualise the log files of BitSimulator.  BitSimulator is a fast wireless nanonetwork bit-level simulator for routing and transport levels.";
    TCLAP::CmdLine cmd(text, ' ', VERSION);

    // options for one of the two programs: define them, but do not add them to cmdline
    TCLAP::SwitchArg graphicModeParam("g","graphicMode","show graphically what's happening", false);

    TCLAP::SwitchArg acceptCollisionedPacketsParam("","acceptCollisionedPackets","Collisioned packets are forwarded to routing agent, it's up to agents to handle them", false);

    TCLAP::SwitchArg doNotUseNeighboursListParam("","doNotUseNeighboursList","Save (a lot) of memory in high density scenarios, but slower", false);

    TCLAP::ValueArg<long> stepLengthParam("s","stepLength","Step length for channel usage mode",false,1000000,"long");
    TCLAP::ValueArg<long> initialTimeSkipParam("","initialTimeSkip","Directly jump to this date",false,0,"long");
    TCLAP::ValueArg<int> chronoParam("","chrono","Switch to chronogram mode and shows the specified node",false,-1,"int");
    TCLAP::ValueArg<int> nodeZoomParam("","nodeZoom","How big nodes will be displayed",false,3,"int");
    TCLAP::ValueArg<string> complementaryNodeFloatInfoFileNameParam("","complementaryNodeFloatInfoFileName", "File containing a float value per node to be displayed as color scale", false,"","string");
    TCLAP::MultiArg<int> othersNodesToDisplayParam("","cn","IDs of other nodes to display in chronogram mode",false,"long");
    TCLAP::SwitchArg precomputeStepsParam("", "precomputeSteps", "Precompute display data. May consume a lot of memory!", false);

    // options for both BitSimulator and VisualTracer
    TCLAP::ValueArg<std::string> scenarioDirectoryParam("D","scenarioDirectory", "Directory for this scenario", false,".","string", cmd);
    TCLAP::ValueArg<std::string> scenarioFileParam("","scenarioFile", "Configuration file for this scenario", false, "scenario.xml","string", cmd);

    // world
    worldZSizeParam = new TCLAP::ValueArg<long>("","worldZSize","Z size of the world",false,0,"long", cmd);
    worldYSizeParam = new TCLAP::ValueArg<long>("","worldYSize","Y size of the world",false,0,"long", cmd);
    worldXSizeParam = new TCLAP::ValueArg<long>("","worldXSize","X size of the world",false,0,"long", cmd);

    // log system
    outputBaseNameParam = new TCLAP::ValueArg<string>("","outputBaseName","Base name for output files",false,"","string", cmd);

    // scenario parameters
    scenarioNameParam = new TCLAP::ValueArg<std::string>("","scenarioName", "Scenario name", false,"","string");
    scenarioDescriptionParam = new TCLAP::ValueArg<std::string>("","scenarioDescription", "Scenario description", false,"","string");

    // BitSimulator-only options
    if (program == 0) {
      cmd.add(graphicModeParam);
      cmd.add(scenarioNameParam);
      cmd.add(scenarioDescriptionParam);

      nodeStartupTimeParam = new TCLAP::ValueArg<long>("","nodeStartupTime","Node startup date (in fs)",false,0,"long", cmd);

      //nodes
      backoffRNGSeedParam = new TCLAP::ValueArg<int>("","backoffRNGSeed","RNG seed for backoff",false,0,"int", cmd);
      defaultBackoffWindowWidthParam = new TCLAP::ValueArg<int>("","defaultBackoff","Default backoff window width",false,100000,"int", cmd);
      //         defaultBetaParam = new TCLAP::ValueArg<long>("","defaultBeta","default beta",false,1000,"long", cmd);
      defaultBetaParam = new TCLAP::ValueArg<int>("","defaultBeta","Default beta",false,1000,"long", cmd);

      genericNodesNumberParam = new TCLAP::ValueArg<int>("","genericNodesCount","Number of generic randomly positioned nodes",false,0,"int", cmd);
      genericNodesRNGSeedParam = new TCLAP::ValueArg<int>("","genericNodesRNGSeed","Seed used for randomly positioning generic nodes",false,0,"int", cmd);
      cmd.add(acceptCollisionedPacketsParam);
      cmd.add(doNotUseNeighboursListParam);

      nodePositionNoiseParam = new TCLAP::ValueArg<distance_t>("","nodePositionNoise","Noise added to x, y, and z coordinates of manually positioned nodes, thus avoiding a perfect grid for example",false,0,"int", cmd);

      // nanowireless
      binaryPayloadRNGSeedParam = new TCLAP::ValueArg<int>("","binaryPayloadRNGSeed", "RNG seed used for binary payloads", false,0,"int", cmd);

      // routing
      routingAgentNameParam = new TCLAP::ValueArg<string>("","routingAgent","Routing agent name",false,"","string", cmd);

      // D1DensityEstimator
      D1GrowRateParam = new TCLAP::ValueArg<float>("","D1GrowRate","Grow rate for D1DensityEstimator",false,2.0,"float", cmd);
      D1ErrorMaxParam = new TCLAP::ValueArg<float>("","D1ErrorMax","e^max for D1DensityEstimator",false,10.0,"float", cmd);
      D1MaxTicParam = new TCLAP::ValueArg<int>("","D1MaxTic","Number of tics per round in for D1DensityEstimator",false,1,"int", cmd);
      D1MaxRoundParam = new TCLAP::ValueArg<int>("","D1MaxRound","Maximum number of rounds for D1DensityEstimator",false,20,"int", cmd);
      D1RemainingFreeRoundsParam = new TCLAP::ValueArg<int>("","D1RemainingFreeRounds","Number of rounds to participate in after getting an estimation",false,1,"int", cmd);
      D1MinPacketsForTerminationParam = new TCLAP::ValueArg<int>("","D1MinPacketsForTermination","Minimum number of packets to stop D1DensityEstimator",false,100,"int", cmd);
      D1DoNotSimulateInitFloodParam = new TCLAP::SwitchArg("","D1DoNotSimulateFlood","D1 magically receive INIT packet at simulation start", cmd, false);

      //SLR
      slrPathWidthParam = new TCLAP::ValueArg<int>("","slrPathWidth","Width of the SLR path (m)",false,1,"int", cmd);

      //HCD
      hcdPathWidthParam = new TCLAP::ValueArg<int>("","hcdPathWidth","Width of the HCD path (m)",false,1,"int", cmd);

      //slrBackoff
      slrBackoffRedundancyParam = new TCLAP::ValueArg<int>("","slrBackoffRedundancy","Redundancy for SLR Backoff",false,1,"int", cmd);
      slrBeaconBackoffRedundancyParam = new TCLAP::ValueArg<int>("","slrBeaconBackoffRedundancy","Redundancy of Beacons for SLR Backoff ",false,40,"int", cmd);
      slrBackoffBeaconMultiplierParam = new TCLAP::ValueArg<float>("","slrBackoffBeaconMultiplier","Multiplier for the backoff window of SLR beacons",false,1.0,"float", cmd);
      slrBackoffMultiplierParam = new TCLAP::ValueArg<float>("","slrBackoffMultiplier","Multiplier for the backoff window",false,1,"float", cmd);

      backoffFloodingRNGSeedParam = new TCLAP::ValueArg<int>("","backoffFloodingRNGSeed","RNG seed for backoff flooding RAD",false,0,"int", cmd);

      // log system
      logAtNodeLevelParam = new TCLAP::SwitchArg("","disableLogsAtNodeLevel","Disable node level logs", cmd, true);
      logAtRoutingLevelParam = new TCLAP::SwitchArg("","disableLogsAtRoutingLevel","Disable routing agent level logs -- NOT IMPLEMENTED", cmd, true);

      // sleep system
      sleepRNGSeedParam = new TCLAP::ValueArg<int>("","sleepRNGSeed","RNG seed for the sleeping system",false,0,"int", cmd);
      awakenNodesParam = new TCLAP::ValueArg<int>("","awakenNodes","Make all nodes sleep to match the given average number of awake neighbours; enables --deden",false,0,"int", cmd);
      awakenDurationParam = new TCLAP::ValueArg<int>("","awakenDuration","Make all nodes sleep to match the given awaken time per Ts; enables --deden",false,100000,"int", cmd);

      //activate DEDeN
      dedenParam = new TCLAP::SwitchArg("","deden","Enable neighbours estimation using DEDeN", cmd, false);
      dedenRNGSeedParam = new TCLAP::ValueArg<int>("","dedenRNGSeed","RNG seed for deden",false,0,"int", cmd);

    } else {  // VisualTracer-only options
      cmd.add(chronoParam);
      cmd.add(nodeZoomParam);
      cmd.add(complementaryNodeFloatInfoFileNameParam);
      cmd.add(othersNodesToDisplayParam);
      cmd.add(precomputeStepsParam);
      cmd.add(stepLengthParam);
      cmd.add(initialTimeSkipParam);
    }

    // ##COMMANDLINE NEW OPTIONS
    cmd.parse( argc, argv );

    scenarioDirectory = scenarioDirectoryParam.getValue();
    configurationFileName = scenarioFileParam.getValue();

    if (program == 0) {
      graphicMode = graphicModeParam.getValue();
      acceptCollisionedPackets = acceptCollisionedPacketsParam.getValue();
      doNotUseNeighboursList = doNotUseNeighboursListParam.getValue();

      nodePositionNoise = nodePositionNoiseParam->getValue();
      slrBackoffRedundancy = slrBackoffRedundancyParam->getValue();
      slrBeaconBackoffRedundancy =slrBeaconBackoffRedundancyParam->getValue();
      slrBackoffMultiplier = slrBackoffMultiplierParam->getValue();
      slrBackoffBeaconMultiplier=slrBackoffBeaconMultiplierParam->getValue();
      backoffFloodingRNGSeed = backoffFloodingRNGSeedParam->getValue();
      slrPathWidth = slrPathWidthParam->getValue();

      hcdPathWidth = hcdPathWidthParam->getValue();

      logAtNodeLevel = logAtNodeLevelParam->getValue();
      logAtRoutingLevel = logAtRoutingLevelParam->getValue();

      awakenDuration = awakenDurationParam->getValue();
      awakenNodes = awakenNodesParam->getValue();
      sleepRNGSeed = sleepRNGSeedParam->getValue();
      sleepIsEnabled = awakenDurationParam->isSet() || awakenNodesParam->isSet();

      D1GrowRate = D1GrowRateParam->getValue();
      D1ErrorMax = D1ErrorMaxParam->getValue();
      D1MaxTic = D1MaxTicParam->getValue();
      D1MaxRound = D1MaxRoundParam->getValue();
      D1RemainingFreeRounds = D1RemainingFreeRoundsParam->getValue();
      D1MinPacketsForTermination = D1MinPacketsForTerminationParam->getValue();
      D1DoNotSimulateInitFlood = D1DoNotSimulateInitFloodParam->getValue();

      dedenIsEnabled = dedenParam->getValue();
      if (awakenNodesParam->isSet() || awakenDurationParam->isSet())
        dedenIsEnabled = true;
      dedenRNGSeed= dedenRNGSeedParam->getValue();
    } else {
      stepDuration = stepLengthParam.getValue();
      initialTimeSkip = initialTimeSkipParam.getValue();
      chronoNodeId = chronoParam.getValue();
      nodeZoom = nodeZoomParam.getValue();
      complementaryNodeFloatInfoFileName = complementaryNodeFloatInfoFileNameParam.getValue();
      othersNodesToDisplayVector = othersNodesToDisplayParam.getValue();
      precomputeSteps = precomputeStepsParam.getValue();
    }
  } catch (TCLAP::ArgException &e) {
    cerr << "*** ERROR *** " << e.error() << " for arg " << e.argId() << std::endl;
  }

  configurationFileCompleteName = scenarioDirectory;
  if (configurationFileCompleteName.back() != '/')
    configurationFileCompleteName += "/";
  configurationFileCompleteName += configurationFileName;
  xmlLoadReturnCode = doc.LoadFile(configurationFileCompleteName.c_str());
  if (xmlLoadReturnCode != 0) {
    cout << "*** ERROR ***, could not find, open or correctly parse the scenario file: " << configurationFileCompleteName << endl;
#if TINYXML2_MAJOR_VERSION >= 6
    cout << "*** ERROR *** Error: " << doc.ErrorStr() << "\n";
#endif
    exit(EXIT_FAILURE);
  }

  XMLRootNode = doc.FirstChildElement("scenario");
  if (XMLRootNode == nullptr) {
    cerr << "*** ERROR *** root node not found in " << configurationFileCompleteName << endl;
    exit(EXIT_FAILURE);
  }

  queryStringElement(XMLRootNode, "name", scenarioName, false, scenarioNameParam, "no \"name\" element in " + configurationFileCompleteName);
  queryStringElement(XMLRootNode, "description", scenarioDescription, false, scenarioDescriptionParam, "no \"description\" element in " + configurationFileCompleteName);
  auto applicationAgentsNode = XMLRootNode->FirstChildElement("applicationAgentsConfig");
  if (applicationAgentsNode != nullptr) {
    auto dedenConfig = applicationAgentsNode->FirstChildElement("DEDeN");
    if (dedenConfig != nullptr && dedenParam != nullptr) {
      std::string enabled;
      queryStringAttr(dedenConfig, "enabled", enabled, false, nullptr,
        "false", "");

      if (dedenParam == nullptr || !dedenParam->isSet())
        dedenIsEnabled = enabled == "true";
    }
  }

  //tinyxml2::XMLElement *worldElement = rootNode->FirstChildElement("world");
  parseWorldElement(XMLRootNode->FirstChildElement("world"));

  parseRoutingElement(XMLRootNode);

  parseLogSystemElement(XMLRootNode);

  tinyxml2::XMLElement *nanoWirelessElement = XMLRootNode->FirstChildElement("nanoWireless");
  if (nanoWirelessElement == nullptr) {
    cerr << "*** ERROR *** \"nanoWirelessElement\" element not found in " << configurationFileCompleteName << endl;
    exit(EXIT_FAILURE);
  }

  //     backoffRNGSeed = queryIntAttribute(nanoWirelessElement, "backoffRNGSeed");
  //     defaultBackoffWindowWidth = queryIntAttribute(nanoWirelessElement, "defaultBackoffWindowWidth");
  //     defaultBeta = (long)queryIntAttribute(nanoWirelessElement, "defaultBeta");

  queryIntAttr(nanoWirelessElement, "backoffRNGSeed",backoffRNGSeed,false,backoffRNGSeedParam,0,"no \"backoffRNGSeed\" attribute in \"nanoWireless\" element in "+configurationFileCompleteName);
  queryIntAttr(nanoWirelessElement, "defaultBackoffWindowWidth",defaultBackoffWindowWidth,false,defaultBackoffWindowWidthParam,100000,"no \"defaultBackoffWindowWidth\" attribute in \"nanoWireless\" element in "+configurationFileCompleteName);

  queryIntAttr(nanoWirelessElement, "defaultBeta",defaultBeta,false,defaultBetaParam,1000,"no \"defaultBeta\" attribute in \"nanoWireless\" element in "+configurationFileCompleteName);

  //       static bool queryIntAttr(tinyxml2::XMLElement *_element, string _attributeName, int &_result, bool _required, TCLAP::ValueArg<int> *_TCLAPParam, int _defaultValue, string _message);

  pulseDuration = queryLongAttribute(nanoWirelessElement,"pulseDuration_fs", "pulseDuration_us");
  communicationRange = queryLongAttribute(nanoWirelessElement,"communicationRange_nm", "communicationRange_mm");
  communicationRangeSmall = queryLongAttribute(nanoWirelessElement,"communicationRangeSmall_nm", "communicationRangeSmall_mm");
  queryLongAttr(nanoWirelessElement,"communicationRangeStandardDeviation_nm","communicationRangeStandardDeviation_mm", communicationRangeStandardDeviation, false, NULL, 0, "problem with communication range standard devitation parameter");
  maxConcurrentReceptions = queryIntAttribute(nanoWirelessElement, "maxConcurrentReceptions");
  minimumIntervalBetweenSends = queryIntAttribute(nanoWirelessElement,"minimumIntervalBetweenSends");
  minimumIntervalBetweenReceiveAndSend = queryIntAttribute(nanoWirelessElement,"minimumIntervalBetweenReceiveAndSend");
  queryIntAttr(nanoWirelessElement, "binaryPayloadRNGSeed", binaryPayloadRNGSeed, false, binaryPayloadRNGSeedParam, 0, "no \"binaryPayloadRNGSeed\" attribute in \"nanoWireless\" element in "+configurationFileCompleteName);

  if (program == 0 && !doNotUseNeighboursList && communicationRangeStandardDeviation != 0) {
    cerr << "*** ERROR *** Incompatible parameters. You must not use neighbours list pre-computation if you intend to use non zero standard deviation of the communication range. Add --doNotUseNeighboursList to your command line" << endl;
    exit(EXIT_FAILURE);
  }
  displayParameters();
}

void ScenarioParameters::initialize(int argc, char **argv, int i) {
  assert (scenarioParameters == nullptr);  // ScenarioParameters::initialize should not be called again
  scenarioParameters = new ScenarioParameters(argc, argv, i);
}

bool ScenarioParameters::queryStringElement(tinyxml2::XMLElement *_parentElement, string _elementName, string &_result, bool _required, TCLAP::ValueArg<std::string> *_TCLAPParam, string _errorMessage) {
  tinyxml2::XMLElement *element = _parentElement->FirstChildElement(_elementName.c_str());

  if ( _TCLAPParam != nullptr && _TCLAPParam->isSet() ) {
    cerr << "*** INFO *** Command line parameter overriding element <" << _elementName << ">" << endl;
    _result = _TCLAPParam->getValue();
    return true;
  }

  if (element != nullptr) {
    if (element->GetText() != nullptr)
      _result = element->GetText();
    else
      _result = "";
    return true;
  } else {
    if (_required) {
      cerr << "*** ERROR *** " << _errorMessage << endl;
      exit(EXIT_FAILURE);
    } else {
      return false;
    }
  }
}

bool ScenarioParameters::queryStringAttr(tinyxml2::XMLElement *_parentElement, string _attributeName, string &_result, bool _required, TCLAP::ValueArg<string> *_TCLAPParam, string _defaultValue, string _message) {
  const char *value;

  if ( _TCLAPParam != nullptr && _TCLAPParam->isSet() ) {
    cerr << "*** INFO *** Command line parameter overriding attribute \"" << _attributeName << "\"" << endl;
    _result = _TCLAPParam->getValue();
    return(true);
  }

  value = _parentElement->Attribute(_attributeName.c_str());
  if (value == NULL) {
    if (_required) {
      cerr << "*** ERROR *** " << _message << endl;
      exit(EXIT_FAILURE);
    } else {
      cerr << "*** WARNING *** " << _message << endl;
      _result = _defaultValue;
      return(false);
    }
  } else {
    _result = string(value);
    return(true);
  }
}

bool ScenarioParameters::queryIntAttr(tinyxml2::XMLElement *_element, string _attributeName, int &_result, bool _required, TCLAP::ValueArg<int> *_TCLAPParam, int _defaultValue, string _message) {
  int xmlQueryReturnCode;
  int returnValue;

  if ( _TCLAPParam != nullptr && _TCLAPParam->isSet() ) {
    cerr << "*** INFO *** Command line parameter overriding attribute \"" << _attributeName << "\"" << endl;
    _result = _TCLAPParam->getValue();
    return(true);
  }

  xmlQueryReturnCode = _element->QueryIntAttribute(_attributeName.c_str(),&returnValue);
  if (xmlQueryReturnCode != 0) {
    if ( _required ) {
      cerr << "*** ERROR ***  does not have a \"" << _attributeName << "\" attribute" << endl;
      exit(EXIT_FAILURE);
    } else {
      cerr << "*** WARNING *** " << _message << endl;
      _result = _defaultValue;
      return(false);
    }
  } else {
    _result = returnValue;
    return(true);
  }
}

bool ScenarioParameters::queryLongAttr(tinyxml2::XMLElement *_element, string _attributeName_nm, string _attributeName_mm, long &_result, bool _required, TCLAP::ValueArg<long> *_TCLAPParam, long _defaultValue, string _errorMessage) {
  long returnValue;
  int value_nm;
  int value_mm;
  int found_nm = _element->QueryIntAttribute(_attributeName_nm.c_str(), &value_nm);
  int found_mm = _element->QueryIntAttribute(_attributeName_mm.c_str(), &value_mm);

  if ( _TCLAPParam != nullptr && _TCLAPParam->isSet() ) {
    cerr << "*** INFO *** Command line parameter overriding attribute \"" << _attributeName_nm << "\" or \"" << _attributeName_mm << "\"" << endl;
    _result = _TCLAPParam->getValue();
    return(true);
  }

  if (found_mm == 0) {
    _result = (long)value_mm * 1000000;
    return(true);
  } else {
    if (found_nm == 0) {
      _result = (long)value_nm;
      return(true);
      returnValue = (long)value_nm;
    } else {
      if (_required) {
        cerr << "*** ERROR *** " << _errorMessage << endl;
        exit(EXIT_FAILURE);
      } else {
        _result = _defaultValue;
        return(false);
      }
    }
  }
  return(returnValue);
}


string ScenarioParameters::queryStringAttribute(tinyxml2::XMLElement *_element, string _attributeName) {
  string returnValue;
  const char *value;
  value = _element->Attribute(_attributeName.c_str());
  if (value == NULL) {
    cerr << "*** ERROR *** does not have a \"" << _attributeName << "\" attribute in " << scenarioDirectory+"/"+configurationFileName << endl;
    exit(EXIT_FAILURE);
  }
  returnValue = value;
  return(returnValue);
}

int ScenarioParameters::queryIntAttribute(tinyxml2::XMLElement *_element, string _elementName) {
  int xmlQueryReturnCode;
  int returnValue;
  xmlQueryReturnCode = _element->QueryIntAttribute(_elementName.c_str(),&returnValue);
  if (xmlQueryReturnCode != 0) {
    cerr << "*** ERROR ***  does not have a \"" << _elementName << "\" attribute in " << scenarioDirectory+"/"+configurationFileName << endl;
    exit(EXIT_FAILURE);
  }
  return(returnValue);
}

long ScenarioParameters::queryLongAttribute(tinyxml2::XMLElement *_element, string _attributeName_nm, string _attributeName_mm) {
  long returnValue;
  int value_nm;
  int value_mm;
  int found_nm = _element->QueryIntAttribute(_attributeName_nm.c_str(), &value_nm);
  int found_mm = _element->QueryIntAttribute(_attributeName_mm.c_str(), &value_mm);
  if (found_mm == 0) {
    returnValue = (long)value_mm * 1000000;
  } else {
    if (found_nm == 0) {
      returnValue = (long)value_nm;
    } else {
      string msg = "*** ERROR *** \"" + _attributeName_nm + "\" or \"" + _attributeName_mm + "\" attribute not found in element \"" + _element->Name() + "\" in " + ScenarioParameters::getScenarioDirectory()+"/"+ScenarioParameters::getConfigurationFileName();
      exit(EXIT_FAILURE);
    }
  }
  return(returnValue);
}

void ScenarioParameters::parseRoutingElement(tinyxml2::XMLElement *_parent) {
  tinyxml2::XMLElement *routingAgElement = _parent->FirstChildElement("routingAgentsConfig");
  if (routingAgElement == nullptr) {
    cerr << "*** ERROR *** \"routingAgentsConfig\" element not found in " << configurationFileCompleteName << endl;
    exit(EXIT_FAILURE);
  }

  tinyxml2::XMLNode *routingElement = routingAgElement->FirstChild();
  if (routingElement == nullptr) {
    cerr << "*** ERROR *** no element found in routingAgentsConfig element in " << configurationFileCompleteName << endl;
    exit(EXIT_FAILURE);
  }

  if ( routingAgentNameParam != nullptr && routingAgentNameParam->isSet() ) {
    cerr << "*** INFO *** Command line parameter overriding element \"agent\"" << endl;
    routingAgentName = routingAgentNameParam->getValue();
  } else
    routingAgentName = routingElement->Value();
}

void ScenarioParameters::parseWorldElement(tinyxml2::XMLElement *_parent) {
  if (_parent == nullptr) {
    cerr << "*** ERROR *** \"world\" element not found in " << configurationFileCompleteName << endl;
    exit(EXIT_FAILURE);
  }

  queryLongAttr(_parent, "sizeX_nm", "sizeX_mm", worldXSize, true, worldXSizeParam, 0, "no sizeX_nm nor sizeX_mm attribute in \"world\" element in "+configurationFileCompleteName);
  queryLongAttr(_parent, "sizeY_nm", "sizeY_mm", worldYSize, true, worldYSizeParam, 0, "no sizeY_nm nor sizeY_mm attribute in \"world\" element in "+configurationFileCompleteName);
  queryLongAttr(_parent, "sizeZ_nm", "sizeZ_mm", worldZSize, true, worldZSizeParam, 0, "no sizeZ_nm nor sizeZ_mm attribute in \"world\" element in "+configurationFileCompleteName);

  queryLongAttr(_parent, "nodeStartupTime_fs", "nodeStartupTime_ns", nodeStartupTime, false, nodeStartupTimeParam, 1000000000, "");

  tinyxml2::XMLElement *genericNodesElement = _parent->FirstChildElement("genericNodes");
  if (genericNodesElement != nullptr) {
    queryIntAttr(genericNodesElement, "count", genericNodesNumber, false, genericNodesNumberParam, 0, "no \"count\" attribute in \"world\" element in "+configurationFileCompleteName);
    queryIntAttr(genericNodesElement, "positionRNGSeed", genericNodesRNGSeed, false, genericNodesRNGSeedParam, 0, "no \"positionRNGSeed\" attribute in \"world\" element in "+configurationFileCompleteName);
  } else {
    genericNodesNumber = 0;
    genericNodesRNGSeed = 0;
  }

  tinyxml2::XMLElement *node;
  int nodeId;
  distance_t nodeX, nodeY, nodeZ;
  bool isAnchor;
  simulationTime_t beaconStartTime=0;

  node = _parent->FirstChildElement("node");
  while (node != nullptr ) {
    nodeId = queryIntAttribute(node, "id");
    nodeX = queryLongAttribute(node, "posX_nm", "posX_mm");
    nodeY = queryLongAttribute(node, "posY_nm", "posY_mm");
    nodeZ = queryLongAttribute(node, "posZ_nm", "posZ_mm");

    isAnchor = false;
    node->QueryBoolAttribute("anchor", &isAnchor);
    if (isAnchor) {
      beaconStartTime = queryLongAttribute(node, "beaconStartTime_fs", "beaconStartTime_us");
    }
    vectNodeInfo.push_back(NodeInfo(nodeId, nodeX, nodeY, nodeZ, isAnchor, beaconStartTime));

    node = node->NextSiblingElement("node");
  }

  rootNodesArea = new NodesArea();

  rootNodesArea->addAreaFromXMLElement( _parent );
  rootNodesArea->print("");
}

void ScenarioParameters::parseLogSystemElement(tinyxml2::XMLElement *_parent) {
  tinyxml2::XMLElement *logSystemElement = _parent->FirstChildElement("logSystem");
  if (logSystemElement == nullptr) {
    cerr << "*** ERROR *** \"logSystem\" element not found in " << configurationFileCompleteName << endl;
    exit(EXIT_FAILURE);
  }

  queryStringAttr(logSystemElement, "baseName", outputBaseName, false, outputBaseNameParam, "", "no \"baseName\" attribute in \"logSystem\" element, using default");

  tinyxml2::XMLElement *node;
  node = logSystemElement->FirstChildElement();
  string outputFileName;
  while (node != nullptr) {
    //outputFileName = queryStringAttribute(node,"output");
    string output, suffix, io;
    queryStringAttr(node, "output", output, true, nullptr, "", "no \"output\" attribute in child of <logSystem> element");
    queryStringAttr(node, "suffix", suffix, false, nullptr, "", "no \"suffix\" attribute in child of <logSystem> element using none");
    queryStringAttr(node, "io", io, false, nullptr, "stream", "no \"io\" attribute in child of <logSystem> element");
    mapOutputFiles.insert(pair<string,logSystemInfo_t>(node->Name(), {output, suffix, io}));
    node = node->NextSiblingElement();
  }
}

void ScenarioParameters::displayParameters() {

  cout << "\033[36;1m================================================\033[0m" << endl;

  cout << "\033[36;1mScenario: \033[0m" << scenarioName << endl;
  cout << scenarioDescription << endl;

  cout << "\033[36;1mScenario parameters:\033[0m" << endl;
  cout << "  directory:            " << scenarioDirectory << endl;
  cout << "  configuration file:   " << configurationFileCompleteName << endl;
  cout << "  log system base name: " << outputBaseName << endl;
  cout << "  world X size:         " << worldXSize << " nm" << endl;
  cout << "  world Y size:         " << worldYSize << " nm" << endl;
  cout << "  world Z size:         " << worldZSize << " nm" << endl;
  cout << endl;

  cout << "\033[36;1mNodes:\033[0m" << endl;
  cout << "  startup time:         " << nodeStartupTime << " fs" << endl;
  cout << endl;

  cout << "\033[36;1mNanoWireless:\033[0m" << endl;
  cout << "  communication range:                    " << communicationRange << " nm" << endl;
  cout << "  communication range (small):            " << communicationRangeSmall << " nm" << endl;
  cout << "  communication range standard deviation: " << communicationRangeStandardDeviation << " nm" << endl;
  cout << "  default beta:                           " << defaultBeta << endl;
  cout << "  pulse duration:                         " << pulseDuration << " fs" << endl;
  cout << "  backoff RNG seed:                       " << backoffRNGSeed << endl;
  cout << "  backoff window width:                   " << defaultBackoffWindowWidth << endl;
  cout << "  maximum concurrent receptions:          " << maxConcurrentReceptions << endl;
  cout << "  binary payload RNG seed:                " << binaryPayloadRNGSeed << endl;
  cout << endl;

  cout << "\033[36;1mRouting agent: \033[0m" << endl;
  cout << "  " << routingAgentName << endl;
  cout << endl;

  cout << "\033[36;1mAnchors: \033[0m" << endl;
  for (auto it=vectNodeInfo.begin(); it != vectNodeInfo.end(); it++) {
    if (it->isAnchor) {
      cout << "  id " << it->id << " (" << it->posX << ", " << it->posY << ", " << it->posZ << ")" << endl;
    }
  }
  cout << endl;
  cout << "\033[36;1mRandom generic nodes:\033[0m" << endl;
  cout << "   number of auto-generated nodes:   " << genericNodesNumber << endl;
  cout << "   RNG seed used for positions:      " << genericNodesRNGSeed << endl;
  cout << endl;

  cout << "\033[36;1mD1DensityEstimator:\033[0m" << endl;
  cout << "   grow rate:                        " << D1GrowRate << endl;
  cout << "   number of tics per round:         " << D1MaxTic << endl;
  cout << "   maximum number of rounds:         " << D1MaxRound << endl;
  cout << "   minimum packets for termination:  " << D1MinPacketsForTermination << endl;

  cout << "\033[36;1m================================================\033[0m" << endl;
}


//==============================================================================
//
//          Various functions for SLR
//
//==============================================================================


int DeltaA(int dstX, int dstY, int /*dstZ*/,int srcX,int srcY,int /*srcZ*/,int X,int Y, int /*Z*/ ){
  return (X-srcX)*(dstY-srcY)-(Y-srcY)*(dstX-srcX);
}


int DeltaB(int dstX, int /*dstY*/, int dstZ,int srcX,int /*srcY*/,int srcZ,int X,int /*Y*/, int Z ){
  return (X-srcX)*(dstZ-srcZ)-(Z-srcZ)*(dstX-srcX);
}

//==============================================================================
//
//          NodesArea  (class)
//
//==============================================================================

NodesArea::NodesArea() {
    this->parent = nullptr;

    this->shape = NodesAreaShape::UNKNOWN;
    this->distribution = NodesAreaDistribution::UNKNOWN;

    this->x = 0;
    this->y = 0;
    this->z = 0;

    this->localX = 0;
    this->localY = 0;
    this->localZ = 0;

    this->sizeX = 0;
    this->sizeY = 0;
    this->sizeZ = 0;

    this->nodesCount = 0;
    this->positionRNGSeed = 0;
}

void NodesArea::setShapeRectangle( distance_t _x, distance_t _y, distance_t _z, distance_t _sizeX, distance_t _sizeY, distance_t _sizeZ ) {
    this->shape = NodesAreaShape::RECTANGLE;
    this->x = _x;
    this->y = _y;
    this->z = _z;
    this->sizeX = _sizeX;
    this->sizeY = _sizeY;
    this->sizeZ = _sizeZ;
}

void NodesArea::addAreaFromXMLElement( tinyxml2::XMLElement *_xmlElement ) {
    NodesArea *newArea = this;
    const char* distrib = nullptr;

    distance_t meanX=0;
    distance_t meanY=0;
    distance_t meanZ=0;
    distance_t deviationX = 0;
    distance_t deviationY = 0;
    distance_t deviationZ = 0;

    if ( strcmp( _xmlElement->Name(), "world") == 0) {
        //
        // If the XMLElement is the World itself, we just update the current NodesArea
        //
        this->shape = NodesAreaShape::RECTANGLE;
        this->x = 0;
        this->y = 0;
        this->z = 0;
        this->localX = 0;
        this->localY = 0;
        this->localZ = 0;
        this->sizeX = ScenarioParameters::getWorldXSize();
        this->sizeY = ScenarioParameters::getWorldYSize();
        this->sizeZ = ScenarioParameters::getWorldZSize();

        this->distribution = NodesAreaDistribution::UNIFORM;
        this->nodesCount = ScenarioParameters::getGenericNodesNumber();
        this->positionRNGSeed = ScenarioParameters::getGenericNodesRNGSeed();

        cout << "add world ..." << endl;
    } else {
        //
        // If the XMLElement is an <area> node, we create a new NodesArea and add it to the children of the current NodesArea
        //
        newArea = new NodesArea();

        newArea->parent = this;

        if (_xmlElement->Attribute("shape", "rectangle")) newArea->shape = NodesAreaShape::RECTANGLE;
        if (_xmlElement->Attribute("shape", "rectangleHole")) newArea->shape = NodesAreaShape::RECTANGLE_HOLE;
        if (_xmlElement->Attribute("shape", "ellipse")) newArea->shape = NodesAreaShape::ELLIPSE;

        if (newArea->shape == NodesAreaShape::UNKNOWN && _xmlElement->Attribute("shape") != NULL) {
            cerr << "*** ERROR: invalid shape " << endl;
            exit(EXIT_FAILURE);
        }

        switch ( newArea->shape ) {
            case NodesAreaShape::RECTANGLE:
            case NodesAreaShape::RECTANGLE_HOLE:
            case NodesAreaShape::ELLIPSE:
                newArea->localX = ScenarioParameters::queryLongAttribute( _xmlElement, "x_nm", "x_mm");
                newArea->localY = ScenarioParameters::queryLongAttribute( _xmlElement, "y_nm", "y_mm");
                newArea->localZ = ScenarioParameters::queryLongAttribute( _xmlElement, "z_nm", "z_mm");
                newArea->sizeX = ScenarioParameters::queryLongAttribute( _xmlElement, "sizeX_nm", "sizeX_mm");
                newArea->sizeY = ScenarioParameters::queryLongAttribute( _xmlElement, "sizeY_nm", "sizeY_mm");
                newArea->sizeZ = ScenarioParameters::queryLongAttribute( _xmlElement, "sizeZ_nm", "sizeZ_mm");

                newArea->x = this->x + newArea->localX;
                newArea->y = this->y + newArea->localY;
                newArea->z = this->z + newArea->localZ;
                break;
            default:
                cout << "  no shape, inheriting from parent." << endl;
                newArea->shape = this->shape;
                newArea->x = this->x;
                newArea->y = this->y;
                newArea->z = this->z;
                newArea->localX = 0;
                newArea->localY = 0;
                newArea->localZ = 0;
                newArea->sizeX = this->sizeX;
                newArea->sizeY = this->sizeY;
                newArea->sizeZ = this->sizeZ;
        }

#if (TINYXML2_MAJOR_VERSION == 6 && TINYXML2_MINOR_VERSION >= 2) || TINYXML2_MAJOR_VERSION >= 7
        bool foundDistribution = _xmlElement->QueryStringAttribute( "distribution", &distrib) == tinyxml2::XML_SUCCESS;
#else
        // QueryStringAttribute appeared in 6.2.0 (released 06/04/2018)
        // cf. https://github.com/leethomason/tinyxml2/commit/5b00e066
        // compared to the #if, this code does not allow empty attribute,
        // but it is only a temporary hack to compile with old tinyxml2
        if (_xmlElement->Attribute("distribution", "uniform"))
          distrib = "uniform";
        else if (_xmlElement->Attribute("distribution", "normal"))
          distrib = "normal";
        else
          distrib = "unknown";
        bool foundDistribution = true;
#endif
        bool foundNodesCount = false;

        if ( foundDistribution ) {
            if ( strcmp( distrib, "uniform") == 0 ) {
                newArea->distribution = NodesAreaDistribution::UNIFORM;
            } else if ( strcmp( distrib, "normal") == 0 ) {
                newArea->distribution = NodesAreaDistribution::NORMAL;
            } else {
                newArea->distribution = NodesAreaDistribution::UNKNOWN;
#if (TINYXML2_MAJOR_VERSION == 6 && TINYXML2_MINOR_VERSION < 2) || TINYXML2_MAJOR_VERSION < 6
                if (newArea->shape != NodesAreaShape::RECTANGLE_HOLE) {
#endif
                  cerr << "*** ERROR: unknown distribution " << distrib << endl;
                  exit(EXIT_FAILURE);
#if (TINYXML2_MAJOR_VERSION == 6 && TINYXML2_MINOR_VERSION < 2) || TINYXML2_MAJOR_VERSION < 6
                }
#endif
            }

            cout << "Distribution: " << distrib << endl;

            //
            // Handle attributes specific to the current distribution
            //
            if ( newArea->shape != NodesAreaShape::RECTANGLE_HOLE && (newArea->distribution == NodesAreaDistribution::UNIFORM || newArea->distribution == NodesAreaDistribution::NORMAL) ) {
                int nodesCount = 0;
                foundNodesCount = _xmlElement->QueryIntAttribute( "nodesCount", &nodesCount ) == tinyxml2::XML_SUCCESS;

                if ( foundNodesCount) {
                    cout << "-- " << distrib << " distribution -- nodesCount:" << nodesCount << endl;
                    newArea->nodesCount = nodesCount;
                } else {
                    cerr << "*** ERROR: a nodes count must be specified when using " << distrib << " distribution" << endl;
                    exit( EXIT_FAILURE );
                }
            }

            if ( newArea->distribution == NodesAreaDistribution::NORMAL ) {
                meanX = ScenarioParameters::queryLongAttribute( _xmlElement, "meanX_nm", "meanX_mm");
                meanY = ScenarioParameters::queryLongAttribute( _xmlElement, "meanY_nm", "meanY_mm");
                meanZ = ScenarioParameters::queryLongAttribute( _xmlElement, "meanZ_nm", "meanZ_mm");
                deviationX = ScenarioParameters::queryLongAttribute( _xmlElement, "deviationX_nm", "deviationX_mm");
                deviationY = ScenarioParameters::queryLongAttribute( _xmlElement, "deviationY_nm", "deviationY_mm");
                deviationZ = ScenarioParameters::queryLongAttribute( _xmlElement, "deviationZ_nm", "deviationZ_mm");
            }
        } else {
            cout << "NO DISTRIBUTION FOUND" << endl;
            newArea->distribution = NodesAreaDistribution::NONE;
        }

        //
        // if not explicitely defined for this NodesArea, the positionRNGSeed is inherited from the parent
        //
        int positionRNGSeed;
        if (_xmlElement->QueryIntAttribute( "positionRNGSeed", &positionRNGSeed ) == tinyxml2::XML_SUCCESS) {
            newArea->positionRNGSeed = positionRNGSeed;
        } else {
            newArea->positionRNGSeed = this->positionRNGSeed;
        }

        cout << "adding subarea ..." << endl;
        this->children.push_back( newArea );
    }

    //
    // We generate the nodes in this NodesArea
    //
    // When generating nodes, be sure to not add nodes outside the current shape

    newArea->positionsRandomGenerator = new mt19937_64(newArea->positionRNGSeed);

    uniform_int_distribution<distance_t> uniformXDistribution(0, newArea->sizeX);
    uniform_int_distribution<distance_t> uniformYDistribution(0, newArea->sizeY);
    uniform_int_distribution<distance_t> uniformZDistribution(0, newArea->sizeZ);

    normal_distribution<double> normalXDistribution(meanX, deviationX);
    normal_distribution<double> normalYDistribution(meanY, deviationY);
    normal_distribution<double> normalZDistribution(meanZ, deviationZ);

    distance_t xx,yy,zz;
    double rx = newArea->sizeX/2;
    double ry = newArea->sizeY/2;
    double rz = newArea->sizeZ/2;

    double belongsToEllipse;

    switch ( newArea->shape ) {
        case NodesAreaShape::RECTANGLE:

            switch ( newArea->distribution ) {
                case NodesAreaDistribution::UNIFORM:
                    int i;
                    for (i=0; i<newArea->nodesCount; i++) {
                        xx = uniformXDistribution(*newArea->positionsRandomGenerator);
                        yy = uniformYDistribution(*newArea->positionsRandomGenerator);
                        zz = uniformZDistribution(*newArea->positionsRandomGenerator);
                        newArea->nodes.push_back(NodePosition(xx,yy,zz));
                    }
                    break;
                case NodesAreaDistribution::NORMAL:
                    for (i=0; i<newArea->nodesCount; i++) {
                        do {
                            xx = normalXDistribution(*newArea->positionsRandomGenerator);
                            yy = normalYDistribution(*newArea->positionsRandomGenerator);
                            zz = normalZDistribution(*newArea->positionsRandomGenerator);
                        } while ( xx < 0 || xx > newArea->sizeX || yy < 0 || yy > newArea->sizeY || zz < 0 || zz > newArea->sizeZ );
                        newArea->nodes.push_back(NodePosition(xx,yy,zz));
                    }

                    break;
                default:
                    break;
            }
            break;
        case NodesAreaShape::ELLIPSE:
            switch ( newArea->distribution ) {
                case NodesAreaDistribution::UNIFORM:
                    int i;
                    for (i=0; i<newArea->nodesCount; i++) {
                        do {
                            xx = uniformXDistribution(*newArea->positionsRandomGenerator) - newArea->sizeX/2;
                            yy = uniformYDistribution(*newArea->positionsRandomGenerator) - newArea->sizeY/2;
                            zz = uniformZDistribution(*newArea->positionsRandomGenerator) - newArea->sizeZ/2;

                            belongsToEllipse = 0;
                            if ( rx != 0 ) belongsToEllipse += (xx*xx)/(rx*rx);
                            if ( ry != 0 ) belongsToEllipse += (yy*yy)/(ry*ry);
                            if ( rz != 0 ) belongsToEllipse += (zz*zz)/(rz*rz);
                        } while ( belongsToEllipse > 1);
                        newArea->nodes.push_back(NodePosition(xx,yy,zz));
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }

    //
    // We look for <area> children in the XMLElement and recursively add them to the NodesArea tree
    //
    tinyxml2::XMLElement *xmlChild;
    xmlChild = _xmlElement->FirstChildElement("area");
    while ( xmlChild != nullptr ) {
        newArea->addAreaFromXMLElement( xmlChild );

        xmlChild = xmlChild->NextSiblingElement("area");
    }
}

void NodesArea::print( string _shift) {
    vector<NodesArea*>::iterator it;

    switch ( this->shape ) {
        case NodesAreaShape::RECTANGLE:
            cout << _shift << "- RECTANGLE global(" << this->x << "," << this->y << "," << this->z << ") local(" << localX << "," << localY << "," << localZ << ") size: " << this->sizeX << " x " << this->sizeY << " x " << this->sizeZ << endl;
            break;
        case NodesAreaShape::RECTANGLE_HOLE:
            cout << _shift << "- RECTANGLE_HOLE global(" << this->x << "," << this->y << "," << this->z << ") local(" << localX << "," << localY << "," << localZ << ") size: " << this->sizeX << " x " << this->sizeY << " x " << this->sizeZ << endl;
            break;
        case NodesAreaShape::ELLIPSE:
            cout << _shift << "- ELLIPSE global(" << this->x << "," << this->y << "," << this->z << ") local(" << localX << "," << localY << "," << localZ << ") size: " << this->sizeX << " x " << this->sizeY << " x " << this->sizeZ << endl;
            break;
        default:
            cout << "*** ERROR: unknown NodesAreaShape ***" << endl;
            exit(EXIT_FAILURE);
            break;
    }

    switch ( this->distribution ) {
        case NodesAreaDistribution::UNIFORM:
            cout << _shift << "  UNIFORM distribution. NodesCount: " << this->nodesCount << "  RNGSeed: " << this->positionRNGSeed << endl;
            break;
        default:
            cout << _shift << "  no distribution" << endl;
            break;
    }

    for ( it=this->children.begin(); it != this->children.end(); it++) {
        (*it)->print( _shift + "  " );
    }
}

vector<NodePosition> NodesArea::getNodesPositionsVector() {
    distance_t childX, childY, childZ;
    distance_t nX, nY, nZ;

    double rx = this->sizeX/2;
    double ry = this->sizeY/2;
    double rz = this->sizeZ/2;

    double belongsToEllipse;
    cout << "getNodesPositionsVector **********************";
    if ( this->shape == NodesAreaShape::RECTANGLE ) cout << " RECTANGLE" << endl;
    if ( this->shape == NodesAreaShape::ELLIPSE ) cout << " ELLIPSE" << endl;
    if ( this->shape == NodesAreaShape::RECTANGLE_HOLE ) cout << " RECTANGLE_HOLE" << endl;

    for (auto childIt=this->children.begin(); childIt!=this->children.end(); childIt++) {
        vector<NodePosition> childNodesPositionsVector = (*childIt)->getNodesPositionsVector();
        childX = (*childIt)->localX;
        childY = (*childIt)->localY;
        childZ = (*childIt)->localZ;

        for (auto nodeIt=childNodesPositionsVector.begin(); nodeIt!=childNodesPositionsVector.end(); nodeIt++) {
            nX = nodeIt->x+childX;
            nY = nodeIt->y+childY;
            nZ = nodeIt->z+childZ;

            switch ( this->shape ) {
                case NodesAreaShape::RECTANGLE:
                case NodesAreaShape::RECTANGLE_HOLE:
                    if (nX >= 0 && nX <= this->sizeX && nY >= 0 && nY <= this->sizeY && nZ > 0 && nZ <= this->sizeZ ) {
                        this->nodes.push_back(NodePosition(nX,nY,nZ));
                    }
                    break;
                case NodesAreaShape::ELLIPSE:
                    belongsToEllipse = 0;
                    if ( rx != 0 ) belongsToEllipse += (nX*nX)/(rx*rx);
                    if ( ry != 0 ) belongsToEllipse += (nY*nY)/(ry*ry);
                    if ( rz != 0 ) belongsToEllipse += (nZ*nZ)/(rz*rz);
                    if ( belongsToEllipse < 1) this->nodes.push_back(NodePosition(nX,nY,nZ));
                    break;
                default:
                    break;
            }
        }
    }

    if ( this->shape == NodesAreaShape::RECTANGLE_HOLE ) {
        for (auto it=parent->nodes.begin(); it!=parent->nodes.end(); ) {
            if ( it->x >= x && it->x <= (x+sizeX) && it->y >= y && it->y <= (y+sizeY) && it->z >= z && it->z <= (z+sizeZ) ) {
                it = parent->nodes.erase( it );
            } else {
                it++;
            }
        }
    }

    return this->nodes;
}
