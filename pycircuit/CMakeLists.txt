cmake_minimum_required(VERSION 3.20)
project(acpy_gfsim LANGUAGES C CXX)
add_subdirectory(mlir)
include(CTest)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
get_filename_component(REPO "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
add_subdirectory("${REPO}/gfsim/cpp" gfsim)
file(GLOB COMPILER_INPUTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.py" "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")
set(MODEL_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/examples/ripes5/model.py")
set(GENERATED "${CMAKE_CURRENT_BINARY_DIR}/compiled")
set(RELOADED "${CMAKE_CURRENT_BINARY_DIR}/emitted")
add_custom_command(OUTPUT "${GENERATED}/model.cpp" "${GENERATED}/model.hpp" "${GENERATED}/model.acir.mlir" "${GENERATED}/ac_support.hpp"
  COMMAND ${CMAKE_COMMAND} -E env "ACPY_MLIR_COMPILER=$<TARGET_FILE:acir-compile>" ${Python3_EXECUTABLE} -m pycircuit compile "${MODEL_SOURCE}" --top CPU --output "${GENERATED}"
  WORKING_DIRECTORY "${REPO}"
  DEPENDS acir-compile ${COMPILER_INPUTS} "${MODEL_SOURCE}" "${CMAKE_CURRENT_SOURCE_DIR}/examples/ripes5/logic.py"
  VERBATIM)
add_custom_target(acpy-compile DEPENDS "${GENERATED}/model.cpp" "${GENERATED}/model.hpp" "${GENERATED}/model.acir.mlir" "${GENERATED}/ac_support.hpp")
add_custom_command(OUTPUT "${RELOADED}/model.cpp" "${RELOADED}/model.hpp" "${RELOADED}/model.acir.mlir" "${RELOADED}/ac_support.hpp"
  COMMAND ${CMAKE_COMMAND} -E env "ACPY_MLIR_COMPILER=$<TARGET_FILE:acir-compile>" ${Python3_EXECUTABLE} -m pycircuit emit "${GENERATED}/model.acir.mlir" --output "${RELOADED}"
  WORKING_DIRECTORY "${REPO}"
  DEPENDS acir-compile ${COMPILER_INPUTS} "${GENERATED}/model.acir.mlir"
  VERBATIM)
add_custom_target(acpy-emit DEPENDS "${RELOADED}/model.cpp" "${RELOADED}/model.hpp" "${RELOADED}/model.acir.mlir" "${RELOADED}/ac_support.hpp")
add_dependencies(acpy-emit acpy-compile)
foreach(variant IN ITEMS compiled emitted)
  set(dir "${CMAKE_CURRENT_BINARY_DIR}/${variant}")
  add_library(acpy-model-${variant} STATIC "${dir}/model.cpp")
  if(variant STREQUAL "compiled")
    add_dependencies(acpy-model-${variant} acpy-compile)
  else()
    add_dependencies(acpy-model-${variant} acpy-emit)
  endif()
  target_include_directories(acpy-model-${variant} PUBLIC "${dir}")
  target_link_libraries(acpy-model-${variant} PUBLIC gfsim::gfsim)
  add_executable(acpy-ripes5-${variant} "${REPO}/gfsim/cpp/examples/ripes5/runner.cpp")
  target_compile_definitions(acpy-ripes5-${variant} PRIVATE ACPY_GENERATED_MODEL=1)
  target_link_libraries(acpy-ripes5-${variant} PRIVATE acpy-model-${variant})
endforeach()
if(BUILD_TESTING)
  set(ACPY_TEST_CXX "${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS}")
  if(GFSIM_SANITIZERS)
    string(APPEND ACPY_TEST_CXX " -fsanitize=address,undefined -fno-omit-frame-pointer")
  endif()
  add_test(NAME acpy-language-and-circuits COMMAND ${CMAKE_COMMAND} -E env "ACPY_CXX=${ACPY_TEST_CXX}" "ACPY_MLIR_OPT=$<TARGET_FILE:acir-opt>" "ACPY_MLIR_COMPILER=$<TARGET_FILE:acir-compile>"
    ${Python3_EXECUTABLE} -m unittest discover -s pycircuit/tests -v)
  set_tests_properties(acpy-language-and-circuits PROPERTIES WORKING_DIRECTORY "${REPO}" TIMEOUT 300)
  if(GFSIM_BUILD_EXAMPLES)
    add_test(NAME acpy-ripes5-five-way COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/examples/ripes5/verify.py"
      --generated-runner $<TARGET_FILE:acpy-ripes5-compiled>
      --emitted-runner $<TARGET_FILE:acpy-ripes5-emitted>
      --cpp-runner $<TARGET_FILE:gfsim-ripes5>
      --runner "${GFSIM_RIPES_REFERENCE}"
      --acir "${GENERATED}/model.acir.mlir"
      --output "${CMAKE_CURRENT_SOURCE_DIR}/examples/ripes5/output/${CMAKE_BUILD_TYPE}")
    set_tests_properties(acpy-ripes5-five-way PROPERTIES TIMEOUT 300)
    add_test(NAME acpy-runner-input-gates COMMAND ${Python3_EXECUTABLE}
      "${REPO}/gfsim/cpp/tests/runner_input.py" $<TARGET_FILE:acpy-ripes5-compiled>)
    add_test(NAME acpy-ripes5-fixed-window COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/examples/ripes5/test_benchmark.py"
      --generated-runner $<TARGET_FILE:acpy-ripes5-compiled>
      --cpp-runner $<TARGET_FILE:gfsim-ripes5>
      --runner "${GFSIM_RIPES_REFERENCE}"
      --output "${CMAKE_CURRENT_SOURCE_DIR}/examples/ripes5/output/${CMAKE_BUILD_TYPE}/fixed-runner-tests.json")
    set_tests_properties(acpy-ripes5-fixed-window PROPERTIES TIMEOUT 180)
    add_test(NAME acpy-ripes5-streaming-gates COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/examples/ripes5/test_benchmark_tools.py")
  endif()
endif()

add_subdirectory(examples/ooo)
add_subdirectory(examples/skyzh_ooo)
