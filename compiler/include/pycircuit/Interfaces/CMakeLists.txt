set(LLVM_TARGET_DEFINITIONS LogicalStateInterfaces.td)
mlir_tablegen(LogicalStateInterfaces.h.inc -gen-op-interface-decls)
mlir_tablegen(LogicalStateInterfaces.cpp.inc -gen-op-interface-defs)
add_public_tablegen_target(LogicalStateInterfacesIncGen)

set(LLVM_TARGET_DEFINITIONS ModuleChildInterfaces.td)
mlir_tablegen(ModuleChildInterfaces.h.inc -gen-op-interface-decls)
mlir_tablegen(ModuleChildInterfaces.cpp.inc -gen-op-interface-defs)
add_public_tablegen_target(ModuleChildInterfacesIncGen)
