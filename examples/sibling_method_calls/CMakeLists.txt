cmake_minimum_required(VERSION 3.25)
project(PycircuitSiblingMethodCalls LANGUAGES CXX)

find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")

pycircuit_add_example(sibling_method_calls
  PACKAGE example_sibling_method_calls
  TOP example_sibling_method_calls.Top.Top
  ENTRY_SOURCE Top
  CYCLES 4
  SOURCES Receiver.py Sender.py Top.py)
