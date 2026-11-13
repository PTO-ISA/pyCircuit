add_executable(pycircuit-source-unit source-unit.cpp)
target_link_libraries(pycircuit-source-unit PRIVATE
  pycircuit_project_options
  ACIRDialect
  ACIRAnalysis
  ACIRSourceCompiler
  MLIRArithDialect
  MLIRFuncDialect
  MLIRParser
  MLIRSupport
)

add_executable(pycircuit-link link.cpp)
target_link_libraries(pycircuit-link PRIVATE
  pycircuit_project_options
  ACIRDialect
  ACIRAnalysis
  GFIRValidation
  HWIRValidation
  GFIRDialect
  ACIRSourceCompiler
  MLIRArithDialect
  MLIRFuncDialect
  MLIRParser
  MLIRSupport
)

# Native source-owned emitter used by the public publication driver.
add_executable(pycircuit-emit emit.cpp)
target_link_libraries(pycircuit-emit PRIVATE
  pycircuit_project_options
  ACIRDialect
  ACIRAnalysis
  HWIRValidation
  GFIRDialect
  ACIRCodegen
  MLIRArithDialect
  MLIRFuncDialect
  MLIRParser
  MLIRSupport
)

add_executable(pycircuit-opt opt.cpp)
target_link_libraries(pycircuit-opt PRIVATE
  pycircuit_project_options
  ACIRDialect
  ACIRAnalysis
  GFIRValidation
  HWIRValidation
  GFIRDialect
  ACIRToGFIR
  GFIRToHWIR
  ACIRSourceCompiler
  ACIRTransforms
  MLIRArithDialect
  MLIRFuncDialect
  MLIROptLib
  MLIRTransforms
)
