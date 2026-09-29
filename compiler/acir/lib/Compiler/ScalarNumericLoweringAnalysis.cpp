#include "ScalarNumericLoweringDetail.h"

#include "Dialect/ACIR/ACIRNumericComposition.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSwitch.h"

using namespace mlir;

namespace acir::compiler::detail {
namespace {

struct RecipeNode {
  DictionaryAttr id;
  ArrayAttr operands;
};

bool isTrueI1(Value value) {
  auto constant = value.getDefiningOp<arith::ConstantOp>();
  auto integer =
      constant ? dyn_cast<IntegerAttr>(constant.getValue()) : IntegerAttr();
  return integer && integer.getType().isInteger(1) &&
         integer.getValue().isOne();
}

bool isNoneTarget(DictionaryAttr target, MLIRContext *context) {
  return target && target.size() == 1 &&
         target.getAs<StringAttr>("kind") == StringAttr::get(context, "none");
}

DictionaryAttr inputValueID(Builder &builder, DictionaryAttr origin) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("origin", origin),
      builder.getNamedAttr("slot", builder.getI32IntegerAttr(0)),
  });
}

FailureOr<RecipeNode> recipeNode(Attribute raw, StringRef opcode,
                                 size_t operandCount, Operation *owner) {
  auto emit = [&] { return owner->emitOpError(); };
  auto node = dyn_cast<DictionaryAttr>(raw);
  auto id = node ? node.getAs<DictionaryAttr>("id") : DictionaryAttr();
  auto op = node ? node.getAs<StringAttr>("operator") : StringAttr();
  auto operands = node ? node.getAs<ArrayAttr>("operands") : ArrayAttr();
  auto target = node ? node.getAs<DictionaryAttr>("target") : DictionaryAttr();
  if (!node || node.size() != 4 || !id || !op || op.getValue() != opcode ||
      !operands || operands.size() != operandCount ||
      !isNoneTarget(target, owner->getContext()) ||
      failed(ac::detail::verifyValueID(id, emit)))
    return emit() << "source numeric recipe node has an unsupported shape";
  return RecipeNode{id, operands};
}

bool exactRef(DictionaryAttr raw, StringRef kind, unsigned ordinal,
              Operation *owner) {
  auto refKind = raw ? raw.getAs<StringAttr>("kind") : StringAttr();
  auto index = raw ? raw.getAs<IntegerAttr>("index") : IntegerAttr();
  if (!raw || raw.size() != 2 || !refKind || refKind.getValue() != kind ||
      !index)
    return false;
  auto decoded = ac::detail::decodeU32(index, "source NumericRef index",
                                       [&] { return owner->emitOpError(); });
  return succeeded(decoded) && *decoded == ordinal;
}

bool oneUseBy(Value value, Operation *consumer) {
  return llvm::hasSingleElement(value.getUses()) &&
         value.getUses().begin()->getOwner() == consumer;
}

bool trueConstantUseOnly(Value value, Operation *consumer) {
  if (!isTrueI1(value))
    return false;
  return llvm::all_of(value.getUses(), [&](OpOperand &use) {
    return use.getOwner() == consumer;
  });
}

LogicalResult verifyNodeIDs(ArrayAttr required, RuleInventory inventory) {
  Operation *owner = inventory.rule;
  auto emit = [&] { return owner->emitOpError(); };
  Builder builder(owner->getContext());
  auto fromNode = recipeNode(required[0], "from_bits", 1, owner);
  auto constantNode = recipeNode(required[1], "constant", 1, owner);
  auto addNode = recipeNode(required[2], "add", 2, owner);
  if (failed(fromNode) || failed(constantNode) || failed(addNode))
    return failure();
  DictionaryAttr inputID = inputValueID(
      builder, inventory.read->getAttrOfType<DictionaryAttr>("ac.origin"));
  if (fromNode->id.getAs<DictionaryAttr>("origin") !=
          inventory.fromBits->getAttrOfType<DictionaryAttr>("ac.origin") ||
      constantNode->id.getAs<DictionaryAttr>("origin") !=
          inventory.constant->getAttrOfType<DictionaryAttr>("ac.origin") ||
      addNode->id.getAs<DictionaryAttr>("origin") !=
          inventory.add->getAttrOfType<DictionaryAttr>("ac.origin") ||
      inputID == fromNode->id || inputID == constantNode->id ||
      inputID == addNode->id || fromNode->id == constantNode->id ||
      fromNode->id == addNode->id || constantNode->id == addNode->id)
    return emit() << "source ValueIDs must use their producer origins";
  if (!exactRef(dyn_cast<DictionaryAttr>(fromNode->operands[0]), "input", 0,
                owner))
    return emit() << "from_bits must reference input 0";
  auto constantRef = dyn_cast<DictionaryAttr>(constantNode->operands[0]);
  auto constantValue = constantRef ? constantRef.getAs<ac::MathIntAttr>("value")
                                   : ac::MathIntAttr();
  if (!constantRef || constantRef.size() != 2 || !constantValue ||
      constantRef.getAs<StringAttr>("kind") !=
          StringAttr::get(owner->getContext(), "constant") ||
      constantValue !=
          inventory.constant->getAttrOfType<ac::MathIntAttr>("value"))
    return emit() << "constant recipe must match its MathConstant producer";
  if (!exactRef(dyn_cast<DictionaryAttr>(addNode->operands[0]), "node", 0,
                owner) ||
      !exactRef(dyn_cast<DictionaryAttr>(addNode->operands[1]), "node", 1,
                owner))
    return emit() << "add recipe must reference from_bits then constant";
  return success();
}

LogicalResult validateRule(RuleInventory inventory) {
  ac::RuleOp rule = inventory.rule;
  auto emit = [&] { return rule.emitOpError(); };
  auto module = rule->getParentOfType<ac::ModuleOp>();
  if (!module || rule.getBody().getBlocks().size() != 1 ||
      !rule.getBody().front().getTerminator() || rule.getInputs().size() != 1 ||
      !rule.getTargets().empty())
    return emit() << "N0-C1 requires one single-block input-only source rule";
  Block *body = &rule.getBody().front();
  if (inventory.read->getBlock() != body ||
      inventory.fromBits->getBlock() != body ||
      inventory.constant->getBlock() != body ||
      inventory.add->getBlock() != body)
    return emit() << "N0-C1 source math operations must share the rule block";

  auto inputTypes = rule->getAttrOfType<ArrayAttr>("ac.input_types");
  auto inputBindings = rule->getAttrOfType<ArrayAttr>("ac.input_bindings");
  auto outputBindings = rule->getAttrOfType<ArrayAttr>("ac.output_bindings");
  auto outputTypes = rule->getAttrOfType<ArrayAttr>("ac.output_types");
  auto required = rule->getAttrOfType<ArrayAttr>("ac.required_numeric");
  auto checks = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
  auto observations =
      rule->getAttrOfType<ArrayAttr>("ac.required_observations");
  if (!inputTypes || inputTypes.size() != 1 || !inputBindings ||
      inputBindings.size() != 1 || !outputBindings || !outputBindings.empty() ||
      !outputTypes || !outputTypes.empty() || !required ||
      required.size() != 3 || (checks && !checks.empty()) ||
      (observations && !observations.empty()) ||
      !body->getOps<ac::ValueBindingOp>().empty() ||
      !body->getOps<ac::NumericProofOp>().empty())
    return emit() << "N0-C1 source rule has existing evidence or unsupported "
                     "targets/checks/observations";
  auto scope = rule->getAttrOfType<DictionaryAttr>("ac.proof_scope");
  if (!scope || failed(ac::detail::verifyProofScope(scope, emit)) ||
      scope.get("registration") != rule.getRegistrationAttr())
    return emit() << "N0-C1 rule proof scope is not canonical";
  auto specialization = scope.getAs<DictionaryAttr>("specialization");
  auto definition = specialization
                        ? specialization.getAs<FlatSymbolRefAttr>("definition")
                        : FlatSymbolRefAttr();
  auto arguments = specialization ? specialization.getAs<ArrayAttr>("arguments")
                                  : ArrayAttr();
  auto symbol = module->getAttrOfType<StringAttr>("sym_name");
  if (!definition || !arguments || !arguments.empty() || !symbol ||
      definition.getValue() != symbol.getValue())
    return emit() << "N0-C1 proof scope must name the enclosing empty SpecKey";

  auto inputDomain = dyn_cast<DictionaryAttr>(inputTypes[0]);
  auto storage =
      inputDomain ? inputDomain.getAs<TypeAttr>("storage") : TypeAttr();
  auto kind =
      inputDomain ? inputDomain.getAs<StringAttr>("kind") : StringAttr();
  auto inputType = dyn_cast<ac::RegType>(rule.getInputs()[0].getType());
  auto current = dyn_cast<BlockArgument>(inventory.read.getCurrent());
  if (!inputDomain || inputDomain != inventory.fromBits.getDomainAttr() ||
      !storage || !kind || kind.getValue() != "integer" || !inputType ||
      inputType.getElementType() != storage.getValue() || !current ||
      current.getOwner() != body || current.getArgNumber() != 0 ||
      inventory.read.getResult().getType() != storage.getValue())
    return emit() << "N0-C1 source read/from_bits must use the exact real "
                     "integer input";
  if (failed(ac::detail::verifyStateRef(
          dyn_cast<DictionaryAttr>(inputBindings[0]), emit)))
    return failure();
  for (DictionaryAttr origin :
       {inventory.read->getAttrOfType<DictionaryAttr>("ac.origin"),
        inventory.fromBits->getAttrOfType<DictionaryAttr>("ac.origin"),
        inventory.constant->getAttrOfType<DictionaryAttr>("ac.origin"),
        inventory.add->getAttrOfType<DictionaryAttr>("ac.origin")})
    if (failed(ac::detail::verifyOccurrence(origin, emit)))
      return failure();
  auto operatorAttr = inventory.add->getAttrOfType<StringAttr>("operator");
  if (inventory.fromBits.getValue() != inventory.read.getResult() ||
      !operatorAttr || operatorAttr.getValue() != "add" ||
      inventory.add->hasAttr("ac.check_template") ||
      inventory.add->hasAttr("domain") ||
      !trueConstantUseOnly(inventory.add.getPath(), inventory.add) ||
      !trueConstantUseOnly(inventory.add.getLhsValid(), inventory.add) ||
      !trueConstantUseOnly(inventory.add.getRhsValid(), inventory.add))
    return emit()
           << "N0-C1 requires one from_bits/constant add with true controls";
  const bool ordered =
      inventory.add.getLhs() == inventory.fromBits.getResult() &&
      inventory.add.getRhs() == inventory.constant.getResult();
  if (!ordered || !oneUseBy(inventory.read.getResult(), inventory.fromBits) ||
      !oneUseBy(inventory.fromBits.getResult(), inventory.add) ||
      !oneUseBy(inventory.constant.getResult(), inventory.add) ||
      !inventory.add.getResult().use_empty() ||
      !inventory.add.getValid().use_empty())
    return emit() << "N0-C1 source graph has unsupported or extra SSA uses";

  for (Operation &operation : body->getOperations()) {
    if (isa<ac::SourceReadOp, ac::MathFromBitsOp, ac::MathConstantOp,
            ac::MathBinaryOp, ac::YieldOp>(operation))
      continue;
    if (auto constant = dyn_cast<arith::ConstantOp>(operation)) {
      if (!isTrueI1(constant.getResult()) ||
          llvm::any_of(constant.getResult().getUsers(), [&](Operation *user) {
            return user != inventory.add.getOperation();
          }))
        return emit() << "N0-C1 permits only true control constants";
      continue;
    }
    return emit() << "N0-C1 source rule contains an unsupported operation";
  }
  return verifyNodeIDs(required, inventory);
}

} // namespace

FailureOr<UnitInventory> inspectUnit(ModuleOp unit,
                                     ac::detail::EmitError emitError) {
  if (!unit)
    return emitError() << "exact input-add inventory requires a module";
  auto stage = unit->getAttrOfType<StringAttr>("ac.stage");
  auto unitKind = unit->getAttrOfType<StringAttr>("ac.unit_kind");
  if (!stage || stage.getValue() != "source" || !unitKind ||
      unitKind.getValue() != "implementation")
    return emitError() << "N0-C1 requires a source implementation unit";
  UnitInventory result;
  bool unsupportedMath = false;
  bool residualMathType = false;
  bool hasAnyEvidence = false;
  bool hasC1LoweredProof = false;
  DenseSet<Operation *> sourceRuleSet;
  unit->walk([&](Operation *operation) {
    StringRef name = operation->getName().getStringRef();
    const bool supported =
        isa<ac::MathFromBitsOp, ac::MathConstantOp, ac::MathBinaryOp,
            ac::MathCompareOp, ac::MathToBitsOp>(operation);
    unsupportedMath |= name.starts_with("ac.math.") && !supported;
    if (supported) {
      auto rule = operation->getParentOfType<ac::RuleOp>();
      if (!rule)
        unsupportedMath = true;
      else if (sourceRuleSet.insert(rule.getOperation()).second)
        result.sourceRules.push_back(rule);
    }
    if (auto proof = dyn_cast<ac::NumericProofOp>(operation)) {
      hasAnyEvidence = true;
      hasC1LoweredProof |= proof.getInputIdsAttr().size() == 1;
    } else if (isa<ac::ValueBindingOp>(operation)) {
      hasAnyEvidence = true;
    }
    for (Type type : operation->getOperandTypes())
      residualMathType |= isa<ac::MathIntType>(type);
    for (Type type : operation->getResultTypes())
      residualMathType |= isa<ac::MathIntType>(type);
  });
  if (unsupportedMath)
    return emitError() << "N0-C1 found unsupported math inventory";
  if (result.sourceRules.empty()) {
    if (residualMathType)
      return emitError() << "numeric lowering found a residual MathInt type";
    result.kind = hasC1LoweredProof ? UnitInventoryKind::Lowered
                                    : UnitInventoryKind::NoOp;
    return result;
  }
  if (hasAnyEvidence)
    return emitError() << "N0-C1 rejects mixed source and lowered inventory";
  result.kind = UnitInventoryKind::Source;
  return result;
}

FailureOr<RuleInventory> inspectRule(ac::RuleOp rule) {
  if (ac::hasNumericCompositionContract(rule))
    return RuleInventory{RuleInventoryKind::Composition, rule, {}, {}, {}, {}};
  SmallVector<ac::SourceReadOp> reads;
  SmallVector<ac::MathFromBitsOp> fromBits;
  SmallVector<ac::MathConstantOp> constants;
  SmallVector<ac::MathBinaryOp> binaries;
  SmallVector<ac::MathCompareOp> comparisons;
  SmallVector<ac::MathToBitsOp> conversions;
  SmallVector<ac::SourceExpectOp> expects;
  rule.walk([&](Operation *operation) {
    if (auto op = dyn_cast<ac::SourceReadOp>(operation))
      reads.push_back(op);
    else if (auto op = dyn_cast<ac::MathFromBitsOp>(operation))
      fromBits.push_back(op);
    else if (auto op = dyn_cast<ac::MathConstantOp>(operation))
      constants.push_back(op);
    else if (auto op = dyn_cast<ac::MathBinaryOp>(operation))
      binaries.push_back(op);
    else if (auto op = dyn_cast<ac::MathCompareOp>(operation))
      comparisons.push_back(op);
    else if (auto op = dyn_cast<ac::MathToBitsOp>(operation))
      conversions.push_back(op);
    else if (auto op = dyn_cast<ac::SourceExpectOp>(operation))
      expects.push_back(op);
  });
  auto required = rule->getAttrOfType<ArrayAttr>("ac.required_numeric");
  auto checks = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
  const bool numericNextUse =
      reads.size() == 1 && fromBits.size() == 1 && constants.size() == 2 &&
      binaries.size() == 2 && comparisons.empty() && conversions.size() == 1 &&
      expects.size() == 1 && required && required.size() == 6 && checks &&
      checks.size() == 1 && rule.getTargets().size() == 1 &&
      rule->hasAttr("ac.required_uses") && rule->hasAttr("ac.yield_bindings");
  if (numericNextUse)
    return RuleInventory{RuleInventoryKind::NumericNextUse,
                         rule,
                         reads.front(),
                         fromBits.front(),
                         constants.front(),
                         binaries.front()};
  const bool checkedRange =
      reads.size() == 1 && fromBits.size() == 1 && conversions.size() == 1 &&
      expects.size() == 1 && constants.empty() && binaries.empty() &&
      comparisons.empty() && required && required.size() == 2 && checks &&
      checks.size() == 1 &&
      conversions[0].getValue() == fromBits[0].getResult() &&
      expects[0].getCondition() == conversions[0].getValid() &&
      expects[0].getPath() == conversions[0].getPath() &&
      expects[0].getKind() == "range";
  if (checkedRange) {
    auto fromNode = dyn_cast<DictionaryAttr>(required[0]);
    auto convertNode = dyn_cast<DictionaryAttr>(required[1]);
    auto fromOpcode =
        fromNode ? fromNode.getAs<StringAttr>("operator") : StringAttr();
    auto convertOpcode =
        convertNode ? convertNode.getAs<StringAttr>("operator") : StringAttr();
    auto requiredCheck = dyn_cast<DictionaryAttr>(checks[0]);
    if (fromNode && fromNode.size() == 4 && fromOpcode &&
        fromOpcode.getValue() == "from_bits" && convertNode &&
        convertNode.size() == 4 && convertOpcode &&
        convertOpcode.getValue() == "to_bits" && requiredCheck &&
        requiredCheck.getAs<StringAttr>("kind") ==
            StringAttr::get(rule.getContext(), "range") &&
        requiredCheck.getAs<DictionaryAttr>("id") ==
            expects[0]->getAttrOfType<DictionaryAttr>("ac.check_id"))
      return RuleInventory{RuleInventoryKind::CheckedToBits,
                           rule,
                           reads.front(),
                           fromBits.front(),
                           {},
                           {}};
  }
  if (!conversions.empty() || !expects.empty() ||
      (required && required.size() == 2))
    return rule.emitOpError() << "D3 checked range inventory is not closed";
  const bool exactMask =
      reads.size() == 1 && fromBits.size() == 1 && constants.size() == 1 &&
      binaries.size() == 1 && comparisons.empty() && required &&
      required.size() == 3 &&
      binaries.front()->getAttrOfType<StringAttr>("operator") &&
      binaries.front()->getAttrOfType<StringAttr>("operator").getValue() ==
          "and_bits";
  const bool lowBitsMask =
      reads.size() == 1 && fromBits.size() == 1 && constants.size() == 2 &&
      binaries.size() == 2 && comparisons.empty() && required &&
      required.size() == 5 &&
      binaries[0]->getAttrOfType<StringAttr>("operator") &&
      (binaries[0]->getAttrOfType<StringAttr>("operator").getValue() == "add" ||
       binaries[0]->getAttrOfType<StringAttr>("operator").getValue() ==
           "sub") &&
      binaries[1]->getAttrOfType<StringAttr>("operator") &&
      binaries[1]->getAttrOfType<StringAttr>("operator").getValue() ==
          "and_bits";
  if (exactMask || lowBitsMask)
    return RuleInventory{RuleInventoryKind::InputMask,
                         rule,
                         reads.front(),
                         fromBits.front(),
                         constants.front(),
                         binaries.front()};
  if (reads.size() != 1 || fromBits.size() != 1 || constants.size() != 1 ||
      binaries.size() + comparisons.size() != 1)
    return rule.emitOpError()
           << "N0 rule inventory is not exactly one read/F/C/scalar operation";
  if (!comparisons.empty()) {
    auto predicate = comparisons.front().getPredicateAttr();
    if (!predicate || !llvm::StringSwitch<bool>(predicate.getValue())
                           .Cases({"eq", "ne", "lt", "le", "gt", "ge"}, true)
                           .Default(false))
      return rule.emitOpError() << "N0-D1 comparison predicate is unsupported";
    return RuleInventory{RuleInventoryKind::ExactInputScalar,
                         rule,
                         reads.front(),
                         fromBits.front(),
                         constants.front(),
                         {}};
  }
  auto opcode = binaries.front()->getAttrOfType<StringAttr>("operator");
  if (opcode && opcode.getValue() == "sub")
    return RuleInventory{RuleInventoryKind::ExactInputScalar,
                         rule,
                         reads.front(),
                         fromBits.front(),
                         constants.front(),
                         binaries.front()};
  if (!opcode || opcode.getValue() != "add")
    return rule.emitOpError() << "N0 scalar binary operator is unsupported";
  RuleInventory result{RuleInventoryKind::ExactInputAdd,
                       rule,
                       reads.front(),
                       fromBits.front(),
                       constants.front(),
                       binaries.front()};
  if (failed(validateRule(result)))
    return failure();
  return result;
}

} // namespace acir::compiler::detail
