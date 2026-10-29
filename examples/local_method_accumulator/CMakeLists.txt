cmake_minimum_required(VERSION 3.25)
project(PycircuitLocalMethodAccumulator LANGUAGES CXX)

find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")

pycircuit_add_example(local_method_accumulator
  PACKAGE example_local_method_accumulator
  TOP example_local_method_accumulator.local_method_accumulator.LocalMethodAccumulator
  ENTRY_SOURCE local_method_accumulator
  CYCLES 8
  SOURCES local_method_accumulator.py)
