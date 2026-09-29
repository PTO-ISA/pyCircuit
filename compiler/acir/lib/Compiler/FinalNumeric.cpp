#include "FinalNumeric.h"
#include "Dialect/ACIR/ACIRNumericComposition.h"
#include "FinalComposedNumeric.h"

#include "Dialect/ACIR/ACIRNumericNextUse.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/DenseSet.h"

using namespace mlir;

namespace acir::compiler {
namespace {

DictionaryAttr attributes(Operation *operation) {
  Builder builder(operation->getContext());
  return builder.getDictionaryAttr(operation->getAttrs());
}

Value replacement(Value value, const DenseMap<Value, Value> &replacements) {
  DenseSet<Value> seen;
  while (value && seen.insert(value).second) {
    auto found = replacements.find(value);
    if (found == replacements.end())
      break;
    value = found->second;
  }
  return value;
}

bool trueI1(Value value) {
  auto constant = value.getDefiningOp<arith::ConstantOp>();
  auto integer =
      constant ? dyn_cast<IntegerAttr>(constant.getValue()) : IntegerAttr();
  return integer && integer.getType().isInteger(1) &&
         integer.getValue().isOne();
}

bool integerConstant(Value value, uint64_t expected, unsigned width) {
  auto constant = value.getDefiningOp<arith::ConstantOp>();
  auto integer =
      constant ? dyn_cast<IntegerAttr>(constant.getValue()) : IntegerAttr();
  return integer && integer.getType().isInteger(width) &&
         integer.getValue().getLimitedValue() == expected;
}

const ProposalContribution *singleContribution(const ProposalGraph &graph,
                                               ac::RuleOp rule) {
  const ProposalContribution *found = nullptr;
  for (const ProposalContribution &candidate : graph.contributions) {
    if (candidate.rule != rule)
      continue;
    if (found)
      return nullptr;
    found = &candidate;
  }
  return found;
}

const CheckBinding *singleCheck(const CheckGraph &graph, ac::RuleOp rule) {
  const CheckBinding *found = nullptr;
  for (const CheckBinding &candidate : graph.bindings) {
    if (candidate.rule != rule)
      continue;
    if (found)
      return nullptr;
    found = &candidate;
  }
  return found;
}

LogicalResult verifyFiniteChain(const FinalNumericRuleSnapshot &snapshot,
                                ac::detail::EmitError emitError) {
  if (snapshot.values.size() != 6 || snapshot.proofs.size() != 2)
    return emitError() << "final U1 evidence cardinality changed";
  const auto &input = snapshot.values[0];
  const auto &lifted = snapshot.values[1];
  const auto &one = snapshot.values[2];
  const auto &mask = snapshot.values[3];
  const auto &low = snapshot.values[4];
  const auto &boundary = snapshot.values[5];
  if (input.value != snapshot.current || lifted.value != snapshot.current ||
      !integerConstant(one.value, 1, 1) || !integerConstant(mask.value, 255, 8))
    return emitError() << "final U1 input or finite constants changed";
  for (const FinalNumericValueSnapshot &value : snapshot.values)
    if (!value.id || !value.domain || !trueI1(value.valid) ||
        !trueI1(value.path))
      return emitError() << "final U1 binding identity/domain/control changed";

  auto finiteAnd = low.value.getDefiningOp<arith::AndIOp>();
  auto add = finiteAnd ? finiteAnd.getLhs().getDefiningOp<arith::AddIOp>()
                       : arith::AddIOp();
  if (!add && finiteAnd)
    add = finiteAnd.getRhs().getDefiningOp<arith::AddIOp>();
  Value maskOperand = finiteAnd && add && finiteAnd.getLhs() == add.getResult()
                          ? finiteAnd.getRhs()
                      : finiteAnd ? finiteAnd.getLhs()
                                  : Value();
  auto extension =
      add ? add.getRhs().getDefiningOp<arith::ExtUIOp>() : arith::ExtUIOp();
  if (!add || add.getLhs() != snapshot.current || !extension ||
      extension.getIn() != one.value || !add.getType().isInteger(8) ||
      add.getOverflowFlags() != arith::IntegerOverflowFlags::none ||
      !finiteAnd || !finiteAnd.getType().isInteger(8) ||
      !finiteAnd->getAttrs().empty() || maskOperand != mask.value ||
      boundary.value != low.value || boundary.valid != low.valid ||
      boundary.path != low.path)
    return emitError() << "final U1 finite add/mask/boundary chain changed";

  if (snapshot.useValue != boundary.value)
    return emitError() << "final U1 ValueUse data authority changed";
  if (snapshot.useValid != boundary.valid || !trueI1(snapshot.usePath))
    return emitError() << "final U1 ValueUse control authority changed";
  if (snapshot.yieldData != boundary.value)
    return emitError() << "final U1 yield data authority changed";
  auto enable = snapshot.yieldEnable.getDefiningOp<arith::AndIOp>();
  if (!enable || enable.getLhs() != snapshot.useValid ||
      enable.getRhs() != snapshot.usePath || !enable->getAttrs().empty())
    return emitError() << "final U1 yield enable is not valid and path";
  if (!trueI1(snapshot.checkCondition) || !trueI1(snapshot.checkPath))
    return emitError() << "final U1 range boundary check changed";

  ArrayRef<Value> lowOperands = snapshot.proofs[0].operands;
  ArrayRef<Value> boundaryOperands = snapshot.proofs[1].operands;
  if (lowOperands.size() != 5 || boundaryOperands.size() != 7 ||
      lowOperands[0] != input.path || lowOperands[1] != input.value ||
      lowOperands[2] != input.valid || lowOperands[3] != low.value ||
      lowOperands[4] != low.valid || boundaryOperands[0] != low.path ||
      boundaryOperands[1] != low.value || boundaryOperands[2] != low.valid ||
      boundaryOperands[3] != boundary.value ||
      boundaryOperands[4] != boundary.valid ||
      boundaryOperands[5] != snapshot.checkCondition ||
      boundaryOperands[6] != snapshot.checkPath)
    return emitError() << "final U1 proof roots changed after source removal";
  return success();
}

} // namespace

bool isSupportedFinalNumericRule(ac::RuleOp rule) {
  auto required = rule ? rule->getAttrOfType<ArrayAttr>("ac.required_numeric")
                       : ArrayAttr();
  if (!required || required.empty())
    return true;
  if (ac::hasNumericCompositionContract(rule))
    return succeeded(ac::verifyNumericCompositionClosure(rule));
  return ac::hasNumericNextUseContract(rule) &&
         succeeded(ac::verifyNumericNextUseClosure(rule));
}

bool isRetainedFinalNumericCarrier(ArrayRef<FinalNumericRuleSnapshot> snapshots,
                                   Operation *operation) {
  if (!operation)
    return false;
  ac::RuleOp owner = dyn_cast<ac::RuleOp>(operation);
  if (!owner)
    owner = operation->getParentOfType<ac::RuleOp>();
  if (!owner)
    return false;
  for (const FinalNumericRuleSnapshot &snapshot : snapshots)
    if (snapshot.rule == owner)
      return isa<ac::RuleOp, ac::ValueBindingOp, ac::NumericProofOp,
                 ac::ValueUseOp>(operation);
  return false;
}

static FailureOr<SmallVector<FinalNumericRuleSnapshot, 0>>
freezeFinalNumericEvidenceImpl(const ModuleGraph &modules,
                               const ProposalGraph &proposals,
                               const CheckGraph &checks, bool finalHardware,
                               ac::detail::EmitError emitError) {
  SmallVector<FinalNumericRuleSnapshot, 0> result;
  DenseSet<Operation *> composedDefinitions;
  for (InstanceView *owner : modules.views) {
    for (ac::RuleOp rule :
         owner->module.getBody().front().getOps<ac::RuleOp>()) {
      auto required = rule->getAttrOfType<ArrayAttr>("ac.required_numeric");
      if (!required || required.empty())
        continue;
      if (ac::hasNumericCompositionContract(rule)) {
        if (!composedDefinitions.insert(rule).second)
          continue;
        auto snapshot = freezeComposedNumeric(*owner, rule, emitError);
        if (failed(snapshot))
          return failure();
        result.push_back(std::move(*snapshot));
        continue;
      }
      if (!ac::hasNumericNextUseContract(rule) ||
          failed(ac::verifyNumericNextUseClosure(rule)))
        return emitError() << "final numeric source rule is not verified U1";
      const ProposalContribution *contribution =
          singleContribution(proposals, rule);
      const CheckBinding *check = singleCheck(checks, rule);
      Block &body = rule.getBody().front();
      auto reads = body.getOps<ac::SourceReadOp>();
      auto uses = body.getOps<ac::ValueUseOp>();
      auto expects = body.getOps<ac::SourceExpectOp>();
      auto bindings = body.getOps<ac::ValueBindingOp>();
      auto proofs = body.getOps<ac::NumericProofOp>();
      auto yield = dyn_cast<ac::YieldOp>(body.getTerminator());
      bool readsMatch =
          finalHardware ? reads.empty() : llvm::hasSingleElement(reads);
      if (!contribution || !check || !readsMatch ||
          !llvm::hasSingleElement(uses) || !llvm::hasSingleElement(expects) ||
          std::distance(bindings.begin(), bindings.end()) != 6 ||
          std::distance(proofs.begin(), proofs.end()) != 2 || !yield ||
          yield.getValues().size() != 2)
        return emitError() << "final U1 graph/evidence inventory is incomplete";

      FinalNumericRuleSnapshot snapshot;
      snapshot.owner = owner;
      snapshot.rule = rule;
      snapshot.stateID = contribution->stateID;
      snapshot.relativeState = cast<DictionaryAttr>(
          rule->getAttrOfType<ArrayAttr>("ac.output_bindings")[0]);
      snapshot.useID = contribution->useID;
      snapshot.sourceID = contribution->sourceID;
      snapshot.checkID = check->checkID;
      snapshot.recipe = required;
      snapshot.requiredUses =
          rule->getAttrOfType<ArrayAttr>("ac.required_uses");
      snapshot.yieldBindings =
          rule->getAttrOfType<ArrayAttr>("ac.yield_bindings");
      snapshot.proofScope =
          rule->getAttrOfType<DictionaryAttr>("ac.proof_scope");
      snapshot.current = finalHardware ? (*bindings.begin()).getValue()
                                       : (*reads.begin()).getCurrent();
      for (ac::ValueBindingOp binding : bindings)
        snapshot.values.push_back({binding, attributes(binding),
                                   binding.getIdAttr(), binding.getDomainAttr(),
                                   binding.getValue(), binding.getValid(),
                                   binding.getPath()});
      for (ac::NumericProofOp proof : proofs) {
        FinalNumericProofSnapshot frozen;
        frozen.operation = proof;
        frozen.attributes = attributes(proof);
        frozen.operands.append(proof->operand_begin(), proof->operand_end());
        snapshot.proofs.push_back(std::move(frozen));
      }
      ac::ValueUseOp use = *uses.begin();
      snapshot.use = use;
      snapshot.useAttributes = attributes(use);
      snapshot.useValue = use.getValue();
      snapshot.useValid = use.getValid();
      snapshot.usePath = use.getPath();
      snapshot.yieldData = yield.getValues()[0];
      snapshot.yieldEnable = yield.getValues()[1];
      snapshot.checkCondition = check->condition;
      snapshot.checkPath = check->path;
      result.push_back(std::move(snapshot));
    }
  }
  return result;
}

FailureOr<SmallVector<FinalNumericRuleSnapshot, 0>> freezeFinalNumericEvidence(
    const ModuleGraph &modules, const ProposalGraph &proposals,
    const CheckGraph &checks, ac::detail::EmitError emitError) {
  return freezeFinalNumericEvidenceImpl(modules, proposals, checks,
                                        /*finalHardware=*/false, emitError);
}

FailureOr<SmallVector<FinalNumericRuleSnapshot, 0>>
freezeFinalNumericEvidenceFromHardware(const ModuleGraph &modules,
                                       const ProposalGraph &proposals,
                                       const CheckGraph &checks,
                                       ac::detail::EmitError emitError) {
  return freezeFinalNumericEvidenceImpl(modules, proposals, checks,
                                        /*finalHardware=*/true, emitError);
}

void rebaseFinalNumericEvidence(
    MutableArrayRef<FinalNumericRuleSnapshot> snapshots,
    const DenseMap<Value, Value> &replacements) {
  for (FinalNumericRuleSnapshot &snapshot : snapshots) {
    for (auto &carrier : snapshot.composedCarriers)
      for (Value &operand : carrier.operands)
        operand = replacement(operand, replacements);
    snapshot.current = replacement(snapshot.current, replacements);
    for (FinalNumericValueSnapshot &value : snapshot.values) {
      value.value = replacement(value.value, replacements);
      value.valid = replacement(value.valid, replacements);
      value.path = replacement(value.path, replacements);
    }
    for (FinalNumericProofSnapshot &proof : snapshot.proofs)
      for (Value &operand : proof.operands)
        operand = replacement(operand, replacements);
    snapshot.useValue = replacement(snapshot.useValue, replacements);
    snapshot.useValid = replacement(snapshot.useValid, replacements);
    snapshot.usePath = replacement(snapshot.usePath, replacements);
    snapshot.yieldData = replacement(snapshot.yieldData, replacements);
    snapshot.yieldEnable = replacement(snapshot.yieldEnable, replacements);
    snapshot.checkCondition =
        replacement(snapshot.checkCondition, replacements);
    snapshot.checkPath = replacement(snapshot.checkPath, replacements);
  }
}

LogicalResult verifyFinalNumericEvidence(
    ArrayRef<FinalNumericRuleSnapshot> snapshots, const ModuleGraph &modules,
    const ProposalGraph &proposals, const CheckGraph &checks,
    ac::detail::EmitError emitError) {
  DenseSet<Operation *> numericRules;
  for (const FinalNumericRuleSnapshot &snapshot : snapshots) {
    if (snapshot.composition) {
      if (!numericRules.insert(snapshot.rule).second ||
          failed(verifyComposedNumeric(snapshot, modules, emitError)))
        return failure();
      continue;
    }
    if (!snapshot.owner || !snapshot.rule ||
        !numericRules.insert(snapshot.rule).second ||
        snapshot.rule->getParentOfType<ac::ModuleOp>() !=
            snapshot.owner->module ||
        !llvm::is_contained(modules.views, snapshot.owner) ||
        !snapshot.stateID || !snapshot.relativeState || !snapshot.useID ||
        !snapshot.sourceID || !snapshot.checkID || !snapshot.recipe ||
        !snapshot.requiredUses || !snapshot.yieldBindings ||
        !snapshot.proofScope)
      return emitError() << "final U1 frozen authority is incomplete";
    ac::RuleOp rule = snapshot.rule;
    if (rule->getAttrOfType<ArrayAttr>("ac.required_numeric") !=
            snapshot.recipe ||
        rule->getAttrOfType<ArrayAttr>("ac.required_uses") !=
            snapshot.requiredUses ||
        rule->getAttrOfType<ArrayAttr>("ac.yield_bindings") !=
            snapshot.yieldBindings ||
        rule->getAttrOfType<DictionaryAttr>("ac.proof_scope") !=
            snapshot.proofScope)
      return emitError() << "final U1 rule evidence attributes changed";
    Block &body = rule.getBody().front();
    SmallVector<ac::ValueBindingOp> liveBindings(
        body.getOps<ac::ValueBindingOp>());
    SmallVector<ac::NumericProofOp> liveProofs(
        body.getOps<ac::NumericProofOp>());
    SmallVector<ac::ValueUseOp> liveUses(body.getOps<ac::ValueUseOp>());
    SmallVector<ac::SourceExpectOp> liveExpects(
        body.getOps<ac::SourceExpectOp>());
    if (!body.getOps<ac::SourceReadOp>().empty() ||
        liveBindings.size() != snapshot.values.size() ||
        liveProofs.size() != snapshot.proofs.size() || liveUses.size() != 1 ||
        liveExpects.size() != 1)
      return emitError() << "final U1 retained evidence inventory changed";
    unsigned adds = 0, ands = 0, extensions = 0, constants = 0;
    unsigned bindings = 0, proofs = 0, uses = 0, expects = 0, yields = 0;
    for (Operation &operation : body) {
      if (isa<arith::AddIOp>(operation))
        ++adds;
      else if (isa<arith::AndIOp>(operation))
        ++ands;
      else if (isa<arith::ExtUIOp>(operation))
        ++extensions;
      else if (isa<arith::ConstantOp>(operation))
        ++constants;
      else if (isa<ac::ValueBindingOp>(operation))
        ++bindings;
      else if (isa<ac::NumericProofOp>(operation))
        ++proofs;
      else if (isa<ac::ValueUseOp>(operation))
        ++uses;
      else if (isa<ac::SourceExpectOp>(operation))
        ++expects;
      else if (isa<ac::YieldOp>(operation))
        ++yields;
      else
        return emitError() << "final U1 rule has an unsupported extra op '"
                           << operation.getName() << "'";
    }
    if (adds != 1 || ands != 2 || extensions != 1 || constants != 4 ||
        bindings != 6 || proofs != 2 || uses != 1 || expects != 1 ||
        yields != 1)
      return emitError() << "final U1 finite operation inventory changed";
    for (auto [index, value] : llvm::enumerate(snapshot.values)) {
      if (liveBindings[index] != value.operation)
        return emitError() << "final U1 ValueBinding identity/order changed";
      ac::ValueBindingOp operation = value.operation;
      if (!operation || operation->getParentOfType<ac::RuleOp>() != rule ||
          attributes(operation) != value.attributes ||
          operation.getIdAttr() != value.id ||
          operation.getDomainAttr() != value.domain ||
          operation.getValue() != value.value ||
          operation.getValid() != value.valid ||
          operation.getPath() != value.path)
        return emitError() << "final U1 retained ValueBinding changed";
    }
    for (auto [index, proof] : llvm::enumerate(snapshot.proofs)) {
      if (liveProofs[index] != proof.operation)
        return emitError() << "final U1 NumericProof identity/order changed";
      ac::NumericProofOp operation = proof.operation;
      if (!operation || operation->getParentOfType<ac::RuleOp>() != rule ||
          attributes(operation) != proof.attributes ||
          operation->getNumOperands() != proof.operands.size())
        return emitError() << "final U1 retained NumericProof changed";
      for (auto [index, operand] : llvm::enumerate(operation->getOperands()))
        if (operand != proof.operands[index])
          return emitError() << "final U1 NumericProof roots changed";
    }
    ac::ValueUseOp retainedUse = snapshot.use;
    if (liveUses.front() != retainedUse || !retainedUse ||
        retainedUse->getParentOfType<ac::RuleOp>() != rule ||
        attributes(retainedUse) != snapshot.useAttributes ||
        retainedUse.getIdAttr() != snapshot.useID ||
        retainedUse.getSourceAttr() != snapshot.sourceID ||
        retainedUse.getValue() != snapshot.useValue ||
        retainedUse.getValid() != snapshot.useValid ||
        retainedUse.getPath() != snapshot.usePath)
      return emitError() << "final U1 retained ValueUse changed";
    if (failed(verifyFiniteChain(snapshot, emitError)))
      return failure();

    const ProposalContribution *contribution =
        singleContribution(proposals, rule);
    const CheckBinding *check = singleCheck(checks, rule);
    if (!contribution || contribution->stateID != snapshot.stateID ||
        contribution->useID != snapshot.useID ||
        contribution->sourceID != snapshot.sourceID ||
        contribution->data != snapshot.yieldData ||
        contribution->enabled != snapshot.yieldEnable ||
        contribution->value != snapshot.useValue ||
        contribution->valid != snapshot.useValid ||
        contribution->path != snapshot.usePath || contribution->use ||
        contribution->valueUse != snapshot.use)
      return emitError() << "final U1 proposal authority changed";
    if (!check || check->checkID != snapshot.checkID || !check->expect ||
        check->expect != liveExpects.front() ||
        check->condition != snapshot.checkCondition ||
        check->path != snapshot.checkPath)
      return emitError() << "final U1 range-check authority changed";
    auto yield = dyn_cast<ac::YieldOp>(body.getTerminator());
    if (!yield || yield.getValues().size() != 2 ||
        yield.getValues()[0] != snapshot.yieldData ||
        yield.getValues()[1] != snapshot.yieldEnable)
      return emitError() << "final U1 yield authority changed";
  }
  for (InstanceView *owner : modules.views)
    for (ac::RuleOp rule :
         owner->module.getBody().front().getOps<ac::RuleOp>()) {
      bool finiteNumeric = false;
      for (Operation &operation : rule.getBody().front())
        finiteNumeric |=
            isa<arith::ExtUIOp, arith::AddIOp, arith::CmpIOp, arith::TruncIOp>(
                operation);
      finiteNumeric |= ac::hasNumericCompositionContract(rule);
      if (finiteNumeric != numericRules.contains(rule))
        return emitError() << "final numeric rule inventory is not frozen";
    }
  return success();
}

} // namespace acir::compiler
