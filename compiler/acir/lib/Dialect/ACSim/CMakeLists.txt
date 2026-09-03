add_acir_library(ACSimDialect SOURCES
  ACSimDialect.cpp
  ACSimOps.cpp
  ACSimTypes.cpp
)
add_dependencies(ACSimDialect
  ACSimDialectIncGen
  ACSimOpsIncGen
  ACSimTypesIncGen
)
target_link_libraries(ACSimDialect PUBLIC
  MLIRControlFlowDialect
  MLIRSideEffectInterfaces
)
target_include_directories(ACSimDialect PUBLIC
  "$<BUILD_INTERFACE:${ACIR_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${ACIR_BINARY_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
add_library(AgenticCircuit::ACSimDialect ALIAS ACSimDialect)
