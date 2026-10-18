LogicalResult Importer::validateRuleParameters(const AstNode &rule) {
  auto args = rule.child("args");
  if (!args.array("posonlyargs").empty() || !args.array("kwonlyargs").empty() ||
      !args.array("defaults").empty() || args.child("vararg") ||
      args.child("kwarg"))
    return error() << "behavioral rule requires ordinary required parameters";
  return success();
}

LogicalResult
Importer::validateRuleFormalName(const AstNode &formal,
                                 ArrayRef<std::string> boundNames) {
  StringRef name = formal.string("arg");
  if (name == "pyc_clk" || name == "pyc_rst")
    return error() << "reserved physical domain pin '" << name
                   << "' cannot be a rule parameter";
  if (names.contains(name) || structs.contains(name) ||
      behavioralRules.contains(name))
    return error()
           << "rule parameter shadows a source declaration or namespace";
  if (llvm::is_contained(boundNames, name.str()))
    return error() << "duplicate rule parameter";
  return success();
}

FailureOr<BoundRuleFormal>
Importer::bindRuleFormal(const AstNode &formal, const AstNode &actual,
                         Value value, BindingBoundary binding, bool ownerActual,
                         OpBuilder &at, FlatSymbolRefAttr symbol) {
  StringRef name = formal.string("arg");
  if (formal.child("annotation")) {
    auto annotation = formal.child("annotation");
    auto type = portType(annotation, symbol);
    if (failed(type))
      return failure();
    BindingBoundary declared{{name.str(), *type, annotation,
                              annotationKind(annotation),
                              fixedAnnotation(annotation)},
                             true};
    if (binding.contract().sourceKind != declared.contract().sourceKind ||
        binding.contract().fixedBits != declared.contract().fixedBits)
      return mlir::emitError(actual.location(b.getContext(), source.path))
             << "rule parameter annotation disagrees with binding source kind";
    if (!sameBindingContract(binding, declared))
      return mlir::emitError(actual.location(b.getContext(), source.path))
             << "rule parameter type mismatch";
    if (!ownerActual)
      binding = declared;
  }
  auto converted = applyBindingBoundary(value, binding, actual, at, symbol);
  if (failed(converted))
    return failure();
  return BoundRuleFormal{name.str(), *converted, binding};
}

FailureOr<SmallVector<Value>>
Importer::behavioralCall(ModuleDecl &decl, const AstNode &call, OpBuilder &at,
                         llvm::StringMap<Value> &values,
                         ArrayRef<StateOwner> states, bool statementCall, bool tupleCall) {
  llvm::StringMap<Instance *> noInstances;
  auto rule = behavioralRules[call.child("func").string("id")];
  auto registration = checkedRuleRegistration(call, rule);
  if (failed(registration)) return failure();
  const auto *plan = *registration;
  ModuleDecl contract = decl;
  auto noValue = [](const AstNode &node) {
    return !node || (node.kind() == "Constant" &&
                     isa_and_nonnull<UnitAttr>(node.get("value")));
  };
  auto annotation = rule.child("returns");
  bool explicitVoid = annotation && noValue(annotation);
  bool valueReturn = false;
  for (size_t i = 0; i < rule.array("body").size(); ++i) {
    auto statement = rule.item("body", i);
    if (statement.kind() == "Return" && !noValue(statement.child("value")))
      valueReturn = true;
  }
  bool voidResult = explicitVoid || (!annotation && !valueReturn);
  if (voidResult) {
    contract.outputs.clear();
    if (!statementCall)
      return error() << "void rule call cannot be used as a value";
  } else if (annotation) {
    if (annotation.kind() != "Dict") {
      llvm::StringSet<> resultNames;
      for (const Port &input : decl.inputs) resultNames.insert(input.name);
      if (failed(readReturnContract(contract, annotation, resultNames, true)))
        return failure();
    } else {
      if (decl.returnShape != ReturnShape::Mapping ||
          annotation.kind() != "Dict" ||
          annotation.array("keys").size() != decl.outputs.size() ||
          annotation.array("values").size() != decl.outputs.size())
        return error() << "rule return annotation requires matching named "
                          "mapping or nominal struct";
      for (size_t i = 0; i < decl.outputs.size(); ++i) {
        auto key = dyn_cast_or_null<StringAttr>(
            annotation.item("keys", i).get("value"));
        auto type = portType(annotation.item("values", i), decl.symbol);
        if (!key || key.getValue() != decl.outputs[i].name || failed(type) ||
            !ac::areEquivalentHardwareTypes(*type, decl.outputs[i].type))
          return error()
                 << "rule return annotation disagrees with module outputs";
      }
    }
  }
  if (contract.returnShape == ReturnShape::Tuple && !statementCall && !tupleCall)
    return error() << "tuple Rule call requires direct Tuple unpacking";
  auto args = rule.child("args");
  if (failed(validateRuleParameters(rule)))
    return failure();
  SmallVector<StringRef> formalNames;
  for (size_t i = 0; i < args.array("args").size(); ++i)
    formalNames.push_back(args.item("args", i).string("arg"));
  auto actuals = normalizeCallArguments(call, formalNames, error,
                                        "behavioral rule call argument arity mismatch");
  if (failed(actuals)) return failure();
  llvm::StringMap<StringRef> fixedAssigned;
  auto classify = [&](auto &&self, AstNode node,
                      StringRef field) -> LogicalResult {
    for (size_t i = 0; i < node.array(field).size(); ++i) {
      auto stmt = node.item(field, i);
      auto fixedTargets = fixedResultTargets(stmt);
      if (failed(fixedTargets))
        return failure();
      if (*fixedTargets)
        for (StringRef name : **fixedTargets)
          fixedAssigned[name] = fixedResultCall(stmt.child("value"));
      if (stmt.kind() == "If") {
        if (failed(self(self, stmt, "body")) ||
            failed(self(self, stmt, "orelse")))
          return failure();
      } else if (stmt.kind() == "Match") {
        auto arms = readMatchSyntax(stmt, [&](const AstNode &site) {
          return mlir::emitError(site.location(b.getContext(), source.path));
        });
        if (failed(arms))
          return failure();
        for (const auto &arm : *arms)
          if (failed(self(self, arm.originalSite, "body")))
            return failure();
      }
    }
    return success();
  };
  if (failed(classify(classify, rule, "body")))
    return failure();
  SmallVector<Value> captures;
  SmallVector<std::string> formals;
  SmallVector<BindingBoundary> formalBoundaries;
  llvm::StringMap<size_t> stateForFormal;
  SmallVector<std::pair<size_t, const SourceOwnerWrite *>> writtenOwners;
  for (const auto &write : plan->writes) {
    bool found = false;
    for (auto [index, state] : llvm::enumerate(states))
      if (state.node.value == write.owner.value &&
          state.node.path.size() == write.owner.path.size() &&
          llvm::all_of(llvm::zip(state.node.path, write.owner.path),
                       [](const auto &steps) {
                         return std::get<0>(steps).field ==
                                    std::get<1>(steps).field &&
                                std::get<0>(steps).index ==
                                    std::get<1>(steps).index;
                       })) {
        stateForFormal[write.formal] = index;
        writtenOwners.emplace_back(index, &write);
        found = true;
        break;
      }
    if (!found)
      return error() << "analyzed rule writer has no declared storage owner";
  }
  llvm::sort(writtenOwners,
             [](const auto &a, const auto &c) { return a.first < c.first; });
  for (size_t i = 0; i < args.array("args").size(); ++i) {
    auto formal = args.item("args", i);
    auto actual = (*actuals)[i];
    StringRef name = formal.string("arg");
    if (failed(validateRuleFormalName(formal, formals)))
      return failure();
    auto value = expression(actual, at, decl.symbol, values, noInstances);
    if (failed(value))
      return failure();
    auto binding = inferredBoundary(name, *value, actual, true);
    bool ownerActual = false;
    for (const auto &state : states)
      if (actual.kind() == "Name" && actual.string("id") == state.name) {
        if (fixedAssigned.contains(name))
          return mlir::emitError(formal.location(b.getContext(), source.path))
                 << fixedAssigned.lookup(name)
                 << " target cannot write persistent owner formal '" << name
                 << "'";
        binding = ownerBoundary(state);
        binding.unsharedContract().name = name.str();
        ownerActual = true;
      }
    auto bound = bindRuleFormal(formal, actual, *value, binding, ownerActual,
                                at, decl.symbol);
    if (failed(bound))
      return failure();
    formals.push_back(bound->name);
    formalBoundaries.push_back(bound->boundary);
    captures.push_back(bound->value);
  }
  // Directly nested Rules retain ordinary lexical wire captures in addition to
  // their explicit owner/formal bindings. This uses the existing Rule operand
  // and region argument convention, without changing any state ownership.
  SmallVector<std::pair<std::string, Value>> lexicalValues;
  bool lexicalRule = llvm::any_of(decl.node.array("body"), [&](Attribute raw) { return raw == rule.value; });
  if (lexicalRule)
    for (const auto &entry : values)
      if (!llvm::is_contained(formals, entry.first().str())) {
        lexicalValues.push_back({entry.first().str(), entry.second});
        captures.push_back(entry.second);
      }
  if (lexicalRule)
    for (const auto &entry : valueHelpers) {
      if (entry.second->lexicalOwner != decl.symbol) continue;
      for (const auto &capture : entry.second->captures)
        if (!llvm::is_contained(captures, capture.second)) {
          lexicalValues.push_back({{}, capture.second});
          captures.push_back(capture.second);
        }
    }
  SmallVector<Type> resultTypes;
  for (auto &port : contract.outputs)
    resultTypes.push_back(port.type);
  for (const auto &item : writtenOwners) {
    const auto &state = states[item.first];
    resultTypes.push_back(state.type);
    resultTypes.push_back(bits(1, state.node, decl.symbol));
  }
  RuleEvaluation evaluation;
  ac::HardwareAnalysis addressAnalysis(*body);
  for (const auto &item : writtenOwners) {
    const auto &state = states[item.first];
    for (auto [pathIndex, path] : llvm::enumerate(item.second->paths)) {
      if (!grantProof->charge(1 + path.selectors.size()))
        return error() << grantProof->failureReason();
      if (path.selectors.empty() ||
          path.selectors.front().kind != SourceWriteSelector::Kind::Index)
        continue;
      auto table = dyn_cast<ac::TableType>(state.type);
      if (!table || table.getShape().size() != 1)
        return error() << "address witness requires a one-dimensional Table";
      if (llvm::any_of(
              ArrayRef<SourceWriteSelector>(path.selectors).drop_front(),
              [](const auto &selector) {
                return selector.kind != SourceWriteSelector::Kind::Field;
              }))
        return error() << "address witness admits only one Table index";
      auto size = addressAnalysis.getTableSize(table, {}, state.op);
      if (failed(size))
        return failure();
      // Precharge the mask's complete scalar storage and private result slots.
      if (!grantProof->charge(*size) || !grantProof->charge(4))
        return error() << grantProof->failureReason();
      auto maskType = ac::TableType::get(b.getContext(), table.getShape(),
                                         bits(1, rule, decl.symbol));
      evaluation.domains.push_back({item.first,
                                    pathIndex,
                                    item.second->formal,
                                    path.assignment,
                                    path.selectors.front().occurrence,
                                    maskType,
                                    {},
                                    {}});
      resultTypes.push_back(maskType);
    }
  }
  size_t checkCount = countSourceChecks(rule, "body");
  for (size_t i = 0; i < checkCount * 2; ++i)
    resultTypes.push_back(bits(1, rule, decl.symbol));
  auto ruleOp = createOp(
      at, rule.location(b.getContext(), source.path),
      ac::RuleOp::getOperationName(), captures, resultTypes,
      {at.getNamedAttr("name", at.getStringAttr(rule.string("name"))),
       at.getNamedAttr("occurrence", sourceOccurrence(at, decl.symbol, call))},
      1);
  auto block = new Block();
  ruleOp->getRegion(0).push_back(block);
  BranchEnvironment environment;
  auto &localValues = environment.values;
  auto &writes = environment.writes;
  llvm::StringSet<> owners;
  for (auto [index, name] : llvm::enumerate(formals)) {
    localValues[name] = captureValue(block, captures[index], ruleOp->getLoc());
    environment.boundaries[name] = formalBoundaries[index];
  }
  for (auto [index, entry] : llvm::enumerate(lexicalValues)) {
    auto value = captureValue(block, captures[formals.size() + index], ruleOp->getLoc());
    if (!entry.first.empty()) {
      localValues[entry.first] = value;
      environment.boundaries[entry.first] = inferredBoundary(entry.first, value, rule);
    }
  }
  auto nested = OpBuilder::atBlockEnd(block);
  {
    auto previousRule = std::exchange(activeRule, &evaluation);
    auto restoreRule = llvm::scope_exit([&] { activeRule = previousRule; });
    if (checkCount) {
      auto one =
          constant(bits(1, rule, decl.symbol), 1, rule, nested, decl.symbol);
      if (failed(one))
        return failure();
      environment.ambient = environment.continuation = *one;
    }
    for (auto &item : stateForFormal) {
      owners.insert(item.first());
      auto zero =
          constant(bits(1, rule, decl.symbol), 0, rule, nested, decl.symbol);
      if (failed(zero))
        return failure();
      writes[item.first()] = *zero;
    }
    AstNode ruleReturn;
    if (failed(statements(rule, "body", nested, decl.symbol, environment,
                          owners, ruleReturn)))
      return failure();
    SmallVector<Value> yields;
    if (voidResult) {
      if (!noValue(ruleReturn.child("value")))
        return error() << "void rule must return no value";
    } else {
      auto outputs = lowerReturn(contract, ruleReturn.child("value"), nested,
                                 localValues, noInstances);
      if (failed(outputs))
        return failure();
      yields = std::move(*outputs);
    }
    for (const auto &item : writtenOwners) {
      const auto &state = states[item.first];
      const auto &formal = item.second->formal;
      Value next = localValues[formal];
      Value en = writes[formal];
      auto checkedNext = applyBindingBoundary(next, ownerBoundary(state),
                                              state.node, nested, decl.symbol);
      BindingBoundary enableBoundary{
          {"", bits(1, rule, decl.symbol), rule, {}, true}, true};
      auto checkedEnable =
          applyBindingBoundary(en, enableBoundary, rule, nested, decl.symbol);
      if (failed(checkedNext) || failed(checkedEnable))
        return failure();
      yields.push_back(*checkedNext);
      yields.push_back(*checkedEnable);
    }
    for (const auto &witness : evaluation.domains) {
      if (!witness.index || !witness.mask ||
          witness.mask.getType() != witness.maskType)
        return error() << "missing or incorrectly typed assignment-time "
                          "address witness";
      yields.push_back(witness.mask);
    }
    for (const auto &check : evaluation.checks) {
      yields.push_back(check.condition);
      yields.push_back(check.path);
    }
    if (evaluation.checks.size() != checkCount)
      return error()
             << "source assertion capture count differs from rule suffix";
    createOp(nested, ruleOp->getLoc(), ac::YieldOp::getOperationName(), yields,
             {}, {});
  }
  if (failed(publishSourceChecks(ruleOp, evaluation, at, decl.symbol)))
    return failure();
  size_t domainBase = contract.outputs.size() + writtenOwners.size() * 2;
  if (!grantProof->chargeProduct(writtenOwners.size(),
                                 evaluation.domains.size()))
    return error() << grantProof->failureReason();
  for (auto [ordinal, item] : llvm::enumerate(writtenOwners)) {
    OwnerProposal proposal{
        item.first,
        ruleOp->getResult(contract.outputs.size() + ordinal * 2),
        ruleOp->getResult(contract.outputs.size() + ordinal * 2 + 1),
        item.second,
        plan,
        {}};
    for (auto [slot, witness] : llvm::enumerate(evaluation.domains))
      if (witness.ownerIndex == item.first) {
        if (!grantProof->charge())
          return error() << grantProof->failureReason();
        proposal.domains.push_back(witness);
        proposal.domains.back().mask = ruleOp->getResult(domainBase + slot);
      }
    ownerProposals.push_back(std::move(proposal));
  }
  SmallVector<Value> results(
      ruleOp->getResults().take_front(contract.outputs.size()));
  // The Rule's completed return boundary publishes the same source authority
  // as a typed Module result. Registration remains a Rule even with no effects.
  for (auto [result, port] : llvm::zip(results, contract.outputs)) {
    remember(result, port.sourceKind);
    if (port.fixedBits) fixedValues.insert(result);
  }
  return results;
}
