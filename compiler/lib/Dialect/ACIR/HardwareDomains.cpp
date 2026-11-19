#include "pycircuit/Dialect/ACIR/HardwareDomains.h"
#include "HardwareSourceCheckPreflight.h"
#include "HardwareSourceChecks.h"
#include "HardwareSampledAdmission.h"
#include "mlir/IR/Builders.h"
#include "pycircuit/Dialect/ACIR/HardwareCDC.h"
#include "pycircuit/Interfaces/LogicalStateInterfaces.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include <map>
#include <memory>
#include <set>
#include <tuple>
#include <vector>
using namespace mlir;
namespace acir::ac {
namespace {
using Domains = std::set<unsigned>;
class Resolver {
  HardwareAnalysis analysis;
  detail::SourceCheckBudget budget;
  HardwareDomainPlan plan;
  bool sourceInference = false;
  unsigned nextInference = ~0u;
  struct Inference { Operation *resource; unsigned occurrence; bool source; Domains constraints; };
  std::map<unsigned, Inference> inferences;
  std::map<std::tuple<Operation *, unsigned, bool>, unsigned> inferenceIds;
  DenseSet<Operation *> active;
  struct Scope {
    unsigned occurrence;
    HardwareBindings bindings;
    DenseMap<Value, unsigned> clockValues;
    DenseMap<Value, Domains> values, controlValues;
    Scope *parent = nullptr;
    DenseMap<Operation *, unsigned> children;
    DenseSet<Value> masks, cuts;
    DenseSet<Value> controls, visiting;
  };
  std::vector<std::unique_ptr<Scope>> scopes;
  LogicalResult reject(Operation *op, StringRef reason) {
    return op->emitOpError() << "logical domain: " << reason;
  }
  FailureOr<unsigned> forwardedInput(Value value, unsigned depth = 0) {
    if (depth > 128)
      return failure();
    if (auto argument = dyn_cast<BlockArgument>(value)) {
      if (isa<ModuleOp>(argument.getOwner()->getParentOp()))
        return argument.getArgNumber();
      if (auto rule = dyn_cast<RuleOp>(argument.getOwner()->getParentOp()))
        return forwardedInput(rule.getCaptures()[argument.getArgNumber()],
                              depth + 1);
    }
    if (auto splat = value.getDefiningOp<TableSplatOp>())
      return forwardedInput(splat.getInput(), depth + 1);
    if (auto view = value.getDefiningOp<TableViewOp>())
      return forwardedInput(view.getInput(), depth + 1);
    if (auto table = value.getDefiningOp<TableCreateOp>()) {
      std::optional<unsigned> selected;
      for (auto input : table->getOperands()) {
        auto result = forwardedInput(input, depth + 1);
        if (failed(result))
          return failure();
        if (selected && *selected != *result)
          return failure();
        selected = *result;
      }
      if (selected)
        return *selected;
      return failure();
    }
    if (auto result = dyn_cast<OpResult>(value)) {
      if (auto rule = dyn_cast<RuleOp>(result.getOwner()))
        return forwardedInput(cast<YieldOp>(rule.getBody().front().back())
                                  .getOperand(result.getResultNumber()),
                              depth + 1);
      if (auto instance = dyn_cast<InstanceOp>(result.getOwner())) {
        auto child =
            dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(instance));
        if (!child)
          return failure();
        auto input =
            forwardedInput(cast<YieldOp>(child.getBody().front().back())
                               .getOperand(result.getResultNumber()),
                           depth + 1);
        if (failed(input) || *input >= instance.getInputs().size())
          return failure();
        return forwardedInput(instance.getInputs()[*input], depth + 1);
      }
    }
    return failure();
  }
  FailureOr<unsigned> clock(Value value, Scope &scope) {
    if (auto it = scope.clockValues.find(value); it != scope.clockValues.end())
      return it->second;
    if (auto argument = dyn_cast<BlockArgument>(value)) {
      if (auto rule = dyn_cast<RuleOp>(argument.getOwner()->getParentOp()))
        return clock(rule.getCaptures()[argument.getArgNumber()], scope);
    }
    if (auto splat = value.getDefiningOp<TableSplatOp>())
      return clock(splat.getInput(), scope);
    if (auto table = value.getDefiningOp<TableCreateOp>()) {
      std::optional<unsigned> identity;
      for (auto input : table->getOperands()) {
        auto actual = clock(input, scope);
        if (failed(actual))
          return failure();
        if (identity && *identity != *actual)
          return reject(table,
                        "clock collection contains distinct actual sources");
        identity = *actual;
      }
      if (identity)
        return *identity;
      return failure();
    }
    if (auto view = value.getDefiningOp<TableViewOp>()) {
      // A uniform identity remains uniform through every checked static view.
      auto shape = analysis.getTableShape(
          cast<TableType>(view.getResult().getType()), scope.bindings, view);
      if (failed(shape))
        return failure();
      auto ordinal = analysis.getViewSourceOrdinal(view, 0, scope.bindings);
      if (failed(ordinal))
        return failure();
      return clock(view.getInput(), scope);
    }
    if (auto read = value.getDefiningOp<TableReadOp>())
      return clock(read.getInput(), scope);
    if (auto get = value.getDefiningOp<TableGetOp>();
        get && value == get.getValue())
      return clock(get.getInput(), scope);
    if (auto extract = value.getDefiningOp<BitsExtractOp>()) {
      auto inputWidth = analysis.getPackedWidth(extract.getInput().getType(),
                                                scope.bindings, extract);
      auto resultWidth =
          analysis.getPackedWidth(value.getType(), scope.bindings, extract);
      auto low =
          analysis.evaluateStatic(extract.getLow(), scope.bindings, extract);
      auto number =
          succeeded(low) ? dyn_cast<MathIntAttr>(*low) : MathIntAttr();
      if (succeeded(inputWidth) && succeeded(resultWidth) && *inputWidth == 1 &&
          *resultWidth == 1 && number && number.getCanonicalValue() == "0")
        return clock(extract.getInput(), scope);
    }
    if (auto result = dyn_cast<OpResult>(value))
      if (auto instance = dyn_cast<InstanceOp>(result.getOwner())) {
        auto child =
            dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(instance));
        if (child) {
          auto input =
              forwardedInput(cast<YieldOp>(child.getBody().front().back())
                                 .getOperand(result.getResultNumber()));
          if (succeeded(input) && *input < instance.getInputs().size())
            return clock(instance.getInputs()[*input], scope);
        }
      }
    if (auto result = dyn_cast<OpResult>(value))
      if (auto rule = dyn_cast<RuleOp>(result.getOwner())) {
        auto yield = cast<YieldOp>(rule.getBody().front().back());
        return clock(yield.getOperand(result.getResultNumber()), scope);
      }
    return reject(value.getParentBlock()->getParentOp(),
                  "clock must forward an actual root clock argument");
  }
  FailureOr<unsigned> domain(Operation *op, StringRef field, Scope &scope) {
    bool source = field == "source_domain" || field == "write_domain";
    if (sourceInference && isCDCInstance(dyn_cast<InstanceOp>(op)) &&
        op->hasAttr(source ? "source_infer_source" : "source_infer_target")) {
      auto key = std::make_tuple(op, scope.occurrence, source);
      auto found = inferenceIds.find(key);
      if (found != inferenceIds.end()) return found->second;
      unsigned token = nextInference--;
      inferenceIds[key] = token;
      inferences.try_emplace(token, Inference{op, scope.occurrence, source, {}});
      return token;
    }
    auto name = op->getAttrOfType<StringAttr>(field);
    auto &clocks = plan.occurrences[scope.occurrence].clocks;
    auto found = name ? clocks.find(name.getValue()) : clocks.end();
    if (found == clocks.end())
      return reject(op, "unbound endpoint domain");
    return found->second;
  }
  bool transactionControlResult(RuleOp rule, unsigned result) {
    auto sampled = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    auto fire =
        sampled ? sampled.getAs<IntegerAttr>("fire_result") : IntegerAttr();
    if (fire && fire.getInt() == result)
      return true;
    auto module = rule->getParentOfType<ModuleOp>();
    auto wiring =
        module->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
    auto writers = wiring ? wiring.getAs<ArrayAttr>("writers") : ArrayAttr();
    if (writers)
      for (auto raw : writers) {
        auto writer = dyn_cast<DictionaryAttr>(raw);
        if (!writer || writer.get("registration") != rule.getOccurrence())
          continue;
        if (auto candidates = writer.getAs<ArrayAttr>("candidates"))
          for (auto candidate : candidates) {
            auto row = dyn_cast<DictionaryAttr>(candidate);
            auto enable =
                row ? row.getAs<IntegerAttr>("enable") : IntegerAttr();
            if (enable && enable.getInt() == result)
              return true;
          }
      }
    return false;
  }
  FailureOr<Domains> dependencies(Value value, Scope &scope,
                                  bool control = false) {
    if (failed(budget.debit(8, value.getParentBlock()->getParentOp())))
      return failure();
    if (control && scope.masks.contains(value))
      return Domains{};
    auto &memo = control ? scope.controlValues : scope.values;
    if (control && scope.cuts.contains(value))
      return scope.values.lookup(value);
    if (auto it = memo.find(value); it != memo.end())
      return it->second;
    if (!scope.visiting.insert(value).second)
      return reject(value.getParentBlock()->getParentOp(),
                    "combinational dependency cycle");
    Domains result;
    auto merge = [&](Value input, bool guard = false) -> LogicalResult {
      auto found = dependencies(input, scope, control || guard);
      if (failed(found))
        return failure();
      result.insert(found->begin(), found->end());
      return success();
    };
    if (auto arg = dyn_cast<BlockArgument>(value)) {
      if (isa<ModuleOp>(arg.getOwner()->getParentOp())) {
        auto &occurrence = plan.occurrences[scope.occurrence];
        if (scope.parent && occurrence.allocation) {
          auto input = occurrence.allocation->getOperand(arg.getArgNumber());
          auto found = dependencies(input, *scope.parent, control);
          if (failed(found))
            return failure();
          result = *found;
        } else
          return reject(arg.getOwner()->getParentOp(),
                        "ordinary root input has no domain owner");
      } else if (auto rule = dyn_cast<RuleOp>(arg.getOwner()->getParentOp())) {
        if (failed(merge(rule.getCaptures()[arg.getArgNumber()]))) return failure();
      } else if (auto transaction = dyn_cast<SourceTransactionOp>(arg.getOwner()->getParentOp())) {
        if (failed(merge(transaction.getCaptures()[arg.getArgNumber()]))) return failure();
      } else if (auto method = dyn_cast<SourceMethodOp>(arg.getOwner()->getParentOp())) {
        unsigned parameters = method.getFunctionType().getNumInputs();
        if (arg.getArgNumber() >= parameters) {
          if (failed(merge(method.getCaptures()[arg.getArgNumber() - parameters]))) return failure();
        } else {
          auto module = plan.occurrences[scope.occurrence].definition;
          auto actual = domain(module, "ac.default_domain", scope);
          if (failed(actual)) return failure();
          result.insert(*actual);
        }
      }
    } else {
      Operation *op = value.getDefiningOp();
      if (auto state = dyn_cast<SourceStateOp>(op)) {
        auto actual = clock(state.getClk(), scope);
        if (failed(actual)) return failure();
        result.insert(*actual);
      } else if (auto transaction = dyn_cast<SourceTransactionOp>(op)) {
        auto actual = clock(transaction.getClk(), scope);
        if (failed(actual)) return failure();
        result.insert(*actual);
      } else if (auto state = dyn_cast<acir::LogicalStateOpInterface>(op)) {
        auto d = domain(state, "domain", scope);
        if (failed(d))
          return failure();
        result.insert(*d);
      } else if (auto instance = dyn_cast<InstanceOp>(op); isCDCInstance(instance)) {
        auto layout = getCDCPrimitiveLayout(instance, scope.bindings);
        if (failed(layout)) return failure();
        auto d = domain(op, layout->isFIFO()
              ? (value == layout->writeAvailable() ? "write_domain" : "read_domain")
              : "target_domain", scope);
        if (failed(d)) return failure();
        result.insert(*d);
      } else if (isa<InstanceOp, CollectionOp>(op) &&
                 scope.children.count(op)) {
        auto &child = *scopes[scope.children.lookup(op)];
        auto definition = plan.occurrences[child.occurrence].definition;
        auto yield = cast<YieldOp>(definition.getBody().front().back());
        auto found = dependencies(
            yield.getOperand(cast<OpResult>(value).getResultNumber()), child,
            control);
        if (failed(found))
          return failure();
        result = *found;
      } else if (auto instance = dyn_cast<InstanceOp>(op);
                 instance && isa_and_nonnull<ModuleImportOp>(analysis.resolveCallee(instance)) &&
                 !analysis.resolveCallee(instance)->hasAttr("ac.default_domain")) {
        // Stateless interfaces retain their published output dependencies;
        // an unused input cannot impose a clock domain on an output.
        auto imported = cast<ModuleImportOp>(analysis.resolveCallee(instance));
        auto bound = analysis.bindInstance(instance, scope.bindings);
        if (failed(bound)) return failure();
        auto summary = analysis.getImportDependencies(imported, *bound);
        if (failed(summary)) return failure();
        unsigned output = cast<OpResult>(value).getResultNumber();
        for (const auto &dependency : *summary)
          if (dependency.output.port == output)
            for (const auto &input : dependency.inputs)
              if (failed(merge(instance.getInputs()[input.port]))) return failure();
      } else if (auto rule = dyn_cast<RuleOp>(op)) {
        auto yield = cast<YieldOp>(rule.getBody().front().back());
        unsigned ordinal = cast<OpResult>(value).getResultNumber();
        auto found =
            dependencies(yield.getOperand(ordinal), scope,
                         control && transactionControlResult(rule, ordinal));
        if (failed(found))
          return failure();
        result = *found;
      } else if (auto select = dyn_cast<BitsSelectOp>(op)) {
        if (failed(merge(select.getCondition(), true)) ||
            failed(merge(select.getTrueValue())) || failed(merge(select.getFalseValue())))
          return failure();
      } else if (auto selected = dyn_cast<ValueMergeOp>(op)) {
        if (failed(merge(selected.getBase()))) return failure();
        for (Value guard : selected.getGuards())
          if (failed(merge(guard, true))) return failure();
        for (Value candidate : selected.getValues())
          if (failed(merge(candidate))) return failure();
      } else if (!isa<BitsConstantOp, BitsConstant4Op, EnumCreateOp,
                      ValueZeroOp>(op)) {
        for (auto input : op->getOperands())
          if (failed(merge(input)))
            return failure();
      }
    }
    scope.visiting.erase(value);
    memo[value] = result;
    return result;
  }
  LogicalResult check(Value value, unsigned expected, Scope &scope,
                      Operation *site, bool control = false) {
    auto found = dependencies(value, scope, control);
    if (failed(found))
      return failure();
    if (sourceInference) {
      for (unsigned d : *found) {
        if (inferences.count(expected)) inferences[expected].constraints.insert(d);
        else if (inferences.count(d)) inferences[d].constraints.insert(expected);
        else if (d != expected) return reject(site, "ordinary data/control connection crosses actual clock domains; use an explicit CDC resource");
      }
      return success();
    }
    if (llvm::any_of(*found, [&](unsigned d) { return d != expected; }))
      return reject(site, "ordinary data/control connection crosses actual clock domains; use an explicit CDC resource");
    return success();
  }
  LogicalResult visit(ModuleOp module, HardwareBindings bindings,
                      HardwareDomainOccurrence occurrence,
                      DenseMap<Value, unsigned> clocks, unsigned depth) {
    if (depth >= 128 || !active.insert(module).second)
      return reject(module, "recursive or excessive hierarchy");
    if (failed(budget.debit(128, module)))
      return failure();
    unsigned index = plan.occurrences.size();
    plan.occurrences.push_back(std::move(occurrence));
    scopes.push_back(std::make_unique<Scope>());
    Scope &scope = *scopes.back();
    if (plan.occurrences[index].parent)
      scope.parent = scopes[*plan.occurrences[index].parent].get();
    scope.occurrence = index;
    scope.bindings = std::move(bindings);
    scope.clockValues = std::move(clocks);
    auto ports = getModulePortLayout(module);
    if (failed(ports))
      return failure();
    for (auto &entry : scope.clockValues)
      if (auto argument = dyn_cast<BlockArgument>(entry.first);
          argument && argument.getOwner() == &module.getBody().front()) {
        if (argument.getArgNumber() < ports->inputValidity.size() &&
            ports->inputValidity[argument.getArgNumber()] >= 0)
          return reject(module, "logical clock sources require total availability");
        plan.occurrences[index].inputClocks[argument.getArgNumber()] =
            entry.second;
      }
    auto formals = module->getAttrOfType<ArrayAttr>("ac.domain_params");
    if (formals) {
      std::set<StringRef> unique;
      for (auto raw : formals) {
        auto name = dyn_cast<StringAttr>(raw);
        if (!name || name.getValue().empty() ||
            !unique.insert(name.getValue()).second ||
            !plan.occurrences[index].clocks.count(name.getValue()))
          return reject(module,
                        "domain formals require unique complete bindings");
      }
      if (unique.size() != plan.occurrences[index].clocks.size())
        return reject(module, "extraneous domain binding");
    }
    std::optional<unsigned> defaultDomain;
    if (module->hasAttr("ac.default_domain")) {
      auto d = domain(module, "ac.default_domain", scope);
      if (failed(d))
        return failure();
      defaultDomain = *d;
    }
    for (auto &entry : scope.clockValues)
      scope.controls.insert(entry.first);
    // The compiler-owned transaction reset remains a dedicated control even
    // when storage uses a different reset, or the scope has no local state.
    if (auto inputs = module->getAttrOfType<DictionaryAttr>("ac.domain_inputs"))
      if (auto reset = inputs.getAs<IntegerAttr>("reset")) {
        if (reset.getInt() < 0 || uint64_t(reset.getInt()) >= module.getBody().front().getNumArguments())
          return reject(module, "transaction reset input ordinal is invalid");
        scope.controls.insert(module.getBody().front().getArgument(reset.getInt()));
      }
    // Resets are dedicated logical controls and never ordinary payload inputs.
    for (auto &op : module.getBody().front()) {
      if (auto state = dyn_cast<acir::LogicalStateOpInterface>(op)) {
        if (state.getStateReset())
          scope.controls.insert(state.getStateReset());
      } else if (auto state = dyn_cast<SourceStateOp>(op)) {
        // Source declarations intentionally do not implement the routed state
        // interface. Their reset operand has the same dedicated control role.
        if (state.getRst()) scope.controls.insert(state.getRst());
      }
      if (isCDCInstance(dyn_cast<InstanceOp>(op)) || isa<CDCRefinementOp>(op))
        scope.controls.insert(op.getOperand(2));
    }
    for (auto arg : module.getBody().front().getArguments())
      if (!scope.parent && defaultDomain)
        scope.values[arg] = {*defaultDomain};
    if (!scope.parent) {
      // Named System samples drive external one-bit controls, not data produced
      // by the synthetic default clock. Actual clock columns keep their clock
      // identities through the override below. Ordinary Module inputs retain
      // their declared domain provenance.
      auto sampled = getSystemSampleInputs(module);
      if (failed(sampled)) return failure();
      for (unsigned input : *sampled)
        scope.values[module.getBody().front().getArgument(input)] = {};
    }
    for (auto &entry : scope.clockValues)
      if (!scope.parent)
        scope.values[entry.first] = {entry.second};
    for (auto &op : module.getBody().front()) {
      if (isa<CDCRefinementOp>(op)) {
        bool reg = op.getAttrOfType<StringAttr>("logical_kind").getValue() ==
                   "async_reg";
        auto first = domain(&op, reg ? "target_domain" : "write_domain", scope);
        auto second = domain(&op, reg ? "target_domain" : "read_domain", scope);
        if (failed(first) || failed(second))
          return failure();
        if (reg)
          scope.values[op.getOperand(4)] = {*second};
        else {
          scope.values[op.getOperand(6)] = {*second};
          scope.values[op.getOperand(7)] = {*first};
          scope.values[op.getOperand(8)] = {*second};
        }
      }
      Operation *callee = nullptr;
      if (auto instance = dyn_cast<InstanceOp>(op))
        callee = analysis.resolveCallee(instance);
      if (auto collection = dyn_cast<CollectionOp>(op))
        callee = analysis.resolveCallee(collection);
      if (isCDCInstance(dyn_cast<InstanceOp>(op))) {
        auto layout = getCDCPrimitiveLayout(cast<InstanceOp>(op), scope.bindings);
        if (failed(layout)) return failure();
        auto first = domain(&op, layout->isFIFO() ? "write_domain" : "target_domain", scope);
        auto second = domain(&op, layout->isFIFO() ? "read_domain" : "target_domain", scope);
        if (failed(first) || failed(second)) return failure();
        scope.values[layout->output()] = {*second};
        if (layout->isFIFO()) {
          scope.values[layout->writeAvailable()] = {*first};
          scope.values[layout->readAvailable()] = {*second};
        }
        scope.controls.insert(layout->reset());
      } else if (callee) {
        auto kind = analysis.getPrimitiveKind(callee);
        if (sourceInference && kind.empty() && isa<ModuleImportOp>(callee)) {
          auto defaultName = callee->getAttrOfType<StringAttr>("ac.default_domain");
          auto bindings = op.getAttrOfType<DictionaryAttr>("ac.domain_bindings");
          auto actualName = defaultName && bindings ? bindings.getAs<StringAttr>(defaultName.getValue()) : StringAttr();
          auto &names = plan.occurrences[index].clocks;
          if (defaultName) {
            if (!actualName || !names.count(actualName.getValue())) return reject(&op, "imported output requires its actual declared default domain");
            for (Value output : op.getResults()) scope.values[output] = {names.lookup(actualName.getValue())};
          }
        } else if (!kind.empty()) {
          auto actual = clock(op.getOperand(0), scope);
          if (failed(actual))
            return failure();
          for (auto result : op.getResults())
            scope.values[result] = {*actual};
          scope.controls.insert(op.getOperand(1));
        }
      }
    }
    for (auto &entry : scope.values)
      scope.cuts.insert(entry.first);
    for (auto rule : module.getBody().front().getOps<RuleOp>()) {
      auto mask = getVerifiedTransactionResetMask(rule);
      if (failed(mask))
        return failure();
      if (*mask)
        scope.masks.insert(*mask);
      auto row = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
      if (auto resets = row ? row.getAs<ArrayAttr>("resets") : ArrayAttr())
        for (auto raw : resets) {
          auto reset = dyn_cast<DictionaryAttr>(raw);
          auto capture =
              reset ? reset.getAs<IntegerAttr>("capture") : IntegerAttr();
          if (capture && capture.getInt() >= 0 &&
              uint64_t(capture.getInt()) < rule.getCaptures().size())
            scope.controls.insert(rule.getCaptures()[capture.getInt()]);
        }
    }
    for (auto &op : module.getBody().front()) {
      if (isa<InstanceOp, CollectionOp>(op)) {
        Operation *instance = &op;
        auto *callee = isa<InstanceOp>(op)
                           ? analysis.resolveCallee(cast<InstanceOp>(op))
                           : analysis.resolveCallee(cast<CollectionOp>(op));
        auto child = dyn_cast_or_null<ModuleOp>(callee);
        if (!child)
          continue;
        auto childBindings =
            isa<InstanceOp>(op)
                ? analysis.bindInstance(cast<InstanceOp>(op), scope.bindings)
                : analysis.bindInstance(cast<CollectionOp>(op), scope.bindings);
        if (failed(childBindings))
          return failure();
        HardwareDomainOccurrence next;
        next.definition = child;
        next.allocation = instance;
        next.parent = index;
        next.instancePath = plan.occurrences[index].instancePath;
        next.instancePath.push_back(
            instance->getAttrOfType<DictionaryAttr>("occurrence"));
        auto map =
            instance->getAttrOfType<DictionaryAttr>("ac.domain_bindings");
        if (auto names = child->getAttrOfType<ArrayAttr>("ac.domain_params"))
          for (auto raw : names) {
            auto name = cast<StringAttr>(raw);
            auto target =
                map ? map.getAs<StringAttr>(name.getValue()) : StringAttr();
            if (!target ||
                !plan.occurrences[index].clocks.count(target.getValue()))
              return reject(
                  instance,
                  "child domain bindings must be explicit and complete");
            next.clocks[name.getValue()] =
                plan.occurrences[index].clocks.lookup(target.getValue());
          }
        if (map)
          for (auto row : map) {
            auto target = dyn_cast<StringAttr>(row.getValue());
            if (!next.clocks.count(row.getName().getValue()) || !target ||
                !plan.occurrences[index].clocks.count(target.getValue()))
              return reject(instance,
                            "unknown or malformed child domain binding");
          }
        DenseMap<Value, unsigned> childClocks;
        for (auto [ordinal, input] : llvm::enumerate(instance->getOperands())) {
          auto found = scope.clockValues.find(input);
          if (found != scope.clockValues.end())
            childClocks[child.getBody().front().getArgument(ordinal)] =
                found->second;
          else if (auto forwarded = forwardedInput(input);
                   succeeded(forwarded)) {
            auto original = module.getBody().front().getArgument(*forwarded);
            auto identity = scope.clockValues.find(original);
            if (identity != scope.clockValues.end())
              childClocks[child.getBody().front().getArgument(ordinal)] =
                  identity->second;
          }
        }
        unsigned childIndex = scopes.size();
        scope.children[instance] = childIndex;
        if (failed(visit(child, *childBindings, std::move(next),
                         std::move(childClocks), depth + 1)))
          return failure();
        // A reset/control forwarded through a stateless parent is still a
        // dedicated child pin. Propagate only that boundary role; ordinary
        // data uses retain their actual dependency in scope.values.
        for (auto argument : child.getBody().front().getArguments()) {
          if (failed(budget.debit(1, instance))) return failure();
          if (!scopes[childIndex]->controls.contains(argument)) continue;
          auto forwarded = dyn_cast<BlockArgument>(instance->getOperand(argument.getArgNumber()));
          if (forwarded && forwarded.getOwner() == &module.getBody().front())
            scope.controls.insert(forwarded);
        }
      }
    }
    active.erase(module);
    return success();
  }
  LogicalResult validate(Scope &scope) {
    auto module = plan.occurrences[scope.occurrence].definition;
    std::optional<unsigned> defaultDomain;
    if (module->hasAttr("ac.default_domain")) {
      auto d = domain(module, "ac.default_domain", scope);
      if (failed(d))
        return failure();
      defaultDomain = *d;
    }
    // Boundary controls are exempt only at their dedicated child pins. The
    // argument itself retains its actual parent's data dependency everywhere.
    for (auto &entry : scope.children) {
      auto &child = *scopes[entry.second];
      auto definition = plan.occurrences[child.occurrence].definition;
      if (!definition->hasAttr("ac.default_domain"))
        continue;
      auto expected = domain(definition, "ac.default_domain", child);
      if (failed(expected))
        return failure();
      llvm::DenseSet<uint64_t> incoming;
      if (definition->hasAttr("ac.transaction_wiring")) {
        auto wiring = detail::parseTransactionWiring(definition, analysis, child.bindings, budget);
        if (failed(wiring)) return failure();
        for (const auto &route : wiring->incoming) {
          incoming.insert(route.dataInput);
          incoming.insert(route.enableInput);
        }
      }
      for (auto [i, input] : llvm::enumerate(entry.first->getOperands())) {
        auto argument = definition.getBody().front().getArgument(i);
        // Verified transaction inputs carry candidates for their actual state
        // owners, which may be below a different default-domain boundary. Keep
        // their SSA dependencies intact for the state/update checks below.
        if (!child.controls.contains(argument) && !incoming.contains(i) &&
            failed(check(input, *expected, scope, entry.first)))
          return failure();
      }
    }
    for (auto &op : module.getBody().front()) {
      Operation *callee = nullptr;
      if (auto instance = dyn_cast<InstanceOp>(op))
        callee = analysis.resolveCallee(instance);
      if (auto collection = dyn_cast<CollectionOp>(op))
        callee = analysis.resolveCallee(collection);
      if (callee && !isCDCInstance(dyn_cast<InstanceOp>(op)) && !op.hasAttr("ac.cdc_component")) {
        auto kind = analysis.getPrimitiveKind(callee);
        if (!kind.empty() && kind!="dff" && kind!="dffe") {
          auto actual=clock(op.getOperand(0),scope);
          if (failed(actual)) return failure();
          for (auto input : op.getOperands().drop_front(2))
            if (failed(check(input,*actual,scope,&op))) return failure();
        }
        if (kind == "dff" || kind == "dffe") {
          auto layout = getRegisterPrimitiveLayout(callee);
          auto actual = clock(op.getOperand(0), scope);
          if (failed(layout) || failed(actual))
            return failure();
          if (failed(check(op.getOperand(layout->data), *actual, scope, &op)))
            return failure();
          if (layout->enable && failed(check(op.getOperand(*layout->enable),
                                             *actual, scope, &op, true)))
            return failure();
        }
      }
      if (auto state = dyn_cast<acir::LogicalStateOpInterface>(op)) {
        auto expected = domain(state, "domain", scope);
        auto actual = clock(state.getStateClock(), scope);
        if (failed(expected) || failed(actual) || *expected != *actual)
          return reject(state, "state clock disagrees with resolved domain");
        for (auto *user : state.getStateQ().getUsers())
          if (auto update = dyn_cast<acir::LogicalStateUpdateOpInterface>(user);
              update && update.getUpdatedStateQ() == state.getStateQ())
            if (failed(
                    check(update.getStateCandidate(), *expected, scope, update)) ||
                failed(
                    check(update.getStateWriteIntent(), *expected, scope, update, true)))
              return failure();
      }
      if (isCDCInstance(dyn_cast<InstanceOp>(op)) || isa<CDCRefinementOp>(op)) {
        auto instance = dyn_cast<InstanceOp>(op);
        std::optional<CDCPrimitiveLayout> layout;
        if (isCDCInstance(instance)) {
          auto checked = getCDCPrimitiveLayout(instance, scope.bindings);
          if (failed(checked)) return failure();
          layout = *checked;
        }
        bool reg = layout ? !layout->isFIFO()
                          : op.getAttrOfType<StringAttr>("logical_kind").getValue() == "async_reg";
        auto a = domain(&op, reg ? "source_domain" : "write_domain", scope),
             b = domain(&op, reg ? "target_domain" : "read_domain", scope);
        if (sourceInference) {
          if (failed(a) || failed(b)) return failure();
          if (reg && failed(check(op.getOperand(3), *a, scope, &op))) return failure();
          continue;
        }
        auto ac = clock(op.getOperand(0), scope),
             bc = clock(op.getOperand(1), scope);
        if (failed(a) || failed(b) || failed(ac) || failed(bc) || *a != *ac ||
            *b != *bc)
          return reject(&op, "CDC endpoint clock disagrees with domain");
        if (reg) {
          if (failed(check(op.getOperand(3), *a, scope, &op)))
            return failure();
        } else if (isa<CDCRefinementOp>(op)) {
          if (failed(check(op.getOperand(3), *a, scope, &op)) ||
              failed(check(op.getOperand(4), *a, scope, &op, true)) ||
              failed(check(op.getOperand(5), *b, scope, &op, true)))
            return failure();
        } else if (layout) {
          if (failed(check(layout->input(), *a, scope, &op)) ||
              failed(check(layout->pushIntent(), *a, scope, &op, true)) ||
              failed(check(layout->popIntent(), *b, scope, &op, true))) return failure();
        }
      }
    }
    if (sourceInference) {
      auto transactions = module.walk([&](SourceTransactionOp transaction) {
        auto actual = clock(transaction.getClk(), scope);
        if (failed(actual)) return WalkResult::interrupt();
        for (Value capture : transaction.getCaptures())
          if (!scope.controls.contains(capture) && failed(check(capture, *actual, scope, transaction)))
            return WalkResult::interrupt();
        return WalkResult::advance();
      });
      if (transactions.wasInterrupted()) return failure();
    }
    if (defaultDomain) {
      auto yield = cast<YieldOp>(module.getBody().front().back());
      auto ports = getModulePortLayout(module);
      if (failed(ports)) return failure();
      // Only authored outputs belong to this Module's default-domain
      // interface. Private transaction exports retain their owners' domains.
      for (auto output : yield.getOperands().take_front(ports->sourceOutputCount))
        if (failed(check(output, *defaultDomain, scope, yield)))
          return failure();
    }
    return success();
  }

  FailureOr<std::pair<Scope *, InstanceOp>> sourceQueueActual(
      Scope &initial, DictionaryAttr receiver, InstanceOp imported = {}) {
    Scope *scope = &initial;
    for (unsigned depth = 0; depth < 128; ++depth) {
      if (failed(budget.debit(8, plan.root)) || !receiver) return failure();
      if (auto reference = receiver.getAs<StringAttr>("reference")) {
        auto enclosing = imported ? imported : dyn_cast_or_null<InstanceOp>(plan.occurrences[scope->occurrence].allocation);
        if (!enclosing || (!imported && !scope->parent)) return reject(plan.root, "Queue reference lacks its actual allocation context");
        auto actuals = enclosing->getAttrOfType<ArrayAttr>("ac.reference_bindings");
        DictionaryAttr actual;
        for (Attribute raw : actuals ? actuals : ArrayAttr::get(plan.root.getContext(), {})) {
          if (failed(budget.debit(1, enclosing))) return failure();
          auto row = cast<DictionaryAttr>(raw);
          if (row.getAs<StringAttr>("name") == reference) actual = row.getAs<DictionaryAttr>("receiver");
        }
        if (!actual) return reject(enclosing, "Queue reference binding is absent");
        if (!imported) scope = scope->parent;
        imported = {};
        receiver = actual;
        continue;
      }
      auto path = receiver.getAs<ArrayAttr>("instance_path");
      if (!path || path.size() != 1 || imported) return std::make_pair(scope, InstanceOp());
      auto definition = plan.occurrences[scope->occurrence].definition;
      for (auto allocation : definition.getBody().front().getOps<InstanceOp>()) {
        if (failed(budget.debit(1, allocation))) return failure();
        if ((path[0] == allocation.getOccurrence() || path[0] == allocation.getInstanceNameAttr()) && isAsyncQueueInstance(allocation))
          return std::make_pair(scope, allocation);
      }
      return std::make_pair(scope, InstanceOp());
    }
    return reject(plan.root, "Queue reference binding depth exceeded");
  }
  LogicalResult inferQueueEffects() {
    Builder builder(plan.root.getContext());
    auto constrain = [&](Scope &scope, SymbolRefAttr callee, DictionaryAttr receiver,
                         unsigned samplingClock, InstanceOp imported = {}) -> LogicalResult {
      if (!callee || callee.getRootReference().getValue() != "pycircuit.__builtins__.async_fifo" || callee.getNestedReferences().size() != 1) return success();
      auto actual = sourceQueueActual(scope, receiver, imported);
      if (failed(actual)) return failure();
      if (!actual->second) return success();
      bool push = callee.getNestedReferences()[0].getValue() == "push";
      auto endpoint = domain(actual->second, push ? "write_domain" : "read_domain", *actual->first);
      if (failed(endpoint)) return failure();
      if (inferences.count(*endpoint)) inferences[*endpoint].constraints.insert(samplingClock);
      else if (*endpoint != samplingClock) return reject(actual->second, "Queue Method crosses its endpoint clock domain");
      return success();
    };
    for (auto &owned : scopes) {
      auto &scope = *owned;
      auto module = plan.occurrences[scope.occurrence].definition;
      auto calls = module.walk([&](SourceMethodCallOp call) {
        auto sampling = call->getParentOfType<SourceTransactionOp>();
        auto actualClock = sampling ? clock(sampling.getClk(), scope) : domain(module, "ac.default_domain", scope);
        if (failed(actualClock)) return WalkResult::interrupt();
        NamedAttrList receiver;
        receiver.set("instance_path", call.getInstancePathAttr());
        if (auto reference = call->getAttr("ac.reference")) receiver.set("reference", reference);
        return failed(constrain(scope, call.getCalleeAttr(), receiver.getDictionary(builder.getContext()), *actualClock))
            ? WalkResult::interrupt() : WalkResult::advance();
      });
      if (calls.wasInterrupted()) return failure();
      for (auto instance : module.getBody().front().getOps<InstanceOp>()) {
        auto imported = dyn_cast_or_null<ModuleImportOp>(analysis.resolveCallee(instance));
        auto summary = imported ? imported->getAttrOfType<DictionaryAttr>("transaction_summary") : DictionaryAttr();
        if (!summary || !instance->hasAttr("ac.reference_bindings")) continue;
        auto defaultName = imported->getAttrOfType<StringAttr>("ac.default_domain");
        auto bindings = instance->getAttrOfType<DictionaryAttr>("ac.domain_bindings");
        auto actualName = bindings && defaultName ? bindings.getAs<StringAttr>(defaultName.getValue()) : StringAttr();
        auto &clocks = plan.occurrences[scope.occurrence].clocks;
        if (!actualName || !clocks.count(actualName.getValue())) return reject(instance, "Queue consumer requires its actual default clock binding");
        unsigned samplingClock = clocks.lookup(actualName.getValue());
        auto effects = [&](auto &&self, ArrayAttr rows, unsigned depth) -> LogicalResult {
          if (depth > 128 || !rows || failed(budget.debit(rows.size() + 1, instance))) return failure();
          for (Attribute raw : rows) {
            auto row = dyn_cast<DictionaryAttr>(raw);
            auto kind = row ? row.getAs<StringAttr>("kind") : StringAttr();
            if (!kind) return reject(instance, "published effect requires its closed kind");
            if (kind.getValue() == "call") {
              if (failed(constrain(scope, row.getAs<SymbolRefAttr>("callee"), row.getAs<DictionaryAttr>("receiver"), samplingClock, instance))) return failure();
            } else if (kind.getValue() == "branch") {
              if (failed(self(self, row.getAs<ArrayAttr>("then_effects"), depth + 1)) ||
                  failed(self(self, row.getAs<ArrayAttr>("else_effects"), depth + 1))) return failure();
            }
          }
          return success();
        };
        for (StringRef inventory : {"methods", "autonomous"})
          for (Attribute raw : summary.getAs<ArrayAttr>(inventory)) {
            auto callable = cast<DictionaryAttr>(raw);
            if (failed(effects(effects, callable.getAs<ArrayAttr>("guard_effects"), 0)) ||
                failed(effects(effects, callable.getAs<ArrayAttr>("body_effects"), 0))) return failure();
          }
      }
    }
    return success();
  }

public:
  Resolver(mlir::ModuleOp package, HardwareSourceCheckLimits limits)
      : analysis(package), budget(limits) {
    budget.setPhase("logical domain resolution");
  }
  LogicalResult infer(ModuleOp rootModule) {
    sourceInference = true;
    plan.root = rootModule;
    HardwareBindings bindings; bindings.owner = rootModule;
    HardwareDomainOccurrence root; root.definition = rootModule;
    DenseMap<Value, unsigned> values;
    auto clocks = rootModule->getAttrOfType<DictionaryAttr>("ac.domain_clock_inputs");
    if (!clocks) return reject(rootModule, "CDC inference requires declared owner clock inputs");
    for (NamedAttribute row : clocks) {
      auto ordinal = dyn_cast<IntegerAttr>(row.getValue());
      if (!ordinal || ordinal.getInt() < 0 || uint64_t(ordinal.getInt()) >= rootModule.getBody().front().getNumArguments())
        return reject(rootModule, "source clock input ordinal is invalid");
      unsigned index = ordinal.getInt();
      root.clocks[row.getName().getValue()] = index;
      values[rootModule.getBody().front().getArgument(index)] = index;
    }
    if (failed(visit(rootModule, bindings, std::move(root), std::move(values), 0))) return failure();
    for (auto &scope : scopes) if (failed(validate(*scope))) return failure();
    if (failed(inferQueueEffects())) return failure();
    for (size_t round = 0; round <= inferences.size(); ++round) {
      bool changed = false;
      for (auto &[token, item] : inferences) {
        Domains expanded;
        for (unsigned constraint : item.constraints) {
          auto other = inferences.find(constraint);
          if (other == inferences.end()) expanded.insert(constraint);
          else expanded.insert(other->second.constraints.begin(), other->second.constraints.end());
        }
        expanded.erase(token);
        if (expanded != item.constraints) { item.constraints = std::move(expanded); changed = true; }
      }
      if (!changed) break;
      if (failed(budget.debit(inferences.size(), rootModule))) return failure();
    }
    std::map<std::pair<Operation *, bool>, std::pair<StringAttr, Value>> bindingsToApply;
    for (const auto &[token, item] : inferences) {
      if (item.constraints.size() != 1 || inferences.count(*item.constraints.begin()))
        return reject(item.resource, "CDC endpoint is ambiguous; specify its Domain explicitly");
      unsigned actual = *item.constraints.begin();
      auto &occurrence = plan.occurrences[item.occurrence];
      StringAttr selected;
      auto prior = item.resource->getAttrOfType<StringAttr>(item.source
          ? (isAsyncQueueInstance(cast<InstanceOp>(item.resource)) ? "write_domain" : "source_domain")
          : (isAsyncQueueInstance(cast<InstanceOp>(item.resource)) ? "read_domain" : "target_domain"));
      if (prior && occurrence.clocks.lookup(prior.getValue()) == actual && occurrence.clocks.count(prior.getValue())) selected = prior;
      if (!selected) for (auto &entry : occurrence.clocks)
        if (entry.second == actual) { selected = StringAttr::get(item.resource->getContext(), entry.first()); break; }
      auto owner = occurrence.definition;
      auto inputs = owner->getAttrOfType<DictionaryAttr>("ac.domain_clock_inputs");
      auto ordinal = selected && inputs ? inputs.getAs<IntegerAttr>(selected.getValue()) : IntegerAttr();
      if (!ordinal) return reject(item.resource, "inferred endpoint has no owner clock input");
      Value clockValue = owner.getBody().front().getArgument(ordinal.getInt());
      auto key = std::make_pair(item.resource, item.source);
      auto [it, inserted] = bindingsToApply.try_emplace(key, std::make_pair(selected, clockValue));
      if (!inserted && it->second.first != selected)
        return reject(item.resource, "shared definition requires explicit endpoint formals");
    }
    for (auto &[key, binding] : bindingsToApply) {
      auto *resource = key.first; bool source = key.second;
      bool fifo = isAsyncQueueInstance(cast<InstanceOp>(resource));
      resource->setAttr(fifo ? (source ? "write_domain" : "read_domain") : (source ? "source_domain" : "target_domain"), binding.first);
      resource->setOperand(source ? 0 : 1, binding.second);
      resource->removeAttr(source ? "source_infer_source" : "source_infer_target");
    }
    return success();
  }
  FailureOr<HardwareDomainPlan> run() {
    auto contract =
        analysis.getPackage()->getAttrOfType<IntegerAttr>("ac.routed_contract");
    if (!contract || contract.getInt() != 7)
      return std::move(plan);
    auto bindings = detail::resolveHardwareRootBindings(analysis);
    if (failed(bindings))
      return failure();
    plan.root = cast<ModuleOp>(bindings->owner);
    HardwareDomainOccurrence root;
    root.definition = plan.root;
    auto clocks =
        plan.root->getAttrOfType<DictionaryAttr>("ac.domain_clock_inputs");
    if (!clocks)
      return reject(plan.root, "contract 7 root requires domain clock inputs");
    DenseMap<Value, unsigned> values;
    auto ports = getModulePortLayout(plan.root);
    if (failed(ports))
      return failure();
    for (auto row : clocks) {
      auto ordinal = dyn_cast<IntegerAttr>(row.getValue());
      if (!ordinal || !ordinal.getType().isSignlessInteger(32) || ordinal.getInt() < 0 ||
          uint64_t(ordinal.getInt()) >= ports->sourceInputCount)
        return reject(plan.root, "invalid root clock input ordinal");
      unsigned index = ordinal.getInt();
      auto value = plan.root.getBody().front().getArgument(index);
      auto width =
          analysis.getPackedWidth(value.getType(), *bindings, plan.root);
      if (failed(width) || *width != 1)
        return reject(plan.root, "root clock must be one bit");
      root.clocks[row.getName().getValue()] = index;
      values[value] = index;
      if (!llvm::is_contained(plan.clockInputs, index))
        plan.clockInputs.push_back(index);
    }
    if (failed(
            visit(plan.root, *bindings, std::move(root), std::move(values), 0)))
      return failure();
    for (auto &scope : scopes)
      if (failed(validate(*scope)))
        return failure();
    return std::move(plan);
  }
};
} // namespace
LogicalResult inferSourceCDCEndpoints(mlir::ModuleOp package,
                                      HardwareSourceCheckLimits limits) {
  for (auto module : package.getBody()->getOps<ModuleOp>()) {
    bool requested = false;
    for (auto instance : module.getBody().front().getOps<InstanceOp>())
      requested |= instance->hasAttr("source_infer_source") || instance->hasAttr("source_infer_target");
    if (requested && failed(Resolver(package, limits).infer(module))) return failure();
    if (requested)
      for (auto instance : module.getBody().front().getOps<InstanceOp>())
        if (instance->hasAttr("source_infer_source") || instance->hasAttr("source_infer_target"))
          return instance.emitOpError() << "CDC endpoint inference did not resolve the resource request";
  }
  return success();
}
FailureOr<HardwareDomainPlan>
getHardwareDomainPlan(mlir::ModuleOp package,
                      HardwareSourceCheckLimits limits) {
  if (failed(verifyFinalHardwareCDCComponents(package)))
    return failure();
  return Resolver(package, limits).run();
}
LogicalResult verifyHardwareDomains(mlir::ModuleOp package,
                                    HardwareSourceCheckLimits limits) {
  return success(succeeded(getHardwareDomainPlan(package, limits)));
}
FailureOr<ArrayAttr>
serializeHardwareDomainBindings(mlir::ModuleOp package,
                                HardwareSourceCheckLimits limits) {
  auto plan = getHardwareDomainPlan(package, limits);
  if (failed(plan))
    return failure();
  Builder b(package.getContext());
  SmallVector<Attribute> rows;
  for (auto &occurrence : plan->occurrences) {
    SmallVector<Attribute> path(occurrence.instancePath.begin(),
                                occurrence.instancePath.end());
    SmallVector<StringRef> names;
    for (auto &entry : occurrence.clocks)
      names.push_back(entry.getKey());
    llvm::sort(names);
    for (auto name : names)
      rows.push_back(b.getDictionaryAttr(
          {b.getNamedAttr("instance_path", b.getArrayAttr(path)),
           b.getNamedAttr("formal", b.getStringAttr(name)),
           b.getNamedAttr(
               "clock_input",
               b.getI64IntegerAttr(occurrence.clocks.lookup(name)))}));
  }
  return b.getArrayAttr(rows);
}
} // namespace acir::ac
