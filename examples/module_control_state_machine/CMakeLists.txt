cmake_minimum_required(VERSION 3.25)
project(PycircuitModuleControlStateMachine LANGUAGES CXX)

find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")

pycircuit_add_example(module_control_state_machine
  PACKAGE example_module_control_state_machine
  TOP example_module_control_state_machine.module_control_state_machine.ModuleControlStateMachine
  ENTRY_SOURCE module_control_state_machine
  CYCLES 5
  SOURCES module_control_state_machine.py)
