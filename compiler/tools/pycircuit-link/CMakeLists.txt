add_executable(pycircuit-link pycircuit-link.cpp)
target_link_libraries(pycircuit-link PRIVATE
  pycircuit_project_options
  ACIRDialect
  ACIRSourceCompiler
  MLIRArithDialect
  MLIRFuncDialect
  MLIRParser
  MLIRSupport
)
