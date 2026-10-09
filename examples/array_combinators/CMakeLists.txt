cmake_minimum_required(VERSION 3.25)
project(PycircuitArrayCombinators LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(array_combinators
  PACKAGE example_array_combinators
  TOP example_array_combinators.array_combinators.ArrayCombinators
  ENTRY_SOURCE array_combinators
  SOURCES array_combinators.py
  SYSTEM_TOP example_array_combinators.bench.ArrayCombinatorsSystem
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 12
  SYSTEM_SOURCES array_combinators.py bench.py)
