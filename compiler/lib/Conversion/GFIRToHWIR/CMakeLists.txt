# GFIRToHWIR: the GFIR→HWIR refinement stage (C-package design §2/B2 tail
# #2). It owns LowerTransactionsPass (ac-lower-transactions), the physical
# storage materialization of verified routed logical state and the exact
# HardwareStateProof correspondence proof (TransactionHardwareState.inc). It
# re-verifies routed admission, then runs the final HWIR package admission;
# the refinement proof stays here while verifyHardwarePackage stays in
# HWIRValidation. pycircuit_verify_layering forbids any edge to the sibling
# ACIRToGFIR conversion stage or to the emitters.
add_acir_library(GFIRToHWIR SOURCES
  LowerTransactions.cpp
  PassRegistration.cpp
  LINK_LIBRARIES
    ACIRConversionCommon
    ACIRDialect
    ACIRAnalysis
    GFIRValidation
    GFIRDialect
    HWIRValidation
    PycircuitStateInterfaces
    MLIRPass
)
target_include_directories(GFIRToHWIR PRIVATE
  "${PYCIRCUIT_SOURCE_DIR}/lib"
)
target_include_directories(GFIRToHWIR PUBLIC
  "$<BUILD_INTERFACE:${PYCIRCUIT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${PYCIRCUIT_BINARY_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
