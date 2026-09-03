find_package(GTest CONFIG REQUIRED)

add_executable(ACSimTypesTests TypesTest.cpp)
target_link_libraries(ACSimTypesTests PRIVATE
  ACSimDialect
  MLIRAsmParser
  MLIRIR
  GTest::gtest_main
)
add_test(NAME ACSimTypesTests COMMAND ACSimTypesTests)

add_executable(ACSimOpsTests OpsTest.cpp)
target_include_directories(ACSimOpsTests PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
)
target_compile_definitions(ACSimOpsTests PRIVATE
  ACSIM_VALID_TEST_FILE="${ACIR_REPO_ROOT}/tests/mlir/agentic-circuit/ACSim/ops-valid.mlir"
  ACSIM_REUSABLE_TEST_FILE="${ACIR_REPO_ROOT}/tests/mlir/agentic-circuit/ACSim/reusable-modules.mlir"
  ACSIM_WRAPPER_ACTIVATION_TEST_FILE="${ACIR_REPO_ROOT}/tests/mlir/agentic-circuit/ACSim/generated-wrapper-activation.mlir"
)
target_link_libraries(ACSimOpsTests PRIVATE
  ACSimDialect
  ACIRDialect
  MLIRArithDialect
  MLIRAsmParser
  MLIRControlFlowDialect
  MLIRIndexDialect
  MLIRIR
  MLIRParser
  MLIRSideEffectInterfaces
  GTest::gtest_main
)
add_test(NAME ACSimOpsTests COMMAND ACSimOpsTests)
