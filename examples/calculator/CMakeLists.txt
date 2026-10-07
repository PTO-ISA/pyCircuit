cmake_minimum_required(VERSION 3.25)
project(PycircuitCalculator LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
# Consumer example target: built outside the framework/SDK install, in nightly.
pycircuit_add_example(calculator PACKAGE example_calculator
  TOP example_calculator.calculator.Calculator
  ENTRY_SOURCE calculator SOURCES calculator.py
  TIMEOUT_SECONDS 120)
