#include "acir/Dialect/ACIR/ACIROps.h"

#include "ACIRFinalContracts.h"
#include "ACIRNumericComposition.h"
#include "ACIRNumericProof.h"
#include "ACIRSourceContracts.h"
#include "ACIRStaticEvaluation.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringSet.h"

#include <optional>

using namespace mlir;

namespace acir::ac {
namespace {

LogicalResult requireSourceUnit(Operation *operation) {
  auto file = operation->getParentOfType<mlir::ModuleOp>();
  auto stage =
      file ? file->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
  auto kind =
      file ? file->getAttrOfType<StringAttr>("ac.unit_kind") : StringAttr();
  if (!stage || stage.getValue() != "source" || !kind ||
      kind.getValue() != "implementation")
    return operation->emitOpError()
           << "source hardware op requires a source-stage implementation unit";
  return success();
}

LogicalResult requireOrigin(Operation *operation) {
  auto origin = operation->getAttrOfType<DictionaryAttr>("ac.origin");
  return detail::verifyOccurrence(origin,
                                  [&] { return operation->emitOpError(); });
}

LogicalResult checkPhysical(Type physical, DictionaryAttr logical,
                            Operation *operation) {
  auto kind = logical.getAs<StringAttr>("kind");
  if (!kind)
    return operation->emitOpError() << "logical type has no kind";
  if (kind.getValue() == "bool")
    return physical.isInteger(1)
               ? success()
               : operation->emitOpError() << "bool requires i1 payload";
  if (kind.getValue() == "integer") {
    auto storage = logical.getAs<TypeAttr>("storage");
    return storage && storage.getValue() == physical
               ? success()
               : operation->emitOpError() << "integer physical payload "
                                             "disagrees with logical storage";
  }
  if (kind.getValue() == "record") {
    auto symbol = logical.getAs<FlatSymbolRefAttr>("symbol");
    auto record = dyn_cast<StructType>(physical);
    return symbol && record && record.getName().getValue() == symbol.getValue()
               ? success()
               : operation->emitOpError()
                     << "record physical payload disagrees with nominal symbol";
  }
  return operation->emitOpError()
         << "list payload requires later source scalarization";
}

LogicalResult verifyPort(DictionaryAttr port, Operation *operation) {
  auto error = [&] { return operation->emitOpError(); };
  if (!port || port.size() != 6)
    return error() << "PortSlot requires six fields";
  auto parameter = port.getAs<StringAttr>("parameter");
  Attribute ordinal = port.get("ordinal");
  auto role = port.getAs<StringAttr>("role");
  auto logical = port.getAs<DictionaryAttr>("type");
  auto origin = port.getAs<DictionaryAttr>("origin");
  auto location = port.getAs<DictionaryAttr>("location");
  if (!parameter || !ordinal || !role || !logical || !origin || !location ||
      (role.getValue() != "current" && role.getValue() != "next") ||
      failed(detail::verifyLogicalTypeStructure(logical, error)) ||
      failed(detail::verifyOccurrence(origin, error)) ||
      failed(detail::verifySourceSpan(location, error)))
    return error() << "PortSlot field is malformed";
  if (!isa<UnitAttr>(ordinal) &&
      failed(detail::decodeU64(dyn_cast<IntegerAttr>(ordinal),
                               "PortSlot ordinal", error)))
    return failure();
  return success();
}

LogicalResult verifyInitialSpec(DictionaryAttr spec, ArrayAttr shape,
                                Operation *operation) {
  auto error = [&] { return operation->emitOpError(); };
  if (!spec)
    return error() << "ac.initial_value must be an InitialSpec";
  auto kind = spec.getAs<StringAttr>("kind");
  if (!kind)
    return error() << "InitialSpec kind must be StringAttr";
  if (kind.getValue() == "scalar") {
    if (spec.size() != 2 || !shape.empty() ||
        !spec.getAs<StaticExprAttr>("value"))
      return error()
             << "scalar InitialSpec requires scalar shape and StaticExpr";
    return success();
  }
  if (shape.size() != 1)
    return error() << "list InitialSpec requires one symbolic shape";
  if (kind.getValue() == "repeat" || kind.getValue() == "expression") {
    if (spec.size() != 2 || !spec.getAs<StaticExprAttr>("value"))
      return error() << "list InitialSpec requires one StaticExpr";
    return success();
  }
  if (kind.getValue() == "elements") {
    auto values = spec.getAs<ArrayAttr>("values");
    if (spec.size() != 2 || !values)
      return error() << "elements InitialSpec requires StaticExpr values";
    for (Attribute value : values)
      if (!isa<StaticExprAttr>(value))
        return error() << "elements InitialSpec contains a non-StaticExpr";
    return success();
  }
  return error() << "InitialSpec kind is invalid";
}

LogicalResult verifyControlPorts(DictionaryAttr controls,
                                 Operation *operation) {
  auto error = [&] { return operation->emitOpError(); };
  if (!controls || controls.size() != 2)
    return error() << "ac.control_ports requires exactly clock and reset";
  auto clock = controls.getAs<IntegerAttr>("clock");
  auto reset = controls.getAs<IntegerAttr>("reset");
  if (!clock || !reset || !clock.getType().isInteger(32) ||
      !reset.getType().isInteger(32))
    return error() << "ac.control_ports indices must be i32 attributes";
  auto clockValue = detail::decodeU64(clock, "clock control index", error);
  auto resetValue = detail::decodeU64(reset, "reset control index", error);
  if (failed(clockValue) || failed(resetValue) || *clockValue != 0 ||
      *resetValue != 1)
    return error() << "ac.control_ports requires clock=0 and reset=1";
  return success();
}

bool isModuleControl(Value value, ModuleOp module, unsigned index) {
  auto argument = dyn_cast<BlockArgument>(value);
  return argument && argument.getOwner() == &module.getBody().front() &&
         argument.getArgNumber() == index && value.getType().isInteger(1);
}

FailureOr<uint64_t> evaluatePositiveLength(StaticExprAttr expression,
                                           Operation *operation) {
  auto error = [&] { return operation->emitOpError(); };
  auto evaluated =
      detail::evaluateSourceStaticExpr(expression, operation, error);
  if (failed(evaluated))
    return failure();
  auto kind = (*evaluated).getAs<StringAttr>("kind");
  auto value = (*evaluated).getAs<MathIntAttr>("value");
  if (!kind || kind.getValue() != "integer" || !value)
    return error() << "source reg shape must evaluate to an integer";
  llvm::APSInt count(value.getCanonicalValue());
  if (count.isNegative() || count.isZero() || count.getActiveBits() > 64)
    return error() << "source reg shape must be a positive u64";
  return count.getZExtValue();
}

FailureOr<uint64_t> decodeRegElement(ArrayAttr element, ArrayAttr shape,
                                     Operation *operation) {
  auto error = [&] { return operation->emitOpError(); };
  if (!element)
    return error() << "source reg requires ac.element";
  if (shape.empty()) {
    if (!element.empty())
      return error() << "scalar source reg requires empty ac.element";
    return 0;
  }
  if (shape.size() != 1 || element.size() != 1)
    return error() << "list source reg requires one shape and element index";
  auto count =
      evaluatePositiveLength(cast<StaticExprAttr>(shape[0]), operation);
  auto index = detail::decodeU64(dyn_cast<IntegerAttr>(element[0]),
                                 "source reg element", error);
  if (failed(count) || failed(index) || *index >= *count)
    return error() << "source reg element is outside its concrete shape";
  return *index;
}

FailureOr<DictionaryAttr> evaluateInitialElement(DictionaryAttr initial,
                                                 ArrayAttr shape,
                                                 uint64_t element,
                                                 Operation *operation) {
  auto error = [&] { return operation->emitOpError(); };
  auto kind = initial.getAs<StringAttr>("kind");
  if (!kind)
    return failure();
  if (kind.getValue() == "scalar" || kind.getValue() == "repeat")
    return detail::evaluateSourceStaticExpr(
        initial.getAs<StaticExprAttr>("value"), operation, error);
  if (kind.getValue() == "elements") {
    auto values = initial.getAs<ArrayAttr>("values");
    auto count =
        evaluatePositiveLength(cast<StaticExprAttr>(shape[0]), operation);
    if (!values || failed(count) || values.size() != *count ||
        element >= values.size())
      return error()
             << "elements InitialSpec length must match concrete source shape";
    return detail::evaluateSourceStaticExpr(
        cast<StaticExprAttr>(values[element]), operation, error);
  }
  if (kind.getValue() == "expression") {
    auto list = detail::evaluateSourceStaticExpr(
        initial.getAs<StaticExprAttr>("value"), operation, error);
    if (failed(list))
      return failure();
    auto listKind = (*list).getAs<StringAttr>("kind");
    auto values = (*list).getAs<ArrayAttr>("values");
    auto count =
        evaluatePositiveLength(cast<StaticExprAttr>(shape[0]), operation);
    if (!listKind || listKind.getValue() != "list" || !values ||
        failed(count) || values.size() != *count || element >= values.size())
      return error()
             << "expression InitialSpec must produce the concrete source shape";
    return cast<DictionaryAttr>(values[element]);
  }
  return failure();
}

LogicalResult verifyRuleInputAuthority(Value handle, DictionaryAttr binding,
                                       DictionaryAttr logical, RuleOp rule) {
  auto error = [&] { return rule.emitOpError(); };
  auto module = rule->getParentOfType<ModuleOp>();
  if (!module)
    return error() << "rule input has no enclosing source module";
  auto kind = binding.getAs<StringAttr>("kind");
  if (!kind)
    return error() << "rule input StateRef has no kind";

  if (kind.getValue() == "owned") {
    auto state = handle.getDefiningOp<RegOp>();
    auto element = binding.getAs<ArrayAttr>("element");
    auto shape =
        state ? state->getAttrOfType<ArrayAttr>("ac.shape") : ArrayAttr();
    if (!state || state->getParentOfType<ModuleOp>() != module || !element ||
        !shape || element != state->getAttrOfType<ArrayAttr>("ac.element") ||
        binding.getAs<DictionaryAttr>("declaration") !=
            state->getAttrOfType<DictionaryAttr>("ac.declaration") ||
        logical != state->getAttrOfType<DictionaryAttr>("ac.logical_element"))
      return error() << "owned rule input does not match its source reg";
    return success();
  }

  auto formal = dyn_cast<BlockArgument>(handle);
  if (!formal || formal.getOwner() != &module.getBody().front())
    return error() << "formal rule input is not an enclosing module port";
  auto ports = module->getAttrOfType<ArrayAttr>("ac.ports");
  if (!ports || formal.getArgNumber() < 2 ||
      formal.getArgNumber() - 2 >= ports.size())
    return error() << "formal rule input has no matching PortSlot";
  auto port = dyn_cast<DictionaryAttr>(ports[formal.getArgNumber() - 2]);
  auto role = port ? port.getAs<StringAttr>("role") : StringAttr();
  if (!port || !role || role.getValue() != "current" ||
      binding.getAs<StringAttr>("parameter") !=
          port.getAs<StringAttr>("parameter") ||
      binding.get("ordinal") != port.get("ordinal") ||
      logical != port.getAs<DictionaryAttr>("type"))
    return error() << "formal rule input does not match its current PortSlot";
  return success();
}

LogicalResult verifyRuleTargetAuthority(Value handle, DictionaryAttr binding,
                                        DictionaryAttr logical, RuleOp rule) {
  auto error = [&] { return rule.emitOpError(); };
  auto module = rule->getParentOfType<ModuleOp>();
  if (!module)
    return error() << "rule target has no enclosing source module";
  auto kind = binding.getAs<StringAttr>("kind");
  if (!kind)
    return error() << "rule target StateRef has no kind";
  if (kind.getValue() == "owned") {
    auto state = handle.getDefiningOp<RegOp>();
    auto element = binding.getAs<ArrayAttr>("element");
    if (!state || state->getParentOfType<ModuleOp>() != module || !element ||
        element != state->getAttrOfType<ArrayAttr>("ac.element") ||
        binding.getAs<DictionaryAttr>("declaration") !=
            state->getAttrOfType<DictionaryAttr>("ac.declaration") ||
        logical != state->getAttrOfType<DictionaryAttr>("ac.logical_element"))
      return error() << "owned rule target does not match its source reg";
    return success();
  }
  auto formal = dyn_cast<BlockArgument>(handle);
  if (!formal || formal.getOwner() != &module.getBody().front())
    return error() << "formal rule target is not an enclosing module port";
  auto ports = module->getAttrOfType<ArrayAttr>("ac.ports");
  if (!ports || formal.getArgNumber() < 2 ||
      formal.getArgNumber() - 2 >= ports.size())
    return error() << "formal rule target has no matching PortSlot";
  auto port = dyn_cast<DictionaryAttr>(ports[formal.getArgNumber() - 2]);
  auto role = port ? port.getAs<StringAttr>("role") : StringAttr();
  if (!port || !role || role.getValue() != "next" ||
      binding.getAs<StringAttr>("parameter") !=
          port.getAs<StringAttr>("parameter") ||
      binding.get("ordinal") != port.get("ordinal") ||
      logical != port.getAs<DictionaryAttr>("type"))
    return error() << "formal rule target does not match its next PortSlot";
  return success();
}

bool reachesValueInBlock(Value source, Value target, Block *block) {
  llvm::SmallVector<Value> pending{source};
  llvm::DenseSet<Value> visited;
  while (!pending.empty()) {
    Value value = pending.pop_back_val();
    if (value == target)
      return true;
    if (!visited.insert(value).second)
      continue;
    for (Operation *user : value.getUsers()) {
      if (user->getBlock() != block)
        continue;
      pending.append(user->getResults().begin(), user->getResults().end());
    }
  }
  return false;
}

FlatSymbolRefAttr occurrenceDefinition(DictionaryAttr occurrence) {
  auto site =
      occurrence ? occurrence.getAs<DictionaryAttr>("site") : DictionaryAttr();
  return site ? site.getAs<FlatSymbolRefAttr>("definition")
              : FlatSymbolRefAttr();
}

LogicalResult verifyObservationItems(ArrayAttr items, size_t valueCount,
                                     Operation *operation) {
  auto error = [&] { return operation->emitOpError(); };
  if (!items)
    return error() << "observation items must be an ArrayAttr";
  uint64_t expectedOrdinal = 0;
  for (Attribute raw : items) {
    auto item = dyn_cast<DictionaryAttr>(raw);
    auto kind = item ? item.getAs<StringAttr>("kind") : StringAttr();
    if (!item || item.size() != 2 || !kind)
      return error() << "observation item must be a closed dictionary";
    if (kind.getValue() == "literal") {
      if (!item.getAs<StringAttr>("text"))
        return error() << "literal observation item requires StringAttr text";
      continue;
    }
    if (kind.getValue() != "value")
      return error() << "observation item kind must be literal or value";
    auto ordinal = detail::decodeU32(item.getAs<IntegerAttr>("ordinal"),
                                     "observation value ordinal", error);
    if (failed(ordinal) || *ordinal != expectedOrdinal)
      return error()
             << "observation value ordinals must exactly cover values in order";
    ++expectedOrdinal;
  }
  if (expectedOrdinal != valueCount)
    return error()
           << "observation value items must exactly cover observed values";
  return success();
}

LogicalResult verifyObservationSpec(StringRef kind, DictionaryAttr spec,
                                    size_t valueCount,
                                    ArrayAttr valueConstraints,
                                    Operation *operation) {
  auto error = [&] { return operation->emitOpError(); };
  if (!spec)
    return error() << "source observation requires a closed spec";
  if (kind == "print") {
    if (spec.size() != 3 || !spec.getAs<StringAttr>("sep") ||
        !spec.getAs<StringAttr>("end") ||
        failed(verifyObservationItems(spec.getAs<ArrayAttr>("items"),
                                      valueCount, operation)))
      return error() << "print observation spec is malformed";
    return success();
  }
  if (kind == "log") {
    auto level = spec.getAs<StringAttr>("level");
    auto event = spec.getAs<StringAttr>("event");
    if (spec.size() != 3 || !level || !event || event.getValue().empty() ||
        !llvm::is_contained(
            ArrayRef<StringRef>{"debug", "info", "warning", "error"},
            level.getValue()) ||
        failed(verifyObservationItems(spec.getAs<ArrayAttr>("items"),
                                      valueCount, operation)))
      return error() << "log observation spec is malformed";
    return success();
  }
  if (kind != "report" || spec.size() != 1 || valueCount != 1 ||
      !spec.getAs<StringAttr>("name") ||
      spec.getAs<StringAttr>("name").getValue().empty() || !valueConstraints ||
      valueConstraints.size() != 1)
    return error() << "report observation spec is malformed";
  auto constraint = dyn_cast<DictionaryAttr>(valueConstraints[0]);
  auto logical =
      constraint ? constraint.getAs<DictionaryAttr>("type") : DictionaryAttr();
  auto logicalKind = logical ? logical.getAs<StringAttr>("kind") : StringAttr();
  auto lower = logical ? logical.getAs<MathIntAttr>("lower") : MathIntAttr();
  auto storage = logical ? logical.getAs<TypeAttr>("storage") : TypeAttr();
  auto integer =
      storage ? dyn_cast<IntegerType>(storage.getValue()) : IntegerType();
  if (!constraint || !logicalKind || logicalKind.getValue() != "integer" ||
      !lower || !integer || integer.getWidth() > 64 ||
      llvm::APSInt(lower.getCanonicalValue()).isNegative())
    return error()
           << "report observation requires one nonnegative u64-capable integer";
  return success();
}

bool isDirectRuleBoolRead(Value value, RuleOp rule) {
  auto read = value.getDefiningOp<SourceReadOp>();
  auto current =
      read ? dyn_cast<BlockArgument>(read.getCurrent()) : BlockArgument();
  auto inputTypes = rule->getAttrOfType<ArrayAttr>("ac.input_types");
  auto logical =
      current && inputTypes && current.getArgNumber() < inputTypes.size()
          ? dyn_cast<DictionaryAttr>(inputTypes[current.getArgNumber()])
          : DictionaryAttr();
  auto kind = logical ? logical.getAs<StringAttr>("kind") : StringAttr();
  auto storage = logical ? logical.getAs<TypeAttr>("storage") : TypeAttr();
  return read && read->getBlock() == &rule.getBody().front() && current &&
         current.getOwner() == &rule.getBody().front() && logical && kind &&
         kind.getValue() == "bool" && storage &&
         storage.getValue().isInteger(1) && value.getType().isInteger(1);
}

bool isTrueI1Constant(Value value) {
  auto constant = value.getDefiningOp<arith::ConstantOp>();
  if (!constant || !value.getType().isInteger(1))
    return false;
  if (auto boolean = dyn_cast<BoolAttr>(constant.getValue()))
    return boolean.getValue();
  auto integer = dyn_cast<IntegerAttr>(constant.getValue());
  return integer && integer.getValue().isOne();
}

LogicalResult verifySourceCheckPath(Value path, Value condition, RuleOp rule,
                                    Operation *operation) {
  if (isTrueI1Constant(path))
    return success();
  if (path == condition)
    return operation->emitOpError()
           << "source check path cannot redirect its condition";
  if (isDirectRuleBoolRead(path, rule))
    return success();
  auto inverted = path.getDefiningOp<arith::XOrIOp>();
  if (!inverted || inverted->getBlock() != &rule.getBody().front())
    return operation->emitOpError()
           << "source check path must be true or a direct bool SourceRead";
  Value source;
  if (isTrueI1Constant(inverted.getLhs()))
    source = inverted.getRhs();
  else if (isTrueI1Constant(inverted.getRhs()))
    source = inverted.getLhs();
  if (!source || source == condition || !isDirectRuleBoolRead(source, rule))
    return operation->emitOpError()
           << "source check inverted path must negate one direct bool "
              "SourceRead";
  return success();
}

LogicalResult verifyRangeTemplate(SourceExpectOp expect, MathToBitsOp toBits,
                                  RuleOp rule) {
  auto error = [&] { return expect.emitOpError(); };
  auto id = expect->getAttrOfType<DictionaryAttr>("ac.check_id");
  auto occurrence = id ? id.getAs<DictionaryAttr>("check") : DictionaryAttr();
  auto obligation = id ? id.getAs<IntegerAttr>("obligation") : IntegerAttr();
  auto check = toBits->getAttrOfType<DictionaryAttr>("ac.check_template");
  auto required = rule->getAttrOfType<ArrayAttr>("ac.required_numeric");
  auto final =
      required && (required.size() == 2 || required.size() == 6)
          ? dyn_cast<DictionaryAttr>(required[required.size() == 2 ? 1 : 5])
          : DictionaryAttr();
  auto resultID = final ? final.getAs<DictionaryAttr>("id") : DictionaryAttr();
  auto operation = final ? final.getAs<StringAttr>("operator") : StringAttr();
  auto target =
      final ? final.getAs<DictionaryAttr>("target") : DictionaryAttr();
  auto targetKind = target ? target.getAs<StringAttr>("kind") : StringAttr();
  if (!check || check.size() != 4 || !occurrence || !obligation ||
      check.getAs<DictionaryAttr>("leaf") !=
          occurrence.getAs<DictionaryAttr>("site") ||
      check.getAs<StringAttr>("kind") !=
          StringAttr::get(expect.getContext(), "range") ||
      check.getAs<IntegerAttr>("obligation") != obligation ||
      check.getAs<DictionaryAttr>("location") !=
          expect->getAttrOfType<DictionaryAttr>("location") ||
      !final || final.size() != 4 || !resultID || !operation ||
      operation.getValue() != "to_bits" || !target || target.size() != 2 ||
      !targetKind || targetKind.getValue() != "integer_boundary" ||
      target.getAs<DictionaryAttr>("domain") != toBits.getDomainAttr() ||
      resultID.getAs<DictionaryAttr>("origin") !=
          toBits->getAttrOfType<DictionaryAttr>("ac.origin"))
    return error() << "range check is not tied to its to_bits template/ValueID";
  if (expect.getCondition() != toBits.getValid() ||
      expect.getPath() != toBits.getPath())
    return error() << "source range check must use to_bits valid/path";
  return success();
}

LogicalResult verifyLoweredRangeSeam(SourceExpectOp expect,
                                     NumericProofOp proof, RuleOp rule) {
  auto error = [&] { return expect.emitOpError(); };
  auto checks = proof.getChecksAttr();
  auto binding = checks && checks.size() == 1
                     ? dyn_cast<DictionaryAttr>(checks[0])
                     : DictionaryAttr();
  auto id = binding ? binding.getAs<DictionaryAttr>("id") : DictionaryAttr();
  auto owner =
      binding ? binding.getAs<DictionaryAttr>("owner") : DictionaryAttr();
  auto kind = binding ? binding.getAs<StringAttr>("kind") : StringAttr();
  auto operand =
      binding ? binding.getAs<IntegerAttr>("operand_ordinal") : IntegerAttr();
  auto ordinal = detail::decodeU32(operand, "range check operand", error);
  if (!binding || binding.size() != 4 ||
      id != expect->getAttrOfType<DictionaryAttr>("ac.check_id") ||
      owner != proof.getResultIdAttr() || !kind || kind.getValue() != "range" ||
      failed(ordinal) || *ordinal != 0 || proof->getNumOperands() != 7 ||
      expect.getCondition() != proof->getOperand(5) ||
      expect.getPath() != proof->getOperand(6))
    return error() << "lowered range check is outside its numeric proof seam";
  if (proof->getParentOfType<RuleOp>() != rule)
    return error() << "lowered range proof belongs to another rule";
  return success();
}

LogicalResult verifyRangeSourceExpect(SourceExpectOp expect, RuleOp rule) {
  SmallVector<MathToBitsOp> conversions(
      rule.getBody().front().getOps<MathToBitsOp>());
  SmallVector<NumericProofOp> proofs(
      rule.getBody().front().getOps<NumericProofOp>());
  if (conversions.size() == 1 && proofs.empty())
    return verifyRangeTemplate(expect, conversions.front(), rule);
  if (conversions.empty()) {
    SmallVector<NumericProofOp> checked;
    llvm::copy_if(
        proofs, std::back_inserter(checked), [](NumericProofOp proof) {
          return proof.getChecksAttr() && !proof.getChecksAttr().empty();
        });
    if (checked.size() == 1)
      return verifyLoweredRangeSeam(expect, checked.front(), rule);
  }
  return expect.emitOpError()
         << "range check requires exactly one source to_bits or lowered proof";
}

} // namespace

LogicalResult SourceReadOp::verify() {
  auto error = [&] { return emitOpError(); };
  if (failed(requireSourceUnit(*this)))
    return failure();
  if (failed(detail::verifyOccurrence(
          (*this)->getAttrOfType<DictionaryAttr>("ac.origin"), error)))
    return failure();
  auto rule = (*this)->getParentOfType<RuleOp>();
  if (!rule || rule.getBody().getBlocks().size() != 1 ||
      (*this)->getBlock() != &rule.getBody().front() ||
      failed(detail::verifyOccurrence(rule.getRegistrationAttr(), error)))
    return error() << "source read requires one enclosing registered rule body";
  auto current = dyn_cast<BlockArgument>(getCurrent());
  if (!current || current.getOwner() != &rule.getBody().front() ||
      current.getArgNumber() >= rule.getInputs().size())
    return error() << "source read must consume a rule entry current payload";
  auto handle =
      dyn_cast<RegType>(rule.getInputs()[current.getArgNumber()].getType());
  if (!handle || handle.getElementType() != getCurrent().getType() ||
      getResult().getType() != getCurrent().getType())
    return error()
           << "source read current and result types must match its rule input";
  return success();
}

LogicalResult SourceUseOp::verify() {
  auto error = [&] { return emitOpError(); };
  if (failed(requireSourceUnit(*this)))
    return failure();
  auto rule = (*this)->getParentOfType<RuleOp>();
  if (!rule || rule.getBody().getBlocks().size() != 1 ||
      (*this)->getBlock() != &rule.getBody().front() ||
      failed(detail::verifyOccurrence(rule.getRegistrationAttr(), error)))
    return error() << "source use requires one enclosing registered rule body";

  auto id = (*this)->getAttrOfType<DictionaryAttr>("id");
  auto source = (*this)->getAttrOfType<DictionaryAttr>("source");
  auto target = (*this)->getAttrOfType<DictionaryAttr>("target");
  if (failed(detail::verifyUseID(id, error)) ||
      failed(detail::verifyValueID(source, error)))
    return failure();
  auto role = id.getAs<StringAttr>("role");
  if (role.getValue() != "next")
    return error() << "source use role is structurally valid but unsupported "
                      "in this context";
  auto useSlot = detail::decodeU32(id.getAs<IntegerAttr>("slot"),
                                   "next SourceUse UseID slot", error);
  if (failed(useSlot) || *useSlot != 0)
    return error() << "next SourceUse UseID slot must be u32 0";

  if (!target || target.size() != 2)
    return error()
           << "next SourceUseTarget must contain exactly kind and state";
  auto kind = target.getAs<StringAttr>("kind");
  auto state = target.getAs<DictionaryAttr>("state");
  if (!kind || kind.getValue() != "next_scalar" ||
      failed(detail::verifyStateRef(state, error)))
    return error() << "source use requires a closed next_scalar target";

  auto bindings = rule->getAttrOfType<ArrayAttr>("ac.output_bindings");
  auto logicalTypes = rule->getAttrOfType<ArrayAttr>("ac.output_types");
  if (!bindings || !logicalTypes ||
      bindings.size() != rule.getTargets().size() ||
      logicalTypes.size() != rule.getTargets().size())
    return error() << "source use requires closed rule target metadata";
  std::optional<size_t> targetIndex;
  for (auto [index, binding] : llvm::enumerate(bindings)) {
    if (binding != state)
      continue;
    if (targetIndex)
      return error() << "next_scalar target must match one unique rule target";
    targetIndex = index;
  }
  if (!targetIndex)
    return error() << "next_scalar target does not match a rule target";
  auto handle = dyn_cast<RegType>(rule.getTargets()[*targetIndex].getType());
  auto logical = dyn_cast<DictionaryAttr>(logicalTypes[*targetIndex]);
  if (!handle || !logical ||
      failed(detail::verifyLogicalTypeStructure(logical, error)) ||
      failed(checkPhysical(handle.getElementType(), logical, *this)) ||
      getValue().getType() != handle.getElementType() ||
      getData().getType() != handle.getElementType())
    return error() << "source use value/result type does not match its target";

  if (auto read = getValue().getDefiningOp<SourceReadOp>()) {
    auto slot = detail::decodeU32(source.getAs<IntegerAttr>("slot"),
                                  "SourceRead ValueID slot", error);
    if (failed(slot) || *slot != 0 ||
        source.getAs<DictionaryAttr>("origin") !=
            read->getAttrOfType<DictionaryAttr>("ac.origin"))
      return error()
             << "direct source read ValueID does not match its producer";
    auto current = dyn_cast<BlockArgument>(read.getCurrent());
    auto inputTypes = rule->getAttrOfType<ArrayAttr>("ac.input_types");
    if (!current || current.getOwner() != &rule.getBody().front() ||
        !inputTypes || current.getArgNumber() >= inputTypes.size() ||
        inputTypes[current.getArgNumber()] != logical)
      return error() << "direct source read logical type does not match its "
                        "next_scalar target";
  }

  auto yield =
      dyn_cast_or_null<YieldOp>(rule.getBody().front().getTerminator());
  size_t dataIndex = 2 * *targetIndex;
  if (!yield || yield.getValues().size() <= dataIndex + 1 ||
      !reachesValueInBlock(getData(), yield.getValues()[dataIndex],
                           &rule.getBody().front()) ||
      !reachesValueInBlock(getEnabled(), yield.getValues()[dataIndex + 1],
                           &rule.getBody().front()))
    return error() << "source use data and enabled must contribute to their "
                      "target yield pair";
  for (size_t index = 0; index < rule.getTargets().size(); ++index) {
    if (index == *targetIndex)
      continue;
    Value otherData = yield.getValues()[2 * index];
    Value otherEnabled = yield.getValues()[2 * index + 1];
    if (reachesValueInBlock(getData(), otherData, &rule.getBody().front()) ||
        reachesValueInBlock(getData(), otherEnabled, &rule.getBody().front()) ||
        reachesValueInBlock(getEnabled(), otherData, &rule.getBody().front()) ||
        reachesValueInBlock(getEnabled(), otherEnabled,
                            &rule.getBody().front()))
      return error()
             << "source use data and enabled must not contribute to another "
                "target yield pair";
  }
  return success();
}

LogicalResult SourceObserveOp::verify() {
  if (isFinalImplementation(*this))
    return verifyFinalObserve(*this);
  auto error = [&] { return emitOpError(); };
  if (failed(requireSourceUnit(*this)))
    return failure();
  auto rule = (*this)->getParentOfType<RuleOp>();
  if (!rule || rule.getBody().getBlocks().size() != 1 ||
      (*this)->getBlock() != &rule.getBody().front())
    return error()
           << "source observation requires one direct registered rule block";

  auto identity = (*this)->getAttrOfType<DictionaryAttr>("ac.observation_id");
  auto registration = identity ? identity.getAs<DictionaryAttr>("registration")
                               : DictionaryAttr();
  auto site =
      identity ? identity.getAs<DictionaryAttr>("site") : DictionaryAttr();
  auto module = (*this)->getParentOfType<ModuleOp>();
  auto definition =
      module ? FlatSymbolRefAttr::get(getContext(), module.getSymName())
             : FlatSymbolRefAttr();
  if (!identity || identity.size() != 2 ||
      failed(detail::verifyOccurrence(registration, error)) ||
      failed(detail::verifyOccurrence(site, error)) ||
      registration != rule.getRegistrationAttr() ||
      occurrenceDefinition(site) != definition)
    return error() << "source observation identity is outside its rule scope";

  auto valueIDs = (*this)->getAttrOfType<ArrayAttr>("ac.value_ids");
  auto constraints = (*this)->getAttrOfType<ArrayAttr>("ac.value_constraints");
  if (!valueIDs || !constraints || valueIDs.size() != getValues().size() ||
      constraints.size() != getValues().size())
    return error()
           << "source observation values require identities and constraints";
  auto inputTypes = rule->getAttrOfType<ArrayAttr>("ac.input_types");
  bool composition = hasNumericCompositionContract(rule);
  for (auto [index, value] : llvm::enumerate(getValues())) {
    auto valueID = dyn_cast<DictionaryAttr>(valueIDs[index]);
    auto constraint = dyn_cast<DictionaryAttr>(constraints[index]);
    auto constraintKind =
        constraint ? constraint.getAs<StringAttr>("kind") : StringAttr();
    auto logical = constraint ? constraint.getAs<DictionaryAttr>("type")
                              : DictionaryAttr();
    auto logicalKind =
        logical ? logical.getAs<StringAttr>("kind") : StringAttr();
    auto read = value.getDefiningOp<SourceReadOp>();
    auto constant = value.getDefiningOp<arith::ConstantOp>();
    auto current =
        read ? dyn_cast<BlockArgument>(read.getCurrent()) : BlockArgument();
    auto slot = valueID ? detail::decodeU32(valueID.getAs<IntegerAttr>("slot"),
                                            "observation ValueID slot", error)
                        : FailureOr<uint32_t>(failure());
    bool directConstant =
        composition && constant && valueID &&
        valueID.getAs<DictionaryAttr>("origin") ==
            constant->getAttrOfType<DictionaryAttr>("ac.origin") &&
        succeeded(checkPhysical(value.getType(), logical, *this));
    if (!valueID || failed(detail::verifyValueID(valueID, error)) ||
        failed(slot) || *slot != 0 || !constraint || constraint.size() != 2 ||
        !constraintKind || constraintKind.getValue() != "logical" || !logical ||
        failed(detail::verifyLogicalTypeStructure(logical, error)) ||
        !logicalKind ||
        (logicalKind.getValue() != "bool" &&
         logicalKind.getValue() != "integer" &&
         logicalKind.getValue() != "record") ||
        failed(checkPhysical(value.getType(), logical, *this)) ||
        (!directConstant &&
         (!read || read->getBlock() != &rule.getBody().front() || !current ||
          current.getOwner() != &rule.getBody().front() || !inputTypes ||
          current.getArgNumber() >= inputTypes.size() ||
          inputTypes[current.getArgNumber()] != logical ||
          valueID.getAs<DictionaryAttr>("origin") !=
              read->getAttrOfType<DictionaryAttr>("ac.origin"))))
      return error()
             << "source observation value is not an exact SourceRead binding";
  }
  if (llvm::is_contained(getValues(), getPath()))
    return error()
           << "source observation path cannot redirect an observed value";
  if (composition && failed(verifyNumericCompositionObserve(*this)))
    return failure();
  StringRef kind = getKind();
  if (kind != "print" && kind != "log" && kind != "report")
    return error() << "source observation kind must be print, log or report";
  return verifyObservationSpec(kind, getSpec(), getValues().size(), constraints,
                               *this);
}

LogicalResult SourceExpectOp::verify() {
  if (isFinalImplementation(*this))
    return verifyFinalExpect(*this);
  auto error = [&] { return emitOpError(); };
  if (failed(requireSourceUnit(*this)))
    return failure();
  auto rule = (*this)->getParentOfType<RuleOp>();
  if (!rule || rule.getBody().getBlocks().size() != 1 ||
      (*this)->getBlock() != &rule.getBody().front())
    return error() << "source check requires one direct registered rule block";
  auto id = (*this)->getAttrOfType<DictionaryAttr>("ac.check_id");
  auto registration =
      id ? id.getAs<DictionaryAttr>("registration") : DictionaryAttr();
  auto check = id ? id.getAs<DictionaryAttr>("check") : DictionaryAttr();
  auto module = (*this)->getParentOfType<ModuleOp>();
  auto definition =
      module ? FlatSymbolRefAttr::get(getContext(), module.getSymName())
             : FlatSymbolRefAttr();
  if (!id || failed(detail::verifyCheckID(id, error)) ||
      registration != rule.getRegistrationAttr() ||
      occurrenceDefinition(check) != definition)
    return error() << "source check identity is outside its rule scope";
  auto location = (*this)->getAttrOfType<DictionaryAttr>("location");
  auto owner = module ? module->getAttrOfType<DictionaryAttr>("ac.source_owner")
                      : DictionaryAttr();
  auto ownerPath = owner ? owner.getAs<StringAttr>("path") : StringAttr();
  auto locationPath =
      location ? location.getAs<StringAttr>("path") : StringAttr();
  if (failed(detail::verifySourceSpan(location, error)) || !ownerPath ||
      !locationPath || ownerPath != locationPath)
    return error()
           << "source check location must belong to its module source owner";
  if (hasNumericCompositionContract(rule))
    return verifyNumericCompositionExpect(*this);
  if (getKind() == "range")
    return verifyRangeSourceExpect(*this, rule);
  if (getKind() != "assert")
    return error() << "source check kind must be assert or range";
  if (!isDirectRuleBoolRead(getCondition(), rule))
    return error()
           << "source check condition must be an exact direct bool SourceRead";
  if (failed(verifySourceCheckPath(getPath(), getCondition(), rule, *this)))
    return failure();
  return success();
}

LogicalResult RegOp::verify() {
  if (isFinalImplementation(*this))
    return verifyFinalReg(*this);
  auto error = [&] { return emitOpError(); };
  if (failed(requireSourceUnit(*this)))
    return failure();
  auto owner = (*this)->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto declaration = (*this)->getAttrOfType<DictionaryAttr>("ac.declaration");
  auto logical = (*this)->getAttrOfType<DictionaryAttr>("ac.logical_element");
  auto shape = (*this)->getAttrOfType<ArrayAttr>("ac.shape");
  auto element = (*this)->getAttrOfType<ArrayAttr>("ac.element");
  auto initial = (*this)->getAttrOfType<DictionaryAttr>("ac.initial_value");
  auto domain = (*this)->getAttrOfType<StringAttr>("ac.domain");
  if (!getNameAttr() || getName().empty() ||
      failed(detail::verifySourceOwner(owner, error)) ||
      failed(detail::verifyOccurrence(declaration, error)) || !logical ||
      failed(detail::verifyLogicalTypeStructure(logical, error)) || !shape ||
      !element || !initial || !domain || domain.getValue() != "default")
    return error() << "source reg state contract is incomplete";
  auto module = (*this)->getParentOfType<ModuleOp>();
  if (!module ||
      owner != module->getAttrOfType<DictionaryAttr>("ac.source_owner"))
    return error() << "reg state must belong to its enclosing source module";
  if (!isModuleControl(getClock(), module, 0) ||
      !isModuleControl(getReset(), module, 1))
    return error()
           << "source reg clock/reset must be enclosing module controls";
  if (failed(checkPhysical(cast<RegType>(getState().getType()).getElementType(),
                           logical, *this)))
    return failure();
  if (shape.size() > 1)
    return error() << "source reg shape supports scalar or fixed list";
  for (Attribute raw : shape)
    if (!isa<StaticExprAttr>(raw))
      return error() << "source reg shape must contain StaticExpr";
  if (failed(verifyInitialSpec(initial, shape, *this)))
    return failure();
  auto elementIndex = decodeRegElement(element, shape, *this);
  if (failed(elementIndex))
    return failure();
  auto evaluated = evaluateInitialElement(initial, shape, *elementIndex, *this);
  if (failed(evaluated))
    return failure();
  auto resolver = [&](FlatSymbolRefAttr symbol) {
    return detail::resolveSourceRecord(symbol, *this, error);
  };
  if (failed(detail::verifyStaticValueMatchesType(
          *evaluated, logical, detail::ExpectedTypeKind::Logical, resolver,
          error)))
    return error() << "source reg reset value does not match its logical type";
  return success();
}

LogicalResult ModuleOp::verify() {
  if (isFinalImplementation(*this))
    return verifyFinalModule(*this);
  auto error = [&] { return emitOpError(); };
  if (failed(requireSourceUnit(*this)))
    return failure();
  auto owner = (*this)->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto origin = (*this)->getAttrOfType<DictionaryAttr>("ac.origin");
  auto ports = (*this)->getAttrOfType<ArrayAttr>("ac.ports");
  auto controls = (*this)->getAttrOfType<DictionaryAttr>("ac.control_ports");
  if (getName().empty() || getSymName().empty() ||
      failed(detail::verifySourceOwner(owner, error)) ||
      failed(detail::verifyOccurrence(origin, error)) || !ports ||
      failed(verifyControlPorts(controls, *this)))
    return error() << "source module declaration metadata is incomplete";
  if (auto rootKind = (*this)->getAttrOfType<StringAttr>("ac.root_kind");
      rootKind && rootKind.getValue() != "system")
    return error() << "ac.root_kind must be system when present";
  bool seenNext = false;
  llvm::DenseSet<Attribute> seen;
  for (Attribute raw : ports) {
    auto port = dyn_cast<DictionaryAttr>(raw);
    if (failed(verifyPort(port, *this)))
      return failure();
    StringRef role = port.getAs<StringAttr>("role").getValue();
    if (role == "next")
      seenNext = true;
    else if (seenNext)
      return error() << "source module ports require current before next";
    auto key = DictionaryAttr::get(
        getContext(),
        {NamedAttribute(StringAttr::get(getContext(), "parameter"),
                        port.get("parameter")),
         NamedAttribute(StringAttr::get(getContext(), "ordinal"),
                        port.get("ordinal")),
         NamedAttribute(StringAttr::get(getContext(), "role"),
                        port.get("role"))});
    if (!seen.insert(key).second)
      return error() << "source module has repeated PortSlot identity";
  }
  return success();
}

LogicalResult ModuleOp::verifyRegions() {
  if (isFinalImplementation(*this))
    return verifyFinalModuleRegion(*this);
  auto error = [&] { return emitOpError(); };
  if (failed(requireSourceUnit(*this)))
    return failure();
  if (getBody().getBlocks().size() != 1)
    return error() << "source module body requires one block";
  auto ports = (*this)->getAttrOfType<ArrayAttr>("ac.ports");
  Block &body = getBody().front();
  if (!ports || body.getNumArguments() != ports.size() + 2)
    return error() << "module body requires clock,reset followed by ac.ports";
  if (!body.getArgument(0).getType().isInteger(1) ||
      !body.getArgument(1).getType().isInteger(1))
    return error() << "module clock/reset controls must be i1";
  for (auto [index, raw] : llvm::enumerate(ports)) {
    auto port = cast<DictionaryAttr>(raw);
    auto handle = dyn_cast<RegType>(body.getArgument(index + 2).getType());
    if (!handle ||
        failed(checkPhysical(handle.getElementType(),
                             port.getAs<DictionaryAttr>("type"), *this)))
      return error() << "module port handle payload mismatch";
  }
  auto terminator = dyn_cast_or_null<YieldOp>(body.getTerminator());
  if (!terminator)
    return error() << "module body must end in ac.yield";
  if (!terminator.getValues().empty())
    return error() << "source module yields no final state values";
  llvm::StringSet<> stateNames;
  struct StateGroup {
    ArrayAttr shape;
    DictionaryAttr logical;
    DictionaryAttr initial;
    uint64_t expected = 1;
    llvm::DenseSet<uint64_t> elements;
  };
  llvm::DenseMap<Attribute, StateGroup> groups;
  for (RegOp state : body.getOps<RegOp>()) {
    if (!stateNames.insert(state.getName()).second)
      return error() << "source module declares one state name more than once";
    auto declaration = state->getAttrOfType<DictionaryAttr>("ac.declaration");
    auto shape = state->getAttrOfType<ArrayAttr>("ac.shape");
    auto element = state->getAttrOfType<ArrayAttr>("ac.element");
    auto elementIndex = decodeRegElement(element, shape, state);
    if (failed(elementIndex))
      return failure();
    uint64_t expected = 1;
    if (!shape.empty()) {
      auto count =
          evaluatePositiveLength(cast<StaticExprAttr>(shape[0]), state);
      if (failed(count))
        return failure();
      expected = *count;
    }
    auto [position, inserted] = groups.try_emplace(declaration);
    StateGroup &group = position->second;
    auto logical = state->getAttrOfType<DictionaryAttr>("ac.logical_element");
    auto initial = state->getAttrOfType<DictionaryAttr>("ac.initial_value");
    if (inserted) {
      group.shape = shape;
      group.logical = logical;
      group.initial = initial;
      group.expected = expected;
    } else if (group.shape != shape || group.logical != logical ||
               group.initial != initial || group.expected != expected) {
      return error()
             << "source reg elements disagree on declaration shape or reset";
    }
    if (!group.elements.insert(*elementIndex).second)
      return error() << "source reg declaration repeats an element";
  }
  for (const auto &[declaration, group] : groups) {
    if (group.elements.size() != group.expected)
      return error() << "source reg declaration has missing concrete elements";
    for (uint64_t index = 0; index < group.expected; ++index)
      if (!group.elements.contains(index))
        return error() << "source reg declaration elements are not a closed "
                          "ordinal set";
  }
  return success();
}

LogicalResult RuleOp::verify() {
  if (isFinalImplementation(*this))
    return verifyFinalRule(*this);
  auto error = [&] { return emitOpError(); };
  if (failed(requireSourceUnit(*this)))
    return failure();
  auto owner = (*this)->getAttrOfType<DictionaryAttr>("ac.source_owner");
  if (failed(detail::verifySourceOwner(owner, error)))
    return failure();
  auto module = (*this)->getParentOfType<ModuleOp>();
  if (!module ||
      owner != module->getAttrOfType<DictionaryAttr>("ac.source_owner"))
    return error() << "rule must belong to its enclosing source module";
  if (getName().empty() ||
      failed(detail::verifyOccurrence(getRegistrationAttr(), error)) ||
      failed(requireOrigin(*this)))
    return error() << "rule registration/body origin is incomplete";
  auto inputs = (*this)->getAttrOfType<ArrayAttr>("ac.input_bindings");
  auto targets = (*this)->getAttrOfType<ArrayAttr>("ac.output_bindings");
  auto inputTypes = (*this)->getAttrOfType<ArrayAttr>("ac.input_types");
  auto outputTypes = (*this)->getAttrOfType<ArrayAttr>("ac.output_types");
  if (!inputs || !targets || !inputTypes || !outputTypes ||
      inputs.size() != getInputs().size() ||
      targets.size() != getTargets().size() ||
      inputTypes.size() != inputs.size() ||
      outputTypes.size() != targets.size())
    return error() << "rule bindings and logical types must match handles";
  for (auto [index, raw] : llvm::enumerate(inputs)) {
    auto binding = dyn_cast<DictionaryAttr>(raw);
    auto logical = dyn_cast<DictionaryAttr>(inputTypes[index]);
    if (!binding || !logical ||
        failed(detail::verifyStateRef(binding, error)) ||
        failed(detail::verifyLogicalTypeStructure(logical, error)) ||
        failed(checkPhysical(
            cast<RegType>(getInputs()[index].getType()).getElementType(),
            logical, *this)) ||
        failed(verifyRuleInputAuthority(getInputs()[index], binding, logical,
                                        *this)))
      return error() << "rule input binding/type mismatch";
  }
  for (auto [index, raw] : llvm::enumerate(targets)) {
    auto binding = dyn_cast<DictionaryAttr>(raw);
    auto logical = dyn_cast<DictionaryAttr>(outputTypes[index]);
    if (!binding || !logical ||
        failed(detail::verifyStateRef(binding, error)) ||
        failed(detail::verifyLogicalTypeStructure(logical, error)) ||
        failed(checkPhysical(
            cast<RegType>(getTargets()[index].getType()).getElementType(),
            logical, *this)) ||
        failed(verifyRuleTargetAuthority(getTargets()[index], binding, logical,
                                         *this)))
      return error() << "rule target binding/type mismatch";
  }
  if (getNextValues().size() != 2 * getTargets().size())
    return error() << "source rule results require data,enable per target";
  return success();
}

LogicalResult RuleOp::verifyRegions() {
  if (isFinalImplementation(*this))
    return verifyFinalRuleRegion(*this);
  auto error = [&] { return emitOpError(); };
  if (failed(requireSourceUnit(*this)))
    return failure();
  if (getBody().getBlocks().size() != 1)
    return error() << "rule computation requires one block";
  Block &body = getBody().front();
  if (body.getNumArguments() != getInputs().size())
    return error() << "rule body inputs must be current payload arguments";
  for (auto [index, handle] : llvm::enumerate(getInputs()))
    if (body.getArgument(index).getType() !=
        cast<RegType>(handle.getType()).getElementType())
      return error() << "rule body input payload type mismatch";
  if (failed(verifyRuleNumericClosure(*this)))
    return failure();
  bool hasSourceFacts = llvm::any_of(body, [](Operation &operation) {
    return isa<SourceReadOp, SourceUseOp>(operation);
  });
  if (hasSourceFacts)
    for (BlockArgument argument : body.getArguments())
      for (OpOperand &use : argument.getUses())
        if (!isa<SourceReadOp>(use.getOwner()))
          return error()
                 << "rule current payload must be consumed through source.read";
  llvm::DenseSet<Attribute> sourceUseIds;
  for (SourceUseOp use : body.getOps<SourceUseOp>()) {
    auto id = use->getAttrOfType<DictionaryAttr>("id");
    if (failed(detail::verifyUseID(id, error)))
      return failure();
    if (!sourceUseIds.insert(id).second)
      return error() << "rule source use identities must be unique";
  }
  SmallVector<SourceObserveOp> observations(body.getOps<SourceObserveOp>());
  auto required = (*this)->getAttrOfType<ArrayAttr>("ac.required_observations");
  if (!observations.empty() || required) {
    if (!required || required.size() != observations.size())
      return error()
             << "rule required observations must match actual observations";
    llvm::DenseSet<Attribute> sites;
    for (auto [index, observe] : llvm::enumerate(observations)) {
      auto identity =
          observe->getAttrOfType<DictionaryAttr>("ac.observation_id");
      auto site =
          identity ? identity.getAs<DictionaryAttr>("site") : DictionaryAttr();
      auto expected = dyn_cast<DictionaryAttr>(required[index]);
      if (!identity || !site || !sites.insert(site).second || !expected ||
          expected.size() != 4 ||
          expected.getAs<DictionaryAttr>("id") != identity ||
          expected.getAs<StringAttr>("kind") != observe.getKindAttr() ||
          expected.getAs<DictionaryAttr>("spec") != observe.getSpecAttr() ||
          expected.getAs<ArrayAttr>("values") !=
              observe->getAttrOfType<ArrayAttr>("ac.value_ids"))
        return error()
               << "rule observation does not match its ordered requirement";
    }
  }
  SmallVector<SourceExpectOp> checks(body.getOps<SourceExpectOp>());
  auto requiredChecks = (*this)->getAttrOfType<ArrayAttr>("ac.required_checks");
  if (!checks.empty() || requiredChecks) {
    if (!requiredChecks || requiredChecks.size() != checks.size())
      return error() << "rule required checks must match actual source checks";
    if (!hasNumericCompositionContract(*this) &&
        llvm::any_of(
            checks,
            [](SourceExpectOp check) { return check.getKind() == "range"; }) &&
        (checks.size() != 1 || checks.front().getKind() != "range"))
      return error() << "D3 range conversion requires exactly one range check";
    llvm::DenseSet<Attribute> checkIDs;
    llvm::DenseSet<Attribute> checkSites;
    for (auto [index, check] : llvm::enumerate(checks)) {
      auto id = check->getAttrOfType<DictionaryAttr>("ac.check_id");
      auto site = id ? id.getAs<DictionaryAttr>("check") : DictionaryAttr();
      auto obligation =
          id ? detail::decodeU64(id.getAs<IntegerAttr>("obligation"),
                                 "CheckID obligation", error)
             : FailureOr<uint64_t>(failure());
      auto expected = dyn_cast<DictionaryAttr>(requiredChecks[index]);
      if (!id || failed(detail::verifyCheckID(id, error)) ||
          failed(obligation) || *obligation != index ||
          !checkIDs.insert(id).second || !site ||
          !checkSites.insert(site).second || !expected ||
          expected.size() != 3 || expected.getAs<DictionaryAttr>("id") != id ||
          expected.getAs<StringAttr>("kind") != check.getKindAttr() ||
          expected.getAs<DictionaryAttr>("location") !=
              check->getAttrOfType<DictionaryAttr>("location"))
        return error() << "rule check does not match its ordered requirement";
    }
  }
  auto terminator = dyn_cast_or_null<YieldOp>(body.getTerminator());
  if (!terminator || terminator.getValues().size() != 2 * getTargets().size())
    return error() << "rule yield must contain data,enable for each target";
  for (size_t index = 0; index < getTargets().size(); ++index) {
    Type payload =
        cast<RegType>(getTargets()[index].getType()).getElementType();
    if (terminator.getValues()[2 * index].getType() != payload ||
        !terminator.getValues()[2 * index + 1].getType().isInteger(1))
      return error() << "rule yield data/enable type mismatch";
  }
  if (getNextValues().size() != terminator.getValues().size())
    return error() << "rule results must match yielded data/enable pairs";
  for (auto [result, yielded] :
       llvm::zip(getNextValues(), terminator.getValues()))
    if (result.getType() != yielded.getType())
      return error() << "rule result type does not match yielded proposal";
  return success();
}

LogicalResult InstanceOp::verify() {
  if (isFinalImplementation(*this))
    return verifyFinalInstance(*this);
  auto error = [&] { return emitOpError(); };
  if (failed(requireSourceUnit(*this)))
    return failure();
  if (getName().empty() || failed(requireOrigin(*this)))
    return error() << "instance identity/origin is incomplete";
  auto module = (*this)->getParentOfType<ModuleOp>();
  if (!module || !isModuleControl(getClock(), module, 0) ||
      !isModuleControl(getReset(), module, 1))
    return error()
           << "instance clock/reset must identity-forward module controls";
  Attribute callee = getCalleeAttr();
  auto staticArguments = (*this)->getAttrOfType<ArrayAttr>("ac.static_args");
  if (!isa<FlatSymbolRefAttr>(callee) || !staticArguments)
    return error() << "source instance requires symbol and static arguments";
  for (Attribute argument : staticArguments)
    if (!isa<StaticExprAttr>(argument))
      return error() << "source instance static arguments must be StaticExpr";
  if (getNextValues().size() != 2 * getTargets().size())
    return error() << "source instance results require data,enable per target";
  for (size_t index = 0; index < getTargets().size(); ++index) {
    Type payload =
        cast<RegType>(getTargets()[index].getType()).getElementType();
    if (getNextValues()[2 * index].getType() != payload ||
        !getNextValues()[2 * index + 1].getType().isInteger(1))
      return error() << "instance proposal result type mismatch";
  }
  return success();
}

} // namespace acir::ac
