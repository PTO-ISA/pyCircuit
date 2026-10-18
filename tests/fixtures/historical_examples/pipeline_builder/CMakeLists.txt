cmake_minimum_required(VERSION 3.25)
project(PycircuitPipelineBuilder LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(pipeline_builder PACKAGE example_pipeline_builder TOP example_pipeline_builder.pipeline_builder.PipelineBuilder
  ENTRY_SOURCE pipeline_builder SOURCES pipeline_builder.py
  SYSTEM_TOP example_pipeline_builder.bench.ExercisePipelineBuilder
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 72
  SYSTEM_SOURCES pipeline_builder.py bench.py)
