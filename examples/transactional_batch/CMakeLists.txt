cmake_minimum_required(VERSION 3.25)
project(PycircuitTransactionalBatch LANGUAGES CXX)

find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")

pycircuit_add_example(transactional_batch
  PACKAGE example_transactional_batch
  TOP example_transactional_batch.Top.Top
  ENTRY_SOURCE Top
  CYCLES 34
  SOURCES design.py Top.py)
