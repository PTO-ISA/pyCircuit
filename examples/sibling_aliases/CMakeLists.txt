cmake_minimum_required(VERSION 3.25)
project(PycircuitSiblingAliases LANGUAGES CXX)

find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")

pycircuit_add_example(sibling_aliases
  PACKAGE example_sibling_aliases
  TOP example_sibling_aliases.bench.SiblingAliases
  ENTRY_SOURCE bench
  CYCLES 4
  SOURCES sibling_aliases.py bench.py)
