add_library(parallel_gfsim STATIC
  src/Logger.cpp
  src/Reporter.cpp
  src/SimAssert.cpp
  src/SimExecutor.cpp
  src/SimGraph.cpp
  src/SimMemory.cpp
  src/SimModule.cpp
  src/SimSystem.cpp
  src/SimTestbench.cpp
  src/SimTrace.cpp
)
add_library(pycircuit::parallel_gfsim ALIAS parallel_gfsim)
target_link_libraries(parallel_gfsim PUBLIC Threads::Threads)
target_compile_features(parallel_gfsim PUBLIC cxx_std_11)
target_include_directories(parallel_gfsim PUBLIC
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

install(TARGETS parallel_gfsim EXPORT pycircuitRuntimeTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES
  Logger.h Reporter.h RID.h SimAssert.h SimMemory.h SimModule.h
  SimQueue.h SimState.h SimSystem.h SimTestbench.h SimTrace.h
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/parallel_gfsim")
