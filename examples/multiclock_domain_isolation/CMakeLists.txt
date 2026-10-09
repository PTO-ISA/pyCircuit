cmake_minimum_required(VERSION 3.25)
project(PycircuitMulticlockDomainIsolation LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(multiclock_domain_isolation
  PACKAGE example_multiclock_domain_isolation
  TOP example_multiclock_domain_isolation.multiclock_domain_isolation.MulticlockDomainIsolation
  ENTRY_SOURCE multiclock_domain_isolation
  SOURCES counter_lane.py multiclock_domain_isolation.py)
