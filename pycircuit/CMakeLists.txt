cmake_minimum_required(VERSION 3.20)
project(acpy_mlir_pipeline LANGUAGES C CXX)
add_subdirectory(mlir)
include(CTest)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
get_filename_component(REPO "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
if(BUILD_TESTING)
  add_test(NAME acpy-mlir-pipeline COMMAND ${CMAKE_COMMAND} -E env
    "ACPY_MLIR_OPT=$<TARGET_FILE:acir-opt>" "ACPY_MLIR_COMPILER=$<TARGET_FILE:acir-compile>"
    ${Python3_EXECUTABLE} -m unittest discover -s pycircuit/tests -v)
  set_tests_properties(acpy-mlir-pipeline PROPERTIES WORKING_DIRECTORY "${REPO}" TIMEOUT 1200)
endif()
