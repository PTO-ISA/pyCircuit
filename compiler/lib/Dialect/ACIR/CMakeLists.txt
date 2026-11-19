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
  HardwareCDCLocalVerification.cpp
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

# ACIRAnalysis: the staged hardware/source analysis above the operations
# archive (B2 step-2 design r3 §4 MOVE-WHOLE rows, adjusted at IX for the
# current TU set). It links ACIRDialect because its members drive the
# generated verifiers (verifyInvariantsImpl, custom verify bodies) and the
# shared grammar helpers homed in the operations TUs; the reverse edge
# (ACIRDialect referencing analysis symbols) is forbidden and enforced by
# pycircuit_verify_layering and the two layer probes.
add_acir_library(ACIRAnalysis SOURCES
  HardwareCDC.cpp
  HardwareCDCGraph.cpp
  HardwareDomains.cpp
  HardwarePhysicalEvents.cpp
  HardwareAnalysis.cpp
  HardwareAnalysisDependencies.cpp
  HardwareAnalysisValidation.cpp
  HardwareDependencySerialization.cpp
  HardwarePhysicalProjection.cpp
  HardwarePhysicalState.cpp
  HardwareQueueResources.cpp
  HardwareSampledAdmission.cpp
  HardwareSourceCheckPreflight.cpp
  HardwareSourceChecks.cpp
  HardwareValueAvailability.cpp
  HardwareWorkPartition.cpp
  PackageVerification.cpp
  SourceHelperAnalysis.cpp
  SourceMethodAnalysis.cpp
  SourceMethodResolution.cpp
  SourceTransactionStateAnalysis.cpp
  SourceTransactionSummary.cpp
  CollectionAnalysis.cpp
  EnumAnalysis.cpp
  StructBoundChecks.cpp
  QueueBoundChecks.cpp
  BitsBoundChecks.cpp
  ModuleDefinitionBoundChecks.cpp
  TransactionBoundChecks.cpp
  EntryBoundaryEnvelope.cpp
  SourceUnitValidation.cpp
  FinalSourceUnitValidation.cpp
  SourceRegistrationEntry.cpp
  HardwareGrantProof.cpp
  "${PYCIRCUIT_SOURCE_DIR}/lib/Analysis/LogicalStateValidation.cpp"
)
add_dependencies(ACIRAnalysis
  ACIRDialectIncGen
  ACIRAttributesIncGen
  ACIRTypesIncGen
  ACIROpsIncGen
)
target_link_libraries(ACIRAnalysis PUBLIC
  ACIRCore
  ACIRDialect
  PycircuitStateInterfaces
  LLVMCore
  MLIRArithDialect
  MLIRFuncDialect
  MLIRSCFDialect
  MLIRSideEffectInterfaces
)
target_include_directories(ACIRAnalysis PUBLIC
  "$<BUILD_INTERFACE:${PYCIRCUIT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${PYCIRCUIT_BINARY_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
target_include_directories(ACIRAnalysis PRIVATE
  "${PYCIRCUIT_SOURCE_DIR}/lib"
)

# GFIRValidation / HWIRValidation: the two package-verification entrypoints
# (C-package design §2). Each links ACIRAnalysis plus MLIRPass — never each
# other and never GFIRDialect: the closed routed admission compares REGISTERED
# op NAME strings, so the shared walk needs no GFIR types. The sibling
# independence and the upper edges are enforced by pycircuit_verify_layering
# and the two standalone probes in tests/compiler/IR.
add_acir_library(GFIRValidation SOURCES
  GFIRPackageValidation.cpp
  LINK_LIBRARIES
    ACIRAnalysis
    MLIRPass
)

add_acir_library(HWIRValidation SOURCES
  HWIRPackageValidation.cpp
  LINK_LIBRARIES
    ACIRAnalysis
    MLIRPass
)
