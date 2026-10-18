#ifndef PYCIRCUIT_SOURCE_FORMAL_PROPOSAL_PROOF_H
#define PYCIRCUIT_SOURCE_FORMAL_PROPOSAL_PROOF_H
#include "pycircuit/Dialect/ACIR/SourceTransactionStateAnalysis.h"
#include "SourceTransactionSummary.h"
#include "llvm/ADT/STLExtras.h"
#include <functional>

namespace acir::ac::detail {

// A bounded proof of the private proposal suffix. Drivers reconstruct this
// history from actual statement visits; no metadata flag supplies its meaning.
// OR and selection retain four-state semantics: in particular 0|Z is not Z.
class SourceFormalProposalProof {
public:
  using Intent = uint64_t;
  using Canonical = std::function<mlir::FailureOr<BoundValueId>(BoundValueId, TransactionPoint)>;
  using Lookup = std::function<mlir::FailureOr<BoundValueId>(mlir::Value, LexicalInstanceId, InvocationFrameId)>;
  using Pay = std::function<bool(uint64_t)>;
  SourceFormalProposalProof(const TransactionStatePlan &plan, const HardwareAnalysis &hardware,
      Canonical canonical, Lookup lookup, Pay pay)
      : plan(plan), hardware(hardware), canonical(std::move(canonical)),
        lookup(std::move(lookup)), pay(std::move(pay)) {}

  mlir::FailureOr<BoundValueId> identity(BoundValueId id, TransactionPoint point) {
    return valueIdentity(id, point);
  }
  mlir::FailureOr<Intent> zero() { return append({Kind::Zero}); }
  mlir::FailureOr<Intent> write(Intent previous, BoundValueId path,
                                 mlir::BoolAttr fact, bool forwarded) {
    if (!forwarded && fact && fact.getValue()) return append({Kind::One});
    Node node{Kind::Or};
    node.left = previous;
    node.path = path;
    node.fact = fact;
    return append(node);
  }
  mlir::FailureOr<Intent> join(BranchId branch, Intent whenTrue, Intent whenFalse) {
    if (whenTrue == whenFalse) return whenTrue;
    Node node{Kind::Join};
    node.branch = branch;
    if (branch >= plan.branches.size()) return mlir::failure();
    node.fact = plan.branches[branch].conditionFact;
    node.left = whenTrue;
    node.right = whenFalse;
    return append(node);
  }
  mlir::FailureOr<bool> equal(BoundValueId lhs, BoundValueId rhs,
                                TransactionPoint point, unsigned depth = 0) {
    if (depth > 128 || !pay(4)) return mlir::failure();
    auto left = valueIdentity(lhs, point), right = valueIdentity(rhs, point);
    if (mlir::failed(left) || mlir::failed(right)) return mlir::failure();
    if (*left == *right) return true;
    auto a = branchResult(*left), b = branchResult(*right);
    if (mlir::failed(a) || mlir::failed(b)) return mlir::failure();
    if (!*a || !*b || (*a)->branch != (*b)->branch) return false;
    const auto &branch = plan.branches[(*a)->branch];
    auto t = equal((*a)->whenTrue, (*b)->whenTrue,
                   {branch.thenExitPrefix, {(*a)->branch, true}}, depth + 1);
    auto f = equal((*a)->whenFalse, (*b)->whenFalse,
                   {branch.elseExitPrefix, {(*a)->branch, false}}, depth + 1);
    if (mlir::failed(t) || mlir::failed(f)) return mlir::failure();
    return *t && *f;
  }
  mlir::LogicalResult check(BoundValueId actual, Intent expected,
                              TransactionPoint point, unsigned depth = 0) {
    if (depth > 128 || expected >= nodes.size() || !pay(8)) return mlir::failure();
    auto normalized = valueIdentity(actual, point);
    if (mlir::failed(normalized)) return mlir::failure();
    const auto &ref = plan.values[*normalized];
    auto known = knownIntent(expected, depth);
    if (mlir::failed(known)) return mlir::failure();
    if (*known) {
      auto actualKnown = knownValue(*normalized, point, depth);
      if (mlir::failed(actualKnown)) return mlir::failure();
      if (*actualKnown && **known == **actualKnown) return mlir::success();
    }
    const auto &node = nodes[expected];
    if (node.kind == Kind::Zero || node.kind == Kind::One) return mlir::failure();
    if (node.kind == Kind::Or) {
      auto operation = ref.value.getDefiningOp<BitsBinaryOp>();
      if (!operation || operation.getOpcode() != "or") return mlir::failure();
      auto lhs = lookup(operation.getLhs(), ref.anchor, ref.frame);
      auto rhs = lookup(operation.getRhs(), ref.anchor, ref.frame);
      if (mlir::failed(lhs) || mlir::failed(rhs) || mlir::failed(check(*lhs, node.left, point, depth + 1)))
        return mlir::failure();
      auto same = equal(*rhs, node.path, point, depth + 1);
      return mlir::success(mlir::succeeded(same) && *same);
    }
    auto result = branchResult(*normalized);
    if (mlir::failed(result) || !*result || (*result)->branch != node.branch) return mlir::failure();
    const auto &branch = plan.branches[node.branch];
    if (mlir::failed(check((*result)->whenTrue, node.left,
            {branch.thenExitPrefix, {node.branch, true}}, depth + 1))) return mlir::failure();
    return check((*result)->whenFalse, node.right,
                 {branch.elseExitPrefix, {node.branch, false}}, depth + 1);
  }
private:
  enum class Kind { Zero, One, Or, Join };
  struct Node {
    Kind kind;
    Intent left = 0, right = 0;
    BoundValueId path = 0;
    BranchId branch = 0;
    mlir::BoolAttr fact;
    std::optional<bool> known;
  };
  mlir::FailureOr<Intent> append(Node node) {
    if (!pay(1 + (sizeof(Node) + 7) / 8)) return mlir::failure();
    if (nodes.size() == nodes.capacity()) {
      auto capacity = nodes.capacity() * 2 + 1;
      if (capacity < nodes.size() || capacity > nodes.max_size() ||
          !pay(nodes.size() + ((capacity - nodes.capacity() - 1) * sizeof(Node) + 7) / 8))
        return mlir::failure();
      nodes.reserve(capacity);
    }
    if (node.kind == Kind::Zero) node.known = false;
    else if (node.kind == Kind::One) node.known = true;
    else {
      if (node.left >= nodes.size()) return mlir::failure();
      auto lhs = nodes[node.left].known;
      if (node.kind == Kind::Or) {
        if ((lhs && *lhs) || (node.fact && node.fact.getValue())) node.known = true;
        else if (lhs && !*lhs && node.fact && !node.fact.getValue()) node.known = false;
      } else {
        if (node.right >= nodes.size()) return mlir::failure();
        auto rhs = nodes[node.right].known;
        if (node.fact) node.known = node.fact.getValue() ? lhs : rhs;
        else if (lhs && rhs && *lhs == *rhs) node.known = lhs;
      }
    }
    Intent id = nodes.size();
    nodes.push_back(node);
    return id;
  }
  mlir::FailureOr<std::optional<bool>> knownIntent(Intent id, unsigned depth) {
    if (depth > 128 || id >= nodes.size() || !pay(2)) return mlir::failure();
    return nodes[id].known;
  }
  mlir::FailureOr<std::optional<bool>> knownValue(BoundValueId id,
      TransactionPoint point, unsigned depth) {
    if (depth > 128 || !pay(4)) return mlir::failure();
    auto normalized = valueIdentity(id, point);
    if (mlir::failed(normalized)) return mlir::failure();
    const auto &ref = plan.values[*normalized];
    auto literal = getSourceLiteralPredicate(ref.value, hardware);
    if (mlir::failed(literal)) return mlir::failure();
    if (*literal) return std::optional<bool>((*literal).getValue());
    if (auto binary = ref.value.getDefiningOp<BitsBinaryOp>();
        binary && binary.getOpcode() == "or") {
      auto lhs = lookup(binary.getLhs(), ref.anchor, ref.frame);
      auto rhs = lookup(binary.getRhs(), ref.anchor, ref.frame);
      if (mlir::failed(lhs) || mlir::failed(rhs)) return mlir::failure();
      auto a = knownValue(*lhs, point, depth + 1), b = knownValue(*rhs, point, depth + 1);
      if (mlir::failed(a) || mlir::failed(b)) return mlir::failure();
      if ((*a && **a) || (*b && **b)) return std::optional<bool>(true);
      if (*a && *b) return std::optional<bool>(false);
      return std::optional<bool>();
    }
    auto branch = branchResult(*normalized);
    if (mlir::failed(branch)) return mlir::failure();
    if (*branch) {
      const auto &owner = plan.branches[(*branch)->branch];
      auto t = knownValue((*branch)->whenTrue,
          {owner.thenExitPrefix, {(*branch)->branch, true}}, depth + 1);
      auto f = knownValue((*branch)->whenFalse,
          {owner.elseExitPrefix, {(*branch)->branch, false}}, depth + 1);
      if (mlir::failed(t) || mlir::failed(f)) return mlir::failure();
      if (*t && *f && **t == **f) return *t;
    }
    return std::optional<bool>();
  }
  mlir::FailureOr<const TransactionBranchResult *> branchResult(BoundValueId id) {
    for (const auto &result : plan.branchResults) {
      if (!pay(1)) return mlir::failure();
      if (result.result == id) return &result;
    }
    return static_cast<const TransactionBranchResult *>(nullptr);
  }
  mlir::FailureOr<BoundValueId> valueIdentity(BoundValueId id, TransactionPoint point) {
    for (unsigned depth = 0; depth <= 258; ++depth) {
      if (!pay(8)) return mlir::failure();
      auto normalized = canonical(id, point);
      if (mlir::failed(normalized)) return mlir::failure();
      id = *normalized;
      const auto &ref = plan.values[id];
      auto extract = ref.value.getDefiningOp<BitsExtractOp>();
      if (!extract) return id;
      auto inputType = mlir::dyn_cast<BitsType>(extract.getInput().getType());
      auto resultType = mlir::dyn_cast<BitsType>(ref.value.getType());
      auto low = extract->getAttrOfType<StaticExprAttr>("low");
      const auto &bindings = plan.instances[ref.anchor].bindings;
      if (!inputType || !resultType || !low ||
          !hardware.isStaticEvaluable(inputType.getWidth(), bindings) ||
          !hardware.isStaticEvaluable(resultType.getWidth(), bindings) ||
          !hardware.isStaticEvaluable(low, bindings)) return id;
      auto a = hardware.getPackedWidth(inputType, bindings, extract);
      auto b = hardware.getPackedWidth(resultType, bindings, extract);
      auto offset = hardware.evaluateStatic(low, bindings, extract);
      if (mlir::failed(a) || mlir::failed(b) || mlir::failed(offset)) return mlir::failure();
      auto integer = mlir::dyn_cast<MathIntAttr>(*offset);
      if (*a != *b || !integer || integer.getCanonicalValue() != "0") return id;
      auto input = lookup(extract.getInput(), ref.anchor, ref.frame);
      if (mlir::failed(input)) return mlir::failure();
      id = *input;
    }
    return mlir::failure();
  }
  const TransactionStatePlan &plan;
  const HardwareAnalysis &hardware;
  Canonical canonical;
  Lookup lookup;
  Pay pay;
  llvm::SmallVector<Node, 0> nodes;
};
} // namespace acir::ac::detail
#endif
