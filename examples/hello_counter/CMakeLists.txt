cmake_minimum_required(VERSION 3.25)
project(PycircuitHelloCounter LANGUAGES CXX)

find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")

pycircuit_add_example(hello_counter
  PACKAGE example_hello_counter
  TOP example_hello_counter.hello_counter.HelloCounter
  ENTRY_SOURCE hello_counter
  SOURCES hello_counter.py)
