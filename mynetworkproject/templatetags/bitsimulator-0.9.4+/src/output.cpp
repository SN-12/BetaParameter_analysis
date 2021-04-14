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
#include "output.h"
#include <cstdarg>

int OutputLineFormat::nextFormatID = 0;

//===========================================================================================================
//
//          LogOutput  (class)
//
//===========================================================================================================

LogOutput::LogOutput() {
  //cout << "LogOutput constructor" << endl;
  outputFile = NULL;
  buffer = (char*)malloc(4096);
  xmlBuffer = (char*)malloc(10000);
  bzero(xmlBuffer, 10000);
  currentFormat = nullptr;
}

LogOutput::~LogOutput() {
  //cout << "LogOutput destructor" << endl;
  free(buffer);
  free(xmlBuffer);
}

void LogOutput::create( string fileName ) {
  outputFile = fopen( fileName.c_str(), "w" );
}

void LogOutput::open( string fileName ) {
  outputFile = fopen( fileName.c_str(), "r" );
}

void LogOutput::close() {
  fclose( outputFile );
}

LogOutput::LineType LogOutput::readNextLine() {
  size_t linecap = 4096;
  ssize_t linelen;

  static int xmlBufferPos = 0;

  //format
  int formatId;
  string formatKey;
  string formatDescription;

  // item
  const char* itemTypeString;
  const char* itemKey;
  const char* itemDescription;

  LogItem::ItemType itemType;

  tinyxml2::XMLElement *elem;

  if ( outputFile == NULL ) {
    cout << "Fail to open" << endl;
    return LineType::END_OF_FILE;
  }

  if ( (linelen = getline(&buffer, &linecap, outputFile)) > 0 ) {
    if (buffer[0] == '#') {
      memcpy( xmlBuffer+xmlBufferPos, buffer+1, linelen-1);
      xmlBufferPos += (linelen-1);
      currentFormat = nullptr;
      return LineType::COMMENT;

    } else {
      if ( xmlBufferPos > 0 ) {
        //printf("xmlbuffer: %s\n", xmlBuffer);

        xmlDoc.Parse(xmlBuffer);
        XMLRootNode = xmlDoc.FirstChildElement( "lineFormat" );
        if ( XMLRootNode != nullptr ) {
          XMLRootNode->QueryIntAttribute( "id", &formatId);
          formatKey = string(XMLRootNode->Attribute( "key" ));
          formatDescription = string(XMLRootNode->Attribute( "description" ));
          ////cout << "FormatId : " << formatId << endl;
          ////cout << "FormatKey : " << formatKey << endl;
          ////cout << "Description : " << formatDescription << endl;

          elem = XMLRootNode->FirstChildElement( "item" );

          OutputLineFormat *format;
          map<int, OutputLineFormat*>::iterator it;
          it = mapKnownFormats.find( formatId );
          if ( it == mapKnownFormats.end() ) {
            format = new OutputLineFormat( formatId, formatKey, formatDescription );
            mapKnownFormats.insert( pair<int, OutputLineFormat*>(formatId, format) );
          } else {
            cerr << "*** ERROR: format id " << formatId << " already seen" << endl;
            exit(EXIT_FAILURE);
          }
          currentFormat = format;

          while ( elem != nullptr ) {
            itemTypeString = elem->Attribute( "type" );
            itemKey = elem->Attribute( "key" );
            itemDescription = elem->GetText();

            ////cout << "  item" << endl;
            ////cout << "    type        : " << itemTypeString << endl;
            ////cout << "    key         : " << itemKey << endl;
            ////cout << "    description : " << itemDescription << endl;

            if ( strcmp( itemTypeString,"Integer 32" ) == 0 ) {
              itemType = LogItem::ItemType::INT32;
            } else if ( strcmp( itemTypeString,"Integer 64" ) == 0 ) {
              itemType = LogItem::ItemType::INT64;
            } else if ( strcmp( itemTypeString,"String" ) == 0 ) {
              itemType = LogItem::ItemType::STRING;
            } else if ( strcmp( itemTypeString,"Double" ) == 0 ) {
              itemType = LogItem::ItemType::DOUBLE;
            } else if ( strcmp( itemTypeString,"Boolean" ) == 0 ) {
              itemType = LogItem::ItemType::BOOLEAN;
            } else {
              cerr << "*** ERROR: Unknown data type \"" + string(itemTypeString) + "\"" << endl;
              exit(EXIT_FAILURE);
            }

            format->addItem( itemType, itemKey, itemDescription );

            elem = elem->NextSiblingElement( "item" );

          }

        } else {
          cout << "*** Error while reading XMLRootNode ***" << endl;
        }

        xmlBufferPos = 0;
        bzero(xmlBuffer, 10000);
      }
      //             cout << "data line : " << buffer;
      char *stringToken = strtok(buffer," ");
      sscanf(stringToken,"%d",&formatId);


      map<int,OutputLineFormat*>::iterator itFormat;
      itFormat = mapKnownFormats.find(formatId);
      currentFormat = itFormat->second;
      //             cout << " id:" << formatId << " " << itFormat->second->formatKey <<endl;

      if ( itFormat != mapKnownFormats.end() ) {
        vector<LogItem*>::iterator itLogItem;
        itLogItem = itFormat->second->vectorItems.begin();
        while ( itLogItem != itFormat->second->vectorItems.end() ) {
          stringToken = strtok(NULL, " ");
          switch ( (*itLogItem)->type ) {
          case LogItem::ItemType::INT32:
            (*itLogItem)->data.intValue = atoi(stringToken);
            break;
          case LogItem::ItemType::INT64:
            (*itLogItem)->data.longValue = atol(stringToken);
            break;
          case LogItem::ItemType::STRING:
            strncpy( (*itLogItem)->data.stringValue, stringToken, 20-1 );
            break;
          case LogItem::ItemType::DOUBLE:
            (*itLogItem)->data.doubleValue = atof(stringToken);
            break;
          case LogItem::ItemType::BOOLEAN:
            (*itLogItem)->data.booleanValue = ( atoi(stringToken) != 0 );
            break;
          default:
            cerr << "*** ERROR *** Unsupported data type while reading log file" << endl;
            break;
          }
          itLogItem++;
        }
      } else {
        cerr << "*** ERROR *** Unknown log format (" << formatId << ") while reading log file" << endl;
      }
      return LineType::DATA;
    }


  } else {
    currentFormat = nullptr;
    return LineType::END_OF_FILE;
  }
}

string LogOutput::currentLineFormatKey() {
  if ( currentFormat != nullptr ) {
    return( currentFormat->formatKey );
  } else {
    cerr << "*** ERROR *** Current line of log file has no format information" << endl;
    return("");
  }
}

int LogOutput::getLogItemIntValue( string pKey ) {
  map<string, LogItem*>::iterator itMapItemsByKey;
  if ( currentFormat != nullptr ) {
    itMapItemsByKey = currentFormat->mapItemsByKey.find( pKey );
    if ( itMapItemsByKey != currentFormat->mapItemsByKey.end() ) {
      return( itMapItemsByKey->second->data.intValue);
    } else {
      cerr << "*** ERROR *** Current line of log file does not have a " << pKey << " item" << endl;
      return( 0 );
    }
  } else {
    cerr << "*** ERROR *** Current line of log file contains no data" << endl;
    return( 0 );
  }
}

long LogOutput::getLogItemLongValue( string pKey ) {
  map<string, LogItem*>::iterator itMapItemsByKey;
  if ( currentFormat != nullptr ) {
    itMapItemsByKey = currentFormat->mapItemsByKey.find( pKey );
    if ( itMapItemsByKey != currentFormat->mapItemsByKey.end() ) {
      return( itMapItemsByKey->second->data.longValue);
    } else {
      cerr << "*** ERROR *** Current line of log file does not have a " << pKey << " item" << endl;
      return( 0 );
    }
  } else {
    cerr << "*** ERROR *** Current line of log file contains no data" << endl;
    return( 0 );
  }
}

string LogOutput::getLogItemStringValue( string pKey ) {
  map<string, LogItem*>::iterator itMapItemsByKey;
  if ( currentFormat != nullptr ) {
    itMapItemsByKey = currentFormat->mapItemsByKey.find( pKey );
    if ( itMapItemsByKey != currentFormat->mapItemsByKey.end() ) {
      return( itMapItemsByKey->second->data.stringValue);
    } else {
      cerr << "*** ERROR *** Current line of log file does not have a " << pKey << " item" << endl;
      return( 0 );
    }
  } else {
    cerr << "*** ERROR *** Current line of log file contains no data" << endl;
    return( 0 );
  }
}

double LogOutput::getLogItemDoubleValue( string pKey ) {
  map<string, LogItem*>::iterator itMapItemsByKey;
  if ( currentFormat != nullptr ) {
    itMapItemsByKey = currentFormat->mapItemsByKey.find( pKey );
    if ( itMapItemsByKey != currentFormat->mapItemsByKey.end() ) {
      return( itMapItemsByKey->second->data.doubleValue);
    } else {
      cerr << "*** ERROR *** Current line of log file does not have a " << pKey << " item" << endl;
      return( 0 );
    }
  } else {
    cerr << "*** ERROR *** Current line of log file contains no data" << endl;
    return( 0 );
  }
}

bool LogOutput::getLogItemBooleanValue( string pKey ) {
  map<string, LogItem*>::iterator itMapItemsByKey;
  if ( currentFormat != nullptr ) {
    itMapItemsByKey = currentFormat->mapItemsByKey.find( pKey );
    if ( itMapItemsByKey != currentFormat->mapItemsByKey.end() ) {
      return( itMapItemsByKey->second->data.booleanValue);
    } else {
      cerr << "*** ERROR *** Current line of log file does not have a " << pKey << " item" << endl;
      return( false );
    }
  } else {
    cerr << "*** ERROR *** Current line of log file contains no data" << endl;
    return( false );
  }
}

bool LogOutput::hasItem( string pKey ) {
  map<string, LogItem*>::iterator itMapItemsByKey;
  if ( currentFormat != nullptr ) {
    itMapItemsByKey = currentFormat->mapItemsByKey.find( pKey );
    if ( itMapItemsByKey != currentFormat->mapItemsByKey.end() ) {
      return( true );
    } else {
      return( false );
    }
  } else {
    cerr << "*** ERROR *** Current line of log file contains no data" << endl;
    return( false );
  }

}

void LogOutput::log( OutputLineFormat &format...) {
  va_list args;
  va_start(args, format);
  string buffer;

  if ( knownFormats.find( format.formatID ) == knownFormats.end() ) {
    knownFormats.insert( format.formatID );

    buffer = "#<lineFormat id=\"" + to_string(format.formatID) + "\" key=\"" + format.formatKey +  "\" description=\"" + format.formatDescription + "\">\n";
    for ( vector<LogItem*>::iterator it = format.vectorItems.begin(); it != format.vectorItems.end(); it++ ) {
      buffer += "#    <item type=\"" + LogItem::toString((*it)->type);
      buffer += "\" key=\"" + (*it)->key + "\">" + (*it)->description + "</item>\n";
    }

    buffer += "#</lineFormat>\n";
    fprintf( outputFile , "%s", buffer.c_str());
  }

  buffer = to_string( format.formatID );

  for ( vector<LogItem*>::iterator it = format.vectorItems.begin(); it != format.vectorItems.end(); it++ ) {
    switch ( (*it)->type ) {
    case LogItem::ItemType::INT32:
      buffer += " " + to_string( va_arg(args, int) );
      break;
    case LogItem::ItemType::INT64:
      buffer += " " + to_string( va_arg(args, long) );
      break;
    case LogItem::ItemType::STRING:
      buffer += " " + string( va_arg(args, const char*) );
      break;
    case LogItem::ItemType::DOUBLE:
      buffer += " " + to_string( va_arg(args, double) );
      break;
    case LogItem::ItemType::BOOLEAN:
      buffer += " " + to_string( (va_arg(args, int) != 0) );
      break;
    default:
      break;
    }
  }
  buffer += "\n";
  fprintf( outputFile, "%s", buffer.c_str());
}

//===========================================================================================================
//
//          LogSystem  (class)
//
//===========================================================================================================
std::ofstream LogSystem::NodeInfo;
std::ofstream LogSystem::EventsLog;
std::ofstream LogSystem::EstimationLog;
std::ofstream LogSystem::SummarizeLog;
std::ofstream LogSystem::RoutingInfoLog;

FILE *LogSystem::EventsLogC;
FILE *LogSystem::NodeInfoC;
FILE *LogSystem::EstimationLogC;
FILE *LogSystem::SummarizeLogC;
FILE *LogSystem::RoutingInfoLogC;


LogOutput LogSystem::EventsLogOutput;
LogOutput LogSystem::NodeInfoLogOutput;
LogOutput LogSystem::EstimationLogOutput;
LogOutput LogSystem::SummarizeLogOutput;
LogOutput LogSystem::RoutingInfoOuput;

OutputLineFormat LogSystem::sentEventLog("s","packet sent");
OutputLineFormat LogSystem::receptionEventLog("r","packet received");
OutputLineFormat LogSystem::collisionEventLog("c","packet collisionned (dropped at MAC level because of too many altered bits)");
OutputLineFormat LogSystem::ignoredEventLog("i","packet ignored (because of maxConcurrentReception)");

OutputLineFormat LogSystem::routingRCV("rr","routing receive");
OutputLineFormat LogSystem::routingSND("rs","routing send");
OutputLineFormat LogSystem::memoryTrace("mt","memory trace");
OutputLineFormat LogSystem::dstReach("dr","destination reach");

OutputLineFormat LogSystem::slrBroadcastReach("br","slr broadcast destination reach");
OutputLineFormat LogSystem::hcdBroadcastReach("hr","hcd broadcast destination reach");
// OutputLineFormat LogSystem::channelUsage("cu","channel usage");

// #StaticLogSystem



LogSystem LogSystem::myLogSystem;

void LogSystem::initOutputStream(string _name, std::ofstream &_stream, FILE **_fileC, LogOutput &_logOutput ) {
  map<string,logSystemInfo_t> mapOutputFiles = ScenarioParameters::getMapOutPutFiles();
  map<string,logSystemInfo_t>::iterator it = mapOutputFiles.find(_name);

  if ( it == mapOutputFiles.end() || it->second.output.compare("cout") == 0 ) {
    _stream.basic_ios<char>::rdbuf(cout.rdbuf());
    cout << "  redirecting " << _name << " to cout" << endl;
  } else {
    if ( it->second.output.compare("cerr") == 0 ) {
      _stream.basic_ios<char>::rdbuf(cerr.rdbuf());
      cout << "  redirecting " << _name << " to cerr" << endl;
    } else {
      if ( it->second.output.compare("") == 0 ) {
        //NodeInfo.open("/dev/null");
        cout << "  redirecting " << _name << "  to /dev/null"<< endl;
      } else {
				string suffix = "";
				if ( it->second.suffix.length() > 0 ) {
					suffix = it->second.suffix;
					if ( ScenarioParameters::getOutputBaseName().length() > 0 ) {
						suffix = "-" + suffix;
					}
				} else {
					cerr << "*** ERROR *** A suffix attribute is required for " << _name << endl;
					exit(EXIT_FAILURE);
				}

        string n = ScenarioParameters::getScenarioDirectory() + "/" + ScenarioParameters::getOutputBaseName() + suffix + ScenarioParameters::getDefaultExtension();
        if (it->second.io.compare("fprintf") == 0) {
          *_fileC = fopen(n.c_str(),"w");
          cout << "  [fprintf] redirecting " << _name << " to " << n << endl;
        } else if (it->second.io.compare("stream") == 0) {
          cout << "  [iostream] redirecting " << _name << " to " << n << endl;
          _stream.open(n.c_str());
        } else if (it->second.io.compare("smartLog") == 0) {
          cout << "  [smartLog] redirecting " << _name << " to " << n << endl;
          _logOutput.create(n.c_str());
        } else {
          cerr << "*** ERROR *** Invalid output system " << it->second.io << endl;
          exit(EXIT_FAILURE);
        }

      }
    }
  }
}

void LogSystem::initLogSystem() {
  cout << "Initializing LogSystem ..." << endl;
  LogSystem::initOutputStream("NodeInfo", NodeInfo, &NodeInfoC, NodeInfoLogOutput);
  LogSystem::initOutputStream("EventsLog", EventsLog, &EventsLogC, EventsLogOutput);
  LogSystem::initOutputStream("EstimationLog", EstimationLog, &EstimationLogC, EstimationLogOutput);
  LogSystem::initOutputStream("SummarizeLog", SummarizeLog, &SummarizeLogC, SummarizeLogOutput);

  receptionEventLog.addItem( LogItem::INT64, "time", "simulation time in fs" );
  receptionEventLog.addItem( LogItem::INT32, "nodeID", "node ID handling the event" );
  receptionEventLog.addItem( LogItem::INT32, "transmitterID", "node ID of the MAC-level transmitter" );
  receptionEventLog.addItem( LogItem::INT32, "beta", "beta" );
  receptionEventLog.addItem( LogItem::INT32, "size", "packet size in bits" );
  receptionEventLog.addItem( LogItem::INT32, "flow", "flow id" );
  receptionEventLog.addItem( LogItem::INT32, "seq", "packet sequence number" );

  sentEventLog.addItem( LogItem::INT64, "time", "simulation time in fs" );
  sentEventLog.addItem( LogItem::INT32, "nodeID", "node ID handling the event" );
  sentEventLog.addItem( LogItem::INT32, "transmitterID", "node ID of the MAC-level transmitter" );
  sentEventLog.addItem( LogItem::INT32, "beta", "beta" );
  sentEventLog.addItem( LogItem::INT32, "size", "packet size in bits" );
  sentEventLog.addItem( LogItem::INT32, "pktType", "packet type" );
  sentEventLog.addItem( LogItem::INT32, "flow", "flow id" );
  sentEventLog.addItem( LogItem::INT32, "seq", "packet sequence number" );

  collisionEventLog.addItem( LogItem::INT64, "time", "simulation time in fs" );
  collisionEventLog.addItem( LogItem::INT32, "nodeID", "node ID handling the event" );
  collisionEventLog.addItem( LogItem::INT32, "transmitterID", "node ID of the MAC-level transmitter" );
  collisionEventLog.addItem( LogItem::INT32, "beta", "beta" );
  collisionEventLog.addItem( LogItem::INT32, "size", "packet size in bits" );
  collisionEventLog.addItem( LogItem::INT32, "pktType", "packet type" );
  collisionEventLog.addItem(LogItem::INT32,"flow","flow id");
  collisionEventLog.addItem(LogItem::INT32,"seq","packet sequence number");
  
  ignoredEventLog.addItem( LogItem::INT64, "time", "simulation time in fs" );
  ignoredEventLog.addItem( LogItem::INT32, "nodeID", "node ID handling the event" );
  ignoredEventLog.addItem( LogItem::INT32, "transmitterID", "node ID of the MAC-level transmitter" );
  ignoredEventLog.addItem( LogItem::INT32, "beta", "beta" );
  ignoredEventLog.addItem( LogItem::INT32, "size", "packet size in bits" );
  ignoredEventLog.addItem( LogItem::INT32, "pktType", "packet type" );
  ignoredEventLog.addItem(LogItem::INT32,"flow","flow id");
  ignoredEventLog.addItem(LogItem::INT32,"seq","packet sequence number");
  
  LogSystem::initOutputStream("RoutingInfoLog", RoutingInfoLog, &RoutingInfoLogC, RoutingInfoOuput);

  routingRCV.addItem(LogItem::INT64,"time","simulation time in fs");
  routingRCV.addItem(LogItem::INT32,"nodeID","node ID handling the event");
  routingRCV.addItem(LogItem::INT32,"pktType","packet type");
  routingRCV.addItem(LogItem::INT32,"flow","flow id");
  routingRCV.addItem(LogItem::INT32,"seq","packet sequence number");
  routingRCV.addItem(LogItem::INT32,"coll","number of collided bits");

  routingSND.addItem(LogItem::INT64,"time","simulation time in fs");
  routingSND.addItem(LogItem::INT32,"nodeID","node ID handling the event");
  routingSND.addItem(LogItem::INT32,"pktType","packet type");
  routingSND.addItem(LogItem::INT32,"flow","flow id");
  routingSND.addItem(LogItem::INT32,"seq","packet sequence number");

  memoryTrace.addItem(LogItem::INT64,"time","simulation time in fs");
  memoryTrace.addItem(LogItem::INT32,"nodeID","node ID handling the event");
  memoryTrace.addItem(LogItem::STRING,"memEv","memory event");  // "+" means new packet in buffer and "-" means the packet is not in the buffer anymore
  memoryTrace.addItem(LogItem::INT32,"flow","flow id");
  memoryTrace.addItem(LogItem::INT32,"seq","packet sequence number");

  dstReach.addItem(LogItem::INT64,"time","simulation time in fs");
  dstReach.addItem(LogItem::INT32,"nodeID","node ID handling the event");
  dstReach.addItem(LogItem::INT32,"pktType","packet Type");
  dstReach.addItem(LogItem::INT32,"flow","flow id");
  dstReach.addItem(LogItem::INT32,"seq","packet sequence number");

  slrBroadcastReach.addItem(LogItem::INT64,"time","simulation time in fs");
  slrBroadcastReach.addItem(LogItem::INT32,"nodeID","node ID handling the event");
  slrBroadcastReach.addItem(LogItem::INT32,"pktType","packet Type");
  slrBroadcastReach.addItem(LogItem::INT32,"flow","flow id");
  slrBroadcastReach.addItem(LogItem::INT32,"seq","packet sequence number");
  slrBroadcastReach.addItem(LogItem::INT32,"SLRX","SLRX");
  slrBroadcastReach.addItem(LogItem::INT32,"SLRY","SLRY");
  slrBroadcastReach.addItem(LogItem::INT32,"SLRZ","SLRZ");

  hcdBroadcastReach.addItem(LogItem::INT64,"time","simulation time in fs");
  hcdBroadcastReach.addItem(LogItem::INT32,"nodeID","node ID handling the event");
  hcdBroadcastReach.addItem(LogItem::INT32,"pktType","packet Type");
  hcdBroadcastReach.addItem(LogItem::INT32,"flow","flow id");
  hcdBroadcastReach.addItem(LogItem::INT32,"seq","packet sequence number");
  hcdBroadcastReach.addItem(LogItem::INT32,"SLRX","SLRX");
  hcdBroadcastReach.addItem(LogItem::INT32,"SLRY","SLRY");
  hcdBroadcastReach.addItem(LogItem::INT32,"SLRZ","SLRZ");

  //     channelUsage.addItem(LogItem::INT64,"time","date in fs");
  //     channelUsage.addItem(LogItem::INT32,"pktType","packet Type");
  //     channelUsage.addItem(LogItem::INT32,"flow","flow id");
  //     channelUsage.addItem(LogItem::INT32,"seq","packet sequence number");
  //     channelUsage.addItem(LogItem::INT32,"beta","beta");
  //     channelUsage.addItem(LogItem::STRING,"chEv","channel event");  // "+" means new packet on channel and "-" means the packet have been received by a node

  // #LogSystem
}
