add_executable(acir-design-harness acir-design-harness.cpp)
target_link_libraries(acir-design-harness PRIVATE
  AgenticCircuit::ProjectOptions
  ACIRDialect
  ACIRSourceCompiler
  MLIRArithDialect
  MLIRFuncDialect
  MLIRParser
  MLIRSupport
)
