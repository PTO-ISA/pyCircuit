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
  bool callable = !decl.system && decl.returnShape == ReturnShape::Struct &&
                  decl.parameters.empty();
  SmallVector<NamedAttribute> domain;
  if (decl.needsDomain) {
    domain.push_back(
        b.getNamedAttr("clock", b.getI64IntegerAttr(decl.sourceInputCount)));
    domain.push_back(b.getNamedAttr(
        "reset", b.getI64IntegerAttr(decl.sourceInputCount + 1)));
  }
  if (!callable && !decl.needsDomain)
    return success();
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
  DictionaryAttr result;
  if (callable) {
    auto encoded = sourceConstraint(decl.outputs.front());
    if (failed(encoded))
      return failure();
    result = *encoded;
  }
  decl.sourceCallAttrs = {
      b.getNamedAttr("ac.domain_inputs", b.getDictionaryAttr(domain))};
  if (callable) {
    decl.sourceCallAttrs.insert(
        decl.sourceCallAttrs.begin(),
        {b.getNamedAttr("ac.return_form", b.getStringAttr("single")),
         b.getNamedAttr("ac.parameters", b.getArrayAttr(parameters)),
         b.getNamedAttr("ac.result_constraints", b.getArrayAttr({result}))});
  }
  // Reuse the common declaration validator before any call is predeclared.
  // The transient import uses the finalized physical signature and disappears
  // before source body emission; it carries no alternate semantics or owner.
  OpBuilder::InsertionGuard insertion(b);
  b.setInsertionPointToEnd(body->getBody());
  SmallVector<NamedAttribute> attrs{
      b.getNamedAttr("sym_name", b.getStringAttr(decl.symbol.getValue())),
      b.getNamedAttr("source_owner", owner),
      b.getNamedAttr("parameters", b.getArrayAttr({})),
      b.getNamedAttr("type_parameters", b.getArrayAttr({})),
      b.getNamedAttr("function_type", TypeAttr::get(decl.type)),
      b.getNamedAttr("input_names", portNames(b, decl.inputs)),
      b.getNamedAttr("output_names", portNames(b, decl.outputs))};
  if (decl.system)
    attrs.push_back(b.getNamedAttr("ac.root_kind", b.getStringAttr("system")));
  llvm::append_range(attrs, decl.sourceCallAttrs);
  auto signature =
      createOp(b, decl.node.location(b.getContext(), source.path),
               ac::ModuleImportOp::getOperationName(), {}, {}, attrs);
  auto valid = ac::detail::verifyModuleSourceCallContract(signature);
  signature->erase();
  return valid;
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
    if (decl.sourceCallAttrs.size() != 4)
      return error() << "direct module value calls require a typed struct "
                        "return and no static parameters";
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
    parameters = view.sourceParameters;
    constraints = view.sourceResultConstraints;
    domain = view.domainInputs;
  }
  auto restore = [&](DictionaryAttr constraint, StringRef name) -> Port {
    auto kind = constraint.getAs<StringAttr>("source_kind").getValue();
    std::optional<ac::detail::ValueKind> sourceKind;
    if (kind == "boolean")
      sourceKind = ac::detail::ValueKind::Boolean;
    if (kind == "integer")
      sourceKind = ac::detail::ValueKind::Integer;
    return {name.str(), constraint.getAs<TypeAttr>("type").getValue(), function,
            sourceKind, kind == "fixed_bits"};
  };
  for (Attribute raw : parameters) {
    auto parameter = cast<DictionaryAttr>(raw);
    result.inputs.push_back(
        restore(parameter.getAs<DictionaryAttr>("constraint"),
                parameter.getAs<StringAttr>("name").getValue()));
  }
  result.result = restore(cast<DictionaryAttr>(constraints[0]), "");
  result.needsDomain = !domain.empty();
  return result;
}
