add_acir_library(ACIRDialect SOURCES
  ACIRDialect.cpp
  ACIRIdentityContracts.cpp
  ACIRPacketOps.cpp
  ACIRPacketTypes.cpp
  ACIRRegistration.cpp
  ACIRSourceContracts.cpp
  ACIRValueContracts.cpp
)
add_dependencies(ACIRDialect
  ACIRDialectIncGen
  ACIRAttributesIncGen
  ACIRTypesIncGen
  ACIROpsIncGen
)
target_link_libraries(ACIRDialect PUBLIC
  MLIRFuncDialect
  MLIRSideEffectInterfaces
)
target_include_directories(ACIRDialect PUBLIC
  "$<BUILD_INTERFACE:${ACIR_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${ACIR_BINARY_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
target_include_directories(ACIRDialect PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
)
add_library(AgenticCircuit::ACIRDialect ALIAS ACIRDialect)
