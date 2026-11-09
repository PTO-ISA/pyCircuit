add_acir_library(HWIRDialect SOURCES
  HWIRDialect.cpp
  HWIROps.cpp
  HWIRVerification.cpp
)
add_dependencies(HWIRDialect
  HWIRDialectIncGen
  HWIROpsIncGen
  ACIRDialect
)
target_link_libraries(HWIRDialect PUBLIC
  ACIRDialect
  MLIRPass
  MLIRSideEffectInterfaces
)
target_include_directories(HWIRDialect PUBLIC
  "$<BUILD_INTERFACE:${PYCIRCUIT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${PYCIRCUIT_BINARY_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
