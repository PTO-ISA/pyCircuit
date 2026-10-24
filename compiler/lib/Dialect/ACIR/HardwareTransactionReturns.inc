// Validate result/availability transport independently of source transaction
// plans. A return lane forwards one public Rule result and that Rule's whole
// fire through actual instance operands to its original lexical consumers.
namespace {
using detail::verifyOccurrence;
using detail::verifyOriginDefinition;
using detail::resolveHardwareRootBindings;
class TransactionReturnProof {
  const HardwareAnalysis &analysis;
  const std::function<LogicalResult(uint64_t, Operation *)> &debit;
  struct Incoming {
    ArrayAttr path;
    DictionaryAttr registration;
    uint64_t ordinal, data, enable;
  };
  struct Definition {
    ModuleOp module;
    SmallVector<Incoming> incoming;
    ArrayAttr writers;
  };
  DenseMap<Operation *, unsigned> indices;
  SmallVector<Definition> definitions;
  using Pin = std::pair<Operation *, uint64_t>;
  std::set<Pin> connected;

  LogicalResult pay(Operation *site, uint64_t count = 1) {
    return debit ? debit(count, site) : success();
  }
  LogicalResult fail(Operation *site, StringRef reason) {
    return site->emitOpError() << "transaction return routing: " << reason;
  }
  bool exact(DictionaryAttr row, ArrayRef<StringRef> fields) {
    return row && row.size() == fields.size() &&
           llvm::all_of(fields, [&](StringRef key) { return bool(row.get(key)); });
  }
  FailureOr<uint64_t> number(DictionaryAttr row, StringRef key, Operation *site) {
    auto value = row ? row.getAs<IntegerAttr>(key) : IntegerAttr();
    if (!value || !value.getType().isSignlessInteger(64) || value.getInt() < 0)
      return fail(site, "ordinals must be nonnegative i64 values");
    return uint64_t(value.getInt());
  }
  FailureOr<ArrayAttr> owner(DictionaryAttr row, Operation *site) {
    auto value = row ? row.getAs<DictionaryAttr>("owner") : DictionaryAttr();
    auto path = value ? value.getAs<ArrayAttr>("instance_path") : ArrayAttr();
    if (!exact(value, {"instance_path"}) || !path || path.size() > 128)
      return fail(site, "owner must be a bounded instance path");
    for (Attribute raw : path)
      if (failed(verifyOccurrence(dyn_cast<DictionaryAttr>(raw),
                                  [&] { return site->emitOpError(); })))
        return failure();
    return path;
  }
  FailureOr<InstanceOp> allocation(ModuleOp module, Attribute occurrence,
                                   Operation *site) {
    InstanceOp result;
    for (auto instance : module.getBody().front().getOps<InstanceOp>()) {
      if (failed(pay(site))) return failure();
      if (instance.getOccurrence() != occurrence) continue;
      if (result) return fail(site, "ambiguous allocation occurrence");
      result = instance;
    }
    if (!result) return fail(site, "owner path has no actual allocation");
    return result;
  }
  LogicalResult target(ModuleOp module, ArrayAttr path,
                       DictionaryAttr registration, Operation *site) {
    for (Attribute occurrence : path) {
      auto instance = allocation(module, occurrence, site);
      if (failed(instance)) return failure();
      module = dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(*instance));
      if (!module || !analysis.getPrimitiveKind(module).empty())
        return fail(site, "owner path must resolve ordinary Modules");
    }
    return verifyOriginDefinition(registration,
        FlatSymbolRefAttr::get(module.getSymNameAttr()),
        "transaction return registration", [&] { return site->emitOpError(); });
  }
  LogicalResult connect(ModuleOp parent, ArrayAttr path,
                         DictionaryAttr registration, uint64_t ordinal,
                         Value data, Value enable, Operation *site) {
    if (path.empty()) return fail(site, "producer must target a child allocation");
    auto instance = allocation(parent, path[0], site);
    if (failed(instance)) return failure();
    auto child = dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(*instance));
    auto index = indices.find(child);
    if (!child || index == indices.end())
      return fail(site, "child lacks its return route declaration");
    auto suffix = path.getValue().drop_front();
    const Incoming *selected = nullptr;
    for (const Incoming &route : definitions[index->second].incoming) {
      if (failed(pay(site, suffix.size() + 1))) return failure();
      if (route.registration == registration && route.ordinal == ordinal &&
          llvm::equal(route.path, suffix)) {
        if (selected) return fail(site, "duplicate child return route");
        selected = &route;
      }
    }
    if (!selected || selected->enable >= instance->getNumOperands())
      return fail(site, "child return route or input pair is missing");
    if (instance->getOperand(selected->data) != data ||
        instance->getOperand(selected->enable) != enable)
      return fail(site, "child result or whole-transaction fire binding differs");
    if (!connected.insert({instance->getOperation(), selected->data}).second)
      return fail(site, "more than one producer drives a return route");
    return success();
  }

public:
  TransactionReturnProof(const HardwareAnalysis &analysis,
      const std::function<LogicalResult(uint64_t, Operation *)> &debit)
      : analysis(analysis), debit(debit) {}

  LogicalResult run() {
    auto package = analysis.getPackage();
    auto placements = package.walk([&](Operation *op) {
      if (op->hasAttr("ac.transaction_returns") && !isa<ModuleOp>(op)) {
        (void)fail(op, "metadata belongs only on Module definitions");
        return WalkResult::interrupt();
      }
      return WalkResult::advance();
    });
    if (placements.wasInterrupted()) return failure();
    for (auto module : package.getOps<ModuleOp>()) {
      Attribute raw = module->getAttr("ac.transaction_returns");
      if (!raw) continue;
      if (failed(payEffectMetadata(raw, module, debit))) return failure();
      auto metadata = dyn_cast<DictionaryAttr>(raw);
      auto incoming = metadata ? metadata.getAs<ArrayAttr>("incoming") : ArrayAttr();
      auto writers = metadata ? metadata.getAs<ArrayAttr>("writers") : ArrayAttr();
      auto ports = getModulePortLayout(module);
      if (!exact(metadata, {"incoming", "writers"}) || !incoming || !writers ||
          (incoming.empty() && writers.empty()) || failed(ports))
        return fail(module, "metadata requires incoming routes or writers");
      if (failed(pay(module, 8 + incoming.size() * 8))) return failure();
      indices.try_emplace(module, definitions.size());
      definitions.push_back({module, {}, writers});
      DenseSet<uint64_t> pins;
      for (Attribute rawRoute : incoming) {
        auto row = dyn_cast<DictionaryAttr>(rawRoute);
        if (!exact(row, {"owner", "registration", "ordinal", "data", "enable"}))
          return fail(module, "incoming route has unknown or missing fields");
        auto path = owner(row, module);
        auto registration = row.getAs<DictionaryAttr>("registration");
        auto ordinal = number(row, "ordinal", module);
        auto data = number(row, "data", module);
        auto enable = number(row, "enable", module);
        if (failed(path) || failed(ordinal) || failed(data) || failed(enable) ||
            failed(verifyOccurrence(registration, [&] { return module.emitOpError(); })))
          return failure();
        if (*data < ports->sourceInputCount || *enable != *data + 1 ||
            *enable >= module.getBody().front().getNumArguments() ||
            !pins.insert(*data).second || !pins.insert(*enable).second)
          return fail(module, "return D/E must be distinct appended input pairs");
        HardwareBindings bindings;
        bindings.owner = module;
        Value predicate = module.getBody().front().getArgument(*enable);
        auto width = analysis.getPackedWidth(predicate.getType(), bindings, module);
        if (!isa<BitsType>(predicate.getType()) || failed(width) || *width != 1)
          return fail(module, "return availability must be one-bit fire");
        if (failed(target(module, *path, registration, module))) return failure();
        for (const Incoming &previous : definitions.back().incoming) {
          if (failed(pay(module))) return failure();
          if (previous.path == *path && previous.registration == registration &&
              previous.ordinal == *ordinal)
            return fail(module, "duplicate return destination");
        }
        definitions.back().incoming.push_back({*path, registration, *ordinal, *data, *enable});
      }
    }
    for (auto module : package.getOps<ModuleOp>())
      for (auto rule : module.getBody().front().getOps<RuleOp>()) {
        if (!rule->hasAttr("ac.registration_path")) continue;
        auto wiring = module->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
        auto writers = wiring ? wiring.getAs<ArrayAttr>("writers") : ArrayAttr();
        if (!writers) return fail(rule, "lifted Rule lacks its writer layout");
        for (Attribute raw : writers) {
          auto writer = dyn_cast<DictionaryAttr>(raw);
          if (!writer) return fail(rule, "invalid transaction writer layout");
          if (writer.get("registration") != rule->getAttr("occurrence")) continue;
          auto count = number(writer, "ordinary_results", rule);
          if (failed(count)) return failure();
          if (*count && !indices.contains(module))
            return fail(rule, "lifted public results lack return metadata");
        }
      }
    if (definitions.empty()) return success();
    auto root = resolveHardwareRootBindings(analysis);
    if (failed(root)) return failure();
    if (auto index = indices.find(root->owner); index != indices.end() &&
        !definitions[index->second].incoming.empty())
      return fail(root->owner, "selected root cannot have unbound return inputs");

    for (const Definition &definition : definitions) {
      auto module = definition.module;
      for (const Incoming &route : definition.incoming) {
        if (route.path.empty()) continue;
        if (failed(connect(module, route.path, route.registration, route.ordinal,
                           module.getBody().front().getArgument(route.data),
                           module.getBody().front().getArgument(route.enable), module)))
          return failure();
      }
      DenseSet<Operation *> producers;
      for (Attribute raw : definition.writers) {
        auto row = dyn_cast<DictionaryAttr>(raw);
        if (!exact(row, {"registration", "owner", "lexical_registration", "results"}))
          return fail(module, "return writer has unknown or missing fields");
        auto path = owner(row, module);
        auto registration = row.getAs<DictionaryAttr>("registration");
        auto lexical = row.getAs<DictionaryAttr>("lexical_registration");
        auto results = row.getAs<ArrayAttr>("results");
        if (failed(path) || path->empty() || !results || results.empty() ||
            failed(verifyOccurrence(registration, [&] { return module.emitOpError(); })) ||
            failed(verifyOccurrence(lexical, [&] { return module.emitOpError(); })))
          return failure();
        RuleOp rule;
        for (auto candidate : module.getBody().front().getOps<RuleOp>()) {
          if (failed(pay(module))) return failure();
          if (candidate->getAttr("occurrence") != registration) continue;
          if (rule) return fail(module, "return writer registration is ambiguous");
          rule = candidate;
        }
        auto admission = rule ? rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission") : DictionaryAttr();
        if (!rule || !admission || !producers.insert(rule).second ||
            rule->getAttr("ac.registration_path") != *path ||
            rule->getAttr("ac.lexical_registration") != lexical)
          return fail(module, "return writer differs from its routed registration");
        auto fire = number(admission, "fire_result", rule);
        if (failed(fire)) return failure();
        auto wiring = module->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
        auto writers = wiring ? wiring.getAs<ArrayAttr>("writers") : ArrayAttr();
        std::optional<uint64_t> ordinary;
        if (writers) for (Attribute rawWriter : writers) {
          auto writer = dyn_cast<DictionaryAttr>(rawWriter);
          if (writer && writer.get("registration") == registration) {
            auto count = number(writer, "ordinary_results", rule);
            if (failed(count) || ordinary) return fail(rule, "ambiguous public result layout");
            ordinary = *count;
          }
        }
        if (!ordinary || *ordinary != results.size())
          return fail(rule, "return routes must cover every public result");
        for (auto [ordinal, rawResult] : llvm::enumerate(results)) {
          auto result = dyn_cast<DictionaryAttr>(rawResult);
          if (!exact(result, {"ordinal", "data", "enable"}))
            return fail(rule, "return result has unknown or missing fields");
          auto position = number(result, "ordinal", rule);
          auto data = number(result, "data", rule);
          auto enable = number(result, "enable", rule);
          if (failed(position) || failed(data) || failed(enable)) return failure();
          if (*position != ordinal || *data != ordinal || *enable != *fire ||
              *enable >= rule->getNumResults())
            return fail(rule, "return pair must use the public result and entire transaction fire");
          if (failed(connect(module, *path, lexical, ordinal,
                             rule->getResult(*data), rule->getResult(*enable), rule)))
            return failure();
        }
      }
      for (auto rule : module.getBody().front().getOps<RuleOp>()) {
        if (!rule->hasAttr("ac.registration_path")) continue;
        auto wiring = module->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
        auto writers = wiring ? wiring.getAs<ArrayAttr>("writers") : ArrayAttr();
        if (writers) for (Attribute raw : writers) {
          auto writer = dyn_cast<DictionaryAttr>(raw);
          if (!writer) return fail(rule, "invalid transaction writer layout");
          if (writer.get("registration") != rule->getAttr("occurrence")) continue;
          auto count = number(writer, "ordinary_results", rule);
          if (failed(count)) return failure();
          if (*count && !producers.contains(rule))
            return fail(rule, "lifted public results lack return routes");
        }
      }
    }
    for (auto module : package.getOps<ModuleOp>())
      for (auto instance : module.getBody().front().getOps<InstanceOp>()) {
        auto child = dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(instance));
        auto found = indices.find(child);
        if (found == indices.end()) continue;
        for (const Incoming &route : definitions[found->second].incoming) {
          if (failed(pay(instance))) return failure();
          if (!connected.contains({instance.getOperation(), route.data}))
            return fail(instance, "return input has no authenticated producer");
        }
      }
    return success();
  }
};
} // namespace

LogicalResult detail::verifyTransactionReturns(
    const HardwareAnalysis &analysis,
    const std::function<LogicalResult(uint64_t, Operation *)> &debit) {
  return TransactionReturnProof(analysis, debit).run();
}
