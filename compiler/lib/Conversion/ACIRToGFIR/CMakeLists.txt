# ACIRToGFIR: the ACIR→GFIR conversion stage (C-package design §1.2/B2 tail
# #1). It owns transaction routing (RouteTransactionsPass /
# ac-route-transactions), the routing preparation and materialization, the
# original-source authentication proof cluster (the Transaction*.inc files
# below) and the staged value-availability clone. It consumes the staged
# analysis, both dialects and the routed package admission; the GFIR→HWIR
# refinement lives in the sibling GFIRToHWIR target, and pycircuit_verify_layering
# forbids any edge between the two conversion stages.
add_acir_library(ACIRToGFIR SOURCES
  RouteTransactions.cpp
  TransactionValueAvailability.cpp
  PassRegistration.cpp
  LINK_LIBRARIES
    ACIRConversionCommon
    ACIRDialect
    ACIRAnalysis
    GFIRValidation
    GFIRDialect
    PycircuitStateInterfaces
    MLIRPass
    MLIRArithDialect
    MLIRSCFDialect
)
target_include_directories(ACIRToGFIR PRIVATE
  "${PYCIRCUIT_SOURCE_DIR}/lib"
)
target_include_directories(ACIRToGFIR PUBLIC
  "$<BUILD_INTERFACE:${PYCIRCUIT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${PYCIRCUIT_BINARY_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
