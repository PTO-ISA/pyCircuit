cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(AgenticCircuitSourceUnits VERSION 0.1.0 LANGUAGES C CXX)
endif()

set(ACIR_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(ACIR_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
get_filename_component(ACIR_REPO_ROOT "${ACIR_SOURCE_DIR}/../.." ABSOLUTE)

if(NOT TARGET MLIRIR)
  find_package(LLVM 22.1.8 EXACT CONFIG REQUIRED)
  find_package(MLIR 22.1.8 EXACT CONFIG REQUIRED)
endif()

list(APPEND CMAKE_MODULE_PATH "${LLVM_DIR}" "${MLIR_DIR}"
     "${ACIR_SOURCE_DIR}/cmake/modules")
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
include_directories("${ACIR_SOURCE_DIR}/include" "${ACIR_BINARY_DIR}/include")
add_definitions(${LLVM_DEFINITIONS})

add_library(acir_project_options INTERFACE)
add_library(AgenticCircuit::ProjectOptions ALIAS acir_project_options)
target_compile_features(acir_project_options INTERFACE cxx_std_20)

option(ACIR_BUILD_TESTING "Build isolated source-unit tests" OFF)
if(ACIR_BUILD_TESTING)
  include(CTest)
  enable_testing()
endif()

add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(tools)

if(ACIR_BUILD_TESTING)
  add_subdirectory(
    "${ACIR_REPO_ROOT}/tests/cpp/agentic-circuit/Dialect/ACIR"
    "${ACIR_BINARY_DIR}/tests/source-unit"
  )
endif()
