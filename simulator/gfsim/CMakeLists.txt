add_library(pyc6_runtime STATIC
  model_input.cpp
  sim_executor.cpp
  system_runner.cpp
)
add_library(pycircuit::pyc6_runtime ALIAS pyc6_runtime)
target_include_directories(pyc6_runtime PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_compile_features(pyc6_runtime PUBLIC cxx_std_20)
set_target_properties(pyc6_runtime PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN YES)
install(TARGETS pyc6_runtime EXPORT pycircuitRuntimeTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES
  include/gfsim/model_api.h include/gfsim/model_input.h
  include/gfsim/SimDFF.h include/gfsim/SimModule.h
  include/gfsim/SimSystem.h include/gfsim/ObservationSlot.h
  include/gfsim/SimExecutor.h include/gfsim/SystemRunner.h
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/gfsim")
