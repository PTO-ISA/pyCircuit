cmake_minimum_required(VERSION 3.25)
project(PycircuitCounter LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(counter
  PACKAGE example_counter TOP example_counter.counter.Counter
  ENTRY_SOURCE counter SOURCES counter.py
  SYSTEM_TOP example_counter.bench.ExerciseCounter
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 263
  SYSTEM_SOURCES counter.py bench.py)
