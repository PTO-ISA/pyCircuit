add_executable(pycircuit-source-unit pycircuit-source-unit.cpp)
target_link_libraries(pycircuit-source-unit PRIVATE
  pycircuit_project_options
  ACIRDialect
  ACIRSourceCompiler
  MLIRArithDialect
  MLIRFuncDialect
  MLIRParser
  MLIRSupport
)
