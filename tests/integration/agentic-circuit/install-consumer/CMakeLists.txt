cmake_minimum_required(VERSION 3.25)

project(AgenticCircuitInstallConsumer VERSION 0.1.0 LANGUAGES C CXX)

# This consumer configures against the installed AgenticCircuit package only.
# It must not see source-tree or build-tree include paths; CI verifies that
# CMAKE_CURRENT_SOURCE_DIR stays this directory.
find_package(AgenticCircuit 0.1.0 EXACT CONFIG REQUIRED)

execute_process(
  COMMAND "${AgenticCircuit_EXECUTABLE}" schema capabilities --json
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  RESULT_VARIABLE ACIR_SCHEMA_RESULT
  OUTPUT_VARIABLE ACIR_SCHEMA_OUTPUT
  ERROR_VARIABLE ACIR_SCHEMA_ERROR
)
if(NOT ACIR_SCHEMA_RESULT EQUAL 0)
  message(FATAL_ERROR
    "installed schema capabilities failed (${ACIR_SCHEMA_RESULT}): "
    "${ACIR_SCHEMA_ERROR}${ACIR_SCHEMA_OUTPUT}")
endif()

execute_process(
  COMMAND "${AgenticCircuit_EXECUTABLE}" check architecture.py
          --project "${CMAKE_CURRENT_SOURCE_DIR}/agentic-circuit.toml" --json
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  RESULT_VARIABLE ACIR_CHECK_RESULT
  OUTPUT_VARIABLE ACIR_CHECK_OUTPUT
  ERROR_VARIABLE ACIR_CHECK_ERROR
)
if(NOT ACIR_CHECK_RESULT EQUAL 0)
  message(FATAL_ERROR
    "installed architecture check failed (${ACIR_CHECK_RESULT}): "
    "${ACIR_CHECK_ERROR}${ACIR_CHECK_OUTPUT}")
endif()

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS} ${MLIR_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(process-state-plan-consumer main.cpp)
target_compile_features(process-state-plan-consumer PRIVATE cxx_std_20)
set_target_properties(process-state-plan-consumer PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(
  process-state-plan-consumer
  PRIVATE
    AgenticCircuit::ACIRAnalysis
    MLIRParser
)
