cmake_minimum_required(VERSION 3.25)
project(PycircuitModules LANGUAGES C CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
add_library(pycircuit_modules STATIC
 )
target_include_directories(pycircuit_modules PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(pycircuit_modules PUBLIC pycircuit::pyc6_runtime)
find_package(verilator REQUIRED)
verilate(pycircuit_modules SOURCES
  "${PYCIRCUIT_RUNTIME_INCLUDE_DIR}/verilog/dff.v"
  "${PYCIRCUIT_RUNTIME_INCLUDE_DIR}/verilog/dffe.v"
  "design_top.sv"
  "sources/rv32i_core/core.v"
  "sources/rv32i_core/execute.v"
  "sources/rv32i_core/frontend.v"
  "sources/rv32i_core/memory.v"
  "sources/rv32i_core/writeback.v"
  TOP_MODULE "pyc_root"
  PREFIX Vpycircuit_modules
  VERILATOR_ARGS --Wno-fatal)
