#ifndef ACIR_LIB_COMPILER_LITERALPAYLOADWIRES_H
#define ACIR_LIB_COMPILER_LITERALPAYLOADWIRES_H

#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/STLFunctionalExtras.h"
#include <functional>
#include <map>
#include <optional>

namespace acir::compiler::detail {

// Match only a representation identity feeding aggregate payload construction.
// Callers choose the admitted owner block and preserve its metadata/fact tables.
inline mlir::Value literalPayloadReplacement(ac::BitsExtractOp extract,
                                             mlir::Block &block) {
  using namespace mlir;
  auto input = extract.getInput();
  auto constant = input.getDefiningOp<ac::BitsConstantOp>();
  if (!constant || constant->getBlock() != &block ||
      input.getType() != extract.getResult().getType() ||
      !extract->getDiscardableAttrDictionary().empty() ||
      extract.getResult().use_empty())
    return {};
  auto low = extract.getLow().getTree();
  auto kind = low.getAs<StringAttr>("kind");
  auto value = low.getAs<DictionaryAttr>("value");
  auto valueKind = value ? value.getAs<StringAttr>("kind") : StringAttr();
  auto integer = value ? value.getAs<ac::MathIntAttr>("value")
                       : ac::MathIntAttr();
  if (!kind || kind.getValue() != "literal" || !valueKind ||
      valueKind.getValue() != "integer" || !integer ||
      !llvm::APSInt(integer.getCanonicalValue()).isZero())
    return {};
  // Constructor identities remain intact. Do not change physical domain
  // wires, initializers, captures, checks, observations or output routes.
  if (!llvm::all_of(extract.getResult().getUsers(), [&](Operation *user) {
        return user->getBlock() == &block &&
               isa<ac::StructCreateOp, ac::TableCreateOp>(user);
      }))
    return {};
  return input;
}

using LiteralPayloadKey = std::pair<mlir::Type, ac::MathIntAttr>;
struct LiteralPayloadKeyLess {
  bool operator()(const LiteralPayloadKey &a, const LiteralPayloadKey &b) const {
    std::less<const void *> less;
    return less(a.first.getAsOpaquePointer(), b.first.getAsOpaquePointer()) ||
           (a.first == b.first &&
            less(a.second.getAsOpaquePointer(), b.second.getAsOpaquePointer()));
  }
};

inline std::optional<LiteralPayloadKey>
literalPayloadConstantKey(ac::BitsConstantOp constant, mlir::Block &block) {
  using namespace mlir;
  if (constant->getBlock() != &block ||
      !constant->getDiscardableAttrDictionary().empty() ||
      constant.getResult().use_empty() ||
      !llvm::all_of(constant.getResult().getUsers(), [&](Operation *user) {
        return user->getBlock() == &block &&
               isa<ac::StructCreateOp, ac::TableCreateOp>(user);
      }))
    return std::nullopt;
  auto tree = constant.getValue().getTree();
  auto kind = tree.getAs<StringAttr>("kind");
  auto value = tree.getAs<DictionaryAttr>("value");
  auto valueKind = value ? value.getAs<StringAttr>("kind") : StringAttr();
  auto integer = value ? value.getAs<ac::MathIntAttr>("value") : ac::MathIntAttr();
  if (!kind || kind.getValue() != "literal" || !valueKind ||
      valueKind.getValue() != "integer" || !integer)
    return std::nullopt;
  return LiteralPayloadKey{constant.getResult().getType(), integer};
}

using LiteralPayloadPlan =
    llvm::SmallVector<std::pair<ac::BitsConstantOp, mlir::Value>, 0>;

// Build the whole plan before editing SSA or importer facts. Imported source
// snapshots are never passed here. The first eligible lexical operation remains
// the representative, including its complete value tree, location and facts.
inline mlir::FailureOr<LiteralPayloadPlan> planLiteralPayloadConstants(
    mlir::Block &block,
    llvm::function_ref<mlir::LogicalResult(uint64_t, uint64_t)> pay) {
  using namespace mlir;
  std::map<LiteralPayloadKey, Value, LiteralPayloadKeyLess> literals;
  LiteralPayloadPlan plan;
  auto lookupWork = [](uint64_t size) {
    uint64_t work = 1;
    for (uint64_t n = size + 1; n > 1; n = n / 2 + n % 2) work += 2;
    return work;
  };
  for (Operation &operation : block) {
    if (failed(pay(1, 0))) return failure();
    auto constant = dyn_cast<ac::BitsConstantOp>(operation);
    if (!constant) continue;
    if (failed(pay(8, 0))) return failure();
    for (OpOperand &use : constant.getResult().getUses()) {
      (void)use;
      if (failed(pay(3, 0))) return failure();
    }
    auto key = literalPayloadConstantKey(constant, block);
    if (!key) continue;
    uint64_t lookup = lookupWork(literals.size());
    if (failed(pay(lookup, 0))) return failure();
    auto found = literals.find(*key);
    if (found == literals.end()) {
      if (failed(pay(lookup + 1, sizeof(decltype(literals)::value_type) +
                                    4 * sizeof(void *)))) return failure();
      literals.emplace(*key, constant.getResult());
      continue;
    }
    if (plan.size() == plan.capacity()) {
      size_t capacity = plan.capacity() <= (plan.max_size() - 1) / 2
                            ? 2 * plan.capacity() + 1
                            : plan.max_size();
      if (capacity <= plan.capacity()) return failure();
      if (failed(pay(plan.size() + 1,
                     (capacity - plan.capacity()) * sizeof(LiteralPayloadPlan::value_type))))
        return failure();
      plan.reserve(capacity);
    }
    if (failed(pay(4, 0))) return failure();
    plan.emplace_back(constant, found->second);
  }
  return std::move(plan);
}

} // namespace acir::compiler::detail

#endif
