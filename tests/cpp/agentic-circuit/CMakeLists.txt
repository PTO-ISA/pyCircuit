# LLVM's own build defaults to no RTTI. When linking such an LLVM build (CI
# builds LLVM from source with default flags), every unit-test target must use
# the same mode: sanitizer instrumentation otherwise emits typeinfo references
# that the no-RTTI runtime libraries cannot provide.
if(NOT LLVM_ENABLE_RTTI)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
endif()

add_subdirectory(gfsim)
add_subdirectory(Dialect)
add_subdirectory(Analysis)
add_subdirectory(Bindings)
add_subdirectory(Conversion)
add_subdirectory(CodeGen)
add_subdirectory(Compiler)
