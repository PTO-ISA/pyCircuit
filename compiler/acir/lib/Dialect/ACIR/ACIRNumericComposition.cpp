#include "ACIRNumericComposition.h"

#include "ACIRNumericCompositionDetail.h"
#include "ACIRNumericNextUse.h"
#include "ACIRSourceContracts.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSwitch.h"

using namespace mlir;

namespace acir::ac {
namespace composition_detail {
namespace {

bool boolInput(Value value, RuleOp rule) {
  auto read = value.getDefiningOp<SourceReadOp>();
  auto argument = read ? dyn_cast<BlockArgument>(read.getCurrent())
                       : dyn_cast<BlockArgument>(value);
  auto types = rule->getAttrOfType<ArrayAttr>("ac.input_types");
  auto logical = argument && argument.getOwner() == &rule.getBody().front() &&
                         types && argument.getArgNumber() < types.size()
                     ? dyn_cast<DictionaryAttr>(types[argument.getArgNumber()])
                     : DictionaryAttr();
  return logical &&
         logical.getAs<StringAttr>("kind") ==
             StringAttr::get(rule.getContext(), "bool") &&
         value.getType().isInteger(1);
}

bool reaches(Value root, Value wanted, DenseSet<Value> &seen) {
  if (root == wanted)
    return true;
  Operation *definition = root ? root.getDefiningOp() : nullptr;
  if (!definition || !seen.insert(root).second)
    return false;
  return llvm::any_of(definition->getOperands(), [&](Value operand) {
    return reaches(operand, wanted, seen);
  });
}

bool ownedCompare(Value value, RuleOp rule) {
  if (!value.getDefiningOp<arith::CmpIOp>())
    return false;
  for (ValueBindingOp binding : rule.getBody().front().getOps<ValueBindingOp>())
    if (binding.getValue() == value)
      return true;
  for (SourceExpectOp expect :
       rule.getBody().front().getOps<SourceExpectOp>()) {
    if (expect.getKind() != "range")
      continue;
    DenseSet<Value> seen;
    if (reaches(expect.getCondition(), value, seen))
      return true;
  }
  return false;
}

enum class Visit : uint8_t { Active, Valid, Invalid };

bool safeControlImpl(Value value, RuleOp rule, DenseMap<Value, Visit> &memo) {
  if (!value || !value.getType().isInteger(1))
    return false;
  auto known = memo.find(value);
  if (known != memo.end())
    return known->second == Visit::Valid;
  memo.try_emplace(value, Visit::Active);
  bool valid = false;
  if (auto constant = value.getDefiningOp<arith::ConstantOp>())
    valid = isa<IntegerAttr>(constant.getValue()) &&
            constant->getAttrs().size() == 1 && constant->hasAttr("value");
  else if (boolInput(value, rule))
    valid = true;
  else if (auto compare = value.getDefiningOp<MathCompareOp>())
    valid = value == compare.getResult() || value == compare.getValid();
  else if (auto binary = value.getDefiningOp<MathBinaryOp>())
    valid = value == binary.getValid();
  else if (auto boundary = value.getDefiningOp<MathToBitsOp>())
    valid = value == boundary.getValid();
  else if (ownedCompare(value, rule))
    valid = true;
  else if (Operation *operation = value.getDefiningOp())
    valid = operation->getBlock() == &rule.getBody().front() &&
            isa<arith::AndIOp, arith::OrIOp, arith::XOrIOp>(operation) &&
            operation->getAttrs().empty() &&
            llvm::all_of(operation->getOperands(), [&](Value operand) {
              return safeControlImpl(operand, rule, memo);
            });
  memo[value] = valid ? Visit::Valid : Visit::Invalid;
  return valid;
}

StringRef opcode(Operation *operation) {
  if (auto binary = dyn_cast<MathBinaryOp>(operation)) {
    auto value = binary->getAttrOfType<StringAttr>("operator");
    return value ? value.getValue() : StringRef();
  }
  if (auto compare = dyn_cast<MathCompareOp>(operation))
    return compare.getPredicate();
  if (isa<MathFromBitsOp>(operation))
    return "from_bits";
  if (isa<MathConstantOp>(operation))
    return "constant";
  if (isa<MathToBitsOp>(operation))
    return "to_bits";
  return {};
}

unsigned slotFor(Operation *operation) {
  return isa<MathFromBitsOp, MathToBitsOp>(operation) ? 1 : 0;
}

bool ref(Attribute raw, StringRef kind, unsigned index) {
  auto value = dyn_cast<DictionaryAttr>(raw);
  auto ordinal = value ? value.getAs<IntegerAttr>("index") : IntegerAttr();
  return value && value.size() == 2 && ordinal &&
         value.getAs<StringAttr>("kind") ==
             StringAttr::get(value.getContext(), kind) &&
         ordinal.getType().isInteger(32) &&
         ordinal.getValue() == APInt(32, index);
}

FailureOr<Operation *> findProducer(Block &body, DictionaryAttr node,
                                    DenseSet<Operation *> &used,
                                    Operation *owner) {
  auto emit = [&] { return owner->emitOpError(); };
  auto id = node.getAs<DictionaryAttr>("id");
  auto expectedOpcode = node.getAs<StringAttr>("operator");
  auto origin = id ? id.getAs<DictionaryAttr>("origin") : DictionaryAttr();
  auto slot = id ? detail::decodeU32(id.getAs<IntegerAttr>("slot"),
                                     "composition ValueID slot", emit)
                 : FailureOr<uint32_t>(failure());
  Operation *found = nullptr;
  for (Operation &candidate : body) {
    if (opcode(&candidate) != expectedOpcode.getValue() ||
        candidate.getAttrOfType<DictionaryAttr>("ac.origin") != origin ||
        used.contains(&candidate))
      continue;
    if (found)
      return emit() << "composition node has ambiguous source producers";
    found = &candidate;
  }
  if (!found || failed(slot) || *slot != slotFor(found))
    return emit() << "composition node ID does not match its source producer";
  used.insert(found);
  return found;
}

bool sourceOrTree(Value value, DenseSet<Value> &leaves, unsigned &falseLeaves) {
  if (leaves.erase(value))
    return true;
  if (auto constant = value.getDefiningOp<arith::ConstantOp>()) {
    auto attr = dyn_cast<IntegerAttr>(constant.getValue());
    if (attr && attr.getType().isInteger(1) && attr.getValue().isZero()) {
      ++falseLeaves;
      return true;
    }
  }
  auto operation = value.getDefiningOp<arith::OrIOp>();
  return operation && operation->getAttrs().empty() &&
         sourceOrTree(operation.getLhs(), leaves, falseLeaves) &&
         sourceOrTree(operation.getRhs(), leaves, falseLeaves);
}

bool andPair(Value value, Value left, Value right) {
  auto operation = value.getDefiningOp<arith::AndIOp>();
  return operation &&
         ((operation.getLhs() == left && operation.getRhs() == right) ||
          (operation.getLhs() == right && operation.getRhs() == left));
}

bool sourceSelectTree(Value value, ArrayRef<SourceUseOp> uses,
                      DenseSet<unsigned> &seen) {
  auto select = value.getDefiningOp<arith::SelectOp>();
  if (!select) {
    auto zero = value.getDefiningOp<arith::ConstantOp>();
    auto attr = zero ? dyn_cast<IntegerAttr>(zero.getValue()) : IntegerAttr();
    return attr && attr.getValue().isZero();
  }
  if (!select->getAttrs().empty())
    return false;
  for (auto [index, rawUse] : llvm::enumerate(uses)) {
    SourceUseOp use = rawUse;
    if (!seen.contains(index) && select.getCondition() == use.getEnabled() &&
        select.getTrueValue() == use.getData()) {
      seen.insert(index);
      return sourceSelectTree(select.getFalseValue(), uses, seen);
    }
  }
  return false;
}

void pathAtoms(Value value, DenseMap<Value, bool> &atoms,
               bool polarity = true) {
  atoms.try_emplace(value, polarity);
  if (auto conjunction = value.getDefiningOp<arith::AndIOp>();
      polarity && conjunction) {
    pathAtoms(conjunction.getLhs(), atoms, polarity);
    pathAtoms(conjunction.getRhs(), atoms, polarity);
    return;
  }
  if (auto inverted = value.getDefiningOp<arith::XOrIOp>()) {
    auto lhs = inverted.getLhs().getDefiningOp<arith::ConstantOp>();
    auto rhs = inverted.getRhs().getDefiningOp<arith::ConstantOp>();
    bool lhsTrue = lhs && cast<IntegerAttr>(lhs.getValue()).getValue().isOne();
    bool rhsTrue = rhs && cast<IntegerAttr>(rhs.getValue()).getValue().isOne();
    if (lhsTrue != rhsTrue) {
      pathAtoms(lhsTrue ? inverted.getRhs() : inverted.getLhs(), atoms,
                !polarity);
      return;
    }
  }
}

bool exclusive(Value left, Value right) {
  DenseMap<Value, bool> lhs, rhs;
  pathAtoms(left, lhs);
  pathAtoms(right, rhs);
  for (const auto &[value, polarity] : lhs) {
    auto found = rhs.find(value);
    if (found != rhs.end() && found->second != polarity)
      return true;
  }
  return false;
}

LogicalResult verifyRangeChecks(RuleOp rule, Block &body) {
  auto emit = [&] { return rule.emitOpError(); };
  auto required = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
  SmallVector<SourceExpectOp> checks(body.getOps<SourceExpectOp>());
  if (!required || required.size() != checks.size())
    return emit() << "composition checks differ from retained requirements";
  for (MathToBitsOp boundary : body.getOps<MathToBitsOp>()) {
    auto shape = boundary->getAttrOfType<DictionaryAttr>("ac.check_template");
    SourceExpectOp found;
    for (SourceExpectOp check : checks) {
      auto id = check->getAttrOfType<DictionaryAttr>("ac.check_id");
      auto occurrence =
          id ? id.getAs<DictionaryAttr>("check") : DictionaryAttr();
      auto site = occurrence ? occurrence.getAs<DictionaryAttr>("site")
                             : DictionaryAttr();
      if (shape && shape.getAs<DictionaryAttr>("leaf") == site) {
        if (found)
          return emit() << "composition boundary has duplicate range checks";
        found = check;
      }
    }
    if (!found || found.getKind() != "range" ||
        found.getCondition() != boundary.getValid() ||
        !andPair(found.getPath(), boundary.getPath(), boundary.getValueValid()))
      return emit()
             << "composition boundary lacks its exact source range check";
  }
  return success();
}

LogicalResult verifySourceUses(RuleOp rule, Block &body) {
  auto emit = [&] { return rule.emitOpError(); };
  auto required = rule->getAttrOfType<ArrayAttr>("ac.required_uses");
  auto rows = rule->getAttrOfType<ArrayAttr>("ac.yield_bindings");
  auto outputs = rule->getAttrOfType<ArrayAttr>("ac.output_bindings");
  auto outputTypes = rule->getAttrOfType<ArrayAttr>("ac.output_types");
  auto yield = dyn_cast_or_null<YieldOp>(body.getTerminator());
  SmallVector<SourceUseOp> uses(body.getOps<SourceUseOp>());
  if (!required || !rows || !outputs || !outputTypes ||
      outputTypes.size() != outputs.size() || required.size() != uses.size() ||
      rows.size() != outputs.size() || !yield ||
      yield.getValues().size() != 2 * rows.size())
    return emit() << "composition source use/yield inventory is incomplete";
  DenseMap<Attribute, SourceUseOp> actual;
  for (SourceUseOp use : uses)
    if (!actual.try_emplace(use.getIdAttr(), use).second)
      return emit() << "composition source repeats a UseID";
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
      return emit() << "composition source RequiredUse schema is invalid";
  }
  DenseSet<Attribute> consumed;
  for (auto [targetIndex, raw] : llvm::enumerate(rows)) {
    auto row = dyn_cast<DictionaryAttr>(raw);
    auto contributions =
        row ? row.getAs<ArrayAttr>("contributions") : ArrayAttr();
    if (!row || row.size() != 4 || !contributions || contributions.empty() ||
        row.getAs<DictionaryAttr>("target") !=
            dyn_cast<DictionaryAttr>(outputs[targetIndex]))
      return emit() << "composition source YieldBinding is malformed";
    SmallVector<SourceUseOp> candidates;
    DenseSet<Value> enables;
    for (Attribute rawContribution : contributions) {
      auto contribution = dyn_cast<DictionaryAttr>(rawContribution);
      auto id = contribution ? contribution.getAs<DictionaryAttr>("use")
                             : DictionaryAttr();
      auto found = actual.find(id);
      auto requirement = requiredByID.find(id);
      if (!contribution || contribution.size() != 2 ||
          !contribution.getAs<UnitAttr>("selection_ordinal") ||
          found == actual.end() || requirement == requiredByID.end() ||
          requirement->second.getAs<DictionaryAttr>("value") !=
              found->second.getSourceAttr() ||
          requirement->second.getAs<DictionaryAttr>("target") !=
              found->second.getTargetAttr() ||
          !consumed.insert(id).second ||
          found->second.getTargetAttr().getAs<DictionaryAttr>("state") !=
              outputs[targetIndex])
        return emit() << "composition contribution has no exact SourceUse";
      auto logical = dyn_cast<DictionaryAttr>(outputTypes[targetIndex]);
      if (auto boundary =
              found->second.getValue().getDefiningOp<MathToBitsOp>()) {
        if (!rhsIdentityMatchesUse(id, found->second.getSourceAttr()) ||
            !valueIDMatchesOriginSlot(
                found->second.getSourceAttr(),
                boundary->getAttrOfType<DictionaryAttr>("ac.origin"), 1) ||
            boundary.getDomainAttr() != logical)
          return emit() << "composition numeric use bypasses its boundary";
        if (found->second.getValid() != boundary.getValid() ||
            found->second.getPath() != boundary.getPath())
          return emit() << "composition numeric use redirects evaluation path";
      } else {
        if (!directUseAuthority(found->second.getValue(), logical, id,
                                found->second.getSourceAttr(), rule) ||
            found->second.getValid() != found->second.getPath())
          return emit()
                 << "composition direct use has no literal/read authority";
      }
      candidates.push_back(found->second);
      enables.insert(found->second.getEnabled());
    }
    for (size_t left = 0; left < candidates.size(); ++left)
      for (size_t right = left + 1; right < candidates.size(); ++right)
        if (!exclusive(candidates[left].getPath(), candidates[right].getPath()))
          return emit() << "composition source contributions may overlap";
    DenseSet<Value> remaining(enables);
    DenseSet<unsigned> selected;
    unsigned falseLeaves = 0;
    if (!sourceOrTree(yield.getValues()[2 * targetIndex + 1], remaining,
                      falseLeaves) ||
        !remaining.empty() || falseLeaves != 1 ||
        !sourceSelectTree(yield.getValues()[2 * targetIndex], candidates,
                          selected) ||
        selected.size() != candidates.size())
      return emit() << "composition source yield is not exact select/OR merge";
  }
  return consumed.size() == uses.size()
             ? success()
             : emit() << "composition source has an unconsumed SourceUse";
}

} // namespace

bool safeControl(Value value, RuleOp rule) {
  DenseMap<Value, Visit> memo;
  return safeControlImpl(value, rule, memo);
}

LogicalResult verifySource(RuleOp rule, ArrayAttr required) {
  auto emit = [&] { return rule.emitOpError(); };
  Block &body = rule.getBody().front();
  if (failed(verifyNodeSchema(rule, required)))
    return failure();
  if (!body.getOps<ValueBindingOp>().empty() ||
      !body.getOps<NumericProofOp>().empty() ||
      !body.getOps<ValueUseOp>().empty())
    return emit() << "composition source cannot contain lowered evidence";
  SmallVector<Operation *> producers;
  SmallVector<unsigned> numericConsumers(required.size());
  DenseSet<Operation *> used;
  DenseSet<Attribute> ids;
  producers.reserve(required.size());
  for (auto [index, raw] : llvm::enumerate(required)) {
    auto node = dyn_cast<DictionaryAttr>(raw);
    auto id = node ? node.getAs<DictionaryAttr>("id") : DictionaryAttr();
    auto operation = node ? node.getAs<StringAttr>("operator") : StringAttr();
    auto operands = node ? node.getAs<ArrayAttr>("operands") : ArrayAttr();
    auto target =
        node ? node.getAs<DictionaryAttr>("target") : DictionaryAttr();
    bool known =
        operation && llvm::StringSwitch<bool>(operation.getValue())
                         .Cases({"from_bits", "constant", "add", "and_bits",
                                 "eq", "ne", "lt", "to_bits"},
                                true)
                         .Default(false);
    if (operation && llvm::StringSwitch<bool>(operation.getValue())
                         .Cases({"sub", "le", "gt", "ge"}, true)
                         .Default(false))
      return emit() << "numeric composition supports only add, and_bits, eq, "
                       "ne, lt and to_bits";
    unsigned arity = operation && operation.getValue() == "from_bits"  ? 1
                     : operation && operation.getValue() == "constant" ? 1
                     : operation && operation.getValue() == "to_bits"  ? 1
                                                                       : 2;
    auto targetKind = target ? target.getAs<StringAttr>("kind") : StringAttr();
    bool targetOK = operation && operation.getValue() == "to_bits"
                        ? target && target.size() == 2 && targetKind &&
                              targetKind.getValue() == "integer_boundary" &&
                              target.getAs<DictionaryAttr>("domain")
                        : target && target.size() == 1 && targetKind &&
                              targetKind.getValue() == "none";
    if (!node || node.size() != 4 || !known || !operands ||
        operands.size() != arity || !targetOK ||
        failed(detail::verifyValueID(id, emit)) || !ids.insert(id).second)
      return emit() << "composition NumericNode schema is invalid";
    auto producer = findProducer(body, node, used, rule);
    if (failed(producer))
      return failure();
    producers.push_back(*producer);
    for (Attribute rawRef : operands) {
      auto reference = dyn_cast<DictionaryAttr>(rawRef);
      auto ordinal =
          reference ? reference.getAs<IntegerAttr>("index") : IntegerAttr();
      if (reference &&
          reference.getAs<StringAttr>("kind") ==
              StringAttr::get(rule.getContext(), "node") &&
          ordinal && ordinal.getValue().getActiveBits() <= 32) {
        unsigned ref = ordinal.getValue().getZExtValue();
        if (ref >= index || ++numericConsumers[ref] > 1)
          return emit() << "composition forbids cross-expression numeric CSE";
      }
    }
    if (operation.getValue() == "from_bits") {
      auto from = cast<MathFromBitsOp>(*producer);
      auto read = from.getValue().getDefiningOp<SourceReadOp>();
      auto current =
          read ? dyn_cast<BlockArgument>(read.getCurrent()) : BlockArgument();
      auto inputTypes = rule->getAttrOfType<ArrayAttr>("ac.input_types");
      auto interpretation =
          from.getDomainAttr().getAs<StringAttr>("interpretation");
      if (!ref(operands[0], "input", 0) || !read || !current ||
          current.getOwner() != &body || !inputTypes ||
          current.getArgNumber() >= inputTypes.size() || !interpretation ||
          interpretation.getValue() != "unsigned" ||
          from.getDomainAttr() !=
              cast<DictionaryAttr>(inputTypes[current.getArgNumber()]))
        return emit() << "composition from_bits has no authoritative input";
      continue;
    }
    if (operation.getValue() == "constant") {
      auto constant = cast<MathConstantOp>(*producer);
      auto reference = dyn_cast<DictionaryAttr>(operands[0]);
      bool negative =
          constant.getValueAttr().getCanonicalValue().starts_with("-");
      if (!reference || reference.size() != 2 ||
          reference.getAs<StringAttr>("kind") !=
              StringAttr::get(rule.getContext(), "constant") ||
          reference.getAs<MathIntAttr>("value") != constant.getValueAttr() ||
          negative)
        return emit() << "composition constant recipe differs from source";
      continue;
    }
    auto valueFor = [&](Attribute rawRef) -> Value {
      auto reference = dyn_cast<DictionaryAttr>(rawRef);
      auto ordinal =
          reference ? reference.getAs<IntegerAttr>("index") : IntegerAttr();
      auto decoded = detail::decodeU32(ordinal, "composition node ref", emit);
      return reference &&
                     reference.getAs<StringAttr>("kind") ==
                         StringAttr::get(rule.getContext(), "node") &&
                     succeeded(decoded) && *decoded < index
                 ? producers[*decoded]->getResult(0)
                 : Value();
    };
    if (operation.getValue() == "to_bits") {
      auto convert = cast<MathToBitsOp>(*producer);
      if (convert.getValue() != valueFor(operands[0]) ||
          !safeControl(convert.getPath(), rule) ||
          !safeControl(convert.getValueValid(), rule) ||
          target.getAs<DictionaryAttr>("domain") != convert.getDomainAttr())
        return emit() << "composition boundary SSA/control is invalid";
      continue;
    }
    Value lhs = valueFor(operands[0]), rhs = valueFor(operands[1]);
    Operation *math = *producer;
    if (!lhs || !rhs || math->getOperand(1) != lhs ||
        math->getOperand(3) != rhs || !safeControl(math->getOperand(0), rule) ||
        !safeControl(math->getOperand(2), rule) ||
        !safeControl(math->getOperand(4), rule))
      return emit() << "composition binary/compare SSA control is invalid";
  }
  size_t actualMath = 0;
  for (Operation &operation : body) {
    actualMath += operation.getName().getStringRef().starts_with("ac.math.");
    if (operation.getName().getStringRef().starts_with("ac.math.") ||
        isa<SourceReadOp, SourceUseOp, SourceExpectOp, SourceObserveOp, YieldOp,
            arith::ConstantOp, arith::AndIOp, arith::OrIOp, arith::XOrIOp,
            arith::SelectOp>(operation))
      continue;
    return emit() << "composition source contains hidden finite arithmetic";
  }
  if (actualMath != required.size())
    return emit() << "composition source has unowned math operations";
  if (failed(verifyRangeChecks(rule, body)))
    return failure();
  return verifySourceUses(rule, body);
}

} // namespace composition_detail

bool hasNumericCompositionContract(RuleOp rule) {
  auto required = rule ? rule->getAttrOfType<ArrayAttr>("ac.required_numeric")
                       : ArrayAttr();
  if (!rule || !required || required.empty())
    return false;
  if (hasNumericNextUseContract(rule))
    return false;
  return rule->hasAttr("ac.required_uses") &&
         rule->hasAttr("ac.yield_bindings");
}

LogicalResult verifyNumericCompositionClosure(RuleOp rule) {
  auto emit = [&] { return rule.emitOpError(); };
  if (!hasNumericCompositionContract(rule) ||
      rule.getBody().getBlocks().size() != 1)
    return emit() << "numeric composition contract is not present";
  auto scope = rule->getAttrOfType<DictionaryAttr>("ac.proof_scope");
  auto required = rule->getAttrOfType<ArrayAttr>("ac.required_numeric");
  if (!scope || failed(detail::verifyProofScope(scope, emit)) ||
      scope.getAs<DictionaryAttr>("registration") != rule.getRegistrationAttr())
    return emit() << "numeric composition proof scope is invalid";
  Block &body = rule.getBody().front();
  bool source = llvm::any_of(body, [](Operation &operation) {
    return operation.getName().getStringRef().starts_with("ac.math.");
  });
  return source ? composition_detail::verifySource(rule, required)
                : composition_detail::verifyLowered(rule, required);
}

LogicalResult verifyNumericCompositionExpect(SourceExpectOp op) {
  auto rule = op->getParentOfType<RuleOp>();
  if (!rule || op->getBlock() != &rule.getBody().front() ||
      !composition_detail::safeControl(op.getCondition(), rule) ||
      !composition_detail::safeControl(op.getPath(), rule))
    return op.emitOpError()
           << "composition check requires verified condition and live path";
  if (op.getKind() == "range" &&
      rule.getBody().front().getOps<MathToBitsOp>().empty()) {
    auto id = op->getAttrOfType<DictionaryAttr>("ac.check_id");
    bool matched = false;
    for (NumericProofOp proof :
         rule.getBody().front().getOps<NumericProofOp>()) {
      auto checks = proof.getChecksAttr();
      auto sizes = proof.getOperandSegmentSizes();
      if (!checks || sizes.size() != 7)
        continue;
      for (Attribute raw : checks) {
        auto check = dyn_cast<DictionaryAttr>(raw);
        auto ordinal =
            check ? check.getAs<IntegerAttr>("operand_ordinal") : IntegerAttr();
        if (!check || check.getAs<DictionaryAttr>("id") != id || !ordinal)
          continue;
        unsigned index = ordinal.getValue().getZExtValue();
        unsigned conditions =
            sizes[0] + sizes[1] + sizes[2] + sizes[3] + sizes[4];
        unsigned paths = conditions + sizes[5];
        if (index >= unsigned(sizes[5]) || index >= unsigned(sizes[6]) ||
            proof->getOperand(conditions + index) != op.getCondition() ||
            proof->getOperand(paths + index) != op.getPath() || matched)
          return op.emitOpError()
                 << "composition range check is outside its unique proof";
        matched = true;
      }
    }
    if (!matched)
      return op.emitOpError() << "composition range check has no proof binding";
  }
  return success();
}

LogicalResult verifyNumericCompositionObserve(SourceObserveOp op) {
  auto rule = op->getParentOfType<RuleOp>();
  if (!rule || op->getBlock() != &rule.getBody().front() ||
      !composition_detail::safeControl(op.getPath(), rule))
    return op.emitOpError() << "composition observation path is invalid";
  return success();
}

} // namespace acir::ac
