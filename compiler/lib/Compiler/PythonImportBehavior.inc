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

LogicalResult
Importer::preflightForCapabilities(const AstNode &statement,
                                   LoopAdmissionSummary &summary) {
  PureScalarSummary inspected;
  SmallVector<FlatSymbolRefAttr> defaultConstructors;
  auto visit = [&](auto &&self, const AstNode &node,
                   bool target = false) -> LogicalResult {
    auto diagnostic = [&] {
      return mlir::emitError(node.location(b.getContext(), source.path));
    };
    std::string reason;
    if (!queryBudget.enter(reason))
      return diagnostic() << "bounded for preflight: " << reason;
    auto leave = llvm::scope_exit([&] { queryBudget.leave(); });
    if (failed(chargePureSource(node, inspected)) ||
        !pureCheckedAdd(summary.nodes, 1))
      return failure();
    auto child = [&](StringRef field, bool assignment = false) {
      return self(self, node.child(field), assignment);
    };
    auto list = [&](StringRef field) -> LogicalResult {
      for (size_t i = 0; i < node.array(field).size(); ++i)
        if (failed(self(self, node.item(field, i))))
          return failure();
      return success();
    };
    StringRef kind = node.kind();
    if (kind == "Pass" || kind == "Name")
      return success();
    if (kind == "Assign" || kind == "AnnAssign") {
      if (kind == "Assign" && node.array("targets").size() != 1)
        return diagnostic() << "bounded for assignment requires one target";
      auto destination =
          kind == "Assign" ? node.item("targets", 0) : node.child("target");
      if (destination.kind() != "Name" && destination.kind() != "Attribute" &&
          destination.kind() != "Subscript")
        return diagnostic() << "bounded for assignment target is unsupported";
      if (failed(self(self, destination, true)))
        return failure();
      if (kind == "AnnAssign") {
        // Annotation syntax is inspected without asking portType to type an
        // unreachable declaration. Calls in annotations are outside this slice.
        auto annotation = node.child("annotation");
        auto inspect = [&](auto &&walk, const AstNode &part) -> LogicalResult {
          if (!part || failed(chargePureSource(part, inspected)) ||
              !pureCheckedAdd(summary.nodes, 1))
            return failure();
          if (part.kind() == "Name" || part.kind() == "Constant")
            return success();
          if (part.kind() == "Attribute")
            return walk(walk, part.child("value"));
          if (part.kind() == "Subscript") {
            if (failed(walk(walk, part.child("value"))))
              return failure();
            return walk(walk, part.child("slice"));
          }
          if (part.kind() == "Tuple" || part.kind() == "List") {
            for (size_t i = 0; i < part.array("elts").size(); ++i)
              if (failed(walk(walk, part.item("elts", i))))
                return failure();
            return success();
          }
          return diagnostic()
                 << "bounded for annotation capability is unsupported";
        };
        if (failed(inspect(inspect, annotation)))
          return failure();
      }
      if (!node.child("value"))
        return diagnostic() << "bounded for assignment requires a value";
      return child("value");
    }
    if (kind == "If") {
      if (failed(child("test")) || failed(list("body")))
        return failure();
      return list("orelse");
    }
    if (kind == "Constant") {
      if (target)
        return diagnostic() << "bounded for target cannot be a literal";
      auto encoded = dyn_cast_or_null<DictionaryAttr>(node.get("value"));
      if (isa_and_nonnull<BoolAttr>(node.get("value")) ||
          (encoded && encoded.getAs<StringAttr>("integer")))
        return success();
      return diagnostic() << "bounded for literal requires Integer or Boolean";
    }
    if (kind == "Attribute")
      return child("value", target);
    if (kind == "Subscript") {
      if (failed(child("value", target)))
        return failure();
      auto index = node.child("slice");
      if (target && index.kind() == "Slice")
        return diagnostic() << "bounded for slice assignment is unsupported";
      return self(self, index);
    }
    if (kind == "Slice") {
      for (StringRef field : {"lower", "upper", "step"})
        if (node.child(field) && failed(child(field)))
          return failure();
      return success();
    }
    if (kind == "Tuple" || kind == "List")
      return target ? diagnostic() << "bounded for unpacking is unsupported"
                    : list("elts");
    if (kind == "UnaryOp") {
      auto operation = node.child("op");
      if (failed(chargePureSource(operation, inspected)) ||
          !pureCheckedAdd(summary.nodes, 1))
        return failure();
      if (!llvm::is_contained(ArrayRef<StringRef>{"Invert", "Not"},
                              operation.kind()))
        return diagnostic() << "bounded for unary operation is unsupported";
      return child("operand");
    }
    if (kind == "BinOp" || kind == "Compare") {
      bool comparison = kind == "Compare";
      if (comparison && (node.array("ops").size() != 1 ||
                         node.array("comparators").size() != 1))
        return diagnostic() << "bounded for comparison must be single";
      auto operation = comparison ? node.item("ops", 0) : node.child("op");
      if (failed(chargePureSource(operation, inspected)) ||
          !pureCheckedAdd(summary.nodes, 1))
        return failure();
      bool admitted =
          comparison
              ? llvm::is_contained(ArrayRef<StringRef>{"Eq", "NotEq", "Lt",
                                                       "LtE", "Gt", "GtE"},
                                   operation.kind())
              : llvm::is_contained(ArrayRef<StringRef>{"Add", "Sub", "Mult",
                                                       "BitAnd", "BitOr",
                                                       "BitXor"},
                                   operation.kind());
      if (!admitted)
        return diagnostic() << "bounded for binary operation is unsupported";
      if (failed(child("left")))
        return failure();
      return comparison ? self(self, node.item("comparators", 0))
                        : child("right");
    }
    if (kind == "BoolOp") {
      auto operation = node.child("op");
      if (failed(chargePureSource(operation, inspected)) ||
          !pureCheckedAdd(summary.nodes, 1))
        return failure();
      if (operation.kind() != "And" && operation.kind() != "Or")
        return diagnostic() << "bounded for Boolean operation is unsupported";
      return list("values");
    }
    if (kind == "IfExp") {
      if (failed(child("test")) || failed(child("body")))
        return failure();
      return child("orelse");
    }
    if (kind == "Call") {
      auto function = node.child("func");
      if (failed(chargePureSource(function, inspected)) ||
          !pureCheckedAdd(summary.nodes, 1))
        return failure();
      auto binding = lookupBinding(function);
      bool constructor =
          binding && binding->category == BindingCategory::Struct;
      if (!constructor && !concatCall(node) &&
          !intrinsic(function, "enum_to_bits")) {
        if (function.kind() != "Name" ||
            !behavioralRules.contains(function.string("id")))
          return diagnostic() << "bounded for call capability is unsupported";
        if (!summary.helpers.contains(node.value)) {
          PureScalarSummary helper;
          helper.arity = node.array("args").size();
          auto base = queryBudget.used(TableQueryResource::Nesting);
          SmallVector<AstNode> bodies;
          if (failed(preflightPureCall(node, {}, helper, bodies, false)))
            return failure();
          // LLVM 22.1.8 DenseMap starts with 64 buckets and doubles before
          // insertion at a 3/4 load. This append-only map has no tombstones.
          // Prepay the complete newly allocated bucket array: old storage
          // remains paid while grow() moves records into the new allocation.
          using HelperMap = decltype(summary.helpers);
          using HelperBucket = HelperMap::value_type;
          constexpr uint64_t slotBytes = sizeof(uint64_t);
          constexpr uint64_t bucketSlots =
              (sizeof(HelperBucket) + slotBytes - 1) / slotBytes;
          constexpr uint64_t mapSlots =
              (sizeof(HelperMap) + slotBytes - 1) / slotBytes;
          constexpr uint64_t minimumBuckets = 64;
          uint64_t oldBuckets =
              summary.helpers.getMemorySize() / sizeof(HelperBucket);
          uint64_t newEntries = summary.helpers.size();
          uint64_t load = 0, capacity = 0, allocatedBuckets = 0;
          bool fits = pureCheckedAdd(newEntries, 1) &&
                      pureCheckedProduct(newEntries, 4, load) &&
                      pureCheckedProduct(oldBuckets, 3, capacity);
          if (!oldBuckets)
            allocatedBuckets = minimumBuckets;
          else if (load >= capacity)
            fits &= pureCheckedProduct(oldBuckets, 2, allocatedBuckets);
          TableQueryCharge cache;
          // Also cover the insertion's temporary key/record and first map
          // header. sizeof(bucket) includes actual key/value padding.
          fits &= TableQueryBudget::add(cache, TableQueryResource::Slots,
                                        bucketSlots) &&
                  TableQueryBudget::addProduct(cache, TableQueryResource::Slots,
                                               allocatedBuckets, bucketSlots);
          if (!oldBuckets)
            fits &= TableQueryBudget::add(cache, TableQueryResource::Slots,
                                          mapSlots);
          if (!fits || !queryBudget.reserve(cache, reason))
            return diagnostic()
                   << "bounded for helper cache: "
                   << (fits ? reason
                            : "entry/allocation slots budget exhausted");
          summary.helpers[node.value] = {helper, helper.depth - base, 0};
        }
      }
      if (failed(list("args")))
        return failure();
      llvm::StringSet<> supplied;
      for (size_t i = 0; i < node.array("keywords").size(); ++i) {
        auto keyword = node.item("keywords", i);
        if (failed(chargePureSource(keyword, inspected)) ||
            !pureCheckedAdd(summary.nodes, 1))
          return failure();
        if (!constructor || keyword.string("arg").empty())
          return diagnostic()
                 << "bounded for call keyword capability is unsupported";
        supplied.insert(keyword.string("arg"));
        if (failed(self(self, keyword.child("value"))))
          return failure();
      }
      if (constructor) {
        auto defaults = structDefaults.find(function.string("id"));
        auto fields = structFields.find(function.string("id"));
        if (defaults != structDefaults.end() && fields != structFields.end()) {
          if (llvm::is_contained(defaultConstructors, binding->symbol))
            return diagnostic() << "bounded for used constructor default "
                                   "recursion is unsupported";
          defaultConstructors.push_back(binding->symbol);
          auto pop = llvm::scope_exit([&] { defaultConstructors.pop_back(); });
          for (auto [i, raw] : llvm::enumerate(fields->second)) {
            auto field = cast<DictionaryAttr>(raw);
            if (!supplied.contains(
                    field.getAs<StringAttr>("name").getValue()) &&
                i < defaults->second.size() && defaults->second[i] &&
                failed(self(self, defaults->second[i])))
              return failure();
          }
        }
      }
      return success();
    }
    return diagnostic() << "bounded for body capability '" << kind
                        << "' is unsupported";
  };
  for (size_t i = 0; i < statement.array("body").size(); ++i)
    if (failed(visit(visit, statement.item("body", i))))
      return failure();
  return success();
}

LogicalResult Importer::chargeLoopDescriptorWork(const AstNode &site,
                                                 StringRef phase,
                                                 uint64_t amount) {
  TableQueryCharge charge;
  std::string reason;
  bool fits = TableQueryBudget::add(charge, TableQueryResource::Work, amount);
  if (!fits || !queryBudget.reserve(charge, reason)) {
    auto diagnostic =
        mlir::emitError(site.location(b.getContext(), source.path));
    diagnostic << "bounded for ";
    if (activeLoop)
      diagnostic << "iteration " << activeLoop->iteration << " ";
    diagnostic << phase << ": "
               << (fits ? reason : "descriptor work budget exhausted");
    return failure();
  }
  return success();
}

LogicalResult Importer::prepayLoopAnalysis(const AstNode &site) {
  // One nonallocating counting visit and the following constructor visit are
  // prepaid before the constructor allocates its existing declaration index.
  for (Operation &operation : body->getBody()->getOperations()) {
    if (failed(chargeLoopDescriptorWork(site, "declaration index", 2)))
      return failure();
    (void)operation;
  }
  return success();
}

FailureOr<uint64_t> Importer::loopStorageWords(Type type,
                                               ac::HardwareAnalysis &analysis,
                                               const AstNode &site) {
  auto visit = [&]() {
    return chargeLoopDescriptorWork(site, "storage type visit");
  };
  return measureTableStorageWords(type, analysis, *body,
                                  site.location(b.getContext(), source.path), 0,
                                  visit);
}

FailureOr<LoopResourceShape>
Importer::loopTypeShape(Type type, const AstNode &site,
                        ac::HardwareAnalysis *sharedAnalysis) {
  std::optional<ac::HardwareAnalysis> localAnalysis;
  if (!sharedAnalysis) {
    if (failed(prepayLoopAnalysis(site)))
      return failure();
    localAnalysis.emplace(*body);
    sharedAnalysis = &*localAnalysis;
  }
  auto &analysis = *sharedAnalysis;
  auto loc = site.location(b.getContext(), source.path);
  auto words = loopStorageWords(type, analysis, site);
  if (failed(words))
    return failure();
  auto schema = [&](auto &&self, Type current,
                    unsigned depth) -> FailureOr<LoopResourceShape> {
    if (failed(chargeLoopDescriptorWork(site, "schema type visit")))
      return failure();
    if (depth >= TableQueryBudget::limit(TableQueryResource::Nesting))
      return mlir::emitError(loc)
             << "bounded for declaration nesting budget exhausted";
    LoopResourceShape result{current, 1, 1, 1, 0};
    if (auto table = dyn_cast<ac::TableType>(current)) {
      auto element = self(self, table.getElementType(), depth + 1);
      auto size = analysis.getTableSize(table, {}, body->getOperation());
      if (failed(element) || failed(size))
        return failure();
      result.precision =
          std::max(element->precision,
                   uint64_t(std::max(1u, llvm::Log2_64_Ceil(*size))));
      result.declarations += element->declarations;
      result.fields = element->fields;
    } else if (auto record = dyn_cast<ac::StructType>(current)) {
      auto declaration = analysis.lookupStruct(record);
      if (!declaration)
        return failure();
      result.fields = declaration.getFields().size();
      for (Attribute raw : declaration.getFields()) {
        if (failed(chargeLoopDescriptorWork(site, "schema field edge")))
          return failure();
        auto field = self(
            self, cast<DictionaryAttr>(raw).getAs<TypeAttr>("type").getValue(),
            depth + 1);
        if (failed(field) ||
            !pureCheckedAdd(result.declarations, field->declarations))
          return failure();
        result.precision = std::max(result.precision, field->precision);
        result.fields = std::max(result.fields, field->fields);
      }
    } else {
      auto width = analysis.getPackedWidth(current, {}, body->getOperation());
      if (failed(width) || !*width)
        return failure();
      result.precision = *width;
    }
    return result;
  };
  auto shape = schema(schema, type, 0);
  if (failed(shape))
    return failure();
  shape->words = *words;
  return shape;
}

FailureOr<LoopResourceShape> Importer::describeLoopExpression(
    const AstNode &node, const BranchEnvironment &environment,
    FlatSymbolRefAttr symbol, LoopStatementEnvelope &envelope,
    std::optional<Type> expected) {
  auto diagnostic = [&] {
    return mlir::emitError(node.location(b.getContext(), source.path));
  };
  if (!pureCheckedAdd(envelope.nodes, 1))
    return failure();
  auto include = [&](LoopResourceShape shape) -> FailureOr<LoopResourceShape> {
    envelope.precision = std::max(envelope.precision, shape.precision);
    envelope.words = std::max(envelope.words, shape.words);
    envelope.operands = std::max(envelope.operands, shape.fields);
    if (!pureCheckedAdd(envelope.declarations, shape.declarations))
      return failure();
    return shape;
  };
  auto typed = [&](Type type) -> FailureOr<LoopResourceShape> {
    auto shape = loopTypeShape(type, node);
    return failed(shape) ? FailureOr<LoopResourceShape>(failure())
                         : include(*shape);
  };
  auto operation = [&](uint64_t count) {
    return pureCheckedAdd(envelope.operations, count);
  };
  auto visit = [&](const AstNode &child,
                   std::optional<Type> hint = std::nullopt) {
    return describeLoopExpression(child, environment, symbol, envelope, hint);
  };
  auto scalar = [&](uint64_t width) -> FailureOr<LoopResourceShape> {
    if (!width ||
        width > 64 * TableQueryBudget::limit(TableQueryResource::PayloadWords))
      return diagnostic() << "bounded for scalar precision envelope exhausted";
    return include({{}, width, width / 64 + (width % 64 != 0), 0, 0});
  };
  if (node.kind() == "Name") {
    auto found = environment.values.find(node.string("id"));
    if (found == environment.values.end())
      return diagnostic() << "bounded for cannot reserve unbound value '"
                          << node.string("id") << "'";
    auto fact = numericValues.find(found->second);
    if (fact != numericValues.end() && fact->second.interval)
      for (const auto *endpoint :
           {&fact->second.interval->lower, &fact->second.interval->upper})
        envelope.factWords = std::max(
            envelope.factWords, uint64_t(endpoint->getBitWidth() / 64 +
                                         (endpoint->getBitWidth() % 64 != 0)));
    return typed(found->second.getType());
  }
  if (node.kind() == "Constant") {
    if (!operation(1))
      return failure();
    uint64_t width = 1;
    if (auto encoded = dyn_cast_or_null<DictionaryAttr>(node.get("value"))) {
      auto spelling = encoded.getAs<StringAttr>("integer");
      if (!spelling ||
          !pureCheckedProduct(4, spelling.getValue().size(), width) ||
          !pureCheckedAdd(width, 2))
        return failure();
    }
    if (expected) {
      auto hint = loopTypeShape(*expected, node);
      if (failed(hint))
        return failure();
      width = std::max(width, hint->precision);
      if (isa<ac::StructType, ac::TableType>(*expected)) {
        // Existing contextual zero constructs every declared leaf and Table.
        // Keep its aggregate footprint rather than describing a scalar zero.
        uint64_t construction = 0;
        if (!pureCheckedProduct(8, hint->declarations, construction) ||
            !operation(construction) ||
            !pureCheckedAdd(envelope.maps, hint->declarations))
          return failure();
        hint->precision = width;
        return include(*hint);
      }
    }
    return scalar(width);
  }
  if (node.kind() == "Attribute") {
    auto nominal = lookupBinding(node.child("value"));
    if (nominal && nominal->category == BindingCategory::Enum) {
      if (!operation(1))
        return failure();
      return typed(ac::EnumType::get(
          b.getContext(), b.getStringAttr(nominal->symbol.getValue())));
    }
    auto base = visit(node.child("value"));
    if (failed(base))
      return failure();
    auto record = dyn_cast_or_null<ac::StructType>(base->type);
    if (failed(prepayLoopAnalysis(node)))
      return failure();
    ac::HardwareAnalysis declarationAnalysis(*body);
    auto declaration =
        record ? declarationAnalysis.lookupStruct(record) : ac::StructOp();
    if (!declaration)
      return diagnostic() << "bounded for projection lacks a declared layout";
    for (Attribute raw : declaration.getFields()) {
      if (failed(chargeLoopDescriptorWork(node, "projection field edge")))
        return failure();
      auto field = cast<DictionaryAttr>(raw);
      if (field.getAs<StringAttr>("name").getValue() == node.string("attr")) {
        if (!operation(1))
          return failure();
        return typed(field.getAs<TypeAttr>("type").getValue());
      }
    }
    return diagnostic() << "bounded for projection lacks a declared field";
  }
  if (node.kind() == "Subscript") {
    auto base = visit(node.child("value"));
    if (failed(base))
      return failure();
    auto index = node.child("slice");
    if (index.kind() == "Slice") {
      for (StringRef field : {"lower", "upper", "step"})
        if (index.child(field) && failed(visit(index.child(field))))
          return failure();
      if (!operation(1))
        return failure();
      return scalar(base->precision);
    }
    if (failed(visit(index)))
      return failure();
    auto table = dyn_cast_or_null<ac::TableType>(base->type);
    if (!table)
      return diagnostic() << "bounded for indexed value lacks a declared Table";
    if (!operation(1))
      return failure();
    return typed(table.getElementType());
  }
  if (node.kind() == "UnaryOp") {
    auto input = visit(node.child("operand"));
    if (failed(input) || !operation(4))
      return failure();
    uint64_t width = input->precision;
    if (!pureCheckedAdd(width, 2))
      return failure();
    return scalar(node.child("op").kind() == "Not" ? 1 : width);
  }
  if (node.kind() == "BinOp" || node.kind() == "Compare") {
    bool comparison = node.kind() == "Compare";
    auto left = visit(node.child("left"));
    auto right =
        visit(comparison ? node.item("comparators", 0) : node.child("right"));
    if (failed(left) || failed(right) || !operation(6))
      return failure();
    uint64_t width = std::max(left->precision, right->precision);
    if (node.kind() == "BinOp" && node.child("op").kind() == "Mult") {
      width = left->precision;
      if (!pureCheckedAdd(width, right->precision))
        return failure();
    }
    if (!pureCheckedAdd(width, 2))
      return failure();
    envelope.precision = std::max(envelope.precision, width);
    return scalar(comparison ? 1 : width);
  }
  if (node.kind() == "BoolOp") {
    for (size_t i = 0; i < node.array("values").size(); ++i)
      if (failed(visit(node.item("values", i))))
        return failure();
    uint64_t count;
    if (!pureCheckedProduct(3, node.array("values").size(), count) ||
        !operation(count))
      return failure();
    return scalar(1);
  }
  if (node.kind() == "IfExp") {
    if (failed(visit(node.child("test"))))
      return failure();
    auto yes = visit(node.child("body"), expected);
    auto no = visit(node.child("orelse"), expected);
    if (failed(yes) || failed(no))
      return failure();
    uint64_t declarations = std::max(yes->declarations, no->declarations),
             count;
    if (!pureCheckedProduct(8, declarations, count) ||
        !pureCheckedAdd(count, 4) || !operation(count))
      return failure();
    auto result = yes->words >= no->words ? *yes : *no;
    result.precision = std::max(yes->precision, no->precision);
    // Existing exact Integer select may add a sign bit to a nonnegative arm.
    // This is conservative scalar scratch, not another source-kind inference.
    if ((!result.type || isa<ac::BitsType>(result.type)) &&
        !pureCheckedAdd(result.precision, 1))
      return failure();
    // A Table (including a Table field of a Struct) selects through existing
    // TableMap regions. Prepay their arguments before either arm is lowered.
    if (!pureCheckedAdd(envelope.maps, declarations))
      return failure();
    return include(result);
  }
  if (node.kind() == "Tuple" || node.kind() == "List") {
    auto table =
        expected ? dyn_cast<ac::TableType>(*expected) : ac::TableType();
    if (!table)
      return diagnostic()
             << "bounded for literal needs its declared Table context";
    envelope.operands =
        std::max(envelope.operands, uint64_t(node.array("elts").size()));
    for (size_t i = 0; i < node.array("elts").size(); ++i)
      if (failed(visit(node.item("elts", i), table.getElementType())))
        return failure();
    if (!operation(1))
      return failure();
    return typed(table);
  }
  if (node.kind() == "Call") {
    auto function = node.child("func");
    auto binding = lookupBinding(function);
    if (binding && binding->category == BindingCategory::Struct) {
      auto type = ac::StructType::get(
          b.getContext(), b.getStringAttr(binding->symbol.getValue()));
      auto shape = loopTypeShape(type, node);
      if (failed(shape))
        return failure();
      if (failed(prepayLoopAnalysis(node)))
        return failure();
      ac::HardwareAnalysis declarationAnalysis(*body);
      auto declaration = declarationAnalysis.lookupStruct(type);
      if (!declaration)
        return failure();
      llvm::StringSet<> supplied;
      if (!node.array("args").empty())
        return diagnostic() << "bounded for Struct constructor resource shape "
                               "requires keywords";
      for (size_t i = 0; i < node.array("keywords").size(); ++i) {
        auto keyword = node.item("keywords", i);
        StringRef name = keyword.string("arg");
        supplied.insert(name);
        Type hint;
        for (Attribute raw : declaration.getFields()) {
          if (failed(chargeLoopDescriptorWork(node, "constructor field edge")))
            return failure();
          auto field = cast<DictionaryAttr>(raw);
          if (field.getAs<StringAttr>("name").getValue() == name)
            hint = field.getAs<TypeAttr>("type").getValue();
        }
        if (!hint || failed(visit(keyword.child("value"), hint)))
          return failure();
      }
      auto localDefaults = structDefaults.find(function.string("id"));
      if (localDefaults != structDefaults.end())
        for (auto [i, raw] : llvm::enumerate(declaration.getFields())) {
          if (failed(chargeLoopDescriptorWork(
                  node, "constructor default field edge")))
            return failure();
          auto field = cast<DictionaryAttr>(raw);
          if (!supplied.contains(field.getAs<StringAttr>("name").getValue()) &&
              i < localDefaults->second.size() && localDefaults->second[i] &&
              failed(visit(localDefaults->second[i],
                           field.getAs<TypeAttr>("type").getValue())))
            return failure();
        }
      uint64_t count;
      if (!pureCheckedProduct(8, shape->declarations, count) ||
          !operation(count) ||
          !pureCheckedAdd(envelope.maps, shape->declarations))
        return failure();
      return include(*shape);
    }
    if (concatCall(node) || intrinsic(function, "enum_to_bits")) {
      uint64_t width = 0;
      for (size_t i = 0; i < node.array("args").size(); ++i) {
        auto argument = visit(node.item("args", i));
        if (failed(argument) || !pureCheckedAdd(width, argument->precision))
          return failure();
      }
      envelope.operands =
          std::max(envelope.operands, uint64_t(node.array("args").size()));
      if (!operation(1))
        return failure();
      return scalar(width);
    }
    auto declaration = pureRuleDeclaration(node);
    if (failed(declaration))
      return failure();
    for (size_t i = 0; i < node.array("args").size(); ++i)
      if (failed(visit(node.item("args", i))))
        return failure();
    // Only the declaration's closed fixed return schema is inspected. Existing
    // helper ownership pays for its body, argument conversions and clones.
    auto annotation = declaration->child("returns");
    auto type = portType(annotation, symbol);
    if (failed(type))
      return failure();
    return typed(*type);
  }
  return diagnostic() << "bounded for cannot reserve expression '"
                      << node.kind() << "'";
}

LogicalResult Importer::reserveLoopEnvironmentCopies(
    const AstNode &site, const BranchEnvironment &incoming,
    const BranchEnvironment *yes, const BranchEnvironment *no) {
  TableQueryCharge charge;
  bool fits = true;
  StringRef phase =
      yes ? "join environment copies" : "branch environment copies";
  auto key = [&](StringRef name, uint64_t copies) {
    fits &=
        TableQueryBudget::addProduct(charge, TableQueryResource::Slots, copies,
                                     8) &&
        TableQueryBudget::addProduct(charge, TableQueryResource::ConstantBytes,
                                     copies, name.size()) &&
        TableQueryBudget::add(charge, TableQueryResource::Work, copies);
  };
  auto binding = [&](StringRef name, StringRef contractName,
                     const AstNode &bindingSite,
                     uint64_t copies) -> LogicalResult {
    key(name, copies);
    fits &=
        TableQueryBudget::addProduct(charge, TableQueryResource::Slots, copies,
                                     8) &&
        TableQueryBudget::addProduct(charge, TableQueryResource::ConstantBytes,
                                     copies, contractName.size()) &&
        TableQueryBudget::add(charge, TableQueryResource::Work, copies);
    for (const AstStep &step : bindingSite.path) {
      if (failed(
              chargeLoopDescriptorWork(site, "environment path measurement")))
        return failure();
      fits &= TableQueryBudget::addProduct(charge, TableQueryResource::Slots,
                                           copies, 8) &&
              TableQueryBudget::addProduct(charge,
                                           TableQueryResource::ConstantBytes,
                                           copies, step.field.size()) &&
              TableQueryBudget::add(charge, TableQueryResource::Work, copies);
    }
    return success();
  };
  if (!yes) {
    for (const auto *map : {&incoming.values, &incoming.writes})
      for (const auto &entry : *map) {
        if (failed(chargeLoopDescriptorWork(site,
                                            "environment entry measurement")))
          return failure();
        key(entry.first(), 2);
      }
    for (const auto &entry : incoming.boundaries) {
      if (failed(chargeLoopDescriptorWork(
              site, "environment boundary measurement")) ||
          failed(binding(entry.first(), entry.second.contract.name,
                         entry.second.contract.node, 2)))
        return failure();
    }
    // Two ambient/continuation handle pairs; no new hardware payload planes.
    fits &= TableQueryBudget::add(charge, TableQueryResource::Slots, 4) &&
            TableQueryBudget::add(charge, TableQueryResource::Work, 4);
  } else {
    for (const auto &entry : incoming.boundaries) {
      if (failed(chargeLoopDescriptorWork(site, "join boundary measurement")) ||
          failed(binding(entry.first(), entry.second.contract.name,
                         entry.second.contract.node, 1)))
        return failure();
    }
    for (const auto *branch : {yes, no})
      for (const auto &entry : branch->boundaries) {
        if (failed(
                chargeLoopDescriptorWork(site, "join declaration measurement")))
          return failure();
        if (entry.second.declared &&
            failed(binding(entry.first(), entry.second.contract.name,
                           entry.second.contract.node, 1)))
          return failure();
      }
    for (const auto &entry : yes->values) {
      if (failed(chargeLoopDescriptorWork(site, "join output measurement")))
        return failure();
      if (!no->values.contains(entry.first()))
        continue;
      key(entry.first(), 1);
      auto yesBoundary = yes->boundaries.find(entry.first());
      auto noBoundary = no->boundaries.find(entry.first());
      bool declared =
          (yesBoundary != yes->boundaries.end() &&
           yesBoundary->second.declared) ||
          (noBoundary != no->boundaries.end() && noBoundary->second.declared);
      if (!incoming.boundaries.contains(entry.first()) && !declared &&
          failed(binding(entry.first(), entry.first(), site, 1)))
        return failure();
    }
    for (const auto &entry : incoming.writes) {
      if (failed(chargeLoopDescriptorWork(site, "join write measurement")))
        return failure();
      key(entry.first(), 1);
    }
    fits &= TableQueryBudget::add(charge, TableQueryResource::Slots, 2) &&
            TableQueryBudget::add(charge, TableQueryResource::Work, 2);
  }
  std::string reason;
  if (!fits || !queryBudget.reserve(charge, reason))
    return mlir::emitError(site.location(b.getContext(), source.path))
           << "bounded for iteration " << activeLoop->iteration << " " << phase
           << ": " << (fits ? reason : "copy storage counter exhausted");
  return success();
}

LogicalResult Importer::payLoopEnvelope(LoopStatementEnvelope &e,
                                        const AstNode &site, StringRef phase) {
  uint64_t B = 0, L = 0, P = 0, h = 0, slots = 0, storage = 0, facts = 0,
           constants = 0, work = 0, frames = 0;
  bool fits = pureCheckedProduct(4, e.maps, B) &&
              pureCheckedProduct(2, e.operations, L) && pureCheckedAdd(L, B);
  h = std::max(e.factWords, e.precision / 64 + (e.precision % 64 != 0));
  fits &= pureCheckedProduct(
      e.words,
      std::max(uint64_t(1), e.precision / 64 + (e.precision % 64 != 0)), P);
  fits &= pureCheckedProduct(e.operands + 2, e.operations, slots) &&
          pureCheckedAdd(slots, B);
  uint64_t part = 0;
  fits &= pureCheckedProduct(3, L, part) && pureCheckedAdd(slots, part);
  part = e.nodes;
  fits &= pureCheckedAdd(part, e.operands) &&
          pureCheckedAdd(part, e.declarations) &&
          pureCheckedProduct(2, part, part) && pureCheckedAdd(slots, part);
  fits &=
      pureCheckedProduct(activeSourceExpansion.size(), e.operations, frames) &&
      pureCheckedAdd(slots, frames);
  fits &= pureCheckedProduct(3, P, storage) &&
          pureCheckedProduct(storage, L, storage);
  fits &= pureCheckedProduct(2, h, facts) &&
          pureCheckedProduct(facts, L, facts) && pureCheckedAdd(storage, facts);
  fits &= pureCheckedProduct(8, P, constants) &&
          pureCheckedProduct(constants, L, constants);
  fits &= pureCheckedProduct(3, e.precision + 2, part) &&
          pureCheckedProduct(part, L, part) && pureCheckedAdd(constants, part);
  work = e.nodes;
  fits &= pureCheckedProduct(2, e.operations, part) &&
          pureCheckedAdd(work, part) && pureCheckedAdd(work, B) &&
          pureCheckedAdd(work, L);
  std::string reason;
  fits &=
      TableQueryBudget::add(e.reservation, TableQueryResource::Operations,
                            e.operations) &&
      TableQueryBudget::add(e.reservation, TableQueryResource::Slots, slots) &&
      TableQueryBudget::add(e.reservation, TableQueryResource::PayloadWords,
                            storage) &&
      TableQueryBudget::add(e.reservation, TableQueryResource::ConstantBytes,
                            constants) &&
      TableQueryBudget::add(e.reservation, TableQueryResource::Work, work);
  if (!fits || !queryBudget.reserve(e.reservation, reason))
    return mlir::emitError(site.location(b.getContext(), source.path))
           << "bounded for iteration " << activeLoop->iteration << " " << phase
           << ": " << (fits ? reason : "resource envelope counter exhausted");
  e.arguments = B;
  e.values = L;
  e.words = P;
  e.factWords = h;
  return success();
}

FailureOr<LoopStatementEnvelope>
Importer::reserveLoopStatement(const AstNode &statement,
                               const BranchEnvironment &environment,
                               FlatSymbolRefAttr symbol, StringRef phase) {
  LoopStatementEnvelope envelope;
  if (statement.kind() == "Pass") {
    if (failed(payLoopEnvelope(envelope, statement, phase)))
      return failure();
    return envelope;
  }
  if (statement.kind() == "If") {
    if (failed(describeLoopExpression(statement.child("test"), environment,
                                      symbol, envelope)))
      return failure();
    // Guard predicate normalization and source-check ambient/continuation.
    envelope.operations += 8;
  } else {
    auto target = statement.kind() == "AnnAssign"
                      ? statement.child("target")
                      : statement.item("targets", 0);
    std::optional<Type> expected;
    auto boundary = environment.boundaries.find(assignmentRoot(target));
    if (target.kind() == "Name" && boundary != environment.boundaries.end() &&
        boundary->second.contract.type)
      expected = boundary->second.contract.type;
    if (statement.kind() == "AnnAssign") {
      auto type = portType(statement.child("annotation"), symbol);
      if (failed(type))
        return failure();
      expected = *type;
    }
    if (target.kind() != "Name") {
      auto shape =
          describeLoopExpression(target, environment, symbol, envelope);
      if (failed(shape))
        return failure();
      expected = shape->type;
      auto root = environment.values.find(assignmentRoot(target));
      if (root == environment.values.end())
        return failure();
      auto aggregate = loopTypeShape(root->second.getType(), target);
      if (failed(aggregate))
        return failure();
      uint64_t count;
      if (!pureCheckedProduct(8, aggregate->declarations, count) ||
          !pureCheckedAdd(count, target.kind() == "Subscript" ||
                                         indexedFieldTarget(target)
                                     ? 12
                                     : 4) ||
          !pureCheckedAdd(envelope.operations, count))
        return failure();
      envelope.precision = std::max(envelope.precision, aggregate->precision);
      envelope.words = std::max(envelope.words, aggregate->words);
      envelope.operands = std::max(envelope.operands, aggregate->fields);
      envelope.declarations += aggregate->declarations;
      envelope.maps += aggregate->declarations;
    }
    // Binding may widen or otherwise reconstruct the completed RHS. A Name
    // description keeps its producer's width, so include the actual destination
    // layout independently before paying the existing conversion envelope.
    if (expected) {
      auto destination = loopTypeShape(*expected, target);
      if (failed(destination))
        return failure();
      envelope.precision = std::max(envelope.precision, destination->precision);
      envelope.words = std::max(envelope.words, destination->words);
      envelope.operands = std::max(envelope.operands, destination->fields);
      if (!pureCheckedAdd(envelope.declarations, destination->declarations))
        return failure();
    }
    if (failed(describeLoopExpression(statement.child("value"), environment,
                                      symbol, envelope, expected)))
      return failure();
    envelope.operations += 3;
  }
  if (failed(payLoopEnvelope(envelope, statement, phase)))
    return failure();
  return envelope;
}

FailureOr<LoopStatementEnvelope> Importer::reserveLoopJoin(
    const AstNode &statement, const BranchEnvironment &incoming,
    const BranchEnvironment &yes, const BranchEnvironment &no) {
  LoopStatementEnvelope envelope;
  if (failed(reserveLoopEnvironmentCopies(statement, incoming, &yes, &no)))
    return failure();
  for (const auto &item : yes.values) {
    auto other = no.values.find(item.first());
    if (other == no.values.end())
      continue;
    if (hasUnchangedIncomingBinding(item.first(), incoming, yes, no))
      continue;
    for (Value value : {item.second, other->second}) {
      auto shape = loopTypeShape(value.getType(), statement);
      if (failed(shape))
        return failure();
      uint64_t selectionPrecision = shape->precision;
      bool negativePeer = false;
      for (Value arm : {item.second, other->second}) {
        auto fact = numericValues.find(arm);
        if (fact == numericValues.end() || !fact->second.interval)
          continue;
        const auto &interval = *fact->second.interval;
        negativePeer |= interval.lower.isNegative();
        for (const auto *endpoint : {&interval.lower, &interval.upper})
          envelope.factWords =
              std::max(envelope.factWords,
                       uint64_t(endpoint->getBitWidth() / 64 +
                                (endpoint->getBitWidth() % 64 != 0)));
      }
      if (negativePeer && isa<ac::BitsType>(value.getType()) &&
          !pureCheckedAdd(selectionPrecision, 1))
        return failure();
      envelope.precision = std::max(envelope.precision, selectionPrecision);
      envelope.words = std::max(envelope.words, shape->words);
      envelope.operands = std::max(envelope.operands, shape->fields);
      envelope.declarations += shape->declarations;
      envelope.maps += shape->declarations;
      envelope.operations += 4 + 8 * shape->declarations + 9;
    }
  }
  if (incoming.continuation)
    envelope.operations += 8;
  if (failed(payLoopEnvelope(envelope, statement, "join")))
    return failure();
  return envelope;
}

LogicalResult
Importer::auditLoopStatement(Block &block, Operation *previous,
                             const LoopStatementEnvelope &envelope,
                             const AstNode &site, StringRef phase) {
  auto diagnostic = [&] {
    return mlir::emitError(site.location(b.getContext(), source.path));
  };
  TableQueryCharge measured;
  uint64_t operations = 0, arguments = 0, values = 0;
  StringRef violatedBound;
  uint64_t observedBound = 0, reservedBound = 0;
  auto within = [&](uint64_t observed, uint64_t reserved, StringRef name) {
    if (observed <= reserved)
      return true;
    violatedBound = name;
    observedBound = observed;
    reservedBound = reserved;
    return false;
  };
  if (failed(prepayLoopAnalysis(site)))
    return failure();
  ac::HardwareAnalysis analysis(*body);
  auto inspectValue = [&](Value value, uint64_t rows) -> LogicalResult {
    if (!within(++values, envelope.values, "value count"))
      return failure();
    auto weight = loopStorageWords(value.getType(), analysis, site);
    if (failed(weight) ||
        !within(*weight, envelope.words, "value layout words") ||
        !TableQueryBudget::addProduct(
            measured, TableQueryResource::PayloadWords, 3 * *weight, rows))
      return failure();
    auto schema = loopTypeShape(value.getType(), site, &analysis);
    if (failed(schema) ||
        !within(schema->precision, envelope.precision, "value precision"))
      return failure();
    auto fact = numericValues.find(value);
    if (fact != numericValues.end()) {
      if (!TableQueryBudget::add(measured, TableQueryResource::Slots, 3))
        return failure();
      const auto &info = fact->second;
      if (info.interval)
        for (const llvm::APSInt *endpoint :
             {&info.interval->lower, &info.interval->upper}) {
          uint64_t words = endpoint->getBitWidth() / 64 +
                           (endpoint->getBitWidth() % 64 != 0);
          if (words > envelope.factWords ||
              (endpoint->isSigned()
                   ? endpoint->getSignificantBits()
                   : endpoint->getActiveBits()) > envelope.precision + 2 ||
              !TableQueryBudget::add(measured, TableQueryResource::PayloadWords,
                                     words) ||
              !TableQueryBudget::add(measured,
                                     TableQueryResource::ConstantBytes,
                                     envelope.precision + 2))
            return failure();
        }
      if (auto integer =
              dyn_cast_or_null<ac::MathIntAttr>(info.closedSourceConstant)) {
        uint64_t spelling = integer.getCanonicalValue().size();
        if (spelling > envelope.precision + 2 ||
            !TableQueryBudget::add(measured, TableQueryResource::ConstantBytes,
                                   spelling))
          return failure();
      }
    }
    return success();
  };
  auto inspect = [&](auto &&self, Operation &operation,
                     uint64_t rows) -> LogicalResult {
    if (activeLoop->helperOwned.contains(&operation))
      return success();
    if (!within(++operations, envelope.operations, "operation count") ||
        !within(operation.getNumOperands(), envelope.operands,
                "operand count") ||
        !within(operation.getNumResults(), 2, "result count") ||
        !TableQueryBudget::add(measured, TableQueryResource::Operations, 1) ||
        !TableQueryBudget::add(measured, TableQueryResource::Slots,
                               operation.getNumOperands() +
                                   operation.getNumResults()))
      return failure();
    // Existing values are paid by captures or their defining statement; inspect
    // operand precision/layout without charging their payload a second time.
    for (Value operand : operation.getOperands()) {
      if (failed(chargeLoopDescriptorWork(site, "audit operand edge")))
        return failure();
      auto shape = loopTypeShape(operand.getType(), site, &analysis);
      if (failed(shape) ||
          !within(shape->precision, envelope.precision, "operand precision") ||
          !within(shape->words, envelope.words, "operand layout words"))
        return failure();
    }
    for (Value result : operation.getResults())
      if (failed(inspectValue(result, rows)))
        return failure();
    if (isa<ac::BitsConstantOp>(operation))
      for (Value result : operation.getResults()) {
        auto weight = loopStorageWords(result.getType(), analysis, site);
        if (failed(weight) ||
            !TableQueryBudget::addProduct(
                measured, TableQueryResource::ConstantBytes, 8 * *weight, rows))
          return failure();
      }
    uint64_t replicated = rows;
    if (auto map = dyn_cast<ac::TableMapOp>(operation)) {
      auto size = analysis.getTableSize(
          cast<ac::TableType>(map.getResult(0).getType()), {}, &operation);
      if (failed(size) || !pureCheckedProduct(rows, *size, replicated))
        return failure();
    }
    for (Region &region : operation.getRegions())
      for (Block &nested : region) {
        if (!within(nested.getNumArguments(), 4, "map argument count"))
          return failure();
        for (Value argument : nested.getArguments()) {
          if (!within(++arguments, envelope.arguments,
                      "region argument count") ||
              failed(inspectValue(argument, replicated)))
            return failure();
        }
        for (Operation &child : nested)
          if (failed(self(self, child, replicated)))
            return failure();
      }
    return success();
  };
  Operation *operation = previous ? previous->getNextNode()
                                  : (block.empty() ? nullptr : &block.front());
  for (; operation; operation = operation->getNextNode()) {
    if (succeeded(inspect(inspect, *operation, 1)))
      continue;
    auto error = diagnostic();
    error << "bounded for iteration " << activeLoop->iteration << " " << phase
          << " exceeded its prepaid typed envelope";
    if (!violatedBound.empty())
      error << " (" << violatedBound << ": " << observedBound << " > "
            << reservedBound << ")";
    return failure();
  }
  for (TableQueryResource resource :
       {TableQueryResource::Operations, TableQueryResource::Slots,
        TableQueryResource::PayloadWords, TableQueryResource::ConstantBytes})
    if (measured.amounts[static_cast<unsigned>(resource)] >
        envelope.reservation.amounts[static_cast<unsigned>(resource)])
      return diagnostic() << "bounded for iteration " << activeLoop->iteration
                          << " " << phase
                          << " exceeded its prepaid resource counter";
  return success();
}

LogicalResult Importer::loopStatements(const AstNode &parent, StringRef field,
                                       OpBuilder &at, FlatSymbolRefAttr symbol,
                                       BranchEnvironment &environment,
                                       llvm::StringSet<> &owners) {
  auto previous = [&]() -> Operation * {
    auto *block = at.getInsertionBlock();
    return block->empty() ? nullptr : &block->back();
  };
  llvm::StringMap<Instance *> noInstances;
  auto &instances = activeRule && activeRule->instances ? *activeRule->instances
                                                        : noInstances;
  for (size_t i = 0; i < parent.array(field).size(); ++i) {
    auto statement = parent.item(field, i);
    if (statement.kind() == "If") {
      auto envelope =
          reserveLoopStatement(statement, environment, symbol, "guard");
      if (failed(envelope))
        return failure();
      Operation *start = previous();
      auto guard = expression(statement.child("test"), at, symbol,
                              environment.values, instances);
      if (failed(guard))
        return failure();
      auto predicate = predicateView(*guard, statement.child("test"), symbol);
      if (failed(predicate))
        return failure();
      if (failed(reserveLoopEnvironmentCopies(statement, environment)))
        return failure();
      auto yes = environment, no = environment;
      if (environment.continuation) {
        auto opposite = checkNot(*guard, statement, at, symbol);
        if (failed(opposite) ||
            failed(enterCheckBranch(yes, *guard, statement, at, symbol)) ||
            failed(enterCheckBranch(no, *opposite, statement, at, symbol)))
          return failure();
      }
      if (failed(auditLoopStatement(*at.getInsertionBlock(), start, *envelope,
                                    statement, "guard")))
        return failure();
      if (failed(loopStatements(statement, "body", at, symbol, yes, owners)) ||
          failed(loopStatements(statement, "orelse", at, symbol, no, owners)))
        return failure();
      auto joinEnvelope = reserveLoopJoin(statement, environment, yes, no);
      if (failed(joinEnvelope))
        return failure();
      start = previous();
      auto joined =
          joinBranches(*predicate, environment, yes, no, statement, at, symbol);
      if (failed(joined) ||
          failed(auditLoopStatement(*at.getInsertionBlock(), start,
                                    *joinEnvelope, statement, "join")))
        return failure();
      environment = std::move(*joined);
      continue;
    }
    auto envelope =
        reserveLoopStatement(statement, environment, symbol, "assignment");
    if (failed(envelope))
      return failure();
    Operation *start = previous();
    // Reuse the original statement owner and AST occurrence in a single-item
    // slice; no synthetic AST, alternative binder or expression lowerer.
    AstNode returned;
    auto lowered = statements(parent, field, at, symbol, environment, owners,
                              returned, i, i + 1);
    if (failed(lowered) || returned ||
        failed(auditLoopStatement(*at.getInsertionBlock(), start, *envelope,
                                  statement, "assignment")))
      return failure();
  }
  return success();
}

LogicalResult Importer::pushHelperFrame(const AstNode &call,
                                        const AstNode &declaration,
                                        FlatSymbolRefAttr symbol) {
  // Empty/default non-loop callers retain their existing occurrences. A helper
  // reached from loop expansion records its lexical call after its actuals.
  if (!activeLoop)
    return success();
  TableQueryCharge charge;
  uint64_t components = call.path.size(), bytes = 0;
  for (const auto &step : call.path)
    if (!pureCheckedAdd(bytes, step.field.size() + (step.index ? 8 : 0)))
      return failure();
  uint64_t frames;
  bool fits =
      pureCheckedProduct(2, components, frames) && pureCheckedAdd(frames, 8) &&
      TableQueryBudget::add(charge, TableQueryResource::Slots, frames) &&
      TableQueryBudget::add(charge, TableQueryResource::ConstantBytes,
                            bytes + qualify(declaration.string("name")).size());
  std::string reason;
  if (!fits || !queryBudget.reserve(charge, reason))
    return mlir::emitError(call.location(b.getContext(), source.path))
           << "bounded for iteration " << activeLoop->iteration
           << " helper frame: "
           << (fits ? reason : "resource counter exhausted");
  auto site = sourceOccurrence(b, symbol, call).getAs<DictionaryAttr>("site");
  activeSourceExpansion.push_back(b.getDictionaryAttr(
      {b.getNamedAttr("kind", b.getStringAttr("call")),
       b.getNamedAttr("site", site),
       b.getNamedAttr("callee", FlatSymbolRefAttr::get(
                                    b.getContext(),
                                    qualify(declaration.string("name"))))}));
  return success();
}

LogicalResult Importer::lowerLiteralFor(const AstNode &statement, OpBuilder &at,
                                        FlatSymbolRefAttr symbol,
                                        BranchEnvironment &environment,
                                        llvm::StringSet<> &owners) {
  auto diagnostic = [&] {
    return mlir::emitError(statement.location(b.getContext(), source.path));
  };
  if (activeLoop)
    return diagnostic() << "nested bounded for is unsupported";
  if (!activeRule || activeRule->structural || !owners.empty())
    return diagnostic() << "bounded for requires a pure behavioral rule "
                           "invocation without persistent-owner writes";
  PureScalarSummary inspected;
  if (failed(chargePureSource(statement, inspected)))
    return failure();
  auto syntax = readLiteralForSyntax(statement, diagnostic);
  if (failed(syntax))
    return failure();
  auto function = statement.child("iter").child("func");
  if (lexicallyShadowed(function, "range") || names.contains("range") ||
      structs.contains("range") || behavioralRules.contains("range"))
    return diagnostic() << "bounded for range builtin is shadowed";
  for (const AstNode &node :
       {statement.child("iter"), function, syntax->target, syntax->bound})
    if (failed(chargePureSource(node, inspected)))
      return failure();
  StringRef spelling = syntax->spelling;
  while (spelling.size() > 1 && spelling.front() == '0')
    spelling = spelling.drop_front();
  std::string maximum = std::to_string(
      TableQueryBudget::limit(TableQueryResource::Occurrences) - 1);
  if (spelling.size() > maximum.size() ||
      (spelling.size() == maximum.size() && spelling.compare(maximum) > 0))
    return diagnostic()
           << "bounded for occurrences budget exhausted by stop literal";
  uint64_t trips = 0;
  if (spelling.getAsInteger(10, trips))
    return failure();
  std::string reason;
  if (!queryBudget.enter(reason))
    return diagnostic() << "bounded for preflight: " << reason;
  auto leave = llvm::scope_exit([&] { queryBudget.leave(); });
  LoopAdmissionSummary admission;
  if (failed(preflightForCapabilities(statement, admission))) {
    diagnostic() << "bounded for capability preflight failed";
    return failure();
  }
  if (!trips)
    return success();

  TableQueryCharge charge;
  bool fits =
      TableQueryBudget::add(charge, TableQueryResource::Occurrences,
                            1 + trips) &&
      TableQueryBudget::addProduct(charge, TableQueryResource::Work, trips,
                                   admission.nodes) &&
      TableQueryBudget::addProduct(charge, TableQueryResource::Slots, trips,
                                   10 + 2 * statement.path.size());
  uint64_t frameBytes = 0;
  for (const auto &step : statement.path)
    fits &=
        pureCheckedAdd(frameBytes, step.field.size() + (step.index ? 8 : 0));
  fits &= TableQueryBudget::addProduct(
      charge, TableQueryResource::ConstantBytes, trips, frameBytes + 24);
  auto slots = [&](uint64_t count) {
    fits &= TableQueryBudget::addProduct(charge, TableQueryResource::Slots,
                                         count, 8);
  };
  auto text = [&](StringRef value) {
    fits &= TableQueryBudget::add(charge, TableQueryResource::ConstantBytes,
                                  value.size());
  };
  auto path = [&](const AstNode &node) {
    slots(node.path.size());
    for (const auto &step : node.path)
      text(step.field);
  };
  auto fact = [&](const NumericValue &info) {
    slots(3);
    if (info.interval)
      for (const llvm::APSInt *endpoint :
           {&info.interval->lower, &info.interval->upper})
        fits &= TableQueryBudget::add(charge, TableQueryResource::PayloadWords,
                                      endpoint->getBitWidth() / 64 +
                                          (endpoint->getBitWidth() % 64 != 0));
    if (auto integer =
            dyn_cast_or_null<ac::MathIntAttr>(info.closedSourceConstant))
      text(integer.getCanonicalValue());
  };
  auto value = [&](Value item, bool capture) {
    auto shape = loopTypeShape(item.getType(), statement);
    if (failed(shape)) {
      fits = false;
      return;
    }
    slots(1);
    fits &=
        TableQueryBudget::addProduct(charge, TableQueryResource::PayloadWords,
                                     shape->words, capture ? 3 : 1);
    auto info = numericValues.find(item);
    if (info != numericValues.end())
      fact(info->second);
  };
  for (const auto &item : environment.values) {
    text(item.first());
    value(item.second, true);
  }
  for (const auto &item : environment.writes) {
    text(item.first());
    value(item.second, true);
  }
  if (environment.ambient)
    value(environment.ambient, true);
  if (environment.continuation)
    value(environment.continuation, true);
  for (const auto &item : environment.boundaries) {
    slots(2);
    text(item.first());
    text(item.second.contract.name);
    path(item.second.contract.node);
  }
  // Snapshot the complete fact maps, including entries not currently named by
  // the local environment. Dense-map slack is prepaid before the copies.
  slots(numericValues.size());
  slots(fixedValues.size());
  slots(arithmeticValues.size());
  for (const auto &item : numericValues)
    fact(item.second);
  slots(activeSourceExpansion.size());
  slots(activeRule->domains.size() + activeRule->checks.size() +
        activeRule->deferredObservations.size() +
        activeRule->bindingValues.size() +
        activeRule->structuralTargets.size() + 16);
  path(activeRule->binding);
  for (const auto &domain : activeRule->domains) {
    text(domain.formal);
    path(domain.assignment);
    path(domain.occurrence);
  }
  for (const auto &check : activeRule->checks) {
    path(check.statement);
    path(check.observationRule);
  }
  for (const auto &entry : activeRule->deferredObservations) {
    path(entry.first);
    path(entry.second);
  }
  if (activeRule->instances)
    for (const auto &entry : *activeRule->instances) {
      text(entry.first());
      slots(1);
      const auto &instance = *entry.second;
      text(instance.name);
      path(instance.node);
      slots(instance.parameters.size() + instance.typeArguments.size() +
            instance.inputs.size() + instance.outputs.size() + 8);
      for (const Port &port : instance.inputs) {
        text(port.name);
        path(port.node);
      }
      for (const Port &port : instance.outputs) {
        text(port.name);
        path(port.node);
      }
    }
  if (activeRule->outputs)
    for (const auto &entry : *activeRule->outputs) {
      text(entry.first());
      slots(entry.second.size());
    }
  // Branch environments and helper ownership are loop bookkeeping, not new
  // hardware. Reserve their maximum finite source footprint before staging.
  uint64_t environmentEntries = environment.values.size() +
                                environment.boundaries.size() +
                                environment.writes.size() + 2;
  fits &= pureCheckedAdd(environmentEntries, admission.nodes) &&
          TableQueryBudget::addProduct(charge, TableQueryResource::Slots,
                                       environmentEntries, 8);
  for (const auto &entry : admission.helpers) {
    uint64_t ownership = entry.second.summary.nodes +
                         entry.second.summary.formals +
                         entry.second.summary.calls;
    fits &= TableQueryBudget::addProduct(charge, TableQueryResource::Slots,
                                         trips, ownership);
  }
  if (!fits || !queryBudget.reserve(charge, reason))
    return diagnostic() << "bounded for snapshot/header: "
                        << (fits ? reason : "resource counter exhausted");

  const auto savedNumeric = numericValues;
  const auto savedFixed = fixedValues;
  const auto savedArithmetic = arithmeticValues;
  RuleEvaluation savedEvaluation = *activeRule;
  llvm::StringMap<Instance *> savedInstances;
  llvm::StringMap<SmallVector<Value>> savedOutputs;
  SmallVector<std::pair<Instance *, Instance>, 0> instanceStates;
  if (activeRule->instances) {
    savedInstances = *activeRule->instances;
    for (const auto &entry : savedInstances)
      instanceStates.emplace_back(entry.second, *entry.second);
  }
  if (activeRule->outputs)
    savedOutputs = *activeRule->outputs;
  size_t expansionDepth = activeSourceExpansion.size();
  OpBuilder::InsertionGuard insertion(at);
  auto temporary = createOp(at, statement.location(b.getContext(), source.path),
                            ac::RuleOp::getOperationName(), {}, {}, {}, 1);
  auto block = new Block();
  temporary->getRegion(0).push_back(block);
  BranchEnvironment staged = environment;
  IRMapping captures;
  auto capture = [&](Value original) {
    if (!original)
      return Value();
    for (const auto &entry : captures.getValueMap())
      if (entry.second == original)
        return entry.first;
    Value argument = captureValue(block, original, temporary->getLoc());
    captures.map(argument, original);
    return argument;
  };
  for (auto &entry : staged.values)
    entry.second = capture(entry.second);
  for (auto &entry : staged.writes)
    entry.second = capture(entry.second);
  staged.ambient = capture(staged.ambient);
  staged.continuation = capture(staged.continuation);
  LoopLoweringContext context{statement, &admission, 0, {}};
  auto previousLoop = std::exchange(activeLoop, &context);
  bool committed = false;
  auto cleanup = llvm::scope_exit([&] {
    activeSourceExpansion.resize(expansionDepth);
    activeLoop = previousLoop;
    if (!committed) {
      numericValues = savedNumeric;
      fixedValues = savedFixed;
      arithmeticValues = savedArithmetic;
      *activeRule = savedEvaluation;
      if (savedEvaluation.instances)
        *savedEvaluation.instances = savedInstances;
      if (savedEvaluation.outputs)
        *savedEvaluation.outputs = savedOutputs;
      for (auto &entry : instanceStates)
        *entry.first = std::move(entry.second);
    } else {
      for (Value argument : block->getArguments()) {
        numericValues.erase(argument);
        fixedValues.erase(argument);
        arithmeticValues.erase(argument);
      }
    }
    temporary->erase();
  });
  auto inside = OpBuilder::atBlockEnd(block);
  for (uint64_t ordinal = 0; ordinal < trips; ++ordinal) {
    context.iteration = ordinal;
    activeSourceExpansion.resize(expansionDepth);
    auto integer = ac::MathIntAttr::get(b.getContext(),
                                        llvm::APSInt::getUnsigned(ordinal));
    auto site =
        sourceOccurrence(b, symbol, statement).getAs<DictionaryAttr>("site");
    activeSourceExpansion.push_back(b.getDictionaryAttr(
        {b.getNamedAttr("kind", b.getStringAttr("iteration")),
         b.getNamedAttr("site", site),
         b.getNamedAttr("ordinal", b.getI64IntegerAttr(ordinal)),
         b.getNamedAttr("value",
                        b.getDictionaryAttr(
                            {b.getNamedAttr("kind", b.getStringAttr("integer")),
                             b.getNamedAttr("value", integer)}))}));
    LoopStatementEnvelope induction;
    induction.operations = 4;
    induction.nodes = 1;
    induction.precision =
        std::max(uint64_t(1), uint64_t(llvm::Log2_64_Ceil(ordinal + 1)));
    auto boundary = staged.boundaries.find(syntax->target.string("id"));
    if (boundary != staged.boundaries.end() && boundary->second.contract.type) {
      auto shape =
          loopTypeShape(boundary->second.contract.type, syntax->target);
      if (failed(shape))
        return failure();
      induction.precision = std::max(induction.precision, shape->precision);
      induction.words = std::max(induction.words, shape->words);
    }
    if (failed(payLoopEnvelope(induction, syntax->target, "binder")))
      return failure();
    Operation *start = block->empty() ? nullptr : &block->back();
    auto hidden = materializeMatchKey(
        {syntax->target, integer, {}},
        bits(std::max(uint64_t(1), uint64_t(llvm::Log2_64_Ceil(ordinal + 1))),
             syntax->target, symbol),
        inside, symbol, false);
    if (failed(hidden)) {
      diagnostic() << "bounded for reachable iteration " << ordinal
                   << " binder failed";
      return failure();
    }
    // The logical ordinal has a singleton mathematical range, independently of
    // its unsigned carrier width. Body writes do not alter the next ordinal.
    remember(*hidden, ac::detail::ValueKind::Integer,
             IntegerInterval{llvm::APSInt::getUnsigned(ordinal),
                             llvm::APSInt::getUnsigned(ordinal + 1)},
             integer);
    auto binding = boundary == staged.boundaries.end()
                       ? inferredBoundary(syntax->target.string("id"), *hidden,
                                          syntax->target)
                       : boundary->second;
    auto converted =
        applyBindingBoundary(*hidden, binding, syntax->target, inside, symbol);
    if (failed(converted) ||
        failed(updateAssignment(syntax->target, *converted, statement, inside,
                                symbol, staged.values))) {
      diagnostic() << "bounded for reachable iteration " << ordinal
                   << " binder failed";
      return failure();
    }
    staged.boundaries[syntax->target.string("id")] = binding;
    if (failed(auditLoopStatement(*block, start, induction, syntax->target,
                                  "binder")) ||
        failed(loopStatements(statement, "body", inside, symbol, staged,
                              owners))) {
      diagnostic() << "bounded for reachable iteration " << ordinal
                   << " failed";
      return failure();
    }
  }
  auto sameEvaluation = [&] {
    bool same =
        activeRule->domains.size() == savedEvaluation.domains.size() &&
        activeRule->checks.size() == savedEvaluation.checks.size() &&
        activeRule->deferredObservations.size() ==
            savedEvaluation.deferredObservations.size() &&
        activeRule->observationEntries == savedEvaluation.observationEntries &&
        activeRule->instances == savedEvaluation.instances &&
        activeRule->outputs == savedEvaluation.outputs &&
        activeRule->target == savedEvaluation.target &&
        sameSourceSite(activeRule->binding, savedEvaluation.binding) &&
        activeRule->bindingValues == savedEvaluation.bindingValues &&
        activeRule->structuralTargets == savedEvaluation.structuralTargets &&
        activeRule->nextStructuralTarget ==
            savedEvaluation.nextStructuralTarget &&
        activeRule->structural == savedEvaluation.structural;
    if (!same)
      return false;
    for (auto [current, saved] :
         llvm::zip(activeRule->domains, savedEvaluation.domains))
      if (current.ownerIndex != saved.ownerIndex ||
          current.pathIndex != saved.pathIndex ||
          current.formal != saved.formal ||
          !sameSourceSite(current.assignment, saved.assignment) ||
          !sameSourceSite(current.occurrence, saved.occurrence) ||
          current.maskType != saved.maskType || current.index != saved.index ||
          current.mask != saved.mask)
        return false;
    for (auto [current, saved] :
         llvm::zip(activeRule->checks, savedEvaluation.checks))
      if (!sameSourceSite(current.statement, saved.statement) ||
          current.condition != saved.condition || current.path != saved.path ||
          current.message != saved.message ||
          current.observationKind != saved.observationKind ||
          current.observationSpec != saved.observationSpec ||
          !sameSourceSite(current.observationRule, saved.observationRule) ||
          current.observationValues != saved.observationValues ||
          current.observationCarriesValue != saved.observationCarriesValue)
        return false;
    for (auto [current, saved] :
         llvm::zip(activeRule->deferredObservations,
                   savedEvaluation.deferredObservations))
      if (!sameSourceSite(current.first, saved.first) ||
          !sameSourceSite(current.second, saved.second))
        return false;
    if (savedEvaluation.instances) {
      if (savedEvaluation.instances->size() != savedInstances.size())
        return false;
      for (const auto &entry : savedInstances) {
        auto found = savedEvaluation.instances->find(entry.first());
        if (found == savedEvaluation.instances->end() ||
            found->second != entry.second)
          return false;
      }
    }
    if (savedEvaluation.outputs) {
      if (savedEvaluation.outputs->size() != savedOutputs.size())
        return false;
      for (const auto &entry : savedOutputs) {
        auto found = savedEvaluation.outputs->find(entry.first());
        if (found == savedEvaluation.outputs->end() ||
            found->second != entry.second)
          return false;
      }
    }
    auto samePorts = [&](ArrayRef<Port> current, ArrayRef<Port> saved) {
      if (current.size() != saved.size())
        return false;
      for (auto [a, c] : llvm::zip(current, saved))
        if (a.name != c.name || a.type != c.type ||
            !sameSourceSite(a.node, c.node) || a.sourceKind != c.sourceKind ||
            a.fixedBits != c.fixedBits)
          return false;
      return true;
    };
    for (const auto &entry : instanceStates) {
      const Instance &current = *entry.first;
      const Instance &saved = entry.second;
      if (current.name != saved.name ||
          !sameSourceSite(current.node, saved.node) ||
          current.callee != saved.callee ||
          current.declaration != saved.declaration ||
          current.parameters != saved.parameters ||
          current.typeArguments != saved.typeArguments ||
          !samePorts(current.inputs, saved.inputs) ||
          !samePorts(current.outputs, saved.outputs) ||
          current.op != saved.op || current.bound != saved.bound ||
          current.clockInput != saved.clockInput ||
          current.resetInput != saved.resetInput)
        return false;
    }
    return true;
  };
  if (!sameEvaluation())
    return diagnostic() << "bounded for pure body changed rule side metadata";
  for (auto &entry : captures.getValueMap()) {
    Value argument = entry.first;
    argument.replaceAllUsesWith(entry.second);
  }
  auto remap = [&](Value value) { return captures.lookupOrDefault(value); };
  for (auto &entry : staged.values)
    entry.second = remap(entry.second);
  for (auto &entry : staged.writes)
    entry.second = remap(entry.second);
  staged.ambient = remap(staged.ambient);
  staged.continuation = remap(staged.continuation);
  while (!block->empty())
    block->front().moveBefore(temporary);
  environment = std::move(staged);
  committed = true;
  return success();
}

LogicalResult Importer::statements(const AstNode &parent, StringRef field,
                                   OpBuilder &at, FlatSymbolRefAttr symbol,
                                   BranchEnvironment &environment,
                                   llvm::StringSet<> &owners, AstNode &returned,
                                   size_t begin, size_t end) {
  auto &values = environment.values;
  auto &writes = environment.writes;
  llvm::StringMap<Instance *> noInstances;
  auto &instances = activeRule && activeRule->instances ? *activeRule->instances
                                                        : noInstances;
  for (size_t i = begin; i < std::min(end, parent.array(field).size()); ++i) {
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
        if (failed(
                captureRuleObservation(parent, stmt, at, symbol, environment)))
          return failure();
        continue;
      }
    }
    if (stmt.kind() == "For") {
      if (failed(lowerLiteralFor(stmt, at, symbol, environment, owners)))
        return failure();
      continue;
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
      auto writeEnable =
          environment.continuation ? environment.continuation : *enabled;
      auto normalized = unsignedBoundary(writeEnable, bits(1, stmt, symbol),
                                         stmt, at, symbol);
      if (failed(normalized))
        return failure();
      writes[name] = *normalized;
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
       b.getNamedAttr("ac.origin",
                      sourceOccurrence(b, decl.symbol, decl.node))},
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
          nullptr, [](Operation *op) {
            if (op)
              op->destroy();
          });
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
      for (auto [name, formalType] :
           llvm::zip(inputNames, signature.getInputs())) {
        auto type = substitute(formalType, callee, instance.parameters,
                               instance.typeArguments);
        if (failed(type))
          return failure();
        instance.inputs.push_back(
            {cast<StringAttr>(name).getValue().str(), *type, stmt});
      }
      for (auto [name, formalType] :
           llvm::zip(outputNames, signature.getResults())) {
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
           at.getNamedAttr("type_arguments",
                           at.getArrayAttr(instance.typeArguments)),
           at.getNamedAttr("occurrence",
                           sourceOccurrence(at, decl.symbol, stmt))});
      structuralCallSites.insert(sourceOccurrence(at, decl.symbol, call));
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
            sourceOccurrence(at, decl.symbol, stmt.child("value"))))
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
    if (failed(memory))
      return failure();
    if (*memory)
      continue;
    auto queued =
        bindQueueResults(stmt, at, decl.symbol, values, structuralInstances);
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
            auto type =
                expressionType(target, values, noInstances, decl.symbol);
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
        at.getNamedAttr("occurrence", sourceOccurrence(at, decl.symbol, stmt))};
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
    if (evaluation.structuralTargets.empty() && !checkCount &&
        !observationCount)
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
                         sourceOccurrence(at, decl.symbol, registration))},
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
      auto one =
          constant(bits(1, rule, decl.symbol), 1, rule, inRule, decl.symbol);
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
        evaluation.nextStructuralTarget != evaluation.structuralTargets.size())
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
      target->op->setOperands(
          ruleOp->getResults().slice(result, target->inputs.size()));
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
  } else if (decl.returnShape == ReturnShape::Mapping &&
             node.kind() == "Call" && node.child("func").kind() == "Name" &&
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
