cmake_minimum_required(VERSION 3.25)
project(PycircuitMulticlockRegs LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(multiclock_regs
  PACKAGE example_multiclock_regs TOP example_multiclock_regs.multiclock_regs.MulticlockRegs
  ENTRY_SOURCE multiclock_regs SOURCES multiclock_regs.py)
