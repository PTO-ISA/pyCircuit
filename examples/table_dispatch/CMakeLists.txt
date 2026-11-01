cmake_minimum_required(VERSION 3.25)
project(PycircuitTableDispatch LANGUAGES CXX)

find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")

pycircuit_add_example(table_dispatch
  PACKAGE example_table_dispatch
  TOP example_table_dispatch.table_dispatch.TableDispatch
  ENTRY_SOURCE table_dispatch
  CYCLES 9
  SOURCES table_dispatch.py)
