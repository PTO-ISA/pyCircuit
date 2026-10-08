cmake_minimum_required(VERSION 3.25)
project(PycircuitRecursivePipeline LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(recursive_pipeline
  PACKAGE example_recursive_pipeline TOP example_recursive_pipeline.recursive_pipeline.RecursivePipeline
  ENTRY_SOURCE recursive_pipeline SOURCES recursive_pipeline.py
  SYSTEM_TOP example_recursive_pipeline.bench.RecursivePipelineSystem
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 158
  SYSTEM_SOURCES recursive_pipeline.py bench.py)
