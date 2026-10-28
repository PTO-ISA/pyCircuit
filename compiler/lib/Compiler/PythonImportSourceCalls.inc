// Source authority is serialized once, after local domain inference. Imported
// call sites consume the registry's validated contract without provider source.
bool Importer::moduleCall(const AstNode &node) const {
  if (node.kind() != "Call")
    return false;
  auto binding = lookupBinding(node.child("func"));
  return binding && binding->category == BindingCategory::Module;
}
FailureOr<DictionaryAttr> Importer::sourceConstraint(const Port &port) {
  using ac::detail::ValueKind;
  StringRef kind;
  ac::SourceDomainAttr domain;
  if (port.sourceKind == ValueKind::Boolean) {
    kind = "boolean";
    domain = ac::SourceDomainAttr::get(
        b.getContext(),
        b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("bool"))}));
  } else if (port.sourceKind == ValueKind::Integer) {
    kind = "integer";
    ac::HardwareAnalysis analysis(*body);
    auto width = analysis.getPackedWidth(port.type, {}, body->getOperation());
    if (failed(width))
      return failure();
    if (*width >= std::numeric_limits<unsigned>::max())
      return error() << "source Integer domain exceeds compiler representation "
                        "capacity";
    llvm::APInt upper(static_cast<unsigned>(*width) + 1, 1);
    upper <<= static_cast<unsigned>(*width);
    domain = ac::SourceDomainAttr::get(
        b.getContext(),
        b.getDictionaryAttr(
            {b.getNamedAttr("kind", b.getStringAttr("integer")),
             b.getNamedAttr("lower",
                            ac::MathIntAttr::get(b.getContext(),
                                                 llvm::APSInt::getUnsigned(0))),
             b.getNamedAttr("upper",
                            ac::MathIntAttr::get(b.getContext(),
                                                 llvm::APSInt(upper, true)))}));
  } else if (port.fixedBits && isa<ac::BitsType>(port.type))
    kind = "fixed_bits";
  else if (isa<ac::StructType, ac::EnumType>(port.type))
    kind = "nominal";
  else if (isa<ac::TableType>(port.type))
    kind = "table";
  else
    return error() << "source call input lacks authoritative source kind";
  SmallVector<NamedAttribute> fields{
      b.getNamedAttr("kind", b.getStringAttr("hardware")),
      b.getNamedAttr("type", TypeAttr::get(port.type)),
      b.getNamedAttr("source_kind", b.getStringAttr(kind))};
  if (domain)
    fields.push_back(b.getNamedAttr("domain", domain));
  return b.getDictionaryAttr(fields);
}
LogicalResult Importer::serializeSourceCall(ModuleDecl &decl) {
  bool callable = !decl.system &&
      (decl.returnShape != ReturnShape::Mapping ||
       (decl.transactionEffects && decl.outputs.empty()));
  SmallVector<NamedAttribute> domain;
  if (decl.needsDomain) {
    domain.push_back(
        b.getNamedAttr("clock", b.getI64IntegerAttr(decl.sourceInputCount)));
    domain.push_back(b.getNamedAttr(
        "reset", b.getI64IntegerAttr(decl.sourceInputCount + 1)));
  }
  SmallVector<Attribute> availability(decl.outputs.size(), b.getStringAttr("compiler"));
  auto policy = b.getNamedAttr("ac.result_availability", b.getArrayAttr(availability));
  if (!callable && !decl.needsDomain) {
    decl.sourceCallAttrs = {policy};
    return success();
  }
  SmallVector<Attribute> parameters;
  for (size_t i = 0; callable && i < decl.sourceInputCount; ++i) {
    const auto &port = decl.inputs[i];
    auto constraint = sourceConstraint(port);
    if (failed(constraint))
      return failure();
    parameters.push_back(b.getDictionaryAttr(
        {b.getNamedAttr("name", b.getStringAttr(port.name)),
         b.getNamedAttr("binding", b.getStringAttr("positional_or_keyword")),
         b.getNamedAttr("constraint", *constraint),
         b.getNamedAttr("default", absentDefault(b)),
         b.getNamedAttr("origin", sourceOccurrence(b, decl.symbol, port.node)),
         b.getNamedAttr("location", sourceSpan(b, source.path, port.node))}));
  }
  SmallVector<Attribute> results;
  if (callable)
    for (const Port &output : decl.outputs) {
      auto encoded = sourceConstraint(output);
      if (failed(encoded))
        return failure();
      results.push_back(*encoded);
    }
  decl.sourceCallAttrs = {
      b.getNamedAttr("ac.domain_inputs", b.getDictionaryAttr(domain)), policy};
  if (callable) {
    decl.sourceCallAttrs.insert(
        decl.sourceCallAttrs.begin(),
        {b.getNamedAttr("ac.return_form", b.getStringAttr(
             decl.outputs.empty() ? "none" :
             decl.returnShape == ReturnShape::Tuple ? "tuple" : "single")),
         b.getNamedAttr("ac.parameters", b.getArrayAttr(parameters)),
         b.getNamedAttr("ac.result_constraints", b.getArrayAttr(results))});
  }
  return success();
}
FailureOr<CallableDecl> Importer::callable(const AstNode &function) {
  auto binding = lookupBinding(function);
  if (!binding || binding->category != BindingCategory::Module)
    return error() << "direct call requires an unshadowed module declaration";
  if (auto found = local.find(function.string("id"));
      found != local.end() && modules[found->second].system)
    return error() << "@system is root-only and cannot be called as a child";
  if (binding->declaration &&
      binding->declaration->getAttrOfType<StringAttr>("ac.root_kind"))
    return error() << "@system is root-only and cannot be called as a child";
  CallableDecl result;
  result.symbol = binding->symbol;
  ArrayAttr parameters, constraints;
  DictionaryAttr domain;
  if (auto found = local.find(function.string("id")); found != local.end()) {
    const ModuleDecl &decl = modules[found->second];
    if (!llvm::any_of(decl.sourceCallAttrs, [](NamedAttribute attr) {
          return attr.getName() == "ac.return_form";
        }))
      return error() << "direct module value calls require a typed scalar, "
                        "Struct or tuple return";
    auto signature = moduleSignatureAttrs(decl);
    if (failed(signature))
      return failure();
    result.signature = *signature;
    result.returnShape = decl.returnShape;
    for (auto attr : decl.sourceCallAttrs) {
      if (attr.getName() == "ac.parameters")
        parameters = cast<ArrayAttr>(attr.getValue());
      if (attr.getName() == "ac.result_constraints")
        constraints = cast<ArrayAttr>(attr.getValue());
      if (attr.getName() == "ac.domain_inputs")
        domain = cast<DictionaryAttr>(attr.getValue());
    }
  } else {
    auto view = headers.lookupModule(binding->symbol);
    if (!view || !view.hasSourceCallContract())
      return error() << "direct module call requires complete validated "
                        "source-call metadata";
    result.signature = view.declaration->getAttrDictionary();
    parameters = view.sourceParameters;
    constraints = view.sourceResultConstraints;
    domain = view.domainInputs;
    result.returnShape = view.sourceReturnForm.getValue() == "tuple"
        ? ReturnShape::Tuple : view.sourceReturnForm.getValue() == "none"
        ? ReturnShape::None : ReturnShape::Struct;
  }
  for (Attribute raw : parameters) {
    auto parameter = cast<DictionaryAttr>(raw);
    result.inputs.push_back(
        sourceCallPort(parameter.getAs<DictionaryAttr>("constraint"),
                       parameter.getAs<StringAttr>("name").getValue(),
                       function));
  }
  for (Attribute raw : constraints)
    result.results.push_back(
        sourceCallPort(cast<DictionaryAttr>(raw), "", function));
  result.needsDomain = !domain.empty();
  return result;
}

FailureOr<CallableDecl> Importer::resolveModuleCall(
    const AstNode &call, FlatSymbolRefAttr parent) {
  auto result = callable(call.child("func"));
  if (failed(result))
    return failure();
  auto parameters = bindParameters(result->signature, call, parent,
                                    /*directCall=*/true);
  auto types = bindTypeArguments(result->signature, call, parent);
  if (failed(parameters) || failed(types))
    return failure();
  result->parameters = std::move(*parameters);
  result->typeArguments = std::move(*types);
  for (auto *ports : {&result->inputs, &result->results})
    for (Port &port : *ports) {
      auto resolved = substitute(port.type, result->signature,
                                  result->parameters, result->typeArguments);
      if (failed(resolved))
        return failure();
      port.type = *resolved;
    }
  return result;
}

Port Importer::sourceCallPort(DictionaryAttr constraint, StringRef name,
                             const AstNode &callSite) const {
  StringRef kind = constraint.getAs<StringAttr>("source_kind").getValue();
  std::optional<ac::detail::ValueKind> sourceKind;
  if (kind == "boolean")
    sourceKind = ac::detail::ValueKind::Boolean;
  if (kind == "integer")
    sourceKind = ac::detail::ValueKind::Integer;
  return {name.str(), constraint.getAs<TypeAttr>("type").getValue(), callSite,
          sourceKind, kind == "fixed_bits"};
}

Port Importer::structFieldPort(DictionaryAttr field, const AstNode &site) const {
  StringRef name = field.getAs<StringAttr>("name").getValue();
  if (auto constraint = field.getAs<DictionaryAttr>("source_constraint"))
    return sourceCallPort(constraint, name, site);
  Type type = field.getAs<TypeAttr>("type").getValue();
  return {name.str(), type, site, std::nullopt, isa<ac::BitsType>(type)};
}

void Importer::rememberStructField(Value value, DictionaryAttr field,
                                   const AstNode &site) {
  Port port = structFieldPort(field, site);
  remember(value, port.sourceKind);
  if (port.fixedBits)
    fixedValues.insert(value);
  else
    fixedValues.erase(value);
}

FailureOr<Value> Importer::structFieldZero(DictionaryAttr field,
                                          const AstNode &site, OpBuilder &at,
                                          FlatSymbolRefAttr symbol) {
  auto value = constant(field.getAs<TypeAttr>("type").getValue(), 0, site, at,
                        symbol);
  if (succeeded(value) && field.get("source_constraint")) {
    fixedValues.erase(*value);
    remember(*value, ac::detail::ValueKind::Boolean, {}, b.getBoolAttr(false));
  }
  return value;
}

bool Importer::methodOwner(const AstNode &function) const {
  if (function.kind() == "Name" &&
      methodOwnerNames.contains(function.string("id")) &&
      !lexicallyShadowed(function, function.string("id"))) {
    auto found = local.find(function.string("id"));
    if (found != local.end() && modules[found->second].sourceInputCount == 0 &&
        modules[found->second].outputs.empty()) return true;
  }
  if (function.kind() == "Name" &&
      !lexicallyShadowed(function, function.string("id"))) {
    auto target = local.find(function.string("id"));
    auto rules = callableRuleNames.find(function.string("id"));
    if (target != local.end() && rules != callableRuleNames.end() &&
        !rules->second.empty()) {
      const ModuleDecl &decl = modules[target->second];
      if (decl.transactionEffects && decl.sourceInputCount == 0 &&
          decl.outputs.empty() && decl.parameters.empty() && !decl.system)
        return true;
    }
  }
  auto binding = lookupBinding(function);
  if (!binding || binding->category != BindingCategory::Module)
    return false;
  auto view = headers.lookupModule(binding->symbol);
  auto methods = view && view.transactionSummary
                     ? view.transactionSummary.getAs<ArrayAttr>("methods")
                     : ArrayAttr();
  // A transaction parent with ordinary inputs/results is a value module,
  // even when its transitive closure requires transaction publication.
  if (!methods || methods.empty() || !view.parameters.empty() ||
      !view.typeParameters.empty() || !view.domainInputs ||
      view.functionType.getNumResults() != 0 ||
      view.functionType.getNumInputs() != view.domainInputs.size() ||
      (view.sourceParameters && !view.sourceParameters.empty()) ||
      (view.sourceResultConstraints && !view.sourceResultConstraints.empty()))
    return false;
  return !view.sourceReturnForm || view.sourceReturnForm.getValue() == "none";
}

// Reconstruct authored positional order without materializing reference values
// as hardware SSA operands. Static integer/type arguments remain keyword-only.
FailureOr<SmallVector<AstNode>> Importer::moduleActuals(
    DictionaryAttr signature, const AstNode &call) {
  auto data = signature.getAs<ArrayAttr>("ac.parameters");
  auto refs = signature.getAs<ArrayAttr>("ac.reference_parameters");
  size_t count = (data ? data.size() : 0) + (refs ? refs.size() : 0);
  SmallVector<StringRef> names(count);
  for (Attribute raw : refs ? refs : b.getArrayAttr({})) {
    auto row = cast<DictionaryAttr>(raw);
    unsigned position = row.getAs<IntegerAttr>("position").getInt();
    if (position >= count || !names[position].empty())
      return error() << "invalid module reference formal position";
    names[position] = row.getAs<StringAttr>("name").getValue();
  }
  unsigned position = 0;
  for (Attribute raw : data ? data : b.getArrayAttr({})) {
    while (position < count && !names[position].empty()) ++position;
    names[position++] = cast<DictionaryAttr>(raw).getAs<StringAttr>("name").getValue();
  }
  SmallVector<AstNode> actuals(count);
  if (call.array("args").size() > count)
    return error() << "too many direct module arguments";
  for (size_t i = 0; i < call.array("args").size(); ++i)
    actuals[i] = call.item("args", i);
  for (size_t i = 0; i < call.array("keywords").size(); ++i) {
    auto kw = call.item("keywords", i);
    auto name = dyn_cast_or_null<StringAttr>(kw.get("arg"));
    if (!name) return error() << "module calls do not admit **kwargs";
    auto found = llvm::find(names, name.getValue());
    if (found == names.end()) {
      bool staticFormal = false;
      for (Attribute raw : signature.getAs<ArrayAttr>("parameters"))
        staticFormal |= cast<DictionaryAttr>(raw).getAs<StringAttr>("name") == name;
      for (Attribute raw : signature.getAs<ArrayAttr>("type_parameters"))
        staticFormal |= raw == name;
      if (staticFormal) continue;
      return error() << "unknown direct module argument";
    }
    unsigned index = std::distance(names.begin(), found);
    if (actuals[index]) return error() << "repeated direct module argument";
    actuals[index] = kw.child("value");
  }
  for (auto [index, actual] : llvm::enumerate(actuals))
    if (!actual || actual.kind() == "Starred")
      return error() << "missing or unsupported direct module argument for parameter '"
                     << names[index] << "'";
  return actuals;
}
FailureOr<ArrayAttr> Importer::bindModuleReferences(
    DictionaryAttr signature, const AstNode &call,
    llvm::StringMap<Instance *> &instances) {
  auto actuals = moduleActuals(signature, call);
  if (failed(actuals)) return failure();
  SmallVector<Attribute> bindings;
  auto refs = signature.getAs<ArrayAttr>("ac.reference_parameters");
  for (Attribute raw : refs ? refs : b.getArrayAttr({})) {
    auto row = cast<DictionaryAttr>(raw);
    auto actual = (*actuals)[row.getAs<IntegerAttr>("position").getInt()];
    auto found = actual.kind() == "Name" ? instances.find(actual.string("id")) : instances.end();
    if (found == instances.end())
      return error() << "module reference actual must name an existing static Module instance";
    const Instance &instance = *found->second;
    if (instance.callee != row.getAs<FlatSymbolRefAttr>("callee"))
      return error() << "module reference actual has the wrong Module declaration";
    SmallVector<NamedAttribute> receiver;
    SmallVector<Attribute> path;
    if (!instance.reference.empty())
      receiver.push_back(b.getNamedAttr("reference", b.getStringAttr(instance.reference)));
    else if (instance.op)
      path.push_back(instance.op->getAttr("occurrence"));
    else return error() << "module reference actual lacks allocation identity";
    receiver.push_back(b.getNamedAttr("instance_path", b.getArrayAttr(path)));
    bindings.push_back(b.getDictionaryAttr({
        b.getNamedAttr("name", row.getAs<StringAttr>("name")),
        b.getNamedAttr("receiver", b.getDictionaryAttr(receiver))}));
  }
  return b.getArrayAttr(bindings);
}
