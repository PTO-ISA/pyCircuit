cmake_minimum_required(VERSION 3.25)
project(PycircuitStructuredPacket LANGUAGES CXX)

find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")

pycircuit_add_example(structured_packet
  PACKAGE example_structured_packet
  TOP example_structured_packet.Top.Top
  ENTRY_SOURCE Top
  CYCLES 18
  SOURCES types.py processor.py Top.py)
