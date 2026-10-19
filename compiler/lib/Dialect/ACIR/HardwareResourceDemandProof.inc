// Reconstruct resource diagnostic demand from the complete ordered control
// forest. This verifier does not use materializer locators or source plans.
static FailureOr<SmallVector<Value, 0>> verifyResourceDemandReach(
    const SampledAdmissionView &view, const HardwareAnalysis &analysis,
    SourceCheckBudget &budget, OrderedPrefixProof &proof) {
  SmallVector<Value, 0> reaches;
  if (failed(charge(budget, view.rule, view.controls.size(), sizeof(Value))))
    return failure();
  for (const auto &control : view.controls) reaches.push_back(control.reached);
  if (view.resources.empty()) return reaches;
  auto error = [&](StringRef message) -> LogicalResult {
    return view.rule->emitOpError() << "resource demand " << message;
  };
  auto normalized = [&](Value value) { return proof.normalize(value); };
  auto same = [&](Value lhs, Value rhs) -> FailureOr<bool> {
    auto a = normalized(lhs), b = normalized(rhs);
    if (failed(a) || failed(b)) return failure();
    return sameEndpoint(*a, *b);
  };
  auto isLiteral = [&](Value value, bool wanted) -> LogicalResult {
    auto endpoint = normalized(value);
    if (failed(endpoint)) return failure();
    auto known = literal(*endpoint, analysis, budget);
    if (failed(known)) return failure();
    return success(known->has_value() && **known == wanted);
  };
  auto binary = [&](Value value) -> FailureOr<BitsBinaryOp> {
    auto endpoint = normalized(value);
    if (failed(endpoint)) return failure();
    auto op = endpoint->value.getDefiningOp<BitsBinaryOp>();
    if (!op || op.getOpcode() != "and") {
      (void)error("requires its canonical conjunction"); return failure();
    }
    return op;
  };
  auto rawSelector = [&](Value value, uint64_t selection, bool positive)
      -> LogicalResult {
    if (selection >= view.controls.size() ||
        view.controls[selection].kind != SampledControlKind::BranchSelect)
      return error("has an invalid selector ancestor");
    if (!positive) {
      auto endpoint = normalized(value);
      if (failed(endpoint)) return failure();
      auto op = endpoint->value.getDefiningOp<BitsUnaryOp>();
      if (!op || op.getOpcode() != "not")
        return error("false ancestry requires the actual raw selector complement");
      value = op.getInput();
    }
    auto equal = same(value, view.controls[selection].raw);
    return succeeded(equal) && *equal ? success()
        : error("ancestry differs from the actual raw selector");
  };
  struct ResourceLeaf { Value predicate, authored; };
  auto leaf = [&](uint64_t ordinal) -> FailureOr<ResourceLeaf> {
    const auto &control = view.controls[ordinal];
    auto endpoint = normalized(control.raw);
    if (failed(endpoint)) return failure();
    auto select = endpoint->value.getDefiningOp<BitsSelectOp>();
    if (!select || failed(isLiteral(select.getFalseValue(), true))) {
      (void)error("resource requires the existing path-selected predicate");
      return failure();
    }
    // rawStatementPath ends in AND(ancestry, authoredPath). Peel the complete
    // structural ancestry, leaving only the independently admitted local path.
    auto path = binary(select.getCondition());
    if (failed(path)) return failure();
    Value ancestry = path->getLhs(), authored = path->getRhs();
    SmallVector<SampledPathRef, 0> parents;
    auto cursor = control.executionPath;
    while (cursor.selectionControl) {
      if (parents.size() >= 128 ||
          failed(charge(budget, view.rule, 1, sizeof(SampledPathRef))))
        return failure();
      parents.push_back(cursor);
      cursor = view.controls[*cursor.selectionControl].executionPath;
    }
    for (const auto &parent : llvm::reverse(parents)) {
      auto part = binary(ancestry);
      if (failed(part) || failed(rawSelector(part->getRhs(),
              *parent.selectionControl, parent.whenTrue))) return failure();
      ancestry = part->getLhs();
    }
    if (failed(isLiteral(ancestry, true))) {
      (void)error("raw path has missing, repeated or extra ancestry");
      return failure();
    }
    return ResourceLeaf{select.getTrueValue(), authored};
  };

  const size_t count = view.controls.size();
  if (failed(charge(budget, view.rule, 1 + 2 * count,
                    sizeof(SmallVector<uint64_t, 0>)))) return failure();
  SmallVector<SmallVector<uint64_t, 0>, 0> blocks(1 + 2 * count);
  auto armBlock = [](uint64_t branch, bool positive) {
    return 1 + 2 * branch + (positive ? 0 : 1);
  };
  for (auto [i, control] : llvm::enumerate(view.controls)) {
    auto path = control.executionPath;
    uint64_t block = path.selectionControl
        ? armBlock(*path.selectionControl, path.whenTrue) : 0;
    if (block >= blocks.size() ||
        failed(charge(budget, view.rule, 1, sizeof(uint64_t)))) return failure();
    blocks[block].push_back(i);
  }
  // A path cannot leave an arm and later reenter it. Reconstruct the complete
  // lexical forest and require exactly the original control order and coverage.
  uint64_t next = 0;
  std::function<LogicalResult(uint64_t, unsigned)> ordered =
      [&](uint64_t block, unsigned depth) -> LogicalResult {
    if (depth > 128) return error("control forest is too deep");
    for (uint64_t ordinal : blocks[block]) {
      if (failed(budget.debit(1, view.rule))) return failure();
      if (ordinal != next++) return error("control forest is not contiguous");
      if (view.controls[ordinal].kind == SampledControlKind::BranchSelect &&
          (failed(ordered(armBlock(ordinal, true), depth + 1)) ||
           failed(ordered(armBlock(ordinal, false), depth + 1)))) return failure();
    }
    return success();
  };
  if (failed(ordered(0, 0)) || next != count) return failure();

  enum class Kind { One, And, Boundary, Select };
  struct Node { Kind kind; Value raw; uint64_t yes, no; };
  SmallVector<Node, 0> nodes;
  if (failed(charge(budget, view.rule, 1, sizeof(Node)))) return failure();
  nodes.push_back({Kind::One, {}, 0, 0});
  auto append = [&](Kind kind, Value raw, uint64_t yes,
                    uint64_t no = 0) -> FailureOr<uint64_t> {
    if ((kind == Kind::Boundary && yes == 0) ||
        (kind == Kind::Select && yes == no)) return yes;
    if (failed(charge(budget, view.rule, 1, sizeof(Node)))) return failure();
    nodes.push_back({kind, raw, yes, no});
    return nodes.size() - 1;
  };
  std::function<FailureOr<uint64_t>(uint64_t, uint64_t, unsigned)> fold =
      [&](uint64_t block, uint64_t continuation, unsigned depth)
          -> FailureOr<uint64_t> {
    if (depth > 128) return failure();
    for (uint64_t ordinal : llvm::reverse(blocks[block])) {
      if (failed(budget.debit(1, view.rule))) return failure();
      const auto &control = view.controls[ordinal];
      FailureOr<uint64_t> result = failure();
      if (control.kind == SampledControlKind::BranchSelect) {
        auto yes = fold(armBlock(ordinal, true), continuation, depth + 1);
        auto no = fold(armBlock(ordinal, false), continuation, depth + 1);
        if (failed(yes) || failed(no)) return failure();
        result = append(Kind::Select, control.raw, *yes, *no);
      } else if (control.kind == SampledControlKind::ResourceAvailable) {
        auto metadata = llvm::find_if(view.resources, [&](const auto &resource) {
          return resource.control == ordinal;
        });
        if (metadata == view.resources.end()) return failure();
        // Memory snapshots admit forward execution. They do not suppress
        // already-reached diagnostics through Queue's speculative demand.
        if (metadata->kind.getValue() == "memory_live") continue;
        auto resource = leaf(ordinal);
        if (failed(resource)) return failure();
        auto required = append(Kind::And, resource->predicate, continuation);
        if (failed(required)) return failure();
        result = append(Kind::Select, resource->authored, *required, continuation);
      } else {
        result = append(Kind::Boundary, control.raw, continuation);
      }
      if (failed(result)) return failure();
      continuation = *result;
    }
    return continuation;
  };
  auto expected = fold(0, 0, 0);
  if (failed(expected)) return failure();

  llvm::DenseSet<Operation *> privateNodes, wrappers;
  auto retain = [&](Operation *op) -> LogicalResult {
    if (!op || op->getBlock() != &view.rule->getRegion(0).front())
      return error("diagnostic graph must stay inside its Rule");
    if (failed(charge(budget, view.rule, 1, sizeof(Operation *) * 2)))
      return failure();
    privateNodes.insert(op);
    return success();
  };
  Value mayExecute;
  for (auto [ordinal, control] : llvm::enumerate(view.controls)) {
    auto wrapper = binary(control.reached);
    if (failed(wrapper)) return failure();
    if (failed(retain(wrapper->getOperation()))) return failure();
    wrappers.insert(wrapper->getOperation());
    if (!mayExecute) mayExecute = wrapper->getRhs();
    auto equal = same(mayExecute, wrapper->getRhs());
    if (failed(equal) || !*equal) {
      (void)error("checks do not share one diagnostic gate"); return failure();
    }
    reaches[ordinal] = wrapper->getLhs();
  }
  auto may = normalized(mayExecute);
  if (failed(may)) return failure();
  auto invert = may->value.getDefiningOp<BitsUnaryOp>();
  if (!invert || invert.getOpcode() != "not" || failed(retain(invert))) {
    (void)error("requires NOT case_eq(H, zero)"); return failure();
  }
  auto tested = normalized(invert.getInput());
  if (failed(tested)) return failure();
  auto compare = tested->value.getDefiningOp<BitsCompareOp>();
  if (!compare || compare.getPredicate() != "case_eq" ||
      failed(isLiteral(compare.getRhs(), false)) || failed(retain(compare))) {
    (void)error("possible execution must retain unknown resource demand");
    return failure();
  }
  if (failed(charge(budget, view.rule, nodes.size(), sizeof(Value)))) return failure();
  SmallVector<Value, 0> matched(nodes.size());
  SmallVector<std::pair<uint64_t, Value>, 0> pending;
  auto push = [&](uint64_t node, Value actual) -> LogicalResult {
    if (failed(charge(budget, view.rule, 1, sizeof(std::pair<uint64_t, Value>))))
      return failure();
    pending.push_back({node, actual}); return success();
  };
  if (failed(push(*expected, compare.getLhs()))) return failure();
  while (!pending.empty()) {
    auto [id, actual] = pending.pop_back_val();
    if (failed(budget.debit(1, view.rule))) return failure();
    if (matched[id]) {
      auto equal = same(matched[id], actual);
      if (failed(equal) || !*equal) {
        (void)error("continuation is not shared by its arms"); return failure();
      }
      continue;
    }
    matched[id] = actual;
    const auto &node = nodes[id];
    if (node.kind == Kind::One) {
      if (failed(isLiteral(actual, true))) {
        (void)error("continuation must start at one"); return failure();
      }
      continue;
    }
    auto endpoint = normalized(actual);
    if (failed(endpoint)) return failure();
    if (node.kind == Kind::And) {
      auto op = binary(actual);
      if (failed(op) || failed(retain(op->getOperation()))) return failure();
      auto equal = same(op->getLhs(), node.raw);
      if (failed(equal) || !*equal || failed(push(node.yes, op->getRhs()))) {
        (void)error("resource leaf differs from its actual predicate"); return failure();
      }
      continue;
    }
    auto select = endpoint->value.getDefiningOp<BitsSelectOp>();
    if (!select || failed(retain(select))) {
      (void)error("requires its canonical raw branch/boundary select"); return failure();
    }
    Value condition = select.getCondition();
    if (node.kind == Kind::Boundary) {
      auto known = normalized(condition);
      if (failed(known)) return failure();
      auto op = known->value.getDefiningOp<BitsCompareOp>();
      if (!op || op.getPredicate() != "case_eq" ||
          failed(isLiteral(op.getRhs(), true)) || failed(retain(op))) {
        (void)error("explicit guard boundary requires known-one"); return failure();
      }
      condition = op.getLhs();
    }
    auto equal = same(condition, node.raw);
    if (failed(equal) || !*equal ||
        failed(push(node.yes, select.getTrueValue())) ||
        failed(push(node.no, select.getFalseValue()))) {
      (void)error("raw selector or guard bypass differs"); return failure();
    }
  }
  // Only the designated check-result slots may export diagnostic nodes. In
  // particular H/mayExecute may not feed payload, final fire or proposals.
  auto yield = cast<YieldOp>(view.rule->getRegion(0).front().getTerminator());
  for (Operation *op : privateNodes)
    for (OpOperand &use : op->getResult(0).getUses()) {
      if (failed(budget.debit(1, view.rule))) return failure();
      if (privateNodes.contains(use.getOwner())) continue;
      bool checkSlot = use.getOwner() == yield && wrappers.contains(op) &&
          llvm::any_of(view.controls, [&](const auto &control) {
            return control.reachedResult == use.getOperandNumber() &&
                   control.reached == op->getResult(0);
          });
      if (!checkSlot) {
        (void)error("diagnostic graph escapes its private check slots"); return failure();
      }
    }
  return reaches;
}
