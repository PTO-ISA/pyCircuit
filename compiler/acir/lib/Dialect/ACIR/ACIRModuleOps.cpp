#include "acir/Dialect/ACIR/ACIROps.h"

#include "ACIRSourceContracts.h"
#include "ACIRStaticEvaluation.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"

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
    auto state = handle.getDefiningOp<DffeOp>();
    auto element = binding.getAs<ArrayAttr>("element");
    auto shape =
        state ? state->getAttrOfType<ArrayAttr>("ac.shape") : ArrayAttr();
    if (!state || state->getParentOfType<ModuleOp>() != module || !element ||
        !element.empty() || !shape || !shape.empty() ||
        binding.getAs<DictionaryAttr>("declaration") !=
            state->getAttrOfType<DictionaryAttr>("ac.declaration") ||
        logical != state->getAttrOfType<DictionaryAttr>("ac.logical_element"))
      return error() << "owned rule input does not match its source DFFE";
    return success();
  }

  auto formal = dyn_cast<BlockArgument>(handle);
  if (!formal || formal.getOwner() != &module.getBody().front())
    return error() << "formal rule input is not an enclosing module port";
  auto ports = module->getAttrOfType<ArrayAttr>("ac.ports");
  if (!ports || formal.getArgNumber() >= ports.size())
    return error() << "formal rule input has no matching PortSlot";
  auto port = dyn_cast<DictionaryAttr>(ports[formal.getArgNumber()]);
  auto role = port ? port.getAs<StringAttr>("role") : StringAttr();
  if (!port || !role || role.getValue() != "current" ||
      binding.getAs<StringAttr>("parameter") !=
          port.getAs<StringAttr>("parameter") ||
      binding.get("ordinal") != port.get("ordinal") ||
      logical != port.getAs<DictionaryAttr>("type"))
    return error() << "formal rule input does not match its current PortSlot";
  return success();
}

} // namespace

LogicalResult DffeOp::verify() {
  auto error = [&] { return emitOpError(); };
  if (failed(requireSourceUnit(*this)))
    return failure();
  auto owner = (*this)->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto declaration = (*this)->getAttrOfType<DictionaryAttr>("ac.declaration");
  auto logical = (*this)->getAttrOfType<DictionaryAttr>("ac.logical_element");
  auto shape = (*this)->getAttrOfType<ArrayAttr>("ac.shape");
  auto initial = (*this)->getAttrOfType<DictionaryAttr>("ac.initial_value");
  auto domain = (*this)->getAttrOfType<StringAttr>("ac.domain");
  if (!getNameAttr() || getName().empty() ||
      failed(detail::verifySourceOwner(owner, error)) ||
      failed(detail::verifyOccurrence(declaration, error)) || !logical ||
      failed(detail::verifyLogicalTypeStructure(logical, error)) || !shape ||
      !initial || !domain || domain.getValue() != "default")
    return error() << "source DFFE state contract is incomplete";
  auto module = (*this)->getParentOfType<ModuleOp>();
  if (!module ||
      owner != module->getAttrOfType<DictionaryAttr>("ac.source_owner"))
    return error() << "DFFE state must belong to its enclosing source module";
  if (failed(
          checkPhysical(cast<DffeType>(getState().getType()).getElementType(),
                        logical, *this)))
    return failure();
  if (shape.size() > 1)
    return error() << "source DFFE shape supports scalar or fixed list";
  for (Attribute raw : shape)
    if (!isa<StaticExprAttr>(raw))
      return error() << "source DFFE shape must contain StaticExpr";
  if (failed(verifyInitialSpec(initial, shape, *this)))
    return failure();
  if (!shape.empty())
    return error() << "U02-A source list initializers require later constant "
                      "evaluation";
  auto expression = initial.getAs<StaticExprAttr>("value");
  auto evaluated = detail::evaluateSourceStaticExpr(expression, *this, error);
  if (failed(evaluated))
    return failure();
  auto resolver = [&](FlatSymbolRefAttr symbol) {
    return detail::resolveSourceRecord(symbol, *this, error);
  };
  if (failed(detail::verifyStaticValueMatchesType(
          *evaluated, logical, detail::ExpectedTypeKind::Logical, resolver,
          error)))
    return error() << "source DFFE reset value does not match its logical type";
  return success();
}

LogicalResult ModuleOp::verify() {
  auto error = [&] { return emitOpError(); };
  if (failed(requireSourceUnit(*this)))
    return failure();
  auto owner = (*this)->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto origin = (*this)->getAttrOfType<DictionaryAttr>("ac.origin");
  auto ports = (*this)->getAttrOfType<ArrayAttr>("ac.ports");
  if (getName().empty() || getSymName().empty() ||
      failed(detail::verifySourceOwner(owner, error)) ||
      failed(detail::verifyOccurrence(origin, error)) || !ports)
    return error() << "source module declaration metadata is incomplete";
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
  auto error = [&] { return emitOpError(); };
  if (failed(requireSourceUnit(*this)))
    return failure();
  if (getBody().getBlocks().size() != 1)
    return error() << "source module body requires one block";
  auto ports = (*this)->getAttrOfType<ArrayAttr>("ac.ports");
  Block &body = getBody().front();
  if (!ports || body.getNumArguments() != ports.size())
    return error() << "module body arguments must match ac.ports";
  for (auto [index, raw] : llvm::enumerate(ports)) {
    auto port = cast<DictionaryAttr>(raw);
    auto handle = dyn_cast<DffeType>(body.getArgument(index).getType());
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
  for (DffeOp state : body.getOps<DffeOp>())
    if (!stateNames.insert(state.getName()).second)
      return error() << "source module declares one state name more than once";
  return success();
}

LogicalResult RuleOp::verify() {
  auto error = [&] { return emitOpError(); };
  if (failed(requireSourceUnit(*this)))
    return failure();
  if (getName().empty() ||
      failed(detail::verifyOccurrence(getRegistrationAttr(), error)) ||
      failed(requireOrigin(*this)))
    return error() << "rule registration/body origin is incomplete";
  auto inputs = (*this)->getAttrOfType<ArrayAttr>("ac.input_bindings");
  auto outputs = (*this)->getAttrOfType<ArrayAttr>("ac.output_bindings");
  auto inputTypes = (*this)->getAttrOfType<ArrayAttr>("ac.input_types");
  auto outputTypes = (*this)->getAttrOfType<ArrayAttr>("ac.output_types");
  if (!inputs || !outputs || !inputTypes || !outputTypes ||
      inputs.size() != getInputs().size() ||
      outputs.size() != getOutputs().size() ||
      inputTypes.size() != inputs.size() ||
      outputTypes.size() != outputs.size())
    return error() << "rule bindings and logical types must match handles";
  for (auto [index, raw] : llvm::enumerate(inputs)) {
    auto binding = dyn_cast<DictionaryAttr>(raw);
    auto logical = dyn_cast<DictionaryAttr>(inputTypes[index]);
    if (!binding || !logical ||
        failed(detail::verifyStateRef(binding, error)) ||
        failed(detail::verifyLogicalTypeStructure(logical, error)) ||
        failed(checkPhysical(
            cast<DffeType>(getInputs()[index].getType()).getElementType(),
            logical, *this)) ||
        failed(verifyRuleInputAuthority(getInputs()[index], binding, logical,
                                        *this)))
      return error() << "rule input binding/type mismatch";
  }
  for (auto [index, raw] : llvm::enumerate(outputs)) {
    auto binding = dyn_cast<DictionaryAttr>(raw);
    auto logical = dyn_cast<DictionaryAttr>(outputTypes[index]);
    if (!binding || !logical ||
        failed(detail::verifyStateRef(binding, error)) ||
        failed(detail::verifyLogicalTypeStructure(logical, error)) ||
        failed(checkPhysical(
            cast<DffeType>(getOutputs()[index].getType()).getElementType(),
            logical, *this)))
      return error() << "rule output binding/type mismatch";
  }
  if (!getNextValues().empty())
    return error() << "source rule has no next-value results";
  return success();
}

LogicalResult RuleOp::verifyRegions() {
  auto error = [&] { return emitOpError(); };
  if (failed(requireSourceUnit(*this)))
    return failure();
  if (!getReadiness().empty() || !getConditions().empty() ||
      !getAdmission().empty())
    return error() << "ordinary source rule has no admission regions";
  if (getBody().getBlocks().size() != 1)
    return error() << "rule computation requires one block";
  Block &body = getBody().front();
  if (body.getNumArguments() != getInputs().size())
    return error() << "rule body inputs must be current payload arguments";
  for (auto [index, handle] : llvm::enumerate(getInputs()))
    if (body.getArgument(index).getType() !=
        cast<DffeType>(handle.getType()).getElementType())
      return error() << "rule body input payload type mismatch";
  auto terminator = dyn_cast_or_null<YieldOp>(body.getTerminator());
  if (!terminator || terminator.getValues().size() != 2 * getOutputs().size())
    return error() << "rule yield must contain data,enable for each output";
  for (size_t index = 0; index < getOutputs().size(); ++index) {
    Type payload =
        cast<DffeType>(getOutputs()[index].getType()).getElementType();
    if (terminator.getValues()[2 * index].getType() != payload ||
        !terminator.getValues()[2 * index + 1].getType().isInteger(1))
      return error() << "rule yield data/enable type mismatch";
  }
  return success();
}

LogicalResult InstanceOp::verify() {
  auto error = [&] { return emitOpError(); };
  if (failed(requireSourceUnit(*this)))
    return failure();
  if (getName().empty() || failed(requireOrigin(*this)))
    return error() << "instance identity/origin is incomplete";
  Attribute callee = getCalleeAttr();
  auto staticArguments = (*this)->getAttrOfType<ArrayAttr>("ac.static_args");
  if (!isa<FlatSymbolRefAttr>(callee) || !staticArguments ||
      !getNextValues().empty())
    return error() << "source instance requires symbol, static arguments and "
                      "no results";
  for (Attribute argument : staticArguments)
    if (!isa<StaticExprAttr>(argument))
      return error() << "source instance static arguments must be StaticExpr";
  return success();
}

} // namespace acir::ac
