cmake_minimum_required(VERSION 3.25)
project(PycircuitEncodedEnumPipeline LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(encoded_enum_pipeline
  PACKAGE example_encoded_enum_pipeline
  TOP example_encoded_enum_pipeline.encoded_enum_pipeline.EncodedEnumPipeline
  ENTRY_SOURCE encoded_enum_pipeline
  SOURCES encoded_enum_pipeline.py
  SYSTEM_TOP example_encoded_enum_pipeline.bench.EncodedEnumPipelineSystem
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 184
  SYSTEM_SOURCES encoded_enum_pipeline.py bench.py)
if(PYC_EXAMPLE_VERIFY)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(IVERILOG_EXECUTABLE iverilog REQUIRED)
find_program(VVP_EXECUTABLE vvp REQUIRED)
add_test(NAME encoded_enum_pipeline_four_state
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/four_state.py"
    --source "${CMAKE_CURRENT_SOURCE_DIR}"
    --build "${CMAKE_CURRENT_BINARY_DIR}"
    --include "${PYCIRCUIT_RUNTIME_INCLUDE_DIR}"
    --scratch "${CMAKE_CURRENT_BINARY_DIR}/four-state"
    --iverilog "${IVERILOG_EXECUTABLE}"
    --vvp "${VVP_EXECUTABLE}")
set_tests_properties(encoded_enum_pipeline_four_state PROPERTIES DEPENDS encoded_enum_pipeline)

endif()
