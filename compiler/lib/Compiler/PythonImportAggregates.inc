// Aggregate lowering belongs to the single captured-source importer.
bool Importer::lexicallyShadowed(const AstNode &node, StringRef name) const {
  return sourceBindingShadowed(source, node, name);
}
std::optional<Import> Importer::lookupBinding(const AstNode &node) const {
  if (node.kind() == "Name") {
    StringRef name = node.string("id");
    auto found = names.find(name);
    if (found != names.end() && queryBound(name))
      return std::nullopt;
    if (found != names.end() && lexicallyShadowed(node, name)) {
      error() << "source binding is shadowed in its lexical scope: '" << name
              << "'";
      return std::nullopt;
    }
    return found == names.end() ? std::nullopt
                                : std::optional<Import>(found->second);
  }
  if (node.kind() != "Attribute")
    return std::nullopt;
  auto parent = lookupBinding(node.child("value"));
  if (!parent || parent->category != BindingCategory::Namespace)
    return std::nullopt;
  parent->category = BindingCategory::Marker;
  parent->remote = node.string("attr").str();
  return parent;
}
std::optional<ResolvedSourceBinding>
Importer::resolveBinding(const AstNode &node) const {
  auto binding = lookupBinding(node);
  if (!binding)
    return std::nullopt;
  if (binding->category == BindingCategory::Marker)
    return ResolvedSourceBinding{
        classifyImportedMarker(binding->importedModule, binding->remote), {}};
  if (binding->category == BindingCategory::Enum ||
      binding->category == BindingCategory::Struct)
    return ResolvedSourceBinding{MarkerKind::None, binding->symbol};
  return std::nullopt;
}
bool Importer::nominalAnnotation(const AstNode &node) const {
  auto binding = resolveBinding(node);
  return binding && binding->marker == MarkerKind::None &&
         binding->nominalSymbol;
}
FailureOr<std::optional<EnumMemberRef>>
Importer::resolveEnumMember(const AstNode &node) const {
  if (node.kind() != "Attribute")
    return std::optional<EnumMemberRef>();
  auto base = node.child("value");
  if (base.kind() == "Name") {
    auto global = names.find(base.string("id"));
    if (global != names.end() && !queryBound(base.string("id")) &&
        global->second.category == BindingCategory::Enum &&
        lexicallyShadowed(base, base.string("id")))
      return error()
             << "Enum member declaration is shadowed in its lexical scope";
  }
  auto binding = lookupBinding(base);
  if (!binding || binding->category != BindingCategory::Enum)
    return std::optional<EnumMemberRef>();
  auto type = ac::EnumType::get(
      b.getContext(),
      StringAttr::get(b.getContext(), binding->symbol.getValue()));
  auto definition =
      ac::HardwareAnalysis(*body).resolveEnum(type, (*body).getOperation());
  if (failed(definition))
    return failure();
  for (const auto &member : definition->members)
    if (member.name.getValue() == node.string("attr"))
      return std::optional<EnumMemberRef>(EnumMemberRef{type, member.name});
  return error() << "unknown nominal Enum member '" << node.string("attr")
                 << "'";
}
FailureOr<Value> Importer::enumToBits(Value value, const AstNode &node,
                                      OpBuilder &at, FlatSymbolRefAttr symbol) {
  auto type = dyn_cast<ac::EnumType>(value.getType());
  if (!type)
    return error() << "enum_to_bits requires a nominal Enum value";
  ac::HardwareAnalysis analysis(*body);
  auto definition = analysis.resolveEnum(type, body->getOperation());
  if (failed(definition))
    return failure();
  auto bitsType = ac::BitsType::get(b.getContext(),
                                    literal(definition->width, node, symbol));
  auto operation = createOp(at, node.location(b.getContext(), source.path),
                            ac::EnumToBitsOp::getOperationName(), {value},
                            TypeRange{bitsType}, {});
  ac::HardwareBindings bindings;
  bindings.owner = operation->getParentOfType<ac::ModuleOp>();
  if (failed(analysis.verifyEnumValueOperation(operation, bindings)))
    return failure();
  fixedValues.insert(operation->getResult(0));
  return operation->getResult(0);
}
FailureOr<SmallVector<Value>>
Importer::enumFromBits(const AstNode &call, OpBuilder &at,
                       FlatSymbolRefAttr symbol, llvm::StringMap<Value> &values,
                       llvm::StringMap<Instance *> &instances,
                       llvm::StringMap<SmallVector<Value>> *captures) {
  auto function = call.child("func");
  if (call.kind() != "Call" || function.kind() != "Subscript" ||
      !intrinsic(function.child("value"), "enum_from_bits") ||
      call.array("args").size() != 1 || !call.array("keywords").empty())
    return error()
           << "enum_from_bits[Enum] requires exactly one positional bits value";
  auto target = portType(function.child("slice"), symbol);
  if (failed(target))
    return failure();
  auto type = dyn_cast<ac::EnumType>(*target);
  if (!type)
    return error() << "enum_from_bits requires an explicit nominal Enum type";
  auto input = expression(call.item("args", 0), at, symbol, values, instances,
                          {}, captures);
  if (failed(input))
    return failure();
  if (!isa<ac::BitsType>(input->getType()) || !fixedValues.contains(*input))
    return error() << "enum_from_bits requires authoritative fixed bits";
  auto operation = createOp(at, call.location(b.getContext(), source.path),
                            ac::EnumFromBitsOp::getOperationName(), {*input},
                            TypeRange{type, bits(1, call, symbol)}, {});
  ac::HardwareAnalysis analysis(*body);
  ac::HardwareBindings bindings;
  bindings.owner = operation->getParentOfType<ac::ModuleOp>();
  if (failed(analysis.verifyEnumValueOperation(operation, bindings)))
    return failure();
  remember(operation->getResult(1), ac::detail::ValueKind::Boolean);
  return SmallVector<Value>{operation->getResult(0), operation->getResult(1)};
}
bool Importer::intrinsic(const AstNode &node, StringRef name) const {
  auto marker = classifyImportedMarker("pycircuit", name);
  if (marker != MarkerKind::None)
    return ruleWrites.resolves(node, marker);
  auto binding = lookupBinding(node);
  return binding && binding->category == BindingCategory::Marker &&
         binding->importedModule == "pycircuit" && binding->remote == name;
}
bool Importer::fixedAnnotation(const AstNode &node) const {
  if (node.kind() == "Subscript" && intrinsic(node.child("value"), "bits"))
    return true;
  auto binding = lookupBinding(node);
  return binding && binding->category == BindingCategory::Marker &&
         fixedBitsAliasWidth(binding->importedModule, binding->remote).has_value();
}
bool Importer::hasDecorator(const AstNode &node, StringRef name) const {
  return ruleWrites.hasDecorator(node, name);
}

ac::StaticExprAttr Importer::literal(uint64_t value, const AstNode &node,
                                     FlatSymbolRefAttr symbol) {
  return ac::StaticExprAttr::get(
      b.getContext(),
      b.getDictionaryAttr(
          {b.getNamedAttr("kind", b.getStringAttr("literal")),
           b.getNamedAttr(
               "value",
               b.getDictionaryAttr(
                   {b.getNamedAttr("kind", b.getStringAttr("integer")),
                    b.getNamedAttr("value",
                                   ac::MathIntAttr::get(
                                       b.getContext(),
                                       llvm::APSInt::getUnsigned(value)))})),
           b.getNamedAttr("origin", occurrence(b, symbol, node)),
           b.getNamedAttr("location", sourceSpan(b, source.path, node))}));
}
Type Importer::bits(unsigned width, const AstNode &node,
                    FlatSymbolRefAttr symbol) {
  return ac::BitsType::get(b.getContext(), literal(width, node, symbol));
}
bool Importer::concatCall(const AstNode &node) const {
  if (node.kind() != "Call")
    return false;
  auto function = node.child("func");
  return intrinsic(function, "concat") ||
         (function.kind() == "Subscript" &&
          intrinsic(function.child("value"), "concat"));
}
LogicalResult Importer::validateConcatCall(const AstNode &node) {
  auto loc = node.location(b.getContext(), source.path);
  if (node.child("func").kind() == "Subscript")
    return mlir::emitError(loc) << "concat does not accept type subscription";
  if (node.array("args").empty() || !node.array("keywords").empty())
    return mlir::emitError(loc)
           << "concat requires one or more positional fixed-bit operands";
  for (size_t i = 0; i < node.array("args").size(); ++i)
    if (node.item("args", i).kind() == "Starred")
      return mlir::emitError(
                 node.item("args", i).location(b.getContext(), source.path))
             << "concat does not accept starred operands";
  return success();
}
FailureOr<Type> Importer::resolveConcatType(const AstNode &node,
                                            ArrayRef<Type> operands,
                                            FlatSymbolRefAttr symbol) {
  ac::HardwareAnalysis analysis(*body);
  uint64_t total = 0;
  for (auto [i, type] : llvm::enumerate(operands)) {
    auto operand = node.item("args", i);
    auto loc = operand.location(b.getContext(), source.path);
    auto fixed = dyn_cast<ac::BitsType>(type);
    if (!fixed)
      return mlir::emitError(loc) << "concat operand " << i + 1
                                  << " requires explicitly unsigned fixed bits";
    if (!analysis.isStaticEvaluable(fixed.getWidth(), {}))
      return mlir::emitError(loc) << "concat operand " << i + 1
                                  << " requires a positive resolved width";
    auto width = analysis.getPackedWidth(type, {}, body->getOperation());
    if (failed(width))
      return failure();
    if (!*width)
      return mlir::emitError(loc) << "concat operand " << i + 1
                                  << " requires a positive resolved width";
    if (*width > std::numeric_limits<uint64_t>::max() - total)
      return mlir::emitError(node.location(b.getContext(), source.path))
             << "concat width sum overflows uint64";
    total += *width;
  }
  return Type(ac::BitsType::get(b.getContext(), literal(total, node, symbol)));
}
StringRef Importer::fixedCountCall(const AstNode &node) const {
  if (node.kind() != "Call")
    return {};
  auto function = node.child("func");
  if (function.kind() == "Subscript")
    function = function.child("value");
  for (StringRef name :
       {"popcount", "count_leading_zeros", "count_trailing_zeros"})
    if (intrinsic(function, name))
      return name;
  return {};
}
LogicalResult Importer::validateFixedCountCall(const AstNode &node,
                                               StringRef name) {
  auto loc = node.location(b.getContext(), source.path);
  if (node.child("func").kind() == "Subscript")
    return mlir::emitError(loc) << name << " does not accept type subscription";
  if (node.array("args").size() != 1 || !node.array("keywords").empty())
    return mlir::emitError(loc)
           << name << " requires exactly one positional fixed-bit operand";
  if (node.item("args", 0).kind() == "Starred")
    return mlir::emitError(
               node.item("args", 0).location(b.getContext(), source.path))
           << name << " does not accept starred operands";
  return success();
}
FailureOr<FixedCountShape>
Importer::resolveFixedCountShape(const AstNode &node, Type operand,
                                 FlatSymbolRefAttr symbol, StringRef name) {
  auto loc = node.item("args", 0).location(b.getContext(), source.path);
  auto fixed = dyn_cast<ac::BitsType>(operand);
  if (!fixed)
    return mlir::emitError(loc)
           << name << " operand requires explicitly unsigned fixed bits";
  ac::HardwareAnalysis analysis(*body);
  if (!analysis.isStaticEvaluable(fixed.getWidth(), {}))
    return mlir::emitError(loc)
           << name << " operand requires a positive resolved width";
  auto width = analysis.getPackedWidth(operand, {}, body->getOperation());
  if (failed(width))
    return failure();
  if (!*width)
    return mlir::emitError(loc)
           << name << " operand requires a positive resolved width";
  uint64_t resultWidth = llvm::Log2_64(*width) + 1;
  return FixedCountShape{
      *width,
      ac::BitsType::get(b.getContext(), literal(resultWidth, node, symbol))};
}
FailureOr<FixedCountShape>
Importer::resolveFixedInputShape(Value input, const AstNode &node,
                                 FlatSymbolRefAttr symbol, StringRef name) {
  auto kind = valueInfo(input).sourceKind;
  if (!isa<ac::BitsType>(input.getType()) || !fixedValues.contains(input) ||
      kind == ac::detail::ValueKind::Boolean ||
      kind == ac::detail::ValueKind::Integer)
    return mlir::emitError(
               node.item("args", 0).location(b.getContext(), source.path))
           << name << " operand requires explicitly unsigned fixed bits";
  return resolveFixedCountShape(node, input.getType(), symbol, name);
}
SmallVector<std::pair<uint64_t, Value>, 64>
Importer::lowerFixedMasks(uint64_t width, const AstNode &node, OpBuilder &at,
                          FlatSymbolRefAttr symbol) {
  auto loc = node.location(b.getContext(), source.path);
  Type workingType =
      ac::BitsType::get(b.getContext(), literal(width, node, symbol));
  auto constant = [&](uint64_t value) -> Value {
    return createOp(at, loc, ac::BitsConstantOp::getOperationName(), {},
                    TypeRange{workingType},
                    {at.getNamedAttr("value", literal(value, node, symbol))})
        ->getResult(0);
  };
  auto binary = [&](StringRef opcode, Value lhs, Value rhs) -> Value {
    return createOp(at, loc, ac::BitsBinaryOp::getOperationName(), {lhs, rhs},
                    TypeRange{workingType},
                    {at.getNamedAttr("opcode", at.getStringAttr(opcode))})
        ->getResult(0);
  };
  auto shift = [&](StringRef opcode, Value value, uint64_t amount) -> Value {
    return binary(opcode, value, constant(amount));
  };
  Value ones = createOp(at, loc, ac::BitsUnaryOp::getOperationName(),
                        {constant(0)}, TypeRange{workingType},
                        {at.getNamedAttr("opcode", at.getStringAttr("not"))})
                   ->getResult(0);
  uint64_t stride = uint64_t(1) << llvm::Log2_64(width - 1);
  Value mask = shift("lshr", ones, width - stride);
  SmallVector<std::pair<uint64_t, Value>, 64> stages;
  stages.emplace_back(stride, mask);
  // Halving and reversing saved stages avoid overflow at uint64 maximum.
  for (stride /= 2; stride; stride /= 2) {
    mask = binary("xor", mask, shift("shl", mask, stride));
    stages.emplace_back(stride, mask);
  }
  return stages;
}
Value Importer::lowerPopcountValue(Value input, const FixedCountShape &shape,
                                   const AstNode &node, OpBuilder &at,
                                   FlatSymbolRefAttr symbol) {
  // The nonempty singleton reduction preserves all value/known/Z planes.
  if (shape.width == 1)
    return input;
  auto loc = node.location(b.getContext(), source.path);
  Type workingType =
      ac::BitsType::get(b.getContext(), literal(shape.width, node, symbol));
  auto constant = [&](uint64_t value) -> Value {
    return createOp(at, loc, ac::BitsConstantOp::getOperationName(), {},
                    TypeRange{workingType},
                    {at.getNamedAttr("value", literal(value, node, symbol))})
        ->getResult(0);
  };
  auto binary = [&](StringRef opcode, Value lhs, Value rhs) -> Value {
    return createOp(at, loc, ac::BitsBinaryOp::getOperationName(), {lhs, rhs},
                    TypeRange{workingType},
                    {at.getNamedAttr("opcode", at.getStringAttr(opcode))})
        ->getResult(0);
  };
  auto shift = [&](StringRef opcode, Value value, uint64_t amount) -> Value {
    return binary(opcode, value, constant(amount));
  };
  auto stages = lowerFixedMasks(shape.width, node, at, symbol);
  Value result = input;
  for (auto [amount, stageMask] : llvm::reverse(stages)) {
    Value low = binary("and", result, stageMask);
    Value high = binary("and", shift("lshr", result, amount), stageMask);
    result = binary("add", low, high);
  }
  result = createOp(at, loc, ac::BitsExtractOp::getOperationName(), {result},
                    TypeRange{shape.resultType},
                    {at.getNamedAttr("low", literal(0, node, symbol))})
               ->getResult(0);
  return result;
}
FailureOr<Value> Importer::lowerFixedCount(Value input, const AstNode &node,
                                           OpBuilder &at,
                                           FlatSymbolRefAttr symbol,
                                           StringRef name) {
  auto loc = node.location(b.getContext(), source.path);
  auto shape = resolveFixedInputShape(input, node, symbol, name);
  if (failed(shape))
    return failure();
  if (name == "popcount") {
    Value result = lowerPopcountValue(input, *shape, node, at, symbol);
    fixedValues.insert(result);
    return result;
  }

  auto type = [&](uint64_t width) -> Type {
    return ac::BitsType::get(b.getContext(), literal(width, node, symbol));
  };
  auto constant = [&](uint64_t value, Type resultType) -> Value {
    return createOp(at, loc, ac::BitsConstantOp::getOperationName(), {},
                    TypeRange{resultType},
                    {at.getNamedAttr("value", literal(value, node, symbol))})
        ->getResult(0);
  };
  auto unaryNot = [&](Value value) -> Value {
    return createOp(at, loc, ac::BitsUnaryOp::getOperationName(), {value},
                    TypeRange{value.getType()},
                    {at.getNamedAttr("opcode", at.getStringAttr("not"))})
        ->getResult(0);
  };
  auto binary = [&](StringRef opcode, Value lhs, Value rhs) -> Value {
    return createOp(at, loc, ac::BitsBinaryOp::getOperationName(), {lhs, rhs},
                    TypeRange{lhs.getType()},
                    {at.getNamedAttr("opcode", at.getStringAttr(opcode))})
        ->getResult(0);
  };
  auto fill = [&](Value value, uint64_t width, StringRef direction) -> Value {
    if (width > 1)
      // Descending powers of two avoid a doubling overflow at UINT64_MAX.
      for (uint64_t stride = uint64_t(1) << llvm::Log2_64(width - 1); stride;
           stride /= 2)
        value =
            binary("or", value,
                   binary(direction, value, constant(stride, value.getType())));
    return value;
  };
  auto extract = [&](Value value, uint64_t low, Type resultType) -> Value {
    return createOp(at, loc, ac::BitsExtractOp::getOperationName(), {value},
                    TypeRange{resultType},
                    {at.getNamedAttr("low", literal(low, node, symbol))})
        ->getResult(0);
  };
  auto extend = [&](Value value, uint64_t from, uint64_t to) -> Value {
    if (from == to)
      return value;
    return createOp(at, loc, ac::BitsResizeOp::getOperationName(), {value},
                    TypeRange{type(to)},
                    {at.getNamedAttr("mode", at.getStringAttr("zext"))})
        ->getResult(0);
  };
  if (shape->width == 1) {
    Value result = unaryNot(input);
    fixedValues.insert(result);
    return result;
  }

  bool leading = name == "count_leading_zeros";
  uint64_t tailWidth = shape->width - 1;
  uint64_t countWidth = llvm::Log2_64(shape->width) + 1;
  uint64_t tailCountWidth = llvm::Log2_64(tailWidth) + 1;
  Value endpoint = extract(input, leading ? tailWidth : 0, type(1));
  Value tail = extract(input, leading ? 0 : 1, type(tailWidth));
  tail = fill(tail, tailWidth, leading ? "lshr" : "shl");
  FixedCountShape tailShape{tailWidth, type(tailCountWidth)};
  Value population =
      lowerPopcountValue(unaryNot(tail), tailShape, node, at, symbol);
  Value distance = binary("add", constant(1, shape->resultType),
                          extend(population, tailCountWidth, countWidth));
  Value mask = fill(distance, countWidth, "lshr");
  Value extendedEndpoint = extend(endpoint, 1, countWidth);
  // Four-state subtraction is essential: unknown endpoints poison the low
  // bit_width(distance) bits while retaining known-zero upper result bits.
  Value poison = binary("sub", extendedEndpoint, extendedEndpoint);
  Value selected =
      createOp(at, loc, ac::BitsSelectOp::getOperationName(),
               {endpoint, constant(0, shape->resultType), distance},
               TypeRange{shape->resultType}, {})
          ->getResult(0);
  Value result = binary("xor", selected, binary("and", poison, mask));
  fixedValues.insert(result);
  return result;
}
StringRef Importer::fixedResultCall(const AstNode &node) const {
  if (node.kind() != "Call")
    return {};
  auto function = node.child("func");
  // This only recognizes a potential fixed-result syntax producer. The query
  // dispatcher resolves the actual receiver and Table contract before typing
  // callbacks or constructing any row-domain operation.
  if (function.kind() == "Attribute" &&
      (function.string("attr") == "first" || function.string("attr") == "argmin"))
    return function.string("attr");
  if (function.kind() == "Subscript")
    function = function.child("value");
  for (StringRef name : {"enum_from_bits", "priority_encode", "onehot_encode"})
    if (intrinsic(function, name))
      return name;
  return {};
}
FailureOr<bool> Importer::validateFixedEncodingCall(const AstNode &node,
                                                    StringRef name) {
  auto diagnostic = [&] {
    return mlir::emitError(node.location(b.getContext(), source.path));
  };
  if (node.child("func").kind() == "Subscript")
    return diagnostic() << name << " does not accept type subscription";
  if (node.array("args").size() != 1)
    return diagnostic() << name
                        << " requires exactly one positional fixed-bit operand";
  if (node.item("args", 0).kind() == "Starred")
    return diagnostic() << name << " does not accept starred operands";
  bool seenOrder = false, high = false;
  for (size_t i = 0; i < node.array("keywords").size(); ++i) {
    auto keyword = node.item("keywords", i);
    if (keyword.string("arg") != "order" || seenOrder)
      return diagnostic() << name
                          << " accepts only one keyword-only order argument";
    seenOrder = true;
    auto value = keyword.child("value");
    auto order = value.kind() == "Constant"
                     ? dyn_cast_or_null<StringAttr>(value.get("value"))
                     : StringAttr();
    if (!order || (order.getValue() != "low" && order.getValue() != "high"))
      return diagnostic() << name << " order must be literal 'low' or 'high'";
    high = order.getValue() == "high";
  }
  return high;
}
SmallVector<Value>
Importer::lowerFixedEncodingValue(Value input, const FixedCountShape &shape,
                                  bool high, bool onehot, const AstNode &node,
                                  OpBuilder &at, FlatSymbolRefAttr symbol) {
  auto loc = node.location(b.getContext(), source.path);
  auto type = [&](uint64_t width) -> Type {
    return ac::BitsType::get(b.getContext(), literal(width, node, symbol));
  };
  auto constant = [&](uint64_t value, Type resultType) -> Value {
    return createOp(at, loc, ac::BitsConstantOp::getOperationName(), {},
                    TypeRange{resultType},
                    {at.getNamedAttr("value", literal(value, node, symbol))})
        ->getResult(0);
  };
  auto unaryNot = [&](Value value) -> Value {
    return createOp(at, loc, ac::BitsUnaryOp::getOperationName(), {value},
                    TypeRange{value.getType()},
                    {at.getNamedAttr("opcode", at.getStringAttr("not"))})
        ->getResult(0);
  };
  auto binary = [&](StringRef opcode, Value lhs, Value rhs) -> Value {
    return createOp(at, loc, ac::BitsBinaryOp::getOperationName(), {lhs, rhs},
                    TypeRange{lhs.getType()},
                    {at.getNamedAttr("opcode", at.getStringAttr(opcode))})
        ->getResult(0);
  };
  auto compare = [&](StringRef predicate, Value lhs, Value rhs) -> Value {
    Value result =
        createOp(at, loc, ac::BitsCompareOp::getOperationName(), {lhs, rhs},
                 TypeRange{type(1)},
                 {at.getNamedAttr("predicate", at.getStringAttr(predicate))})
            ->getResult(0);
    remember(result, ac::detail::ValueKind::Boolean);
    return result;
  };

  Value index;
  if (shape.width == 1) {
    index = constant(0, type(1));
  } else {
    Value inverted = unaryNot(input);
    auto masks = lowerFixedMasks(shape.width, node, at, symbol);
    SmallVector<Value, 64> indexBits;
    // Masks are saved from the highest index bit to the lowest, which is also
    // concat order. Each lane represents r -> A OR (B AND r).
    for (const auto &stage : masks) {
      Value mask = unaryNot(stage.second);
      Value a = binary("and", input, mask);
      Value c = binary("or", inverted, mask);
      StringRef direction = high ? "shl" : "lshr";
      for (uint64_t stride = 1; stride < shape.width;) {
        // Both updates use the old stage. Vacated B lanes contain one, the
        // composition identity; A lanes contain zero.
        Value oldA = a, oldB = c;
        Value shift = constant(stride, input.getType());
        Value shiftedA = binary(direction, oldA, shift);
        Value shiftedB = unaryNot(binary(direction, unaryNot(oldB), shift));
        a = binary("or", oldA, binary("and", oldB, shiftedA));
        c = binary("and", oldB, shiftedB);
        if (stride > (shape.width - 1) / 2)
          break;
        stride *= 2;
      }
      indexBits.push_back(
          createOp(at, loc, ac::BitsExtractOp::getOperationName(), {a},
                   TypeRange{type(1)},
                   {at.getNamedAttr("low", literal(high ? shape.width - 1 : 0,
                                                   node, symbol))})
              ->getResult(0));
    }
    uint64_t indexWidth = llvm::Log2_64(shape.width - 1) + 1;
    index = indexBits.size() == 1
                ? indexBits.front()
                : createOp(at, loc, ac::BitsConcatOp::getOperationName(),
                           indexBits, TypeRange{type(indexWidth)}, {})
                      ->getResult(0);
  }
  fixedValues.insert(index);
  Value valid = compare("ne", input, constant(0, input.getType()));
  SmallVector<Value> results{index, valid};
  if (onehot) {
    Value population = lowerPopcountValue(input, shape, node, at, symbol);
    // Even for a singleton, compare preserves an unknown input as unknown.
    results.push_back(
        compare("ult", constant(1, shape.resultType), population));
  }
  return results;
}
FailureOr<SmallVector<Value>> Importer::fixedEncodingResults(
    const AstNode &node, OpBuilder &at, FlatSymbolRefAttr symbol,
    llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances) {
  StringRef name = fixedResultCall(node);
  auto high = validateFixedEncodingCall(node, name);
  if (failed(high))
    return failure();
  // Evaluate once, at natural width, before destination conversions.
  auto input = expression(node.item("args", 0), at, symbol, values, instances);
  if (failed(input))
    return failure();
  auto shape = resolveFixedInputShape(*input, node, symbol, name);
  if (failed(shape))
    return failure();
  return lowerFixedEncodingValue(*input, *shape, *high, name == "onehot_encode",
                                 node, at, symbol);
}
FailureOr<SliceShape> Importer::resolveSlice(const AstNode &slice, Type base,
                                             FlatSymbolRefAttr symbol) {
  if (slice.kind() != "Slice")
    return error() << "bit extraction requires slice syntax";
  if (!isa<ac::BitsType>(base))
    return mlir::emitError(slice.location(b.getContext(), source.path))
           << "slice requires explicitly unsigned fixed bits";

  ac::HardwareAnalysis analysis(*body);
  Operation *site = body->getOperation();
  auto width = analysis.getPackedWidth(base, {}, site);
  if (failed(width))
    return failure();
  auto evaluateInteger = [&](StringRef field,
                             StringRef label) -> FailureOr<llvm::APSInt> {
    AstNode value = slice.child(field);
    auto expression = staticExpr(value, symbol);
    if (failed(expression))
      return failure();
    if (!analysis.isStaticEvaluable(*expression, {}))
      return mlir::emitError(value.location(b.getContext(), source.path))
             << "slice " << label << " must be a bound static Integer";
    auto evaluated = analysis.evaluateStatic(*expression, {}, site);
    if (failed(evaluated))
      return failure();
    auto integer = dyn_cast<ac::MathIntAttr>(*evaluated);
    if (!integer)
      return mlir::emitError(value.location(b.getContext(), source.path))
             << "slice " << label << " must be a static Integer";
    return llvm::APSInt(integer.getCanonicalValue());
  };

  Attribute lowerField = slice.get("lower");
  Attribute upperField = slice.get("upper");
  Attribute stepField = slice.get("step");
  llvm::APSInt low = llvm::APSInt::getUnsigned(0);
  llvm::APSInt high = llvm::APSInt::getUnsigned(*width);
  if (!isa<UnitAttr>(lowerField)) {
    auto value = evaluateInteger("lower", "lower bound");
    if (failed(value))
      return failure();
    low = *value;
  }
  if (!isa<UnitAttr>(upperField)) {
    auto value = evaluateInteger("upper", "upper bound");
    if (failed(value))
      return failure();
    high = *value;
  }
  if (!isa<UnitAttr>(stepField)) {
    auto value = evaluateInteger("step", "step");
    if (failed(value))
      return failure();
    if (llvm::APSInt::compareValues(*value, llvm::APSInt::getUnsigned(1)) != 0)
      return mlir::emitError(slice.location(b.getContext(), source.path))
             << "slice step must be the static Integer 1";
  }

  llvm::APSInt zero = llvm::APSInt::getUnsigned(0);
  llvm::APSInt limit = llvm::APSInt::getUnsigned(*width);
  if (llvm::APSInt::compareValues(low, zero) < 0 ||
      llvm::APSInt::compareValues(low, high) >= 0 ||
      llvm::APSInt::compareValues(high, limit) > 0)
    return mlir::emitError(slice.location(b.getContext(), source.path))
           << "slice bounds require 0 <= lower < upper <= source width";

  // Conversion is safe only after the arbitrary-precision range proof above.
  uint64_t lowValue = low.getZExtValue();
  uint64_t resultWidth = high.getZExtValue() - lowValue;
  return SliceShape{
      ac::BitsType::get(b.getContext(), literal(resultWidth, slice, symbol)),
      literal(lowValue, slice, symbol)};
}
LogicalResult Importer::scanStructs() {
  for (auto &item : structs) {
    SmallVector<Attribute> fields;
    SmallVector<AstNode> defaults;
    llvm::StringSet<> seen;
    auto symbol = FlatSymbolRefAttr::get(b.getContext(), qualify(item.first()));
    for (size_t i = 0; i < item.second.array("body").size(); ++i) {
      auto field = item.second.item("body", i);
      if (docstring(field))
        continue;
      if (field.kind() != "AnnAssign" || field.child("target").kind() != "Name")
        return error() << "struct fields require named annotations";
      auto name = field.child("target").string("id");
      if (!seen.insert(name).second)
        return error() << "duplicate struct field";
      auto annotation = field.child("annotation");
      bool tableAnnotation = annotation.kind() == "Subscript" &&
                             intrinsic(annotation.child("value"), "table");
      if (!fixedAnnotation(annotation) && !nominalAnnotation(annotation) &&
          !tableAnnotation)
        return error() << "struct fields require explicitly unsigned bits or "
                          "nominal structs/enums or finite tables";
      auto type = portType(annotation, symbol);
      if (failed(type))
        return failure();
      if (!isa<ac::BitsType, ac::StructType, ac::EnumType, ac::TableType>(*type))
        return error() << "struct field requires bits, struct, enum or table";
      if (auto table = dyn_cast<ac::TableType>(*type)) {
        auto shape = ac::HardwareAnalysis(*body).resolveShape(
            table.getShape(), {}, body->getOperation());
        if (failed(shape))
          return failure();
        if (shape->size() != 1)
          return error()
                 << "struct Table field requires a positive closed "
                    "one-dimensional shape";
      }
      defaults.push_back(field.child("value"));
      fields.push_back(
          b.getDictionaryAttr({b.getNamedAttr("name", b.getStringAttr(name)),
                               b.getNamedAttr("type", TypeAttr::get(*type))}));
    }
    structFields[item.first()] = b.getArrayAttr(fields);
    structDefaults[item.first()] = std::move(defaults);
  }
  return success();
}
FailureOr<Value> Importer::constant(Type type, unsigned value,
                                    const AstNode &node, OpBuilder &at,
                                    FlatSymbolRefAttr symbol) {
  auto loc = node.location(b.getContext(), source.path);
  if (auto enumeration = dyn_cast<ac::EnumType>(type)) {
    if (value)
      return error() << "recursive Enum zero requires zero value";
    auto definition = ac::HardwareAnalysis(*body).resolveEnum(
        enumeration, body->getOperation());
    if (failed(definition))
      return failure();
    for (const auto &member : definition->members)
      if (llvm::APSInt(member.code.getCanonicalValue()).isZero())
        return createOp(at, loc, ac::EnumCreateOp::getOperationName(), {},
                        TypeRange{type},
                        {at.getNamedAttr("member", member.name)})
            ->getResult(0);
    return error()
           << "recursive Enum zero requires a declared zero-code member";
  }
  if (isa<ac::BitsType>(type)) {
    auto result =
        createOp(at, loc, ac::BitsConstantOp::getOperationName(), {},
                 TypeRange{type},
                 {at.getNamedAttr("value", literal(value, node, symbol))})
            ->getResult(0);
    fixedValues.insert(result);
    return result;
  }
  if (value)
    return error() << "aggregate contextual literal must be zero";
  if (auto table = dyn_cast<ac::TableType>(type)) {
    auto element = constant(table.getElementType(), 0, node, at, symbol);
    if (failed(element))
      return failure();
    return createOp(at, loc, ac::TableSplatOp::getOperationName(), {*element},
                    TypeRange{type},
                    {at.getNamedAttr("shape", table.getShape())})
        ->getResult(0);
  }
  auto record = dyn_cast<ac::StructType>(type);
  ac::HardwareAnalysis analysis(*body);
  auto declaration = record ? analysis.lookupStruct(record) : ac::StructOp();
  if (!declaration)
    return error() << "unknown aggregate type";
  SmallVector<Value> fields;
  for (Attribute raw : declaration.getFields()) {
    auto field = cast<DictionaryAttr>(raw);
    auto zero =
        constant(field.getAs<TypeAttr>("type").getValue(), 0, node, at, symbol);
    if (failed(zero))
      return failure();
    fields.push_back(*zero);
  }
  return createOp(at, loc, ac::StructCreateOp::getOperationName(), fields,
                  TypeRange{type}, {})
      ->getResult(0);
}
FailureOr<Value> Importer::select(Value guard, Value yes, Value no,
                                  const AstNode &node, OpBuilder &at,
                                  FlatSymbolRefAttr symbol) {
  if (yes == no)
    return yes;
  if (!ac::areEquivalentHardwareTypes(yes.getType(), no.getType()))
    return error() << "branch values require equal hardware types";
  auto loc = node.location(b.getContext(), source.path);
  if (isa<ac::EnumType>(yes.getType())) {
    auto a = enumToBits(yes, node, at, symbol);
    auto c = enumToBits(no, node, at, symbol);
    if (failed(a) || failed(c))
      return failure();
    auto selected = createOp(at, loc, ac::BitsSelectOp::getOperationName(),
                             {guard, *a, *c}, TypeRange{a->getType()}, {})
                        ->getResult(0);
    return createOp(at, loc, ac::EnumFromBitsOp::getOperationName(), {selected},
                    TypeRange{yes.getType(), bits(1, node, symbol)}, {})
        ->getResult(0);
  }
  if (isa<ac::BitsType>(yes.getType())) {
    auto result = createOp(at, loc, ac::BitsSelectOp::getOperationName(),
                           {guard, yes, no}, TypeRange{yes.getType()}, {})
                      ->getResult(0);
    if (fixedValues.contains(yes) && fixedValues.contains(no))
      fixedValues.insert(result);
    else if (valueInfo(yes).sourceKind == ac::detail::ValueKind::Boolean &&
             valueInfo(no).sourceKind == ac::detail::ValueKind::Boolean)
      remember(result, ac::detail::ValueKind::Boolean);
    return result;
  }
  if (auto record = dyn_cast<ac::StructType>(yes.getType())) {
    ac::HardwareAnalysis analysis(*body);
    SmallVector<Value> result;
    for (Attribute raw : analysis.lookupStruct(record).getFields()) {
      auto field = cast<DictionaryAttr>(raw);
      auto type = field.getAs<TypeAttr>("type").getValue();
      auto name = field.getAs<StringAttr>("name");
      auto project = [&](Value value) {
        return createOp(at, loc, ac::StructGetOp::getOperationName(), {value},
                        TypeRange{type}, {at.getNamedAttr("field", name)})
            ->getResult(0);
      };
      auto value = select(guard, project(yes), project(no), node, at, symbol);
      if (failed(value))
        return failure();
      result.push_back(*value);
    }
    return createOp(at, loc, ac::StructCreateOp::getOperationName(), result,
                    TypeRange{record}, {})
        ->getResult(0);
  }
  auto table = cast<ac::TableType>(yes.getType());
  ac::HardwareAnalysis analysis(*body);
  auto size =
      analysis.getTableSize(table, {}, at.getInsertionBlock()->getParentOp());
  if (failed(size))
    return failure();
  auto guards =
      createOp(at, loc, ac::TableSplatOp::getOperationName(), {guard},
               TypeRange{ac::TableType::get(b.getContext(), table.getShape(),
                                            guard.getType())},
               {at.getNamedAttr("shape", table.getShape())})
          ->getResult(0);
  auto map = createOp(
      at, loc, ac::TableMapOp::getOperationName(), {guards, yes, no},
      TypeRange{table},
      {at.getNamedAttr("shape", table.getShape()),
       at.getNamedAttr("operandSegmentSizes", at.getDenseI32ArrayAttr({3, 0}))},
      1);
  auto block = new Block();
  map->getRegion(0).push_back(block);
  block->addArgument(
      bits(std::max(1u, llvm::Log2_64_Ceil(*size)), node, symbol), loc);
  auto g = block->addArgument(guard.getType(), loc);
  auto a = block->addArgument(table.getElementType(), loc);
  auto c = block->addArgument(table.getElementType(), loc);
  auto nested = OpBuilder::atBlockEnd(block);
  auto merged = select(g, a, c, node, nested, symbol);
  if (failed(merged))
    return failure();
  createOp(nested, loc, ac::YieldOp::getOperationName(), {*merged}, {}, {});
  return map->getResult(0);
}
LogicalResult Importer::proveIndex(Value table, Value index,
                                   const AstNode &node) {
  auto info = valueInfo(index);
  bool knownUnsigned = fixedValues.contains(index) ||
                       info.sourceKind == ac::detail::ValueKind::Boolean ||
                       (info.sourceKind == ac::detail::ValueKind::Integer &&
                        info.interval && !info.interval->lower.isNegative());
  if (!knownUnsigned)
    return mlir::emitError(node.location(b.getContext(), source.path))
           << "table index requires proven nonnegative unsigned provenance";
  ac::HardwareAnalysis analysis(*body);
  auto size = analysis.getTableSize(
      cast<ac::TableType>(table.getType()), {},
      table.getDefiningOp()
          ? table.getDefiningOp()
          : cast<BlockArgument>(table).getOwner()->getParentOp());
  auto width = analysis.getPackedWidth(
      index.getType(), {},
      index.getDefiningOp()
          ? index.getDefiningOp()
          : cast<BlockArgument>(index).getOwner()->getParentOp());
  if (failed(size) || failed(width))
    return failure();
  if (info.interval && !info.interval->lower.isNegative() &&
      llvm::APSInt::compareValues(info.interval->lower, info.interval->upper) < 0 &&
      llvm::APSInt::compareValues(info.interval->upper,
                                 llvm::APSInt::getUnsigned(*size)) <= 0)
    return success();
  if (*width >= 64 || (uint64_t(1) << *width) > *size)
    return mlir::emitError(node.location(b.getContext(), source.path))
           << "table index complete width is not proven in range";
  return success();
}
FailureOr<Value> Importer::tableUpdate(Value table, Value index,
                                       Value replacement,
                                       ArrayRef<Attribute> fieldPath,
                                       const AstNode &node,
                                       OpBuilder &at,
                                       FlatSymbolRefAttr symbol,
                                       std::optional<size_t> domainSlot) {
  if (failed(proveIndex(table, index, node)))
    return failure();
  auto type = cast<ac::TableType>(table.getType());
  SmallVector<Type> resultTypes{type};
  if (domainSlot) {
    if (!activeRule || *domainSlot >= activeRule->domains.size())
      return error() << "invalid assignment-time address witness slot";
    auto &witness = activeRule->domains[*domainSlot];
    if (witness.index || witness.mask || !isa<ac::BitsType>(index.getType()) ||
        !sameSourceSite(witness.assignment, node))
      return error() << "invalid or duplicate assignment-time address witness";
    witness.index = index;
    resultTypes.push_back(witness.maskType);
  }
  auto loc = node.location(b.getContext(), source.path);
  auto splat = [&](Value value) {
    return createOp(at, loc, ac::TableSplatOp::getOperationName(), {value},
                    TypeRange{ac::TableType::get(
                        b.getContext(), type.getShape(), value.getType())},
                    {at.getNamedAttr("shape", type.getShape())})
        ->getResult(0);
  };
  auto map = createOp(
      at, loc, ac::TableMapOp::getOperationName(),
      {table, splat(index), splat(replacement)}, resultTypes,
      {at.getNamedAttr("shape", type.getShape()),
       at.getNamedAttr("operandSegmentSizes", at.getDenseI32ArrayAttr({3, 0}))},
      1);
  ac::HardwareAnalysis analysis(*body);
  auto size = analysis.getTableSize(type, {}, map);
  if (failed(size))
    return failure();
  auto block = new Block();
  map->getRegion(0).push_back(block);
  auto ordinalWidth = std::max(1u, llvm::Log2_64_Ceil(*size));
  auto ordinalType = bits(ordinalWidth, node, symbol);
  Value ordinal = block->addArgument(ordinalType, loc);
  auto old = block->addArgument(type.getElementType(), loc);
  Value idx = block->addArgument(index.getType(), loc);
  auto next = block->addArgument(replacement.getType(), loc);
  auto nested = OpBuilder::atBlockEnd(block);
  // Range proof does not establish known bits. Compare at the wider width so
  // high X/Z index bits participate, and normalize equal-width spelling only.
  auto width = analysis.getPackedWidth(idx.getType(), {}, map);
  if (failed(width))
    return failure();
  if (*width == ordinalWidth &&
      !ac::areEquivalentHardwareTypes(idx.getType(), ordinalType)) {
    auto equivalent = equivalentBoundaryTypes(idx.getType(), ordinalType, map);
    if (failed(equivalent))
      return failure();
    if (!*equivalent)
      return error() << "table index comparison requires equivalent equal-width types";
    idx = boundaryBitsIdentity(idx, ordinalType, node, nested, symbol);
  } else if (*width < ordinalWidth) {
    idx = createOp(nested, loc, ac::BitsResizeOp::getOperationName(), {idx},
                   TypeRange{ordinalType},
                   {nested.getNamedAttr("mode", nested.getStringAttr("zext"))})
              ->getResult(0);
  } else if (*width > ordinalWidth) {
    ordinal = createOp(nested, loc, ac::BitsResizeOp::getOperationName(), {ordinal},
                       TypeRange{idx.getType()},
                       {nested.getNamedAttr("mode", nested.getStringAttr("zext"))})
                  ->getResult(0);
  }
  auto equal =
      createOp(nested, loc, ac::BitsCompareOp::getOperationName(),
               {ordinal, idx}, TypeRange{bits(1, node, symbol)},
               {nested.getNamedAttr("predicate", nested.getStringAttr("eq"))})
          ->getResult(0);
  Value value;
  if (fieldPath.empty()) {
    auto selected = select(equal, next, old, node, nested, symbol);
    if (failed(selected))
      return failure();
    value = *selected;
  } else {
    // The index equality masks data only; the source owner enable is separate.
    // Use each current element as the base so untouched fields never read
    // through the possibly unknown index.
    value = createOp(
                nested, loc, ac::ValueMergeOp::getOperationName(),
                {old, equal, next}, TypeRange{old.getType(), equal.getType()},
                {nested.getNamedAttr(
                     "paths", nested.getArrayAttr({nested.getArrayAttr(fieldPath)})),
                 nested.getNamedAttr("operandSegmentSizes",
                                     nested.getDenseI32ArrayAttr({1, 1, 1}))})
                ->getResult(0);
  }
  SmallVector<Value> yields{value};
  if (domainSlot) {
    yields.push_back(equal);
    activeRule->domains[*domainSlot].mask = map->getResult(1);
  }
  createOp(nested, loc, ac::YieldOp::getOperationName(), yields, {}, {});
  return map->getResult(0);
}

FailureOr<Value> Importer::unsignedBoundary(Value value, Type target,
                                            const AstNode &node, OpBuilder &at,
                                            FlatSymbolRefAttr symbol,
                                            bool allowFixedWidening) {
  auto info = valueInfo(value);
  auto targetBits = dyn_cast<ac::BitsType>(target);
  auto equivalent = equivalentBoundaryTypes(value.getType(), target,
                                            at.getBlock()->getParentOp());
  if (failed(equivalent))
    return failure();
  if (targetBits && !fixedValues.contains(value) && !info.sourceKind)
    return mlir::emitError(node.location(b.getContext(), source.path))
           << "unsigned boundary requires authoritative bits, Boolean or "
              "Integer provenance";
  if (targetBits && info.sourceKind == ac::detail::ValueKind::Integer &&
      !fixedValues.contains(value)) {
    if (*equivalent &&
        !ac::areEquivalentHardwareTypes(value.getType(), target)) {
      value = boundaryBitsIdentity(value, target, node, at, symbol);
      info = valueInfo(value);
    }
    auto converted = lowerExactIntegerBoundary(at, numericSite(node, symbol),
                                               info, targetBits);
    if (failed(converted))
      return failure();
    value = *converted;
  } else if (*equivalent && targetBits &&
             isa<ac::BitsType>(value.getType()) &&
             fixedValues.contains(value) &&
             !ac::areEquivalentHardwareTypes(value.getType(), target)) {
    // Only the width spelling changes. Keep the producer's kind and value facts
    // while giving this boundary wire the declared destination type.
    return boundaryBitsIdentity(value, target, node, at, symbol);
  } else if (allowFixedWidening && targetBits &&
             isa<ac::BitsType>(value.getType()) &&
             fixedValues.contains(value) &&
             !*equivalent) {
    ac::HardwareAnalysis analysis(*body);
    auto *owner = at.getBlock()->getParentOp();
    auto sourceWidth = analysis.getPackedWidth(value.getType(), {}, owner);
    auto targetWidth = analysis.getPackedWidth(target, {}, owner);
    if (failed(sourceWidth) || failed(targetWidth))
      return failure();
    if (*sourceWidth > *targetWidth)
      return mlir::emitError(node.location(b.getContext(), source.path))
             << "unsigned boundary implicit narrowing is unsupported";
    auto loc = node.location(b.getContext(), source.path);
    auto result =
        *sourceWidth == *targetWidth
            ? createOp(at, loc, ac::BitsExtractOp::getOperationName(), {value},
                       TypeRange{target},
                       {at.getNamedAttr("low", literal(0, node, symbol))})
                  ->getResult(0)
            : createOp(at, loc, ac::BitsResizeOp::getOperationName(), {value},
                       TypeRange{target},
                       {at.getNamedAttr("mode", at.getStringAttr("zext"))})
                  ->getResult(0);
    fixedValues.insert(result);
    return result;
  } else if (!*equivalent)
    return mlir::emitError(node.location(b.getContext(), source.path))
           << "unsigned boundary hardware type mismatch";
  if (!targetBits || fixedValues.contains(value))
    return value;
  // A fresh identity wire gives the destination unsigned facts without changing
  // the Boolean/Integer producer, including any aliases already captured.
  auto result = createOp(at, node.location(b.getContext(), source.path),
                         ac::BitsExtractOp::getOperationName(), {value},
                         TypeRange{target},
                         {at.getNamedAttr("low", literal(0, node, symbol))})
                    ->getResult(0);
  fixedValues.insert(result);
  return result;
}
