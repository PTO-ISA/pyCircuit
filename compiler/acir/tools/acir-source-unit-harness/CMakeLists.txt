add_executable(acir-source-unit-harness acir-source-unit-harness.cpp)
target_link_libraries(acir-source-unit-harness PRIVATE
  AgenticCircuit::ProjectOptions
  ACIRDialect
  ACIRSourceCompiler
  MLIRArithDialect
  MLIRFuncDialect
  MLIRParser
  MLIRSupport
)
