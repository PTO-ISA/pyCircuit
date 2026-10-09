cmake_minimum_required(VERSION 3.25)
project(PycircuitDodgeballGame LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(dodgeball_game
  PACKAGE example_dodgeball_game TOP example_dodgeball_game.dodgeball_game.DodgeballGame
  ENTRY_SOURCE dodgeball_game SOURCES dodgeball_game.py
  SYSTEM_TOP example_dodgeball_game.bench.ExerciseDodgeballGame
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 4001
  SYSTEM_SOURCES dodgeball_game.py bench.py)
