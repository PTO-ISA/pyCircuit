cmake_minimum_required(VERSION 3.25)
project(pycircuit VERSION 6.1.0 LANGUAGES C CXX)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(PYC_BUILD_COMPILER_DEV "Build the native source compiler" ON)
option(PYC_BUILD_RUNTIME_LIB "Build the pyc6 simulation runtime" ON)
option(PYC_BUILD_TESTING "Build native compiler tests" OFF)
option(PYC_INSTALL_PYTHON "Install the Python capture and driver" "${PYC_BUILD_COMPILER_DEV}")
option(PYC_INSTALL_CMAKE_PACKAGE "Install pycircuit CMake package metadata" ON)
option(PYC_PACKAGE_BUNDLE_RUNTIME_DEPS "Bundle native compiler runtime dependencies" ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(APPLE)
  set(CMAKE_INSTALL_RPATH "@loader_path/../${CMAKE_INSTALL_LIBDIR}")
else()
  set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
endif()
if(PYC_BUILD_TESTING)
  include(CTest)
  enable_testing()
endif()
if(PYC_BUILD_RUNTIME_LIB)
  add_subdirectory(runtime)
endif()
if(PYC_BUILD_COMPILER_DEV)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  find_package(LLVM 22.1.8 EXACT CONFIG REQUIRED)
  find_package(MLIR 22.1.8 EXACT CONFIG REQUIRED)
  set(PYCIRCUIT_BUILD_TESTING "${PYC_BUILD_TESTING}" CACHE BOOL "Build native tests" FORCE)
  add_subdirectory(compiler)
  install(TARGETS ACIRDialect ACIRSourceCompiler ACIRCodegen ACIRTransforms pycircuit_project_options
    EXPORT pycircuitCompilerTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
  install(TARGETS pycircuit-source-unit pycircuit-link pycircuit-emit pycircuit-opt
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
  install(DIRECTORY compiler/include/pycircuit
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h" PATTERN "*.td")
  install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/compiler/include/pycircuit"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" FILES_MATCHING PATTERN "*.inc")
  install(DIRECTORY compiler/lib/Dialect/ACIR
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/Dialect" FILES_MATCHING PATTERN "*.h")
  # Install the active Compiler interface closure, not every historical header
  # kept for future semantic migration. An unbuilt header is not an SDK API.
  set(_pyc_compiler_headers
    FinalCppNames.h
    FinalCppSourceParts.h
    FinalEmitCpp.h
    FinalEmitVerilog.h
    FinalSourceMap.h
    FinalVerilogSourceParts.h
    HardwareEmitCommon.h
    PythonImportAST.h
    PythonImportContext.h
    PythonImportEnumSyntax.h
    PythonImportRecords.h
    SourceHeaderHelpers.h
    SourceDeclarationContext.h
    SourceMemoryBindings.h
    SourceIdentifierUnicodeData.h
    SourceLink.h
    SourceNamespace.h
    SourceRuleWrites.h
    SourceUnit.h
    SourceIdentifierUnicodeData.LICENSE.txt)
  list(TRANSFORM _pyc_compiler_headers PREPEND "compiler/lib/Compiler/")
  install(FILES ${_pyc_compiler_headers}
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/Compiler")
endif()
if(PYC_INSTALL_PYTHON)
  install(DIRECTORY python/pycircuit
    DESTINATION "${CMAKE_INSTALL_DATADIR}/pycircuit/python"
    PATTERN "__pycache__" EXCLUDE PATTERN "*.pyc" EXCLUDE)
  configure_file(cmake/pycircuit-driver.in "${CMAKE_CURRENT_BINARY_DIR}/pycircuit" @ONLY)
  install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/pycircuit"
    DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
install(FILES LICENSE README.md DESTINATION "${CMAKE_INSTALL_DATADIR}/pycircuit")

set(PYC_TOOLCHAIN_GIT_SHA "" CACHE STRING "External source revision for archive builds")
if(NOT PYC_TOOLCHAIN_GIT_SHA)
  execute_process(COMMAND git rev-parse HEAD WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE PYC_TOOLCHAIN_GIT_SHA OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
endif()
if(NOT PYC_TOOLCHAIN_GIT_SHA)
  set(PYC_TOOLCHAIN_GIT_SHA "unknown")
elseif(NOT PYC_TOOLCHAIN_GIT_SHA MATCHES "^[0-9a-f]+$")
  message(FATAL_ERROR "Source revision must be a lowercase Git commit SHA")
else()
  string(LENGTH "${PYC_TOOLCHAIN_GIT_SHA}" _revision_length)
  if(NOT _revision_length EQUAL 40)
    message(FATAL_ERROR "Source revision must be a full 40-character Git commit SHA")
  endif()
endif()
set(PYC_LLVM_VERSION_JSON "null")
if(PYC_BUILD_COMPILER_DEV)
  set(PYC_LLVM_VERSION_JSON "\"${LLVM_PACKAGE_VERSION}\"")
endif()
configure_file(cmake/toolchain-metadata.json.in toolchain-metadata.json @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/toolchain-metadata.json"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/pycircuit")

if(PYC_INSTALL_CMAKE_PACKAGE)
  set(PYCIRCUIT_INSTALL_CONFIGDIR "${CMAKE_INSTALL_DATADIR}/pycircuit/cmake")
  configure_package_config_file(cmake/pycircuitConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/pycircuitConfig.cmake"
    INSTALL_DESTINATION "${PYCIRCUIT_INSTALL_CONFIGDIR}")
  write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/pycircuitConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}" COMPATIBILITY SameMajorVersion)
  if(PYC_BUILD_RUNTIME_LIB)
    install(EXPORT pycircuitRuntimeTargets NAMESPACE pycircuit::
      DESTINATION "${PYCIRCUIT_INSTALL_CONFIGDIR}")
  endif()
  if(PYC_BUILD_COMPILER_DEV)
    install(EXPORT pycircuitCompilerTargets NAMESPACE pycircuit::
      DESTINATION "${PYCIRCUIT_INSTALL_CONFIGDIR}")
  endif()
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/pycircuitConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/pycircuitConfigVersion.cmake"
    cmake/PycircuitExamples.cmake cmake/prepare_output.py cmake/verify_example.py
    DESTINATION "${PYCIRCUIT_INSTALL_CONFIGDIR}")
endif()
if(PYC_PACKAGE_BUNDLE_RUNTIME_DEPS AND PYC_BUILD_COMPILER_DEV)
  configure_file(cmake/install_runtime_deps.cmake.in install_runtime_deps.cmake @ONLY)
  install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/install_runtime_deps.cmake")
endif()
set(CPACK_PACKAGE_NAME "pycircuit")
set(CPACK_PACKAGE_VENDOR "pyCircuit")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "pyCircuit: Python to common ACIR to C++/Verilog")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_GENERATOR "TGZ")
include(CPack)
