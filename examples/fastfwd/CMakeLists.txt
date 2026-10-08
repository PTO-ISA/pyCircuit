cmake_minimum_required(VERSION 3.25)
project(PycircuitFastfwd LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(fastfwd PACKAGE example_fastfwd TOP example_fastfwd.fastfwd.Fastfwd
  ENTRY_SOURCE fastfwd SOURCES fastfwd.py
  SYSTEM_TOP example_fastfwd.bench.FastfwdSystem
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 24
  SYSTEM_SOURCES fastfwd.py bench.py)
