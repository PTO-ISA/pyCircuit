cmake_minimum_required(VERSION 3.25)
project(PycircuitEnumHelpers LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(enum_helpers
  PACKAGE example_enum_helpers
  TOP example_enum_helpers.enum_helpers.EnumHelpers
  ENTRY_SOURCE enum_helpers
  SOURCES enum_helpers.py
  SYSTEM_TOP example_enum_helpers.bench.EnumHelpersSystem
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 184
  SYSTEM_SOURCES enum_helpers.py bench.py)
if(PYC_EXAMPLE_VERIFY)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(IVERILOG_EXECUTABLE iverilog REQUIRED)
find_program(VVP_EXECUTABLE vvp REQUIRED)
add_test(NAME enum_helpers_four_state
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/four_state.py"
    --source "${CMAKE_CURRENT_SOURCE_DIR}"
    --build "${CMAKE_CURRENT_BINARY_DIR}"
    --include "${PYCIRCUIT_RUNTIME_INCLUDE_DIR}"
    --scratch "${CMAKE_CURRENT_BINARY_DIR}/four-state"
    --iverilog "${IVERILOG_EXECUTABLE}"
    --vvp "${VVP_EXECUTABLE}")
set_tests_properties(enum_helpers_four_state PROPERTIES DEPENDS enum_helpers)

endif()
