# Native source-owned emitter used by the public publication driver.
add_executable(pycircuit-emit pycircuit-emit.cpp)
target_link_libraries(pycircuit-emit PRIVATE
  pycircuit_project_options
  ACIRDialect
  ACIRCodegen
  MLIRArithDialect
  MLIRFuncDialect
  MLIRParser
  MLIRSupport
)
