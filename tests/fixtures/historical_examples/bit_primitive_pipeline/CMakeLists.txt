cmake_minimum_required(VERSION 3.25)
project(PycircuitBitPrimitivePipeline LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(bit_primitive_pipeline
  PACKAGE example_bit_primitive_pipeline
  TOP example_bit_primitive_pipeline.bit_primitive_pipeline.BitPrimitivePipeline
  ENTRY_SOURCE bit_primitive_pipeline
  SOURCES bit_primitive_pipeline.py
  SYSTEM_TOP example_bit_primitive_pipeline.bench.BitPrimitivePipelineSystem
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 344
  SYSTEM_SOURCES bit_primitive_pipeline.py bench.py)
if(PYC_EXAMPLE_VERIFY)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(IVERILOG_EXECUTABLE iverilog REQUIRED)
find_program(VVP_EXECUTABLE vvp REQUIRED)
add_test(NAME bit_primitive_pipeline_four_state
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/four_state.py"
    --source "${CMAKE_CURRENT_SOURCE_DIR}"
    --build "${CMAKE_CURRENT_BINARY_DIR}"
    --include "${PYCIRCUIT_RUNTIME_INCLUDE_DIR}"
    --scratch "${CMAKE_CURRENT_BINARY_DIR}/four-state"
    --iverilog "${IVERILOG_EXECUTABLE}"
    --vvp "${VVP_EXECUTABLE}")
set_tests_properties(bit_primitive_pipeline_four_state PROPERTIES DEPENDS bit_primitive_pipeline)

endif()
