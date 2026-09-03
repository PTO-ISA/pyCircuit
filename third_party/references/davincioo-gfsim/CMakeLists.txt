cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(DavinciOOGfsimReference LANGUAGES CXX)
  include(CTest)
endif()

set(DAVINCIOO_GFSIM_REFERENCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/upstream")

add_executable(davincioo-gfsim-reference
  upstream/src/backend/cube.cpp
  upstream/src/backend/dispatch.cpp
  upstream/src/backend/engine.cpp
  upstream/src/backend/issue_queue.cpp
  upstream/src/backend/rename.cpp
  upstream/src/backend/ready_table.cpp
  upstream/src/backend/rob.cpp
  upstream/src/backend/scalar.cpp
  upstream/src/backend/tma.cpp
  upstream/src/backend/vector.cpp
  upstream/src/pto_inst.cpp
  upstream/src/frontend/trace.cpp
  upstream/src/frontend/trace_source_module.cpp
  upstream/src/model_top/config.cpp
  upstream/src/model_top/cli.cpp
  upstream/src/model_top/core.cpp
  upstream/src/model_top/core_system.cpp
  upstream/src/model_top/kanata_pipeview.cpp
  upstream/src/model_top/perfetto_trace.cpp
  upstream/src/model_top/main.cpp
)

target_compile_features(davincioo-gfsim-reference PRIVATE cxx_std_20)
set_target_properties(davincioo-gfsim-reference PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(davincioo-gfsim-reference PRIVATE
  "${DAVINCIOO_GFSIM_REFERENCE_ROOT}/include"
  "${DAVINCIOO_GFSIM_REFERENCE_ROOT}/src"
)

if(BUILD_TESTING)
  add_test(
    NAME DavinciOOGfsimReferenceHelp
    COMMAND $<TARGET_FILE:davincioo-gfsim-reference> --help
  )
  add_test(
    NAME DavinciOOGfsimReferenceSmoke
    COMMAND $<TARGET_FILE:davincioo-gfsim-reference>
      simulate
      --trace
      "${DAVINCIOO_GFSIM_REFERENCE_ROOT}/tests/fixtures/traces/examples_intermediate_softmax.pto.trace"
  )
  set_tests_properties(DavinciOOGfsimReferenceHelp PROPERTIES
    LABELS "reference;davincioo"
    PASS_REGULAR_EXPRESSION "gfsim simulate"
    TIMEOUT 10
  )
  set_tests_properties(DavinciOOGfsimReferenceSmoke PROPERTIES
    LABELS "reference;davincioo"
    PASS_REGULAR_EXPRESSION "simulated_cycles: 453"
    TIMEOUT 10
  )
endif()
