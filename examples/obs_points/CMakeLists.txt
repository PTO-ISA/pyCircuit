cmake_minimum_required(VERSION 3.25)
project(PycircuitObsPoints LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(obs_points
  PACKAGE example_obs_points TOP example_obs_points.obs_points.ObsPoints
  ENTRY_SOURCE obs_points SOURCES obs_points.py
  SYSTEM_TOP example_obs_points.bench.ExerciseObsPoints
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 264
  SYSTEM_SOURCES obs_points.py bench.py)
