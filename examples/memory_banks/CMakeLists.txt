cmake_minimum_required(VERSION 3.25)
project(PycircuitMemoryBanks LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
# Consumer example executable; not part of the framework or SDK exports.
pycircuit_add_example(memory_banks PACKAGE example_memory_banks
  TOP example_memory_banks.memory_banks.MemoryBanks
  ENTRY_SOURCE memory_banks SOURCES memory_banks.py)
