cmake_minimum_required(VERSION 3.25)
project(PycircuitMultiInputRulePipeline LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(multi_input_rule_pipeline
  PACKAGE example_multi_input_rule_pipeline
  TOP example_multi_input_rule_pipeline.multi_input_rule_pipeline.MultiInputRulePipeline
  ENTRY_SOURCE multi_input_rule_pipeline
  SOURCES multi_input_rule_pipeline.py
  SYSTEM_TOP example_multi_input_rule_pipeline.bench.MultiInputRulePipelineSystem
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 158
  SYSTEM_SOURCES multi_input_rule_pipeline.py bench.py)
if(PYC_EXAMPLE_VERIFY)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(IVERILOG_EXECUTABLE iverilog REQUIRED)
find_program(VVP_EXECUTABLE vvp REQUIRED)
add_test(NAME multi_input_rule_pipeline_four_state
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/four_state.py"
    --source "${CMAKE_CURRENT_SOURCE_DIR}"
    --build "${CMAKE_CURRENT_BINARY_DIR}"
    --include "${PYCIRCUIT_RUNTIME_INCLUDE_DIR}"
    --scratch "${CMAKE_CURRENT_BINARY_DIR}/four-state"
    --iverilog "${IVERILOG_EXECUTABLE}"
    --vvp "${VVP_EXECUTABLE}")
set_tests_properties(multi_input_rule_pipeline_four_state PROPERTIES DEPENDS multi_input_rule_pipeline)

endif()
