cmake_minimum_required(VERSION 3.25)
project(PycircuitRecursiveArrayUpdates LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(recursive_array_updates
  PACKAGE example_recursive_array_updates
  TOP example_recursive_array_updates.recursive_array_updates.RecursiveArrayUpdates
  ENTRY_SOURCE recursive_array_updates
  SOURCES recursive_array_updates.py
  SYSTEM_TOP example_recursive_array_updates.bench.RecursiveArrayUpdatesSystem
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 16
  SYSTEM_SOURCES recursive_array_updates.py bench.py)
