cmake_minimum_required(VERSION 3.25)
project(PycircuitArrayReductions LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(array_reductions
  PACKAGE example_array_reductions
  TOP example_array_reductions.array_reductions.ArrayReductions
  ENTRY_SOURCE array_reductions
  SOURCES array_reductions.py
  SYSTEM_TOP example_array_reductions.bench.ArrayReductionsSystem
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 16
  SYSTEM_SOURCES array_reductions.py bench.py)
