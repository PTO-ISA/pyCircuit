add_executable(acir-source-unit-harness acir-source-unit-harness.cpp)
target_link_libraries(acir-source-unit-harness PRIVATE
  acir_project_options
  ACIRDialect
  ACIRSourceCompiler
  MLIRArithDialect
  MLIRFuncDialect
  MLIRParser
  MLIRSupport
)
