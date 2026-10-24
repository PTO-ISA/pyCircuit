cmake_minimum_required(VERSION 3.25)
project(PycircuitSiblingAtomic LANGUAGES CXX)

find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")

pycircuit_add_example(sibling_atomic
  PACKAGE example_sibling_atomic
  TOP example_sibling_atomic.bench.SiblingAtomic
  ENTRY_SOURCE bench
  CYCLES 4
  SOURCES sibling_atomic.py bench.py)
