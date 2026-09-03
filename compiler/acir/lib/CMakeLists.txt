# LLVM's own build defaults to no RTTI. Match that contract for MLIR-facing
# targets. gfsim is a sibling subsystem under simulator/ and owns its flags.
if(NOT LLVM_ENABLE_RTTI)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
endif()

add_subdirectory(Dialect)
add_subdirectory(Analysis)
add_subdirectory(Bindings)
add_subdirectory(Transforms)
add_subdirectory(Conversion)
add_subdirectory(CodeGen)
add_subdirectory(Compiler)

# Install every compiler library that owns symbols declared by public headers.
install(
  TARGETS
    ACIRDialect
    ACSimDialect
    ACIRBindings
    ACIRAnalysis
    ACIRTransforms
    ACIRToACSim
    ACIRCodeGen
    ACIRCompiler
  EXPORT AgenticCircuitTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)
