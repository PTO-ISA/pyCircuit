#include "SourceEntryBoundary.h"
#include "Dialect/ACIR/EntryBoundary.h"
#include "Dialect/ACIR/HardwareSourceCheckPreflight.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/STLExtras.h"
using namespace mlir;
namespace acir::compiler {
namespace {
using ac::detail::SourceCheckBudget;
LogicalResult prepay(ModuleOp package, SourceCheckBudget &budget) {
  // Match transaction lowering's bounded clone accounting, including every
  // block argument, before either native admission or allocating a clone.
  SmallVector<Operation *> pending;
  if (failed(budget.debit(1, package))) return failure();
  pending.push_back(package);
  while (!pending.empty()) {
    Operation *op = pending.pop_back_val();
    uint64_t units = SourceCheckBudget::add(3,
        SourceCheckBudget::multiply(op->getNumOperands(), 2));
    units = SourceCheckBudget::add(units,
        SourceCheckBudget::multiply(op->getNumResults(), 2));
    units = SourceCheckBudget::add(units, op->getNumRegions());
    units = SourceCheckBudget::add(units, op->getRawDictionaryAttrs().size());
    units = SourceCheckBudget::add(units, op->getPropertiesStorageSize());
    if (failed(budget.debit(units, op))) return failure();
    for (Region &region : op->getRegions())
      for (Block &block : region) {
        if (failed(budget.debit(SourceCheckBudget::add(1,
                SourceCheckBudget::multiply(block.getNumArguments(), 2)), op)))
          return failure();
        for (Operation &nested : block) {
          if (failed(budget.debit(1, op))) return failure();
          pending.push_back(&nested);
        }
      }
  }
  return success();
}
LogicalResult sameTree(Operation *left, Operation *right, IRMapping &mapping,
                       SourceCheckBudget &budget) {
  if (failed(budget.debit(16 + left->getNumResults(), left))) return failure();
  if (left->getName() != right->getName() || left->getLoc() != right->getLoc() ||
      left->getAttrs() != right->getAttrs() || left->getPropertiesAsAttribute() != right->getPropertiesAsAttribute() ||
      left->getResultTypes() != right->getResultTypes() || left->getNumRegions() != right->getNumRegions() ||
      left->getNumOperands() != right->getNumOperands()) return failure();
  mapping.map(left, right);
  for (auto [a, b] : llvm::zip(left->getResults(), right->getResults())) mapping.map(a, b);
  for (auto [a, b] : llvm::zip(left->getRegions(), right->getRegions())) {
    if (a.getBlocks().size() != b.getBlocks().size()) return failure();
    for (auto [x, y] : llvm::zip(a, b)) {
      if (x.getArgumentTypes() != y.getArgumentTypes() || x.getOperations().size() != y.getOperations().size()) return failure();
      mapping.map(&x, &y);
      for (auto [v, w] : llvm::zip(x.getArguments(), y.getArguments())) {
        if (v.getLoc() != w.getLoc()) return failure();
        mapping.map(v, w);
      }
      for (auto [v, w] : llvm::zip(x, y)) if (failed(sameTree(&v, &w, mapping, budget))) return failure();
    }
  }
  return success();
}
}
LogicalResult verifyEntryBoundaryNormalization(ModuleOp original, ModuleOp candidate) {
  ac::HardwareSourceCheckLimits limits;
  limits.maxWorkUnits = 1024 * 1024;
  SourceCheckBudget budget(limits);
  budget.setPhase("entry boundary normalization proof");
  if (!original || !candidate || failed(prepay(original, budget)) || failed(prepay(candidate, budget)) ||
      failed(mlir::verify(original)) || failed(mlir::verify(candidate))) return failure();
  auto debit = [&](uint64_t n, Operation *op) { return budget.debit(n, op); };
  ac::HardwareAnalysis before(original), after(candidate);
  if (failed(ac::detail::verifyEntryBoundary(before, true, debit))) return failure();
  auto originalSystems = original.getOps<ac::SystemOp>();
  auto candidateSystems = candidate.getOps<ac::SystemOp>();
  if (!llvm::hasSingleElement(originalSystems) || !llvm::hasSingleElement(candidateSystems)) return failure();
  auto oldSystem = *originalSystems.begin(), newSystem = *candidateSystems.begin();
  if (oldSystem->hasAttr("ac.entry_boundary")) return oldSystem.emitOpError("normalization input already has an entry boundary");
  auto applies = ac::detail::needsEntryBoundary(before, debit);
  if (failed(applies) || !*applies) return original.emitOpError("entry normalization applicability proof failed");
  if (failed(ac::detail::verifyEntryBoundary(after, true, debit))) return failure();
  auto marker = newSystem->getAttrOfType<DictionaryAttr>("ac.entry_boundary");
  if (!marker || marker.getAs<DictionaryAttr>("logical_entry") != oldSystem.getEntry() ||
      original->getAttrs() != candidate->getAttrs() || original.getLoc() != candidate.getLoc() ||
      original->getPropertiesAsAttribute() != candidate->getPropertiesAsAttribute())
    return candidate.emitOpError("entry normalization changed the original package or logical entry");
  SmallVector<Operation *> retained;
  for (Operation &op : *candidate.getBody()) {
    auto module = dyn_cast<ac::ModuleOp>(op);
    if (module && module.getSymName() == ac::detail::entryBoundarySymbol) continue;
    if (failed(budget.debit(8, &op))) return failure();
    retained.push_back(&op);
  }
  if (retained.size() != original.getBody()->getOperations().size())
    return candidate.emitOpError("entry normalization original definition inventory differs");
  IRMapping mapping;
  for (auto [a, b] : llvm::zip(*original.getBody(), retained)) {
    if (&a == oldSystem.getOperation()) {
      OwningOpRef<ac::SystemOp> expected(cast<ac::SystemOp>(a.clone()));
      expected->setEntryAttr(newSystem.getEntry());
      (*expected)->setAttr("ac.entry_boundary", marker);
      IRMapping local;
      if (failed(sameTree(expected->getOperation(), b, local, budget)))
        return b->emitOpError("entry normalization changed original System fields");
      mapping.map(&a, b);
    } else if (failed(sameTree(&a, b, mapping, budget)))
      return b->emitOpError("entry normalization changed an original definition");
  }
  // Independently compare SSA and CFG wiring after the complete one-to-one map
  // exists. The producer supplies no map, proof flag or resolved-type authority.
  auto wiring = original.walk([&](Operation *op) {
    auto *copy = mapping.lookupOrNull(op);
    if (op == original.getOperation()) return WalkResult::advance();
    if (!copy || failed(budget.debit(op->getNumOperands() + op->getNumSuccessors() + 1, op))) return WalkResult::interrupt();
    for (auto [a, b] : llvm::zip(op->getOperands(), copy->getOperands()))
      if (mapping.lookupOrNull(a) != b) return WalkResult::interrupt();
    if (op->getNumSuccessors() != copy->getNumSuccessors()) return WalkResult::interrupt();
    for (auto [a, b] : llvm::zip(op->getSuccessors(), copy->getSuccessors()))
      if (mapping.lookupOrNull(a) != b) return WalkResult::interrupt();
    return WalkResult::advance();
  });
  if (wiring.wasInterrupted()) return candidate.emitOpError("entry normalization changed original SSA/CFG wiring");
  return success();
}
FailureOr<OwningOpRef<ModuleOp>> normalizeEntryBoundary(ModuleOp original) {
  // Applicability is an admission query, not a transformation. Preserve the
  // existing ordinary-graph limit and original operation identity when this
  // narrow normalization does not apply.
  SourceCheckBudget admission(ac::HardwareSourceCheckLimits{});
  admission.setPhase("entry boundary applicability");
  if (!original || failed(prepay(original, admission)) ||
      failed(mlir::verify(original))) return failure();
  ac::HardwareAnalysis analysis(original);
  auto debit = [&](uint64_t n, Operation *op) { return admission.debit(n, op); };
  if (failed(ac::detail::verifyEntryBoundary(analysis, true, debit))) return failure();
  auto applies = ac::detail::needsEntryBoundary(analysis, debit);
  if (failed(applies)) return failure();
  if (!*applies) return OwningOpRef<ModuleOp>();
  ac::HardwareSourceCheckLimits limits;
  limits.maxWorkUnits = 1024 * 1024;
  SourceCheckBudget budget(limits);
  budget.setPhase("entry boundary normalization");
  if (failed(prepay(original, budget))) return failure();
  OwningOpRef<ModuleOp> candidate(cast<ModuleOp>(original->clone()));
  auto bindings = ac::detail::resolveHardwareRootBindings(analysis);
  if (failed(bindings)) return failure();
  auto root = cast<ac::ModuleOp>(bindings->owner);
  auto origin = root->getAttrOfType<DictionaryAttr>("ac.origin");
  auto location = dyn_cast<FileLineColLoc>(root.getLoc());
  if (!origin || !location) return root.emitOpError("entry normalization requires source declaration origin and FileLineColLoc");
  Builder attrs(original.getContext());
  SmallVector<Type> inputs, outputs;
  for (auto [types, resolved] : {std::pair<TypeRange, SmallVector<Type> *>{root.getFunctionType().getInputs(), &inputs},
                               {root.getFunctionType().getResults(), &outputs}})
    for (Type type : types) {
      auto concrete = analysis.resolveType(type, *bindings, root);
      if (failed(concrete)) return failure();
      resolved->push_back(*concrete);
    }
  OperationState state(root.getLoc(), ac::ModuleOp::getOperationName());
  state.addAttribute("sym_name", attrs.getStringAttr(ac::detail::entryBoundarySymbol));
  state.addAttribute("source_owner", root.getSourceOwner());
  state.addAttribute("parameters", attrs.getArrayAttr({}));
  state.addAttribute("type_parameters", attrs.getArrayAttr({}));
  state.addAttribute("function_type", TypeAttr::get(FunctionType::get(original.getContext(), inputs, outputs)));
  state.addAttribute("input_names", root.getInputNames());
  state.addAttribute("output_names", root.getOutputNames());
  if (auto domain = root->getAttr("ac.domain_inputs")) state.addAttribute("ac.domain_inputs", domain);
  state.addRegion();
  auto adapter = cast<ac::ModuleOp>(Operation::create(state));
  candidate->getBody()->push_back(adapter);
  auto *body = new Block;
  adapter.getBody().push_back(body);
  for (Type type : inputs) body->addArgument(type, root.getLoc());
  auto span = attrs.getDictionaryAttr({attrs.getNamedAttr("path", location.getFilename()),
      attrs.getNamedAttr("line", attrs.getI64IntegerAttr(location.getLine())),
      attrs.getNamedAttr("column", attrs.getI64IntegerAttr(location.getColumn())),
      attrs.getNamedAttr("end_line", attrs.getI64IntegerAttr(location.getLine())),
      attrs.getNamedAttr("end_column", attrs.getI64IntegerAttr(location.getColumn()))});
  SmallVector<Attribute> actuals;
  for (Attribute raw : root.getParameters()) {
    auto name = cast<DictionaryAttr>(raw).getAs<StringAttr>("name");
    actuals.push_back(ac::StaticExprAttr::get(original.getContext(), attrs.getDictionaryAttr({
        attrs.getNamedAttr("kind", attrs.getStringAttr("literal")),
        attrs.getNamedAttr("value", attrs.getDictionaryAttr({attrs.getNamedAttr("kind", attrs.getStringAttr("integer")),
            attrs.getNamedAttr("value", bindings->integers.lookup(name.getValue()))})),
        attrs.getNamedAttr("origin", origin), attrs.getNamedAttr("location", span)})));
  }
  NamedAttrList occurrence(origin), site(origin.getAs<DictionaryAttr>("site"));
  site.set("definition", FlatSymbolRefAttr::get(original.getContext(), ac::detail::entryBoundarySymbol));
  occurrence.set("site", attrs.getDictionaryAttr(site.getAttrs()));
  OperationState childState(root.getLoc(), ac::InstanceOp::getOperationName());
  childState.addOperands(body->getArguments()); childState.addTypes(outputs);
  childState.addAttribute("instance_name", attrs.getStringAttr("entry"));
  childState.addAttribute("callee", FlatSymbolRefAttr::get(original.getContext(), root.getSymName()));
  childState.addAttribute("parameters", attrs.getArrayAttr(actuals));
  childState.addAttribute("type_arguments", attrs.getArrayAttr({}));
  childState.addAttribute("occurrence", attrs.getDictionaryAttr(occurrence.getAttrs()));
  auto *child = Operation::create(childState); body->push_back(child);
  OperationState yieldState(root.getLoc(), ac::YieldOp::getOperationName());
  yieldState.addOperands(child->getResults()); body->push_back(Operation::create(yieldState));
  auto system = *candidate->getOps<ac::SystemOp>().begin();
  auto entry = system.getEntry();
  system->setAttr("ac.entry_boundary", attrs.getDictionaryAttr({
      attrs.getNamedAttr("version", attrs.getI64IntegerAttr(1)), attrs.getNamedAttr("logical_entry", entry)}));
  system.setEntryAttr(attrs.getDictionaryAttr({attrs.getNamedAttr("callee", FlatSymbolRefAttr::get(original.getContext(), ac::detail::entryBoundarySymbol)),
      attrs.getNamedAttr("parameters", attrs.getArrayAttr({})), attrs.getNamedAttr("type_arguments", attrs.getArrayAttr({}))}));
  if (failed(verifyEntryBoundaryNormalization(original, *candidate))) return failure();
  return std::move(candidate);
}
}
