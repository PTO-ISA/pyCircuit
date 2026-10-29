cmake_minimum_required(VERSION 3.25)
project(PycircuitScalarQueuePipeline LANGUAGES CXX)

find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")

pycircuit_add_example(scalar_queue_pipeline
  PACKAGE example_scalar_queue_pipeline
  TOP example_scalar_queue_pipeline.Top.Top
  ENTRY_SOURCE Top
  CYCLES 18
  SOURCES Accumulator.py BiasStage.py SumStage.py Top.py)
