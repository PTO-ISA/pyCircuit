include_guard(GLOBAL)

set(ACIR_REQUIRED_LLVM_VERSION "22.1.8")

function(acir_configure_llvm)
  if(TARGET MLIRIR)
    if(NOT LLVM_PACKAGE_VERSION)
      message(FATAL_ERROR
        "the parent project supplied LLVM targets without LLVM_PACKAGE_VERSION")
    endif()
    message(STATUS
      "Reusing parent LLVM/MLIR ${LLVM_PACKAGE_VERSION} targets for Agentic Circuit")
  else()
    if(NOT MLIR_DIR)
      message(FATAL_ERROR "MLIR_DIR is required and must select MLIR 22.1.8")
    endif()

    get_filename_component(_llvm_cmake_dir "${MLIR_DIR}/../llvm" ABSOLUTE)
    if(NOT LLVM_DIR)
      set(LLVM_DIR "${_llvm_cmake_dir}" CACHE PATH "LLVM CMake package directory")
    endif()

    find_package(LLVM ${ACIR_REQUIRED_LLVM_VERSION} EXACT CONFIG REQUIRED)
    find_package(MLIR ${ACIR_REQUIRED_LLVM_VERSION} EXACT CONFIG REQUIRED)
  endif()

  if(NOT LLVM_PACKAGE_VERSION STREQUAL ACIR_REQUIRED_LLVM_VERSION)
    message(FATAL_ERROR
      "LLVM ${ACIR_REQUIRED_LLVM_VERSION} is required; found ${LLVM_PACKAGE_VERSION}"
    )
  endif()

  foreach(_package_variable
      LLVM_DEFINITIONS
      LLVM_INCLUDE_DIRS
      LLVM_TOOLS_BINARY_DIR
      MLIR_INCLUDE_DIRS
      MLIR_TABLEGEN_EXE)
    set(${_package_variable} "${${_package_variable}}" PARENT_SCOPE)
  endforeach()
  message(STATUS "Using LLVM ${LLVM_PACKAGE_VERSION} from ${LLVM_DIR}")
  message(STATUS "Using MLIR ${ACIR_REQUIRED_LLVM_VERSION} from ${MLIR_DIR}")
endfunction()
