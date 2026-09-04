find_package(GTest CONFIG REQUIRED)
string(JOIN "|" ACIR_TEST_LLVM_INCLUDE_DIRS ${LLVM_INCLUDE_DIRS})
string(JOIN "|" ACIR_TEST_LLVM_LINK_FLAGS ${ACIR_LLVM_LINK_FLAGS})
set(ACIR_TEST_SANITIZER_FLAG "")
set(ACIR_TEST_RTTI_FLAG "")
if(CMAKE_CXX_FLAGS MATCHES "-fsanitize=address")
  set(ACIR_TEST_SANITIZER_FLAG "-fsanitize=address")
elseif(CMAKE_CXX_FLAGS MATCHES "-fsanitize=undefined")
  set(ACIR_TEST_SANITIZER_FLAG "-fsanitize=undefined")
endif()
if(NOT LLVM_ENABLE_RTTI)
  set(ACIR_TEST_RTTI_FLAG "-fno-rtti")
endif()
add_executable(CodeGenTests
  BuildTest.cpp
  CodeGenTest.cpp
  GeneratedModelCompileTest.cpp
  GeneratedModelRuntimeTest.cpp
  GeneratorTest.cpp
  ModelPlanTest.cpp
  QueueGraphPlanTest.cpp
)
target_include_directories(CodeGenTests PRIVATE
  ${ACIR_SOURCE_DIR}/include
  ${ACIR_SOURCE_DIR}/lib/CodeGen
)
target_link_libraries(CodeGenTests PRIVATE
  ACIRCodeGen
  ACIRTransforms
  gfsim
  MLIRIndexDialect
  MLIRParser
  GTest::gtest_main
)
target_compile_definitions(CodeGenTests PRIVATE
  ACSIM_VALID_TEST_FILE="${ACIR_REPO_ROOT}/tests/mlir/agentic-circuit/ACSim/ops-valid.mlir"
  ACSIM_PROCESS_CONTROL_FLOW_TEST_FILE="${ACIR_REPO_ROOT}/tests/mlir/agentic-circuit/CodeGen/process-control-flow.mlir"
  ACIR_TEST_BINARY_DIR="${ACIR_BINARY_DIR}"
  ACIR_TEST_CXX_COMPILER="${CMAKE_CXX_COMPILER}"
  ACIR_TEST_LLVM_INCLUDE_DIRS="${ACIR_TEST_LLVM_INCLUDE_DIRS}"
  ACIR_TEST_LLVM_LINK_FLAGS="${ACIR_TEST_LLVM_LINK_FLAGS}"
  ACIR_TEST_RTTI_FLAG="${ACIR_TEST_RTTI_FLAG}"
  ACIR_TEST_SANITIZER_FLAG="${ACIR_TEST_SANITIZER_FLAG}"
  ACIR_TEST_SOURCE_DIR="${ACIR_REPO_ROOT}"
)
add_test(NAME CodeGenTests COMMAND CodeGenTests)
