set(SKYZH_SOURCES model.py types.py logic.py frontend.py execution.py storage.py)
foreach(variant IN ITEMS compiled emitted noopt)
  set(dir "${CMAKE_CURRENT_BINARY_DIR}/${variant}")
  if(variant STREQUAL "compiled")
    set(action compile "${CMAKE_CURRENT_SOURCE_DIR}/model.py" --top CPU)
    set(input ${SKYZH_SOURCES})
  else()
    set(action emit "${CMAKE_CURRENT_BINARY_DIR}/compiled/model.acir.mlir")
    set(input "${CMAKE_CURRENT_BINARY_DIR}/compiled/model.acir.mlir")
    if(variant STREQUAL "noopt")
      list(APPEND action --no-opt)
    endif()
  endif()
  add_custom_command(OUTPUT "${dir}/model.cpp" "${dir}/model.hpp" "${dir}/model.acir.mlir" "${dir}/ac_support.hpp"
    COMMAND ${CMAKE_COMMAND} -E env "ACPY_MLIR_COMPILER=$<TARGET_FILE:acir-compile>"
      ${Python3_EXECUTABLE} -m pycircuit ${action} --output "${dir}"
    DEPENDS acir-compile ${COMPILER_INPUTS} ${input}
    WORKING_DIRECTORY "${REPO}" VERBATIM)
  add_custom_target(acpy-skyzh-generate-${variant} DEPENDS "${dir}/model.cpp" "${dir}/model.hpp" "${dir}/model.acir.mlir" "${dir}/ac_support.hpp")
  if(NOT variant STREQUAL "compiled")
    add_dependencies(acpy-skyzh-generate-${variant} acpy-skyzh-generate-compiled)
  endif()
  add_library(acpy-skyzh-model-${variant} STATIC "${dir}/model.cpp")
  add_dependencies(acpy-skyzh-model-${variant} acpy-skyzh-generate-${variant})
  target_include_directories(acpy-skyzh-model-${variant} PUBLIC "${dir}")
  target_link_libraries(acpy-skyzh-model-${variant} PUBLIC gfsim::gfsim)
  add_executable(acpy-skyzh-${variant} tests/runner.cpp)
  target_link_libraries(acpy-skyzh-${variant} PRIVATE acpy-skyzh-model-${variant})
endforeach()
set(SKYZH_REFERENCE_SOURCE "${REPO}/reference/skyzh-riscv-reference" CACHE PATH "Clean pinned native skyzh checkout")
set(SKYZH_REFERENCE_BINARY "${CMAKE_CURRENT_BINARY_DIR}/reference/skyzh-reference")
file(GLOB_RECURSE SKYZH_NATIVE_INPUTS "${SKYZH_REFERENCE_SOURCE}/src/*.cpp" "${SKYZH_REFERENCE_SOURCE}/src/*.h" "${SKYZH_REFERENCE_SOURCE}/src/*.hpp")
add_custom_command(OUTPUT "${SKYZH_REFERENCE_BINARY}" "${CMAKE_CURRENT_BINARY_DIR}/reference/build.json"
  COMMAND ${Python3_EXECUTABLE} -m pycircuit.examples.skyzh_ooo.tests.reference
    --source "${SKYZH_REFERENCE_SOURCE}" --output "${CMAKE_CURRENT_BINARY_DIR}/reference"
    --cxx "${CMAKE_CXX_COMPILER}"
  DEPENDS ${SKYZH_NATIVE_INPUTS} tests/reference.py tests/reference.cpp tests/observer.hpp
  WORKING_DIRECTORY "${REPO}" VERBATIM)
add_custom_target(acpy-skyzh-reference ALL DEPENDS "${SKYZH_REFERENCE_BINARY}")
if(BUILD_TESTING)
  add_test(NAME acpy-skyzh-queue-cpu COMMAND ${Python3_EXECUTABLE} -m pycircuit.examples.skyzh_ooo.tests.verify
    --compiled $<TARGET_FILE:acpy-skyzh-compiled> --emitted $<TARGET_FILE:acpy-skyzh-emitted>
    --no-opt $<TARGET_FILE:acpy-skyzh-noopt> --reference "${SKYZH_REFERENCE_BINARY}"
    --output "${CMAKE_CURRENT_BINARY_DIR}/evidence")
  set_tests_properties(acpy-skyzh-queue-cpu PROPERTIES WORKING_DIRECTORY "${REPO}" TIMEOUT 900)
endif()
