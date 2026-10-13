LogicalResult Importer::scanSourceCallables() {
  llvm::StringMap<SmallVector<std::pair<AstNode, CapturedCallableSyntax>, 0>>
      staged;
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    AstNode module = source.module.item("body", i);
    if (module.kind() != "FunctionDef" ||
        (!hasDecorator(module, "module") && !hasDecorator(module, "system")))
      continue;
    StringRef moduleName = module.string("name");
    for (size_t j = 0; j < module.array("body").size(); ++j) {
      AstNode declaration = module.item("body", j);
      if (declaration.kind() != "FunctionDef")
        continue;
      bool marked = false;
      for (size_t k = 0; k < declaration.array("decorator_list").size(); ++k) {
        AstNode decorator = declaration.item("decorator_list", k);
        AstNode target =
            decorator.kind() == "Call" ? decorator.child("func") : decorator;
        auto resolved = resolveBinding(target);
        marked |= resolved && (resolved->marker == MarkerKind::Rule ||
                               resolved->marker == MarkerKind::Method);
      }
      if (!marked)
        continue;
      auto syntax = readCallableSyntax(
          declaration,
          [&](const AstNode &node) { return resolveBinding(node); }, error);
      if (failed(syntax)) {
        return failure();
      }
      staged[moduleName].push_back({declaration, *syntax});
      if (syntax->marker == MarkerKind::Method)
        methodOwnerNames.insert(moduleName);
    }
  }
  // Classify the actual static module-call graph before reading callable
  // contracts. This includes imported transaction value modules and propagates
  // their publication route through local parents without making them handles.
  llvm::StringMap<SmallVector<std::string, 0>> edges;
  llvm::StringSet<> moduleNames;
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    AstNode module = source.module.item("body", i);
    if (module.kind() == "FunctionDef" &&
        (hasDecorator(module, "module") || hasDecorator(module, "system")))
      moduleNames.insert(module.string("name"));
  }
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    AstNode module = source.module.item("body", i);
    if (module.kind() != "FunctionDef" ||
        !moduleNames.contains(module.string("name")))
      continue;
    StringRef moduleName = module.string("name");
    if (methodOwnerNames.contains(moduleName))
      transactionModuleNames.insert(moduleName);
    auto walk = [&](auto &&self, const AstNode &node) -> void {
      if (node.kind() == "FunctionDef" || node.kind() == "Lambda")
        return;
      if (node.kind() == "Call") {
        AstNode function = node.child("func");
        if (function.kind() == "Name" &&
            moduleNames.contains(function.string("id")) &&
            !lexicallyShadowed(function, function.string("id")))
          edges[moduleName].push_back(function.string("id").str());
        auto binding = lookupBinding(function);
        if (binding && binding->category == BindingCategory::Module) {
          auto view = headers.lookupModule(binding->symbol);
          if (view && view.semanticKind &&
              view.semanticKind.getValue() == "transactions")
            transactionModuleNames.insert(moduleName);
        }
      }
      for (NamedAttribute field : node.fields()) {
        if (auto child = dyn_cast<DictionaryAttr>(field.getValue())) {
          if (child.getAs<StringAttr>("kind"))
            self(self, node.child(field.getName().getValue()));
        } else if (auto children = dyn_cast<ArrayAttr>(field.getValue())) {
          for (size_t j = 0; j < children.size(); ++j)
            if (auto child = dyn_cast<DictionaryAttr>(children[j]);
                child && child.getAs<StringAttr>("kind"))
              self(self, node.item(field.getName().getValue(), j));
        }
      }
    };
    for (size_t j = 0; j < module.array("body").size(); ++j)
      walk(walk, module.item("body", j));
  }
  bool changed;
  do {
    changed = false;
    for (auto &entry : edges)
      if (!transactionModuleNames.contains(entry.first()))
        for (const std::string &child : entry.second)
          if (transactionModuleNames.contains(child)) {
            transactionModuleNames.insert(entry.first());
            changed = true;
            break;
          }
  } while (changed);
  // Existing nested Rule syntax outside the selected typed closure remains on
  // its established structural/behavioral route during this staged cutover.
  for (auto &moduleEntry : staged) {
    StringRef moduleName = moduleEntry.first();
    if (!transactionModuleNames.contains(moduleName)) {
      for (const auto &entry : moduleEntry.second)
        if (entry.second.marker == MarkerKind::Rule && entry.second.guard)
          return error()
                 << "guarded nested Rule cannot use the legacy Rule path";
      continue;
    }
    for (auto &entry : moduleEntry.second) {
      AstNode declaration = entry.first;
      CapturedCallableSyntax syntax = entry.second;
      SourceCallableDecl callable{declaration, syntax};
      AstNode arguments = declaration.child("args");
      if (!arguments.array("posonlyargs").empty() ||
          !arguments.array("kwonlyargs").empty() ||
          !arguments.array("defaults").empty() || arguments.child("vararg") ||
          arguments.child("kwarg"))
        return error() << "Rule/Method parameters require ordinary required "
                          "positional arguments";
      llvm::StringSet<> names;
      for (size_t k = 0; k < arguments.array("args").size(); ++k) {
        AstNode argument = arguments.item("args", k);
        if (!argument.child("annotation") ||
            !names.insert(argument.string("arg")).second)
          return error()
                 << "Method parameter requires a unique explicit annotation";
        auto type = portType(
            argument.child("annotation"),
            FlatSymbolRefAttr::get(b.getContext(), qualify(moduleName)));
        if (failed(type))
          return failure();
        callable.inputs.push_back(
            {argument.string("arg").str(), *type, argument,
             annotationKind(argument.child("annotation")),
             fixedAnnotation(argument.child("annotation"))});
      }
      AstNode returns = declaration.child("returns");
      bool none =
          returns.kind() == "Constant" && isa<UnitAttr>(returns.get("value"));
      if (!returns)
        return error() << "Method/Rule requires an explicit return annotation";
      if (!none) {
        auto type = portType(returns, FlatSymbolRefAttr::get(
                                          b.getContext(), qualify(moduleName)));
        if (failed(type))
          return failure();
        callable.results.push_back(*type);
        callable.resultContracts.push_back({"result", *type, returns,
                                            annotationKind(returns),
                                            fixedAnnotation(returns)});
      }
      SmallVector<Type> inputs;
      for (const Port &port : callable.inputs)
        inputs.push_back(port.type);
      callable.type =
          FunctionType::get(b.getContext(), inputs, callable.results);
      auto &inventory = syntax.marker == MarkerKind::Method
                            ? sourceMethods[moduleName]
                            : sourceRules[moduleName];
      if (!inventory.try_emplace(declaration.string("name"), callable).second)
        return error() << "duplicate Rule/Method declaration name";
    }
  }
  // A guarded top-level Rule is newly recognized syntax. The old pure-rule
  // route has no place to preserve its guard, so reject it explicitly.
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    AstNode declaration = source.module.item("body", i);
    if (declaration.kind() != "FunctionDef")
      continue;
    bool marked = false;
    for (size_t j = 0; j < declaration.array("decorator_list").size(); ++j) {
      AstNode decorator = declaration.item("decorator_list", j);
      AstNode target =
          decorator.kind() == "Call" ? decorator.child("func") : decorator;
      auto resolved = resolveBinding(target);
      marked |= resolved && resolved->marker == MarkerKind::Rule;
    }
    if (!marked)
      continue;
    auto syntax = readCallableSyntax(
        declaration, [&](const AstNode &node) { return resolveBinding(node); },
        error);
    if (succeeded(syntax) && syntax->marker == MarkerKind::Rule &&
        syntax->guard)
      return error()
             << "guarded top-level Rule requires the typed transaction route";
    if (failed(syntax))
      return failure();
  }
  return success();
}

LogicalResult Importer::emitSourceTransactions(ModuleDecl &decl) {
  if (!decl.parameters.empty())
    return error()
           << "typed transaction modules do not accept static parameters";
  if (decl.methodOwner && (decl.sourceInputCount != 0 || !decl.outputs.empty()))
    return error() << "Method owner cannot declare authored parameters or an "
                      "ordinary return";

  b.setInsertionPointToEnd(body->getBody());
  Operation *module = createOp(
      b, decl.node.location(b.getContext(), source.path),
      ac::ModuleOp::getOperationName(), {}, {},
      {b.getNamedAttr("sym_name", b.getStringAttr(decl.symbol.getValue())),
       b.getNamedAttr("source_owner", owner),
       b.getNamedAttr("parameters", b.getArrayAttr({})),
       b.getNamedAttr("type_parameters", b.getArrayAttr({})),
       b.getNamedAttr("function_type", TypeAttr::get(decl.type)),
       b.getNamedAttr("input_names", portNames(b, decl.inputs)),
       b.getNamedAttr("output_names", portNames(b, decl.outputs)),
       b.getNamedAttr("ac.declaration_role", b.getStringAttr("definition")),
       b.getNamedAttr("ac.origin",
                      sourceOccurrence(b, decl.symbol, decl.node))},
      1);
  if (decl.system)
    module->setAttr("ac.root_kind", b.getStringAttr("system"));
  for (NamedAttribute attr : decl.sourceCallAttrs)
    module->setAttr(attr.getName(), attr.getValue());
  auto *graph = new Block();
  module->getRegion(0).push_back(graph);
  llvm::StringMap<Value> moduleValues;
  for (const Port &port : decl.inputs) {
    Value value = graph->addArgument(port.type, module->getLoc());
    moduleValues[port.name] = value;
    remember(value, port.sourceKind);
    if (port.fixedBits)
      fixedValues.insert(value);
  }
  auto clock = moduleValues.find("pyc_clk"),
       reset = moduleValues.find("pyc_rst");
  if (clock == moduleValues.end() || reset == moduleValues.end())
    return error() << "typed transactions require inferred clock and reset";
  OpBuilder at = OpBuilder::atBlockEnd(graph);
  llvm::StringMap<StateOwner> states;
  auto *previousBehavioralDecl = activeBehavioralDecl;
  activeBehavioralDecl = &decl;
  auto restoreBehavioralDecl = llvm::scope_exit(
      [&] { activeBehavioralDecl = previousBehavioralDecl; });
  if (failed(predeclareDirectCalls(decl, at)))
    return failure();

  SmallVector<Instance, 0> instanceStorage;
  llvm::StringMap<Instance *> instances;
  auto &methods = sourceMethods[decl.node.string("name")];
  auto &rules = sourceRules[decl.node.string("name")];
  using CaptureSnapshot = SmallVector<std::pair<std::string, Value>, 0>;
  llvm::StringMap<CaptureSnapshot> methodCaptures, ruleCaptures;
  llvm::DenseSet<DictionaryAttr> ordinaryModuleAssignments;
  llvm::StringSet<> authoredInputs;
  for (size_t i = 0; i < decl.sourceInputCount; ++i)
    authoredInputs.insert(decl.inputs[i].name);

  auto snapshot = [&](CaptureSnapshot &captures) {
    captures.clear();
    for (const auto &value : moduleValues)
      if (value.first() != "pyc_clk" && value.first() != "pyc_rst")
        captures.push_back({value.first().str(), value.second});
  };

  size_t instanceCount = 0;
  for (size_t i = 0; i < decl.node.array("body").size(); ++i) {
    AstNode statement = decl.node.item("body", i);
    if (statement.kind() == "Assign" &&
        statement.array("targets").size() == 1 &&
        statement.item("targets", 0).kind() == "Name" &&
        statement.child("value").kind() == "Call")
      ++instanceCount;
  }
  instanceStorage.reserve(instanceCount);
  for (size_t i = 0; i < decl.node.array("body").size(); ++i) {
    AstNode statement = decl.node.item("body", i);
    if (statement.kind() == "FunctionDef") {
      StringRef name = statement.string("name");
      if (statement.array("decorator_list").empty()) {
        if (failed(declareValueHelper(statement, decl.symbol, moduleValues))) return failure();
      } else if (!methods.contains(name) && !rules.contains(name))
        return error() << "nested callable requires Rule, Method or ordinary typed helper";
      if (methods.contains(name))
        snapshot(methodCaptures[name]);
      else if (rules.contains(name))
        snapshot(ruleCaptures[name]);
      continue;
    }
    if (docstring(statement) || statement.kind() == "Return" ||
        statement.kind() == "Expr" || statement.kind() == "Pass")
      continue;
    if (statement.kind() == "Assign" &&
        statement.child("value").kind() == "Call" &&
        statement.child("value").child("func").kind() == "Name" &&
        rules.contains(statement.child("value").child("func").string("id")))
      continue;
    if (statement.kind() == "AnnAssign") {
      if (statement.child("target").kind() != "Name" ||
          !statement.child("value"))
        return error() << "typed owner state requires a fixed annotated Name "
                          "initializer";
      StringRef name = statement.child("target").string("id");
      if (states.contains(name) || methods.contains(name) ||
          rules.contains(name))
        return error() << "persistent owner name is duplicated";
      AstNode annotation = statement.child("annotation");
      if (!fixedAnnotation(annotation))
        return error() << "typed transaction state requires explicitly fixed "
                          "Bits annotation";
      auto type = portType(annotation, decl.symbol);
      if (failed(type) || !isa<ac::BitsType>(*type))
        return error()
               << "typed transaction state requires fixed Bits scalar type";
      llvm::StringMap<Instance *> none;
      auto initial = expression(statement.child("value"), at, decl.symbol,
                                moduleValues, none, *type);
      if (failed(initial))
        return failure();
      Port boundaryPort{name.str(), *type, annotation,
                        annotationKind(annotation), true};
      auto converted = applyBindingBoundary(
          *initial, BindingBoundary{boundaryPort, true}, statement.child("value"),
          at, decl.symbol);
      if (failed(converted))
        return failure();
      Operation *state = createOp(
          at, statement.location(b.getContext(), source.path),
          ac::SourceStateOp::getOperationName(),
          ValueRange{clock->second, reset->second, *converted},
          TypeRange{*type},
          {at.getNamedAttr("instance_name", at.getStringAttr(name)),
           at.getNamedAttr("occurrence",
                           sourceOccurrence(at, decl.symbol, statement))});
      helperOwnerBindings.insert(name);
      states[name] = {name.str(), *type, *converted, state, statement};
      continue;
    }
    if (statement.kind() == "Assign" &&
        statement.array("targets").size() == 1 &&
        statement.item("targets", 0).kind() == "Name") {
      StringRef name = statement.item("targets", 0).string("id");
      AstNode value = statement.child("value");
      if (states.contains(name))
        return error()
               << "instance handle assignment shadows a persistent owner";
      if (methods.contains(name) || rules.contains(name) ||
          instances.contains(name))
        return error() << "instance handle cannot be rebound and requires a "
                          "fresh immutable name";
      if (value.kind() == "Name" && instances.contains(value.string("id"))) {
        if (moduleValues.contains(name))
          return error() << "instance handle cannot be rebound and requires a "
                            "fresh immutable name";
        instances[name] = instances[value.string("id")];
        continue;
      }
      if (value.kind() == "Call" && value.child("func").kind() == "Attribute" &&
          value.child("func").child("value").kind() == "Name" &&
          instances.contains(value.child("func").child("value").string("id")))
        return error() << "Method call requires a Rule transaction scope";
      if (value.kind() == "Call" && methodOwner(value.child("func"))) {
        if (moduleValues.contains(name))
          return error() << "instance handle cannot be rebound and requires a "
                            "fresh immutable name";
        if (!value.array("args").empty() || !value.array("keywords").empty())
          return error()
                 << "Method owner construction takes no authored parameters";
        AstNode function = value.child("func");
        FlatSymbolRefAttr callee;
        Operation *declaration = nullptr;
        unsigned clockInput = 0, resetInput = 1;
        auto target = function.kind() == "Name"
                          ? local.find(function.string("id"))
                          : local.end();
        if (target != local.end()) {
          if (modules[target->second].system)
            return error() << "@system is root-only and cannot be called as a child";
          callee = modules[target->second].symbol;
        } else {
          auto binding = lookupBinding(function);
          auto view = headers.lookupModule(binding->symbol);
          if (view.declaration->hasAttr("ac.root_kind"))
            return error() << "@system is root-only and cannot be called as a child";
          callee = view.symbol;
          declaration = view.declaration;
          clockInput = view.domainInputs.getAs<IntegerAttr>("clock").getInt();
          resetInput = view.domainInputs.getAs<IntegerAttr>("reset").getInt();
        }
        SmallVector<Value> inputs(2);
        inputs[clockInput] = clock->second;
        inputs[resetInput] = reset->second;
        Operation *instance = createOp(
            at, statement.location(b.getContext(), source.path),
            ac::InstanceOp::getOperationName(), inputs, TypeRange{},
            {at.getNamedAttr("instance_name", at.getStringAttr(name)),
             at.getNamedAttr("callee", callee),
             at.getNamedAttr("parameters", at.getArrayAttr({})),
             at.getNamedAttr("type_arguments", at.getArrayAttr({})),
             at.getNamedAttr("occurrence",
                             sourceOccurrence(at, decl.symbol, statement))});
        instanceStorage.push_back({name.str(), statement, callee, declaration});
        instanceStorage.back().op = instance;
        instanceStorage.back().clockInput = clockInput;
        instanceStorage.back().resetInput = resetInput;
        instances[name] = &instanceStorage.back();
        continue;
      }
      if (authoredInputs.contains(name))
        return error()
               << "module-local assignment cannot rebind an authored input";
      auto ordinary =
          expression(value, at, decl.symbol, moduleValues, instances);
      if (failed(ordinary))
        return failure();
      moduleValues[name] = *ordinary;
      ordinaryModuleAssignments.insert(
          sourceOccurrence(at, decl.symbol, statement));
      continue;
    }
    return error() << "unsupported typed transaction module statement";
  }

  auto boundHardwareIdentity = [&](StringRef name) {
    return methods.contains(name) || rules.contains(name) ||
           instances.contains(name) || names.contains(name) ||
           structs.contains(name) || enums.contains(name) ||
           behavioralRules.contains(name);
  };

  SmallVector<ValueHelperDecl *> lexicalHelpers;
  for (auto &entry : valueHelpers)
    if (entry.second->lexicalOwner == decl.symbol) lexicalHelpers.push_back(entry.second.get());
  for (auto *helper : lexicalHelpers)
    if (failed(lowerValueHelper(*helper))) return failure();
  auto retainHelperSnapshots = [&](CaptureSnapshot &captures) {
    for (const auto &entry : valueHelpers) {
      if (entry.second->lexicalOwner != decl.symbol) continue;
      for (const auto &capture : entry.second->captures)
        if (llvm::none_of(captures, [&](const auto &existing) { return existing.second == capture.second; }))
          captures.push_back({{}, capture.second});
    }
  };
  for (auto &entry : methodCaptures) retainHelperSnapshots(entry.second);
  for (auto &entry : ruleCaptures) retainHelperSnapshots(entry.second);

  auto emitStatements = [&](SourceCallableDecl &callable, OpBuilder &inside,
                            llvm::StringMap<Value> &values,
                            bool guard) -> FailureOr<SmallVector<Value>> {
    SourceEffectContext context{&decl,      &callable, &states,
                                &instances, &methods,  guard};
    auto *saved = activeSourceEffects;
    activeSourceEffects = &context;
    llvm::scope_exit restore([&] { activeSourceEffects = saved; });
    SmallVector<Value> yielded;
    if (guard) {
      AstNode expressionNode = callable.syntax.guard
                                   ? callable.syntax.guard.child("body")
                                   : callable.declaration;
      auto value = callable.syntax.guard
                       ? expression(expressionNode, inside, decl.symbol, values,
                                    instances)
                       : trueValue(expressionNode, inside, decl.symbol);
      if (failed(value))
        return error() << "guard expression must be pure and return a predicate";
      if (!callable.syntax.guard) {
        yielded.push_back(*value);
      } else {
        auto predicate = predicateView(*value, expressionNode, decl.symbol);
        if (failed(predicate))
          return error()
                 << "guard requires a Boolean or authoritative one-bit value";
        yielded.push_back(predicate->value);
      }
      return yielded;
    }
    AstNode returned;
    for (size_t i = 0; i < callable.declaration.array("body").size(); ++i) {
      AstNode statement = callable.declaration.item("body", i);
      if (returned)
        return error() << "declaration after Method/Rule return is unsupported";
      if (statement.kind() == "If")
        return error() << "effectful branch in Method/Rule is unsupported";
      if (statement.kind() == "For" || statement.kind() == "While")
        return error() << "effectful loop in Method/Rule is unsupported";
      if (statement.kind() == "FunctionDef")
        return error() << "nested Rule/Method declaration is unsupported";
      if (statement.kind() == "Pass")
        continue;
      if (statement.kind() == "Return") {
        returned = statement;
        if (statement.child("value")) {
          if (callable.results.size() != 1)
            return error() << "None Method/Rule cannot return a value";
          auto value = expression(statement.child("value"), inside, decl.symbol,
                                  values, instances, callable.results.front());
          if (failed(value))
            return failure();
          const Port &contract = callable.resultContracts.front();
          auto converted = applyBindingBoundary(
              *value, BindingBoundary{contract, true}, statement.child("value"),
              inside, decl.symbol);
          if (failed(converted))
            return failure();
          if (converted->getType() != contract.type &&
              isa<ac::BitsType>(converted->getType()) &&
              isa<ac::BitsType>(contract.type) &&
              ac::areEquivalentHardwareTypes(converted->getType(),
                                             contract.type))
            converted = boundaryBitsIdentity(*converted, contract.type,
                                             statement.child("value"), inside,
                                             decl.symbol);
          yielded.push_back(*converted);
        }
        continue;
      }
      AstNode target, rhs;
      if (statement.kind() == "Assign" &&
          statement.array("targets").size() == 1) {
        target = statement.item("targets", 0);
        rhs = statement.child("value");
      } else if (statement.kind() == "AnnAssign") {
        target = statement.child("target");
        rhs = statement.child("value");
      } else if (statement.kind() == "Expr") {
        AstNode expressionNode = statement.child("value");
        if (expressionNode.kind() == "Call" &&
            expressionNode.child("func").kind() == "Name" &&
            rules.contains(expressionNode.child("func").string("id")))
          return error() << "nested Rule calls are unsupported";
        auto helper = expressionNode.kind() == "Call" ? resolveValueHelper(expressionNode.child("func"), decl.symbol)
                                                    : FailureOr<ValueHelperDecl *>(static_cast<ValueHelperDecl *>(nullptr));
        if (failed(helper)) return failure();
        if (*helper) {
          if (failed(valueHelperCall(expressionNode, inside, decl.symbol, values, instances))) return failure();
          continue;
        }
        auto results = sourceMethodCall(statement.child("value"), inside,
                                        decl.symbol, values, instances);
        if (failed(results))
          return error()
                 << "Rule/Method expression requires a direct Method call";
        continue;
      } else {
        return error() << "unsupported Method/Rule statement";
      }
      if (target.kind() != "Name" || !rhs)
        return error() << "Method/Rule assignment requires a direct Name";
      StringRef name = target.string("id");
      if (rhs.kind() == "Call" && rhs.child("func").kind() == "Name" &&
          rules.contains(rhs.child("func").string("id")))
        return error() << "nested Rule calls are unsupported";
      if (rhs.kind() == "Attribute" && rhs.child("value").kind() == "Name" &&
          instances.contains(rhs.child("value").string("id")))
        return error() << "indirect Method values are unsupported";
      if (states.contains(name)) {
        if (statement.kind() == "AnnAssign")
          return error() << "owner shadow declaration is unsupported";
        auto value = expression(rhs, inside, decl.symbol, values, instances,
                                states[name].type);
        if (failed(value))
          return failure();
        auto converted = applyBindingBoundary(
            *value, ownerBoundary(states[name]), rhs, inside, decl.symbol);
        if (failed(converted))
          return failure();
        auto path = trueValue(statement, inside, decl.symbol);
        if (failed(path))
          return failure();
        createOp(inside, statement.location(b.getContext(), source.path),
                 ac::SourceStateWriteOp::getOperationName(),
                 ValueRange{*path, *converted}, TypeRange{},
                 {inside.getNamedAttr("state", inside.getStringAttr(name)),
                  inside.getNamedAttr("selectors", inside.getArrayAttr({})),
                  inside.getNamedAttr(
                      "occurrence",
                      sourceOccurrence(inside, decl.symbol, statement))});
        continue;
      }
      if (boundHardwareIdentity(name))
        return error()
               << "Method/Rule local shadows a bound callable, instance, "
                  "import or type declaration";
      if (statement.kind() == "AnnAssign" && values.contains(name))
        return error()
               << "Method/Rule local annotation cannot redeclare a binding";
      std::optional<Type> expected;
      Port localContract;
      if (statement.kind() == "AnnAssign") {
        auto type = portType(statement.child("annotation"), decl.symbol);
        if (failed(type))
          return failure();
        expected = *type;
        localContract = {name.str(), *type, statement.child("annotation"),
                         annotationKind(statement.child("annotation")),
                         fixedAnnotation(statement.child("annotation"))};
      }
      auto value =
          expression(rhs, inside, decl.symbol, values, instances, expected);
      if (failed(value))
        return failure();
      if (expected) {
        auto converted = applyBindingBoundary(
            *value, BindingBoundary{localContract, true}, rhs, inside, decl.symbol);
        if (failed(converted))
          return failure();
        values[name] = *converted;
      } else {
        values[name] = *value;
      }
    }
    if (callable.results.size() != yielded.size())
      return error() << "Method/Rule result requires one final return";
    return yielded;
  };

  for (auto &entry : methods) {
    SourceCallableDecl &callable = entry.second;
    for (const Port &port : callable.inputs) {
      if (states.contains(port.name))
        return error() << "Method parameter shadows an owner state";
      if (boundHardwareIdentity(port.name))
        return error()
               << "Method parameter shadows a bound callable, instance, "
                  "import or type declaration";
    }
    SmallVector<Value> captures;
    SmallVector<StringRef> captureNames;
    for (const auto &value : methodCaptures[entry.first()]) {
      if (llvm::any_of(callable.inputs, [&](const Port &formal) {
            return formal.name == value.first;
          }))
        continue;
      captureNames.push_back(value.first);
      captures.push_back(value.second);
    }
    SmallVector<Attribute> argumentNames, parameters, resultConstraints;
    for (const Port &port : callable.inputs) {
      argumentNames.push_back(at.getStringAttr(port.name));
      auto constraint = sourceConstraint(port);
      if (failed(constraint))
        return failure();
      parameters.push_back(at.getDictionaryAttr(
          {at.getNamedAttr("name", at.getStringAttr(port.name)),
           at.getNamedAttr("binding",
                           at.getStringAttr("positional_or_keyword")),
           at.getNamedAttr("constraint", *constraint),
           at.getNamedAttr("default", absentDefault(at)),
           at.getNamedAttr("origin",
                           sourceOccurrence(at, decl.symbol, port.node)),
           at.getNamedAttr("location",
                           sourceSpan(at, source.path, port.node))}));
    }
    for (const Port &port : callable.resultContracts) {
      auto constraint = sourceConstraint(port);
      if (failed(constraint))
        return failure();
      resultConstraints.push_back(*constraint);
    }
    Operation *method = createOp(
        at, callable.declaration.location(b.getContext(), source.path),
        ac::SourceMethodOp::getOperationName(), captures, TypeRange{},
        {at.getNamedAttr("sym_name", at.getStringAttr(entry.first())),
         at.getNamedAttr("function_type", TypeAttr::get(callable.type)),
         at.getNamedAttr("argument_names", at.getArrayAttr(argumentNames)),
         at.getNamedAttr("ac.parameters", at.getArrayAttr(parameters)),
         at.getNamedAttr("ac.result_constraints",
                         at.getArrayAttr(resultConstraints)),
         at.getNamedAttr(
             "ac.return_form",
             at.getStringAttr(callable.results.empty() ? "none" : "single")),
         at.getNamedAttr("occurrence", sourceOccurrence(at, decl.symbol,
                                                        callable.declaration))},
        2);
    for (unsigned regionIndex = 0; regionIndex < 2; ++regionIndex) {
      auto *block = new Block();
      method->getRegion(regionIndex).push_back(block);
      llvm::StringMap<Value> values;
      for (const Port &port : callable.inputs) {
        Value argument = block->addArgument(port.type, method->getLoc());
        values[port.name] = argument;
        remember(argument, port.sourceKind);
        if (port.fixedBits)
          fixedValues.insert(argument);
      }
      for (auto [name, original] : llvm::zip(captureNames, captures)) {
        auto captured = captureValue(block, original, method->getLoc());
        if (!name.empty()) values[name] = captured;
      }
      OpBuilder inside = OpBuilder::atBlockEnd(block);
      auto yielded = emitStatements(callable, inside, values, regionIndex == 0);
      if (failed(yielded))
        return failure();
      createOp(inside,
               callable.declaration.location(b.getContext(), source.path),
               ac::YieldOp::getOperationName(), *yielded, {}, {});
    }
  }

  llvm::StringMap<unsigned> registrationCounts;
  SmallVector<Value> moduleOutputs;
  llvm::DenseSet<DictionaryAttr> ordinaryModuleReturns;
  bool returned = false;
  for (size_t i = 0; i < decl.node.array("body").size(); ++i) {
    AstNode statement = decl.node.item("body", i);
    AstNode call = statement.kind() == "Expr"     ? statement.child("value")
                   : statement.kind() == "Return" ? statement.child("value")
                   : statement.kind() == "Assign" ? statement.child("value")
                                                  : AstNode();
    bool registration = call.kind() == "Call" &&
                        call.child("func").kind() == "Name" &&
                        rules.contains(call.child("func").string("id"));
    if (statement.kind() == "Return") {
      if (decl.methodOwner)
        return error() << "Method owner cannot return an ordinary module value";
      if (returned)
        return error() << "module requires one final return";
      returned = true;
    }
    if (!registration) {
      if (statement.kind() != "Return")
        continue;
      auto outputs = lowerReturn(decl, call, at, moduleValues, instances);
      if (failed(outputs))
        return failure();
      llvm::append_range(moduleOutputs, *outputs);
      ordinaryModuleReturns.insert(sourceOccurrence(at, decl.symbol, statement));
      continue;
    }
    StringRef name = call.child("func").string("id");
    if (++registrationCounts[name] != 1)
      return error() << "Rule declaration must be registered exactly once";
    SourceCallableDecl &callable = rules[name];
    if (!callable.inputs.empty() || !call.array("args").empty() ||
        !call.array("keywords").empty())
      return error() << "nested Rule registration arguments are unsupported";
    SmallVector<Value> captures;
    SmallVector<StringRef> captureNames;
    for (const auto &value : ruleCaptures[name]) {
      captureNames.push_back(value.first);
      captures.push_back(value.second);
    }
    Operation *transaction = createOp(
        at, call.location(b.getContext(), source.path),
        ac::SourceTransactionOp::getOperationName(),
        SmallVector<Value>{clock->second, reset->second}, callable.results,
        {at.getNamedAttr("name", at.getStringAttr(name)),
         at.getNamedAttr("declaration", sourceOccurrence(at, decl.symbol,
                                                         callable.declaration)),
         at.getNamedAttr("occurrence",
                         sourceOccurrence(at, decl.symbol, call))},
        2);
    transaction->insertOperands(2, captures);
    for (unsigned regionIndex = 0; regionIndex < 2; ++regionIndex) {
      auto *block = new Block();
      transaction->getRegion(regionIndex).push_back(block);
      llvm::StringMap<Value> values;
      for (auto [captureName, original] : llvm::zip(captureNames, captures)) {
        auto captured = captureValue(block, original, transaction->getLoc());
        if (!captureName.empty()) values[captureName] = captured;
      }
      OpBuilder inside = OpBuilder::atBlockEnd(block);
      auto yielded = emitStatements(callable, inside, values, regionIndex == 0);
      if (failed(yielded))
        return failure();
      createOp(inside,
               callable.declaration.location(b.getContext(), source.path),
               ac::YieldOp::getOperationName(), *yielded, {}, {});
    }
    if (statement.kind() == "Return")
      llvm::append_range(moduleOutputs, transaction->getResults());
  }
  for (const auto &rule : rules)
    if (registrationCounts[rule.first()] != 1)
      return error() << "Rule declaration must be registered exactly once";
  for (size_t i = 0; i < decl.node.array("body").size(); ++i) {
    AstNode statement = decl.node.item("body", i);
    if (statement.kind() == "FunctionDef" || statement.kind() == "AnnAssign" ||
        docstring(statement) || statement.kind() == "Pass")
      continue;
    AstNode value = statement.kind() == "Expr"     ? statement.child("value")
                    : statement.kind() == "Return" ? statement.child("value")
                    : statement.kind() == "Assign" ? statement.child("value")
                                                   : AstNode();
    bool registration = value.kind() == "Call" &&
                        value.child("func").kind() == "Name" &&
                        rules.contains(value.child("func").string("id"));
    bool construction =
        statement.kind() == "Assign" && value.kind() == "Call" &&
        methodOwner(value.child("func"));
    bool alias = statement.kind() == "Assign" && value.kind() == "Name" &&
                 instances.contains(value.string("id"));
    bool ordinary = ordinaryModuleAssignments.contains(
        sourceOccurrence(at, decl.symbol, statement));
    if (registration || construction || alias || ordinary ||
        ordinaryModuleReturns.contains(
            sourceOccurrence(at, decl.symbol, statement)))
      continue;
    if (decl.methodOwner && statement.kind() == "Return")
      return error() << "Method owner cannot return an ordinary module value";
    if (statement.kind() == "Expr" || statement.kind() == "Return")
      return error() << "Method calls require a registered Rule transaction";
    return error() << "unsupported typed transaction module statement";
  }
  if (moduleOutputs.size() != decl.outputs.size())
    return error()
           << "typed transaction module return does not match its interface";
  createOp(at, module->getLoc(), ac::YieldOp::getOperationName(), moduleOutputs,
           {}, {});
  return success();
}

FailureOr<SmallVector<Value>> Importer::sourceMethodCall(
    const AstNode &node, OpBuilder &at, FlatSymbolRefAttr owner,
    llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances) {
  if (!activeSourceEffects || node.kind() != "Call")
    return failure();
  AstNode function = node.child("func");
  SourceCallableDecl *target = nullptr;
  std::optional<ac::SourceMethodView> imported;
  Instance *receiver = nullptr;
  bool recognizedReceiver = false;
  StringRef methodName;
  SmallVector<Attribute> path;
  FlatSymbolRefAttr moduleSymbol;
  if (function.kind() == "Name" && activeSourceEffects->methods) {
    methodName = function.string("id");
    auto found = activeSourceEffects->methods->find(methodName);
    if (found != activeSourceEffects->methods->end()) {
      target = &found->second;
      moduleSymbol = activeSourceEffects->module->symbol;
    }
  } else if (function.kind() == "Attribute" &&
             function.child("value").kind() == "Name") {
    StringRef receiverName = function.child("value").string("id");
    auto found = instances.find(receiverName);
    if (found != instances.end()) {
      recognizedReceiver = true;
      receiver = found->second;
      methodName = function.string("attr");
      bool localReceiver = false;
      for (auto &candidate : modules)
        if (candidate.symbol == receiver->callee) {
          localReceiver = true;
          auto methods = sourceMethods.find(candidate.node.string("name"));
          if (methods != sourceMethods.end()) {
            auto method = methods->second.find(methodName);
            if (method != methods->second.end())
              target = &method->second;
          }
          break;
        }
      if (!localReceiver) {
        auto view = headers.lookupModule(receiver->callee);
        if (!view)
          return error() << "Method receiver lacks a validated module contract";
        auto method = ac::lookupSourceMethod(view.declaration, methodName,
                                             body->getOperation());
        if (failed(method))
          return failure();
        imported = *method;
      }
      moduleSymbol = receiver->callee;
      // Aliases retain the actual instance identity, not their local spelling.
      path.push_back(at.getStringAttr(receiver->name));
    }
  }
  if (!target && !imported && recognizedReceiver)
    return error() << "unknown Method on statically identified instance";
  if (!target && !imported)
    return failure();
  if (activeSourceEffects->guard)
    return error() << "guard cannot call a Method";

  SmallVector<Port> importedInputs, importedResults;
  SmallVector<Type> importedResultTypes;
  if (imported) {
    for (Attribute raw : imported->getParameters()) {
      auto parameter = cast<DictionaryAttr>(raw);
      importedInputs.push_back(sourceCallPort(
          parameter.getAs<DictionaryAttr>("constraint"),
          parameter.getAs<StringAttr>("name").getValue(), node));
    }
    for (Attribute raw : imported->getResultConstraints())
      importedResults.push_back(
          sourceCallPort(cast<DictionaryAttr>(raw), "result", node));
    llvm::append_range(importedResultTypes,
                       imported->getFunctionType().getResults());
  }
  ArrayRef<Port> inputs = target ? ArrayRef<Port>(target->inputs)
                                 : ArrayRef<Port>(importedInputs);
  ArrayRef<Port> resultContracts = target
                                      ? ArrayRef<Port>(target->resultContracts)
                                      : ArrayRef<Port>(importedResults);
  ArrayRef<Type> results = target ? ArrayRef<Type>(target->results)
                                  : ArrayRef<Type>(importedResultTypes);
  if (!node.array("keywords").empty() ||
      node.array("args").size() != inputs.size())
    return error() << "Method call arguments must match its typed signature";
  SmallVector<Value> arguments;
  for (auto [index, port] : llvm::enumerate(inputs)) {
    AstNode actual = node.item("args", index);
    auto value = expression(actual, at, owner, values, instances, port.type);
    if (failed(value))
      return failure();
    // Typed actuals share declared-binding authority: fixed Bits use the
    // unsigned boundary, while logical Integer/Boolean keep strict kind checks.
    auto converted = applyBindingBoundary(
        *value, BindingBoundary{port, true}, actual, at, owner);
    if (failed(converted))
      return failure();
    arguments.push_back(*converted);
  }
  auto enabled = trueValue(node, at, owner);
  if (failed(enabled))
    return failure();
  auto nested = FlatSymbolRefAttr::get(b.getContext(), methodName);
  auto callee =
      SymbolRefAttr::get(b.getContext(), moduleSymbol.getValue(), {nested});
  auto operation = createOp(
      at, node.location(b.getContext(), source.path),
      ac::SourceMethodCallOp::getOperationName(), SmallVector<Value>{*enabled},
      results,
      {at.getNamedAttr("callee", callee),
       at.getNamedAttr("instance_path", at.getArrayAttr(path)),
       at.getNamedAttr("occurrence", sourceOccurrence(at, owner, node))});
  operation->insertOperands(1, arguments);
  for (auto [result, contract] :
       llvm::zip(operation->getResults(), resultContracts)) {
    remember(result, contract.sourceKind);
    if (contract.fixedBits)
      fixedValues.insert(result);
  }
  return SmallVector<Value>(operation->getResults());
}

FailureOr<std::optional<Value>> Importer::sourceEffectExpression(
    const AstNode &node, OpBuilder &at, FlatSymbolRefAttr owner,
    llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances) {
  if (!activeSourceEffects)
    return std::optional<Value>();
  if (node.kind() == "Name" && activeSourceEffects->states) {
    auto found = activeSourceEffects->states->find(node.string("id"));
    if (found != activeSourceEffects->states->end()) {
      auto path = trueValue(node, at, owner);
      if (failed(path))
        return failure();
      auto read = createOp(
          at, node.location(b.getContext(), source.path),
          ac::SourceStateReadOp::getOperationName(), {*path},
          TypeRange{found->second.type},
          {at.getNamedAttr("state", at.getStringAttr(found->second.name)),
           at.getNamedAttr("selectors", at.getArrayAttr({})),
           at.getNamedAttr("occurrence", sourceOccurrence(at, owner, node))});
      if (isa<ac::BitsType>(found->second.type))
        fixedValues.insert(read->getResult(0));
      return std::optional<Value>(read->getResult(0));
    }
  }
  bool methodCall = false;
  if (node.kind() == "Call") {
    AstNode function = node.child("func");
    methodCall = function.kind() == "Name" && activeSourceEffects->methods &&
                 activeSourceEffects->methods->contains(function.string("id"));
    methodCall |= function.kind() == "Attribute" &&
                  function.child("value").kind() == "Name" &&
                  instances.contains(function.child("value").string("id"));
  }
  if (!methodCall)
    return std::optional<Value>();
  auto results = sourceMethodCall(node, at, owner, values, instances);
  if (failed(results))
    return failure();
  if (results->size() != 1)
    return error() << "Method expression requires exactly one result";
  return std::optional<Value>(results->front());
}
