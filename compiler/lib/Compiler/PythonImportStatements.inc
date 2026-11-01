// Native ownership, control-flow and transaction construction.
FailureOr<Value> Importer::allocate(Type data, StringRef name,
                                    const AstNode &node, Scope &scope,
                                    AstNode initial, Attribute depth) {
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
  auto type = resource(data, depth ? "queue" : "state");
  SmallVector<NamedAttribute> attrs{
      b.getNamedAttr("instance_name", b.getStringAttr(name))};
  if (depth)
    attrs.push_back(b.getNamedAttr("depth", depth));
  auto *allocation = op(node, "ac.resource.alloc", operands, type, attrs);
  return allocation ? FailureOr<Value>(allocation->getResult(0))
                    : FailureOr<Value>(failure());
}

FailureOr<SmallVector<Value>>
Importer::instantiate(Declaration *decl, Operation *imported,
                      StringRef instanceName, const AstNode &call, Scope &scope,
                      ArrayRef<Value> destinations) {
  if (!scope.module || scope.conditionalDepth)
    return (void)fail(call ? call : scope.owner->node,
                      "Module instances require direct Module wiring"),
           failure();
  if (decl && failed(lower(*decl)))
    return failure();
  auto *definition = decl ? decl->operation : imported;
  auto type = cast<FunctionType>(
      definition->getAttrOfType<TypeAttr>("function_type").getValue());
  SmallVector<StringRef> formals;
  for (auto attr : definition->getAttrOfType<ArrayAttr>("input_names"))
    formals.push_back(cast<StringAttr>(attr).getValue());
  auto parameters = definition->getAttrOfType<ArrayAttr>("parameters");
  SmallVector<AstNode> actuals(formals.size());
  SmallVector<Attribute> staticActuals(parameters.size());
  if (call && call.kind() == "Call") {
    if (call.array("args").size() > formals.size())
      return (void)fail(call, "too many Module input bindings"), failure();
    for (size_t i = 0; i < call.array("args").size(); ++i)
      actuals[i] = call.item("args", i);
    for (size_t i = 0; i < call.array("keywords").size(); ++i) {
      auto keyword = call.item("keywords", i);
      auto name = keyword.string("arg");
      bool matched = false;
      for (size_t j = 0; j < formals.size(); ++j) {
        if (formals[j] != name)
          continue;
        if (actuals[j])
          return (void)fail(keyword, "repeated Module input"), failure();
        actuals[j] = keyword.child("value");
        matched = true;
      }
      for (size_t j = 0; j < parameters.size(); ++j) {
        if (cast<DictionaryAttr>(parameters[j])
                .getAs<StringAttr>("name")
                .getValue() != name)
          continue;
        if (staticActuals[j])
          return (void)fail(keyword, "repeated static parameter"), failure();
        auto value = staticExpr(keyword.child("value"), scope.owner->symbol);
        if (failed(value))
          return failure();
        staticActuals[j] = *value;
        matched = true;
      }
      if (!matched)
        return (void)fail(keyword, "unknown Module argument"), failure();
    }
  }
  for (auto actual : actuals)
    if (!actual)
      return (void)fail(call ? call : scope.owner->node,
                        "instance requires explicit input bindings"),
             failure();
  ac::HardwareAnalysis hardware(*result);
  ac::HardwareBindings bindings;
  bindings.owner = definition;
  for (size_t i = 0; i < parameters.size(); ++i) {
    auto parameter = cast<DictionaryAttr>(parameters[i]);
    if (!staticActuals[i]) {
      auto value = parameter.getAs<ac::StaticExprAttr>("default");
      if (!value)
        return (void)fail(call ? call : scope.owner->node,
                          "missing required static parameter"),
               failure();
      staticActuals[i] = hardware.substituteAttribute(value, bindings);
    }
    bindings.integers[parameter.getAs<StringAttr>("name").getValue()] =
        staticActuals[i];
  }
  bool invalidType = false;
  AttrTypeReplacer canonicalize;
  canonicalize.addReplacement([&](Type payload) -> std::optional<Type> {
    if (!isa<ac::BitsType, ac::ArrayType>(payload))
      return std::nullopt;
    auto resolved = hardware.resolveType(payload, {}, definition);
    if (failed(resolved)) {
      invalidType = true;
      return payload;
    }
    return *resolved;
  });
  type = cast<FunctionType>(
      canonicalize.replace(hardware.substituteType(type, bindings)));
  if (invalidType)
    return failure();
  SmallVector<Value> operands;
  for (auto actual : actuals) {
    auto values = expression(actual, scope);
    if (failed(values) || values->size() != 1 ||
        !(isa<ac::ModuleRefType>(values->front().getType()) ||
          isResource(values->front().getType())))
      return (void)fail(actual,
                        "Module reference requires an existing instance"),
             failure();
    operands.push_back(values->front());
  }
  auto at = call ? call : scope.owner->node;
  auto kinds = definition->getAttrOfType<ArrayAttr>("output_kinds");
  SmallVector<Value> boundOutputs(type.getNumResults());
  for (unsigned i = 0; i < type.getNumResults(); ++i) {
    if (cast<StringAttr>(kinds[i]).getValue() != "bound")
      continue;
    Value destination = i < destinations.size() ? destinations[i] : Value();
    if (!destination) {
      auto *connection =
          op(at, "ac.resource.connection", {},
             resource(payload(type.getResult(i))),
             {b.getNamedAttr("instance_name",
                             b.getStringAttr(instanceName.str() + ".out" +
                                             std::to_string(i)))});
      if (!connection)
        return failure();
      destination = connection->getResult(0);
    }
    if (!isResource(destination.getType()) ||
        !equal(payload(destination.getType()), payload(type.getResult(i))))
      return (void)fail(at, "output binding requires a matching resource"),
             failure();
    operands.push_back(destination);
    boundOutputs[i] = destination;
  }
  auto *instance =
      op(at, "ac.instance", operands, type.getResults(),
         {b.getNamedAttr("instance_name", b.getStringAttr(instanceName)),
          b.getNamedAttr(
              "callee", symbol(definition->getAttrOfType<StringAttr>("sym_name")
                                   .getValue())),
          b.getNamedAttr("parameters", b.getArrayAttr(staticActuals)),
          b.getNamedAttr("type_arguments", b.getArrayAttr({})),
          b.getNamedAttr("occurrence", origin(at, scope.owner->symbol))});
  SmallVector<Value> results(instance->getResults());
  for (unsigned i = 0; i < results.size(); ++i)
    if (boundOutputs[i])
      results[i] = boundOutputs[i];
  // A void call still denotes its allocation. This reference is not a data
  // result or a physical port; aliases use it to address the same instance.
  if (results.empty()) {
    auto target = definition->getAttrOfType<StringAttr>("sym_name").getValue();
    auto type = ac::ModuleRefType::get(b.getContext(), symbol(target));
    results.push_back(
        op(at, "ac.instance.ref", {}, type,
           {b.getNamedAttr("instance_path", b.getArrayAttr(
                {b.getStringAttr(instanceName)}))})->getResult(0));
  }
  return results;
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

// Rules and Methods share source return normalization. Their final boundary
// separately writes output resources or yields SSA values.
LogicalResult Importer::callableBody(Callable &call, Scope &scope) {
  if (failed(statements(call.node, "body", scope)))
    return failure();
  if (!scope.terminated && !call.results.empty())
    return fail(call.node, "callable must return every declared payload");
  return success();
}

LogicalResult Importer::defineRule(Callable &call, Scope &scope) {
  if (call.defined)
    return success();
  auto previousDefinition = currentDefinition;
  if (call.global)
    currentDefinition = call.name;
  auto restoreDefinition =
      llvm::scope_exit([&] { currentDefinition = previousDefinition; });
  if (!scope.module)
    return fail(call.node, "Rule definitions require a Module or source owner");
  Scope captured = scope;
  captured.module = false;
  captured.callable = &call;
  captured.resultTypes = call.results;
  captured.outputs.clear();
  captured.returned.clear();
  captured.terminated = false;
  captured.globalRule = call.global;
  if (call.global)
    captured.bindings.clear();
  SmallVector<Type> inputTypes, outputTypes;
  SmallVector<Attribute> inputNames;
  for (auto &input : call.inputs) {
    inputTypes.push_back(resource(input.type));
    inputNames.push_back(b.getStringAttr(input.name));
  }
  for (auto result : call.results)
    outputTypes.push_back(resource(result));
  SmallVector<Value> captures;
  for (auto &[name, binding] : captured.bindings)
    if (binding.value && !llvm::is_contained(captures, binding.value))
      captures.push_back(binding.value);
  auto *definition =
      op(call.node, "ac.rule", captures, {},
         {b.getNamedAttr("sym_name", b.getStringAttr(call.name)),
          b.getNamedAttr("function_type", TypeAttr::get(b.getFunctionType(
                                              inputTypes, outputTypes))),
          b.getNamedAttr("input_names", b.getArrayAttr(inputNames))},
         2);
  if (call.global) {
    definition->setAttr("ac.source_owner", owner);
    definition->setAttr("ac.declaration_role", b.getStringAttr("definition"));
  }
  for (unsigned region = 0; region < 2; ++region) {
    auto *block = new Block;
    definition->getRegion(region).push_back(block);
    Scope inner = captured;
    inner.pure = region == 0;
    for (unsigned i = 0; i < inputTypes.size(); ++i)
      inner.bindings[call.inputs[i].name] =
          Binding{block->addArgument(inputTypes[i], definition->getLoc()),
                  {},
                  {},
                  nullptr};
    for (auto &input : call.inputs)
      inner.bindings[input.name].parameter = true;
    for (auto type : outputTypes)
      inner.outputs.push_back(block->addArgument(type, definition->getLoc()));
    DenseMap<Value, Value> mapped;
    for (auto value : captures)
      mapped[value] = block->addArgument(value.getType(), definition->getLoc());
    for (auto &[name, binding] : inner.bindings)
      if (binding.value)
        if (mapped.count(binding.value))
          binding.value = mapped.lookup(binding.value);
    OpBuilder::InsertionGuard save(b);
    b.setInsertionPointToEnd(block);
    if (region == 0) {
      if (failed(emitGuard(call, inner)))
        return failure();
    } else {
      if (failed(callableBody(call, inner)))
        return failure();
      for (auto [output, value] : llvm::zip(inner.outputs, inner.returned))
        if (failed(write(output, value, call.node, inner)))
          return failure();
      yield(call.node);
    }
  }
  call.defined = true;
  return success();
}

FailureOr<SmallVector<Value>>
Importer::registerRule(Callable &call, const AstNode &registration,
                       Scope &scope, ArrayRef<Value> destinations) {
  if (!scope.module || !call.defined)
    return (void)fail(registration,
                      "Rule must be defined and registered in a Module"),
           failure();
  if (!call.global && call.registrations++)
    return (void)fail(registration,
                      "Rule is already registered in this Module"),
           failure();
  SmallVector<StringRef> names;
  for (auto &input : call.inputs)
    names.push_back(input.name);
  SmallVector<AstNode> actuals;
  if (registration.kind() == "Call") {
    auto normalized =
        detail::normalizeCallArguments(registration, names, error);
    if (failed(normalized))
      return failure();
    actuals = *normalized;
  } else if (!names.empty())
    return failure();
  SmallVector<Value> operands, outputs;
  for (unsigned i = 0; i < actuals.size(); ++i) {
    auto values = expression(actuals[i], scope);
    if (failed(values) || values->size() != 1 ||
        !isResource(values->front().getType()) ||
        !equal(payload(values->front().getType()),
               payload(call.inputs[i].type)))
      return (void)fail(actuals[i], "Rule input requires a matching resource"),
             failure();
    operands.push_back(values->front());
  }
  for (unsigned i = 0; i < call.results.size(); ++i) {
    Value target = i < destinations.size() ? destinations[i] : Value();
    if (!target) {
      // Output resources are static Module wiring. Only registration carries
      // the enclosing branch predicate.
      OpBuilder::InsertionGuard save(b);
      if (scope.conditionalDepth) {
        Operation *anchor = b.getInsertionBlock()->getParentOp();
        while (anchor->getParentOp() != scope.owner->operation)
          anchor = anchor->getParentOp();
        b.setInsertionPoint(anchor);
      }
      auto *connection = op(
          registration, "ac.resource.connection", {},
          resource(payload(call.results[i])),
          {b.getNamedAttr("instance_name",
                          b.getStringAttr("$rule" + std::to_string(unique++) +
                                          ".out" + std::to_string(i)))});
      if (!connection)
        return failure();
      target = connection->getResult(0);
    }
    if (!isResource(target.getType()) ||
        !equal(payload(target.getType()), payload(call.results[i])))
      return (void)fail(registration,
                        "Rule output requires a matching resource"),
             failure();
    operands.push_back(target);
    outputs.push_back(target);
  }
  op(registration, "ac.rule.call", operands, {},
     {b.getNamedAttr("callee", symbol(call.name)),
      b.getNamedAttr("input_count", b.getI64IntegerAttr(call.inputs.size())),
      b.getNamedAttr("occurrence", origin(registration, scope.owner->symbol))});
  return outputs;
}

LogicalResult Importer::declareHelper(Callable &call) {
  SmallVector<Type> inputs;
  SmallVector<Attribute> parameters, constraints;
  auto constraint = [&](Type type, const AstNode &annotation) {
    bool boolean =
        annotation.kind() == "Name" && annotation.string("id") == "bool";
    SmallVector<NamedAttribute> fields{
        b.getNamedAttr("kind", b.getStringAttr("hardware")),
        b.getNamedAttr("type", TypeAttr::get(type)),
        b.getNamedAttr("source_kind",
                       b.getStringAttr(boolean                   ? "boolean"
                                       : isa<ac::BitsType>(type) ? "fixed_bits"
                                                                 : "nominal"))};
    if (boolean)
      fields.push_back(b.getNamedAttr(
          "domain",
          ac::SourceDomainAttr::get(b.getContext(),
                                    b.getDictionaryAttr({b.getNamedAttr(
                                        "kind", b.getStringAttr("bool"))}))));
    return b.getDictionaryAttr(fields);
  };
  for (auto &input : call.inputs) {
    inputs.push_back(input.type);
    parameters.push_back(b.getDictionaryAttr(
        {b.getNamedAttr("name", b.getStringAttr(input.name)),
         b.getNamedAttr("binding", b.getStringAttr("positional_or_keyword")),
         b.getNamedAttr("constraint",
                        constraint(input.type, input.node.child("annotation"))),
         b.getNamedAttr("default", detail::absentDefault(b)),
         b.getNamedAttr("origin", origin(input.node, call.name)),
         b.getNamedAttr("location",
                        detail::sourceSpan(b, source.path, input.node))}));
  }
  auto returns = call.node.child("returns");
  for (auto [index, type] : llvm::enumerate(call.results)) {
    auto annotation = call.results.size() > 1
                          ? returns.child("slice").item("elts", index)
                          : returns;
    constraints.push_back(constraint(type, annotation));
  }
  auto *definition = op(
      call.node, "func.func", {}, {},
      {b.getNamedAttr("sym_name", b.getStringAttr(call.name)),
       b.getNamedAttr("function_type",
                      TypeAttr::get(b.getFunctionType(inputs, call.results))),
       b.getNamedAttr("ac.helper_kind", b.getStringAttr("value")),
       b.getNamedAttr("ac.source_owner", owner),
       b.getNamedAttr("ac.origin", origin(call.node, call.name)),
       b.getNamedAttr("ac.declaration_role", b.getStringAttr("definition")),
       b.getNamedAttr("ac.parameters", b.getArrayAttr(parameters)),
       b.getNamedAttr("ac.result_constraints", b.getArrayAttr(constraints)),
       b.getNamedAttr("ac.capture_types", b.getArrayAttr({})),
       b.getNamedAttr("ac.return_form",
                      b.getStringAttr(call.results.empty()       ? "none"
                                      : call.results.size() == 1 ? "single"
                                                                 : "tuple"))},
      1);
  names[call.node.string("name").str()].declaration = definition;
  return success();
}

LogicalResult Importer::defineHelper(Callable &call) {
  auto helper =
      cast<func::FuncOp>(names.at(call.node.string("name").str()).declaration);
  auto *block = helper.addEntryBlock();
  OpBuilder::InsertionGuard save(b);
  b.setInsertionPointToEnd(block);
  auto previousDefinition = currentDefinition;
  currentDefinition = call.name;
  auto restore =
      llvm::scope_exit([&] { currentDefinition = previousDefinition; });
  Declaration owner;
  owner.node = call.node;
  owner.symbol = call.name;
  Scope scope;
  scope.owner = &owner;
  scope.callable = &call;
  scope.pure = true;
  scope.resultTypes = call.results;
  for (auto [input, value] : llvm::zip(call.inputs, block->getArguments()))
    scope.bindings[input.name] = Binding{value, {}, {}, nullptr};
  if (failed(callableBody(call, scope)))
    return failure();
  op(call.node, "func.return", scope.returned, {});
  return success();
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
  auto *method = op(
      call.node, "ac.method", captures, {},
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
      if (failed(callableBody(call, inner)))
        return failure();
      yield(call.node, inner.returned);
    }
  }
  return success();
}

FailureOr<Importer::AssignmentTarget>
Importer::assignmentTarget(const AstNode &node, Scope &scope) {
  AssignmentTarget target;
  AstNode root = node;
  AstNode slice;
  auto tableSelection = [&](const AstNode &syntax) {
    if (syntax.kind() != "Subscript" ||
        syntax.child("value").kind() != "Name")
      return false;
    auto found = scope.bindings.find(syntax.child("value").string("id").str());
    return found != scope.bindings.end() &&
           isa_and_nonnull<ac::TableType>(found->second.type());
  };
  if (root.kind() == "Subscript" && !tableSelection(root)) {
    slice = root.child("slice"); root = root.child("value");
  }
  SmallVector<Attribute> reversed;
  while (root.kind() == "Attribute") {
    reversed.push_back(b.getStringAttr(root.string("attr")));
    root = root.child("value");
  }
  if (tableSelection(root)) {
    auto selected = expression(root, scope);
    if (failed(selected) || selected->size() != 1)
      return failure();
    target.root = selected->front();
    auto state = dyn_cast<ac::StateType>(target.root.getType());
    if (!state)
      return (void)fail(node, "Table assignment requires a State member"),
             failure();
    target.state = true;
    target.type = state.getPayload();
  } else {
  if (root.kind() != "Name")
    return (void)fail(node,"assignment requires a local or State field/slice"), failure();
  target.name = root.string("id").str();
  auto found = scope.bindings.find(target.name);
  if (found == scope.bindings.end())
    return (void)fail(node,"partial assignment requires an existing binding"), failure();
  target.root = found->second.value;
  target.type = found->second.type();
  if (isResource(target.type)) {
    Type data = payload(target.type);
    if (found->second.parameter) {
      // Typed Rule parameters denote loaded payloads. Rebinding any part of
      // that value is local SSA, just like an ordinary whole-name assignment.
      auto loaded = read(target.root, node, scope);
      if (failed(loaded)) return failure();
      target.root = *loaded;
    } else {
      if (!isa<ac::StateType>(target.type))
        return (void)fail(node,"partial resource writes require Module-owned State; read Queue payloads explicitly"), failure();
      target.state = true;
    }
    target.type = data;
  }
  }
  for (Attribute name : llvm::reverse(reversed)) {
    auto structure = dyn_cast<ac::StructType>(target.type);
    auto *decl = structure ? SymbolTable::lookupSymbolIn(*result, structure.getName().getValue()) : nullptr;
    Type next;
    if (decl)
      for (Attribute raw : decl->getAttrOfType<ArrayAttr>("fields")) {
        auto field = cast<DictionaryAttr>(raw);
        if (field.getAs<StringAttr>("name") == name)
          next = field.getAs<TypeAttr>("type").getValue();
      }
    if (!next) return (void)fail(node,"assignment requires a declared Struct field"),failure();
    target.fields.push_back(name);
    target.fieldTypes.push_back(next);
    target.type = next;
  }
  if (slice) {
    auto input = dyn_cast<ac::BitsType>(target.type);
    if (!input) return (void)fail(node,"slice assignment requires Bits"),failure();
    target.low = staticLiteral(llvm::APSInt(llvm::APInt(1,0),true),node,scope.owner->symbol);
    target.high = input.getWidth();
    if (slice.kind() == "Slice") {
      if (slice.child("step")) return (void)fail(slice,"static slices require unit stride"),failure();
      if (auto lower = slice.child("lower")) {
        auto expr = staticExpr(lower,scope.owner->symbol);
        if (failed(expr)) return failure(); target.low = *expr;
      }
      if (auto upper = slice.child("upper")) {
        auto expr = staticExpr(upper,scope.owner->symbol);
        if (failed(expr)) return failure(); target.high = *expr;
      }
    } else {
      auto expr = staticExpr(slice,scope.owner->symbol);
      if (failed(expr)) return failure(); target.low = *expr;
      target.high = ac::StaticExprAttr::get(b.getContext(),b.getDictionaryAttr({
        b.getNamedAttr("kind",b.getStringAttr("binary")),
        b.getNamedAttr("operator",b.getStringAttr("add")),
        b.getNamedAttr("lhs",target.low),
        b.getNamedAttr("rhs",staticLiteral(llvm::APSInt(llvm::APInt(2,1),true),node,scope.owner->symbol)),
        b.getNamedAttr("origin",origin(node,scope.owner->symbol)),
        b.getNamedAttr("location",detail::sourceSpan(b,source.path,node))}));
    }
    auto width = ac::StaticExprAttr::get(b.getContext(),b.getDictionaryAttr({
        b.getNamedAttr("kind",b.getStringAttr("binary")),
        b.getNamedAttr("operator",b.getStringAttr("sub")),
        b.getNamedAttr("lhs",target.high),b.getNamedAttr("rhs",target.low),
        b.getNamedAttr("origin",origin(node,scope.owner->symbol)),
        b.getNamedAttr("location",detail::sourceSpan(b,source.path,node))}));
    target.type = ac::BitsType::get(b.getContext(),width);
  }
  return target;
}
LogicalResult Importer::assignTarget(const AssignmentTarget &target, Value value,
                                    const AstNode &node, Scope &scope) {
  auto converted = convert(value,target.type,node);
  if (failed(converted)) return failure();
  auto fields = b.getArrayAttr(target.fields);
  if (target.state) {
    if (scope.module || scope.pure) return fail(node,"State updates require a Rule or Method body");
    if (target.fields.empty() && !target.low)
      return write(target.root, *converted, node, scope);
    SmallVector<NamedAttribute> attrs{b.getNamedAttr("fields",fields)};
    if (target.low) { attrs.push_back(b.getNamedAttr("low",target.low)); attrs.push_back(b.getNamedAttr("high",target.high)); }
    return success(op(node,"ac.state.write_partial",ValueRange{target.root,*converted},{},attrs) != nullptr);
  }
  Value replacement = *converted;
  Value base = target.root;
  // A mixed field/slice assignment first patches its Bits leaf, then uses the
  // same immutable field update operation to rebuild the containing record.
  if (target.low) {
    for (auto [raw, type] : llvm::zip(target.fields, target.fieldTypes)) {
      base = op(node,"ac.struct.get",base,type,{b.getNamedAttr("field",raw)})->getResult(0);
    }
    replacement = op(node,"ac.bits.update",ValueRange{base,replacement},base.getType(),
        {b.getNamedAttr("low",target.low),b.getNamedAttr("high",target.high)})->getResult(0);
  }
  if (!target.fields.empty())
    replacement = op(node,"ac.struct.update",ValueRange{target.root,replacement},target.root.getType(),
        {b.getNamedAttr("fields",fields)})->getResult(0);
  scope.bindings[target.name] = Binding{replacement,{}, {},nullptr};
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
  if (found != scope.bindings.end() && isResource(found->second.type()) &&
      !found->second.parameter && !scope.loopRebindings.count(name)) {
    if (scope.module) {
      if (values.front() == found->second.value)
        return success();
      if (!isResource(values.front().getType()) ||
          !equal(payload(values.front().getType()),
                 payload(found->second.type())))
        return fail(target,
                    "Module resource connection requires matching payloads");
      if (scope.conditionalDepth)
        return fail(target, "conditional binding cannot select a different resource");
      op(target, "ac.resource.connection",
         ValueRange{found->second.value, values.front()}, found->second.type(),
         {b.getNamedAttr("instance_name",
                         b.getStringAttr(name + ".connection"))});
      return success();
    }
    auto value = read(values.front(), target, scope);
    if (failed(value))
      return failure();
    return write(found->second.value, *value, target, scope);
  }
  if (found != scope.bindings.end() &&
      (found->second.pending || found->second.pendingImport ||
       (isa_and_nonnull<ac::ModuleRefType>(found->second.type()) &&
        !scope.loopRebindings.count(name))))
    return fail(target, "instance identity cannot be reassigned");
  Value value = values.front();
  if (!scope.module) {
    auto loaded = read(value, target, scope);
    if (failed(loaded))
      return failure();
    value = *loaded;
  }
  scope.bindings[name] = Binding{value, {}, {}, nullptr};
  scope.staticLocals.erase(name);
  scope.mathematicalStaticLocals.erase(name);
  return success();
}

LogicalResult Importer::statements(const AstNode &node, StringRef field,
                                   Scope &scope,
                                   ArrayRef<StatementRange> continuation) {
  return statements(StatementRange{node, field, 0}, scope,
                    continuation);
}

LogicalResult Importer::statements(ArrayRef<StatementRange> ranges,
                                   Scope &scope) {
  for (size_t i = 0; i < ranges.size(); ++i) {
    if (failed(statements(ranges[i], scope, ranges.drop_front(i + 1))))
      return failure();
    if (scope.terminated)
      break;
  }
  return success();
}

LogicalResult Importer::statements(const StatementRange &range, Scope &scope,
                                   ArrayRef<StatementRange> continuation) {
  for (size_t i = range.begin; i < range.owner.array(range.field).size(); ++i) {
    auto child = range.owner.item(range.field, i);
    if (scope.terminated)
      return fail(child, "statement after unconditional return is unreachable");
    SmallVector<StatementRange> tail;
    if (child.kind() == "If" || child.kind() == "For") {
      if (i + 1 < range.owner.array(range.field).size())
        tail.push_back({range.owner, range.field, i + 1});
      tail.append(continuation.begin(), continuation.end());
    }
    if (failed(statement(child, scope, tail)))
      return failure();
    // Return ends this callable path, including its unreachable source tail.
    // A normalized branch has already consumed each live continuation.
    if (!scope.module && scope.terminated)
      return success();
  }
  return success();
}

SmallVector<AstNode> Importer::returnExpressions(const AstNode &node) {
  auto value = node.child("value");
  SmallVector<AstNode> actuals;
  if (!value || (value.kind() == "Constant" &&
                 isa_and_nonnull<UnitAttr>(value.get("value"))))
    return actuals;
  if (value.kind() == "Tuple") {
    for (size_t i = 0; i < value.array("elts").size(); ++i)
      actuals.push_back(value.item("elts", i));
  } else
    actuals.push_back(value);
  return actuals;
}

LogicalResult Importer::moduleReturn(const AstNode &node, Scope &scope) {
  if (scope.conditionalDepth)
    return fail(node, "Module resource exports require a terminal return");
  auto actuals = returnExpressions(node);
  if (actuals.size() != scope.resultTypes.size())
    return fail(node, "return arity differs from declared result types");
  SmallVector<Value> values;
  for (size_t i = 0; i < actuals.size(); ++i) {
    auto value = expression(actuals[i], scope, payload(scope.resultTypes[i]),
                            ArrayRef<Value>(scope.outputs[i]));
    if (failed(value) || value->size() != 1)
      return failure();
    Value result = value->front();
    if (!isResource(result.getType()))
      return fail(actuals[i], "Module payload outputs require a Rule result");
    if (!equal(payload(result.getType()), payload(scope.resultTypes[i])))
      return fail(actuals[i],
                  "Module output payload does not match its annotation");
    if (result.getDefiningOp<ac::ResourceConnectionOp>()) {
      op(actuals[i], "ac.resource.connection",
         ValueRange{scope.outputs[i], result}, result.getType(),
         {b.getNamedAttr("instance_name",
                         b.getStringAttr("$return" + std::to_string(i)))});
      result = scope.outputs[i];
    }
    values.push_back(result);
  }
  SmallVector<Attribute> kinds;
  SmallVector<Type> outputs;
  SmallVector<unsigned> erased;
  for (unsigned i = 0; i < values.size(); ++i) {
    bool bound = values[i] == scope.outputs[i];
    kinds.push_back(b.getStringAttr(bound ? "bound" : "export"));
    outputs.push_back(values[i].getType());
    if (!bound)
      erased.push_back(cast<BlockArgument>(scope.outputs[i]).getArgNumber());
  }
  auto *definition = scope.owner->operation;
  auto signature = cast<FunctionType>(
      definition->getAttrOfType<TypeAttr>("function_type").getValue());
  definition->setAttr(
      "function_type",
      TypeAttr::get(b.getFunctionType(signature.getInputs(), outputs)));
  definition->setAttr("output_kinds", b.getArrayAttr(kinds));
  for (auto index : llvm::reverse(erased)) {
    auto &body = definition->getRegion(0).front();
    if (!body.getArgument(index).use_empty())
      return fail(node, "exported resource cannot replace a used bound output");
    body.eraseArgument(index);
  }
  yield(node, values);
  scope.returned = values;
  scope.terminated = true;
  return success();
}

FailureOr<std::set<std::string>> Importer::loopLiveNames(
    const AstNode &node, ArrayRef<StatementRange> continuation) {
  using Names = std::set<std::string>;
  Names live;
  auto defineTarget = [&](const AstNode &target, Names &defined) {
    return detail::forEachAssignmentLeaf(target, [&](const AstNode &leaf) {
      if (leaf.kind() == "Name")
        defined.insert(leaf.string("id").str());
      return success();
    });
  };
  Names persistent;
  if (activeScope && activeScope->module) {
    std::function<LogicalResult(const AstNode &)> owners;
    owners = [&](const AstNode &syntax) -> LogicalResult {
      if (!charge(syntax))
        return failure();
      if (syntax.kind() == "FunctionDef")
        return success();
      if (syntax.kind() == "AnnAssign") {
        auto nominal = resolve(syntax.child("annotation"));
        bool instance = nominal && nominal->symbol &&
                        (declarations.count(nominal->remote) ||
                         isa_and_nonnull<ac::ModuleImportOp>(nominal->declaration));
        if (!instance && failed(defineTarget(syntax.child("target"), persistent)))
          return failure();
      } else if (syntax.kind() == "Assign") {
        auto value = syntax.child("value");
        auto function = value.child("func");
        if (value.kind() == "Call" && function.kind() == "Subscript" &&
            marker(function.child("value"), "queue"))
          for (size_t i = 0; i < syntax.array("targets").size(); ++i)
            if (failed(defineTarget(syntax.item("targets", i), persistent)))
              return failure();
      }
      for (StringRef field : {"body", "orelse"}) {
        auto children = syntax.array(field);
        for (size_t i = 0; children && i < children.size(); ++i)
          if (failed(owners(syntax.item(field, i))))
            return failure();
      }
      return success();
    };
    if (failed(owners(node)))
      return failure();
  }
  std::function<LogicalResult(const AstNode &, const Names &)> readSyntax;
  std::function<LogicalResult(const AstNode &, Names &)> scanStatement;
  auto scanBody = [&](const AstNode &owner, StringRef field, size_t begin,
                      Names &defined) -> LogicalResult {
    auto body = owner.array(field);
    for (size_t i = begin; body && i < body.size(); ++i)
      if (failed(scanStatement(owner.item(field, i), defined)))
        return failure();
    return success();
  };
  auto defineParameters = [&](const AstNode &args, Names &defined) {
    for (StringRef field : {"args", "posonlyargs", "kwonlyargs"}) {
      auto parameters = args.array(field);
      for (size_t i = 0; parameters && i < parameters.size(); ++i)
        defined.insert(args.item(field, i).string("arg").str());
    }
    for (StringRef field : {"vararg", "kwarg"})
      if (auto parameter = args.child(field))
        defined.insert(parameter.string("arg").str());
  };
  readSyntax = [&](const AstNode &syntax, const Names &defined) -> LogicalResult {
    if (!syntax || syntax.kind().empty())
      return success();
    if (!charge(syntax))
      return failure();
    if (syntax.kind() == "Name") {
      if (syntax.child("ctx").kind() != "Store" &&
          !defined.count(syntax.string("id").str()))
        live.insert(syntax.string("id").str());
      return success();
    }
    if (syntax.kind() == "Lambda") {
      Names inner = defined;
      defineParameters(syntax.child("args"), inner);
      return readSyntax(syntax.child("body"), inner);
    }
    for (NamedAttribute field : syntax.fields()) {
      auto name = field.getName().getValue();
      if (isa<DictionaryAttr>(field.getValue())) {
        if (failed(readSyntax(syntax.child(name), defined)))
          return failure();
      } else if (auto array = dyn_cast<ArrayAttr>(field.getValue())) {
        for (size_t i = 0; i < array.size(); ++i)
          if (isa<DictionaryAttr>(array[i]) &&
              failed(readSyntax(syntax.item(name, i), defined)))
            return failure();
      }
    }
    return success();
  };
  scanStatement = [&](const AstNode &statement, Names &defined) -> LogicalResult {
    if (!charge(statement))
      return failure();
    if (statement.kind() == "FunctionDef") {
      // Rules and Methods read captured Module bindings before sequential
      // rebinding. Only parameters and explicit local declarations mask those
      // captures; ordinary helper functions retain Python lexical locals.
      bool capturesAssignments = false;
      auto decorators = statement.array("decorator_list");
      for (size_t i = 0; decorators && i < decorators.size(); ++i) {
        auto decorator = statement.item("decorator_list", i);
        auto target = decorator.kind() == "Call" ? decorator.child("func")
                                                 : decorator;
        capturesAssignments |= marker(target, "rule") ||
                               marker(target, "autorule") || marker(target, "method");
      }
      Names local, nonlocal, written;
      defineParameters(statement.child("args"), local);
      std::function<LogicalResult(const AstNode &)> bindings;
      bindings = [&](const AstNode &syntax) -> LogicalResult {
        if (!charge(syntax))
          return failure();
        if (syntax.kind() == "FunctionDef") {
          local.insert(syntax.string("name").str());
          return success();
        }
        if (syntax.kind() == "Nonlocal") {
          for (Attribute raw : syntax.array("names"))
            nonlocal.insert(cast<StringAttr>(raw).getValue().str());
        } else if (syntax.kind() == "Assign") {
          for (size_t i = 0; i < syntax.array("targets").size(); ++i)
            if (failed(defineTarget(syntax.item("targets", i),
                                    capturesAssignments ? written : local)))
              return failure();
        } else if (syntax.kind() == "AnnAssign" || syntax.kind() == "AugAssign" ||
                   syntax.kind() == "For") {
          Names &targets = capturesAssignments && syntax.kind() != "AnnAssign"
                               ? written : local;
          if (failed(defineTarget(syntax.child("target"), targets)))
            return failure();
        }
        for (StringRef field : {"body", "orelse"}) {
          auto children = syntax.array(field);
          for (size_t i = 0; children && i < children.size(); ++i)
            if (failed(bindings(syntax.item(field, i))))
              return failure();
        }
        return success();
      };
      for (size_t i = 0; i < statement.array("body").size(); ++i)
        if (failed(bindings(statement.item("body", i))))
          return failure();
      for (const auto &name : nonlocal)
        local.erase(name);
      // Writing captured storage uses its owner even when the new payload does
      // not read the old value. Payload-only rebindings remain ordinary kills.
      for (const auto &name : written)
        if (persistent.count(name) && !local.count(name) && !defined.count(name))
          live.insert(name);
      Names inner = defined;
      inner.insert(local.begin(), local.end());
      if (failed(readSyntax(statement.child("args"), defined)) ||
          failed(readSyntax(statement.child("returns"), defined)) ||
          failed(scanBody(statement, "body", 0, inner)))
        return failure();
      for (size_t i = 0; decorators && i < decorators.size(); ++i)
        if (failed(readSyntax(statement.item("decorator_list", i), defined)))
          return failure();
      defined.insert(statement.string("name").str());
      return success();
    }
    if (statement.kind() == "If") {
      if (failed(readSyntax(statement.child("test"), defined)))
        return failure();
      Names arms[2] = {defined, defined};
      if (failed(scanBody(statement, "body", 0, arms[0])) ||
          failed(scanBody(statement, "orelse", 0, arms[1])))
        return failure();
      for (const auto &name : arms[0])
        if (arms[1].count(name))
          defined.insert(name);
      return success();
    }
    if (statement.kind() == "For") {
      if (failed(readSyntax(statement.child("iter"), defined)))
        return failure();
      Names inner = defined;
      if (failed(defineTarget(statement.child("target"), inner)) ||
          failed(scanBody(statement, "body", 0, inner)))
        return failure();
      // The nested loop may execute zero times; its assignments are not
      // definite in this containing iteration or continuation.
      return success();
    }
    if (statement.kind() == "Assign" || statement.kind() == "AnnAssign" ||
        statement.kind() == "AugAssign") {
      if (failed(readSyntax(statement, defined)))
        return failure();
      if (statement.kind() == "Assign") {
        for (size_t i = 0; i < statement.array("targets").size(); ++i)
          if (failed(defineTarget(statement.item("targets", i), defined)))
            return failure();
      } else {
        auto target = statement.child("target");
        if (statement.kind() == "AugAssign" && target.kind() == "Name" &&
            !defined.count(target.string("id").str()))
          live.insert(target.string("id").str());
        if (failed(defineTarget(target, defined)))
          return failure();
      }
      return success();
    }
    return readSyntax(statement, defined);
  };
  Names defined{node.child("target").string("id").str()};
  if (failed(scanBody(node, "body", 0, defined)))
    return failure();
  defined.clear();
  for (const auto &range : continuation)
    if (failed(scanBody(range.owner, range.field, range.begin, defined)))
      return failure();
  return live;
}

LogicalResult Importer::staticFor(const AstNode &node, Scope &scope,
                                ArrayRef<StatementRange> continuation) {
  auto target = node.child("target"), iterator = node.child("iter");
  auto range = iterator.child("func");
  Value iterationTable;
  std::string memberName;
  bool enumerated = iterator.kind() == "Call" &&
                    range.kind() == "Name" &&
                    range.string("id") == "enumerate" &&
                    !scope.bindings.count("enumerate") &&
                    !names.count("enumerate") &&
                    iterator.array("args").size() == 1 &&
                    iterator.array("keywords").empty();
  bool ranged = iterator.kind() == "Call" && range.kind() == "Name" &&
                range.string("id") == "range";
  if (!ranged) {
    auto values = expression(enumerated ? iterator.item("args", 0) : iterator,
                             scope);
    if (failed(values) || values->size() != 1 ||
        !isa<ac::TableType>(values->front().getType()))
      return fail(iterator, "static iteration requires range or a component Table");
    iterationTable = values->front();
    if (enumerated) {
      if (target.kind() != "Tuple" || target.array("elts").size() != 2 ||
          target.item("elts", 0).kind() != "Name" ||
          target.item("elts", 1).kind() != "Name")
        return fail(target, "enumerate requires an index and a member name");
      memberName = target.item("elts", 1).string("id").str();
      target = target.item("elts", 0);
    } else {
      if (target.kind() != "Name")
        return fail(target, "Table iteration requires one member name");
      memberName = target.string("id").str();
    }
  }
  if (target.kind() != "Name" || (!iterationTable && (iterator.kind() != "Call" ||
      range.kind() != "Name" || range.string("id") != "range" ||
      scope.bindings.count("range") || names.count("range") ||
      detail::sourceBindingShadowed(source, range, "range") ||
      !iterator.array("keywords").empty() || iterator.array("args").empty() ||
      iterator.array("args").size() > 3)) || !node.array("orelse").empty())
    return fail(node, "static for requires one name and range with one to three "
                      "static arguments, without for-else");
  std::string indexName = iterationTable && !enumerated
                              ? "$table_index_" + std::to_string(unique++)
                              : target.string("id").str();
  if (names.count(indexName) || globalRules.count(indexName) ||
      scope.owner->callables.count(indexName))
    return fail(target, "loop target cannot shadow an imported or callable name");

  // Determine the existing bindings that the body may replace. This is a
  // syntactic binding scan, not evaluation or expansion of the loop body.
  llvm::StringSet<> assigned;
  auto assignmentName = [&](const AstNode &leaf) -> LogicalResult {
    AstNode root = leaf;
    while (root.kind() == "Attribute" || root.kind() == "Subscript")
      root = root.child("value");
    if (root.kind() == "Name")
      assigned.insert(root.string("id"));
    return success();
  };
  std::function<LogicalResult(const AstNode &)> scanBody;
  scanBody = [&](const AstNode &statement) -> LogicalResult {
    if (statement.kind() == "FunctionDef")
      return fail(statement, "callables must be declared directly in their Module");
    if (statement.kind() == "Return" || statement.kind() == "Break" ||
        statement.kind() == "Continue")
      return fail(statement, "static for does not support return, break or continue");
    if (statement.kind() == "Assign") {
      for (size_t i = 0; i < statement.array("targets").size(); ++i)
        if (failed(detail::forEachAssignmentLeaf(
                statement.item("targets", i), assignmentName)))
          return failure();
    } else if (statement.kind() == "AnnAssign" ||
               statement.kind() == "AugAssign" || statement.kind() == "For") {
      if (failed(detail::forEachAssignmentLeaf(statement.child("target"),
                                               assignmentName)))
        return failure();
    }
    for (StringRef field : {"body", "orelse"}) {
      auto children = statement.array(field);
      for (size_t i = 0; children && i < children.size(); ++i)
        if (failed(scanBody(statement.item(field, i))))
          return failure();
    }
    return success();
  };
  assigned.insert(indexName);
  if (!memberName.empty())
    assigned.erase(memberName);
  for (size_t i = 0; i < node.array("body").size(); ++i)
    if (failed(scanBody(node.item("body", i))))
      return failure();

  auto loopOrigin = origin(node, currentDefinition);
  SmallVector<ac::StaticExprAttr> bounds;
  unsigned arity = iterationTable ? 1 : iterator.array("args").size();
  if (arity == 1)
    bounds.push_back(staticLiteral(llvm::APSInt(llvm::APInt(2, 0), true),
                                   node, scope.owner->symbol));
  for (unsigned i = 0; i < arity; ++i) {
    auto bound = iterationTable
                     ? FailureOr<ac::StaticExprAttr>(
                           cast<ac::TableType>(iterationTable.getType()).getCapacity())
                     : staticExpr(iterator.item("args", i), scope.owner->symbol);
    if (failed(bound))
      return failure();
    bounds.push_back(*bound);
  }
  if (arity < 3)
    bounds.push_back(staticLiteral(llvm::APSInt(llvm::APInt(2, 1), true),
                                   node, scope.owner->symbol));
  SmallVector<Value> operands;
  for (auto bound : bounds)
    operands.push_back(op(node, "ac.source.static_index", {}, b.getIndexType(),
                          {b.getNamedAttr("value", bound)})
                           ->getResult(0));
  SmallVector<std::string> carriedNames;
  SmallVector<Type> types;
  for (const auto &[name, binding] : scope.bindings) {
    if (!assigned.count(name) || !binding.value)
      continue;
    if (isa<ac::TableType>(binding.type()) || name == memberName)
      continue;
    if (name == indexName &&
        (isResource(binding.type()) || isa<ac::ModuleRefType>(binding.type())))
      return fail(target, "loop target requires a local payload binding");
    if (isResource(binding.type()) && !binding.parameter &&
        (!scope.module || isa<ac::StateType, ac::QueueType>(binding.type()))) {
      continue;
    }
    if (isa<ac::ModuleRefType>(binding.type()) && !scope.module)
      continue;
    Value initial = binding.value;
    if (!scope.module && binding.parameter && isResource(initial.getType())) {
      auto loaded = read(initial, node, scope);
      if (failed(loaded))
        return failure();
      initial = *loaded;
    }
    carriedNames.push_back(name);
    types.push_back(initial.getType());
    operands.push_back(initial);
  }
  auto live = loopLiveNames(node, continuation);
  if (failed(live))
    return failure();
  std::set<std::string> freshNames;
  for (const auto &name : assigned)
    if (!scope.bindings.count(name.getKey().str()) &&
        live->count(name.getKey().str()))
      freshNames.insert(name.getKey().str());
  for (const auto &name : freshNames) {
    auto type = ac::InferType::get(b.getContext());
    auto unbound = op(node, "ac.source.unbound", {}, type)->getResult(0);
    carriedNames.push_back(name);
    types.push_back(type);
    operands.push_back(unbound);
  }
  auto *loop = op(node, "scf.for", operands, types,
                  {b.getNamedAttr("occurrence", loopOrigin)}, 1);
  auto *body = new Block;
  loop->getRegion(0).push_back(body);
  auto induction = body->addArgument(b.getIndexType(), loop->getLoc());
  Scope inner = scope;
  for (auto [name, type] : llvm::zip(carriedNames, types)) {
    inner.bindings[name] = Binding{body->addArgument(type, loop->getLoc()),
                                    {}, {}, nullptr};
    inner.bindings[name].loopUnbound =
        freshNames.count(name) || scope.bindings.at(name).loopUnbound;
    if (scope.module &&
        (freshNames.count(name) || isResource(type) || isa<ac::ModuleRefType>(type)))
      inner.loopRebindings.insert(name);
  }
  {
    OpBuilder::InsertionGuard save(b);
    b.setInsertionPointToEnd(body);
    Type indexType = ac::InferType::get(b.getContext());
    auto incomingIndex = scope.bindings.find(indexName);
    if (incomingIndex != scope.bindings.end() &&
        isa<ac::BitsType>(incomingIndex->second.type()))
      indexType = incomingIndex->second.type();
    auto index = op(node, "ac.source.index", induction, indexType,
                    {b.getNamedAttr("occurrence", loopOrigin),
                     b.getNamedAttr("source_location",
                                    detail::sourceSpan(b, source.path, node)),
                     b.getNamedAttr("is_signed", b.getBoolAttr(true))})
                     ->getResult(0);
    inner.bindings[indexName] = Binding{index, {}, {}, nullptr};
    auto ref = b.getDictionaryAttr(
        {b.getNamedAttr("kind", b.getStringAttr("induction")),
         b.getNamedAttr("loop", loopOrigin)});
    auto expression = ac::StaticExprAttr::get(
        b.getContext(), b.getDictionaryAttr({
            b.getNamedAttr("kind", b.getStringAttr("reference")),
            b.getNamedAttr("ref", ref),
            b.getNamedAttr("origin", origin(target, scope.owner->symbol)),
            b.getNamedAttr("location", detail::sourceSpan(b, source.path, target))}));
    inner.inductions[indexName] = {index, expression};
    if (iterationTable) {
      auto type = cast<ac::TableType>(iterationTable.getType()).getElementType();
      auto selected = op(node, "ac.table.select", ValueRange{iterationTable, index},
                         type)->getResult(0);
      inner.bindings[memberName] = Binding{selected, {}, {}, nullptr};
    }
    if (failed(statements(node, "body", inner)))
      return failure();
    SmallVector<Value> values;
    for (auto [name, type] : llvm::zip(carriedNames, types)) {
      Value value = inner.bindings.at(name).value;
      if (!value ||
          (!scope.module && (isResource(value.getType()) ||
                            isa<ac::ModuleRefType>(value.getType()))))
        return fail(node, "loop-carried local requires a payload value");
      if ((isResource(type) || isa<ac::ModuleRefType>(type) ||
           isResource(value.getType()) ||
           isa<ac::ModuleRefType>(value.getType())) &&
          !isa<ac::InferType>(type) && !equal(value.getType(), type))
        return fail(node, "loop-carried references require the same interface type");
      if (value.getType() != type)
        value = op(node, "ac.payload.pending", value, type,
                    {b.getNamedAttr("kind", b.getStringAttr("convert")),
                     b.getNamedAttr("ac.join_transport", b.getUnitAttr())})
                    ->getResult(0);
      values.push_back(value);
    }
    op(node, "scf.yield", values, {});
  }
  b.setInsertionPointAfter(loop);
  for (auto [name, value] : llvm::zip(carriedNames, loop->getResults())) {
    Binding binding = inner.bindings.at(name);
    Type finalType = binding.type();
    if (isa<ac::InferType>(value.getType()) && finalType &&
        !isa<ac::InferType>(finalType))
      value = op(node, "ac.payload.pending", value, finalType,
                  {b.getNamedAttr("kind", b.getStringAttr("convert"))})
                  ->getResult(0);
    binding.value = value;
    binding.parameter = false;
    scope.bindings[name] = binding;
    scope.inductions.erase(name);
  }
  return success();
}

LogicalResult Importer::statement(const AstNode &node, Scope &scope,
                                  ArrayRef<StatementRange> continuation) {
  auto *previousScope = activeScope;
  activeScope = &scope;
  auto restoreScope = llvm::scope_exit([&] { activeScope = previousScope; });
  if (!charge(node))
    return failure();
  if (node.kind() == "Pass")
    return success();
  if (node.kind() == "For")
    return staticFor(node, scope, continuation);
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
    auto previous = scope.bindings.find(name);
    if (previous != scope.bindings.end() && !previous->second.loopUnbound)
      return fail(target, "duplicate declared local");
    scope.loopRebindings.erase(name);
    if (names.count(name) || globalRules.count(name) ||
        scope.owner->callables.count(name))
      return fail(
          target,
          "local declaration cannot shadow imported or callable source names");
    auto typeNode = node.child("annotation");
    if (scope.module) {
      if (scope.conditionalDepth)
        return fail(node,
                    "resources must be declared directly in their Module");
      if (typeNode.kind() == "Subscript" &&
          marker(typeNode.child("value"), "table")) {
        auto type = dyn_cast_or_null<ac::TableType>(
            annotation(typeNode, scope.owner->symbol));
        if (!type)
          return failure();
        SmallVector<Value> initializers;
        auto initial = node.child("value");
        if (initial) {
          auto state = dyn_cast<ac::StateType>(type.getElementType());
          if (!state)
            return fail(initial, "Module Table members use default construction");
          if (initial.kind() == "Tuple" || initial.kind() == "List") {
            for (size_t i = 0; i < initial.array("elts").size(); ++i) {
              auto value = scalar(initial.item("elts", i), scope,
                                  state.getPayload());
              if (failed(value))
                return failure();
              initializers.push_back(*value);
            }
          } else {
            auto value = scalar(initial, scope, state.getPayload());
            if (failed(value))
              return failure();
            initializers.push_back(*value);
          }
        }
        auto *allocation = op(
            node, "ac.table.alloc", initializers, type,
            {b.getNamedAttr("instance_name", b.getStringAttr(name)),
             b.getNamedAttr("initial_image", b.getBoolAttr(
                 initial && (initial.kind() == "Tuple" || initial.kind() == "List")))});
        if (!allocation)
          return failure();
        scope.bindings[name] = Binding{allocation->getResult(0), {}, {}, nullptr};
        return success();
      }
      if (auto nominal = resolve(typeNode))
        if (nominal->symbol &&
            (declarations.count(nominal->remote) ||
             isa_and_nonnull<ac::ModuleImportOp>(nominal->declaration))) {
          if (node.child("value"))
            return fail(
                node, "declare an instance then connect it at its wiring site");
          Binding binding;
          binding.instanceName = name;
          binding.declaration = node;
          binding.instanceTarget = nominal->symbol.getValue().str();
          if (declarations.count(nominal->remote))
            binding.pending = declarations.at(nominal->remote).get();
          else
            binding.pendingImport = nominal->declaration;
          scope.bindings[name] = binding;
          bool closed = binding.pending
                            ? binding.pending->inputs.empty()
                            : cast<FunctionType>(
                                  binding.pendingImport
                                      ->getAttrOfType<TypeAttr>("function_type")
                                      .getValue())
                                      .getNumInputs() == 0;
          if (closed && failed(expression(target, scope)))
            return failure();
          return success();
        }
      if (typeNode.kind() == "Subscript" &&
          marker(typeNode.child("value"), "queue")) {
        Type type = annotation(typeNode, scope.owner->symbol);
        if (!type)
          return failure();
        auto values = expression(node.child("value"), scope);
        if (failed(values) || values->size() != 1 ||
            !isa<ac::QueueType>(values->front().getType()))
          return failure();
        auto value = convert(values->front(), type, node);
        if (failed(value))
          return failure();
        scope.bindings[name] = Binding{*value, {}, {}, nullptr};
        return success();
      }
      AstNode initial = node.child("value");
      // A constructor annotation declares initialized storage; an ordinary
      // assignment of the same constructor remains a combinational value.
      if (typeNode.kind() == "Call") {
        if (initial)
          return fail(node, "constructor state declaration already has an initializer");
        initial = typeNode;
        typeNode = typeNode.child("func");
      }
      Type type = annotation(typeNode, scope.owner->symbol);
      if (!type)
        return failure();
      if (!isa<ac::BitsType, ac::EnumType, ac::StructType>(type))
        return fail(node, "State requires a Bits, Enum or Struct variable");
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
    return success();
  }
  if (node.kind() == "Assign") {
    if (node.array("targets").size() != 1)
      return fail(node, "chained assignment is not supported");
    auto target = node.item("targets", 0);
    if (target.kind() == "Attribute" || target.kind() == "Subscript") {
      auto destination = assignmentTarget(target, scope);
      if (failed(destination)) return failure();
      auto value = scalar(node.child("value"), scope, destination->type);
      if (failed(value)) return failure();
      return assignTarget(*destination, *value, target, scope);
    }
    Type expected;
    if (target.kind() == "Name") {
      auto name = target.string("id").str();
      auto binding = scope.bindings.find(name);
      if (binding != scope.bindings.end() && isResource(binding->second.type()))
        expected = payload(binding->second.type());
    }
    SmallVector<Value> destinations;
    if (scope.module && node.child("value").kind() == "Call") {
      auto addDestination = [&](const AstNode &leaf) -> LogicalResult {
        if (leaf.kind() != "Name")
          return fail(
              leaf, "output bindings require names or a direct tuple of names");
        auto found = scope.bindings.find(leaf.string("id").str());
        destinations.push_back(found != scope.bindings.end() &&
                                       !scope.loopRebindings.count(
                                           leaf.string("id").str()) &&
                                       isResource(found->second.type())
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
    auto savedRequirement = scope.pendingResultRequirement;
    auto savedArity = scope.pendingResultArity;
    scope.pendingResultRequirement =
        target.kind() == "Tuple" ? "tuple" : "scalar";
    scope.pendingResultArity =
        target.kind() == "Tuple" ? target.array("elts").size() : 1;
    auto values =
        expression(node.child("value"), scope, expected, destinations);
    scope.pendingResultRequirement = savedRequirement;
    scope.pendingResultArity = savedArity;
    if (failed(values))
      return failure();
    ac::StaticExprAttr staticValue;
    bool mathematicalValue = false;
    if (target.kind() == "Name" && values->size() == 1 &&
        isStaticExpression(node.child("value"), scope)) {
      auto captured = staticExpr(node.child("value"), scope.owner->symbol);
      if (failed(captured))
        return failure();
      staticValue = *captured;
      mathematicalValue = isStaticExpression(node.child("value"), scope, true);
    }
    if (failed(bind(target, *values, scope)))
      return failure();
    if (staticValue) {
      auto name = target.string("id").str();
      const auto &binding = scope.bindings.at(name);
      if (!isResource(binding.type()) && !isa<ac::ModuleRefType>(binding.type())) {
        scope.staticLocals[name] = {binding.value, staticValue};
        if (mathematicalValue && isa<ac::InferType>(binding.type()))
          scope.mathematicalStaticLocals.insert(name);
      }
    }
    return success();
  }
  if (node.kind() == "AugAssign") {
    auto target = node.child("target");
    std::optional<AssignmentTarget> destination;
    if (target.kind() != "Name") {
      auto parsed = assignmentTarget(target, scope);
      if (failed(parsed)) return failure();
      destination = *parsed;
    }
    // Resolve an indexed receiver once, as Python augmented assignment does.
    // Re-evaluating the target could invoke an index-producing Method twice.
    FailureOr<Value> lhs = failure();
    if (destination) {
      lhs = read(destination->root, target, scope);
      if (failed(lhs))
        return failure();
      for (auto [field, type] :
           llvm::zip(destination->fields, destination->fieldTypes))
        lhs = op(target, "ac.struct.get", *lhs, type,
                 {b.getNamedAttr("field", field)})->getResult(0);
      if (destination->low)
        lhs = op(target, "ac.bits.extract", *lhs, destination->type,
                 {b.getNamedAttr("low", destination->low)})->getResult(0);
    } else {
      lhs = scalar(target, scope);
    }
    if (failed(lhs))
      return failure();
    auto rhs = scalar(node.child("value"), scope);
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
    Value value = op(node, "ac.payload.pending", ValueRange{*lhs, *rhs},
                     ac::InferType::get(b.getContext()),
                     {b.getNamedAttr("kind", b.getStringAttr("binary")),
                      b.getNamedAttr("opcode", b.getStringAttr(opcode))})
                      ->getResult(0);
    if (destination) return assignTarget(*destination, value, target, scope);
    return bind(target, ArrayRef<Value>(value), scope);
  }
  if (node.kind() == "Return") {
    if (scope.module)
      return moduleReturn(node, scope);
    auto actuals = returnExpressions(node);
    if (actuals.size() != scope.resultTypes.size())
      return fail(node, "return arity differs from declared result types");
    SmallVector<Value> values;
    for (auto [actual, type] : llvm::zip(actuals, scope.resultTypes)) {
      auto value = scalar(actual, scope, type);
      if (failed(value))
        return failure();
      values.push_back(*value);
    }
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
    auto savedRequirement = scope.pendingResultRequirement;
    auto savedArity = scope.pendingResultArity;
    scope.pendingResultRequirement = "ignored";
    scope.pendingResultArity = 0;
    auto restoreDemand = llvm::scope_exit([&] {
      scope.pendingResultRequirement = savedRequirement;
      scope.pendingResultArity = savedArity;
    });
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
    auto test = node.child("test");
    bool staticComparison = false;
    if (test.kind() == "Compare" && test.array("ops").size() == 1 &&
        test.array("comparators").size() == 1) {
      auto form = test.item("ops", 0).kind();
      staticComparison =
          (form == "Eq" || form == "NotEq" || form == "Lt" || form == "LtE" ||
           form == "Gt" || form == "GtE") &&
          isStaticExpression(test.child("left"), scope, true) &&
          isStaticExpression(test.item("comparators", 0), scope, true);
    }
    Value selected;
    if (staticComparison) {
      auto value = staticExpr(test, scope.owner->symbol);
      if (failed(value))
        return failure();
      selected = op(node, "ac.source.static_predicate", {}, b.getI1Type(),
                     {b.getNamedAttr("value", *value)})->getResult(0);
    } else {
      auto condition = scalar(test, scope, bits(1, node));
      if (failed(condition))
        return failure();
      if (!scope.module)
        if (auto known = constantPredicate(*condition, node))
          return statements(node, *known ? "body" : "orelse", scope,
                            continuation);
      selected = op(node, "ac.source.predicate", *condition, b.getI1Type())
                     ->getResult(0);
    }
    auto *branch = op(node, "scf.if", selected, {}, {}, 2);
    std::map<std::string, Binding> branchResources;
    Scope arms[2] = {scope, scope};
    for (unsigned arm = 0; arm < 2; ++arm) {
      auto *block = new Block;
      branch->getRegion(arm).push_back(block);
      OpBuilder::InsertionGuard save(b);
      b.setInsertionPointToEnd(block);
      Scope &inner = arms[arm];
      ++inner.conditionalDepth;
      if (failed(statements(node, arm == 0 ? "body" : "orelse", inner,
                            continuation)))
        return failure();
      if (scope.module)
        for (const auto &[name, binding] : inner.bindings)
          if (!scope.bindings.count(name) && isResource(binding.type())) {
            if (!branchResources.emplace(name, binding).second)
              return fail(node, "branch outputs require distinct names or an "
                                "explicitly declared common destination");
          }
    }
    SmallVector<std::string> mergedNames;
    SmallVector<Value> values[2];
    bool complete = arms[0].terminated || arms[1].terminated;
    if (complete) {
      // Returned arms bypass the continuation; live arms retain their local
      // bindings until their own return.
      // Re-emitted statements remain charged by statement()/expression().
      for (unsigned arm = 0; arm < 2; ++arm) {
        b.setInsertionPointToEnd(&branch->getRegion(arm).front());
        if (!arms[arm].terminated &&
            failed(statements(continuation, arms[arm])))
          return failure();
        if (!arms[arm].terminated && !scope.resultTypes.empty())
          return fail(node, "callable path must return every declared payload");
        values[arm] = arms[arm].returned;
      }
    } else {
      for (const auto &[name, left] : arms[0].bindings) {
        auto right = arms[1].bindings.find(name);
        if (right == arms[1].bindings.end())
          continue;
        const auto &other = right->second;
        if (left.value == other.value) {
          if (!scope.bindings.count(name))
            scope.bindings[name] = left;
          continue;
        }
        Value incoming[2] = {left.value, other.value};
        // Rebinding a Rule parameter makes a local payload. The untouched
        // arm observes that formal at its own use site before joining values.
        if (scope.bindings.count(name) && scope.bindings.at(name).parameter &&
            bool(isResource(incoming[0].getType())) !=
                bool(isResource(incoming[1].getType()))) {
          for (unsigned arm = 0; arm < 2; ++arm) {
            if (!isResource(incoming[arm].getType()))
              continue;
            OpBuilder::InsertionGuard save(b);
            b.setInsertionPointToEnd(&branch->getRegion(arm).front());
            auto loaded = read(incoming[arm], node, arms[arm]);
            if (failed(loaded))
              return failure();
            // This observation represents the untouched phi alternative.
            // Common analysis derives its demand from actual uses of the join.
            loaded->getDefiningOp()->setAttr("ac.parameter_join", b.getUnitAttr());
            incoming[arm] = *loaded;
          }
        }
        if (!incoming[0] || !incoming[1] ||
            isResource(incoming[0].getType()) ||
            isResource(incoming[1].getType()) || isa<ac::ModuleRefType>(left.type()) ||
            isa<ac::ModuleRefType>(other.type()))
          return fail(node, "conditional binding cannot select a different resource");
        mergedNames.push_back(name);
        values[0].push_back(incoming[0]);
        values[1].push_back(incoming[1]);
      }
    }
    // Result types are solved from both yields and their use contexts by the
    // common payload analysis, just like ordinary conditional expressions.
    b.setInsertionPoint(branch);
    SmallVector<Type> types =
        complete ? scope.resultTypes
                 : SmallVector<Type>(mergedNames.size(),
                                     ac::InferType::get(b.getContext()));
    if (!complete)
      for (unsigned i = 0; i < types.size(); ++i) {
        Type declared = values[0][i].getType();
        // Matching declared SSA types retain their field/slice contract;
        // differing types remain constraints for common analysis.
        if (!isa<ac::InferType>(declared) &&
            declared == values[1][i].getType())
          types[i] = declared;
      }
    auto *merged = op(node, "scf.if", selected, types, {}, 2);
    for (unsigned arm = 0; arm < 2; ++arm) {
      merged->getRegion(arm).takeBody(branch->getRegion(arm));
      b.setInsertionPointToEnd(&merged->getRegion(arm).front());
      for (auto [index, type] : llvm::enumerate(types))
        if (values[arm][index].getType() != type)
          values[arm][index] =
              op(node, "ac.payload.pending", values[arm][index], type,
                 {b.getNamedAttr("kind", b.getStringAttr("convert")),
                  b.getNamedAttr("ac.join_transport", b.getUnitAttr())})
                  ->getResult(0);
      op(node, "scf.yield", values[arm], {});
    }
    branch->erase();
    b.setInsertionPointAfter(merged);
    for (auto [name, value] : llvm::zip(mergedNames, merged->getResults()))
      scope.bindings[name] = Binding{value, {}, {}, nullptr};
    if (complete) {
      scope.returned.assign(merged->getResults().begin(),
                            merged->getResults().end());
      scope.terminated = true;
    }
    scope.bindings.insert(branchResources.begin(), branchResources.end());
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
    outputs.push_back(resource(decl.results[i]));
    outputNames.push_back(b.getStringAttr("out" + std::to_string(i)));
  }
  decl.operation =
      op(decl.node, "ac.module", {}, {},
         {b.getNamedAttr("sym_name", b.getStringAttr(decl.symbol)),
          b.getNamedAttr("source_owner", owner),
          b.getNamedAttr("parameters", b.getArrayAttr(decl.parameters)),
          b.getNamedAttr("type_parameters", b.getArrayAttr({})),
          b.getNamedAttr("function_type",
                         TypeAttr::get(b.getFunctionType(inputs, outputs))),
          b.getNamedAttr("input_names", b.getArrayAttr(inputNames)),
          b.getNamedAttr("output_names", b.getArrayAttr(outputNames)),
          b.getNamedAttr("output_kinds",
                         b.getArrayAttr(SmallVector<Attribute>(
                             outputs.size(), b.getStringAttr("bound")))),
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
  for (auto type : outputs)
    scope.outputs.push_back(block->addArgument(type, decl.operation->getLoc()));
  if (failed(statements(decl.node, "body", scope)))
    return failure();
  // Explicit zero-input Module declarations allocate even when their reference
  // is never called, preserving static hierarchy and autonomous Rules.
  for (auto &[name, binding] : scope.bindings) {
    if (binding.pendingImport) {
      if (scope.terminated)
        return fail(decl.node,
                    "connect instances before returning from Module");
      auto values = instantiate(nullptr, binding.pendingImport, name,
                                binding.declaration, scope);
      if (failed(values))
        return failure();
      binding.pendingImport = nullptr;
    }
    if (binding.pending) {
      if (scope.terminated)
        return fail(decl.node, "unconnected explicit instance must be "
                               "connected before Module return");
      if (!binding.pending->inputs.empty())
        return fail(decl.node, "explicit Module instance has unbound inputs");
      auto values = instantiate(binding.pending, nullptr, name,
                                binding.declaration, scope);
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
  if (decl.system) {
    decl.operation->setAttr("ac.root_kind", b.getStringAttr("system"));
    if (failed(systemSamples(decl)))
      return failure();
  }
  return success();
}

LogicalResult Importer::systemSamples(Declaration &decl) {
  auto decorator = decl.node.item("decorator_list", 0);
  if (decorator.kind() != "Call")
    return success();
  for (size_t i = 0; i < decorator.array("keywords").size(); ++i) {
    auto sequence = decorator.item("keywords", i).child("value");
    auto isSequence = [](const AstNode &node) {
      return node.kind() == "Tuple" || node.kind() == "List";
    };
    if (!isSequence(sequence) || sequence.array("elts").empty() ||
        sequence.array("elts").size() >
            ac::HardwareSourceCheckLimits{}.maxWorkUnits)
      return fail(sequence,
                  "System samples require a bounded nonempty sequence");
    SmallVector<Attribute> rows;
    for (size_t rowIndex = 0; rowIndex < sequence.array("elts").size();
         ++rowIndex) {
      auto row = sequence.item("elts", rowIndex);
      if (!charge(row))
        return failure();
      if (!isSequence(row) || row.array("elts").size() != 2)
        return fail(row, "System sample requires (clock, reset)");
      std::string pattern;
      for (size_t column = 0; column < 2; ++column) {
        auto value = row.item("elts", column);
        if (auto number = integer(value);
            number && (*number == 0 || *number == 1)) {
          pattern += *number == 0 ? '0' : '1';
          continue;
        }
        if (value.kind() != "Call" ||
            !marker(value.child("func"), "four_state") ||
            value.array("args").size() != 1 || !value.array("keywords").empty())
          return fail(value,
                      "System sample requires 0, 1 or one-bit four_state");
        auto text =
            dyn_cast_or_null<StringAttr>(value.item("args", 0).get("value"));
        auto bit = text ? text.getValue().lower() : std::string();
        if (bit.size() != 1 ||
            StringRef("01xz").find(bit[0]) == StringRef::npos)
          return fail(value,
                      "System sample requires a one-bit 0/1/x/z pattern");
        pattern += bit;
      }
      rows.push_back(b.getStringAttr(pattern));
    }
    decl.operation->setAttr(
        "ac.system_samples",
        b.getDictionaryAttr(
            {b.getNamedAttr("rows", b.getArrayAttr(rows)),
             b.getNamedAttr("origin", origin(sequence, decl.symbol)),
             b.getNamedAttr("location",
                            detail::sourceSpan(b, source.path, sequence))}));
  }
  return success();
}
