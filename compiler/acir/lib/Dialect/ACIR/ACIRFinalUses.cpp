#include "ACIRFinalUses.h"

#include "ACIRSourceContracts.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"

#include <algorithm>

using namespace mlir;

namespace acir::ac {
namespace {

struct Required {
  DictionaryAttr id;
  DictionaryAttr source;
  DictionaryAttr state;
  size_t targetIndex = 0;
  DictionaryAttr logical;
};

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

bool trueI1(Value value) {
  auto constant = value.getDefiningOp<arith::ConstantOp>();
  auto attr =
      constant ? dyn_cast<IntegerAttr>(constant.getValue()) : IntegerAttr();
  return attr && attr.getType().isInteger(1) && attr.getValue().isOne();
}

FailureOr<DictionaryAttr> directInput(Value value, RuleOp rule) {
  auto argument = dyn_cast<BlockArgument>(value);
  auto types = rule->getAttrOfType<ArrayAttr>("ac.input_types");
  if (!argument || argument.getOwner() != &rule.getBody().front() || !types ||
      argument.getArgNumber() >= types.size())
    return failure();
  return dyn_cast<DictionaryAttr>(types[argument.getArgNumber()]);
}

bool boolInput(Value value, RuleOp rule) {
  auto logical = directInput(value, rule);
  auto kind =
      succeeded(logical) ? (*logical).getAs<StringAttr>("kind") : StringAttr();
  return kind && kind.getValue() == "bool" && value.getType().isInteger(1);
}

bool safeControl(Value value, RuleOp rule) {
  if (trueI1(value) || boolInput(value, rule))
    return true;
  auto inverted = value.getDefiningOp<arith::XOrIOp>();
  if (!inverted || inverted->getBlock() != &rule.getBody().front() ||
      inverted->getAttrs().size() != 0)
    return false;
  Value source;
  if (trueI1(inverted.getLhs()))
    source = inverted.getRhs();
  else if (trueI1(inverted.getRhs()))
    source = inverted.getLhs();
  return source && boolInput(source, rule);
}

LogicalResult physical(Type type, DictionaryAttr logical, Operation *owner) {
  auto emit = [&] { return owner->emitOpError(); };
  if (!logical || failed(detail::verifyLogicalTypeStructure(logical, emit)))
    return failure();
  auto kind = logical.getAs<StringAttr>("kind");
  if (kind.getValue() == "bool")
    return type.isInteger(1) ? success()
                             : emit() << "final use bool value must be i1";
  if (kind.getValue() != "integer")
    return emit() << "generic final use supports bool/integer scalar values";
  auto storage = logical.getAs<TypeAttr>("storage");
  return storage && storage.getValue() == type
             ? success()
             : emit() << "final use integer type differs from logical storage";
}

LogicalResult constantInDomain(arith::ConstantOp constant,
                               DictionaryAttr logical) {
  auto kind = logical.getAs<StringAttr>("kind");
  auto value = dyn_cast<IntegerAttr>(constant.getValue());
  if (!kind || !value)
    return constant.emitOpError() << "final use constant must be integer typed";
  if (kind.getValue() == "bool")
    return value.getType().isInteger(1)
               ? success()
               : constant.emitOpError() << "bool constant must be i1";
  auto lower = logical.getAs<MathIntAttr>("lower");
  auto upper = logical.getAs<MathIntAttr>("upper");
  auto interpretation = logical.getAs<StringAttr>("interpretation");
  if (kind.getValue() != "integer" || !lower || !upper || !interpretation)
    return constant.emitOpError() << "integer constant domain is incomplete";
  APSInt actual(value.getValue(), interpretation.getValue() == "unsigned");
  return compare(actual, mathValue(lower)) >= 0 &&
                 compare(actual, mathValue(upper)) < 0
             ? success()
             : constant.emitOpError()
                   << "final use constant is outside the target domain";
}

LogicalResult proofScope(RuleOp rule) {
  auto emit = [&] { return rule.emitOpError(); };
  auto scope = rule->getAttrOfType<DictionaryAttr>("ac.proof_scope");
  auto module = rule->getParentOfType<ModuleOp>();
  if (!scope || failed(detail::verifyProofScope(scope, emit)) ||
      scope.getAs<DictionaryAttr>("registration") !=
          rule.getRegistrationAttr() ||
      !module)
    return emit() << "generic final use proof scope is incomplete";
  auto specialization = scope.getAs<DictionaryAttr>("specialization");
  auto definition = specialization
                        ? specialization.getAs<FlatSymbolRefAttr>("definition")
                        : FlatSymbolRefAttr();
  auto arguments = specialization ? specialization.getAs<ArrayAttr>("arguments")
                                  : ArrayAttr();
  if (!definition || definition.getValue() != module.getSymName() ||
      !arguments || !arguments.empty())
    return emit() << "generic final use scope must name its empty SpecKey";
  return success();
}

LogicalResult actualTarget(RuleOp rule, DictionaryAttr state, size_t index) {
  auto emit = [&] { return rule.emitOpError(); };
  Value handle = rule.getTargets()[index];
  auto module = rule->getParentOfType<ModuleOp>();
  auto kind = state.getAs<StringAttr>("kind");
  if (kind.getValue() == "owned") {
    auto reg = handle.getDefiningOp<RegOp>();
    if (!reg || reg->getParentOfType<ModuleOp>() != module ||
        state.getAs<DictionaryAttr>("declaration") !=
            reg->getAttrOfType<DictionaryAttr>("ac.declaration") ||
        state.getAs<ArrayAttr>("element") !=
            reg->getAttrOfType<ArrayAttr>("ac.element"))
      return emit() << "generic final use owned target is redirected";
    return success();
  }
  auto argument = dyn_cast<BlockArgument>(handle);
  auto ports = module->getAttrOfType<ArrayAttr>("ac.ports");
  if (!argument || argument.getOwner() != &module.getBody().front() ||
      argument.getArgNumber() < 2 || !ports ||
      argument.getArgNumber() - 2 >= ports.size())
    return emit() << "generic final use formal target has no PortSlot";
  auto port = cast<DictionaryAttr>(ports[argument.getArgNumber() - 2]);
  if (port.getAs<StringAttr>("role").getValue() != "next" ||
      port.getAs<StringAttr>("parameter") !=
          state.getAs<StringAttr>("parameter") ||
      port.get("ordinal") != state.get("ordinal"))
    return emit() << "generic final use formal target is redirected";
  return success();
}

FailureOr<SmallVector<Required>> requirements(RuleOp rule) {
  auto emit = [&] { return rule.emitOpError(); };
  auto rawUses = rule->getAttrOfType<ArrayAttr>("ac.required_uses");
  auto outputs = rule->getAttrOfType<ArrayAttr>("ac.output_bindings");
  auto types = rule->getAttrOfType<ArrayAttr>("ac.output_types");
  if (!rawUses || rawUses.empty() || !outputs || !types ||
      outputs.size() != rule.getTargets().size() ||
      types.size() != outputs.size())
    return emit()
           << "generic final use requirement/output metadata is incomplete";
  SmallVector<Required> result;
  DenseSet<Attribute> ids;
  for (Attribute raw : rawUses) {
    auto use = dyn_cast<DictionaryAttr>(raw);
    auto id = use ? use.getAs<DictionaryAttr>("id") : DictionaryAttr();
    auto source = use ? use.getAs<DictionaryAttr>("value") : DictionaryAttr();
    auto target = use ? use.getAs<DictionaryAttr>("target") : DictionaryAttr();
    auto state =
        target ? target.getAs<DictionaryAttr>("state") : DictionaryAttr();
    auto role = id ? id.getAs<StringAttr>("role") : StringAttr();
    auto slot =
        detail::decodeU32(id ? id.getAs<IntegerAttr>("slot") : IntegerAttr(),
                          "generic final UseID slot", emit);
    if (!use || use.size() != 3 || failed(detail::verifyUseID(id, emit)) ||
        !ids.insert(id).second || failed(detail::verifyValueID(source, emit)) ||
        !target || target.size() != 2 ||
        target.getAs<StringAttr>("kind") !=
            StringAttr::get(rule.getContext(), "next_scalar") ||
        failed(detail::verifyStateRef(state, emit)) || !role ||
        role.getValue() != "next" || failed(slot) || *slot != 0)
      return emit() << "generic final RequiredUse is malformed";
    std::optional<size_t> index;
    for (auto [candidate, output] : llvm::enumerate(outputs))
      if (output == state) {
        if (index)
          return emit() << "generic final target is not unique";
        index = candidate;
      }
    if (!index || failed(actualTarget(rule, state, *index)))
      return failure();
    result.push_back(
        {id, source, state, *index, cast<DictionaryAttr>(types[*index])});
  }
  return result;
}

ValueBindingOp bindingFor(Block &body, DictionaryAttr id) {
  ValueBindingOp found;
  for (ValueBindingOp binding : body.getOps<ValueBindingOp>())
    if (binding.getIdAttr() == id) {
      if (found)
        return {};
      found = binding;
    }
  return found;
}

LogicalResult useProvenance(ValueUseOp use, const Required &required,
                            RuleOp rule) {
  auto emit = [&] { return use.emitOpError(); };
  auto unit = rule->getParentOfType<mlir::ModuleOp>();
  auto module = rule->getParentOfType<ModuleOp>();
  auto unitOwner = unit ? unit->getAttrOfType<DictionaryAttr>("ac.source_owner")
                        : DictionaryAttr();
  auto moduleOwner =
      module ? module->getAttrOfType<DictionaryAttr>("ac.source_owner")
             : DictionaryAttr();
  auto ownerPath =
      unitOwner ? unitOwner.getAs<StringAttr>("path") : StringAttr();
  auto location = dyn_cast<FileLineColLoc>(use.getLoc());
  auto origin = required.id.getAs<DictionaryAttr>("origin");
  auto site = origin ? origin.getAs<DictionaryAttr>("site") : DictionaryAttr();
  auto definition =
      site ? site.getAs<FlatSymbolRefAttr>("definition") : FlatSymbolRefAttr();
  auto expansion = origin ? origin.getAs<ArrayAttr>("expansion") : ArrayAttr();
  if (!unitOwner || failed(detail::verifySourceOwner(unitOwner, emit)) ||
      moduleOwner != unitOwner || !ownerPath || !location ||
      location.getFilename() != ownerPath.getValue() ||
      failed(detail::verifyOccurrence(origin, emit)) || !site || !definition ||
      !module || definition.getValue() != module.getSymName() || !expansion ||
      !expansion.empty())
    return emit()
           << "generic final ValueUse provenance is outside its source module";
  return success();
}

} // namespace

bool hasGenericFinalUses(RuleOp rule) {
  return rule && rule->hasAttr("ac.required_uses") &&
         rule->hasAttr("ac.yield_bindings") &&
         !rule->hasAttr("ac.required_numeric");
}

LogicalResult verifyGenericFinalUses(RuleOp rule) {
  auto emit = [&] { return rule.emitOpError(); };
  auto unit = rule->getParentOfType<mlir::ModuleOp>();
  auto stage =
      unit ? unit->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
  auto kind =
      unit ? unit->getAttrOfType<StringAttr>("ac.unit_kind") : StringAttr();
  if (!hasGenericFinalUses(rule) || failed(proofScope(rule)) ||
      rule.getBody().getBlocks().size() != 1 || !stage ||
      stage.getValue() != "final" || !kind ||
      kind.getValue() != "implementation")
    return failure();
  Block &body = rule.getBody().front();
  if (!body.getOps<NumericProofOp>().empty() ||
      !body.getOps<SourceReadOp>().empty() ||
      !body.getOps<SourceUseOp>().empty())
    return emit() << "generic final use rejects numeric/source evidence";
  auto required = requirements(rule);
  if (failed(required))
    return failure();
  SmallVector<ValueBindingOp> bindings(body.getOps<ValueBindingOp>());
  SmallVector<ValueUseOp> uses(body.getOps<ValueUseOp>());
  DenseMap<Attribute, ValueBindingOp> expectedBindings;
  for (const Required &item : *required) {
    auto binding = bindingFor(body, item.source);
    if (!binding || binding->getAttrs().size() != 2 ||
        binding.getDomainAttr() != item.logical ||
        failed(physical(binding.getValue().getType(), item.logical, binding)) ||
        !safeControl(binding.getValid(), rule) ||
        !safeControl(binding.getPath(), rule))
      return emit() << "generic final value binding is missing or invalid";
    auto input = directInput(binding.getValue(), rule);
    auto constant = binding.getValue().getDefiningOp<arith::ConstantOp>();
    if ((succeeded(input) && *input != item.logical) ||
        (failed(input) && (!constant || constant->getAttrs().size() != 1 ||
                           !constant->hasAttr("value") ||
                           failed(constantInDomain(constant, item.logical)))))
      return emit() << "generic final value has no direct source authority";
    expectedBindings.try_emplace(item.source, binding);
  }
  if (bindings.size() != expectedBindings.size() ||
      uses.size() != required->size())
    return emit() << "generic final binding/use inventory has orphan entries";
  DenseMap<Attribute, ValueUseOp> actualUses;
  for (ValueUseOp use : uses)
    if (!actualUses.try_emplace(use.getIdAttr(), use).second)
      return emit() << "generic final ValueUse identity is duplicated";
  auto yield = dyn_cast_or_null<YieldOp>(body.getTerminator());
  auto yieldBindings = rule->getAttrOfType<ArrayAttr>("ac.yield_bindings");
  if (!yield || !yieldBindings ||
      yieldBindings.size() != rule.getTargets().size() ||
      yield.getValues().size() != 2 * yieldBindings.size())
    return emit() << "generic final yield binding arity is invalid";
  DenseSet<Attribute> consumed;
  for (auto [targetIndex, raw] : llvm::enumerate(yieldBindings)) {
    auto binding = dyn_cast<DictionaryAttr>(raw);
    auto contributions =
        binding ? binding.getAs<ArrayAttr>("contributions") : ArrayAttr();
    auto contribution = contributions && contributions.size() == 1
                            ? dyn_cast<DictionaryAttr>(contributions[0])
                            : DictionaryAttr();
    auto useID = contribution ? contribution.getAs<DictionaryAttr>("use")
                              : DictionaryAttr();
    auto dataIndex = detail::decodeU32(
        binding ? binding.getAs<IntegerAttr>("data_operand") : IntegerAttr(),
        "generic final yield data operand", emit);
    auto enableIndex = detail::decodeU32(
        binding ? binding.getAs<IntegerAttr>("enable_operand") : IntegerAttr(),
        "generic final yield enable operand", emit);
    auto requiredUse = llvm::find_if(
        *required, [&](const Required &item) { return item.id == useID; });
    if (!binding || binding.size() != 4 || failed(dataIndex) ||
        *dataIndex != 2 * targetIndex || failed(enableIndex) ||
        *enableIndex != 2 * targetIndex + 1 ||
        binding.getAs<DictionaryAttr>("target") !=
            cast<DictionaryAttr>(rule->getAttrOfType<ArrayAttr>(
                "ac.output_bindings")[targetIndex]) ||
        !contribution || contribution.size() != 2 ||
        !contribution.getAs<UnitAttr>("selection_ordinal") ||
        requiredUse == required->end() ||
        requiredUse->targetIndex != targetIndex ||
        !consumed.insert(useID).second)
      return emit() << "generic final YieldBinding is stale or redirected";
    auto use = actualUses.find(useID);
    auto valueBinding = expectedBindings.find(requiredUse->source);
    if (use == actualUses.end() || valueBinding == expectedBindings.end() ||
        use->second->getAttrs().size() != 2 ||
        use->second.getSourceAttr() != requiredUse->source ||
        use->second.getValue() != valueBinding->second.getValue() ||
        use->second.getValid() != valueBinding->second.getValid() ||
        !safeControl(use->second.getPath(), rule) ||
        yield.getValues()[2 * targetIndex] != use->second.getValue())
      return emit() << "generic final ValueUse does not reach its exact yield";
    if (failed(useProvenance(use->second, *requiredUse, rule)))
      return failure();
    auto enable =
        yield.getValues()[2 * targetIndex + 1].getDefiningOp<arith::AndIOp>();
    if (!enable || !enable->getAttrs().empty() ||
        enable.getLhs() != use->second.getValid() ||
        enable.getRhs() != use->second.getPath())
      return emit() << "generic final enable is not valid AND path";
  }
  if (consumed.size() != required->size())
    return emit() << "generic final RequiredUse is not consumed exactly once";
  if (std::distance(body.getOps<arith::AndIOp>().begin(),
                    body.getOps<arith::AndIOp>().end()) !=
      static_cast<ptrdiff_t>(rule.getTargets().size()))
    return emit() << "generic final use has an extra or missing enable AND";
  for (Operation &operation : body)
    if (!isa<arith::ConstantOp, arith::AndIOp, arith::XOrIOp, ValueBindingOp,
             ValueUseOp, SourceExpectOp, SourceObserveOp, YieldOp>(operation))
      return emit() << "generic final use contains hidden arithmetic";
    else if (operation.getNumResults() == 1 &&
             operation.getResult(0).use_empty())
      return emit() << "generic final use contains an orphan finite producer";
  return success();
}

} // namespace acir::ac
