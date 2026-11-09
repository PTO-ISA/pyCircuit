add_executable(pycircuit-source-unit source-unit.cpp)
target_link_libraries(pycircuit-source-unit PRIVATE
  pycircuit_project_options
  ACIRSourceCompiler
)

add_executable(pycircuit-link link.cpp)
target_link_libraries(pycircuit-link PRIVATE
  pycircuit_project_options
  ACIRSourceCompiler
)

# Native source-owned emitter used by the public publication driver.
add_executable(pycircuit-emit emit.cpp)
target_link_libraries(pycircuit-emit PRIVATE
  pycircuit_project_options
  ACIRCodegen
)

add_executable(pycircuit-opt opt.cpp)
target_link_libraries(pycircuit-opt PRIVATE
  pycircuit_project_options
  ACIRSourceCompiler
  ACIRTransforms
  MLIROptLib
  MLIRTransforms
)
