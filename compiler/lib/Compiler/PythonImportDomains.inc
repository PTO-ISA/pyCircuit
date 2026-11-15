// Clock configurations use the existing static-expression grammar and bindings.
FailureOr<ac::StaticExprAttr> Importer::normalizeDomainExpression(ac::StaticExprAttr expression) {
  auto tree = expression.getTree();
  StringRef kind = tree.getAs<StringAttr>("kind").getValue();
  if (kind != "field" && kind != "element") return expression;
  auto base = normalizeDomainExpression(tree.getAs<ac::StaticExprAttr>("base"));
  if (failed(base)) return failure();
  auto value = base->getTree();
  StringRef baseKind = value.getAs<StringAttr>("kind").getValue();
  if (baseKind == "literal") {
    NamedAttrList selected(tree);
    selected.set("base", *base);
    auto closed = ac::StaticExprAttr::get(b.getContext(), selected.getDictionary(b.getContext()));
    ac::HardwareAnalysis analysis(*body);
    if (analysis.isStaticEvaluable(closed, {})) {
      auto evaluated = analysis.evaluateStatic(closed, {}, body->getOperation());
      if (failed(evaluated)) return failure();
      auto result = dyn_cast<DictionaryAttr>(*evaluated);
      if (!result) return error() << "selected static configuration must be a Domain";
      selected.set("kind", b.getStringAttr("literal"));
      selected.erase("base");
      selected.erase("index");
      selected.erase("name");
      selected.set("value", result);
      return ac::StaticExprAttr::get(b.getContext(), selected.getDictionary(b.getContext()));
    }
  }
  if (kind == "field" && baseKind == "dict") {
    auto fields = value.getAs<DictionaryAttr>("fields");
    auto child = fields.getAs<ac::StaticExprAttr>(tree.getAs<StringAttr>("name").getValue());
    if (!child) return error() << "static domain key is absent";
    return normalizeDomainExpression(child);
  }
  if (kind == "element" && baseKind == "list") {
    ac::HardwareAnalysis analysis(*body);
    auto indexExpression = tree.getAs<ac::StaticExprAttr>("index");
    if (!analysis.isStaticEvaluable(indexExpression, {})) {
      NamedAttrList fields(tree);
      fields.set("base", *base);
      return ac::StaticExprAttr::get(b.getContext(), fields.getDictionary(b.getContext()));
    }
    auto index = analysis.evaluateStatic(indexExpression, {}, body->getOperation());
    auto integer = succeeded(index) ? dyn_cast<ac::MathIntAttr>(*index) : ac::MathIntAttr();
    if (!integer) return error() << "domain index requires a closed static integer";
    llvm::APSInt ordinal(integer.getCanonicalValue());
    auto elements = value.getAs<ArrayAttr>("elements");
    if (ordinal.isNegative() || ordinal.getActiveBits() > 64 || ordinal.getZExtValue() >= elements.size())
      return error() << "domain index is outside the static container";
    return normalizeDomainExpression(cast<ac::StaticExprAttr>(elements[ordinal.getZExtValue()]));
  }
  NamedAttrList fields(tree);
  fields.set("base", *base);
  return ac::StaticExprAttr::get(b.getContext(), fields.getDictionary(b.getContext()));
}
FailureOr<std::string> Importer::clockDomainName(ac::StaticExprAttr expression) {
  using Identity = std::pair<std::string, DictionaryAttr>;
  // Preserve an unresolved integer formal index as a structural selector. The
  // existing parameter substitution/evaluator resolves it at the binding site.
  auto indexIdentity = [&](auto &&self, ac::StaticExprAttr expression) -> FailureOr<std::string> {
    auto tree = expression.getTree();
    StringRef kind = tree.getAs<StringAttr>("kind").getValue();
    auto encode = [](StringRef tag, StringRef text) {
      return tag.str() + std::to_string(text.size()) + "_" + text.str();
    };
    if (kind == "literal") {
      auto value = tree.getAs<DictionaryAttr>("value").getAs<ac::MathIntAttr>("value");
      if (!value) return error() << "Domain index requires a static integer";
      return encode("v", value.getCanonicalValue());
    }
    if (kind == "reference") {
      auto reference = tree.getAs<DictionaryAttr>("ref");
      auto name = reference.getAs<StringAttr>("name");
      auto owner = reference.getAs<FlatSymbolRefAttr>("owner");
      for (const auto &module : modules)
        if (module.symbol == owner)
          for (const auto &parameter : module.parameters)
            if (parameter.name == name.getValue() && !parameter.staticType)
              return encode("r", name.getValue());
      return error() << "Domain index must reference a declared integer static parameter";
    }
    if (kind == "unary" || kind == "binary") {
      std::string identity = encode(kind == "unary" ? "u" : "b", tree.getAs<StringAttr>("operator").getValue());
      SmallVector<StringRef, 2> fields;
      if (kind == "unary") fields.push_back("operand");
      else fields.append({"lhs", "rhs"});
      for (StringRef field : fields) {
        auto child = self(self, tree.getAs<ac::StaticExprAttr>(field));
        if (failed(child)) return failure();
        identity += encode("e", *child);
      }
      return identity;
    }
    return error() << "Domain index requires a static integer expression";
  };
  auto identify = [&](auto &&self, ac::StaticExprAttr expression) -> FailureOr<Identity> {
    auto normalized = normalizeDomainExpression(expression);
    if (failed(normalized)) return failure();
    auto tree = normalized->getTree();
    StringRef kind = tree.getAs<StringAttr>("kind").getValue();
    if (kind == "literal") {
      auto value = tree.getAs<DictionaryAttr>("value");
      StringRef valueKind = value.getAs<StringAttr>("kind").getValue();
      if (valueKind == "list" || valueKind == "dict") {
        // Reuse the same structural container path for published StaticValues
        // and directly captured container expressions, without evaluating them.
        auto childExpression = [&](Attribute value) {
          NamedAttrList child(tree);
          child.set("value", value);
          return ac::StaticExprAttr::get(b.getContext(), child.getDictionary(b.getContext()));
        };
        NamedAttrList container(tree);
        container.set("kind", b.getStringAttr(valueKind));
        container.erase("value");
        if (valueKind == "list") {
          SmallVector<Attribute> elements;
          for (Attribute child : value.getAs<ArrayAttr>("values")) elements.push_back(childExpression(child));
          container.set("elements", b.getArrayAttr(elements));
        } else {
          SmallVector<NamedAttribute> fields;
          for (NamedAttribute child : value.getAs<DictionaryAttr>("values"))
            fields.push_back(b.getNamedAttr(child.getName().getValue(), childExpression(child.getValue())));
          container.set("fields", b.getDictionaryAttr(fields));
        }
        return self(self, ac::StaticExprAttr::get(b.getContext(), container.getDictionary(b.getContext())));
      }
      auto clock = value.getAs<StringAttr>("clock");
      if (!clock || value.getAs<StringAttr>("kind").getValue() != "domain")
        return error() << "clock binding requires a static Domain";
      return Identity{"c" + std::to_string(clock.getValue().size()) + "_" + clock.getValue().str(),
          b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("domain"))})};
    }
    if (kind == "list" || kind == "dict") {
      DictionaryAttr elementType;
      std::string name = kind == "list" ? "l" : "m";
      auto append = [&](ac::StaticExprAttr child, StringRef key) -> LogicalResult {
        FailureOr<Identity> identity = self(self, child);
        if (failed(identity)) return failure();
        if (elementType && elementType != identity->second)
          return error() << "Domain container elements must have one static type";
        elementType = identity->second;
        name += std::to_string(key.size()) + "_" + key.str() +
                std::to_string(identity->first.size()) + "_" + identity->first;
        return success();
      };
      if (kind == "list") {
        for (Attribute child : tree.getAs<ArrayAttr>("elements"))
          if (failed(append(cast<ac::StaticExprAttr>(child), {}))) return failure();
      } else {
        for (NamedAttribute child : tree.getAs<DictionaryAttr>("fields"))
          if (failed(append(cast<ac::StaticExprAttr>(child.getValue()), child.getName().getValue()))) return failure();
      }
      if (!elementType) return error() << "selected Domain container cannot be empty";
      return Identity{name, b.getDictionaryAttr({
          b.getNamedAttr("kind", b.getStringAttr(kind == "list" ? "domain_list" : "domain_dict")),
          b.getNamedAttr("element", elementType)})};
    }
    if (kind == "reference") {
      auto ref = tree.getAs<DictionaryAttr>("ref");
      StringRef name = ref.getAs<StringAttr>("name").getValue();
      auto owner = ref.getAs<FlatSymbolRefAttr>("owner");
      for (const auto &module : modules)
        if (module.symbol == owner)
          for (const auto &parameter : module.parameters)
            if (parameter.name == name && parameter.staticType)
              return Identity{"p" + std::to_string(name.size()) + "_" + name.str(), parameter.staticType};
      return error() << "clock binding reference must name a declared Domain parameter";
    }
    if (kind == "field" || kind == "element") {
      FailureOr<Identity> base = self(self, tree.getAs<ac::StaticExprAttr>("base"));
      if (failed(base)) return failure();
      StringRef baseKind = base->second.getAs<StringAttr>("kind").getValue();
      if (kind == "field") {
        if (baseKind != "domain_dict") return error() << "named Domain selection requires a dict parameter";
        StringRef name = tree.getAs<StringAttr>("name").getValue();
        return Identity{base->first + "k" + std::to_string(name.size()) + "_" + name.str(),
                        base->second.getAs<DictionaryAttr>("element")};
      }
      if (baseKind != "domain_list") return error() << "indexed Domain selection requires a list parameter";
      ac::HardwareAnalysis analysis(*body);
      auto indexExpression = tree.getAs<ac::StaticExprAttr>("index");
      if (!analysis.isStaticEvaluable(indexExpression, {})) {
        auto selector = indexIdentity(indexIdentity, indexExpression);
        if (failed(selector)) return failure();
        return Identity{base->first + "s" + std::to_string(selector->size()) + "_" + *selector,
                        base->second.getAs<DictionaryAttr>("element")};
      }
      auto index = analysis.evaluateStatic(indexExpression, {}, body->getOperation());
      auto integer = succeeded(index) ? dyn_cast<ac::MathIntAttr>(*index) : ac::MathIntAttr();
      if (!integer) return error() << "domain index requires a static integer";
      llvm::APSInt ordinal(integer.getCanonicalValue());
      if (ordinal.isNegative() || ordinal.getActiveBits() > 64)
        return error() << "domain index must be nonnegative";
      return Identity{base->first + "i" + std::to_string(ordinal.getZExtValue()) + "_",
                      base->second.getAs<DictionaryAttr>("element")};
    }
    return error() << "clock binding requires a static Domain path";
  };
  auto identity = identify(identify, expression);
  if (failed(identity)) return failure();
  if (identity->second.getAs<StringAttr>("kind").getValue() != "domain")
    return error() << "clock binding selects a container instead of one Domain";
  return identity->first;
}
LogicalResult Importer::prepareDomainInputs(ModuleDecl &decl) {
  NamedAttrList expressions;
  auto add = [&](ac::StaticExprAttr expression) -> LogicalResult {
    auto normalized = normalizeDomainExpression(expression);
    if (failed(normalized)) return failure();
    auto name = clockDomainName(*normalized);
    if (failed(name)) return failure();
    expressions.set(*name, *normalized);
    return success();
  };
  // Only declarations directly owned by the Module are persistent storage.
  for (size_t i = 0; i < decl.node.array("body").size(); ++i) {
    auto node = decl.node.item("body", i);
    if (node.kind() == "AnnAssign" && cdcKind(node.child("annotation")).empty()) {
      auto value = node.child("value");
      bool policy = value.kind() == "Call" && intrinsic(value.child("func"), "state");
      bool noReset = false, explicitReset = false;
      if (policy)
        for (size_t i = 0; i < value.array("keywords").size(); ++i) {
          auto keyword = value.item("keywords", i);
          explicitReset |= keyword.string("arg") == "reset";
          auto literal = keyword.child("value");
          auto text = dyn_cast_or_null<StringAttr>(literal.get("value"));
          noReset |= keyword.string("arg") == "reset_kind" && text && text.getValue() == "none";
        }
      decl.needsDefaultReset |= !policy || (!explicitReset && !noReset);
    }
    if (node.kind() == "AnnAssign" && !cdcKind(node.child("annotation")).empty() && !node.child("value"))
      decl.needsDefaultReset = true;
  }
  auto walk = [&](auto &&self, const AstNode &node) -> LogicalResult {
    if (queueCall(node) || !memoryKind(node).empty()) decl.needsDefaultReset = true;

    if (node.kind() == "Call" && intrinsic(node.child("func"), "domain")) {
      auto expression = staticExpr(node, decl.symbol);
      if (failed(expression) || failed(add(*expression))) return failure();
    }
    if (node.kind() == "Call" && !cdcKind(node).empty()) {
      auto call = node;
      bool explicitReset = false;
      for (size_t i = 0; call && i < call.array("keywords").size(); ++i) {
        auto keyword = call.item("keywords", i);
        StringRef key = keyword.string("arg");
        explicitReset |= key == "reset";
        if (llvm::is_contained(ArrayRef<StringRef>{"source", "target", "write", "read"}, key)) {
          auto expression = staticExpr(keyword.child("value"), decl.symbol);
          if (failed(expression) || failed(add(*expression))) return failure();
        }
      }
      decl.needsDefaultReset |= !explicitReset;
    }
    if (moduleCall(node)) {
      auto binding = lookupBinding(node.child("func"));
      DictionaryAttr signature;
      if (auto found = local.find(node.child("func").string("id")); found != local.end()) {
        auto encoded = moduleSignatureAttrs(modules[found->second]);
        if (failed(encoded)) return failure();
        signature = *encoded;
        decl.needsDefaultReset |= modules[found->second].needsDefaultReset;
      } else {
        auto view = headers.lookupModule(binding->symbol);
        if (view) signature = view.declaration->getAttrDictionary();
        // Published reset use is explicit metadata, not an input-name heuristic.
        if (signature && signature.getAs<BoolAttr>("ac.uses_default_reset"))
          decl.needsDefaultReset |= signature.getAs<BoolAttr>("ac.uses_default_reset").getValue();
      }
      if (signature) {
        auto actuals = bindParameters(signature, node, decl.symbol, true);
        if (failed(actuals)) return failure();
        if (auto paths = signature.getAs<DictionaryAttr>("ac.domain_parameter_paths"))
          for (NamedAttribute path : paths) {
            auto expression = cast<ac::StaticExprAttr>(substituteParameterAttribute(
                path.getValue(), signature, *actuals, {}));
            if (failed(add(expression))) return failure();
          }
      }
      for (size_t i = 0; i < node.array("keywords").size(); ++i) {
        auto keyword = node.item("keywords", i);
        if (keyword.string("arg") == "domain") {
          auto expression = staticExpr(keyword.child("value"), decl.symbol);
          if (failed(expression) || failed(add(*expression))) return failure();
        }
      }
    }
    for (NamedAttribute field : node.fields()) {
      if (auto child = dyn_cast<DictionaryAttr>(field.getValue())) {
        if (child.getAs<StringAttr>("kind") && failed(self(self, node.child(field.getName().getValue())))) return failure();
      } else if (auto children = dyn_cast<ArrayAttr>(field.getValue())) {
        for (size_t i = 0; i < children.size(); ++i)
          if (auto child = dyn_cast<DictionaryAttr>(children[i]); child && child.getAs<StringAttr>("kind"))
            if (failed(self(self, node.item(field.getName().getValue(), i)))) return failure();
      }
    }
    return success();
  };
  for (size_t i = 0; i < decl.node.array("body").size(); ++i)
    if (failed(walk(walk, decl.node.item("body", i)))) return failure();
  decl.domainExpressions = expressions.getDictionary(b.getContext());
  decl.needsDomain |= !expressions.empty();
  return success();
}
LogicalResult Importer::bindDomainInputs(
    const CallableDecl &callee, const AstNode &call, OpBuilder &at,
    FlatSymbolRefAttr parent, llvm::StringMap<Value> &values,
    SmallVectorImpl<Value> &inputs, Operation *instance) {
  if (!callee.needsDomain) return success();
  if (!values.contains("pyc_clk") || !values.contains("pyc_rst"))
    return error() << "direct module call lacks inferred physical domain";
  bool explicitDefault = false;
  for (size_t i = 0; i < call.array("keywords").size(); ++i)
    explicitDefault |= call.item("keywords", i).string("arg") == "domain";
  if (!explicitDefault && activeBehavioralDecl && activeBehavioralDecl->system &&
      !activeBehavioralDecl->sampleNames.empty() &&
      !llvm::is_contained(activeBehavioralDecl->sampleNames, at.getStringAttr("pyc_clk")))
    return error() << "clocked System child requires an explicit domain binding";
  NamedAttrList bindings;
  Value defaultClock = values["pyc_clk"];
  StringAttr defaultDomain = at.getStringAttr("D");
  for (size_t i = 0; i < call.array("keywords").size(); ++i) {
    auto keyword = call.item("keywords", i);
    if (keyword.string("arg") != "domain") continue;
    auto expression = staticExpr(keyword.child("value"), parent);
    if (failed(expression)) return failure();
    auto name = clockDomainName(*expression);
    if (failed(name)) return failure();
    defaultDomain = at.getStringAttr(*name);
    defaultClock = values.lookup("__pyc_clock_" + *name);
    if (!defaultClock) return error() << "explicit clock binding lacks its inferred input";
  }
  inputs.push_back(defaultClock);
  inputs.push_back(values["pyc_rst"]);
  if (auto names = callee.signature.getAs<ArrayAttr>("ac.domain_params"))
    if (llvm::is_contained(names, at.getStringAttr("D"))) bindings.set("D", defaultDomain);
  if (auto paths = callee.signature.getAs<DictionaryAttr>("ac.domain_parameter_paths"))
    for (NamedAttribute path : paths) {
      auto expression = cast<ac::StaticExprAttr>(substituteParameterAttribute(
          path.getValue(), callee.signature, callee.parameters, callee.typeArguments));
      auto name = clockDomainName(expression);
      if (failed(name)) return failure();
      Value clock = values.lookup("__pyc_clock_" + *name);
      if (!clock) return error() << "extra clock binding lacks its inferred input";
      inputs.push_back(clock);
      bindings.set(path.getName(), at.getStringAttr(*name));
    }
  if (!bindings.empty()) instance->setAttr("ac.domain_bindings", bindings.getDictionary(at.getContext()));
  return success();
}
