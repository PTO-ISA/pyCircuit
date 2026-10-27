// Native expressions over explicit resources and ordinary SSA values.
FailureOr<Value> Importer::constant(const AstNode &node, Type type,
                                    const llvm::APSInt &number) {
  unsigned size = width(type);
  if (!isa<ac::BitsType>(type) || !size) {
    (void)fail(node, "integer literal requires a Bits context");
    return failure();
  }
  if ((!number.isNegative() && number.getActiveBits() > size) ||
      (number.isNegative() && number.getSignificantBits() > size)) {
    (void)fail(node, "integer literal does not fit its declared Bits context");
    return failure();
  }
  llvm::APInt value =
      number.isSigned() ? number.sextOrTrunc(size) : number.zextOrTrunc(size);
  std::string pattern(size, '0');
  for (unsigned i = 0; i < size; ++i)
    if (value[i])
      pattern[size - 1 - i] = '1';
  auto *created = op(node, "ac.bits.constant4", {}, type,
                     {b.getNamedAttr("pattern", b.getStringAttr(pattern))});
  return created ? FailureOr<Value>(created->getResult(0))
                 : FailureOr<Value>(failure());
}

FailureOr<Value> Importer::convert(Value value, Type target,
                                   const AstNode &node, bool allowNarrow) {
  if (equal(value.getType(), target))
    return value;
  if (!isa<ac::BitsType>(value.getType()) || !isa<ac::BitsType>(target)) {
    (void)fail(node, "nominal payload types must match exactly");
    return failure();
  }
  unsigned from = width(value.getType()), to = width(target);
  if (!from || !to)
    return failure();
  if (to < from && !allowNarrow) {
    (void)fail(node,
               "Bits narrowing requires an explicit slice; source slicing "
               "is not yet supported by this profile");
    return failure();
  }
  auto *created = op(
      node, "ac.bits.resize", value, target,
      {b.getNamedAttr("mode", b.getStringAttr(to < from ? "trunc" : "zext"))});
  return created ? FailureOr<Value>(created->getResult(0))
                 : FailureOr<Value>(failure());
}

FailureOr<Value> Importer::read(Value handle, const AstNode &node,
                                Scope &scope) {
  auto type = dyn_cast<ac::StateType>(handle.getType());
  if (!type)
    return handle;
  auto *created =
      op(node, "ac.method.call", ValueRange{handle}, type.getPayload(),
         {b.getNamedAttr("callee", symbol("read")),
          b.getNamedAttr("occurrence", origin(node, scope.owner->symbol))});
  return created ? FailureOr<Value>(created->getResult(0))
                 : FailureOr<Value>(failure());
}

LogicalResult Importer::write(Value handle, Value value, const AstNode &node,
                              Scope &scope) {
  if (scope.pure || scope.module)
    return fail(node, "resource updates require a Rule or Method body");
  auto type = dyn_cast<ac::StateType>(handle.getType());
  if (!type)
    return fail(node, "assignment target is not a resource");
  auto converted = convert(value, type.getPayload(), node);
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
  auto values = expression(node, scope, expected);
  if (failed(values))
    return failure();
  if (values->size() != 1) {
    (void)fail(node, "expression requires exactly one same-cycle payload");
    return failure();
  }
  Value value = values->front();
  if (isa<ac::StateType>(value.getType())) {
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

Type Importer::infer(const AstNode &node, Scope &scope, Type expected) {
  if (!node)
    return {};
  if (node.kind() == "Name") {
    auto found = scope.bindings.find(node.string("id").str());
    if (found != scope.bindings.end() && found->second.type())
      return payload(found->second.type());
    auto hint = scope.hints.find(node.string("id").str());
    return hint == scope.hints.end() ? expected : hint->second;
  }
  if (node.kind() == "Compare" || node.kind() == "BoolOp" ||
      (node.kind() == "UnaryOp" && node.child("op").kind() == "Not"))
    return bits(1, node);
  if (node.kind() == "UnaryOp")
    return infer(node.child("operand"), scope, expected);
  if (node.kind() == "BinOp") {
    Type left = infer(node.child("left"), scope),
         right = infer(node.child("right"), scope);
    if (node.child("op").kind() == "LShift" ||
        node.child("op").kind() == "RShift")
      return left ? left : expected;
    if (left && right && isa<ac::BitsType>(left) && isa<ac::BitsType>(right))
      return width(left) >= width(right) ? left : right;
    return left ? left : right ? right : expected;
  }
  if (node.kind() == "IfExp")
    return infer(node.child("body"), scope,
                 infer(node.child("orelse"), scope, expected));
  if (node.kind() == "Attribute") {
    auto binding = resolve(node.child("value"));
    if (binding && isa_and_nonnull<ac::EnumOp>(binding->declaration))
      return ac::EnumType::get(b.getContext(),
                               b.getStringAttr(binding->symbol.getValue()));
    Type base = infer(node.child("value"), scope);
    if (auto structure = dyn_cast_or_null<ac::StructType>(base)) {
      auto *definition =
          SymbolTable::lookupSymbolIn(*result, structure.getName().getValue());
      if (definition)
        for (auto field : definition->getAttrOfType<ArrayAttr>("fields")) {
          auto item = cast<DictionaryAttr>(field);
          if (item.getAs<StringAttr>("name").getValue() == node.string("attr"))
            return item.getAs<TypeAttr>("type").getValue();
        }
    }
  }
  if (node.kind() == "Call") {
    auto func = node.child("func");
    auto binding = resolve(func);
    if (binding && (binding->module == "pycircuit" ||
                    isa_and_nonnull<ac::StructOp>(binding->declaration))) {
      if (binding->remote != "module" && binding->remote != "queue") {
        if (binding->module == "pycircuit" && binding->remote != "bits" &&
            !StringRef(binding->remote).starts_with("u"))
          return expected;
        return annotation(func, scope.owner->symbol);
      }
    }
    if (func.kind() == "Subscript" && marker(func.child("value"), "bits"))
      return annotation(func, scope.owner->symbol);
    if (func.kind() == "Name") {
      auto local = scope.owner->callables.find(func.string("id").str());
      if (!scope.globalRule && local != scope.owner->callables.end() &&
          local->second.kind == "method" && local->second.results.size() == 1)
        return local->second.results[0];
    }
  }
  return expected;
}

LogicalResult Importer::inferHints(const AstNode &node, Scope &scope) {
  // Backward constraints come only from declared results and assignment types.
  // Iterate to a fixed point over local aliases; no source-name conventions.
  std::function<void(const AstNode &, Type)> constrain =
      [&](const AstNode &expr, Type type) {
        if (!expr || !type)
          return;
        if (expr.kind() == "Name")
          scope.hints.emplace(expr.string("id").str(), type);
        else if (expr.kind() == "BinOp") {
          constrain(expr.child("left"), type);
          constrain(expr.child("right"), type);
        } else if (expr.kind() == "IfExp") {
          constrain(expr.child("body"), type);
          constrain(expr.child("orelse"), type);
        }
      };
  std::function<void(const AstNode &)> visit = [&](const AstNode &at) {
    if (at.kind() == "Return") {
      auto value = at.child("value");
      if (scope.resultTypes.size() == 1)
        constrain(value, scope.resultTypes[0]);
      else if (value.kind() == "Tuple")
        for (size_t i = 0;
             i < std::min(value.array("elts").size(), scope.resultTypes.size());
             ++i)
          constrain(value.item("elts", i), scope.resultTypes[i]);
    }
    if (at.kind() == "Assign" && at.array("targets").size() == 1) {
      auto target = at.item("targets", 0);
      if (target.kind() == "Name") {
        auto name = target.string("id").str();
        auto found = scope.hints.find(name);
        Type type = found == scope.hints.end() ? Type() : found->second;
        auto binding = scope.bindings.find(name);
        if (binding != scope.bindings.end() && binding->second.type())
          type = payload(binding->second.type());
        constrain(at.child("value"), type);
        auto inferred = infer(at.child("value"), scope, type);
        if (inferred)
          scope.hints.emplace(name, inferred);
      }
    }
    if (at.kind() != "FunctionDef" || at.value == node.value)
      for (StringRef field : {"body", "orelse"})
        for (size_t i = 0; i < (at.array(field) ? at.array(field).size() : 0);
             ++i)
          visit(at.item(field, i));
  };
  for (unsigned i = 0; i < 4; ++i)
    visit(node);
  return success();
}

FailureOr<SmallVector<Value>>
Importer::expression(const AstNode &node, Scope &scope, Type expected,
                     ArrayRef<Value> destinations) {
  if (!charge(node))
    return failure();
  if (auto number = integer(node)) {
    auto value = constant(node, expected ? expected : bits(32, node), *number);
    if (failed(value))
      return failure();
    return SmallVector<Value>{*value};
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
      auto values =
          instantiate(pending, nullptr, found->second.instanceName, found->second.declaration, scope);
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
      auto values = instantiate(nullptr, definition, found->second.instanceName, found->second.declaration, scope);
      if (failed(values)) return failure();
      auto type = ac::ModuleRefType::get(b.getContext(), symbol(found->second.instanceTarget));
      found->second.value = op(node, "ac.instance.ref", {}, type,
          {b.getNamedAttr("instance_path", b.getArrayAttr({b.getStringAttr(found->second.instanceName)}))})->getResult(0);
      found->second.pendingImport = nullptr;
    }
    if (!found->second.value)
      return failure();
    return SmallVector<Value>{found->second.value};
  }
  if (auto number = integer(node)) {
    Type type = expected ? expected : bits(32, node);
    auto value = constant(node, type, *number);
    if (failed(value))
      return failure();
    return SmallVector<Value>{*value};
  }
  if (node.kind() == "Tuple") {
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
    auto structure = dyn_cast<ac::StructType>(base->getType());
    Type type = infer(node, scope);
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
    Type type = infer(left, scope), rhsType = infer(right, scope);
    bool shift = !compare && (node.child("op").kind() == "LShift" ||
                              node.child("op").kind() == "RShift");
    Type natural = type ? type : expected;
    if (!type)
      type = rhsType;
    if (type && rhsType && isa<ac::BitsType>(type) &&
        isa<ac::BitsType>(rhsType) && width(rhsType) > width(type))
      type = rhsType;
    if (!type)
      type = compare ? bits(32, node) : expected ? expected : bits(32, node);
    auto lhs = scalar(left, scope, type), rhs = scalar(right, scope, type);
    if (failed(lhs) || failed(rhs))
      return failure();
    if (auto enumeration = dyn_cast<ac::EnumType>(type)) {
      if (!compare) {
        (void)fail(node,
                   "Enum arithmetic requires an explicit carrier conversion");
        return failure();
      }
      Type carrier = bits(width(enumeration), node);
      lhs = op(node, "ac.enum.to_bits", *lhs, carrier)->getResult(0);
      rhs = op(node, "ac.enum.to_bits", *rhs, carrier)->getResult(0);
      type = carrier;
    }
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
    if (opcode.empty() || !isa<ac::BitsType>(type)) {
      (void)fail(node, "unsupported transactional binary operation");
      return failure();
    }
    Type output = compare ? bits(1, node) : type;
    auto *created = op(node, compare ? "ac.bits.compare" : "ac.bits.binary",
                       ValueRange{*lhs, *rhs}, output,
                       {b.getNamedAttr(compare ? "predicate" : "opcode",
                                       b.getStringAttr(opcode))});
    Value value = created->getResult(0);
    if (shift && natural && !equal(natural, value.getType())) {
      auto restored = convert(value, natural, node, true);
      if (failed(restored))
        return failure();
      value = *restored;
    }
    return SmallVector<Value>{value};
  }
  if (node.kind() == "UnaryOp") {
    StringRef kind = node.child("op").kind();
    Type natural = kind == "Not"
                       ? bits(1, node)
                       : infer(node.child("operand"), scope, expected);
    auto value = scalar(node.child("operand"), scope, natural);
    if (failed(value))
      return failure();
    if (kind != "Not" && kind != "Invert" && kind != "USub") {
      (void)fail(node, "unsupported unary operation");
      return failure();
    }
    return SmallVector<Value>{
        op(node, "ac.bits.unary", *value, value->getType(),
           {b.getNamedAttr("opcode",
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
    Type type = infer(node, scope, expected);
    if (!type) {
      (void)fail(node, "conditional payload needs a declared type constraint");
      return failure();
    }
    Scope pure = scope;
    pure.pure = true;
    auto selected = scalar(node.child("body"), pure, type);
    auto alternative = scalar(node.child("orelse"), pure, type);
    if (failed(selected) || failed(alternative))
      return failure();
    if (!isa<ac::BitsType>(type)) {
      (void)fail(node,
                 "conditional expression currently requires Bits payloads");
      return failure();
    }
    return SmallVector<Value>{
        op(node, "ac.bits.select",
           ValueRange{*condition, *selected, *alternative}, type)
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
    (void)fail(node, "Queue allocation awaits transactional resource binding");
    return failure();
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
    Type type =
        infer(node.item("args", 0), scope, infer(node.item("args", 1), scope));
    if (!type) {
      (void)fail(node, "case_eq needs a declared payload type");
      return failure();
    }
    auto lhs = scalar(node.item("args", 0), scope, type),
         rhs = scalar(node.item("args", 1), scope, type);
    if (failed(lhs) || failed(rhs))
      return failure();
    return SmallVector<Value>{
        op(node, "ac.bits.compare", ValueRange{*lhs, *rhs}, bits(1, node),
           {b.getNamedAttr("predicate", b.getStringAttr("case_eq")),
            b.getNamedAttr("ac.verification_observer",
                           b.getStringAttr("case_eq"))})
            ->getResult(0)};
  }
  if (func.kind() == "Attribute") {
    auto base = func.child("value");
    if (base.kind() == "Name") {
      auto binding = scope.bindings.find(base.string("id").str());
      if (binding != scope.bindings.end() &&
          isa_and_nonnull<ac::StateType>(binding->second.type())) {
        auto resourceType = cast<ac::StateType>(binding->second.type());
        auto method = func.string("attr");
        if (method == "read") {
          if (!node.array("args").empty() || !node.array("keywords").empty()) {
            (void)fail(node, "resource read takes no arguments");
            return failure();
          }
          if (scope.pure && method == "pop") {
            (void)fail(node, "pure region cannot consume a Queue");
            return failure();
          }
          Scope access = scope;
          access.pure = scope.pure || method == "peek";
          auto value = read(binding->second.value, node, access);
          if (failed(value))
            return failure();
          return SmallVector<Value>{*value};
        }
        if (method == "write") {
          if (node.array("args").size() != 1 ||
              !node.array("keywords").empty()) {
            (void)fail(node, "resource update requires one payload");
            return failure();
          }
          auto value =
              scalar(node.item("args", 0), scope, resourceType.getPayload());
          if (failed(value) ||
              failed(write(binding->second.value, *value, node, scope)))
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
    return (void)fail(node, "Module combination cannot call a Method"), failure();
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
  auto *invoke = op(node, "ac.method.call", operands, call.results,
                    {b.getNamedAttr("callee", symbol(call.name)),
                     b.getNamedAttr("occurrence", origin(node, scope.owner->symbol))});
  return SmallVector<Value>(invoke->getResults());
}

FailureOr<SmallVector<Value>>
Importer::referenceCall(const AstNode &node, Scope &scope, Type expected) {
  if (scope.module || !node.array("keywords").empty())
    return (void)fail(node, "reference Methods require transactional positional calls"), failure();
  auto func = node.child("func");
  auto values = expression(func.child("value"), scope);
  if (failed(values) || values->size() != 1) return failure();
  auto receiver = values->front();
  auto ref = dyn_cast<ac::ModuleRefType>(receiver.getType());
  if (!ref) return (void)fail(node, "Method receiver requires a Module reference"), failure();
  Operation *target = nullptr;
  for (Operation &decl : *result->getBody())
    if (decl.getAttrOfType<StringAttr>("sym_name") &&
        decl.getAttrOfType<StringAttr>("sym_name").getValue() == ref.getModule().getValue())
      target = &decl;
  if (!target) return (void)fail(node, "missing Module reference declaration"), failure();
  auto name = func.string("attr");
  auto methods = ac::getTransactionalMethodInterfaces(target);
  DictionaryAttr prior;
  for (auto attr : methods) {
    auto item = cast<DictionaryAttr>(attr);
    if (item.getAs<StringAttr>("name").getValue() == name) prior = item;
  }
  bool requirement = ac::isTransactionalReferenceRequirement(target);
  if (!prior && !requirement)
    return (void)fail(node, "receiver does not publish requested Method"), failure();
  FunctionType signature;
  if (prior) signature = cast<FunctionType>(prior.getAs<TypeAttr>("function_type").getValue());
  if (signature && signature.getNumInputs() != node.array("args").size())
    return (void)fail(node, "Method argument arity differs"), failure();
  SmallVector<Value> operands{receiver};
  SmallVector<Type> inputs, outputs;
  for (size_t i = 0; i < node.array("args").size(); ++i) {
    auto at = node.item("args", i);
    Type type = signature ? signature.getInput(i) : infer(at, scope);
    if (!type) return (void)fail(at, "reference Method argument needs a declared payload type"), failure();
    auto value = scalar(at, scope, type);
    if (failed(value)) return failure();
    operands.push_back(*value);
    inputs.push_back(value->getType());
  }
  if (signature) llvm::append_range(outputs, signature.getResults());
  else if (expected) outputs.push_back(expected);
  if (requirement) {
    SmallVector<Attribute> updated;
    for (auto attr : methods)
      if (cast<DictionaryAttr>(attr).getAs<StringAttr>("name").getValue() != name)
        updated.push_back(attr);
    bool pure = scope.pure || (prior && prior.getAs<BoolAttr>("read_only").getValue());
    updated.push_back(b.getDictionaryAttr({
        b.getNamedAttr("name", b.getStringAttr(name)),
        b.getNamedAttr("function_type", TypeAttr::get(b.getFunctionType(inputs, outputs))),
        b.getNamedAttr("read_only", b.getBoolAttr(pure))}));
    target->setAttr("ac.methods", b.getArrayAttr(updated));
  }
  auto *call = op(node, "ac.method.call", operands, outputs,
      {b.getNamedAttr("callee", symbol(name)),
       b.getNamedAttr("occurrence", origin(node, scope.owner->symbol))});
  return SmallVector<Value>(call->getResults());
}
