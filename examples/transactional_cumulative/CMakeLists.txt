cmake_minimum_required(VERSION 3.25)
project(PycircuitTransactionalCumulative LANGUAGES CXX)

find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")

pycircuit_add_example(transactional_cumulative
  PACKAGE example_transactional_cumulative
  TOP example_transactional_cumulative.Top.Top
  ENTRY_SOURCE Top
  CYCLES 22
  SOURCES design.py Top.py)
