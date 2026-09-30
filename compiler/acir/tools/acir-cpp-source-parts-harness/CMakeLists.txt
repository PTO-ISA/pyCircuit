# Private test harness; deliberately no install rule or public CLI wiring.
add_executable(acir-cpp-source-parts-harness acir-cpp-source-parts-harness.cpp)
target_link_libraries(acir-cpp-source-parts-harness PRIVATE
  acir_project_options
  ACIRDialect
  ACIRSourceCompiler
  MLIRArithDialect
  MLIRFuncDialect
  MLIRParser
  MLIRSupport
)
