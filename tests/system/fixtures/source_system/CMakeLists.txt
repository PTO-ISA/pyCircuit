cmake_minimum_required(VERSION 3.25)
project(SourceSystemExecution LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(source_system
  PACKAGE checks TOP checks.bench.ExerciseAccumulator
  ENTRY_SOURCE bench SOURCES dut.py bench.py)
