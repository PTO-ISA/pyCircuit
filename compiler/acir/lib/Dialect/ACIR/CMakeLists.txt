add_acir_library(ACIRDialect SOURCES
  ACIRDialect.cpp
  ACIRInterfaces.cpp
  ACIROps.cpp
  ACIRResources.cpp
  ACIRTypes.cpp
  GraphRegion.cpp
  ProcessLowerability.cpp
)
add_dependencies(ACIRDialect
  ACIRDialectIncGen
  ACIRAttributesIncGen
  ACIRTypesIncGen
  ACIROpsIncGen
  ACIROpInterfacesIncGen
)
target_link_libraries(ACIRDialect PUBLIC
  MLIRControlFlowInterfaces
  MLIRDLTIDialect
  MLIRDataLayoutInterfaces
  MLIRFunctionInterfaces
  MLIRFuncDialect
  MLIRPresburger
  MLIRSCFDialect
  MLIRSideEffectInterfaces
)
target_include_directories(ACIRDialect PUBLIC
  "$<BUILD_INTERFACE:${ACIR_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${ACIR_BINARY_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
add_library(AgenticCircuit::ACIRDialect ALIAS ACIRDialect)
