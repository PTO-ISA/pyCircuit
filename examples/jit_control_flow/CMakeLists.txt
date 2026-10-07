cmake_minimum_required(VERSION 3.25)
project(PycircuitJitControlFlow LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(jit_control_flow
  PACKAGE example_jit_control_flow TOP example_jit_control_flow.jit_control_flow.JitControlFlow
  ENTRY_SOURCE jit_control_flow SOURCES jit_control_flow.py)
