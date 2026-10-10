// Declaration-only source inference. Runtime expressions are not evaluated
// here.
StringRef Importer::memoryReference(const AstNode &node) const {
  auto binding = lookupBinding(node);
  if (!binding)
    return {};
  for (StringRef kind : {StringRef("sync_mem"), StringRef("sync_mem_dp"),
                         StringRef("byte_mem")}) {
    auto builtin = headers.lookupBuiltin(kind);
    if (!builtin || !headers.isTrustedBuiltin(builtin.declaration))
      continue;
    if ((binding->category == BindingCategory::Module &&
         binding->symbol == builtin.symbol) ||
        (binding->category == BindingCategory::Marker &&
         binding->importedModule == "pycircuit" && binding->remote == kind))
      return kind;
  }
  return {};
}
StringRef Importer::memoryKind(const AstNode &call) const {
  return call.kind() == "Call" && call.child("func").kind() == "Subscript"
             ? memoryReference(call.child("func").child("value"))
             : StringRef();
}
llvm::StringMap<unsigned>
Importer::sourceNameWrites(const AstNode &module) const {
  llvm::StringMap<unsigned> result;
  auto target = [&](auto &&self, const AstNode &node) -> void {
    if (auto root = assignmentRoot(node); !root.empty())
      ++result[root];
    else
      for (size_t i = 0; i < node.array("elts").size(); ++i)
        self(self, node.item("elts", i));
  };
  auto walk = [&](auto &&self, const AstNode &node) -> void {
    if (node.kind() == "FunctionDef" || node.kind() == "ClassDef")
      return;
    if (node.kind() == "Assign")
      for (size_t i = 0; i < node.array("targets").size(); ++i)
        target(target, node.item("targets", i));
    else if (node.kind() == "AnnAssign" || node.kind() == "AugAssign" ||
             node.kind() == "NamedExpr" || node.kind() == "For")
      target(target, node.child("target"));
    for (NamedAttribute field : node.fields()) {
      auto name = field.getName().getValue();
      if (auto child = dyn_cast<DictionaryAttr>(field.getValue())) {
        if (child.getAs<StringAttr>("kind"))
          self(self, node.child(name));
      } else if (auto children = dyn_cast<ArrayAttr>(field.getValue()))
        for (size_t i = 0; i < children.size(); ++i)
          if (auto child = dyn_cast<DictionaryAttr>(children[i]);
              child && child.getAs<StringAttr>("kind"))
            self(self, node.item(name, i));
    }
  };
  for (size_t i = 0; i < module.array("body").size(); ++i)
    walk(walk, module.item("body", i));
  return result;
}
LogicalResult Importer::prepareMemoryBindings() {
  for (auto &decl : modules) {
    auto writes = sourceNameWrites(decl.node);
    llvm::DenseMap<DictionaryAttr, AstNode> assignments;
    for (size_t i = 0; i < decl.node.array("body").size(); ++i) {
      auto statement = decl.node.item("body", i);
      if (statement.kind() == "Assign" &&
          !memoryKind(statement.child("value")).empty())
        assignments.try_emplace(
            sourceOccurrence(b, decl.symbol, statement.child("value")),
            statement);
    }
    llvm::DenseSet<DictionaryAttr> inspectedRules;
    auto walk = [&](auto &&self, const AstNode &node) -> LogicalResult {
      if (node.kind() == "Assign" || node.kind() == "AnnAssign") {
        auto rhs = node.child("value");
        if ((rhs.kind() == "Name" || rhs.kind() == "Attribute") &&
            !memoryReference(rhs).empty())
          return error() << "arbitrary memory callable aliases are unsupported";
      }
      StringRef kind = memoryKind(node);
      if (!kind.empty()) {
        auto diagnostic = [&] {
          return mlir::emitError(node.location(b.getContext(), source.path));
        };
        auto found = assignments.find(sourceOccurrence(b, decl.symbol, node));
        if (found == assignments.end())
          return diagnostic() << "memory allocation requires a direct "
                                 "module-body assignment";
        auto statement = found->second;
        if (statement.array("targets").size() != 1)
          return diagnostic() << "memory allocation requires one target";
        unsigned resultCount = kind == "sync_mem_dp" ? 2 : 1;
        auto target = statement.item("targets", 0);
        if ((resultCount == 1 && target.kind() != "Name") ||
            (resultCount == 2 &&
             (target.kind() != "Tuple" || target.array("elts").size() != 2)))
          return diagnostic() << "memory result requires " << resultCount
                              << " ordinary Names";
        MemoryCallPlan plan;
        plan.module = decl.node;
        plan.call = node;
        plan.occurrence = sourceOccurrence(b, decl.symbol, node);
        for (unsigned i = 0; i < resultCount; ++i) {
          auto element = resultCount == 1 ? target : target.item("elts", i);
          auto name = element.string("id");
          if (element.kind() != "Name" || name.empty() ||
              writes.lookup(name) != 1 || names.contains(name) ||
              behavioralRules.contains(name) ||
              llvm::is_contained(plan.resultNames, name.str()) ||
              llvm::any_of(decl.inputs,
                           [&](const Port &p) { return p.name == name; }) ||
              name == "pyc_clk" || name == "pyc_rst")
            return diagnostic()
                   << "memory results require fresh immutable ordinary Names";
          plan.resultNames.push_back(name.str());
        }
        auto builtin = headers.lookupBuiltin(kind);
        if (!builtin || !headers.isTrustedBuiltin(builtin.declaration))
          return diagnostic()
                 << "memory call requires its trusted canonical builtin";
        plan.callee = builtin.symbol;
        if (!SymbolTable::lookupSymbolIn(*body, builtin.symbol)) {
          auto *copy = builtin.declaration->clone();
          copy->setAttr("ac.declaration_role",
                        b.getStringAttr("import_snapshot"));
          body->getBody()->push_back(copy);
        }
        ac::HardwareAnalysis analysis(*body);
        auto payloadAnnotation = node.child("func").child("slice");
        if (!fixedAnnotation(payloadAnnotation) &&
            !nominalAnnotation(payloadAnnotation))
          return diagnostic() << "memory payload requires fixed bits or a "
                                 "nominal struct/enum";
        auto payload = portType(payloadAnnotation, decl.symbol);
        if (failed(payload))
          return failure();
        if (!isa<ac::BitsType, ac::StructType, ac::EnumType>(*payload))
          return diagnostic()
                 << "memory payload requires a scalar hardware binding";
        auto width =
            analysis.getPackedWidth(*payload, {}, body->getOperation());
        if (failed(width))
          return failure();
        if (kind == "byte_mem" && *width % 8)
          return diagnostic() << "byte_mem payload must be byte aligned";
        plan.payloadWidth = *width;
        plan.strobeWidth = *width / 8 + (*width % 8 != 0);
        AstNode depth;
        for (size_t i = 0; i < node.array("keywords").size(); ++i) {
          auto keyword = node.item("keywords", i);
          if (keyword.string("arg") == "depth") {
            if (depth)
              return diagnostic() << "duplicate memory depth";
            depth = keyword.child("value");
          }
        }
        if (!depth)
          return diagnostic() << "memory depth is required and keyword-only";
        auto depthExpression = staticExpr(depth, decl.symbol);
        if (failed(depthExpression))
          return failure();
        auto rawDepth =
            analysis.evaluateStatic(*depthExpression, {}, body->getOperation());
        auto integer = succeeded(rawDepth)
                           ? dyn_cast<ac::MathIntAttr>(*rawDepth)
                           : ac::MathIntAttr();
        if (!integer)
          return diagnostic() << "memory depth requires a positive closed "
                                 "Integer fitting u64";
        llvm::APSInt depthValue(integer.getCanonicalValue());
        if (depthValue.isNegative() || depthValue.isZero() ||
            depthValue.getActiveBits() > 64)
          return diagnostic() << "memory depth requires a positive closed "
                                 "Integer fitting u64";
        plan.depth = depthValue.getZExtValue();
        plan.addressWidth =
            plan.depth == 1 ? 1 : llvm::Log2_64(plan.depth - 1) + 1;
        plan.parameters = {literal(plan.addressWidth, node, decl.symbol),
                           *depthExpression};
        plan.typeArguments = {TypeAttr::get(*payload)};
        auto *declaration =
            analysis.lookupDefinition(builtin.symbol.getValue());
        ac::HardwareBindings bindings;
        bindings.owner = declaration;
        bindings.types["T"] = *payload;
        bindings.integers["ADDR_WIDTH"] = ac::MathIntAttr::get(
            b.getContext(), llvm::APSInt::getUnsigned(plan.addressWidth));
        bindings.integers["DEPTH"] = integer;
        auto signature = builtin.declaration.getFunctionType();
        auto inputNames = builtin.declaration.getInputNames();
        if (inputNames.size() < 2 ||
            cast<StringAttr>(inputNames[0]).getValue() != "clk" ||
            cast<StringAttr>(inputNames[1]).getValue() != "rst")
          return diagnostic() << "canonical memory signature lacks physical "
                                 "domain positions";
        for (size_t i = 2; i < signature.getNumInputs(); ++i) {
          auto type = analysis.resolveType(signature.getInput(i), bindings,
                                           body->getOperation());
          if (failed(type))
            return failure();
          plan.inputTypes.push_back(*type);
          plan.inputNames.push_back(
              cast<StringAttr>(inputNames[i]).getValue().str());
        }
        for (Type type : signature.getResults()) {
          auto resolved =
              analysis.resolveType(type, bindings, body->getOperation());
          if (failed(resolved))
            return failure();
          plan.resultTypes.push_back(*resolved);
          plan.resultFixedBits.push_back(isa<ac::BitsType>(*resolved));
        }
        plan.actuals.resize(plan.inputTypes.size());
        if (node.array("args").size() > plan.actuals.size())
          return diagnostic() << "too many memory runtime arguments";
        for (size_t i = 0; i < node.array("args").size(); ++i)
          plan.actuals[i] = node.item("args", i);
        for (size_t i = 0; i < node.array("keywords").size(); ++i) {
          auto keyword = node.item("keywords", i);
          auto name = keyword.string("arg");
          if (name == "depth")
            continue;
          auto position = llvm::find(plan.inputNames, name.str());
          if (position == plan.inputNames.end() ||
              plan.actuals[position - plan.inputNames.begin()])
            return diagnostic()
                   << "unknown or repeated memory runtime argument '" << name
                   << "'";
          plan.actuals[position - plan.inputNames.begin()] =
              keyword.child("value");
        }
        for (auto [i, actual] : llvm::enumerate(plan.actuals)) {
          if (!actual || actual.kind() == "Starred")
            return diagnostic() << "missing or starred memory runtime argument";
          // Inspect declared address authority without creating SSA or sampling
          // inputs.
          if (StringRef(plan.inputNames[i]).starts_with("raddr") ||
              plan.inputNames[i] == "waddr")
            for (const Port &port : decl.inputs)
              if (actual.kind() == "Name" && actual.string("id") == port.name &&
                  port.fixedBits &&
                  !ac::areEquivalentHardwareTypes(port.type,
                                                  plan.inputTypes[i]))
                return diagnostic() << "memory address width disagrees with "
                                       "depth-derived signature";
        }
        memoryPlans.push_back(std::move(plan));
        decl.queueDomain = decl.behavioral = true;
        return success();
      }
      if (node.kind() == "Call" && node.child("func").kind() == "Name") {
        auto rule = behavioralRules.find(node.child("func").string("id"));
        if (rule != behavioralRules.end() &&
            inspectedRules.insert(rule->second.value).second)
          if (failed(self(self, rule->second)))
            return failure();
      }
      for (NamedAttribute field : node.fields()) {
        auto name = field.getName().getValue();
        if (auto child = dyn_cast<DictionaryAttr>(field.getValue())) {
          if (child.getAs<StringAttr>("kind") &&
              failed(self(self, node.child(name))))
            return failure();
        } else if (auto children = dyn_cast<ArrayAttr>(field.getValue()))
          for (size_t i = 0; i < children.size(); ++i)
            if (auto child = dyn_cast<DictionaryAttr>(children[i]);
                child && child.getAs<StringAttr>("kind") &&
                failed(self(self, node.item(name, i))))
              return failure();
      }
      return success();
    };
    for (size_t i = 0; i < decl.node.array("body").size(); ++i)
      if (failed(walk(walk, decl.node.item("body", i))))
        return failure();
  }
  return success();
}
