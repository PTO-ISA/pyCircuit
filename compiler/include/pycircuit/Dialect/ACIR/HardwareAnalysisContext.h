#ifndef PYCIRCUIT_DIALECT_ACIR_HARDWAREANALYSISCONTEXT_H
#define PYCIRCUIT_DIALECT_ACIR_HARDWAREANALYSISCONTEXT_H

#include "pycircuit/Dialect/ACIR/ACIRAttributes.h"
#include "mlir/IR/Attributes.h"
#include "mlir/IR/Types.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include <cstdint>

namespace mlir {
class Operation;
}
namespace acir::ac {
/// Bindings describe an occurrence of an existing definition, not cloned IR.
struct HardwareBindings {
  mlir::Operation *owner = nullptr;
  llvm::StringMap<mlir::Attribute> integers;
  llvm::StringMap<mlir::Type> types;
};

struct HardwareSourceCheckLimits {
  uint64_t maxWorkUnits = 4 * 1024 * 1024;
};

/// Validated reset policy on the single root selection; the analysis-class
/// header stays above the operation layer, so the closed policy enum lives
/// here with the limits (B2 step-2 design r9 header split).
enum class HardwareResetSemantics { Strict, IfOne };

/// Validated raw clock/reset rows on a System Module/ModuleImport
/// declaration live with the shared context types: both the operations-layer
/// declaration grammar and the staged analysis consume the same view.
struct EnumMember {
  mlir::StringAttr name;
  MathIntAttr code;
};
/// Validated declaration data shared by analysis and both emitters.
struct EnumDefinitionView {
  uint64_t width;
  mlir::StringAttr encoding;
  llvm::SmallVector<EnumMember> members;
};

/// Source physical prefix and final-only compiler-generated port suffix.
/// (Shared with the packed-width core's leaf records; the layout paths are
/// plain lexical field names, independent of any occurrence binding.)
using FieldPath = llvm::SmallVector<mlir::StringAttr>;
struct PackedLeaf {
  FieldPath path;
  mlir::Type type;
  uint64_t low = 0;
  uint64_t width = 0;
};
} // namespace acir::ac
#endif
