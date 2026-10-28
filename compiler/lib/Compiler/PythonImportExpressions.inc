if (node.kind() == "Call" && intrinsic(node.child("func"), "four_state")) {
  auto pattern = fourStatePattern(node);
  if (failed(pattern))
    return failure();
  auto result =
      createOp(at, node.location(b.getContext(), source.path),
               ac::BitsConstant4Op::getOperationName(), {},
               TypeRange{internalBits(pattern->size(), node, ownerSymbol)},
               {at.getNamedAttr("pattern", at.getStringAttr(pattern->lower()))})
          ->getResult(0);
  remember(result, std::nullopt);
  fixedValues.insert(result);
  return result;
}
if (node.kind() == "Call" && (intrinsic(node.child("func"), "case_eq") ||
                              intrinsic(node.child("func"), "is_known"))) {
  bool known = intrinsic(node.child("func"), "is_known");
  if (node.array("args").size() != (known ? 1u : 2u) ||
      !node.array("keywords").empty())
    return error() << "four-state observer requires exactly "
                   << (known ? "one" : "two") << " positional Bits operands";
  auto lhs = expression(node.item("args", 0), at, ownerSymbol, values,
                        instances, {}, captures);
  if (failed(lhs))
    return failure();
  FailureOr<Value> rhs = known
                             ? lhs
                             : expression(node.item("args", 1), at, ownerSymbol,
                                          values, instances, {}, captures);
  if (failed(rhs))
    return failure();
  if (!isa<ac::BitsType>(lhs->getType()) ||
      !isa<ac::BitsType>(rhs->getType()) || !fixedValues.contains(*lhs) ||
      !fixedValues.contains(*rhs) ||
      !ac::areEquivalentHardwareTypes(lhs->getType(), rhs->getType()))
    return error()
           << "four-state observer requires equal-width fixed Bits operands";
  auto bit = internalBits(1, node, ownerSymbol);
  auto compare = [&](Value left, Value right, StringRef predicate) {
    return createOp(at, node.location(b.getContext(), source.path),
                    ac::BitsCompareOp::getOperationName(), {left, right},
                    TypeRange{bit},
                    {at.getNamedAttr("predicate", at.getStringAttr(predicate))})
        ->getResult(0);
  };
  Value result;
  if (known) {
    auto equal = compare(*lhs, *lhs, "eq");
    auto one = constant(bit, 1, node, at, ownerSymbol);
    if (failed(one))
      return failure();
    result = compare(equal, *one, "case_eq");
  } else
    result = compare(*lhs, *rhs, "case_eq");
  result.getDefiningOp()->setAttr(
      "ac.verification_observer",
      at.getStringAttr(known ? "is_known" : "case_eq"));
  result.getDefiningOp()->setAttr("ac.origin",
                                  sourceOccurrence(at, ownerSymbol, node));
  remember(result, ValueKind::Boolean);
  return result;
}
// Memory handles are static identities; all data accesses are Rule operations.
AstNode memoryBase = node.kind() == "Attribute" ? node.child("value") : node;
if (memoryBase.kind() == "Name") {
  auto found = instances.find(memoryBase.string("id"));
  if (found != instances.end() && memoryInstance(found->second) &&
      !values.contains(memoryBase.string("id")))
    return error() << "Memory handle is a static allocation identity, not a payload value";
}
// Queue construction produces an ordinary immutable instance handle. Its three
// public raw views are ordinary instance outputs.
auto queueRaw = queueRawExpression(node, at, ownerSymbol, values, instances);
if (failed(queueRaw)) return failure();
if (*queueRaw) return **queueRaw;
if (activeSourceEffects &&
    (node.kind() == "Name" || node.kind() == "Call")) {
  auto effect = sourceEffectExpression(node, at, ownerSymbol, values, instances);
  if (failed(effect))
    return failure();
  if (*effect)
    return **effect;
}
if (node.kind() == "Call" && node.child("func").kind() == "Attribute" &&
    llvm::is_contained(
        ArrayRef<StringRef>{"map", "fold", "all", "any", "count"},
        node.child("func").string("attr")))
  return tableValueCall(node, at, ownerSymbol, values, instances);
auto member = resolveEnumMember(node);
if (failed(member))
  return failure();
if (*member)
  return createOp(at, node.location(b.getContext(), source.path),
                  ac::EnumCreateOp::getOperationName(), {},
                  TypeRange{(*member)->type},
                  {at.getNamedAttr("member", (*member)->member)})
      ->getResult(0);
if (auto name = fixedCountCall(node); !name.empty()) {
  if (failed(validateFixedCountCall(node, name)))
    return failure();
  auto input = expression(node.item("args", 0), at, ownerSymbol, values,
                          instances, {}, captures);
  if (failed(input))
    return failure();
  return lowerFixedCount(*input, node, at, ownerSymbol, name);
}
if (concatCall(node)) {
  if (failed(validateConcatCall(node)))
    return failure();
  SmallVector<Value> operands;
  SmallVector<Type> types;
  for (size_t i = 0; i < node.array("args").size(); ++i) {
    auto operand = node.item("args", i);
    auto value = expression(operand, at, ownerSymbol, values, instances, {},
                            captures);
    if (failed(value))
      return failure();
    if (!isa<ac::BitsType>(value->getType()) || !fixedValues.contains(*value))
      return mlir::emitError(operand.location(b.getContext(), source.path))
             << "concat operand " << i + 1
             << " requires explicitly unsigned fixed bits";
    operands.push_back(*value);
    types.push_back(value->getType());
  }
  auto type = resolveConcatType(node, types, ownerSymbol);
  if (failed(type))
    return failure();
  auto result =
      createOp(at, node.location(b.getContext(), source.path),
               ac::BitsConcatOp::getOperationName(), operands, TypeRange{*type},
               {})
          ->getResult(0);
  fixedValues.insert(result);
  return result;
}
if (node.kind() == "Call" && intrinsic(node.child("func"), "enum_to_bits")) {
  if (node.array("args").size() != 1 || !node.array("keywords").empty())
    return error() << "enum_to_bits requires exactly one positional Enum value";
  auto input = expression(node.item("args", 0), at, ownerSymbol, values,
                          instances, {}, captures);
  if (failed(input))
    return failure();
  return enumToBits(*input, node, at, ownerSymbol);
}
if (auto producer = fixedResultCall(node); !producer.empty())
  return mlir::emitError(node.location(b.getContext(), source.path))
         << producer
         << " produces fixed results; unpack first into ordinary "
            "local Names";
if ((node.kind() == "Constant" || sourceLiteral) && expected && isa<ac::EnumType>(*expected))
  return error() << "source Integer/Boolean literals cannot be implicitly "
                    "converted to Enum";
// Aggregate and unsigned expressions use the same expression recursion.
if (node.kind() == "Tuple" || node.kind() == "List") {
  auto table = expected ? dyn_cast<ac::TableType>(*expected) : ac::TableType();
  if (!table)
    return error() << "tuple/list literal requires an expected Table type";
  ac::HardwareAnalysis analysis(*body);
  auto shape = analysis.getTableShape(table, {}, body->getOperation());
  if (failed(shape))
    return failure();
  if (shape->size() != 1 ||
      !isa<ac::BitsType, ac::EnumType, ac::StructType>(table.getElementType()))
    return error() << "Table literal requires a positive closed "
                      "one-dimensional Table of bits, enum or struct";
  if (node.array("elts").size() != shape->front())
    return error() << "Table literal element count disagrees with extent";
  SmallVector<Value> elements;
  for (size_t i = 0; i < node.array("elts").size(); ++i) {
    auto element = node.item("elts", i);
    auto value = expression(element, at, ownerSymbol, values, instances,
                            table.getElementType(), captures);
    if (failed(value))
      return failure();
    auto converted = unsignedBoundary(*value, table.getElementType(), element,
                                      at, ownerSymbol);
    if (failed(converted))
      return failure();
    elements.push_back(*converted);
  }
  return createOp(at, node.location(b.getContext(), source.path),
                  ac::TableCreateOp::getOperationName(), elements,
                  TypeRange{table}, {})
      ->getResult(0);
}
if ((node.kind() == "Constant" || sourceLiteral) && expected && !isa<ac::BitsType>(*expected)) {
  auto literalValue = sourceLiteral ? sourceLiteral : staticValue(b, node.get("value"), error);
  auto integer = literalValue ? literalValue.getAs<ac::MathIntAttr>("value") : ac::MathIntAttr();
  if (!integer || integer.getCanonicalValue() != "0")
    return error() << "aggregate literal requires contextual zero";
  return constant(*expected, 0, node, at, ownerSymbol);
}
if (node.kind() == "Attribute" && !sourceLiteral &&
    !(node.child("value").kind() == "Name" &&
      !queryBound(node.child("value").string("id")) &&
      instances.contains(node.child("value").string("id")) &&
      !values.contains(node.child("value").string("id")))) {
  auto base = expression(node.child("value"), at, ownerSymbol, values,
                         instances, {}, captures);
  if (failed(base))
    return failure();
  auto record = dyn_cast<ac::StructType>(base->getType());
  ac::HardwareAnalysis analysis(*body);
  auto declaration = record ? analysis.lookupStruct(record) : ac::StructOp();
  if (!declaration)
    return error() << "field projection requires a struct";
  for (Attribute raw : declaration.getFields()) {
    auto field = cast<DictionaryAttr>(raw);
    if (field.getAs<StringAttr>("name").getValue() == node.string("attr")) {
      auto result =
          createOp(at, node.location(b.getContext(), source.path),
                   ac::StructGetOp::getOperationName(), {*base},
                   TypeRange{field.getAs<TypeAttr>("type").getValue()},
                   {at.getNamedAttr("field", field.getAs<StringAttr>("name"))})
              ->getResult(0);
      rememberStructField(result, field, node);
      return result;
    }
  }
  return error() << "unknown struct field";
}
if (node.kind() == "Call") {
  auto constructor = lookupBinding(node.child("func"));
  if (constructor && constructor->category == BindingCategory::Struct)
    return constructStruct(node, at, ownerSymbol, values, instances, expected,
                           captures);
}
if (node.kind() == "Call" && node.child("func").kind() == "Name" &&
    activeNestedRuleScope &&
    nestedRuleBinding(node.child("func").string("id")) &&
    values.contains(node.child("func").string("id")) &&
    behavioralRules.contains(node.child("func").string("id"))) {
  {
    auto checked = checkedRuleRegistration(node, behavioralRules[node.child("func").string("id")]);
    if (failed(checked)) return failure();
    if (activeValueHelper || !activeBehavioralDecl || !activeStates ||
        !isa<ac::ModuleOp>(at.getInsertionBlock()->getParentOp()))
      return error() << "Rule calls are only complete Module/System registrations";
    auto results = behavioralCall(*activeBehavioralDecl, node, at, values, *activeStates);
    if (failed(results)) return failure();
    if (results->size() != 1) return error() << "Rule value registration requires one ordinary result";
    return results->front();
  }
}
if (node.kind() == "Call") {
  auto helper = resolveValueHelper(node.child("func"), ownerSymbol, &values);
  if (failed(helper)) return failure();
  if (*helper) {
    auto results = valueHelperCall(node, at, ownerSymbol, values, instances);
    if (failed(results)) return failure();
    if (results->size() != 1)
      return error() << "None pure helper is admitted only as a statement";
    return results->front();
  }
}
if (node.kind() == "Call" && node.child("func").kind() == "Name" &&
    behavioralRules.contains(node.child("func").string("id"))) {
  if (activeValueHelper || !activeBehavioralDecl || !activeStates ||
      !isa<ac::ModuleOp>(at.getInsertionBlock()->getParentOp()))
    return error() << "Rule calls are only complete Module/System registrations";
  auto results = behavioralCall(*activeBehavioralDecl, node, at, values, *activeStates);
  if (failed(results)) return failure();
  if (results->size() != 1)
    return error() << "Rule value registration requires one ordinary result";
  return results->front();
}
if (node.kind() == "Call" && node.child("func").kind() == "Name" &&
    local.contains(node.child("func").string("id"))) {
  if (activeValueHelper)
    return error() << "pure helper cannot instantiate a module";
  else
    return directModuleCall(node, at, ownerSymbol, values, instances);
}
if (node.kind() == "Subscript") {
  auto base = expression(node.child("value"), at, ownerSymbol, values,
                         instances, {}, captures);
  if (failed(base))
    return failure();
  if (node.child("slice").kind() == "Slice") {
    if (!isa<ac::BitsType>(base->getType()) || !fixedValues.contains(*base))
      return error() << "slice requires explicitly unsigned fixed bits";
    auto shape =
        resolveSlice(node.child("slice"), base->getType(), ownerSymbol);
    if (failed(shape))
      return failure();
    auto result =
        createOp(at, node.location(b.getContext(), source.path),
                 ac::BitsExtractOp::getOperationName(), {*base},
                 TypeRange{shape->type}, {at.getNamedAttr("low", shape->low)})
            ->getResult(0);
    ac::HardwareAnalysis analysis(*body);
    // A symbolic slice still has fixed-bit authority. Range/value facts need
    // concrete widths and remain absent until actual binding proves them.
    if (!analysis.isStaticEvaluable(cast<ac::BitsType>(shape->type).getWidth(), {}) ||
        !analysis.isStaticEvaluable(shape->low, {})) {
      remember(result, std::nullopt);
      fixedValues.insert(result);
      return result;
    }
    auto low = analysis.evaluateStatic(shape->low, {}, result.getDefiningOp());
    auto width = analysis.getPackedWidth(shape->type, {}, result.getDefiningOp());
    if (failed(low) || failed(width))
      return failure();
    auto info = valueInfo(*base);
    std::optional<IntegerInterval> interval;
    Attribute closed;
    if (llvm::APSInt(cast<ac::MathIntAttr>(*low).getCanonicalValue()).isZero()) {
      // A low slice preserves a bound only when every possible value fits.
      if (info.interval && !info.interval->lower.isNegative() &&
          !info.interval->upper.isNegative()) {
        const auto &upper = info.interval->upper;
        // U <= 2**width, without allocating an integer of the carrier's width.
        if (upper.getActiveBits() <= *width ||
            (upper.isPowerOf2() && upper.getActiveBits() - 1 <= *width))
          interval = info.interval;
      }
      if (auto integer =
              dyn_cast_or_null<ac::MathIntAttr>(info.closedSourceConstant)) {
        llvm::APSInt value(integer.getCanonicalValue());
        if (!value.isNegative() && value.getActiveBits() <= *width)
          closed = integer;
      } else if (isa_and_nonnull<BoolAttr>(info.closedSourceConstant))
        closed = info.closedSourceConstant;
    }
    remember(result, std::nullopt, std::move(interval), closed);
    fixedValues.insert(result);
    return result;
  }
  auto index = expression(node.child("slice"), at, ownerSymbol, values,
                          instances, {}, captures);
  if (failed(index))
    return failure();
  if (isa<ac::BitsType>(base->getType())) {
    if (!fixedValues.contains(*base))
      return error() << "bit index requires authoritative fixed bits, not a "
                        "Boolean or Integer carrier";
    ac::HardwareAnalysis analysis(*body);
    auto width = analysis.getPackedWidth(base->getType(), {},
                                         at.getInsertionBlock()->getParentOp());
    if (failed(width) || failed(proveIndexExtent(*index, *width, node, "bit")))
      return failure();
    // Keep the complete index carrier, including high X/Z bits. A logical
    // Boolean/Integer index crosses an explicit fresh fixed-Bits boundary;
    // aliases of the original source value retain their source-kind facts.
    auto count = unsignedBoundary(*index, index->getType(), node.child("slice"),
                                  at, ownerSymbol);
    if (failed(count))
      return failure();
    auto shifted =
        fixedUnsignedShift(*base, *count, "lshr", node, node.child("value"),
                           node.child("slice"), at, ownerSymbol);
    if (failed(shifted))
      return failure();
    auto result =
        createOp(at, node.location(b.getContext(), source.path),
                 ac::BitsExtractOp::getOperationName(), {*shifted},
                 TypeRange{internalBits(1, node, ownerSymbol)},
                 {at.getNamedAttr("low", literal(0, node, ownerSymbol))})
            ->getResult(0);
    remember(result, std::nullopt);
    fixedValues.insert(result);
    return result;
  }
  auto type = dyn_cast<ac::TableType>(base->getType());
  if (!type)
    return error() << "subscript requires fixed bits or a table";
  auto admission = proveIndex(*base, *index, node);
  if (failed(admission)) return failure();
  SmallVector<Type> results{type.getElementType()};
  bool deferred = *admission == SourceIndexAdmission::Deferred;
  if (!deferred) results.push_back(internalBits(1, node, ownerSymbol));
  auto result =
      createOp(at, node.location(b.getContext(), source.path),
               deferred ? ac::TableReadOp::getOperationName() : ac::TableGetOp::getOperationName(),
               {*base, *index}, results, {})
          ->getResult(0);
  if (isa<ac::BitsType>(result.getType()))
    fixedValues.insert(result);
  return result;
}
if (node.kind() == "BoolOp") {
  if (node.array("values").size() < 2)
    return error() << "Boolean operator requires at least two operands";
  FailureOr<Value> result = expression(node.item("values", 0), at, ownerSymbol,
                                       values, instances, {}, captures);
  if (failed(result))
    return failure();
  if (failed(predicateView(*result, node.item("values", 0), ownerSymbol)))
    return failure();
  bool anyFixed = fixedValues.contains(*result);
  for (size_t i = 1; i < node.array("values").size(); ++i) {
    auto next = expression(node.item("values", i), at, ownerSymbol, values,
                           instances, {}, captures);
    if (failed(next))
      return failure();
    if (failed(predicateView(*next, node.item("values", i), ownerSymbol)))
      return failure();
    bool nextFixed = fixedValues.contains(*next);
    Value previous = *result;
    result =
        binary(node.child("op").kind() == "And" ? "and" : "or", *result, *next);
    anyFixed |= nextFixed;
    if (anyFixed)
      fixedValues.insert(*result);
    else {
      remember(*result, ValueKind::Boolean);
      if (failed(propagateClosed(*result,
                                 node.child("op").kind() == "And"
                                     ? ValueOpcode::AndBool
                                     : ValueOpcode::OrBool,
                                 {previous, *next}, node)))
        return failure();
    }
  }
  return *result;
}
if (node.kind() == "UnaryOp" &&
    (node.child("op").kind() == "Not" || node.child("op").kind() == "Invert")) {
  auto value = expression(node.child("operand"), at, ownerSymbol, values,
                          instances, expected, captures);
  if (failed(value))
    return failure();
  if (isa<ac::EnumType>(value->getType()))
    return error() << "Enum unary operations require explicit enum_to_bits";
  bool logical = node.child("op").kind() == "Not";
  if (logical && failed(predicateView(*value, node, ownerSymbol)))
    return failure();
  if (!logical && !fixedValues.contains(*value) &&
      valueInfo(*value).sourceKind == ValueKind::Integer)
    return error() << "Integer unary inversion is unsupported; use explicitly "
                      "typed bits";
  auto result = createOp(at, node.location(b.getContext(), source.path),
                         ac::BitsUnaryOp::getOperationName(), {*value},
                         TypeRange{value->getType()},
                         {at.getNamedAttr("opcode", at.getStringAttr("not"))})
                    ->getResult(0);
  if (fixedValues.contains(*value))
    fixedValues.insert(result);
  else if (valueInfo(*value).sourceKind == ValueKind::Boolean) {
    remember(result, ValueKind::Boolean);
    // Both spellings invert a one-bit Boolean here; Integer Invert has a
    // different evaluator contract and must not reinterpret this value.
    if (failed(propagateClosed(result, ValueOpcode::Not, {*value}, node)))
      return failure();
  }
  return result;
}
