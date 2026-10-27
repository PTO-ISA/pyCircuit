static std::string sourceRuleSpecializationKey(FlatSymbolRefAttr owner,
                                               StringRef name,
                                               const SourceCallableDecl &callable) {
  std::string key = (owner.getValue() + "|" + name).str();
  for (const Port &port : callable.inputs)
    if (auto alias = callable.stateFormals.find(port.name); alias != callable.stateFormals.end())
      key += "|" + port.name + "=" + alias->second;
  return key;
}

LogicalResult Importer::scanStaticInstanceReferences() {
  auto diagnostic = [&](const AstNode &site) {
    diagnosticFailed = true;
    return mlir::emitError(site.location(b.getContext(), source.path));
  };
  llvm::DenseSet<DictionaryAttr> admittedCalls;
  for (const auto &module : modules) {
    auto &references = staticInstanceReferences[module.symbol.getValue()];
    llvm::StringSet<> preceding;
    for (const auto &input : module.inputs) preceding.insert(input.name);
    for (const auto &parameter : module.parameters) preceding.insert(parameter.name);
    if (module.references)
      for (Attribute raw : module.references)
        preceding.insert(cast<DictionaryAttr>(raw).getAs<StringAttr>("name").getValue());
    bool returned = false;
    for (size_t i = 0; i < module.node.array("body").size(); ++i) {
      auto statement = module.node.item("body", i);
      auto value = statement.child("value");
      bool intrinsicCall = value.kind() == "Call" &&
                           intrinsic(value.child("func"), "instance_ref");
      auto alias = value.kind() == "Name" ? references.find(value.string("id"))
                                          : references.end();
      if (intrinsicCall || alias != references.end()) {
        if (returned)
          return diagnostic(statement) << "static instance reference declaration after Module return is unsupported";
        if (statement.kind() != "Assign" || statement.array("targets").size() != 1 ||
            statement.item("targets", 0).kind() != "Name")
          return diagnostic(statement) << "instance_ref requires one unannotated immutable Module-body Name";
        StringRef name = statement.item("targets", 0).string("id");
        if (preceding.contains(name) || references.contains(name) || names.contains(name) ||
            structs.contains(name) || enums.contains(name))
          return diagnostic(statement) << "static instance reference requires a fresh immutable binding";
        StaticInstanceReference reference;
        if (intrinsicCall) {
          auto args = value.array("args");
          if (args.empty() || !value.array("keywords").empty() || args.size() > 129)
            return diagnostic(value) << "instance_ref requires a Module provider and at most 128 literal path components";
          auto provider = lookupBinding(value.item("args", 0));
          if (!provider || provider->category != BindingCategory::Module || !provider->symbol)
            return diagnostic(value.item("args", 0)) << "instance_ref first argument must resolve to a Module provider";
          bool ordinary = false;
          for (const auto &candidate : modules)
            if (candidate.symbol == provider->symbol) ordinary = !candidate.system;
          if (provider->declaration)
            ordinary = isa<ac::ModuleImportOp>(provider->declaration) &&
                       !provider->declaration->hasAttr("primitive_kind") &&
                       !provider->declaration->hasAttr("ac.root_kind");
          if (!ordinary)
            return diagnostic(value.item("args", 0)) << "instance_ref requires an ordinary Module provider; System and primitive providers are unsupported";
          admittedCalls.insert(namespaceSite(value));
          reference.expectedModule = provider->symbol;
          bool descended = false;
          for (size_t k = 1; k < args.size(); ++k) {
            auto item = value.item("args", k);
            auto component = item.kind() == "Constant"
                                 ? dyn_cast_or_null<StringAttr>(item.get("value")) : StringAttr();
            if (!component)
              return diagnostic(item) << "instance_ref path requires literal strings";
            if (component.getValue() == "..") {
              if (descended)
                return diagnostic(item) << "instance_ref parent steps must precede child names";
              ++reference.ancestorDepth;
            } else {
              if (failed(ac::detail::verifyPythonAstIdentifier(component.getValue(), true,
                         [&] { return diagnostic(item); })))
                return failure();
              descended = true;
              reference.childNames.push_back(component);
            }
          }
          if (!reference.ancestorDepth && reference.childNames.empty() &&
              reference.expectedModule != module.symbol)
            return diagnostic(value) << "empty instance_ref path requires the current Module provider";
        } else {
          reference = alias->second;
        }
        if (failed(chargeLoopDescriptorWork(statement, "static instance reference binding",
                                            16 + 4 * reference.childNames.size())))
          return failure();
        reference.declaration = statement;
        references.try_emplace(name, std::move(reference));
        staticReferenceAssignments.insert(namespaceSite(statement));
      }
      if (statement.kind() == "Return") returned = true;
      if (statement.kind() == "FunctionDef") preceding.insert(statement.string("name"));
      if (statement.kind() == "Assign")
        for (size_t k = 0; k < statement.array("targets").size(); ++k)
          preceding.insert(assignmentRoot(statement.item("targets", k)));
      if (statement.kind() == "AnnAssign" || statement.kind() == "AugAssign")
        preceding.insert(assignmentRoot(statement.child("target")));
    }
    auto uses = [&](auto &&self, const AstNode &node) -> LogicalResult {
      if (failed(chargeLoopDescriptorWork(node, "static instance reference use")))
        return failure();
      if (node.kind() == "Assign" && staticReferenceAssignments.contains(namespaceSite(node)))
        return success();
      if (node.kind() == "Name" && references.contains(node.string("id")))
        return diagnostic(node) << "static instance reference is immutable and receiver-only";
      if (node.kind() == "arg" && references.contains(node.string("arg")))
        return diagnostic(node) << "static instance reference cannot be shadowed by a callable argument";
      if ((node.kind() == "FunctionDef" || node.kind() == "ClassDef") &&
          references.contains(node.string("name")))
        return diagnostic(node) << "static instance reference cannot be rebound by a declaration";
      if (node.kind() == "Call" && intrinsic(node.child("func"), "instance_ref"))
        return diagnostic(node) << "instance_ref declaration requires direct Module-body assignment";
      bool receiver = node.kind() == "Call" && node.child("func").kind() == "Attribute" &&
                      node.child("func").child("value").kind() == "Name" &&
                      references.contains(node.child("func").child("value").string("id"));
      for (NamedAttribute field : node.fields()) {
        if (receiver && field.getName().getValue() == "func") continue;
        if (auto child = dyn_cast<DictionaryAttr>(field.getValue())) {
          if (child.getAs<StringAttr>("kind") &&
              failed(self(self, node.child(field.getName().getValue())))) return failure();
        } else if (auto children = dyn_cast<ArrayAttr>(field.getValue()))
          for (size_t k = 0; k < children.size(); ++k)
            if (auto child = dyn_cast<DictionaryAttr>(children[k]);
                child && child.getAs<StringAttr>("kind") &&
                failed(self(self, node.item(field.getName().getValue(), k)))) return failure();
      }
      return success();
    };
    for (size_t i = 0; i < module.node.array("body").size(); ++i)
      if (failed(uses(uses, module.node.item("body", i)))) return failure();
  }
  // Unused global callables still cannot hide an illegal reference declaration.
  // This checks trusted syntax only; topology demands remain emitted-call owned.
  auto scope = [&](auto &&self, const AstNode &node) -> LogicalResult {
    if (failed(chargeLoopDescriptorWork(node, "static instance reference scope"))) return failure();
    if (node.kind() == "Call" && intrinsic(node.child("func"), "instance_ref") &&
        !admittedCalls.contains(namespaceSite(node)))
      return diagnostic(node) << "instance_ref declaration requires direct Module-body assignment";
    for (NamedAttribute field : node.fields()) {
      if (auto child = dyn_cast<DictionaryAttr>(field.getValue())) {
        if (child.getAs<StringAttr>("kind") &&
            failed(self(self, node.child(field.getName().getValue())))) return failure();
      } else if (auto children = dyn_cast<ArrayAttr>(field.getValue()))
        for (size_t k = 0; k < children.size(); ++k)
          if (auto child = dyn_cast<DictionaryAttr>(children[k]);
              child && child.getAs<StringAttr>("kind") &&
              failed(self(self, node.item(field.getName().getValue(), k)))) return failure();
    }
    return success();
  };
  return scope(scope, source.module);
}

LogicalResult Importer::scanSourceCallables() {
  llvm::StringMap<SmallVector<std::pair<AstNode, CapturedCallableSyntax>, 0>>
      staged;
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    AstNode module = source.module.item("body", i);
    if (module.kind() != "FunctionDef" ||
        (!hasDecorator(module, "module") && !hasDecorator(module, "system")))
      continue;
    StringRef moduleName = module.string("name");
    for (size_t j = 0; j < module.array("body").size(); ++j) {
      AstNode declaration = module.item("body", j);
      if (declaration.kind() != "FunctionDef")
        continue;
      bool marked = false;
      for (size_t k = 0; k < declaration.array("decorator_list").size(); ++k) {
        AstNode decorator = declaration.item("decorator_list", k);
        AstNode target =
            decorator.kind() == "Call" ? decorator.child("func") : decorator;
        auto resolved = resolveBinding(target);
        marked |= resolved && (resolved->marker == MarkerKind::Rule ||
                               resolved->marker == MarkerKind::Method);
      }
      if (!marked)
        continue;
      auto syntax = readCallableSyntax(
          declaration,
          [&](const AstNode &node) { return resolveBinding(node); }, error);
      if (failed(syntax)) {
        return failure();
      }
      if (syntax->after) return error() << "source after is retired; independent Rules read current state";
      staged[moduleName].push_back({declaration, *syntax});
      if (syntax->marker == MarkerKind::Rule) transactionModuleNames.insert(moduleName);
      if (syntax->marker == MarkerKind::Method)
        methodOwnerNames.insert(moduleName);
      if (syntax->marker == MarkerKind::Rule && (syntax->guard || syntax->after))
        transactionModuleNames.insert(moduleName);
      if (syntax->marker != MarkerKind::Rule || syntax->guard)
        continue;
      // Only statement-arm observations need the typed route here. Assertions
      // and unconditional observations retain their existing behavioral route.
      // In particular, selectors and nested helper/loop bodies are not scanned.
      struct PendingStatement {
        AstNode node;
        bool inArm;
      };
      SmallVector<PendingStatement, 0> pending;
      auto enqueue = [&](const AstNode &parent, StringRef field,
                         bool inArm) -> LogicalResult {
        for (size_t k = parent.array(field).size(); k != 0; --k) {
          TableQueryCharge charge;
          std::string reason;
          // item() copies the path, then appends two steps. Pay possible
          // copy-and-grow storage, retained-step moves and both appends before
          // constructing that AstNode.
          uint64_t pathCapacity =
              3 * std::max(parent.path.size(), AstNode().path.capacity()) + 1;
          bool fits = TableQueryBudget::addProduct(
                          charge, TableQueryResource::Slots, pathCapacity,
                          (sizeof(AstStep) + 7) / 8) &&
                      TableQueryBudget::addProduct(
                          charge, TableQueryResource::Work,
                          parent.path.size(), 2) &&
                      TableQueryBudget::add(charge, TableQueryResource::Work, 2);
          size_t capacity = pending.capacity();
          if (pending.size() == capacity) {
            capacity = 2 * capacity + 1;
            fits &= TableQueryBudget::addProduct(
                        charge, TableQueryResource::Slots, capacity,
                        (sizeof(PendingStatement) + 7) / 8) &&
                    TableQueryBudget::add(charge, TableQueryResource::Work,
                                          pending.size());
          }
          if (!fits || !queryBudget.reserve(charge, reason))
            return error() << (fits ? reason
                                   : "source callable scan budget exhausted");
          pending.reserve(capacity);
          pending.push_back({parent.item(field, k - 1), inArm});
        }
        return success();
      };
      if (failed(enqueue(declaration, "body", false)))
        return failure();
      while (!pending.empty()) {
        PendingStatement statement = pending.pop_back_val();
        if (statement.node.kind() == "If") {
          if (failed(enqueue(statement.node, "orelse", true)) ||
              failed(enqueue(statement.node, "body", true)))
            return failure();
          continue;
        }
        if (!statement.inArm || statement.node.kind() != "Expr")
          continue;
        // Each child() copies the path and can move all retained steps when
        // appending. The second child starts with the first child's new step.
        TableQueryCharge charge;
        std::string reason;
        uint64_t pathCapacity =
            3 * std::max(statement.node.path.size() + 1,
                         AstNode().path.capacity()) + 1;
        bool fits = TableQueryBudget::addProduct(
                        charge, TableQueryResource::Slots, pathCapacity,
                        2 * ((sizeof(AstStep) + 7) / 8)) &&
                    TableQueryBudget::addProduct(
                        charge, TableQueryResource::Work,
                        statement.node.path.size(), 4) &&
                    TableQueryBudget::add(charge, TableQueryResource::Work, 4);
        if (!fits || !queryBudget.reserve(charge, reason))
          return error() << (fits ? reason
                                 : "source callable scan budget exhausted");
        AstNode call = statement.node.child("value");
        if (call.kind() != "Call")
          continue;
        AstNode function = call.child("func");
        if (intrinsic(function, "log") || intrinsic(function, "report")) {
          transactionModuleNames.insert(moduleName);
          break;
        }
      }
    }
  }
  llvm::DenseSet<DictionaryAttr> globalRuleDefinitions;
  for (const auto &global : behavioralRules) globalRuleDefinitions.insert(global.second.value);
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    AstNode module = source.module.item("body", i);
    if (module.kind() != "FunctionDef" ||
        (!hasDecorator(module, "module") && !hasDecorator(module, "system"))) continue;
    auto &entries = staged[module.string("name")];
    llvm::StringSet<> localNames;
    for (const auto &entry : entries) localNames.insert(entry.first.string("name"));
    auto visit = [&](auto &&self, const AstNode &node) -> LogicalResult {
      if (node.kind() == "ClassDef" || node.kind() == "Lambda") return success();
      if (node.kind() == "Call" && node.child("func").kind() == "Name") {
        StringRef name = node.child("func").string("id");
        auto global = behavioralRules.find(name);
        if (global != behavioralRules.end() && !localNames.contains(name)) {
          auto syntax = readCallableSyntax(global->second,
              [&](const AstNode &site) { return resolveBinding(site); }, error);
          if (failed(syntax)) return failure();
          entries.push_back({global->second, *syntax});
          localNames.insert(name);
        }
      }
      for (NamedAttribute field : node.fields()) {
        if (auto child = dyn_cast<DictionaryAttr>(field.getValue())) {
          if (child.getAs<StringAttr>("kind") && failed(self(self, node.child(field.getName().getValue())))) return failure();
        } else if (auto children = dyn_cast<ArrayAttr>(field.getValue()))
          for (size_t k = 0; k < children.size(); ++k)
            if (auto child = dyn_cast<DictionaryAttr>(children[k]); child && child.getAs<StringAttr>("kind") &&
                failed(self(self, node.item(field.getName().getValue(), k)))) return failure();
      }
      return success();
    };
    for (size_t j = 0; j < module.array("body").size(); ++j)
      if (failed(visit(visit, module.item("body", j)))) return failure();
    // A staged source-global template may call another template that has no
    // direct use in this Module. Close the same callable inventory before
    // creating signatures; appending each name once also bounds recursion.
    for (size_t j = 0; j < entries.size(); ++j) {
      AstNode declaration = entries[j].first;
      if (globalRuleDefinitions.contains(declaration.value) &&
          failed(visit(visit, declaration)))
        return failure();
    }
  }
  // A Rule may be a callable definition without an autonomous registration.
  // Local nested invocations use the same typed effect producer as Methods.
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    AstNode module = source.module.item("body", i);
    auto entries = staged.find(module.string("name"));
    if (module.kind() != "FunctionDef" || entries == staged.end()) continue;
    StringRef moduleName = module.string("name");
    llvm::StringSet<> localRules;
    llvm::StringMap<unsigned> registrations;
    for (const auto &entry : entries->second)
      if (entry.second.marker == MarkerKind::Rule)
        localRules.insert(entry.first.string("name"));
    for (size_t j = 0; j < module.array("body").size(); ++j) {
      auto statement = module.item("body", j);
      auto call = statement.child("value");
      if (call.kind() == "Call" && call.child("func").kind() == "Name" &&
          localRules.contains(call.child("func").string("id")))
        ++registrations[call.child("func").string("id")];
    }
    for (const auto &name : localRules)
      if (!registrations.lookup(name.getKey()))
        callableRuleNames[moduleName].insert(name.getKey());
    auto visit = [&](auto &&self, const AstNode &node) -> void {
      if (node.kind() == "FunctionDef" || node.kind() == "Lambda") return;
      if (node.kind() == "Call" && node.child("func").kind() == "Name" &&
          localRules.contains(node.child("func").string("id")))
        callableRuleNames[moduleName].insert(node.child("func").string("id"));
      for (NamedAttribute field : node.fields()) {
        if (auto child = dyn_cast<DictionaryAttr>(field.getValue())) {
          if (child.getAs<StringAttr>("kind")) self(self, node.child(field.getName().getValue()));
        } else if (auto children = dyn_cast<ArrayAttr>(field.getValue()))
          for (size_t k = 0; k < children.size(); ++k)
            if (auto child = dyn_cast<DictionaryAttr>(children[k]);
                child && child.getAs<StringAttr>("kind"))
              self(self, node.item(field.getName().getValue(), k));
      }
    };
    llvm::StringSet<> moduleOwners;
    for (size_t j = 0; j < module.array("body").size(); ++j) {
      auto statement = module.item("body", j);
      if (statement.kind() == "AnnAssign")
        moduleOwners.insert(assignmentRoot(statement.child("target")));
      auto value = statement.child("value");
      if (statement.kind() == "Assign" && statement.array("targets").size() == 1 &&
          value.kind() == "Call" && value.child("func").kind() == "Subscript" &&
          intrinsic(value.child("func").child("value"), "table"))
        moduleOwners.insert(assignmentRoot(statement.item("targets", 0)));
    }
    for (const auto &entry : entries->second) {
      llvm::StringSet<> formals;
      bool typedContract = static_cast<bool>(entry.first.child("returns"));
      auto args = entry.first.child("args");
      for (size_t k = 0; k < args.array("args").size(); ++k) {
        formals.insert(args.item("args", k).string("arg"));
        typedContract &= static_cast<bool>(args.item("args", k).child("annotation"));
      }
      if (typedContract && entry.second.marker == MarkerKind::Rule)
        callableRuleNames[moduleName].insert(entry.first.string("name"));
      auto effects = [&](auto &&self, const AstNode &node) -> void {
        if (node.kind() == "FunctionDef" || node.kind() == "Lambda") return;
        if (typedContract && (node.kind() == "For" || node.kind() == "While"))
          transactionModuleNames.insert(moduleName);
        auto ownerWrite = [&](const AstNode &target) {
          (void)forEachAssignmentLeaf(target, [&](const AstNode &leaf) {
            StringRef root = assignmentRoot(leaf);
            if (moduleOwners.contains(root) && !formals.contains(root))
              transactionModuleNames.insert(moduleName);
            return success();
          });
        };
        if (node.kind() == "Assign")
          for (size_t k = 0; k < node.array("targets").size(); ++k)
            ownerWrite(node.item("targets", k));
        else if (node.kind() == "AnnAssign" || node.kind() == "AugAssign" || node.kind() == "For")
          ownerWrite(node.child("target"));
        for (StringRef field : {"body", "orelse"}) {
          auto statements = node.array(field);
          for (size_t k = 0; statements && k < statements.size(); ++k)
            self(self, node.item(field, k));
        }
      };
      for (size_t j = 0; j < entry.first.array("body").size(); ++j) {
        auto statement = entry.first.item("body", j);
        visit(visit, statement);
        effects(effects, statement);
      }
    }
    if (!callableRuleNames[moduleName].empty() ||
        llvm::any_of(registrations, [](const auto &entry) { return entry.second > 1; }))
      transactionModuleNames.insert(moduleName);
  }
  // Every authored Rule registration uses the same transaction producer.
  // Static elaboration-only binding Rules are no longer a second Rule route.
  for (const auto &entry : staged)
    if (!entry.second.empty()) transactionModuleNames.insert(entry.first());
  // Classify the actual static module-call graph before reading callable
  // contracts. This includes imported transaction value modules and propagates
  // their publication route through local parents without making them handles.
  llvm::StringMap<SmallVector<std::string, 0>> edges;
  llvm::StringSet<> moduleNames;
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    AstNode module = source.module.item("body", i);
    if (module.kind() == "FunctionDef" &&
        (hasDecorator(module, "module") || hasDecorator(module, "system")))
      moduleNames.insert(module.string("name"));
  }
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    AstNode module = source.module.item("body", i);
    if (module.kind() != "FunctionDef" ||
        !moduleNames.contains(module.string("name")))
      continue;
    StringRef moduleName = module.string("name");
    if (methodOwnerNames.contains(moduleName))
      transactionModuleNames.insert(moduleName);
    auto walk = [&](auto &&self, const AstNode &node) -> void {
      if (node.kind() == "FunctionDef" || node.kind() == "Lambda")
        return;
      if (node.kind() == "Call") {
        AstNode function = node.child("func");
        if (function.kind() == "Name" &&
            moduleNames.contains(function.string("id")) &&
            !lexicallyShadowed(function, function.string("id")))
          edges[moduleName].push_back(function.string("id").str());
        auto binding = lookupBinding(function);
        if (binding && binding->category == BindingCategory::Module) {
          auto view = headers.lookupModule(binding->symbol);
          if (view && view.semanticKind &&
              view.semanticKind.getValue() == "transactions")
            transactionModuleNames.insert(moduleName);
        }
      }
      for (NamedAttribute field : node.fields()) {
        if (auto child = dyn_cast<DictionaryAttr>(field.getValue())) {
          if (child.getAs<StringAttr>("kind"))
            self(self, node.child(field.getName().getValue()));
        } else if (auto children = dyn_cast<ArrayAttr>(field.getValue())) {
          for (size_t j = 0; j < children.size(); ++j)
            if (auto child = dyn_cast<DictionaryAttr>(children[j]);
                child && child.getAs<StringAttr>("kind"))
              self(self, node.item(field.getName().getValue(), j));
        }
      }
    };
    for (size_t j = 0; j < module.array("body").size(); ++j)
      walk(walk, module.item("body", j));
  }
  bool changed;
  do {
    changed = false;
    for (auto &entry : edges)
      if (!transactionModuleNames.contains(entry.first()))
        for (const std::string &child : entry.second)
          if (transactionModuleNames.contains(child)) {
            transactionModuleNames.insert(entry.first());
            changed = true;
            break;
          }
  } while (changed);
  // Build one typed callable inventory for all registered Rules and Methods.
  for (auto &moduleEntry : staged) {
    StringRef moduleName = moduleEntry.first();
    if (!transactionModuleNames.contains(moduleName))
      continue;
    for (auto &entry : moduleEntry.second) {
      AstNode declaration = entry.first;
      CapturedCallableSyntax syntax = entry.second;
      SourceCallableDecl callable{declaration, syntax};
      callable.lexical = !globalRuleDefinitions.contains(declaration.value);
      callable.autonomous = callable.lexical && syntax.marker == MarkerKind::Rule;
      AstNode arguments = declaration.child("args");
      if (!arguments.array("posonlyargs").empty() ||
          !arguments.array("kwonlyargs").empty() ||
          ((!callable.lexical || syntax.marker != MarkerKind::Rule) &&
           !arguments.array("defaults").empty()) || arguments.child("vararg") ||
          arguments.child("kwarg"))
        return error() << "Rule/Method parameters require ordinary required "
                          "positional arguments";
      if (callable.lexical && syntax.marker == MarkerKind::Rule &&
          arguments.array("defaults").size() != arguments.array("args").size())
        return error() << "autonomous Rule formal requires an explicit default binding";
      llvm::StringSet<> names;
      for (size_t k = 0; k < arguments.array("args").size(); ++k) {
        AstNode argument = arguments.item("args", k);
        if (!names.insert(argument.string("arg")).second)
          return error() << "Rule/Method parameter requires a unique name";
        if (!argument.child("annotation")) {
          if (syntax.marker == MarkerKind::Method)
            return error() << "Method parameter requires an explicit annotation";
          callable.inferredSignature = true;
          callable.inputs.push_back({argument.string("arg").str(), {}, argument, {}, false});
          continue;
        }
        auto type = portType(
            argument.child("annotation"),
            FlatSymbolRefAttr::get(b.getContext(), qualify(moduleName)));
        if (failed(type))
          return failure();
        callable.inputs.push_back(
            {argument.string("arg").str(), *type, argument,
             annotationKind(argument.child("annotation")),
             fixedAnnotation(argument.child("annotation"))});
      }
      if (syntax.marker == MarkerKind::Rule) {
        auto assigned = [&](auto &&self, const AstNode &node) -> void {
          if (node.kind() == "FunctionDef" || node.kind() == "Lambda") return;
          auto target = [&](const AstNode &binding) {
            (void)forEachAssignmentLeaf(binding, [&](const AstNode &leaf) {
              StringRef root = assignmentRoot(leaf);
              if (llvm::any_of(callable.inputs, [&](const Port &port) { return port.name == root; }))
                callable.assignedFormals.insert(root);
              return success();
            });
          };
          if (node.kind() == "Assign")
            for (size_t k = 0; k < node.array("targets").size(); ++k)
              target(node.item("targets", k));
          else if (node.kind() == "AnnAssign" || node.kind() == "AugAssign" || node.kind() == "For")
            target(node.child("target"));
          for (NamedAttribute field : node.fields()) {
            if (auto child = dyn_cast<DictionaryAttr>(field.getValue())) {
              if (child.getAs<StringAttr>("kind")) self(self, node.child(field.getName().getValue()));
            } else if (auto children = dyn_cast<ArrayAttr>(field.getValue()))
              for (size_t k = 0; k < children.size(); ++k)
                if (auto child = dyn_cast<DictionaryAttr>(children[k]);
                    child && child.getAs<StringAttr>("kind"))
                  self(self, node.item(field.getName().getValue(), k));
          }
        };
        for (size_t k = 0; k < declaration.array("body").size(); ++k)
          assigned(assigned, declaration.item("body", k));
      }
      AstNode returns = declaration.child("returns");
      bool none =
          returns.kind() == "Constant" && isa<UnitAttr>(returns.get("value"));
      if (!returns) {
        if (syntax.marker == MarkerKind::Method)
          return error() << "Method requires an explicit return annotation";
        callable.inferredSignature = true;
        bool valueReturn = false;
        // Seed result inference from the suites that may execute in the outer
        // statement context. Actual zero-trip and targeting-break admission
        // remains with emitLoopElse: a dead body's break cannot make else
        // conditional, and a live conditional return must still be rejected.
        auto resultPresence = [&](auto &&self, const AstNode &statement) -> LogicalResult {
          if (failed(chargeLoopDescriptorWork(statement, "Rule result presence")))
            return failure();
          auto value = statement.child("value");
          if (statement.kind() == "Return" && value &&
              !(value.kind() == "Constant" && isa<UnitAttr>(value.get("value"))))
            valueReturn = true;
          if (statement.kind() == "For" || statement.kind() == "While")
            for (size_t k = 0; k < statement.array("orelse").size(); ++k)
              if (failed(self(self, statement.item("orelse", k))))
                return failure();
          return success();
        };
        for (size_t k = 0; k < declaration.array("body").size(); ++k)
          if (failed(resultPresence(resultPresence, declaration.item("body", k))))
            return failure();
        callable.inferResults = valueReturn;
        none = true;
      }
      if (!none) {
        if (returns.kind() == "Dict" && syntax.marker == MarkerKind::Method)
          return error() << "Method returns require a scalar, nominal type or fixed tuple";
        ModuleDecl contract;
        contract.symbol = FlatSymbolRefAttr::get(b.getContext(), qualify(moduleName));
        llvm::StringSet<> resultNames;
        if (returns.kind() != "Dict")
          for (const Port &input : callable.inputs) resultNames.insert(input.name);
        if (failed(readReturnContract(contract, returns, resultNames, true)))
          return failure();
        callable.returnShape = contract.returnShape;
        callable.resultContracts = std::move(contract.outputs);
        for (const Port &result : callable.resultContracts)
          callable.results.push_back(result.type);
      }
      SmallVector<Type> inputs;
      for (const Port &port : callable.inputs)
        inputs.push_back(port.type);
      if (llvm::all_of(inputs, [](Type type) { return static_cast<bool>(type); }))
        callable.type = FunctionType::get(b.getContext(), inputs, callable.results);
      auto &inventory = syntax.marker == MarkerKind::Method
                            ? sourceMethods[moduleName]
                            : sourceRules[moduleName];
      if (!inventory.try_emplace(declaration.string("name"), callable).second)
        return error() << "duplicate Rule/Method declaration name";
    }
  }
  // Forwarded owner parameters retain write intent through the whole static
  // Rule call graph. This pass records which formal positions can propose;
  // the emitted body still derives the actual D and raw intent SSA.
  llvm::StringMap<llvm::StringMap<FlatSymbolRefAttr>> receiverSymbols;
  llvm::StringMap<FlatSymbolRefAttr> sourceModuleSymbols;
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    auto node = source.module.item("body", i);
    if (node.kind() == "FunctionDef" && (hasDecorator(node, "module") || hasDecorator(node, "system")))
      sourceModuleSymbols[node.string("name")] = FlatSymbolRefAttr::get(b.getContext(), qualify(node.string("name")));
  }
  for (size_t moduleIndex = 0; moduleIndex < source.module.array("body").size(); ++moduleIndex) {
    auto module = source.module.item("body", moduleIndex);
    if (!sourceModuleSymbols.contains(module.string("name"))) continue;
    auto &receivers = receiverSymbols[module.string("name")];
    for (size_t i = 0; i < module.array("body").size(); ++i) {
      auto statement = module.item("body", i);
      if (statement.kind() != "Assign" || statement.array("targets").size() != 1 ||
          statement.item("targets", 0).kind() != "Name") continue;
      StringRef name = statement.item("targets", 0).string("id");
      auto value = statement.child("value");
      if (value.kind() == "Call") {
        auto function = value.child("func");
        if (intrinsic(function, "instance_ref") && !value.array("args").empty()) {
          auto provider = value.item("args", 0);
          if (provider.kind() == "Name" && sourceModuleSymbols.contains(provider.string("id")) &&
              !lexicallyShadowed(provider, provider.string("id")))
            receivers[name] = sourceModuleSymbols.lookup(provider.string("id"));
          else if (auto binding = lookupBinding(provider);
                   binding && binding->category == BindingCategory::Module)
            receivers[name] = binding->symbol;
        } else if (function.kind() == "Name" && sourceModuleSymbols.contains(function.string("id")) &&
            !lexicallyShadowed(function, function.string("id")))
          receivers[name] = sourceModuleSymbols.lookup(function.string("id"));
        else if (auto binding = lookupBinding(function);
                 binding && binding->category == BindingCategory::Module)
          receivers[name] = binding->symbol;
      } else if (value.kind() == "Name" && receivers.contains(value.string("id")))
        receivers[name] = receivers.lookup(value.string("id"));
    }
  }
  do {
    changed = false;
    for (auto &module : sourceRules)
      for (auto &entry : module.second) {
        SourceCallableDecl &caller = entry.second;
        auto visit = [&](auto &&self, const AstNode &node) -> LogicalResult {
          if (node.kind() == "FunctionDef" || node.kind() == "Lambda") return success();
          if (failed(chargeLoopDescriptorWork(node, "Rule formal proposal closure")))
            return failure();
          if (node.kind() == "Call") {
            auto function = node.child("func");
            SourceCallableDecl *target = nullptr;
            std::optional<ac::SourceMethodView> imported;
            StringRef calleeName;
            FlatSymbolRefAttr receiver;
            if (function.kind() == "Name") {
              calleeName = function.string("id");
              auto found = module.second.find(calleeName);
              if (found != module.second.end()) target = &found->second;
            } else if (function.kind() == "Attribute" && function.child("value").kind() == "Name") {
              calleeName = function.string("attr");
              receiver = receiverSymbols[module.first()].lookup(function.child("value").string("id"));
              if (receiver) {
                for (const auto &decl : sourceModuleSymbols)
                  if (decl.second == receiver) {
                    auto found = sourceRules[decl.first()].find(calleeName);
                    if (found != sourceRules[decl.first()].end()) target = &found->second;
                    break;
                  }
                if (!target) {
                  auto view = headers.lookupModule(receiver);
                  if (view) {
                    auto method = ac::lookupSourceMethod(view.declaration, calleeName, body->getOperation());
                    if (failed(method)) return failure();
                    imported = *method;
                  }
                }
              }
            }
            SmallVector<StringRef> names;
            SmallVector<unsigned> writable;
            if (target) {
              for (auto [index, port] : llvm::enumerate(target->inputs)) {
                names.push_back(port.name);
                if (target->assignedFormals.contains(port.name)) writable.push_back(index);
              }
            } else if (imported) {
              for (Attribute raw : imported->getParameters())
                names.push_back(cast<DictionaryAttr>(raw).getAs<StringAttr>("name").getValue());
              auto proposals = imported->getFormalProposals();
              if (proposals)
                for (Attribute raw : proposals) writable.push_back(cast<IntegerAttr>(raw).getInt());
            }
            if (!writable.empty()) {
              auto actuals = normalizeCallArguments(node, names, error);
              if (failed(actuals)) return failure();
              for (unsigned index : writable) {
                auto actual = (*actuals)[index];
                if (actual.kind() != "Name") continue;
                StringRef name = actual.string("id");
                if (llvm::any_of(caller.inputs, [&](const Port &port) { return port.name == name; }))
                  changed |= caller.assignedFormals.insert(name).second;
              }
            }
          }
          for (NamedAttribute field : node.fields()) {
            if (auto child = dyn_cast<DictionaryAttr>(field.getValue())) {
              if (child.getAs<StringAttr>("kind") && failed(self(self, node.child(field.getName().getValue()))))
                return failure();
            } else if (auto children = dyn_cast<ArrayAttr>(field.getValue()))
              for (size_t k = 0; k < children.size(); ++k)
                if (auto child = dyn_cast<DictionaryAttr>(children[k]);
                    child && child.getAs<StringAttr>("kind") &&
                    failed(self(self, node.item(field.getName().getValue(), k))))
                  return failure();
          }
          return success();
        };
        for (size_t i = 0; i < caller.declaration.array("body").size(); ++i)
          if (failed(visit(visit, caller.declaration.item("body", i)))) return failure();
      }
  } while (changed);
  for (auto &module : callableRuleNames)
    for (const auto &name : module.second) {
      auto rule = sourceRules[module.first()].find(name.getKey());
      if (rule == sourceRules[module.first()].end())
        return error() << "callable Rule lacks a source declaration";
      if (!sourceMethods[module.first()].try_emplace(name.getKey(), rule->second).second)
        return error() << "Rule and Method callable names conflict";
    }
  for (auto &module : sourceRules)
    for (const auto &entry : module.second)
      sourceMethods[module.first()].try_emplace(entry.first(), entry.second);
  // Ordering metadata remains lexical to an owning Module. A source-global
  // Rule can carry a guard but cannot name an outer Module registration.
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    AstNode declaration = source.module.item("body", i);
    if (declaration.kind() != "FunctionDef")
      continue;
    bool marked = false;
    for (size_t j = 0; j < declaration.array("decorator_list").size(); ++j) {
      AstNode decorator = declaration.item("decorator_list", j);
      AstNode target =
          decorator.kind() == "Call" ? decorator.child("func") : decorator;
      auto resolved = resolveBinding(target);
      marked |= resolved && resolved->marker == MarkerKind::Rule;
    }
    if (!marked)
      continue;
    auto syntax = readCallableSyntax(
        declaration, [&](const AstNode &node) { return resolveBinding(node); },
        error);
    if (succeeded(syntax) && syntax->after)
      return error() << "source after is retired; independent Rules read current state";
    if (failed(syntax))
      return failure();
  }
  return success();
}

LogicalResult Importer::emitSourceTransactions(ModuleDecl &decl) {
  auto signature = moduleSignatureAttrs(decl);
  if (failed(signature)) return failure();

  b.setInsertionPointToEnd(body->getBody());
  Operation *module = createOp(
      b, decl.node.location(b.getContext(), source.path),
      ac::ModuleOp::getOperationName(), {}, {},
      {b.getNamedAttr("sym_name", b.getStringAttr(decl.symbol.getValue())),
       b.getNamedAttr("source_owner", owner),
       b.getNamedAttr("parameters", signature->getAs<ArrayAttr>("parameters")),
       b.getNamedAttr("type_parameters", b.getArrayAttr({})),
       b.getNamedAttr("function_type", TypeAttr::get(decl.type)),
       b.getNamedAttr("input_names", portNames(b, decl.inputs)),
       b.getNamedAttr("output_names", portNames(b, decl.outputs)),
       b.getNamedAttr("ac.declaration_role", b.getStringAttr("definition")),
       b.getNamedAttr("ac.origin",
                      sourceOccurrence(b, decl.symbol, decl.node))},
      1);
  if (decl.references && !decl.references.empty())
    module->setAttr("ac.reference_parameters", decl.references);
  if (decl.system)
    module->setAttr("ac.root_kind", b.getStringAttr("system"));
  if (decl.systemSamples)
    module->setAttr("ac.system_samples", decl.systemSamples);
  for (NamedAttribute attr : decl.sourceCallAttrs)
    module->setAttr(attr.getName(), attr.getValue());
  auto *graph = new Block();
  module->getRegion(0).push_back(graph);
  llvm::StringMap<Value> moduleValues;
  for (const Port &port : decl.inputs) {
    Value value = graph->addArgument(port.type, module->getLoc());
    moduleValues[port.name] = value;
    remember(value, port.sourceKind);
    if (port.fixedBits)
      fixedValues.insert(value);
  }
  // Keep SSA values, not StringMap iterators: later ordinary bindings and
  // compiler-owned Queue view captures can grow this lexical map.
  Value clock = moduleValues.lookup("pyc_clk"),
        reset = moduleValues.lookup("pyc_rst");
  if (!clock || !reset)
    return error() << "typed transactions require inferred clock and reset";
  OpBuilder at = OpBuilder::atBlockEnd(graph);
  llvm::StringMap<StateOwner> states;
  auto *previousBehavioralDecl = activeBehavioralDecl;
  activeBehavioralDecl = &decl;
  auto restoreBehavioralDecl = llvm::scope_exit(
      [&] { activeBehavioralDecl = previousBehavioralDecl; });
  structuralCallSites.clear();
  SmallVector<Instance, 0> instanceStorage;
  llvm::StringMap<Instance *> instances;
  if (failed(predeclareStructuralInstances(decl, at, instanceStorage, instances)) ||
      failed(predeclareDirectCalls(decl, at)))
    return failure();
  auto &methods = sourceMethods[decl.node.string("name")];
  auto &rules = sourceRules[decl.node.string("name")];
  using CaptureSnapshot = SmallVector<std::pair<std::string, Value>, 0>;
  llvm::StringMap<CaptureSnapshot> methodCaptures, ruleCaptures;
  llvm::DenseSet<DictionaryAttr> ordinaryModuleAssignments;
  llvm::StringSet<> authoredInputs;
  for (size_t i = 0; i < decl.sourceInputCount; ++i)
    authoredInputs.insert(decl.inputs[i].name);

  // Child outputs are explicit interfaces. An ordinary alias/computation of a
  // local Rule result is not a new input boundary and must not create hidden
  // availability dependencies merely because snapshots inventory lexical wires.
  auto localRuleResult = [&](Value value) {
    SmallVector<Value> pending{value};
    llvm::DenseSet<Value> seen;
    while (!pending.empty()) {
      Value current = pending.pop_back_val();
      if (!seen.insert(current).second) continue;
      auto *producer = current.getDefiningOp();
      if (!producer || isa<ac::InstanceOp, ac::SourceStateOp>(producer)) continue;
      if (isa<ac::SourceTransactionOp>(producer)) return true;
      llvm::append_range(pending, producer->getOperands());
    }
    return false;
  };
  auto snapshot = [&](CaptureSnapshot &captures) {
    captures.clear();
    llvm::DenseSet<Value> seen;
    for (const auto &value : moduleValues)
      if (value.first() != "pyc_clk" && value.first() != "pyc_rst" &&
          !localRuleResult(value.second)) {
        captures.push_back({value.first().str(), value.second});
        seen.insert(value.second);
      }
    // Instance outputs are ordinary lexical SSA captures. Empty names keep
    // them out of the source value namespace while captureValue records their
    // actual producer identity for raw-view lookup inside Method/Rule regions.
    for (const auto &entry : instances)
      if (entry.second->op)
      for (Value output : entry.second->op->getResults())
        if (seen.insert(output).second)
          captures.push_back({"", output});
  };

  auto boundHardwareIdentity = [&](StringRef name) {
    return methods.contains(name) || rules.contains(name) ||
           instances.contains(name) || names.contains(name) ||
           structs.contains(name) || enums.contains(name) ||
           behavioralRules.contains(name);
  };
  llvm::StringSet<> authoredOwnerNames;
  for (size_t i = 0; i < decl.node.array("body").size(); ++i) {
    auto statement = decl.node.item("body", i);
    if (statement.kind() == "AnnAssign")
      authoredOwnerNames.insert(assignmentRoot(statement.child("target")));
    auto value = statement.child("value");
    if (statement.kind() == "Assign" && statement.array("targets").size() == 1 &&
        value.kind() == "Call" && value.child("func").kind() == "Subscript" &&
        intrinsic(value.child("func").child("value"), "table"))
      authoredOwnerNames.insert(assignmentRoot(statement.item("targets", 0)));
  }
  // Bind local writable actuals to the physical owner before producing the
  // callable body. A delayed D/E suffix alone would miss lexical aliases and
  // nested calls that observe the same owner's intermediate candidate.
  llvm::StringSet<> lexicalNames;
  auto collectNames = [&](auto &&self, const AstNode &node) -> LogicalResult {
    if (failed(chargeLoopDescriptorWork(node, "Rule specialization name hygiene"))) return failure();
    if (node.kind() == "Name") lexicalNames.insert(node.string("id"));
    if (node.kind() == "arg") lexicalNames.insert(node.string("arg"));
    if (node.kind() == "FunctionDef") lexicalNames.insert(node.string("name"));
    for (NamedAttribute field : node.fields()) {
      if (auto child = dyn_cast<DictionaryAttr>(field.getValue())) {
        if (child.getAs<StringAttr>("kind") && failed(self(self, node.child(field.getName().getValue()))))
          return failure();
      } else if (auto children = dyn_cast<ArrayAttr>(field.getValue()))
        for (size_t k = 0; k < children.size(); ++k)
          if (auto child = dyn_cast<DictionaryAttr>(children[k]);
              child && child.getAs<StringAttr>("kind") &&
              failed(self(self, node.item(field.getName().getValue(), k))))
            return failure();
    }
    return success();
  };
  if (failed(collectNames(collectNames, decl.node))) return failure();
  SmallVector<std::string> methodWork;
  llvm::StringSet<> queuedMethods;
  for (const auto &entry : methods)
    if (!entry.second.inferredSignature) {
      methodWork.push_back(entry.first().str());
      queuedMethods.insert(entry.first());
    }
  auto retainHelperSnapshots = [&](CaptureSnapshot &captures) {
    for (const auto &entry : valueHelpers) {
      if (entry.second->lexicalOwner != decl.symbol) continue;
      for (const auto &capture : entry.second->captures)
        if (!localRuleResult(capture.second) && llvm::none_of(captures, [&](const auto &existing) { return existing.second == capture.second; }))
          captures.push_back({{}, capture.second});
    }
  };
  llvm::StringMap<Operation *> emittedMethods;
  std::function<LogicalResult(StringRef)> emitMethod;
  std::function<FailureOr<std::string>(StringRef, const SourceCallableDecl &)> specialize =
      [&](StringRef name, const SourceCallableDecl &specialized) -> FailureOr<std::string> {
    if (failed(chargeLoopDescriptorWork(specialized.declaration, "Rule actual-SSA specialization")))
      return failure();
    if (specialized.lexical) retainHelperSnapshots(methodCaptures[name]);
    std::string key = sourceRuleSpecializationKey(decl.symbol, name, specialized);
    llvm::raw_string_ostream encoded(key);
    for (auto [index, port] : llvm::enumerate(specialized.inputs)) {
      // Types and closed attributes are uniqued in this compiler context.
      // Identity keys avoid serializing their full provenance trees again.
      encoded << "|" << port.name << ":" << port.type.getAsOpaquePointer()
              << ":" << port.fixedBits << ":";
      if (port.sourceKind) encoded << static_cast<unsigned>(*port.sourceKind);
      if (index < specialized.actualFacts.size()) {
        auto facts = valueInfo(specialized.actualFacts[index]);
        if (facts.closedSourceConstant)
          encoded << ":" << facts.closedSourceConstant.getAsOpaquePointer();
      }
    }
    encoded << "|return:" << specialized.inferResults << ":"
            << static_cast<unsigned>(specialized.returnShape);
    for (const Port &port : specialized.resultContracts) {
      encoded << ":" << port.type.getAsOpaquePointer() << ":" << port.fixedBits;
      if (port.sourceKind) encoded << ":" << static_cast<unsigned>(*port.sourceKind);
    }
    for (const auto &capture : methodCaptures[name])
      encoded << "|capture:" << capture.first << ":" << capture.second.getAsOpaquePointer();
    encoded.flush();
    auto existing = sourceRuleSpecializations.find(key);
    if (existing != sourceRuleSpecializations.end()) return existing->second;
    std::string privateName;
    size_t serial = sourceRuleSpecializations.size();
    do {
      privateName = "_pyc_owner_" + name.str() + "_" + std::to_string(serial++);
    } while (methods.contains(privateName) || lexicalNames.contains(privateName) ||
             this->names.contains(privateName) || structs.contains(privateName) || enums.contains(privateName));
    sourceRuleSpecializations[key] = privateName;
    auto &created = methods[privateName];
    created = specialized;
    created.inferredSignature = false;
    created.privateSpecialization = true;
    methodCaptures[privateName] = methodCaptures[name];
    if (queuedMethods.insert(privateName).second) methodWork.push_back(privateName);
    if (created.inferResults && failed(emitMethod(privateName))) return failure();
    return privateName;
  };

  auto emitStatements = [&](SourceCallableDecl &callable, OpBuilder &inside,
                            llvm::StringMap<Value> &values,
                            bool guard) -> FailureOr<SmallVector<Value>> {
    // Owner-bound Rule formals name the analyzed physical owner. Ordinary
    // Method/value parameters remain SSA, and saved read results stay snapshots.
    auto callableStates = callable.lexical ? states : llvm::StringMap<StateOwner>();
    for (const Port &port : callable.inputs) callableStates.erase(port.name);
    for (const auto &formal : callable.stateFormals)
      callableStates[formal.first()] = states.lookup(formal.second);
    auto &states = callableStates;
    auto callableInstances = callable.lexical ? instances : llvm::StringMap<Instance *>();
    auto &instances = callableInstances;
    auto boundHardwareIdentity = [&](StringRef name) {
      return names.contains(name) || structs.contains(name) || enums.contains(name) ||
             behavioralRules.contains(name) ||
             (callable.lexical && (methods.contains(name) || rules.contains(name) || instances.contains(name)));
    };
    SourceEffectContext context{&decl,      &callable, &states,
                                &instances, &methods,  guard};
    context.specialize = &specialize;
    auto *savedNestedScope = activeNestedRuleScope;
    if (!callable.lexical) activeNestedRuleScope = nullptr;
    auto restoreNestedScope = llvm::scope_exit([&] {
      activeNestedRuleScope = savedNestedScope;
    });
    auto *saved = activeSourceEffects;
    activeSourceEffects = &context;
    llvm::scope_exit restore([&] { activeSourceEffects = saved; });
    SmallVector<Value> yielded;
    if (guard) {
      AstNode expressionNode = callable.syntax.guard
                                   ? callable.syntax.guard.child("body")
                                   : callable.declaration;
      auto value = callable.syntax.guard
                       ? expression(expressionNode, inside, decl.symbol, values,
                                    instances)
                       : trueValue(expressionNode, inside, decl.symbol);
      if (failed(value))
        return error() << "guard expression must be pure and return a predicate";
      if (!callable.syntax.guard) {
        yielded.push_back(*value);
      } else {
        auto predicate = predicateView(*value, expressionNode, decl.symbol);
        if (failed(predicate))
          return error()
                 << "guard requires a Boolean or authoritative one-bit value";
        yielded.push_back(predicate->value);
      }
      return yielded;
    }
    AstNode returned;
    BranchEnvironment environment;
    environment.values.swap(values);
    auto restoreValues = llvm::scope_exit([&] {
      environment.values.swap(values);
    });
    // A Rule's explicit parameter boundary applies to every assignment, even
    // when the actual argument is an ordinary value rather than an owner.
    for (const Port &port : callable.inputs)
      if (!states.contains(port.name))
        environment.boundaries[port.name] = port.node.child("annotation")
            ? BindingBoundary{port, true}
            : inferredBoundary(port.name, environment.values.lookup(port.name), port.node, true);
    bool formalProposals = isa<ac::SourceMethodOp>(inside.getBlock()->getParentOp()) &&
                           callable.syntax.marker == MarkerKind::Rule;
    auto intentName = [](StringRef formal) { return ("$intent." + formal).str(); };
    auto writeSelectors = [&](const AstNode &target,
                              OpBuilder &at) -> FailureOr<ArrayAttr> {
      SmallVector<Attribute> selectors;
      AstNode current = target;
      while (current.kind() == "Attribute" || current.kind() == "Subscript") {
        TableQueryCharge charge;
        std::string reason;
        if (!TableQueryBudget::add(charge, TableQueryResource::Work, 1) ||
            !TableQueryBudget::add(charge, TableQueryResource::Slots, 4) ||
            !queryBudget.reserve(charge, reason))
          return error() << "selected write footprint: " << reason;
        if (current.kind() == "Attribute")
          selectors.push_back(at.getDictionaryAttr({at.getNamedAttr(
              "field", at.getStringAttr(current.string("attr")))}));
        else {
          if (current.child("value").kind() != "Name")
            return error() << "indexed assignment requires a named table";
          selectors.push_back(at.getDictionaryAttr(
              {at.getNamedAttr("index", at.getI64IntegerAttr(0))}));
        }
        current = current.child("value");
      }
      if (current.kind() != "Name")
        return error() << "selected write requires a named payload root";
      std::reverse(selectors.begin(), selectors.end());
      return at.getArrayAttr(selectors);
    };
    auto emitFormalWrite = [&](StringRef name, Value payload,
                               const AstNode &site, OpBuilder &at,
                               BranchEnvironment &environment,
                               Value active = Value(), ArrayAttr selectors = {},
                               ValueRange indices = {}) -> LogicalResult {
      if (!formalProposals || !callable.assignedFormals.contains(name) ||
          callable.stateFormals.contains(name))
        return success();
      auto formal = llvm::find_if(callable.inputs, [&](const Port &port) { return port.name == name; });
      if (formal == callable.inputs.end())
        return error() << "formal write lacks its parameter contract";
      auto path = active ? FailureOr<Value>(active) : trueValue(site, at, decl.symbol);
      if (failed(path))
        return failure();
      SmallVector<Value> operands{payload, *path,
                                  environment.values.lookup(name)};
      llvm::append_range(operands, indices);
      createOp(
          at, site.location(b.getContext(), source.path), "ac.formal.write",
          operands, {},
          {at.getNamedAttr("formal", at.getI64IntegerAttr(std::distance(
                                         callable.inputs.begin(), formal))),
           at.getNamedAttr("selectors",
                           selectors ? selectors : at.getArrayAttr({})),
           at.getNamedAttr(
               "operandSegmentSizes",
               at.getDenseI32ArrayAttr(
                   {1, 1, 1, static_cast<int32_t>(indices.size()), 0})),
           at.getNamedAttr("occurrence",
                           sourceOccurrence(at, decl.symbol, site)),
           at.getNamedAttr("ac.location", sourceSpan(at, source.path, site))});
      return success();
    };
    if (formalProposals)
      for (const Port &port : callable.inputs)
        if (callable.assignedFormals.contains(port.name) &&
            !callable.stateFormals.contains(port.name)) {
          auto zero = constant(internalBits(1, callable.declaration, decl.symbol), 0,
                               callable.declaration, inside, decl.symbol);
          if (failed(zero)) return failure();
          remember(*zero, ac::detail::ValueKind::Boolean);
          fixedValues.erase(*zero);
          environment.values[intentName(port.name)] = *zero;
        }
    auto statementError = [&](const AstNode &site) {
      diagnosticFailed = true;
      return mlir::emitError(site.location(b.getContext(), source.path));
    };
    std::function<LogicalResult(const AstNode &, StringRef, OpBuilder &,
                                BranchEnvironment &, bool)> emitBody;
    struct LoopControl { std::string live, continuing; };
    SmallVector<LoopControl> loopControls;
    unsigned loopOrdinal = 0;
    auto loopControl = [&](const AstNode &loop) {
      std::pair<bool, bool> control{false, false};
      auto scan = [&](auto &&self, const AstNode &node) -> void {
        if (node.kind() == "FunctionDef" || node.kind() == "Lambda") return;
        control.first |= node.kind() == "Break";
        control.second |= node.kind() == "Continue";
        bool nestedLoop = node.kind() == "For" || node.kind() == "While";
        for (StringRef childField : {"body", "orelse"}) {
          if (nestedLoop && childField == "body") continue;
          auto statements = node.array(childField);
          for (size_t k = 0; statements && k < statements.size(); ++k)
            self(self, node.item(childField, k));
        }
      };
      for (size_t k = 0; k < loop.array("body").size(); ++k)
        scan(scan, loop.item("body", k));
      return control;
    };
    auto emitLoopElse = [&](const AstNode &statement, OpBuilder &at,
                            BranchEnvironment &environment, bool inBranch,
                            Value live) -> LogicalResult {
      if (!live) return emitBody(statement, "orelse", at, environment, inBranch);
      if (statement.array("orelse").empty()) return success();
      std::string reason;
      if (!queryBudget.enter(reason))
        return statementError(statement) << "source loop-else branch: " << reason;
      auto leave = llvm::scope_exit([&] { queryBudget.leave(); });
      TableQueryCharge charge;
      bool fits = TableQueryBudget::add(charge, TableQueryResource::Occurrences, 1) &&
                  TableQueryBudget::add(charge, TableQueryResource::Operations, 4) &&
                  TableQueryBudget::add(charge, TableQueryResource::Slots, 12) &&
                  TableQueryBudget::add(charge, TableQueryResource::Work, 4);
      if (!fits || !queryBudget.reserve(charge, reason))
        return statementError(statement) << "source loop-else branch construction: "
               << (fits ? reason : "operation storage counter exhausted");
      auto predicate = predicateView(live, statement, decl.symbol);
      if (failed(predicate) || failed(reserveSourceEnvironmentCopies(statement, environment))) return failure();
      auto branch = cast<ac::SourceBranchOp>(createOp(at,
          statement.location(b.getContext(), source.path), ac::SourceBranchOp::getOperationName(),
          {live}, {},
          {at.getNamedAttr("occurrence", sourceOccurrence(at, decl.symbol, statement)),
           at.getNamedAttr("ac.condition_location", sourceSpan(at, source.path, statement)),
           at.getNamedAttr("ac.loop_continuation", at.getBoolAttr(true))}, 2));
      branch.getThenRegion().push_back(new Block());
      branch.getElseRegion().push_back(new Block());
      OpBuilder thenAt = OpBuilder::atBlockEnd(&branch.getThenRegion().front());
      BranchEnvironment yes = environment, no = environment;
      if (failed(emitBody(statement, "orelse", thenAt, yes, true))) return failure();
      auto joined = joinBranches(*predicate, environment, yes, no, statement, at, decl.symbol, branch);
      if (failed(joined)) return failure();
      environment = std::move(*joined);
      return success();
    };
    std::function<LogicalResult(const AstNode &, StringRef, OpBuilder &,
                                BranchEnvironment &, bool, size_t, bool)> emitRange;
    emitRange = [&](const AstNode &parent, StringRef field, OpBuilder &inside,
                   BranchEnvironment &environment,
                   bool inBranch, size_t start, bool guarded) -> LogicalResult {
      auto &values = environment.values;
      Value entryContinue = loopControls.empty() ? Value()
          : values.lookup(loopControls.back().continuing);
      for (size_t i = start; i < parent.array(field).size(); ++i) {
        AstNode statement = parent.item(field, i);
        if (docstring(statement))
          continue;
        if (!loopControls.empty()) {
          Value continuing = values.lookup(loopControls.back().continuing);
          auto known = continuing ? dyn_cast_or_null<BoolAttr>(valueInfo(continuing).closedSourceConstant) : BoolAttr();
          if (continuing && ((i == start && !guarded && !(known && known.getValue())) ||
                             (i != start && continuing != entryContinue))) {
            if (failed(reserveSourceEnvironmentCopies(statement, environment))) return failure();
            auto predicate = predicateView(continuing, statement, decl.symbol);
            if (failed(predicate)) return failure();
            auto branch = cast<ac::SourceBranchOp>(createOp(inside,
                statement.location(b.getContext(), source.path),
                ac::SourceBranchOp::getOperationName(), {continuing}, {},
                {inside.getNamedAttr("occurrence", sourceOccurrence(inside, decl.symbol, statement)),
                 inside.getNamedAttr("ac.condition_location", sourceSpan(inside, source.path, statement)),
                 inside.getNamedAttr("ac.loop_continuation", inside.getBoolAttr(true))}, 2));
            branch.getThenRegion().push_back(new Block());
            branch.getElseRegion().push_back(new Block());
            OpBuilder thenAt = OpBuilder::atBlockEnd(&branch.getThenRegion().front());
            BranchEnvironment yes = environment, no = environment;
            if (failed(emitRange(parent, field, thenAt, yes, inBranch, i, true))) return failure();
            auto joined = joinBranches(*predicate, environment, yes, no, statement,
                                        inside, decl.symbol, branch);
            if (failed(joined)) return failure();
            environment = std::move(*joined);
            return success();
          }
        }
        if (returned)
          return error() << "declaration after Method/Rule return is unsupported";
        if (statement.kind() == "Break" || statement.kind() == "Continue") {
          if (loopControls.empty()) return statementError(statement) << "loop control requires an enclosing loop";
          auto zero = constant(internalBits(1, statement, decl.symbol), 0, statement, inside, decl.symbol);
          if (failed(zero)) return failure();
          remember(*zero, ac::detail::ValueKind::Boolean);
          fixedValues.erase(*zero);
          remember(*zero, ac::detail::ValueKind::Boolean, std::nullopt, inside.getBoolAttr(false));
          values[loopControls.back().continuing] = *zero;
          if (statement.kind() == "Break") values[loopControls.back().live] = *zero;
          return success();
        }
        if (formalProposals) {
          auto markIntent = [&](const AstNode &target) -> LogicalResult {
            return forEachAssignmentLeaf(target, [&](const AstNode &leaf) -> LogicalResult {
              StringRef name = assignmentRoot(leaf);
              if (!callable.assignedFormals.contains(name) || callable.stateFormals.contains(name)) return success();
              auto enabled = trueValue(statement, inside, decl.symbol);
              if (failed(enabled)) return failure();
              remember(*enabled, ac::detail::ValueKind::Boolean);
              environment.values[intentName(name)] = *enabled;
              return success();
            });
          };
          if (statement.kind() == "Assign") {
            for (size_t k = 0; k < statement.array("targets").size(); ++k)
              if (failed(markIntent(statement.item("targets", k)))) return failure();
          } else if (statement.kind() == "AnnAssign" || statement.kind() == "AugAssign")
            if (failed(markIntent(statement.child("target")))) return failure();
        }
        if (statement.kind() == "Match") {
          llvm::StringSet<> owners;
          auto arm = [&](const AstNode &site, OpBuilder &at,
                          BranchEnvironment &branchEnvironment) {
            return emitRange(site, "body", at, branchEnvironment, true, 0, true);
          };
          if (failed(matchStatement(statement, inside, decl.symbol, environment,
                                     owners, arm, &instances))) return failure();
          continue;
        }
        if (statement.kind() == "If") {
          std::string reason;
          if (!queryBudget.enter(reason))
            return mlir::emitError(statement.location(b.getContext(), source.path))
                   << "source branch: " << reason;
          auto leave = llvm::scope_exit([&] { queryBudget.leave(); });
          if (failed(chargeLoopDescriptorWork(statement,
                                              "source branch statement")))
            return failure();
          // Statement arms admit effects; evaluating their selector remains
          // pure, independently of whether this statement is in another arm.
          auto test = [&]() {
            bool savedPureExpression =
                std::exchange(context.pureExpression, true);
            auto restorePureExpression = llvm::scope_exit([&] {
              context.pureExpression = savedPureExpression;
            });
            return expression(statement.child("test"), inside, decl.symbol,
                              values, instances);
          }();
          if (failed(test))
            return failure();
          auto predicate = predicateView(*test, statement.child("test"), decl.symbol);
          if (failed(predicate))
            return failure();
          // Seed capture boundaries before any environment copies. Captures
          // retain their original authority, including unchanged aggregates.
          for (const auto &binding : values) {
            if (environment.boundaries.contains(binding.first()))
              continue;
            TableQueryCharge charge;
            bool fits = TableQueryBudget::add(charge, TableQueryResource::Slots, 16) &&
                        TableQueryBudget::add(charge, TableQueryResource::Work, 2) &&
                        TableQueryBudget::addProduct(
                            charge, TableQueryResource::ConstantBytes, 2,
                            binding.first().size());
            for (const AstStep &step : statement.path)
              fits &= TableQueryBudget::add(charge, TableQueryResource::Slots, 8) &&
                      TableQueryBudget::add(charge, TableQueryResource::Work, 1) &&
                      TableQueryBudget::add(charge, TableQueryResource::ConstantBytes,
                                            step.field.size());
            if (!fits || !queryBudget.reserve(charge, reason))
              return mlir::emitError(statement.location(b.getContext(), source.path))
                     << "source branch capture binding: "
                     << (fits ? reason : "binding storage counter exhausted");
            environment.boundaries[binding.first()] =
                inferredBoundary(binding.first(), binding.second, statement);
          }
          if (failed(reserveSourceEnvironmentCopies(statement, environment)))
            return failure();
          BranchEnvironment yes = environment, no = environment;
          TableQueryCharge charge;
          bool fits = TableQueryBudget::add(charge, TableQueryResource::Occurrences, 1) &&
                      TableQueryBudget::add(charge, TableQueryResource::Operations, 4) &&
                      TableQueryBudget::add(charge, TableQueryResource::Slots, 12) &&
                      TableQueryBudget::add(charge, TableQueryResource::Work, 4);
          if (!fits || !queryBudget.reserve(charge, reason))
            return mlir::emitError(statement.location(b.getContext(), source.path))
                   << "source branch construction: "
                   << (fits ? reason : "operation storage counter exhausted");
          auto branch = cast<ac::SourceBranchOp>(createOp(
              inside, statement.location(b.getContext(), source.path),
              ac::SourceBranchOp::getOperationName(), {predicate->value}, {},
              {inside.getNamedAttr("occurrence",
                                   sourceOccurrence(inside, decl.symbol, statement)),
               inside.getNamedAttr("ac.condition_location",
                                   sourceSpan(inside, source.path,
                                              statement.child("test")))}, 2));
          branch.getThenRegion().push_back(new Block());
          branch.getElseRegion().push_back(new Block());
          OpBuilder thenAt = OpBuilder::atBlockEnd(&branch.getThenRegion().front());
          OpBuilder elseAt = OpBuilder::atBlockEnd(&branch.getElseRegion().front());
          // Reaching this statement already establishes the enclosing loop's
          // continuation. The authored selector controls its two arms; adding
          // the same continuation guard inside each arm duplicates proof IR.
          if (failed(emitRange(statement, "body", thenAt, yes, true, 0, true)) ||
              failed(emitRange(statement, "orelse", elseAt, no, true, 0, true)) ||
              failed(reserveSourceEnvironmentCopies(statement, environment, &yes, &no)))
            return failure();
          auto joined = joinBranches(*predicate, environment, yes, no, statement,
                                      inside, decl.symbol, branch);
          if (failed(joined))
            return failure();
          environment = std::move(*joined);
          continue;
        }
        if (statement.kind() == "For") {
          AstNode target = statement.child("target"), range = statement.child("iter");
          if (target.kind() != "Name" || range.kind() != "Call" ||
              range.child("func").kind() != "Name" || range.child("func").string("id") != "range" ||
              lexicallyShadowed(range.child("func"), "range") || !range.array("keywords").empty() ||
              range.array("args").empty() || range.array("args").size() > 3)
            return statementError(statement) << "typed for requires an ordinary Name and finite range";
          StringRef inductionName = target.string("id");
          bool stateInduction = states.contains(inductionName);
          bool formalInduction = formalProposals && callable.assignedFormals.contains(inductionName) &&
                                 !stateInduction;
          if (boundHardwareIdentity(inductionName) ||
              helperOwnerBindings.contains(inductionName) ||
              llvm::is_contained(ArrayRef<StringRef>{"pyc_clk", "pyc_rst", "range", "int", "bool", "tuple", "Annotated"}, inductionName))
            return statementError(target) << "typed for induction requires an assignable ordinary Name or state owner";
          auto exactBound = [&](const AstNode &node) -> FailureOr<int64_t> {
            bool savedPure = std::exchange(context.pureExpression, true);
            auto restorePure = llvm::scope_exit([&] { context.pureExpression = savedPure; });
            auto value = expression(node, inside, decl.symbol, values, instances);
            if (failed(value)) return failure();
            auto info = valueInfo(*value);
            auto integer = dyn_cast_or_null<ac::MathIntAttr>(info.closedSourceConstant);
            if (info.sourceKind != ac::detail::ValueKind::Integer || fixedValues.contains(*value) || !integer)
              return statementError(node) << "typed for range bounds require closed Integer values";
            llvm::APSInt number(integer.getCanonicalValue());
            if ((number.isUnsigned() && number.getActiveBits() > 63) ||
                (!number.isUnsigned() && !number.isSignedIntN(64)))
              return statementError(node) << "typed for range exceeds signed index representation";
            return number.isUnsigned() ? static_cast<int64_t>(number.getZExtValue()) : number.getSExtValue();
          };
          auto hasNestedFor = [&](auto &&self, const AstNode &node) -> bool {
            if (node.kind() == "FunctionDef" || node.kind() == "Lambda")
              return false;
            if (node.kind() == "For")
              return true;
            for (StringRef field : {"body", "orelse"}) {
              auto statements = node.array(field);
              for (size_t k = 0; statements && k < statements.size(); ++k)
                if (self(self, node.item(field, k)))
                  return true;
            }
            return false;
          };
          for (StringRef field : {"body", "orelse"})
            for (size_t k = 0; k < statement.array(field).size(); ++k)
              if (hasNestedFor(hasNestedFor, statement.item(field, k)))
                return statementError(statement) << "nested source for loops are unsupported";
          // The newly admitted else suite retains the bounded For profile;
          // existing finite While in a For body or after the For is unchanged.
          auto hasElseLoop = [&](auto &&self, const AstNode &node) -> bool {
            if (node.kind() == "FunctionDef" || node.kind() == "Lambda")
              return false;
            if (node.kind() == "For" || node.kind() == "While")
              return true;
            for (NamedAttribute field : node.fields()) {
              if (auto child = dyn_cast<DictionaryAttr>(field.getValue())) {
                if (child.getAs<StringAttr>("kind") &&
                    self(self, node.child(field.getName().getValue())))
                  return true;
              } else if (auto children = dyn_cast<ArrayAttr>(field.getValue()))
                for (size_t k = 0; k < children.size(); ++k)
                  if (auto child = dyn_cast<DictionaryAttr>(children[k]);
                      child && child.getAs<StringAttr>("kind") &&
                      self(self, node.item(field.getName().getValue(), k)))
                    return true;
            }
            return false;
          };
          for (size_t k = 0; k < statement.array("orelse").size(); ++k)
            if (hasElseLoop(hasElseLoop, statement.item("orelse", k)))
              return statementError(statement) << "nested source loop in for else is unsupported";
          size_t arity = range.array("args").size();
          if (arity != 1)
            return statementError(range) << "typed for range requires a single closed Integer bound";
          int64_t lower = 0, upper, step = 1;
          auto first = exactBound(range.item("args", 0));
          if (failed(first)) return failure();
          if (*first < 0)
            return statementError(range) << "typed for range requires a nonnegative closed Integer bound";
          if (arity == 1) upper = *first;
          else {
            lower = *first;
            auto last = exactBound(range.item("args", 1));
            if (failed(last)) return failure();
            upper = *last;
          }
          if (arity == 3) {
            auto increment = exactBound(range.item("args", 2));
            if (failed(increment)) return failure();
            step = *increment;
          }
          if (step <= 0) return statementError(statement) << "typed for range requires a positive step";
          llvm::APInt lo(128, static_cast<uint64_t>(lower), true),
              hi(128, static_cast<uint64_t>(upper), true), increment(128, step);
          llvm::APInt count(128, 0);
          if (lo.slt(hi)) count = (hi - lo + increment - 1).udiv(increment);
          if (count.getActiveBits() > 64)
            return statementError(statement) << "typed for range is not finitely representable";
          uint64_t trips = count.getZExtValue();
          // Natural zero-trip completion runs else in the incoming environment;
          // no induction binding or dead body reference is constructed.
          if (trips == 0) {
            if (failed(emitLoopElse(statement, inside, environment, inBranch, {})))
              return failure();
            continue;
          }
          llvm::APInt last = trips ? lo + increment * (trips - 1) : lo;
          bool signedIndex = lower < 0;
          unsigned indexWidth = std::max(1u, signedIndex
              ? std::max(lo.getSignificantBits(), last.getSignificantBits())
              : std::max(lo.getActiveBits(), last.getActiveBits()));
          Type indexType = internalBits(indexWidth, target, decl.symbol);
          auto controlKinds = loopControl(statement);
          std::optional<LoopControl> control;
          if (controlKinds.first || controlKinds.second) {
            std::string suffix = std::to_string(loopOrdinal++);
            control = LoopControl{"$loop.live." + suffix, "$loop.continue." + suffix};
            auto live = trueValue(statement, inside, decl.symbol);
            if (failed(live)) return failure();
            remember(*live, ac::detail::ValueKind::Boolean);
            values[control->live] = *live;
            if (controlKinds.first && trips && !stateInduction) {
              auto boundary = environment.boundaries.find(inductionName);
              if (boundary == environment.boundaries.end() || !boundary->second.declared ||
                  !values.contains(inductionName)) {
                auto seed = constant(indexType, 0, statement, inside, decl.symbol);
                if (failed(seed)) return failure();
                remember(*seed, ac::detail::ValueKind::Integer);
                fixedValues.erase(*seed);
                values[inductionName] = *seed;
              }
            }
          }
          llvm::StringSet<> assigned;
          auto assignments = [&](auto &&self, const AstNode &node) -> void {
            if (node.kind() == "FunctionDef" || node.kind() == "Lambda") return;
            auto mark = [&](const AstNode &target) {
              (void)forEachAssignmentLeaf(target, [&](const AstNode &leaf) {
                assigned.insert(assignmentRoot(leaf));
                return success();
              });
            };
            if (node.kind() == "Assign")
              for (size_t k = 0; k < node.array("targets").size(); ++k)
                mark(node.item("targets", k));
            else if (node.kind() == "AnnAssign" || node.kind() == "AugAssign" || node.kind() == "For")
              mark(node.child("target"));
            for (StringRef field : {"body", "orelse"}) {
              auto statements = node.array(field);
              for (size_t k = 0; statements && k < statements.size(); ++k)
                self(self, node.item(field, k));
            }
          };
          for (size_t k = 0; k < statement.array("body").size(); ++k)
            assignments(assignments, statement.item("body", k));
          SmallVector<std::string> carriedNames;
          for (const auto &name : assigned)
            if (name.getKey() != inductionName && values.contains(name.getKey()) &&
                !states.contains(name.getKey())) {
              carriedNames.push_back(name.getKey().str());
              if (formalProposals && callable.assignedFormals.contains(name.getKey()) &&
                  !callable.stateFormals.contains(name.getKey()))
                carriedNames.push_back(intentName(name.getKey()));
            }
          llvm::sort(carriedNames);
          if (control) carriedNames.push_back(control->live);
          if (controlKinds.first && trips && !stateInduction) carriedNames.push_back(inductionName.str());
          if (formalInduction) carriedNames.push_back(intentName(inductionName));
          auto location = statement.location(b.getContext(), source.path);
          SmallVector<Value> operands;
          for (int64_t bound : {lower, upper, step})
            operands.push_back(arith::ConstantIndexOp::create(inside, location, bound));
          SmallVector<Type> resultTypes;
          for (const auto &name : carriedNames) {
            operands.push_back(values.lookup(name));
            resultTypes.push_back(values.lookup(name).getType());
          }
          auto occurrence = sourceOccurrence(inside, decl.symbol, statement);
          Operation *loop = createOp(inside, location, scf::ForOp::getOperationName(),
              operands, resultTypes, {inside.getNamedAttr("occurrence", occurrence)}, 1);
          auto *block = new Block();
          loop->getRegion(0).push_back(block);
          Value index = block->addArgument(inside.getIndexType(), location);
          if (failed(chargeLoopDescriptorWork(statement, "source for statement")) ||
              failed(reserveSourceEnvironmentCopies(statement, environment)))
            return failure();
          BranchEnvironment iteration = environment;
          for (const auto &name : carriedNames) {
            Value original = values.lookup(name);
            auto kind = valueInfo(original).sourceKind;
            if (kind == ac::detail::ValueKind::Integer && name != inductionName) {
              auto boundary = iteration.boundaries.find(name);
              if (boundary == iteration.boundaries.end() || !boundary->second.declared ||
                  boundary->second.integerMode != IntegerBoundaryMode::Range) {
                auto formal = llvm::find_if(callable.inputs, [&](const Port &port) {
                  return port.name == name && port.sourceKind == kind;
                });
                if (formal == callable.inputs.end())
                  return statementError(statement) << "Integer loop carries require an explicit bounded annotation";
                iteration.boundaries[name] = BindingBoundary{*formal, true};
              }
            }
            Value argument = block->addArgument(original.getType(), location);
            remember(argument, kind);
            if (fixedValues.contains(original)) fixedValues.insert(argument);
            iteration.values[name] = argument;
            if (!iteration.boundaries.contains(name))
              iteration.boundaries[name] = inferredBoundary(name, argument, statement, true);
          }
          OpBuilder loopAt = OpBuilder::atBlockEnd(block);
          Value induction = createOp(loopAt, location, ac::SourceIndexOp::getOperationName(),
              {index}, {indexType},
              {loopAt.getNamedAttr("occurrence", sourceOccurrence(loopAt, decl.symbol, target)),
               loopAt.getNamedAttr("source_location", sourceSpan(loopAt, source.path, target)),
               loopAt.getNamedAttr("is_signed", loopAt.getBoolAttr(signedIndex))})->getResult(0);
          remember(induction, ac::detail::ValueKind::Integer,
              IntegerInterval{llvm::APSInt(lo, false), llvm::APSInt(last + 1, false)});
          if (auto boundary = iteration.boundaries.find(inductionName);
              boundary != iteration.boundaries.end()) {
            auto converted = applyBindingBoundary(induction, boundary->second,
                                                   target, loopAt, decl.symbol);
            if (failed(converted)) return failure();
            induction = *converted;
          }
          if (stateInduction) {
            auto converted = applyBindingBoundary(induction, ownerBoundary(states[inductionName]),
                                                   target, loopAt, decl.symbol);
            if (failed(converted)) return failure();
            induction = *converted;
            auto path = control ? FailureOr<Value>(iteration.values.lookup(control->live))
                                : trueValue(statement, loopAt, decl.symbol);
            if (failed(path)) return failure();
            createOp(loopAt, location, ac::SourceStateWriteOp::getOperationName(), {*path, induction}, {},
                {loopAt.getNamedAttr("state", loopAt.getStringAttr(states[inductionName].name)),
                 loopAt.getNamedAttr("selectors", loopAt.getArrayAttr({})),
                 loopAt.getNamedAttr("occurrence", sourceOccurrence(loopAt, decl.symbol, target))});
          } else if (controlKinds.first && trips) {
            auto facts = valueInfo(induction);
            Value prior = iteration.values.lookup(inductionName);
            if (induction.getType() != prior.getType() &&
                ac::areEquivalentHardwareTypes(induction.getType(), prior.getType()))
              induction = boundaryBitsIdentity(induction, prior.getType(), target, loopAt, decl.symbol);
            auto selected = select(iteration.values.lookup(control->live), induction, prior,
                                   target, loopAt, decl.symbol);
            if (failed(selected)) return failure();
            induction = *selected;
            remember(induction, facts.sourceKind, facts.interval);
          }
          iteration.values[inductionName] = induction;
          if (formalInduction) {
            auto active = control ? FailureOr<Value>(iteration.values.lookup(control->live))
                                  : trueValue(statement, loopAt, decl.symbol);
            if (failed(active)) return failure();
            remember(*active, ac::detail::ValueKind::Boolean);
            auto key = intentName(inductionName);
            Value prior = iteration.values.lookup(key);
            Value intent = createOp(loopAt, location, ac::BitsBinaryOp::getOperationName(),
                {prior, *active}, {prior.getType()},
                {loopAt.getNamedAttr("opcode", loopAt.getStringAttr("or"))})->getResult(0);
            remember(intent, ac::detail::ValueKind::Boolean);
            iteration.values[key] = intent;
            if (failed(emitFormalWrite(inductionName, induction, target, loopAt, iteration, *active))) return failure();
          }
          if (control) {
            iteration.values[control->continuing] = iteration.values.lookup(control->live);
            loopControls.push_back(*control);
          }
          auto emitted = emitBody(statement, "body", loopAt, iteration, true);
          if (control) {
            loopControls.pop_back();
            iteration.values.erase(control->continuing);
            iteration.boundaries.erase(control->continuing);
          }
          if (failed(emitted)) return failure();
          if (stateInduction) iteration.values.erase(inductionName);
          SmallVector<Value> yielded;
          for (const auto &name : carriedNames) {
            Value value = iteration.values.lookup(name);
            Type originalType = values.lookup(name).getType();
            if (value && value.getType() != originalType &&
                isa<ac::BitsType>(value.getType()) &&
                isa<ac::BitsType>(originalType) &&
                ac::areEquivalentHardwareTypes(value.getType(), originalType)) {
              value = boundaryBitsIdentity(value, originalType, statement,
                                            loopAt, decl.symbol);
            }
            if (!value || value.getType() != values.lookup(name).getType())
              return statementError(statement) << "typed for carried values must retain their declared hardware type";
            yielded.push_back(value);
          }
          if (trips) {
            SmallVector<std::string> introduced;
            for (const auto &binding : iteration.values)
              if ((binding.first() == inductionName && !controlKinds.first) || !values.contains(binding.first()))
                introduced.push_back(binding.first().str());
            llvm::sort(introduced);
            OpBuilder seeds(loop);
            for (const auto &name : introduced) {
              Value value = iteration.values.lookup(name);
              auto seed = inactiveCarrier(value.getType(), statement, seeds, decl.symbol);
              if (failed(seed)) return failure();
              operands.push_back(*seed);
              resultTypes.push_back(value.getType());
              block->addArgument(value.getType(), location);
              carriedNames.push_back(name);
              yielded.push_back(value);
            }
          }
          createOp(loopAt, location, scf::YieldOp::getOperationName(), yielded, {}, {});
          Operation *complete = createOp(inside, location, scf::ForOp::getOperationName(),
              operands, resultTypes, {inside.getNamedAttr("occurrence", occurrence)}, 1);
          complete->getRegion(0).takeBody(loop->getRegion(0));
          loop->erase();
          for (auto [ordinal, name] : llvm::enumerate(carriedNames)) {
            Value result = complete->getResult(ordinal);
            Value authority = trips ? yielded[ordinal] : values.lookup(name);
            auto facts = valueInfo(authority);
            remember(result, facts.sourceKind, facts.interval,
                     trips ? Attribute() : facts.closedSourceConstant);
            if (fixedValues.contains(authority)) fixedValues.insert(result);
            bool introduced = !values.contains(name);
            values[name] = result;
            if (introduced && iteration.boundaries.contains(name) &&
                iteration.boundaries.lookup(name).declared)
              environment.boundaries[name] = iteration.boundaries.lookup(name);
          }
          if (trips && !assigned.contains(inductionName) && !controlKinds.first && !stateInduction) {
            Value final = values.lookup(inductionName);
            remember(final, valueInfo(final).sourceKind,
                IntegerInterval{llvm::APSInt(last, false), llvm::APSInt(last + 1, false)},
                ac::MathIntAttr::get(b.getContext(), llvm::APSInt(last, false)));
          }
          Value live = control && controlKinds.first
                           ? values.lookup(control->live) : Value();
          if (control) {
            values.erase(control->live);
            environment.boundaries.erase(control->live);
          }
          if (failed(emitLoopElse(statement, inside, environment, inBranch, live))) return failure();
          continue;
        }
        if (statement.kind() == "While") {
          if (failed(chargeLoopDescriptorWork(statement, "source while statement")) ||
              failed(reserveSourceEnvironmentCopies(statement, environment)))
            return failure();
          auto controlKinds = loopControl(statement);
          std::optional<LoopControl> control;
          if (controlKinds.first || controlKinds.second) {
            std::string suffix = std::to_string(loopOrdinal++);
            control = LoopControl{"$loop.live." + suffix, "$loop.continue." + suffix};
            auto live = trueValue(statement, inside, decl.symbol);
            if (failed(live)) return failure();
            remember(*live, ac::detail::ValueKind::Boolean);
            values[control->live] = *live;
          }
          llvm::StringSet<> assigned;
          auto assignments = [&](auto &&self, const AstNode &node) -> void {
            if (node.kind() == "FunctionDef" || node.kind() == "Lambda") return;
            auto mark = [&](const AstNode &target) {
              (void)forEachAssignmentLeaf(target, [&](const AstNode &leaf) {
                assigned.insert(assignmentRoot(leaf));
                return success();
              });
            };
            if (node.kind() == "Assign")
              for (size_t k = 0; k < node.array("targets").size(); ++k)
                mark(node.item("targets", k));
            else if (node.kind() == "AnnAssign" || node.kind() == "AugAssign" || node.kind() == "For")
              mark(node.child("target"));
            for (StringRef childField : {"body", "orelse"}) {
              auto statements = node.array(childField);
              for (size_t k = 0; statements && k < statements.size(); ++k)
                self(self, node.item(childField, k));
            }
          };
          for (size_t k = 0; k < statement.array("body").size(); ++k)
            assignments(assignments, statement.item("body", k));
          SmallVector<std::string> carriedNames;
          for (const auto &name : assigned)
            if (values.contains(name.getKey()) && !states.contains(name.getKey())) {
              carriedNames.push_back(name.getKey().str());
              if (formalProposals && callable.assignedFormals.contains(name.getKey()) &&
                  !callable.stateFormals.contains(name.getKey()))
                carriedNames.push_back(intentName(name.getKey()));
            }
          llvm::sort(carriedNames);
          if (control) carriedNames.push_back(control->live);
          SmallVector<Value> operands;
          SmallVector<Type> resultTypes;
          for (const auto &name : carriedNames) {
            operands.push_back(values.lookup(name));
            resultTypes.push_back(values.lookup(name).getType());
          }
          auto location = statement.location(b.getContext(), source.path);
          Operation *loop = createOp(inside, location, scf::WhileOp::getOperationName(),
              operands, resultTypes,
              {inside.getNamedAttr("occurrence", sourceOccurrence(inside, decl.symbol, statement)),
               inside.getNamedAttr("ac.condition_location",
                                   sourceSpan(inside, source.path, statement.child("test")))}, 2);
          BranchEnvironment conditionEnvironment = environment;
          BranchEnvironment iteration = environment;
          auto *before = new Block(), *after = new Block();
          loop->getRegion(0).push_back(before);
          loop->getRegion(1).push_back(after);
          for (const auto &name : carriedNames) {
            Value original = values.lookup(name);
            auto kind = valueInfo(original).sourceKind;
            if (kind == ac::detail::ValueKind::Integer) {
              auto boundary = environment.boundaries.find(name);
              if (boundary == environment.boundaries.end() || !boundary->second.declared ||
                  boundary->second.integerMode != IntegerBoundaryMode::Range) {
                auto formal = llvm::find_if(callable.inputs, [&](const Port &port) {
                  return port.name == name && port.sourceKind == kind;
                });
                if (formal == callable.inputs.end())
                  return statementError(statement) << "Integer loop carries require an explicit bounded annotation";
                conditionEnvironment.boundaries[name] = BindingBoundary{*formal, true};
                iteration.boundaries[name] = BindingBoundary{*formal, true};
              }
            }
            for (auto [block, local] : {std::pair{before, &conditionEnvironment},
                                       std::pair{after, &iteration}}) {
              Value argument = block->addArgument(original.getType(), location);
              remember(argument, kind);
              if (fixedValues.contains(original)) fixedValues.insert(argument);
              local->values[name] = argument;
              if (!local->boundaries.contains(name))
                local->boundaries[name] = inferredBoundary(name, argument, statement, true);
            }
          }
          OpBuilder conditionAt = OpBuilder::atBlockEnd(before);
          auto test = [&]() {
            bool savedPure = std::exchange(context.pureExpression, true);
            auto restorePure = llvm::scope_exit([&] { context.pureExpression = savedPure; });
            return expression(statement.child("test"), conditionAt, decl.symbol,
                              conditionEnvironment.values, instances);
          }();
          if (failed(test)) return failure();
          auto predicate = predicateView(*test, statement.child("test"), decl.symbol);
          if (failed(predicate)) return failure();
          Value continuation = predicate->value;
          if (control) {
            auto live = checkAnd(continuation, conditionEnvironment.values.lookup(control->live),
                                  statement, conditionAt, decl.symbol);
            if (failed(live)) return failure();
            continuation = *live;
          }
          Value transported = createOp(conditionAt, location,
              ac::SourcePredicateOp::getOperationName(), {continuation},
              {conditionAt.getI1Type()}, {})->getResult(0);
          SmallVector<Value> forwarded{transported};
          llvm::append_range(forwarded, before->getArguments());
          createOp(conditionAt, location, scf::ConditionOp::getOperationName(), forwarded, {}, {});
          OpBuilder loopAt = OpBuilder::atBlockEnd(after);
          if (control) {
            auto continuing = trueValue(statement, loopAt, decl.symbol);
            if (failed(continuing)) return failure();
            remember(*continuing, ac::detail::ValueKind::Boolean, std::nullopt,
                     loopAt.getBoolAttr(true));
            iteration.values[control->continuing] = *continuing;
            loopControls.push_back(*control);
          }
          auto emitted = emitBody(statement, "body", loopAt, iteration, true);
          if (control) {
            loopControls.pop_back();
            iteration.values.erase(control->continuing);
            iteration.boundaries.erase(control->continuing);
          }
          if (failed(emitted)) return failure();
          SmallVector<Value> nextValues;
          for (const auto &name : carriedNames) {
            Value next = iteration.values.lookup(name);
            Type originalType = values.lookup(name).getType();
            if (next && next.getType() != originalType &&
                isa<ac::BitsType>(next.getType()) && isa<ac::BitsType>(originalType) &&
                ac::areEquivalentHardwareTypes(next.getType(), originalType)) {
              next = boundaryBitsIdentity(next, originalType, statement,
                                           loopAt, decl.symbol);
            }
            if (!next || next.getType() != originalType)
              return statementError(statement) << "typed while carried values must retain their declared hardware type";
            nextValues.push_back(next);
          }
          createOp(loopAt, location, scf::YieldOp::getOperationName(), nextValues, {}, {});
          for (auto [ordinal, name] : llvm::enumerate(carriedNames)) {
            Value original = values.lookup(name), result = loop->getResult(ordinal);
            remember(result, valueInfo(original).sourceKind);
            if (fixedValues.contains(original)) fixedValues.insert(result);
            values[name] = result;
          }
          Value live = control && controlKinds.first
                           ? values.lookup(control->live) : Value();
          if (control) {
            values.erase(control->live);
            environment.boundaries.erase(control->live);
          }
          if (failed(emitLoopElse(statement, inside, environment, inBranch, live))) return failure();
          continue;
        }
        if (statement.kind() == "FunctionDef")
          return error() << "nested Rule/Method declaration is unsupported";
        if (statement.kind() == "Pass")
          continue;
        if (statement.kind() == "Return") {
          if (inBranch)
            return statementError(statement) << "return in a Method/Rule branch is unsupported";
          returned = statement;
          AstNode valueNode = statement.child("value");
          bool none = !valueNode || (valueNode.kind() == "Constant" &&
                                     isa<UnitAttr>(valueNode.get("value")));
          if (callable.inferResults) {
            callable.results.clear();
            callable.resultContracts.clear();
            callable.returnShape = none ? ReturnShape::None :
                valueNode.kind() == "Tuple" ? ReturnShape::Tuple : ReturnShape::Struct;
            if (valueNode.kind() == "Dict")
              return error() << "unannotated named mapping return requires its enclosing Module result context";
            size_t count = none ? 0 : callable.returnShape == ReturnShape::Tuple
                ? valueNode.array("elts").size() : 1;
            if (!none && callable.returnShape == ReturnShape::Tuple && !count)
              return error() << "inferred tuple Rule return requires at least one finite result";
            for (size_t index = 0; index < count; ++index) {
              auto element = callable.returnShape == ReturnShape::Tuple
                  ? valueNode.item("elts", index) : valueNode;
              auto value = expression(element, inside, decl.symbol, values, instances);
              if (failed(value)) return failure();
              auto binding = inferredBoundary("result", *value, element, true);
              binding.unsharedContract().name = count == 1 ? "result" : "result_" + std::to_string(index);
              if (failed(sourceConstraint(binding.contract()))) return failure();
              callable.resultContracts.push_back(binding.contract());
              callable.results.push_back(value->getType());
              yielded.push_back(*value);
            }
            callable.inferResults = false;
            callable.type = FunctionType::get(b.getContext(), callable.type.getInputs(), callable.results);
            continue;
          }
          if (callable.results.empty()) {
            if (!none) return error() << "None Method/Rule cannot return a value";
          } else {
            if (none) return error() << "Method/Rule must return its declared results";
            bool tuple = callable.returnShape == ReturnShape::Tuple;
            bool mapping = callable.returnShape == ReturnShape::Mapping;
            if (mapping && (valueNode.kind() != "Dict" ||
                valueNode.array("keys").size() != callable.results.size() ||
                valueNode.array("values").size() != callable.results.size()))
              return error() << "named Rule return requires the enclosing Module result mapping";
            if (tuple && (valueNode.kind() != "Tuple" ||
                valueNode.array("elts").size() != callable.results.size()))
              return error() << "tuple Method/Rule return requires explicit Tuple with matching arity";
            for (auto [index, contract] : llvm::enumerate(callable.resultContracts)) {
              AstNode element = tuple ? valueNode.item("elts", index) :
                  mapping ? valueNode.item("values", index) : valueNode;
              if (mapping) {
                auto key = dyn_cast_or_null<StringAttr>(valueNode.item("keys", index).get("value"));
                if (!key || key.getValue() != contract.name)
                  return error() << "named Rule return key disagrees with its result contract";
              }
              auto value = expression(element, inside, decl.symbol, values,
                                      instances, contract.type);
              if (failed(value)) return failure();
              auto converted = applyBindingBoundary(*value,
                  BindingBoundary{contract, true}, element, inside, decl.symbol);
              if (failed(converted)) return failure();
              if (converted->getType() != contract.type &&
                  isa<ac::BitsType>(converted->getType()) &&
                  isa<ac::BitsType>(contract.type) &&
                  ac::areEquivalentHardwareTypes(converted->getType(), contract.type))
                converted = boundaryBitsIdentity(*converted, contract.type,
                                                 element, inside, decl.symbol);
              yielded.push_back(*converted);
            }
          }
          continue;
        }
        if (statement.kind() == "Assert") {
          auto message = sourceAssertMessage(statement);
          if (failed(message))
            return failure();
          auto condition = expression(statement.child("test"), inside,
                                      decl.symbol, values, instances);
          if (failed(condition))
            return failure();
          auto predicate =
              predicateView(*condition, statement.child("test"), decl.symbol);
          auto path = trueValue(statement, inside, decl.symbol);
          if (failed(predicate) || failed(path))
            return failure();
          SmallVector<NamedAttribute> attrs{
              inside.getNamedAttr("kind", inside.getStringAttr("assert")),
              inside.getNamedAttr("location",
                                  sourceSpan(inside, source.path, statement)),
              inside.getNamedAttr(
                  "occurrence",
                  sourceOccurrence(inside, decl.symbol, statement))};
          if (*message)
            attrs.push_back(inside.getNamedAttr("ac.message", *message));
          createOp(inside, statement.location(b.getContext(), source.path),
                   ac::SourceExpectOp::getOperationName(),
                   ValueRange{predicate->value, *path}, TypeRange{}, attrs);
          continue;
        }
        if (statement.kind() == "Expr") {
          SmallVector<AstNode, 4> valueItems;
          DictionaryAttr spec;
          StringRef observationKind;
          AstNode expressionNode = statement.child("value");
          bool observation =
              expressionNode.kind() == "Call" &&
              (intrinsic(expressionNode.child("func"), "log") ||
               intrinsic(expressionNode.child("func"), "report"));
          if (observation) {
            if (failed(buildObservation(statement, valueItems, spec,
                                        observationKind)))
              return failure();
            auto path = trueValue(statement, inside, decl.symbol);
            if (failed(path))
              return failure();
            SmallVector<Value> operands{*path};
            for (const AstNode &item : valueItems) {
              auto value =
                  expression(item, inside, decl.symbol, values, instances);
              if (failed(value))
                return failure();
              operands.push_back(*value);
            }
            createOp(
                inside, statement.location(b.getContext(), source.path),
                ac::SourceObserveOp::getOperationName(), operands, TypeRange{},
                {inside.getNamedAttr("kind",
                                     inside.getStringAttr(observationKind)),
                 inside.getNamedAttr("spec", spec),
                 inside.getNamedAttr(
                     "occurrence",
                     sourceOccurrence(inside, decl.symbol, statement)),
                 inside.getNamedAttr("ac.location",
                                     sourceSpan(inside, source.path, statement))});
            continue;
          }
        }
        AstNode target, rhs;
        if (statement.kind() == "Assign" &&
            statement.array("targets").size() == 1) {
          target = statement.item("targets", 0);
          rhs = statement.child("value");
        } else if (statement.kind() == "AnnAssign") {
          target = statement.child("target");
          rhs = statement.child("value");
        } else if (statement.kind() == "Expr") {
          AstNode expressionNode = statement.child("value");
          if (expressionNode.kind() == "Call" &&
              expressionNode.child("func").kind() == "Name" &&
              rules.contains(expressionNode.child("func").string("id")) &&
              !methods.contains(expressionNode.child("func").string("id")))
            return error() << "nested Rule calls are unsupported";
          auto helper = expressionNode.kind() == "Call" ? resolveValueHelper(expressionNode.child("func"), decl.symbol)
                                                      : FailureOr<ValueHelperDecl *>(static_cast<ValueHelperDecl *>(nullptr));
          if (failed(helper)) return failure();
          if (*helper) {
            if (failed(valueHelperCall(expressionNode, inside, decl.symbol, values, instances))) return failure();
            continue;
          }
          auto results = sourceMethodCall(statement.child("value"), inside,
                                          decl.symbol, values, instances, std::nullopt);
          if (failed(results))
            return error()
                   << "Rule/Method expression requires a direct Method call";
          continue;
        } else {
          return error() << "unsupported Method/Rule statement";
        }
        llvm::StringSet<> fixedForbidden;
        for (const auto &state : states) fixedForbidden.insert(state.first());
        for (const auto &method : methods) fixedForbidden.insert(method.first());
        for (const auto &rule : rules) fixedForbidden.insert(rule.first());
        auto fixed = bindFixedResults(statement, inside, decl.symbol, values,
                                      instances, &environment.boundaries,
                                      fixedForbidden, false);
        if (failed(fixed)) return failure();
        if (*fixed) {
          if (failed(forEachAssignmentLeaf(target, [&](const AstNode &leaf) {
                StringRef name = assignmentRoot(leaf);
                return emitFormalWrite(name, values.lookup(name), statement,
                                       inside, environment);
              }))) return failure();
          continue;
        }
        if (statement.kind() == "Assign" && target.kind() == "Tuple" &&
            rhs.kind() == "Call") {
          if (rhs.child("func").kind() == "Name" &&
              rules.contains(rhs.child("func").string("id")) &&
            !methods.contains(rhs.child("func").string("id")))
            return error() << "nested Rule calls are unsupported";
          llvm::StringSet<> forbidden;
          for (const auto &state : states) forbidden.insert(state.first());
          for (const auto &method : methods) forbidden.insert(method.first());
          for (const auto &rule : rules) forbidden.insert(rule.first());
          auto results = sourceMethodCall(rhs, inside, decl.symbol, values,
                                          instances, true);
          if (failed(results) || failed(bindCallableResults(statement, *results,
                  inside, decl.symbol, values, instances, forbidden,
                  &environment.boundaries)))
            return failure();
          if (failed(forEachAssignmentLeaf(target, [&](const AstNode &leaf) {
                StringRef name = assignmentRoot(leaf);
                return emitFormalWrite(name, values.lookup(name), statement, inside, environment);
              }))) return failure();
          continue;
        }
        StringRef ownerRoot = assignmentRoot(target);
        if (statement.kind() == "Assign" && target.kind() != "Name" && rhs &&
            states.contains(ownerRoot)) {
          const StateOwner &owner = states[ownerRoot];
          llvm::StringMap<Value> typeValues(values);
          typeValues[ownerRoot] = owner.op->getResult(0);
          DictionaryAttr selectedField;
          auto selectedType = expressionType(target, typeValues, instances,
                                             decl.symbol, &selectedField);
          if (failed(selectedType)) return failure();
          auto replacement = expression(rhs, inside, decl.symbol, values,
                                          instances, *selectedType);
          if (failed(replacement)) return failure();
          auto converted = selectedField
              ? applyBindingBoundary(
                    *replacement,
                    BindingBoundary(structFieldPort(selectedField, target), true),
                    rhs, inside, decl.symbol)
              : unsignedBoundary(*replacement, *selectedType, rhs, inside,
                                   decl.symbol);
          if (failed(converted)) return failure();
          auto selectors = writeSelectors(target, inside);
          if (failed(selectors))
            return failure();
          SmallVector<Value> operands;
          auto path = trueValue(statement, inside, decl.symbol);
          if (failed(path))
            return failure();
          operands.append({*path, *converted});
          // Bind the target index once after the RHS. Native state lowering
          // reconstructs this selected write against the current candidate.
          AstNode element = target.kind() == "Subscript"
                                ? target
                                : indexedFieldTarget(target);
          if (element) {
            auto index = expression(element.child("slice"), inside, decl.symbol,
                                    values, instances);
            if (failed(index) ||
                failed(proveIndex(owner.op->getResult(0), *index, statement)))
              return failure();
            operands.push_back(*index);
          }
          createOp(
              inside, statement.location(b.getContext(), source.path),
              ac::SourceStateWriteOp::getOperationName(), operands, TypeRange{},
              {inside.getNamedAttr("state", inside.getStringAttr(owner.name)),
               inside.getNamedAttr("selectors", *selectors),
               inside.getNamedAttr(
                   "occurrence",
                   sourceOccurrence(inside, decl.symbol, statement))});
          continue;
        }
        if (statement.kind() == "Assign" && rhs &&
            (target.kind() == "Attribute" || target.kind() == "Subscript") &&
            values.contains(ownerRoot) &&
            isa<ac::TableType>(values.lookup(ownerRoot).getType())) {
          if (failed(chargeLoopDescriptorWork(statement, "local Table reconstruction")))
            return failure();
          DictionaryAttr selectedField;
          auto selectedType = expressionType(target, values, instances,
                                             decl.symbol, &selectedField);
          if (failed(selectedType)) return failure();
          auto replacement = expression(rhs, inside, decl.symbol, values,
                                          instances, *selectedType);
          if (failed(replacement)) return failure();
          auto converted = selectedField
              ? applyBindingBoundary(
                    *replacement,
                    BindingBoundary(structFieldPort(selectedField, target), true),
                    rhs, inside, decl.symbol)
              : unsignedBoundary(*replacement, *selectedType, rhs, inside,
                                   decl.symbol);
          SmallVector<Value> indices;
          if (failed(converted) ||
              failed(updateAssignment(target, *converted, statement, inside,
                                      decl.symbol, values, &indices)))
            return failure();
          if (auto boundary = environment.boundaries.find(ownerRoot);
              boundary != environment.boundaries.end()) {
            auto bounded = applyBindingBoundary(values.lookup(ownerRoot), boundary->second,
                                                 target, inside, decl.symbol);
            if (failed(bounded)) return failure();
            values[ownerRoot] = *bounded;
          }
          auto selectors = writeSelectors(target, inside);
          if (failed(selectors) ||
              failed(emitFormalWrite(ownerRoot, *converted, statement, inside,
                                     environment, {}, *selectors, indices)))
            return failure();
          continue;
        }
        if (statement.kind() == "Assign" && target.kind() == "Attribute" && rhs) {
          auto reserveLocal = [&](uint64_t work, uint64_t slots,
                                  uint64_t operations = 0) -> LogicalResult {
            TableQueryCharge charge;
            std::string reason;
            bool fits = TableQueryBudget::add(charge, TableQueryResource::Work, work) &&
                        TableQueryBudget::add(charge, TableQueryResource::Slots, slots) &&
                        TableQueryBudget::add(charge, TableQueryResource::Operations, operations);
            if (!fits || !queryBudget.reserve(charge, reason))
              return error() << "local Struct field reconstruction: "
                             << (fits ? reason : "cost counter exhausted");
            return success();
          };
          if (failed(reserveLocal(target.path.size() + 1,
                  (target.path.size() + 1) * ((sizeof(AstStep) + 7) / 8))))
            return failure();
          AstNode rootTarget = target;
          SmallVector<StringRef, 0> fieldNames;
          while (rootTarget.kind() == "Attribute") {
            if (fieldNames.size() >= 128)
              return error() << "local Struct field target depth exceeded";
            uint64_t pathSlots =
                3 * (rootTarget.path.size() + 2) * ((sizeof(AstStep) + 7) / 8);
            if (failed(reserveLocal(rootTarget.path.size() + fieldNames.size() + 3,
                                    pathSlots + fieldNames.size() + 1)))
              return failure();
            fieldNames.reserve(fieldNames.size() + 1);
            fieldNames.push_back(rootTarget.string("attr"));
            rootTarget = rootTarget.child("value");
          }
          if (rootTarget.kind() != "Name")
            return error() << "local Struct field target requires an Attribute-only Name path";
          StringRef root = rootTarget.string("id");
          bool formal = llvm::any_of(callable.inputs, [&](const Port &port) {
            return port.name == root;
          });
          if (states.contains(root) || boundHardwareIdentity(root) ||
              (helperOwnerBindings.contains(root) && !formal) || root == "pyc_clk" ||
              root == "pyc_rst" || !values.contains(root))
            return error() << "local Struct field target requires a bound ordinary payload";
          Value original = values.lookup(root);
          if (!isa<ac::StructType>(original.getType()))
            return error() << "local field assignment requires a nominal Struct payload";
          auto admitted = resolveBranchPayloadType(original.getType(), inside, statement);
          if (failed(admitted)) return failure();
          std::reverse(fieldNames.begin(), fieldNames.end());
          Type selectedType = *admitted;
          DictionaryAttr selectedField;
          ac::HardwareAnalysis analysis(*body);
          for (StringRef fieldName : fieldNames) {
            auto record = dyn_cast<ac::StructType>(selectedType);
            auto declaration = record ? analysis.lookupStruct(record) : ac::StructOp();
            if (!declaration)
              return error() << "local field path intermediates require Struct payloads";
            auto fields = declaration.getFields();
            if (failed(reserveLocal(fields.size() + fieldName.size(), 0)))
              return failure();
            Type next;
            for (Attribute raw : fields) {
              auto field = cast<DictionaryAttr>(raw);
              StringRef name = field.getAs<StringAttr>("name").getValue();
              if (failed(reserveLocal(1 + (name.size() == fieldName.size()
                                               ? name.size() : 0), 0)))
                return failure();
              if (name == fieldName) {
                next = field.getAs<TypeAttr>("type").getValue();
                selectedField = field;
              }
            }
            if (!next) return error() << "unknown local Struct assignment field";
            selectedType = next;
          }
          // Each actual replaceField call pays its own concrete reconstruction.
          // Repeated recursive target evaluation still copies AST paths and
          // projects ancestors; no constructor reuse is assumed for those
          // intermediate projection results.
          uint64_t depth = fieldNames.size();
          for (uint64_t level = 0; level < depth; ++level) {
            uint64_t ancestors = depth - level - 1;
            uint64_t path = target.path.size() + depth + 1;
            if (failed(reserveLocal(
                    (ancestors + 1) * (path + 1),
                    (ancestors + 1) * path * ((sizeof(AstStep) + 7) / 8) +
                        ancestors * 32 + 32,
                    ancestors + 1)))
              return failure();
          }
          BindingBoundary rootBoundary = environment.boundaries.contains(root)
              ? environment.boundaries.lookup(root)
              : inferredBoundary(root, original, statement);
          BindingBoundary fieldBoundary{
              structFieldPort(selectedField, target), true};
          auto value = expression(rhs, inside, decl.symbol, values, instances,
                                  selectedType);
          if (failed(value)) return failure();
          auto converted = applyBindingBoundary(*value, fieldBoundary, rhs,
                                                inside, decl.symbol);
          bool preserveProjectionWitnesses =
              formalProposals && callable.assignedFormals.contains(root) &&
              !callable.stateFormals.contains(root);
          if (failed(converted) || failed(updateAssignment(target, *converted,
                  statement, inside, decl.symbol, values, nullptr,
                  preserveProjectionWitnesses)))
            return failure();
          if (values.lookup(root).getType() != original.getType())
            return error() << "local field reconstruction changed root nominal type";
          environment.boundaries[root] = std::move(rootBoundary);
          auto selectors = writeSelectors(target, inside);
          if (failed(selectors) ||
              failed(emitFormalWrite(root, *converted, statement, inside,
                                     environment, {}, *selectors)))
            return failure();
          continue;
        }
        if (target.kind() == "Subscript")
          return error() << "indexed assignment requires a table";
        if (target.kind() != "Name" || !rhs)
          return error()
                 << "Method/Rule assignment requires a Name or local Struct field";
        StringRef name = target.string("id");
        if (rhs.kind() == "Call" && rhs.child("func").kind() == "Name" &&
            rules.contains(rhs.child("func").string("id")) &&
            !methods.contains(rhs.child("func").string("id")))
          return error() << "nested Rule calls are unsupported";
        if (rhs.kind() == "Attribute" && rhs.child("value").kind() == "Name" &&
            instances.contains(rhs.child("value").string("id")) &&
            !values.contains(rhs.child("value").string("id"))) {
          auto *instance = instances.lookup(rhs.child("value").string("id"));
          bool output = llvm::any_of(instance->outputs, [&](const Port &port) {
            return port.name == rhs.string("attr");
          });
          if (!output)
            return error() << "indirect Method values are unsupported";
        }
        if (states.contains(name)) {
          if (statement.kind() == "AnnAssign")
            return error() << "owner shadow declaration is unsupported";
          auto value = expression(rhs, inside, decl.symbol, values, instances,
                                  states[name].type);
          if (failed(value))
            return failure();
          auto converted = applyBindingBoundary(
              *value, ownerBoundary(states[name]), rhs, inside, decl.symbol);
          if (failed(converted))
            return failure();
          auto path = trueValue(statement, inside, decl.symbol);
          if (failed(path))
            return failure();
          createOp(inside, statement.location(b.getContext(), source.path),
                   ac::SourceStateWriteOp::getOperationName(),
                   ValueRange{*path, *converted}, TypeRange{},
                   {inside.getNamedAttr("state", inside.getStringAttr(states[name].name)),
                    inside.getNamedAttr("selectors", inside.getArrayAttr({})),
                    inside.getNamedAttr(
                        "occurrence",
                        sourceOccurrence(inside, decl.symbol, statement))});
          continue;
        }
        if (boundHardwareIdentity(name))
          return error()
                 << "Method/Rule local shadows a bound callable, instance, "
                    "import or type declaration";
        if (statement.kind() == "AnnAssign" && values.contains(name))
          return error()
                 << "Method/Rule local annotation cannot redeclare a binding";
        std::optional<Type> expected;
        Port localContract;
        if (statement.kind() == "AnnAssign") {
          auto type = portType(statement.child("annotation"), decl.symbol);
          if (failed(type))
            return failure();
          expected = *type;
          localContract = {name.str(), *type, statement.child("annotation"),
                           annotationKind(statement.child("annotation")),
                           fixedAnnotation(statement.child("annotation"))};
        }
        auto value =
            expression(rhs, inside, decl.symbol, values, instances, expected);
        if (failed(value))
          return failure();
        if (expected) {
          auto converted = applyBindingBoundary(
              *value, BindingBoundary{localContract, true}, rhs, inside, decl.symbol);
          if (failed(converted))
            return failure();
          values[name] = *converted;
          environment.boundaries[name] = BindingBoundary{localContract, true};
        } else {
          auto binding = environment.boundaries.find(name);
          if (binding != environment.boundaries.end()) {
            auto converted = applyBindingBoundary(*value, binding->second, rhs,
                                                   inside, decl.symbol);
            if (failed(converted))
              return failure();
            values[name] = *converted;
          } else {
            values[name] = *value;
          }
        }
        if (failed(emitFormalWrite(name, values.lookup(name), statement, inside, environment))) return failure();
      }
      return success();
    };
    emitBody = [&](const AstNode &parent, StringRef field, OpBuilder &at,
                   BranchEnvironment &environment, bool inBranch) {
      return emitRange(parent, field, at, environment, inBranch, 0, false);
    };
    if (failed(emitBody(callable.declaration, "body", inside, environment, false)))
      return failure();
    if (callable.results.size() != yielded.size())
      return error() << "Method/Rule result requires one final return";
    if (formalProposals)
      for (const Port &port : callable.inputs)
        if (callable.assignedFormals.contains(port.name) &&
            !callable.stateFormals.contains(port.name)) {
          auto proposal = applyBindingBoundary(environment.values.lookup(port.name),
              BindingBoundary{port, true}, callable.declaration, inside, decl.symbol);
          if (failed(proposal)) return failure();
          Value data = *proposal;
          if (data.getType() != port.type && isa<ac::BitsType>(data.getType()) &&
              isa<ac::BitsType>(port.type) && ac::areEquivalentHardwareTypes(data.getType(), port.type))
            data = boundaryBitsIdentity(data, port.type, callable.declaration, inside, decl.symbol);
          yielded.push_back(data);
          Value intent = environment.values.lookup(intentName(port.name));
          Type predicateType = internalBits(1, callable.declaration, decl.symbol);
          if (intent.getType() != predicateType)
            intent = boundaryBitsIdentity(intent, predicateType, callable.declaration,
                                           inside, decl.symbol);
          yielded.push_back(intent);
        }
    return yielded;
  };


  emitMethod = [&](StringRef requestedName) -> LogicalResult {
    std::string methodName = requestedName.str();
    if (emittedMethods.contains(methodName)) return success();
    auto &entry = *methods.find(methodName);
    SourceCallableDecl &callable = entry.second;
    for (const Port &port : callable.inputs) {
      if (states.contains(port.name) && callable.syntax.marker == MarkerKind::Method)
        return error() << "Method parameter shadows an owner state";
      if (callable.lexical ? boundHardwareIdentity(port.name) :
          names.contains(port.name) || structs.contains(port.name) ||
          enums.contains(port.name) || behavioralRules.contains(port.name))
        return error()
               << "Method parameter shadows a bound callable, instance, "
                  "import or type declaration";
    }
    if (callable.lexical) retainHelperSnapshots(methodCaptures[entry.first()]);
    SmallVector<Value> captures;
    SmallVector<StringRef> captureNames;
    for (const auto &value : methodCaptures[entry.first()]) {
      if (llvm::any_of(callable.inputs, [&](const Port &formal) {
            return formal.name == value.first;
          }))
        continue;
      captureNames.push_back(value.first);
      captures.push_back(value.second);
    }
    SmallVector<Attribute> argumentNames, parameters, resultConstraints, formalProposals;
    SmallVector<Type> physicalResults(callable.results);
    for (const Port &port : callable.inputs) {
      argumentNames.push_back(at.getStringAttr(port.name));
      auto constraint = sourceConstraint(port);
      if (failed(constraint))
        return failure();
      parameters.push_back(at.getDictionaryAttr(
          {at.getNamedAttr("name", at.getStringAttr(port.name)),
           at.getNamedAttr("binding",
                           at.getStringAttr("positional_or_keyword")),
           at.getNamedAttr("constraint", *constraint),
           at.getNamedAttr("default", absentDefault(at)),
           at.getNamedAttr("origin",
                           sourceOccurrence(at, decl.symbol, port.node)),
           at.getNamedAttr("location",
                           sourceSpan(at, source.path, port.node))}));
    }
    Operation *method = createOp(
        at, callable.declaration.location(b.getContext(), source.path),
        ac::SourceMethodOp::getOperationName(), captures, TypeRange{},
        {at.getNamedAttr("sym_name", at.getStringAttr(entry.first())),
         at.getNamedAttr("function_type", TypeAttr::get(callable.type)),
         at.getNamedAttr("argument_names", at.getArrayAttr(argumentNames)),
         at.getNamedAttr("ac.parameters", at.getArrayAttr(parameters)),
         at.getNamedAttr("ac.result_constraints",
                         at.getArrayAttr(resultConstraints)),
         at.getNamedAttr(
             "ac.return_form",
             at.getStringAttr(callable.results.empty() ? "none" :
                 (callable.returnShape == ReturnShape::Tuple ||
                  (callable.returnShape == ReturnShape::Mapping && callable.results.size() != 1)) ? "tuple" : "single")),
         at.getNamedAttr("ac.guard_location",
                         sourceSpan(at, source.path,
                                    callable.syntax.guard
                                        ? callable.syntax.guard
                                        : callable.declaration)),
         at.getNamedAttr("occurrence", sourceOccurrence(at, decl.symbol,
                                                        callable.declaration))},
        2);

    emittedMethods[methodName] = method;
    for (auto [index, port] : llvm::enumerate(callable.inputs))
      if (callable.syntax.marker == MarkerKind::Rule &&
          callable.assignedFormals.contains(port.name) && !callable.stateFormals.contains(port.name))
        formalProposals.push_back(at.getI64IntegerAttr(index));
    if (!formalProposals.empty())
      method->setAttr("ac.formal_proposals", at.getArrayAttr(formalProposals));
    if (callable.autonomous || callable.privateSpecialization || !callable.stateFormals.empty())
      method->setAttr("sym_visibility", at.getStringAttr("private"));
    for (unsigned regionIndex = 0; regionIndex < 2; ++regionIndex) {
      auto *block = new Block();
      method->getRegion(regionIndex).push_back(block);
      llvm::StringMap<Value> values;
      for (auto [index, port] : llvm::enumerate(callable.inputs)) {
        Value argument = block->addArgument(port.type, method->getLoc());
        values[port.name] = argument;
        if (callable.privateSpecialization && index < callable.actualFacts.size()) {
          auto facts = valueInfo(callable.actualFacts[index]);
          facts.value = argument;
          if (!isa<ac::EnumType>(argument.getType())) numericValues[argument] = facts;
          if (arithmeticValues.contains(callable.actualFacts[index]))
            arithmeticValues.insert(argument);
        } else remember(argument, port.sourceKind);
        if (port.fixedBits)
          fixedValues.insert(argument);
      }
      for (auto [name, original] : llvm::zip(captureNames, captures)) {
        auto captured = captureValue(block, original, method->getLoc());
        if (!name.empty()) values[name] = captured;
      }
      OpBuilder inside = OpBuilder::atBlockEnd(block);
      auto yielded = emitStatements(callable, inside, values, regionIndex == 0);
      if (failed(yielded))
        return failure();
      createOp(inside,
               callable.declaration.location(b.getContext(), source.path),
               ac::YieldOp::getOperationName(), *yielded, {}, {});
    }
    formalProposals.clear();
    physicalResults = callable.results;
    for (const Port &port : callable.resultContracts) {
      auto constraint = sourceConstraint(port);
      if (failed(constraint))
        return failure();
      resultConstraints.push_back(*constraint);
    }
    if (callable.syntax.marker == MarkerKind::Rule)
      for (auto [index, port] : llvm::enumerate(callable.inputs))
        if (callable.assignedFormals.contains(port.name) &&
            !callable.stateFormals.contains(port.name)) {
          formalProposals.push_back(at.getI64IntegerAttr(index));
          auto data = sourceConstraint(port);
          Type predicateType = internalBits(1, callable.declaration, decl.symbol);
          Port predicate{"intent", predicateType, callable.declaration,
                         std::nullopt, true};
          auto intent = sourceConstraint(predicate);
          if (failed(data) || failed(intent)) return failure();
          resultConstraints.push_back(*data);
          resultConstraints.push_back(*intent);
          physicalResults.push_back(port.type);
          physicalResults.push_back(predicateType);
        }
    auto physicalType = FunctionType::get(b.getContext(), callable.type.getInputs(),
                                          physicalResults);
    method->setAttr("function_type", TypeAttr::get(physicalType));
    method->setAttr("ac.result_constraints", at.getArrayAttr(resultConstraints));
    method->setAttr("ac.return_form", at.getStringAttr(callable.results.empty() ? "none" :
        (callable.returnShape == ReturnShape::Tuple ||
         (callable.returnShape == ReturnShape::Mapping && callable.results.size() != 1)) ? "tuple" : "single"));
    return success();
  };

  llvm::DenseMap<DictionaryAttr, Operation *> registeredTransactions;
  llvm::DenseMap<DictionaryAttr, SmallVector<Value>> ruleArguments;
  llvm::DenseMap<DictionaryAttr, std::string> registrationMethods;
  llvm::DenseMap<DictionaryAttr, SourceCallableDecl> registeredCallables;
  llvm::StringMap<SmallVector<Operation *>> transactionsForCallable;
  llvm::StringSet<> transactionNames;
  auto registerRule = [&](const AstNode &statement,
                          const AstNode &call) -> LogicalResult {
    bool localDeclaration = call.kind() == "FunctionDef";
    StringRef name = localDeclaration ? call.string("name") : call.child("func").string("id");
    DictionaryAttr invocation = sourceOccurrence(at, decl.symbol, call);
    SourceCallableDecl &callable = registeredCallables[invocation];
    callable = rules[name];
    if (callable.autonomous && !localDeclaration)
      return error() << "autonomous local Rule cannot be called; use its result at Module wiring";
    if (callable.inferResults && statement.kind() == "Return") {
      callable.resultContracts = decl.outputs;
      callable.returnShape = decl.returnShape;
      for (const Port &port : decl.outputs) callable.results.push_back(port.type);
      callable.inferResults = false;
    }
    for (const Port &port : callable.inputs)
      if ((callable.lexical ? boundHardwareIdentity(port.name) :
               names.contains(port.name) || structs.contains(port.name) ||
               enums.contains(port.name) || behavioralRules.contains(port.name)) ||
          port.name == "pyc_clk" || port.name == "pyc_rst")
        return error() << "Rule parameter shadows a protected hardware binding";
    auto plan = ruleWrites.lookup(decl.node, call, [&](uint64_t work) {
      return succeeded(chargeLoopDescriptorWork(call, "typed Rule owner binding", work));
    });
    if (failed(plan) || !*plan ||
        !sameSourceSite((*plan)->rule, callable.declaration))
      return error() << "typed Rule registration differs from its source owner plan";
    for (const auto &write : (*plan)->writes) {
      auto state = llvm::find_if(states, [&](const auto &entry) {
        return sameSourceSite(entry.second.node, write.owner);
      });
      auto formal = llvm::find_if(callable.inputs, [&](const Port &port) {
        return port.name == write.formal;
      });
      if (state == states.end() || formal == callable.inputs.end())
        return error() << "typed Rule writable formal lacks its actual state owner";
      if (formal->type && !sameBindingContract(ownerBoundary(state->second),
                               BindingBoundary{*formal, true}))
        return error() << "Rule owner formal annotation must match its actual owner";
      callable.stateFormals[write.formal] = state->first().str();
    }
    SmallVector<StringRef> formalNames;
    for (const Port &port : callable.inputs) formalNames.push_back(port.name);
    SmallVector<AstNode> sourceActuals;
    SmallVector<size_t> sourceOrder;
    auto expectedKind = localDeclaration ? SourceRuleCallPlan::Kind::LocalDeclaration
                                         : SourceRuleCallPlan::Kind::GlobalTemplateCall;
    if ((*plan)->kind != expectedKind || !sameSourceSite((*plan)->statement, statement))
      return error() << "Rule registration kind/statement differs from its real source site";
    if (localDeclaration) {
      auto formals = callable.declaration.child("args");
      if (formals.array("defaults").size() != callable.inputs.size())
        return error() << "autonomous Rule formal requires an explicit default binding";
      for (size_t index = 0; index < callable.inputs.size(); ++index) {
        sourceActuals.push_back(formals.item("defaults", index));
        sourceOrder.push_back(index);
      }
    } else {
      auto normalized = normalizeCallArguments(call, formalNames, error,
          "Rule template binding argument arity mismatch", &sourceOrder);
      if (failed(normalized)) return failure();
      sourceActuals = std::move(*normalized);
    }
    if ((*plan)->bindings.size() != sourceOrder.size())
      return error() << "Rule binding count differs from its real source site";
    for (auto [sourceOrdinal, binding] : llvm::zip(sourceOrder, (*plan)->bindings)) {
      if (failed(chargeLoopDescriptorWork(call, "Rule source binding authentication",
              1 + binding.expression.path.size() + sourceActuals[sourceOrdinal].path.size())))
        return failure();
      if (binding.ordinal != sourceOrdinal ||
          !sameSourceSite(binding.expression, sourceActuals[sourceOrdinal]))
        return error() << "Rule binding expression/ordinal differs from its real source site";
    }
    SmallVector<AstNode> actuals(callable.inputs.size());
    for (const auto &binding : (*plan)->bindings) {
      if (binding.ordinal >= actuals.size() || actuals[binding.ordinal])
        return error() << "Rule binding formal ordinals differ from source plan";
      actuals[binding.ordinal] = binding.expression;
    }
    if ((*plan)->bindings.size() != callable.inputs.size())
      return error() << "Rule bindings differ from callable formal signature";
    llvm::StringSet<> writableOwners;
    for (auto [index, port] : llvm::enumerate(callable.inputs)) {
      if (!callable.assignedFormals.contains(port.name)) continue;
      auto actual = actuals[index];
      if (actual.kind() != "Name") continue;
      auto state = states.find(actual.string("id"));
      if (state == states.end()) continue;
      if (!writableOwners.insert(state->second.name).second)
        return error() << "overlapping writable owner aliases";
      if (port.type && !sameBindingContract(ownerBoundary(state->second), BindingBoundary{port, true}))
        return error() << "Rule owner formal annotation must match its actual owner";
      callable.stateFormals[port.name] = state->first().str();
    }
    llvm::StringMap<Value> actualValues(moduleValues);
    for (const auto &entry : states) {
      Value oldQ = entry.second.op->getResult(0);
      actualValues[entry.first()] = oldQ;
      rememberState(oldQ, entry.second);
    }
    FailureOr<SmallVector<Value>> arguments = SmallVector<Value>(callable.inputs.size());
    for (const auto &sourceBinding : (*plan)->bindings) {
      unsigned index = sourceBinding.ordinal;
      auto actual = sourceBinding.expression;
      auto &port = callable.inputs[index];
      auto value = expression(actual, at, decl.symbol, actualValues, instances,
                              port.type ? std::optional<Type>(port.type) : std::nullopt);
      if (failed(value)) return failure();
      if (localDeclaration && localRuleResult(*value))
        return mlir::emitError(actual.location(b.getContext(), source.path))
               << "local Rule result alias is not a Rule input; wire through a child Module interface";
      // Complete the declared argument boundary before deriving the formal's
      // binding facts, as bindCallableArguments does for explicit calls.
      // In particular a contextual integer literal acquires its checked Bits
      // boundary here; unannotated defaults retain their actual source kind.
      if (port.type) {
        auto converted = applyBindingBoundary(*value, BindingBoundary{port, true},
                                               actual, at, decl.symbol);
        if (failed(converted)) return failure();
        (*arguments)[index] = *converted;
      } else {
        (*arguments)[index] = *value;
      }
    }
    for (auto [index, port] : llvm::enumerate(callable.inputs)) {
      auto binding = inferredBoundary(port.name, (*arguments)[index], actuals[index], true);
      auto actualState = actuals[index].kind() == "Name"
          ? states.find(actuals[index].string("id")) : states.end();
      bool ownerActual = actualState != states.end();
      if (ownerActual) binding = ownerBoundary(actualState->second);
      auto bound = bindRuleFormal(port.node, actuals[index], (*arguments)[index],
                                   binding, ownerActual, at, decl.symbol);
      if (failed(bound)) return failure();
      if (!port.type) {
        port = bound->boundary.contract();
        port.name = bound->name;
        port.node = callable.declaration.child("args").item("args", index);
      }
      (*arguments)[index] = bound->value;
    }
    callable.actualFacts.assign(arguments->begin(), arguments->end());
    SmallVector<Type> inferredInputs;
    for (const Port &port : callable.inputs) inferredInputs.push_back(port.type);
    callable.type = FunctionType::get(b.getContext(), inferredInputs, callable.results);
    if (callable.inferredSignature || !callable.stateFormals.empty()) {
      auto specialized = specialize(name, callable);
      if (failed(specialized)) return failure();
      registrationMethods[invocation] = *specialized;
      callable = methods.find(*specialized)->second;
    } else registrationMethods[invocation] = name.str();
    for (auto [index, port] : llvm::enumerate(callable.inputs))
      if ((*arguments)[index].getType() != port.type && isa<ac::BitsType>(port.type) &&
          ac::areEquivalentHardwareTypes((*arguments)[index].getType(), port.type))
        (*arguments)[index] = boundaryBitsIdentity((*arguments)[index], port.type,
                                                   actuals[index], at, decl.symbol);
    ruleArguments[invocation] = std::move(*arguments);
    std::string transactionName = name.str();
    unsigned collision = 1;
    while (!transactionNames.insert(transactionName).second)
      transactionName = name.str() + "_" + std::to_string(collision++);
    Operation *transaction = createOp(
        at, call.location(b.getContext(), source.path),
        ac::SourceTransactionOp::getOperationName(),
        SmallVector<Value>{clock, reset}, callable.results,
        {at.getNamedAttr("name", at.getStringAttr(transactionName)),
         at.getNamedAttr("declaration", sourceOccurrence(at, decl.symbol,
                                                         callable.declaration)),
         at.getNamedAttr("ac.guard_location",
                         sourceSpan(at, source.path,
                                    callable.syntax.guard
                                        ? callable.syntax.guard
                                        : callable.declaration)),
         at.getNamedAttr("occurrence", sourceOccurrence(at, decl.symbol, call))},
        2);
    registeredTransactions[invocation] = transaction;
    transactionsForCallable[name].push_back(transaction);
    for (auto [result, contract] : llvm::zip(transaction->getResults(),
                                            callable.resultContracts)) {
      remember(result, contract.sourceKind);
      if (contract.fixedBits) fixedValues.insert(result);
    }
    if (localDeclaration) {
      if (auto *binding = nestedRuleBinding(name)) {
        if (binding->registered) return error() << "autonomous Rule declaration is already registered";
        binding->registered = true;
        binding->results.assign(transaction->getResults().begin(), transaction->getResults().end());
        binding->returnShape = callable.returnShape;
      }
      if (callable.results.size() == 1 && callable.returnShape != ReturnShape::Tuple)
        moduleValues[name] = transaction->getResult(0);
      return success();
    }
    if (statement.kind() == "Return" && callable.returnShape == ReturnShape::Tuple)
      return error() << "tuple Rule call requires direct Tuple unpacking";
    if (statement.kind() != "Assign") return success();
    llvm::StringSet<> forbidden;
    for (const auto &input : authoredInputs) forbidden.insert(input.getKey());
    for (const auto &state : authoredOwnerNames) forbidden.insert(state.getKey());
    for (const auto &method : methods) forbidden.insert(method.first());
    for (const auto &rule : rules) forbidden.insert(rule.first());
    if (callable.returnShape == ReturnShape::Tuple)
      return bindCallableResults(statement, transaction->getResults(), at,
                                  decl.symbol, moduleValues, instances, forbidden);
    if (statement.array("targets").size() != 1 ||
        statement.item("targets", 0).kind() != "Name" ||
        callable.results.size() != 1)
      return error() << "single Rule result requires one ordinary Name target";
    StringRef target = statement.item("targets", 0).string("id");
    if (forbidden.contains(target) || boundHardwareIdentity(target) ||
        target == "pyc_clk" || target == "pyc_rst" ||
        queueResultNames.contains(target) || memoryResultNames.contains(target))
      return error() << "Rule result cannot write a protected hardware binding";
    moduleValues[target] = transaction->getResult(0);
    return success();
  };

  llvm::StringMap<unsigned> moduleAssignmentCounts;
  llvm::StringMap<BindingBoundary> ordinaryBoundaries;
  auto ordinaryStatement = [&](size_t ordinal) -> LogicalResult {
    auto ordinaryValues = std::move(moduleValues);
    auto restoreValues = llvm::scope_exit([&] {
      for (const auto &entry : states)
        ordinaryValues.erase(entry.first());
      moduleValues = std::move(ordinaryValues);
    });
    llvm::StringSet<> protectedNames;
    for (const auto &input : authoredInputs)
      protectedNames.insert(input.getKey());
    for (const auto &entry : states) {
      Value oldQ = entry.second.op->getResult(0);
      ordinaryValues[entry.first()] = oldQ;
      rememberState(oldQ, entry.second);
      protectedNames.insert(entry.first());
    }
    // A nested Rule can also have a shared Method entry in this inventory.
    // Its earlier data spelling is governed by requireNestedRuleDataTarget,
    // not by the later callable's presence in either map.
    for (const auto &entry : methods)
      if (!nestedRuleBinding(entry.first()))
        protectedNames.insert(entry.first());
    for (const auto &entry : rules)
      if (!nestedRuleBinding(entry.first()))
        protectedNames.insert(entry.first());
    if (failed(moduleStatement(decl, ordinal, at, ordinaryValues,
                               ordinaryBoundaries, instances, protectedNames)))
      return failure();
    auto statement = decl.node.item("body", ordinal);
    if (statement.kind() == "Assign" &&
        statement.array("targets").size() == 1 &&
        statement.item("targets", 0).kind() == "Name")
      ++moduleAssignmentCounts[statement.item("targets", 0).string("id")];
    ordinaryModuleAssignments.insert(
        sourceOccurrence(at, decl.symbol, statement));
    return success();
  };
  bool sawStructuralReturn = false;
  for (size_t i = 0; i < decl.node.array("body").size(); ++i) {
    AstNode statement = decl.node.item("body", i);
    if (staticReferenceAssignments.contains(namespaceSite(statement)))
      continue;
    if (statement.kind() == "Return") {
      sawStructuralReturn = true;
      continue;
    }
    if (sawStructuralReturn &&
        (statement.kind() == "Assign" || statement.kind() == "AnnAssign" ||
         statement.kind() == "AugAssign" || statement.kind() == "If" ||
         statement.kind() == "Match" || statement.kind() == "For"))
      return error() << "new local binding after structural return";
  }
  size_t instanceCount = 0;
  for (size_t i = 0; i < decl.node.array("body").size(); ++i) {
    AstNode statement = decl.node.item("body", i);
    if (staticReferenceAssignments.contains(namespaceSite(statement)))
      continue;
    if (statement.kind() == "Assign" &&
        statement.array("targets").size() == 1 &&
        statement.item("targets", 0).kind() == "Name" &&
        statement.child("value").kind() == "Call")
      ++instanceCount;
  }
  instanceStorage.reserve(instanceCount + (decl.references ? decl.references.size() : 0));
  for (Attribute raw : decl.references ? decl.references : at.getArrayAttr({})) {
    auto reference = cast<DictionaryAttr>(raw);
    std::string name = reference.getAs<StringAttr>("name").getValue().str();
    instanceStorage.push_back({name, decl.node, reference.getAs<FlatSymbolRefAttr>("callee")});
    instanceStorage.back().reference = name;
    instances[name] = &instanceStorage.back();
  }
  for (size_t i = 0; i < decl.node.array("body").size(); ++i) {
    AstNode statement = decl.node.item("body", i);
    if (staticReferenceAssignments.contains(namespaceSite(statement)))
      continue;
    if (statement.kind() == "FunctionDef") {
      StringRef name = statement.string("name");
      if (statement.array("decorator_list").empty()) {
        if (failed(declareValueHelper(statement, decl.symbol, moduleValues))) return failure();
      } else if (!methods.contains(name) && !rules.contains(name))
        return error() << "nested callable requires Rule, Method or ordinary typed helper";
      if (decl.returnShape == ReturnShape::Mapping && rules.contains(name) &&
          moduleAssignmentCounts.lookup(name) > 1)
        return error() << "structural local '" << name
                       << "' cannot be rebound before its Rule declaration";
      if (rules.contains(name) && rules[name].lexical &&
          failed(noteNestedRuleDeclaration(statement))) return failure();
      if (methods.contains(name)) snapshot(methodCaptures[name]);
      if (rules.contains(name) && rules[name].lexical) {
        snapshot(ruleCaptures[name]);
        if (failed(registerRule(statement, statement))) return failure();
      }
      continue;
    }
    if (statement.kind() == "Assign" &&
        structuralCallSites.contains(sourceOccurrence(
            at, decl.symbol, statement.child("value")))) {
      ordinaryModuleAssignments.insert(sourceOccurrence(at, decl.symbol, statement));
      continue;
    }
    if (statement.kind() == "Expr") {
      auto bound = moduleInstanceBinding(statement.child("value"), at,
                                         decl.symbol, moduleValues, instances);
      if (failed(bound)) return failure();
      if (*bound) {
        ordinaryModuleAssignments.insert(sourceOccurrence(at, decl.symbol, statement));
        continue;
      }
    }
    if (statement.kind() == "Assign" && statement.child("value").kind() == "Name") {
      StringRef resultName = statement.child("value").string("id");
      if (auto *binding = nestedRuleBinding(resultName);
          binding && binding->returnShape == ReturnShape::Tuple) {
        if (failed(requireNestedRuleDataVisible(resultName, statement.child("value")))) return failure();
        llvm::StringSet<> forbidden;
        for (const auto &input : authoredInputs) forbidden.insert(input.getKey());
        for (const auto &state : states) forbidden.insert(state.first());
        if (failed(bindCallableResults(statement, binding->results, at, decl.symbol,
                                        moduleValues, instances, forbidden))) return failure();
        ordinaryModuleAssignments.insert(sourceOccurrence(at, decl.symbol, statement));
        continue;
      }
    }
    if (statement.kind() == "Assign" &&
        !memoryKind(statement.child("value")).empty()) {
      llvm::StringMap<Value> memoryValues(moduleValues);
      for (const auto &entry : states) {
        Value oldQ = entry.second.op->getResult(0);
        memoryValues[entry.first()] = oldQ;
        rememberState(oldQ, entry.second);
      }
      auto bound = bindMemoryResults(statement, at, decl.symbol, memoryValues,
                                     instances, instanceStorage);
      if (failed(bound) || !*bound) return failure();
      auto *plan = memoryPlan(statement.child("value"), decl.symbol);
      if (!plan) return failure();
      for (const auto &name : plan->resultNames)
        moduleValues[name] = memoryValues.lookup(name);
      ordinaryModuleAssignments.insert(sourceOccurrence(at, decl.symbol, statement));
      continue;
    }
    if (statement.kind() == "Assign" &&
        queueCall(statement.child("value"))) {
      llvm::StringMap<Value> queueValues(moduleValues);
      for (const auto &entry : states) {
        Value oldQ = entry.second.op->getResult(0);
        queueValues[entry.first()] = oldQ;
        rememberState(oldQ, entry.second);
      }
      auto queued = bindQueueResults(statement, at, decl.symbol, queueValues,
                                     instances, instanceStorage);
      if (failed(queued)) return failure();
      if (*queued) {
        if (queueCall(statement.child("value"))) {
          auto targets = queueTargets(statement);
          if (failed(targets)) return failure();
          if (targets->size() == 3)
            for (StringRef name : *targets) moduleValues[name] = queueValues.lookup(name);
        }
        ordinaryModuleAssignments.insert(sourceOccurrence(at, decl.symbol, statement));
        continue;
      }
    }
    AstNode registrationCall =
        statement.kind() == "Expr" || statement.kind() == "Return" ||
                statement.kind() == "Assign"
            ? statement.child("value") : AstNode();
    if (registrationCall.kind() == "Call" &&
        registrationCall.child("func").kind() == "Name" &&
        rules.contains(registrationCall.child("func").string("id"))) {
      auto *binding = nestedRuleBinding(registrationCall.child("func").string("id"));
      if (binding && binding->phase == NestedRulePhase::Data) {
        auto order = nestedRuleOrder(registrationCall, binding->declaration);
        if (failed(order)) return failure();
        if (*order == SourceDeclarationOrder::Before)
          return error() << "ordinary data binding is not a callable Rule";
      }
      if (failed(registerRule(statement, registrationCall))) return failure();
      if (binding) binding->registered = true;
      continue;
    }
    if (statement.kind() == "If" || statement.kind() == "Match" ||
        statement.kind() == "For") {
      if (failed(ordinaryStatement(i)))
        return failure();
      continue;
    }
    if (statement.kind() == "Expr" && moduleCall(statement.child("value"))) {
      llvm::StringMap<Value> ordinaryValues(moduleValues);
      for (const auto &entry : states) {
        Value oldQ = entry.second.op->getResult(0);
        ordinaryValues[entry.first()] = oldQ;
        rememberState(oldQ, entry.second);
      }
      auto results = directModuleResults(statement.child("value"), at, decl.symbol, ordinaryValues, instances);
      if (failed(results)) return failure();
      if (!results->empty()) return error() << "Module value results require an explicit binding";
      ordinaryModuleAssignments.insert(sourceOccurrence(at, decl.symbol, statement));
      continue;
    }
    if (docstring(statement) || statement.kind() == "Return" ||
        statement.kind() == "Expr" || statement.kind() == "Pass")
      continue;
    // Module-scope fixed-result queries (typically Table `.first`/`.argmin`)
    // publish independent ordinary values. They cannot write a persistent
    // owner, input, Method or Rule registration.
    if (statement.kind() == "Assign" &&
        statement.array("targets").size() == 1 &&
        statement.item("targets", 0).kind() == "Tuple" &&
        !fixedResultCall(statement.child("value")).empty()) {
      llvm::StringMap<Value> ordinaryValues(moduleValues);
      for (const auto &entry : states) {
        Value oldQ = entry.second.op->getResult(0);
        ordinaryValues[entry.first()] = oldQ;
        rememberState(oldQ, entry.second);
      }
      llvm::StringSet<> forbidden;
      for (const auto &input : authoredInputs)
        forbidden.insert(input.getKey());
      for (const auto &entry : methods)
        forbidden.insert(entry.first());
      for (const auto &entry : rules)
        forbidden.insert(entry.first());
      for (const auto &entry : states)
        forbidden.insert(entry.first());
      auto fixed = bindFixedResults(statement, at, decl.symbol, ordinaryValues,
                                    instances, nullptr, forbidden, false);
      if (failed(fixed)) return failure();
      if (*fixed) {
        auto target = statement.item("targets", 0);
        for (size_t i = 0; i < target.array("elts").size(); ++i) {
          AstNode leaf = target.item("elts", i);
          StringRef name = assignmentRoot(leaf);
          moduleValues[name] = ordinaryValues.lookup(name);
          noteNestedRuleDataBinding(name);
        }
        ordinaryModuleAssignments.insert(
            sourceOccurrence(at, decl.symbol, statement));
        continue;
      }
    }
    if (statement.kind() == "Assign" &&
        statement.child("value").kind() == "Call" &&
        moduleCall(statement.child("value")) &&
        !methodOwner(statement.child("value").child("func"))) {
      llvm::StringSet<> forbidden;
      for (const auto &input : authoredInputs)
        forbidden.insert(input.getKey());
      for (const auto &entry : methods)
        forbidden.insert(entry.first());
      for (const auto &entry : rules)
        forbidden.insert(entry.first());
      for (size_t j = 0; j < decl.node.array("body").size(); ++j) {
        auto declaration = decl.node.item("body", j);
        if (declaration.kind() == "AnnAssign")
          forbidden.insert(assignmentRoot(declaration.child("target")));
      }
      llvm::StringMap<Value> ordinaryValues(moduleValues);
      for (const auto &entry : states) {
        Value oldQ = entry.second.op->getResult(0);
        ordinaryValues[entry.first()] = oldQ;
        rememberState(oldQ, entry.second);
      }
      auto target = statement.array("targets").size() == 1
          ? statement.item("targets", 0) : AstNode();
      if (target.kind() == "Name") {
        StringRef name = target.string("id");
        if (forbidden.contains(name) || boundHardwareIdentity(name) ||
            moduleValues.contains(name) || name == "pyc_clk" || name == "pyc_rst")
          return error() << "Module instance binding requires a fresh ordinary Name";
        auto call = statement.child("value");
        auto contract = callable(call.child("func"));
        if (failed(contract)) return failure();
        auto results = directModuleResults(call, at, decl.symbol, ordinaryValues, instances);
        if (failed(results)) return failure();
        if (failed(registerDirectModuleHandle(name, statement, *contract, at,
                       decl.symbol, instanceStorage, instances)))
          return failure();
        if (results->size() == 1 && contract->returnShape != ReturnShape::Tuple)
          moduleValues[name] = results->front();
        ordinaryModuleAssignments.insert(sourceOccurrence(at, decl.symbol, statement));
        continue;
      }
      auto bound = bindModuleResults(statement, at, decl.symbol, ordinaryValues,
                                     instances, forbidden);
      if (failed(bound))
        return failure();
      if (*bound) {
        auto target = statement.item("targets", 0);
        for (size_t j = 0; j < target.array("elts").size(); ++j) {
          StringRef name = target.item("elts", j).string("id");
          moduleValues[name] = ordinaryValues[name];
        }
        ordinaryModuleAssignments.insert(
            sourceOccurrence(at, decl.symbol, statement));
        continue;
      }
    }
    if (statement.kind() == "Assign" && statement.array("targets").size() == 1 &&
        statement.item("targets", 0).kind() == "Tuple" &&
        statement.child("value").kind() == "Name") {
      auto instance = instances.find(statement.child("value").string("id"));
      if (instance != instances.end() && instance->second->valueHandle &&
          instance->second->returnShape == ReturnShape::Tuple) {
        llvm::StringSet<> forbidden;
        for (const auto &name : authoredInputs) forbidden.insert(name.getKey());
        for (const auto &name : authoredOwnerNames) forbidden.insert(name.getKey());
        for (const auto &name : methods) forbidden.insert(name.first());
        for (const auto &name : rules) forbidden.insert(name.first());
        if (failed(bindCallableResults(statement, instance->second->op->getResults(),
                at, decl.symbol, moduleValues, instances, forbidden)))
          return failure();
        ordinaryModuleAssignments.insert(sourceOccurrence(at, decl.symbol, statement));
        continue;
      }
    }
    AstNode allocation = statement.child("value");
    bool tableState = statement.kind() == "Assign" &&
        statement.array("targets").size() == 1 &&
        allocation.kind() == "Call" && allocation.child("func").kind() == "Subscript" &&
        intrinsic(allocation.child("func").child("value"), "table");
    if (statement.kind() == "AnnAssign" || tableState) {
      auto target = tableState ? statement.item("targets", 0) : statement.child("target");
      AstNode annotation = tableState ? allocation.child("func") : statement.child("annotation");
      AstNode initializer = statement.child("value");
      if (tableState) {
        if (!allocation.array("args").empty() || allocation.array("keywords").size() != 1 ||
            allocation.item("keywords", 0).string("arg") != "init")
          return error() << "table allocation requires exactly init";
        initializer = allocation.item("keywords", 0).child("value");
      }
      if (target.kind() != "Name" || !initializer)
        return error() << "typed owner state requires an annotated Name initializer";
      StringRef name = target.string("id");
      if (states.contains(name) || boundHardwareIdentity(name) ||
          moduleValues.contains(name) || name == "pyc_clk" || name == "pyc_rst")
        return error() << "persistent owner name is duplicated or protected";
      auto type = portType(annotation, decl.symbol);
      if (failed(type)) return failure();
      Port boundaryPort{name.str(), *type, annotation, annotationKind(annotation),
                        fixedAnnotation(annotation)};
      llvm::StringMap<Instance *> none;
      if (initializer.kind() == "Name" &&
          states.contains(initializer.string("id")))
        return error()
               << "persistent initializer must be a static complete value";
      auto initial = stateInitializer(boundaryPort, initializer, statement,
                                       at, decl.symbol, moduleValues, none, true);
      if (failed(initial)) return failure();
      auto payloadConstraint = sourceConstraint(boundaryPort);
      if (failed(payloadConstraint)) return failure();
      Operation *state = createOp(
          at, statement.location(b.getContext(), source.path),
          ac::SourceStateOp::getOperationName(),
          ValueRange{clock, reset, *initial}, TypeRange{*type},
          {at.getNamedAttr("instance_name", at.getStringAttr(name)),
           at.getNamedAttr("ac.payload_constraint", *payloadConstraint),
           at.getNamedAttr("occurrence", sourceOccurrence(at, decl.symbol, statement))});
      helperOwnerBindings.insert(name);
      states[name] = {name.str(), *type, *initial, state, statement};
      rememberState(state->getResult(0), states[name]);
      continue;
    }
    if (statement.kind() == "Assign" &&
        statement.array("targets").size() == 1 &&
        (statement.item("targets", 0).kind() == "Attribute" ||
         statement.item("targets", 0).kind() == "Subscript")) {
      auto target = statement.item("targets", 0);
      StringRef root = assignmentRoot(target);
      if (root.empty() || states.contains(root) || authoredInputs.contains(root) ||
          instances.contains(root))
        return error() << "module-local assignment cannot write a persistent "
                          "owner, input or instance handle";
      if (failed(ordinaryStatement(i)))
        return failure();
      continue;
    }
    if (statement.kind() == "Assign" &&
        statement.array("targets").size() == 1 &&
        statement.item("targets", 0).kind() == "Name") {
      StringRef name = statement.item("targets", 0).string("id");
      AstNode value = statement.child("value");
      if (decl.methodOwner && rules.contains(name))
        return error() << "Method-owner module local '" << name
                       << "' conflicts with Rule registration";
      if (names.contains(name))
        return error()
               << "module-local assignment cannot rebind imported binding '"
               << name << "'";
      if (states.contains(name))
        return error()
               << "instance handle assignment shadows a persistent owner";
      bool earlierRuleData = nestedRuleBinding(name) &&
          (methods.contains(name) || rules.contains(name));
      if (earlierRuleData && failed(requireNestedRuleDataTarget(name, statement)))
        return failure();
      if ((!earlierRuleData && (methods.contains(name) || rules.contains(name))) ||
          instances.contains(name))
        return error() << "instance handle cannot be rebound and requires a "
                          "fresh immutable name";
      if (value.kind() == "Name" && instances.contains(value.string("id"))) {
        if (earlierRuleData || moduleValues.contains(name))
          return error() << "instance handle cannot be rebound and requires a "
                            "fresh immutable name";
        instances[name] = instances[value.string("id")];
        if (auto payload = moduleValues.lookup(value.string("id")))
          moduleValues[name] = payload;
        continue;
      }
      if (value.kind() == "Call" && value.child("func").kind() == "Attribute" &&
          value.child("func").child("value").kind() == "Name" &&
          instances.contains(value.child("func").child("value").string("id")))
        return error() << "Method call requires a Rule transaction scope";
      if (value.kind() == "Call" && methodOwner(value.child("func"))) {
        if (earlierRuleData || moduleValues.contains(name))
          return error() << "instance handle cannot be rebound and requires a "
                            "fresh immutable name";
        auto referenceContract = callable(value.child("func"));
        if (failed(referenceContract)) return failure();
        auto referenceBindings = bindModuleReferences(referenceContract->signature, value, instances);
        if (failed(referenceBindings)) return failure();
        AstNode function = value.child("func");
        FlatSymbolRefAttr callee;
        Operation *declaration = nullptr;
        unsigned clockInput = 0, resetInput = 1;
        auto target = function.kind() == "Name"
                          ? local.find(function.string("id"))
                          : local.end();
        if (target != local.end()) {
          if (modules[target->second].system)
            return error() << "@system is root-only and cannot be called as a child";
          callee = modules[target->second].symbol;
        } else {
          auto binding = lookupBinding(function);
          auto view = headers.lookupModule(binding->symbol);
          if (view.declaration->hasAttr("ac.root_kind"))
            return error() << "@system is root-only and cannot be called as a child";
          callee = view.symbol;
          declaration = view.declaration;
          clockInput = view.domainInputs.getAs<IntegerAttr>("clock").getInt();
          resetInput = view.domainInputs.getAs<IntegerAttr>("reset").getInt();
        }
        SmallVector<Value> inputs(2);
        inputs[clockInput] = clock;
        inputs[resetInput] = reset;
        Operation *instance = createOp(
            at, statement.location(b.getContext(), source.path),
            ac::InstanceOp::getOperationName(), inputs, TypeRange{},
            {at.getNamedAttr("instance_name", at.getStringAttr(name)),
             at.getNamedAttr("callee", callee),
             at.getNamedAttr("parameters", at.getArrayAttr({})),
             at.getNamedAttr("type_arguments", at.getArrayAttr({})),
             at.getNamedAttr("occurrence",
                             sourceOccurrence(at, decl.symbol, statement))});
        if (!referenceBindings->empty())
          instance->setAttr("ac.reference_bindings", *referenceBindings);
        instanceStorage.push_back({name.str(), statement, callee, declaration});
        instanceStorage.back().op = instance;
        instanceStorage.back().clockInput = clockInput;
        instanceStorage.back().resetInput = resetInput;
        instances[name] = &instanceStorage.back();
        continue;
      }
      if (authoredInputs.contains(name))
        return error()
               << "module-local assignment cannot rebind an authored input";
      // The shared statement owner reads committed state and retains ordinary
      // binding facts; it cannot publish a candidate or write a state owner.
      if (failed(ordinaryStatement(i)))
        return failure();
      continue;
    }
    return error() << "unsupported typed transaction module statement";
  }


  SmallVector<ValueHelperDecl *> lexicalHelpers;
  for (auto &entry : valueHelpers)
    if (entry.second->lexicalOwner == decl.symbol) lexicalHelpers.push_back(entry.second.get());
  for (auto *helper : lexicalHelpers)
    if (failed(lowerValueHelper(*helper))) return failure();

  for (auto &entry : methodCaptures)
    if (methods.find(entry.first()) != methods.end() && methods.find(entry.first())->second.lexical)
      retainHelperSnapshots(entry.second);
  for (auto &entry : ruleCaptures) retainHelperSnapshots(entry.second);

  SmallVector<Value> moduleOutputs;
  llvm::DenseSet<DictionaryAttr> ordinaryModuleReturns;
  bool returned = false;
  for (size_t i = 0; i < decl.node.array("body").size(); ++i) {
    AstNode statement = decl.node.item("body", i);
    if (staticReferenceAssignments.contains(namespaceSite(statement)))
      continue;
    AstNode call = statement.kind() == "Expr"     ? statement.child("value")
                   : statement.kind() == "Return" ? statement.child("value")
                   : statement.kind() == "Assign" ? statement.child("value")
                                                  : AstNode();
    bool localDeclaration = statement.kind() == "FunctionDef" &&
                            rules.contains(statement.string("name")) &&
                            rules[statement.string("name")].lexical;
    if (localDeclaration) call = statement;
    bool registration = localDeclaration || (call.kind() == "Call" &&
                        call.child("func").kind() == "Name" &&
                        rules.contains(call.child("func").string("id")));
    if (statement.kind() == "Return") {
      if (returned)
        return error() << "module requires one final return";
      returned = true;
    }
    if (!registration) {
      if (statement.kind() != "Return")
        continue;
      llvm::StringMap<Value> returnedValues(moduleValues);
      for (const auto &entry : states) {
        Value oldQ = entry.second.op->getResult(0);
        rememberState(oldQ, entry.second);
        returnedValues[entry.first()] = oldQ;
      }
      auto outputs = lowerReturn(decl, call, at, returnedValues, instances);
      if (failed(outputs))
        return failure();
      llvm::append_range(moduleOutputs, *outputs);
      ordinaryModuleReturns.insert(sourceOccurrence(at, decl.symbol, statement));
      continue;
    }
    StringRef name = localDeclaration ? call.string("name") : call.child("func").string("id");
    DictionaryAttr invocation = sourceOccurrence(at, decl.symbol, call);
    SourceCallableDecl &callable = registeredCallables[invocation];
    Operation *transaction = registeredTransactions.lookup(invocation);
    auto methodName = registrationMethods.find(invocation);
    if (!transaction || methodName == registrationMethods.end())
      return error() << "Rule registration lacks its shared callable entry";
    transaction->setAttr("ac.registration_entry", at.getUnitAttr());
    SmallVector<Value> captures(ruleArguments[invocation]);
    transaction->insertOperands(2, captures);
    for (unsigned regionIndex = 0; regionIndex < 2; ++regionIndex) {
      auto *block = new Block();
      transaction->getRegion(regionIndex).push_back(block);
      SmallVector<Value> arguments;
      for (Value original : captures)
        arguments.push_back(captureValue(block, original, transaction->getLoc()));
      OpBuilder inside = OpBuilder::atBlockEnd(block);
      auto path = trueValue(call, inside, decl.symbol);
      if (failed(path)) return failure();
      SmallVector<Value> yielded;
      if (regionIndex == 0) yielded.push_back(*path);
      else {
        SmallVector<Type> physicalResults(callable.results);
        for (const Port &port : callable.inputs)
          if (callable.assignedFormals.contains(port.name) &&
              !callable.stateFormals.contains(port.name)) {
            physicalResults.push_back(port.type);
            physicalResults.push_back(internalBits(1, callable.declaration, decl.symbol));
          }
        auto callee = SymbolRefAttr::get(b.getContext(), decl.symbol.getValue(),
            {FlatSymbolRefAttr::get(b.getContext(), methodName->second)});
        SmallVector<Value> operands{*path};
        llvm::append_range(operands, arguments);
        Operation *entry = createOp(inside, call.location(b.getContext(), source.path),
            ac::SourceMethodCallOp::getOperationName(), operands, physicalResults,
            {inside.getNamedAttr("callee", callee),
             inside.getNamedAttr("instance_path", inside.getArrayAttr({})),
             inside.getNamedAttr("occurrence", transaction->getAttr("occurrence")),
             inside.getNamedAttr("ac.location",
                                 sourceSpan(inside, source.path, call))});
        llvm::append_range(yielded, entry->getResults().take_front(callable.results.size()));
      }
      createOp(inside, call.location(b.getContext(), source.path),
          ac::YieldOp::getOperationName(), yielded, {}, {});
    }
    if (statement.kind() == "Return") {
      if (decl.outputs.size() != callable.resultContracts.size())
        return error() << "Rule registration return contract disagrees with module outputs";
      for (auto [output, result] : llvm::zip(decl.outputs, callable.resultContracts))
        if (output.sourceKind != result.sourceKind || output.fixedBits != result.fixedBits ||
            !ac::areEquivalentHardwareTypes(output.type, result.type))
          return error() << "Rule registration return contract disagrees with module outputs";
      llvm::append_range(moduleOutputs, transaction->getResults());
    }
  }
  for (size_t pending = 0; pending < methodWork.size(); ++pending)
    if (failed(emitMethod(methodWork[pending]))) return failure();

  for (size_t i = 0; i < decl.node.array("body").size(); ++i) {
    AstNode statement = decl.node.item("body", i);
    if (staticReferenceAssignments.contains(namespaceSite(statement)))
      continue;
    if (statement.kind() == "FunctionDef" || statement.kind() == "AnnAssign" ||
        docstring(statement) || statement.kind() == "Pass")
      continue;
    AstNode value = statement.kind() == "Expr"     ? statement.child("value")
                    : statement.kind() == "Return" ? statement.child("value")
                    : statement.kind() == "Assign" ? statement.child("value")
                                                   : AstNode();
    bool registration = value.kind() == "Call" &&
                        value.child("func").kind() == "Name" &&
                        rules.contains(value.child("func").string("id"));
    bool construction =
        statement.kind() == "Assign" && value.kind() == "Call" &&
        methodOwner(value.child("func"));
    bool alias = statement.kind() == "Assign" && value.kind() == "Name" &&
                 instances.contains(value.string("id"));
    bool ordinary = ordinaryModuleAssignments.contains(
        sourceOccurrence(at, decl.symbol, statement));
    bool stateDeclaration = statement.kind() == "Assign" &&
        statement.array("targets").size() == 1 &&
        states.contains(assignmentRoot(statement.item("targets", 0))) &&
        sameSourceSite(states[assignmentRoot(statement.item("targets", 0))].node,
                       statement);
    if (registration || construction || alias || ordinary || stateDeclaration ||
        ordinaryModuleReturns.contains(
            sourceOccurrence(at, decl.symbol, statement)))
      continue;
    if (statement.kind() == "Expr" || statement.kind() == "Return")
      return error() << "Method calls require a registered Rule transaction";
    return error() << "unsupported typed transaction module statement";
  }
  if (moduleOutputs.size() != decl.outputs.size())
    return error()
           << "typed transaction module return does not match its interface";
  for (const auto &site : directCallSites)
    if (!site.second.bound)
      return error() << "direct module call site was not bound";
  for (const Instance &instance : instanceStorage)
    if (instance.node.kind() == "Assign" &&
        structuralCallSites.contains(sourceOccurrence(
            at, decl.symbol, instance.node.child("value"))) && !instance.bound)
      return error() << "unbound structural instance";
  structuralCallSites.clear();
  directCallSites.clear();
  forwardCallBindings.clear();
  createOp(at, module->getLoc(), ac::YieldOp::getOperationName(), moduleOutputs,
           {}, {});
  return success();
}

FailureOr<SmallVector<Value>> Importer::sourceMethodCall(
    const AstNode &node, OpBuilder &at, FlatSymbolRefAttr owner,
    llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances,
    std::optional<bool> tupleCall) {
  if (!activeSourceEffects || node.kind() != "Call")
    return failure();
  AstNode function = node.child("func");
  SourceCallableDecl *target = nullptr;
  std::optional<ac::SourceMethodView> imported;
  Instance *receiver = nullptr;
  const StaticInstanceReference *relative = nullptr;
  bool recognizedReceiver = false;
  bool nativeQueue = false;
  StringRef methodName;
  std::string specializedMethodName;
  SmallVector<Attribute> path;
  StringAttr reference;
  FlatSymbolRefAttr moduleSymbol;
  if (function.kind() == "Name" && activeSourceEffects->methods) {
    methodName = function.string("id");
    auto found = activeSourceEffects->methods->find(methodName);
    if (found != activeSourceEffects->methods->end()) {
      target = &found->second;
      moduleSymbol = activeSourceEffects->module->symbol;
    }
  } else if (function.kind() == "Attribute" &&
             function.child("value").kind() == "Name") {
    StringRef receiverName = function.child("value").string("id");
    auto found = instances.find(receiverName);
    if (found != instances.end()) {
      recognizedReceiver = true;
      receiver = found->second;
      methodName = function.string("attr");
      if (memoryInstance(receiver))
        return memoryMethodCall(node, *receiver, at, owner, values, instances,
                                tupleCall);
      nativeQueue = queueInstance(receiver);
      if (nativeQueue && methodName != "push" && methodName != "pop" &&
          methodName != "peek")
        return error() << "unknown Queue operation";
      bool localReceiver = false;
      for (auto &candidate : modules)
        if (candidate.symbol == receiver->callee) {
          localReceiver = true;
          auto methods = sourceMethods.find(candidate.node.string("name"));
          if (methods != sourceMethods.end()) {
            auto method = methods->second.find(methodName);
            if (method != methods->second.end())
              target = &method->second;
          }
          break;
        }
      if (!localReceiver && !nativeQueue) {
        auto view = headers.lookupModule(receiver->callee);
        if (!view)
          return error() << "Method receiver lacks a validated module contract";
        auto method = ac::lookupSourceMethod(view.declaration, methodName,
                                             body->getOperation());
        if (failed(method))
          return failure();
        imported = *method;
      }
      moduleSymbol = receiver->callee;
      // Aliases retain the actual instance identity, not their local spelling.
      if (!receiver->reference.empty())
        reference = at.getStringAttr(receiver->reference);
      else path.push_back(at.getStringAttr(receiver->name));
    } else {
      auto &references = staticInstanceReferences[activeSourceEffects->module->symbol.getValue()];
      auto selected = references.find(receiverName);
      if (selected != references.end()) {
        relative = &selected->second;
        if (relative->ancestorDepth || relative->childNames.size() != 1)
          return error() << "Rule receiver must be a direct child or declared Module-reference interface";
        recognizedReceiver = true;
        methodName = function.string("attr");
        moduleSymbol = relative->expectedModule;
        llvm::append_range(path, relative->childNames);
        bool localProvider = false;
        for (const auto &candidate : modules)
          if (candidate.symbol == moduleSymbol) {
            localProvider = true;
            auto inventory = sourceMethods.find(candidate.node.string("name"));
            if (inventory != sourceMethods.end()) {
              auto method = inventory->second.find(methodName);
              if (method != inventory->second.end()) target = &method->second;
            }
            break;
          }
        if (!localProvider) {
          auto provider = headers.lookupModule(moduleSymbol);
          if (!provider)
            return error() << "reference Method requires its authenticated provider interface";
          auto method = ac::lookupSourceMethod(provider.declaration, methodName, body->getOperation());
          if (failed(method))
            return failure();
          imported = *method;
        }
      }
    }
  }
  if (!target && !imported && !nativeQueue && recognizedReceiver)
    return error() << "unknown Method on statically identified instance";
  if (!target && !imported && !nativeQueue)
    return failure();
  if (nativeQueue && activeSourceEffects->callable &&
      !activeSourceEffects->callable->lexical)
    return error() << "global Rule cannot capture a caller Module Queue handle";
  if (target && activeSourceEffects->callable &&
      !activeSourceEffects->callable->lexical && target->lexical)
    return error() << "source-global Rule cannot capture a Module-local callable";
  auto importedVisibility = imported ? imported->getVisibility() : StringAttr();
  if ((target && (target->autonomous || target->privateSpecialization || !target->stateFormals.empty())) ||
      (importedVisibility && importedVisibility.getValue() == "private"))
    return error() << "private compiler callable is not an authored source interface";
  if (relative && activeSourceEffects->callable && !activeSourceEffects->callable->lexical)
    return error() << "source-global Rule cannot capture a Module-local static reference";
  if (target && target->inferredSignature && (receiver || relative))
    return error() << "callable Rule export requires explicit parameter and result annotations";
  if (activeSourceEffects->guard)
    return error() << "guard cannot call a Method";
  if (activeSourceEffects->pureExpression)
    return error()
           << "Method calls are forbidden in branch tests and conditional expressions";

  SmallVector<Port> importedInputs, importedResults;
  SmallVector<Type> importedResultTypes;
  SmallVector<unsigned> proposalInputs;
  if (imported) {
    for (Attribute raw : imported->getParameters()) {
      auto parameter = cast<DictionaryAttr>(raw);
      importedInputs.push_back(sourceCallPort(
          parameter.getAs<DictionaryAttr>("constraint"),
          parameter.getAs<StringAttr>("name").getValue(), node));
    }
    for (Attribute raw : imported->getResultConstraints())
      importedResults.push_back(
          sourceCallPort(cast<DictionaryAttr>(raw), "result", node));
    llvm::append_range(importedResultTypes,
                       imported->getFunctionType().getResults());
    auto proposals = imported->getFormalProposals();
    if (proposals)
      for (Attribute raw : proposals)
        proposalInputs.push_back(cast<IntegerAttr>(raw).getInt());
  } else if (nativeQueue) {
    Port payload = receiver->outputs[2];
    if (methodName == "push") {
      payload.name = "value";
      importedInputs.push_back(payload);
    } else {
      payload.name = "result";
      importedResults.push_back(payload);
      importedResultTypes.push_back(payload.type);
    }
  }
  if (target && target->syntax.marker == MarkerKind::Rule)
    for (auto [index, port] : llvm::enumerate(target->inputs))
      if (target->assignedFormals.contains(port.name) &&
          !target->stateFormals.contains(port.name)) proposalInputs.push_back(index);
  SmallVector<Port> inputContracts = target ? target->inputs : importedInputs;
  ArrayRef<Port> inputs = inputContracts;
  SmallVector<StringRef> formalNames;
  for (const Port &port : inputs) formalNames.push_back(port.name);
  auto actuals = normalizeCallArguments(node, formalNames, error);
  if (failed(actuals)) return failure();
  llvm::StringSet<> writableOwners;
  llvm::StringSet<> writableFormals;
  SmallVector<Attribute> formalOwnerBindings;
  SmallVector<unsigned> deferredReferences;
  for (unsigned index : proposalInputs) {
    auto actual = (*actuals)[index];
    if (actual.kind() != "Name" || !activeSourceEffects->states) continue;
    auto state = activeSourceEffects->states->find(actual.string("id"));
    if (state == activeSourceEffects->states->end()) {
      std::string intentName = ("$intent." + actual.string("id")).str();
      if (!values.contains(intentName) || !activeSourceEffects->callable) continue;
      if (!writableFormals.insert(actual.string("id")).second)
        return error() << "overlapping writable owner aliases";
      auto &callerInputs = activeSourceEffects->callable->inputs;
      auto formal = llvm::find_if(callerInputs, [&](const Port &port) {
        return port.name == actual.string("id");
      });
      if (formal == callerInputs.end()) return error() << "forwarded formal lacks its caller contract";
      deferredReferences.push_back(index);
      formalOwnerBindings.push_back(at.getDictionaryAttr(
          {at.getNamedAttr("formal", at.getI64IntegerAttr(index)),
           at.getNamedAttr("caller_formal", at.getI64IntegerAttr(std::distance(callerInputs.begin(), formal)))}));
      continue;
    }
    if (!writableOwners.insert(state->second.name).second)
      return error() << "overlapping writable owner aliases";
    const Port &contract = inputs[index];
    auto boundary = ownerBoundary(state->second);
    auto equivalent = contract.type ? equivalentBoundaryTypes(boundary.contract().type, contract.type,
                                               at.getBlock()->getParentOp()) : FailureOr<bool>(true);
    if (contract.type && (failed(equivalent) || !*equivalent ||
        boundary.contract().sourceKind != contract.sourceKind ||
        boundary.contract().fixedBits != contract.fixedBits))
      return error() << "writable Rule owner actual requires its exact declared type and kind";
    deferredReferences.push_back(index);
    formalOwnerBindings.push_back(at.getDictionaryAttr(
        {at.getNamedAttr("formal", at.getI64IntegerAttr(index)),
         at.getNamedAttr("state", at.getStringAttr(state->second.name))}));
  }
  // Private local owner specializations omit D/E suffixes, but their owner
  // reference inputs still use the same call-entry read ordering.
  if (target)
    for (auto [index, port] : llvm::enumerate(inputs))
      if (target->stateFormals.contains(port.name))
        deferredReferences.push_back(index);
  auto arguments = bindCallableArguments(node, inputs, at, owner, values, instances,
                                         deferredReferences);
  if (failed(arguments)) return failure();
  if (target && target->syntax.marker == MarkerKind::Rule && !receiver && !relative) {
    SourceCallableDecl specialized = *target;
    for (auto [index, port] : llvm::enumerate(specialized.inputs)) {
      auto actual = (*actuals)[index];
      auto binding = inferredBoundary(port.name, (*arguments)[index], actual, true);
      bool ownerActual = false;
      if (actual.kind() == "Name" && activeSourceEffects->states) {
        auto state = activeSourceEffects->states->find(actual.string("id"));
        if (state != activeSourceEffects->states->end()) {
          binding = ownerBoundary(state->second);
          ownerActual = true;
          if (specialized.assignedFormals.contains(port.name))
            specialized.stateFormals[port.name] = state->second.name;
        }
      }
      auto bound = bindRuleFormal(port.node, actual, (*arguments)[index], binding,
                                   ownerActual, at, owner);
      if (failed(bound)) return failure();
      if (!port.type) {
        auto formalNode = port.node;
        port = bound->boundary.contract();
        port.name = bound->name;
        port.node = formalNode;
      }
      (*arguments)[index] = bound->value;
    }
    if (specialized.inferredSignature || !specialized.stateFormals.empty()) {
      if (!activeSourceEffects->specialize)
        return error() << "Rule actual specialization lacks its lexical worklist";
      if (activeSourceEffects->callable) {
        const auto &caller = *activeSourceEffects->callable;
        if (sameSourceSite(caller.declaration, specialized.declaration) ||
            llvm::is_contained(caller.specializationAncestry, specialized.declaration.value))
          return error() << "recursive Rule calls are unsupported";
        specialized.specializationAncestry = caller.specializationAncestry;
        specialized.specializationAncestry.push_back(caller.declaration.value);
      }
      specialized.actualFacts.assign(arguments->begin(), arguments->end());
      SmallVector<Type> parameterTypes;
      for (const Port &port : specialized.inputs) parameterTypes.push_back(port.type);
      specialized.type = FunctionType::get(b.getContext(), parameterTypes, specialized.results);
      auto resolved = (*activeSourceEffects->specialize)(methodName, specialized);
      if (failed(resolved)) return failure();
      specializedMethodName = *resolved;
      methodName = specializedMethodName;
      target = &activeSourceEffects->methods->find(methodName)->second;
      inputContracts = target->inputs;
      inputs = inputContracts;
      llvm::erase_if(proposalInputs, [&](unsigned index) {
        return target->stateFormals.contains(inputs[index].name);
      });
      llvm::erase_if(formalOwnerBindings, [&](Attribute raw) {
        unsigned index = cast<DictionaryAttr>(raw).getAs<IntegerAttr>("formal").getInt();
        return !llvm::is_contained(proposalInputs, index);
      });
    }
  }
  ArrayRef<Port> resultContracts = target ? ArrayRef<Port>(target->resultContracts)
                                         : ArrayRef<Port>(importedResults);
  SmallVector<Type> physicalResults = target ? target->results : importedResultTypes;
  if (target)
    for (unsigned index : proposalInputs) {
      physicalResults.push_back(inputs[index].type);
      physicalResults.push_back(internalBits(1, target->declaration, moduleSymbol));
    }
  size_t publicResults = physicalResults.size() - 2 * proposalInputs.size();
  bool tupleResult = target ? target->returnShape == ReturnShape::Tuple ||
                                 (target->returnShape == ReturnShape::Mapping && target->results.size() != 1)
                            : imported ? imported->getReturnForm().getValue() == "tuple"
                                       : false;
  if (tupleCall && tupleResult != *tupleCall)
    return error() << (tupleResult ? "tuple Method call requires direct Tuple unpacking"
                                  : "single Method result cannot be Tuple-unpacked");
  auto enabled = trueValue(node, at, owner);
  if (failed(enabled))
    return failure();
  auto nested = FlatSymbolRefAttr::get(b.getContext(), methodName);
  auto callee =
      SymbolRefAttr::get(b.getContext(), moduleSymbol.getValue(), {nested});
  auto operation = createOp(
      at, node.location(b.getContext(), source.path),
      ac::SourceMethodCallOp::getOperationName(), SmallVector<Value>{*enabled},
      physicalResults,
      {at.getNamedAttr("callee", callee),
       at.getNamedAttr("instance_path", at.getArrayAttr(path)),
       at.getNamedAttr("occurrence", sourceOccurrence(at, owner, node)),
       at.getNamedAttr("ac.location", sourceSpan(at, source.path, node))});
  if (reference) operation->setAttr("ac.reference", reference);
  if (relative) {
    operation->setAttr("receiver_kind", at.getStringAttr("reference"));
    operation->setAttr("ancestor_depth", at.getI64IntegerAttr(relative->ancestorDepth));
  }
  operation->insertOperands(1, *arguments);
  if (!formalOwnerBindings.empty())
    operation->setAttr("ac.formal_owner_bindings", at.getArrayAttr(formalOwnerBindings));
  for (auto [result, contract] :
       llvm::zip(operation->getResults(), resultContracts)) {
    remember(result, contract.sourceKind);
    if (contract.fixedBits)
      fixedValues.insert(result);
  }
  for (auto [ordinal, index] : llvm::enumerate(proposalInputs)) {
    auto actual = (*actuals)[index];
    if (actual.kind() != "Name") continue;
    Value data = operation->getResult(publicResults + 2 * ordinal);
    Value intent = operation->getResult(publicResults + 2 * ordinal + 1);
    remember(data, inputs[index].sourceKind);
    if (inputs[index].fixedBits) fixedValues.insert(data);
    remember(intent, ac::detail::ValueKind::Boolean);
    fixedValues.erase(intent);
    std::string intentName = ("$intent." + actual.string("id")).str();
    if (Value prior = values.lookup(intentName)) {
      // A lexical Rule formal is an ordinary SSA snapshot until the outer
      // invocation binds its proposal to an actual physical owner.
      Value previous = values.lookup(actual.string("id"));
      auto selected = select(intent, data, previous, node, at, owner);
      if (failed(selected)) return failure();
      values[intentName] = createOp(at, node.location(b.getContext(), source.path),
          ac::BitsBinaryOp::getOperationName(), {prior, intent}, {prior.getType()},
          {at.getNamedAttr("opcode", at.getStringAttr("or"))})->getResult(0);
      remember(values.lookup(intentName), ac::detail::ValueKind::Boolean);
      auto &callerInputs = activeSourceEffects->callable->inputs;
      auto formal = llvm::find_if(callerInputs, [&](const Port &port) {
        return port.name == actual.string("id");
      });
      if (formal == callerInputs.end()) return error() << "forwarded proposal lacks its caller formal";
      Value snapshot =
          createOp(
              at, node.location(b.getContext(), source.path), "ac.formal.write",
              {data, intent, *selected, previous}, {previous.getType()},
              {at.getNamedAttr("formal", at.getI64IntegerAttr(std::distance(
                                             callerInputs.begin(), formal))),
               at.getNamedAttr("selectors", at.getArrayAttr({})),
               at.getNamedAttr("operandSegmentSizes",
                               at.getDenseI32ArrayAttr({1, 1, 1, 0, 1})),
               at.getNamedAttr("occurrence", sourceOccurrence(at, owner, node)),
               at.getNamedAttr("ac.location",
                               sourceSpan(at, source.path, node))})
              ->getResult(0);
      remember(snapshot, formal->sourceKind);
      if (formal->fixedBits) fixedValues.insert(snapshot);
      values[actual.string("id")] = snapshot;
      continue;
    }
    if (!activeSourceEffects->states) continue;
    auto state = activeSourceEffects->states->find(actual.string("id"));
    if (state == activeSourceEffects->states->end()) continue;
    createOp(at, node.location(b.getContext(), source.path),
             ac::SourceStateWriteOp::getOperationName(), {intent, data}, {},
             {at.getNamedAttr("state", at.getStringAttr(state->second.name)),
              at.getNamedAttr("selectors", at.getArrayAttr({})),
              at.getNamedAttr("occurrence", sourceOccurrence(at, owner, node))});
  }
  return SmallVector<Value>(operation->getResults().take_front(publicResults));
}

FailureOr<std::optional<Value>> Importer::sourceEffectExpression(
    const AstNode &node, OpBuilder &at, FlatSymbolRefAttr owner,
    llvm::StringMap<Value> &values, llvm::StringMap<Instance *> &instances) {
  if (!activeSourceEffects)
    return std::optional<Value>();
  if (node.kind() == "Name" && !queryBound(node.string("id")) &&
      activeSourceEffects->states) {
    auto found = activeSourceEffects->states->find(node.string("id"));
    if (found != activeSourceEffects->states->end()) {
      auto path = trueValue(node, at, owner);
      if (failed(path))
        return failure();
      auto read = createOp(
          at, node.location(b.getContext(), source.path),
          ac::SourceStateReadOp::getOperationName(), {*path},
          TypeRange{found->second.type},
          {at.getNamedAttr("state", at.getStringAttr(found->second.name)),
           at.getNamedAttr("selectors", at.getArrayAttr({})),
           at.getNamedAttr("occurrence", sourceOccurrence(at, owner, node))});
      rememberState(read->getResult(0), found->second);
      return std::optional<Value>(read->getResult(0));
    }
  }
  bool methodCall = false;
  if (node.kind() == "Call") {
    AstNode function = node.child("func");
    methodCall = function.kind() == "Name" && activeSourceEffects->methods &&
                 activeSourceEffects->methods->contains(function.string("id"));
    if (function.kind() == "Attribute" && function.child("value").kind() == "Name") {
      StringRef receiver = function.child("value").string("id");
      methodCall |= instances.contains(receiver);
      if (activeSourceEffects->module) {
        auto references = staticInstanceReferences.find(activeSourceEffects->module->symbol.getValue());
        methodCall |= references != staticInstanceReferences.end() &&
                      references->second.contains(receiver);
      }
    }
  }
  if (!methodCall)
    return std::optional<Value>();
  auto results = sourceMethodCall(node, at, owner, values, instances);
  if (failed(results))
    return failure();
  if (results->size() != 1)
    return error() << "Method expression requires exactly one result";
  return std::optional<Value>(results->front());
}
