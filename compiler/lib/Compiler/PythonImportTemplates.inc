// Module templates retain the same immutable capture and reuse this importer
// after the ordinary integer binding and callee interfaces are available.
bool anySourceTemplateChild(
    const AstNode &node, llvm::function_ref<bool(const AstNode &)> visit,
    StringRef ignoredField = {}) {
  for (NamedAttribute field : node.fields()) {
    StringRef name = field.getName().getValue();
    if (name == ignoredField)
      continue;
    if (auto child = dyn_cast<DictionaryAttr>(field.getValue())) {
      if (child.getAs<StringAttr>("kind") && visit(node.child(name)))
        return true;
    } else if (auto children = dyn_cast<ArrayAttr>(field.getValue()))
      for (size_t k = 0; k < children.size(); ++k)
        if (auto child = dyn_cast<DictionaryAttr>(children[k]);
            child && child.getAs<StringAttr>("kind") && visit(node.item(name, k)))
          return true;
  }
  return false;
}

bool Importer::needsSourceTemplates() {
  // A re-export-only unit has no body to defer and owns no capture payload.
  if (modules.empty())
    return false;
  for (const auto &entry : names)
    if (entry.second.category == BindingCategory::Module) {
      auto view = lookupModule(entry.second.symbol);
      if (view && view.semanticKind &&
          view.semanticKind.getValue() == "source_template")
        return true;
    }
  auto templateDependency = [&](auto &&self, const AstNode &node) -> bool {
    if (node.kind() == "Call") {
      auto binding = lookupBinding(node.child("func"));
      if (binding && binding->category == BindingCategory::Module) {
        auto view = lookupModule(binding->symbol);
        if (view && view.semanticKind &&
            view.semanticKind.getValue() == "source_template")
          return true;
      }
    }
    return anySourceTemplateChild(node, [&](const AstNode &child) {
      return self(self, child);
    });
  };
  if (templateDependency(templateDependency, source.module))
    return true;
  for (const ModuleDecl &decl : modules) {
    if (decl.parameters.empty())
      continue;
    auto visit = [&](auto &&self, const AstNode &node) -> bool {
      if (node.kind() == "For") {
        auto range = node.child("iter");
        if (range.kind() == "Call" && range.array("args").size() == 1 &&
            range.item("args", 0).kind() != "Constant")
          return true;
      }
      return anySourceTemplateChild(node, [&](const AstNode &child) {
        return self(self, child);
      });
    };
    for (size_t k = 0; k < decl.node.array("body").size(); ++k) {
      auto callable = decl.node.item("body", k);
      if (callable.kind() == "FunctionDef" &&
          (hasDecorator(callable, "rule") ||
           hasDecorator(callable, "method")) &&
          visit(visit, callable))
        return true;
    }
  }
  return false;
}

FailureOr<DictionaryAttr>
Importer::templateDescription(FlatSymbolRefAttr target, ArrayAttr capture) {
  auto found = llvm::find_if(
      modules, [&](const ModuleDecl &decl) { return decl.symbol == target; });
  if (found == modules.end())
    return error() << "source template target is absent";
  const auto &decl = *found;
  auto signature = moduleSignatureAttrs(decl);
  if (failed(signature))
    return failure();
  SmallVector<Attribute> methods;
  // Preserve source declaration order rather than StringMap iteration order.
  auto &inventory = sourceMethods[decl.node.string("name")];
  for (size_t k = 0; k < decl.node.array("body").size(); ++k) {
    auto node = decl.node.item("body", k);
    if (node.kind() != "FunctionDef" || !hasDecorator(node, "method"))
      continue;
    auto method = inventory.find(node.string("name"));
    if (method == inventory.end() || !method->second.type)
      return error()
             << "source template Method requires its declared signature";
    const auto &callable = method->second;
    SmallVector<Attribute> parameters, results;
    for (const Port &port : callable.inputs) {
      auto constraint = sourceConstraint(port);
      if (failed(constraint))
        return failure();
      parameters.push_back(b.getDictionaryAttr(
          {b.getNamedAttr("name", b.getStringAttr(port.name)),
           b.getNamedAttr("binding", b.getStringAttr("positional_or_keyword")),
           b.getNamedAttr("constraint", *constraint),
           b.getNamedAttr("default", absentDefault(b)),
           b.getNamedAttr("origin", sourceOccurrence(b, target, port.node)),
           b.getNamedAttr("location", sourceSpan(b, source.path, port.node))}));
    }
    for (const Port &port : callable.resultContracts) {
      auto constraint = sourceConstraint(port);
      if (failed(constraint))
        return failure();
      results.push_back(*constraint);
    }
    Attribute guard = callable.syntax.guard
                          ? Attribute(namespaceSite(callable.syntax.guard)
                                          .getAs<ArrayAttr>("ast_path"))
                          : Attribute(b.getUnitAttr());
    methods.push_back(b.getDictionaryAttr(
        {b.getNamedAttr("name", b.getStringAttr(node.string("name"))),
         b.getNamedAttr("function_type", TypeAttr::get(callable.type)),
         b.getNamedAttr("ac.parameters", b.getArrayAttr(parameters)),
         b.getNamedAttr(
             "ac.return_form",
             b.getStringAttr(callable.results.empty() ? "none"
                             : callable.returnShape == ReturnShape::Tuple
                                 ? "tuple"
                                 : "single")),
         b.getNamedAttr("ac.result_constraints", b.getArrayAttr(results)),
         b.getNamedAttr("declaration", sourceOccurrence(b, target, node)),
         b.getNamedAttr("guard", guard)}));
  }
  NamedAttrList attributes(*signature);
  attributes.set("source_owner", owner);
  attributes.set("ac.origin", sourceOccurrence(b, target, decl.node));
  attributes.set("ac.declaration_role", b.getStringAttr("definition"));
  attributes.set("capture_owner", owner);
  attributes.set("target", target);
  attributes.set("ast_path",
                 namespaceSite(decl.node).getAs<ArrayAttr>("ast_path"));
  attributes.set(
      "template_contract",
      b.getDictionaryAttr(
          {b.getNamedAttr("version", b.getI64IntegerAttr(1)),
           b.getNamedAttr("target", target),
           b.getNamedAttr("capture_digest",
                          sourceTemplateCaptureDigest(owner, capture)),
           b.getNamedAttr("methods", b.getArrayAttr(methods))}));
  return attributes.getDictionary(b.getContext());
}

FailureOr<SmallVector<std::pair<AstNode, SourceTemplateRequest>>>
Importer::templateCallees(bool preserveParameters) {
  auto selected = llvm::find_if(modules, [&](const ModuleDecl &decl) {
    return decl.symbol == boundTarget;
  });
  if (selected == modules.end())
    return error() << "source template target is absent";
  SmallVector<std::pair<AstNode, SourceTemplateRequest>> requests;
  // Reference formals need the provider's concrete interface without creating
  // an allocation or changing its nominal Module identity.
  for (Attribute raw : selected->references) {
    if (failed(chargeLoopDescriptorWork(
            selected->node, "source template reference declaration")))
      return failure();
    auto callee = cast<DictionaryAttr>(raw).getAs<FlatSymbolRefAttr>("callee");
    requests.push_back({AstNode{}, {callee, {}, true}});
  }
  auto visit = [&](auto &&self, const AstNode &node) -> LogicalResult {
    if (node.kind() == "FunctionDef" || node.kind() == "Lambda")
      return success();
    if (failed(chargeLoopDescriptorWork(node,
                                        "source template child declaration")))
      return failure();
    if (moduleCall(node)) {
      auto function = node.child("func");
      auto binding = lookupBinding(function);
      if (!binding)
        return error() << "source template child lacks a declaration";
      DictionaryAttr signature;
      auto localChild = llvm::find_if(modules, [&](const ModuleDecl &decl) {
        return decl.symbol == binding->symbol;
      });
      if (localChild != modules.end()) {
        auto attrs = moduleSignatureAttrs(*localChild);
        if (failed(attrs))
          return failure();
        signature = *attrs;
      } else {
        auto view = lookupModule(binding->symbol);
        if (!view)
          return error() << "source template child lacks an admitted interface";
        if (view.isBuiltin())
          return success();
        signature = view.declaration->getAttrDictionary();
      }
      auto actuals = bindParameters(signature, node, boundTarget, true);
      if (failed(actuals))
        return failure();
      SourceTemplateRequest request{binding->symbol, {}, preserveParameters};
      if (!preserveParameters) {
        ac::HardwareAnalysis analysis(*body);
        for (auto [formal, actual] :
             llvm::zip(signature.getAs<ArrayAttr>("parameters"), *actuals)) {
          auto value = analysis.evaluateStatic(cast<ac::StaticExprAttr>(actual),
                                               {}, body->getOperation());
          auto integer = succeeded(value) ? dyn_cast<ac::MathIntAttr>(*value)
                                          : ac::MathIntAttr();
          if (!integer)
            return error() << "source template child parameter requires a closed "
                              "mathematical Integer";
          request.bindings.push_back(
              {cast<DictionaryAttr>(formal).getAs<StringAttr>("name"), integer});
        }
      }
      requests.push_back({function, std::move(request)});
    }
    bool failedChild = anySourceTemplateChild(
        node, [&](const AstNode &child) { return failed(self(self, child)); },
        "annotation");
    return failure(failedChild);
  };
  for (size_t k = 0; k < selected->node.array("body").size(); ++k)
    if (failed(visit(visit, selected->node.item("body", k))))
      return failure();
  return requests;
}
