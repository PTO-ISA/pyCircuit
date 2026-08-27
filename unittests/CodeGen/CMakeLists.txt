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
  EmitCxxTest.cpp
  GeneratedModelCompileTest.cpp
  GeneratedModelRuntimeTest.cpp
  GeneratorTest.cpp
  ModelPlanTest.cpp
  QueueGraphPlanTest.cpp
)
target_include_directories(CodeGenTests PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/lib
  ${PROJECT_SOURCE_DIR}/lib/CodeGen
)
target_link_libraries(CodeGenTests PRIVATE
  ACIRCodeGen
  ACIRToACSim
  ACIRTransforms
  ACIRDialect
  ACSimDialect
  gfsim
  MLIRIndexDialect
  MLIRParser
  MLIRPass
  LLVMSupport
  GTest::gtest_main
)
target_compile_definitions(CodeGenTests PRIVATE
  ACSIM_VALID_TEST_FILE="${PROJECT_SOURCE_DIR}/test/ACSim/ops-valid.mlir"
  ACSIM_PROCESS_CONTROL_FLOW_TEST_FILE="${PROJECT_SOURCE_DIR}/test/CodeGen/process-control-flow.mlir"
  ACIR_TEST_BINARY_DIR="${PROJECT_BINARY_DIR}"
  ACIR_TEST_CXX_COMPILER="${CMAKE_CXX_COMPILER}"
  ACIR_TEST_LLVM_INCLUDE_DIRS="${ACIR_TEST_LLVM_INCLUDE_DIRS}"
  ACIR_TEST_LLVM_LINK_FLAGS="${ACIR_TEST_LLVM_LINK_FLAGS}"
  ACIR_TEST_RTTI_FLAG="${ACIR_TEST_RTTI_FLAG}"
  ACIR_TEST_SANITIZER_FLAG="${ACIR_TEST_SANITIZER_FLAG}"
  ACIR_TEST_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
  ACIR_GFSIM_INCLUDE="${PROJECT_SOURCE_DIR}/include"
  ACIR_GFSIM_LIBRARY="$<TARGET_FILE:gfsim>"
  ACIR_EXAMPLES_DIR="${PROJECT_SOURCE_DIR}/examples"
)
add_dependencies(CodeGenTests gfsim)
add_test(NAME CodeGenTests COMMAND CodeGenTests)
