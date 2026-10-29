// Native expressions over explicit resources and ordinary SSA values.
FailureOr<Value> Importer::constant(const AstNode &node, Type type,
                                    const llvm::APSInt &number) {
  return op(node, "ac.payload.pending", {}, type,
            {b.getNamedAttr("kind", b.getStringAttr("literal")),
             b.getNamedAttr("number",
                            ac::MathIntAttr::get(b.getContext(), number))})
      ->getResult(0);
}

FailureOr<Value> Importer::convert(Value value, Type target,
                                   const AstNode &node) {
  if (equal(value.getType(), target))
    return value;
  return op(node, "ac.payload.pending", value, target,
            {b.getNamedAttr("kind", b.getStringAttr("convert"))})
      ->getResult(0);
}

FailureOr<Value> Importer::read(Value handle, const AstNode &node,
                                Scope &scope) {
  if (!isResource(handle.getType()))
    return handle;
  Binding *parameter = nullptr;
  for (auto &[name, binding] : scope.bindings)
    if (binding.parameter && binding.value == handle) {
      parameter = &binding;
      if (binding.cachedRead)
        return binding.cachedRead;
      break;
    }
  auto data = payload(handle.getType());
  auto *created =
      op(node, "ac.method.call", ValueRange{handle}, data,
         {b.getNamedAttr("callee", symbol("read")),
          b.getNamedAttr("occurrence", origin(node, scope.owner->symbol))});
  if (created && parameter)
    parameter->cachedRead = created->getResult(0);
  return created ? FailureOr<Value>(created->getResult(0))
                 : FailureOr<Value>(failure());
}

LogicalResult Importer::write(Value handle, Value value, const AstNode &node,
                              Scope &scope) {
  if (scope.pure || scope.module)
    return fail(node, "resource updates require a Rule or Method body");
  if (!isResource(handle.getType()))
    return fail(node, "assignment target is not a resource");
  auto converted = convert(value, payload(handle.getType()), node);
  if (failed(converted))
    return failure();
  auto *created =
      op(node, "ac.method.call", ValueRange{handle, *converted}, {},
         {b.getNamedAttr("callee", symbol("write")),
          b.getNamedAttr("occurrence", origin(node, scope.owner->symbol))});
  return success(created != nullptr);
}

FailureOr<Value> Importer::scalar(const AstNode &node, Scope &scope,
                                  Type expected) {
  auto savedRequirement = scope.pendingResultRequirement;
  auto savedArity = scope.pendingResultArity;
  scope.pendingResultRequirement = "scalar";
  scope.pendingResultArity = 1;
  auto restoreDemand = llvm::scope_exit([&] {
    scope.pendingResultRequirement = savedRequirement;
    scope.pendingResultArity = savedArity;
  });
  auto values = expression(node, scope, expected);
  if (failed(values))
    return failure();
  if (values->size() != 1) {
    (void)fail(node, "expression requires exactly one same-cycle payload");
    return failure();
  }
  Value value = values->front();
  if (isResource(value.getType())) {
    auto loaded = read(value, node, scope);
    if (failed(loaded))
      return failure();
    value = *loaded;
  }
  if (isa<ac::ModuleRefType>(value.getType())) {
    (void)fail(node, "instance references are not data payloads");
    return failure();
  }
  return expected ? convert(value, expected, node) : FailureOr<Value>(value);
}

FailureOr<SmallVector<Value>>
Importer::expression(const AstNode &node, Scope &scope, Type expected,
                     ArrayRef<Value> destinations) {
  if (!charge(node))
    return failure();
  if (auto number = integer(node)) {
    if (expected) {
      auto value = constant(node, expected, *number);
      if (failed(value))
        return failure();
      return SmallVector<Value>{*value};
    }
    return SmallVector<Value>{
        op(node, "ac.payload.pending", {}, ac::InferType::get(b.getContext()),
           {b.getNamedAttr("kind", b.getStringAttr("literal")),
            b.getNamedAttr("number",
                           ac::MathIntAttr::get(b.getContext(), *number))})
            ->getResult(0)};
  }
  if (node.kind() == "Name") {
    auto found = scope.bindings.find(node.string("id").str());
    if (found == scope.bindings.end()) {
      (void)fail(node, "unbound transactional local");
      return failure();
    }
    if (found->second.pending && !found->second.value) {
      auto pending = found->second.pending;
      if (!pending->inputs.empty()) {
        (void)fail(
            node,
            "instance must be connected before it is used as a reference");
        return failure();
      }
      auto values = instantiate(pending, nullptr, found->second.instanceName,
                                found->second.declaration, scope);
      if (failed(values))
        return failure();
      auto refType =
          ac::ModuleRefType::get(b.getContext(), symbol(pending->symbol));
      auto *ref = op(
          node, "ac.instance.ref", {}, refType,
          {b.getNamedAttr("instance_path", b.getArrayAttr({b.getStringAttr(
                                               found->second.instanceName)}))});
      found->second.value = ref->getResult(0);
      found->second.pending = nullptr;
    }
    if (found->second.pendingImport) {
      auto *definition = found->second.pendingImport;
      auto values = instantiate(nullptr, definition, found->second.instanceName,
                                found->second.declaration, scope);
      if (failed(values))
        return failure();
      auto type = ac::ModuleRefType::get(b.getContext(),
                                         symbol(found->second.instanceTarget));
      found->second.value =
          op(node, "ac.instance.ref", {}, type,
             {b.getNamedAttr("instance_path",
                             b.getArrayAttr({b.getStringAttr(
                                 found->second.instanceName)}))})
              ->getResult(0);
      found->second.pendingImport = nullptr;
    }
    if (!found->second.value)
      return failure();
    return SmallVector<Value>{found->second.value};
  }
  if (node.kind() == "Tuple") {
    auto savedRequirement = scope.pendingResultRequirement;
    auto savedArity = scope.pendingResultArity;
    scope.pendingResultRequirement = "scalar";
    scope.pendingResultArity = 1;
    auto restoreDemand = llvm::scope_exit([&] {
      scope.pendingResultRequirement = savedRequirement;
      scope.pendingResultArity = savedArity;
    });

    SmallVector<Value> values;
    for (size_t i = 0; i < node.array("elts").size(); ++i) {
      auto item = expression(node.item("elts", i), scope);
      if (failed(item) || item->size() != 1) {
        (void)fail(node, "tuple elements must be scalar");
        return failure();
      }
      values.push_back(item->front());
    }
    return values;
  }
  if (node.kind() == "Attribute") {
    auto binding = resolve(node.child("value"));
    if (binding && isa_and_nonnull<ac::EnumOp>(binding->declaration)) {
      auto type = ac::EnumType::get(
          b.getContext(), b.getStringAttr(binding->symbol.getValue()));
      return SmallVector<Value>{
          op(node, "ac.enum.create", {}, type,
             {b.getNamedAttr("member", b.getStringAttr(node.string("attr")))})
              ->getResult(0)};
    }
    auto base = scalar(node.child("value"), scope);
    if (failed(base))
      return failure();
    if (isa<ac::InferType>(base->getType()))
      return SmallVector<Value>{
          op(node, "ac.payload.pending", *base,
             ac::InferType::get(b.getContext()),
             {b.getNamedAttr("kind", b.getStringAttr("field")),
              b.getNamedAttr("opcode", b.getStringAttr(node.string("attr")))})
              ->getResult(0)};
    auto structure = dyn_cast<ac::StructType>(base->getType());
    Type type;
    if (structure) {
      auto *definition =
          SymbolTable::lookupSymbolIn(*result, structure.getName().getValue());
      if (definition)
        for (auto raw : definition->getAttrOfType<ArrayAttr>("fields")) {
          auto field = cast<DictionaryAttr>(raw);
          if (field.getAs<StringAttr>("name").getValue() == node.string("attr"))
            type = field.getAs<TypeAttr>("type").getValue();
        }
    }
    if (!structure || !type) {
      (void)fail(node, "field access requires a declared Struct field");
      return failure();
    }
    return SmallVector<Value>{
        op(node, "ac.struct.get", *base, type,
           {b.getNamedAttr("field", b.getStringAttr(node.string("attr")))})
            ->getResult(0)};
  }
  if (node.kind() == "Call")
    return invoke(node, scope, expected, destinations);
  if (node.kind() == "BinOp" || node.kind() == "Compare") {
    bool compare = node.kind() == "Compare";
    if (compare && (node.array("ops").size() != 1 ||
                    node.array("comparators").size() != 1)) {
      (void)fail(node,
                 "chained comparisons require explicit Boolean expressions");
      return failure();
    }
    AstNode left = node.child("left"),
            right = compare ? node.item("comparators", 0) : node.child("right");
    auto lhs = scalar(left, scope), rhs = scalar(right, scope);
    if (failed(lhs) || failed(rhs))
      return failure();
    StringRef operation =
        compare ? node.item("ops", 0).kind() : node.child("op").kind();
    std::string opcode;
    for (auto item : {std::pair<StringRef, StringRef>{"Add", "add"},
                      {"Sub", "sub"},
                      {"Mult", "mul"},
                      {"BitAnd", "and"},
                      {"BitOr", "or"},
                      {"BitXor", "xor"},
                      {"LShift", "shl"},
                      {"RShift", "lshr"},
                      {"Eq", "eq"},
                      {"NotEq", "ne"},
                      {"Lt", "ult"},
                      {"LtE", "ule"},
                      {"Gt", "ugt"},
                      {"GtE", "uge"}})
      if (item.first == operation)
        opcode = item.second.str();
    if (opcode.empty()) {
      (void)fail(node, "unsupported transactional binary operation");
      return failure();
    }
    Type output =
        compare ? bits(1, node) : Type(ac::InferType::get(b.getContext()));
    return SmallVector<Value>{
        op(node, "ac.payload.pending", ValueRange{*lhs, *rhs}, output,
           {b.getNamedAttr("kind",
                           b.getStringAttr(compare ? "compare" : "binary")),
            b.getNamedAttr("opcode", b.getStringAttr(opcode))})
            ->getResult(0)};
  }
  if (node.kind() == "UnaryOp") {
    StringRef kind = node.child("op").kind();
    auto value = scalar(node.child("operand"), scope,
                        kind == "Not" ? bits(1, node) : Type());
    if (failed(value))
      return failure();
    if (kind != "Not" && kind != "Invert" && kind != "USub") {
      (void)fail(node, "unsupported unary operation");
      return failure();
    }
    return SmallVector<Value>{
        op(node, "ac.payload.pending", *value,
           ac::InferType::get(b.getContext()),
           {b.getNamedAttr("kind", b.getStringAttr("unary")),
            b.getNamedAttr("opcode",
                           b.getStringAttr(kind == "USub" ? "neg" : "not"))})
            ->getResult(0)};
  }
  if (node.kind() == "BoolOp") {
    auto items = node.array("values");
    if (items.size() < 2)
      return failure();
    auto first = scalar(node.item("values", 0), scope, bits(1, node));
    if (failed(first))
      return failure();
    Value value = *first;
    // Source Boolean short circuiting is a real branch, preserving required
    // control reachedness and dependency availability.
    for (size_t i = 1; i < items.size(); ++i) {
      bool conjunction = node.child("op").kind() == "And";
      auto *branch = op(
          node, "ac.branch", value, bits(1, node),
          {b.getNamedAttr("occurrence", origin(node, scope.owner->symbol))}, 2);
      for (unsigned arm = 0; arm < 2; ++arm) {
        auto *block = new Block;
        branch->getRegion(arm).push_back(block);
        OpBuilder::InsertionGuard save(b);
        b.setInsertionPointToEnd(block);
        if ((arm == 0) == conjunction) {
          auto next = scalar(node.item("values", i), scope, bits(1, node));
          if (failed(next))
            return failure();
          yield(node, *next);
        } else
          yield(node, truth(node, !conjunction));
      }
      value = branch->getResult(0);
    }
    return SmallVector<Value>{value};
  }
  if (node.kind() == "IfExp") {
    auto condition = scalar(node.child("test"), scope, bits(1, node));
    if (failed(condition))
      return failure();
    Scope pure = scope;
    pure.pure = true;
    auto selected = scalar(node.child("body"), pure);
    auto alternative = scalar(node.child("orelse"), pure);
    if (failed(selected) || failed(alternative))
      return failure();
    return SmallVector<Value>{
        op(node, "ac.payload.pending",
           ValueRange{*condition, *selected, *alternative},
           ac::InferType::get(b.getContext()),
           {b.getNamedAttr("kind", b.getStringAttr("select"))})
            ->getResult(0)};
  }
  (void)fail(node, "unsupported expression in transactional native frontend");
  return failure();
}

FailureOr<SmallVector<Value>> Importer::invoke(const AstNode &node,
                                               Scope &scope, Type expected,
                                               ArrayRef<Value> destinations) {
  AstNode func = node.child("func");
  auto resolved = resolve(func);
  if (func.kind() == "Subscript" && marker(func.child("value"), "queue")) {
    auto data = annotation(func.child("slice"), scope.owner->symbol);
    if (!data || !node.array("args").empty())
      return failure();
    unsigned depth = 0;
    for (size_t i = 0; i < node.array("keywords").size(); ++i) {
      auto keyword = node.item("keywords", i);
      auto number = integer(keyword.child("value"));
      if (keyword.string("arg") != "depth" || depth || !number ||
          number->isNegative() || number->isZero() ||
          number->getActiveBits() > 31)
        return (void)fail(keyword,
                          "Queue depth must be a positive static integer"),
               failure();
      depth = number->getZExtValue();
    }
    if (!depth)
      return (void)fail(node, "Queue allocation requires a positive depth"),
             failure();
    auto value = allocate(data, "$queue" + std::to_string(unique++), node,
                          scope, {}, depth);
    if (failed(value))
      return failure();
    return SmallVector<Value>{*value};
  }
  if (resolved &&
      isa_and_nonnull<ac::TransactionRuleOp>(resolved->declaration)) {
    auto *definition = resolved->declaration;
    Callable call;
    call.kind = "rule";
    call.name = resolved->symbol.getValue().str();
    call.global = call.defined = true;
    auto type = cast<FunctionType>(
        definition->getAttrOfType<TypeAttr>("function_type").getValue());
    auto names = definition->getAttrOfType<ArrayAttr>("input_names");
    if (!names || names.size() != type.getNumInputs() ||
        !llvm::all_of(names,
                      [](Attribute name) { return isa<StringAttr>(name); }))
      return (void)fail(node, "published Rule requires its parameter names"),
             failure();
    for (unsigned i = 0; i < type.getNumInputs(); ++i)
      call.inputs.push_back(
          {cast<StringAttr>(names[i]).getValue().str(), type.getInput(i), {}});
    call.results.append(type.getResults().begin(), type.getResults().end());
    return registerRule(call, node, scope, destinations);
  }
  // Payload constructors are ordinary combinational values.
  bool bitsConstructor =
      (resolved && resolved->module == "pycircuit" &&
       detail::fixedBitsAliasWidth(resolved->module, resolved->remote)
           .has_value()) ||
      (func.kind() == "Subscript" && marker(func.child("value"), "bits"));
  if (bitsConstructor) {
    auto type = annotation(func, scope.owner->symbol);
    if (!type || node.array("args").size() != 1 ||
        !node.array("keywords").empty()) {
      (void)fail(node, "Bits construction requires one payload");
      return failure();
    }
    auto argument = node.item("args", 0);
    if (auto number = integer(argument)) {
      auto value = constant(argument, type, *number);
      if (failed(value))
        return failure();
      return SmallVector<Value>{*value};
    }
    auto value = scalar(argument, scope);
    if (failed(value))
      return failure();
    auto converted = convert(*value, type, node);
    if (failed(converted))
      return failure();
    return SmallVector<Value>{*converted};
  }
  if (resolved && isa_and_nonnull<ac::StructOp>(resolved->declaration)) {
    auto fields = resolved->declaration->getAttrOfType<ArrayAttr>("fields");
    SmallVector<StringRef> formals;
    for (auto attr : fields)
      formals.push_back(
          cast<DictionaryAttr>(attr).getAs<StringAttr>("name").getValue());
    auto args = detail::normalizeCallArguments(node, formals, error);
    if (failed(args))
      return failure();
    SmallVector<Value> values;
    for (size_t i = 0; i < args->size(); ++i) {
      auto value = scalar(
          (*args)[i], scope,
          cast<DictionaryAttr>(fields[i]).getAs<TypeAttr>("type").getValue());
      if (failed(value))
        return failure();
      values.push_back(*value);
    }
    auto type = ac::StructType::get(
        b.getContext(), b.getStringAttr(resolved->symbol.getValue()));
    return SmallVector<Value>{
        op(node, "ac.struct.create", values, type)->getResult(0)};
  }
  if (marker(func, "four_state")) {
    if (node.array("args").size() != 1 || !node.array("keywords").empty()) {
      (void)fail(node, "four_state requires one literal pattern");
      return failure();
    }
    auto pattern = node.item("args", 0).get("value");
    auto text = dyn_cast_or_null<StringAttr>(pattern);
    if (!text || text.getValue().empty() || text.getValue().size() > MaxWidth) {
      (void)fail(node, "four_state requires a finite literal pattern");
      return failure();
    }
    return SmallVector<Value>{op(node, "ac.bits.constant4", {},
                                 bits(text.getValue().size(), node),
                                 {b.getNamedAttr("pattern", text)})
                                  ->getResult(0)};
  }
  if (marker(func, "case_eq")) {
    if (node.array("args").size() != 2 || !node.array("keywords").empty()) {
      (void)fail(node, "case_eq requires two payloads");
      return failure();
    }
    auto lhs = scalar(node.item("args", 0), scope),
         rhs = scalar(node.item("args", 1), scope);
    if (failed(lhs) || failed(rhs))
      return failure();
    return SmallVector<Value>{
        op(node, "ac.payload.pending", ValueRange{*lhs, *rhs}, bits(1, node),
           {b.getNamedAttr("kind", b.getStringAttr("compare")),
            b.getNamedAttr("opcode", b.getStringAttr("case_eq"))})
            ->getResult(0)};
  }
  if (func.kind() == "Attribute") {
    auto base = func.child("value");
    if (base.kind() == "Name") {
      auto binding = scope.bindings.find(base.string("id").str());
      if (binding != scope.bindings.end() &&
          isResource(binding->second.type())) {
        auto resourcePayload = payload(binding->second.type());
        auto method = func.string("attr");
        if (method == "read" || method == "head" || method == "peek" || method == "pop") {
          if (!node.array("args").empty() || !node.array("keywords").empty()) {
            (void)fail(node, "resource read takes no arguments");
            return failure();
          }
          if (scope.pure && method == "pop") {
            (void)fail(node, "pure region cannot consume a Queue");
            return failure();
          }
          Scope access = scope;
          access.pure = scope.pure || method == "peek" || method == "head";
          auto *accessOp =
              op(node, "ac.method.call", ValueRange{binding->second.value},
                 resourcePayload,
                 {b.getNamedAttr("callee", symbol(method)),
                  b.getNamedAttr("occurrence",
                                 origin(node, scope.owner->symbol))});
          if (!accessOp)
            return failure();
          return SmallVector<Value>{accessOp->getResult(0)};
        }
        if (method == "write" || method == "push") {
          if (node.array("args").size() != 1 ||
              !node.array("keywords").empty()) {
            (void)fail(node, "resource update requires one payload");
            return failure();
          }
          auto value = scalar(node.item("args", 0), scope, resourcePayload);
          if (failed(value))
            return failure();
          if (scope.pure || scope.module)
            return (void)fail(node,
                              "resource updates require a Rule or Method body"),
                   failure();
          if (!op(node, "ac.method.call",
                  ValueRange{binding->second.value, *value}, {},
                  {b.getNamedAttr("callee", symbol(method)),
                   b.getNamedAttr("occurrence",
                                  origin(node, scope.owner->symbol))}))
            return failure();
          return SmallVector<Value>{};
        }
      }
    }
    return referenceCall(node, scope, expected);
  }
  if (func.kind() == "Name") {
    auto name = func.string("id").str();
    auto binding = scope.bindings.find(name);
    if (binding != scope.bindings.end() && binding->second.pendingImport) {
      auto values =
          instantiate(nullptr, binding->second.pendingImport,
                      binding->second.instanceName, node, scope, destinations);
      if (failed(values))
        return failure();
      auto type = ac::ModuleRefType::get(
          b.getContext(), symbol(binding->second.instanceTarget));
      auto *ref = op(node, "ac.instance.ref", {}, type,
                     {b.getNamedAttr("instance_path",
                                     b.getArrayAttr({b.getStringAttr(
                                         binding->second.instanceName)}))});
      binding->second.value = ref->getResult(0);
      binding->second.pendingImport = nullptr;
      return values;
    }
    if (binding != scope.bindings.end() && binding->second.pending) {
      auto *decl = binding->second.pending;
      auto values = instantiate(decl, nullptr, binding->second.instanceName,
                                node, scope, destinations);
      if (failed(values))
        return failure();
      auto type = ac::ModuleRefType::get(b.getContext(), symbol(decl->symbol));
      auto *ref = op(node, "ac.instance.ref", {}, type,
                     {b.getNamedAttr("instance_path",
                                     b.getArrayAttr({b.getStringAttr(
                                         binding->second.instanceName)}))});
      binding->second.value = ref->getResult(0);
      binding->second.pending = nullptr;
      return values;
    }
    if (binding != scope.bindings.end()) {
      (void)fail(
          node,
          "bound local is not callable, or this instance is already connected");
      return failure();
    }
    auto nested = scope.owner->callables.find(name);
    if (!scope.globalRule && nested != scope.owner->callables.end()) {
      if (nested->second.kind == "method")
        return methodCall(nested->second, node, scope);
      if (nested->second.autonomous) {
        (void)fail(node, "autorule cannot be explicitly registered again");
        return failure();
      }
      return registerRule(nested->second, node, scope, destinations);
    }
    auto external = globalRules.find(name);
    if (external != globalRules.end())
      return registerRule(external->second, node, scope, destinations);
    auto decl = declarations.find(name);
    if (decl != declarations.end())
      return instantiate(decl->second.get(), nullptr,
                         "instance_" + std::to_string(unique++), node, scope,
                         destinations);
  }
  if (resolved && isa_and_nonnull<ac::ModuleImportOp>(resolved->declaration))
    return instantiate(nullptr, resolved->declaration,
                       "instance_" + std::to_string(unique++), node, scope,
                       destinations);
  (void)fail(node, "unsupported callable in transactional source");
  return failure();
}

FailureOr<SmallVector<Value>>
Importer::methodCall(Callable &call, const AstNode &node, Scope &scope) {
  if (scope.module)
    return (void)fail(node, "Module combination cannot call a Method"),
           failure();
  SmallVector<StringRef> names;
  for (const auto &input : call.inputs)
    names.push_back(input.name);
  auto actuals = detail::normalizeCallArguments(node, names, error);
  if (failed(actuals))
    return failure();
  SmallVector<Value> operands{scope.bindings.at("$self").value};
  for (auto [actual, formal] : llvm::zip(*actuals, call.inputs)) {
    auto value = scalar(actual, scope, formal.type);
    if (failed(value))
      return failure();
    operands.push_back(*value);
  }
  auto *invoke =
      op(node, "ac.method.call", operands, call.results,
         {b.getNamedAttr("callee", symbol(call.name)),
          b.getNamedAttr("occurrence", origin(node, scope.owner->symbol))});
  return SmallVector<Value>(invoke->getResults());
}

FailureOr<SmallVector<Value>>
Importer::referenceCall(const AstNode &node, Scope &scope, Type expected) {
  if (scope.module || !node.array("keywords").empty())
    return (void)fail(
               node,
               "reference Methods require transactional positional calls"),
           failure();
  auto func = node.child("func");
  auto values = expression(func.child("value"), scope);
  if (failed(values) || values->size() != 1)
    return failure();
  auto receiver = values->front();
  auto ref = dyn_cast<ac::ModuleRefType>(receiver.getType());
  if (!ref)
    return (void)fail(node, "Method receiver requires a Module reference"),
           failure();
  Operation *target = nullptr;
  for (Operation &decl : *result->getBody())
    if (decl.getAttrOfType<StringAttr>("sym_name") &&
        decl.getAttrOfType<StringAttr>("sym_name").getValue() ==
            ref.getModule().getValue())
      target = &decl;
  if (!target)
    return (void)fail(node, "missing Module reference declaration"), failure();
  auto name = func.string("attr");
  auto methods = ac::getTransactionalMethodInterfaces(target);
  DictionaryAttr prior;
  for (auto attr : methods) {
    auto item = cast<DictionaryAttr>(attr);
    if (item.getAs<StringAttr>("name").getValue() == name)
      prior = item;
  }
  bool requirement = ac::isTransactionalReferenceRequirement(target);
  if (!prior && !requirement)
    return (void)fail(node, "receiver does not publish requested Method"),
           failure();
  FunctionType signature;
  if (prior && !requirement)
    signature =
        cast<FunctionType>(prior.getAs<TypeAttr>("function_type").getValue());
  if (signature && signature.getNumInputs() != node.array("args").size())
    return (void)fail(node, "Method argument arity differs"), failure();
  SmallVector<Value> operands{receiver};
  SmallVector<Type> outputs;
  for (size_t i = 0; i < node.array("args").size(); ++i) {
    auto value = scalar(node.item("args", i), scope,
                        signature ? signature.getInput(i) : Type());
    if (failed(value))
      return failure();
    operands.push_back(*value);
  }
  SmallVector<NamedAttribute> attrs{
      b.getNamedAttr("callee", symbol(name)),
      b.getNamedAttr("occurrence", origin(node, scope.owner->symbol))};
  if (signature)
    llvm::append_range(outputs, signature.getResults());
  else {
    if (scope.pendingResultRequirement != "ignored")
      for (unsigned i = 0; i < scope.pendingResultArity; ++i)
        outputs.push_back(ac::InferType::get(b.getContext()));
    attrs.push_back(b.getNamedAttr(
        "result_requirement", b.getStringAttr(scope.pendingResultRequirement)));
    attrs.push_back(b.getNamedAttr("read_only", b.getBoolAttr(scope.pure)));
  }
  auto *call = op(node, signature ? "ac.method.call" : "ac.method.pending",
                  operands, outputs, attrs);
  if (scope.pendingResultRequirement == "ignored")
    return SmallVector<Value>{};
  return SmallVector<Value>(call->getResults());
}
