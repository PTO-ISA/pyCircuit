# ACIRTransforms keeps the source-stage transforms only (C-package design
# B2 tail #6): the transaction routing/refinement conversion moved to the
# Conversion/ACIRToGFIR and Conversion/GFIRToHWIR targets, which own their
# pass registration. registerACIRPasses registers these five source passes.
add_acir_library(ACIRTransforms SOURCES
  PassRegistration.cpp
  NormalizeSourceControlFlow.cpp
  ExtractSourceInterface.cpp
  SimplifyRecordWires.cpp
  AnalyzeSourceMethods.cpp
  AnalyzeTransactionStateSSA.cpp
  LINK_LIBRARIES
    ACIRDialect
    ACIRAnalysis
    PycircuitStateInterfaces
    MLIRPass
    MLIRArithDialect
    MLIRSCFDialect
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
