cmake_minimum_required(VERSION 3.20)
project(pycircuit VERSION 6.0.0 LANGUAGES C CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(PYC_BUILD_MLIR_TOOLS "Build MLIR-based pyc tools" ON)
option(PYC_BUILD_RUNTIME_LIB "Build pyc6 runtime library" ON)
option(PYC_RUNTIME_ENABLE_ZLIB_TRACE "Enable gzip trace output in runtime (optional)" OFF)
option(PYC_RUNTIME_BUILD_SHARED "Build shared pyc6 runtime library" OFF)
option(PYC_INSTALL_TEMPLATES "Install runtime template libraries" ON)
option(PYC_INSTALL_PYTHON "Install Python frontend sources (for binary packages)" ON)
option(PYC_INSTALL_CMAKE_PACKAGE "Install pycircuit CMake package metadata" ON)
option(PYC_PACKAGE_BUNDLE_RUNTIME_DEPS "Bundle non-system runtime deps for installed pyc tools" ON)

if(APPLE)
  set(PYC_INSTALL_RPATH "@loader_path/../${CMAKE_INSTALL_LIBDIR}")
else()
  set(PYC_INSTALL_RPATH "\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
endif()

set(PYC_LLVM_VERSION "unknown")
set(PYC_TOOLCHAIN_GIT_SHA "unknown")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
  execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE PYC_TOOLCHAIN_GIT_SHA
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  if(NOT PYC_TOOLCHAIN_GIT_SHA)
    set(PYC_TOOLCHAIN_GIT_SHA "unknown")
  endif()
endif()

if(PYC_BUILD_MLIR_TOOLS)
  add_subdirectory(compiler/mlir)
endif()

if(PYC_BUILD_RUNTIME_LIB)
  add_subdirectory(runtime/cpp)
endif()

if(PYC_INSTALL_TEMPLATES)
  install(
    DIRECTORY runtime/cpp
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    PATTERN "CMakeLists.txt" EXCLUDE
    PATTERN "*.cpp" EXCLUDE
  )
  install(DIRECTORY runtime/verilog DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
  if(PYC_BUILD_RUNTIME_LIB)
    set(PYC_RUNTIME_INSTALL_TARGETS pyc6_runtime)
    if(PYC_RUNTIME_BUILD_SHARED)
      list(APPEND PYC_RUNTIME_INSTALL_TARGETS pyc6_runtime_shared)
    endif()
    install(TARGETS ${PYC_RUNTIME_INSTALL_TARGETS}
      EXPORT pycircuitTargets
      ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
      LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
  endif()
endif()

if(PYC_INSTALL_PYTHON)
  install(
    DIRECTORY compiler/frontend/pycircuit
    DESTINATION "${CMAKE_INSTALL_DATADIR}/pycircuit/python"
    PATTERN "__pycache__" EXCLUDE
    PATTERN "*.pyc" EXCLUDE
  )
  install(FILES pyproject.toml DESTINATION "${CMAKE_INSTALL_DATADIR}/pycircuit")
endif()

install(FILES LICENSE README.md DESTINATION "${CMAKE_INSTALL_DATADIR}/pycircuit")

set(PYC_TOOLCHAIN_METADATA_PATH "${CMAKE_CURRENT_BINARY_DIR}/toolchain-metadata.json")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain-metadata.json.in"
  "${PYC_TOOLCHAIN_METADATA_PATH}"
  @ONLY
)
install(FILES "${PYC_TOOLCHAIN_METADATA_PATH}" DESTINATION "${CMAKE_INSTALL_DATADIR}/pycircuit")

if(PYC_INSTALL_CMAKE_PACKAGE AND PYC_BUILD_RUNTIME_LIB)
  set(PYCIRCUIT_INSTALL_CONFIGDIR "${CMAKE_INSTALL_DATADIR}/pycircuit/cmake")
  set(PYCIRCUIT_PYCC_EXECUTABLE_RELATIVE "${CMAKE_INSTALL_BINDIR}/pycc")
  set(PYCIRCUIT_RUNTIME_INCLUDE_DIR_RELATIVE "${CMAKE_INSTALL_INCLUDEDIR}")
  set(PYCIRCUIT_PRIMITIVES_DIR_RELATIVE "${CMAKE_INSTALL_INCLUDEDIR}/verilog")
  set(PYCIRCUIT_PYTHON_DIR_RELATIVE "${CMAKE_INSTALL_DATADIR}/pycircuit/python")

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/pycircuitConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/pycircuitConfig.cmake"
    INSTALL_DESTINATION "${PYCIRCUIT_INSTALL_CONFIGDIR}"
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
  )
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/pycircuitConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
  )

  install(EXPORT pycircuitTargets
    FILE pycircuitTargets.cmake
    NAMESPACE pycircuit::
    DESTINATION "${PYCIRCUIT_INSTALL_CONFIGDIR}"
  )
  install(
    FILES
      "${CMAKE_CURRENT_BINARY_DIR}/pycircuitConfig.cmake"
      "${CMAKE_CURRENT_BINARY_DIR}/pycircuitConfigVersion.cmake"
    DESTINATION "${PYCIRCUIT_INSTALL_CONFIGDIR}"
  )
endif()

if(PYC_PACKAGE_BUNDLE_RUNTIME_DEPS AND PYC_BUILD_MLIR_TOOLS)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/install_runtime_deps.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/install_runtime_deps.cmake"
    @ONLY
  )
  install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/install_runtime_deps.cmake")
endif()

# Packaging (binary tarball via CPack).
set(CPACK_PACKAGE_NAME "pycircuit" CACHE STRING "CPack package name")
set(CPACK_PACKAGE_VENDOR "pyCircuit" CACHE STRING "CPack package vendor")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "pyCircuit: Python -> PYC (MLIR) -> Verilog/C++" CACHE STRING "CPack summary")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}" CACHE STRING "CPack package version")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_GENERATOR "TGZ" CACHE STRING "CPack generator")
include(CPack)
