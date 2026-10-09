cmake_minimum_required(VERSION 3.25)
project(PycircuitBoundedIntegerOperations LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(bounded_integer_operations
  PACKAGE example_bounded_integer_operations
  TOP example_bounded_integer_operations.bounded_integer_operations.BoundedIntegerOperations
  ENTRY_SOURCE bounded_integer_operations
  SOURCES bounded_integer_operations.py
  SYSTEM_TOP example_bounded_integer_operations.bench.BoundedIntegerOperationsSystem
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 96
  SYSTEM_SOURCES bounded_integer_operations.py bench.py)
