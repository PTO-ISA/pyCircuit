add_acir_library(ACIRTransforms SOURCES
  VerifyHardware.cpp
  ExtractSourceInterface.cpp
  SimplifyRecordWires.cpp
  LINK_LIBRARIES
    ACIRDialect
    MLIRPass
    MLIRTransformUtils
)
target_include_directories(ACIRTransforms PRIVATE
  "${PYCIRCUIT_SOURCE_DIR}/lib"
)
target_include_directories(ACIRTransforms PUBLIC
  "$<BUILD_INTERFACE:${PYCIRCUIT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${PYCIRCUIT_BINARY_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
