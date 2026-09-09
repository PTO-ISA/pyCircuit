add_library(gfsim STATIC
  model_input.cpp
  observation.cpp
  showcase.cpp
  system.cpp
)
target_include_directories(gfsim PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
target_compile_features(gfsim PUBLIC cxx_std_20)
set_target_properties(gfsim PROPERTIES EXPORT_NAME Gfsim)

install(TARGETS gfsim EXPORT AgenticCircuitRuntimeTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
