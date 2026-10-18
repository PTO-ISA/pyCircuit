#ifndef PYCIRCUIT_HARDWARE_GRANT_PROOF_H
#define PYCIRCUIT_HARDWARE_GRANT_PROOF_H

#include "pycircuit/Dialect/ACIR/ACIRAttributes.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/LogicalResult.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

namespace acir::ac::detail {
/// Reuses the existing four-state owner-enable implication algebra. Node IDs
/// identify actual bound endpoints, including invocation/instance context. The
/// adapter, graph and bindings must remain immutable for this proof's lifetime.
/// A caller must not use a raw SSA Value as identity across distinct scopes.
/// Canonicalization must be stable and idempotent. Inspect must return verified
/// types/patterns and charge traversal and data-sized work through the same
/// caller accounting. The two largest uint64_t IDs are reserved by DenseMap.
class HardwareGrantProof {
public:
  using Node = uint64_t;
  struct Constant {
    MathIntAttr value;
    uint64_t width;
  };
  struct NodeView {
    enum Kind { Raw, Not, And, Or, Select, Eq, Ne } kind = Raw;
    llvm::SmallVector<Node, 3> operands;
    std::optional<uint64_t> width;
    std::optional<Constant> fullyKnown;
  };
  using Canonicalize = std::function<mlir::FailureOr<Node>(Node, unsigned depth)>;
  using Inspect = std::function<mlir::FailureOr<NodeView>(Node)>;
  using Charge = std::function<bool(uint64_t)>;

  HardwareGrantProof(Canonicalize canonicalize, Inspect inspect, Charge charge);
  ~HardwareGrantProof();
  HardwareGrantProof(const HardwareGrantProof &) = delete;
  HardwareGrantProof &operator=(const HardwareGrantProof &) = delete;
  mlir::FailureOr<bool> disjoint(Node first, Node second);
  mlir::FailureOr<bool> disjointAtAddress(Node first, Node second,
                                         Node firstIndex, Node secondIndex);
  llvm::StringRef failureReason() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl;
};
} // namespace acir::ac::detail
#endif
