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

#ifndef AGENTS_INCIDENT_OBSERVER_AGENT_H_
#define AGENTS_INCIDENT_OBSERVER_AGENT_H_

#include "application-agent.h"


struct IncidentShape {
public:
  enum class Function {
    Constant,
    Linear,
    Sine
  };

  // position.
  distance_t x;
  distance_t y;
  distance_t z;

  Function function;

  // for constant values.
  double constantValue; // ex. 50_000
  double startTime; // ex. 1_000_000

  // for linear growth.
  double startValue; // ex. -100
  double growthPerFs; // ex. 0.001

  // for sine-shaped.
  double fsPerCycle; // ex. 10_000_000
  double amplitude; // ex. 400_000

  // in arbitrary intensity units.
  static constexpr double detectThreshold = 1e-9;

  // returns current value in arbitrary intensity units.
  double getValueAt(simulationTime_t time, double dist) const;

  double getMaxDistance() const;
};




class IncidentObserverAgent : public ApplicationAgent {
private:
  shared_ptr<const IncidentShape> incident;

  double distanceToIncident;

  static constexpr int startupTime = 1000000; // = 1ns in fs.
  static constexpr int measureInterval = 1000000; // = 1ns in fs.
  static constexpr int maxObservations = 100;

  void scheduleObservation(simulationTime_t at = measureInterval);
  int observations = 0;

public:
  IncidentObserverAgent(Node *_hostNode, shared_ptr<const IncidentShape> shape,
    mt19937& rnd);

  void observeIncident();

  static void initializeAgent();
};


using IncidentObservationEvent =
  CallMethodEvent<IncidentObserverAgent,
    &IncidentObserverAgent::observeIncident,
    EventType::INCIDENT_OBSERVATION>;


#endif /* AGENTS_INCIDENT_OBSERVER_AGENT_H_ */
