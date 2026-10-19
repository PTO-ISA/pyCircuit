// Consume immutable inference plans; only runtime SSA evaluation occurs here.
const MemoryCallPlan *Importer::memoryPlan(const AstNode &call,
                                           FlatSymbolRefAttr symbol) {
  auto site = sourceOccurrence(b, symbol, call);
  for (const auto &plan : memoryPlans)
    if (plan.occurrence == site)
      return &plan;
  return nullptr;
}
LogicalResult
Importer::bindMemoryInputs(const MemoryCallPlan &plan, Operation *instance,
                           OpBuilder &at, FlatSymbolRefAttr symbol,
                           llvm::StringMap<Value> &values,
                           llvm::StringMap<Instance *> &instances) {
  if (!values.contains("pyc_clk") || !values.contains("pyc_rst"))
    return error() << "memory binding lacks its inferred physical domain";
  SmallVector<Value> inputs(plan.inputTypes.size() + 2);
  inputs[plan.clockPosition] = values["pyc_clk"];
  inputs[plan.resetPosition] = values["pyc_rst"];
  if (!plan.handleName.empty()) {
    for (auto [i, type] : llvm::enumerate(plan.inputTypes)) {
      auto idle = inactiveCarrier(type, plan.call, at, symbol);
      if (failed(idle)) return failure();
      inputs[i + 2] = *idle;
    }
    instance->setOperands(inputs);
    return success();
  }
  for (auto [i, actual] : llvm::enumerate(plan.actuals)) {
    auto value =
        expression(actual, at, symbol, values, instances, plan.inputTypes[i]);
    if (failed(value))
      return failure();
    StringRef name = plan.inputNames[i];
    bool control =
        name == "ren" || name == "ren0" || name == "ren1" || name == "wvalid";
    bool address = name.starts_with("raddr") || name == "waddr";
    if (control) {
      if (failed(predicateView(*value, actual, symbol)))
        return failure();
    } else {
      if (address && fixedValues.contains(*value) &&
          !ac::areEquivalentHardwareTypes(value->getType(), plan.inputTypes[i]))
        return mlir::emitError(actual.location(b.getContext(), source.path))
               << "memory address width disagrees with depth-derived signature";
      auto converted = unsignedBoundary(*value, plan.inputTypes[i], actual, at,
                                        symbol, !address);
      if (failed(converted))
        return failure();
      value = *converted;
    }
    inputs[i + 2] = *value;
  }
  instance->setOperands(inputs);
  return success();
}
FailureOr<bool> Importer::bindMemoryResults(
    const AstNode &statement, OpBuilder &at, FlatSymbolRefAttr symbol,
    llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances,
    SmallVectorImpl<Instance> &instanceStorage) {
  auto call = statement.child("value");
  if (memoryKind(call).empty())
    return false;
  auto plan = memoryPlan(call, symbol);
  if (!plan)
    return error()
           << "memory allocation has no initialized source binding plan";
  auto found = directCallSites.find(plan->occurrence);
  if (found == directCallSites.end() || found->second.bound)
    return error() << "memory allocation must bind once at its lexical site";
  if (!plan->handleName.empty() &&
      (values.contains(plan->handleName) || instances.contains(plan->handleName) ||
       nestedRuleBinding(plan->handleName)))
    return error() << "Memory handle requires a fresh immutable name";
  for (const auto &name : plan->resultNames)
    if (values.contains(name) || nestedRuleBinding(name))
      return error() << "memory result shadows an existing hardware value '"
                     << name << "'";
  if (failed(bindMemoryInputs(*plan, found->second.op, at, symbol, values,
                              instances)))
    return failure();
  found->second.bound = true;
  if (!plan->handleName.empty()) {
    Instance instance;
    instance.name = plan->handleName;
    instance.node = statement;
    instance.callee = plan->callee;
    instance.parameters = plan->parameters;
    instance.typeArguments = plan->typeArguments;
    instance.op = found->second.op;
    instance.bound = instance.valueHandle = true;
    for (auto [i, type] : llvm::enumerate(plan->inputTypes))
      instance.inputs.push_back({plan->inputNames[i], type, call, std::nullopt, true});
    auto annotation = call.child("func").child("slice");
    instance.outputs.push_back({"result", plan->resultTypes.front(), annotation,
                               annotationKind(annotation), fixedAnnotation(annotation)});
    instanceStorage.push_back(std::move(instance));
    instances[plan->handleName] = &instanceStorage.back();
  }
  for (auto [i, name] : llvm::enumerate(plan->resultNames)) {
    Value value = found->second.op->getResult(i);
    values[name] = value;
    if (plan->resultFixedBits[i])
      fixedValues.insert(value);
  }
  return true;
}

Instance *Importer::memoryInstance(Instance *instance) const {
  if (!instance || !instance->op ||
      !isa<ac::InstanceOp>(instance->op) ||
      !instance->op->hasAttr("ac.source_memory"))
    return nullptr;
  return instance;
}

FailureOr<SmallVector<Value>> Importer::memoryMethodCall(
    const AstNode &node, Instance &receiver, OpBuilder &at,
    FlatSymbolRefAttr symbol, llvm::StringMap<Value> &values,
    llvm::StringMap<Instance *> &instances, std::optional<bool> tupleCall) {
  if (!activeSourceEffects || activeSourceEffects->guard ||
      activeSourceEffects->pureExpression)
    return error() << "Memory operations require a Rule body, not a guard or selector";
  if (activeSourceEffects->callable && !activeSourceEffects->callable->lexical)
    return error() << "global Rule cannot capture a caller Module Memory handle";
  if (tupleCall && *tupleCall)
    return error() << "Memory operation does not return a Tuple";
  StringRef kind = memoryKind(receiver.node.child("value"));
  StringRef method = node.child("func").string("attr");
  bool sync = kind != "byte_mem";
  bool response = method == "result", request = method == "request";
  bool read = method == "read", write = method == "write";
  if ((!response && !request && !read && !write) ||
      ((response || request) && !sync) || (read && sync))
    return error() << "unknown Memory operation for " << kind;
  SmallVector<StringRef> formals;
  if (!response) formals.push_back("address");
  if (write) llvm::append_range(formals, ArrayRef<StringRef>{"value", "mask"});
  else if (sync) formals.push_back("port");
  size_t optional = write || sync ? 1 : 0;
  size_t positional = write ? 2 : response ? 0 : 1;
  if (node.array("args").size() > positional)
    return error() << "Memory port and mask arguments are keyword-only";
  SmallVector<size_t> order;
  auto actuals = normalizeCallArguments(node, formals, error,
      "Memory operation argument arity mismatch", &order, optional);
  if (failed(actuals)) return failure();
  unsigned port = 0;
  if (sync && !write && actuals->back()) {
    auto expression = staticExpr(actuals->back(), symbol);
    if (failed(expression)) return failure();
    auto value = ac::HardwareAnalysis(*body).evaluateStatic(
        *expression, {}, body->getOperation());
    auto integer = succeeded(value) ? dyn_cast<ac::MathIntAttr>(*value)
                                    : ac::MathIntAttr();
    if (!integer)
      return error() << "Memory port requires a closed static Integer";
    llvm::APSInt index(integer.getCanonicalValue());
    unsigned count = kind == "sync_mem_dp" ? 2 : 1;
    if (index.isNegative() || index.getActiveBits() > 32 ||
        index.getZExtValue() >= count)
      return error() << "Memory port is outside its physical read-port range";
    port = index.getZExtValue();
  }
  auto input = [&](StringRef name) -> const Port * {
    auto found = llvm::find_if(receiver.inputs,
        [&](const Port &candidate) { return candidate.name == name; });
    return found == receiver.inputs.end() ? nullptr : &*found;
  };
  SmallVector<const Port *> contracts;
  if (!response) {
    std::string address = write ? "waddr" : kind == "sync_mem_dp"
        ? ("raddr" + Twine(port)).str() : "raddr";
    const Port *contract = input(address);
    if (!contract) return error() << "Memory allocation lacks address authority";
    contracts.push_back(contract);
  }
  if (write) {
    contracts.push_back(&receiver.outputs.front());
    auto mask = input("wstrb");
    if (!mask) return error() << "Memory allocation lacks strobe authority";
    contracts.push_back(mask);
  }
  SmallVector<Value> arguments(contracts.size());
  // Evaluate authored runtime actuals exactly once, in Python source order.
  for (size_t index : order) {
    if (index >= contracts.size()) continue; // Closed static port only.
    const AstNode &actual = (*actuals)[index];
    const Port &contract = *contracts[index];
    auto value = expression(actual, at, symbol, values, instances, contract.type);
    if (failed(value)) return failure();
    if (index == 0 && fixedValues.contains(*value) &&
        !ac::areEquivalentHardwareTypes(value->getType(), contract.type))
      return error() << "memory address width disagrees with depth-derived signature";
    auto converted = write && index == 1
        ? applyBindingBoundary(*value, BindingBoundary{contract, true}, actual, at, symbol)
        : unsignedBoundary(*value, contract.type, actual, at, symbol, index != 0);
    if (failed(converted)) return failure();
    arguments[index] = *converted;
  }
  if (write && !arguments.back()) {
    auto zero = constant(contracts.back()->type, 0, node, at, symbol);
    if (failed(zero)) return failure();
    arguments.back() = createOp(at, node.location(b.getContext(), source.path),
        ac::BitsUnaryOp::getOperationName(), {*zero}, {contracts.back()->type},
        {at.getNamedAttr("opcode", at.getStringAttr("not"))})->getResult(0);
    fixedValues.insert(arguments.back());
  }
  auto enabled = trueValue(node, at, symbol);
  if (failed(enabled)) return failure();
  SmallVector<Value> operands{*enabled};
  llvm::append_range(operands, arguments);
  SmallVector<Type> results;
  if (response || read) results.push_back(receiver.outputs.front().type);
  std::string terminal = sync && !write ? (method + Twine(port)).str() : method.str();
  auto callee = SymbolRefAttr::get(b.getContext(), receiver.callee.getValue(),
      {FlatSymbolRefAttr::get(b.getContext(), terminal)});
  auto operation = createOp(at, node.location(b.getContext(), source.path),
      ac::SourceMethodCallOp::getOperationName(), operands, results,
      {at.getNamedAttr("callee", callee),
       at.getNamedAttr("instance_path", at.getArrayAttr({at.getStringAttr(receiver.name)})),
       at.getNamedAttr("occurrence", sourceOccurrence(at, symbol, node)),
       at.getNamedAttr("ac.location", sourceSpan(at, source.path, node))});
  if (!results.empty()) {
    auto result = operation->getResult(0);
    remember(result, receiver.outputs.front().sourceKind);
    if (receiver.outputs.front().fixedBits) fixedValues.insert(result);
  }
  return SmallVector<Value>(operation->getResults());
}
