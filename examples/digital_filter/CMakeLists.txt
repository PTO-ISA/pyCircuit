cmake_minimum_required(VERSION 3.25)
project(PycircuitDigitalFilter LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(digital_filter PACKAGE example_digital_filter TOP example_digital_filter.digital_filter.DigitalFilter
  ENTRY_SOURCE digital_filter SOURCES digital_filter.py)
