add_library(pyc6_runtime STATIC
  model_input.cpp
  sim_executor.cpp
  system_runner.cpp
)
add_library(pycircuit::pyc6_runtime ALIAS pyc6_runtime)
find_package(Threads REQUIRED)
target_link_libraries(pyc6_runtime PUBLIC Threads::Threads)
target_include_directories(pyc6_runtime PUBLIC
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
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
  "${PROJECT_SOURCE_DIR}/include/gfsim/model_api.h"
  "${PROJECT_SOURCE_DIR}/include/gfsim/model_input.h"
  "${PROJECT_SOURCE_DIR}/include/gfsim/dff.h"
  "${PROJECT_SOURCE_DIR}/include/gfsim/fifo.h"
  "${PROJECT_SOURCE_DIR}/include/gfsim/SimObj.h"
  "${PROJECT_SOURCE_DIR}/include/gfsim/SimModule.h"
  "${PROJECT_SOURCE_DIR}/include/gfsim/WorkExecutor.h"
  "${PROJECT_SOURCE_DIR}/include/gfsim/SimSystem.h"
  "${PROJECT_SOURCE_DIR}/include/gfsim/ObservationSlot.h"
  "${PROJECT_SOURCE_DIR}/include/gfsim/SimExecutor.h"
  "${PROJECT_SOURCE_DIR}/include/gfsim/SystemRunner.h"
  "${PROJECT_SOURCE_DIR}/include/gfsim/bits.h"
  "${PROJECT_SOURCE_DIR}/include/gfsim/four_state.h"
  "${PROJECT_SOURCE_DIR}/include/gfsim/wire.h"
  "${PROJECT_SOURCE_DIR}/include/gfsim/collection.h"
  "${PROJECT_SOURCE_DIR}/include/gfsim/memory_detail.h"
  "${PROJECT_SOURCE_DIR}/include/gfsim/sync_mem.h"
  "${PROJECT_SOURCE_DIR}/include/gfsim/byte_mem.h"
  "${PROJECT_SOURCE_DIR}/include/gfsim/README.md"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/gfsim")
install(FILES
  "${PROJECT_SOURCE_DIR}/include/verilog/dff.v"
  "${PROJECT_SOURCE_DIR}/include/verilog/dffe.v"
  "${PROJECT_SOURCE_DIR}/include/verilog/fifo.v"
  "${PROJECT_SOURCE_DIR}/include/verilog/sync_mem.v"
  "${PROJECT_SOURCE_DIR}/include/verilog/sync_mem_dp.v"
  "${PROJECT_SOURCE_DIR}/include/verilog/byte_mem.v"
  "${PROJECT_SOURCE_DIR}/include/verilog/README.md"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/verilog")
install(FILES
  "${PROJECT_SOURCE_DIR}/include/CODING_STYLE.md"
  "${PROJECT_SOURCE_DIR}/include/MODULE_TEMPLATE.md"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
