cmake_minimum_required(VERSION 3.25)
project(PycircuitJitPipelineVec LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(jit_pipeline_vec PACKAGE example_jit_pipeline_vec
  TOP example_jit_pipeline_vec.jit_pipeline_vec.JitPipelineVec
  ENTRY_SOURCE jit_pipeline_vec SOURCES jit_pipeline_vec.py)
