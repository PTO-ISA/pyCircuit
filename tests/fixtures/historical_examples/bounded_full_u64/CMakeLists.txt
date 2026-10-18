cmake_minimum_required(VERSION 3.25)
project(PycircuitBoundedFullU64 LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(bounded_full_u64
  PACKAGE example_bounded_full_u64 TOP example_bounded_full_u64.bounded_full_u64.BoundedFullU64
  ENTRY_SOURCE bounded_full_u64 SOURCES bounded_full_u64.py
  SYSTEM_TOP example_bounded_full_u64.bench.BoundedFullU64System
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 158
  SYSTEM_SOURCES bounded_full_u64.py bench.py)
