LogicalResult Importer::inferModuleGraph() {
  SmallVector<SmallVector<size_t>> edges(modules.size());
  for (auto [index, decl] : llvm::enumerate(modules)) {
    for (size_t i = 0; i < decl.sourceInputCount; ++i)
      if (local.contains(decl.inputs[i].name))
        return error() << "module input shadows a source module";
    llvm::StringMap<AstNode> nestedRules;
    llvm::StringSet<> structuralInstances;
    for (size_t i = 0; i < decl.node.array("body").size(); ++i) {
      auto statement = decl.node.item("body", i);
      if (statement.kind() == "FunctionDef" && hasDecorator(statement, "rule"))
        nestedRules[statement.string("name")] = statement;
      if (statement.kind() == "Assign" && statement.array("targets").size() == 1 &&
          statement.item("targets", 0).kind() == "Name" && moduleCall(statement.child("value"))) {
        auto binding = lookupBinding(statement.child("value").child("func"));
        bool direct = false;
        if (binding && binding->category == BindingCategory::Module) {
          if (auto found = local.find(statement.child("value").child("func").string("id")); found != local.end())
            direct = modules[found->second].returnShape != ReturnShape::Mapping || modules[found->second].sourceInputCount;
          else {
            auto view = headers.lookupModule(binding->symbol);
            direct = view && view.hasSourceCallContract();
          }
          if (!direct) structuralInstances.insert(statement.item("targets", 0).string("id"));
        }
      }
    }
    auto structuralRule = [&](const AstNode &rule) {
      bool behavior = false;
      for (size_t i = 0; i < rule.array("body").size(); ++i) {
        auto statement = rule.item("body", i);
        if (docstring(statement) || statement.kind() == "Pass") continue;
        if (statement.kind() == "Assert") { behavior = true; continue; }
        auto call = statement.kind() == "Expr" ? statement.child("value") : AstNode();
        if (call.kind() == "Call" &&
            (intrinsic(call.child("func"), "log") || intrinsic(call.child("func"), "report") ||
             (call.child("func").kind() == "Name" && structuralInstances.contains(call.child("func").string("id"))))) {
          behavior = true; continue;
        }
        return false;
      }
      return behavior;
    };
    auto walk = [&](auto &&self, const AstNode &node,
                    bool controlled) -> LogicalResult {
      if (node.kind() == "FunctionDef" || node.kind() == "Lambda")
        return success();
      controlled |= node.kind() == "If" || node.kind() == "IfExp" ||
                    node.kind() == "BoolOp" || node.kind() == "For" ||
                    node.kind() == "While";
      if (queueCall(node)) {
        if (controlled)
          return error()
                 << "queue allocation under runtime control is unsupported";
        decl.queueDomain = true;
        decl.behavioral = true;
      }
      if (node.kind() == "Call") {
        auto function = node.child("func");
        StringRef name =
            function.kind() == "Name" ? function.string("id") : StringRef();
        auto binding = lookupBinding(function);
        if (binding && binding->category == BindingCategory::Enum)
          return error() << "nominal declaration is not a module constructor";
        if (nestedRules.contains(name) && !structuralRule(nestedRules[name])) {
          if (controlled) return error() << "Rule registration under runtime control is unsupported";
          decl.behavioral = true;
        }
        if (behavioralRules.contains(name)) {
          if (controlled)
            return error() << "behavioral rule calls under runtime control are "
                              "unsupported";
          decl.behavioral = true;
        }
        if (!local.contains(name) && binding &&
            binding->category == BindingCategory::Module) {
          auto view = headers.lookupModule(binding->symbol);
          if (view && view.domainInputs)
            decl.queueDomain |= !view.domainInputs.empty();
          if (view && view.semanticKind &&
              view.semanticKind.getValue() == "transactions") {
            decl.transactionEffects = true;
            decl.behavioral = true;
            if (methodOwner(function) && controlled)
              return error() << "Method owner construction under runtime "
                                "control is unsupported";
          }
          if (view && view.hasSourceCallContract()) {
            if (controlled)
              return error() << "direct module calls under runtime control are "
                                "unsupported";
            decl.behavioral = true;
          }
        }
        if (auto found = local.find(name); found != local.end()) {
          edges[index].push_back(found->second);
          if (modules[found->second].methodOwner) {
            if (controlled)
              return error() << "Method owner construction under runtime control is unsupported";
            decl.transactionEffects = true;
            decl.behavioral = true;
          }
          if (modules[found->second].returnShape != ReturnShape::Mapping ||
              !node.array("args").empty()) {
            if (controlled)
              return error() << "direct module calls under runtime control are "
                                "unsupported";
            decl.behavioral = true;
          }
        }
      }
      for (NamedAttribute field : node.fields()) {
        if (auto child = dyn_cast<DictionaryAttr>(field.getValue())) {
          if (child.getAs<StringAttr>("kind") &&
              failed(self(self, node.child(field.getName().getValue()),
                          controlled)))
            return failure();
        } else if (auto children = dyn_cast<ArrayAttr>(field.getValue())) {
          for (size_t i = 0; i < children.size(); ++i)
            if (auto child = dyn_cast<DictionaryAttr>(children[i]);
                child && child.getAs<StringAttr>("kind"))
              if (failed(self(self, node.item(field.getName().getValue(), i),
                              controlled)))
                return failure();
        }
      }
      return success();
    };
    for (size_t i = 0; i < decl.node.array("body").size(); ++i)
      if (failed(walk(walk, decl.node.item("body", i), false)))
        return failure();
  }
  SmallVector<unsigned> marks(modules.size(), 0);
  auto visit = [&](auto &&self, size_t index) -> LogicalResult {
    if (marks[index] == 1)
      return error() << "recursive module instantiation is unsupported";
    if (marks[index] == 2)
      return success();
    marks[index] = 1;
    bool domain = modules[index].system || modules[index].persistent ||
                  modules[index].queueDomain || modules[index].transactionEffects;
    for (size_t child : edges[index]) {
      if (failed(self(self, child)))
        return failure();
      domain |= modules[child].needsDomain;
      modules[index].transactionEffects |= modules[child].transactionEffects;
      modules[index].behavioral |= modules[child].transactionEffects;
    }
    modules[index].needsDomain = domain;
    marks[index] = 2;
    return success();
  };
  for (size_t i = 0; i < modules.size(); ++i)
    if (failed(visit(visit, i)))
      return failure();
  for (auto &decl : modules) {
    if (decl.needsDomain) {
      for (StringRef pin : {StringRef("pyc_clk"), StringRef("pyc_rst")}) {
        if (llvm::any_of(decl.inputs,
                         [&](const Port &port) { return port.name == pin; }) ||
            llvm::any_of(decl.outputs,
                         [&](const Port &port) { return port.name == pin; }) ||
            llvm::any_of(decl.parameters, [&](const Parameter &parameter) {
              return parameter.name == pin;
            }))
          return error() << "reserved physical domain port name '" << pin
                         << "'";
        decl.inputs.push_back(
            {pin.str(), bits(1, decl.node, decl.symbol), decl.node, {}, false});
      }
    }
    SmallVector<Type> inputs, outputs;
    for (auto &port : decl.inputs)
      inputs.push_back(port.type);
    for (auto &port : decl.outputs)
      outputs.push_back(port.type);
    decl.type = FunctionType::get(b.getContext(), inputs, outputs);
    if (failed(serializeSourceCall(decl)))
      return failure();
  }
  return success();
}
LogicalResult Importer::predeclareDirectCalls(ModuleDecl &decl, OpBuilder &at) {
  // These registries belong to this definition, never to a callee or argument
  // spelling. Occurrence includes the owner and the complete captured AST path.
  directCallSites.clear();
  forwardCallBindings.clear();
  queueResultNames.clear();
  memoryResultNames.clear();
  auto writes = sourceNameWrites(decl.node);
  queueNameWrites = writes;
  llvm::StringSet<> shadows;
  for (const Port &port : decl.inputs)
    shadows.insert(port.name);
  auto classify = [&](auto &&self, const AstNode &node) -> void {
    if (node.kind() == "Lambda")
      return;
    if (node.kind() == "FunctionDef" || node.kind() == "ClassDef") {
      shadows.insert(node.string("name"));
      return;
    }
    for (NamedAttribute field : node.fields()) {
      if (auto child = dyn_cast<DictionaryAttr>(field.getValue())) {
        if (child.getAs<StringAttr>("kind"))
          self(self, node.child(field.getName().getValue()));
      } else if (auto children = dyn_cast<ArrayAttr>(field.getValue())) {
        for (size_t i = 0; i < children.size(); ++i)
          if (auto child = dyn_cast<DictionaryAttr>(children[i]);
              child && child.getAs<StringAttr>("kind"))
            self(self, node.item(field.getName().getValue(), i));
      }
    }
  };
  for (size_t i = 0; i < decl.node.array("body").size(); ++i)
    classify(classify, decl.node.item("body", i));
  llvm::DenseMap<DictionaryAttr, AstNode> queueAssignments;
  for (size_t i = 0; i < decl.node.array("body").size(); ++i) {
    auto statement = decl.node.item("body", i);
    if (!queueCall(statement.child("value")))
      continue;
    auto targets = queueTargets(statement);
    if (failed(targets))
      return failure();
    for (StringRef name : *targets) {
      if (writes[name] != 1 || shadows.contains(name))
        return error()
               << "queue results require fresh immutable module-local Names: '"
               << name << "'";
      queueResultNames.insert(name);
    }
    queueAssignments.try_emplace(
        sourceOccurrence(at, decl.symbol, statement.child("value")), statement);
  }
  auto declare = [&](auto &&self, const AstNode &node) -> LogicalResult {
    if (node.kind() == "FunctionDef" || node.kind() == "ClassDef" ||
        node.kind() == "Lambda")
      return success();
    auto nextName = [&](StringRef prefix) {
      std::string name;
      do {
        name = (prefix + Twine(directCallOrdinal++)).str();
      } while (writes.contains(name) || shadows.contains(name) ||
               names.contains(name) || structs.contains(name) ||
               behavioralRules.contains(name));
      return name;
    };
    if (!memoryKind(node).empty()) {
      auto plan = memoryPlan(node, decl.symbol);
      if (!plan)
        return error() << "memory call is missing its inferred binding plan";
      auto op = createOp(
          at, node.location(b.getContext(), source.path),
          ac::InstanceOp::getOperationName(), {}, plan->resultTypes,
          {at.getNamedAttr("instance_name",
                           at.getStringAttr(nextName("__pyc_memory_"))),
           at.getNamedAttr("callee", plan->callee),
           at.getNamedAttr("parameters", at.getArrayAttr(plan->parameters)),
           at.getNamedAttr("type_arguments",
                           at.getArrayAttr(plan->typeArguments)),
           at.getNamedAttr("occurrence", plan->occurrence)});
      directCallSites.try_emplace(plan->occurrence, DirectCallSite{op});
      for (auto [ordinal, name] : llvm::enumerate(plan->resultNames)) {
        memoryResultNames.insert(name);
        forwardCallBindings[name] = {plan->occurrence, true,
                                     static_cast<unsigned>(ordinal)};
        if (plan->resultFixedBits[ordinal])
          fixedValues.insert(op->getResult(ordinal));
      }
      return success();
    }
    if (queueCall(node)) {
      auto found =
          queueAssignments.find(sourceOccurrence(at, decl.symbol, node));
      if (found == queueAssignments.end())
        return error() << "queue allocation requires a direct module-body "
                          "handle or three-name Tuple assignment";
      if (failed(
              declareQueue(found->second, decl, at, nextName("__pyc_queue_"))))
        return failure();
    } else if (moduleCall(node) && !methodOwner(node.child("func")) &&
               !structuralCallSites.contains(
                   sourceOccurrence(at, decl.symbol, node))) {
      auto callee = callable(node.child("func"));
      if (failed(callee))
        return failure();
      auto site = sourceOccurrence(at, decl.symbol, node);
      SmallVector<Type> results;
      for (const Port &result : callee->results)
        results.push_back(result.type);
      auto op =
          createOp(at, node.location(b.getContext(), source.path),
                   ac::InstanceOp::getOperationName(), {},
                   results,
                   {at.getNamedAttr("instance_name",
                                    at.getStringAttr(nextName("__pyc_call_"))),
                    at.getNamedAttr("callee", callee->symbol),
                    at.getNamedAttr("parameters", at.getArrayAttr({})),
                    at.getNamedAttr("type_arguments", at.getArrayAttr({})),
                    at.getNamedAttr("occurrence", site)});
      directCallSites.try_emplace(site, DirectCallSite{op});
      for (auto [value, contract] : llvm::zip(op->getResults(), callee->results)) {
        remember(value, contract.sourceKind);
        if (contract.fixedBits)
          fixedValues.insert(value);
      }
    }
    for (NamedAttribute field : node.fields()) {
      // Type annotations are declarations, not runtime expression sites.
      if (field.getName().getValue() == "annotation")
        continue;
      if (auto child = dyn_cast<DictionaryAttr>(field.getValue())) {
        if (child.getAs<StringAttr>("kind") &&
            failed(self(self, node.child(field.getName().getValue()))))
          return failure();
      } else if (auto children = dyn_cast<ArrayAttr>(field.getValue())) {
        for (size_t i = 0; i < children.size(); ++i)
          if (auto child = dyn_cast<DictionaryAttr>(children[i]);
              child && child.getAs<StringAttr>("kind"))
            if (failed(self(self, node.item(field.getName().getValue(), i))))
              return failure();
      }
    }
    return success();
  };
  for (size_t i = 0; i < decl.node.array("body").size(); ++i)
    if (failed(declare(declare, decl.node.item("body", i))))
      return failure();
  for (size_t i = 0; i < decl.node.array("body").size(); ++i) {
    auto stmt = decl.node.item("body", i);
    if (queueCall(stmt.child("value"))) {
      auto targets = queueTargets(stmt);
      if (failed(targets))
        return failure();
      auto site = sourceOccurrence(at, decl.symbol, stmt.child("value"));
      if (targets->size() == 3)
        for (auto [ordinal, name] : llvm::enumerate(*targets))
          forwardCallBindings[name] = {site, true,
                                       static_cast<unsigned>(ordinal)};
      continue;
    }
    if (!memoryKind(stmt.child("value")).empty())
      continue;
    if (stmt.kind() != "Assign" || stmt.array("targets").size() != 1)
      continue;
    auto rhs = stmt.child("value");
    auto site = sourceOccurrence(at, decl.symbol, rhs);
    if (!directCallSites.contains(site))
      continue;
    auto callee = callable(rhs.child("func"));
    if (failed(callee))
      return failure();
    SmallVector<StringRef> targets;
    // A named instance bundle has no scalar forward value. Its results are
    // unpacked later while its actual allocation identity remains available.
    if (callee->results.empty() ||
        (callee->returnShape == ReturnShape::Tuple &&
         stmt.item("targets", 0).kind() == "Name"))
      continue;
    if (callee->returnShape == ReturnShape::Tuple) {
      auto unpacked = moduleResultTargets(stmt, callee->results.size());
      if (failed(unpacked))
        return failure();
      targets = std::move(*unpacked);
    } else if (stmt.item("targets", 0).kind() == "Name")
      targets.push_back(stmt.item("targets", 0).string("id"));
    for (auto [ordinal, name] : llvm::enumerate(targets)) {
      bool eligible = writes[name] == 1 && !shadows.contains(name) &&
                      !names.contains(name) && !structs.contains(name) &&
                      !behavioralRules.contains(name) && name != "pyc_clk" &&
                      name != "pyc_rst";
      forwardCallBindings[name] = {site, eligible,
                                   static_cast<unsigned>(ordinal)};
    }
  }
  return success();
}
FailureOr<Value> Importer::forwardModuleValue(StringRef name,
                                              FlatSymbolRefAttr symbol) {
  auto found = forwardCallBindings.find(name);
  if (!activeBehavioralDecl || activeBehavioralDecl->symbol != symbol ||
      found == forwardCallBindings.end())
    return error() << "unknown hardware value '" << name << "'";
  if (!found->second.eligible)
    return error() << "ambiguous forward module result '" << name << "'";
  // Never visit the future call's actuals here. They bind at its lexical site.
  return directCallSites.find(found->second.site)
      ->second.op->getResult(found->second.ordinal);
}
FailureOr<SmallVector<StringRef>>
Importer::moduleResultTargets(const AstNode &assignment, size_t arity) {
  if (assignment.kind() != "Assign" || assignment.array("targets").size() != 1)
    return error() << "tuple module results require one flat Tuple target";
  auto target = assignment.item("targets", 0);
  if (target.kind() != "Tuple" || target.array("elts").size() != arity)
    return error() << "tuple module result target arity must match its declaration";
  SmallVector<StringRef> result;
  for (size_t i = 0; i < arity; ++i) {
    auto element = target.item("elts", i);
    if (element.kind() != "Name")
      return error() << "tuple module result targets must be ordinary Names";
    StringRef name = element.string("id");
    if (failed(requireNestedRuleDataTarget(name, assignment)))
      return failure();
    if (name.empty() || names.contains(name) || structs.contains(name) ||
        enums.contains(name) ||
        (behavioralRules.contains(name) && !nestedRuleBinding(name)) ||
        llvm::is_contained(ArrayRef<StringRef>{"pyc_clk", "pyc_rst", "module",
                                               "rule", "method", "system", "log",
                                               "report", "bool", "int", "range",
                                               "Annotated", "tuple"}, name))
      return error() << "tuple module result target shadows a protected binding: '"
                     << name << "'";
    if (llvm::is_contained(result, name))
      return error() << "tuple module result targets must be distinct Names";
    result.push_back(name);
  }
  return result;
}
LogicalResult Importer::bindCallableResults(
    const AstNode &assignment, ValueRange results, OpBuilder &at,
    FlatSymbolRefAttr symbol, llvm::StringMap<Value> &values,
    llvm::StringMap<Instance *> &instances,
    const llvm::StringSet<> &forbidden,
    llvm::StringMap<BindingBoundary> *boundaries) {
  auto targets = moduleResultTargets(assignment, results.size());
  if (failed(targets)) return failure();
  SmallVector<Value> converted;
  for (auto [index, name] : llvm::enumerate(*targets)) {
    if (forbidden.contains(name) || instances.contains(name) ||
        queueResultNames.contains(name) || memoryResultNames.contains(name))
      return error() << "tuple callable result cannot write a protected owner: '"
                     << name << "'";
    auto boundary = boundaries && boundaries->contains(name)
        ? boundaries->lookup(name)
        : inferredBoundary(name, results[index], assignment);
    auto value = applyBindingBoundary(results[index], boundary,
                                       assignment.child("value"), at, symbol);
    if (failed(value)) return failure();
    converted.push_back(*value);
  }
  for (auto [index, name] : llvm::enumerate(*targets)) {
    values[name] = converted[index];
    noteNestedRuleDataBinding(name);
  }
  return success();
}

FailureOr<SmallVector<Value>> Importer::bindCallableArguments(
    const AstNode &call, ArrayRef<Port> inputs, OpBuilder &at,
    FlatSymbolRefAttr symbol, llvm::StringMap<Value> &values,
    llvm::StringMap<Instance *> &instances,
    ArrayRef<unsigned> deferredReferences) {
  SmallVector<StringRef> names;
  for (const Port &port : inputs) names.push_back(port.name);
  SmallVector<size_t> evaluationOrder;
  auto actuals = normalizeCallArguments(call, names, error,
      "callable argument arity mismatch", &evaluationOrder);
  if (failed(actuals)) return failure();
  SmallVector<Value> arguments(inputs.size());
  // Bind by formal identity while evaluating effectful Method actuals in their
  // source order. Keyword permutation must not reorder candidate updates.
  auto bind = [&](size_t index) -> LogicalResult {
    const auto &actual = (*actuals)[index];
    const auto &port = inputs[index];
    auto value = expression(actual, at, symbol, values, instances,
                            port.type ? std::optional<Type>(port.type) : std::nullopt);
    if (failed(value)) return failure();
    if (!port.type) {
      arguments[index] = *value;
      return success();
    }
    auto converted = applyBindingBoundary(*value, BindingBoundary{port, true},
                                           actual, at, symbol);
    if (failed(converted)) return failure();
    arguments[index] = *converted;
    return success();
  };
  for (size_t index : evaluationOrder)
    if (!llvm::is_contained(deferredReferences, index) && failed(bind(index)))
      return failure();
  // Owner actuals denote the candidate at call entry, after all ordinary
  // argument expressions have executed. Saved value actuals remain snapshots.
  for (size_t index : evaluationOrder)
    if (llvm::is_contained(deferredReferences, index) && failed(bind(index)))
      return failure();
  return arguments;
}

FailureOr<bool> Importer::bindModuleResults(
    const AstNode &assignment, OpBuilder &at, FlatSymbolRefAttr symbol,
    llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances,
    const llvm::StringSet<> &forbidden) {
  auto call = assignment.child("value");
  if (!moduleCall(call) || methodOwner(call.child("func")))
    return false;
  auto callee = callable(call.child("func"));
  if (failed(callee))
    return failure();
  if (callee->returnShape != ReturnShape::Tuple)
    return false;
  auto targets = moduleResultTargets(assignment, callee->results.size());
  if (failed(targets))
    return failure();
  for (StringRef name : *targets)
    if (forbidden.contains(name) || instances.contains(name) ||
        queueResultNames.contains(name) || memoryResultNames.contains(name))
      return error() << "tuple module result cannot write a protected owner: '"
                     << name << "'";
  auto results = directModuleResults(call, at, symbol, values, instances);
  if (failed(results))
    return failure();
  SmallVector<Value> converted;
  for (auto [index, name] : llvm::enumerate(*targets)) {
    auto binding = inferredBoundary(name, (*results)[index], assignment);
    auto value = applyBindingBoundary((*results)[index], binding, call, at, symbol);
    if (failed(value))
      return failure();
    converted.push_back(*value);
  }
  // Publish the names only after all targets and conversions have succeeded.
  for (auto [index, name] : llvm::enumerate(*targets)) {
    values[name] = converted[index];
    noteNestedRuleDataBinding(name);
  }
  return true;
}
FailureOr<Value> Importer::directModuleCall(
    const AstNode &call, OpBuilder &at, FlatSymbolRefAttr symbol,
    llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances) {
  auto callee = callable(call.child("func"));
  if (failed(callee))
    return failure();
  if (callee->returnShape == ReturnShape::Tuple)
    return error() << "tuple module call requires direct Tuple unpacking";
  auto results = directModuleResults(call, at, symbol, values, instances);
  if (failed(results))
    return failure();
  if (results->size() != 1)
    return error() << "Module instance without a scalar result cannot be used as a value";
  return results->front();
}
FailureOr<SmallVector<Value>> Importer::directModuleResults(
    const AstNode &call, OpBuilder &at, FlatSymbolRefAttr symbol,
    llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances) {
  if (!isa<ac::ModuleOp>(at.getInsertionBlock()->getParentOp()))
    return error() << "direct module calls are only admitted at module scope";
  auto found = directCallSites.find(sourceOccurrence(at, symbol, call));
  if (found == directCallSites.end())
    return error() << "direct module call requires a static module site";
  if (found->second.bound)
    return SmallVector<Value>(found->second.op->getResults());
  auto normalized = callable(call.child("func"));
  if (failed(normalized))
    return failure();
  const CallableDecl &callee = *normalized;
  SmallVector<AstNode> actuals(callee.inputs.size());
  SmallVector<size_t> lexicalOrder;
  if (call.array("args").size() > actuals.size())
    return error() << "too many direct module arguments";
  for (size_t i = 0; i < call.array("args").size(); ++i) {
    actuals[i] = call.item("args", i);
    lexicalOrder.push_back(i);
  }
  for (size_t i = 0; i < call.array("keywords").size(); ++i) {
    auto kw = call.item("keywords", i);
    auto name = dyn_cast_or_null<StringAttr>(kw.get("arg"));
    size_t ordinal = actuals.size();
    for (size_t j = 0; name && j < actuals.size(); ++j)
      if (callee.inputs[j].name == name.getValue())
        ordinal = j;
    if (ordinal == actuals.size() || actuals[ordinal])
      return error() << "unknown or repeated direct module argument";
    actuals[ordinal] = kw.child("value");
    lexicalOrder.push_back(ordinal);
  }
  for (const AstNode &actual : actuals) {
    if (!actual)
      return error() << "missing direct module argument";
    if (actual.kind() == "Starred")
      return error() << "direct module calls do not admit *args";
  }
  SmallVector<Value> inputs(callee.inputs.size());
  for (size_t i : lexicalOrder) {
    auto value = expression(actuals[i], at, symbol, values, instances,
                            callee.inputs[i].type);
    if (failed(value))
      return failure();
    auto converted =
        applyBindingBoundary(*value, BindingBoundary{callee.inputs[i], true},
                             actuals[i], at, symbol);
    if (failed(converted))
      return failure();
    inputs[i] = *converted;
  }
  if (callee.needsDomain) {
    if (!values.contains("pyc_clk") || !values.contains("pyc_rst"))
      return error() << "direct module call lacks inferred physical domain";
    inputs.push_back(values["pyc_clk"]);
    inputs.push_back(values["pyc_rst"]);
  }
  found->second.op->setOperands(inputs);
  found->second.bound = true;
  return SmallVector<Value>(found->second.op->getResults());
}
FailureOr<Value> Importer::stateInitializer(
    const Port &contract, const AstNode &initializer, const AstNode &site,
    OpBuilder &at, FlatSymbolRefAttr symbol, llvm::StringMap<Value> &values,
    llvm::StringMap<Instance *> &instances, bool allowCapturedConstants) {
  ac::HardwareAnalysis analysis(*body);
  if (auto table = dyn_cast<ac::TableType>(contract.type)) {
    if (failed(analysis.getTableSize(table, {},
                                     at.getInsertionBlock()->getParentOp())) ||
        failed(analysis.getPackedWidth(table.getElementType(), {},
                                       at.getInsertionBlock()->getParentOp())))
      return failure();
  } else if (failed(analysis.getPackedWidth(
                 contract.type, {}, at.getInsertionBlock()->getParentOp())))
    return failure();
  // Typed states additionally admit closed captured constants; SourceStateOp
  // verifies the complete actual initializer SSA before publication.
  if (!allowCapturedConstants && !staticInitializer(initializer))
    return error() << "persistent initializer must be a static complete value";
  auto tableType = dyn_cast<ac::TableType>(contract.type);
  auto encoded =
      initializer.kind() == "Constant"
          ? dyn_cast_or_null<DictionaryAttr>(initializer.get("value"))
          : DictionaryAttr();
  auto literalInteger =
      encoded ? encoded.getAs<StringAttr>("integer") : StringAttr();
  bool recursiveZero = literalInteger && literalInteger.getValue() == "0";
  bool elementImage = tableType && !recursiveZero;
  Type initType = elementImage ? tableType.getElementType() : contract.type;
  auto init =
      tableType && recursiveZero
          ? constant(contract.type, 0, initializer, at, symbol)
          : expression(initializer, at, symbol, values, instances, initType);
  if (failed(init))
    return failure();
  BindingBoundary initialBoundary{
      {contract.name, initType, contract.node, contract.sourceKind,
       contract.fixedBits || (tableType && isa<ac::BitsType>(initType))},
      true};
  auto convertedInit =
      applyBindingBoundary(*init, initialBoundary, initializer, at, symbol);
  if (failed(convertedInit))
    return failure();
  init = *convertedInit;
  if (elementImage)
    init = createOp(at, site.location(b.getContext(), source.path),
                    ac::TableSplatOp::getOperationName(), {*init},
                    TypeRange{contract.type},
                    {at.getNamedAttr("shape", tableType.getShape())})
               ->getResult(0);
  return *init;
}

FailureOr<SmallVector<Value>>
Importer::lowerReturn(const ModuleDecl &decl, const AstNode &node,
                      OpBuilder &at, llvm::StringMap<Value> &values,
                      llvm::StringMap<Instance *> &instances) {
  SmallVector<Value> outputs;
  if (decl.returnShape == ReturnShape::None) {
    if (node && !(node.kind() == "Constant" && isa<UnitAttr>(node.get("value"))))
      return error() << "None Module cannot return an ordinary value";
    return outputs;
  }
  if (decl.returnShape == ReturnShape::Struct) {
    auto value = expression(node, at, decl.symbol, values, instances,
                            decl.outputs.front().type);
    if (failed(value))
      return failure();
    const Port &output = decl.outputs.front();
    auto converted = output.sourceKind
        ? applyBindingBoundary(*value, BindingBoundary{output, true}, node,
                               at, decl.symbol)
        : unsignedBoundary(*value, output.type, node, at, decl.symbol,
                           output.fixedBits);
    if (failed(converted))
      return failure();
    outputs.push_back(*converted);
    return outputs;
  }
  bool tuple = decl.returnShape == ReturnShape::Tuple;
  if (tuple) {
    if (node.kind() != "Tuple" || node.array("elts").size() != decl.outputs.size())
      return error() << "tuple module return requires an explicit Tuple with matching arity";
  } else if (node.kind() != "Dict" ||
             node.array("keys").size() != decl.outputs.size())
    return error() << "return requires matching named output mapping";
  for (size_t i = 0; i < decl.outputs.size(); ++i) {
    if (!tuple) {
      auto key = dyn_cast_or_null<StringAttr>(node.item("keys", i).get("value"));
      if (!key || key.getValue() != decl.outputs[i].name)
        return error() << "return output key/order mismatch";
    }
    AstNode element = node.item(tuple ? "elts" : "values", i);
    auto value = expression(element, at, decl.symbol, values,
                            instances, decl.outputs[i].type);
    if (failed(value))
      return failure();
    auto sourceKind = valueInfo(*value).sourceKind;
    if (decl.outputs[i].sourceKind && sourceKind &&
        decl.outputs[i].sourceKind != sourceKind)
      return error() << "source Boolean and Integer kinds cannot be implicitly "
                        "converted";
    auto converted = tuple && decl.outputs[i].sourceKind
                         ? applyBindingBoundary(
                               *value, BindingBoundary{decl.outputs[i], true},
                               element, at, decl.symbol)
                         : unsignedBoundary(*value, decl.outputs[i].type, element,
                                            at, decl.symbol,
                                            decl.outputs[i].fixedBits);
    if (failed(converted))
      return failure();
    outputs.push_back(*converted);
  }
  return outputs;
}
void Importer::bindState(const StateOwner &state, Value next, Value en,
                         OpBuilder &at, llvm::StringMap<Value> &values) {
  SmallVector<Value> inputs{values["pyc_clk"], values["pyc_rst"], en, next,
                            state.init};
  if (auto table = dyn_cast<ac::TableType>(state.type))
    for (size_t i : {size_t(0), size_t(1), size_t(2)})
      inputs[i] =
          createOp(at, state.op->getLoc(), ac::TableSplatOp::getOperationName(),
                   {inputs[i]},
                   TypeRange{ac::TableType::get(
                       b.getContext(), table.getShape(), inputs[i].getType())},
                   {at.getNamedAttr("shape", table.getShape())})
              ->getResult(0);
  state.op->setOperands(inputs);
}

LogicalResult Importer::bindProposals(ArrayRef<StateOwner> states,
                                      OpBuilder &at,
                                      llvm::StringMap<Value> &values,
                                      FlatSymbolRefAttr symbol) {
  if (!activeBehavioralDecl ||
      failed(preflightProposals(activeBehavioralDecl->node)))
    return failure();
  for (auto [index, state] : llvm::enumerate(states)) {
    SmallVector<const OwnerProposal *> proposals;
    for (const auto &proposal : ownerProposals)
      if (proposal.ownerIndex == index)
        proposals.push_back(&proposal);
    if (proposals.empty())
      continue;
    if (proposals.size() == 1) {
      bindState(state, proposals.front()->next, proposals.front()->enable, at,
                values);
      continue;
    }
    ac::HardwareAnalysis analysis(*body);
    auto loc = state.op->getLoc();
    auto predicateType = bits(1, state.node, symbol);
    auto gate = [&](OpBuilder &builder, StringRef opcode, Value a, Value c) {
      return createOp(builder, loc, ac::BitsBinaryOp::getOperationName(),
                      {a, c}, TypeRange{predicateType},
                      {builder.getNamedAttr("opcode",
                                            builder.getStringAttr(opcode))})
          ->getResult(0);
    };
    auto project = [&](Value value, ArrayRef<StringAttr> path,
                       OpBuilder &builder) -> FailureOr<Value> {
      for (auto name : path) {
        auto record = dyn_cast<ac::StructType>(value.getType());
        auto declaration =
            record ? analysis.lookupStruct(record) : ac::StructOp();
        Type selected;
        if (declaration)
          for (Attribute raw : declaration.getFields()) {
            auto field = cast<DictionaryAttr>(raw);
            if (field.getAs<StringAttr>("name") == name) {
              selected = field.getAs<TypeAttr>("type").getValue();
              break;
            }
          }
        if (!selected)
          return error()
                 << "analyzed writer path requires a declared Struct field";
        value =
            createOp(builder, loc, ac::StructGetOp::getOperationName(), {value},
                     TypeRange{selected}, {builder.getNamedAttr("field", name)})
                ->getResult(0);
      }
      return value;
    };
    SmallVector<size_t> domainOffsets;
    SmallVector<Value> addressMasks;
    for (const auto *proposal : proposals) {
      if (!grantProof->charge(1 + proposal->domains.size()))
        return error() << grantProof->failureReason();
      domainOffsets.push_back(addressMasks.size());
      for (const auto &witness : proposal->domains)
        addressMasks.push_back(witness.mask);
    }
    // Compose each declared scalar leaf exactly once. Table candidates already
    // contain the rule's sequential updates and old-Q for untouched elements.
    auto compose = [&](Value old, ArrayRef<Value> candidates,
                       ArrayRef<Value> enables, Value ordinal,
                       ArrayRef<Value> masks,
                       OpBuilder &builder) -> FailureOr<Value> {
      auto leaves = analysis.getFieldPaths(old.getType(), {}, state.op);
      if (failed(leaves))
        return failure();
      auto zero = constant(predicateType, 0, state.node, builder, symbol);
      auto one = constant(predicateType, 1, state.node, builder, symbol);
      if (failed(zero) || failed(one))
        return failure();
      SmallVector<Value> guards, leafValues;
      SmallVector<Attribute> paths;
      for (const auto &leaf : *leaves) {
        auto current = project(old, leaf, builder);
        if (failed(current))
          return failure();
        if (!isa<ac::BitsType, ac::EnumType>(current->getType()))
          return error() << "multiwriter composition requires scalar Bits or "
                            "Enum leaves";
        Value guard = *zero;
        bool written = false;
        for (auto [p, proposal] : llvm::enumerate(proposals)) {
          Value domain = *zero;
          bool selected = false;
          for (auto [pathIndex, path] :
               llvm::enumerate(proposal->write->paths)) {
            if (!grantProof->charge(1 + path.selectors.size()))
              return error() << grantProof->failureReason();
            size_t firstField = 0;
            const SourceWriteSelector *selector = nullptr;
            if (ordinal && !path.selectors.empty()) {
              selector = &path.selectors.front();
              if (selector->kind != SourceWriteSelector::Kind::Index)
                return error()
                       << "Table writer path requires one leading index";
              firstField = 1;
            }
            auto fields = ArrayRef<SourceWriteSelector>(path.selectors)
                              .drop_front(firstField);
            for (const auto &field : fields)
              if (field.kind != SourceWriteSelector::Kind::Field)
                return error()
                       << "multiwriter composition admits only one Table index";
            if (fields.size() > leaf.size() ||
                !llvm::all_of(llvm::enumerate(fields), [&](auto entry) {
                  return leaf[entry.index()].getValue() == entry.value().field;
                }))
              continue;
            Value mask = *one;
            if (selector) {
              bool found = false;
              for (auto [slot, witness] : llvm::enumerate(proposal->domains)) {
                if (!grantProof->charge())
                  return error() << grantProof->failureReason();
                if (witness.pathIndex != pathIndex)
                  continue;
                size_t offset = domainOffsets[p] + slot;
                if (found || offset >= masks.size())
                  return error() << "invalid composition address mask slot";
                mask = masks[offset];
                found = true;
              }
              if (!found)
                return error()
                       << "missing composition assignment-time address mask";
            }
            domain = gate(builder, "or", domain, mask);
            selected = true;
          }
          if (!selected)
            continue;
          auto candidate = project(candidates[p], leaf, builder);
          if (failed(candidate))
            return failure();
          Value effective = gate(builder, "and", domain, enables[p]);
          auto chosen = select(effective, *candidate, *current, state.node,
                               builder, symbol);
          if (failed(chosen))
            return failure();
          current = *chosen;
          guard = gate(builder, "or", guard, effective);
          written = true;
        }
        if (written) {
          guards.push_back(guard);
          leafValues.push_back(*current);
          paths.push_back(builder.getArrayAttr(
              SmallVector<Attribute>(leaf.begin(), leaf.end())));
        }
      }
      SmallVector<Value> operands{old};
      llvm::append_range(operands, guards);
      llvm::append_range(operands, leafValues);
      auto merge =
          createOp(builder, loc, ac::ValueMergeOp::getOperationName(), operands,
                   TypeRange{old.getType(), predicateType},
                   {builder.getNamedAttr("paths", builder.getArrayAttr(paths)),
                    builder.getNamedAttr(
                        "operandSegmentSizes",
                        builder.getDenseI32ArrayAttr(
                            {1, static_cast<int32_t>(guards.size()),
                             static_cast<int32_t>(leafValues.size())}))});
      return merge->getResult(0);
    };
    SmallVector<Value> candidates, enables;
    for (const auto *proposal : proposals) {
      candidates.push_back(proposal->next);
      enables.push_back(proposal->enable);
    }
    Value next;
    if (auto table = dyn_cast<ac::TableType>(state.type)) {
      if (table.getShape().size() != 1)
        return error()
               << "multiwriter Table composition requires one dimension";
      auto size = analysis.getTableSize(table, {}, state.op);
      if (failed(size))
        return failure();
      if (!grantProof->chargeProduct(*size, addressMasks.size()) ||
          !grantProof->charge(1 + candidates.size() + addressMasks.size() +
                              enables.size()))
        return error() << grantProof->failureReason();
      SmallVector<Value> operands{state.op->getResult(0)};
      llvm::append_range(operands, candidates);
      llvm::append_range(operands, addressMasks);
      llvm::append_range(operands, enables);
      auto map = createOp(
          at, loc, ac::TableMapOp::getOperationName(), operands,
          TypeRange{state.type},
          {at.getNamedAttr("shape", table.getShape()),
           at.getNamedAttr("operandSegmentSizes",
                           at.getDenseI32ArrayAttr(
                               {static_cast<int32_t>(1 + candidates.size() +
                                                     addressMasks.size()),
                                static_cast<int32_t>(enables.size())}))},
          1);
      auto block = new Block();
      map->getRegion(0).push_back(block);
      auto ordinal = block->addArgument(
          bits(std::max(1u, llvm::Log2_64_Ceil(*size)), state.node, symbol),
          loc);
      auto old = block->addArgument(table.getElementType(), loc);
      SmallVector<Value> elementCandidates, elementEnables, elementMasks;
      for (size_t p = 0; p < proposals.size(); ++p)
        elementCandidates.push_back(
            block->addArgument(table.getElementType(), loc));
      for (Value mask : addressMasks)
        elementMasks.push_back(block->addArgument(
            cast<ac::TableType>(mask.getType()).getElementType(), loc));
      for (Value enable : enables)
        elementEnables.push_back(block->addArgument(enable.getType(), loc));
      auto nested = OpBuilder::atBlockEnd(block);
      auto merged = compose(old, elementCandidates, elementEnables, ordinal,
                            elementMasks, nested);
      if (failed(merged))
        return failure();
      createOp(nested, loc, ac::YieldOp::getOperationName(), {*merged}, {}, {});
      next = map->getResult(0);
    } else {
      auto merged =
          compose(state.op->getResult(0), candidates, enables, {}, {}, at);
      if (failed(merged))
        return failure();
      next = *merged;
    }
    auto zero = constant(predicateType, 0, state.node, at, symbol);
    if (failed(zero))
      return failure();
    Value active = *zero, poison = *zero;
    for (Value enable : enables) {
      active = gate(at, "or", active, enable);
      poison = gate(at, "or", poison, gate(at, "xor", enable, enable));
    }
    // Only original rule-owner enables control the single outside binding.
    bindState(state, next, gate(at, "xor", active, poison), at, values);
  }
  return success();
}
