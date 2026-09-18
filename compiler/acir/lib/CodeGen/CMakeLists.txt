add_acir_library(ACIRCodeGen SOURCES
  QueueBlockContract.cpp
  QueueGraphCost.cpp
  QueueGraphPlan.cpp
  QueueGraphGenerator.cpp
  QueueGraphPyc.cpp
)
string(JOIN "|" ACIR_CODEGEN_LLVM_INCLUDE_DIRS ${LLVM_INCLUDE_DIRS})
target_compile_definitions(ACIRCodeGen PRIVATE
  ACIR_CODEGEN_LLVM_INCLUDE_DIRS="${ACIR_CODEGEN_LLVM_INCLUDE_DIRS}"
)
target_link_libraries(ACIRCodeGen PUBLIC
  ACIRAnalysis
  ACIRBindings
  ACIRDialect
  MLIRArithDialect
  MLIRControlFlowDialect
  MLIRIndexDialect
  MLIRPass
  LLVMSupport
)
target_include_directories(ACIRCodeGen PUBLIC
  "$<BUILD_INTERFACE:${ACIR_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${ACIR_BINARY_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
add_library(AgenticCircuit::ACIRCodeGen ALIAS ACIRCodeGen)
