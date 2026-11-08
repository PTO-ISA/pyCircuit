# ACIRCore: generated type/attribute definitions and the grammar/contract
# leaf TUs below the operations layer (B2 step-2 design r9 §1). GFIRDialect
# links exactly this plus the shared interfaces. Every member here is
# self-contained: no concrete-op out-of-line definitions, no staged analysis.
# The WHOLE_ARCHIVE core probe in tests/compiler/IR pulls every member and
# fails at link time if any of them references a symbol above this layer.
add_acir_library(ACIRCore SOURCES
  ACIRCoreDefs.cpp
  ACIRPacketTypes.cpp
  ACIRIdentityContracts.cpp
  ACIRSourceContracts.cpp
  SourceStaticValueContracts.cpp
  ACIRStaticContracts.cpp
  SourceAttributeContracts.cpp
  SourceSyntaxSiteContracts.cpp
  SourceValueSemantics.cpp
  "${PYCIRCUIT_SOURCE_DIR}/lib/IR/StateDeclarationContracts.cpp"
  "${PYCIRCUIT_SOURCE_DIR}/lib/Compiler/SourceIdentifier.cpp"
)
add_dependencies(ACIRCore
  ACIRDialectIncGen
  ACIRAttributesIncGen
  ACIRTypesIncGen
  ACIROpsIncGen
)
target_link_libraries(ACIRCore PUBLIC
  MLIRFuncDialect
  PycircuitStateInterfaces
)
target_include_directories(ACIRCore PUBLIC
  "$<BUILD_INTERFACE:${PYCIRCUIT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${PYCIRCUIT_BINARY_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
# ACIRSourceContracts.h and SourceIdentifierUnicodeData.h live under lib/.
target_include_directories(ACIRCore PRIVATE
  "$<BUILD_INTERFACE:${PYCIRCUIT_SOURCE_DIR}/lib>"
)

add_acir_library(ACIRDialect SOURCES
  ACIRStaticEvaluation.cpp
  SourceStaticRecordConstructors.cpp
  SourceStaticDomains.cpp
  TransactionalContracts.cpp
  HardwarePackedWidth.cpp
  TransactionalLinkedChecks.cpp
  ACIRModuleContracts.cpp
  EntryBoundary.cpp
  SourceNamespaceValidation.cpp
  ACIRTransactionOps.cpp
  ACIRModuleOps.cpp
  ACIRQueueOps.cpp
  ACIRSourceControlFlowOps.cpp
  ACIRSystemOps.cpp
  ACIRPacketOps.cpp
  ACIRBitsOps.cpp
  CollectionLocalVerifies.cpp
  ACIREnumOps.cpp
  ACIRConstantOps.cpp
  ACIRDialect.cpp
  ACIRMathOps.cpp
  ACIRNumericProofOps.cpp
  ACIRRegistration.cpp
  SourceEffects.cpp
)
add_dependencies(ACIRDialect
  ACIRDialectIncGen
  ACIRAttributesIncGen
  ACIRTypesIncGen
  ACIROpsIncGen
)
target_link_libraries(ACIRDialect PUBLIC
  ACIRCore
  PycircuitStateInterfaces
  LLVMCore
  MLIRArithDialect
  MLIRFuncDialect
  MLIRSCFDialect
  MLIRSideEffectInterfaces
)
target_include_directories(ACIRDialect PUBLIC
  "$<BUILD_INTERFACE:${PYCIRCUIT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${PYCIRCUIT_BINARY_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
target_include_directories(ACIRDialect PRIVATE
  "${PYCIRCUIT_SOURCE_DIR}/lib"
)

# component-split: superseded by pr/analysis (the ACIRAnalysis target
# definition returns with the staged analysis component).
