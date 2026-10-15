#pragma once
#include "mlir/IR/BuiltinOps.h"
#include <filesystem>
namespace acir {
void emitCpp(mlir::ModuleOp module, const std::filesystem::path &directory);
}
