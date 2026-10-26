#include "EntryBoundary.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;
namespace acir::ac::detail {
namespace {
bool entryShape(DictionaryAttr entry) {
  if (!entry || entry.size() != 3 || !entry.getAs<FlatSymbolRefAttr>("callee") ||
      !entry.getAs<ArrayAttr>("parameters") || !entry.getAs<ArrayAttr>("type_arguments") ||
      entry.getAs<ArrayAttr>("parameters").size() > HardwareSourceCheckLimits{}.maxWorkUnits ||
      entry.getAs<ArrayAttr>("type_arguments").size() > HardwareSourceCheckLimits{}.maxWorkUnits) return false;
  return llvm::all_of(entry.getAs<ArrayAttr>("parameters"), [](Attribute a) { return isa<StaticExprAttr>(a); }) &&
      llvm::all_of(entry.getAs<ArrayAttr>("type_arguments"), [](Attribute a) { auto t = dyn_cast<TypeAttr>(a); return t && isa<BitsType, StructType>(t.getValue()); });
}
}
LogicalResult verifyEntryBoundaryAttribute(Operation *operation, Attribute attribute) {
  auto system = dyn_cast<SystemOp>(operation);
  auto package = dyn_cast_or_null<mlir::ModuleOp>(operation->getParentOp());
  auto stage = package ? package->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
  auto record = dyn_cast<DictionaryAttr>(attribute);
  auto version = record ? record.getAs<IntegerAttr>("version") : IntegerAttr();
  if (!system || !package || package->hasAttr("ac.unit_kind") ||
      (stage ? stage.getValue() != "routed" && stage.getValue() != "final"
             : !package->getAttrOfType<ArrayAttr>("ac.source_units")) ||
      !record || record.size() != 2 || !version || !version.getType().isSignlessInteger(64) ||
      version.getInt() != 1 || !entryShape(record.getAs<DictionaryAttr>("logical_entry")))
    return operation->emitOpError("ac.entry_boundary requires exactly version=1 : i64 and logical_entry on the executable System");
  if (stage) {
    auto contract = package->getAttrOfType<IntegerAttr>("ac.routed_contract");
    if (!contract || !contract.getType().isSignlessInteger(64) ||
        contract.getInt() != 3)
      return operation->emitOpError("ac.entry_boundary requires ac.routed_contract = 3 : i64 in routed and final packages");
  }
  return success();
}

FailureOr<bool> needsEntryBoundary(const HardwareAnalysis &analysis,
    const std::function<LogicalResult(uint64_t, Operation *)> &debit) {
  auto package = analysis.getPackage();
  auto systems = package.getOps<SystemOp>();
  if (!llvm::hasSingleElement(systems)) return package.emitOpError("entry normalization requires one System");
  auto selected = resolveHardwareRootBindings(analysis);
  if (failed(selected)) return failure();
  auto root = cast<ModuleOp>(selected->owner);
  if (root->hasAttr("ac.root_kind")) return false;
  if (!root.getTypeParameters().empty() || root->hasAttr("ac.reference_parameters"))
    return root.emitOpError("entry normalization requires ordinary Integer formals");
  auto closure = [&](ModuleOp start, auto &&visit) -> LogicalResult {
    SmallVector<ModuleOp> pending;
    llvm::DenseSet<Operation *> seen;
    if (failed(debit(16, start))) return failure();
    pending.push_back(start);
    while (!pending.empty()) {
      auto module = pending.pop_back_val();
      if (failed(debit(16, module))) return failure();
      if (!seen.insert(module).second) continue;
      if (failed(visit(module))) return failure();
      for (Operation &op : module.getBody().front()) {
        if (failed(debit(8, &op))) return failure();
        Operation *callee = nullptr;
        if (auto instance = dyn_cast<InstanceOp>(op)) callee = analysis.resolveCallee(instance);
        if (auto collection = dyn_cast<CollectionOp>(op)) callee = analysis.resolveCallee(collection);
        if (auto child = dyn_cast_or_null<ModuleOp>(callee)) {
          if (failed(debit(8, &op))) return failure();
          pending.push_back(child);
        }
      }
    }
    return success();
  };
  // Occurrence lifting depends on the complete allocation closure, just as
  // transaction state analysis does. A fixed wrapper can therefore share a
  // parameterized state owner without having integer formals of its own.
  bool transactions = false, integerParameters = false, allocated = false;
  if (failed(closure(root, [&](ModuleOp module) {
        integerParameters |= !module.getParameters().empty();
        for (Operation &op : module.getBody().front()) {
          if (failed(debit(1, &op))) return failure();
          transactions |= isa<SourceTransactionOp>(op);
        }
        return success();
      }))) return failure();
  if (!transactions || !integerParameters) return false;
  for (auto module : package.getOps<ModuleOp>()) {
    if (failed(debit(1, module))) return failure();
    // A distinct closed root must actually allocate the selection. Counting
    // the selected fixed wrapper's zero-edge closure would admit an adapter
    // even when it has no external allocating occurrence.
    if (module == root || !module.getParameters().empty() || !module.getTypeParameters().empty() || module->hasAttr("ac.reference_parameters") ||
        module->hasAttr("ac.root_kind")) continue;
    if (failed(closure(module, [&](ModuleOp child) {
          allocated |= child == root;
          return success();
        }))) return failure();
    if (allocated) return true;
  }
  return false;
}

LogicalResult verifyEntryBoundary(const HardwareAnalysis &analysis, bool normalizedInput,
    const std::function<LogicalResult(uint64_t, Operation *)> &debit) {
  auto package = analysis.getPackage();
  SystemOp system;
  ModuleOp adapter;
  unsigned systems = 0, markers = 0, reserved = 0;
  auto inventory = package.walk([&](Operation *op) {
    if (failed(debit(8, op))) return WalkResult::interrupt();
    if (auto selected = dyn_cast<SystemOp>(op)) { system = selected; ++systems; }
    if (op->hasAttr("ac.entry_boundary")) {
      ++markers;
      if (failed(verifyEntryBoundaryAttribute(op, op->getAttr("ac.entry_boundary")))) return WalkResult::interrupt();
    }
    if (auto name = SymbolTable::getSymbolName(op); name && name.getValue() == entryBoundarySymbol) {
      ++reserved;
      adapter = dyn_cast<ModuleOp>(op);
    }
    return WalkResult::advance();
  });
  if (inventory.wasInterrupted()) return failure();
  // Only the physical selection and the independently checked provenance of
  // generated routes may mention this compiler identity. An otherwise ignored
  // attribute must not become a second alias or an unauthenticated use.
  bool badReference = false;
  std::function<void(Attribute, Operation *)> references = [&](Attribute attr, Operation *op) {
    if (badReference) return;
    if (failed(debit(1, op))) { badReference = true; return; }
    if (auto symbol = dyn_cast<SymbolRefAttr>(attr)) {
      badReference = symbol.getRootReference().getValue() == entryBoundarySymbol;
    } else if (auto dictionary = dyn_cast<DictionaryAttr>(attr)) {
      for (NamedAttribute field : dictionary) references(field.getValue(), op);
    } else if (auto array = dyn_cast<ArrayAttr>(attr)) {
      for (Attribute element : array) references(element, op);
    } else if (auto expression = dyn_cast<StaticExprAttr>(attr)) {
      references(expression.getTree(), op);
    }
  };
  auto referencesWalk = package.walk([&](Operation *op) {
    for (NamedAttribute attr : op->getAttrs()) {
      auto name = attr.getName().getValue();
      if (markers && op == system.getOperation() && name == "entry") continue;
      bool routedRule = isa<RuleOp>(op) && op->hasAttr("ac.registration_path") &&
          op->hasAttr("ac.lexical_registration") && op->hasAttr("ac.registration_owner");
      bool canonicalChild = isa<InstanceOp>(op) && op->getParentOp() == adapter.getOperation();
      bool routeProvenance = (name == "occurrence" && (canonicalChild || routedRule)) ||
          (routedRule && llvm::is_contained(ArrayRef<StringRef>{"ac.sampled_admission", "ac.required_checks",
              "ac.required_observations", "ac.registration_path", "ac.lexical_registration", "ac.registration_owner"}, name)) ||
          (isa<SourceExpectOp>(op) && name == "ac.check_id") ||
          (isa<SourceObserveOp>(op) && name == "ac.observation_id") ||
          (isa<ModuleOp>(op) && llvm::is_contained(ArrayRef<StringRef>{
              "ac.transaction_wiring", "ac.queue_wiring", "ac.transaction_returns"}, name));
      if (markers && routeProvenance) continue;
      references(attr.getValue(), op);
      if (badReference) {
        op->emitOpError("reserved entry boundary symbol has an unauthorized reference");
        return WalkResult::interrupt();
      }
    }
    return WalkResult::advance();
  });
  if (referencesWalk.wasInterrupted()) return failure();
  if (!markers) {
    if (reserved) return package.emitOpError("reserved entry boundary symbol requires canonical metadata");
    return success();
  }
  if (markers != 1 || systems != 1 || reserved != 1 || !adapter || adapter->getParentOp() != package)
    return package.emitOpError("entry boundary requires one System and exactly one canonical adapter definition");
  auto record = system->getAttrOfType<DictionaryAttr>("ac.entry_boundary");
  if (!record) return package.emitOpError("entry boundary metadata belongs only to the sole System");
  auto logicalEntry = record.getAs<DictionaryAttr>("logical_entry");
  Builder builder(package.getContext());
  auto empty = builder.getArrayAttr({});
  auto physicalEntry = builder.getDictionaryAttr({
      builder.getNamedAttr("callee", FlatSymbolRefAttr::get(package.getContext(), entryBoundarySymbol)),
      builder.getNamedAttr("parameters", empty), builder.getNamedAttr("type_arguments", empty)});
  if (system.getEntry() != physicalEntry || logicalEntry.getAs<FlatSymbolRefAttr>("callee").getValue() == entryBoundarySymbol)
    return system.emitOpError("entry boundary physical/logical selection mismatch");
  auto bound = resolveHardwareEntryBindings(analysis, logicalEntry);
  if (failed(bound)) return failure();
  auto logical = cast<ModuleOp>(bound->owner);
  auto origin = logical->getAttrOfType<DictionaryAttr>("ac.origin");
  auto source = logical.getSourceOwner();
  auto location = dyn_cast<FileLineColLoc>(logical.getLoc());
  auto owners = package->getAttrOfType<ArrayAttr>("ac.source_units");
  auto error = [&] { return adapter.emitOpError(); };
  if (logical->hasAttr("ac.root_kind") ||
      !logical.getTypeParameters().empty() || logical->hasAttr("ac.reference_parameters") ||
      !origin || !location || location.getFilename().getValue().empty() ||
      !location.getLine() || !location.getColumn() || !source || !owners || !llvm::is_contained(owners, source) ||
      failed(verifySourceOwner(source, error)) || failed(verifyOccurrence(origin, error)) ||
      failed(verifyOriginDefinition(origin, FlatSymbolRefAttr::get(package.getContext(), logical.getSymName()), "logical entry", error)))
    return error() << "entry boundary requires authenticated ordinary logical root provenance";
  if (adapter.getSourceOwner() != source || adapter.getLoc() != logical.getLoc() ||
      !adapter.getParameters().empty() || !adapter.getTypeParameters().empty() ||
      adapter->getAttr("ac.domain_inputs") != logical->getAttr("ac.domain_inputs"))
    return error() << "entry boundary owner, location, formals or domain mismatch";
  // The adapter has no source-call or source-state declaration authority.
  for (NamedAttribute attr : adapter->getAttrs()) {
    StringRef name = attr.getName().getValue();
    if (llvm::is_contained(ArrayRef<StringRef>{"sym_name", "source_owner", "parameters", "type_parameters", "function_type", "input_names", "output_names", "ac.domain_inputs"}, name)) continue;
    if (!normalizedInput && llvm::is_contained(ArrayRef<StringRef>{"ac.internal_ports", "ac.input_validity", "ac.output_validity", "ac.routed_output_validity", "ac.transaction_wiring", "ac.queue_wiring", "ac.transaction_returns"}, name)) continue;
    return error() << "unexpected entry boundary declaration metadata: " << name;
  }
  auto logicalLayout = getModulePortLayout(logical);
  auto adapterLayout = getModulePortLayout(adapter);
  if (failed(logicalLayout) || failed(adapterLayout)) return failure();
  if (logicalLayout->sourceInputCount != adapterLayout->sourceInputCount ||
      logicalLayout->sourceOutputCount != adapterLayout->sourceOutputCount)
    return error() << "entry boundary public prefix count mismatch";
  auto logicalType = logical.getFunctionType(), adapterType = adapter.getFunctionType();
  for (bool inputs : {true, false}) {
    unsigned count = inputs ? logicalLayout->sourceInputCount : logicalLayout->sourceOutputCount;
    auto expected = inputs ? logicalType.getInputs() : logicalType.getResults();
    auto actual = inputs ? adapterType.getInputs() : adapterType.getResults();
    auto expectedNames = inputs ? logical.getInputNames() : logical.getOutputNames();
    auto actualNames = inputs ? adapter.getInputNames() : adapter.getOutputNames();
    for (unsigned i = 0; i < count; ++i) {
      if (failed(debit(8, adapter))) return failure();
      auto resolved = analysis.resolveType(expected[i], *bound, logical);
      if (failed(resolved)) return failure();
      if (*resolved != actual[i] || expectedNames[i] != actualNames[i])
        return error() << "entry boundary public port type/name mismatch";
    }
  }
  if (!adapter.getBody().hasOneBlock()) return error() << "entry boundary requires one block";
  auto &body = adapter.getBody().front();
  InstanceOp child;
  unsigned children = 0;
  for (Operation &op : body) {
    if (failed(debit(8, &op))) return failure();
    if (auto instance = dyn_cast<InstanceOp>(op)) { child = instance; ++children; }
    if (normalizedInput && !isa<InstanceOp, YieldOp>(op)) return error() << "normalized entry boundary accepts only canonical child and yield";
    if (isa<SourceStateOp, SourceMethodOp, SourceTransactionOp, CollectionOp>(op))
      return error() << "entry boundary cannot own storage, collections or source declarations";
  }
  if (children != 1 || !child || child.getInstanceName() != "entry" ||
      child.getCallee() != logical.getSymName() || !child.getTypeArguments().empty() ||
      child.getLoc() != logical.getLoc() || body.back().getLoc() != logical.getLoc() ||
      !isa<YieldOp>(body.back())) return error() << "entry boundary canonical child/yield mismatch";
  if (!body.back().getAttrs().empty())
    return error() << "entry boundary yield cannot carry metadata";
  for (NamedAttribute attr : child->getAttrs())
    if (!llvm::is_contained(ArrayRef<StringRef>{"instance_name", "callee", "parameters", "type_arguments", "occurrence"},
                            attr.getName().getValue()))
      return error() << "entry boundary child cannot carry additional metadata";
  NamedAttrList site(origin.getAs<DictionaryAttr>("site"));
  site.set("definition", FlatSymbolRefAttr::get(package.getContext(), entryBoundarySymbol));
  NamedAttrList occurrence(origin);
  occurrence.set("site", builder.getDictionaryAttr(site.getAttrs()));
  if (child.getOccurrence() != builder.getDictionaryAttr(occurrence.getAttrs()))
    return error() << "entry boundary derived occurrence mismatch";
  auto span = builder.getDictionaryAttr({builder.getNamedAttr("path", location.getFilename()),
      builder.getNamedAttr("line", builder.getI64IntegerAttr(location.getLine())),
      builder.getNamedAttr("column", builder.getI64IntegerAttr(location.getColumn())),
      builder.getNamedAttr("end_line", builder.getI64IntegerAttr(location.getLine())),
      builder.getNamedAttr("end_column", builder.getI64IntegerAttr(location.getColumn()))});
  SmallVector<Attribute> actuals;
  for (Attribute raw : logical.getParameters()) {
    if (failed(debit(16, child))) return failure();
    auto name = cast<DictionaryAttr>(raw).getAs<StringAttr>("name");
    auto value = dyn_cast_or_null<MathIntAttr>(bound->integers.lookup(name.getValue()));
    if (!value) return error() << "entry boundary integer actual is not closed";
    actuals.push_back(StaticExprAttr::get(package.getContext(), builder.getDictionaryAttr({
        builder.getNamedAttr("kind", builder.getStringAttr("literal")),
        builder.getNamedAttr("value", builder.getDictionaryAttr({builder.getNamedAttr("kind", builder.getStringAttr("integer")), builder.getNamedAttr("value", value)})),
        builder.getNamedAttr("origin", origin), builder.getNamedAttr("location", span)})));
  }
  if (child.getParameters() != builder.getArrayAttr(actuals)) return error() << "entry boundary canonical literal actuals mismatch";
  unsigned inputs = adapterLayout->sourceInputCount, outputs = adapterLayout->sourceOutputCount;
  if (normalizedInput && (body.getOperations().size() != 2 || child->getNumOperands() != inputs || child->getNumResults() != outputs ||
      body.getNumArguments() != inputs || body.back().getNumOperands() != outputs)) return error() << "normalized entry boundary has extra wiring";
  if (child->getNumOperands() < inputs || child->getNumResults() < outputs || body.back().getNumOperands() < outputs) return error() << "entry boundary public wiring is incomplete";
  for (unsigned i = 0; i < inputs; ++i)
    if (child->getOperand(i) != body.getArgument(i)) return error() << "entry boundary public input is not directly connected";
  for (unsigned i = 0; i < outputs; ++i)
    if (body.back().getOperand(i) != child->getResult(i)) return error() << "entry boundary public output is not directly connected";
  auto childLayout = getModulePortLayout(logical);
  if (failed(childLayout)) return failure();
  for (unsigned i = 0; i < outputs; ++i) {
    int64_t from = childLayout->outputValidity[i], to = adapterLayout->outputValidity[i];
    if ((from < 0) != (to < 0) ||
        (from >= 0 && body.back().getOperand(to) != child->getResult(from)))
      return error() << "entry boundary output availability is not forwarded directly";
  }
  // Any extra allocation of this symbol would add a second executable boundary.
  auto uses = package.walk([&](Operation *op) {
    if (failed(debit(1, op))) return WalkResult::interrupt();
    if (auto instance = dyn_cast<InstanceOp>(op); instance && instance.getCallee() == entryBoundarySymbol) {
      instance.emitOpError("entry boundary may only be selected by the sole System"); return WalkResult::interrupt();
    }
    if (auto collection = dyn_cast<CollectionOp>(op); collection && collection.getCallee() == entryBoundarySymbol) {
      collection.emitOpError("entry boundary cannot be a collection callee"); return WalkResult::interrupt();
    }
    return WalkResult::advance();
  });
  return failure(uses.wasInterrupted());
}
}
