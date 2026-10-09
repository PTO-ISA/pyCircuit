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
    if (binding.contract.sourceKind != declared.contract.sourceKind ||
        binding.contract.fixedBits != declared.contract.fixedBits)
      return error() << "rule parameter annotation disagrees with binding "
                        "source kind";
    if (!sameBindingContract(binding, declared))
      return error() << "rule parameter type mismatch";
    if (!ownerActual)
      binding = declared;
  }
  auto converted = applyBindingBoundary(value, binding, actual, at, symbol);
  if (failed(converted))
    return failure();
  return BoundRuleFormal{name.str(), *converted, binding};
}

FailureOr<SmallVector<BoundRuleFormal>>
Importer::bindPureActuals(const AstNode &rule, const AstNode &call,
                          OpBuilder &at, FlatSymbolRefAttr symbol,
                          llvm::StringMap<Value> &values,
                          llvm::StringMap<Instance *> &instances) {
  auto args = rule.child("args");
  if (failed(validateRuleParameters(rule)))
    return failure();
  if (!call.array("keywords").empty() ||
      call.array("args").size() != args.array("args").size())
    return mlir::emitError(call.location(b.getContext(), source.path))
           << "scalar helper requires its exact positional argument list";
  SmallVector<Value> actuals;
  SmallVector<std::string> names;
  SmallVector<BoundRuleFormal> bound;
  for (size_t i = 0; i < args.array("args").size(); ++i) {
    auto formal = args.item("args", i);
    if (failed(validateRuleFormalName(formal, names)))
      return failure();
    names.push_back(formal.string("arg").str());
    // The list, rather than a binary operator's internal operand order, owns
    // once-only source-order actual evaluation. No formal width hint is given.
    auto value =
        expression(call.item("args", i), at, symbol, values, instances);
    if (failed(value))
      return failure();
    actuals.push_back(*value);
    auto actual = call.item("args", i);
    auto binding = inferredBoundary(names[i], actuals[i], actual, true);
    auto checked =
        bindRuleFormal(formal, actual, actuals[i], binding, false, at, symbol);
    if (failed(checked))
      return failure();
    bound.push_back(std::move(*checked));
  }
  return bound;
}

FailureOr<Value> Importer::lowerPureBody(const AstNode &rule,
                                         ArrayRef<BoundRuleFormal> formals,
                                         OpBuilder &at,
                                         FlatSymbolRefAttr symbol) {
  auto &context = *activePureScalar;
  auto diagnostic = [&] {
    return mlir::emitError(rule.location(b.getContext(), source.path));
  };
  if (++context.calls > context.summary.calls ||
      !pureCheckedAdd(context.formals, formals.size()) ||
      context.formals > context.summary.formals)
    return diagnostic() << "scalar helper binding exceeds its source envelope";
  if (llvm::any_of(context.bodies, [&](const AstNode &ancestor) {
        return samePureDeclaration(ancestor, rule);
      }))
    return diagnostic() << "scalar helper body recursion is unsupported";
  // Actuals are already evaluated and exactly bound. Declaration identity
  // tracks only active bodies, independently of the shared nesting guards.
  context.bodies.push_back(rule);
  context.peakBodies = std::max(context.peakBodies,
                                static_cast<uint64_t>(context.bodies.size()));
  auto pop = llvm::scope_exit([&] { context.bodies.pop_back(); });
  // Preflight established the final Return on this immutable declaration.
  // Do not rescan its potentially long Pass/docstring prefix while typing.
  auto bodyStatements = rule.array("body");
  if (bodyStatements.empty())
    return diagnostic() << "scalar helper is missing its preflighted return";
  auto returned = rule.item("body", bodyStatements.size() - 1);
  if (returned.kind() != "Return" || !returned.child("value"))
    return diagnostic() << "scalar helper is missing its preflighted return";
  auto type = portType(rule.child("returns"), symbol);
  if (failed(type))
    return failure();
  llvm::StringMap<Value> values;
  llvm::StringMap<Instance *> noInstances;
  for (const auto &formal : formals)
    values[formal.name] = formal.value;
  if (values.size() != formals.size() ||
      context.peakFrameSlots > context.summary.slots[4])
    return diagnostic() << "scalar helper formal frame exceeds its reservation";
  auto value = expression(returned.child("value"), at, symbol, values,
                          noInstances, *type);
  if (failed(value))
    return failure();
  // The existing unsigned boundary owns fit, widening and fresh fixed facts.
  return unsignedBoundary(*value, *type, returned.child("value"), at, symbol);
}

FailureOr<Value>
Importer::pureRuleCall(const AstNode &call, OpBuilder &at,
                       FlatSymbolRefAttr symbol, llvm::StringMap<Value> &values,
                       llvm::StringMap<Instance *> &instances) {
  auto diagnostic = [&] {
    return mlir::emitError(call.location(b.getContext(), source.path));
  };
  if (!queryArguments.empty())
    return diagnostic() << "scalar helpers are unsupported in Table callbacks";
  // This guard owns the entire call, including root actuals and all cleanup.
  // Inside an expanded body it is the single measured call-descent guard;
  // there is no second reservation or temporary wrapper for that call.
  std::string reason;
  if (!queryBudget.enter(reason))
    return diagnostic() << reason;
  auto leave = llvm::scope_exit([&] { queryBudget.leave(); });
  auto declaration = pureRuleDeclaration(call);
  if (failed(declaration))
    return failure();
  if (activePureScalar) {
    auto &context = *activePureScalar;
    if (queryBudget.used(TableQueryResource::Nesting) > context.summary.depth)
      return diagnostic() << "scalar helper call exceeds its source envelope";
    uint64_t slots = 6 * declaration->child("args").array("args").size();
    context.frameSlots += slots;
    context.peakFrameSlots =
        std::max(context.peakFrameSlots, context.frameSlots);
    auto frame = llvm::scope_exit([&] { context.frameSlots -= slots; });
    if (context.peakFrameSlots > context.summary.slots[4])
      return diagnostic()
             << "scalar helper actual frame exceeds its reservation";
    auto formals =
        bindPureActuals(*declaration, call, at, symbol, values, instances);
    if (failed(formals))
      return failure();
    return lowerPureBody(*declaration, *formals, at, symbol);
  }
  PureScalarSummary summary;
  summary.arity = call.array("args").size();
  {
    SmallVector<AstNode> bodies;
    if (failed(preflightPureCall(call, {}, summary, bodies, false)))
      return failure();
  }
  if (failed(reservePureScalar(summary, call)))
    return failure();
  // Keep the caller's lexical/effect context until every actual has been
  // evaluated and exact B0 binding has succeeded. Root-actual helpers each
  // own their independent envelope while this lifecycle guard remains active.
  auto formals =
      bindPureActuals(*declaration, call, at, symbol, values, instances);
  if (failed(formals))
    return failure();
  ac::HardwareAnalysis analysis(*body);
  for (const auto &formal : *formals) {
    auto width = analysis.getPackedWidth(formal.value.getType(), {},
                                         at.getInsertionBlock()->getParentOp());
    auto fact = numericValues.find(formal.value);
    auto bounded = [&](const llvm::APSInt &integer) {
      return integer.getBitWidth() / 64 + (integer.getBitWidth() % 64 != 0) <=
                 summary.words &&
             (integer.isSigned() ? integer.getSignificantBits()
                                 : integer.getActiveBits()) <= summary.height;
    };
    if (failed(width) || *width > summary.width)
      return diagnostic() << "scalar helper capture exceeds its fact envelope";
    if (fact == numericValues.end())
      continue;
    const auto &info = fact->second;
    if (info.interval &&
        (!bounded(info.interval->lower) || !bounded(info.interval->upper)))
      return diagnostic() << "scalar helper capture exceeds its fact envelope";
    if (auto closed =
            dyn_cast_or_null<ac::MathIntAttr>(info.closedSourceConstant)) {
      auto decoded = pureAuditInteger(closed, summary);
      if (!decoded || !bounded(*decoded))
        return diagnostic()
               << "scalar helper capture exceeds its fact envelope";
    }
  }
  auto loc = call.location(b.getContext(), source.path);
  // Existing ac.rule staging pattern: this holder is type-only, with no
  // registered rule contract, occurrence plan, writes, instances or effects.
  auto temporary =
      createOp(at, loc, ac::RuleOp::getOperationName(), {}, {}, {}, 1);
  auto block = new Block();
  temporary->getRegion(0).push_back(block);
  Operation *firstClone = nullptr;
  uint64_t cloneCount = 0;
  bool publishClone = false;
  auto cleanup = llvm::scope_exit([&] {
    if (!publishClone) {
      Operation *operation = firstClone;
      for (uint64_t i = 1; i < cloneCount; ++i)
        operation = operation->getNextNode();
      for (uint64_t i = 0; i < cloneCount; ++i) {
        Operation *previous = operation->getPrevNode();
        for (Value value : operation->getResults()) {
          numericValues.erase(value);
          fixedValues.erase(value);
          arithmeticValues.erase(value);
        }
        operation->erase();
        operation = previous;
      }
    }
    for (Value value : block->getArguments()) {
      numericValues.erase(value);
      fixedValues.erase(value);
      arithmeticValues.erase(value);
    }
    for (Operation &operation : *block)
      for (Value value : operation.getResults()) {
        numericValues.erase(value);
        fixedValues.erase(value);
        arithmeticValues.erase(value);
      }
    temporary->erase();
  });
  // Map capture arguments directly to the accepted original Values; the formal
  // records then hold the captured Values used by the formal-only body frame.
  IRMapping mapping;
  for (auto &formal : *formals) {
    Value original = formal.value;
    formal.value = captureValue(block, original, loc);
    mapping.map(formal.value, original);
  }
  PureScalarContext context{summary};
  context.frameSlots = context.peakFrameSlots = 6 * summary.arity;
  auto previousPure = std::exchange(activePureScalar, &context);
  auto previousRule = std::exchange(activeRule, nullptr);
  auto previousDecl = std::exchange(activeBehavioralDecl, nullptr);
  auto previousStates = std::exchange(activeStates, nullptr);
  auto restore = llvm::scope_exit([&] {
    activeStates = previousStates;
    activeBehavioralDecl = previousDecl;
    activeRule = previousRule;
    activePureScalar = previousPure;
  });
  auto inside = OpBuilder::atBlockEnd(block);
  auto result = lowerPureBody(*declaration, *formals, inside, symbol);
  if (failed(result))
    return failure();
  createOp(inside, loc, ac::YieldOp::getOperationName(), {*result}, {}, {});
  if (failed(auditPureScalar(*block, context, call, symbol,
                             mapping.getValueMap().size())))
    return failure();
  for (Operation &operation : *block) {
    if (isa<ac::YieldOp>(operation))
      break;
    SmallVector<Value, 3> operands;
    for (Value value : operation.getOperands()) {
      if (!mapping.contains(value))
        return diagnostic() << "scalar helper clone has an unmapped input";
      operands.push_back(mapping.lookup(value));
    }
    // Flat scalar cloning uses only the Value map. Generic recursive clone
    // would also retain operation/block maps that are outside this envelope.
    auto copy =
        createOp(at, operation.getLoc(), operation.getName().getStringRef(),
                 operands, operation.getResultTypes(), operation.getAttrs());
    if (!firstClone)
      firstClone = copy;
    ++cloneCount;
    Value old = operation.getResult(0), value = copy->getResult(0);
    mapping.map(old, value);
    auto info = valueInfo(old);
    info.value = value;
    numericValues[value] = std::move(info);
    if (fixedValues.contains(old))
      fixedValues.insert(value);
    if (arithmeticValues.contains(old))
      arithmeticValues.insert(value);
  }
  if (failed(auditPureScalar(*block, context, call, symbol,
                             mapping.getValueMap().size(),
                             at.getInsertionBlock(), firstClone)))
    return failure();
  if (!mapping.contains(*result))
    return diagnostic() << "scalar helper result is missing its scalar clone";
  publishClone = true;
  return mapping.lookup(*result);
}

FailureOr<SmallVector<Value>>
Importer::behavioralCall(ModuleDecl &decl, const AstNode &call, OpBuilder &at,
                         llvm::StringMap<Value> &values,
                         ArrayRef<StateOwner> states, bool statementCall) {
  llvm::StringMap<Instance *> noInstances;
  auto rule = behavioralRules[call.child("func").string("id")];
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
    auto resultBinding = lookupBinding(annotation);
    if (resultBinding && resultBinding->category == BindingCategory::Struct) {
      auto type = portType(annotation, decl.symbol);
      if (failed(type))
        return failure();
      contract.returnShape = ReturnShape::Struct;
      contract.outputs = {{"result", *type, annotation, {}, false}};
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
  auto args = rule.child("args");
  if (failed(validateRuleParameters(rule)))
    return failure();
  if (call.array("args").size() != args.array("args").size() ||
      !call.array("keywords").empty())
    return error() << "behavioral rule call argument arity mismatch";
  const auto *plan = ruleWrites.lookup(decl.node, call);
  if (!plan)
    return error() << "registered rule call is missing its source write analysis";
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
    auto actual = call.item("args", i);
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
        binding.contract.name = name.str();
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
      if (llvm::any_of(ArrayRef<SourceWriteSelector>(path.selectors).drop_front(),
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
      auto maskType = ac::TableType::get(
          b.getContext(), table.getShape(), bits(1, rule, decl.symbol));
      evaluation.domains.push_back(
          {item.first, pathIndex, item.second->formal, path.assignment,
           path.selectors.front().occurrence, maskType, {}, {}});
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
       at.getNamedAttr("occurrence", occurrence(at, decl.symbol, call))},
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
  if (!grantProof->chargeProduct(writtenOwners.size(), evaluation.domains.size()))
    return error() << grantProof->failureReason();
  for (auto [ordinal, item] : llvm::enumerate(writtenOwners)) {
    OwnerProposal proposal{
        item.first, ruleOp->getResult(contract.outputs.size() + ordinal * 2),
        ruleOp->getResult(contract.outputs.size() + ordinal * 2 + 1),
        item.second, plan, {}};
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
  return results;
}
