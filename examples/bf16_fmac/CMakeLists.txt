cmake_minimum_required(VERSION 3.25)
project(PycircuitBF16Fmac LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(bf16_fmac
  PACKAGE example_bf16_fmac TOP example_bf16_fmac.bf16_fmac.BF16Fmac
  ENTRY_SOURCE bf16_fmac SOURCES bf16_fmac.py
  SYSTEM_TOP example_bf16_fmac.bench.ExerciseBF16Fmac
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 86
  SYSTEM_SOURCES bf16_fmac.py bench.py)
