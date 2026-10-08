// Source defaults remain importer facts, never published declaration metadata.
bool Importer::staticInitializer(const AstNode &node) const {
  auto member = resolveEnumMember(node);
  if (failed(member))
    return false;
  if (*member)
    return true;
  if (node.kind() == "Constant")
    return true;
  if (node.kind() == "BinOp")
    return staticInitializer(node.child("left")) &&
           staticInitializer(node.child("right"));
  if (node.kind() != "Call" || node.child("func").kind() != "Name" ||
      !structs.contains(node.child("func").string("id")))
    return false;
  for (size_t i = 0; i < node.array("args").size(); ++i)
    if (!staticInitializer(node.item("args", i)))
      return false;
  for (size_t i = 0; i < node.array("keywords").size(); ++i)
    if (!staticInitializer(node.item("keywords", i).child("value")))
      return false;
  return true;
}
FailureOr<Value> Importer::constructStruct(
    const AstNode &node, OpBuilder &at, FlatSymbolRefAttr symbol,
    llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances,
    std::optional<Type> expected,
    llvm::StringMap<SmallVector<Value>> *captures) {
  StringRef name = node.child("func").string("id");
  auto binding = lookupBinding(node.child("func"));
  if (!binding || binding->category != BindingCategory::Struct || !binding->symbol)
    return error() << "struct construction requires a bound nominal declaration";
  auto type = ac::StructType::get(
      b.getContext(), b.getStringAttr(binding->symbol.getValue()));
  if (expected && !ac::areEquivalentHardwareTypes(type, *expected))
    return error()
           << "struct constructor nominal type disagrees with destination";
  bool localDefaults = structs.contains(name);
  auto declaration = dyn_cast_or_null<ac::StructOp>(binding->declaration);
  auto fields = localDefaults ? structFields[name]
                             : (declaration ? declaration.getFields() : ArrayAttr());
  if (!fields)
    return error() << "struct constructor lacks its published field declaration";
  if (!node.array("args").empty())
    return error() << "struct construction requires named fields";
  llvm::StringMap<AstNode> actuals;
  for (size_t i = 0; i < node.array("keywords").size(); ++i) {
    auto kw = node.item("keywords", i);
    auto key = dyn_cast_or_null<StringAttr>(kw.get("arg"));
    if (!key || actuals.contains(key.getValue()))
      return error() << "duplicate struct constructor field";
    if (!llvm::any_of(fields, [&](Attribute raw) {
          return cast<DictionaryAttr>(raw).getAs<StringAttr>("name") == key;
        }))
      return error() << "unknown struct constructor field";
    actuals[key.getValue()] = kw.child("value");
  }
  SmallVector<Value> operands;
  for (auto [index, raw] : llvm::enumerate(fields)) {
    auto field = cast<DictionaryAttr>(raw);
    StringRef key = field.getAs<StringAttr>("name").getValue();
    Type fieldType = field.getAs<TypeAttr>("type").getValue();
    AstNode valueSite = node;
    FailureOr<Value> value = failure();
    if (actuals.contains(key)) {
      valueSite = actuals[key];
      value = expression(valueSite, at, symbol, values, instances, fieldType,
                         captures);
    } else if (!localDefaults) {
      return error() << "imported struct construction requires every field explicitly; "
                        "provider defaults are not published";
    } else if (AstNode defaultNode = structDefaults[name][index]) {
      if (!staticInitializer(defaultNode))
        return error()
               << "struct field default must be a static complete value";
      valueSite = defaultNode;
      llvm::StringMap<Value> noValues;
      llvm::StringMap<Instance *> noInstances;
      value = expression(defaultNode, at,
                         FlatSymbolRefAttr::get(b.getContext(), qualify(name)),
                         noValues, noInstances, fieldType);
    } else
      value = constant(fieldType, 0, node, at, symbol);
    if (failed(value))
      return failure();
    auto converted = unsignedBoundary(*value, fieldType, valueSite, at, symbol);
    if (failed(converted))
      return failure();
    operands.push_back(*converted);
  }
  return createOp(at, node.location(b.getContext(), source.path),
                  ac::StructCreateOp::getOperationName(), operands,
                  TypeRange{type}, {})
      ->getResult(0);
}
LogicalResult Importer::validateDefaults() {
  OwningOpRef<ModuleOp> scratch(ModuleOp::create(body->getLoc()));
  auto at = OpBuilder::atBlockEnd(scratch->getBody());
  llvm::StringMap<Value> values;
  llvm::StringMap<Instance *> instances;
  LogicalResult status = success();
  for (auto &item : structDefaults) {
    auto symbol = FlatSymbolRefAttr::get(b.getContext(), qualify(item.first()));
    for (auto [index, node] : llvm::enumerate(item.second)) {
      if (!node)
        continue;
      if (!staticInitializer(node)) {
        error() << "struct field default must be a static complete value";
        status = failure();
        break;
      }
      Type type = cast<DictionaryAttr>(structFields[item.first()][index])
                      .getAs<TypeAttr>("type")
                      .getValue();
      auto value = expression(node, at, symbol, values, instances, type);
      if (failed(value) ||
          failed(unsignedBoundary(*value, type, node, at, symbol))) {
        auto field = cast<DictionaryAttr>(structFields[item.first()][index])
                         .getAs<StringAttr>("name");
        mlir::emitError(node.location(b.getContext(), source.path))
            << "invalid static struct field default '" << item.first() << "."
            << field.getValue() << "'";
        status = failure();
        break;
      }
    }
    if (failed(status))
      break;
  }
  // Values from validation are discarded, including all private SSA facts.
  scratch->walk([&](Operation *op) {
    for (Value result : op->getResults()) {
      numericValues.erase(result);
      arithmeticValues.erase(result);
      fixedValues.erase(result);
    }
  });
  return status;
}
