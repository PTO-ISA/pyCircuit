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
if (node.kind() == "Constant" && expected && isa<ac::EnumType>(*expected))
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
if (node.kind() == "Constant" && expected && !isa<ac::BitsType>(*expected)) {
  auto encoded = dyn_cast_or_null<DictionaryAttr>(node.get("value"));
  auto spelling = encoded ? encoded.getAs<StringAttr>("integer") : StringAttr();
  if (!spelling || spelling.getValue() != "0")
    return error() << "aggregate literal requires contextual zero";
  return constant(*expected, 0, node, at, ownerSymbol);
}
if (node.kind() == "Attribute" &&
    !(node.child("value").kind() == "Name" &&
      node.child("value").string("id") != queryArgument &&
      instances.contains(node.child("value").string("id")))) {
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
      if (isa<ac::BitsType>(result.getType()))
        fixedValues.insert(result);
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
    behavioralRules.contains(node.child("func").string("id"))) {
  if (!activeBehavioralDecl || !activeStates ||
      !isa<ac::ModuleOp>(at.getInsertionBlock()->getParentOp()))
    return error() << "behavioral rule calls are only admitted at module scope";
  auto results =
      behavioralCall(*activeBehavioralDecl, node, at, values, *activeStates);
  if (failed(results))
    return failure();
  if (results->size() != 1 || !isa<ac::StructType>(results->front().getType()))
    return error() << "rule value calls require a typed struct return contract";
  return results->front();
}
if (node.kind() == "Call" && node.child("func").kind() == "Name" &&
    local.contains(node.child("func").string("id")))
  return directModuleCall(node, at, ownerSymbol, values, instances);
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
    fixedValues.insert(result);
    return result;
  }
  auto index = expression(node.child("slice"), at, ownerSymbol, values,
                          instances, {}, captures);
  if (failed(index))
    return failure();
  auto type = dyn_cast<ac::TableType>(base->getType());
  if (!type)
    return error() << "subscript requires a table";
  if (failed(proveIndex(*base, *index, node)))
    return failure();
  auto result =
      createOp(at, node.location(b.getContext(), source.path),
               ac::TableGetOp::getOperationName(), {*base, *index},
               TypeRange{type.getElementType(), bits(1, node, ownerSymbol)}, {})
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
