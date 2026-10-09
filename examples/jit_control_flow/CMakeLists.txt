cmake_minimum_required(VERSION 3.25)
project(PycircuitJitControlFlow LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(jit_control_flow
  PACKAGE example_jit_control_flow TOP example_jit_control_flow.jit_control_flow.JitControlFlow
  ENTRY_SOURCE jit_control_flow SOURCES jit_control_flow.py
  SYSTEM_TOP example_jit_control_flow.bench.ExerciseJitControlFlow
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 65
  SYSTEM_SOURCES jit_control_flow.py bench.py)
