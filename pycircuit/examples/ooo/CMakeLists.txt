set(OOO_SOURCES model.py types.py frontend.py issue.py execute.py commit.py)
set(OOO_COMPILED "${CMAKE_CURRENT_BINARY_DIR}/compiled")
set(OOO_EMITTED "${CMAKE_CURRENT_BINARY_DIR}/emitted")
add_custom_command(OUTPUT "${OOO_COMPILED}/model.cpp" "${OOO_COMPILED}/model.hpp" "${OOO_COMPILED}/model.acir.mlir" "${OOO_COMPILED}/ac_support.hpp"
  COMMAND ${CMAKE_COMMAND} -E env "ACPY_MLIR_COMPILER=$<TARGET_FILE:acir-compile>" ${Python3_EXECUTABLE} -m pycircuit compile "${CMAKE_CURRENT_SOURCE_DIR}/model.py" --top CPU --output "${OOO_COMPILED}"
  WORKING_DIRECTORY "${REPO}"
  DEPENDS acir-compile ${COMPILER_INPUTS} ${OOO_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/../ripes5/logic.py" VERBATIM)
add_custom_target(acpy-ooo-compile DEPENDS "${OOO_COMPILED}/model.cpp" "${OOO_COMPILED}/model.hpp")
add_custom_command(OUTPUT "${OOO_EMITTED}/model.cpp" "${OOO_EMITTED}/model.hpp" "${OOO_EMITTED}/model.acir.mlir" "${OOO_EMITTED}/ac_support.hpp"
  COMMAND ${CMAKE_COMMAND} -E env "ACPY_MLIR_COMPILER=$<TARGET_FILE:acir-compile>" ${Python3_EXECUTABLE} -m pycircuit emit "${OOO_COMPILED}/model.acir.mlir" --output "${OOO_EMITTED}"
  WORKING_DIRECTORY "${REPO}"
  DEPENDS acir-compile ${COMPILER_INPUTS} "${OOO_COMPILED}/model.acir.mlir" VERBATIM)
add_custom_target(acpy-ooo-emit DEPENDS "${OOO_EMITTED}/model.cpp" "${OOO_EMITTED}/model.hpp")
add_dependencies(acpy-ooo-emit acpy-ooo-compile)
foreach(variant IN ITEMS compiled emitted)
  set(dir "${CMAKE_CURRENT_BINARY_DIR}/${variant}")
  add_library(acpy-ooo-model-${variant} STATIC "${dir}/model.cpp")
  if(variant STREQUAL "compiled")
    add_dependencies(acpy-ooo-model-${variant} acpy-ooo-compile)
  else()
    add_dependencies(acpy-ooo-model-${variant} acpy-ooo-emit)
  endif()
  target_include_directories(acpy-ooo-model-${variant} PUBLIC "${dir}")
  target_link_libraries(acpy-ooo-model-${variant} PUBLIC gfsim::gfsim)
  add_executable(acpy-ooo-${variant} runner.cpp)
  target_link_libraries(acpy-ooo-${variant} PRIVATE acpy-ooo-model-${variant})
endforeach()
if(BUILD_TESTING)
  add_test(NAME acpy-ooo-end-to-end COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/verify.py"
    --compiled $<TARGET_FILE:acpy-ooo-compiled> --emitted $<TARGET_FILE:acpy-ooo-emitted>
    --output "${CMAKE_CURRENT_SOURCE_DIR}/output/${CMAKE_BUILD_TYPE}")
  set_tests_properties(acpy-ooo-end-to-end PROPERTIES TIMEOUT 300)
endif()
if(BUILD_TESTING)
  add_test(NAME acpy-ooo-tools COMMAND ${Python3_EXECUTABLE} -m unittest pycircuit.examples.ooo.test_tools -v)
  set_tests_properties(acpy-ooo-tools PROPERTIES WORKING_DIRECTORY "${REPO}")
  add_test(NAME acpy-ooo-input-gates COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/input_gates.py" $<TARGET_FILE:acpy-ooo-compiled>)
endif()
