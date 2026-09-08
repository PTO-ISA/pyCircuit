add_library(gfsim STATIC
  npu.cpp
  observation.cpp
  pto_trace.cpp
  showcase.cpp
  system.cpp
)
target_include_directories(gfsim PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
target_compile_features(gfsim PUBLIC cxx_std_20)
set_target_properties(gfsim PROPERTIES EXPORT_NAME Gfsim)

add_library(gfsim_tooling STATIC
  harness.cpp
  trace.cpp
)
target_include_directories(gfsim_tooling PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tooling/include>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include/agentic-circuit-tooling>"
  "$<INSTALL_INTERFACE:include>"
)
target_compile_features(gfsim_tooling PUBLIC cxx_std_20)
target_link_libraries(gfsim_tooling PUBLIC gfsim ACIRBindings)
if(NOT LLVM_ENABLE_RTTI)
  target_compile_options(gfsim_tooling PRIVATE -fno-rtti)
endif()
set_target_properties(gfsim_tooling PROPERTIES EXPORT_NAME GfsimTooling)

install(TARGETS gfsim EXPORT AgenticCircuitRuntimeTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(TARGETS gfsim_tooling EXPORT AgenticCircuitCompilerDevTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tooling/include/gfsim"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/agentic-circuit-tooling")
