cmake_minimum_required(VERSION 3.25)
project(PycircuitRuntimeConsumer LANGUAGES CXX)

find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
if(TARGET pycircuit::ACIRSourceCompiler OR TARGET pycircuit::ACIRDialect)
  message(FATAL_ERROR "Runtime package unexpectedly exposes compiler targets")
endif()

add_executable(runtime_consumer main.cpp)
target_compile_features(runtime_consumer PRIVATE cxx_std_20)
target_link_libraries(runtime_consumer PRIVATE pycircuit::pyc6_runtime)
