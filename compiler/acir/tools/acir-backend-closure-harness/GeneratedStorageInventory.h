#ifndef ACIR_GENERATED_STORAGE_INVENTORY_H
#define ACIR_GENERATED_STORAGE_INVENTORY_H
#include "Compiler/FinalProgram.h"
mlir::FailureOr<uint64_t>
generatedStorageInventory(llvm::StringRef text, bool cpp,
                          acir::ac::detail::EmitError error);
#endif
