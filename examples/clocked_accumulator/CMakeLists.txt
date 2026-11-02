cmake_minimum_required(VERSION 3.25)
project(ClockedAccumulator LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(clocked_accumulator PACKAGE clocked_accumulator
  TOP clocked_accumulator.model.Accumulator ENTRY_SOURCE model SOURCES provider.py model.py
  MODEL_SOURCES provider.py model.py CYCLES 32)
