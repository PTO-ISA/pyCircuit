cmake_minimum_required(VERSION 3.25)
project(PycircuitSiblingCalls LANGUAGES CXX)

find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")

pycircuit_add_example(sibling_calls
  PACKAGE example_sibling_calls
  TOP example_sibling_calls.sibling_calls.SiblingCalls
  ENTRY_SOURCE sibling_calls
  CYCLES 4
  SOURCES counter.py clients.py sibling_calls.py)
