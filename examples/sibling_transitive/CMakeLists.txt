cmake_minimum_required(VERSION 3.25)
project(PycircuitSiblingTransitive LANGUAGES CXX)

find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")

pycircuit_add_example(sibling_transitive
  PACKAGE example_sibling_transitive
  TOP example_sibling_transitive.bench.SiblingTransitive
  ENTRY_SOURCE bench
  CYCLES 4
  SOURCES sibling_transitive.py bench.py)
