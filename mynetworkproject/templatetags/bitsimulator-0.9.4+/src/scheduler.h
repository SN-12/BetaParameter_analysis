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

#ifndef SCHEDULER_H_
#define SCHEDULER_H_


#include <iostream>
#include <map>
#include "utils.h"
#include "events.h"
using namespace std;

#define TIME_PICO   (simulationTime_t)1000
#define TIME_NANO   (simulationTime_t)1000000
#define TIME_MICRO  (simulationTime_t)1000000000
#define TIME_MILL   (simulationTime_t)1000000000000
#define TIME_SECOND (simulationTime_t)1000000000000000

//===========================================================================================================
//
//          Scheduler  (class)
//
//===========================================================================================================

class Scheduler {
private:
  Scheduler();

  static Scheduler myScheduler;

  multimap<simulationTime_t,EventPtr> eventsMap;
  //map<simulationTime_t,queue<EventPtr>> eventsMap;
  static simulationTime_t currentDate;
  simulationTime_t maximumDate;
  int eventsMapSize, largestEventsMapSize;
  bool prematureEnd;

public:
  ~Scheduler();
  static Scheduler &getScheduler() {
    //static Scheduler instance;
    return(myScheduler);
  }

  void schedule(Event *_ev);
  static simulationTime_t now() { return(myScheduler.currentDate); }
  static void initScheduler() { myScheduler = Scheduler(); }
  static void endSimulation() { myScheduler.prematureEnd = true; }
  void run();
};


#endif /* SCHEDULER_H_ */
