add_acir_library(ACIRSourceCompiler SOURCES
  FixedUnsignedDivRemLowering.cpp
  NumericLowering.cpp
  PythonImportAST.cpp
  PythonImportCaseSyntax.cpp
  PythonImportContext.cpp
  PythonImportEnumSyntax.cpp
  PythonImportRecords.cpp
  SourceHeaderRegistry.cpp
  SourceHeaderHelpers.cpp
  SourceBodySnapshots.cpp
  SourceDependencies.cpp
  SourceDeclarationContext.cpp
  SourceLink.cpp
  SourceNamespace.cpp
  SourceMemoryBindings.cpp
  SourceRuleWrites.cpp
  SourceUnit.cpp
  TableQueryLowering.cpp
  TableQueryStages.cpp
  LINK_LIBRARIES
    ACIRDialect
    ACIRTransforms
    MLIRArithDialect
    MLIRFuncDialect
    MLIRParser
    MLIRPass
    MLIRSCFDialect
)
target_include_directories(ACIRSourceCompiler PUBLIC
  "$<BUILD_INTERFACE:${PYCIRCUIT_SOURCE_DIR}/lib>"
  "$<INSTALL_INTERFACE:include>"
)

add_acir_library(ACIRCodegen SOURCES
  FinalCppNames.cpp
  FinalEmitCpp.cpp
  FinalEmitVerilog.cpp
  FinalSourceMap.cpp
  HardwareEmitCommon.cpp
  HardwareEmitCppChecks.cpp
  HardwareEmitCppDefinition.cpp
  HardwareEmitVerilogDefinition.cpp
  LINK_LIBRARIES ACIRDialect MLIRParser
)
target_include_directories(ACIRCodegen PUBLIC
  "$<BUILD_INTERFACE:${PYCIRCUIT_SOURCE_DIR}/lib>"
  "$<INSTALL_INTERFACE:include>"
)
