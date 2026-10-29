cmake_minimum_required(VERSION 3.25)
project(PycircuitParentStateInterfaces LANGUAGES CXX)

find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")

pycircuit_add_example(parent_state_interfaces
  PACKAGE example_parent_state_interfaces
  TOP example_parent_state_interfaces.top.Top
  ENTRY_SOURCE top
  CYCLES 4
  SOURCES rules.py stage.py top.py)
