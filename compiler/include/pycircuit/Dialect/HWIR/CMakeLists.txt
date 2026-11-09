set(LLVM_TARGET_DEFINITIONS HWIRDialect.td)
mlir_tablegen(HWIRDialect.h.inc -gen-dialect-decls -dialect=hwir)
mlir_tablegen(HWIRDialect.cpp.inc -gen-dialect-defs -dialect=hwir)
add_public_tablegen_target(HWIRDialectIncGen)

set(LLVM_TARGET_DEFINITIONS HWIROps.td)
mlir_tablegen(HWIROps.h.inc -gen-op-decls)
mlir_tablegen(HWIROps.cpp.inc -gen-op-defs)
add_public_tablegen_target(HWIROpsIncGen)
