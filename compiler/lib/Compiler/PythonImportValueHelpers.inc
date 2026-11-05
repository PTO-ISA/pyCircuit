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
      if (kind == "Match") {
        if (failed(self(self, site.child("subject"), false, false)))
          return failure();
        // Traverse source patterns under the ordinary helper work/depth budget
        // before the shared closed-pattern reader prepares its arm views.
        for (size_t i = 0; i < site.array("cases").size(); ++i) {
          auto arm = site.item("cases", i);
          if (failed(chargeSourceWalk(arm, sourceWork)) ||
              failed(self(self, arm.child("pattern"), false, false)))
            return failure();
        }
        auto syntax = readMatchSyntax(site, diagnostic);
        if (failed(syntax))
          return failure();
        for (const auto &arm : *syntax)
          for (size_t i = 0; i < arm.originalSite.array("body").size(); ++i)
            if (failed(
                    self(self, arm.originalSite.item("body", i), true, false)))
              return failure();
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
        if (lhs.kind() == "Tuple" && site.child("value").kind() == "Call") {
          auto targets = moduleResultTargets(site, lhs.array("elts").size());
          if (failed(targets)) return failure();
          for (StringRef name : *targets) locals.insert(name);
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
      auto classification = classifyCall(function);
      if (classification.transaction || local.contains(name) ||
          classification.allocation || classification.observation)
        return diagnostic(site) << "pure helper cannot call Rule, Method, allocation or IO";
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
  ModuleDecl returnContract;
  returnContract.symbol = helper->symbol;
  llvm::StringSet<> resultNames;
  for (const Port &input : helper->inputs) resultNames.insert(input.name);
  if (failed(readReturnContract(returnContract, node.child("returns"), resultNames, true)))
    return failure();
  if (returnContract.returnShape == ReturnShape::Mapping)
    return error() << "pure helper requires None, one ordinary result or a fixed tuple";
  helper->returnShape = returnContract.returnShape;
  helper->results = std::move(returnContract.outputs);
  for (const Port &result : helper->results) {
    auto constraint = sourceConstraint(result);
    if (failed(constraint)) return failure();
    results.push_back(result.type);
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
  function->setAttr("ac.return_form", b.getStringAttr(
      helper->returnShape == ReturnShape::None ? "none" :
      helper->returnShape == ReturnShape::Tuple ? "tuple" : "single"));
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
    if (queryBound(function.string("id")) ||
        (activeHelperFrame && activeHelperFrame->localNames.contains(function.string("id"))) ||
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
  auto classification = classifyCall(function);
  const auto &binding = classification.binding;
  if (!classification.helper()) return static_cast<ValueHelperDecl *>(nullptr);
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
  auto form = helper->function->getAttrOfType<StringAttr>("ac.return_form");
  helper->returnShape = form.getValue() == "tuple" ? ReturnShape::Tuple :
                        form.getValue() == "none" ? ReturnShape::None : ReturnShape::Struct;
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

// A helper body has its own lexical, control and effect frame. Keep the saved
// fields together so every exit, including a failed nested helper, restores the
// caller completely. This scope does not own declarations or numeric facts.
struct Importer::HelperScope {
  Importer &importer;
  NestedRuleScope *nestedRules;
  ValueHelperDecl *frame;
  bool pure;
  FlatSymbolRefAttr lexicalOwner;
  RuleEvaluation *rule;
  ModuleStatementContext *moduleStatements;
  ModuleDecl *declaration;
  SmallVector<StateOwner> *states;
  SourceEffectContext *effects;
  LoopLoweringContext *loop;
  SmallVector<DictionaryAttr> expansion;
  SmallVector<SmallVector<StringRef>> queryBindings;

  HelperScope(Importer &importer, ValueHelperDecl &helper)
      : importer(importer),
        nestedRules(std::exchange(importer.activeNestedRuleScope, nullptr)),
        frame(std::exchange(importer.activeHelperFrame, &helper)),
        pure(std::exchange(importer.activeValueHelper, true)),
        lexicalOwner(std::exchange(importer.activeHelperLexicalOwner,
                                   helper.lexicalOwner)),
        rule(std::exchange(importer.activeRule, nullptr)),
        moduleStatements(std::exchange(importer.activeModuleStatements, nullptr)),
        declaration(std::exchange(importer.activeBehavioralDecl, nullptr)),
        states(std::exchange(importer.activeStates, nullptr)),
        effects(std::exchange(importer.activeSourceEffects, nullptr)),
        loop(std::exchange(importer.activeLoop, nullptr)),
        expansion(std::move(importer.activeSourceExpansion)),
        queryBindings(std::move(importer.queryArguments)) {
    importer.activeSourceExpansion.clear();
    importer.queryArguments.clear();
  }
  ~HelperScope() {
    importer.activeNestedRuleScope = nestedRules;
    importer.activeHelperFrame = frame;
    importer.activeValueHelper = pure;
    importer.activeHelperLexicalOwner = lexicalOwner;
    importer.activeRule = rule;
    importer.activeModuleStatements = moduleStatements;
    importer.activeBehavioralDecl = declaration;
    importer.activeStates = states;
    importer.activeSourceEffects = effects;
    importer.activeLoop = loop;
    importer.activeSourceExpansion = std::move(expansion);
    importer.queryArguments = std::move(queryBindings);
  }
  HelperScope(const HelperScope &) = delete;
  HelperScope &operator=(const HelperScope &) = delete;
};

LogicalResult Importer::lowerValueHelper(ValueHelperDecl &helper) {
  if (helper.state == 2) return success();
  if (helper.state == 1) return success(); // Recursion is diagnosed by the IR inliner.
  std::string reason;
  if (!queryBudget.enter(reason)) return error() << reason;
  auto leaveHelper = llvm::scope_exit([&] { queryBudget.leave(); });
  helper.state = 1;
  HelperScope scope(*this, helper);
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
    rememberContract(argument, port);
    if (port.fixedBits) fixedValues.insert(argument);
  }
  for (const auto &capture : helper.captures) {
    auto argument = block->getArgument(ordinal++);
    originalCaptures[argument] = capture.second;
    mapValueFacts(capture.second, argument);
    if (!capture.first.empty()) {
      environment.values[capture.first] = argument;
    }
  }
  auto inside = OpBuilder::atBlockEnd(block);
  llvm::StringSet<> noOwners;
  AstNode returned;
  if (failed(statements(helper.node, "body", inside, helper.symbol, environment, noOwners, returned))) return failure();
  llvm::StringMap<Instance *> noInstances;
  auto valueNode = returned ? returned.child("value") : AstNode();
  ModuleDecl returnContract;
  returnContract.symbol = helper.symbol;
  returnContract.outputs = helper.results;
  returnContract.returnShape = helper.returnShape;
  auto lowered = lowerReturn(returnContract, valueNode, inside, environment.values, noInstances);
  if (failed(lowered)) return failure();
  SmallVector<Value> results;
  for (auto [index, value] : llvm::enumerate(*lowered)) {
    auto element = helper.returnShape == ReturnShape::Tuple
        ? valueNode.item("elts", index) : valueNode;
    auto exact = exactHelperValue(value, helper.results[index].type, element, inside, helper.symbol);
    if (failed(exact)) return failure();
    results.push_back(*exact);
  }
  inside.create<func::ReturnOp>(helper.function.getLoc(), results);
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
  // Query templates inside a helper borrow its declaration capture arguments;
  // a function frame must never acquire synthetic Rule capture operands.
  if (activeHelperFrame &&
      block == &activeHelperFrame->function.getBody().front())
    for (auto [index, capture] : llvm::enumerate(activeHelperFrame->captures))
      if (capture.second == original)
        return activeHelperFrame->function.getArgument(
            activeHelperFrame->inputs.size() + index);
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

FailureOr<SmallVector<Value>> Importer::instantiateValueHelper(
    ValueHelperDecl &helper, ValueRange inputs, const AstNode &call,
    OpBuilder &at, FlatSymbolRefAttr symbol) {
  auto *operation = createOp(
      at, call.location(b.getContext(), source.path), "ac.call", inputs,
      helper.function.getResultTypes(),
      {at.getNamedAttr("callee", helper.symbol),
       at.getNamedAttr("ac.origin", sourceOccurrence(at, symbol, call))});
  if (activeLoop) activeLoop->helperOwned.insert(operation);
  for (auto [result, port] : llvm::zip(operation->getResults(), helper.results)) {
    rememberContract(result, port);
    if (port.fixedBits) fixedValues.insert(result);
  }
  return SmallVector<Value>(operation->getResults());
}

FailureOr<SmallVector<Value>> Importer::valueHelperCall(
    const AstNode &call, OpBuilder &at, FlatSymbolRefAttr symbol,
    llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances,
    bool tupleCall) {
  auto helper = resolveValueHelper(call.child("func"), symbol, &values);
  if (failed(helper) || !*helper) return error() << "call does not resolve to a pure helper";
  if (((*helper)->returnShape == ReturnShape::Tuple) != tupleCall)
    return error() << "tuple helper call requires direct Tuple unpacking";
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
    // Helper actuals intentionally have no destination context. Their existing
    // exact-carrier and loop-ownership work surrounds the common conversion.
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

FailureOr<bool> Importer::bindValueHelperResults(
    const AstNode &assignment, OpBuilder &at, FlatSymbolRefAttr symbol,
    llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances,
    llvm::StringMap<BindingBoundary> *boundaries,
    const llvm::StringSet<> &forbidden, bool immutable) {
  auto call = assignment.child("value");
  if (call.kind() != "Call") return false;
  auto helper = resolveValueHelper(call.child("func"), symbol, &values);
  if (failed(helper)) return failure();
  if (!*helper || (*helper)->returnShape != ReturnShape::Tuple) return false;
  auto targets = moduleResultTargets(assignment, (*helper)->results.size());
  if (failed(targets)) return failure();
  for (StringRef name : *targets)
    if (forbidden.contains(name) || instances.contains(name) ||
        (immutable && values.contains(name)))
      return error() << "tuple helper result cannot write a protected binding";
  auto results = valueHelperCall(call, at, symbol, values, instances, true);
  if (failed(results) || failed(bindCallableResults(assignment, *results, at,
          symbol, values, instances, forbidden, boundaries))) return failure();
  return true;
}

FailureOr<Value> Importer::namedHelperMap(const AstNode &call, const AstNode &name,
                                         ValueRange tables, OpBuilder &at,
                                         FlatSymbolRefAttr symbol,
                                         const llvm::StringMap<Value> &values) {
  auto helper = resolveValueHelper(name, symbol, &values);
  if (failed(helper)) return failure();
  if (!*helper) return error() << "named Table callback must resolve to an ordinary typed helper";
  if (failed(lowerValueHelper(**helper))) return failure();
  if (!(*helper)->captures.empty() || (*helper)->inputs.size() != tables.size() ||
      (*helper)->returnShape == ReturnShape::Tuple ||
      (*helper)->results.size() != 1 || !(*helper)->results.front().fixedBits ||
      !isa<ac::BitsType>((*helper)->results.front().type))
    return error() << "named Table callback requires a zero-capture typed scalar helper";
  auto storage = std::make_unique<Block>();
  auto *block = storage.get();
  auto cleanup = llvm::scope_exit([&] {
    for (Value argument : block->getArguments()) forgetValueFacts(argument);
    block->walk([&](Operation *operation) { for (Value result : operation->getResults()) forgetValueFacts(result); });
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
  auto results = instantiateValueHelper(**helper, actuals, call, inside, symbol);
  if (failed(results)) return failure();
  auto staged = retainQueryMap(*block, rows, tables, *results, false, call, at, symbol);
  if (failed(staged)) return failure();
  storage.release();
  return staged->front();
}
