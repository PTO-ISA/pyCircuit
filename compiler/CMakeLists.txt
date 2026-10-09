cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(pycircuit VERSION 6.1.0 LANGUAGES C CXX)
endif()

set(PYCIRCUIT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(PYCIRCUIT_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
get_filename_component(PYCIRCUIT_REPO_ROOT "${PYCIRCUIT_SOURCE_DIR}/.." ABSOLUTE)

if(NOT TARGET MLIRIR)
  find_package(LLVM 22.1.8 EXACT CONFIG REQUIRED)
  find_package(MLIR 22.1.8 EXACT CONFIG REQUIRED)
endif()

list(APPEND CMAKE_MODULE_PATH "${LLVM_DIR}" "${MLIR_DIR}"
     "${PYCIRCUIT_SOURCE_DIR}/cmake")
include(TableGen)
include(AddLLVM)
include(AddMLIR)
include(AddACIR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS} ${MLIR_INCLUDE_DIRS})
include_directories("${PYCIRCUIT_SOURCE_DIR}/include" "${PYCIRCUIT_BINARY_DIR}/include")
add_definitions(${LLVM_DEFINITIONS})

add_library(pycircuit_project_options INTERFACE)
target_compile_features(pycircuit_project_options INTERFACE cxx_std_20)
# Compiler consumers must match LLVM's RTTI configuration. Keep this on the
# compiler interface so standalone Runtime targets retain their own policy.
if(NOT LLVM_ENABLE_RTTI)
  if(MSVC)
    target_compile_options(pycircuit_project_options INTERFACE "$<$<COMPILE_LANGUAGE:CXX>:/GR->")
  else()
    target_compile_options(pycircuit_project_options INTERFACE "$<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>")
  endif()
endif()

option(PYCIRCUIT_BUILD_TESTING "Build isolated source-unit tests" OFF)
if(PYCIRCUIT_BUILD_TESTING)
  include(CTest)
  enable_testing()
endif()

add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(tools)

if(PYCIRCUIT_BUILD_TESTING)
  add_subdirectory(
    "${PYCIRCUIT_REPO_ROOT}/tests/compiler/IR"
    "${PYCIRCUIT_BINARY_DIR}/tests/source-unit"
  )
endif()
