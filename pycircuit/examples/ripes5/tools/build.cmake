# Included by the compiler project; keep the existing build output paths.
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
  add_executable(acpy-ripes5-${variant} "${REPO}/gfsim/cpp/examples/ripes5/tests/runner.cpp")
  target_compile_definitions(acpy-ripes5-${variant} PRIVATE ACPY_GENERATED_MODEL=1)
  target_link_libraries(acpy-ripes5-${variant} PRIVATE acpy-model-${variant})
endforeach()
if(BUILD_TESTING AND GFSIM_BUILD_EXAMPLES)
  add_test(NAME acpy-ripes5-five-way COMMAND ${Python3_EXECUTABLE}
    -m pycircuit.examples.ripes5.tests.verify
    --generated-runner $<TARGET_FILE:acpy-ripes5-compiled>
    --emitted-runner $<TARGET_FILE:acpy-ripes5-emitted>
    --cpp-runner $<TARGET_FILE:gfsim-ripes5>
    --runner "${GFSIM_RIPES_REFERENCE}"
    --acir "${GENERATED}/model.acir.mlir"
    --output "${CMAKE_CURRENT_BINARY_DIR}/ripes5-evidence")
  set_tests_properties(acpy-ripes5-five-way PROPERTIES TIMEOUT 300)
  add_test(NAME acpy-runner-input-gates COMMAND ${Python3_EXECUTABLE}
    "${REPO}/gfsim/cpp/tests/runner_input.py" $<TARGET_FILE:acpy-ripes5-compiled>)
  add_test(NAME acpy-ripes5-fixed-window COMMAND ${Python3_EXECUTABLE}
    -m pycircuit.examples.ripes5.tests.test_benchmark
    --generated-runner $<TARGET_FILE:acpy-ripes5-compiled>
    --cpp-runner $<TARGET_FILE:gfsim-ripes5>
    --runner "${GFSIM_RIPES_REFERENCE}"
    --output "${CMAKE_CURRENT_BINARY_DIR}/ripes5-evidence/fixed-runner-tests.json")
  set_tests_properties(acpy-ripes5-fixed-window PROPERTIES TIMEOUT 180)
  add_test(NAME acpy-ripes5-streaming-gates COMMAND ${Python3_EXECUTABLE}
    -m pycircuit.examples.ripes5.tests.test_benchmark_tools)
  set_tests_properties(acpy-ripes5-five-way acpy-ripes5-fixed-window acpy-ripes5-streaming-gates
    PROPERTIES WORKING_DIRECTORY "${REPO}")
endif()
