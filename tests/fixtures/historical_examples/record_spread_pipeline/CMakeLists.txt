cmake_minimum_required(VERSION 3.25)
project(PycircuitRecordSpreadPipeline LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(record_spread_pipeline
  PACKAGE example_record_spread_pipeline
  TOP example_record_spread_pipeline.record_spread_pipeline.RecordSpreadPipeline
  ENTRY_SOURCE record_spread_pipeline
  SOURCES record_spread_pipeline.py
  SYSTEM_TOP example_record_spread_pipeline.bench.ExerciseRecordSpreadPipeline
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 158
  SYSTEM_SOURCES record_spread_pipeline.py bench.py)
if(PYC_EXAMPLE_VERIFY)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(IVERILOG_EXECUTABLE iverilog REQUIRED)
find_program(VVP_EXECUTABLE vvp REQUIRED)
add_test(NAME record_spread_pipeline_four_state
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/four_state.py"
    --source "${CMAKE_CURRENT_SOURCE_DIR}"
    --build "${CMAKE_CURRENT_BINARY_DIR}"
    --include "${PYCIRCUIT_RUNTIME_INCLUDE_DIR}"
    --scratch "${CMAKE_CURRENT_BINARY_DIR}/four-state"
    --iverilog "${IVERILOG_EXECUTABLE}"
    --vvp "${VVP_EXECUTABLE}")
set_tests_properties(record_spread_pipeline_four_state PROPERTIES DEPENDS record_spread_pipeline)

endif()
