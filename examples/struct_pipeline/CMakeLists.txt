cmake_minimum_required(VERSION 3.25)
project(PycircuitStructPipeline LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(struct_pipeline
  PACKAGE example_struct_pipeline
  TOP example_struct_pipeline.struct_pipeline.StructPipeline
  ENTRY_SOURCE struct_pipeline
  SOURCES struct_pipeline.py
  SYSTEM_TOP example_struct_pipeline.bench.StructPipelineSystem
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 184
  SYSTEM_SOURCES struct_pipeline.py bench.py)
if(PYC_EXAMPLE_VERIFY)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(IVERILOG_EXECUTABLE iverilog REQUIRED)
find_program(VVP_EXECUTABLE vvp REQUIRED)
add_test(NAME struct_pipeline_four_state
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/four_state.py"
    --source "${CMAKE_CURRENT_SOURCE_DIR}"
    --build "${CMAKE_CURRENT_BINARY_DIR}"
    --include "${PYCIRCUIT_RUNTIME_INCLUDE_DIR}"
    --scratch "${CMAKE_CURRENT_BINARY_DIR}/four-state"
    --iverilog "${IVERILOG_EXECUTABLE}"
    --vvp "${VVP_EXECUTABLE}")
set_tests_properties(struct_pipeline_four_state PROPERTIES DEPENDS struct_pipeline)

endif()
