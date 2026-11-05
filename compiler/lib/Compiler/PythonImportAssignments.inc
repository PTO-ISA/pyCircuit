// Preparation owns the destination contract; evaluation still receives its
// contextual type before any conversion. No environment entry changes until
// every result has passed its boundary.
FailureOr<LocalAssignmentBinding> Importer::prepareLocalAssignment(
    const AstNode &statement, const BranchEnvironment &environment,
    FlatSymbolRefAttr symbol) {
  bool annotated = statement.kind() == "AnnAssign";
  auto target = annotated ? statement.child("target")
                          : statement.item("targets", 0);
  StringRef name = target.string("id");
  LocalAssignmentBinding prepared;
  prepared.destination.name = name;
  auto existing = environment.boundaries.find(name);
  if (existing != environment.boundaries.end() && existing->second.declared)
    prepared.destination.boundary = existing->second;
  if (annotated) {
    auto annotation = statement.child("annotation");
    auto type = portType(annotation, symbol);
    if (failed(type))
      return failure();
    BindingBoundary declared{{name.str(), *type, annotation,
                              annotationKind(annotation),
                              fixedAnnotation(annotation)}, true};
    auto &binding = prepared.destination.boundary;
    if (binding && !sameBindingBoundary(*binding, declared))
      return mlir::emitError(statement.location(b.getContext(), source.path))
             << "annotation cannot replace a declared binding boundary";
    if (!binding)
      binding = std::move(declared);
  }
  if (prepared.destination.boundary &&
      prepared.destination.boundary->contract().type)
    prepared.expected = prepared.destination.boundary->contract().type;
  return prepared;
}

LogicalResult Importer::bindLocalValues(
    ArrayRef<PreparedLocalBinding> destinations, ValueRange results,
    const AstNode &node, OpBuilder &at, FlatSymbolRefAttr symbol,
    llvm::StringMap<Value> &values,
    llvm::StringMap<BindingBoundary> *boundaries, bool noteDataBindings) {
  assert(destinations.size() == results.size());
  SmallVector<Value, 1> converted;
  for (auto [destination, result] : llvm::zip(destinations, results)) {
    const auto &binding = destination.boundary;
    if (binding && binding->declared) {
      auto value = applyBindingBoundary(result, *binding, node, at, symbol);
      if (failed(value))
        return failure();
      result = *value;
    }
    converted.push_back(result);
  }
  for (auto [index, destination] : llvm::enumerate(destinations)) {
    values[destination.name] = converted[index];
    if (boundaries) {
      if (destination.boundary && destination.boundary->declared)
        (*boundaries)[destination.name] = *destination.boundary;
      else
        boundaries->erase(destination.name);
    }
    if (noteDataBindings)
      noteNestedRuleDataBinding(destination.name);
  }
  return success();
}

FailureOr<Value> Importer::lowerBoundExpression(
    const AstNode &node, const BindingBoundary &binding, OpBuilder &at,
    FlatSymbolRefAttr symbol, llvm::StringMap<Value> &values,
    llvm::StringMap<Instance *> &instances) {
  auto value = expression(node, at, symbol, values, instances,
                          binding.contract().type);
  if (failed(value)) return failure();
  return applyBindingBoundary(*value, binding, node, at, symbol);
}

LogicalResult Importer::lowerLocalAssignment(
    const AstNode &statement, OpBuilder &at, FlatSymbolRefAttr symbol,
    BranchEnvironment &environment, llvm::StringMap<Instance *> &instances) {
  auto target = statement.kind() == "AnnAssign" ? statement.child("target")
                                               : statement.item("targets", 0);
  if (failed(validateAssignmentRoot(target, statement, at)))
    return failure();
  auto prepared = prepareLocalAssignment(statement, environment, symbol);
  if (failed(prepared))
    return failure();
  auto rhs = statement.child("value");
  auto value = expression(rhs, at, symbol, environment.values, instances,
                          prepared->expected);
  if (failed(value))
    return failure();
  return bindLocalValues({prepared->destination}, {*value}, rhs, at, symbol,
                         environment.values, &environment.boundaries,
                         activeModuleStatements != nullptr);
}

// This is a fixed-result producer contract, not general tuple unpacking.
FailureOr<std::optional<SmallVector<StringRef, 2>>>
Importer::fixedResultTargets(const AstNode &assignment) {
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
    if (failed(requireNestedRuleDataTarget(name, assignment))) return failure();
    if (name.empty() || reserved || names.contains(name) ||
        structs.contains(name) ||
        (behavioralRules.contains(name) && !nestedRuleBinding(name)))
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
  auto helper = bindValueHelperResults(assignment, at, symbol, values, instances,
                                        boundaries, forbidden, immutable);
  if (failed(helper) || *helper) return helper;
  auto targets = fixedResultTargets(assignment);
  if (failed(targets))
    return failure();
  if (!*targets)
    return false;
  for (StringRef name : **targets)
    if ((forbidden.contains(name) && !nestedRuleBinding(name)) || instances.contains(name) ||
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
  SmallVector<PreparedLocalBinding, 2> destinations;
  for (StringRef name : **targets) {
    std::optional<BindingBoundary> binding;
    if (boundaries && boundaries->contains(name) && boundaries->lookup(name).declared)
      binding = boundaries->lookup(name);
    destinations.push_back({name, std::move(binding)});
  }
  if (failed(bindLocalValues(destinations, *results, call, at, symbol, values,
                             boundaries)))
    return failure();
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
bool Importer::plainAccessPath(const AstNode &node) const {
  // This syntax predicate needs no source-location path copies.
  AstNode current{node.value, {}};
  while (current.kind() == "Attribute" || current.kind() == "Subscript") {
    AstNode index{dyn_cast_or_null<DictionaryAttr>(current.get("slice")), {}};
    if (current.kind() == "Subscript" && index.kind() == "Slice")
      return false;
    current.value = dyn_cast_or_null<DictionaryAttr>(current.get("value"));
  }
  return current.kind() == "Name";
}

FailureOr<DictionaryAttr> Importer::accessField(Type type, StringRef name) {
  auto record = dyn_cast<ac::StructType>(type);
  auto declaration = record ? ac::HardwareAnalysis(*body).lookupStruct(record)
                            : ac::StructOp();
  if (!declaration)
    return error() << "field projection requires a struct";
  for (Attribute raw : declaration.getFields()) {
    auto field = cast<DictionaryAttr>(raw);
    if (field.getAs<StringAttr>("name").getValue() == name)
      return field;
  }
  return error() << "unknown struct field";
}

FailureOr<const AstNode *> Importer::accessSite(TypedAccessPath &path,
                                                size_t depth) {
  assert(depth <= path.steps.size());
  size_t size = path.baseDepth + depth;
  size_t previous = path.cursor.path.size();
  TableQueryCharge charge;
  std::string reason;
  bool fits = TableQueryBudget::add(
                  charge, TableQueryResource::Work,
                  (size > previous ? size - previous : previous - size) + 1) &&
              TableQueryBudget::addProduct(
                  charge, TableQueryResource::ConstantBytes,
                  size > previous ? size - previous : 0, StringRef("value").size());
  if (!fits || !queryBudget.reserve(charge, reason))
    return error() << "typed access cursor: "
                   << (fits ? reason : "cursor counter exhausted");
  // resolveAccessPath reserves the deepest path once. Seeking never allocates
  // another path buffer and preserves the exact source-occurrence suffix.
  assert(size <= path.cursor.path.capacity());
  if (size < previous) path.cursor.path.resize(size);
  while (path.cursor.path.size() < size)
    path.cursor.path.push_back({"value", std::nullopt});
  path.cursor.value = depth == path.steps.size()
      ? path.root : path.steps[path.steps.size() - depth - 1].node;
  return &path.cursor;
}

FailureOr<TypedAccessPath> Importer::resolveAccessPath(
    const AstNode &target, Type rootType, FlatSymbolRefAttr symbol) {
  TypedAccessPath path;
  path.rootType = rootType;
  path.baseDepth = target.path.size();
  AstNode current{target.value, {}};
  size_t depth = 0;
  while (current.kind() == "Attribute" || current.kind() == "Subscript") {
    if (failed(chargeLoopDescriptorWork(target, "typed access syntax", 1)))
      return failure();
    AstNode index{dyn_cast_or_null<DictionaryAttr>(current.get("slice")), {}};
    if (current.kind() == "Subscript" && index.kind() == "Slice")
      return error() << "assignment path does not admit slices";
    ++depth;
    current.value = dyn_cast_or_null<DictionaryAttr>(current.get("value"));
  }
  if (current.kind() != "Name" || !rootType)
    return error() << "access path requires a bound named payload root";
  path.root = current.value;
  // Pay the single provenance buffer and descriptor capacity before either
  // reserve. SmallVector grows to max(requested, 2 * old capacity + 1).
  size_t sourceSize = target.path.size() + depth;
  size_t sourceCapacity = path.cursor.path.capacity();
  if (sourceSize > sourceCapacity)
    sourceCapacity = std::max(sourceSize, 2 * sourceCapacity + 1);
  size_t stepCapacity = path.steps.capacity();
  if (depth > stepCapacity)
    stepCapacity = std::max(depth, 2 * stepCapacity + 1);
  TableQueryCharge storage;
  std::string reason;
  bool fits = TableQueryBudget::add(storage, TableQueryResource::Work,
                                     target.path.size() + depth + 1) &&
              TableQueryBudget::addProduct(storage, TableQueryResource::Slots,
                  sourceCapacity, (sizeof(AstStep) + 7) / 8) &&
              TableQueryBudget::addProduct(storage, TableQueryResource::Slots,
                  stepCapacity, (sizeof(TypedAccessStep) + 7) / 8);
  for (const AstStep &step : target.path)
    fits &= TableQueryBudget::add(storage, TableQueryResource::ConstantBytes,
                                   step.field.size());
  if (!fits || !queryBudget.reserve(storage, reason))
    return error() << "typed access path: "
                   << (fits ? reason : "path storage counter exhausted");
  path.cursor.path.reserve(sourceCapacity);
  path.cursor.path.append(target.path.begin(), target.path.end());
  path.cursor.value = target.value;
  path.steps.reserve(stepCapacity);
  current.value = target.value;
  for (size_t i = 0; i < depth; ++i) {
    path.steps.push_back({current.value, i, {}, {}, {}, {}, {}});
    current.value = dyn_cast_or_null<DictionaryAttr>(current.get("value"));
  }
  std::reverse(path.steps.begin(), path.steps.end());
  Type selected = rootType;
  for (auto &step : path.steps) {
    auto site = accessSite(path, step.depth);
    if (failed(site)) return failure();
    step.parentType = selected;
    if ((*site)->kind() == "Attribute") {
      auto record = dyn_cast<ac::StructType>(selected);
      auto declaration = record ? ac::HardwareAnalysis(*body).lookupStruct(record)
                                : ac::StructOp();
      if (declaration && failed(chargeLoopDescriptorWork(
              **site, "typed access fields",
              declaration.getFields().size() + (*site)->string("attr").size())))
        return failure();
      auto field = accessField(selected, (*site)->string("attr"));
      if (failed(field)) return failure();
      step.field = *field;
      selected = step.field.getAs<TypeAttr>("type").getValue();
    } else if (auto table = dyn_cast<ac::TableType>(selected)) {
      selected = table.getElementType();
    } else if (isa<ac::BitsType>(selected)) {
      selected = internalBits(1, **site, symbol);
    } else {
      return error() << "subscript requires fixed bits or a table";
    }
    step.type = selected;
  }
  return path;
}

FailureOr<Value> Importer::readAccessField(Value base, DictionaryAttr field,
                                           const AstNode &site, OpBuilder &at) {
  auto result = createOp(at, site.location(b.getContext(), source.path),
                         ac::StructGetOp::getOperationName(), {base},
                         TypeRange{field.getAs<TypeAttr>("type").getValue()},
                         {at.getNamedAttr("field", field.getAs<StringAttr>("name"))})
                    ->getResult(0);
  rememberStructField(result, field, site);
  return result;
}

FailureOr<Value> Importer::readAccessIndex(Value base, Value index,
                                            const AstNode &node, OpBuilder &at,
                                            FlatSymbolRefAttr ownerSymbol) {
  if (isa<ac::BitsType>(base.getType())) {
    if (!fixedValues.contains(base))
      return error() << "bit index requires authoritative fixed bits, not a "
                        "Boolean or Integer carrier";
    ac::HardwareAnalysis analysis(*body);
    auto width = analysis.getPackedWidth(base.getType(), {},
                                         at.getInsertionBlock()->getParentOp());
    if (failed(width) || failed(proveIndexExtent(index, *width, node, "bit", at)))
      return failure();
    // Keep the complete index carrier, including high X/Z bits. A logical
    // Boolean/Integer index crosses an explicit fresh fixed-Bits boundary;
    // aliases of the original source value retain their source-kind facts.
    auto count = unsignedBoundary(index, index.getType(), node.child("slice"),
                                  at, ownerSymbol);
    if (failed(count))
      return failure();
    auto shifted =
        fixedUnsignedShift(base, *count, "lshr", node, node.child("value"),
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
  auto type = dyn_cast<ac::TableType>(base.getType());
  if (!type)
    return error() << "subscript requires fixed bits or a table";
  auto admission = proveIndex(base, index, node, at);
  if (failed(admission)) return failure();
  SmallVector<Type> results{type.getElementType()};
  bool deferred = *admission == SourceIndexAdmission::Deferred;
  if (!deferred) results.push_back(internalBits(1, node, ownerSymbol));
  auto result =
      createOp(at, node.location(b.getContext(), source.path),
               deferred ? ac::TableReadOp::getOperationName() : ac::TableGetOp::getOperationName(),
               {base, index}, results, {})
          ->getResult(0);
  if (isa<ac::BitsType>(result.getType()))
    fixedValues.insert(result);
  return result;
}

LogicalResult Importer::bindAccessIndex(
    TypedAccessPath &path, TypedAccessStep &step, OpBuilder &at, FlatSymbolRefAttr symbol,
    llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances,
    llvm::StringMap<SmallVector<Value>> *captures) {
  if (step.field || step.index) return success();
  auto site = accessSite(path, step.depth);
  if (failed(site)) return failure();
  auto index = expression((*site)->child("slice"), at, symbol, values,
                          instances, {}, captures);
  if (failed(index)) return failure();
  step.index = *index;
  return success();
}

FailureOr<Value> Importer::readAccessPath(
    TypedAccessPath &path, OpBuilder &at, FlatSymbolRefAttr symbol,
    llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances,
    llvm::StringMap<SmallVector<Value>> *captures, size_t length) {
  auto root = accessSite(path, path.steps.size());
  if (failed(root)) return failure();
  auto value = expression(**root, at, symbol, values, instances, {}, captures);
  if (failed(value)) return failure();
  length = std::min(length, path.steps.size());
  for (size_t i = 0; i < length; ++i) {
    auto &step = path.steps[i];
    step.parent = *value;
    if (failed(bindAccessIndex(path, step, at, symbol, values, instances, captures)))
      return failure();
    auto site = accessSite(path, step.depth);
    if (failed(site)) return failure();
    if (step.field)
      value = readAccessField(*value, step.field, **site, at);
    else
      value = readAccessIndex(*value, step.index, **site, at, symbol);
    if (failed(value)) return failure();
  }
  return *value;
}

FailureOr<ArrayAttr> Importer::accessSelectors(const TypedAccessPath &path,
                                                OpBuilder &at) {
  SmallVector<Attribute, 2> selectors;
  bool indexed = false;
  for (const auto &step : path.steps) {
    TableQueryCharge charge;
    std::string reason;
    if (!TableQueryBudget::add(charge, TableQueryResource::Work, 1) ||
        !TableQueryBudget::add(charge, TableQueryResource::Slots, 4) ||
        !queryBudget.reserve(charge, reason))
      return error() << "selected write footprint: " << reason;
    if (step.field)
      selectors.push_back(at.getDictionaryAttr({at.getNamedAttr(
          "field", step.field.getAs<StringAttr>("name"))}));
    else {
      if (indexed)
        return error() << "selected writes support one Table index per path";
      indexed = true;
      selectors.push_back(at.getDictionaryAttr(
          {at.getNamedAttr("index", at.getI64IntegerAttr(0))}));
    }
  }
  return at.getArrayAttr(selectors);
}

// A source Struct remains immutable: only exact operands of its complete,
// nominally matching constructor can replace otherwise-required projections.
static Value reusableStructFieldOperand(Value value, ac::StructOp declaration,
                                        size_t index, Type fieldType) {
  auto constructor = value.getDefiningOp<ac::StructCreateOp>();
  auto type = dyn_cast<ac::StructType>(value.getType());
  auto name = declaration->getAttrOfType<StringAttr>("sym_name");
  if (!constructor || !type || !name || type.getName() != name ||
      constructor->getNumOperands() != declaration.getFields().size() ||
      index >= constructor->getNumOperands())
    return {};
  Value operand = constructor->getOperand(index);
  return operand.getType() == fieldType ? operand : Value();
}
FailureOr<Value> Importer::replaceField(Value value, StringRef name,
                                        Value replacement, const AstNode &node,
                                        OpBuilder &at,
                                        FlatSymbolRefAttr symbol,
                                        bool preserveProjectionWitnesses) {
  auto type = dyn_cast<ac::StructType>(value.getType());
  ac::HardwareAnalysis analysis(*body);
  auto declaration = type ? analysis.lookupStruct(type) : ac::StructOp();
  if (!declaration)
    return error() << "field assignment requires a nominal struct";
  // Pay the actual reconstruction before allocating it. The field vector and
  // OperationState each own Value handles; the final constructor owns its
  // OpOperands. Formal proposals retain explicit projections so their proof
  // authenticates the exact current snapshot; ordinary locals can reuse exact
  // unchanged constructor operands.
  uint64_t projections = 0, comparisons = 0;
  for (auto [index, raw] : llvm::enumerate(declaration.getFields())) {
    auto field = cast<DictionaryAttr>(raw);
    auto key = field.getAs<StringAttr>("name").getValue();
    Type fieldType = field.getAs<TypeAttr>("type").getValue();
    comparisons += 1 + (key.size() == name.size() ? name.size() : 0);
    if (key != name &&
        (preserveProjectionWitnesses ||
         !reusableStructFieldOperand(value, declaration, index, fieldType)))
      ++projections;
  }
  TableQueryCharge charge;
  std::string reason;
  uint64_t count = declaration.getFields().size();
  bool fits = TableQueryBudget::addProduct(charge, TableQueryResource::Work,
                                           comparisons, 2) &&
              TableQueryBudget::addProduct(charge, TableQueryResource::Work,
                                           count, 4) &&
              TableQueryBudget::addProduct(charge, TableQueryResource::Slots,
                                           count, 4) &&
              TableQueryBudget::addProduct(charge, TableQueryResource::Work,
                                           projections, 16) &&
              TableQueryBudget::addProduct(charge, TableQueryResource::Slots,
                                           projections, 32) &&
              TableQueryBudget::add(charge, TableQueryResource::Work, 32) &&
              TableQueryBudget::add(charge, TableQueryResource::Slots, 32) &&
              TableQueryBudget::add(charge, TableQueryResource::Operations,
                                     projections + 2);
  if (!fits || !queryBudget.reserve(charge, reason))
    return error() << "local Struct reconstruction: "
                   << (fits ? reason : "cost counter exhausted");
  bool found = false;
  SmallVector<Value> fields;
  fields.reserve(count);
  for (auto [index, raw] : llvm::enumerate(declaration.getFields())) {
    auto field = cast<DictionaryAttr>(raw);
    Type fieldType = field.getAs<TypeAttr>("type").getValue();
    auto key = field.getAs<StringAttr>("name");
    if (key.getValue() == name) {
      auto converted = applyBindingBoundary(
          replacement, BindingBoundary(structFieldPort(field, node), true),
          node, at, symbol);
      if (failed(converted))
        return failure();
      fields.push_back(*converted);
      found = true;
    } else if (Value operand = preserveProjectionWitnesses
                                   ? Value()
                                   : reusableStructFieldOperand(
                                         value, declaration, index, fieldType)) {
      fields.push_back(operand);
    } else {
      auto projected = readAccessField(value, field, node, at);
      if (failed(projected)) return failure();
      fields.push_back(*projected);
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

LogicalResult Importer::validateAssignmentRoot(const AstNode &target,
                                                const AstNode &node,
                                                OpBuilder &at) {
  StringRef root = assignmentRoot(target);
  if (failed(requireNestedRuleDataTarget(root, node))) return failure();
  if (isa<ac::ModuleOp>(at.getInsertionBlock()->getParentOp()) &&
      (queueResultNames.contains(root) || memoryResultNames.contains(root)))
    return error() << "queue result binding is immutable: '" << root << "'";
  if (root == "pyc_clk" || root == "pyc_rst")
    return error() << "reserved physical domain pin '" << root
                   << "' cannot be assigned in source";
  if (target.kind() == "Name") {
    StringRef name = target.string("id");
    bool helperLocal = activeValueHelper && activeHelperFrame &&
                       activeHelperFrame->localNames.contains(name);
    if (!helperLocal && (names.contains(name) || structs.contains(name) ||
                         (behavioralRules.contains(name) && !nestedRuleBinding(name))))
      return error() << "assignment shadows a source declaration";
  }
  return success();
}

// Resolve the leaf's declaration before lowering the RHS. Index evaluation and
// publication remain with the owning assignment dispatcher.
FailureOr<Value> Importer::prepareAssignmentLeaf(
    TypedAccessPath &path, const AstNode &rhs, OpBuilder &at,
    FlatSymbolRefAttr symbol, llvm::StringMap<Value> &values,
    llvm::StringMap<Instance *> &instances, AssignmentIndexPolicy indexPolicy) {
  assert(!path.steps.empty());
  auto target = accessSite(path, 0);
  if (failed(target)) return failure();
  const auto &leaf = path.steps.back();
  auto replacement = expression(rhs, at, symbol, values, instances, leaf.type);
  if (failed(replacement)) return failure();
  if (!leaf.field && indexPolicy == AssignmentIndexPolicy::UnsignedBoundary)
    return unsignedBoundary(*replacement, leaf.type, rhs, at, symbol);
  BindingBoundary boundary = leaf.field
      ? BindingBoundary{structFieldPort(leaf.field, **target), true}
      : BindingBoundary{{path.name().str(), leaf.type, **target, {},
                          isa<ac::BitsType>(leaf.type)}, true};
  return applyBindingBoundary(*replacement, boundary, rhs, at, symbol);
}

LogicalResult Importer::updateAssignment(
    TypedAccessPath &path, Value replacement, const AstNode &node, OpBuilder &at,
    FlatSymbolRefAttr symbol, llvm::StringMap<Value> &values,
    SmallVectorImpl<Value> *boundIndices, bool preserveProjectionWitnesses) {
  auto target = accessSite(path, 0);
  if (failed(target) || failed(validateAssignmentRoot(**target, node, at)))
    return failure();
  llvm::StringMap<Instance *> noInstances;
  bool selectedField = path.steps.size() > 1 && !path.steps.front().field &&
      llvm::all_of(ArrayRef<TypedAccessStep>(path.steps).drop_front(),
                   [](const TypedAccessStep &step) { return bool(step.field); });
  if (selectedField) {
    // Preserve the existing masked Table field update. Reading and rebuilding a
    // whole selected row would change unknown-index behavior and write proofs.
    auto root = accessSite(path, path.steps.size());
    if (failed(root)) return failure();
    auto container = expression(**root, at, symbol, values, noInstances);
    if (failed(container) || !isa<ac::TableType>(container->getType()))
      return error() << "indexed assignment requires a Table payload";
    auto &index = path.steps.front();
    index.parent = *container;
    target = accessSite(path, 0);
    if (failed(target)) return failure();
    auto converted = applyBindingBoundary(
        replacement, BindingBoundary(structFieldPort(path.steps.back().field,
                                                      **target), true),
        node, at, symbol);
    if (failed(converted) ||
        failed(bindAccessIndex(path, index, at, symbol, values, noInstances)))
      return failure();
    SmallVector<Attribute, 2> fields;
    for (const auto &step : ArrayRef<TypedAccessStep>(path.steps).drop_front())
      fields.push_back(step.field.getAs<StringAttr>("name"));
    auto site = accessSite(path, index.depth);
    if (failed(site)) return failure();
    auto slot = addressDomainSlot(path.name(), node, (*site)->child("slice"));
    if (failed(slot)) return failure();
    auto updated = tableUpdate(*container, index.index, *converted, fields,
                               node, at, symbol, *slot);
    if (failed(updated)) return failure();
    replacement = *updated;
  } else if (!path.steps.empty()) {
    // The old recursive updater prepaid its ancestor projections separately
    // from replaceField. The typed path emits each prefix Field only once;
    // reserve those actual reads before materializing any parent snapshot.
    uint64_t projections = llvm::count_if(
        ArrayRef<TypedAccessStep>(path.steps).drop_back(),
        [](const TypedAccessStep &step) { return bool(step.field); });
    TableQueryCharge reads;
    std::string reason;
    bool fits = TableQueryBudget::addProduct(
                    reads, TableQueryResource::Work, projections, 16) &&
                TableQueryBudget::addProduct(
                    reads, TableQueryResource::Slots, projections, 32) &&
                TableQueryBudget::add(
                    reads, TableQueryResource::Operations, projections);
    if (!fits || !queryBudget.reserve(reads, reason))
      return error() << "assignment parent projections: "
                     << (fits ? reason : "projection counter exhausted");
    auto container = readAccessPath(path, at, symbol, values, noInstances,
                                    nullptr, path.steps.size() - 1);
    if (failed(container)) return failure();
    auto &last = path.steps.back();
    last.parent = *container;
    if (failed(bindAccessIndex(path, last, at, symbol, values, noInstances)))
      return failure();
    for (auto &step : llvm::reverse(path.steps)) {
      FailureOr<Value> updated = failure();
      if (step.field) {
        updated = replaceField(step.parent,
                                step.field.getAs<StringAttr>("name").getValue(),
                                replacement, node, at, symbol,
                                preserveProjectionWitnesses);
      } else {
        if (!isa<ac::TableType>(step.parentType))
          return error() << "indexed assignment requires a Table payload";
        auto site = accessSite(path, step.depth);
        if (failed(site)) return failure();
        auto slot = addressDomainSlot(path.name(), node, (*site)->child("slice"));
        if (failed(slot)) return failure();
        updated = tableUpdate(step.parent, step.index, replacement, {}, node,
                               at, symbol, *slot);
      }
      if (failed(updated)) return failure();
      replacement = *updated;
    }
  }
  auto root = accessSite(path, path.steps.size());
  if (failed(root) || failed(validateAssignmentRoot(**root, node, at)))
    return failure();
  if (boundIndices)
    for (const auto &step : path.steps)
      if (!step.field) boundIndices->push_back(step.index);
  return bindLocalValues({{path.name(), std::nullopt}}, {replacement}, node,
                         at, symbol, values);
}
