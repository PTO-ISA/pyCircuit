// Legacy source capture adapter for the common scoped-node grant algebra.
class OwnerEnableProof {
public:
  explicit OwnerEnableProof(ModuleOp body, uint64_t spent)
      : analysis(body), work(spent) {
    proof = std::make_unique<ac::detail::HardwareGrantProof>(
        [this](Node node, unsigned depth) -> FailureOr<Node> {
          if (node >= nodes.size()) return failure();
          auto value = canonical(nodes[node], depth);
          if (failed(value)) return failure();
          return intern(*value);
        },
        [this](Node node) -> FailureOr<NodeView> { return inspect(node); },
        [this](uint64_t amount) { return charge(amount); });
  }
  FailureOr<bool> disjoint(Value first, Value second) {
    auto a = intern(first), b = intern(second);
    if (failed(a) || failed(b)) return failure();
    return proof->disjoint(*a, *b);
  }
  FailureOr<bool> disjointAtAddress(Value first, Value second, Value firstIndex, Value secondIndex) {
    auto a = intern(first), b = intern(second), i = intern(firstIndex), j = intern(secondIndex);
    if (failed(a) || failed(b) || failed(i) || failed(j)) return failure();
    return proof->disjointAtAddress(*a, *b, *i, *j);
  }
  bool chargeProduct(uint64_t first, uint64_t second) {
    if (first && second > 1048576 / first) return charge(1048577);
    return charge(first * second);
  }
  StringRef failureReason() const { return reason.empty() ? proof->failureReason() : StringRef(reason); }
  bool charge(uint64_t amount = 1) {
    if (!reason.empty()) return false;
    if (work > 1048576 || amount > 1048576 - work) {
      reason = "owner-enable proof work budget exhausted";
      return false;
    }
    work += amount;
    return true;
  }
private:
  using Node = ac::detail::HardwareGrantProof::Node;
  using NodeView = ac::detail::HardwareGrantProof::NodeView;
  void fail(StringRef message) { if (reason.empty()) reason = message.str(); }
  FailureOr<Node> intern(Value value) {
    if (!charge()) return failure();
    if (auto found = ids.find(value); found != ids.end()) return found->second;
    if (!charge(3)) return failure();
    Node node = nodes.size();
    nodes.push_back(value);
    ids.try_emplace(value, node);
    return node;
  }
  FailureOr<NodeView> inspect(Node node) {
    if (node >= nodes.size()) return failure();
    Value value = nodes[node];
    auto bits = width(value);
    auto known = constant(value, 0);
    if (failed(bits) || failed(known)) return failure();
    NodeView view;
    view.width = *bits;
    view.fullyKnown = *known;
    SmallVector<Value, 3> operands;
    if (auto unary = value.getDefiningOp<ac::BitsUnaryOp>(); unary && unary.getOpcode() == "not") {
      view.kind = NodeView::Not; operands.push_back(unary.getInput());
    } else if (auto binary = value.getDefiningOp<ac::BitsBinaryOp>(); binary &&
               (binary.getOpcode() == "and" || binary.getOpcode() == "or")) {
      view.kind = binary.getOpcode() == "and" ? NodeView::And : NodeView::Or;
      operands.append({binary.getLhs(), binary.getRhs()});
    } else if (auto select = value.getDefiningOp<ac::BitsSelectOp>()) {
      view.kind = NodeView::Select;
      operands.append({select.getCondition(), select.getTrueValue(), select.getFalseValue()});
    } else if (auto compare = value.getDefiningOp<ac::BitsCompareOp>(); compare &&
               (compare.getPredicate() == "eq" || compare.getPredicate() == "ne")) {
      view.kind = compare.getPredicate() == "eq" ? NodeView::Eq : NodeView::Ne;
      operands.append({compare.getLhs(), compare.getRhs()});
    }
    for (Value operand : operands) {
      auto bound = intern(operand);
      if (failed(bound)) return failure();
      view.operands.push_back(*bound);
    }
    return view;
  }
  bool scanConstantAttribute(Attribute attr, unsigned depth = 0) {
    if (depth >= 128) {
      reason = "owner-enable proof recursion budget exhausted";
      return false;
    }
    if (!charge())
      return false;
    if (auto integer = dyn_cast<ac::MathIntAttr>(attr))
      return charge(integer.getCanonicalValue().size());
    if (auto expression = dyn_cast<ac::StaticExprAttr>(attr))
      return scanConstantAttribute(expression.getTree(), depth + 1);
    if (auto dictionary = dyn_cast<DictionaryAttr>(attr)) {
      for (auto field : dictionary)
        if (!scanConstantAttribute(field.getValue(), depth + 1))
          return false;
    } else if (auto array = dyn_cast<ArrayAttr>(attr)) {
      for (auto element : array)
        if (!scanConstantAttribute(element, depth + 1))
          return false;
    }
    return true;
  }
  FailureOr<std::optional<llvm::APSInt>> integer(ac::StaticExprAttr expr,
                                                 Operation *site) {
    if (!scanConstantAttribute(expr))
      return failure();
    if (!analysis.isStaticEvaluable(expr, {}))
      return std::optional<llvm::APSInt>();
    auto result = analysis.evaluateStatic(expr, {}, site);
    if (failed(result))
      return failure();
    auto value = dyn_cast<ac::MathIntAttr>(*result);
    if (!value)
      return std::optional<llvm::APSInt>();
    return std::optional<llvm::APSInt>(llvm::APSInt(value.getCanonicalValue()));
  }
  FailureOr<std::optional<uint64_t>> width(Value value) {
    auto type = dyn_cast<ac::BitsType>(value.getType());
    if (!type)
      return std::optional<uint64_t>();
    auto result = integer(type.getWidth(), value.getDefiningOp());
    if (failed(result))
      return failure();
    if (!*result || (**result).isNegative() || (**result).isZero() ||
        (**result).getActiveBits() > 64)
      return std::optional<uint64_t>();
    return std::optional<uint64_t>((**result).getZExtValue());
  }
  FailureOr<Value> canonical(Value value, unsigned depth) {
    if (!charge())
      return failure();
    if (depth >= 128) {
      fail("owner-enable proof recursion budget exhausted");
      return failure();
    }
    auto cached = normalized.find(value);
    if (cached != normalized.end())
      return cached->second;
    if (!normalizing.insert(value).second) {
      fail("cycle in owner-enable proof normalization");
      return failure();
    }
    Value next;
    if (auto result = dyn_cast<OpResult>(value)) {
      if (auto rule = dyn_cast<ac::RuleOp>(result.getOwner())) {
        if (!rule->getRegion(0).hasOneBlock() ||
            rule->getRegion(0).front().empty()) {
          fail("missing owner-enable rule yield endpoint");
          return failure();
        }
        auto yield = dyn_cast<ac::YieldOp>(rule->getRegion(0).front().back());
        if (!yield || result.getResultNumber() >= yield->getNumOperands()) {
          fail("missing owner-enable rule yield endpoint");
          return failure();
        }
        next = yield->getOperand(result.getResultNumber());
      }
    } else if (auto argument = dyn_cast<BlockArgument>(value)) {
      auto rule =
          dyn_cast_or_null<ac::RuleOp>(argument.getOwner()->getParentOp());
      if (rule) {
        if (argument.getArgNumber() >= rule->getNumOperands()) {
          fail("missing owner-enable actual capture endpoint");
          return failure();
        }
        next = rule->getOperand(argument.getArgNumber());
      }
    }
    if (auto extract = value.getDefiningOp<ac::BitsExtractOp>()) {
      auto low = integer(extract.getLow(), extract);
      auto from = width(extract.getInput()), to = width(value);
      if (failed(low) || failed(from) || failed(to))
        return failure();
      if (*low && (**low).isZero() && *from && *to && **from == **to)
        next = extract.getInput();
    } else if (auto resize = value.getDefiningOp<ac::BitsResizeOp>()) {
      auto from = width(resize.getInput()), to = width(value);
      if (failed(from) || failed(to))
        return failure();
      if (*from && *to && **from == **to)
        next = resize.getInput();
    }
    if (next) {
      auto resolved = canonical(next, depth + 1);
      if (failed(resolved))
        return failure();
      next = *resolved;
    } else
      next = value;
    normalizing.erase(value);
    normalized.try_emplace(value, next);
    return next;
  }
  using Constant = ac::detail::HardwareGrantProof::Constant;
  FailureOr<std::optional<Constant>> constant(Value value, unsigned depth) {
    auto resolved = canonical(value, depth);
    if (failed(resolved))
      return failure();
    auto op = resolved->getDefiningOp<ac::BitsConstantOp>();
    if (!op)
      return std::optional<Constant>();
    auto count = width(*resolved);
    auto code = integer(op.getValue(), op);
    if (failed(count) || failed(code))
      return failure();
    if (!*count || !*code || (**code).isNegative() ||
        (**code).getActiveBits() > **count)
      return std::optional<Constant>();
    // Charge the entire padded pattern before allocating it.
    if (!charge(**count / 64 + (**count % 64 != 0)) ||
        **count > std::numeric_limits<unsigned>::max())
      return failure();
    llvm::APSInt pattern((**code).zextOrTrunc(**count), true);
    return std::optional<Constant>(
        Constant{ac::MathIntAttr::get(value.getContext(), pattern), **count});
  }
  ac::HardwareAnalysis analysis;
  uint64_t work;
  std::string reason;
  llvm::DenseMap<Value, Value> normalized;
  llvm::DenseSet<Value> normalizing;
  SmallVector<Value> nodes;
  llvm::DenseMap<Value, Node> ids;
  std::unique_ptr<ac::detail::HardwareGrantProof> proof;
};
