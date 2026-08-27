add_library(gfsim STATIC
  harness.cpp
  npu.cpp
  observation.cpp
  pto_trace.cpp
  showcase.cpp
  system.cpp
  trace.cpp
)
add_dependencies(gfsim acir_runtime_headers)
target_include_directories(gfsim PUBLIC
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
target_compile_features(gfsim PUBLIC cxx_std_20)
target_link_libraries(gfsim PUBLIC ACIRBindings)
if(NOT LLVM_ENABLE_RTTI)
  target_compile_options(gfsim PRIVATE -fno-rtti)
endif()
set_target_properties(gfsim PROPERTIES EXPORT_NAME Gfsim)
