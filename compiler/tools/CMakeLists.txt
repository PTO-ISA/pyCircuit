# component-split: interim tool link lines. GFIRValidation/HWIRValidation
# (link, emit, opt) and the conversion targets (opt) join with pr/validation
# and pr/conversion.
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
  GFIRDialect
  ACIRSourceCompiler
  ACIRTransforms
  MLIRArithDialect
  MLIRFuncDialect
  MLIROptLib
  MLIRTransforms
)
