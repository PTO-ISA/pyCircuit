cmake_minimum_required(VERSION 3.25)
project(PycircuitResourceComposition LANGUAGES CXX)

find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")

pycircuit_add_example(resource_composition
  PACKAGE example_resource_composition
  TOP example_resource_composition.Top.Top
  ENTRY_SOURCE Top
  CYCLES 20
  SOURCES counter.py split_stage.py relay.py observer.py Top.py)
