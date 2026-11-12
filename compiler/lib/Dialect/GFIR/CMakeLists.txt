# GFIR registers in the same acir library namespace as the common dialect but
# owns only the routed stage's logical-state operations. It links the common
# ac IR (payload types, declaration contracts, shared state interfaces) and
# never the staged package analysis for registration.
add_acir_library(GFIRDialect SOURCES
  GFIRDialect.cpp
  GFIROps.cpp
)
add_dependencies(GFIRDialect
  GFIRDialectIncGen
  GFIROpsIncGen
)
# Link direction (B2 step-2 design r9): GFIR depends on the ACIR core and
# the shared interfaces only, never the operations or analysis layers.
target_link_libraries(GFIRDialect PUBLIC
  ACIRCore
  PycircuitStateInterfaces
  MLIRIR
  MLIRSideEffectInterfaces
)
target_include_directories(GFIRDialect PRIVATE
  "${PYCIRCUIT_SOURCE_DIR}/lib"
)
target_include_directories(GFIRDialect PUBLIC
  "$<BUILD_INTERFACE:${PYCIRCUIT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${PYCIRCUIT_BINARY_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
