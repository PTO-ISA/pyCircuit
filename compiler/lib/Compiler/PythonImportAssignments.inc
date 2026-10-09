// This is a fixed-result producer contract, not general tuple unpacking.
FailureOr<std::optional<SmallVector<StringRef, 2>>>
Importer::fixedResultTargets(const AstNode &assignment) const {
  StringRef producer = fixedResultCall(assignment.child("value"));
  if (producer.empty())
    return std::optional<SmallVector<StringRef, 2>>();
  size_t arity = producer == "onehot_encode" ? 3 : 2;
  StringRef arityName = arity == 3 ? "three" : "two";
  auto diagnostic = [&] {
    return mlir::emitError(assignment.location(b.getContext(), source.path));
  };
  if (assignment.kind() != "Assign" || assignment.array("targets").size() != 1)
    return diagnostic() << producer << " requires one " << arityName
                        << "-name Tuple target; unpack first";
  auto target = assignment.item("targets", 0);
  if (target.kind() != "Tuple" || target.array("elts").size() != arity)
    return diagnostic() << producer << " requires exactly " << arityName
                        << " ordinary Names; unpack first";
  SmallVector<StringRef, 2> targets;
  for (size_t i = 0; i < arity; ++i) {
    auto element = target.item("elts", i);
    if (element.kind() != "Name")
      return diagnostic() << producer << " targets must be ordinary Names";
    StringRef name = element.string("id");
    bool reserved = llvm::is_contained(
        ArrayRef<StringRef>{"pyc_clk", "pyc_rst", "module", "rule", "system",
                            "log", "report", "bool", "int", "range",
                            "Annotated"},
        name);
    if (name.empty() || reserved || names.contains(name) ||
        structs.contains(name) || behavioralRules.contains(name))
      return diagnostic() << producer
                          << " target shadows a reserved/source binding: '"
                          << name << "'";
    if (llvm::is_contained(targets, name))
      return diagnostic() << producer << " targets must be distinct Names";
    targets.push_back(name);
  }
  return std::optional<SmallVector<StringRef, 2>>(std::move(targets));
}
FailureOr<bool> Importer::bindFixedResults(
    const AstNode &assignment, OpBuilder &at, FlatSymbolRefAttr symbol,
    llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances,
    llvm::StringMap<BindingBoundary> *boundaries,
    const llvm::StringSet<> &forbidden, bool immutable) {
  auto targets = fixedResultTargets(assignment);
  if (failed(targets))
    return failure();
  if (!*targets)
    return false;
  for (StringRef name : **targets)
    if (forbidden.contains(name) || instances.contains(name) ||
        (immutable && values.contains(name)))
      return mlir::emitError(assignment.location(b.getContext(), source.path))
             << fixedResultCall(assignment.child("value"))
             << " target cannot write an owner or shadow a "
                "binding: '"
             << name << "'";
  // The private producer lowers its raw argument exactly once and the common
  // verifier owns exact-width and nominal conversion checks.
  auto call = assignment.child("value");
  StringRef producer = fixedResultCall(call);
  bool query = producer == "first" || producer == "argmin", committed = false;
  auto savedQueryBudget = queryBudget;
  auto restoreQueryPlan = llvm::scope_exit([&] {
    if (query && !committed) queryBudget = savedQueryBudget;
  });
  auto results = producer == "enum_from_bits"
                     ? enumFromBits(call, at, symbol, values, instances)
                 : producer == "first" || producer == "argmin"
                     ? tableQueryResults(call, at, symbol, values, instances)
                     : fixedEncodingResults(call, at, symbol, values, instances);
  if (failed(results))
    return failure();
  SmallVector<Value, 2> converted;
  SmallVector<BindingBoundary, 2> contracts;
  for (auto [index, name] : llvm::enumerate(**targets)) {
    BindingBoundary binding =
        inferredBoundary(name, (*results)[index], assignment);
    if (boundaries && boundaries->contains(name))
      binding = boundaries->lookup(name);
    else if (values.contains(name))
      binding = inferredBoundary(name, values.lookup(name), assignment);
    auto value = applyBindingBoundary((*results)[index], binding,
                                      assignment.child("value"), at, symbol);
    if (failed(value))
      return failure();
    converted.push_back(*value);
    contracts.push_back(binding);
  }
  // No local is visible until every destination contract has succeeded.
  for (auto [index, name] : llvm::enumerate(**targets)) {
    values[name] = converted[index];
    if (boundaries)
      (*boundaries)[name] = contracts[index];
  }
  committed = true;
  return true;
}
StringRef Importer::assignmentRoot(const AstNode &target) const {
  if (target.kind() == "Name")
    return target.string("id");
  if (target.kind() == "Attribute" || target.kind() == "Subscript")
    return assignmentRoot(target.child("value"));
  return {};
}
AstNode Importer::indexedFieldTarget(const AstNode &target) const {
  if (target.kind() != "Attribute")
    return {};
  auto base = target;
  while (base.kind() == "Attribute")
    base = base.child("value");
  if (base.kind() == "Subscript" && base.child("value").kind() == "Name" &&
      base.child("slice").kind() != "Slice")
    return base;
  return {};
}
FailureOr<Value> Importer::replaceField(Value value, StringRef name,
                                        Value replacement, const AstNode &node,
                                        OpBuilder &at,
                                        FlatSymbolRefAttr symbol) {
  auto type = dyn_cast<ac::StructType>(value.getType());
  ac::HardwareAnalysis analysis(*body);
  auto declaration = type ? analysis.lookupStruct(type) : ac::StructOp();
  if (!declaration)
    return error() << "field assignment requires a nominal struct";
  bool found = false;
  SmallVector<Value> fields;
  for (Attribute raw : declaration.getFields()) {
    auto field = cast<DictionaryAttr>(raw);
    Type fieldType = field.getAs<TypeAttr>("type").getValue();
    auto key = field.getAs<StringAttr>("name");
    if (key.getValue() == name) {
      auto converted =
          unsignedBoundary(replacement, fieldType, node, at, symbol);
      if (failed(converted))
        return failure();
      fields.push_back(*converted);
      found = true;
    } else {
      auto projected =
          createOp(at, node.location(b.getContext(), source.path),
                   ac::StructGetOp::getOperationName(), {value},
                   TypeRange{fieldType}, {at.getNamedAttr("field", key)})
              ->getResult(0);
      if (isa<ac::BitsType>(fieldType))
        fixedValues.insert(projected);
      fields.push_back(projected);
    }
  }
  if (!found)
    return error() << "unknown struct assignment field";
  return createOp(at, node.location(b.getContext(), source.path),
                  ac::StructCreateOp::getOperationName(), fields,
                  TypeRange{type}, {})
      ->getResult(0);
}
FailureOr<std::optional<size_t>> Importer::addressDomainSlot(
    StringRef formal, const AstNode &assignment, const AstNode &index) {
  if (!activeRule)
    return std::optional<size_t>();
  std::optional<size_t> slot;
  for (auto [i, witness] : llvm::enumerate(activeRule->domains)) {
    if (!grantProof->charge(1 + assignment.path.size() + index.path.size()))
      return error() << grantProof->failureReason();
    if (witness.formal != formal ||
        !sameSourceSite(witness.assignment, assignment) ||
        !sameSourceSite(witness.occurrence, index))
      continue;
    if (slot)
      return error() << "duplicate assignment-time address witness slot";
    slot = i;
  }
  return slot;
}

LogicalResult Importer::updateAssignment(const AstNode &target,
                                         Value replacement, const AstNode &node,
                                         OpBuilder &at,
                                         FlatSymbolRefAttr symbol,
                                         llvm::StringMap<Value> &values) {
  StringRef root = assignmentRoot(target);
  if (isa<ac::ModuleOp>(at.getInsertionBlock()->getParentOp()) &&
      (queueResultNames.contains(root) || memoryResultNames.contains(root)))
    return error() << "queue result binding is immutable: '" << root << "'";
  if (root == "pyc_clk" || root == "pyc_rst")
    return error() << "reserved physical domain pin '" << root
                   << "' cannot be assigned in source";
  if (target.kind() == "Name") {
    StringRef name = target.string("id");
    if (names.contains(name) || structs.contains(name) ||
        behavioralRules.contains(name))
      return error() << "assignment shadows a source declaration";
    values[name] = replacement;
    return success();
  }
  llvm::StringMap<Instance *> noInstances;
  if (auto element = indexedFieldTarget(target)) {
    StringRef name = element.child("value").string("id");
    if (!values.contains(name) || !isa<ac::TableType>(values[name].getType()))
      return error() << "indexed assignment requires a named table";
    auto targetType = expressionType(target, values, noInstances, symbol);
    if (failed(targetType))
      return failure();
    auto converted = unsignedBoundary(replacement, *targetType, node, at, symbol);
    if (failed(converted))
      return failure();
    auto index =
        expression(element.child("slice"), at, symbol, values, noInstances);
    if (failed(index))
      return failure();
    SmallVector<Attribute> fieldPath;
    for (auto field = target; field.kind() == "Attribute";
         field = field.child("value"))
      fieldPath.push_back(at.getStringAttr(field.string("attr")));
    std::reverse(fieldPath.begin(), fieldPath.end());
    auto slot = addressDomainSlot(name, node, element.child("slice"));
    if (failed(slot))
      return failure();
    auto updated = tableUpdate(values[name], *index, *converted, fieldPath, node,
                               at, symbol, *slot);
    if (failed(updated))
      return failure();
    values[name] = *updated;
    return success();
  }
  if (target.kind() == "Attribute") {
    auto base =
        expression(target.child("value"), at, symbol, values, noInstances);
    if (failed(base))
      return failure();
    auto updated = replaceField(*base, target.string("attr"), replacement, node,
                                at, symbol);
    if (failed(updated))
      return failure();
    return updateAssignment(target.child("value"), *updated, node, at, symbol,
                            values);
  }
  if (target.kind() == "Subscript" && target.child("value").kind() == "Name" &&
      values.contains(target.child("value").string("id"))) {
    StringRef name = target.child("value").string("id");
    if (!isa<ac::TableType>(values[name].getType()))
      return error() << "indexed assignment requires a table";
    auto index =
        expression(target.child("slice"), at, symbol, values, noInstances);
    if (failed(index))
      return failure();
    auto slot = addressDomainSlot(name, node, target.child("slice"));
    if (failed(slot))
      return failure();
    auto updated = tableUpdate(values[name], *index, replacement, {}, node, at,
                               symbol, *slot);
    if (failed(updated))
      return failure();
    values[name] = *updated;
    return success();
  }
  return error() << "assignment requires a local name, nested field or named "
                    "table index";
}
