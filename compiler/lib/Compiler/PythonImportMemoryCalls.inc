// Consume immutable inference plans; only runtime SSA evaluation occurs here.
const MemoryCallPlan *Importer::memoryPlan(const AstNode &call,
                                          FlatSymbolRefAttr symbol) {
  auto site = occurrence(b, symbol, call);
  for (const auto &plan : memoryPlans)
    if (plan.occurrence == site)
      return &plan;
  return nullptr;
}
LogicalResult Importer::bindMemoryInputs(
    const MemoryCallPlan &plan, Operation *instance, OpBuilder &at,
    FlatSymbolRefAttr symbol, llvm::StringMap<Value> &values,
    llvm::StringMap<Instance *> &instances) {
  if (!values.contains("pyc_clk") || !values.contains("pyc_rst"))
    return error() << "memory binding lacks its inferred physical domain";
  SmallVector<Value> inputs(plan.inputTypes.size() + 2);
  inputs[plan.clockPosition] = values["pyc_clk"];
  inputs[plan.resetPosition] = values["pyc_rst"];
  for (auto [i, actual] : llvm::enumerate(plan.actuals)) {
    auto value = expression(actual, at, symbol, values, instances, plan.inputTypes[i]);
    if (failed(value)) return failure();
    StringRef name = plan.inputNames[i];
    bool control = name == "ren" || name == "ren0" || name == "ren1" || name == "wvalid";
    bool address = name.starts_with("raddr") || name == "waddr";
    if (control) {
      if (failed(predicateView(*value, actual, symbol))) return failure();
    } else {
      if (address && fixedValues.contains(*value) &&
          !ac::areEquivalentHardwareTypes(value->getType(), plan.inputTypes[i]))
        return mlir::emitError(actual.location(b.getContext(), source.path))
               << "memory address width disagrees with depth-derived signature";
      auto converted = unsignedBoundary(*value, plan.inputTypes[i], actual, at, symbol,
                                       !address);
      if (failed(converted)) return failure();
      value = *converted;
    }
    inputs[i + 2] = *value;
  }
  instance->setOperands(inputs);
  return success();
}
FailureOr<bool> Importer::bindMemoryResults(
    const AstNode &statement, OpBuilder &at, FlatSymbolRefAttr symbol,
    llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances) {
  auto call = statement.child("value");
  if (memoryKind(call).empty()) return false;
  auto plan = memoryPlan(call, symbol);
  if (!plan) return error() << "memory allocation has no initialized source binding plan";
  auto found = directCallSites.find(plan->occurrence);
  if (found == directCallSites.end() || found->second.bound)
    return error() << "memory allocation must bind once at its lexical site";
  for (const auto &name : plan->resultNames)
    if (values.contains(name))
      return error() << "memory result shadows an existing hardware value '" << name << "'";
  if (failed(bindMemoryInputs(*plan, found->second.op, at, symbol, values, instances)))
    return failure();
  found->second.bound = true;
  for (auto [i, name] : llvm::enumerate(plan->resultNames)) {
    Value value = found->second.op->getResult(i);
    values[name] = value;
    if (plan->resultFixedBits[i]) fixedValues.insert(value);
  }
  return true;
}
