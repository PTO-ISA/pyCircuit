// Read-only CDC implementation equations shared by graph refinement and
// standalone final verification. This private Analysis interface neither
// constructs IR nor depends on the conversion libraries.
#ifndef PYCIRCUIT_HARDWARE_CDC_GRAPH_H
#define PYCIRCUIT_HARDWARE_CDC_GRAPH_H

#include "mlir/IR/IRMapping.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/OperationSupport.h"
#include "mlir/Support/LogicalResult.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringRef.h"
#include <cstdint>
#include <optional>
#include <string>

namespace acir::ac::detail {
class SourceCheckBudget;
}

namespace acir::ac::detail::cdc {
inline constexpr llvm::StringLiteral registerSymbol =
    "pycircuit.__builtins__.__cdc_dffe";
struct Ref {
  mlir::Value external;
  unsigned node = 0, result = 0;
  Ref() = default;
  explicit Ref(mlir::Value value) : external(value) {}
  Ref(unsigned node, unsigned result = 0) : node(node), result(result) {}
};
struct Node {
  std::string operation;
  llvm::SmallVector<mlir::Type> types;
  mlir::NamedAttrList attributes;
  llvm::SmallVector<Ref> inputs;
};
struct Circuit {
  mlir::Operation *source;
  std::string kind;
  std::optional<uint64_t> depth;
  llvm::SmallVector<Node, 0> nodes;
  llvm::SmallVector<Ref> outputs;
};

/// Exact component equations and ownership without endpoint admission parsing.
/// Queue route closure uses this to avoid recursive intent authentication.
mlir::FailureOr<llvm::DenseMap<mlir::Operation *, mlir::Operation *>>
getCDCComponentRoutingOwners(mlir::ModuleOp package);

mlir::FailureOr<Circuit> getCircuit(mlir::Operation *source,
                                   SourceCheckBudget &budget);
mlir::Value resolve(Ref ref,
                    const llvm::SmallVector<mlir::Operation *, 0> &nodes,
                    const mlir::IRMapping &mapping);
} // namespace acir::ac::detail::cdc
#endif
