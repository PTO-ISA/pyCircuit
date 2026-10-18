cmake_minimum_required(VERSION 3.25)
project(PycircuitAggregatePayloadPipeline LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(aggregate_payload_pipeline
  PACKAGE example_aggregate_payload_pipeline
  TOP example_aggregate_payload_pipeline.aggregate_payload_pipeline.AggregatePayloadPipeline
  ENTRY_SOURCE aggregate_payload_pipeline
  SOURCES aggregate_payload_pipeline.py
  SYSTEM_TOP example_aggregate_payload_pipeline.bench.AggregatePayloadPipelineSystem
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 48
  SYSTEM_SOURCES aggregate_payload_pipeline.py bench.py)

if(PYC_EXAMPLE_VERIFY)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  find_program(IVERILOG_EXECUTABLE iverilog REQUIRED)
  find_program(VVP_EXECUTABLE vvp REQUIRED)
  add_test(NAME aggregate_payload_pipeline_four_state
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/four_state.py"
      --source "${CMAKE_CURRENT_SOURCE_DIR}"
      --build "${CMAKE_CURRENT_BINARY_DIR}"
      --include "${PYCIRCUIT_RUNTIME_INCLUDE_DIR}"
      --scratch "${CMAKE_CURRENT_BINARY_DIR}/four-state"
      --iverilog "${IVERILOG_EXECUTABLE}"
      --vvp "${VVP_EXECUTABLE}")
  set_tests_properties(aggregate_payload_pipeline_four_state
    PROPERTIES DEPENDS aggregate_payload_pipeline)
endif()
