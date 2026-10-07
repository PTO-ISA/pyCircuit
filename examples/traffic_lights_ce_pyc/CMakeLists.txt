cmake_minimum_required(VERSION 3.25)
project(PycircuitTrafficLights LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(traffic_lights_ce_pyc PACKAGE example_traffic_lights_ce_pyc
  TOP example_traffic_lights_ce_pyc.traffic_lights_ce_pyc.TopTrafficLights
  ENTRY_SOURCE traffic_lights_ce_pyc SOURCES traffic_lights_ce_pyc.py)
if(PYC_EXAMPLE_VERIFY)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME traffic_lights_encoder
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/verify_encoder.py"
    --driver "${PYCIRCUIT_DRIVER_EXECUTABLE}"
    --unit "${CMAKE_CURRENT_BINARY_DIR}/units/traffic_lights_ce_pyc"
    --source "${CMAKE_CURRENT_SOURCE_DIR}"
    --build "${CMAKE_CURRENT_BINARY_DIR}/encoder-check"
    --include "${PYCIRCUIT_RUNTIME_INCLUDE_DIR}"
    --runtime "$<TARGET_FILE:pycircuit::pyc6_runtime>"
    --cxx "${CMAKE_CXX_COMPILER}"
    --verilator "${VERILATOR_EXECUTABLE}")

endif()
