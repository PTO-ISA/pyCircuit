#include "ACIRSourceContracts.h"
#include "mlir/IR/SymbolTable.h"
#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"
#include "llvm/ADT/DenseSet.h"

using namespace mlir;
namespace acir::ac {
namespace {
// Symbol-use checks run before child custom verifiers. Validate only the
// receiver header here; never recurse into body verification or Method calls.
LogicalResult receiverHeader(Operation *module, Operation *site) {
  if (!isa_and_nonnull<ModuleOp, ModuleImportOp>(module))
    return site->emitOpError() << "Method receiver must resolve to a module";
  if (auto body = dyn_cast<ModuleOp>(module)) {
    if (body->getNumRegions() != 1 || !body->getRegion(0).hasOneBlock())
      return site->emitOpError()
             << "Method receiver requires one module body block";
    if (failed(body.verifyInvariantsImpl()))
      return failure();
    return body.verify();
  }
  auto imported = cast<ModuleImportOp>(module);
  if (imported->getNumRegions() != 0)
    return site->emitOpError()
           << "imported Method receiver cannot have regions";
  if (failed(imported.verifyInvariantsImpl()))
    return failure();
  return imported.verify();
}
} // namespace
FunctionType SourceMethodView::getFunctionType() const {
  return definition ? cast<FunctionType>(
                          definition->getAttrOfType<TypeAttr>("function_type")
                              .getValue())
                    : cast<FunctionType>(
                          row.getAs<TypeAttr>("function_type").getValue());
}
ArrayAttr SourceMethodView::getArgumentNames() const {
  return definition ? definition->getAttrOfType<ArrayAttr>("argument_names")
                    : row.getAs<ArrayAttr>("argument_names");
}
ArrayAttr SourceMethodView::getParameters() const {
  return definition ? definition->getAttrOfType<ArrayAttr>("ac.parameters")
                    : row.getAs<ArrayAttr>("ac.parameters");
}
ArrayAttr SourceMethodView::getResultConstraints() const {
  return definition
             ? definition->getAttrOfType<ArrayAttr>("ac.result_constraints")
             : row.getAs<ArrayAttr>("ac.result_constraints");
}
StringAttr SourceMethodView::getReturnForm() const {
  return definition ? definition->getAttrOfType<StringAttr>("ac.return_form")
                    : row.getAs<StringAttr>("ac.return_form");
}
DictionaryAttr SourceMethodView::getDeclaration() const {
  return definition ? definition->getAttrOfType<DictionaryAttr>("occurrence")
                    : row.getAs<DictionaryAttr>("declaration");
}
SymbolRefAttr SourceMethodView::getSymbol() const {
  auto name = definition ? definition->getAttrOfType<StringAttr>("sym_name")
                         : row.getAs<StringAttr>("name");
  return SymbolRefAttr::get(module->getContext(),
                            SymbolTable::getSymbolName(module).getValue(),
                            {FlatSymbolRefAttr::get(name)});
}
FailureOr<SourceMethodView>
lookupSourceMethod(Operation *module, StringRef name, Operation *site) {
  if (failed(receiverHeader(module, site)))
    return failure();
  SourceMethodView result{module, {}, {}};
  if (auto body = dyn_cast<ModuleOp>(module)) {
    if (!body.getBody().hasOneBlock())
      return site->emitOpError()
             << "Method receiver requires one module body block";
    for (auto candidate : body.getBody().front().getOps<SourceMethodOp>()) {
      if (candidate->getNumRegions() != 2)
        return candidate.emitOpError()
               << "method requires guard and body regions";
      for (Region &region : candidate->getRegions())
        if (!region.hasOneBlock() || region.front().empty())
          return candidate.emitOpError()
                 << "method guard and body require one nonempty block";
      if (failed(candidate.verifyInvariantsImpl()))
        return failure();
      if (candidate.getSymName() == name) {
        if (result.definition)
          return site->emitOpError() << "callee receiver method is ambiguous";
        if (failed(candidate.verify()))
          return failure();
        result.definition = candidate;
      }
    }
  } else {
    auto summary = module->getAttrOfType<DictionaryAttr>("transaction_summary");
    if (!summary || failed(verifySourceTransactionSummary(module, summary)))
      return site->emitOpError() << "imported Method receiver requires a "
                                    "verified transaction summary";
    for (auto raw : summary.getAs<ArrayAttr>("methods")) {
      auto candidate = cast<DictionaryAttr>(raw);
      if (candidate.getAs<StringAttr>("name").getValue() == name) {
        if (result.row)
          return site->emitOpError() << "callee receiver method is ambiguous";
        result.row = candidate;
      }
    }
  }
  if (!result.definition && !result.row)
    return site->emitOpError()
           << "callee must resolve to a direct receiver method";
  return result;
}
namespace {
LogicalResult admitDependency(Operation *caller, Operation *target,
                              Operation *site) {
  auto provider = target->getAttrOfType<DictionaryAttr>("source_owner");
  // Native generic packages have no source envelope and retain native
  // admission.
  auto package = caller->getParentOfType<mlir::ModuleOp>();
  auto interfaces = package ? package->getAttrOfType<ArrayAttr>("ac.interfaces")
                            : ArrayAttr();
  // Source-header callers supply canonical authorities after validating every
  // primitive snapshot against the builtin catalog. Native callers establish
  // only structural legality; publication still requires that registry check.
  bool primitive =
      isa<ModuleImportOp>(target) && target->hasAttr("primitive_kind");
  if (interfaces && provider && !primitive &&
      !llvm::is_contained(interfaces, provider))
    return site->emitOpError() << "Method receiver provider is absent from "
                                  "caller source ac.interfaces closure";
  return success();
}
FailureOr<ResolvedSourceMethod> resolve(Operation *caller, SymbolRefAttr symbol,
                                        ArrayAttr path, bool names,
                                        const HardwareBindings &parent,
                                        SourceModuleLookup lookup,
                                        Operation *site) {
  if (!caller || !symbol || symbol.getNestedReferences().size() != 1 || !path)
    return site->emitOpError()
           << "Method call requires a qualified callee and receiver path";
  if (path.size() > 128)
    return site->emitOpError() << "Method receiver depth budget exceeded";
  if (failed(receiverHeader(caller, site)))
    return failure();
  auto package = caller->getParentOfType<mlir::ModuleOp>();
  if (!package)
    return site->emitOpError()
           << "Method receiver requires an enclosing package";
  HardwareAnalysis hardware(package);
  Operation *owner = caller;
  HardwareBindings bindings = parent;
  if (!bindings.owner)
    bindings.owner = caller;
  SmallVector<Attribute> occurrences;
  uint64_t work = 0;
  for (Attribute endpoint : path) {
    if (failed(receiverHeader(owner, site)))
      return failure();
    Operation *allocation = nullptr;
    DictionaryAttr selected, identity;
    llvm::DenseSet<Attribute> seenNames, seenOccurrences;
    auto consider = [&](StringAttr name, DictionaryAttr occurrence,
                        Operation *op, DictionaryAttr row) -> LogicalResult {
      if (++work > 1024 * 1024)
        return site->emitOpError() << "Method receiver work budget exceeded";
      if (!name || !occurrence ||
          failed(detail::verifyOccurrence(
              occurrence, [&] { return site->emitOpError(); })) ||
          failed(detail::verifyOriginDefinition(
              occurrence,
              FlatSymbolRefAttr::get(SymbolTable::getSymbolName(owner)),
              "Method receiver instance", [&] { return site->emitOpError(); })))
        return failure();
      if (!seenNames.insert(name).second ||
          !seenOccurrences.insert(occurrence).second)
        return site->emitOpError()
               << "Method receiver instances require distinct names and "
                  "occurrence identities";
      bool matches = names ? endpoint == name : endpoint == occurrence;
      if (!matches)
        return success();
      if (identity)
        return site->emitOpError() << "Method receiver instance is ambiguous";
      allocation = op;
      selected = row;
      identity = occurrence;
      return success();
    };
    if (auto body = dyn_cast<ModuleOp>(owner)) {
      if (!body.getBody().hasOneBlock())
        return site->emitOpError()
               << "Method receiver requires one module body block";
      for (auto instance : body.getBody().front().getOps<InstanceOp>()) {
        if (failed(instance.verifyInvariantsImpl()) ||
            failed(instance.verify()) ||
            failed(consider(instance.getInstanceNameAttr(),
                            instance.getOccurrence(), instance, {})))
          return failure();
      }
    } else {
      auto summary =
          owner->getAttrOfType<DictionaryAttr>("transaction_summary");
      if (!summary || failed(verifySourceTransactionSummary(owner, summary)))
        return site->emitOpError()
               << "Method receiver suffix requires published instance owners";
      for (Attribute raw : summary.getAs<ArrayAttr>("owners")) {
        auto row = cast<DictionaryAttr>(raw);
        if (row.getAs<StringAttr>("kind").getValue() == "instance" &&
            failed(consider(row.getAs<StringAttr>("name"),
                            row.getAs<DictionaryAttr>("occurrence"), nullptr,
                            row)))
          return failure();
      }
    }
    if (!identity)
      return site->emitOpError()
             << "Method instance path does not name a direct child instance";
    auto callee = allocation
                      ? allocation->getAttrOfType<FlatSymbolRefAttr>("callee")
                      : selected.getAs<FlatSymbolRefAttr>("callee");
    Operation *target = callee ? lookup(callee) : nullptr;
    if (failed(receiverHeader(target, site)))
      return failure();
    if (failed(admitDependency(caller, target, site)))
      return failure();
    auto actualParameters =
        allocation ? allocation->getAttrOfType<ArrayAttr>("parameters")
                   : selected.getAs<ArrayAttr>("parameters");
    auto actualTypes =
        allocation ? allocation->getAttrOfType<ArrayAttr>("type_arguments")
                   : selected.getAs<ArrayAttr>("type_arguments");
    auto bound = hardware.bindCallee(target, actualParameters, actualTypes,
                                     bindings, site);
    if (failed(bound))
      return failure();
    auto signature = cast<FunctionType>(
        target->getAttrOfType<TypeAttr>("function_type").getValue());
    auto checkTypes = [&](TypeRange actual, TypeRange formal) -> LogicalResult {
      if (actual.size() != formal.size())
        return site->emitOpError()
               << "Method receiver instance signature arity mismatch";
      for (auto [a, f] : llvm::zip(actual, formal)) {
        auto actualType = hardware.resolveType(a, bindings, site);
        auto formalType = hardware.resolveType(f, *bound, site);
        if (failed(actualType) || failed(formalType))
          return failure();
        if (!areEquivalentHardwareTypes(*actualType, *formalType))
          return site->emitOpError()
                 << "Method receiver instance payload type mismatch";
      }
      return success();
    };
    SmallVector<Type> inputs, results;
    if (allocation) {
      inputs.assign(allocation->getOperandTypes().begin(),
                    allocation->getOperandTypes().end());
      results.assign(allocation->getResultTypes().begin(),
                     allocation->getResultTypes().end());
    } else {
      for (Attribute raw : selected.getAs<ArrayAttr>("input_types"))
        inputs.push_back(cast<TypeAttr>(raw).getValue());
      for (Attribute raw : selected.getAs<ArrayAttr>("result_types"))
        results.push_back(cast<TypeAttr>(raw).getValue());
    }
    if (failed(checkTypes(inputs, signature.getInputs())) ||
        failed(checkTypes(results, signature.getResults())))
      return failure();
    occurrences.push_back(identity);
    bindings = *bound;
    owner = target;
  }
  if (symbol.getRootReference() != SymbolTable::getSymbolName(owner))
    return site->emitOpError()
           << "callee module must match resolved receiver module";
  auto method = lookupSourceMethod(
      owner, symbol.getNestedReferences().front().getValue(), site);
  if (failed(method))
    return failure();
  auto receiver = DictionaryAttr::get(
      site->getContext(),
      {NamedAttribute(StringAttr::get(site->getContext(), "instance_path"),
                      ArrayAttr::get(site->getContext(), occurrences))});
  return ResolvedSourceMethod{*method, bindings, receiver};
}
} // namespace
FailureOr<ResolvedSourceMethod>
resolveSourceMethodCall(SourceMethodCallOp call) {
  auto caller = call->getParentOfType<ModuleOp>();
  HardwareBindings parent;
  parent.owner = caller;
  auto package = call->getParentOfType<mlir::ModuleOp>();
  SymbolTableCollection symbols;
  auto lookup = [&](FlatSymbolRefAttr symbol) -> Operation * {
    return symbols.lookupSymbolIn(package, symbol);
  };
  return resolve(caller, call.getCalleeAttr(), call.getInstancePath(), true,
                 parent, lookup, call);
}
FailureOr<ResolvedSourceMethod>
resolveSourceMethodCall(Operation *caller, DictionaryAttr row,
                        const HardwareBindings &parent,
                        SourceModuleLookup lookup, Operation *site) {
  if (!row)
    return site->emitOpError() << "Method call requires a summary call row";
  auto receiver = row.getAs<DictionaryAttr>("receiver");
  auto resolved = resolve(caller, row.getAs<SymbolRefAttr>("callee"),
                          receiver ? receiver.getAs<ArrayAttr>("instance_path")
                                   : ArrayAttr(),
                          false, parent, lookup, site);
  if (failed(resolved))
    return failure();
  auto actuals = row.getAs<ArrayAttr>("arguments");
  auto formal = resolved->method.getFunctionType();
  if (!actuals || actuals.size() != formal.getNumInputs())
    return site->emitOpError()
           << "Method call argument arity must match signature";
  HardwareAnalysis hardware(caller->getParentOfType<mlir::ModuleOp>());
  for (auto [actual, type] : llvm::zip(actuals, formal.getInputs())) {
    auto value = dyn_cast<DictionaryAttr>(actual);
    auto fact = value ? value.getAs<DictionaryAttr>("fact") : DictionaryAttr();
    auto dependencies =
        value ? value.getAs<DictionaryAttr>("dependencies") : DictionaryAttr();
    auto kind = fact ? fact.getAs<StringAttr>("kind") : StringAttr();
    if (!value || value.size() != 2 || !fact || !dependencies || !kind)
      return site->emitOpError()
             << "Method argument requires a predicate fact and dependencies";
    // Local ordinal scope and dependency authority are checked by the existing
    // summary structure verifier. This resolver checks closed predicate shape
    // and bound formal widths without inventing an opaque actual payload type.
    if (kind.getValue() == "unknown") {
      if (fact.size() != 1)
        return site->emitOpError()
               << "unknown Method argument fact has extra fields";
      continue;
    }
    if (kind.getValue() == "constant") {
      if (fact.size() != 2 || !fact.getAs<BoolAttr>("value"))
        return site->emitOpError()
               << "constant Method argument fact requires BoolAttr";
    } else if (kind.getValue() == "argument") {
      auto ordinal = fact.getAs<IntegerAttr>("ordinal");
      if (fact.size() != 2 || !ordinal ||
          !ordinal.getType().isSignlessInteger(64) ||
          ordinal.getValue().isNegative())
        return site->emitOpError()
               << "Method argument fact requires a nonnegative i64 ordinal";
    } else {
      return site->emitOpError()
             << "unsupported Method argument predicate fact kind";
    }
    auto boundType = hardware.resolveType(type, resolved->bindings, site);
    if (failed(boundType))
      return failure();
    auto width = hardware.getPackedWidth(*boundType, resolved->bindings, site);
    if (failed(width))
      return failure();
    if (!isa<BitsType>(*boundType) || *width != 1)
      return site->emitOpError() << "known Method argument predicate requires "
                                    "a bound physical one-bit formal";
  }
  return resolved;
}
} // namespace acir::ac
