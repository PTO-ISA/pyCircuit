#pragma once
#include "mlir/IR/BuiltinOps.h"
#include <optional>
namespace acir {
// Static record/tuple coordinates only. Resource reads and dynamic array
// selection remain opaque, preserving their checks and snapshot identity.
std::optional<unsigned> fieldIndex(mlir::Type type, mlir::Attribute field,
                                   mlir::ModuleOp module);
bool containsAssertion(mlir::Value value, bool explicitOnly = false);
} // namespace acir
