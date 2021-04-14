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

#include <sstream>
#include <chrono>
#include <assert.h>
#include "scheduler.h"

using namespace std;

//===========================================================================================================
//
//          Scheduler  (class)
//
//===========================================================================================================

simulationTime_t Scheduler::currentDate = 0;
Scheduler Scheduler::myScheduler;

Scheduler::Scheduler() {
  //cout << "Scheduler instantiation" << endl;

  currentDate = 0;
  maximumDate = 1*TIME_SECOND;
  //         maximumDate = 5*100000;
  eventsMapSize = 0;
  largestEventsMapSize = 0;
  prematureEnd = false;
}


Scheduler::~Scheduler() {
  //cout << "Scheduler destruction" << endl;
}

void Scheduler::run() {
  long processedEventCounter = 0;
  double elapsed_milliseconds;

  cerr << "*** Simulation start ***" << endl;

  std::chrono::time_point<std::chrono::system_clock> start, end;
  std::chrono::time_point<std::chrono::system_clock> startPeriod, endPeriod;
  start = std::chrono::system_clock::now();
  startPeriod = std::chrono::system_clock::now();

  multimap<simulationTime_t, EventPtr>::iterator first;
  EventPtr pev;

  while ( (!eventsMap.empty() ) && currentDate < maximumDate && !prematureEnd) {
    first=eventsMap.begin();
    pev = (*first).second;
    currentDate = pev->date;
    //                 cout << currentDate << " : " << pev->getEventName() << endl;
    pev->consume();
    eventsMap.erase(first);
    eventsMapSize--;
    if (processedEventCounter % 100000 == 0) {
      endPeriod = std::chrono::system_clock::now();
      elapsed_milliseconds = (double)std::chrono::duration_cast<std::chrono::milliseconds>(endPeriod-startPeriod).count();
      cout << "  processed up to " << currentDate << " ( " << 100000 / (elapsed_milliseconds/1000.0) << " events/s )" << endl;
      startPeriod = std::chrono::system_clock::now();
    }
    processedEventCounter++;
  }

  if (eventsMap.empty()) cerr << "all events processed (fin at " << currentDate << ")" << endl;
  cout << "*** Simulation end ***" << endl;

  end = std::chrono::system_clock::now();

  elapsed_milliseconds = (double)std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
  std::time_t end_time = std::chrono::system_clock::to_time_t(end);

  std::cerr<< "*** finished computation at " << std::ctime(&end_time) << "*** elapsed time: " << elapsed_milliseconds/1000 << "s\n";
  cerr << "*** " << processedEventCounter << " events processed" << endl;
  if (elapsed_milliseconds > 0) {
    cerr << "*** " << (double)processedEventCounter / (elapsed_milliseconds/1000.0) << " events/s" << endl;
  }
  cerr << "*** maximum events list depth " << largestEventsMapSize << endl;
}

//void Scheduler::run() {
// long processedEventCounter = 0;
//
// cerr << "*** Simulation start ***" << endl;
//
// std::chrono::time_point<std::chrono::system_clock> start, end;
// start = std::chrono::system_clock::now();
//
// multimap<simulationTime_t, queue<EventPtr>>::iterator first;
// EventPtr pev;
//
// while ( (!eventsMap.empty() ) && currentDate < maximumDate && !prematureEnd) {
//  first=eventsMap.begin();
//  pev = (*first).second.front();
//  currentDate = pev->date;
//  pev->consume();
//  (*first).second.pop();
//  if ( (*first).second.empty() ) eventsMap.erase(first);
//  eventsMapSize--;
//  if (processedEventCounter % 100000 == 0) cout << "  processed up to " << currentDate << endl;
//  processedEventCounter++;
// }
//
// if (eventsMap.empty()) cerr << "all events processed" << endl;
// cout << "*** Simulation end ***" << endl;
//
// end = std::chrono::system_clock::now();
//
// float elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
// std::time_t end_time = std::chrono::system_clock::to_time_t(end);
//
// std::cerr<< "*** finished computation at " << std::ctime(&end_time) << "*** elapsed time: " << elapsed_milliseconds << "ms\n";
// cerr << "*** " << processedEventCounter << " events processed" << endl;
// if (elapsed_milliseconds > 0) {
//  cerr << "*** " << processedEventCounter / (elapsed_milliseconds/1000) << " events/s" << endl;
// }
// cerr << "*** maximum events list depth " << largestEventsMapSize << endl;
//}

void Scheduler::schedule(Event *_ev) {
  assert(_ev != NULL);
  //stringstream info;

  EventPtr pev(_ev);

  //info << "Schedule a " << pev->getEventName() << " (" << _ev->id << ")";
  //cout << info.str() << endl;

  if (pev->date < Scheduler::currentDate) {
    cerr << "ERROR: An event cannot be scheduled in the past!\n";
    cerr << "current time: " << Scheduler::currentDate << endl;
    cerr << "ev->eventDate: " << pev->date << endl;
    cerr << "ev->getEventName(): " << pev->getEventName() << endl;
    return;
  }

  if (pev->date > maximumDate) {
    cerr << "WARNING: An event should not be scheduled beyond the end of simulation date!\n";
    cerr << "pev->date: " << pev->date << endl;
    cerr << "maximumDate: " << maximumDate << endl;
    return;
  }

  //cout<< " Event insertion: " << pev->getEventName() << " at date " << pev->date << endl;

  eventsMap.insert(pair<long, EventPtr>(pev->date,pev));

  // auto it = eventsMap.lower_bound(pev->date);
  // if (it != eventsMap.end() && it->first == pev->date) {
  //  it->second.push(pev);
  // } else {
  //  queue<EventPtr> q;
  //  q.push(pev);
  //  eventsMap.insert(it, pair<long, queue<EventPtr>>(pev->date,q));
  // }

  // if (it == eventsMap.end()) {
  //  queue<EventPtr> q;
  //  q.push(pev);
  //  eventsMap.insert(pair<long, queue<EventPtr>>(pev->date,q));
  // } else {
  //  it->second.push(pev);
  // }

  eventsMapSize++;
  if (largestEventsMapSize < eventsMapSize) largestEventsMapSize = eventsMapSize;
}
