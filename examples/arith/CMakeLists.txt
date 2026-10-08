cmake_minimum_required(VERSION 3.25)
project(PycircuitArith LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(arith
  PACKAGE example_arith TOP example_arith.arith.Arith
  ENTRY_SOURCE arith SOURCES arith.py
  SYSTEM_TOP example_arith.bench.ExerciseArith
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 10
  SYSTEM_SOURCES arith.py bench.py)
