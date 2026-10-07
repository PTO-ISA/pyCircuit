cmake_minimum_required(VERSION 3.25)
project(PycircuitFastfwd LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(fastfwd PACKAGE example_fastfwd TOP example_fastfwd.fastfwd.Fastfwd
  ENTRY_SOURCE fastfwd SOURCES fastfwd.py)
