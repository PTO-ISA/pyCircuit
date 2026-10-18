// Independent FINAL admission: these helpers consume actual typed SSA. Source
// replay remains responsible for the original statement/assignment inventory.
  struct FinalWriteDomain {
    ArrayAttr selectors;
    HardwareIdentityEndpoint mask, path;
    std::optional<HardwareIdentityEndpoint> index;
  };
  struct DomainUse { ArrayRef<Attribute> path; HardwareIdentityEndpoint mask; };

  FailureOr<HardwareIdentityEndpoint> domainOperand(
      const HardwareIdentityEndpoint &at, Value value) {
    return normal(value, at.scope);
  }
  LogicalResult domainSame(const HardwareIdentityEndpoint &actual,
                           const HardwareIdentityEndpoint &expected,
                           Operation *site, StringRef what) {
    if (failed(payScope(actual.scope, budget, site)) ||
        failed(payScope(expected.scope, budget, site))) return failure();
    if (!sameEndpoint(actual, expected))
      return site->emitOpError() << "write-domain " << what << " origin differs";
    return success();
  }
  FailureOr<HardwareIdentityEndpoint> domainSplat(
      Value value, const HardwareIdentityScope &scope, size_t state,
      PrivateRoute::Kind kind) {
    auto actual = normal(value, scope);
    if (failed(actual)) return failure();
    auto splat = actual->value.getDefiningOp<TableSplatOp>();
    if (!splat || failed(allowUse(actual->scope, splat, 0, state, kind)))
      return valueSite(value)->emitOpError() << "write-domain Table input requires exact splat";
    return normal(splat.getInput(), actual->scope);
  }
  FailureOr<FinalWriteDomain> finalWriteDomain(
      const SampledWriteDomainView &domain, RuleOp rule,
      const HardwareIdentityScope &scope, size_t state) {
    auto mask = normal(domain.mask, scope);
    auto exported = normal(rule->getResult(domain.maskResult), scope);
    if (failed(mask) || failed(exported)) return failure();
    FinalWriteDomain result{domain.selectors, *exported, *mask, {}};
    if (!domain.table) return result;
    auto output = dyn_cast<OpResult>(mask->value);
    auto map = output ? dyn_cast<TableMapOp>(output.getOwner()) : TableMapOp();
    if (!map || output.getResultNumber() != 1 || map->getNumResults() != 2 ||
        map.getTables().size() != 4 || !map.getCaptures().empty() ||
        !map.getBody().hasOneBlock())
      return rule->emitOpError() << "indexed write-domain mask requires its native selected-write TableMap";
    auto bindings = scopeBindings(mask->scope, analysis, budget, map);
    if (failed(bindings) || failed(analysis.verifyResolvedOperation(map, *bindings))) return failure();
    auto &body = map.getBody().front();
    auto yielded = dyn_cast<YieldOp>(body.getTerminator());
    if (body.getNumArguments() != 5 || !yielded || yielded->getNumOperands() != 2)
      return map->emitOpError() << "selected-write TableMap bindings differ";
    auto index = domainSplat(map->getOperand(1), mask->scope, state, PrivateRoute::Mask);
    auto path = domainSplat(map->getOperand(3), mask->scope, state, PrivateRoute::Mask);
    auto rhs = domainSplat(map->getOperand(2), mask->scope, state, PrivateRoute::Mask);
    if (failed(index) || failed(path) || failed(rhs)) return failure();
    auto rawMask = yielded->getOperand(1).getDefiningOp<BitsBinaryOp>();
    if (!rawMask || rawMask.getOpcode() != "and" || rawMask.getLhs() != body.getArgument(4))
      return map->emitOpError() << "selected-write mask must AND raw path with full-width address equality";
    auto equal = rawMask.getRhs().getDefiningOp<BitsCompareOp>();
    if (!equal || equal.getPredicate() != "eq")
      return map->emitOpError() << "selected-write address requires ordinary four-state equality";
    auto ordinalWidth = analysis.getPackedWidth(body.getArgument(0).getType(), *bindings, map);
    auto indexWidth = analysis.getPackedWidth(index->resolvedType, *bindings, map);
    if (failed(ordinalWidth) || failed(indexWidth)) return failure();
    auto fullWidth = [&](Value actual, Value expected, uint64_t from, uint64_t to) -> LogicalResult {
      if (from < to) {
        auto resize = actual.getDefiningOp<BitsResizeOp>();
        if (!resize || resize.getMode() != "zext" || resize.getInput() != expected)
          return map->emitOpError() << "selected-write equality must zero-extend its narrower full operand";
        auto width = analysis.getPackedWidth(actual.getType(), *bindings, map);
        if (failed(width) || *width != to) return failure();
        return success();
      }
      if (actual == expected) return success();
      auto extract = actual.getDefiningOp<BitsExtractOp>();
      if (!extract || extract.getInput() != expected)
        return map->emitOpError() << "selected-write equality changes its full address operand";
      auto low = analysis.evaluateStatic(extract.getLow(), *bindings, map);
      auto width = analysis.getPackedWidth(actual.getType(), *bindings, map);
      auto integer = succeeded(low) ? dyn_cast<MathIntAttr>(*low) : MathIntAttr();
      if (!integer || !llvm::APSInt(integer.getCanonicalValue()).isZero() ||
          failed(width) || *width != from || from != to)
        return map->emitOpError() << "selected-write equality truncates its address";
      return success();
    };
    uint64_t width = std::max(*ordinalWidth, *indexWidth);
    if (failed(fullWidth(equal.getLhs(), body.getArgument(0), *ordinalWidth, width)) ||
        failed(fullWidth(equal.getRhs(), body.getArgument(2), *indexWidth, width))) return failure();
    auto merge = yielded->getOperand(0).getDefiningOp<ValueMergeOp>();
    if (!merge || merge.getBase() != body.getArgument(1) || merge.getGuards().size() != 1 ||
        merge.getGuards()[0] != equal.getResult() || merge.getValues().size() != 1 ||
        merge.getValues()[0] != body.getArgument(3) || merge.getPaths().size() != 1 ||
        merge.getPaths()[0] != domain.suffix || !merge.getEn().use_empty())
      return map->emitOpError() << "selected-write data/mask does not share the exact address and selected field";
    result.path = *path; result.index = *index;
    return result;
  }

  LogicalResult proveWriteDomainsDisjoint(size_t stateIndex) {
    auto &state = states[stateIndex];
    using Core = HardwareGrantProof; using Node = Core::Node;
    struct NodeEntry { std::optional<HardwareIdentityEndpoint> endpoint; Core::NodeView view; };
    SmallVector<NodeEntry, 0> nodes;
    using Key = std::tuple<size_t, const void *, const void *>;
    std::map<Key, Node> ids;
    auto synthetic = [&](Core::NodeView view) -> FailureOr<Node> {
      if (failed(charge(budget, state.storage, 1, sizeof(NodeEntry)))) return failure();
      Node id = nodes.size(); nodes.push_back({{}, std::move(view)}); return id;
    };
    auto bind = [&](const HardwareIdentityEndpoint &endpoint) -> FailureOr<Node> {
      auto occurrence = occurrenceFor(endpoint.scope);
      if (failed(occurrence) || failed(budget.debit(orderedLookupCost(ids.size()), state.storage))) return failure();
      Key key{*occurrence, endpoint.value.getAsOpaquePointer(), endpoint.resolvedType.getAsOpaquePointer()};
      auto found = ids.find(key); if (found != ids.end()) return found->second;
      if (failed(payScope(endpoint.scope, budget, state.storage)) ||
          failed(charge(budget, state.storage, 1, sizeof(NodeEntry) + sizeof(Key) + 4 * sizeof(void *)))) return failure();
      Node id = nodes.size(); nodes.push_back({endpoint, {}}); ids.emplace(key, id); return id;
    };
    auto combine = [&](Node left, Node right, Core::NodeView::Kind kind = Core::NodeView::And) -> FailureOr<Node> {
      Core::NodeView view; view.kind = kind; view.width = 1; view.operands.push_back(left);
      if (kind != Core::NodeView::Not) view.operands.push_back(right);
      return synthetic(std::move(view));
    };
    Core proof([&](Node id, unsigned) -> FailureOr<Node> {
      if (id >= nodes.size() || failed(budget.debit(1, state.storage))) return failure();
      return id;
    }, [&](Node id) -> FailureOr<Core::NodeView> {
      if (id >= nodes.size() || failed(budget.debit(1, state.storage))) return failure();
      if (!nodes[id].endpoint) return nodes[id].view;
      // Copy before binding operands: appending nodes may reallocate the vector.
      auto endpoint = *nodes[id].endpoint;
      if (failed(payScope(endpoint.scope, budget, state.storage))) return failure();
      auto bindings = scopeBindings(endpoint.scope, analysis, budget, state.storage);
      if (failed(bindings)) return failure();
      Core::NodeView view;
      if (!isa<BitsType>(endpoint.resolvedType)) return view;
      auto width = analysis.getPackedWidth(endpoint.resolvedType, *bindings, state.storage);
      if (failed(width)) return failure(); view.width = *width;
      auto *operation = endpoint.value.getDefiningOp();
      if (auto constant = dyn_cast_or_null<BitsConstantOp>(operation)) {
        if (failed(payAttribute(constant.getValue(), budget, constant))) return failure();
        auto value = analysis.evaluateStatic(constant.getValue(), *bindings, constant);
        if (failed(value)) return failure();
        if (auto integer = dyn_cast<MathIntAttr>(*value)) {
          if (failed(budget.debit(SourceCheckBudget::add(integer.getCanonicalValue().size(), *width / 64 + 1), constant))) return failure();
          llvm::APSInt number(integer.getCanonicalValue());
          if (!number.isNegative() && number.getActiveBits() <= *width)
            view.fullyKnown = Core::Constant{integer, *width};
        }
        return view;
      }
      if (auto unary = dyn_cast_or_null<BitsUnaryOp>(operation); unary && unary.getOpcode() == "not") view.kind = Core::NodeView::Not;
      else if (auto binary = dyn_cast_or_null<BitsBinaryOp>(operation)) {
        if (binary.getOpcode() == "and") view.kind = Core::NodeView::And;
        if (binary.getOpcode() == "or") view.kind = Core::NodeView::Or;
      } else if (isa_and_nonnull<BitsSelectOp>(operation)) view.kind = Core::NodeView::Select;
      else if (auto compare = dyn_cast_or_null<BitsCompareOp>(operation)) {
        if (compare.getPredicate() == "eq") view.kind = Core::NodeView::Eq;
        if (compare.getPredicate() == "ne") view.kind = Core::NodeView::Ne;
      }
      // case_eq is deliberately opaque: negative KnownOne includes X/Z.
      if (view.kind != Core::NodeView::Raw) {
        if (failed(analysis.verifyResolvedOperation(operation, *bindings))) return failure();
        for (Value value : operation->getOperands()) {
          auto child = normal(value, endpoint.scope); if (failed(child)) return failure();
          auto node = bind(*child); if (failed(node)) return failure(); view.operands.push_back(*node);
        }
      }
      return view;
    }, [&](uint64_t cost) { return succeeded(budget.debit(cost, state.storage)); });
    struct Grant { FinalWriteDomain domain; Node path; };
    SmallVector<SmallVector<Grant, 0>, 0> writers;
    auto contains = [&](const HardwareIdentityEndpoint &path, const HardwareIdentityEndpoint &condition, bool polarity) -> FailureOr<bool> {
      std::function<FailureOr<bool>(HardwareIdentityEndpoint, unsigned)> visit = [&](HardwareIdentityEndpoint at, unsigned depth) -> FailureOr<bool> {
        if (depth > 128 || failed(budget.debit(1, state.storage))) return failure();
        if (polarity && sameEndpoint(at, condition)) return true;
        if (auto inverse = at.value.getDefiningOp<BitsUnaryOp>(); inverse && inverse.getOpcode() == "not" && !polarity) {
          auto raw = normal(inverse.getInput(), at.scope); if (failed(raw)) return failure(); return sameEndpoint(*raw, condition);
        }
        if (auto binary = at.value.getDefiningOp<BitsBinaryOp>(); binary && binary.getOpcode() == "and") {
          for (Value value : binary->getOperands()) { auto child = normal(value, at.scope); if (failed(child)) return failure(); auto found = visit(*child, depth + 1); if (failed(found) || *found) return found; }
        }
        return false;
      };
      return visit(path, 0);
    };
    for (const auto &candidate : state.candidates) {
      const auto &entry = occurrences[candidate.occurrence];
      auto sampled = writerAdmission(entry, candidate.rule); if (failed(sampled)) return failure();
      SmallVector<Grant, 0> grants;
      auto append = [&](FinalWriteDomain domain, Node path) -> LogicalResult {
        for (const auto &guard : (*sampled)->controls) {
          if (guard.kind != SampledControlKind::CallableGuard) continue;
          bool applicable = true; auto execution = guard.executionPath;
          for (unsigned depth = 0; execution.selectionControl; ++depth) {
            if (depth > 128 || failed(budget.debit(1, candidate.rule))) return failure();
            const auto &selection = (*sampled)->controls[*execution.selectionControl];
            auto condition = normal(selection.raw, entry.scope); if (failed(condition)) return failure();
            auto implied = contains(domain.path, *condition, execution.whenTrue); if (failed(implied)) return failure();
            if (!*implied) { applicable = false; break; }
            execution = selection.executionPath;
          }
          if (!applicable) continue;
          auto raw = normal(guard.raw, entry.scope); if (failed(raw)) return failure();
          auto predicate = bind(*raw); if (failed(predicate)) return failure();
          auto joined = combine(path, *predicate); if (failed(joined)) return failure(); path = *joined;
        }
        if (failed(charge(budget, candidate.rule, 1, sizeof(Grant)))) return failure();
        grants.push_back({std::move(domain), path}); return success();
      };
      for (const auto &domain : (*sampled)->writeDomains) {
        if (domain.candidate != candidate.ordinal) continue;
        auto parsed = finalWriteDomain(domain, candidate.rule, entry.scope, stateIndex);
        if (failed(parsed)) return failure();
        auto path = bind(parsed->path); if (failed(path) || failed(append(std::move(*parsed), *path))) return failure();
      }
      if (grants.empty()) {
        // The existing whole-owner contract has no selected mask rows. Its
        // authenticated E=AND(finalFire, rawIntent) still retains branch intent.
        const TransactionWriterView *writer = nullptr;
        for (const auto &possible : entry.wiring->writers) {
          if (failed(budget.debit(1, candidate.rule))) return failure();
          if (possible.rule == candidate.rule) writer = &possible;
        }
        if (!writer || candidate.ordinal >= writer->candidates.size()) return failure();
        auto yielded = cast<YieldOp>(candidate.rule->getRegion(0).front().getTerminator());
        auto effective = normal(yielded->getOperand(writer->candidates[candidate.ordinal].enableIndex), entry.scope);
        if (failed(effective)) return failure();
        auto andOp = effective->value.getDefiningOp<BitsBinaryOp>();
        if (!andOp || andOp.getOpcode() != "and")
          return candidate.rule->emitOpError() << "whole-owner grant requires authenticated raw candidate intent";
        auto intent = normal(andOp.getRhs(), effective->scope); if (failed(intent)) return failure();
        auto path = bind(*intent); if (failed(path)) return failure();
        FinalWriteDomain implicit{ArrayAttr::get(candidate.rule->getContext(), {}), candidate.enable, *intent, {}};
        if (failed(append(std::move(implicit), *path))) return failure();
      }
      if (failed(charge(budget, candidate.rule, 1, sizeof(decltype(grants))))) return failure();
      writers.push_back(std::move(grants));
    }
    for (size_t i = 0; i < writers.size(); ++i) for (size_t j = 0; j < i; ++j)
      for (const auto &left : writers[i]) for (const auto &right : writers[j]) {
        bool fieldsDisjoint = false, indexed = false;
        for (size_t k = 0; k < std::min(left.domain.selectors.size(), right.domain.selectors.size()); ++k) {
          if (failed(budget.debit(1, state.storage))) return failure();
          auto l = cast<DictionaryAttr>(left.domain.selectors[k]), r = cast<DictionaryAttr>(right.domain.selectors[k]);
          auto lf = l.getAs<StringAttr>("field"), rf = r.getAs<StringAttr>("field");
          if (lf && rf && lf != rf) { fieldsDisjoint = true; break; }
          if (bool(lf) != bool(rf)) break;
          if (!lf) indexed = true;
        }
        if (fieldsDisjoint) continue;
        FailureOr<bool> separated = failure();
        if (indexed && left.domain.index && right.domain.index) {
          auto a = bind(*left.domain.index), b = bind(*right.domain.index); if (failed(a) || failed(b)) return failure();
          separated = proof.disjointAtAddress(left.path, right.path, *a, *b);
        } else separated = proof.disjoint(left.path, right.path);
        if (failed(separated)) return state.storage->emitOpError() << "write-domain grant proof failed: " << proof.failureReason();
        if (!*separated) return state.storage->emitOpError() << "unordered writers have a possible field/address collision";
      }
    return success();
  }
  FailureOr<SmallVector<DomainUse, 0>> domainFieldUses(
      ArrayRef<DomainUse> uses, StringAttr field, Operation *site) {
    SmallVector<DomainUse, 0> result;
    for (const auto &use : uses) {
      if (failed(budget.debit(1, site))) return failure();
      if (use.path.empty()) {
        if (failed(charge(budget, site, 1, sizeof(DomainUse)))) return failure();
        result.push_back(use);
      } else if (cast<DictionaryAttr>(use.path.front()).getAs<StringAttr>("field") == field) {
        if (failed(charge(budget, site, 1, sizeof(DomainUse)))) return failure();
        result.push_back({use.path.drop_front(), use.mask});
      }
    }
    return result;
  }
  LogicalResult domainProjection(HardwareIdentityEndpoint actual,
      const HardwareIdentityEndpoint &root, ArrayRef<Attribute> fields,
      size_t state, PrivateRoute::Kind kind = PrivateRoute::Data) {
    for (Attribute field : llvm::reverse(fields)) {
      if (failed(budget.debit(1, valueSite(actual.value)))) return failure();
      auto get = actual.value.getDefiningOp<StructGetOp>();
      if (!get || get.getFieldAttr() != field)
        return valueSite(actual.value)->emitOpError() << "write-domain projection field differs";
      if (failed(privateRoute(actual.value, actual.scope, state, kind)) ||
          failed(allowUse(actual.scope, get, 0, state, kind))) return failure();
      auto parent = normal(get.getValue(), actual.scope); if (failed(parent)) return failure(); actual = *parent;
    }
    return domainSame(actual, root, valueSite(actual.value), "projection");
  }
  FailureOr<HardwareIdentityEndpoint> domainBase(
      HardwareIdentityEndpoint actual, Type type, ArrayRef<DomainUse> uses,
      size_t state, unsigned depth = 0) {
    if (depth > 128 || failed(budget.debit(1, valueSite(actual.value)))) return failure();
    if (uses.empty()) return actual;
    if (auto structure = dyn_cast<StructType>(type)) {
      auto create = actual.value.getDefiningOp<StructCreateOp>();
      auto declaration = analysis.lookupStruct(structure);
      if (!create || !declaration || declaration.getFields().empty() ||
          create->getNumOperands() != declaration.getFields().size())
        return valueSite(actual.value)->emitOpError() << "write-domain Struct reconstruction differs";
      auto field = cast<DictionaryAttr>(declaration.getFields()[0]);
      auto name = field.getAs<StringAttr>("name");
      auto selected = domainFieldUses(uses, name, create);
      auto bindings = scopeBindings(actual.scope, analysis, budget, create);
      if (failed(selected) || failed(bindings)) return failure();
      auto member = analysis.resolveType(field.getAs<TypeAttr>("type").getValue(), *bindings, create);
      auto child = normal(create->getOperand(0), actual.scope);
      if (failed(member) || failed(child)) return failure();
      auto base = domainBase(*child, *member, *selected, state, depth + 1); if (failed(base)) return failure();
      auto get = base->value.getDefiningOp<StructGetOp>();
      if (!get || get.getFieldAttr() != name)
        return create->emitOpError() << "write-domain Struct predecessor projection differs";
      return normal(get.getValue(), base->scope);
    }
    if (isa<TableType>(type)) {
      auto map = actual.value.getDefiningOp<TableMapOp>();
      if (!map || map.getTables().empty())
        return valueSite(actual.value)->emitOpError() << "write-domain Table composition differs";
      return normal(map->getOperand(0), actual.scope);
    }
    auto merge = actual.value.getDefiningOp<ValueMergeOp>();
    if (!merge) return valueSite(actual.value)->emitOpError() << "write-domain leaf requires value.merge";
    return normal(merge.getBase(), actual.scope);
  }
  LogicalResult domainComposition(HardwareIdentityEndpoint actual, Type type,
      const HardwareIdentityEndpoint &base, const HardwareIdentityEndpoint &current,
      const HardwareIdentityEndpoint &enabled, ArrayRef<DomainUse> uses,
      ArrayRef<Attribute> fields, size_t state, unsigned depth = 0) {
    if (depth > 128 || failed(budget.debit(1, valueSite(actual.value)))) return failure();
    if (uses.empty()) return domainProjection(actual, base, fields, state);
    if (failed(privateRoute(actual.value, actual.scope, state, PrivateRoute::Data))) return failure();
    auto bindings = scopeBindings(actual.scope, analysis, budget, valueSite(actual.value));
    if (failed(bindings)) return failure();
    if (auto structure = dyn_cast<StructType>(type)) {
      auto create = actual.value.getDefiningOp<StructCreateOp>();
      auto declaration = analysis.lookupStruct(structure);
      if (!create || !declaration || create->getNumOperands() != declaration.getFields().size())
        return valueSite(actual.value)->emitOpError() << "write-domain Struct composition differs";
      for (auto [ordinal, raw] : llvm::enumerate(declaration.getFields())) {
        auto field = cast<DictionaryAttr>(raw); auto name = field.getAs<StringAttr>("name");
        auto selected = domainFieldUses(uses, name, create);
        auto member = analysis.resolveType(field.getAs<TypeAttr>("type").getValue(), *bindings, create);
        auto child = normal(create->getOperand(ordinal), actual.scope);
        if (failed(selected) || failed(member) || failed(child) ||
            failed(charge(budget, create, fields.size() + 1, sizeof(Attribute)))) return failure();
        SmallVector<Attribute> path(fields.begin(), fields.end()); path.push_back(name);
        if (failed(allowUse(actual.scope, create, ordinal, state, PrivateRoute::Data)) ||
            failed(domainComposition(*child, *member, base, current, enabled, *selected, path, state, depth + 1))) return failure();
      }
      return success();
    }
    if (auto table = dyn_cast<TableType>(type)) {
      auto map = actual.value.getDefiningOp<TableMapOp>();
      if (!map || map->getNumResults() != 1 || !map.getCaptures().empty() ||
          map.getTables().size() != uses.size() + 3 || !map.getBody().hasOneBlock() ||
          failed(analysis.verifyResolvedOperation(map, *bindings)))
        return valueSite(actual.value)->emitOpError() << "write-domain Table composition bindings differ";
      auto old = normal(map->getOperand(0), actual.scope), next = normal(map->getOperand(1), actual.scope);
      auto enable = domainSplat(map->getOperand(2), actual.scope, state, PrivateRoute::Enable);
      if (failed(old) || failed(next) || failed(enable) ||
          failed(domainProjection(*old, base, fields, state)) ||
          failed(domainProjection(*next, current, fields, state)) ||
          failed(domainSame(*enable, enabled, map, "Table enable"))) return failure();
      if (failed(allowUse(actual.scope, map, 0, state, PrivateRoute::Data)) ||
          failed(allowUse(actual.scope, map, 1, state, PrivateRoute::Data)) ||
          failed(allowUse(actual.scope, map, 2, state, PrivateRoute::Enable))) return failure();
      auto &body = map.getBody().front();
      auto yield = dyn_cast<YieldOp>(body.getTerminator());
      if (!yield || yield->getNumOperands() != 1 || body.getNumArguments() != uses.size() + 4)
        return map->emitOpError() << "write-domain Table body bindings differ";
      SmallVector<DomainUse, 0> nested;
      for (auto [ordinal, use] : llvm::enumerate(uses)) {
        auto mask = isa<BitsType>(use.mask.resolvedType)
            ? domainSplat(map->getOperand(ordinal + 3), actual.scope, state, PrivateRoute::Mask)
            : normal(map->getOperand(ordinal + 3), actual.scope);
        if (failed(mask) || failed(domainSame(*mask, use.mask, map, "Table mask")) ||
            failed(allowUse(actual.scope, map, ordinal + 3, state, PrivateRoute::Mask))) return failure();
        auto path = use.path;
        if (!path.empty()) {
          if (!cast<DictionaryAttr>(path.front()).getAs<IntegerAttr>("index"))
            return map->emitOpError() << "write-domain Table selector differs";
          path = path.drop_front();
        }
        auto element = normal(body.getArgument(ordinal + 4), actual.scope);
        if (failed(element) || failed(charge(budget, map, 1, sizeof(DomainUse)))) return failure();
        if (failed(privateRoute(element->value, element->scope, state, PrivateRoute::Mask))) return failure();
        nested.push_back({path, *element});
      }
      auto result = normal(yield->getOperand(0), actual.scope);
      auto oldElement = normal(body.getArgument(1), actual.scope), nextElement = normal(body.getArgument(2), actual.scope);
      auto active = normal(body.getArgument(3), actual.scope);
      if (failed(result) || failed(oldElement) || failed(nextElement) || failed(active)) return failure();
      if (failed(privateRoute(nextElement->value, nextElement->scope, state, PrivateRoute::Data)) ||
          failed(privateRoute(active->value, active->scope, state, PrivateRoute::Enable))) return failure();
      if (failed(allowUse(actual.scope, yield, 0, state, PrivateRoute::Data))) return failure();
      return domainComposition(*result, table.getElementType(), *oldElement, *nextElement,
                               *active, nested, {}, state, depth + 1);
    }
    if (!isa<BitsType, EnumType>(type)) return failure();
    auto merge = actual.value.getDefiningOp<ValueMergeOp>();
    if (!merge || actual.value != merge.getNext() || merge.getGuards().size() != 1 ||
        merge.getValues().size() != 1 || merge.getPaths().size() != 1 ||
        !cast<ArrayAttr>(merge.getPaths()[0]).empty() || !merge.getEn().use_empty())
      return valueSite(actual.value)->emitOpError() << "write-domain leaf composition must merge one whole leaf";
    auto old = normal(merge.getBase(), actual.scope), next = normal(merge.getValues()[0], actual.scope);
    auto guard = normal(merge.getGuards()[0], actual.scope);
    if (failed(old) || failed(next) || failed(guard) ||
        failed(domainProjection(*old, base, fields, state)) ||
        failed(domainProjection(*next, current, fields, state))) return failure();
    auto andOp = guard->value.getDefiningOp<BitsBinaryOp>();
    if (!andOp || andOp.getOpcode() != "and") return merge->emitOpError() << "write-domain leaf guard requires AND(enable, masks)";
    auto enable = normal(andOp.getLhs(), guard->scope), mask = normal(andOp.getRhs(), guard->scope);
    if (failed(enable) || failed(mask) || failed(domainSame(*enable, enabled, merge, "leaf enable"))) return failure();
    for (size_t ordinal = uses.size(); ordinal > 1; --ordinal) {
      auto either = mask->value.getDefiningOp<BitsBinaryOp>();
      if (!either || either.getOpcode() != "or") return merge->emitOpError() << "write-domain masks require their exact OR chain";
      auto latest = normal(either.getRhs(), mask->scope), earlier = normal(either.getLhs(), mask->scope);
      if (failed(latest) || failed(earlier) || failed(domainSame(*latest, uses[ordinal - 1].mask, merge, "leaf mask")) ||
          failed(allowUse(mask->scope, either, 0, state, PrivateRoute::Mask)) ||
          failed(allowUse(mask->scope, either, 1, state, PrivateRoute::Mask))) return failure();
      if (failed(privateRoute(mask->value, mask->scope, state, PrivateRoute::Mask))) return failure();
      mask = *earlier;
    }
    if (failed(domainSame(*mask, uses.front().mask, merge, "leaf first mask"))) return failure();
    if (failed(privateRoute(guard->value, guard->scope, state, PrivateRoute::Enable))) return failure();
    return success(succeeded(allowUse(actual.scope, merge, 0, state, PrivateRoute::Data)) &&
                   succeeded(allowUse(actual.scope, merge, 1, state, PrivateRoute::Enable)) &&
                   succeeded(allowUse(actual.scope, merge, 1, state, PrivateRoute::Mask)) &&
                   succeeded(allowUse(actual.scope, merge, 2, state, PrivateRoute::Data)) &&
                   succeeded(allowUse(guard->scope, andOp, 0, state, PrivateRoute::Enable)) &&
                   succeeded(allowUse(guard->scope, andOp, 1, state, PrivateRoute::Mask)));
  }
  FailureOr<HardwareIdentityEndpoint> peelDomainCommit(
      const HardwareIdentityEndpoint &data, const StateCandidate &candidate, size_t state) {
    auto sampled = writerAdmission(occurrences[candidate.occurrence], candidate.rule);
    if (failed(sampled)) return failure();
    SmallVector<DomainUse, 0> uses;
    for (const auto &domain : (*sampled)->writeDomains) {
      if (domain.candidate != candidate.ordinal) continue;
      auto mask = normal(candidate.rule->getResult(domain.maskResult), candidate.data.scope);
      if (failed(mask) || failed(charge(budget, candidate.rule, 1, sizeof(DomainUse)))) return failure();
      uses.push_back({domain.selectors.getValue(), *mask});
    }
    return peelDomainPair(data, candidate.data, candidate.enable, state, std::move(uses));
  }
  FailureOr<HardwareIdentityEndpoint> peelDomainPair(
      const HardwareIdentityEndpoint &data,
      const HardwareIdentityEndpoint &current,
      const HardwareIdentityEndpoint &enabled, size_t state,
      SmallVector<DomainUse, 0> uses = {}) {
    if (uses.empty()) {
      // Recover the implicit literal-one mask from an actual leaf, then require
      // that value to be one. This does not authorize arbitrary extra masks.
      HardwareIdentityEndpoint leaf = data; Type type = states[state].payload;
      for (unsigned depth = 0; depth <= 128; ++depth) {
        if (failed(budget.debit(1, valueSite(data.value)))) return failure();
        if (auto structure = dyn_cast<StructType>(type)) {
          auto create = leaf.value.getDefiningOp<StructCreateOp>(); auto decl = analysis.lookupStruct(structure);
          if (!create || !decl || create->getNumOperands() == 0)
            return valueSite(leaf.value)->emitOpError()
                   << "whole-owner composition requires its exact Struct reconstruction";
          auto field = cast<DictionaryAttr>(decl.getFields()[0]);
          auto bindings = scopeBindings(leaf.scope, analysis, budget, create); if (failed(bindings)) return failure();
          auto member = analysis.resolveType(field.getAs<TypeAttr>("type").getValue(), *bindings, create);
          auto child = normal(create->getOperand(0), leaf.scope); if (failed(member) || failed(child)) return failure();
          type = *member; leaf = *child; continue;
        }
        if (isa<TableType>(type)) {
          auto map = leaf.value.getDefiningOp<TableMapOp>();
          if (!map || map.getTables().size() != 4)
            return valueSite(leaf.value)->emitOpError()
                   << "whole-owner composition requires its exact Table map";
          auto mask = domainSplat(map->getOperand(3), leaf.scope, state, PrivateRoute::Mask);
          if (failed(mask)) return failure(); auto known = literal(*mask, analysis, budget);
          if (failed(known)) return failure();
          if (!known->has_value() || !**known)
            return map->emitOpError()
                   << "whole-owner composition requires a known-one Table mask";
          uses.push_back({{}, *mask}); break;
        }
        auto merge = leaf.value.getDefiningOp<ValueMergeOp>();
        auto guard = merge && merge.getGuards().size() == 1 ? merge.getGuards()[0].getDefiningOp<BitsBinaryOp>() : BitsBinaryOp();
        if (!guard || guard.getOpcode() != "and")
          return valueSite(leaf.value)->emitOpError()
                 << "whole-owner composition requires exact AND(enable, mask) leaf merge";
        auto mask = normal(guard.getRhs(), leaf.scope); if (failed(mask)) return failure();
        auto known = literal(*mask, analysis, budget);
        if (failed(known)) return failure();
        if (!known->has_value() || !**known)
          return guard->emitOpError()
                 << "whole-owner composition requires a known-one leaf mask";
        uses.push_back({{}, *mask}); break;
      }
    }
    if (uses.empty())
      return valueSite(data.value)->emitOpError()
             << "whole-owner composition lacks its exact literal-one mask";
    auto base = domainBase(data, states[state].payload, uses, state);
    if (failed(base) || failed(domainComposition(data, states[state].payload, *base,
        current, enabled, uses, {}, state))) return failure();
    return *base;
  }
  FailureOr<HardwareIdentityEndpoint> peelDomainEnable(
      const HardwareIdentityEndpoint &enabled, const HardwareIdentityEndpoint &latest, size_t state) {
    auto binary = [&](HardwareIdentityEndpoint at, StringRef opcode)
        -> FailureOr<std::pair<HardwareIdentityEndpoint, HardwareIdentityEndpoint>> {
      auto op = at.value.getDefiningOp<BitsBinaryOp>();
      if (failed(privateRoute(at.value, at.scope, state, PrivateRoute::Enable))) return failure();
      if (!op || op.getOpcode() != opcode) return valueSite(at.value)->emitOpError() << "unordered commit poison-aware enable differs";
      auto left = normal(op.getLhs(), at.scope), right = normal(op.getRhs(), at.scope);
      if (failed(left) || failed(right) || failed(allowUse(at.scope, op, 0, state, PrivateRoute::Enable)) ||
          failed(allowUse(at.scope, op, 1, state, PrivateRoute::Enable))) return failure();
      return std::make_pair(*left, *right);
    };
    auto outer = binary(enabled, "xor"); if (failed(outer)) return failure();
    auto active = binary(outer->first, "or"), poison = binary(outer->second, "or");
    if (failed(active) || failed(poison) || failed(domainSame(active->second, latest, states[state].storage, "latest enable"))) return failure();
    auto oldPoison = binary(poison->first, "xor"), newPoison = binary(poison->second, "xor");
    if (failed(oldPoison) || failed(newPoison) ||
        failed(domainSame(oldPoison->first, active->first, states[state].storage, "prior poison")) ||
        failed(domainSame(oldPoison->second, active->first, states[state].storage, "prior poison")) ||
        failed(domainSame(newPoison->first, latest, states[state].storage, "latest poison")) ||
        failed(domainSame(newPoison->second, latest, states[state].storage, "latest poison"))) return failure();
    return active->first;
  }
  FailureOr<const TransactionIncomingView *> incomingFor(
      size_t occurrence, size_t state) {
    const auto &entry = occurrences[occurrence];
    if (!entry.wiring) return static_cast<const TransactionIncomingView *>(nullptr);
    for (const auto &incoming : entry.wiring->incoming) {
      auto actual = target(entry, incoming.stateID);
      if (failed(actual)) return failure();
      if (*actual == state) return &incoming;
    }
    return static_cast<const TransactionIncomingView *>(nullptr);
  }
  LogicalResult rootIncomingZero(const Occurrence &entry,
                                const TransactionIncomingView &incoming) {
    if (entry.parent || !entry.scope.path.empty() || !entry.wiring)
      return entry.module->emitOpError() << "incoming default requires the exact definition root";
    const auto &ports = entry.wiring->ports;
    for (uint64_t input : {incoming.dataInput, incoming.enableInput}) {
      if (failed(budget.debit(1, entry.module))) return failure();
      if (input < ports.sourceInputCount ||
          input - ports.sourceInputCount >= ports.rootInputDefaults.size())
        return entry.module->emitOpError() << "root incoming lacks its private default";
      auto value = dyn_cast<IntegerAttr>(ports.rootInputDefaults[input - ports.sourceInputCount]);
      if (!value || !value.getType().isSignlessInteger(64) || !value.getValue().isZero())
        return entry.module->emitOpError() << "root incoming D/E defaults must both be known zero";
    }
    return success();
  }
  FailureOr<bool> hasIncomingStage(size_t stage, size_t state) {
    auto owner = occurrenceFor(states[state].scope);
    if (failed(owner)) return failure();
    bool containsStage = false, incoming = false;
    for (std::optional<size_t> at = *owner; at; at = occurrences[*at].parent) {
      if (failed(budget.debit(1, states[state].storage))) return failure();
      containsStage |= *at == stage;
      auto route = incomingFor(*at, state);
      if (failed(route)) return failure();
      incoming |= bool(*route);
    }
    if (!containsStage)
      return states[state].storage->emitOpError() << "transaction prefix stage is outside its actual owner path";
    return incoming;
  }
  // Prove one actual module stage. The only recursive edge is the exact
  // declared incoming pair through its actual allocation in the parent. Thus a
  // transit pair contains precisely its ancestor prefix; storage contains all
  // contributors, each identified by occurrence, registration and ordinal.
  /// A single candidate lane may be either the same-scope raw D/E pair or a
  /// cross-scope lane pre-composed with its declared domains. The latter must
  /// peel back to exactly the physical old Q, so the write permission proved
  /// by domain disjointness is tied to the actual committed leaves.
  LogicalResult proveSingleCandidate(const HardwareIdentityEndpoint &data,
                                     const HardwareIdentityEndpoint &enable,
                                     size_t stateIndex,
                                     const StateCandidate &candidate) {
    auto &state = states[stateIndex];
    auto sampled = writerAdmission(occurrences[candidate.occurrence], candidate.rule);
    if (failed(sampled)) return failure();
    bool domains = false;
    for (const auto &domain : (*sampled)->writeDomains) {
      if (failed(budget.debit(1, candidate.rule))) return failure();
      domains |= domain.candidate == candidate.ordinal;
    }
    if (!domains)
      return success(
          succeeded(domainSame(data, candidate.data, state.storage,
                               "single whole-owner data")) &&
          succeeded(domainSame(enable, candidate.enable, state.storage,
                               "single whole-owner enable")));
    if (sameScope(occurrences[candidate.occurrence].scope, state.scope) &&
        sameEndpoint(data, candidate.data) &&
        sameEndpoint(enable, candidate.enable))
      return success();
    auto base = peelDomainCommit(data, candidate, stateIndex);
    auto oldQ = normal(state.storage->getResult(0), state.scope);
    if (failed(base) || failed(oldQ) || !sameEndpoint(*oldQ, *base) ||
        !sameEndpoint(enable, candidate.enable))
      return state.storage->emitOpError()
             << "single cross-scope partial lane was not composed over old Q";
    return success();
  }

  LogicalResult proveStagedCommit(HardwareIdentityEndpoint data,
      HardwareIdentityEndpoint enable, size_t stage, size_t stateIndex,
      bool storage, unsigned depth = 0) {
    auto &state = states[stateIndex];
    const auto &entry = occurrences[stage];
    if (depth > 128 || failed(budget.debit(1, entry.module))) return failure();
    auto incoming = incomingFor(stage, stateIndex);
    if (failed(incoming)) return failure();
    SmallVector<const StateCandidate *, 0> local;
    uint64_t expected = 0;
    for (const auto &candidate : state.candidates) {
      if (failed(budget.debit(1, candidate.rule))) return failure();
      bool ancestor = false;
      for (std::optional<size_t> at = stage; at; at = occurrences[*at].parent) {
        if (failed(budget.debit(1, candidate.rule))) return failure();
        ancestor |= *at == candidate.occurrence;
      }
      expected += ancestor;
      if (candidate.occurrence == stage) {
        if (failed(charge(budget, candidate.rule, 1, sizeof(StateCandidate *)))) return failure();
        local.push_back(&candidate);
      }
    }
    if (storage && expected != state.candidates.size())
      return state.storage->emitOpError() << "storage commit misses actual owner contributors";
    auto oldQ = normal(state.storage->getResult(0), state.scope);
    if (failed(oldQ)) return failure();
    if (!*incoming && expected != local.size())
      return entry.module->emitOpError()
             << "transaction prefix is missing its ancestor incoming route";
    if (!*incoming && local.size() == 1)
      return proveSingleCandidate(data, enable, stateIndex, *local.front());
    if (!*incoming && local.empty()) {
      auto enabled = literal(enable, analysis, budget);
      if (failed(enabled) || !enabled->has_value() || **enabled)
        return entry.module->emitOpError() << "empty transaction prefix requires known-zero enable";
      if (sameEndpoint(data, *oldQ)) return success();
      auto zero = literal(data, analysis, budget);
      if (failed(zero) || !zero->has_value() || **zero)
        return entry.module->emitOpError() << "empty transaction prefix requires old Q or known-zero data";
      return success();
    }
    if (!*incoming && !state.unordered.value_or(false)) {
      // The producer starts an ordered local group with its exact first
      // candidate, then folds later candidates using selection and enable OR.
      // A descendant incoming closure must preserve this ancestor prefix; it
      // does not turn that group into disjoint unordered domain writers.
      for (size_t i = local.size(); i > 1; --i) {
        const auto &candidate = *local[i - 1];
        auto prior = peelSelection(data, candidate.enable, candidate.data,
                                   stateIndex);
        auto earlier = peelEnableOr(enable, candidate.enable, stateIndex);
        if (failed(prior) || failed(earlier) ||
            failed(privateRoute(data.value, data.scope, stateIndex,
                                PrivateRoute::Data)) ||
            failed(privateRoute(enable.value, enable.scope, stateIndex,
                                PrivateRoute::Enable))) return failure();
        data = *prior;
        enable = *earlier;
      }
      // The first ancestor partial lane is already composed with physical
      // old Q before routing. Authenticate its masks/base independently rather
      // than requiring the raw Rule result used by a same-scope writer.
      return proveSingleCandidate(data, enable, stateIndex, *local.front());
    }
    for (size_t i = local.size(); i > 0; --i) {
      const auto &candidate = *local[i - 1];
      auto prior = peelDomainCommit(data, candidate, stateIndex);
      if (failed(prior)) return failure();
      if (*incoming || i > 1) {
        auto earlier = peelDomainEnable(enable, candidate.enable, stateIndex);
        if (failed(earlier)) return failure();
        enable = *earlier;
      } else if (failed(domainSame(enable, candidate.enable, entry.module, "first enable"))) {
        return failure();
      }
      data = *prior;
    }
    if (!*incoming)
      return domainSame(data, *oldQ, entry.module, "staged old-Q");
    auto rawData = normal((*incoming)->data, entry.scope);
    auto rawEnable = normal((*incoming)->enable, entry.scope);
    if (failed(rawData) || failed(rawEnable) ||
        failed(domainSame(enable, *rawEnable, entry.module, "incoming enable"))) return failure();
    // An otherwise unwritten incoming hold may retain its exact raw pair.
    // With no actual contributor anywhere, recursion still authenticates the
    // parent pair and the standalone known-zero defaults. A live hierarchy
    // instead requires the closed old-Q prefix at every incoming stage.
    bool unusedHold = state.candidates.empty() && sameEndpoint(data, *rawData);
    if (!unusedHold) {
      auto base = peelDomainPair(data, *rawData, *rawEnable, stateIndex);
      if (failed(base) || failed(domainSame(*base, *oldQ, entry.module, "incoming old-Q"))) return failure();
    }
    if (!entry.parent)
      return rootIncomingZero(entry, **incoming);
    return proveStagedCommit(*rawData, *rawEnable, *entry.parent,
                             stateIndex, false, depth + 1);
  }
  LogicalResult proveUnorderedCommit(const HardwareIdentityEndpoint &data,
      const HardwareIdentityEndpoint &enable, size_t stateIndex) {
    auto &state = states[stateIndex];
    if (state.commitProven) return success(state.data && state.enable && sameEndpoint(*state.data, data) && sameEndpoint(*state.enable, enable));
    if (failed(proveWriteDomainsDisjoint(stateIndex))) return failure();
    HardwareIdentityEndpoint prior = data, enabled = enable;
    for (size_t i = state.candidates.size(); i > 0; --i) {
      auto base = peelDomainCommit(prior, state.candidates[i - 1], stateIndex); if (failed(base)) return failure();
      if (failed(privateRoute(prior.value, prior.scope, stateIndex, PrivateRoute::Data))) return failure();
      prior = *base;
      if (i > 1) { auto previous = peelDomainEnable(enabled, state.candidates[i - 1].enable, stateIndex); if (failed(previous)) return failure(); enabled = *previous; }
    }
    auto oldQ = normal(state.storage->getResult(0), state.scope);
    if (failed(oldQ) || failed(domainSame(prior, *oldQ, state.storage, "unordered old-Q")) ||
        failed(domainSame(enabled, state.candidates.front().enable, state.storage, "first enable"))) return failure();
    state.data = data; state.enable = enable; state.commitProven = true; return success();
  }
