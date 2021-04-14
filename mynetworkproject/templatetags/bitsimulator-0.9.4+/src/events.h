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

#ifndef EVENTS_H_
#define EVENTS_H_

#include <string>
#include <iostream>
#include <memory>

#include "eventtypes.h"
#include "utils.h"

class Node;


/**
 * The base class for all events.
 * Events are run by the scheduler (scheduler.h), which can accept new events
 * to be run with `Scheduler::getScheduler().schedule(event);`.
 */
class Event {
protected:
  static long nextId;
  static long nbLivingEvents;

public:
  long id;    // unique ID of the event (mainly for debugging purpose)
  simulationTime_t date;    // time at which the event will be processed. 0 means simulation start
  EventType eventType;   // see the various types at the beginning of this file

  Event(simulationTime_t _t);
  Event(Event *_ev);

  virtual ~Event();

  virtual void consume() = 0;
  virtual const std::string getEventName();

  static long getNextId();
  static long getNbLivingEvents();
};


using EventPtr = std::shared_ptr<Event>;


/**
 * A generic event to call a method of an object when the event occurs.
 * This class is specific for the simple case of call-this-at-that-time, so
 * will not fit all cases. If CallMethodEvent is unsuitable or too complex,
 * create a new class that derives directly from Event (see above).
 *
 * Three template parameters supply:
 * - The name of the class that this event calls a method from
 * - A function pointer to the method that will be called
 * - The event type of this event.
 *
 * To use a new event type, first declare the class that calls the function
 * that executes the event:
 *
 * using NodeStartupEvent =
 *   CallMethodEvent<Node, &Node::startupCode, EventType::NODE_STARTUP>;
 *
 * Then, create an instance of this class with the point in time and the
 * specific object to call the method on:
 *
 * Node *n = ...;
 * auto startupEvent = new NodeStartupEvent(Scheduler::now() + 1_ns, n);
 * Scheduler::getScheduler().schedule(startupEvent);
 *
 */
template<typename T, void (T::*event)(), EventType type>
class CallMethodEvent : public Event {
private:
  T* subject;

public:
  CallMethodEvent(simulationTime_t t,
    // std::shared_ptr<T> subject)
    T* subject)
    : Event(t), subject(subject) {
    eventType = type;
  }

  const std::string getEventName() {
    return "CallMethodEvent<T,F,E>";
  }

  void consume() {
    ((*subject).*event)();
  }
};


/**
 * A generic event to call a method of an object with a specific parameter
 * when the event occurs. Extends the CallMethodEvent with an argument that
 * is specified at event creation.
 * This class is specific for the simple case of call-this-at-that-time, so
 * will not fit all cases. If CallArgMethodEvent is unsuitable or too complex,
 * create a new class that derives directly from Event (see above).
 *
 * Four template parameters supply:
 * - The name of the class that this event calls a method from
 * - The type of the argument to the method that will be called
 * - A function pointer to the method that will be called
 * - The event type of this event.
 *
 * To use a new event type, first declare the class that calls the function
 * that executes the event:
 *
 * using StartSendPacketEvent = CallMethodArgEvent<Node, PacketPtr,
 *   &Node::processStartSendEvent,
 *   EventType::START_SEND_PACKET>;
 *
 * Then, create an instance of this class with the point in time, the
 * specific object to call the method on, and the argument to pass:
 *
 * Node *n = ...;
 * PacketPtr p = ...;
 * auto startSendEvent = new StartSendPacketEvent(
 *   Scheduler::now() + 400_ps, n, p);
 * Scheduler::getScheduler().schedule(startSendEvent);
 */
template<typename T, typename S, void (T::*event)(S), EventType type>
class CallMethodArgEvent : public Event {
private:
  T* subject;
  S argument;

public:
  CallMethodArgEvent(simulationTime_t t, T* subject, S argument)
    : Event(t), subject(subject), argument(argument) {
    eventType = type;
  }

  const std::string getEventName() {
    return "CallMethodArgEvent<T,F,E,S>";
  }

  void consume() {
    ((*subject).*event)(argument);
  }
};


#endif /* EVENTS_H_ */
