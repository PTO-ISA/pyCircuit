cmake_minimum_required(VERSION 3.25)
project(PycircuitSelectPipeline LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(select_pipeline
  PACKAGE example_select_pipeline
  TOP example_select_pipeline.select_pipeline.SelectPipeline
  ENTRY_SOURCE select_pipeline
  SOURCES select_pipeline.py
  SYSTEM_TOP example_select_pipeline.bench.ExerciseSelectPipeline
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 1829
  SYSTEM_SOURCES select_pipeline.py bench.py)
if(PYC_EXAMPLE_VERIFY)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(IVERILOG_EXECUTABLE iverilog REQUIRED)
find_program(VVP_EXECUTABLE vvp REQUIRED)
add_test(NAME select_pipeline_four_state
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/four_state.py"
    --source "${CMAKE_CURRENT_SOURCE_DIR}"
    --build "${CMAKE_CURRENT_BINARY_DIR}"
    --include "${PYCIRCUIT_RUNTIME_INCLUDE_DIR}"
    --scratch "${CMAKE_CURRENT_BINARY_DIR}/four-state"
    --iverilog "${IVERILOG_EXECUTABLE}"
    --vvp "${VVP_EXECUTABLE}")
set_tests_properties(select_pipeline_four_state PROPERTIES DEPENDS select_pipeline)

endif()
