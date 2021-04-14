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

#ifndef EVENTTYPES_H_
#define EVENTTYPES_H_

//
// Add here your custom events identifiers
//

enum class EventType {
  GENERIC,
  NODE_STARTUP,
  DATA_SINK_LOG,
  NODE_LOG,
  CBR_PACKET_GENERATION,
  DENSITY_ESTIMATOR_PACKET_GENERATION,
  SLR_INITIALISATION_PACKET_GENERATION,
  HCD_INITIALISATION_PACKET_GENERATION,
  START_SEND_PACKET,
  END_SEND_PACKET,
  START_RECEIVE_PACKET,
  END_RECEIVE_PACKET,
  BACKOFF_SENDING_EVENT,
  BACKOFF_RING_SENDING_EVENT,
  SLR_BACKOFF_INITIALISATION_PACKET_GENERATION,
  SLR_BACKOFF_SENDING_EVENT,
  SLEEPING_SETUP_EVENT,
  INCIDENT_OBSERVATION,
  BACKOFF_DEVIATION_PROPAGATION_PHASE_EVENT,
  BACKOFF_DEVIATION_DELAYED_SEND_EVENT
};

#endif /* EVENTTYPES_H_ */
