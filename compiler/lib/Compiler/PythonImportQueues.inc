// Queue allocation and lexical binding remain part of the captured importer.
bool Importer::queueCall(const AstNode &node) const {
  if (node.kind() != "Call")
    return false;
  auto function = node.child("func");
  return intrinsic(function.kind() == "Subscript" ? function.child("value")
                                                  : function,
                   "queue");
}
FailureOr<SmallVector<StringRef, 3>>
Importer::queueTargets(const AstNode &assignment) const {
  auto diagnostic = [&] {
    return mlir::emitError(assignment.location(b.getContext(), source.path));
  };
  if (assignment.kind() != "Assign" || assignment.array("targets").size() != 1)
    return diagnostic() << "queue requires one three-name Tuple assignment";
  auto target = assignment.item("targets", 0);
  if (target.kind() != "Tuple" || target.array("elts").size() != 3)
    return diagnostic() << "queue requires exactly three ordinary Names";
  SmallVector<StringRef, 3> result;
  for (size_t i = 0; i < 3; ++i) {
    auto element = target.item("elts", i);
    if (element.kind() != "Name")
      return diagnostic() << "queue targets must be ordinary Names";
    StringRef name = element.string("id");
    if (name.empty() ||
        llvm::is_contained(ArrayRef<StringRef>{"pyc_clk", "pyc_rst", "module",
                                               "rule", "system", "queue", "log",
                                               "report", "bool", "int", "range",
                                               "Annotated"},
                           name) ||
        names.contains(name) || structs.contains(name) ||
        behavioralRules.contains(name))
      return diagnostic() << "queue target shadows a reserved/source binding: '"
                          << name << "'";
    if (llvm::is_contained(result, name))
      return diagnostic() << "queue targets must be distinct Names";
    result.push_back(name);
  }
  return result;
}
LogicalResult Importer::declareQueue(const AstNode &assignment,
                                     ModuleDecl &decl, OpBuilder &at,
                                     StringRef name) {
  auto call = assignment.child("value");
  auto function = call.child("func");
  if (function.kind() != "Subscript")
    return error() << "queue requires an explicit payload type";
  auto annotation = function.child("slice");
  if (!fixedAnnotation(annotation) && !nominalAnnotation(annotation) &&
      !(annotation.kind() == "Subscript" &&
        intrinsic(annotation.child("value"), "table")))
    return error() << "queue payload requires fixed bits, a nominal type or an "
                      "admitted table";
  auto payload = portType(annotation, decl.symbol);
  if (failed(payload))
    return failure();
  ac::StaticExprAttr depth = literal(2, call, decl.symbol);
  ac::StaticExprAttr latency = literal(1, call, decl.symbol);
  StringAttr policy = at.getStringAttr("local_occupancy");
  llvm::StringSet<> seen;
  if (call.array("args").size() > 3)
    return error() << "queue configuration arguments are keyword-only";
  for (size_t i = 0; i < call.array("keywords").size(); ++i) {
    auto keyword = call.item("keywords", i);
    auto key = dyn_cast_or_null<StringAttr>(keyword.get("arg"));
    if (!key || !seen.insert(key.getValue()).second)
      return error() << "unknown or repeated queue argument";
    if (key.getValue() == "depth" || key.getValue() == "latency") {
      auto expression = staticExpr(keyword.child("value"), decl.symbol);
      if (failed(expression))
        return failure();
      (key.getValue() == "depth" ? depth : latency) = *expression;
    } else if (key.getValue() == "ready_policy") {
      auto value = keyword.child("value");
      policy = value.kind() == "Constant"
                   ? dyn_cast_or_null<StringAttr>(value.get("value"))
                   : StringAttr();
      if (!policy)
        return error() << "queue ready_policy requires a static string";
    } else if (key.getValue() != "in_valid" && key.getValue() != "in_data" &&
               key.getValue() != "out_ready")
      return error() << "unknown queue argument '" << key.getValue() << "'";
  }
  Type control = bits(1, call, decl.symbol);
  auto site = occurrence(at, decl.symbol, call);
  auto op = createOp(
      at, call.location(b.getContext(), source.path),
      ac::QueueOp::getOperationName(), {},
      TypeRange{control, control, *payload},
      {at.getNamedAttr("instance_name", at.getStringAttr(name)),
       at.getNamedAttr("depth", depth), at.getNamedAttr("ready_policy", policy),
       at.getNamedAttr("availability_latency", latency),
       at.getNamedAttr("head_read_latency", literal(0, call, decl.symbol)),
       at.getNamedAttr("empty_flow", at.getBoolAttr(false)),
       at.getNamedAttr("read_during_write", at.getStringAttr("old")),
       at.getNamedAttr("reset_policy", at.getStringAttr("sync_high_empty")),
       at.getNamedAttr("empty_data", at.getStringAttr("zero")),
       at.getNamedAttr("occurrence", site)});
  directCallSites.try_emplace(site, DirectCallSite{op});
  fixedValues.insert(op->getResult(0));
  fixedValues.insert(op->getResult(1));
  if (isa<ac::BitsType>(*payload))
    fixedValues.insert(op->getResult(2));
  return success();
}
FailureOr<bool> Importer::bindQueueResults(
    const AstNode &assignment, OpBuilder &at, FlatSymbolRefAttr symbol,
    llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances) {
  auto call = assignment.child("value");
  if (!queueCall(call))
    return false;
  if (!activeBehavioralDecl || activeBehavioralDecl->symbol != symbol ||
      !isa<ac::ModuleOp>(at.getInsertionBlock()->getParentOp()))
    return error() << "queue allocation requires direct module scope";
  auto targets = queueTargets(assignment);
  if (failed(targets))
    return failure();
  auto found = directCallSites.find(occurrence(at, symbol, call));
  if (found == directCallSites.end() || !isa<ac::QueueOp>(found->second.op) ||
      found->second.bound)
    return error()
           << "queue requires one predeclared owner per direct occurrence";
  for (StringRef target : *targets)
    if (values.contains(target) || instances.contains(target))
      return error() << "queue target cannot shadow an existing binding";
  SmallVector<AstNode, 3> actuals(3);
  if (call.array("args").size() > actuals.size())
    return error() << "queue configuration arguments are keyword-only";
  for (size_t i = 0; i < call.array("args").size(); ++i)
    actuals[i] = call.item("args", i);
  constexpr StringRef formals[] = {"in_valid", "in_data", "out_ready"};
  for (size_t i = 0; i < call.array("keywords").size(); ++i) {
    auto keyword = call.item("keywords", i);
    auto key = dyn_cast_or_null<StringAttr>(keyword.get("arg"));
    if (!key)
      return error() << "queue does not admit **kwargs";
    for (size_t j = 0; j < actuals.size(); ++j)
      if (key.getValue() == formals[j]) {
        if (actuals[j])
          return error() << "unknown or repeated queue argument";
        actuals[j] = keyword.child("value");
      }
  }
  SmallVector<Value> operands;
  for (size_t i = 0; i < actuals.size(); ++i) {
    if (!actuals[i])
      return error() << "missing queue input '" << formals[i] << "'";
    if (actuals[i].kind() == "Starred")
      return error() << "queue does not admit *args";
    Type expected = found->second.op->getResult(i == 1 ? 2 : 0).getType();
    auto value =
        expression(actuals[i], at, symbol, values, instances, expected);
    if (failed(value))
      return failure();
    if (i == 1) {
      auto converted =
          unsignedBoundary(*value, expected, actuals[i], at, symbol);
      if (failed(converted))
        return failure();
      operands.push_back(*converted);
    } else {
      auto predicate = predicateView(*value, actuals[i], symbol);
      if (failed(predicate))
        return failure();
      operands.push_back(predicate->value);
    }
  }
  if (!values.contains("pyc_clk") || !values.contains("pyc_rst"))
    return error() << "queue lacks inferred physical domain";
  found->second.op->setOperands({values["pyc_clk"], values["pyc_rst"],
                                 operands[0], operands[1], operands[2]});
  ac::HardwareAnalysis analysis(*body);
  ac::HardwareBindings bindings;
  bindings.owner = at.getInsertionBlock()->getParentOp();
  if (failed(analysis.verifyQueueOperation(cast<ac::QueueOp>(found->second.op),
                                           bindings)))
    return failure();
  found->second.bound = true;
  for (auto [ordinal, target] : llvm::enumerate(*targets))
    values[target] = found->second.op->getResult(ordinal);
  return true;
}
