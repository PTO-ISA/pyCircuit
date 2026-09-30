cmake_minimum_required(VERSION 3.25)
project(PycircuitCounter NONE)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
find_program(_driver NAMES pycircuit HINTS "${PYCIRCUIT_TOOLCHAIN_ROOT}/bin" NO_DEFAULT_PATH REQUIRED)
foreach(_helper_name acir-source-unit-harness acir-design-harness acir-cpp-source-parts-harness)
  find_program(_helper_${_helper_name} NAMES ${_helper_name}
    HINTS "${PYCIRCUIT_TOOLCHAIN_ROOT}/bin" NO_DEFAULT_PATH REQUIRED)
  list(APPEND _compiler_dependencies "${_helper_${_helper_name}}")
endforeach()
list(APPEND _compiler_dependencies "${_driver}"
  "${CMAKE_CURRENT_SOURCE_DIR}/prepare_output.py"
  "${PYCIRCUIT_TOOLCHAIN_ROOT}/share/pycircuit/toolchain-metadata.json")
file(GLOB _compiler_configs CONFIGURE_DEPENDS
  "${PYCIRCUIT_TOOLCHAIN_ROOT}/share/pycircuit/cmake/*.cmake")
list(APPEND _compiler_dependencies ${_compiler_configs})
if(DEFINED PYCIRCUIT_PYTHON_DIR AND IS_DIRECTORY "${PYCIRCUIT_PYTHON_DIR}/pycircuit")
  file(GLOB_RECURSE _compiler_python CONFIGURE_DEPENDS
    "${PYCIRCUIT_PYTHON_DIR}/pycircuit/*.py")
  list(APPEND _compiler_dependencies ${_compiler_python})
endif()
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/units")
foreach(_source types counter design_top)
  set(_unit "${CMAKE_CURRENT_BINARY_DIR}/units/${_source}")
  set(_imports)
  set(_dependencies "${CMAKE_CURRENT_SOURCE_DIR}/${_source}.py" ${_compiler_dependencies})
  if(NOT _source STREQUAL "types")
    list(APPEND _imports -I "${CMAKE_CURRENT_BINARY_DIR}/units/types")
    list(APPEND _dependencies "${CMAKE_CURRENT_BINARY_DIR}/units/types/types.interface.ac" "${CMAKE_CURRENT_BINARY_DIR}/units/types/unit.json")
  endif()
  if(_source STREQUAL "design_top")
    list(APPEND _imports -I "${CMAKE_CURRENT_BINARY_DIR}/units/counter")
    list(APPEND _dependencies "${CMAKE_CURRENT_BINARY_DIR}/units/counter/counter.interface.ac" "${CMAKE_CURRENT_BINARY_DIR}/units/counter/unit.json")
  endif()
  add_custom_command(OUTPUT "${_unit}/${_source}.ac" "${_unit}/${_source}.interface.ac" "${_unit}/unit.json"
    BYPRODUCTS "${_unit}/${_source}.d"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare_output.py" "${_unit}"
    COMMAND "${_driver}" compile -c "${CMAKE_CURRENT_SOURCE_DIR}/${_source}.py" --source-root "${CMAKE_CURRENT_SOURCE_DIR}" --package-prefix preview ${_imports} -o "${_unit}" --replace
    DEPFILE "${_unit}/${_source}.d" DEPENDS ${_dependencies} VERBATIM)
  list(APPEND _units "${_unit}")
  list(APPEND _link_inputs "${_unit}/${_source}.ac" "${_unit}/${_source}.interface.ac" "${_unit}/unit.json")
endforeach()
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/design_top.ac"
  COMMAND "${_driver}" link ${_units} --top preview.design_top.DesignTop -o "${CMAKE_CURRENT_BINARY_DIR}/design_top.ac" --replace
  DEPENDS ${_link_inputs} ${_compiler_dependencies} VERBATIM)
# Keep the exact generated receipt inventory in the clean graph. Publication
# controls are deliberately outside these artifacts and remain untouched.
set(_cpp_files CMakeLists.txt dut.h model_api.cpp pycircuit_support.hpp
  pycircuit_system.hpp runner_metadata.hpp runner_main.cpp
  sources/preview/types.hpp sources/preview/counter.hpp
  sources/preview/counter.cpp sources/preview/design_top.hpp
  sources/preview/design_top.cpp)
set(_verilog_files CMakeLists.txt design_top.sv runner_bridge.sv rtl_system.hpp
  runner_metadata.hpp runner_main.cpp sources/preview/counter.sv
  sources/preview/design_top.sv)
foreach(_target cpp verilog)
  set(_byproducts)
  foreach(_file IN LISTS _${_target}_files)
    list(APPEND _byproducts "${CMAKE_CURRENT_BINARY_DIR}/${_target}/${_file}")
  endforeach()
  foreach(_source types counter design_top)
    list(APPEND _byproducts "${CMAKE_CURRENT_BINARY_DIR}/${_target}/sources/preview/${_source}.source-map.json")
  endforeach()
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${_target}/generated.json"
    BYPRODUCTS ${_byproducts}
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare_output.py" "${CMAKE_CURRENT_BINARY_DIR}/${_target}"
    COMMAND "${_driver}" emit "${CMAKE_CURRENT_BINARY_DIR}/design_top.ac" --target ${_target} -o "${CMAKE_CURRENT_BINARY_DIR}/${_target}" --replace
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/design_top.ac" ${_compiler_dependencies} VERBATIM)
  add_custom_target(${_target}_artifacts ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${_target}/generated.json")
endforeach()
