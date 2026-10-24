// Physical refinement is a separate transformation of independently verified
// routed IR. None of the source preparation's borrowed records enter this proof.
struct HardwareStatePrimitives {
  ac::ModuleImportOp dff, dffe;
};

FailureOr<HardwareStatePrimitives>
hardwareStatePrimitives(mlir::ModuleOp routed, SourceCheckBudget &budget) {
  ac::HardwareAnalysis analysis(routed);
  HardwareStatePrimitives result;
  unsigned dffeCount = 0;
  for (auto imported : routed.getOps<ac::ModuleImportOp>()) {
    if (!charge(budget, imported) || failed(analysis.verifyImport(imported)))
      return failure();
    StringRef kind = analysis.getPrimitiveKind(imported);
    if (kind.empty())
      return imported.emitOpError()
             << "hardware refinement requires an implementation for an ordinary external module";
    if (kind == "dff" && !result.dff)
      result.dff = imported;
    if (kind == "dffe") {
      ++dffeCount;
      result.dffe = imported;
    }
  }
  bool hasState = false;
  for (auto module : routed.getOps<ac::ModuleOp>())
    for (auto state : module.getBody().front().getOps<ac::SourceStateOp>()) {
      if (!charge(budget, state)) return failure();
      hasState = true;
    }
  if (hasState && (!result.dffe || dffeCount != 1))
    return routed.emitError()
           << "hardware state refinement requires exactly one verified DFFE primitive";
  return result;
}

FailureOr<ac::StateUpdateOp> logicalUpdate(ac::SourceStateOp state,
                                         SourceCheckBudget &budget) {
  ac::StateUpdateOp result;
  for (auto *user : state.getQ().getUsers()) {
    if (!charge(budget, state)) return failure();
    auto update = dyn_cast<ac::StateUpdateOp>(user);
    if (!update || update.getState() != state.getQ()) continue;
    // One operation may use Q in both the state and candidate positions.
    if (result && result != update)
      return state.emitOpError() << "logical state has multiple update bindings";
    result = update;
  }
  if (!result)
    return state.emitOpError() << "logical state has no update binding";
  return result;
}

bool unconditionalScalarState(ac::SourceStateOp state, ac::StateUpdateOp update,
                              const ac::HardwareAnalysis &analysis,
                              SourceCheckBudget &budget) {
  if (isa<ac::TableType>(state.getQ().getType())) return false;
  auto rule = update.getIntent().getDefiningOp<ac::RuleOp>();
  return rule && rule->getParentOp() == state->getParentOp() &&
         knownTrue(update.getIntent(), analysis, budget);
}

// Derive lexical correspondence from the two live graphs. A materializer map is
// not accepted: every inserted operation must occupy the exact storage slot,
// and all unaffected operations, regions and SSA edges remain exact clones.
class HardwareStateProof {
public:
  HardwareStateProof(mlir::ModuleOp routed, mlir::ModuleOp candidate,
                     SourceCheckBudget &budget,
                     const HardwareStatePrimitives &primitives)
      : routed(routed), candidate(candidate), budget(budget),
        analysis(routed), primitives(primitives) {}

  LogicalResult run() {
    if (failed(index(routed, candidate))) return failure();
    for (auto [source, target] : operations)
      if (failed(exact(source, target))) return failure();
    for (const auto &pair : states)
      if (failed(stateProof(pair))) return failure();
    return success();
  }

private:
  struct StatePair {
    ac::SourceStateOp source;
    Operation *target;
    SmallVector<ac::TableSplatOp, 3> splats;
  };
  LogicalResult reject(StringRef message) {
    return routed.emitError() << "hardware state refinement proof: " << message;
  }

  LogicalResult index(Operation *source, Operation *target) {
    if (!charge(budget, routed, 16 + 4 * source->getNumResults()) ||
        source->getNumResults() != target->getNumResults() ||
        source->getNumRegions() != target->getNumRegions())
      return reject("operation/result/region inventory differs");
    operations.push_back({source, target});
    mapping.map(source, target);
    for (auto [from, to] : llvm::zip(source->getResults(), target->getResults()))
      mapping.map(from, to);
    for (auto [from, to] : llvm::zip(source->getRegions(), target->getRegions())) {
      if (from.getBlocks().size() != to.getBlocks().size())
        return reject("region block inventory differs");
      for (auto [left, right] : llvm::zip(from, to)) {
        if (!charge(budget, routed, 4 + 4 * left.getNumArguments()) ||
            left.getNumArguments() != right.getNumArguments())
          return reject("block argument inventory differs");
        mapping.map(&left, &right);
        for (auto [a, b] : llvm::zip(left.getArguments(), right.getArguments())) {
          if (a.getType() != b.getType() || a.getLoc() != b.getLoc())
            return reject("block argument type/location differs");
          mapping.map(a, b);
        }
        auto next = right.begin();
        for (Operation &op : left) {
          if (!charge(budget, &op)) return failure();
          if (isa<ac::StateUpdateOp>(op)) continue;
          if (auto state = dyn_cast<ac::SourceStateOp>(op)) {
            StatePair pair{state, nullptr, {}};
            if (isa<ac::TableType>(state.getQ().getType()))
              for (unsigned i = 0; i < 3; ++i) {
                if (next == right.end()) return reject("missing Table control splat");
                auto splat = dyn_cast<ac::TableSplatOp>(&*next++);
                if (!splat) return reject("Table state control is not a splat");
                pair.splats.push_back(splat);
              }
            if (next == right.end()) return reject("missing physical state");
            pair.target = &*next++;
            if (pair.target->getNumResults() != 1 ||
                pair.target->getNumRegions() != 0 ||
                pair.target->getNumSuccessors() != 0)
              return reject("physical state shape differs");
            if (!charge(budget, state, 24)) return failure();
            mapping.map(state.getOperation(), pair.target);
            mapping.map(state.getQ(), pair.target->getResult(0));
            states.push_back(std::move(pair));
          } else {
            if (next == right.end()) return reject("missing unchanged operation");
            if (failed(index(&op, &*next++))) return failure();
          }
        }
        if (next != right.end()) return reject("extra physical operation");
      }
    }
    return success();
  }

  LogicalResult exact(Operation *source, Operation *target) {
    if (!charge(budget, source, 8 + 2 * source->getNumOperands() +
                                  source->getAttrs().size()))
      return failure();
    NamedAttrList expected(source->getAttrs());
    if (source == routed.getOperation())
      expected.set("ac.stage", StringAttr::get(source->getContext(), "final"));
    if (source->getName() != target->getName() ||
        expected.getAttrs() != target->getAttrs() ||
        source->getPropertiesAsAttribute() != target->getPropertiesAsAttribute() ||
        source->getLoc() != target->getLoc() ||
        source->getResultTypes() != target->getResultTypes() ||
        source->getNumOperands() != target->getNumOperands() ||
        source->getNumSuccessors() != target->getNumSuccessors())
      return reject("unchanged operation shape/attributes/properties differ");
    for (auto [a, b] : llvm::zip(source->getOperands(), target->getOperands()))
      if (mapping.lookupOrNull(a) != b)
        return reject("unchanged SSA edge differs");
    for (auto [a, b] : llvm::zip(source->getSuccessors(), target->getSuccessors()))
      if (mapping.lookupOrNull(a) != b)
        return reject("unchanged successor differs");
    return success();
  }

  LogicalResult stateProof(const StatePair &pair) {
    auto state = pair.source;
    auto update = logicalUpdate(state, budget);
    if (failed(update)) return failure();
    auto table = dyn_cast<ac::TableType>(state.getQ().getType());
    // Re-derive the restricted optimization from actual routed SSA. Neither a
    // producer flag nor the candidate primitive's missing enable is authority.
    bool dff = unconditionalScalarState(state, *update, analysis, budget);
    auto primitive = dff ? primitives.dff : primitives.dffe;
    if (!primitive) return reject("required physical primitive is absent");
    auto *target = pair.target;
    Builder b(routed.getContext());
    NamedAttrList attributes;
    attributes.append("instance_name", state.getInstanceNameAttr());
    attributes.append("occurrence", state.getOccurrence());
    attributes.append("callee", FlatSymbolRefAttr::get(primitive.getSymNameAttr()));
    attributes.append("parameters", b.getArrayAttr({}));
    attributes.append("type_arguments", b.getArrayAttr({TypeAttr::get(
        table ? table.getElementType() : state.getQ().getType())}));
    if (table) attributes.append("shape", table.getShape());
    SmallVector<Value> inputs{mapping.lookup(state.getClk()),
                              mapping.lookup(state.getRst())};
    if (!dff) inputs.push_back(mapping.lookup(update->getIntent()));
    inputs.push_back(mapping.lookup(update->getCandidate()));
    inputs.push_back(mapping.lookup(state.getInitial()));
    if (table) {
      if (pair.splats.size() != 3) return reject("Table splat inventory differs");
      for (unsigned i = 0; i < 3; ++i) {
        auto splat = pair.splats[i];
        auto type = ac::TableType::get(state.getContext(), table.getShape(),
                                       inputs[i].getType());
        if (!charge(budget, state, 16) || splat.getInput() != inputs[i] ||
            splat->getNumOperands() != 1 || splat->getNumResults() != 1 ||
            splat.getResult().getType() != type ||
            splat.getShape() != table.getShape() ||
            splat->getAttrs() != b.getDictionaryAttr(
                {b.getNamedAttr("shape", table.getShape())}).getValue() ||
            splat.getLoc() != state.getLoc())
          return reject("Table control splat differs");
        inputs[i] = splat.getResult();
      }
    }
    if (target->getName().getStringRef() !=
            (table ? ac::CollectionOp::getOperationName()
                   : ac::InstanceOp::getOperationName()) ||
        target->getAttrDictionary() != attributes.getDictionary(state.getContext()))
      return reject("physical allocation kind/identity/type arguments differ");
    if (target->getResult(0).getType() != state.getQ().getType() ||
        target->getLoc() != state.getLoc())
      return reject("physical allocation payload type/location differs");
    if (!llvm::equal(target->getOperands(), inputs))
      return reject("physical allocation domain/initial/candidate/intent differs");
    return success();
  }

  mlir::ModuleOp routed, candidate;
  SourceCheckBudget &budget;
  ac::HardwareAnalysis analysis;
  const HardwareStatePrimitives &primitives;
  IRMapping mapping;
  SmallVector<std::pair<Operation *, Operation *>, 0> operations;
  SmallVector<StatePair, 0> states;
};
