cmake_minimum_required(VERSION 3.25)

project(AgenticCircuitRuntimeConsumer VERSION 0.1.0 LANGUAGES CXX)

find_package(AgenticCircuit 0.1.0 EXACT CONFIG REQUIRED COMPONENTS Runtime)

if(TARGET AgenticCircuit::GfsimTooling)
  message(FATAL_ERROR
    "Runtime component unexpectedly exposes AgenticCircuit::GfsimTooling")
endif()
if(TARGET AgenticCircuit::ACIRBindings)
  message(FATAL_ERROR
    "Runtime component unexpectedly exposes AgenticCircuit::ACIRBindings")
endif()

add_executable(runtime-consumer main.cpp)
target_compile_features(runtime-consumer PRIVATE cxx_std_20)
set_target_properties(runtime-consumer PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(runtime-consumer PRIVATE AgenticCircuit::Gfsim)
