// Queue source syntax normalizes to ordinary canonical FIFO instances.
bool Importer::queueCall(const AstNode &node) const {
  if (node.kind() != "Call") return false;
  auto function = node.child("func");
  return intrinsic(function.kind() == "Subscript" ? function.child("value")
                                                  : function, "queue");
}
Instance *Importer::queueInstance(Instance *instance) const {
  if (!instance || !instance->op)
    return nullptr;
  auto allocation = dyn_cast<ac::InstanceOp>(instance->op);
  return allocation && ac::isQueueInstance(allocation) ? instance : nullptr;
}
FailureOr<SmallVector<StringRef, 3>>
Importer::queueTargets(const AstNode &assignment) const {
  auto diagnostic = [&] {
    return mlir::emitError(assignment.location(b.getContext(), source.path));
  };
  if (assignment.kind() != "Assign" || assignment.array("targets").size() != 1)
    return diagnostic() << "queue requires one handle or three-name Tuple assignment";
  auto target = assignment.item("targets", 0);
  if (target.kind() != "Name" &&
      (target.kind() != "Tuple" || target.array("elts").size() != 3))
    return diagnostic() << "queue requires one ordinary Name or exactly three ordinary Names";
  SmallVector<StringRef, 3> result;
  unsigned count = target.kind() == "Name" ? 1 : 3;
  for (unsigned i = 0; i < count; ++i) {
    auto element = count == 1 ? target : target.item("elts", i);
    if (element.kind() != "Name")
      return diagnostic() << "queue targets must be ordinary Names";
    StringRef name = element.string("id");
    if (name.empty() ||
        llvm::is_contained(ArrayRef<StringRef>{"pyc_clk", "pyc_rst", "module",
                                               "rule", "system", "queue", "log",
                                               "report", "bool", "int", "range",
                                               "Annotated"}, name) ||
        names.contains(name) || structs.contains(name) || behavioralRules.contains(name))
      return diagnostic() << "queue target shadows a reserved/source binding: '" << name << "'";
    if (llvm::is_contained(result, name))
      return diagnostic() << "queue targets must be distinct Names";
    result.push_back(name);
  }
  return result;
}
LogicalResult Importer::declareQueue(const AstNode &assignment, ModuleDecl &decl,
                                     OpBuilder &at, StringRef name) {
  auto targets = queueTargets(assignment);
  if (failed(targets)) return failure();
  bool named = targets->size() == 1;
  auto call = assignment.child("value"), function = call.child("func");
  if (function.kind() != "Subscript")
    return error() << "queue requires an explicit payload type";
  auto annotation = function.child("slice");
  if (!named && !fixedAnnotation(annotation) && !nominalAnnotation(annotation) &&
      !(annotation.kind() == "Subscript" && intrinsic(annotation.child("value"), "table")))
    return error() << "queue payload requires fixed bits, a nominal type or an admitted table";
  auto payload = portType(annotation, decl.symbol);
  if (failed(payload)) return failure();
  Port contract{targets->front().str(), *payload, annotation,
                annotationKind(annotation), fixedAnnotation(annotation)};
  ac::StaticExprAttr depth = literal(2, call, decl.symbol);
  ac::StaticExprAttr latency = literal(1, call, decl.symbol);
  StringAttr policy = at.getStringAttr(named ? "downstream_pop" : "local_occupancy");
  llvm::StringSet<> seen;
  if (call.array("args").size() > (named ? 0u : 3u))
    return error() << "named queue bindings and queue configuration are keyword-only";
  for (size_t i = 0; i < call.array("keywords").size(); ++i) {
    auto keyword = call.item("keywords", i);
    auto key = dyn_cast_or_null<StringAttr>(keyword.get("arg"));
    if (!key || !seen.insert(key.getValue()).second)
      return error() << "unknown or repeated queue argument";
    if (key.getValue() == "depth" || key.getValue() == "latency") {
      auto expression = staticExpr(keyword.child("value"), decl.symbol);
      if (failed(expression)) return failure();
      (key.getValue() == "depth" ? depth : latency) = *expression;
    } else if (key.getValue() == "ready_policy") {
      auto value = keyword.child("value");
      policy = value.kind() == "Constant"
                   ? dyn_cast_or_null<StringAttr>(value.get("value")) : StringAttr();
      if (!policy) return error() << "queue ready_policy requires a static string";
    } else if (key.getValue() != "in_valid" && key.getValue() != "in_data" &&
               key.getValue() != "out_ready")
      return error() << "unknown queue argument '" << key.getValue() << "'";
  }
  if (named && seen.contains("in_valid") != seen.contains("in_data"))
    return error() << "named queue raw input requires in_valid and in_data together";
  if (policy.getValue() != "local_occupancy" &&
      policy.getValue() != "downstream_pop")
    return error() << "queue ready_policy requires 'local_occupancy' or "
                      "'downstream_pop'";
  Type control = named ? internalBits(1, call, decl.symbol)
                       : bits(1, call, decl.symbol);
  auto site = sourceOccurrence(at, decl.symbol, call);
  StringRef primitive = policy.getValue() == "local_occupancy"
                            ? "fifo_local" : "fifo_downstream_pop";
  auto builtin = headers.lookupBuiltin(primitive);
  if (!builtin || !headers.isTrustedBuiltin(builtin.declaration))
    return error() << "queue requires its trusted canonical builtin";
  if (!SymbolTable::lookupSymbolIn(*body, builtin.symbol)) {
    auto *copy = builtin.declaration->clone();
    copy->setAttr("ac.declaration_role", b.getStringAttr("import_snapshot"));
    body->getBody()->push_back(copy);
  }
  auto op = createOp(at, call.location(b.getContext(), source.path),
      ac::InstanceOp::getOperationName(), {},
      TypeRange{control, control, *payload,
                internalBits(1, call, decl.symbol)},
      {at.getNamedAttr("instance_name", at.getStringAttr(named ? targets->front() : name)),
       at.getNamedAttr("callee", builtin.symbol),
       at.getNamedAttr("parameters", at.getArrayAttr({depth, latency})),
       at.getNamedAttr("type_arguments",
                       at.getArrayAttr({TypeAttr::get(*payload)})),
       at.getNamedAttr("occurrence", site)});
  directCallSites.try_emplace(site, DirectCallSite{op});
  fixedValues.insert(op->getResult(0));
  fixedValues.insert(op->getResult(1));
  fixedValues.insert(op->getResult(3));
  if (named) {
    auto constraint = sourceConstraint(contract);
    if (failed(constraint)) return failure();
    op->setAttr("ac.payload_constraint", *constraint);
    if (!seen.contains("in_valid")) op->setAttr("ac.source_unbound_push", at.getBoolAttr(true));
    if (!seen.contains("out_ready")) op->setAttr("ac.source_unbound_pop", at.getBoolAttr(true));
    remember(op->getResult(2), contract.sourceKind);
    if (contract.fixedBits) fixedValues.insert(op->getResult(2));
  } else if (isa<ac::BitsType>(*payload))
    fixedValues.insert(op->getResult(2));
  return success();
}
FailureOr<bool> Importer::bindQueueResults(
    const AstNode &assignment, OpBuilder &at, FlatSymbolRefAttr symbol,
    llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances,
    SmallVectorImpl<Instance> &instanceStorage) {
  auto call = assignment.child("value");
  if (!queueCall(call)) return false;
  if (!activeBehavioralDecl || activeBehavioralDecl->symbol != symbol ||
      !isa<ac::ModuleOp>(at.getInsertionBlock()->getParentOp()))
    return error() << "queue allocation and aliases require direct module scope";
  auto targets = queueTargets(assignment);
  if (failed(targets)) return failure();
  for (StringRef target : *targets)
    if (values.contains(target) || instances.contains(target) || enums.contains(target) ||
        nestedRuleBinding(target) ||
        sourceMethods[activeBehavioralDecl->node.string("name")].contains(target) ||
        sourceRules[activeBehavioralDecl->node.string("name")].contains(target))
      return error() << "queue target cannot shadow an existing binding";
  bool named = targets->size() == 1;
  auto found = directCallSites.find(sourceOccurrence(at, symbol, call));
  if (found == directCallSites.end() ||
      !ac::isQueueInstance(dyn_cast<ac::InstanceOp>(found->second.op)) ||
      found->second.bound)
    return error() << "queue requires one predeclared owner per direct occurrence";
  SmallVector<AstNode, 3> actuals(3);
  if (call.array("args").size() > (named ? 0u : 3u))
    return error() << "named queue bindings and queue configuration are keyword-only";
  for (size_t i = 0; i < call.array("args").size(); ++i) actuals[i] = call.item("args", i);
  constexpr StringRef formals[] = {"in_valid", "in_data", "out_ready"};
  for (size_t i = 0; i < call.array("keywords").size(); ++i) {
    auto keyword = call.item("keywords", i);
    auto key = dyn_cast_or_null<StringAttr>(keyword.get("arg"));
    if (!key) return error() << "queue does not admit **kwargs";
    for (size_t j = 0; j < actuals.size(); ++j)
      if (key.getValue() == formals[j]) {
        if (actuals[j]) return error() << "unknown or repeated queue argument";
        actuals[j] = keyword.child("value");
      }
  }
  if (named && bool(actuals[0]) != bool(actuals[1]))
    return error() << "named queue raw input requires in_valid and in_data together";
  SmallVector<Value> operands;
  Port payloadContract{named ? targets->front().str() : "data",
                       found->second.op->getResult(2).getType(),
                       call.child("func").child("slice"),
                       annotationKind(call.child("func").child("slice")),
                       fixedAnnotation(call.child("func").child("slice"))};
  for (size_t i = 0; i < actuals.size(); ++i) {
    Type expected = found->second.op->getResult(i == 1 ? 2 : 0).getType();
    if (!actuals[i]) {
      if (!named) return error() << "missing queue input '" << formals[i] << "'";
      auto inactive = i == 1 ? inactiveCarrier(expected, call, at, symbol)
                             : constant(expected, 0, call, at, symbol);
      if (failed(inactive)) return failure();
      operands.push_back(*inactive);
      continue;
    }
    if (actuals[i].kind() == "Starred") return error() << "queue does not admit *args";
    auto value = expression(actuals[i], at, symbol, values, instances, expected);
    if (failed(value)) return failure();
    if (i == 1) {
      auto converted = named
          ? applyBindingBoundary(*value,
                                 BindingBoundary{payloadContract, true},
                                 actuals[i], at, symbol)
          : unsignedBoundary(*value, expected, actuals[i], at, symbol);
      if (failed(converted)) return failure();
      operands.push_back(*converted);
    } else {
      auto predicate = predicateView(*value, actuals[i], symbol);
      if (failed(predicate)) return failure();
      operands.push_back(predicate->value);
    }
  }
  if (!values.contains("pyc_clk") || !values.contains("pyc_rst"))
    return error() << "queue lacks inferred physical domain";
  found->second.op->setOperands({values["pyc_clk"], values["pyc_rst"], operands[0], operands[1], operands[2]});
  ac::HardwareAnalysis analysis(*body);
  ac::HardwareBindings bindings;
  bindings.owner = at.getInsertionBlock()->getParentOp();
  if (failed(analysis.verifyQueueOperation(cast<ac::InstanceOp>(found->second.op),
                                           bindings)))
    return failure();
  found->second.bound = true;
  if (named) {
    Instance instance;
    instance.name = targets->front().str();
    instance.node = assignment;
    instance.callee = cast<ac::InstanceOp>(found->second.op).getCalleeAttr();
    instance.parameters.assign(
        cast<ac::InstanceOp>(found->second.op).getParameters().begin(),
        cast<ac::InstanceOp>(found->second.op).getParameters().end());
    instance.typeArguments.assign(
        cast<ac::InstanceOp>(found->second.op).getTypeArguments().begin(),
        cast<ac::InstanceOp>(found->second.op).getTypeArguments().end());
    instance.inputs = {
        {"clk", found->second.op->getOperand(0).getType(), call, std::nullopt,
         true},
        {"rst", found->second.op->getOperand(1).getType(), call, std::nullopt,
         true},
        {"in_valid", found->second.op->getOperand(2).getType(), call,
         std::nullopt, true},
        {"in_data", payloadContract.type, payloadContract.node,
         payloadContract.sourceKind, payloadContract.fixedBits},
        {"out_ready", found->second.op->getOperand(4).getType(), call,
         std::nullopt, true}};
    instance.outputs = {
        {"ready", found->second.op->getResult(0).getType(), call,
         std::nullopt, true},
        {"valid", found->second.op->getResult(1).getType(), call,
         std::nullopt, true},
        payloadContract};
    instance.op = found->second.op;
    instance.bound = true;
    instance.valueHandle = true;
    instanceStorage.push_back(std::move(instance));
    instances[targets->front()] = &instanceStorage.back();
  } else {
    for (auto [ordinal, target] : llvm::enumerate(*targets)) values[target] = found->second.op->getResult(ordinal);
  }
  return true;
}
FailureOr<std::optional<Type>> Importer::queueExpressionType(
    const AstNode &node, FlatSymbolRefAttr symbol, llvm::StringMap<Value> &values,
    llvm::StringMap<Instance *> &instances) {
  AstNode attribute = node.kind() == "Call" ? node.child("func") : node;
  if (attribute.kind() != "Attribute" || attribute.child("value").kind() != "Name")
    return std::optional<Type>();
  StringRef base = attribute.child("value").string("id");
  auto found = instances.find(base);
  auto handle = values.contains(base) || queryBound(base) ||
                        found == instances.end()
                    ? nullptr
                    : queueInstance(found->second);
  if (!handle) return std::optional<Type>();
  if (activeSourceEffects && activeSourceEffects->callable &&
      !activeSourceEffects->callable->lexical)
    return error() << "global Rule cannot capture a caller Module Queue handle";
  StringRef member = attribute.string("attr");
  if (node.kind() == "Call") {
    if (member == "pop" || member == "peek")
      return std::optional<Type>(handle->outputs[2].type);
    if (member == "push") return error() << "queue.push does not produce a payload value";
  } else {
    if (member == "ready" || member == "valid")
      return std::optional<Type>(handle->outputs[member == "ready" ? 0 : 1].type);
    if (member == "data") return std::optional<Type>(handle->outputs[2].type);
  }
  return error() << "unknown Queue member or indirect Queue method value";
}
FailureOr<std::optional<Value>> Importer::queueRawExpression(
    const AstNode &node, OpBuilder &at, FlatSymbolRefAttr symbol,
    llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances) {
  auto named = node.kind() == "Name" ? instances.find(node.string("id"))
                                    : instances.end();
  if (node.kind() == "Name" && named != instances.end() &&
      queueInstance(named->second) && !values.contains(node.string("id")))
    return error() << "queue handle is a static allocation identity, not a payload value";
  if (node.kind() != "Attribute" || node.child("value").kind() != "Name") return std::optional<Value>();
  StringRef base = node.child("value").string("id");
  auto found = instances.find(base);
  auto handle = values.contains(base) || queryBound(base) ||
                        found == instances.end()
                    ? nullptr
                    : queueInstance(found->second);
  if (!handle) return std::optional<Value>();
  if (activeSourceEffects && activeSourceEffects->callable &&
      !activeSourceEffects->callable->lexical)
    return error() << "global Rule cannot capture a caller Module Queue handle";
  StringRef field = node.string("attr");
  unsigned ordinal = field == "ready" ? 0 : field == "valid" ? 1 : field == "data" ? 2 : 3;
  if (ordinal == 3) return error() << "unknown Queue raw view or indirect Queue method value";
  Value raw = handle->op->getResult(ordinal);
  if (!isa<ac::ModuleOp>(at.getInsertionBlock()->getParentOp()))
    raw = helperCapture(raw, at.getBlock(),
                        node.location(b.getContext(), source.path));
  if (!raw)
    return error() << "Queue raw view is absent from its lexical capture frame";
  return std::optional<Value>(raw);
}
