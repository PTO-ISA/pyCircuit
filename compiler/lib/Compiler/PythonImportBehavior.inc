FailureOr<MatchPatternKey> Importer::matchPatternKey(const AstNode &atom) {
  auto diagnostic = [&] {
    return mlir::emitError(atom.location(b.getContext(), source.path));
  };
  if (atom.kind() == "MatchSingleton")
    return MatchPatternKey{atom, atom.get("value"), {}};
  auto value = atom.child("value");
  if (value.kind() == "Attribute") {
    auto member = resolveEnumMember(value);
    if (failed(member))
      return diagnostic() << "cannot resolve canonical match Enum member key";
    if (!*member)
      return diagnostic()
             << "match Attribute key requires a canonical Enum member";
    return MatchPatternKey{atom, (*member)->member, (*member)->type};
  }
  bool negative = value.kind() == "UnaryOp";
  auto literal = negative ? value.child("operand") : value;
  auto encoded = dyn_cast_or_null<DictionaryAttr>(literal.get("value"));
  auto integer = parseStaticInteger(
      b, encoded.getAs<StringAttr>("integer").getValue(), diagnostic);
  if (failed(integer))
    return failure();
  Attribute key = *integer;
  if (negative) {
    auto zero =
        ac::MathIntAttr::get(b.getContext(), llvm::APSInt::getUnsigned(0));
    auto result = ac::detail::evaluateValue(ac::detail::ValueOpcode::Sub,
                                            {zero, key}, diagnostic);
    if (failed(result))
      return failure();
    key = *result;
  }
  return MatchPatternKey{atom, key, {}};
}

FailureOr<Value> Importer::materializeMatchKey(const MatchPatternKey &key,
                                               Type type, OpBuilder &at,
                                               FlatSymbolRefAttr ownerSymbol,
                                               bool fixedBits) {
  using ac::detail::ValueKind;
  if (key.enumeration)
    return createOp(at, key.site.location(b.getContext(), source.path),
                    ac::EnumCreateOp::getOperationName(), {}, TypeRange{type},
                    {at.getNamedAttr("member", cast<StringAttr>(key.value))})
        ->getResult(0);
  auto boolean = dyn_cast<BoolAttr>(key.value);
  Attribute code =
      boolean ? Attribute(ac::MathIntAttr::get(
                    b.getContext(),
                    llvm::APSInt::getUnsigned(boolean.getValue() ? 1 : 0)))
              : key.value;
  auto site = numericSite(key.site, ownerSymbol);
  auto literal = ac::StaticExprAttr::get(
      b.getContext(),
      at.getDictionaryAttr(
          {at.getNamedAttr("kind", at.getStringAttr("literal")),
           at.getNamedAttr(
               "value",
               at.getDictionaryAttr(
                   {at.getNamedAttr("kind", at.getStringAttr("integer")),
                    at.getNamedAttr("value", code)})),
           at.getNamedAttr("origin", site.origin),
           at.getNamedAttr("location", site.sourceSpan)}));
  auto value = createOp(at, key.site.location(b.getContext(), source.path),
                        ac::BitsConstantOp::getOperationName(), {},
                        TypeRange{type}, {at.getNamedAttr("value", literal)})
                   ->getResult(0);
  if (fixedBits)
    fixedValues.insert(value);
  else
    remember(value, boolean ? ValueKind::Boolean : ValueKind::Integer, {},
             key.value);
  return value;
}

LogicalResult Importer::matchStatement(const AstNode &statement, OpBuilder &at,
                                       FlatSymbolRefAttr ownerSymbol,
                                       BranchEnvironment &environment,
                                       llvm::StringSet<> &owners) {
  using ac::detail::ValueKind;
  auto syntax = readMatchSyntax(statement, [&](const AstNode &site) {
    return mlir::emitError(site.location(b.getContext(), source.path));
  });
  if (failed(syntax))
    return failure();
  // Resolve all keys through existing literal/member authorities before reading
  // the subject. No key expression is executed as a runtime source expression.
  SmallVector<SmallVector<MatchPatternKey>> keys;
  for (const auto &arm : *syntax) {
    SmallVector<MatchPatternKey> alternatives;
    for (const AstNode &atom : arm.atoms) {
      auto key = matchPatternKey(atom);
      if (failed(key))
        return failure();
      if (llvm::any_of(alternatives, [&](const MatchPatternKey &prior) {
            return prior.value == key->value &&
                   prior.enumeration == key->enumeration;
          }))
        return mlir::emitError(atom.location(b.getContext(), source.path))
               << "duplicate canonical match alternatives within one arm";
      alternatives.push_back(*key);
    }
    keys.push_back(std::move(alternatives));
  }
  BranchEnvironment incoming = environment;
  llvm::StringMap<Instance *> noInstances;
  auto &instances = activeRule && activeRule->instances ? *activeRule->instances
                                                        : noInstances;
  auto subjectSite = statement.child("subject");
  auto subject =
      expression(subjectSite, at, ownerSymbol, incoming.values, instances);
  if (failed(subject))
    return failure();
  auto diagnostic = [&] {
    return mlir::emitError(subjectSite.location(b.getContext(), source.path));
  };
  auto enumeration = dyn_cast<ac::EnumType>(subject->getType());
  auto scalar = dyn_cast<ac::BitsType>(subject->getType());
  auto info = valueInfo(*subject);
  bool boolean = !enumeration && info.sourceKind == ValueKind::Boolean;
  bool integer = !enumeration && info.sourceKind == ValueKind::Integer;
  bool fixed = !enumeration && fixedValues.contains(*subject);
  if (!enumeration && (!scalar || (!boolean && !integer && !fixed)))
    return diagnostic()
           << "match selector requires authoritative Boolean, unsigned bits, "
              "finite nonnegative Integer or nominal Enum";
  if (integer && (arithmeticValues.contains(*subject) || !info.interval ||
                  info.interval->lower.isNegative()))
    return diagnostic()
           << "match signed or mathematical Integer selectors are unsupported";
  ac::HardwareAnalysis analysis(*body);
  if (scalar && !analysis.isStaticEvaluable(scalar.getWidth(), {}))
    return diagnostic() << "match selector requires a closed positive width";
  auto width = analysis.getPackedWidth(subject->getType(), {},
                                       at.getInsertionBlock()->getParentOp());
  if (failed(width))
    return failure();
  if (!*width || (boolean && *width != 1))
    return diagnostic() << "match selector requires a positive finite width";
  size_t enumMemberCount = 0;
  if (enumeration) {
    auto definition = analysis.resolveEnum(
        enumeration, at.getInsertionBlock()->getParentOp());
    if (failed(definition))
      return failure();
    enumMemberCount = definition->members.size();
  }
  SmallVector<MatchPatternKey> seen;
  auto sameKey = [](const MatchPatternKey &a, const MatchPatternKey &c) {
    return a.value == c.value && a.enumeration == c.enumeration;
  };
  for (auto [index, arm] : llvm::enumerate(*syntax)) {
    bool anyRepeated = false, allRepeated = !arm.catchAll;
    for (const MatchPatternKey &key : keys[index]) {
      auto error = [&] {
        return mlir::emitError(key.site.location(b.getContext(), source.path));
      };
      if (enumeration) {
        if (key.enumeration != enumeration)
          return error()
                 << "match Enum key requires a member of the same nominal type";
      } else if (boolean) {
        if (!isa<BoolAttr>(key.value) || key.enumeration)
          return error()
                 << "match Boolean selector requires True/False singleton keys";
      } else {
        auto literal = dyn_cast<ac::MathIntAttr>(key.value);
        if (!literal || key.enumeration)
          return error() << "match unsigned/Integer selector requires Integer "
                            "literal keys";
        llvm::APSInt value(literal.getCanonicalValue());
        if (value.isNegative() || value.getActiveBits() > *width)
          return error() << "match Integer key is outside the selector's "
                            "unsigned width";
      }
      bool repeated = llvm::any_of(seen, [&](const MatchPatternKey &prior) {
        return sameKey(prior, key);
      });
      anyRepeated |= repeated;
      allRepeated &= repeated;
      if (!repeated)
        seen.push_back(key);
    }
    if (allRepeated)
      mlir::emitWarning(arm.originalSite.location(b.getContext(), source.path))
          << "match arm is shadowed for known keys; arm remains in four-state "
             "priority";
    else if (anyRepeated)
      mlir::emitWarning(arm.originalSite.location(b.getContext(), source.path))
          << "match arm repeats earlier keys; arm remains in four-state "
             "priority";
  }
  // Cardinality checks stay sparse. Width is compared to log2(N); this does not
  // allocate 2**W, a width-sized mask, or pad literal keys to the selector
  // width.
  uint64_t count = seen.size();
  bool physicalComplete =
      count && llvm::isPowerOf2_64(count) && llvm::Log2_64(count) == *width;
  bool knownComplete = boolean       ? count == 2
                       : enumeration ? count == enumMemberCount
                                     : physicalComplete;
  bool integerFactsComplete = false;
  if (integer) {
    uint64_t inRange = 0;
    for (const auto &key : seen) {
      llvm::APSInt value(cast<ac::MathIntAttr>(key.value).getCanonicalValue());
      if (llvm::APSInt::compareValues(value, info.interval->lower) >= 0 &&
          llvm::APSInt::compareValues(value, info.interval->upper) < 0)
        ++inRange;
    }
    auto lower = ac::MathIntAttr::get(b.getContext(), info.interval->lower);
    auto upper = ac::MathIntAttr::get(b.getContext(), info.interval->upper);
    auto distance = ac::detail::evaluateValue(ac::detail::ValueOpcode::Sub,
                                              {upper, lower}, diagnostic);
    if (failed(distance))
      return failure();
    integerFactsComplete =
        llvm::APSInt::compareValues(
            llvm::APSInt(cast<ac::MathIntAttr>(*distance).getCanonicalValue()),
            llvm::APSInt::getUnsigned(inRange)) == 0;
    knownComplete |= integerFactsComplete;
  }
  StringRef covered = enumeration && !physicalComplete ? "declared Enum members"
                      : integerFactsComplete && !physicalComplete
                          ? "proven Integer range"
                          : "physical known codes";
  if (knownComplete) {
    auto site =
        syntax->back().catchAll ? syntax->back().originalSite : statement;
    mlir::emitRemark(site.location(b.getContext(), source.path))
        << "match covers " << covered
        << "; source catch-all or physical fallthrough is retained";
  }
  SmallVector<Value> predicates;
  for (auto [index, arm] : llvm::enumerate(*syntax)) {
    Value predicate;
    for (const MatchPatternKey &key : keys[index]) {
      auto value = materializeMatchKey(key, subject->getType(), at, ownerSymbol,
                                       fixed && !boolean);
      if (failed(value))
        return failure();
      auto equal = compareSourceValues(*subject, *value, "eq", subjectSite,
                                       key.site, key.site, at, ownerSymbol);
      if (failed(equal))
        return failure();
      if (!predicate)
        predicate = *equal;
      else {
        Value previous = predicate;
        predicate =
            createOp(at, key.site.location(b.getContext(), source.path),
                     ac::BitsBinaryOp::getOperationName(), {previous, *equal},
                     TypeRange{bits(1, key.site, ownerSymbol)},
                     {at.getNamedAttr("opcode", at.getStringAttr("or"))})
                ->getResult(0);
        remember(predicate, ValueKind::Boolean);
        if (failed(propagateClosed(predicate, ac::detail::ValueOpcode::OrBool,
                                   {previous, *equal}, key.site)))
          return failure();
      }
    }
    predicates.push_back(predicate);
  }
  SmallVector<BranchEnvironment> environments;
  Value remaining;
  if (incoming.continuation) {
    auto one = constant(bits(1, statement, ownerSymbol), 1, statement, at,
                        ownerSymbol);
    if (failed(one))
      return failure();
    remaining = *one;
  }
  for (auto [index, arm] : llvm::enumerate(*syntax)) {
    environments.push_back(incoming);
    if (remaining) {
      Value demand = remaining;
      if (!arm.catchAll) {
        auto selected = checkAnd(remaining, predicates[index], arm.originalSite,
                                 at, ownerSymbol);
        auto opposite =
            checkNot(predicates[index], arm.originalSite, at, ownerSymbol);
        if (failed(selected) || failed(opposite))
          return failure();
        auto unmatched =
            checkAnd(remaining, *opposite, arm.originalSite, at, ownerSymbol);
        if (failed(unmatched))
          return failure();
        demand = *selected;
        remaining = *unmatched;
      }
      if (failed(enterCheckBranch(environments.back(), demand, arm.originalSite,
                                  at, ownerSymbol)))
        return failure();
    }
    AstNode returned;
    if (failed(statements(arm.originalSite, "body", at, ownerSymbol,
                          environments.back(), owners, returned)))
      return failure();
    if (returned)
      return mlir::emitError(returned.location(b.getContext(), source.path))
             << "rule return inside a match arm is unsupported";
  }
  BranchEnvironment fallback =
      syntax->back().catchAll ? environments.back() : incoming;
  BranchEnvironment joinIncoming = incoming;
  if (incoming.continuation) {
    // Fold local arm continuations by ordered selection. The incoming
    // continuation is applied once after the complete first-match fold.
    auto one = constant(bits(1, statement, ownerSymbol), 1, statement, at,
                        ownerSymbol);
    if (failed(one))
      return failure();
    joinIncoming.continuation = *one;
    if (!syntax->back().catchAll)
      fallback.continuation = *one;
  }
  for (size_t i = syntax->size(); i-- > 0;) {
    if ((*syntax)[i].catchAll)
      continue;
    auto condition =
        predicateView(predicates[i], (*syntax)[i].originalSite, ownerSymbol);
    if (failed(condition))
      return failure();
    auto joined =
        joinBranches(*condition, joinIncoming, environments[i], fallback,
                     (*syntax)[i].originalSite, at, ownerSymbol);
    if (failed(joined))
      return failure();
    fallback = std::move(*joined);
  }
  if (incoming.continuation) {
    auto continued = checkAnd(incoming.continuation, fallback.continuation,
                              statement, at, ownerSymbol);
    if (failed(continued))
      return failure();
    fallback.ambient = incoming.ambient;
    fallback.continuation = *continued;
  }
  environment = std::move(fallback);
  return success();
}

LogicalResult Importer::statements(const AstNode &parent, StringRef field,
                                   OpBuilder &at, FlatSymbolRefAttr symbol,
                                   BranchEnvironment &environment,
                                   llvm::StringSet<> &owners,
                                   AstNode &returned) {
  auto &values = environment.values;
  auto &writes = environment.writes;
  llvm::StringMap<Instance *> noInstances;
  auto &instances = activeRule && activeRule->instances ? *activeRule->instances
                                                        : noInstances;
  for (size_t i = 0; i < parent.array(field).size(); ++i) {
    auto stmt = parent.item(field, i);
    auto loc = stmt.location(b.getContext(), source.path);
    if (docstring(stmt) || stmt.kind() == "Pass")
      continue;
    if (returned)
      return error() << "statement after rule return is unsupported";
    if (stmt.kind() == "Return") {
      if (activeRule && activeRule->structural && stmt.child("value"))
        return error() << "structural rule return must be empty";
      if (activeRule && activeRule->structural &&
          parent.kind() != "FunctionDef")
        return error() << "rule return inside a branch is unsupported";
      if (activeRule && activeRule->structural)
        continue;
      if (failed(fixedResultTargets(stmt)))
        return failure();
      returned = stmt;
      continue;
    }
    if (stmt.kind() == "Assert") {
      if (failed(captureSourceCheck(stmt, at, symbol, environment)))
        return failure();
      continue;
    }
    if (activeRule && activeRule->structural && stmt.kind() == "Expr") {
      auto call = stmt.child("value");
      auto function = call.kind() == "Call" ? call.child("func") : AstNode();
      if ((activeRule->binding && call.value == activeRule->binding.value) ||
          (function.kind() == "Name" &&
           instances.contains(function.string("id")))) {
        if (failed(structuralBinding(call, at, symbol, environment)))
          return failure();
        continue;
      }
      StringRef callee =
          call.kind() == "Call" && call.child("func").kind() == "Name"
              ? call.child("func").string("id")
              : StringRef();
      if (call.kind() == "Call" && intrinsic(call.child("func"), "log"))
        callee = "log";
      if (call.kind() == "Call" && intrinsic(call.child("func"), "report"))
        callee = "report";
      if (parent.kind() == "FunctionDef" &&
          (callee == "log" || callee == "report")) {
        // A structural rule does not lower its own observations: `ac.observe`
        // requires module placement, so the module-level instrumentation pass
        // republishes every site from the same rule body. Record the site so
        // that pass must prove it published it; a source observation may not
        // disappear because two readings of one rule body disagreed.
        activeRule->deferredObservations.push_back({parent, stmt});
        continue;
      }
      return error() << "unsupported expression statement in pure rule";
    }
    // A `log`/`report` statement in a behavioral rule body. The observation
    // value is a rule-local SSA value, so it leaves the rule on the existing
    // result suffix -- the same route a source assertion already takes -- and
    // publishSourceChecks creates the module-placed `ac.observe` once the rule
    // operation exists. An observation carries an unconditional path, so a
    // branch-nested site stays unsupported and is named here.
    if (activeRule && !activeRule->structural && stmt.kind() == "Expr") {
      auto call = stmt.child("value");
      StringRef callee =
          call.kind() == "Call" && call.child("func").kind() == "Name"
              ? call.child("func").string("id")
              : StringRef();
      if (call.kind() == "Call" && intrinsic(call.child("func"), "log"))
        callee = "log";
      if (call.kind() == "Call" && intrinsic(call.child("func"), "report"))
        callee = "report";
      if (callee == "log" || callee == "report") {
        if (parent.kind() != "FunctionDef")
          return mlir::emitError(loc)
                 << callee
                 << " observation must be an unconditional rule statement";
        if (failed(captureRuleObservation(parent, stmt, at, symbol, environment)))
          return failure();
        continue;
      }
    }
    if (stmt.kind() == "Match") {
      if (failed(matchStatement(stmt, at, symbol, environment, owners)))
        return failure();
      continue;
    }
    if (stmt.kind() == "If") {
      auto guard =
          expression(stmt.child("test"), at, symbol, values, instances);
      if (failed(guard))
        return failure();
      auto condition = predicateView(*guard, stmt.child("test"), symbol);
      if (failed(condition))
        return failure();
      auto yes = environment, no = environment;
      if (environment.continuation) {
        auto opposite = checkNot(*guard, stmt, at, symbol);
        if (failed(opposite) ||
            failed(enterCheckBranch(yes, *guard, stmt, at, symbol)) ||
            failed(enterCheckBranch(no, *opposite, stmt, at, symbol)))
          return failure();
      }
      AstNode yr, nr;
      if (failed(statements(stmt, "body", at, symbol, yes, owners, yr)) ||
          failed(statements(stmt, "orelse", at, symbol, no, owners, nr)))
        return failure();
      if (yr || nr)
        return error() << "rule return inside a branch is unsupported";
      auto joined =
          joinBranches(*condition, environment, yes, no, stmt, at, symbol);
      if (failed(joined))
        return failure();
      environment = std::move(*joined);
      continue;
    }
    if (stmt.kind() != "Assign" && stmt.kind() != "AnnAssign")
      return mlir::emitError(loc)
             << "unsupported behavioral rule statement '" << stmt.kind() << "'";
    if (activeRule && activeRule->structural)
      return mlir::emitError(loc)
             << "structural rule-local assignments are unsupported";
    auto fixed = bindFixedResults(stmt, at, symbol, values, instances,
                                  &environment.boundaries, owners, false);
    if (failed(fixed))
      return failure();
    if (*fixed)
      continue;
    AstNode target = stmt.kind() == "AnnAssign" ? stmt.child("target")
                                                : stmt.item("targets", 0);
    if (stmt.kind() == "Assign" && stmt.array("targets").size() != 1)
      return error() << "rule assignment requires one target";
    if (StringRef root = assignmentRoot(target);
        root == "pyc_clk" || root == "pyc_rst")
      return error() << "reserved physical domain pin '" << root
                     << "' cannot be assigned in source";
    std::optional<BindingBoundary> binding;
    if (target.kind() == "Name") {
      auto found = environment.boundaries.find(target.string("id"));
      if (found != environment.boundaries.end())
        binding = found->second;
    }
    if (stmt.kind() == "AnnAssign") {
      auto annotation = stmt.child("annotation");
      auto type = portType(annotation, symbol);
      if (failed(type))
        return failure();
      BindingBoundary declared{{assignmentRoot(target).str(), *type, annotation,
                                annotationKind(annotation),
                                fixedAnnotation(annotation)},
                               true};
      if (binding && binding->declared &&
          !sameBindingBoundary(*binding, declared))
        return mlir::emitError(loc)
               << "annotation cannot replace a declared binding boundary";
      if (!binding || !binding->declared)
        binding = declared;
    }
    if (target.kind() == "Attribute") {
      Type targetType;
      if (indexedFieldTarget(target)) {
        auto type = expressionType(target, values, instances, symbol);
        if (failed(type))
          return failure();
        targetType = *type;
      } else {
        auto current = expression(target, at, symbol, values, instances);
        if (failed(current))
          return failure();
        targetType = current->getType();
      }
      binding = BindingBoundary{{assignmentRoot(target).str(),
                                 targetType,
                                 target,
                                 {},
                                 isa<ac::BitsType>(targetType)},
                                true};
    }
    if (target.kind() == "Subscript") {
      if (target.child("value").kind() != "Name" ||
          !values.contains(target.child("value").string("id")))
        return error() << "indexed assignment requires a named table";
      auto table = dyn_cast<ac::TableType>(
          values[target.child("value").string("id")].getType());
      if (!table)
        return error() << "indexed assignment requires a table";
      binding = BindingBoundary{{assignmentRoot(target).str(),
                                 table.getElementType(),
                                 target,
                                 {},
                                 isa<ac::BitsType>(table.getElementType())},
                                true};
    }
    std::optional<Type> expected;
    if (binding && binding->contract.type)
      expected = binding->contract.type;
    auto value = expression(stmt.child("value"), at, symbol, values, instances,
                            expected);
    if (failed(value))
      return failure();
    if (!binding)
      binding = inferredBoundary(assignmentRoot(target), *value, stmt);
    auto converted =
        applyBindingBoundary(*value, *binding, stmt.child("value"), at, symbol);
    if (failed(converted))
      return failure();
    value = *converted;
    StringRef name = assignmentRoot(target);
    if (name.empty() ||
        failed(updateAssignment(target, *value, stmt, at, symbol, values)))
      return failure();
    if (target.kind() == "Name")
      environment.boundaries[name] = *binding;
    if (owners.contains(name)) {
      auto enabled = constant(bits(1, stmt, symbol), 1, stmt, at, symbol);
      if (failed(enabled))
        return failure();
      writes[name] =
          environment.continuation ? environment.continuation : *enabled;
    }
  }
  return success();
}

LogicalResult Importer::emitBehavioral(ModuleDecl &decl) {
  if (!decl.parameters.empty())
    return error() << "behavioral module static parameters are unsupported";
  b.setInsertionPointToEnd(body->getBody());
  auto module = createOp(
      b, decl.node.location(b.getContext(), source.path),
      ac::ModuleOp::getOperationName(), {}, {},
      {b.getNamedAttr("sym_name", b.getStringAttr(decl.symbol.getValue())),
       b.getNamedAttr("source_owner", owner),
       b.getNamedAttr("parameters", b.getArrayAttr({})),
       b.getNamedAttr("type_parameters", b.getArrayAttr({})),
       b.getNamedAttr("function_type", TypeAttr::get(decl.type)),
       b.getNamedAttr("input_names", portNames(b, decl.inputs)),
       b.getNamedAttr("output_names", portNames(b, decl.outputs)),
       b.getNamedAttr("ac.declaration_role", b.getStringAttr("definition")),
       b.getNamedAttr("ac.origin", occurrence(b, decl.symbol, decl.node))},
      1);
  if (decl.system)
    module->setAttr("ac.root_kind", b.getStringAttr("system"));
  for (NamedAttribute attr : decl.sourceCallAttrs)
    module->setAttr(attr.getName(), attr.getValue());
  if (failed(ac::detail::verifyModuleSourceCallContract(module)))
    return failure();
  auto graph = new Block();
  module->getRegion(0).push_back(graph);
  llvm::StringMap<Value> values;
  for (auto &port : decl.inputs) {
    auto value = graph->addArgument(port.type, module->getLoc());
    values[port.name] = value;
    remember(value, port.sourceKind);
    if (port.fixedBits)
      fixedValues.insert(value);
  }
  auto at = OpBuilder::atBlockEnd(graph);
  structuralCallSites.clear();
  SmallVector<Instance, 0> structuralStorage;
  llvm::StringMap<Instance *> structuralInstances;
  llvm::StringMap<AstNode> structuralRules;
  SmallVector<AstNode> structuralRegistrations;
  if (decl.system) {
    size_t capacity = 0;
    for (size_t i = 0; i < decl.node.array("body").size(); ++i) {
      auto stmt = decl.node.item("body", i);
      if (stmt.kind() == "Assign" && stmt.array("targets").size() == 1 &&
          moduleCall(stmt.child("value")))
        ++capacity;
    }
    structuralStorage.reserve(capacity);
    for (size_t i = 0; i < decl.node.array("body").size(); ++i) {
      auto stmt = decl.node.item("body", i);
      if (stmt.kind() == "FunctionDef" && hasDecorator(stmt, "rule")) {
        structuralRules[stmt.string("name")] = stmt;
        continue;
      }
      if (stmt.kind() != "Assign" || stmt.array("targets").size() != 1 ||
          stmt.item("targets", 0).kind() != "Name")
        continue;
      auto call = stmt.child("value");
      if (!moduleCall(call))
        continue;
      auto binding = lookupBinding(call.child("func"));
      if (!binding || binding->category != BindingCategory::Module)
        continue;
      bool callable = false;
      if (auto found = local.find(call.child("func").string("id"));
          found != local.end())
        callable = modules[found->second].sourceCallAttrs.size() == 4;
      else {
        auto view = headers.lookupModule(binding->symbol);
        callable = view && view.hasSourceCallContract();
      }
      if (callable)
        continue;
      if (!call.array("args").empty() || !call.array("keywords").empty())
        return error() << "structural module declaration requires an empty "
                          "constructor; bind authored inputs in a rule";
      Operation *callee = binding->declaration;
      std::unique_ptr<Operation, void (*)(Operation *)> temporary(
          nullptr, [](Operation *op) { if (op) op->destroy(); });
      if (!callee) {
        auto found = local.find(call.child("func").string("id"));
        if (found == local.end())
          return error() << "structural module declaration has no provider";
        ModuleDecl &target = modules[found->second];
        OperationState state(module->getLoc(),
                             ac::ModuleImportOp::getOperationName());
        SmallVector<Attribute> targetParams;
        for (const Parameter &parameter : target.parameters) {
          ac::StaticExprAttr defaultValue;
          if (parameter.value) {
            auto value = staticExpr(*parameter.value, target.symbol);
            if (failed(value))
              return failure();
            defaultValue = *value;
          }
          targetParams.push_back(
              ac::getStaticParameterAttr(b, parameter.name, defaultValue));
        }
        state.addAttribute("parameters", b.getArrayAttr(targetParams));
        state.addAttribute("type_parameters", b.getArrayAttr({}));
        state.addAttribute("input_names", portNames(b, target.inputs));
        state.addAttribute("output_names", portNames(b, target.outputs));
        state.addAttribute("function_type", TypeAttr::get(target.type));
        for (NamedAttribute attr : target.sourceCallAttrs)
          state.addAttribute(attr.getName(), attr.getValue());
        callee = Operation::create(state);
        temporary.reset(callee);
      }
      if (callee->getAttrOfType<StringAttr>("ac.root_kind"))
        return error() << "@system is root-only and cannot be instantiated as "
                          "a child";
      auto signature = dyn_cast<FunctionType>(
          callee->getAttrOfType<TypeAttr>("function_type").getValue());
      auto inputNames = callee->getAttrOfType<ArrayAttr>("input_names");
      auto outputNames = callee->getAttrOfType<ArrayAttr>("output_names");
      if (!signature || !inputNames || !outputNames ||
          inputNames.size() != signature.getNumInputs() ||
          outputNames.size() != signature.getNumResults())
        return error() << "structural module declaration has invalid signature";
      Instance instance;
      instance.name = stmt.item("targets", 0).string("id").str();
      instance.node = stmt;
      instance.callee = binding->symbol;
      instance.declaration = binding->declaration;
      auto parameters = bindParameters(callee, call, decl.symbol);
      auto typeArguments = bindTypeArguments(callee, call, decl.symbol);
      if (failed(parameters) || failed(typeArguments))
        return failure();
      instance.parameters = std::move(*parameters);
      instance.typeArguments = std::move(*typeArguments);
      for (auto [name, formalType] : llvm::zip(inputNames, signature.getInputs())) {
        auto type = substitute(formalType, callee, instance.parameters,
                               instance.typeArguments);
        if (failed(type))
          return failure();
        instance.inputs.push_back(
            {cast<StringAttr>(name).getValue().str(), *type, stmt});
      }
      for (auto [name, formalType] : llvm::zip(outputNames, signature.getResults())) {
        auto type = substitute(formalType, callee, instance.parameters,
                               instance.typeArguments);
        if (failed(type))
          return failure();
        instance.outputs.push_back(
            {cast<StringAttr>(name).getValue().str(), *type, stmt});
      }
      if (auto domain =
              callee->getAttrOfType<DictionaryAttr>("ac.domain_inputs")) {
        instance.clockInput =
            domain.getAs<IntegerAttr>("clock").getValue().getZExtValue();
        instance.resetInput =
            domain.getAs<IntegerAttr>("reset").getValue().getZExtValue();
      }
      SmallVector<Type> resultTypes;
      for (const Port &output : instance.outputs)
        resultTypes.push_back(output.type);
      instance.op = createOp(
          at, stmt.location(b.getContext(), source.path),
          ac::InstanceOp::getOperationName(), {}, resultTypes,
          {at.getNamedAttr("instance_name", at.getStringAttr(instance.name)),
           at.getNamedAttr("callee", instance.callee),
           at.getNamedAttr("parameters", at.getArrayAttr(instance.parameters)),
           at.getNamedAttr("type_arguments", at.getArrayAttr(instance.typeArguments)),
           at.getNamedAttr("occurrence", occurrence(at, decl.symbol, stmt))});
      structuralCallSites.insert(occurrence(at, decl.symbol, call));
      structuralStorage.push_back(std::move(instance));
      structuralInstances[structuralStorage.back().name] =
          &structuralStorage.back();
    }
  }
  SmallVector<StateOwner> states;
  ownerProposals.clear();
  activeBehavioralDecl = &decl;
  activeStates = &states;
  if (failed(predeclareDirectCalls(decl, at)))
    return failure();
  llvm::StringMap<Instance *> noInstances;
  AstNode returned;
  for (size_t i = 0; i < decl.node.array("body").size(); ++i) {
    auto stmt = decl.node.item("body", i);
    if (docstring(stmt))
      continue;
    if (stmt.kind() == "Return") {
      if (failed(fixedResultTargets(stmt)))
        return failure();
      if (returned)
        return error() << "multiple module returns";
      returned = stmt;
      continue;
    }
    if (returned)
      return error() << "declaration after module return is unsupported";
    if (decl.system && stmt.kind() == "FunctionDef")
      continue;
    if (decl.system && stmt.kind() == "Assign" &&
        structuralCallSites.contains(
            occurrence(at, decl.symbol, stmt.child("value"))))
      continue;
    if (stmt.kind() == "Expr") {
      auto call = stmt.child("value");
      if (decl.system && call.kind() == "Call" &&
          call.child("func").kind() == "Name" &&
          structuralRules.contains(call.child("func").string("id"))) {
        if (!call.array("args").empty() || !call.array("keywords").empty())
          return error() << "structural rule registration takes no arguments";
        structuralRegistrations.push_back(call);
        continue;
      }
      if (call.kind() == "Call" && call.child("func").kind() == "Name" &&
          behavioralRules.contains(call.child("func").string("id"))) {
        if (failed(behavioralCall(decl, call, at, values, states, true)))
          return failure();
        continue;
      }
    }
    llvm::StringSet<> tupleForbidden;
    for (const Port &input : decl.inputs)
      tupleForbidden.insert(input.name);
    for (const StateOwner &state : states)
      tupleForbidden.insert(state.name);
    auto memory = bindMemoryResults(stmt, at, decl.symbol, values, noInstances);
    if (failed(memory)) return failure();
    if (*memory) continue;
    auto queued = bindQueueResults(stmt, at, decl.symbol, values,
                                   structuralInstances);
    if (failed(queued))
      return failure();
    if (*queued)
      continue;
    for (const auto &name : queueResultNames)
      tupleForbidden.insert(name.getKey());
    auto fixed = bindFixedResults(stmt, at, decl.symbol, values, noInstances,
                                  nullptr, tupleForbidden, false);
    if (failed(fixed))
      return failure();
    if (*fixed)
      continue;
    AstNode target, initializer, annotation;
    if (stmt.kind() == "AnnAssign") {
      target = stmt.child("target");
      initializer = stmt.child("value");
      annotation = stmt.child("annotation");
    } else if (stmt.kind() == "Assign" && stmt.array("targets").size() == 1) {
      target = stmt.item("targets", 0);
      auto rhs = stmt.child("value");
      if (rhs.kind() == "Call" && rhs.child("func").kind() == "Subscript" &&
          intrinsic(rhs.child("func").child("value"), "table")) {
        annotation = rhs.child("func");
        if (!rhs.array("args").empty() || rhs.array("keywords").size() != 1 ||
            rhs.item("keywords", 0).string("arg") != "init")
          return error() << "table allocation requires exactly init";
        initializer = rhs.item("keywords", 0).child("value");
      } else {
        StringRef root = assignmentRoot(target);
        if (root == "pyc_clk" || root == "pyc_rst")
          return error() << "reserved physical domain pin '" << root
                         << "' cannot be assigned in source";
        if (root.empty() || llvm::any_of(states, [&](const StateOwner &state) {
              return state.name == root;
            }))
          return error()
                 << "module-local assignment cannot write a persistent owner";
        std::optional<Type> expected;
        if (target.kind() == "Attribute") {
          if (indexedFieldTarget(target)) {
            auto type = expressionType(target, values, noInstances, decl.symbol);
            if (failed(type))
              return failure();
            expected = *type;
          } else {
            auto old = expression(target, at, decl.symbol, values, noInstances);
            if (failed(old))
              return failure();
            expected = old->getType();
          }
        }
        auto value =
            expression(rhs, at, decl.symbol, values, noInstances, expected);
        if (failed(value))
          return failure();
        if (failed(updateAssignment(target, *value, stmt, at, decl.symbol,
                                    values)))
          return failure();
        continue;
      }
    } else
      return error() << "behavioral module supports persistent declarations "
                        "and module-scope rule calls";
    if (queueResultNames.contains(assignmentRoot(target)))
      return error() << "queue result binding is immutable";
    if (target.kind() != "Name" || !initializer ||
        values.contains(target.string("id")) ||
        names.contains(target.string("id")) ||
        structs.contains(target.string("id")) ||
        behavioralRules.contains(target.string("id")))
      return error()
             << "persistent declaration requires a unique name and initializer";
    auto type = portType(annotation, decl.symbol);
    if (failed(type))
      return failure();
    ac::HardwareAnalysis analysis(*body);
    if (auto table = dyn_cast<ac::TableType>(*type)) {
      if (failed(analysis.getTableSize(table, {}, module)) ||
          failed(analysis.getPackedWidth(table.getElementType(), {}, module)))
        return failure();
    } else if (failed(analysis.getPackedWidth(*type, {}, module)))
      return failure();
    if (!staticInitializer(initializer))
      return error()
             << "persistent initializer must be a static complete value";
    auto tableType = dyn_cast<ac::TableType>(*type);
    auto encoded =
        initializer.kind() == "Constant"
            ? dyn_cast_or_null<DictionaryAttr>(initializer.get("value"))
            : DictionaryAttr();
    auto literalInteger =
        encoded ? encoded.getAs<StringAttr>("integer") : StringAttr();
    bool recursiveZero = literalInteger && literalInteger.getValue() == "0";
    bool elementImage = tableType && !recursiveZero;
    Type initType = elementImage ? tableType.getElementType() : *type;
    auto init = tableType && recursiveZero
                    ? constant(*type, 0, initializer, at, decl.symbol)
                    : expression(initializer, at, decl.symbol, values,
                                 noInstances, initType);
    if (failed(init))
      return failure();
    BindingBoundary initialBoundary{
        {target.string("id").str(), initType, annotation,
         annotationKind(annotation),
         fixedAnnotation(annotation) ||
             (tableType && isa<ac::BitsType>(initType))},
        true};
    auto convertedInit = applyBindingBoundary(*init, initialBoundary,
                                              initializer, at, decl.symbol);
    if (failed(convertedInit))
      return failure();
    init = *convertedInit;
    if (elementImage)
      init = createOp(at, stmt.location(b.getContext(), source.path),
                      ac::TableSplatOp::getOperationName(), {*init},
                      TypeRange{*type},
                      {at.getNamedAttr("shape", tableType.getShape())})
                 ->getResult(0);
    auto builtin = headers.lookupBuiltin("dffe");
    if (!builtin || !headers.isTrustedBuiltin(builtin.declaration))
      return error() << "trusted dffe unavailable";
    auto table = dyn_cast<ac::TableType>(*type);
    Type element = table ? table.getElementType() : *type;
    SmallVector<NamedAttribute> attrs{
        at.getNamedAttr("instance_name", at.getStringAttr(target.string("id"))),
        at.getNamedAttr("callee", builtin.symbol),
        at.getNamedAttr("parameters", at.getArrayAttr({})),
        at.getNamedAttr("type_arguments",
                        at.getArrayAttr({TypeAttr::get(element)})),
        at.getNamedAttr("occurrence", occurrence(at, decl.symbol, stmt))};
    if (table)
      attrs.push_back(at.getNamedAttr("shape", table.getShape()));
    auto storage = createOp(at, stmt.location(b.getContext(), source.path),
                            table ? ac::CollectionOp::getOperationName()
                                  : ac::InstanceOp::getOperationName(),
                            {}, TypeRange{*type}, attrs);
    states.push_back({target.string("id").str(), *type, *init, storage, stmt});
    values[target.string("id")] = storage->getResult(0);
    remember(storage->getResult(0), annotationKind(annotation));
    if (fixedAnnotation(annotation))
      fixedValues.insert(storage->getResult(0));
    auto held = constant(bits(1, stmt, decl.symbol), 0, stmt, at, decl.symbol);
    if (failed(held))
      return failure();
    bindState(states.back(), storage->getResult(0), *held, at, values);
  }
  for (const AstNode &registration : structuralRegistrations) {
    AstNode rule = structuralRules[registration.child("func").string("id")];
    RuleEvaluation evaluation;
    evaluation.structural = true;
    evaluation.instances = &structuralInstances;
    size_t checkCount = countSourceAssertions(rule, "body");
    size_t observationCount = 0;
    for (size_t i = 0; i < rule.array("body").size(); ++i) {
      auto stmt = rule.item("body", i);
      auto call = stmt.kind() == "Expr" ? stmt.child("value") : AstNode();
      auto function = call.kind() == "Call" ? call.child("func") : AstNode();
      if (call.kind() == "Call" &&
          (intrinsic(function, "log") || intrinsic(function, "report"))) {
        ++observationCount;
        continue;
      }
      if (function.kind() != "Name")
        continue;
      auto found = structuralInstances.find(function.string("id"));
      if (found == structuralInstances.end())
        continue;
      if (found->second->bound ||
          llvm::is_contained(evaluation.structuralTargets, found->second))
        return error() << "each structural instance requires one complete "
                          "binding";
      evaluation.structuralTargets.push_back(found->second);
    }
    if (evaluation.structuralTargets.empty() && !checkCount && !observationCount)
      return error() << "registered structural rule has no hardware behavior";
    SmallVector<Value> captures;
    for (const auto &item : values)
      captures.push_back(item.second);
    for (const Instance &item : structuralStorage)
      llvm::append_range(captures, item.op->getResults());
    SmallVector<Type> resultTypes;
    for (Instance *target : evaluation.structuralTargets)
      for (const Port &input : target->inputs)
        resultTypes.push_back(input.type);
    for (size_t i = 0; i < 2 * checkCount; ++i)
      resultTypes.push_back(bits(1, rule, decl.symbol));
    auto ruleOp = createOp(
        at, rule.location(b.getContext(), source.path),
        ac::RuleOp::getOperationName(), captures, resultTypes,
        {at.getNamedAttr("name", at.getStringAttr(rule.string("name"))),
         at.getNamedAttr("occurrence",
                         occurrence(at, decl.symbol, registration))},
        1);
    Block *block = new Block();
    ruleOp->getRegion(0).push_back(block);
    BranchEnvironment environment;
    unsigned ordinal = 0;
    auto captureArgument = [&]() {
      return captureValue(block, captures[ordinal++], ruleOp->getLoc());
    };
    for (const auto &item : values)
      environment.values[item.first()] = captureArgument();
    llvm::StringMap<SmallVector<Value>> capturedOutputs;
    for (const Instance &item : structuralStorage)
      for (const Port &port : item.outputs) {
        (void)port;
        capturedOutputs[item.name].push_back(captureArgument());
      }
    evaluation.outputs = &capturedOutputs;
    activeRule = &evaluation;
    OpBuilder inRule = OpBuilder::atBlockEnd(block);
    if (checkCount) {
      auto one = constant(bits(1, rule, decl.symbol), 1, rule, inRule,
                          decl.symbol);
      if (failed(one))
        return failure();
      environment.ambient = environment.continuation = *one;
    }
    llvm::StringSet<> noOwners;
    AstNode ruleReturn;
    if (failed(statements(rule, "body", inRule, decl.symbol, environment,
                          noOwners, ruleReturn)))
      return failure();
    size_t inputCount = 0;
    for (Instance *target : evaluation.structuralTargets)
      inputCount += target->inputs.size();
    if (evaluation.bindingValues.size() != inputCount ||
        evaluation.nextStructuralTarget !=
            evaluation.structuralTargets.size())
      return error() << "structural rule did not bind every inferred input";
    SmallVector<Value> yields = evaluation.bindingValues;
    for (const auto &check : evaluation.checks) {
      yields.push_back(check.condition);
      yields.push_back(check.path);
    }
    if (evaluation.checks.size() != checkCount)
      return error() << "source assertion capture count differs from rule "
                        "suffix";
    createOp(inRule, rule.location(b.getContext(), source.path),
             ac::YieldOp::getOperationName(), yields, {}, {});
    activeRule = nullptr;
    if (failed(publishSourceChecks(ruleOp, evaluation, at, decl.symbol)))
      return failure();
    unsigned result = 0;
    for (Instance *target : evaluation.structuralTargets) {
      target->op->setOperands(ruleOp->getResults().slice(
          result, target->inputs.size()));
      result += target->inputs.size();
      target->bound = true;
    }
    unsigned instrumentationOrdinal = 0;
    for (size_t i = 0; i < rule.array("body").size(); ++i) {
      auto stmt = rule.item("body", i);
      auto value = stmt.kind() == "Expr" ? stmt.child("value") : AstNode();
      StringRef callee =
          value.kind() == "Call" && value.child("func").kind() == "Name"
              ? value.child("func").string("id")
              : StringRef();
      if (value.kind() == "Call" && intrinsic(value.child("func"), "log"))
        callee = "log";
      if (value.kind() == "Call" && intrinsic(value.child("func"), "report"))
        callee = "report";
      if (stmt.kind() == "Assert") {
        ++instrumentationOrdinal;
        continue;
      }
      if (callee == "log" || callee == "report")
        if (failed(emitInstrumentation(decl, rule, stmt,
                                       instrumentationOrdinal++, at, values,
                                       structuralInstances)))
          return failure();
    }
  }
  for (const Instance &instance : structuralStorage)
    if (!instance.bound)
      return error() << "unbound instance '" << instance.name << "'";
  if (decl.system && returned && returned.child("value"))
    return error() << "@system cannot return a value";
  if (!returned && !decl.system)
    return error() << "behavioral module requires a return";
  SmallVector<Value> outputs;
  auto node = returned ? returned.child("value") : AstNode();
  if (decl.system) {
    outputs.clear();
  } else if (decl.returnShape == ReturnShape::Mapping && node.kind() == "Call" &&
      node.child("func").kind() == "Name" &&
      behavioralRules.contains(node.child("func").string("id"))) {
    auto result = behavioralCall(decl, node, at, values, states);
    if (failed(result))
      return failure();
    outputs = std::move(*result);
    if (outputs.size() != decl.outputs.size())
      return error() << "rule output arity disagrees with module";
    for (size_t i = 0; i < outputs.size(); ++i)
      if (!ac::areEquivalentHardwareTypes(outputs[i].getType(),
                                          decl.outputs[i].type))
        return error() << "rule output type disagrees with module";
  } else {
    auto result = lowerReturn(decl, node, at, values, noInstances);
    if (failed(result))
      return failure();
    outputs = std::move(*result);
  }
  if (failed(bindProposals(states, at, values, decl.symbol)))
    return failure();
  ownerProposals.clear();
  activeBehavioralDecl = nullptr;
  activeStates = nullptr;
  for (auto &site : directCallSites)
    if (!site.second.bound)
      return error() << "direct module call site was not bound";
  directCallSites.clear();
  structuralCallSites.clear();
  forwardCallBindings.clear();
  createOp(at, module->getLoc(), ac::YieldOp::getOperationName(), outputs, {},
           {});
  return success();
}
