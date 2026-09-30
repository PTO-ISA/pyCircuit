cmake_minimum_required(VERSION 3.25)

project(PycircuitSourceUnitBuild NONE)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

get_filename_component(
  _default_repository_root
  "${CMAKE_CURRENT_LIST_DIR}/../../../../"
  ABSOLUTE
)
set(PYCIRCUIT_REPOSITORY_ROOT "${_default_repository_root}" CACHE PATH
    "pyCircuit checkout containing the compiler and Python driver")
set(PYCIRCUIT_PYTHON_EXECUTABLE "${Python3_EXECUTABLE}" CACHE FILEPATH
    "Python interpreter used for pycircuit compile/link")
set(PYCIRCUIT_IMPORT_ROOTS
    "${PYCIRCUIT_REPOSITORY_ROOT}/python/pycircuit/src;${PYCIRCUIT_REPOSITORY_ROOT}/python/semantic-core/src;${PYCIRCUIT_REPOSITORY_ROOT}/python/agentic-circuit/src"
    CACHE STRING "Python import roots passed to the public driver")
set(PYCIRCUIT_SOURCE_UNIT_HARNESS "" CACHE FILEPATH
    "Path to acir-source-unit-harness")
set(PYCIRCUIT_DESIGN_HARNESS "" CACHE FILEPATH
    "Path to acir-design-harness")
set(PYCIRCUIT_OUTPUT_ROOT "${CMAKE_BINARY_DIR}/generated units" CACHE PATH
    "Directory for source units and the linked program")
set(PYCIRCUIT_TOOLCHAIN_CONFIG
    "${PYCIRCUIT_REPOSITORY_ROOT}/pyproject.toml;${PYCIRCUIT_REPOSITORY_ROOT}/python/semantic-core/pyproject.toml;${PYCIRCUIT_REPOSITORY_ROOT}/python/agentic-circuit/pyproject.toml"
    CACHE STRING
    "Additional compiler/toolchain configuration files to depend on")

foreach(_required
    PYCIRCUIT_PYTHON_EXECUTABLE
    PYCIRCUIT_SOURCE_UNIT_HARNESS
    PYCIRCUIT_DESIGN_HARNESS)
  if("${${_required}}" STREQUAL "" OR
     NOT EXISTS "${${_required}}")
    message(FATAL_ERROR "${_required} must name an existing file")
  endif()
endforeach()
if(NOT PYCIRCUIT_IMPORT_ROOTS)
  message(FATAL_ERROR "PYCIRCUIT_IMPORT_ROOTS must contain the three source roots")
endif()

if(WIN32)
  set(_python_path_separator ";")
else()
  set(_python_path_separator ":")
endif()
list(JOIN PYCIRCUIT_IMPORT_ROOTS "${_python_path_separator}" _python_path)

# Capture, transport, publication, link, and support modules all affect the
# command-line compiler contract. Track every Python source below each actual
# import root so an overridden import root cannot run untracked compiler code.
set(_driver_sources)
foreach(_import_root IN LISTS PYCIRCUIT_IMPORT_ROOTS)
  file(GLOB_RECURSE _import_sources CONFIGURE_DEPENDS "${_import_root}/*.py")
  list(APPEND _driver_sources ${_import_sources})
endforeach()
set(_compiler_dependencies
  ${_driver_sources}
  "${PYCIRCUIT_REPOSITORY_ROOT}/pyproject.toml"
  "${PYCIRCUIT_REPOSITORY_ROOT}/python/semantic-core/pyproject.toml"
  "${PYCIRCUIT_REPOSITORY_ROOT}/python/agentic-circuit/pyproject.toml"
  "${PYCIRCUIT_SOURCE_UNIT_HARNESS}"
  "${PYCIRCUIT_DESIGN_HARNESS}"
  ${PYCIRCUIT_TOOLCHAIN_CONFIG}
)

set(_source_root "${CMAKE_CURRENT_LIST_DIR}/src")
set(_prepare_unit_output
  "${CMAKE_CURRENT_LIST_DIR}/prepare_unit_output.py")
set(_types_dir "${PYCIRCUIT_OUTPUT_ROOT}/units/types")
set(_child_dir "${PYCIRCUIT_OUTPUT_ROOT}/units/child")
set(_parent_dir "${PYCIRCUIT_OUTPUT_ROOT}/units/parent")
set(_independent_dir "${PYCIRCUIT_OUTPUT_ROOT}/units/independent")

function(add_source_unit target stem)
  cmake_parse_arguments(UNIT "" "" "INTERFACE_UNITS" ${ARGN})
  set(_unit_dir "${PYCIRCUIT_OUTPUT_ROOT}/units/${stem}")
  set(_source "${_source_root}/${stem}.py")
  set(_body "${_unit_dir}/${stem}.ac")
  set(_interface "${_unit_dir}/${stem}.interface.ac")
  set(_depfile "${_unit_dir}/${stem}.d")
  set(_receipt "${_unit_dir}/unit.json")

  set(_interface_arguments)
  set(_interface_dependencies)
  foreach(_interface_unit IN LISTS UNIT_INTERFACE_UNITS)
    set(_interface_dir "${PYCIRCUIT_OUTPUT_ROOT}/units/${_interface_unit}")
    list(APPEND _interface_arguments -I "${_interface_dir}")
    list(APPEND _interface_dependencies
      "${_interface_dir}/${_interface_unit}.interface.ac"
      "${_interface_dir}/unit.json"
    )
  endforeach()

  add_custom_command(
    OUTPUT "${_body}" "${_interface}" "${_receipt}"
    BYPRODUCTS "${_depfile}"
    DEPFILE "${_depfile}"
    COMMAND "${PYCIRCUIT_PYTHON_EXECUTABLE}"
      "${_prepare_unit_output}" "${_unit_dir}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
      "${PYCIRCUIT_OUTPUT_ROOT}/units"
    COMMAND "${CMAKE_COMMAND}" -E env
      "PYTHONPATH=${_python_path}"
      "ACIR_SOURCE_UNIT_HARNESS=${PYCIRCUIT_SOURCE_UNIT_HARNESS}"
      "ACIR_DESIGN_HARNESS=${PYCIRCUIT_DESIGN_HARNESS}"
      "${PYCIRCUIT_PYTHON_EXECUTABLE}" -m pycircuit.cli compile
      -c "${_source}"
      --source-root "${_source_root}"
      --package-prefix demo
      ${_interface_arguments}
      -o "${_unit_dir}"
      --replace
    DEPENDS "${_source}" "${_prepare_unit_output}"
      ${_interface_dependencies} ${_compiler_dependencies}
    WORKING_DIRECTORY "${PYCIRCUIT_REPOSITORY_ROOT}"
    COMMENT "Compile source unit ${stem}"
    VERBATIM
  )
  add_custom_target(${target} DEPENDS "${_body}" "${_interface}" "${_receipt}")
  set(${target}_BODY "${_body}" PARENT_SCOPE)
  set(${target}_INTERFACE "${_interface}" PARENT_SCOPE)
  set(${target}_RECEIPT "${_receipt}" PARENT_SCOPE)
endfunction()

add_source_unit(source-unit-types types)
add_source_unit(source-unit-child child INTERFACE_UNITS types)
add_source_unit(source-unit-parent parent INTERFACE_UNITS child types)
add_source_unit(source-unit-independent independent)

set(_program "${PYCIRCUIT_OUTPUT_ROOT}/linked/parent.ac")
add_custom_command(
  OUTPUT "${_program}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory
    "${PYCIRCUIT_OUTPUT_ROOT}/linked"
  COMMAND "${CMAKE_COMMAND}" -E env
    "PYTHONPATH=${_python_path}"
    "ACIR_SOURCE_UNIT_HARNESS=${PYCIRCUIT_SOURCE_UNIT_HARNESS}"
    "ACIR_DESIGN_HARNESS=${PYCIRCUIT_DESIGN_HARNESS}"
    "${PYCIRCUIT_PYTHON_EXECUTABLE}" -m pycircuit.cli link
    "${_types_dir}" "${_child_dir}" "${_parent_dir}"
    --top demo.parent.Parent
    -o "${_program}"
    --replace
  DEPENDS
    "${source-unit-types_BODY}" "${source-unit-types_INTERFACE}"
    "${source-unit-types_RECEIPT}"
    "${source-unit-child_BODY}" "${source-unit-child_INTERFACE}"
    "${source-unit-child_RECEIPT}"
    "${source-unit-parent_BODY}" "${source-unit-parent_INTERFACE}"
    "${source-unit-parent_RECEIPT}"
    ${_compiler_dependencies}
  WORKING_DIRECTORY "${PYCIRCUIT_REPOSITORY_ROOT}"
  COMMENT "Link source-unit fixture program"
  VERBATIM
)
add_custom_target(source-unit-program DEPENDS "${_program}")

add_custom_target(source-unit-all)
add_dependencies(source-unit-all
  source-unit-types source-unit-child source-unit-parent
  source-unit-independent source-unit-program
)
