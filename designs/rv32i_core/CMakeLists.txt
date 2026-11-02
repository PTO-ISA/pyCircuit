cmake_minimum_required(VERSION 3.25)
project(PycircuitRV32I LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(rv32i_core PACKAGE rv32i_core TOP rv32i_core.core.Core
  ENTRY_SOURCE core SOURCES types.py decoder.py arithmetic.py writeback.py memory.py execute.py frontend.py core.py
  MODEL_SOURCES types.py decoder.py arithmetic.py writeback.py memory.py execute.py frontend.py core.py CYCLES 32)
