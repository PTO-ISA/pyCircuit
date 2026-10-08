cmake_minimum_required(VERSION 3.25)
project(PycircuitCacheParams LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(cache_params
  PACKAGE example_cache_params TOP example_cache_params.cache_params.CacheParams
  ENTRY_SOURCE cache_params SOURCES cache_params.py
  SYSTEM_TOP example_cache_params.bench.ExerciseCacheParams
  SYSTEM_ENTRY_SOURCE bench
  SYSTEM_CYCLES 14
  SYSTEM_SOURCES cache_params.py bench.py)
