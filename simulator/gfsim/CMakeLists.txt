add_library(gfsim STATIC
  harness.cpp
  npu.cpp
  observation.cpp
  pto_trace.cpp
  showcase.cpp
  system.cpp
  trace.cpp
)
target_include_directories(gfsim PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
target_compile_features(gfsim PUBLIC cxx_std_20)
target_link_libraries(gfsim PUBLIC ACIRBindings)
if(NOT LLVM_ENABLE_RTTI)
  target_compile_options(gfsim PRIVATE -fno-rtti)
endif()
set_target_properties(gfsim PROPERTIES EXPORT_NAME Gfsim)

set(GFSIM_LLVM_HEADER_SOURCE "")
set(GFSIM_LLVM_C_HEADER_SOURCE "")
foreach(_llvm_include_root IN LISTS LLVM_INCLUDE_DIRS)
  if(IS_DIRECTORY "${_llvm_include_root}/llvm" AND
     IS_DIRECTORY "${_llvm_include_root}/llvm-c")
    file(REAL_PATH "${_llvm_include_root}/llvm" GFSIM_LLVM_HEADER_SOURCE)
    file(REAL_PATH "${_llvm_include_root}/llvm-c" GFSIM_LLVM_C_HEADER_SOURCE)
    break()
  endif()
endforeach()
if(NOT GFSIM_LLVM_HEADER_SOURCE OR NOT GFSIM_LLVM_C_HEADER_SOURCE)
  message(FATAL_ERROR "gfsim requires the LLVM public headers")
endif()

install(TARGETS gfsim EXPORT AgenticCircuitTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(DIRECTORY "${GFSIM_LLVM_HEADER_SOURCE}/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/llvm")
install(DIRECTORY "${GFSIM_LLVM_C_HEADER_SOURCE}/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/llvm-c")
