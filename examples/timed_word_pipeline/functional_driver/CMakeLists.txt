cmake_minimum_required(VERSION 3.25)
project(PycircuitAdmissionProbe LANGUAGES CXX)

find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
if(NOT PYCIRCUIT_GENERATED_CPP OR
   NOT EXISTS "${PYCIRCUIT_GENERATED_CPP}/CMakeLists.txt")
  message(FATAL_ERROR "Set PYCIRCUIT_GENERATED_CPP to an emitted execution entry")
endif()

add_subdirectory("${PYCIRCUIT_GENERATED_CPP}" generated)
add_executable(functional_projection functional_projection.cpp)
target_compile_features(functional_projection PRIVATE cxx_std_20)
target_include_directories(functional_projection PRIVATE "${PYCIRCUIT_GENERATED_CPP}")
target_link_libraries(functional_projection PRIVATE pycircuit_modules pycircuit::pyc6_runtime)
option(PYC_PROBE_TIMING "Probe the timing entry instead of the functional entry" OFF)
if(PYC_PROBE_TIMING)
  target_compile_definitions(functional_projection PRIVATE PYC_TIMING)
endif()
