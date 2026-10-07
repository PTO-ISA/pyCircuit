cmake_minimum_required(VERSION 3.25)
project(PycircuitBitfieldDecodePipeline LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(bitfield_decode_pipeline
  PACKAGE example_bitfield_decode_pipeline
  TOP example_bitfield_decode_pipeline.bitfield_decode_pipeline.BitfieldDecodePipeline
  ENTRY_SOURCE bitfield_decode_pipeline
  SOURCES bitfield_decode_pipeline.py)
if(PYC_EXAMPLE_VERIFY)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(IVERILOG_EXECUTABLE iverilog REQUIRED)
find_program(VVP_EXECUTABLE vvp REQUIRED)
add_test(NAME bitfield_decode_pipeline_four_state
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/four_state.py"
    --source "${CMAKE_CURRENT_SOURCE_DIR}"
    --build "${CMAKE_CURRENT_BINARY_DIR}"
    --include "${PYCIRCUIT_RUNTIME_INCLUDE_DIR}"
    --scratch "${CMAKE_CURRENT_BINARY_DIR}/four-state"
    --iverilog "${IVERILOG_EXECUTABLE}"
    --vvp "${VVP_EXECUTABLE}")
set_tests_properties(bitfield_decode_pipeline_four_state PROPERTIES DEPENDS bitfield_decode_pipeline)

endif()
