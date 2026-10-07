cmake_minimum_required(VERSION 3.25)
project(PycircuitBoundaryValuePorts LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(boundary_value_ports
  PACKAGE example_boundary_value_ports TOP example_boundary_value_ports.boundary_value_ports.BoundaryValuePorts
  ENTRY_SOURCE boundary_value_ports SOURCES boundary_value_ports.py)
