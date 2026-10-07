cmake_minimum_required(VERSION 3.25)
project(PycircuitRob LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(rob PACKAGE example_rob TOP example_rob.rob.Rob
  ENTRY_SOURCE rob SOURCES rob.py)
