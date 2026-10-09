cmake_minimum_required(VERSION 3.25)
project(PycircuitHierModules LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(hier_modules
  PACKAGE example_hier_modules TOP example_hier_modules.hier_modules.HierModules
  ENTRY_SOURCE hier_modules SOURCES hier_modules.py
  SYSTEM_TOP example_hier_modules.bench.ExerciseHierModules
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 12
  SYSTEM_SOURCES hier_modules.py bench.py)
