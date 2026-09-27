add_acir_library(ACIRSourceCompiler SOURCES
  PythonImportAST.cpp
  PythonImportContext.cpp
  PythonImportHelpers.cpp
  PythonImportRecords.cpp
  PythonImportSignature.cpp
  SourceHeaderRegistry.cpp
  SourceHeaderHelpers.cpp
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
