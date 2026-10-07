cmake_minimum_required(VERSION 3.25)
project(PycircuitDecodeRules LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(decode_rules
  PACKAGE example_decode_rules TOP example_decode_rules.decode_rules.DecodeRules
  ENTRY_SOURCE decode_rules SOURCES decode_rules.py)
