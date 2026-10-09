add_executable(pycircuit-opt pycircuit-opt.cpp)
target_link_libraries(pycircuit-opt PRIVATE
  pycircuit_project_options
  ACIRDialect
  ACIRSourceCompiler
  ACIRTransforms
  MLIRArithDialect
  MLIRFuncDialect
  MLIROptLib
  MLIRTransforms
)
