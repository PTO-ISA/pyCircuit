cmake_minimum_required(VERSION 3.25)
project(PycircuitDigitalClock LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(digital_clock PACKAGE example_digital_clock
  TOP example_digital_clock.digital_clock.DigitalClock
  ENTRY_SOURCE digital_clock TIMEOUT_SECONDS 626 SOURCES digital_clock.py
  SYSTEM_TOP example_digital_clock.bench.ExerciseDigitalClock
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 100000001
  SYSTEM_SOURCES digital_clock.py bench.py)
if(PYC_EXAMPLE_VERIFY)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME digital_clock_encoder
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/verify_encoder.py"
    --driver "${PYCIRCUIT_DRIVER_EXECUTABLE}"
    --unit "${CMAKE_CURRENT_BINARY_DIR}/units/digital_clock"
    --source "${CMAKE_CURRENT_SOURCE_DIR}"
    --build "${CMAKE_CURRENT_BINARY_DIR}/encoder-check"
    --include "${PYCIRCUIT_RUNTIME_INCLUDE_DIR}"
    --runtime "$<TARGET_FILE:pycircuit::pyc6_runtime>"
    --cxx "${CMAKE_CXX_COMPILER}"
    --verilator "${VERILATOR_EXECUTABLE}")

endif()
