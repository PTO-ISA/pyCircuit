# component-split: interim source-transform layer. The package-verification
# entrypoints resolve through GFIRValidation/HWIRValidation (pr/validation),
# and the transaction routing/refinement TUs stay here until pr/conversion
# moves them to the Conversion/ACIRToGFIR and Conversion/GFIRToHWIR targets.
add_acir_library(ACIRTransforms SOURCES
  PassRegistration.cpp
  NormalizeSourceControlFlow.cpp
  ExtractSourceInterface.cpp
  SimplifyRecordWires.cpp
  AnalyzeSourceMethods.cpp
  AnalyzeTransactionStateSSA.cpp
  LowerTransactions.cpp
  TransactionValueAvailability.cpp
  VerifyHardware.cpp
  LINK_LIBRARIES
    ACIRDialect
    ACIRAnalysis
    # component-split: GFIRValidation/HWIRValidation join in pr/validation.
    GFIRDialect
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
