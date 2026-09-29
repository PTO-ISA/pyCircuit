#include "ACIRNumericCompositionDetail.h"

#include "ACIRSourceContracts.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir::ac::composition_detail {
namespace {

ValueBindingOp binding(Block &body, DictionaryAttr id) {
  ValueBindingOp found;
  for (ValueBindingOp candidate : body.getOps<ValueBindingOp>())
    if (candidate.getIdAttr() == id) {
      if (found)
        return {};
      found = candidate;
    }
  return found;
}

bool boolConstant(Value value, bool expected) {
  auto op = value.getDefiningOp<arith::ConstantOp>();
  auto attr = op ? dyn_cast<IntegerAttr>(op.getValue()) : IntegerAttr();
  return attr && attr.getType().isInteger(1) &&
         attr.getValue() == APInt(1, expected);
}

bool orTree(Value value, DenseSet<Value> &leaves, unsigned &falseLeaves) {
  if (leaves.erase(value))
    return true;
  if (boolConstant(value, false)) {
    ++falseLeaves;
    return true;
  }
  auto operation = value.getDefiningOp<arith::OrIOp>();
  return operation && operation->getAttrs().empty() &&
         orTree(operation.getLhs(), leaves, falseLeaves) &&
         orTree(operation.getRhs(), leaves, falseLeaves);
}

bool selectTree(Value value, ArrayRef<std::pair<Value, Value>> candidates,
                DenseSet<unsigned> &seen) {
  auto select = value.getDefiningOp<arith::SelectOp>();
  if (!select) {
    auto zero = value.getDefiningOp<arith::ConstantOp>();
    auto attr = zero ? dyn_cast<IntegerAttr>(zero.getValue()) : IntegerAttr();
    return attr && attr.getValue().isZero();
  }
  if (!select->getAttrs().empty())
    return false;
  for (auto [index, candidate] : llvm::enumerate(candidates))
    if (!seen.contains(index) && select.getCondition() == candidate.first &&
        select.getTrueValue() == candidate.second) {
      seen.insert(index);
      return selectTree(select.getFalseValue(), candidates, seen);
    }
  return false;
}

void pathAtoms(Value value, DenseMap<Value, bool> &atoms,
               bool polarity = true) {
  atoms.try_emplace(value, polarity);
  if (auto conjunction = value.getDefiningOp<arith::AndIOp>();
      polarity && conjunction) {
    pathAtoms(conjunction.getLhs(), atoms, true);
    pathAtoms(conjunction.getRhs(), atoms, true);
    return;
  }
  if (auto inverted = value.getDefiningOp<arith::XOrIOp>()) {
    bool lhs = boolConstant(inverted.getLhs(), true);
    bool rhs = boolConstant(inverted.getRhs(), true);
    if (lhs != rhs)
      pathAtoms(lhs ? inverted.getRhs() : inverted.getLhs(), atoms, !polarity);
  }
}

bool exclusive(Value left, Value right) {
  DenseMap<Value, bool> lhs, rhs;
  pathAtoms(left, lhs);
  pathAtoms(right, rhs);
  return llvm::any_of(lhs, [&](const auto &item) {
    auto found = rhs.find(item.first);
    return found != rhs.end() && found->second != item.second;
  });
}

} // namespace

LogicalResult verifyUses(RuleOp rule, Block &body) {
  auto emit = [&] { return rule.emitOpError(); };
  auto required = rule->getAttrOfType<ArrayAttr>("ac.required_uses");
  auto rows = rule->getAttrOfType<ArrayAttr>("ac.yield_bindings");
  auto outputs = rule->getAttrOfType<ArrayAttr>("ac.output_bindings");
  auto outputTypes = rule->getAttrOfType<ArrayAttr>("ac.output_types");
  auto numeric = rule->getAttrOfType<ArrayAttr>("ac.required_numeric");
  SmallVector<ValueUseOp> uses(body.getOps<ValueUseOp>());
  auto yield = dyn_cast_or_null<YieldOp>(body.getTerminator());
  if (!required || !rows || !outputs || !outputTypes || !numeric ||
      outputTypes.size() != outputs.size() || rows.size() != outputs.size() ||
      !yield || yield.getValues().size() != 2 * rows.size() ||
      uses.size() != required.size())
    return emit() << "composition final use/yield inventory is incomplete";
  DenseMap<Attribute, ValueUseOp> actual;
  for (ValueUseOp use : uses)
    if (!actual.try_emplace(use.getIdAttr(), use).second)
      return emit() << "composition repeats one ValueUse identity";
  DenseMap<Attribute, DictionaryAttr> requiredByID;
  for (Attribute raw : required) {
    auto item = dyn_cast<DictionaryAttr>(raw);
    auto id = item ? item.getAs<DictionaryAttr>("id") : DictionaryAttr();
    auto source = item ? item.getAs<DictionaryAttr>("value") : DictionaryAttr();
    auto target =
        item ? item.getAs<DictionaryAttr>("target") : DictionaryAttr();
    if (!item || item.size() != 3 || failed(detail::verifyUseID(id, emit)) ||
        failed(detail::verifyValueID(source, emit)) || !target ||
        target.getAs<StringAttr>("kind") !=
            StringAttr::get(rule.getContext(), "next_scalar") ||
        failed(detail::verifyStateRef(target.getAs<DictionaryAttr>("state"),
                                      emit)) ||
        !requiredByID.try_emplace(id, item).second)
      return emit() << "composition RequiredUse schema is invalid";
  }
  DenseSet<Attribute> expectedBindings;
  Builder attrs(rule.getContext());
  for (Attribute raw : numeric) {
    auto node = cast<DictionaryAttr>(raw);
    auto id = node.getAs<DictionaryAttr>("id");
    expectedBindings.insert(id);
    if (node.getAs<StringAttr>("operator").getValue() == "from_bits")
      expectedBindings.insert(attrs.getDictionaryAttr({
          attrs.getNamedAttr("origin", id.get("origin")),
          attrs.getNamedAttr("slot", attrs.getI32IntegerAttr(0)),
      }));
  }
  DenseSet<Attribute> consumed;
  for (auto [targetIndex, raw] : llvm::enumerate(rows)) {
    auto row = dyn_cast<DictionaryAttr>(raw);
    auto contributions =
        row ? row.getAs<ArrayAttr>("contributions") : ArrayAttr();
    auto data = row ? row.getAs<IntegerAttr>("data_operand") : IntegerAttr();
    auto enable =
        row ? row.getAs<IntegerAttr>("enable_operand") : IntegerAttr();
    if (!row || row.size() != 4 || !contributions || contributions.empty() ||
        !data || data.getValue() != APInt(32, 2 * targetIndex) || !enable ||
        enable.getValue() != APInt(32, 2 * targetIndex + 1) ||
        row.getAs<DictionaryAttr>("target") !=
            dyn_cast<DictionaryAttr>(outputs[targetIndex]))
      return emit() << "composition YieldBinding row is malformed";
    SmallVector<std::pair<Value, Value>> candidates;
    SmallVector<Value> paths;
    DenseSet<Value> enables;
    for (Attribute rawContribution : contributions) {
      auto contribution = dyn_cast<DictionaryAttr>(rawContribution);
      auto id = contribution ? contribution.getAs<DictionaryAttr>("use")
                             : DictionaryAttr();
      auto use = actual.find(id);
      auto requirement = requiredByID.find(id);
      auto source = requirement != requiredByID.end()
                        ? requirement->second.getAs<DictionaryAttr>("value")
                        : DictionaryAttr();
      auto sourceBinding = source ? binding(body, source) : ValueBindingOp();
      if (!contribution || contribution.size() != 2 ||
          !contribution.getAs<UnitAttr>("selection_ordinal") ||
          use == actual.end() || requirement == requiredByID.end() ||
          !sourceBinding || use->second.getSourceAttr() != source ||
          use->second.getValue() != sourceBinding.getValue() ||
          use->second.getValid() != sourceBinding.getValid() ||
          requirement->second.getAs<DictionaryAttr>("target")
                  .getAs<DictionaryAttr>("state") != outputs[targetIndex] ||
          !consumed.insert(id).second ||
          !safeControl(use->second.getValid(), rule) ||
          !safeControl(use->second.getPath(), rule))
        return emit() << "composition contribution lacks exact ValueUse";
      auto logical = dyn_cast<DictionaryAttr>(outputTypes[targetIndex]);
      bool boundary = llvm::any_of(numeric, [&](Attribute rawNode) {
        auto node = cast<DictionaryAttr>(rawNode);
        return node.getAs<DictionaryAttr>("id") == source &&
               node.getAs<StringAttr>("operator").getValue() == "to_bits";
      });
      if (boundary) {
        if (!rhsIdentityMatchesUse(id, source) ||
            sourceBinding.getDomainAttr() != logical)
          return emit() << "composition numeric use bypasses its boundary";
      } else {
        if (!directUseAuthority(sourceBinding.getValue(), logical, id, source,
                                rule) ||
            sourceBinding.getDomainAttr() != logical)
          return emit()
                 << "composition direct use has no literal/read authority";
        expectedBindings.insert(source);
      }
      auto active = dyn_cast_or_null<arith::AndIOp>(use->second->getNextNode());
      bool matches = active && ((active.getLhs() == use->second.getValid() &&
                                 active.getRhs() == use->second.getPath()) ||
                                (active.getRhs() == use->second.getValid() &&
                                 active.getLhs() == use->second.getPath()));
      if (!matches || !active->getAttrs().empty())
        return emit() << "composition ValueUse has no exact enable gate";
      candidates.push_back({active.getResult(), use->second.getValue()});
      paths.push_back(use->second.getPath());
      enables.insert(active.getResult());
    }
    for (size_t left = 0; left < paths.size(); ++left)
      for (size_t right = left + 1; right < paths.size(); ++right)
        if (!exclusive(paths[left], paths[right]))
          return emit() << "composition final contributions may overlap";
    DenseSet<Value> remaining(enables);
    DenseSet<unsigned> selected;
    unsigned falseLeaves = 0;
    if (!orTree(yield.getValues()[2 * targetIndex + 1], remaining,
                falseLeaves) ||
        !remaining.empty() || falseLeaves != 1 ||
        !selectTree(yield.getValues()[2 * targetIndex], candidates, selected) ||
        selected.size() != candidates.size())
      return emit() << "composition final yield is not exact select/OR merge";
  }
  if (consumed.size() != required.size())
    return emit() << "composition has an unconsumed RequiredUse";
  SmallVector<ValueBindingOp> bindings(body.getOps<ValueBindingOp>());
  if (bindings.size() != expectedBindings.size() ||
      llvm::any_of(bindings, [&](ValueBindingOp value) {
        return !expectedBindings.contains(value.getIdAttr());
      }))
    return emit() << "composition binding inventory has orphan entries";
  return success();
}

LogicalResult verifyFiniteInventory(RuleOp rule, Block &body) {
  auto emit = [&] { return rule.emitOpError(); };
  for (Operation &operation : body) {
    if (!isa<SourceReadOp, ValueBindingOp, NumericProofOp, ValueUseOp,
             SourceExpectOp, SourceObserveOp, YieldOp, arith::ConstantOp,
             arith::AndIOp, arith::OrIOp, arith::XOrIOp, arith::ExtUIOp,
             arith::ExtSIOp, arith::TruncIOp, arith::AddIOp, arith::CmpIOp,
             arith::SelectOp>(operation))
      return emit() << "lowered composition contains an unsupported operation";
    if (auto compare = dyn_cast<arith::CmpIOp>(operation);
        compare &&
        (operation.getAttrs().size() != 1 || !operation.hasAttr("predicate") ||
         !safeControl(compare.getResult(), rule)))
      return emit() << "lowered composition comparison is not closed/owned";
    if (auto add = dyn_cast<arith::AddIOp>(operation);
        add && (add.getOverflowFlags() != arith::IntegerOverflowFlags::none ||
                (!operation.getAttrs().empty() &&
                 (operation.getAttrs().size() != 1 ||
                  !operation.hasAttr("overflowFlags")))))
      return emit() << "lowered composition addition flags/attrs are invalid";
    if (isa<arith::AndIOp, arith::OrIOp, arith::XOrIOp, arith::SelectOp,
            arith::ExtUIOp, arith::ExtSIOp>(operation) &&
        !operation.getAttrs().empty())
      return emit() << "lowered composition finite operation "
                    << operation.getName() << " has unknown attrs";
    if (auto trunc = dyn_cast<arith::TruncIOp>(operation);
        trunc &&
        (trunc.getOverflowFlags() != arith::IntegerOverflowFlags::none ||
         operation.getAttrs().size() > 1 ||
         (operation.getAttrs().size() == 1 &&
          !operation.hasAttr("overflowFlags"))))
      return emit() << "lowered composition truncation flags/attrs are invalid";
    if (auto constant = dyn_cast<arith::ConstantOp>(operation)) {
      bool plain =
          constant->getAttrs().size() == 1 && constant->hasAttr("value");
      bool observed =
          constant->getAttrs().size() == 2 && constant->hasAttr("value") &&
          constant->hasAttr("ac.origin") &&
          llvm::any_of(constant.getResult().getUsers(), [](Operation *user) {
            return isa<SourceObserveOp>(user);
          });
      if (!plain && !observed)
        return emit()
               << "lowered composition constant attributes are not closed";
    }
    if (operation.getNumResults() == 1 && operation.getResult(0).use_empty())
      return emit() << "lowered composition contains an orphan finite result";
  }
  return success();
}

} // namespace acir::ac::composition_detail
