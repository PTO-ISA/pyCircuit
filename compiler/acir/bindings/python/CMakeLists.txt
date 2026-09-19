find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter Development.Module)

set(ACIR_PYTHON_SOURCE_DIR
  "${ACIR_REPO_ROOT}/python/agentic-circuit/src/agentic_circuit")
set(ACIR_SEMANTIC_CORE_SOURCE_DIR
  "${ACIR_REPO_ROOT}/python/semantic-core/src/_pycircuit_semantics")
set(ACIR_SCHEMA_SOURCE_DIR
  "${ACIR_REPO_ROOT}/schemas/agentic-circuit")
set(ACIR_PYTHON_BUILD_PACKAGE_DIR
  "${ACIR_BINARY_DIR}/python/agentic_circuit")
set(ACIR_SEMANTIC_CORE_BUILD_PACKAGE_DIR
  "${ACIR_BINARY_DIR}/python/_pycircuit_semantics")
set(ACIR_PYTHON_BUILD_DATA_DIR
  "${ACIR_PYTHON_BUILD_PACKAGE_DIR}/_data")

foreach(_required_path IN ITEMS
    "${ACIR_PYTHON_SOURCE_DIR}"
    "${ACIR_SEMANTIC_CORE_SOURCE_DIR}"
    "${ACIR_SCHEMA_SOURCE_DIR}")
  if(NOT IS_DIRECTORY "${_required_path}")
    message(FATAL_ERROR "required Agentic Circuit source is missing: ${_required_path}")
  endif()
endforeach()

set(
  ACIR_PYTHON_INSTALL_DIR
  "${CMAKE_INSTALL_LIBDIR}/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages"
  CACHE STRING
  "Install destination for the Agentic Circuit Python extension"
)

# Assemble one build-tree Python package from the canonical Python and schema
# roots. The schema directory identity is retained in package data; no
# compiler-local resource copy exists.
file(REMOVE_RECURSE "${ACIR_PYTHON_BUILD_DATA_DIR}")
file(MAKE_DIRECTORY "${ACIR_PYTHON_BUILD_PACKAGE_DIR}")
file(COPY "${ACIR_PYTHON_SOURCE_DIR}/"
  DESTINATION "${ACIR_PYTHON_BUILD_PACKAGE_DIR}"
  PATTERN "__pycache__" EXCLUDE
  PATTERN "*.pyc" EXCLUDE)
file(REMOVE_RECURSE "${ACIR_SEMANTIC_CORE_BUILD_PACKAGE_DIR}")
file(MAKE_DIRECTORY "${ACIR_SEMANTIC_CORE_BUILD_PACKAGE_DIR}")
file(COPY "${ACIR_SEMANTIC_CORE_SOURCE_DIR}/"
  DESTINATION "${ACIR_SEMANTIC_CORE_BUILD_PACKAGE_DIR}"
  PATTERN "__pycache__" EXCLUDE
  PATTERN "*.pyc" EXCLUDE)
file(MAKE_DIRECTORY
  "${ACIR_PYTHON_BUILD_DATA_DIR}/schemas")
file(COPY "${ACIR_SCHEMA_SOURCE_DIR}"
  DESTINATION "${ACIR_PYTHON_BUILD_DATA_DIR}/schemas")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/resource-package-init.py"
  "${ACIR_PYTHON_BUILD_DATA_DIR}/schemas/__init__.py"
  COPYONLY)

Python3_add_library(agentic_circuit_native MODULE NativeModule.cpp)
string(JOIN "|" ACIR_NATIVE_LLVM_LINK_FLAGS ${ACIR_LLVM_LINK_FLAGS})
target_compile_definitions(agentic_circuit_native PRIVATE Py_LIMITED_API=0x030B0000)
target_compile_definitions(agentic_circuit_native PRIVATE
  ACIR_NATIVE_CXX_COMPILER="${CMAKE_CXX_COMPILER}"
  ACIR_NATIVE_LLVM_LINK_FLAGS="${ACIR_NATIVE_LLVM_LINK_FLAGS}"
)
if(NOT LLVM_ENABLE_RTTI)
  target_compile_options(agentic_circuit_native PRIVATE -fno-rtti)
endif()
target_link_libraries(agentic_circuit_native PRIVATE
  ACIRCompiler
  Python3::Module
)

if(WIN32)
  # CPython's PC/pyconfig.h records a pragma-based auto-link directive for the
  # import library an extension must resolve. Under Py_LIMITED_API it names the
  # stable-ABI library "python3.lib"; without it, "python<major><minor>.lib".
  # The directive is unconditional up to Python 3.13 (Py_NO_LINK_LIB only
  # arrived in 3.14), and Python3::Module deliberately links no library at all,
  # so nothing puts the interpreter's libs directory on the link line. Link the
  # versioned import library explicitly and make sure the directory holding it
  # is searched, which is what the directive needs.
  get_filename_component(_acir_python_root "${Python3_EXECUTABLE}" DIRECTORY)
  set(_acir_python_libs "${_acir_python_root}/libs")
  set(_acir_python_import_lib
    "${_acir_python_libs}/python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}.lib")
  if(NOT EXISTS "${_acir_python_import_lib}" AND Python3_LIBRARY)
    get_filename_component(_acir_python_libs "${Python3_LIBRARY}" DIRECTORY)
    set(_acir_python_import_lib "${Python3_LIBRARY}")
  endif()
  if(NOT EXISTS "${_acir_python_import_lib}")
    message(FATAL_ERROR
      "Python import library for ${Python3_VERSION} is missing; looked for "
      "${_acir_python_libs}/python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}.lib")
  endif()
  target_link_directories(agentic_circuit_native PRIVATE "${_acir_python_libs}")
  target_link_libraries(agentic_circuit_native PRIVATE "${_acir_python_import_lib}")
  # The hosted python-versions builds ship only the versioned import library, so
  # stage a copy under the name the limited-API directive uses. Both libraries
  # export the stable ABI, and Py_LIMITED_API keeps the extension inside it.
  if(NOT EXISTS "${_acir_python_libs}/python3.lib")
    set(_acir_python_abi_dir "${ACIR_BINARY_DIR}/python-stable-abi")
    file(MAKE_DIRECTORY "${_acir_python_abi_dir}")
    configure_file("${_acir_python_import_lib}"
      "${_acir_python_abi_dir}/python3.lib" COPYONLY)
    target_link_directories(agentic_circuit_native PRIVATE "${_acir_python_abi_dir}")
  endif()
endif()
if(APPLE)
  set(ACIR_PYTHON_INSTALL_RPATH "@loader_path/../../..")
else()
  set(ACIR_PYTHON_INSTALL_RPATH "\$ORIGIN/../../..")
endif()
set_target_properties(agentic_circuit_native PROPERTIES
  OUTPUT_NAME "_native"
  LIBRARY_OUTPUT_DIRECTORY "${ACIR_PYTHON_BUILD_PACKAGE_DIR}"
  INSTALL_RPATH "${ACIR_PYTHON_INSTALL_RPATH}"
)

install(
  DIRECTORY "${ACIR_PYTHON_SOURCE_DIR}/"
  DESTINATION "${ACIR_PYTHON_INSTALL_DIR}/agentic_circuit"
  PATTERN "__pycache__" EXCLUDE
  PATTERN "*.pyc" EXCLUDE)
install(
  DIRECTORY "${ACIR_SEMANTIC_CORE_SOURCE_DIR}/"
  DESTINATION "${ACIR_PYTHON_INSTALL_DIR}/_pycircuit_semantics"
  PATTERN "__pycache__" EXCLUDE
  PATTERN "*.pyc" EXCLUDE)
install(
  DIRECTORY "${ACIR_SCHEMA_SOURCE_DIR}"
  DESTINATION "${ACIR_PYTHON_INSTALL_DIR}/agentic_circuit/_data/schemas")
install(
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/resource-package-init.py"
  DESTINATION "${ACIR_PYTHON_INSTALL_DIR}/agentic_circuit/_data/schemas"
  RENAME "__init__.py")
install(
  TARGETS agentic_circuit_native
  LIBRARY DESTINATION "${ACIR_PYTHON_INSTALL_DIR}/agentic_circuit"
  RUNTIME DESTINATION "${ACIR_PYTHON_INSTALL_DIR}/agentic_circuit"
)
install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/agentic-circuit"
  DESTINATION "${CMAKE_INSTALL_BINDIR}")
