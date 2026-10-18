cmake_minimum_required(VERSION 3.25)
project(PycircuitQueueRulePipeline LANGUAGES CXX)

find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")

pycircuit_add_example(queue_rule_pipeline
  PACKAGE example_queue_rule_pipeline
  TOP example_queue_rule_pipeline.bench.QueueRulePipelineSystem
  ENTRY_SOURCE bench
  SOURCES queue_rule_pipeline.py bench.py
  CYCLES 12)
