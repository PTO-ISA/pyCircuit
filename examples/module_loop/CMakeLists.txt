cmake_minimum_required(VERSION 3.25)
project(PycircuitModuleLoop LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(module_loop
  PACKAGE example_loop TOP example_loop.module_loop.Top ENTRY_SOURCE module_loop
  SOURCES child.py module_loop.py RUNNER pycircuit_system)
