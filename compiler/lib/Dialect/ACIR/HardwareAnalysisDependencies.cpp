#include "ACIRSourceContracts.h"
#include "HardwareSourceCheckPreflight.h"
#include "HardwareSourceChecks.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/StringSet.h"
#include <optional>
using namespace mlir;
namespace acir::ac {
namespace {
Type dependencyPayload(Type type) {
  while (auto table = dyn_cast<TableType>(type))
    type = table.getElementType();
  return type;
}
bool prefix(ArrayRef<StringAttr> a, ArrayRef<StringAttr> b) {
  return a.size() <= b.size() && std::equal(a.begin(), a.end(), b.begin());
}
struct Engine {
  const HardwareAnalysis &analysis;
  llvm::DenseSet<Operation *> active;
  // Validation consumes dependency summaries and cycle checks, not schedules.
  // Retain the full ModuleAnalysis only for public scheduling requests.
  bool validationOnly = false;
  std::function<LogicalResult(uint64_t, Operation *)> debit;
  LogicalResult charge(uint64_t units, Operation *site) const {
    return debit ? debit(units, site) : success();
  }
  struct LocalReport {
    SourceObserveOp observe;
    StringAttr name;
    SmallVector<InstanceOp, 0> ownerPath;
  };
  struct CompletedModule {
    ModuleOp module;
    HardwareBindings bindings;
    ModuleAnalysis result;
    SmallVector<LocalReport, 0> reports;
  };
  // A result depends on the definition and its full binding environment, not on
  // the parent occurrence. Keep it only for this immutable analysis traversal.
  SmallVector<CompletedModule, 0> completed{};
  uint64_t resultCost(const ModuleAnalysis &result) const {
    using Budget = detail::SourceCheckBudget;
    uint64_t size = Budget::add(
        sizeof(ModuleAnalysis),
        Budget::multiply(result.schedule.size() + result.storageWork.size(),
                         sizeof(Operation *)));
    for (const auto &wire : result.wireSchedule)
      size = Budget::add(size,
                         Budget::add(sizeof(WireStep),
                                     Budget::multiply(wire.output.path.size(),
                                                      sizeof(StringAttr))));
    for (const auto &dependency : result.dependencies) {
      size = Budget::add(
          size, Budget::add(sizeof(OutputDependency),
                            Budget::multiply(dependency.output.path.size(),
                                             sizeof(StringAttr))));
      for (const auto &input : dependency.inputs)
        size = Budget::add(size,
                           Budget::add(sizeof(PortEndpoint),
                                       Budget::multiply(input.path.size(),
                                                        sizeof(StringAttr))));
    }
    return Budget::bytes(size);
  }
  bool sameBindings(const HardwareBindings &left,
                    const HardwareBindings &right) const {
    auto sameMap = [](const auto &a, const auto &b) {
      if (a.size() != b.size())
        return false;
      for (const auto &entry : a) {
        auto found = b.find(entry.getKey());
        if (found == b.end() || found->second != entry.second)
          return false;
      }
      return true;
    };
    return left.owner == right.owner &&
           sameMap(left.integers, right.integers) &&
           sameMap(left.types, right.types);
  }
  FailureOr<SmallVector<FieldPath>>
  computePaths(Type type, const HardwareBindings &bindings, Operation *site,
               FieldPath base = {}) {
    if (failed(charge(detail::SourceCheckBudget::add(1, base.size()), site)))
      return failure();
    auto resolved = analysis.resolveType(type, bindings, site);
    if (failed(resolved))
      return failure();
    SmallVector<FieldPath> out;
    if (auto table = dyn_cast<TableType>(*resolved))
      return computePaths(table.getElementType(), bindings, site, base);
    if (auto record = dyn_cast<StructType>(*resolved)) {
      auto decl = analysis.lookupStruct(record);
      if (!decl)
        return failure();
      for (Attribute raw : decl.getFields()) {
        auto field = cast<DictionaryAttr>(raw);
        if (failed(
                charge(detail::SourceCheckBudget::add(1, base.size()), site)))
          return failure();
        auto path = base;
        path.push_back(field.getAs<StringAttr>("name"));
        auto children = computePaths(field.getAs<TypeAttr>("type").getValue(),
                                     bindings, site, path);
        if (failed(children))
          return failure();
        for (const auto &child : *children)
          if (failed(charge(
                  detail::SourceCheckBudget::add(
                      detail::SourceCheckBudget::bytes(sizeof(FieldPath)),
                      child.size()),
                  site)))
            return failure();
        llvm::append_range(out, *children);
      }
    } else if (failed(charge(
                   detail::SourceCheckBudget::add(
                       detail::SourceCheckBudget::bytes(sizeof(FieldPath)),
                       base.size()),
                   site)))
      return failure();
    else
      out.push_back(base);
    return out;
  }
  // Repeated operands often use the same nominal record layout. Cache only
  // exact empty-binding layouts within this immutable analysis invocation;
  // actual parameterized occurrences retain their original resolution path.
  llvm::DenseMap<std::pair<Operation *, Type>, SmallVector<FieldPath>>
      pathLayouts{};
  FailureOr<SmallVector<FieldPath>>
  paths(Type type, const HardwareBindings &bindings, Operation *site) {
    if (isa<BitsType, EnumType>(type) || !bindings.integers.empty() ||
        !bindings.types.empty())
      return computePaths(type, bindings, site);
    if (failed(charge(4, site)))
      return failure();
    auto key = std::make_pair(bindings.owner, type);
    if (auto found = pathLayouts.find(key); found != pathLayouts.end()) {
      uint64_t cost =
          detail::SourceCheckBudget::bytes(sizeof(SmallVector<FieldPath>));
      for (const auto &path : found->second)
        cost = detail::SourceCheckBudget::add(
            cost, detail::SourceCheckBudget::add(
                      path.size(), detail::SourceCheckBudget::bytes(
                                       sizeof(FieldPath) +
                                       path.size() * sizeof(StringAttr))));
      if (failed(charge(cost, site)))
        return failure();
      return found->second;
    }
    auto result = computePaths(type, bindings, site);
    if (failed(result))
      return failure();
    using Budget = detail::SourceCheckBudget;
    using Bucket = decltype(pathLayouts)::value_type;
    uint64_t old = pathLayouts.getMemorySize() / sizeof(Bucket),
             capacity = std::max(uint64_t(64), old);
    while ((pathLayouts.size() + 1) * 4 >= capacity * 3) {
      if (failed(charge(1, site)))
        return failure();
      capacity = Budget::multiply(capacity, 2);
    }
    uint64_t copy = 16;
    for (const auto &path : *result)
      copy = Budget::add(copy, Budget::bytes(sizeof(FieldPath) +
                                             path.size() * sizeof(StringAttr)));
    if (capacity > old)
      copy = Budget::add(
          copy, Budget::add(old, Budget::bytes(Budget::multiply(
                                     capacity - old, sizeof(Bucket)))));
    if (failed(charge(copy, site)))
      return failure();
    pathLayouts.try_emplace(key, *result);
    return result;
  }
  FailureOr<Type> subtree(Type type, ArrayRef<StringAttr> path,
                          const HardwareBindings &bindings, Operation *site) {
    auto resolved = analysis.resolveType(type, bindings, site);
    if (failed(resolved))
      return failure();
    type = *resolved;
    for (auto name : path) {
      if (auto table = dyn_cast<TableType>(type))
        type = table.getElementType();
      auto record = dyn_cast<StructType>(type);
      if (!record)
        return site->emitOpError()
               << "dependency path projects a non-struct payload";
      auto decl = analysis.lookupStruct(record);
      bool found = false;
      for (auto raw : decl.getFields()) {
        auto field = cast<DictionaryAttr>(raw);
        if (field.getAs<StringAttr>("name") != name)
          continue;
        type = field.getAs<TypeAttr>("type").getValue();
        found = true;
        break;
      }
      if (!found)
        return site->emitOpError() << "dependency path names unknown field";
    }
    return type;
  }
  FailureOr<PortEndpoint> endpoint(Attribute raw, TypeRange types,
                                   const HardwareBindings &bindings,
                                   Operation *site) {
    auto dict = dyn_cast<DictionaryAttr>(raw);
    auto port = dict ? dict.getAs<IntegerAttr>("port") : IntegerAttr();
    auto path = dict ? dict.getAs<ArrayAttr>("path") : ArrayAttr();
    if (!dict || dict.size() != 2 || !port || port.getValue().isNegative() ||
        port.getValue().getActiveBits() > 32 ||
        port.getValue().getZExtValue() >= types.size() || !path)
      return site->emitOpError()
             << "dependency endpoint requires valid port and field path";
    PortEndpoint out;
    out.port = port.getValue().getZExtValue();
    if (failed(
            charge(detail::SourceCheckBudget::add(
                       detail::SourceCheckBudget::bytes(sizeof(PortEndpoint)),
                       path.size()),
                   site)))
      return failure();
    for (auto item : path) {
      auto name = dyn_cast<StringAttr>(item);
      if (!name || name.getValue().empty())
        return site->emitOpError() << "dependency path requires field names";
      out.path.push_back(name);
    }
    if (failed(subtree(types[out.port], out.path, bindings, site)))
      return failure();
    return out;
  }
  FailureOr<SmallVector<OutputDependency>>
  summary(ModuleImportOp imported, const HardwareBindings &bindings) {
    if (failed(detail::verifyModuleImportSummarySelection(imported)))
      return failure();
    if (imported.getSemanticKind() != "hardware")
      return imported.emitOpError()
             << "hardware dependencies cannot consume a transactions import";
    auto sig = imported.getFunctionType();
    SmallVector<OutputDependency> out;
    for (Attribute raw : imported.getDependencySummaryAttr()) {
      if (failed(
              charge(detail::SourceCheckBudget::bytes(sizeof(OutputDependency)),
                     imported)))
        return failure();
      auto item = dyn_cast<DictionaryAttr>(raw);
      auto inputs = item ? item.getAs<ArrayAttr>("inputs") : ArrayAttr();
      if (!item || item.size() != 2 || !inputs)
        return imported.emitOpError()
               << "dependency entry requires output and inputs";
      auto output =
          endpoint(item.get("output"), sig.getResults(), bindings, imported);
      if (failed(output))
        return failure();
      auto outputType = subtree(sig.getResult(output->port), output->path,
                                bindings, imported);
      if (failed(outputType))
        return failure();
      Type outputPayload = dependencyPayload(*outputType);
      auto outputPaths = paths(*outputType, bindings, imported);
      if (failed(outputPaths))
        return failure();
      for (auto suffix : *outputPaths) {
        if (failed(charge(
                detail::SourceCheckBudget::add(
                    detail::SourceCheckBudget::bytes(sizeof(OutputDependency)),
                    output->path.size() + suffix.size()),
                imported)))
          return failure();
        OutputDependency entry;
        entry.output = *output;
        llvm::append_range(entry.output.path, suffix);
        for (auto rawInput : inputs) {
          if (failed(charge(detail::SourceCheckBudget::add(
                                1, detail::SourceCheckBudget::bytes(
                                       sizeof(PortEndpoint))),
                            imported)))
            return failure();
          auto input = endpoint(rawInput, sig.getInputs(), bindings, imported);
          if (failed(input))
            return failure();
          auto inputType = subtree(sig.getInput(input->port), input->path,
                                   bindings, imported);
          if (failed(inputType))
            return failure();
          Type inputPayload = dependencyPayload(*inputType);
          if (!isa<BitsType, EnumType>(inputPayload)) {
            if (inputPayload != outputPayload)
              return imported.emitOpError()
                     << "aggregate dependency endpoint types must match";
            llvm::append_range(input->path, suffix);
          }
          if (failed(charge(detail::SourceCheckBudget::add(input->path.size(),
                                                           entry.inputs.size()),
                            imported)))
            return failure();
          if (llvm::is_contained(entry.inputs, *input))
            return imported.emitOpError()
                   << "duplicate dependency input endpoint";
          entry.inputs.push_back(*input);
        }
        if (failed(charge(out.size(), imported)))
          return failure();
        for (const auto &previous : out)
          if (previous.output == entry.output)
            return imported.emitOpError()
                   << "overlapping dependency output prefixes";
        out.push_back(entry);
      }
    }
    for (auto [port, type] : llvm::enumerate(sig.getResults())) {
      auto leaves = paths(type, bindings, imported);
      if (failed(leaves))
        return failure();
      for (auto path : *leaves)
        if (llvm::count_if(out, [&](auto &e) {
              return e.output.port == port && e.output.path == path;
            }) != 1)
          return imported.emitOpError() << "dependency summary must cover "
                                           "every output leaf exactly once";
    }
    return out;
  }
  FailureOr<SmallVector<WireEndpoint>>
  dependencies(WireEndpoint endpoint, const HardwareBindings &bindings) {
    if (failed(charge(detail::SourceCheckBudget::add(1, endpoint.path.size()),
                      endpoint.value.getParentRegion()->getParentOp())))
      return failure();
    Value value = endpoint.value;
    Operation *op = value.getDefiningOp();
    SmallVector<WireEndpoint> out;
    if (auto arg = dyn_cast<BlockArgument>(value)) {
      if (auto rule = dyn_cast_or_null<RuleOp>(arg.getOwner()->getParentOp()))
        out.push_back({rule.getCaptures()[arg.getArgNumber()], endpoint.path});
      else if (auto map = dyn_cast_or_null<TableMapOp>(
                   arg.getOwner()->getParentOp())) {
        unsigned i = arg.getArgNumber();
        if (i)
          out.push_back({map.getOperand(i - 1), endpoint.path});
      } else if (auto match = dyn_cast_or_null<TableMatchOp>(
                     arg.getOwner()->getParentOp()))
        out.push_back({match.getOperand(arg.getArgNumber()), endpoint.path});
      return out;
    }
    if (!op)
      return failure();
    auto addAll = [&](Value input) -> LogicalResult {
      auto leaves = paths(input.getType(), bindings, op);
      if (failed(leaves))
        return failure();
      for (auto path : *leaves)
        out.push_back({input, path});
      return success();
    };
    if (isa<BitsConstantOp, BitsConstant4Op, EnumCreateOp, SourceStateOp>(op))
      return out;
    if (isa<EnumToBitsOp, EnumFromBitsOp>(op)) {
      out.push_back({op->getOperand(0), {}});
      return out;
    }
    if (auto get = dyn_cast<StructGetOp>(op)) {
      FieldPath path{get.getFieldAttr()};
      llvm::append_range(path, endpoint.path);
      out.push_back({get.getValue(), path});
      return out;
    }
    if (auto create = dyn_cast<StructCreateOp>(op)) {
      if (endpoint.path.empty())
        return create.emitOpError() << "struct dependency requires leaf path";
      auto decl = analysis.lookupStruct(create.getResult().getType());
      for (auto [index, raw] : llvm::enumerate(decl.getFields()))
        if (cast<DictionaryAttr>(raw).getAs<StringAttr>("name") ==
            endpoint.path.front()) {
          out.push_back(
              {create.getValues()[index],
               FieldPath(endpoint.path.begin() + 1, endpoint.path.end())});
          return out;
        }
      return create.emitOpError() << "unknown struct dependency field";
    }
    if (auto rule = dyn_cast<RuleOp>(op)) {
      auto yield = cast<YieldOp>(rule.getBody().front().back());
      out.push_back({yield.getValues()[cast<OpResult>(value).getResultNumber()],
                     endpoint.path});
      return out;
    }
    if (auto queue = dyn_cast<InstanceOp>(op); queue && isQueueInstance(queue)) {
      if (failed(analysis.verifyQueueOperation(queue, bindings)))
        return failure();
      // Committed capacity is a state read even for downstream-pop queues.
      // Only physical ready follows the current accepted pop request. The
      // outer walk still checks every Queue input for genuine cycles.
      if (value == queue.getOutputs()[3])
        return out;
      auto config = getQueueConfiguration(queue);
      if (failed(config))
        return failure();
      if (value == queue.getOutputs()[0] &&
          config->getAs<StringAttr>("ready_policy").getValue() ==
              "downstream_pop")
        out.push_back({queue.getInputs()[4], {}});
      return out;
    }
    if (isa<InstanceOp, CollectionOp>(op)) {
      Operation *instance = op;
      auto binding =
          isa<InstanceOp>(op)
              ? analysis.bindInstance(cast<InstanceOp>(op), bindings)
              : analysis.bindInstance(cast<CollectionOp>(op), bindings);
      if (failed(binding))
        return failure();
      SmallVector<OutputDependency> importedEntries;
      ArrayRef<OutputDependency> entries;
      if (auto imported = dyn_cast<ModuleImportOp>(binding->owner)) {
        auto deps = summary(imported, *binding);
        if (failed(deps))
          return failure();
        importedEntries = std::move(*deps);
        entries = importedEntries;
      } else {
        auto module = cast<ModuleOp>(binding->owner);
        if (active.contains(module))
          return module.emitOpError("recursive hardware instance hierarchy");
        const ModuleAnalysis *proof = nullptr;
        for (const auto &cached : completed) {
          if (failed(charge(
                  1 + detail::SourceCheckBudget::bindings(cached.bindings) +
                      detail::SourceCheckBudget::bindings(*binding),
                  module)))
            return failure();
          if (cached.module == module &&
              sameBindings(cached.bindings, *binding)) {
            proof = &cached.result;
            break;
          }
        }
        if (!proof) {
          auto nested = analyze(module, *binding);
          if (failed(nested))
            return failure();
          proof = &completed.back().result;
        }
        // Read the completed immutable summary in place. A field lookup must
        // not copy the callee's entire schedule and every other field summary.
        // No recursive analysis occurs while this ArrayRef is being consumed.
        entries = proof->dependencies;
      }
      unsigned port = cast<OpResult>(value).getResultNumber();
      for (const auto &entry : entries) {
        if (failed(charge(1 + endpoint.path.size(), instance)))
          return failure();
        if (entry.output.port == port && entry.output.path == endpoint.path) {
          for (const auto &input : entry.inputs) {
            if (failed(charge(
                    detail::SourceCheckBudget::bytes(sizeof(WireEndpoint)) +
                        input.path.size(),
                    instance)))
              return failure();
            out.push_back({instance->getOperand(input.port), input.path});
          }
          return out;
        }
      }
      return instance->emitOpError() << "missing resolved field dependency";
    }
    if (auto map = dyn_cast<TableMapOp>(op)) {
      auto yield = cast<YieldOp>(map.getBody().front().back());
      out.push_back({yield.getValues()[cast<OpResult>(value).getResultNumber()],
                     endpoint.path});
      return out;
    }
    if (auto match = dyn_cast<TableMatchOp>(op)) {
      auto yield = cast<YieldOp>(match.getBody().front().back());
      out.push_back({yield.getValues()[0], {}});
      return out;
    }
    if (auto merge = dyn_cast<ValueMergeOp>(op)) {
      if (cast<OpResult>(value).getResultNumber() == 1) {
        for (Value guard : merge.getGuards())
          out.push_back({guard, {}});
        return out;
      }
      out.push_back({merge.getBase(), endpoint.path});
      for (auto [i, raw] : llvm::enumerate(merge.getPaths())) {
        FieldPath path;
        for (auto n : cast<ArrayAttr>(raw))
          path.push_back(cast<StringAttr>(n));
        if (prefix(path, endpoint.path)) {
          out.push_back({merge.getGuards()[i], {}});
          out.push_back({merge.getValues()[i],
                         FieldPath(endpoint.path.begin() + path.size(),
                                   endpoint.path.end())});
        }
      }
      return out;
    }
    if (auto create = dyn_cast<TableCreateOp>(op)) {
      for (Value input : create.getInputs())
        out.push_back({input, endpoint.path});
      return out;
    }
    if (auto splat = dyn_cast<TableSplatOp>(op)) {
      out.push_back({splat.getInput(), endpoint.path});
      return out;
    }
    if (auto view = dyn_cast<TableViewOp>(op)) {
      out.push_back({view.getInput(), endpoint.path});
      return out;
    }
    if (auto read = dyn_cast<TableReadOp>(op)) {
      out.push_back({read.getInput(), endpoint.path});
      out.push_back({read.getIndex(), {}});
      return out;
    }
    if (auto get = dyn_cast<TableGetOp>(op)) {
      if (cast<OpResult>(value).getResultNumber() == 0)
        out.push_back({get.getInput(), endpoint.path});
      out.push_back({get.getIndex(), {}});
      return out;
    }
    if (auto index = dyn_cast<TableIndexOp>(op)) {
      for (Value coord : index.getCoords())
        out.push_back({coord, {}});
      return out;
    }
    if (auto choose = dyn_cast<TableChooseOp>(op)) {
      out.push_back({choose.getMask(), {}});
      return out;
    }
    if (auto fold = dyn_cast<TableFoldOp>(op)) {
      out.push_back({fold.getInput(), {}});
      return out;
    }
    if (auto require = dyn_cast<ValueRequireOp>(op)) {
      out.push_back({require.getAvailability(), {}});
      out.push_back({require.getPath(), {}});
      return out;
    }
    if (!isa<BitsUnaryOp, BitsBinaryOp, BitsCompareOp, BitsSelectOp,
             BitsConcatOp, BitsExtractOp, BitsResizeOp>(op))
      return op->emitOpError() << "operation outside closed hardware graph";
    for (Value input : op->getOperands())
      if (failed(addAll(input)))
        return failure();
    return out;
  }
  FailureOr<ModuleAnalysis> analyze(ModuleOp module,
                                    const HardwareBindings &bindings) {
    if (!active.insert(module).second)
      return module.emitOpError() << "recursive hardware instance hierarchy";
    llvm::scope_exit cleanup([&] { active.erase(module); });
    for (const auto &cached : completed) {
      if (failed(charge(
              detail::SourceCheckBudget::add(
                  1, detail::SourceCheckBudget::add(
                         detail::SourceCheckBudget::bindings(bindings),
                         detail::SourceCheckBudget::bindings(cached.bindings))),
              module)))
        return failure();
      if (cached.module == module && sameBindings(cached.bindings, bindings)) {
        if (failed(charge(resultCost(cached.result), module)))
          return failure();
        return cached.result;
      }
    }
    // Only completed exact-binding proofs may skip declaration resolution in
    // this immutable invocation. Every miss authenticates all observation
    // kinds.
    SmallVector<LocalReport, 0> reports;
    for (auto observe : module.getBody().front().getOps<SourceObserveOp>()) {
      auto owner = detail::resolveObservationOwner(
          analysis, observe, bindings,
          [&](uint64_t units, Operation *site) { return charge(units, site); });
      if (failed(owner))
        return failure();
      if (observe.getKind() != "report")
        continue;
      auto name = observe.getSpec().getAs<StringAttr>("name");
      for (const auto &prior : reports) {
        if (failed(charge(
                detail::SourceCheckBudget::add(
                    2, std::max(prior.ownerPath.size(), owner->path.size())),
                observe)))
          return failure();
        if (prior.name == name && ArrayRef<InstanceOp>(prior.ownerPath) ==
                                      ArrayRef<InstanceOp>(owner->path))
          return observe.emitOpError()
                 << "duplicate report name at logical owner";
      }
      if (failed(charge(
              detail::SourceCheckBudget::add(
                  detail::SourceCheckBudget::bytes(sizeof(LocalReport)),
                  detail::SourceCheckBudget::multiply(
                      owner->path.size(),
                      detail::SourceCheckBudget::bytes(sizeof(InstanceOp)))),
              observe)))
        return failure();
      reports.push_back(
          {observe, name,
           SmallVector<InstanceOp, 0>(owner->path.begin(), owner->path.end())});
    }
    if (failed(charge(detail::SourceCheckBudget::add(
                          1, detail::SourceCheckBudget::bindings(bindings)),
                      module)))
      return failure();
    ModuleAnalysis result;
    struct StoredWire {
      Value value;
      SmallVector<StringAttr, 0> path;
      bool matches(const WireEndpoint &other) const {
        return value == other.value &&
               ArrayRef<StringAttr>(path) == ArrayRef<StringAttr>(other.path);
      }
    };
    SmallVector<StoredWire> visiting, done;
    // A wire endpoint is a Value plus its field path. Index candidate paths by
    // Value, retaining vector order for schedules and the original cycle span.
    using EndpointKey = std::pair<Value, ArrayAttr>;
    llvm::DenseMap<EndpointKey, size_t> doneEndpoints;
    llvm::DenseMap<Value, SmallVector<size_t, 0>> visitingForValue;
    auto reserveEndpoint = [&](auto &map) -> LogicalResult {
      using Budget = detail::SourceCheckBudget;
      using Bucket =
          typename std::remove_reference_t<decltype(map)>::value_type;
      uint64_t old = map.getMemorySize() / sizeof(Bucket),
               capacity = std::max(uint64_t(64), old);
      while ((map.size() + 1) * 4 >= capacity * 3) {
        if (failed(charge(1, module)))
          return failure();
        capacity = Budget::multiply(capacity, 2);
      }
      if (failed(charge(2, module)) ||
          (capacity > old &&
           failed(charge(Budget::add(old, Budget::bytes(Budget::multiply(
                                              capacity - old, sizeof(Bucket)))),
                         module))))
        return failure();
      map.reserve(map.size() + 1);
      return success();
    };
    using InputIndices = SmallVector<size_t, 0>;
    SmallVector<PortEndpoint> inputEndpoints;
    // Each distinct module input field is interned once. Per-wire dependency
    // caches carry only its index, rather than copying the full field path at
    // every computation and cache lookup.
    SmallVector<InputIndices> inputsForDone;
    llvm::DenseSet<Operation *> scheduled;
    std::function<FailureOr<InputIndices>(WireEndpoint)> visit;
    visit = [&](WireEndpoint e) -> FailureOr<InputIndices> {
      if (failed(charge(1, module)))
        return failure();
      if (failed(charge(4 + 3 * e.path.size(), module)))
        return failure();
      SmallVector<Attribute> path(e.path.begin(), e.path.end());
      EndpointKey key{e.value, ArrayAttr::get(module.getContext(), path)};
      if (auto found = doneEndpoints.find(key); found != doneEndpoints.end()) {
        const auto &ports = inputsForDone[found->second];
        if (failed(charge(
                detail::SourceCheckBudget::bytes(sizeof(InputIndices) +
                                                 ports.size() * sizeof(size_t)),
                module)))
          return failure();
        return ports;
      }
      auto cycle = visiting.end();
      if (auto found = visitingForValue.find(e.value);
          found != visitingForValue.end()) {
        for (size_t index : found->second) {
          if (failed(charge(1 + e.path.size(), module)))
            return failure();
          if (visiting[index].matches(e)) {
            cycle = visiting.begin() + index;
            break;
          }
        }
      }
      if (cycle != visiting.end()) {
        bool tableSummary =
            llvm::any_of(llvm::make_range(cycle, visiting.end()),
                         [](const StoredWire &node) {
                           return isa<TableType>(node.value.getType());
                         });
        return module.emitOpError()
               << (tableSummary
                       ? "cannot prove absence of combinational cycle "
                         "in compact table field dependencies"
                       : "combinational cycle in hardware field dependencies");
      }
      if (failed(
              charge(detail::SourceCheckBudget::add(
                         detail::SourceCheckBudget::bytes(sizeof(StoredWire)),
                         e.path.size()),
                     module)))
        return failure();
      if (failed(charge(detail::SourceCheckBudget::bytes(
                            sizeof(std::pair<Value, SmallVector<size_t, 0>>) +
                            sizeof(size_t)),
                        module)))
        return failure();
      visitingForValue[e.value].push_back(visiting.size());
      visiting.push_back(
          {e.value, SmallVector<StringAttr, 0>(e.path.begin(), e.path.end())});
      llvm::scope_exit pop([&] {
        visiting.pop_back();
        auto found = visitingForValue.find(e.value);
        found->second.pop_back();
        if (found->second.empty())
          visitingForValue.erase(found);
      });
      InputIndices ports;
      std::optional<llvm::DenseSet<size_t>> portMembership;
      if (auto argument = dyn_cast<BlockArgument>(e.value);
          argument && argument.getOwner() == &module.getBody().front()) {
        if (failed(charge(detail::SourceCheckBudget::add(
                              detail::SourceCheckBudget::bytes(
                                  sizeof(PortEndpoint) + sizeof(size_t)),
                              e.path.size()),
                          module)))
          return failure();
        // The done endpoint cache already guarantees uniqueness of this
        // (argument Value, field path) pair within this module invocation.
        ports.push_back(inputEndpoints.size());
        inputEndpoints.push_back({argument.getArgNumber(), e.path});
      } else {
        auto deps = dependencies(e, bindings);
        if (failed(deps))
          return failure();
        for (auto dependency : *deps) {
          auto nested = visit(dependency);
          if (failed(nested))
            return failure();
          for (size_t p : *nested) {
            bool present;
            if (ports.size() < 32) {
              if (failed(charge(ports.size() + 1, module)))
                return failure();
              present = llvm::is_contained(ports, p);
            } else {
              // Keep first-encounter order for canonical dependency summaries,
              // but avoid a linear scan for every repeated transitive input.
              if (!portMembership) {
                if (failed(charge(16 + 128 + ports.size(), module)))
                  return failure();
                portMembership.emplace();
                portMembership->reserve(ports.size() + 1);
                portMembership->insert(ports.begin(), ports.end());
              }
              if (failed(charge(2, module)))
                return failure();
              present = portMembership->contains(p);
              if (!present) {
                uint64_t bytes = portMembership->getMemorySize();
                uint64_t capacity = bytes / sizeof(size_t);
                if ((portMembership->size() + 1) * 4 >= capacity * 3 &&
                    failed(
                        charge(detail::SourceCheckBudget::add(
                                   capacity,
                                   detail::SourceCheckBudget::bytes(bytes * 2)),
                               module)))
                  return failure();
                portMembership->insert(p);
              }
            }
            if (!present) {
              if (failed(
                      charge(detail::SourceCheckBudget::bytes(sizeof(size_t)),
                             module)))
                return failure();
              ports.push_back(p);
            }
          }
        }
        if (Operation *op = e.value.getDefiningOp(); op && !validationOnly) {
          if (failed(
                  charge(detail::SourceCheckBudget::add(
                             detail::SourceCheckBudget::bytes(sizeof(WireStep)),
                             e.path.size() + 1),
                         module)))
            return failure();
          result.wireSchedule.push_back({e, op});
          if (scheduled.insert(op).second)
            result.schedule.push_back(op);
        }
      }
      uint64_t cacheCost = detail::SourceCheckBudget::add(
          detail::SourceCheckBudget::add(
              detail::SourceCheckBudget::bytes(sizeof(StoredWire)),
              e.path.size()),
          detail::SourceCheckBudget::bytes(sizeof(InputIndices) +
                                           ports.size() * sizeof(size_t)));
      if (failed(charge(detail::SourceCheckBudget::multiply(2, cacheCost),
                        module)))
        return failure();
      if (failed(reserveEndpoint(doneEndpoints)))
        return failure();
      doneEndpoints[key] = done.size();
      done.push_back(
          {e.value, SmallVector<StringAttr, 0>(e.path.begin(), e.path.end())});
      inputsForDone.push_back(ports);
      return ports;
    };
    auto yield = cast<YieldOp>(module.getBody().front().back());
    for (auto [port, value] : llvm::enumerate(yield.getValues())) {
      auto leaves = paths(value.getType(), bindings, module);
      if (failed(leaves))
        return failure();
      for (auto path : *leaves) {
        auto inputs = visit({value, path});
        if (failed(inputs))
          return failure();
        if (failed(charge(
                detail::SourceCheckBudget::add(
                    detail::SourceCheckBudget::bytes(sizeof(OutputDependency)),
                    path.size()),
                module)))
          return failure();
        OutputDependency dependency;
        dependency.output = {static_cast<unsigned>(port), path};
        for (size_t index : *inputs) {
          const auto &input = inputEndpoints[index];
          if (failed(charge(
                  detail::SourceCheckBudget::add(
                      detail::SourceCheckBudget::bytes(sizeof(PortEndpoint)),
                      input.path.size()),
                  module)))
            return failure();
          dependency.inputs.push_back(input);
        }
        result.dependencies.push_back(std::move(dependency));
      }
    }
    // Check unobserved computations and every storage input too. Dead cycles
    // are still invalid hardware; a temporal cut only applies at Q outputs.
    for (Operation &op : module.getBody().front().without_terminator()) {
      if (failed(charge(1, &op)))
        return failure();
      if (isa<SourceObserveOp, SourceExpectOp, StateUpdateOp>(op)) {
        for (auto operand : op.getOperands()) {
          auto leaves = paths(operand.getType(), bindings, &op);
          if (failed(leaves))
            return failure();
          for (auto path : *leaves)
            if (failed(visit({operand, path})))
              return failure();
        }
        if (!validationOnly)
          result.schedule.push_back(&op);
        continue;
      }
      for (Value value : op.getResults()) {
        auto leaves = paths(value.getType(), bindings, &op);
        if (failed(leaves))
          return failure();
        for (auto path : *leaves)
          if (failed(visit({value, path})))
            return failure();
      }
      if (auto logical = dyn_cast<SourceStateOp>(op)) {
        for (Value input : logical->getOperands()) {
          auto leaves = paths(input.getType(), bindings, logical);
          if (failed(leaves)) return failure();
          for (auto path : *leaves)
            if (failed(visit({input, path}))) return failure();
        }
      }
      if (validationOnly && op.getNumRegions()) {
        // A dead operation within a normalized Rule/Table body must not hide a
        // cycle simply because no yield or update reads its result.
        auto nested = op.walk([&](Operation *inner) {
          if (inner == &op) return WalkResult::advance();
          if (failed(charge(1, inner))) return WalkResult::interrupt();
          for (Value value : inner->getResults()) {
            auto leaves = paths(value.getType(), bindings, inner);
            if (failed(leaves)) return WalkResult::interrupt();
            for (auto path : *leaves)
              if (failed(visit({value, path}))) return WalkResult::interrupt();
          }
          return WalkResult::advance();
        });
        if (nested.wasInterrupted()) return failure();
      }
      if (isa<InstanceOp, CollectionOp>(op)) {
        Operation *instance = &op;
        auto binding =
            isa<InstanceOp>(op)
                ? analysis.bindInstance(cast<InstanceOp>(op), bindings)
                : analysis.bindInstance(cast<CollectionOp>(op), bindings);
        if (failed(binding))
          return failure();
        // Result-free instances still carry their complete semantic closure.
        // Reuse this engine's cache and recursion budget instead of relying on
        // output wire traversal to discover a transaction import below them.
        if (auto child = dyn_cast<ModuleOp>(binding->owner)) {
          if (failed(analyze(child, *binding)))
            return failure();
        } else if (auto imported = dyn_cast<ModuleImportOp>(binding->owner)) {
          if (failed(summary(imported, *binding)))
            return failure();
        }
        if (failed(isa<InstanceOp>(op)
                       ? analysis.verifyInstance(cast<InstanceOp>(op), bindings)
                       : analysis.verifyInstance(cast<CollectionOp>(op),
                                                 bindings)))
          return failure();
        for (auto input : instance->getOperands()) {
          auto leaves = paths(input.getType(), bindings, instance);
          if (failed(leaves))
            return failure();
          for (auto path : *leaves)
            if (failed(visit({input, path})))
              return failure();
        }
        // A result-free occurrence can still own checks or state. Retain its
        // Work after preparing inputs even when no output wire roots it.
        if (!validationOnly && scheduled.insert(instance).second)
          result.schedule.push_back(instance);
        auto primitive = analysis.getPrimitiveKind(binding->owner);
        if (!primitive.empty() && !validationOnly)
          result.storageWork.push_back(&op);

      }
    }
    // A local downward owner can collide only with a direct report declared
    // at one of its ancestor-prefix children. Child proofs already establish
    // their complete subtrees, including each Collection once per bindings.
    for (const auto &report : reports) {
      if (failed(charge(detail::SourceCheckBudget::bindings(bindings),
                        report.observe)))
        return failure();
      HardwareBindings bound = bindings;
      for (size_t index = 0; index < report.ownerPath.size(); ++index) {
        auto instance = report.ownerPath[index];
        if (failed(charge(detail::SourceCheckBudget::add(
                              1, detail::SourceCheckBudget::bindings(bound)),
                          instance)))
          return failure();
        auto childBindings = analysis.bindInstance(instance, bound);
        if (failed(childBindings))
          return failure();
        auto child = dyn_cast<ModuleOp>(childBindings->owner);
        if (!child)
          return report.observe->emitOpError()
                 << "report child definition is unresolved";
        const CompletedModule *proof = nullptr;
        for (const auto &cached : completed) {
          if (failed(charge(detail::SourceCheckBudget::add(
                                2, detail::SourceCheckBudget::add(
                                       detail::SourceCheckBudget::bindings(
                                           *childBindings),
                                       detail::SourceCheckBudget::bindings(
                                           cached.bindings))),
                            instance)))
            return failure();
          if (cached.module == child && sameBindings(cached.bindings, *childBindings)) {
            proof = &cached;
            break;
          }
        }
        if (!proof)
          return report.observe->emitOpError()
                 << "report child proof is incomplete";
        ArrayRef<InstanceOp> suffix(report.ownerPath.data() + index + 1,
                                    report.ownerPath.size() - index - 1);
        for (const auto &local : proof->reports) {
          if (failed(charge(
                  detail::SourceCheckBudget::add(
                      2, std::max(local.ownerPath.size(), suffix.size())),
                  report.observe)))
            return failure();
          if (local.name == report.name &&
              ArrayRef<InstanceOp>(local.ownerPath) == suffix)
            return report.observe->emitOpError()
                   << "duplicate report name at logical owner across instances";
        }
        bound = std::move(*childBindings);
      }
    }
    if (failed(charge(
            detail::SourceCheckBudget::add(
                detail::SourceCheckBudget::bindings(bindings),
                detail::SourceCheckBudget::multiply(2, resultCost(result))),
            module)))
      return failure();
    if (failed(charge(detail::SourceCheckBudget::bytes(sizeof(CompletedModule)),
                      module)))
      return failure();
    completed.push_back(
        {module, bindings, result, std::move(reports)});
    return result;
  }
};
} // namespace
FailureOr<SmallVector<FieldPath>> HardwareAnalysis::getFieldPaths(
    Type type, const HardwareBindings &bindings, Operation *site,
    const std::function<LogicalResult(uint64_t, Operation *)> &debit) const {
  Engine engine{*this, {}};
  engine.debit = debit;
  return engine.paths(type, bindings, site);
}
FailureOr<SmallVector<OutputDependency>>
HardwareAnalysis::getImportDependencies(
    ModuleImportOp imported, const HardwareBindings &bindings) const {
  Engine engine{*this, {}};
  return engine.summary(imported, bindings);
}
FailureOr<SmallVector<WireEndpoint>> HardwareAnalysis::getDependencies(
    Value value, ArrayRef<StringAttr> path, const HardwareBindings &bindings,
    const std::function<LogicalResult(uint64_t, Operation *)> &debit) const {
  Engine engine{*this, {}};
  engine.debit = debit;
  return engine.dependencies({value, FieldPath(path.begin(), path.end())},
                             bindings);
}
FailureOr<ModuleAnalysis>
HardwareAnalysis::analyzeModule(ModuleOp module,
                                const HardwareBindings &bindings) const {
  Engine engine{*this, {}};
  return engine.analyze(module, bindings);
}
namespace detail {
LogicalResult verifySourceCheckDependencies(
    const HardwareAnalysis &analysis,
    ArrayRef<std::pair<ModuleOp, HardwareBindings>> scopes,
    const std::function<LogicalResult(uint64_t, Operation *)> &debit) {
  // Byte memory's pure committed read follows the verified raddr dependency.
  // Every input is still visited; write pins are sampled once after pure SSA.
  Engine combinational{analysis, {}};
  combinational.validationOnly = true;
  combinational.debit = debit;
  for (const auto &[module, bindings] : scopes)
    if (failed(combinational.analyze(module, bindings)))
      return failure();
  return success();
}
} // namespace detail
} // namespace acir::ac
