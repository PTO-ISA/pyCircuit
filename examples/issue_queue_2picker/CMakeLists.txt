cmake_minimum_required(VERSION 3.25)
project(PycircuitIssueQueue2Picker LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(issue_queue_2picker
  PACKAGE example_issue_queue_2picker TOP example_issue_queue_2picker.issue_queue_2picker.IssueQueue2Picker
  ENTRY_SOURCE issue_queue_2picker SOURCES issue_queue_2picker.py)
