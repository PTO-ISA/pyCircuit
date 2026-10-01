#include "ACIRFinalContracts.h"

#include "ACIRFinalRecordUses.h"
#include "ACIRFinalUses.h"
#include "ACIRNumericComposition.h"
#include "ACIRNumericProof.h"
#include "ACIRSourceContracts.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"

#include <algorithm>

using namespace mlir;

namespace acir::ac {
namespace {

mlir::ModuleOp finalPackage(Operation *operation) {
  auto unit = operation ? operation->getParentOfType<mlir::ModuleOp>()
                        : mlir::ModuleOp();
  auto package = unit ? dyn_cast_or_null<mlir::ModuleOp>(unit->getParentOp())
                      : mlir::ModuleOp();
  auto stage =
      package ? package->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
  if (!unit || !package || !stage || stage.getValue() != "final" ||
      package->hasAttr("ac.unit_kind") || !package->hasAttr("ac.entry") ||
      !package->hasAttr("ac.instance_bindings") ||
      !llvm::hasSingleElement(package.getBody()->getOps<SystemOp>()))
    return {};
  return package;
}

APSInt mathValue(MathIntAttr attr) {
  StringRef text = attr.getCanonicalValue();
  bool negative = text.consume_front("-");
  unsigned width = std::max(2u, unsigned(text.size() * 4 + 2));
  APInt bits(width, text, 10);
  if (negative)
    bits = -bits;
  return APSInt(std::move(bits), !negative);
}
APInt extend(const APSInt &value, unsigned width) {
  return value.isUnsigned() ? value.zextOrTrunc(width)
                            : value.sextOrTrunc(width);
}
int compare(const APSInt &left, const APSInt &right) {
  unsigned width = std::max(left.getBitWidth(), right.getBitWidth()) + 1;
  APInt lhs = extend(left, width), rhs = extend(right, width);
  if (lhs == rhs)
    return 0;
  return lhs.slt(rhs) ? -1 : 1;
}
LogicalResult initialInDomain(Attribute initial, DictionaryAttr logical,
                              Operation *owner) {
  auto kind = logical.getAs<StringAttr>("kind");
  if (!kind || kind.getValue() != "integer")
    return success();
  auto value = dyn_cast<IntegerAttr>(initial);
  auto lower = logical.getAs<MathIntAttr>("lower");
  auto upper = logical.getAs<MathIntAttr>("upper");
  auto interpretation = logical.getAs<StringAttr>("interpretation");
  if (!value || !lower || !upper || !interpretation)
    return owner->emitOpError() << "final integer reset domain is incomplete";
  bool isUnsigned = interpretation.getValue() == "unsigned";
  APSInt actual(value.getValue(), isUnsigned);
  if (compare(actual, mathValue(lower)) < 0 ||
      compare(actual, mathValue(upper)) >= 0)
    return owner->emitOpError()
           << "final register reset image is outside its logical interval";
  return success();
}
LogicalResult envelope(Operation *operation) {
  auto emit = [&] { return operation->emitOpError(); };
  auto file = operation->getParentOfType<mlir::ModuleOp>();
  auto stage =
      file ? file->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
  auto kind =
      file ? file->getAttrOfType<StringAttr>("ac.unit_kind") : StringAttr();
  auto owner = file ? file->getAttrOfType<DictionaryAttr>("ac.source_owner")
                    : DictionaryAttr();
  if (!file || !stage || stage.getValue() != "final" || !kind ||
      kind.getValue() != "implementation" ||
      failed(detail::verifySourceOwner(owner, emit)) ||
      !finalPackage(operation))
    return emit() << "final hardware requires a final implementation unit "
                     "with retained source owner and unique system package";
  return success();
}

LogicalResult physical(Type type, DictionaryAttr logical, Operation *owner) {
  auto emit = [&] { return owner->emitOpError(); };
  if (!logical || failed(detail::verifyLogicalTypeStructure(logical, emit)))
    return failure();
  auto kind = logical.getAs<StringAttr>("kind");
  if (kind.getValue() == "bool")
    return type.isInteger(1) ? success()
                             : emit() << "final bool payload must be i1";
  if (kind.getValue() == "integer") {
    auto storage = logical.getAs<TypeAttr>("storage");
    return storage && storage.getValue() == type
               ? success()
               : emit() << "final integer payload differs from storage";
  }
  if (kind.getValue() == "record") {
    auto symbol = logical.getAs<FlatSymbolRefAttr>("symbol");
    auto record = dyn_cast<StructType>(type);
    return symbol && record && record.getName().getValue() == symbol.getValue()
               ? success()
               : emit() << "final record payload differs from nominal type";
  }
  return emit() << "final native packet rejects nonscalar logical payload";
}

LogicalResult port(DictionaryAttr value, Operation *owner) {
  auto emit = [&] { return owner->emitOpError(); };
  if (!value || value.size() != 6 || !value.getAs<StringAttr>("parameter") ||
      !value.get("ordinal") || !value.getAs<StringAttr>("role") ||
      !value.getAs<DictionaryAttr>("type") ||
      failed(detail::verifyLogicalTypeStructure(
          value.getAs<DictionaryAttr>("type"), emit)) ||
      failed(detail::verifyOccurrence(value.getAs<DictionaryAttr>("origin"),
                                      emit)) ||
      failed(detail::verifySourceSpan(value.getAs<DictionaryAttr>("location"),
                                      emit)))
    return emit() << "final PortSlot is malformed";
  StringRef role = value.getAs<StringAttr>("role").getValue();
  if (role != "current" && role != "next")
    return emit() << "final PortSlot role must be current or next";
  Attribute ordinal = value.get("ordinal");
  if (!isa<UnitAttr>(ordinal) &&
      failed(detail::decodeU64(dyn_cast<IntegerAttr>(ordinal),
                               "final PortSlot ordinal", emit)))
    return failure();
  return success();
}

LogicalResult controls(ModuleOp module) {
  auto emit = [&] { return module.emitOpError(); };
  auto value = module->getAttrOfType<DictionaryAttr>("ac.control_ports");
  auto clock = value ? value.getAs<IntegerAttr>("clock") : IntegerAttr();
  auto reset = value ? value.getAs<IntegerAttr>("reset") : IntegerAttr();
  return value && value.size() == 2 && clock && reset &&
                 clock.getType().isInteger(32) &&
                 reset.getType().isInteger(32) && clock.getValue().isZero() &&
                 reset.getValue().isOne()
             ? success()
             : emit() << "final controls require clock=0 and reset=1";
}

bool moduleControl(Value value, ModuleOp module, unsigned index) {
  auto argument = dyn_cast<BlockArgument>(value);
  return argument && argument.getOwner() == &module.getBody().front() &&
         argument.getArgNumber() == index && value.getType().isInteger(1);
}

FailureOr<DictionaryAttr> directInput(Value value, RuleOp rule,
                                      Operation *owner) {
  auto argument = dyn_cast<BlockArgument>(value);
  auto types = rule->getAttrOfType<ArrayAttr>("ac.input_types");
  if (!argument || argument.getOwner() != &rule.getBody().front() || !types ||
      argument.getArgNumber() >= types.size())
    return owner->emitOpError() << "final finite value is not a rule input";
  auto logical = dyn_cast<DictionaryAttr>(types[argument.getArgNumber()]);
  if (!logical || failed(physical(value.getType(), logical, owner)))
    return failure();
  return logical;
}

bool trueI1(Value value) {
  auto op = value.getDefiningOp<arith::ConstantOp>();
  auto attr = op ? dyn_cast<IntegerAttr>(op.getValue()) : IntegerAttr();
  return attr && attr.getType().isInteger(1) && attr.getValue().isOne();
}

bool boolInput(Value value, RuleOp rule) {
  auto argument = dyn_cast<BlockArgument>(value);
  if (!argument || argument.getOwner() != &rule.getBody().front())
    return false;
  auto logical = directInput(value, rule, rule);
  auto kind =
      succeeded(logical) ? (*logical).getAs<StringAttr>("kind") : StringAttr();
  return kind && kind.getValue() == "bool";
}

bool approvedPath(Value value, RuleOp rule) {
  if (trueI1(value) || boolInput(value, rule))
    return true;
  auto inverted = value.getDefiningOp<arith::XOrIOp>();
  if (!inverted || inverted->getBlock() != &rule.getBody().front())
    return false;
  Value source;
  if (trueI1(inverted.getLhs()))
    source = inverted.getRhs();
  else if (trueI1(inverted.getRhs()))
    source = inverted.getLhs();
  return source && boolInput(source, rule);
}

LogicalResult handle(Value value, DictionaryAttr ref, DictionaryAttr logical,
                     StringRef role, RuleOp rule) {
  auto emit = [&] { return rule.emitOpError(); };
  auto module = rule->getParentOfType<ModuleOp>();
  auto kind = ref ? ref.getAs<StringAttr>("kind") : StringAttr();
  if (!module || failed(detail::verifyStateRef(ref, emit)) || !kind)
    return failure();
  if (kind.getValue() == "owned") {
    auto reg = value.getDefiningOp<RegOp>();
    if (!reg || reg->getParentOfType<ModuleOp>() != module ||
        ref.getAs<DictionaryAttr>("declaration") !=
            reg->getAttrOfType<DictionaryAttr>("ac.declaration") ||
        ref.getAs<ArrayAttr>("element") !=
            reg->getAttrOfType<ArrayAttr>("ac.element") ||
        logical != reg->getAttrOfType<DictionaryAttr>("ac.logical_type"))
      return emit() << "final owned rule handle does not match its register";
    return success();
  }
  auto argument = dyn_cast<BlockArgument>(value);
  auto ports = module->getAttrOfType<ArrayAttr>("ac.ports");
  if (!argument || argument.getOwner() != &module.getBody().front() || !ports ||
      argument.getArgNumber() < 2 ||
      argument.getArgNumber() - 2 >= ports.size())
    return emit() << "final formal rule handle has no module PortSlot";
  auto actual = dyn_cast<DictionaryAttr>(ports[argument.getArgNumber() - 2]);
  if (!actual ||
      actual.getAs<StringAttr>("role") !=
          StringAttr::get(rule.getContext(), role) ||
      actual.getAs<StringAttr>("parameter") !=
          ref.getAs<StringAttr>("parameter") ||
      actual.get("ordinal") != ref.get("ordinal") ||
      actual.getAs<DictionaryAttr>("type") != logical)
    return emit() << "final formal rule handle differs from its PortSlot";
  return success();
}

FlatSymbolRefAttr definition(DictionaryAttr occurrence) {
  auto site =
      occurrence ? occurrence.getAs<DictionaryAttr>("site") : DictionaryAttr();
  return site ? site.getAs<FlatSymbolRefAttr>("definition")
              : FlatSymbolRefAttr();
}

LogicalResult observationSpec(SourceObserveOp op, ArrayAttr logicalTypes) {
  auto emit = [&] { return op.emitOpError(); };
  StringRef kind = op.getKind();
  auto spec = op.getSpecAttr();
  if (kind == "print")
    return spec && spec.size() == 3 && spec.getAs<ArrayAttr>("items") &&
                   spec.getAs<StringAttr>("sep") &&
                   spec.getAs<StringAttr>("end")
               ? success()
               : emit() << "final print spec is malformed";
  if (kind == "log")
    return spec && spec.size() == 3 && spec.getAs<ArrayAttr>("items") &&
                   spec.getAs<StringAttr>("level") &&
                   spec.getAs<StringAttr>("event")
               ? success()
               : emit() << "final log spec is malformed";
  if (kind != "report" || !spec || spec.size() != 1 ||
      !spec.getAs<StringAttr>("name") || op.getValues().size() != 1 ||
      !logicalTypes || logicalTypes.size() != 1)
    return emit() << "final report spec is malformed";
  auto logical = dyn_cast<DictionaryAttr>(logicalTypes[0]);
  auto lower = logical ? logical.getAs<MathIntAttr>("lower") : MathIntAttr();
  if (!logical ||
      logical.getAs<StringAttr>("kind") !=
          StringAttr::get(op.getContext(), "integer") ||
      !lower || llvm::APSInt(lower.getCanonicalValue()).isNegative())
    return emit() << "final report requires one nonnegative integer";
  return success();
}

LogicalResult identity(SourceExpectOp op, RuleOp rule) {
  auto emit = [&] { return op.emitOpError(); };
  auto id = op->getAttrOfType<DictionaryAttr>("ac.check_id");
  auto module = op->getParentOfType<ModuleOp>();
  auto check = id ? id.getAs<DictionaryAttr>("check") : DictionaryAttr();
  auto location = op->getAttrOfType<DictionaryAttr>("location");
  auto owner = module ? module->getAttrOfType<DictionaryAttr>("ac.source_owner")
                      : DictionaryAttr();
  if (!id || !owner || failed(detail::verifyCheckID(id, emit)) ||
      id.getAs<DictionaryAttr>("registration") != rule.getRegistrationAttr() ||
      !module ||
      definition(check) !=
          FlatSymbolRefAttr::get(op.getContext(), module.getSymName()) ||
      failed(detail::verifySourceSpan(location, emit)) ||
      location.getAs<StringAttr>("path") != owner.getAs<StringAttr>("path"))
    return emit() << "final check identity/location is outside its rule";
  return success();
}

} // namespace

LogicalResult verifyFinalRuleHandle(RuleOp rule, Value actual,
                                    DictionaryAttr state,
                                    DictionaryAttr logical, StringRef role) {
  return handle(actual, state, logical, role, rule);
}

bool isFinalImplementation(Operation *operation) {
  auto file = operation ? operation->getParentOfType<mlir::ModuleOp>()
                        : mlir::ModuleOp();
  auto stage =
      file ? file->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
  return stage && stage.getValue() == "final";
}

LogicalResult verifyFinalReg(RegOp op) {
  auto emit = [&] { return op.emitOpError(); };
  if (failed(envelope(op)))
    return failure();
  auto owner = op->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto declaration = op->getAttrOfType<DictionaryAttr>("ac.declaration");
  auto element = op->getAttrOfType<ArrayAttr>("ac.element");
  auto logical = op->getAttrOfType<DictionaryAttr>("ac.logical_type");
  auto initial = op->getAttr("ac.initial_value");
  auto domain = op->getAttrOfType<StringAttr>("ac.domain");
  auto typed = dyn_cast_if_present<TypedAttr>(initial);
  auto module = op->getParentOfType<ModuleOp>();
  auto state = dyn_cast<RegType>(op.getState().getType());
  if (op.getName().empty() || failed(detail::verifySourceOwner(owner, emit)) ||
      failed(detail::verifyOccurrence(declaration, emit)) || !element ||
      !logical || !initial || !typed || !domain ||
      domain.getValue() != "default" || op->hasAttr("ac.shape") ||
      op->hasAttr("ac.logical_element") || op->hasAttr("ac.initial_image") ||
      !module ||
      owner != module->getAttrOfType<DictionaryAttr>("ac.source_owner") ||
      !state || typed.getType() != state.getElementType() ||
      failed(physical(state.getElementType(), logical, op)) ||
      failed(initialInDomain(initial, logical, op)) ||
      !moduleControl(op.getClock(), module, 0) ||
      !moduleControl(op.getReset(), module, 1))
    return emit() << "final register metadata is incomplete or mixed";
  for (Attribute raw : element)
    if (failed(detail::decodeU64(dyn_cast<IntegerAttr>(raw),
                                 "final register element", emit)))
      return failure();
  return success();
}

LogicalResult verifyFinalModule(ModuleOp op) {
  auto emit = [&] { return op.emitOpError(); };
  if (failed(envelope(op)))
    return failure();
  auto owner = op->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto origin = op->getAttrOfType<DictionaryAttr>("ac.origin");
  auto ports = op->getAttrOfType<ArrayAttr>("ac.ports");
  if (op.getName().empty() || op.getSymName().empty() ||
      failed(detail::verifySourceOwner(owner, emit)) ||
      failed(detail::verifyOccurrence(origin, emit)) || !ports ||
      op->hasAttr("ac.final_ports") || failed(controls(op)))
    return emit() << "final module metadata is incomplete or mixed";
  bool seenNext = false;
  llvm::DenseSet<Attribute> identities;
  for (Attribute raw : ports) {
    auto value = dyn_cast<DictionaryAttr>(raw);
    if (failed(port(value, op)))
      return failure();
    StringRef role = value.getAs<StringAttr>("role").getValue();
    seenNext |= role == "next";
    if (role == "current" && seenNext)
      return emit() << "final ports require current before next";
    auto key = DictionaryAttr::get(
        op.getContext(), {NamedAttribute(StringAttr::get(op.getContext(), "p"),
                                         value.get("parameter")),
                          NamedAttribute(StringAttr::get(op.getContext(), "o"),
                                         value.get("ordinal")),
                          NamedAttribute(StringAttr::get(op.getContext(), "r"),
                                         value.get("role"))});
    if (!identities.insert(key).second)
      return emit() << "final module repeats a PortSlot";
  }
  return success();
}

LogicalResult verifyFinalRule(RuleOp op) {
  auto emit = [&] { return op.emitOpError(); };
  if (failed(envelope(op)))
    return failure();
  auto module = op->getParentOfType<ModuleOp>();
  auto owner = op->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto inputs = op->getAttrOfType<ArrayAttr>("ac.input_bindings");
  auto outputs = op->getAttrOfType<ArrayAttr>("ac.output_bindings");
  auto inputTypes = op->getAttrOfType<ArrayAttr>("ac.input_types");
  auto outputTypes = op->getAttrOfType<ArrayAttr>("ac.output_types");
  auto checks = op->getAttrOfType<ArrayAttr>("ac.required_checks");
  auto observations = op->getAttrOfType<ArrayAttr>("ac.required_observations");
  if (!module ||
      owner != module->getAttrOfType<DictionaryAttr>("ac.source_owner") ||
      failed(detail::verifySourceOwner(owner, emit)) || op.getName().empty() ||
      failed(detail::verifyOccurrence(op.getRegistrationAttr(), emit)) ||
      failed(detail::verifyOccurrence(
          op->getAttrOfType<DictionaryAttr>("ac.origin"), emit)) ||
      !inputs || !outputs || !inputTypes || !outputTypes || !checks ||
      !observations || inputs.size() != op.getInputs().size() ||
      outputs.size() != op.getTargets().size() ||
      inputTypes.size() != inputs.size() ||
      outputTypes.size() != outputs.size())
    return emit() << "final rule metadata is incomplete";
  for (auto [index, raw] : llvm::enumerate(inputs)) {
    auto logical = dyn_cast<DictionaryAttr>(inputTypes[index]);
    if (!logical ||
        failed(physical(
            cast<RegType>(op.getInputs()[index].getType()).getElementType(),
            logical, op)) ||
        failed(handle(op.getInputs()[index], dyn_cast<DictionaryAttr>(raw),
                      logical, "current", op)))
      return emit() << "final rule input authority is invalid";
  }
  for (auto [index, raw] : llvm::enumerate(outputs)) {
    auto logical = dyn_cast<DictionaryAttr>(outputTypes[index]);
    if (!logical ||
        failed(physical(
            cast<RegType>(op.getTargets()[index].getType()).getElementType(),
            logical, op)) ||
        failed(handle(op.getTargets()[index], dyn_cast<DictionaryAttr>(raw),
                      logical, "next", op)))
      return emit() << "final rule output authority is invalid";
  }
  return op.getNextValues().size() == 2 * op.getTargets().size()
             ? success()
             : emit() << "final rule results require data,enable per target";
}

LogicalResult verifyFinalRuleRegion(RuleOp op) {
  auto emit = [&] { return op.emitOpError(); };
  if (op.getBody().getBlocks().size() != 1)
    return emit() << "final rule requires one block";
  if (hasFinalRecordUses(op))
    return verifyFinalRecordUses(op);
  Block &body = op.getBody().front();
  if (body.getNumArguments() != op.getInputs().size())
    return emit() << "final rule block arguments differ from inputs";
  for (auto [index, value] : llvm::enumerate(op.getInputs()))
    if (body.getArgument(index).getType() !=
        cast<RegType>(value.getType()).getElementType())
      return emit() << "final rule current payload type is invalid";
  if (!body.getOps<SourceReadOp>().empty() ||
      !body.getOps<SourceUseOp>().empty() ||
      llvm::any_of(body, [](Operation &nested) {
        return nested.getName().getStringRef().starts_with("ac.math.");
      }))
    return emit() << "final rule retains source-only computation";
  auto requiredNumeric = op->getAttrOfType<ArrayAttr>("ac.required_numeric");
  bool numeric = requiredNumeric && !requiredNumeric.empty();
  if (numeric) {
    if (failed(verifyRuleNumericClosure(op)))
      return failure();
  } else if (!op.getTargets().empty()) {
    if (!hasGenericFinalUses(op) || failed(verifyGenericFinalUses(op)))
      return emit()
             << "nonnumeric final target rule lacks generic use authority";
  } else if (op->hasAttr("ac.required_uses") ||
             op->hasAttr("ac.yield_bindings") ||
             !body.getOps<ValueBindingOp>().empty() ||
             !body.getOps<ValueUseOp>().empty()) {
    return emit() << "targetless final rule has orphan generic use evidence";
  }
  SmallVector<SourceObserveOp> observations(body.getOps<SourceObserveOp>());
  auto requiredObservations =
      op->getAttrOfType<ArrayAttr>("ac.required_observations");
  if ((!observations.empty() || requiredObservations) &&
      (!requiredObservations ||
       requiredObservations.size() != observations.size()))
    return emit() << "final observations differ from retained requirements";
  for (auto [index, observation] : llvm::enumerate(observations)) {
    auto expected = dyn_cast<DictionaryAttr>(requiredObservations[index]);
    if (!expected ||
        expected.getAs<DictionaryAttr>("id") !=
            observation->getAttrOfType<DictionaryAttr>("ac.observation_id") ||
        expected.getAs<StringAttr>("kind") != observation.getKindAttr() ||
        expected.getAs<DictionaryAttr>("spec") != observation.getSpecAttr() ||
        expected.getAs<ArrayAttr>("values") !=
            observation->getAttrOfType<ArrayAttr>("ac.value_ids"))
      return emit() << "final observation requirement was redirected";
  }
  SmallVector<SourceExpectOp> checks(body.getOps<SourceExpectOp>());
  auto requiredChecks = op->getAttrOfType<ArrayAttr>("ac.required_checks");
  if ((!checks.empty() || requiredChecks) &&
      (!requiredChecks || requiredChecks.size() != checks.size()))
    return emit() << "final checks differ from retained requirements";
  for (auto [index, check] : llvm::enumerate(checks)) {
    auto expected = dyn_cast<DictionaryAttr>(requiredChecks[index]);
    auto obligation =
        detail::decodeU64(check->getAttrOfType<DictionaryAttr>("ac.check_id")
                              .getAs<IntegerAttr>("obligation"),
                          "final check obligation", emit);
    if (failed(obligation) || *obligation != index || !expected ||
        expected.getAs<DictionaryAttr>("id") !=
            check->getAttrOfType<DictionaryAttr>("ac.check_id") ||
        expected.getAs<StringAttr>("kind") != check.getKindAttr() ||
        expected.getAs<DictionaryAttr>("location") !=
            check->getAttrOfType<DictionaryAttr>("location"))
      return emit() << "final check requirement was redirected";
  }
  auto yield = dyn_cast_or_null<YieldOp>(body.getTerminator());
  if (!yield || yield.getValues().size() != 2 * op.getTargets().size() ||
      op.getNextValues().size() != yield.getValues().size())
    return emit() << "final rule yield/result arity is invalid";
  for (size_t index = 0; index < op.getTargets().size(); ++index) {
    Type payload =
        cast<RegType>(op.getTargets()[index].getType()).getElementType();
    if (yield.getValues()[2 * index].getType() != payload ||
        !yield.getValues()[2 * index + 1].getType().isInteger(1))
      return emit() << "final rule yield data/enable type is invalid";
  }
  for (auto [result, yielded] :
       llvm::zip(op.getNextValues(), yield.getValues()))
    if (result.getType() != yielded.getType())
      return emit() << "final rule result type differs from its yield";
  return success();
}

LogicalResult verifyFinalExpect(SourceExpectOp op) {
  auto rule = op->getParentOfType<RuleOp>();
  if (!rule || op->getBlock() != &rule.getBody().front() ||
      failed(identity(op, rule)))
    return failure();
  if (hasNumericCompositionContract(rule))
    return verifyNumericCompositionExpect(op);
  if (op.getKind() == "range") {
    SmallVector<NumericProofOp> proofs(
        rule.getBody().front().getOps<NumericProofOp>());
    NumericProofOp checked;
    for (NumericProofOp proof : proofs)
      if (!proof.getChecksAttr().empty()) {
        if (checked)
          return op.emitOpError() << "final range check has multiple proofs";
        checked = proof;
      }
    if (!checked || checked->getNumOperands() != 7 ||
        checked->getOperand(5) != op.getCondition() ||
        checked->getOperand(6) != op.getPath())
      return op.emitOpError() << "final range check is outside its proof";
    return success();
  }
  if (op.getKind() != "assert" || !boolInput(op.getCondition(), rule) ||
      !approvedPath(op.getPath(), rule))
    return op.emitOpError()
           << "final assert requires direct finite bool condition/path";
  return success();
}

LogicalResult verifyFinalObserve(SourceObserveOp op) {
  auto emit = [&] { return op.emitOpError(); };
  auto rule = op->getParentOfType<RuleOp>();
  auto module = op->getParentOfType<ModuleOp>();
  auto id = op->getAttrOfType<DictionaryAttr>("ac.observation_id");
  auto registration =
      id ? id.getAs<DictionaryAttr>("registration") : DictionaryAttr();
  auto site = id ? id.getAs<DictionaryAttr>("site") : DictionaryAttr();
  auto valueIDs = op->getAttrOfType<ArrayAttr>("ac.value_ids");
  auto logicalTypes = op->getAttrOfType<ArrayAttr>("ac.logical_types");
  bool composition = hasNumericCompositionContract(rule);
  bool pathOK =
      rule && (composition ? succeeded(verifyNumericCompositionObserve(op))
                           : approvedPath(op.getPath(), rule));
  if (!rule || op->getBlock() != &rule.getBody().front() || !module || !id ||
      id.size() != 2 || failed(detail::verifyOccurrence(registration, emit)) ||
      failed(detail::verifyOccurrence(site, emit)) ||
      registration != rule.getRegistrationAttr() ||
      definition(site) !=
          FlatSymbolRefAttr::get(op.getContext(), module.getSymName()) ||
      !valueIDs || !logicalTypes || op->hasAttr("ac.value_constraints") ||
      valueIDs.size() != op.getValues().size() ||
      logicalTypes.size() != op.getValues().size() || !pathOK)
    return emit()
           << "final observation identity, path or value list is invalid";
  for (auto [index, value] : llvm::enumerate(op.getValues())) {
    auto valueID = dyn_cast<DictionaryAttr>(valueIDs[index]);
    auto logical = dyn_cast<DictionaryAttr>(logicalTypes[index]);
    auto constant = value.getDefiningOp<arith::ConstantOp>();
    bool literal = composition && constant && valueID && logical &&
                   constant->getAttrs().size() == 2 &&
                   constant->hasAttr("value") &&
                   constant->hasAttr("ac.origin") &&
                   valueID.getAs<DictionaryAttr>("origin") ==
                       constant->getAttrOfType<DictionaryAttr>("ac.origin") &&
                   succeeded(physical(value.getType(), logical, op)) &&
                   succeeded(initialInDomain(constant.getValue(), logical, op));
    auto actual = literal ? FailureOr<DictionaryAttr>(failure())
                          : directInput(value, rule, op);
    if (!valueID || failed(detail::verifyValueID(valueID, emit)) || !logical ||
        failed(detail::verifyLogicalTypeStructure(logical, emit)) ||
        (!literal && (failed(actual) || *actual != logical)))
      return emit() << "final observation value is not a retained finite input";
  }
  return observationSpec(op, logicalTypes);
}

} // namespace acir::ac
