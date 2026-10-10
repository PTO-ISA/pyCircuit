cmake_minimum_required(VERSION 3.25)
project(PycircuitMigrationPreview NONE)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

get_filename_component(_repo "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(PYCIRCUIT_REPOSITORY_ROOT "${_repo}" CACHE PATH "Current pyCircuit checkout")
set(PYCIRCUIT_NATIVE_BUILD
    "${PYCIRCUIT_REPOSITORY_ROOT}/.pycircuit_out/w10-pm/build"
    CACHE PATH "Native helpers built from this checkout")
set(PYCIRCUIT_DESIGN "design_top" CACHE STRING "Checked-in preview design")
set(PYCIRCUIT_TEST_CONSUMER "" CACHE FILEPATH "Optional external testbench consumer")
set(_designs design_top hold_top zero_rule_top failure_top value_probe_top
    report_probe_top duplicate_report_top)
if(NOT PYCIRCUIT_DESIGN IN_LIST _designs)
  message(FATAL_ERROR "Unsupported preview design")
endif()
set(_definition_design_top DesignTop)
set(_definition_hold_top HoldTop)
set(_definition_zero_rule_top ZeroRuleTop)
set(_definition_failure_top FailureTop)
set(_definition_value_probe_top ValueProbeTop)
set(_definition_report_probe_top ReportProbeTop)
set(_definition_duplicate_report_top DuplicateReportTop)
set(_unit_helper "${PYCIRCUIT_NATIVE_BUILD}/bin/pycircuit-source-unit")
set(_design_helper "${PYCIRCUIT_NATIVE_BUILD}/bin/pycircuit-link")
set(_parts_helper "${PYCIRCUIT_NATIVE_BUILD}/bin/pycircuit-emit")
foreach(_helper _unit_helper _design_helper _parts_helper)
  if(NOT EXISTS "${${_helper}}")
    message(FATAL_ERROR "Build current-checkout native helper ${${_helper}} first")
  endif()
endforeach()
set(_roots
  "${PYCIRCUIT_REPOSITORY_ROOT}/python")
if(WIN32)
  set(_sep ";")
else()
  set(_sep ":")
endif()
list(JOIN _roots "${_sep}" _python_path)
set(_environment "PYTHONPATH=${_python_path}"
  "PYCIRCUIT_SOURCE_COMPILER=${_unit_helper}"
  "PYCIRCUIT_LINKER=${_design_helper}")
set(_python_sources)
foreach(_root IN LISTS _roots)
  file(GLOB_RECURSE _files CONFIGURE_DEPENDS "${_root}/*.py")
  list(APPEND _python_sources ${_files})
endforeach()
set(_prepare "${PYCIRCUIT_REPOSITORY_ROOT}/tests/integration/source-unit-build/prepare_unit_output.py")
set(_src "${CMAKE_CURRENT_LIST_DIR}/src")
set(_units "${CMAKE_BINARY_DIR}/units")

function(source_unit stem)
  set(_directory "${_units}/${stem}")
  set(_interfaces)
  set(_dependencies)
  foreach(_dependency IN LISTS ARGN)
    list(APPEND _interfaces -I "${_units}/${_dependency}")
    list(APPEND _dependencies
      "${_units}/${_dependency}/${_dependency}.interface.ac"
      "${_units}/${_dependency}/unit.json")
  endforeach()
  add_custom_command(
    OUTPUT "${_directory}/${stem}.ac"
      "${_directory}/${stem}.interface.ac" "${_directory}/unit.json"
    BYPRODUCTS "${_directory}/${stem}.d"
    DEPFILE "${_directory}/${stem}.d"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${_units}"
    COMMAND "${Python3_EXECUTABLE}" "${_prepare}" "${_directory}"
    COMMAND "${CMAKE_COMMAND}" -E env ${_environment}
      "${Python3_EXECUTABLE}" -m pycircuit.cli compile
      -c "${_src}/${stem}.py" --source-root "${_src}"
      --package-prefix preview ${_interfaces} -o "${_directory}" --replace
    DEPENDS "${_src}/${stem}.py" "${_prepare}" ${_dependencies}
      ${_python_sources} "${_unit_helper}" "${_design_helper}"
      "${PYCIRCUIT_REPOSITORY_ROOT}/pyproject.toml"
    VERBATIM)
endfunction()

source_unit(types)
set(_stems types)
if(PYCIRCUIT_DESIGN STREQUAL "design_top")
  source_unit(counter types)
  source_unit(design_top types counter)
  list(APPEND _stems counter design_top)
else()
  source_unit("${PYCIRCUIT_DESIGN}" types)
  list(APPEND _stems "${PYCIRCUIT_DESIGN}")
endif()
set(_link_inputs)
set(_link_dependencies)
foreach(_stem IN LISTS _stems)
  list(APPEND _link_inputs "${_units}/${_stem}")
  list(APPEND _link_dependencies "${_units}/${_stem}/${_stem}.ac"
    "${_units}/${_stem}/${_stem}.interface.ac"
    "${_units}/${_stem}/unit.json")
endforeach()
set(_final "${CMAKE_BINARY_DIR}/linked/${PYCIRCUIT_DESIGN}.ac")
add_custom_command(OUTPUT "${_final}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/linked"
  COMMAND "${CMAKE_COMMAND}" -E env ${_environment}
    "${Python3_EXECUTABLE}" -m pycircuit.cli link ${_link_inputs}
    --top "preview.${PYCIRCUIT_DESIGN}.${_definition_${PYCIRCUIT_DESIGN}}"
    -o "${_final}" --replace
  DEPENDS ${_link_dependencies} ${_python_sources} "${_design_helper}"
  VERBATIM)
set(_materializer "${PYCIRCUIT_REPOSITORY_ROOT}/tools/materialize_source_preview.py")
set(_artifacts "${CMAKE_BINARY_DIR}/artifacts")
set(_stamp "${CMAKE_BINARY_DIR}/preview-artifacts.stamp")
add_custom_command(OUTPUT "${_stamp}"
  BYPRODUCTS "${_artifacts}/cpp/generated.json"
    "${_artifacts}/verilog/generated.json"
    "${_artifacts}/cpp/CMakeLists.txt" "${_artifacts}/verilog/CMakeLists.txt"
  COMMAND "${Python3_EXECUTABLE}" "${_prepare}" "${_artifacts}/cpp"
  COMMAND "${Python3_EXECUTABLE}" "${_prepare}" "${_artifacts}/verilog"
  COMMAND "${CMAKE_COMMAND}" -E env ${_environment}
    "${Python3_EXECUTABLE}" "${_materializer}" "${_final}"
    "${_artifacts}" "${_parts_helper}"
  COMMAND "${CMAKE_COMMAND}" -E touch "${_stamp}"
  DEPENDS "${_final}" "${_parts_helper}" "${_materializer}" ${_python_sources}
  VERBATIM)
add_custom_target(preview-artifacts DEPENDS "${_stamp}")

add_custom_target(preview-build
  COMMAND "${CMAKE_COMMAND}" -S "${_artifacts}/cpp"
    -B "${CMAKE_BINARY_DIR}/model-cpp" -G Ninja
    "-DPYCIRCUIT_RUNTIME_ROOT=${PYCIRCUIT_REPOSITORY_ROOT}/runtime"
    "-DPYCIRCUIT_TEST_CONSUMER=${PYCIRCUIT_TEST_CONSUMER}"
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
  COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}/model-cpp" --parallel 4
  COMMAND "${CMAKE_COMMAND}" -S "${_artifacts}/verilog"
    -B "${CMAKE_BINARY_DIR}/model-verilog" -G Ninja
    "-DPYCIRCUIT_RUNTIME_ROOT=${PYCIRCUIT_REPOSITORY_ROOT}/runtime"
    "-DPYCIRCUIT_TEST_CONSUMER=${PYCIRCUIT_TEST_CONSUMER}"
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
  COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}/model-verilog" --parallel 4
  DEPENDS preview-artifacts
  USES_TERMINAL VERBATIM)
