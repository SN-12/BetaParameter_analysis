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
#include <thread>

#include "scheduler.h"
#include "world.h"

void toto() {
  Scheduler::getScheduler().run();
}

int main(int argc, char **argv) {
  puts("\033[36;1m" PACKAGE " " VERSION "\033[0m");

  ScenarioParameters::initialize(argc, argv, 0);
  Scheduler::initScheduler();
  LogSystem::initLogSystem();
  World::initWorld();
  BinaryPayload::initialize(ScenarioParameters::getBinaryPayloadRNGSeed());
  World::initAgents();

  // start the simulation
  if (ScenarioParameters::getGraphicMode()) {
    thread t(toto);
    World::getWorld()->visualizationLoop(World::getWorld());
    t.join();
  } else
    Scheduler::getScheduler().run();

  World::getWorld()->destroyWorld();

  return EXIT_SUCCESS;
}
