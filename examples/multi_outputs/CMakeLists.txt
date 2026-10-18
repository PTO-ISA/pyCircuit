cmake_minimum_required(VERSION 3.25)
project(PycircuitMultiOutputs LANGUAGES CXX)

find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")

pycircuit_add_example(multi_outputs
  PACKAGE example_multi_outputs
  TOP example_multi_outputs.bench.MultiOutputsSystem
  ENTRY_SOURCE bench
  SOURCES multi_outputs.py bench.py
  CYCLES 8)
