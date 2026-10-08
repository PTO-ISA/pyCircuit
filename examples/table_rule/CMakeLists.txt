cmake_minimum_required(VERSION 3.25)
project(PycircuitTableRule LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(table_rule
  PACKAGE example_table_rule
  TOP example_table_rule.table_rule.TableRule
  ENTRY_SOURCE table_rule
  SOURCES table_rule.py
  SYSTEM_TOP example_table_rule.bench.ExerciseTableRule
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 1343
  SYSTEM_SOURCES table_rule.py bench.py)
if(PYC_EXAMPLE_VERIFY)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(IVERILOG_EXECUTABLE iverilog REQUIRED)
find_program(VVP_EXECUTABLE vvp REQUIRED)
add_test(NAME table_rule_four_state
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/four_state.py"
    --source "${CMAKE_CURRENT_SOURCE_DIR}"
    --build "${CMAKE_CURRENT_BINARY_DIR}"
    --include "${PYCIRCUIT_RUNTIME_INCLUDE_DIR}"
    --scratch "${CMAKE_CURRENT_BINARY_DIR}/four-state"
    --iverilog "${IVERILOG_EXECUTABLE}"
    --vvp "${VVP_EXECUTABLE}")
set_tests_properties(table_rule_four_state PROPERTIES DEPENDS table_rule)

endif()
