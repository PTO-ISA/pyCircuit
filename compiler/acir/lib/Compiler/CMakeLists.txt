add_acir_library(ACIRSourceCompiler SOURCES
  PythonImportAST.cpp
  PythonImportContext.cpp
  PythonImportHelpers.cpp
  PythonImportModuleAnalysis.cpp
  PythonImportModuleBody.cpp
  PythonImportModules.cpp
  PythonImportRecords.cpp
  PythonImportRules.cpp
  PythonImportSignature.cpp
  SourceHeaderRegistry.cpp
  SourceHeaderHelpers.cpp
  SourceBodySnapshots.cpp
  SourceNamespace.cpp
  SourceIdentifier.cpp
  SourceUnit.cpp
  LINK_LIBRARIES
    ACIRDialect
    MLIRArithDialect
    MLIRFuncDialect
    MLIRParser
)
target_include_directories(ACIRSourceCompiler PUBLIC
  "$<BUILD_INTERFACE:${ACIR_SOURCE_DIR}/lib>"
)
add_library(AgenticCircuit::SourceCompiler ALIAS ACIRSourceCompiler)
