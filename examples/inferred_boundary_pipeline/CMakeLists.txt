cmake_minimum_required(VERSION 3.25)
project(PycircuitInferredBoundaryPipeline LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(inferred_boundary_pipeline
  PACKAGE example_inferred_boundary_pipeline
  TOP example_inferred_boundary_pipeline.inferred_boundary_pipeline.InferredBoundaryPipeline
  ENTRY_SOURCE inferred_boundary_pipeline
  SOURCES inferred_boundary_pipeline.py)
if(PYC_EXAMPLE_VERIFY)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(IVERILOG_EXECUTABLE iverilog REQUIRED)
find_program(VVP_EXECUTABLE vvp REQUIRED)
add_test(NAME inferred_boundary_pipeline_four_state
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/four_state.py"
    --source "${CMAKE_CURRENT_SOURCE_DIR}"
    --build "${CMAKE_CURRENT_BINARY_DIR}"
    --include "${PYCIRCUIT_RUNTIME_INCLUDE_DIR}"
    --scratch "${CMAKE_CURRENT_BINARY_DIR}/four-state"
    --iverilog "${IVERILOG_EXECUTABLE}"
    --vvp "${VVP_EXECUTABLE}")
set_tests_properties(inferred_boundary_pipeline_four_state PROPERTIES DEPENDS inferred_boundary_pipeline)

endif()
