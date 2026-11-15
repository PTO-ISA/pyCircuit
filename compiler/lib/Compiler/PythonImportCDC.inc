// CDC syntax allocates the existing canonical resources; connection inference
// records endpoint domains and leaves hardware legality to common analysis.
StringRef Importer::cdcKind(const AstNode &node) const {
  auto function = node.kind() == "Call" ? node.child("func") : node;
  auto base = function.kind() == "Subscript" ? function.child("value") : function;
  if (intrinsic(base, "AsyncReg")) return "async_reg";
  if (intrinsic(base, "AsyncFIFO")) return "async_fifo";
  return {};
}

LogicalResult Importer::declareCDC(const AstNode &assignment, ModuleDecl &decl,
                                    OpBuilder &at) {
  AstNode target = assignment.kind() == "AnnAssign" ? assignment.child("target")
      : assignment.array("targets").size() == 1 ? assignment.item("targets", 0) : AstNode();
  if (target.kind() != "Name") return error() << "CDC allocation requires one named resource";
  auto call = assignment.child("value");
  auto syntax = call ? call : assignment.child("annotation");
  auto function = syntax.kind() == "Call" ? syntax.child("func") : syntax;
  StringRef kind = cdcKind(syntax);
  bool fifo = kind == "async_fifo";
  if (kind.empty() || function.kind() != "Subscript")
    return error() << "CDC resource requires an explicit payload type";
  auto annotation = function.child("slice");
  AstNode payloadNode = annotation;
  ac::StaticExprAttr depth;
  if (fifo) {
    if (annotation.kind() != "Tuple" || annotation.array("elts").size() != 2)
      return error() << "AsyncFIFO requires payload type and static depth";
    payloadNode = annotation.item("elts", 0);
    auto expression = staticExpr(annotation.item("elts", 1), decl.symbol);
    if (failed(expression)) return failure();
    depth = *expression;
  }
  auto payload = portType(payloadNode, decl.symbol);
  if (failed(payload)) return failure();
  if (!fifo) {
    ac::HardwareAnalysis analysis(*body);
    ac::HardwareBindings bindings;
    bindings.owner = at.getInsertionBlock()->getParentOp();
    auto width = analysis.getPackedWidth(*payload, bindings, bindings.owner);
    if (!isa<ac::BitsType>(*payload) || failed(width) || *width != 1)
      return error() << "AsyncReg requires a one-bit payload";
  }
  if (call && !call.array("args").empty())
    return error() << "CDC endpoint configuration is keyword-only";
  llvm::StringSet<> seen;
  StringRef sourceKey = fifo ? "write" : "source", targetKey = fifo ? "read" : "target";
  SmallVector<NamedAttribute> attributes;
  StringAttr sourceDomain = at.getStringAttr("D"), targetDomain = sourceDomain;
  StringAttr sourceEdge = at.getStringAttr("posedge"), targetEdge = sourceEdge;
  StringAttr polarity = at.getStringAttr("high");
  for (size_t i = 0; i < call.array("keywords").size(); ++i) {
    auto keyword = call.item("keywords", i);
    StringRef name = keyword.string("arg");
    if (name.empty() || !seen.insert(name).second) return error() << "repeated CDC configuration argument";
    if (name == sourceKey || name == targetKey) {
      auto expression = staticExpr(keyword.child("value"), decl.symbol);
      auto domain = succeeded(expression) ? clockDomainName(*expression) : FailureOr<std::string>(failure());
      if (failed(domain)) return failure();
      (name == sourceKey ? sourceDomain : targetDomain) = at.getStringAttr(*domain);
    } else if (name == "reset") {
      continue;
    } else if (name == "reset_polarity" || name == (fifo ? "write_edge" : "source_edge") ||
               name == (fifo ? "read_edge" : "target_edge")) {
      auto value = keyword.child("value");
      auto text = value.kind() == "Constant" ? dyn_cast_or_null<StringAttr>(value.get("value")) : StringAttr();
      if (!text || (name == "reset_polarity" ? text.getValue() != "high" && text.getValue() != "low"
                                             : text.getValue() != "posedge" && text.getValue() != "negedge"))
        return error() << "CDC policy requires a supported literal string";
      if (name == "reset_polarity") polarity = text;
      else if (name == (fifo ? "write_edge" : "source_edge")) sourceEdge = text;
      else targetEdge = text;
    } else return error() << "unknown CDC configuration argument '" << name << "'";
  }
  auto builtin = headers.lookupBuiltin(kind);
  if (!builtin || !headers.isTrustedBuiltin(builtin.declaration))
    return error() << "CDC resource requires its canonical builtin";
  if (!SymbolTable::lookupSymbolIn(*body, builtin.symbol)) {
    auto *copy = builtin.declaration->clone();
    copy->setAttr("ac.declaration_role", at.getStringAttr("import_snapshot"));
    body->getBody()->push_back(copy);
  }
  auto control = internalBits(1, syntax, decl.symbol);
  SmallVector<Type> results{*payload};
  if (fifo) results.append({control, control});
  auto occurrence = sourceOccurrence(at, decl.symbol, syntax);
  attributes = {
      at.getNamedAttr("instance_name", at.getStringAttr(target.string("id"))),
      at.getNamedAttr("callee", builtin.symbol),
      at.getNamedAttr("parameters", at.getArrayAttr(fifo ? SmallVector<Attribute>{depth} : SmallVector<Attribute>{})),
      at.getNamedAttr("type_arguments", at.getArrayAttr(fifo ? SmallVector<Attribute>{TypeAttr::get(*payload)} : SmallVector<Attribute>{})),
      at.getNamedAttr("occurrence", occurrence),
      at.getNamedAttr("resource_contract", at.getI64IntegerAttr(1)),
      at.getNamedAttr(fifo ? "write_domain" : "source_domain", sourceDomain),
      at.getNamedAttr(fifo ? "read_domain" : "target_domain", targetDomain),
      at.getNamedAttr(fifo ? "write_edge" : "source_edge", sourceEdge),
      at.getNamedAttr(fifo ? "read_edge" : "target_edge", targetEdge),
      at.getNamedAttr("reset_polarity", polarity)};
  auto *allocation = createOp(at, syntax.location(b.getContext(), source.path),
      ac::InstanceOp::getOperationName(), {}, results, attributes);
  if (!seen.contains(sourceKey)) allocation->setAttr("source_infer_source", at.getUnitAttr());
  if (!seen.contains(targetKey)) allocation->setAttr("source_infer_target", at.getUnitAttr());
  cdcDeclarations[allocation] = assignment;
  directCallSites.try_emplace(occurrence, DirectCallSite{allocation});
  Port contract{target.string("id").str(), *payload, payloadNode,
                annotationKind(payloadNode), fixedAnnotation(payloadNode)};
  remember(allocation->getResult(0), contract.sourceKind);
  if (contract.fixedBits) fixedValues.insert(allocation->getResult(0));
  if (fifo) {
    auto constraint = sourceConstraint(contract);
    if (failed(constraint)) return failure();
    allocation->setAttr("ac.payload_constraint", *constraint);
    allocation->setAttr("ac.source_unbound_push", at.getBoolAttr(true));
    allocation->setAttr("ac.source_unbound_pop", at.getBoolAttr(true));
    fixedValues.insert(allocation->getResult(1));
    fixedValues.insert(allocation->getResult(2));
  }
  return success();
}

FailureOr<bool> Importer::bindCDCStatement(
    const AstNode &assignment, OpBuilder &at, FlatSymbolRefAttr symbol,
    llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances,
    SmallVectorImpl<Instance> &instanceStorage) {
  auto target = assignment.kind() == "AnnAssign" ? assignment.child("target")
      : assignment.kind() == "Assign" && assignment.array("targets").size() == 1
          ? assignment.item("targets", 0) : AstNode();
  if (target.kind() != "Name") return false;
  StringRef name = target.string("id");
  auto call = assignment.child("value");
  auto syntax = cdcKind(call).empty() ? assignment.child("annotation") : call;
  StringRef kind = cdcKind(syntax);
  if (kind.empty()) {
    auto existing = values.lookup(name);
    auto resource = existing ? existing.getDefiningOp<ac::InstanceOp>() : ac::InstanceOp();
    if (!resource || resource.getCalleeAttr().getValue() != "pycircuit.__builtins__.async_reg") return false;
    if (!call) return error() << "AsyncReg input assignment requires a value";
    if (!cdcInputs.insert(resource).second) return error() << "AsyncReg input requires one driver per allocation";
    auto input = expression(call, at, symbol, values, instances, resource.getResult(0).getType());
    if (failed(input)) return failure();
    resource->setOperand(3, *input);
    return true;
  }
  auto found = directCallSites.find(sourceOccurrence(at, symbol, syntax));
  if (found == directCallSites.end() || found->second.bound)
    return error() << "CDC declaration requires one predeclared allocation";
  auto *allocation = found->second.op;
  if (values.contains(name) || instances.contains(name)) return error() << "CDC resource cannot replace an existing binding";
  auto clock = [&](StringRef field) -> FailureOr<Value> {
    auto domain = allocation->getAttrOfType<StringAttr>(field);
    Value result = domain.getValue() == "D" ? values.lookup("pyc_clk")
        : values.lookup(("__pyc_clock_" + domain.getValue()).str());
    if (!result) return error() << "CDC endpoint clock is not declared in its owning Module";
    return result;
  };
  bool fifo = kind == "async_fifo";
  auto sourceClock = clock(fifo ? "write_domain" : "source_domain");
  auto targetClock = clock(fifo ? "read_domain" : "target_domain");
  if (failed(sourceClock) || failed(targetClock)) return failure();
  Value reset = values.lookup("pyc_rst");
  for (size_t i = 0; i < call.array("keywords").size(); ++i) {
    auto keyword = call.item("keywords", i);
    if (keyword.string("arg") != "reset") continue;
    auto actual = expression(keyword.child("value"), at, symbol, values, instances, (*sourceClock).getType());
    if (failed(actual)) return failure();
    reset = *actual;
  }
  if (!reset) return error() << "CDC allocation requires its owner reset or an explicit whole reset";
  auto payload = inactiveCarrier(allocation->getResult(0).getType(), syntax, at, symbol);
  if (failed(payload)) return failure();
  SmallVector<Value> operands{*sourceClock, *targetClock, reset, *payload};
  if (fifo) {
    auto zero = constant((*sourceClock).getType(), 0, syntax, at, symbol);
    if (failed(zero)) return failure();
    operands.append({*zero, *zero});
  }
  allocation->setOperands(operands);
  found->second.bound = true;
  if (!fifo) {
    values[name] = allocation->getResult(0);
    return true;
  }
  Instance instance;
  instance.name = name.str();
  instance.node = assignment;
  instance.callee = cast<ac::InstanceOp>(allocation).getCalleeAttr();
  instance.parameters.assign(cast<ac::InstanceOp>(allocation).getParameters().begin(), cast<ac::InstanceOp>(allocation).getParameters().end());
  instance.typeArguments.assign(cast<ac::InstanceOp>(allocation).getTypeArguments().begin(), cast<ac::InstanceOp>(allocation).getTypeArguments().end());
  auto allocationSyntax = syntax.kind() == "Call" ? syntax.child("func") : syntax;
  auto payloadNode = allocationSyntax.child("slice").item("elts", 0);
  Port payloadContract{name.str(), allocation->getResult(0).getType(), payloadNode,
                       annotationKind(payloadNode), fixedAnnotation(payloadNode)};
  auto bit = allocation->getResult(1).getType();
  instance.inputs = {{"write_clock", bit, syntax, std::nullopt, true},
                     {"read_clock", bit, syntax, std::nullopt, true},
                     {"whole_reset", bit, syntax, std::nullopt, true},
                     payloadContract,
                     {"push_intent", bit, syntax, std::nullopt, true},
                     {"pop_intent", bit, syntax, std::nullopt, true}};
  // Native Queue call contracts use logical ready/valid/payload ordering;
  // the physical allocation retains the canonical CDC result ordering.
  instance.outputs = {{"ready", bit, syntax, std::nullopt, true},
                      {"valid", bit, syntax, std::nullopt, true}, payloadContract};
  instance.op = allocation;
  instance.bound = true;
  instance.valueHandle = true;
  instanceStorage.push_back(std::move(instance));
  instances[name] = &instanceStorage.back();
  return true;
}

LogicalResult Importer::finishCDCResources(OpBuilder &at, FlatSymbolRefAttr,
                                           llvm::StringMap<Value> &) {
  for (const auto &[resource, declaration] : cdcDeclarations)
    if (resource->getParentOp() == at.getInsertionBlock()->getParentOp() &&
        !ac::isAsyncQueueInstance(cast<ac::InstanceOp>(resource)) && !cdcInputs.contains(resource))
      return error() << "AsyncReg declaration requires its input connection";
  return success();
}
