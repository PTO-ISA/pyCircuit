cmake_minimum_required(VERSION 3.25)
project(PycircuitArrayScans LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(array_scans
  PACKAGE example_array_scans
  TOP example_array_scans.array_scans.ArrayScans
  ENTRY_SOURCE array_scans
  SOURCES array_scans.py
  SYSTEM_TOP example_array_scans.bench.ArrayScansSystem
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 16
  SYSTEM_SOURCES array_scans.py bench.py)
