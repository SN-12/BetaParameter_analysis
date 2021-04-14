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

#ifndef OUTPUT_H_
#define OUTPUT_H_

#include <vector>
#include <string>
#include <map>
#include <set>
#include <utility>

#include <tinyxml2.h>

using namespace std;

typedef struct {
  string output;
  string suffix;
  string io;
} logSystemInfo_t;

//==============================================================================
//
//          LogItem  (class)
//
//==============================================================================


class LogItem {
public:
  enum ItemType {
    INT32,
    INT64,
    STRING,
    FLOAT,
    DOUBLE,
    BOOLEAN
  };

  ItemType type;
  string key;
  string description;

  union Data {
    int intValue;
    long longValue;
    char stringValue[20];
    double doubleValue;
    bool booleanValue;
  } data;

  LogItem( ItemType pType, string pKey, string pDescription ) {
    type = pType;
    key = pKey;
    description = pDescription;
  }

  static string toString( ItemType pItemType ) {
    switch ( pItemType ) {
    case INT32:
      return "Integer 32";
      break;
    case INT64:
      return "Integer 64";
      break;
    case STRING:
      return "String";
      break;
    case DOUBLE:
      return "Double";
      break;
    case BOOLEAN:
      return "Boolean";
      break;
    default:
      return "Undefined Item Type";

    }
  }
};



//==============================================================================
//
//          OutputLineFormat  (class)
//
//==============================================================================

struct comparer
{
public:
  bool operator()(const std::string x, const std::string y)
  {
    return x.compare(y)==0;
  }
};

class OutputLineFormat {
private:
  static int nextFormatID;
public:

  int formatID;

  string formatKey;
  string formatDescription;
  vector<LogItem*> vectorItems;
  map<string, LogItem*> mapItemsByKey;

  OutputLineFormat( string pFormatKey, string pFormatDescription ) {
    formatID = OutputLineFormat::nextFormatID++;

    formatKey = pFormatKey;
    formatDescription = pFormatDescription;
  }

  OutputLineFormat( int pId, string pFormatKey, string pFormatDescription ) {
    if ( pId >= nextFormatID ) {
      nextFormatID = pId+1;
    }

    formatID = pId;
    formatKey = pFormatKey;
    formatDescription = pFormatDescription;
  }

  ~OutputLineFormat() {
  }

  void addItem( LogItem::ItemType pType, string pKey, string pDescription ) {
    LogItem *newItem = new LogItem(pType, pKey, pDescription);

    vectorItems.push_back( newItem );
    mapItemsByKey.insert(pair<string, LogItem*>(pKey, newItem));
  }

  void print() {
    cout << formatKey << " : " << formatDescription << endl;
    for ( vector<LogItem*>::iterator it = vectorItems.begin(); it != vectorItems.end(); it++ ) {
      cout << "  " << (*it)->key << " " << (*it)->description << endl;
    }
  }
};

//==============================================================================
//
//          LogOutput  (class)
//
//==============================================================================

class LogOutput {
private:
  set<int> knownFormats;

  map<int,OutputLineFormat*> mapKnownFormats;

  char *buffer;
  char *xmlBuffer;
  tinyxml2::XMLDocument xmlDoc;
  tinyxml2::XMLElement *XMLRootNode;

  OutputLineFormat *currentFormat;

public:
  enum class LineType {
    END_OF_FILE,
    DATA,
    COMMENT
  };

  FILE* outputFile;

  LogOutput();
  ~LogOutput();

  void create( string fileName );
  void open( string fileName );
  void close();

  LineType readNextLine();
  string currentLineFormatKey();

  int getLogItemIntValue( string pKey );
  long getLogItemLongValue( string pKey );
  string getLogItemStringValue( string pKey );
  double getLogItemDoubleValue( string pKey );
  bool getLogItemBooleanValue( string pKey );

  bool hasItem( string pKey );
  void log( OutputLineFormat &format...);
};


//==============================================================================
//
//          LogSystem  (class)
//
//==============================================================================

class LogSystem {
private:
  static LogSystem myLogSystem;
public:
  static std::ofstream NodeInfo;
  static std::ofstream EventsLog;
  static std::ofstream EstimationLog;
  static std::ofstream SummarizeLog;
  static std::ofstream RoutingInfoLog;

  static FILE *EventsLogC;
  static FILE *NodeInfoC;
  static FILE *EstimationLogC;
  static FILE *SummarizeLogC;
  static FILE *RoutingInfoLogC;

  static LogOutput EventsLogOutput;
  static LogOutput NodeInfoLogOutput;
  static LogOutput EstimationLogOutput;
  static LogOutput SummarizeLogOutput;
  static LogOutput RoutingInfoOuput;

  static OutputLineFormat receptionEventLog;
  static OutputLineFormat sentEventLog;
  static OutputLineFormat collisionEventLog;
  static OutputLineFormat ignoredEventLog;

  static OutputLineFormat routingRCV;
  static OutputLineFormat routingSND;
  static OutputLineFormat memoryTrace;
  static OutputLineFormat dstReach;

  static OutputLineFormat slrBroadcastReach;

  static OutputLineFormat hcdBroadcastReach;
  //     static OutputLineFormat channelUsage;

  // #LogSystem NEW OPTIONS

  LogSystem() {
  }

  static void initOutputStream(string _name, std::ofstream &_stream, FILE **_fileC, LogOutput &_logOutput);
  static void initLogSystem();
};

#endif /* OUTPUT_H_ */
