cmake_minimum_required(VERSION 3.25)
project(PycircuitFifoLoopback LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(fifo_loopback
  PACKAGE example_fifo_loopback
  TOP example_fifo_loopback.fifo_loopback.FifoLoopback
  ENTRY_SOURCE fifo_loopback
  SOURCES fifo_loopback.py
  SYSTEM_TOP example_fifo_loopback.bench.FifoLoopbackSystem
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 344
  SYSTEM_SOURCES fifo_loopback.py bench.py)
if(PYC_EXAMPLE_VERIFY)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(IVERILOG_EXECUTABLE iverilog REQUIRED)
find_program(VVP_EXECUTABLE vvp REQUIRED)
add_test(NAME fifo_loopback_four_state
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/four_state.py"
    --source "${CMAKE_CURRENT_SOURCE_DIR}"
    --build "${CMAKE_CURRENT_BINARY_DIR}"
    --include "${PYCIRCUIT_RUNTIME_INCLUDE_DIR}"
    --scratch "${CMAKE_CURRENT_BINARY_DIR}/four-state"
    --iverilog "${IVERILOG_EXECUTABLE}"
    --vvp "${VVP_EXECUTABLE}")
set_tests_properties(fifo_loopback_four_state PROPERTIES DEPENDS fifo_loopback)

endif()
