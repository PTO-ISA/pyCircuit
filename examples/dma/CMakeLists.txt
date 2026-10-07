cmake_minimum_required(VERSION 3.25)
project(PycircuitDma LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
# Consumer example executable; not part of the framework or SDK exports.
pycircuit_add_example(dma PACKAGE example_dma
  TOP example_dma.dma.Dma
  ENTRY_SOURCE dma SOURCES dma.py)
