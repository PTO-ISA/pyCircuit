set(LLVM_TARGET_DEFINITIONS ACSimDialect.td)
mlir_tablegen(ACSimDialect.h.inc -gen-dialect-decls -dialect=acsim)
mlir_tablegen(ACSimDialect.cpp.inc -gen-dialect-defs -dialect=acsim)
add_public_tablegen_target(ACSimDialectIncGen)

set(LLVM_TARGET_DEFINITIONS ACSimTypes.td)
mlir_tablegen(ACSimTypes.h.inc -gen-typedef-decls -typedefs-dialect=acsim)
mlir_tablegen(ACSimTypes.cpp.inc -gen-typedef-defs -typedefs-dialect=acsim)
add_public_tablegen_target(ACSimTypesIncGen)

set(LLVM_TARGET_DEFINITIONS ACSimOps.td)
mlir_tablegen(ACSimOps.h.inc -gen-op-decls)
mlir_tablegen(ACSimOps.cpp.inc -gen-op-defs)
add_public_tablegen_target(ACSimOpsIncGen)
