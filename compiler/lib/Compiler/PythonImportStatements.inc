// Native ownership, control-flow and transaction construction.
FailureOr<Value> Importer::allocate(Type data, StringRef name,
                                    const AstNode &node, Scope &scope,
                                    AstNode initial, unsigned depth) {
  if (!scope.module)
    return (void)fail(node,
                      "persistent resources may only be declared in a Module"),
           failure();
  SmallVector<Value> operands;
  if (initial) {
    Scope pure = scope;
    pure.pure = true;
    auto value = scalar(initial, pure, data);
    if (failed(value))
      return failure();
    operands.push_back(*value);
  }
  auto type = resource(data, depth ? "queue" : "state", depth);
  auto *allocation =
      op(node, "ac.resource.alloc", operands, type,
         {b.getNamedAttr("instance_name", b.getStringAttr(name))});
  return allocation ? FailureOr<Value>(allocation->getResult(0))
                    : FailureOr<Value>(failure());
}

FailureOr<SmallVector<Value>>
Importer::instantiate(Declaration *decl, Operation *imported,
                      StringRef instanceName, const AstNode &call, Scope &scope,
                      ArrayRef<Value> destinations) {
  if (!scope.module || !destinations.empty())
    return (void)fail(call ? call : scope.owner->node,
                     "Module instances require direct Module wiring"), failure();
  if (decl && failed(lower(*decl)))
    return failure();
  auto *definition = decl ? decl->operation : imported;
  auto type = cast<FunctionType>(definition->getAttrOfType<TypeAttr>("function_type").getValue());
  SmallVector<StringRef> formals;
  for (auto attr : definition->getAttrOfType<ArrayAttr>("input_names"))
    formals.push_back(cast<StringAttr>(attr).getValue());
  SmallVector<AstNode> actuals;
  if (call && call.kind() == "Call") {
    auto normalized = detail::normalizeCallArguments(call, formals, error);
    if (failed(normalized)) return failure();
    actuals = *normalized;
  } else if (!formals.empty())
    return (void)fail(scope.owner->node, "instance requires explicit input bindings"), failure();
  SmallVector<Value> operands;
  for (auto actual : actuals) {
    auto values = expression(actual, scope);
    if (failed(values) || values->size() != 1 ||
        !isa<ac::ModuleRefType>(values->front().getType()))
      return (void)fail(actual, "Module reference requires an existing instance"), failure();
    operands.push_back(values->front());
  }
  auto at = call ? call : scope.owner->node;
  auto *instance = op(at, "ac.instance", operands, type.getResults(),
     {b.getNamedAttr("instance_name", b.getStringAttr(instanceName)),
      b.getNamedAttr("callee", symbol(definition->getAttrOfType<StringAttr>("sym_name").getValue())),
      b.getNamedAttr("parameters", b.getArrayAttr({})),
      b.getNamedAttr("type_arguments", b.getArrayAttr({})),
      b.getNamedAttr("occurrence", origin(at, scope.owner->symbol))});
  return SmallVector<Value>(instance->getResults());
}

LogicalResult Importer::emitGuard(Callable &call, Scope &scope) {
  if (call.guard) {
    auto value = scalar(call.guard, scope, bits(1, call.guard));
    if (failed(value))
      return failure();
    yield(call.guard, *value);
  } else
    yield(call.node, truth(call.node, true));
  return success();
}

LogicalResult Importer::defineRule(Callable &call, Scope &scope) {
  if (call.defined)
    return success();
  if (!scope.module || !call.inputs.empty() || !call.results.empty())
    return fail(call.node, "transactional compilation currently requires local "
                           "Rules without parameters or results");
  Scope captured = scope;
  captured.module = false;
  captured.callable = &call;
  captured.resultTypes.clear();
  captured.outputs.clear();
  captured.returned.clear();
  captured.terminated = false;
  SmallVector<Value> captures;
  for (auto &[name, binding] : captured.bindings)
    if (binding.value && !llvm::is_contained(captures, binding.value))
      captures.push_back(binding.value);
  auto *definition =
      op(call.node, "ac.rule", captures, {},
         {b.getNamedAttr("sym_name", b.getStringAttr(call.name)),
          b.getNamedAttr("function_type",
                         TypeAttr::get(b.getFunctionType({}, {})))},
         2);
  for (unsigned region = 0; region < 2; ++region) {
    auto *block = new Block;
    definition->getRegion(region).push_back(block);
    Scope inner = captured;
    inner.pure = region == 0;
    DenseMap<Value, Value> mapped;
    for (auto value : captures)
      mapped[value] = block->addArgument(value.getType(), definition->getLoc());
    for (auto &[name, binding] : inner.bindings)
      if (binding.value)
        binding.value = mapped.lookup(binding.value);
    OpBuilder::InsertionGuard save(b);
    b.setInsertionPointToEnd(block);
    if (region == 0) {
      if (failed(emitGuard(call, inner)))
        return failure();
    } else {
      if (failed(inferHints(call.node, inner)) ||
          failed(statements(call.node, "body", inner)))
        return failure();
      if (!inner.terminated)
        yield(call.node);
    }
  }
  call.defined = true;
  return success();
}

FailureOr<SmallVector<Value>>
Importer::registerRule(Callable &call, const AstNode &registration,
                       Scope &scope, ArrayRef<Value> destinations) {
  if (!scope.module || !destinations.empty() || !call.inputs.empty() ||
      !call.results.empty()) {
    (void)fail(registration, "transactional compilation currently requires "
                             "local Rules without parameters or results");
    return failure();
  }
  if (!call.defined) {
    (void)fail(registration, "Rule must be declared before registration");
    return failure();
  }
  if (call.registrations++) {
    (void)fail(registration, "Rule is already registered in this Module");
    return failure();
  }
  op(registration, "ac.rule.call", {}, {},
     {b.getNamedAttr("callee", symbol(call.name)),
      b.getNamedAttr("input_count", b.getI64IntegerAttr(0)),
      b.getNamedAttr("occurrence", origin(registration, scope.owner->symbol))});
  return SmallVector<Value>{};
}

LogicalResult Importer::emitMethod(Callable &call, Scope &scope) {
  SmallVector<Value> captures;
  Scope captured = scope;
  captured.module = false;
  captured.callable = &call;
  captured.resultTypes = call.results;
  captured.outputs.clear();
  captured.returned.clear();
  captured.terminated = false;
  for (auto &[name, value] : captured.bindings)
    if (value.value)
      captures.push_back(value.value);
  SmallVector<Type> types;
  for (auto &input : call.inputs) {
    types.push_back(input.type);
  }
  auto *method =
      op(call.node, "ac.method", captures, {},
         {b.getNamedAttr("sym_name", b.getStringAttr(call.name)),
          b.getNamedAttr("function_type",
                         TypeAttr::get(b.getFunctionType(types, call.results)))},
         2);
  for (unsigned region = 0; region < 2; ++region) {
    auto *block = new Block;
    method->getRegion(region).push_back(block);
    Scope inner = captured;
    inner.pure = region == 0;
    for (auto &input : call.inputs)
      inner.bindings[input.name] = Binding{
          block->addArgument(input.type, method->getLoc()), {}, {}, nullptr};
    for (auto &[name, value] : captured.bindings)
      if (value.value) {
        auto argument =
            block->addArgument(value.value.getType(), method->getLoc());
        if (!llvm::any_of(call.inputs, [&](const NamedType &input) {
              return input.name == name;
            }))
          inner.bindings[name].value = argument;
      }
    OpBuilder::InsertionGuard save(b);
    b.setInsertionPointToEnd(block);
    if (region == 0) {
      if (failed(emitGuard(call, inner)))
        return failure();
    } else {
      if (failed(inferHints(call.node, inner)) ||
          failed(statements(call.node, "body", inner)))
        return failure();
      if (!inner.terminated) {
        if (!call.results.empty())
          return fail(call.node,
                      "Method must return every declared SSA result");
        yield(call.node);
      }
    }
  }
  return success();
}

LogicalResult Importer::bind(const AstNode &target, ArrayRef<Value> values,
                             Scope &scope) {
  if (target.kind() == "Tuple") {
    if (target.array("elts").size() != values.size())
      return fail(target, "independent output unpacking arity mismatch");
    for (size_t i = 0; i < values.size(); ++i)
      if (failed(bind(target.item("elts", i), values.slice(i, 1), scope)))
        return failure();
    return success();
  }
  if (target.kind() != "Name" || values.size() != 1)
    return fail(
        target,
        "assignment requires one named local or direct tuple unpacking");
  std::string name = target.string("id").str();
  auto found = scope.bindings.find(name);
  if (names.count(name) || globalRules.count(name) ||
      scope.owner->callables.count(name))
    return fail(target,
                "assignment cannot shadow imported or callable source names");
  if (found != scope.bindings.end() &&
      isa_and_nonnull<ac::StateType>(found->second.type())) {
    if (scope.module) {
      if (values.front() == found->second.value)
        return success();
      return fail(target, "ordinary Module assignment cannot replace an "
                          "existing resource identity");
    }
    auto value = values.front();
    if (isa<ac::StateType>(value.getType())) {
      auto loaded = read(value, target, scope);
      if (failed(loaded))
        return failure();
      value = *loaded;
    }
    return write(found->second.value, value, target, scope);
  }
  if (found != scope.bindings.end() &&
      (found->second.pending || found->second.pendingImport ||
       isa_and_nonnull<ac::ModuleRefType>(found->second.type())))
    return fail(target, "instance identity cannot be reassigned");
  Value value = values.front();
  if (!scope.module && isa<ac::StateType>(value.getType())) {
    auto loaded = read(value, target, scope);
    if (failed(loaded))
      return failure();
    value = *loaded;
  }
  scope.bindings[name] = Binding{value, {}, {}, nullptr};
  return success();
}

LogicalResult Importer::statements(const AstNode &node, StringRef field,
                                   Scope &scope) {
  for (size_t i = 0; i < node.array(field).size(); ++i) {
    auto child = node.item(field, i);
    if (scope.terminated)
      return fail(child, "statement after unconditional return is unreachable");
    if (failed(statement(child, scope)))
      return failure();
  }
  return success();
}

LogicalResult Importer::statement(const AstNode &node, Scope &scope) {
  if (!charge(node))
    return failure();
  if (node.kind() == "Pass")
    return success();
  if (node.kind() == "FunctionDef") {
    if (!scope.module || scope.conditionalDepth)
      return fail(node,
                  "nested callables must be declared directly in their Module");
    auto found = scope.owner->callables.find(node.string("name").str());
    if (found == scope.owner->callables.end())
      return fail(node, "unknown nested callable declaration");
    if (found->second.kind == "method")
      return emitMethod(found->second, scope);
    if (failed(defineRule(found->second, scope)))
      return failure();
    if (found->second.autonomous &&
        failed(registerRule(found->second, node, scope)))
      return failure();
    return success();
  }
  if (node.kind() == "AnnAssign") {
    auto target = node.child("target");
    if (target.kind() != "Name")
      return fail(target, "declaration requires one named binding");
    std::string name = target.string("id").str();
    if (scope.bindings.count(name))
      return fail(target, "duplicate declared local");
    if (names.count(name) || globalRules.count(name) ||
        scope.owner->callables.count(name))
      return fail(
          target,
          "local declaration cannot shadow imported or callable source names");
    auto typeNode = node.child("annotation");
    if (scope.module) {
      if (scope.conditionalDepth)
        return fail(node, "resources must be declared directly in their Module");
      if (auto nominal = resolve(typeNode))
        if (nominal->symbol &&
            (declarations.count(nominal->remote) ||
             isa_and_nonnull<ac::ModuleImportOp>(nominal->declaration)))
          {
            if (node.child("value"))
              return fail(node, "declare an instance then connect it at its wiring site");
            Binding binding;
            binding.instanceName = name;
            binding.declaration = node;
            binding.instanceTarget = nominal->symbol.getValue().str();
            if (declarations.count(nominal->remote))
              binding.pending = declarations.at(nominal->remote).get();
            else
              binding.pendingImport = nominal->declaration;
            scope.bindings[name] = binding;
            bool closed = binding.pending ? binding.pending->inputs.empty()
                : cast<FunctionType>(binding.pendingImport->getAttrOfType<TypeAttr>(
                      "function_type").getValue()).getNumInputs() == 0;
            if (closed && failed(expression(target, scope)))
              return failure();
            return success();
          }
      if (typeNode.kind() == "Subscript" &&
          marker(typeNode.child("value"), "queue"))
        return fail(node,
                    "Queue allocation awaits transactional resource binding");
      Type type = annotation(typeNode, scope.owner->symbol);
      AstNode initial = node.child("value");
      if (!type)
        return failure();
      if (!isa<ac::BitsType>(type) || !initial)
        return fail(node,
                    "State currently requires an initialized Bits variable");
      auto value = allocate(type, name, node, scope, initial);
      if (failed(value))
        return failure();
      scope.bindings[name] = Binding{*value, {}, {}, nullptr};
      return success();
    }
    auto type = annotation(typeNode, scope.owner->symbol);
    if (!type)
      return failure();
    if (!node.child("value"))
      return fail(node, "Rule/Method locals require values, not persistent "
                        "storage declarations");
    auto value = scalar(node.child("value"), scope, type);
    if (failed(value))
      return failure();
    scope.bindings[name] = Binding{*value, {}, {}, nullptr};
    scope.hints[name] = type;
    return success();
  }
  if (node.kind() == "Assign") {
    if (node.array("targets").size() != 1)
      return fail(node, "chained assignment is not supported");
    auto target = node.item("targets", 0);
    Type expected;
    if (target.kind() == "Name") {
      auto name = target.string("id").str();
      auto binding = scope.bindings.find(name);
      if (binding != scope.bindings.end())
        expected = payload(binding->second.type());
      if (!expected) {
        auto hint = scope.hints.find(name);
        if (hint != scope.hints.end())
          expected = hint->second;
      }
    }
    SmallVector<Value> destinations;
    if (scope.module && node.child("value").kind() == "Call") {
      auto addDestination = [&](const AstNode &leaf) -> LogicalResult {
        if (leaf.kind() != "Name")
          return fail(
              leaf, "output bindings require names or a direct tuple of names");
        auto found = scope.bindings.find(leaf.string("id").str());
        destinations.push_back(
            found != scope.bindings.end() &&
                    isa_and_nonnull<ac::StateType>(found->second.type())
                ? found->second.value
                : Value());
        return success();
      };
      if (target.kind() == "Tuple") {
        for (size_t i = 0; i < target.array("elts").size(); ++i)
          if (failed(addDestination(target.item("elts", i))))
            return failure();
      } else if (failed(addDestination(target)))
        return failure();
    }
    auto values =
        expression(node.child("value"), scope, expected, destinations);
    if (failed(values))
      return failure();
    return bind(target, *values, scope);
  }
  if (node.kind() == "AugAssign") {
    auto target = node.child("target");
    if (target.kind() != "Name")
      return fail(target,
                  "compound assignment requires a named resource or local");
    auto lhs = scalar(target, scope);
    if (failed(lhs))
      return failure();
    auto rhs = scalar(node.child("value"), scope, lhs->getType());
    if (failed(rhs))
      return failure();
    StringRef kind = node.child("op").kind();
    StringRef opcode = kind == "Add"      ? "add"
                       : kind == "Sub"    ? "sub"
                       : kind == "BitAnd" ? "and"
                       : kind == "BitOr"  ? "or"
                       : kind == "BitXor" ? "xor"
                                          : "";
    if (opcode.empty())
      return fail(node, "unsupported compound assignment");
    Value value =
        op(node, "ac.bits.binary", ValueRange{*lhs, *rhs}, lhs->getType(),
           {b.getNamedAttr("opcode", b.getStringAttr(opcode))})
            ->getResult(0);
    return bind(target, ArrayRef<Value>(value), scope);
  }
  if (node.kind() == "Return") {
    if (scope.conditionalDepth)
      return fail(node, "Module branch returns await result merging");
    auto expressionNode = node.child("value");
    SmallVector<Value> values;
    if (expressionNode &&
        !(expressionNode.kind() == "Constant" &&
          isa_and_nonnull<UnitAttr>(expressionNode.get("value")))) {
      SmallVector<AstNode> actuals;
      if (expressionNode.kind() == "Tuple")
        for (size_t i = 0; i < expressionNode.array("elts").size(); ++i)
          actuals.push_back(expressionNode.item("elts", i));
      else
        actuals.push_back(expressionNode);
      if (actuals.size() != scope.resultTypes.size())
        return fail(node, "return arity differs from declared result types");
      for (size_t i = 0; i < actuals.size(); ++i) {
        if (scope.module) {
          auto value = expression(actuals[i], scope);
          if (failed(value) || value->size() != 1 ||
              !isa<ac::StateType>(value->front().getType()) ||
              !equal(payload(value->front().getType()), scope.resultTypes[i]))
            return fail(actuals[i],
                        "Module returns must export existing resource "
                        "identities with declared payloads");
          values.push_back(value->front());
        } else {
          auto value = scalar(actuals[i], scope, scope.resultTypes[i]);
          if (failed(value))
            return failure();
          values.push_back(*value);
        }
      }
    } else if (!scope.resultTypes.empty())
      return fail(node, "missing declared return payload");
    if (scope.callable && scope.callable->kind != "method") {
      for (size_t i = 0; i < values.size(); ++i)
        if (failed(write(scope.outputs[i], values[i], node, scope)))
          return failure();
      yield(node);
    } else
      yield(node, values);
    scope.returned = values;
    scope.terminated = true;
    return success();
  }
  if (node.kind() == "Assert") {
    if (scope.module || scope.pure)
      return fail(node,
                  "assertions execute only within admitted Rule/Method bodies");
    auto value = scalar(node.child("test"), scope, bits(1, node));
    if (failed(value))
      return failure();
    if (node.child("msg") &&
        !isa_and_nonnull<StringAttr>(node.child("msg").get("value")))
      return fail(node, "assertion message must be a literal string");
    op(node, "ac.expect", ValueRange{*value, truth(node, true)}, {},
       {b.getNamedAttr("kind", b.getStringAttr("assert")),
        b.getNamedAttr("occurrence", origin(node, scope.owner->symbol)),
        b.getNamedAttr("ac.message", node.child("msg")
                                         ? node.child("msg").get("value")
                                         : b.getStringAttr("")),
        b.getNamedAttr("location", detail::sourceSpan(b, source.path, node))});
    return success();
  }
  if (node.kind() == "Expr") {
    auto expr = node.child("value");
    if (expr.kind() == "Constant" &&
        isa_and_nonnull<StringAttr>(expr.get("value")))
      return success();
    if (expr.kind() == "Call" && marker(expr.child("func"), "log")) {
      if (scope.module || scope.pure)
        return fail(node,
                    "logs execute only within admitted Rule/Method bodies");
      SmallVector<Value> values{truth(node, true)};
      SmallVector<Attribute> items;
      if (!expr.array("keywords").empty())
        return fail(expr, "log does not accept keyword arguments");
      if (expr.array("args").size() < 2)
        return fail(expr, "log requires level, event and optional items");
      auto level =
          dyn_cast_or_null<StringAttr>(expr.item("args", 0).get("value"));
      auto event =
          dyn_cast_or_null<StringAttr>(expr.item("args", 1).get("value"));
      if (!level || !event || event.getValue().empty() ||
          !llvm::is_contained(
              ArrayRef<StringRef>{"debug", "info", "warning", "error"},
              level.getValue()))
        return fail(expr, "log requires a static level and event");
      for (size_t i = 2; i < expr.array("args").size(); ++i) {
        auto arg = expr.item("args", i);
        if (auto text = dyn_cast_or_null<StringAttr>(arg.get("value"))) {
          items.push_back(b.getDictionaryAttr(
              {b.getNamedAttr("kind", b.getStringAttr("literal")),
               b.getNamedAttr("text", text)}));
        } else {
          auto value = scalar(arg, scope);
          if (failed(value))
            return failure();
          items.push_back(b.getDictionaryAttr(
              {b.getNamedAttr("kind", b.getStringAttr("value")),
               b.getNamedAttr("ordinal",
                              b.getI64IntegerAttr(values.size() - 1))}));
          values.push_back(*value);
        }
      }
      op(node, "ac.observe", values, {},
         {b.getNamedAttr("kind", b.getStringAttr("log")),
          b.getNamedAttr("spec",
                         b.getDictionaryAttr(
                             {b.getNamedAttr("level", level),
                              b.getNamedAttr("event", event),
                              b.getNamedAttr("items", b.getArrayAttr(items))})),
          b.getNamedAttr("occurrence", origin(node, scope.owner->symbol)),
          b.getNamedAttr("ac.location",
                         detail::sourceSpan(b, source.path, node))});
      return success();
    }
    auto values = expression(expr, scope);
    if (failed(values))
      return failure();
    return success();
  }
  if (node.kind() == "If") {
    if (!scope.module)
      return fail(node, "Rule body control flow awaits transactional migration");
    auto condition = scalar(node.child("test"), scope, bits(1, node));
    if (failed(condition))
      return failure();
    auto selected = op(node, "ac.source.predicate", *condition, b.getI1Type())
                        ->getResult(0);
    auto *branch = op(node, "scf.if", selected, {}, {}, 2);
    for (unsigned arm = 0; arm < 2; ++arm) {
      auto *block = new Block;
      branch->getRegion(arm).push_back(block);
      OpBuilder::InsertionGuard save(b);
      b.setInsertionPointToEnd(block);
      Scope inner = scope;
      ++inner.conditionalDepth;
      if (failed(statements(node, arm == 0 ? "body" : "orelse", inner)))
        return failure();
      // Results are deliberately absent in this stage. Do not silently retain
      // the old value of a Python local assigned only along one branch.
      for (const auto &[name, binding] : scope.bindings)
        if (inner.bindings.at(name).value != binding.value)
          return fail(node, "cross-branch local assignments await result merging");
      op(node, "scf.yield", {}, {});
    }
    return success();
  }
  return fail(node, "unsupported statement in transactional native frontend");
}

LogicalResult Importer::lower(Declaration &decl) {
  if (decl.phase == 2)
    return success();
  if (decl.phase == 1)
    return fail(decl.node, "recursive Module instantiation is not supported");
  decl.phase = 1;
  OpBuilder::InsertionGuard save(b);
  auto previousDefinition = currentDefinition;
  currentDefinition = decl.symbol;
  auto restore =
      llvm::scope_exit([&] { currentDefinition = previousDefinition; });
  b.setInsertionPointToEnd(result->getBody());
  SmallVector<Type> inputs, outputs;
  SmallVector<Attribute> inputNames, outputNames;
  for (auto &input : decl.inputs) {
    inputs.push_back(input.type);
    inputNames.push_back(b.getStringAttr(input.name));
  }
  for (size_t i = 0; i < decl.results.size(); ++i) {
    outputs.push_back(resource(decl.results[i], "state"));
    outputNames.push_back(b.getStringAttr("out" + std::to_string(i)));
  }
  decl.operation =
      op(decl.node, "ac.module", {}, {},
         {b.getNamedAttr("sym_name", b.getStringAttr(decl.symbol)),
          b.getNamedAttr("source_owner", owner),
          b.getNamedAttr("parameters", b.getArrayAttr({})),
          b.getNamedAttr("type_parameters", b.getArrayAttr({})),
          b.getNamedAttr("function_type",
                         TypeAttr::get(b.getFunctionType(inputs, outputs))),
          b.getNamedAttr("input_names", b.getArrayAttr(inputNames)),
          b.getNamedAttr("output_names", b.getArrayAttr(outputNames)),
          b.getNamedAttr("output_kinds",
                         b.getArrayAttr(SmallVector<Attribute>(
                             outputs.size(), b.getStringAttr("export")))),
          b.getNamedAttr("ac.declaration_role", b.getStringAttr("definition"))},
         1);
  names.at(decl.name).declaration = decl.operation;
  auto *block = new Block;
  decl.operation->getRegion(0).push_back(block);
  b.setInsertionPointToEnd(block);
  Scope scope;
  scope.owner = &decl;
  scope.module = true;
  scope.pure = true;
  scope.resultTypes = decl.results;
  if (llvm::any_of(decl.callables, [](const auto &entry) {
        return entry.second.kind == "method";
      })) {
    auto type = ac::ModuleRefType::get(b.getContext(), symbol(decl.symbol));
    auto *self = op(decl.node, "ac.instance.ref", {}, type,
                    {b.getNamedAttr("instance_path", b.getArrayAttr({}))});
    scope.bindings["$self"].value = self->getResult(0);
  }
  for (auto &input : decl.inputs)
    scope.bindings[input.name] =
        Binding{block->addArgument(input.type, decl.operation->getLoc()),
                {},
                {},
                nullptr};
  if (failed(inferHints(decl.node, scope)) ||
      failed(statements(decl.node, "body", scope)))
    return failure();
  // Explicit zero-input Module declarations allocate even when their reference
  // is never called, preserving static hierarchy and autonomous Rules.
  for (auto &[name, binding] : scope.bindings) {
    if (binding.pendingImport) {
      if (scope.terminated)
        return fail(decl.node, "connect instances before returning from Module");
      auto values = instantiate(nullptr, binding.pendingImport, name, binding.declaration, scope);
      if (failed(values)) return failure();
      binding.pendingImport = nullptr;
    }
    if (binding.pending) {
      if (scope.terminated)
        return fail(decl.node, "unconnected explicit instance must be "
                               "connected before Module return");
      if (!binding.pending->inputs.empty())
        return fail(decl.node, "explicit Module instance has unbound inputs");
      auto values = instantiate(binding.pending, nullptr, name, binding.declaration, scope);
      if (failed(values))
        return failure();
      binding.pending = nullptr;
    }
  }
  if (!scope.terminated) {
    if (!decl.results.empty())
      return fail(decl.node,
                  "Module must export every declared resource result");
    yield(decl.node);
  }
  outputs.clear();
  for (auto value : scope.returned)
    outputs.push_back(value.getType());
  decl.operation->setAttr("function_type",
                          TypeAttr::get(b.getFunctionType(inputs, outputs)));
  decl.phase = 2;
  if (decl.system)
    decl.operation->setAttr("ac.root_kind", b.getStringAttr("system"));
  return success();
}
