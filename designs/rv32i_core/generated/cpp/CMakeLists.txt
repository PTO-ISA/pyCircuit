cmake_minimum_required(VERSION 3.25)
project(PycircuitModules LANGUAGES C CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
add_library(pycircuit_modules STATIC
  "sources/rv32i_core/core.cpp"
  "sources/rv32i_core/execute.cpp"
  "sources/rv32i_core/frontend.cpp"
  "sources/rv32i_core/memory.cpp"
  "sources/rv32i_core/writeback.cpp"
 )
target_include_directories(pycircuit_modules PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(pycircuit_modules PUBLIC pycircuit::pyc6_runtime)
