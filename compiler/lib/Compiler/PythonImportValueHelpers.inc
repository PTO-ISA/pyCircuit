// Ordinary definitions share the Importer's statement/expression owners. This
// file owns declaration frames and typed-template transport, never expression
// semantics or hardware execution.
LogicalResult Importer::preflightValueHelper(const AstNode &node,
                                             llvm::StringSet<> &freeNames,
                                             llvm::StringSet<> &localNames) {
  auto diagnostic = [&](const AstNode &site) {
    return mlir::emitError(site.location(b.getContext(), source.path));
  };
  if (!node.array("decorator_list").empty() || !node.child("returns"))
    return diagnostic(node) << "pure helper requires an ordinary typed def";
  auto args = node.child("args");
  if (!args.array("posonlyargs").empty() || !args.array("kwonlyargs").empty() ||
      !args.array("defaults").empty() || !args.array("kw_defaults").empty() ||
      args.child("vararg") || args.child("kwarg"))
    return diagnostic(node) << "pure helper requires required positional parameters";
  llvm::StringSet<> locals;
  for (size_t i = 0; i < args.array("args").size(); ++i) {
    auto argument = args.item("args", i);
    if (!argument.child("annotation") ||
        !locals.insert(argument.string("arg")).second)
      return diagnostic(argument) << "pure helper parameter requires a unique name and annotation";
  }
  SourceWalkSummary sourceWork;
  SmallVector<SmallVector<StringRef, 2>, 2> callbackParameters;
  unsigned loopDepth = 0;
  auto scan = [&](auto &&self, const AstNode &site, bool statement,
                  bool topLevel, bool target = false) -> LogicalResult {
    if (!site)
      return success();
    std::string reason;
    if (!queryBudget.enter(reason))
      return diagnostic(site) << reason;
    auto leave = llvm::scope_exit([&] { queryBudget.leave(); });
    if (failed(chargeSourceWalk(site, sourceWork)))
      return failure();
    StringRef kind = site.kind();
    if (statement) {
      if (docstring(site) || kind == "Pass") return success();
      if (kind == "Return") {
        if (!topLevel)
          return diagnostic(site) << "pure helper return must be final and unconditional";
        return self(self, site.child("value"), false, false);
      }
      if (kind == "If") {
        if (failed(self(self, site.child("test"), false, false))) return failure();
        for (StringRef field : {"body", "orelse"})
          for (size_t i = 0; i < site.array(field).size(); ++i)
            if (failed(self(self, site.item(field, i), true, false))) return failure();
        return success();
      }
      if (kind == "For") {
        if (loopDepth)
          return diagnostic(site) << "nested bounded for is unsupported";
        auto syntax = readLiteralForSyntax(site, [&] { return diagnostic(site); });
        if (failed(syntax)) return failure();
        locals.insert(syntax->target.string("id"));
        if (failed(self(self, syntax->target, false, false, true)) ||
            failed(self(self, syntax->bound, false, false))) return failure();
        ++loopDepth;
        auto leaveLoop = llvm::scope_exit([&] { --loopDepth; });
        for (size_t i = 0; i < site.array("body").size(); ++i)
          if (failed(self(self, site.item("body", i), true, false))) return failure();
        return success();
      }
      if (kind == "Assign" || kind == "AnnAssign") {
        if (kind == "Assign" && site.array("targets").size() != 1)
          return diagnostic(site) << "pure helper assignment requires one target";
        auto lhs = kind == "Assign" ? site.item("targets", 0) : site.child("target");
        // Fixed-result intrinsics use the shared assignment binder during
        // lowering. Record their lexical targets without inventing tuple values.
        if (!fixedResultCall(site.child("value")).empty()) {
          auto targets = fixedResultTargets(site);
          if (failed(targets)) return failure();
          for (StringRef name : **targets) locals.insert(name);
          return self(self, site.child("value"), false, false);
        }
        // Locals have Python lexical identity throughout the helper, and never
        // accidentally capture a same-spelled enclosing wire.
        StringRef root = assignmentRoot(lhs);
        if (root.empty() || (lhs.kind() != "Name" && lhs.kind() != "Attribute" &&
                             lhs.kind() != "Subscript"))
          return diagnostic(lhs) << "pure helper assignment requires a local Name, Struct field or Table element";
        if (lhs.kind() == "Name") locals.insert(root);
        if (failed(self(self, lhs, false, false, true))) return failure();
        return self(self, site.child("value"), false, false);
      }
      if (kind == "Expr" && site.child("value").kind() == "Call")
        return self(self, site.child("value"), false, false);
      return diagnostic(site) << "unsupported pure helper statement '" << kind << "'";
    }
    if (kind == "Name") {
      bool callbackLocal = llvm::any_of(callbackParameters, [&](const auto &parameters) {
        return llvm::is_contained(parameters, site.string("id"));
      });
      if (!target && !callbackLocal) freeNames.insert(site.string("id"));
      return success();
    }
    if (target && (kind == "Attribute" || kind == "Subscript")) {
      // Updating an immutable aggregate reads its base and index. Only a Name
      // target binds a local; indices retain ordinary lexical capture rules.
      if (failed(self(self, site.child("value"), false, false))) return failure();
      return kind == "Subscript"
                 ? self(self, site.child("slice"), false, false)
                 : success();
    }
    if (llvm::is_contained(ArrayRef<StringRef>{"Lambda", "NamedExpr", "Await",
          "Yield", "YieldFrom", "ListComp", "SetComp", "DictComp",
          "GeneratorExp", "Starred", "FunctionDef", "ClassDef"}, kind))
      return diagnostic(site) << "unsupported pure helper expression '" << kind << "'";
    if (kind == "Call") {
      auto function = site.child("func");
      StringRef name = function.kind() == "Name" ? function.string("id") : StringRef();
      if (behavioralRules.contains(name) || local.contains(name) || queueCall(site) ||
          !memoryKind(site).empty() || methodOwner(function) ||
          intrinsic(function, "log") || intrinsic(function, "report"))
        return diagnostic(site) << "pure helper cannot call Rule, Method, allocation or IO";
      auto binding = lookupBinding(function);
      if (binding && binding->category == BindingCategory::Module)
        return diagnostic(site) << "pure helper cannot instantiate a module";
      // Only the existing Table owners admit callback Lambdas: the first map
      // actual, first/argmin's where, and argmin's key. Actual receiver typing,
      // scalar purity and capture staging remain with preflightQueryCallback.
      StringRef method = function.kind() == "Attribute" ? function.string("attr") : StringRef();
      auto argument = [&](const AstNode &actual, bool callback, size_t arity) -> LogicalResult {
        if (!callback || actual.kind() != "Lambda")
          return self(self, actual, false, false);
        if (failed(validateTableQueryLambda(actual, [&] { return diagnostic(actual); }, arity)) ||
            failed(chargeSourceWalk(actual, sourceWork))) return failure();
        SmallVector<StringRef, 2> parameters;
        auto arguments = actual.child("args");
        TableQueryCharge storage;
        bool fits = TableQueryBudget::add(storage, TableQueryResource::Slots, arity + 1) &&
                    TableQueryBudget::add(storage, TableQueryResource::Work, arity + 1);
        std::string reason;
        if (!fits || !queryBudget.reserve(storage, reason))
          return diagnostic(actual) << (fits ? reason : "Table callback binding storage exhausted");
        for (StringRef field : {"posonlyargs", "args"})
          for (size_t i = 0; i < arguments.array(field).size(); ++i) {
            auto parameter = arguments.item(field, i);
            if (failed(chargeSourceWalk(parameter, sourceWork))) return failure();
            parameters.push_back(parameter.string("arg"));
          }
        callbackParameters.push_back(std::move(parameters));
        auto restore = llvm::scope_exit([&] { callbackParameters.pop_back(); });
        return self(self, actual.child("body"), false, false);
      };
      // The callee's declaration is resolved by the typed call owner. It is
      // not a runtime lexical capture.
      for (size_t i = 0; i < site.array("args").size(); ++i)
        if (failed(argument(site.item("args", i), method == "map" && i == 0,
                            site.array("args").size()))) return failure();
      for (size_t i = 0; i < site.array("keywords").size(); ++i) {
        auto keyword = site.item("keywords", i);
        bool callback = ((method == "first" || method == "argmin") && keyword.string("arg") == "where") ||
                        (method == "argmin" && keyword.string("arg") == "key");
        if (failed(chargeSourceWalk(keyword, sourceWork)) ||
            failed(argument(keyword.child("value"), callback, 1))) return failure();
      }
      if (function.kind() == "Attribute")
        return self(self, function.child("value"), false, false);
      if (function.kind() == "Subscript")
        return self(self, function.child("slice"), false, false);
      return success();
    }
    for (NamedAttribute field : site.fields()) {
      auto key = field.getName().getValue();
      if (auto child = dyn_cast<DictionaryAttr>(field.getValue())) {
        if (child.getAs<StringAttr>("kind") &&
            failed(self(self, site.child(key), false, false, target))) return failure();
      } else if (auto children = dyn_cast<ArrayAttr>(field.getValue())) {
        for (size_t i = 0; i < children.size(); ++i)
          if (auto child = dyn_cast<DictionaryAttr>(children[i]); child && child.getAs<StringAttr>("kind"))
            if (failed(self(self, site.item(key, i), false, false, target))) return failure();
      }
    }
    return success();
  };
  bool returned = false;
  for (size_t i = 0; i < node.array("body").size(); ++i) {
    auto statement = node.item("body", i);
    if (returned && !docstring(statement))
      return diagnostic(statement) << "pure helper return must be final";
    if (failed(scan(scan, statement, true, true))) return failure();
    returned |= statement.kind() == "Return";
  }
  if (!returned && !(node.child("returns").kind() == "Constant" &&
                     isa_and_nonnull<UnitAttr>(node.child("returns").get("value"))))
    return diagnostic(node) << "valued pure helper requires one final return";
  for (const auto &entry : locals) {
    freeNames.erase(entry.getKey());
    localNames.insert(entry.getKey());
  }
  return success();
}

LogicalResult Importer::declareValueHelper(const AstNode &node,
                                           FlatSymbolRefAttr lexicalOwner,
                                           const llvm::StringMap<Value> &values) {
  std::string key = lexicalOwner ? (Twine(lexicalOwner.getValue()) + "." + node.string("name")).str()
                                : qualify(node.string("name"));
  if (valueHelpers.contains(key))
    return error() << "duplicate pure helper declaration '" << key << "'";
  auto helper = std::make_unique<ValueHelperDecl>();
  helper->node = node;
  helper->symbol = FlatSymbolRefAttr::get(b.getContext(), key);
  helper->lexicalOwner = lexicalOwner;
  llvm::StringSet<> freeNames;
  if (failed(preflightValueHelper(node, freeNames, helper->localNames))) return failure();
  SmallVector<StringRef> ordered;
  for (const auto &name : freeNames) ordered.push_back(name.getKey());
  llvm::sort(ordered);
  for (StringRef name : ordered) {
    if (failed(requireNestedRuleDataVisible(name, node))) return failure();
    auto found = values.find(name);
    if (found == values.end()) continue; // Unbound names fail in expression.
    Value value = found->second;
    Value root = originalCaptures.lookup(value);
    if (root) value = root;
    if (name == "pyc_clk" || name == "pyc_rst" || helperOwnerBindings.contains(name))
      return mlir::emitError(node.location(b.getContext(), source.path))
             << "pure helper cannot implicitly capture a state or instance owner";
    helper->captures.push_back({name.str(), value});
  }
  auto args = node.child("args");
  SmallVector<Type> inputs, results;
  SmallVector<Attribute> parameters, constraints, captureTypes;
  for (size_t i = 0; i < args.array("args").size(); ++i) {
    auto formal = args.item("args", i);
    auto annotation = formal.child("annotation");
    auto type = portType(annotation, helper->symbol);
    if (failed(type)) return failure();
    Port port{formal.string("arg").str(), *type, formal,
              annotationKind(annotation), fixedAnnotation(annotation)};
    auto constraint = sourceConstraint(port);
    if (failed(constraint)) return failure();
    helper->inputs.push_back(port);
    inputs.push_back(*type);
    parameters.push_back(b.getDictionaryAttr({
        b.getNamedAttr("name", b.getStringAttr(port.name)),
        b.getNamedAttr("binding", b.getStringAttr("positional_or_keyword")),
        b.getNamedAttr("constraint", *constraint),
        b.getNamedAttr("default", absentDefault(b)),
        b.getNamedAttr("origin", sourceOccurrence(b, helper->symbol, formal)),
        b.getNamedAttr("location", sourceSpan(b, source.path, formal))}));
  }
  auto annotation = node.child("returns");
  bool none = annotation.kind() == "Constant" && isa_and_nonnull<UnitAttr>(annotation.get("value"));
  if (!none) {
    auto type = portType(annotation, helper->symbol);
    if (failed(type)) return failure();
    Port result{"result", *type, annotation, annotationKind(annotation), fixedAnnotation(annotation)};
    auto constraint = sourceConstraint(result);
    if (failed(constraint)) return failure();
    helper->results.push_back(result);
    results.push_back(*type);
    constraints.push_back(*constraint);
  }
  for (const auto &capture : helper->captures) {
    inputs.push_back(capture.second.getType());
    captureTypes.push_back(TypeAttr::get(capture.second.getType()));
  }
  OpBuilder top = OpBuilder::atBlockEnd(body->getBody());
  auto function = top.create<func::FuncOp>(node.location(b.getContext(), source.path), key,
                                           FunctionType::get(b.getContext(), inputs, results));
  helper->function = function;
  function->setAttr("ac.helper_kind", b.getStringAttr("value"));
  function->setAttr("ac.source_owner", owner);
  function->setAttr("ac.origin", sourceOccurrence(b, helper->symbol, node));
  function->setAttr("ac.declaration_role", b.getStringAttr("definition"));
  function->setAttr("ac.parameters", b.getArrayAttr(parameters));
  function->setAttr("ac.result_constraints", b.getArrayAttr(constraints));
  function->setAttr("ac.return_form", b.getStringAttr(none ? "none" : "single"));
  function->setAttr("ac.capture_types", b.getArrayAttr(captureTypes));
  if (lexicalOwner) {
    function.setPrivate();
    function->setAttr("ac.lexical_owner", lexicalOwner);
  } else {
    names[node.string("name")] = {helper->symbol, nullptr, node, {}, node.string("name").str(), false,
                                  BindingCategory::Helper};
  }
  valueHelpers[key] = std::move(helper);
  return success();
}

LogicalResult Importer::scanValueHelpers() {
  llvm::StringMap<Value> noCaptures;
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    auto node = source.module.item("body", i);
    if (node.kind() != "FunctionDef" || !node.array("decorator_list").empty()) continue;
    if (names.contains(node.string("name"))) return error() << "pure helper name shadows source binding";
    if (failed(declareValueHelper(node, {}, noCaptures))) return failure();
  }
  return success();
}

FailureOr<ValueHelperDecl *> Importer::resolveValueHelper(const AstNode &function,
                                                         FlatSymbolRefAttr ownerSymbol,
                                                         const llvm::StringMap<Value> *values) {
  if (function.kind() == "Name") {
    if ((activeHelperFrame && activeHelperFrame->localNames.contains(function.string("id"))) ||
        (values && values->contains(function.string("id"))))
      return error() << "pure helper callable binding is shadowed by a local value";
    FlatSymbolRefAttr lexical = activeValueHelper ? activeHelperLexicalOwner : ownerSymbol;
    bool lexicalScope = activeValueHelper && bool(lexical);
    if (!activeValueHelper && lexical)
      for (const auto &module : modules)
        if (module.symbol == lexical && function.path.size() >= module.node.path.size() &&
            llvm::all_of(llvm::zip(ArrayRef<AstStep>(function.path).take_front(module.node.path.size()), module.node.path),
                         [](const auto &steps) { return std::get<0>(steps).field == std::get<1>(steps).field &&
                                                     std::get<0>(steps).index == std::get<1>(steps).index; }))
          lexicalScope = true;
    if (lexical && lexicalScope) {
      std::string key = (Twine(lexical.getValue()) + "." + function.string("id")).str();
      auto found = valueHelpers.find(key);
      if (found != valueHelpers.end()) return found->second.get();
    }
  }
  auto binding = lookupBinding(function);
  if (!binding || binding->category != BindingCategory::Helper) return static_cast<ValueHelperDecl *>(nullptr);
  auto found = valueHelpers.find(binding->symbol.getValue());
  if (found != valueHelpers.end()) return found->second.get();
  auto functionBody = dyn_cast_or_null<func::FuncOp>(binding->declaration);
  if (!functionBody) functionBody = headers.lookupHelper(binding->symbol);
  auto kind = functionBody ? functionBody->getAttrOfType<StringAttr>("ac.helper_kind") : StringAttr();
  if (!functionBody || !kind || kind.getValue() != "value" ||
      functionBody->getAttrOfType<FlatSymbolRefAttr>("ac.lexical_owner"))
    return error() << "imported helper must be a public top-level value definition";
  auto helper = std::make_unique<ValueHelperDecl>();
  helper->symbol = binding->symbol;
  helper->function = dyn_cast_or_null<func::FuncOp>(SymbolTable::lookupSymbolIn(*body, binding->symbol));
  if (!helper->function) return error() << "imported helper snapshot is missing";
  helper->state = 2;
  for (Attribute raw : helper->function->getAttrOfType<ArrayAttr>("ac.parameters")) {
    auto parameter = cast<DictionaryAttr>(raw);
    helper->inputs.push_back(sourceCallPort(parameter.getAs<DictionaryAttr>("constraint"),
                                            parameter.getAs<StringAttr>("name").getValue(), function));
  }
  for (Attribute raw : helper->function->getAttrOfType<ArrayAttr>("ac.result_constraints"))
    helper->results.push_back(sourceCallPort(cast<DictionaryAttr>(raw), "result", function));
  auto *result = helper.get();
  valueHelpers[binding->symbol.getValue()] = std::move(helper);
  return result;
}

LogicalResult Importer::lowerValueHelper(ValueHelperDecl &helper) {
  auto *previousNestedRules = std::exchange(activeNestedRuleScope, nullptr);
  auto restoreNestedRules = llvm::scope_exit([&] { activeNestedRuleScope = previousNestedRules; });
  if (helper.state == 2) return success();
  if (helper.state == 1) return error() << "recursive pure helper call closure is unsupported";
  std::string reason;
  if (!queryBudget.enter(reason)) return error() << reason;
  auto leaveHelper = llvm::scope_exit([&] { queryBudget.leave(); });
  helper.state = 1;
  auto savedFrame = std::exchange(activeHelperFrame, &helper);
  auto savedPure = std::exchange(activeValueHelper, true);
  auto savedLexical = std::exchange(activeHelperLexicalOwner, helper.lexicalOwner);
  auto savedRule = std::exchange(activeRule, nullptr);
  auto savedDecl = std::exchange(activeBehavioralDecl, nullptr);
  auto savedStates = std::exchange(activeStates, nullptr);
  auto savedEffects = std::exchange(activeSourceEffects, nullptr);
  auto savedLoop = std::exchange(activeLoop, nullptr);
  auto savedExpansion = std::move(activeSourceExpansion);
  activeSourceExpansion.clear();
  auto restore = llvm::scope_exit([&] {
    activeHelperFrame = savedFrame;
    activeValueHelper = savedPure; activeHelperLexicalOwner = savedLexical;
    activeRule = savedRule; activeBehavioralDecl = savedDecl; activeStates = savedStates;
    activeSourceEffects = savedEffects; activeLoop = savedLoop;
    activeSourceExpansion = std::move(savedExpansion);
  });
  // All lexical declarations have names and their own declaration snapshots
  // before any body is demanded. Seal transitive capture inputs from callee
  // snapshots, without consulting a dynamic Module or caller value frame.
  if (helper.lexicalOwner) {
    auto dependencies = [&](auto &&self, const AstNode &site) -> LogicalResult {
      if (site.kind() == "Call") {
        auto callee = resolveValueHelper(site.child("func"), helper.lexicalOwner);
        if (failed(callee)) return failure();
        if (*callee) {
          if (failed(lowerValueHelper(**callee))) return failure();
          for (const auto &capture : (*callee)->captures)
            if (llvm::none_of(helper.captures, [&](const auto &own) { return own.second == capture.second; }))
              helper.captures.push_back({{}, capture.second});
        }
      }
      for (NamedAttribute field : site.fields()) {
        auto key = field.getName().getValue();
        if ((key == "args" && site.kind() == "FunctionDef") || key == "returns") continue;
        if (auto child = dyn_cast<DictionaryAttr>(field.getValue())) {
          if (child.getAs<StringAttr>("kind") && failed(self(self, site.child(key)))) return failure();
        } else if (auto children = dyn_cast<ArrayAttr>(field.getValue()))
          for (size_t i = 0; i < children.size(); ++i)
            if (auto child = dyn_cast<DictionaryAttr>(children[i]); child && child.getAs<StringAttr>("kind"))
              if (failed(self(self, site.item(key, i)))) return failure();
      }
      return success();
    };
    if (failed(dependencies(dependencies, helper.node))) return failure();
    SmallVector<Type> inputs;
    SmallVector<Attribute> captures;
    for (const Port &port : helper.inputs) inputs.push_back(port.type);
    for (const auto &capture : helper.captures) {
      inputs.push_back(capture.second.getType());
      captures.push_back(TypeAttr::get(capture.second.getType()));
    }
    helper.function->setAttr("function_type", TypeAttr::get(FunctionType::get(b.getContext(), inputs, helper.function.getResultTypes())));
    helper.function->setAttr("ac.capture_types", b.getArrayAttr(captures));
  }
  auto *block = helper.function.addEntryBlock();
  BranchEnvironment environment;
  unsigned ordinal = 0;
  for (const Port &port : helper.inputs) {
    auto argument = block->getArgument(ordinal++);
    environment.values[port.name] = argument;
    environment.boundaries[port.name] = BindingBoundary{port, true};
    remember(argument, port.sourceKind);
    if (port.fixedBits) fixedValues.insert(argument);
  }
  for (const auto &capture : helper.captures) {
    auto argument = block->getArgument(ordinal++);
    auto info = valueInfo(capture.second);
    info.value = argument;
    numericValues[argument] = info;
    if (fixedValues.contains(capture.second)) fixedValues.insert(argument);
    if (arithmeticValues.contains(capture.second)) arithmeticValues.insert(argument);
    if (!capture.first.empty()) {
      environment.values[capture.first] = argument;
      environment.boundaries[capture.first] = inferredBoundary(capture.first, argument, helper.node);
    }
  }
  auto inside = OpBuilder::atBlockEnd(block);
  llvm::StringSet<> noOwners;
  AstNode returned;
  if (failed(statements(helper.node, "body", inside, helper.symbol, environment, noOwners, returned))) return failure();
  SmallVector<Value> results;
  llvm::StringMap<Instance *> noInstances;
  auto valueNode = returned ? returned.child("value") : AstNode();
  bool none = !valueNode || (valueNode.kind() == "Constant" && isa_and_nonnull<UnitAttr>(valueNode.get("value")));
  if (helper.results.empty()) {
    if (!none) return error() << "None pure helper cannot return a value";
  } else {
    if (none) return error() << "valued pure helper must return a value";
    auto value = expression(valueNode, inside, helper.symbol, environment.values, noInstances, helper.results[0].type);
    if (failed(value)) return failure();
    auto converted = applyBindingBoundary(*value, BindingBoundary{helper.results[0], true}, valueNode, inside, helper.symbol);
    if (failed(converted)) return failure();
    auto exact = exactHelperValue(*converted, helper.results[0].type, valueNode, inside, helper.symbol);
    if (failed(exact)) return failure();
    results.push_back(*exact);
  }
  inside.create<func::ReturnOp>(helper.function.getLoc(), results);
  auto verifyHelper = [&] {
    return ac::verifySourceValueHelper(helper.function, [&](FlatSymbolRefAttr symbol) {
      return dyn_cast_or_null<func::FuncOp>(SymbolTable::lookupSymbolIn(*body, symbol));
    });
  };
  if (failed(verifyHelper())) return failure();
  // This is a newly captured local definition, never an imported snapshot.
  // Prepay the scans, metadata erases and use rewrites before changing either
  // its SSA or importer facts. The existing clone planner then sees smaller IR.
  TableQueryCharge scan;
  bool fits = true;
  for (Operation &operation : *block) {
    fits &= TableQueryBudget::add(scan, TableQueryResource::Work, 2);
    if (auto extract = dyn_cast<ac::BitsExtractOp>(operation)) {
      fits &= TableQueryBudget::add(scan, TableQueryResource::Work, 16);
      for (OpOperand &use : extract.getResult().getUses()) {
        (void)use;
        fits &= TableQueryBudget::add(scan, TableQueryResource::Work, 3);
      }
    }
  }
  std::string scanReason;
  if (!fits || !queryBudget.reserve(scan, scanReason))
    return error() << (fits ? scanReason : "pure helper payload scan work exhausted");
  for (auto extract :
       llvm::make_early_inc_range(block->getOps<ac::BitsExtractOp>())) {
    Value replacement = literalPayloadReplacement(extract, *block);
    if (!replacement) continue;
    Value result = extract.getResult();
    numericValues.erase(result);
    fixedValues.erase(result);
    arithmeticValues.erase(result);
    result.replaceAllUsesWith(replacement);
    extract.erase();
  }
  auto literalPlan = planLiteralPayloadConstants(
      *block, [&](uint64_t work, uint64_t bytes) -> LogicalResult {
        TableQueryCharge charge;
        bool fits = TableQueryBudget::add(charge, TableQueryResource::Work, work) &&
                    TableQueryBudget::add(charge, TableQueryResource::Slots,
                                          bytes / 8 + (bytes % 8 != 0));
        if (!fits || !queryBudget.reserve(charge, scanReason))
          return error() << (fits ? scanReason : "pure helper literal plan exhausted");
        return success();
      });
  if (failed(literalPlan)) return failure();
  for (auto [constant, representative] : *literalPlan) {
    Value result = constant.getResult();
    numericValues.erase(result);
    fixedValues.erase(result);
    arithmeticValues.erase(result);
    result.replaceAllUsesWith(representative);
    constant.erase();
  }
  if (failed(verifyHelper())) return failure();
  helper.state = 2;
  return success();
}

FailureOr<Value> Importer::exactHelperValue(Value value, Type target,
                                           const AstNode &node, OpBuilder &at,
                                           FlatSymbolRefAttr symbol) {
  if (value.getType() == target) return value;
  Operation *typeScope = at.getBlock()->getParentOp();
  if (!typeScope) typeScope = body->getOperation();
  auto equivalent = equivalentBoundaryTypes(value.getType(), target, typeScope);
  if (failed(equivalent) || !*equivalent) return error() << "pure helper physical type mismatch";
  if (isa<ac::BitsType>(target)) return boundaryBitsIdentity(value, target, node, at, symbol);
  if (auto table = dyn_cast<ac::TableType>(target)) {
    auto sourceTable = dyn_cast<ac::TableType>(value.getType());
    if (!sourceTable) return error() << "pure helper Table boundary requires a Table value";
    auto elementEquivalent = equivalentBoundaryTypes(sourceTable.getElementType(), table.getElementType(), typeScope);
    if (failed(elementEquivalent) || !*elementEquivalent)
      return error() << "pure helper Table boundary requires equivalent element types";
    // reshape is an existing, verified Table identity transport. Only source
    // provenance changes; packed shape, lanes and V/K/Z payload remain equal.
    return createOp(at, node.location(b.getContext(), source.path),
                     ac::TableViewOp::getOperationName(), {value}, TypeRange{target},
                     {at.getNamedAttr("kind", at.getStringAttr("reshape")),
                      at.getNamedAttr("parameters", at.getDictionaryAttr({at.getNamedAttr("shape", table.getShape())}))})->getResult(0);
  }
  return error() << "pure helper physical nominal type mismatch";
}

Value Importer::helperCapture(Value original, Block *block, Location location) {
  Value root = originalCaptures.lookup(original);
  if (root) original = root;
  if (original.getParentBlock() == block) return original;
  auto found = helperCaptureBindings[block].find(original);
  if (found != helperCaptureBindings[block].end()) return found->second;
  Operation *ownerOp = block->getParentOp();
  // Source branches are transparent lexical regions. Their arms may use an
  // ordinary instance output captured by the enclosing Method/Rule block;
  // no new branch argument or synthetic source binding is required.
  if (isa<ac::SourceBranchOp>(ownerOp))
    return helperCapture(original, ownerOp->getBlock(), location);
  if (isa<ac::RuleOp>(ownerOp)) {
    auto *parentBlock = ownerOp->getBlock();
    Value outer = helperCapture(original, parentBlock, location);
    ownerOp->insertOperands(ownerOp->getNumOperands(), outer);
    return captureValue(block, outer, location);
  }
  if (isa<ac::ModuleOp>(ownerOp)) return original;
  // Method/transaction region captures are populated from the complete static
  // Module capture inventory before either region is lowered.
  error() << "pure helper lexical snapshot is absent from its enclosing capture frame";
  return Value();
}

// These containers cannot contain a source occurrence, even transitively.
// Retain their immutable handles without a memo row or a temporary copy.
static bool invariantHelperAttributeContainer(Attribute attribute) {
  auto leaf = [](Attribute value) {
    return !isa<ac::StaticExprAttr, DictionaryAttr, ArrayAttr>(value);
  };
  if (auto dictionary = dyn_cast<DictionaryAttr>(attribute))
    return llvm::all_of(dictionary, [&](NamedAttribute field) { return leaf(field.getValue()); });
  if (auto array = dyn_cast<ArrayAttr>(attribute))
    return llvm::all_of(array, leaf);
  return false;
}

// Retain at most one small bucket array per attribute cache. Clearing reuses
// storage; all repeated walks/copies and both cache-clear walks remain charged.
static constexpr uint64_t helperAttributeMemoBuckets = 1024;
static constexpr uint64_t helperAttributeMemoEntries =
    helperAttributeMemoBuckets * 3 / 4 - 1;

// Aggregate projections create fresh importer facts. Their typed helper's
// internal scalar facts cannot escape except through an actual returned SSA
// value. Scalar results retain all facts for operand-sensitive refinements.
static bool aggregateValueHelperResults(func::FuncOp helper) {
  auto aggregate = [](Type type) {
    return isa<ac::StructType, ac::TableType>(type);
  };
  auto returned = cast<func::ReturnOp>(helper.getBody().front().getTerminator());
  return !helper.getResultTypes().empty() &&
         llvm::all_of(helper.getResultTypes(), aggregate) &&
         llvm::all_of(returned.getOperandTypes(), aggregate);
}

FailureOr<TableQueryCharge> Importer::valueHelperCost(func::FuncOp root,
                                                     bool preserveAllFacts) {
  // Cost depends on frame count, never frame contents. Actual rebasing remains
  // separately memoized by Attribute inside each complete inline invocation.
  llvm::DenseMap<std::pair<Operation *, uint64_t>, TableQueryCharge> memo;
  llvm::DenseSet<Operation *> visiting;
  auto add = [&](TableQueryCharge &sum, const TableQueryCharge &amount) {
    for (unsigned i = 0; i < amount.amounts.size(); ++i)
      if (!TableQueryBudget::add(sum, static_cast<TableQueryResource>(i), amount.amounts[i])) return false;
    return true;
  };
  auto visit = [&](auto &&self, func::FuncOp helper, unsigned depth,
                   uint64_t frames) -> FailureOr<TableQueryCharge> {
    if (depth > TableQueryBudget::limit(TableQueryResource::Nesting))
      return error() << "pure helper call depth budget exhausted";
    auto key = std::make_pair(helper.getOperation(), frames);
    auto cached = memo.find(key);
    if (cached != memo.end()) return cached->second;
    if (!visiting.insert(helper.getOperation()).second)
      return error() << "recursive pure helper closure is unsupported";
    auto leave = llvm::scope_exit([&] { visiting.erase(helper.getOperation()); });
    TableQueryCharge cost;
    bool fits = TableQueryBudget::add(cost, TableQueryResource::Occurrences, 1) &&
                TableQueryBudget::add(cost, TableQueryResource::Slots, helper.getNumArguments() * 4 + 24);
    auto returned = cast<func::ReturnOp>(helper.getBody().front().getTerminator());
    // Both planning and cloning inspect this borrowed range; no retention set
    // or new fact inventory is allocated. Staging retains every fact.
    fits &= TableQueryBudget::add(cost, TableQueryResource::Work,
        preserveAllFacts ? 2 : 2 * (1 + helper.getResultTypes().size() +
                                        returned.getNumOperands()));
    bool pruneFacts = !preserveAllFacts && aggregateValueHelperResults(helper);
    auto retainsFacts = [&](Value value) {
      if (!pruneFacts) return true;
      for (Value operand : returned.getOperands()) {
        fits &= TableQueryBudget::add(cost, TableQueryResource::Work, 2);
        if (operand == value) return true;
      }
      return false;
    };
    auto chargeValue = [&](Value value, uint64_t copies = 2,
                           bool cloneFacts = true) {
      fits &= TableQueryBudget::add(cost, TableQueryResource::Work, 3);
      auto facts = numericValues.find(value);
      // A missing/empty fact row has exactly valueInfo's default meaning. The
      // clone need not allocate a second empty row for an aggregate SSA handle.
      if (facts != numericValues.end()) {
        bool meaningful = facts->second.sourceKind || facts->second.interval ||
                          facts->second.closedSourceConstant;
        fits &= TableQueryBudget::addProduct(cost, TableQueryResource::Slots,
            meaningful ? copies : 1, (sizeof(NumericValue) + 7) / 8 + 4);
        if (facts->second.interval)
          for (const auto *endpoint : {&facts->second.interval->lower,
                                       &facts->second.interval->upper})
            fits &= TableQueryBudget::addProduct(cost, TableQueryResource::PayloadWords,
                (endpoint->getBitWidth() + 63) / 64, cloneFacts ? 3 : 1);
      }
      if (fixedValues.contains(value))
        fits &= TableQueryBudget::addProduct(cost, TableQueryResource::Slots, copies, 2);
      if (arithmeticValues.contains(value))
        fits &= TableQueryBudget::addProduct(cost, TableQueryResource::Slots, copies, 2);
    };
    // Function arguments map to existing actuals; no clone argument is created.
    for (Value argument : helper.getArguments()) chargeValue(argument, 1);
    // The planning set and clone map hold the same container keys. Reserve
    // their bucket arrays at actual DenseMap growth boundaries, rather than a
    // per-node multiplier that charges unused growth slack for every key.
    llvm::DenseSet<Attribute> attributes;
    uint64_t attributeCapacity = 0;
    auto chargeAttribute = [&](auto &&walk, Attribute attribute, unsigned level) -> void {
      if (!fits) return;
      if (!attribute || level > TableQueryBudget::limit(TableQueryResource::Nesting)) { fits = false; return; }
      fits &= TableQueryBudget::add(cost, TableQueryResource::Work, 2);
      if (!isa<ac::StaticExprAttr, DictionaryAttr, ArrayAttr>(attribute)) return;
      uint64_t children = 0;
      if (auto dictionary = dyn_cast<DictionaryAttr>(attribute)) children = dictionary.size();
      if (auto array = dyn_cast<ArrayAttr>(attribute)) children = array.size();
      // Both the planning and rebasing walkers test this shallow fast path.
      fits &= TableQueryBudget::addProduct(cost, TableQueryResource::Work, children, 2);
      if (invariantHelperAttributeContainer(attribute)) return;
      // StaticExpr is a uniqued wrapper around its dictionary. Memoizing both
      // handles retains redundant keys; memoize the shared tree alone.
      if (auto expression = dyn_cast<ac::StaticExprAttr>(attribute)) {
        fits &= TableQueryBudget::add(cost, TableQueryResource::Slots, 2);
        walk(walk, expression.getTree(), level + 1);
        return;
      }
      // Each operation's attribute dictionary is streamed once. Retain only
      // child containers in the invocation memo, paying root copies per use.
      if (level != 0 && attributes.contains(attribute)) return;
      if (auto dictionary = dyn_cast<DictionaryAttr>(attribute)) {
        // Temporary NamedAttrList and the rebuilt immutable dictionary retain
        // name/value handles; neither copies String or mathematical payloads.
        fits &= TableQueryBudget::addProduct(cost, TableQueryResource::Slots, dictionary.size(), 4) &&
                TableQueryBudget::addProduct(cost, TableQueryResource::Work, dictionary.size(), 2);
        auto expansion = dictionary.getAs<ArrayAttr>("expansion");
        if (dictionary.getAs<DictionaryAttr>("site") && expansion) {
          uint64_t appended = expansion.size();
          fits &= pureCheckedAdd(appended, frames);
          // Growing scratch vector (<2N) plus the final ArrayAttr (N).
          fits &= TableQueryBudget::addProduct(cost, TableQueryResource::Slots, appended, 3) &&
                  TableQueryBudget::addProduct(cost, TableQueryResource::Work, appended, 3);
        } else {
          for (NamedAttribute field : dictionary) walk(walk, field.getValue(), level + 1);
        }
      } else if (auto array = dyn_cast<ArrayAttr>(attribute)) {
        fits &= TableQueryBudget::addProduct(cost, TableQueryResource::Slots, array.size(), 3) &&
                TableQueryBudget::add(cost, TableQueryResource::Work, array.size());
        for (Attribute item : array) walk(walk, item, level + 1);
      }
      if (level != 0) {
        if (!fits) return;
        // Insert after children, matching the rebasing walk exactly. A clear
        // may cause later siblings to revisit a subtree; those uses pay again.
        if (attributes.size() == helperAttributeMemoEntries) {
          fits &= TableQueryBudget::addProduct(cost, TableQueryResource::Work,
                                               attributeCapacity, 3);
          if (!fits) return;
          attributes.clear();
        }
        uint64_t capacity = std::max(uint64_t(64), attributeCapacity);
        uint64_t count = attributes.size() + 1;
        while (count * 4 >= capacity * 3) capacity *= 2;
        if (capacity != attributeCapacity) {
          fits &= TableQueryBudget::addProduct(cost, TableQueryResource::Slots,
                                               capacity, 3) &&
                  TableQueryBudget::addProduct(cost, TableQueryResource::Work,
                                               attributes.size(), 2);
          attributeCapacity = capacity;
        }
        if (!fits) return;
        attributes.insert(attribute);
      }
    };
    // One invocation owns one mapping. Source SSA values and operations are
    // unique even across sibling regions, so retaining earlier keys is safe and
    // avoids copying every enclosing prefix for each nested block.
    uint64_t mappedValues = 0, mappedOperations = 0;
    uint64_t valueMapCapacity = 0, operationMapCapacity = 0;
    auto chargeMap = [&](uint64_t &count, uint64_t &capacity,
                         uint64_t inserted) {
      for (uint64_t i = 0; i < inserted; ++i) {
        uint64_t grown = std::max(uint64_t(64), capacity);
        if ((count + 1) * 4 >= grown * 3) grown *= 2;
        if (grown != capacity) {
          fits &= TableQueryBudget::addProduct(cost, TableQueryResource::Slots,
                                               grown, 2) &&
                  TableQueryBudget::add(cost, TableQueryResource::Work, count);
          capacity = grown;
        }
        ++count;
        fits &= TableQueryBudget::add(cost, TableQueryResource::Work, 1);
      }
    };
    chargeMap(mappedValues, valueMapCapacity, helper.getNumArguments());
    helper.walk<WalkOrder::PreOrder>([&](Operation *operation) {
      if (operation == helper.getOperation() || isa<func::ReturnOp>(operation)) return;
      fits &= TableQueryBudget::add(cost, TableQueryResource::Work, 2);
      if (auto call = dyn_cast<func::CallOp>(operation)) {
        auto callee = dyn_cast_or_null<func::FuncOp>(SymbolTable::lookupSymbolIn(*body, call.getCalleeAttr()));
        if (!callee) { fits = false; return; }
        auto origin = call->getAttrOfType<DictionaryAttr>("ac.origin");
        auto saved = origin ? origin.getAs<ArrayAttr>("expansion") : ArrayAttr();
        uint64_t childFrames = frames;
        fits &= pureCheckedAdd(childFrames, saved ? saved.size() : 0) &&
                pureCheckedAdd(childFrames, origin ? 1 : 0);
        fits &= TableQueryBudget::add(cost, TableQueryResource::Operations, 1) &&
                TableQueryBudget::add(cost, TableQueryResource::Slots, 16) &&
                TableQueryBudget::addProduct(cost, TableQueryResource::Slots, childFrames, 2) &&
                TableQueryBudget::addProduct(cost, TableQueryResource::Slots,
                    call->getNumOperands() + call->getNumResults() + 1, 4);
        auto child = self(self, callee, depth + 1, childFrames);
        if (failed(child)) { fits = false; return; }
        // Every call edge contributes its complete cost, including repeated
        // edges to one callee. Exponential helper DAGs still reject up front.
        fits &= add(cost, *child);
      } else {
        fits &= TableQueryBudget::add(cost, TableQueryResource::Operations, 2) &&
                TableQueryBudget::addProduct(cost, TableQueryResource::Slots,
                    8 + operation->getNumOperands() + operation->getNumResults(), 2);
        chargeMap(mappedOperations, operationMapCapacity, 1);
        for (Value value : operation->getResults()) {
          bool retained = retainsFacts(value);
          chargeValue(value, retained ? 2 : 1, retained);
        }
        chargeAttribute(chargeAttribute, operation->getAttrDictionary(), 0);
      }
      for (Region &region : operation->getRegions())
        for (Block &block : region) {
          fits &= TableQueryBudget::add(cost, TableQueryResource::Slots, 8) &&
                  TableQueryBudget::addProduct(cost, TableQueryResource::Slots,
                                               block.getNumArguments(), 2);
          chargeMap(mappedValues, valueMapCapacity, block.getNumArguments());
          for (Value argument : block.getArguments()) {
            bool retained = retainsFacts(argument);
            chargeValue(argument, retained ? 2 : 1, retained);
          }
        }
      chargeMap(mappedValues, valueMapCapacity, operation->getNumResults());
    });
    if (!fits) return error() << "pure helper expanded DAG budget exhausted";
    memo[key] = cost;
    return cost;
  };
  return visit(visit, root, 0, activeSourceExpansion.size() + 1);
}

LogicalResult Importer::reserveValueHelper(func::FuncOp helper, const AstNode &call,
                                          bool preserveAllFacts) {
  auto cost = valueHelperCost(helper, preserveAllFacts);
  if (failed(cost)) return failure();
  // The cost owns the complete clone closure and its exact expansion-frame
  // counts. Fixed template/clone fees never scale with appended frame depth.
  std::string reason;
  if (!queryBudget.reserve(*cost, reason))
    return mlir::emitError(call.location(b.getContext(), source.path)) << reason;
  return success();
}

FailureOr<SmallVector<Value>> Importer::instantiateValueHelper(
    ValueHelperDecl &helper, ValueRange inputs, const AstNode &call,
    OpBuilder &at, FlatSymbolRefAttr symbol, bool forceInline) {
  if (activeValueHelper && !forceInline) {
    // Loop statement envelopes leave helper expansion to this existing DAG
    // budget, including typed calls retained until the outer helper is inlined.
    if (activeLoop && failed(reserveValueHelper(helper.function, call, true))) return failure();
    auto op = at.create<func::CallOp>(call.location(b.getContext(), source.path),
                                    helper.symbol, helper.function.getResultTypes(), inputs);
    if (activeLoop) activeLoop->helperOwned.insert(op.getOperation());
    op->setAttr("ac.origin", sourceOccurrence(at, symbol, call));
    for (auto [result, port] : llvm::zip(op.getResults(), helper.results)) {
      remember(result, port.sourceKind);
      if (port.fixedBits) fixedValues.insert(result);
    }
    return SmallVector<Value>(op.getResults());
  }
  bool preserveAllFacts = forceInline || activeLoop;
  if (failed(ac::verifySourceValueHelper(helper.function, [&](FlatSymbolRefAttr callee) {
        return dyn_cast_or_null<func::FuncOp>(SymbolTable::lookupSymbolIn(*body, callee));
      })) || failed(reserveValueHelper(helper.function, call, preserveAllFacts))) return failure();
  size_t frameDepth = activeSourceExpansion.size();
  if (failed(pushHelperFrame(call, helper.node, symbol, helper.symbol))) return failure();
  auto restoreFrame = llvm::scope_exit([&] { activeSourceExpansion.resize(frameDepth); });
  auto copyFacts = [&](Value original, Value copied) {
    auto facts = numericValues.find(original);
    if (facts != numericValues.end() &&
        (facts->second.sourceKind || facts->second.interval || facts->second.closedSourceConstant)) {
      auto info = facts->second; info.value = copied;
      numericValues[copied] = std::move(info);
    }
    if (fixedValues.contains(original)) fixedValues.insert(copied);
    if (arithmeticValues.contains(original)) arithmeticValues.insert(copied);
  };
  auto inlineBody = [&](auto &&self, func::FuncOp function, ValueRange actuals,
                        OpBuilder &target) -> FailureOr<SmallVector<Value>> {
    auto returned = cast<func::ReturnOp>(function.getBody().front().getTerminator());
    bool pruneFacts = !preserveAllFacts && aggregateValueHelperResults(function);
    auto retainsFacts = [&](Value value) {
      return !pruneFacts || llvm::is_contained(returned.getOperands(), value);
    };
    // The expansion frame is fixed for this complete function invocation.
    // Nested helpers create their own map and restore the caller's frame before
    // returning; an attribute can never reuse a different invocation's origin.
    llvm::DenseMap<Attribute, Attribute> expandedAttributes;
    auto expandAttribute = [&](auto &&expand, Attribute attribute,
                               bool memoize = true) -> Attribute {
      if (!isa<ac::StaticExprAttr, DictionaryAttr, ArrayAttr>(attribute) ||
          invariantHelperAttributeContainer(attribute)) return attribute;
      if (auto expression = dyn_cast<ac::StaticExprAttr>(attribute)) {
        Attribute tree = expand(expand, expression.getTree());
        return tree == expression.getTree() ? attribute :
            ac::StaticExprAttr::get(at.getContext(), cast<DictionaryAttr>(tree));
      }
      if (memoize) {
        auto found = expandedAttributes.find(attribute);
        if (found != expandedAttributes.end()) return found->second;
      }
      Attribute result = attribute;
      if (auto dictionary = dyn_cast<DictionaryAttr>(attribute)) {
        if (dictionary.getAs<DictionaryAttr>("site") && dictionary.getAs<ArrayAttr>("expansion")) {
          SmallVector<Attribute> frames;
          llvm::append_range(frames, dictionary.getAs<ArrayAttr>("expansion"));
          llvm::append_range(frames, activeSourceExpansion);
          NamedAttrList fields(dictionary); fields.set("expansion", at.getArrayAttr(frames));
          result = fields.getDictionary(at.getContext());
        } else {
          NamedAttrList fields(dictionary);
          bool changed = false;
          for (NamedAttribute field : dictionary) {
            Attribute value = expand(expand, field.getValue());
            if (value != field.getValue()) { fields.set(field.getName(), value); changed = true; }
          }
          if (changed) result = fields.getDictionary(at.getContext());
        }
      } else if (auto array = dyn_cast<ArrayAttr>(attribute)) {
        SmallVector<Attribute> items;
        bool changed = false;
        for (Attribute item : array) {
          Attribute value = expand(expand, item);
          changed |= value != item;
          items.push_back(value);
        }
        if (changed) result = at.getArrayAttr(items);
      }
      if (memoize) {
        if (expandedAttributes.size() == helperAttributeMemoEntries)
          expandedAttributes.clear();
        expandedAttributes.try_emplace(attribute, result);
      }
      return result;
    };
    IRMapping mapping;
    for (auto [formal, actual] : llvm::zip(function.getArguments(), actuals)) mapping.map(formal, actual);
    auto expandBlock = [&](auto &&expand, Block &sourceBlock, Block &destination,
                           IRMapping &map, OpBuilder &builder) -> LogicalResult {
      (void)destination;
      for (Operation &operation : sourceBlock) {
        if (isa<func::ReturnOp>(operation)) continue;
        if (auto nestedCall = dyn_cast<func::CallOp>(operation)) {
          auto callee = dyn_cast_or_null<func::FuncOp>(SymbolTable::lookupSymbolIn(*body, nestedCall.getCalleeAttr()));
          if (!callee) return error() << "pure helper callee is missing";
          SmallVector<Value> bound;
          for (Value operand : nestedCall.getOperands()) bound.push_back(map.lookup(operand));
          auto callerExpansion = std::move(activeSourceExpansion);
          activeSourceExpansion.clear();
          auto restoreExpansion = llvm::scope_exit([&] {
            activeSourceExpansion = std::move(callerExpansion);
          });
          auto origin = nestedCall->getAttrOfType<DictionaryAttr>("ac.origin");
          // A typed call may itself have been expanded from a loop. Retain
          // those declaration frames before the current caller's frames, as
          // expandAttribute does for every other source occurrence.
          if (origin)
            if (auto expansion = origin.getAs<ArrayAttr>("expansion"))
              for (Attribute frame : expansion)
                activeSourceExpansion.push_back(cast<DictionaryAttr>(frame));
          llvm::append_range(activeSourceExpansion, callerExpansion);
          if (origin)
            activeSourceExpansion.push_back(builder.getDictionaryAttr({
                builder.getNamedAttr("kind", builder.getStringAttr("call")),
                builder.getNamedAttr("site", origin.getAs<DictionaryAttr>("site")),
                builder.getNamedAttr("callee", nestedCall.getCalleeAttr())}));
          // The outer reservation already includes this exact call edge and
          // its inherited/saved frame count. Do not prepay that closure twice.
          auto values = self(self, callee, bound, builder);
          if (failed(values)) return failure();
          for (auto [result, value] : llvm::zip(nestedCall.getResults(), *values)) map.map(result, value);
          continue;
        }
        auto *copy = builder.cloneWithoutRegions(operation, map);
        if (activeLoop) activeLoop->helperOwned.insert(copy);
        for (auto [original, result] : llvm::zip(operation.getResults(), copy->getResults()))
          if (retainsFacts(original)) copyFacts(original, result);
        copy->setAttrs(cast<DictionaryAttr>(expandAttribute(expandAttribute, copy->getAttrDictionary(), false)));
        for (auto [sourceRegion, copiedRegion] : llvm::zip(operation.getRegions(), copy->getRegions()))
          for (Block &regionBlock : sourceRegion) {
            auto *newBlock = new Block(); copiedRegion.push_back(newBlock);
            // Every source key belongs to this invocation. Region-local keys
            // can remain in the map; no later source operand can alias them.
            for (Value argument : regionBlock.getArguments()) {
              auto result = newBlock->addArgument(argument.getType(), argument.getLoc());
              map.map(argument, result);
              if (retainsFacts(argument)) copyFacts(argument, result);
            }
            auto inside = OpBuilder::atBlockEnd(newBlock);
            if (failed(expand(expand, regionBlock, *newBlock, map, inside))) return failure();
          }
      }
      return success();
    };
    if (failed(expandBlock(expandBlock, function.getBody().front(), *target.getBlock(), mapping, target))) return failure();
    SmallVector<Value> results;
    for (Value operand : returned.getOperands()) results.push_back(mapping.lookup(operand));
    return results;
  };
  auto results = inlineBody(inlineBody, helper.function, inputs, at);
  if (failed(results)) return failure();
  for (auto [result, port] : llvm::zip(*results, helper.results)) {
    remember(result, port.sourceKind);
    if (port.fixedBits) fixedValues.insert(result);
  }
  return results;
}

FailureOr<SmallVector<Value>> Importer::valueHelperCall(
    const AstNode &call, OpBuilder &at, FlatSymbolRefAttr symbol,
    llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances) {
  auto helper = resolveValueHelper(call.child("func"), symbol, &values);
  if (failed(helper) || !*helper) return error() << "call does not resolve to a pure helper";
  if (!queryArguments.empty()) return error() << "pure helper calls inside expression Lambdas are unsupported";
  if (failed(lowerValueHelper(**helper))) return failure();
  if (!call.array("keywords").empty() || call.array("args").size() != (*helper)->inputs.size())
    return error() << "pure helper requires its exact positional argument list";
  // Hold the source-wide nesting guard across actual-expression descent and
  // publication. Declaration typing has its own bounded closure owner above.
  std::string reason;
  if (!queryBudget.enter(reason))
    return mlir::emitError(call.location(b.getContext(), source.path)) << reason;
  auto leaveCall = llvm::scope_exit([&] { queryBudget.leave(); });
  SmallVector<Value> actuals;
  // Source-order, once-only actual evaluation happens before entering the
  // helper's isolated body frame; no declaration capture is looked up by name.
  for (size_t i = 0; i < call.array("args").size(); ++i) {
    auto actualNode = call.item("args", i);
    auto actual = expression(actualNode, at, symbol, values, instances);
    if (failed(actual)) return failure();
    Operation *previous = at.getBlock()->empty() ? nullptr : &at.getBlock()->back();
    auto bound = applyBindingBoundary(*actual, BindingBoundary{(*helper)->inputs[i], true}, actualNode, at, symbol);
    if (failed(bound)) return failure();
    auto exact = exactHelperValue(*bound, (*helper)->inputs[i].type, actualNode, at, symbol);
    if (failed(exact)) return failure();
    actuals.push_back(*exact);
    if (activeLoop) {
      auto *operation = previous ? previous->getNextNode() : (at.getBlock()->empty() ? nullptr : &at.getBlock()->front());
      for (; operation; operation = operation->getNextNode()) activeLoop->helperOwned.insert(operation);
    }
  }
  for (const auto &capture : (*helper)->captures) {
    Value value;
    if (!activeValueHelper)
      value = helperCapture(capture.second, at.getBlock(), call.location(b.getContext(), source.path));
    if (activeValueHelper) {
      // Calls between private templates use their own declaration capture
      // argument, identified by original SSA identity, never current spelling.
      for (auto &entry : valueHelpers)
        if (entry.second->symbol == symbol)
          for (auto [index, ownCapture] : llvm::enumerate(entry.second->captures))
            if (ownCapture.second == capture.second)
              value = entry.second->function.getArgument(entry.second->inputs.size() + index);
    }
    if (!value) return error() << "pure helper capture is not in its declaration frame";
    actuals.push_back(value);
  }
  return instantiateValueHelper(**helper, actuals, call, at, symbol);
}

FailureOr<Value> Importer::namedHelperMap(const AstNode &call, const AstNode &name,
                                         ValueRange tables, TableQueryLowering &lowering,
                                         const TableQueryHooks &hooks, OpBuilder &at,
                                         FlatSymbolRefAttr symbol,
                                         const llvm::StringMap<Value> &values) {
  auto helper = resolveValueHelper(name, symbol, &values);
  if (failed(helper)) return failure();
  if (!*helper) return error() << "named Table callback must resolve to an ordinary typed helper";
  if (failed(lowerValueHelper(**helper))) return failure();
  if (!(*helper)->captures.empty() || (*helper)->inputs.size() != tables.size() ||
      (*helper)->results.size() != 1 || !(*helper)->results.front().fixedBits ||
      !isa<ac::BitsType>((*helper)->results.front().type))
    return error() << "named Table callback requires a zero-capture typed scalar helper";
  auto storage = std::make_unique<Block>();
  auto *block = storage.get();
  auto cleanup = llvm::scope_exit([&] {
    for (Value argument : block->getArguments()) hooks.forgetFacts(argument);
    block->walk([&](Operation *operation) { for (Value result : operation->getResults()) hooks.forgetFacts(result); });
  });
  SmallVector<Value> rows, actuals;
  OpBuilder inside(b.getContext());
  inside.setInsertionPointToEnd(block);
  for (auto [index, table] : llvm::enumerate(tables)) {
    auto actual = index ? call.item("args", index) : call.child("func").child("value");
    auto actualLocation = actual.location(b.getContext(), source.path);
    auto type = cast<ac::TableType>(table.getType()).getElementType();
    if (!isa<ac::BitsType>(type) || !(*helper)->inputs[index].fixedBits)
      return mlir::emitError(actualLocation)
             << "named Table callback requires fixed Bits elements and parameters";
    auto row = block->addArgument(type, actualLocation); fixedValues.insert(row); rows.push_back(row);
    auto equivalent = equivalentBoundaryTypes(type, (*helper)->inputs[index].type, body->getOperation());
    if (failed(equivalent) || !*equivalent)
      return mlir::emitError(actualLocation) << "named Table callback parameter type mismatch";
    auto exact = exactHelperValue(row, (*helper)->inputs[index].type, actual, inside, symbol);
    if (failed(exact)) return failure();
    actuals.push_back(*exact);
  }
  auto results = instantiateValueHelper(**helper, actuals, call, inside, symbol, true);
  if (failed(results)) return failure();
  auto staged = lowering.stage(*block, rows, tables, *results, false);
  if (failed(staged)) return failure();
  return staged->front();
}
