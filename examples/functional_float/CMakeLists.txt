cmake_minimum_required(VERSION 3.25)
project(PycircuitFunctionalWordCommit LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(functional_float
  PACKAGE functional_float
  TOP functional_float.tracked_bit_bench.CommitSystem
  ENTRY_SOURCE tracked_bit_bench
  SOURCES word_arithmetic.py bit_transform.py tracked_bit_transform.py tracked_bit_bench.py
  CYCLES 8)
