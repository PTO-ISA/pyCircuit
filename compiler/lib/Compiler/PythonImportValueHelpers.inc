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
      if (kind == "Assign" || kind == "AnnAssign") {
        if (kind == "Assign" && site.array("targets").size() != 1)
          return diagnostic(site) << "pure helper assignment requires one target";
        auto lhs = kind == "Assign" ? site.item("targets", 0) : site.child("target");
        // Locals have Python lexical identity throughout the helper, and never
        // accidentally capture a same-spelled enclosing wire.
        StringRef root = assignmentRoot(lhs);
        if (root.empty() || lhs.kind() == "Subscript")
          return diagnostic(lhs) << "pure helper assignment requires a local Name or Struct field";
        if (lhs.kind() == "Name") locals.insert(root);
        else if (failed(self(self, lhs.child("value"), false, false))) return failure();
        if (failed(self(self, lhs, false, false, true))) return failure();
        return self(self, site.child("value"), false, false);
      }
      if (kind == "Expr" && site.child("value").kind() == "Call")
        return self(self, site.child("value"), false, false);
      return diagnostic(site) << "unsupported pure helper statement '" << kind << "'";
    }
    if (kind == "Name") {
      if (!target) freeNames.insert(site.string("id"));
      return success();
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
      // The callee's declaration is resolved by the typed call owner. It is
      // not a runtime lexical capture.
      for (StringRef field : {"args", "keywords"})
        for (size_t i = 0; i < site.array(field).size(); ++i)
          if (failed(self(self, site.item(field, i), false, false))) return failure();
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
  helper.state = 2;
  return ac::verifySourceValueHelper(helper.function, [&](FlatSymbolRefAttr symbol) {
    return dyn_cast_or_null<func::FuncOp>(SymbolTable::lookupSymbolIn(*body, symbol));
  });
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

FailureOr<TableQueryCharge> Importer::valueHelperCost(func::FuncOp root) {
  llvm::DenseMap<Operation *, TableQueryCharge> memo;
  llvm::DenseSet<Operation *> visiting;
  ac::HardwareAnalysis analysis(*body);
  auto add = [&](TableQueryCharge &sum, const TableQueryCharge &amount) {
    for (unsigned i = 0; i < amount.amounts.size(); ++i)
      if (!TableQueryBudget::add(sum, static_cast<TableQueryResource>(i), amount.amounts[i])) return false;
    return true;
  };
  auto visit = [&](auto &&self, func::FuncOp helper, unsigned depth) -> FailureOr<TableQueryCharge> {
    if (depth > TableQueryBudget::limit(TableQueryResource::Nesting))
      return error() << "pure helper call depth budget exhausted";
    auto cached = memo.find(helper.getOperation());
    if (cached != memo.end()) return cached->second;
    if (!visiting.insert(helper.getOperation()).second)
      return error() << "recursive pure helper closure is unsupported";
    auto leave = llvm::scope_exit([&] { visiting.erase(helper.getOperation()); });
    TableQueryCharge cost;
    bool fits = TableQueryBudget::add(cost, TableQueryResource::Occurrences, 1) &&
                TableQueryBudget::add(cost, TableQueryResource::Slots, helper.getNumArguments() * 4 + 8);
    auto chargeValue = [&](Value value) {
      auto words = measureTableStorageWords(value.getType(), analysis, *body, helper.getLoc());
      if (failed(words)) { fits = false; return; }
      fits &= TableQueryBudget::addProduct(cost, TableQueryResource::PayloadWords, *words, 3);
      fits &= TableQueryBudget::add(cost, TableQueryResource::Slots, 4);
    };
    for (Value argument : helper.getArguments()) chargeValue(argument);
    auto chargeAttribute = [&](auto &&attributeWalk, Attribute attribute, unsigned attributeDepth) -> void {
      if (!attribute || attributeDepth > TableQueryBudget::limit(TableQueryResource::Nesting)) { fits = false; return; }
      fits &= TableQueryBudget::add(cost, TableQueryResource::Work, 1) &&
              TableQueryBudget::add(cost, TableQueryResource::Slots, 1);
      if (auto text = dyn_cast<StringAttr>(attribute))
        fits &= TableQueryBudget::add(cost, TableQueryResource::ConstantBytes, text.getValue().size());
      else if (auto number = dyn_cast<ac::MathIntAttr>(attribute))
        fits &= TableQueryBudget::add(cost, TableQueryResource::ConstantBytes, number.getCanonicalValue().size());
      else if (auto expression = dyn_cast<ac::StaticExprAttr>(attribute)) attributeWalk(attributeWalk, expression.getTree(), attributeDepth + 1);
      else if (auto dictionary = dyn_cast<DictionaryAttr>(attribute))
        for (NamedAttribute field : dictionary) attributeWalk(attributeWalk, field.getValue(), attributeDepth + 1);
      else if (auto array = dyn_cast<ArrayAttr>(attribute))
        for (Attribute item : array) attributeWalk(attributeWalk, item, attributeDepth + 1);
    };
    helper.walk([&](Operation *operation) {
      if (operation == helper.getOperation() || isa<func::ReturnOp>(operation)) return;
      fits &= TableQueryBudget::add(cost, TableQueryResource::Work, 1) &&
              TableQueryBudget::add(cost, TableQueryResource::Operations, 1) &&
              TableQueryBudget::add(cost, TableQueryResource::Slots,
                   8 + operation->getNumOperands() + operation->getNumResults());
      for (Value value : operation->getResults()) chargeValue(value);
      for (Region &region : operation->getRegions())
        for (Block &block : region)
          for (Value argument : block.getArguments()) chargeValue(argument);
      chargeAttribute(chargeAttribute, operation->getAttrDictionary(), 0);
      if (auto call = dyn_cast<func::CallOp>(operation)) {
        auto callee = dyn_cast_or_null<func::FuncOp>(SymbolTable::lookupSymbolIn(*body, call.getCalleeAttr()));
        if (!callee) { fits = false; return; }
        auto child = self(self, callee, depth + 1);
        if (failed(child)) { fits = false; return; }
        fits &= add(cost, *child);
      }
    });
    if (!fits) return error() << "pure helper expanded DAG budget exhausted";
    for (unsigned i = 1; i < cost.amounts.size(); ++i)
      if (cost.amounts[i] > TableQueryBudget::limit(static_cast<TableQueryResource>(i)))
        return error() << "pure helper expanded DAG budget exhausted";
    memo[helper.getOperation()] = cost;
    return cost;
  };
  return visit(visit, root, 0);
}

LogicalResult Importer::reserveValueHelper(func::FuncOp helper, const AstNode &call) {
  auto cost = valueHelperCost(helper);
  if (failed(cost)) return failure();
  // Typing is shared once; charge the complete recursive clone before building
  // any executable IR. Multiplicities accumulate on the typed call DAG.
  TableQueryCharge reservation;
  bool fits = true;
  for (unsigned i = 1; i < cost->amounts.size(); ++i)
    fits &= TableQueryBudget::addProduct(reservation, static_cast<TableQueryResource>(i), cost->amounts[i], 2 + activeSourceExpansion.size());
  std::string reason;
  if (!fits || !queryBudget.reserve(reservation, reason))
    return mlir::emitError(call.location(b.getContext(), source.path))
           << (fits ? reason : "pure helper resource counter overflow");
  return success();
}

FailureOr<SmallVector<Value>> Importer::instantiateValueHelper(
    ValueHelperDecl &helper, ValueRange inputs, const AstNode &call,
    OpBuilder &at, FlatSymbolRefAttr symbol, bool forceInline) {
  if (activeValueHelper && !forceInline) {
    auto op = at.create<func::CallOp>(call.location(b.getContext(), source.path),
                                    helper.symbol, helper.function.getResultTypes(), inputs);
    op->setAttr("ac.origin", sourceOccurrence(at, symbol, call));
    for (auto [result, port] : llvm::zip(op.getResults(), helper.results)) {
      remember(result, port.sourceKind);
      if (port.fixedBits) fixedValues.insert(result);
    }
    return SmallVector<Value>(op.getResults());
  }
  if (failed(ac::verifySourceValueHelper(helper.function, [&](FlatSymbolRefAttr callee) {
        return dyn_cast_or_null<func::FuncOp>(SymbolTable::lookupSymbolIn(*body, callee));
      })) || failed(reserveValueHelper(helper.function, call))) return failure();
  size_t frameDepth = activeSourceExpansion.size();
  if (failed(pushHelperFrame(call, helper.node, symbol, helper.symbol))) return failure();
  auto restoreFrame = llvm::scope_exit([&] { activeSourceExpansion.resize(frameDepth); });
  auto copyFacts = [&](Value original, Value copied) {
    auto info = valueInfo(original); info.value = copied;
    numericValues[copied] = info;
    if (fixedValues.contains(original)) fixedValues.insert(copied);
    if (arithmeticValues.contains(original)) arithmeticValues.insert(copied);
  };
  auto expandAttribute = [&](auto &&self, Attribute attribute) -> Attribute {
    if (auto expression = dyn_cast<ac::StaticExprAttr>(attribute))
      return ac::StaticExprAttr::get(at.getContext(), cast<DictionaryAttr>(self(self, expression.getTree())));
    if (auto dictionary = dyn_cast<DictionaryAttr>(attribute)) {
      if (dictionary.getAs<DictionaryAttr>("site") && dictionary.getAs<ArrayAttr>("expansion")) {
        SmallVector<Attribute> frames;
        llvm::append_range(frames, dictionary.getAs<ArrayAttr>("expansion"));
        llvm::append_range(frames, activeSourceExpansion);
        NamedAttrList fields(dictionary); fields.set("expansion", at.getArrayAttr(frames));
        return fields.getDictionary(at.getContext());
      }
      NamedAttrList fields;
      for (NamedAttribute field : dictionary) fields.append(field.getName(), self(self, field.getValue()));
      return fields.getDictionary(at.getContext());
    }
    if (auto array = dyn_cast<ArrayAttr>(attribute)) {
      SmallVector<Attribute> items;
      for (Attribute item : array) items.push_back(self(self, item));
      return at.getArrayAttr(items);
    }
    return attribute;
  };
  auto inlineBody = [&](auto &&self, func::FuncOp function, ValueRange actuals,
                        OpBuilder &target) -> FailureOr<SmallVector<Value>> {
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
          size_t expansionDepth = activeSourceExpansion.size();
          auto origin = nestedCall->getAttrOfType<DictionaryAttr>("ac.origin");
          if (origin)
            activeSourceExpansion.push_back(builder.getDictionaryAttr({
                builder.getNamedAttr("kind", builder.getStringAttr("call")),
                builder.getNamedAttr("site", origin.getAs<DictionaryAttr>("site")),
                builder.getNamedAttr("callee", nestedCall.getCalleeAttr())}));
          auto restoreExpansion = llvm::scope_exit([&] { activeSourceExpansion.resize(expansionDepth); });
          auto values = self(self, callee, bound, builder);
          if (failed(values)) return failure();
          for (auto [result, value] : llvm::zip(nestedCall.getResults(), *values)) map.map(result, value);
          continue;
        }
        auto *copy = builder.cloneWithoutRegions(operation, map);
        if (activeLoop) activeLoop->helperOwned.insert(copy);
        for (auto [original, result] : llvm::zip(operation.getResults(), copy->getResults())) copyFacts(original, result);
        copy->setAttrs(cast<DictionaryAttr>(expandAttribute(expandAttribute, copy->getAttrDictionary())));
        for (auto [sourceRegion, copiedRegion] : llvm::zip(operation.getRegions(), copy->getRegions()))
          for (Block &regionBlock : sourceRegion) {
            auto *newBlock = new Block(); copiedRegion.push_back(newBlock);
            IRMapping localMap(map);
            for (Value argument : regionBlock.getArguments()) {
              auto result = newBlock->addArgument(argument.getType(), argument.getLoc());
              localMap.map(argument, result); copyFacts(argument, result);
            }
            auto inside = OpBuilder::atBlockEnd(newBlock);
            if (failed(expand(expand, regionBlock, *newBlock, localMap, inside))) return failure();
          }
      }
      return success();
    };
    if (failed(expandBlock(expandBlock, function.getBody().front(), *target.getBlock(), mapping, target))) return failure();
    auto returned = cast<func::ReturnOp>(function.getBody().front().getTerminator());
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
