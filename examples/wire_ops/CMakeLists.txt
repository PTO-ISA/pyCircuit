cmake_minimum_required(VERSION 3.25)
project(PycircuitWireOps LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(wire_ops
  PACKAGE example_wire_ops TOP example_wire_ops.wire_ops.WireOps
  ENTRY_SOURCE wire_ops SOURCES wire_ops.py)
