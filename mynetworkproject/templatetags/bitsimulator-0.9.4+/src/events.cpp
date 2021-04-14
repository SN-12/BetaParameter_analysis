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

#include "events.h"


long Event::nextId = 0;
long Event::nbLivingEvents = 0;


Event::Event(simulationTime_t _t) {
  id = nextId;
  nextId++;
  nbLivingEvents++;
  date = _t;
  eventType = EventType::GENERIC;
}

Event::Event(Event *_ev) {
  id = nextId;
  nextId++;
  nbLivingEvents++;
  date = _ev->date;
  eventType = _ev->eventType;
}

Event::~Event() {
  nbLivingEvents--;
}

const std::string Event::getEventName() {
  return("Generic Event");
}

long Event::getNextId() {
  return(nextId);
}

long Event::getNbLivingEvents() {
  return(nbLivingEvents);
}

