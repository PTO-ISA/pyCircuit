cmake_minimum_required(VERSION 3.25)
project(PycircuitStructTransform LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(struct_transform PACKAGE example_struct_transform TOP example_struct_transform.struct_transform.StructTransform
  ENTRY_SOURCE struct_transform SOURCES struct_transform.py
  SYSTEM_TOP example_struct_transform.bench.ExerciseStructTransform
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 72
  SYSTEM_SOURCES struct_transform.py bench.py)
