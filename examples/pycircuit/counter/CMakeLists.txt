cmake_minimum_required(VERSION 3.25)
project(PycircuitCounter NONE)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
find_program(_driver NAMES pycircuit HINTS "${PYCIRCUIT_TOOLCHAIN_ROOT}/bin" NO_DEFAULT_PATH REQUIRED)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/units")
foreach(_source types counter design_top)
  set(_unit "${CMAKE_CURRENT_BINARY_DIR}/units/${_source}")
  set(_imports)
  set(_dependencies "${CMAKE_CURRENT_SOURCE_DIR}/${_source}.py" "${_driver}")
  if(NOT _source STREQUAL "types")
    list(APPEND _imports -I "${CMAKE_CURRENT_BINARY_DIR}/units/types")
    list(APPEND _dependencies "${CMAKE_CURRENT_BINARY_DIR}/units/types/types.interface.ac" "${CMAKE_CURRENT_BINARY_DIR}/units/types/unit.json")
  endif()
  if(_source STREQUAL "design_top")
    list(APPEND _imports -I "${CMAKE_CURRENT_BINARY_DIR}/units/counter")
    list(APPEND _dependencies "${CMAKE_CURRENT_BINARY_DIR}/units/counter/counter.interface.ac" "${CMAKE_CURRENT_BINARY_DIR}/units/counter/unit.json")
  endif()
  add_custom_command(OUTPUT "${_unit}/${_source}.ac" "${_unit}/${_source}.interface.ac" "${_unit}/unit.json"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare_output.py" "${_unit}"
    COMMAND "${_driver}" compile -c "${CMAKE_CURRENT_SOURCE_DIR}/${_source}.py" --source-root "${CMAKE_CURRENT_SOURCE_DIR}" --package-prefix preview ${_imports} -o "${_unit}" --replace
    DEPFILE "${_unit}/${_source}.d" DEPENDS ${_dependencies} VERBATIM)
  list(APPEND _units "${_unit}")
  list(APPEND _link_inputs "${_unit}/${_source}.ac" "${_unit}/${_source}.interface.ac" "${_unit}/unit.json")
endforeach()
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/design_top.ac"
  COMMAND "${_driver}" link ${_units} --top preview.design_top.DesignTop -o "${CMAKE_CURRENT_BINARY_DIR}/design_top.ac" --replace
  DEPENDS ${_link_inputs} "${_driver}" VERBATIM)
foreach(_target cpp verilog)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${_target}/generated.json"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare_output.py" "${CMAKE_CURRENT_BINARY_DIR}/${_target}"
    COMMAND "${_driver}" emit "${CMAKE_CURRENT_BINARY_DIR}/design_top.ac" --target ${_target} -o "${CMAKE_CURRENT_BINARY_DIR}/${_target}" --replace
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/design_top.ac" "${_driver}" VERBATIM)
  add_custom_target(${_target}_artifacts ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${_target}/generated.json")
endforeach()
