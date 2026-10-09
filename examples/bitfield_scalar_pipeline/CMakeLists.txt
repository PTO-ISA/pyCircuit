cmake_minimum_required(VERSION 3.25)
project(PycircuitBitfieldScalarPipeline LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(bitfield_scalar_pipeline
  PACKAGE example_bitfield_scalar_pipeline
  TOP example_bitfield_scalar_pipeline.bitfield_scalar_pipeline.BitfieldScalarPipeline
  ENTRY_SOURCE bitfield_scalar_pipeline
  SOURCES bitfield_scalar_pipeline.py
  SYSTEM_TOP example_bitfield_scalar_pipeline.bench.BitfieldScalarPipelineSystem
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 158
  SYSTEM_SOURCES bitfield_scalar_pipeline.py bench.py)
if(PYC_EXAMPLE_VERIFY)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(IVERILOG_EXECUTABLE iverilog REQUIRED)
find_program(VVP_EXECUTABLE vvp REQUIRED)
add_test(NAME bitfield_scalar_pipeline_four_state
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/four_state.py"
    --source "${CMAKE_CURRENT_SOURCE_DIR}"
    --build "${CMAKE_CURRENT_BINARY_DIR}"
    --include "${PYCIRCUIT_RUNTIME_INCLUDE_DIR}"
    --scratch "${CMAKE_CURRENT_BINARY_DIR}/four-state"
    --iverilog "${IVERILOG_EXECUTABLE}"
    --vvp "${VVP_EXECUTABLE}")
set_tests_properties(bitfield_scalar_pipeline_four_state PROPERTIES DEPENDS bitfield_scalar_pipeline)

endif()
