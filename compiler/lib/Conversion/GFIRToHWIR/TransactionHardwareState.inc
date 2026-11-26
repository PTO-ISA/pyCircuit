// Physical refinement is a separate transformation of independently verified
// routed IR. None of the source preparation's borrowed records enter this proof.
struct HardwareStatePrimitives {
  ac::ModuleImportOp dff, dffe, dffPolicy, dffePolicy;
  gfir::StateOp policyState;
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
    unsigned version = 1;
    if (kind == "dff" || kind == "dffe") {
      auto contract = ac::getRegisterContract(imported);
      if (failed(contract)) return failure();
      version = *contract;
    }
    if (version == 3) {
      if (kind == "dff") result.dffPolicy = imported;
      else result.dffePolicy = imported;
      continue;
    }
    if (kind == "dff" && !result.dff)
      result.dff = imported;
    if (kind == "dffe") {
      ++dffeCount;
      result.dffe = imported;
    }
  }
  bool hasState = false;
  for (auto module : routed.getOps<ac::ModuleOp>())
    for (auto state : module.getBody().front().getOps<gfir::StateOp>()) {
      if (!charge(budget, state)) return failure();
      auto contract = routed->getAttrOfType<IntegerAttr>("ac.routed_contract");
      if (contract && contract.getInt() == 7) result.policyState = state;
      else hasState = true;
    }
  if (hasState && (!result.dffe || dffeCount != 1))
    return routed.emitError()
           << "hardware state refinement requires exactly one verified DFFE primitive";
  return result;
}

// Canonical physical imports are a refinement product, never a logical
// anchor requirement. Their types remain nominally owned by the new symbol.
Operation *registerPolicyImport(gfir::StateOp state, bool dff) {
  Builder b(state.getContext());
  StringRef symbol = dff ? "pycircuit.__builtins__.dff_policy"
                         : "pycircuit.__builtins__.dffe_policy";
  auto payload = ac::TypeParamType::get(state.getContext(),
      FlatSymbolRefAttr::get(state.getContext(), symbol), b.getStringAttr("T"));
  auto bit = ac::BitsType::get(state.getContext(),
      bitsLiteralAttribute(state.getClk().getType(), state.getOccurrence(), state.getContext(), true));
  SmallVector<Type> types{bit, bit};
  SmallVector<Attribute> names{b.getStringAttr("clk"), b.getStringAttr("rst")};
  if (!dff) { types.push_back(bit); names.push_back(b.getStringAttr("en")); }
  for (StringRef name : {"d", "init", "reset_value"}) {
    types.push_back(payload); names.push_back(b.getStringAttr(name));
  }
  OperationState op(state.getLoc(), ac::ModuleImportOp::getOperationName());
  op.addAttribute("sym_name", b.getStringAttr(symbol));
  op.addAttribute("source_owner", b.getDictionaryAttr({
      b.getNamedAttr("package", b.getStringAttr("pycircuit")),
      b.getNamedAttr("path", b.getStringAttr("__builtins__.py"))}));
  op.addAttribute("parameters", b.getArrayAttr({}));
  op.addAttribute("type_parameters", b.getArrayAttr({b.getStringAttr("T")}));
  op.addAttribute("function_type", TypeAttr::get(FunctionType::get(state.getContext(), types, {payload})));
  op.addAttribute("input_names", b.getArrayAttr(names));
  op.addAttribute("output_names", b.getArrayAttr({b.getStringAttr("q")}));
  op.addAttribute("semantic_kind", b.getStringAttr("hardware"));
  op.addAttribute("primitive_kind", b.getStringAttr(dff ? "dff" : "dffe"));
  op.addAttribute("ac.register_contract", b.getI64IntegerAttr(3));
  auto output = b.getDictionaryAttr({b.getNamedAttr("port", b.getI64IntegerAttr(0)),
                                    b.getNamedAttr("path", b.getArrayAttr({}))});
  op.addAttribute("dependency_summary", b.getArrayAttr({b.getDictionaryAttr({
      b.getNamedAttr("output", output), b.getNamedAttr("inputs", b.getArrayAttr({}))})}));
  return Operation::create(op);
}

FailureOr<gfir::StateUpdateOp> logicalUpdate(gfir::StateOp state, SourceCheckBudget &budget) {
  auto paired = ac::admissionPairLogicalStateUpdate(state, budget);
  if (failed(paired)) return failure();
  return cast<gfir::StateUpdateOp>(paired->getOperation());
}

bool unconditionalScalarState(gfir::StateOp state,
                              acir::LogicalStateUpdateOpInterface update,
                              const ac::HardwareAnalysis &analysis,
                              SourceCheckBudget &budget) {
  if (isa<ac::TableType>(state.getQ().getType())) return false;
  auto rule = update.getStateWriteIntent().getDefiningOp<ac::RuleOp>();
  return rule && rule->getParentOp() == state->getParentOp() &&
         knownTrue(update.getStateWriteIntent(), analysis, budget);
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
    gfir::StateOp source;
    Operation *target;
    SmallVector<ac::TableSplatOp, 3> splats;
    ac::BitsConstantOp resetZero;
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
        if (source == routed.getOperation() && primitives.policyState) {
          for (bool dff : {true, false}) {
            if (dff ? bool(primitives.dffPolicy) : bool(primitives.dffePolicy)) continue;
            if (next == right.end()) return reject("missing generated register import");
            auto *actual = &*next++;
            if (!charge(budget, routed, 128)) return failure();
            auto *expected = registerPolicyImport(primitives.policyState, dff);
            bool matches = actual->getName() == expected->getName() &&
                actual->getAttrs() == expected->getAttrs() &&
                actual->getLoc() == expected->getLoc() &&
                !actual->getNumOperands() && !actual->getNumResults() &&
                !actual->getNumRegions() && !actual->getNumSuccessors() &&
                actual->getPropertiesAsAttribute() == expected->getPropertiesAsAttribute();
            expected->destroy();
            if (!matches) return reject("generated register import differs");
          }
        }
        for (Operation &op : left) {
          if (!charge(budget, &op)) return failure();
          if (isa<gfir::StateUpdateOp>(op)) continue;
          if (auto state = dyn_cast<gfir::StateOp>(op)) {
            StatePair pair{state, nullptr, {}, {}};
            auto layout = ac::getStateLayout(state);
            if (failed(layout)) return failure();
            if (!layout->reset) {
              if (next == right.end()) return reject("missing reset-free physical tieoff");
              pair.resetZero = dyn_cast<ac::BitsConstantOp>(&*next++);
              if (!pair.resetZero || pair.resetZero->getResult(0).getType() != state.getClk().getType() ||
                  pair.resetZero.getValue() != bitsLiteralAttribute(state.getClk().getType(), state.getOccurrence(), state.getContext(), false) ||
                  pair.resetZero->getNumOperands() || pair.resetZero->getNumResults() != 1 ||
                  pair.resetZero->getAttrs().size() != 1 || pair.resetZero.getLoc() != state.getLoc())
                return reject("reset-free physical tieoff differs");
            }
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
    bool policyState = bool(state->getAttr("operandSegmentSizes"));
    auto primitive = policyState ? (dff ? primitives.dffPolicy : primitives.dffePolicy)
                                 : (dff ? primitives.dff : primitives.dffe);
    if (!primitive && !policyState) return reject("required physical primitive is absent");
    auto *target = pair.target;
    Builder b(routed.getContext());
    NamedAttrList attributes;
    attributes.append("instance_name", state.getInstanceNameAttr());
    attributes.append("occurrence", state.getOccurrence());
    auto symbol = primitive ? primitive.getSymNameAttr() : b.getStringAttr(
        dff ? "pycircuit.__builtins__.dff_policy" : "pycircuit.__builtins__.dffe_policy");
    attributes.append("callee", FlatSymbolRefAttr::get(symbol));
    attributes.append("parameters", b.getArrayAttr({}));
    attributes.append("type_arguments", b.getArrayAttr({TypeAttr::get(
        table ? table.getElementType() : state.getQ().getType())}));
    if (table) attributes.append("shape", table.getShape());
    if (policyState)
      for (StringRef name : {"edge", "reset_kind", "reset_polarity", "init_kind"})
        if (auto attr = state->getAttr(name)) attributes.append(name, attr);
    SmallVector<Value> inputs{mapping.lookup(state.getClk()),
                              state.getRst() ? mapping.lookup(state.getRst()) : pair.resetZero->getResult(0)};
    if (!dff) inputs.push_back(mapping.lookup(update->getIntent()));
    inputs.push_back(mapping.lookup(update->getCandidate()));
    inputs.push_back(mapping.lookup(state.getInitial() ? state.getInitial() : state.getQ()));
    if (policyState)
      inputs.push_back(mapping.lookup(state.getResetValue() ? state.getResetValue() : state.getQ()));
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
