#include "pycircuit/Dialect/ACIR/HardwareClockSources.h"
#include "ACIRSourceContracts.h"
#include "EntryBoundary.h"
#include "HardwareSourceChecks.h"
#include "SourceCheckBudget.h"
#include "mlir/IR/Builders.h"
#include "pycircuit/Interfaces/LogicalStateInterfaces.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include <algorithm>
#include <limits>
#include <set>
#include <tuple>
#include <utility>

using namespace mlir;
namespace acir::ac {
namespace {
// This component proves clock identity only. Ordinary payload dependency
// checking and executable-package admission remain separate obligations.
class SourceResolver {
public:
  SourceResolver(const HardwareClockSourcePlan &plan,
                 HardwareAnalysis &analysis, detail::SourceCheckBudget &budget,
                 bool diagnose = true)
      : plan(plan), analysis(analysis), budget(budget), diagnose(diagnose) {}

  FailureOr<unsigned> clock(Value value, unsigned owner, unsigned depth = 0) {
    if (failed(step(value, owner, depth)))
      return failure();
    auto &scope = plan.occurrences[owner];
    if (auto source = value.getDefiningOp<ClockSourceOp>()) {
      if (failed(budget.debit(plan.sources.size(), source)))
        return failure();
      for (auto [id, row] : llvm::enumerate(plan.sources))
        if (row.ownerOccurrence == owner && row.declaration == source) {
          return static_cast<unsigned>(id);
        }
      return reject(source, "undeclared clock source");
    }
    if (auto argument = dyn_cast<BlockArgument>(value)) {
      auto *parent = argument.getOwner()->getParentOp();
      if (auto rule = dyn_cast<RuleOp>(parent))
        return clock(rule.getCaptures()[argument.getArgNumber()], owner,
                     depth + 1);
      if (parent == scope.definition) {
        auto ports = getModulePortLayout(scope.definition);
        if (failed(ports))
          return failure();
        unsigned ordinal = argument.getArgNumber();
        if (ordinal >= ports->sourceInputCount ||
            (ordinal < ports->inputValidity.size() &&
             ports->inputValidity[ordinal] >= 0))
          return reject(parent,
                        "clock forwarding requires a total source input");
        if (scope.parent)
          return clock(scope.allocation->getOperand(ordinal), *scope.parent,
                       depth + 1);
        if (failed(budget.debit(plan.sources.size(), parent)))
          return failure();
        for (auto [id, row] : llvm::enumerate(plan.sources))
          if (row.rootInput == ordinal) {
            return static_cast<unsigned>(id);
          }
      }
    }
    if (auto extract = value.getDefiningOp<BitsExtractOp>()) {
      auto low = natural(extract.getLow(), owner, extract);
      auto width = analysis.getPackedWidth(extract.getInput().getType(),
                                           scope.bindings, extract);
      if (failed(low) || failed(width))
        return failure();
      if (*low == 0 && *width == 1)
        return clock(extract.getInput(), owner, depth + 1);
    }
    if (auto result = dyn_cast<OpResult>(value)) {
      if (auto found = scope.children.find(result.getOwner());
          found != scope.children.end()) {
        auto child = plan.occurrences[found->second].definition;
        auto yield = dyn_cast<YieldOp>(child.getBody().front().back());
        if (!yield || result.getResultNumber() >= yield.getNumOperands())
          return reject(child, "invalid forwarded clock result");
        auto ports = getModulePortLayout(child);
        if (failed(ports))
          return failure();
        auto ordinal = result.getResultNumber();
        if (ordinal >= ports->sourceOutputCount ||
            (ordinal < ports->outputValidity.size() &&
             ports->outputValidity[ordinal] >= 0))
          return reject(child,
                        "clock forwarding requires a total source output");
        return clock(yield.getOperand(ordinal), found->second, depth + 1);
      }
    }
    return reject(value.getParentBlock()->getParentOp(),
                  "clock must forward a declared source; transaction results "
                  "and combinational gates are not clocks");
  }

  // Producer admission has already verified the closed initializer DAG.
  // Follow only the selected packed bit, since its neighbors may be X or Z.
  FailureOr<bool> initialBit(Value value, unsigned owner, uint64_t bit,
                             unsigned depth = 0) {
    if (failed(step(value, owner, depth)))
      return failure();
    auto &scope = plan.occurrences[owner];
    auto width = analysis.getPackedWidth(value.getType(), scope.bindings,
                                         scope.definition);
    if (failed(width) || bit >= *width)
      return reject(scope.definition,
                    "registered clock initial bit is out of range");
    if (auto constant = value.getDefiningOp<BitsConstantOp>()) {
      auto raw = analysis.evaluateStatic(constant.getValue(), scope.bindings,
                                         constant);
      auto number =
          succeeded(raw) ? dyn_cast<MathIntAttr>(*raw) : MathIntAttr();
      if (!number)
        return reject(
            constant,
            "registered clock initializer is not an integer constant");
      llvm::APSInt bits(number.getCanonicalValue());
      if (bits.isNegative() || bits.getActiveBits() > *width)
        return reject(constant,
                      "registered clock initializer does not fit its payload");
      return bit < bits.getBitWidth() && bits[static_cast<unsigned>(bit)];
    }
    if (auto constant = value.getDefiningOp<BitsConstant4Op>()) {
      auto pattern = constant.getPattern();
      if (pattern.size() != *width)
        return reject(
            constant,
            "registered clock initializer width disagrees with its pattern");
      char level = pattern[pattern.size() - 1 - bit];
      if (level != '0' && level != '1')
        return reject(constant, "registered clock initial bit is unknown");
      return level == '1';
    }
    auto *op = value.getDefiningOp();
    if (auto member = dyn_cast_or_null<EnumCreateOp>(op)) {
      auto resolved =
          analysis.resolveType(value.getType(), scope.bindings, member);
      auto type =
          succeeded(resolved) ? dyn_cast<EnumType>(*resolved) : EnumType();
      auto declaration = type ? analysis.lookupEnum(type) : EnumOp();
      if (!declaration)
        return reject(member,
                      "registered clock initializer requires a resolved Enum");
      for (auto raw : declaration.getMembers()) {
        auto row = cast<DictionaryAttr>(raw);
        if (row.getAs<StringAttr>("name") != member.getMemberAttr())
          continue;
        llvm::APSInt bits(row.getAs<MathIntAttr>("code").getCanonicalValue());
        if (bits.isNegative() || bits.getActiveBits() > *width)
          return reject(
              member,
              "registered clock Enum initializer does not fit its payload");
        return bit < bits.getBitWidth() && bits[static_cast<unsigned>(bit)];
      }
      return reject(
          member, "registered clock initializer names an unknown Enum member");
    }
    if (auto cast = dyn_cast_or_null<EnumToBitsOp>(op))
      return initialBit(cast->getOperand(0), owner, bit, depth + 1);
    if (isa_and_nonnull<StructCreateOp, TableCreateOp>(op)) {
      for (Value input : llvm::reverse(op->getOperands())) {
        auto inputWidth =
            analysis.getPackedWidth(input.getType(), scope.bindings, op);
        if (failed(inputWidth))
          return failure();
        if (bit < *inputWidth)
          return initialBit(input, owner, bit, depth + 1);
        bit -= *inputWidth;
      }
    }
    if (auto splat = dyn_cast_or_null<TableSplatOp>(op)) {
      Value element = splat->getOperand(0);
      auto elementWidth =
          analysis.getPackedWidth(element.getType(), scope.bindings, splat);
      if (failed(elementWidth) || *elementWidth == 0)
        return failure();
      return initialBit(element, owner, bit % *elementWidth, depth + 1);
    }
    if (auto get = dyn_cast_or_null<StructGetOp>(op)) {
      auto offset = structFieldOffset(get, owner);
      if (failed(offset) || *offset > std::numeric_limits<uint64_t>::max() - bit)
        return failure();
      return initialBit(get.getValue(), owner, *offset + bit, depth + 1);
    }
    if (auto extract = dyn_cast_or_null<BitsExtractOp>(op)) {
      auto low = natural(extract.getLow(), owner, extract);
      if (failed(low) || *low > std::numeric_limits<uint64_t>::max() - bit)
        return failure();
      return initialBit(extract.getInput(), owner, *low + bit, depth + 1);
    }
    if (auto resize = dyn_cast_or_null<BitsResizeOp>(op)) {
      Value input = resize.getInput();
      auto inputWidth =
          analysis.getPackedWidth(input.getType(), scope.bindings, resize);
      if (failed(inputWidth) || *inputWidth == 0)
        return failure();
      if (bit < *inputWidth)
        return initialBit(input, owner, bit, depth + 1);
      if (resize.getMode() == "zext")
        return false;
      return initialBit(input, owner, *inputWidth - 1, depth + 1);
    }
    return reject(op ? op : scope.definition.operator->(),
                  "cannot prove the selected bit of the static initializer");
  }

  LogicalResult producer(Value value, unsigned owner,
                         HardwareClockSource &source, uint64_t bit = 0,
                         unsigned depth = 0) {
    if (failed(step(value, owner, depth)))
      return failure();
    auto &scope = plan.occurrences[owner];
    auto width = analysis.getPackedWidth(value.getType(), scope.bindings,
                                         scope.definition);
    if (failed(width) || bit >= *width)
      return reject(scope.definition,
                    "registered clock bit projection is out of range");
    if (auto argument = dyn_cast<BlockArgument>(value)) {
      if (argument.getOwner()->getParentOp() == scope.definition &&
          scope.parent) {
        auto ports = getModulePortLayout(scope.definition);
        if (failed(ports))
          return failure();
        auto ordinal = argument.getArgNumber();
        if (ordinal >= ports->sourceInputCount ||
            (ordinal < ports->inputValidity.size() &&
             ports->inputValidity[ordinal] >= 0))
          return reject(scope.definition,
                        "registered clock input must have total availability");
        return producer(scope.allocation->getOperand(ordinal), *scope.parent,
                        source, bit, depth + 1);
      }
    }
    auto *op = value.getDefiningOp();
    if (auto extract = dyn_cast_or_null<BitsExtractOp>(op)) {
      auto low = natural(extract.getLow(), owner, extract);
      auto inputWidth = analysis.getPackedWidth(extract.getInput().getType(),
                                                scope.bindings, extract);
      if (failed(low) || failed(inputWidth))
        return failure();
      if (*low > *inputWidth || *width > *inputWidth - *low)
        return reject(extract, "registered clock extraction is out of range");
      return producer(extract.getInput(), owner, source, *low + bit, depth + 1);
    }
    if (auto get = dyn_cast_or_null<StructGetOp>(op)) {
      auto offset = structFieldOffset(get, owner);
      if (failed(offset))
        return failure();
      if (*offset > std::numeric_limits<uint64_t>::max() - bit)
        return reject(get, "invalid registered clock field projection");
      return producer(get.getValue(), owner, source, *offset + bit, depth + 1);
    }
    if (auto cast = dyn_cast_or_null<EnumToBitsOp>(op))
      return producer(cast->getOperand(0), owner, source, bit, depth + 1);
    if (auto result = dyn_cast<OpResult>(value)) {
      if (auto found = scope.children.find(op); found != scope.children.end()) {
        auto child = plan.occurrences[found->second].definition;
        auto yield = dyn_cast<YieldOp>(child.getBody().front().back());
        auto ports = getModulePortLayout(child);
        if (failed(ports))
          return failure();
        unsigned ordinal = result.getResultNumber();
        if (!yield || ordinal >= yield.getNumOperands() ||
            ordinal >= ports->sourceOutputCount ||
            (ordinal < ports->outputValidity.size() &&
             ports->outputValidity[ordinal] >= 0))
          return reject(child,
                        "registered clock output must have total availability");
        return producer(yield.getOperand(ordinal), found->second, source, bit,
                        depth + 1);
      }
    }
    if (auto state = dyn_cast_or_null<acir::LogicalStateOpInterface>(op)) {
      if (value != state.getStateQ())
        return reject(op, "clock producer is not state Q");
      auto policy = getRegisterPolicy(op);
      if (failed(policy) || !policy->hasInitial)
        return reject(op, "registered clock requires explicit initialization");
      source.storageClock = state.getStateClock();
      source.initial = state.getStateInitial();
    } else if (auto instance = dyn_cast_or_null<InstanceOp>(op)) {
      auto *callee = analysis.resolveCallee(instance);
      if (!callee)
        return reject(op, "unresolved registered clock producer");
      auto kind = analysis.getPrimitiveKind(callee);
      if ((kind != "dff" && kind != "dffe") || value != instance->getResult(0))
        return reject(op, "clock producer must be one registered state bit");
      if (failed(analysis.verifyInstance(instance, scope.bindings)))
        return failure();
      auto pins = getRegisterPrimitiveLayout(callee);
      auto policy = getRegisterPolicy(op);
      if (failed(pins) || failed(policy) || !policy->hasInitial)
        return reject(op,
                      "registered clock requires initialized DFF/DFFE storage");
      source.storageClock = instance->getOperand(pins->clock);
      source.initial = instance->getOperand(pins->initial);
    } else {
      return reject(op ? op : scope.definition.operator->(),
                    "clock producer must be a registered bit; combinational "
                    "and transaction-result clocks are excluded");
    }
    auto occurrence = op->getAttrOfType<DictionaryAttr>("occurrence");
    if (failed(detail::verifyOccurrence(occurrence,
                                        [&] { return op->emitOpError(); })) ||
        failed(detail::verifyOriginDefinition(
            occurrence,
            FlatSymbolRefAttr::get(op->getContext(),
                                   scope.definition->getAttrOfType<StringAttr>(
                                       "sym_name").getValue()),
            "clock producer", [&] { return op->emitOpError(); })) ||
        failed(detail::verifyClosedStateInitializer(
            analysis, source.initial, scope.bindings, op, &budget)))
      return failure();
    source.storage = op;
    source.storageOwnerOccurrence = owner;
    source.bitOffset = bit;
    return success();
  }

private:
  const HardwareClockSourcePlan &plan;
  HardwareAnalysis &analysis;
  detail::SourceCheckBudget &budget;
  bool diagnose;
  LogicalResult reject(Operation *op, StringRef reason) {
    if (!diagnose)
      return failure();
    return op->emitOpError() << "clock source: " << reason;
  }
  LogicalResult step(Value value, unsigned owner, unsigned depth) {
    if (!value || owner >= plan.occurrences.size())
      return failure();
    if (depth >= 256)
      return reject(plan.occurrences[owner].definition,
                    "cyclic or excessive clock forwarding");
    return budget.debit(16, plan.occurrences[owner].definition);
  }
  FailureOr<uint64_t> structFieldOffset(StructGetOp get, unsigned owner) {
    auto &bindings = plan.occurrences[owner].bindings;
    auto resolved = analysis.resolveType(get.getValue().getType(), bindings, get);
    auto record =
        succeeded(resolved) ? dyn_cast<StructType>(*resolved) : StructType();
    auto declaration = record ? analysis.lookupStruct(record) : StructOp();
    if (!declaration)
      return reject(get, "clock projection needs a resolved Struct");
    uint64_t offset = 0;
    for (auto raw : llvm::reverse(declaration.getFields())) {
      if (failed(budget.debit(8, get)))
        return failure();
      auto field = cast<DictionaryAttr>(raw);
      if (field.getAs<StringAttr>("name") == get.getFieldAttr())
        return offset;
      auto width = analysis.getPackedWidth(
          field.getAs<TypeAttr>("type").getValue(), bindings, get);
      if (failed(width) || *width > std::numeric_limits<uint64_t>::max() - offset)
        return failure();
      offset += *width;
    }
    return reject(get, "invalid registered clock field projection");
  }
  FailureOr<uint64_t> natural(StaticExprAttr expression, unsigned owner,
                              Operation *site) {
    auto number = analysis.evaluateStatic(
        expression, plan.occurrences[owner].bindings, site);
    auto integer =
        succeeded(number) ? dyn_cast<MathIntAttr>(*number) : MathIntAttr();
    if (!integer)
      return reject(site,
                    "clock bit projection requires a static natural offset");
    llvm::APSInt value(integer.getCanonicalValue());
    if (value.isNegative() || value.getActiveBits() > 64)
      return reject(site, "clock bit projection offset is out of range");
    return value.getZExtValue();
  }
};

class SourcePlanner {
public:
  SourcePlanner(mlir::ModuleOp package, HardwareSourceCheckLimits limits)
      : analysis(package), budget(limits) {
    budget.setPhase("registered clock source planning");
  }
  FailureOr<HardwareClockSourcePlan> run() {
    auto package = analysis.getPackage();
    auto profile = package->getAttrOfType<IntegerAttr>("ac.routed_contract");
    auto stage = package->getAttrOfType<StringAttr>("ac.stage");
    if (!profile || profile.getInt() != 7 || !stage ||
        (stage.getValue() != "routed" && stage.getValue() != "final"))
      return package.emitOpError()
             << "clock source planning requires contract-7 routed/final IR";
    auto bindings = detail::resolveHardwareRootBindings(analysis);
    if (failed(bindings))
      return failure();
    plan.root = cast<ModuleOp>(bindings->owner);
    if (failed(collect(plan.root, *bindings, std::nullopt, {}, 0)))
      return failure();
    DenseSet<Operation *> selected;
    for (auto &scope : plan.occurrences)
      selected.insert(scope.definition);
    for (auto module : package.getOps<ModuleOp>())
      if (!selected.contains(module) && !module.getBody().empty() &&
          !module.getBody().front().getOps<ClockSourceOp>().empty())
        return reject(
            module,
            "registered source definition requires a selected occurrence");
    auto clocks =
        plan.root->getAttrOfType<DictionaryAttr>("ac.domain_clock_inputs");
    auto ports = getModulePortLayout(plan.root);
    if (!clocks || failed(ports))
      return reject(plan.root, "root requires declared clock inputs");
    DenseMap<unsigned, unsigned> rootSources;
    for (auto row : clocks) {
      if (failed(budget.debit(32, plan.root)))
        return failure();
      auto ordinal = dyn_cast<IntegerAttr>(row.getValue());
      if (!ordinal || !ordinal.getType().isSignlessInteger(32) ||
          ordinal.getInt() < 0 ||
          uint64_t(ordinal.getInt()) >= ports->sourceInputCount)
        return reject(plan.root, "invalid root clock ordinal");
      unsigned input = ordinal.getInt();
      auto value = plan.root.getBody().front().getArgument(input);
      auto width =
          analysis.getPackedWidth(value.getType(), *bindings, plan.root);
      if (failed(width) || *width != 1 ||
          (input < ports->inputValidity.size() &&
           ports->inputValidity[input] >= 0))
        return reject(plan.root, "root clock requires a total one-bit input");
      auto inserted = rootSources.try_emplace(input, plan.sources.size());
      if (inserted.second) {
        HardwareClockSource source;
        source.rootInput = input;
        plan.sources.push_back(source);
      }
      plan.occurrences[0].domains[row.getName().getValue()] =
          inserted.first->second;
    }
    for (auto [owner, scope] : llvm::enumerate(plan.occurrences)) {
      DenseSet<Attribute> occurrences;
      for (auto source :
           scope.definition.getBody().front().getOps<ClockSourceOp>()) {
        if (failed(budget.debit(64, source)) || failed(source.verify()))
          return failure();
        if (!occurrences.insert(source.getOccurrence()).second ||
            scope.domains.count(source.getDomain()))
          return reject(source, "duplicate local clock domain or occurrence");
        scope.domains[source.getDomain()] = plan.sources.size();
        HardwareClockSource row;
        row.ownerOccurrence = owner;
        row.declaration = source;
        plan.sources.push_back(row);
      }
    }
    if (plan.root.getSymName() == detail::entryBoundarySymbol) {
      auto &adapter = plan.occurrences.front();
      if (!llvm::hasSingleElement(adapter.children) ||
          cast<InstanceOp>(adapter.children.begin()->first).getInstanceName() !=
              "entry")
        return reject(plan.root,
                      "entry boundary requires one logical entry occurrence");
      const auto &entry = plan.occurrences[adapter.children.begin()->second];
      // Mirror the registered identity without changing its owner or its SSA.
      for (const auto &binding : entry.domains)
        if (!adapter.domains.try_emplace(binding.getKey(), binding.getValue())
                 .second)
          return reject(
              entry.definition,
              "registered entry domain conflicts with a root clock input");
    }
    if (failed(bindDomains()))
      return failure();
    SourceResolver resolver(plan, analysis, budget);
    std::set<std::tuple<unsigned, Operation *, uint64_t>> producers;
    for (auto &source : plan.sources) {
      if (source.rootInput)
        continue;
      auto declaration = source.declaration;
      auto owner = source.ownerOccurrence;
      if (failed(resolver.producer(declaration.getInput(), owner, source)))
        return failure();
      auto initial = resolver.initialBit(
          source.initial, source.storageOwnerOccurrence, source.bitOffset);
      if (failed(initial))
        return failure();
      source.initialLevel = *initial;
      auto actual =
          resolver.clock(source.storageClock, source.storageOwnerOccurrence);
      auto &domains = plan.occurrences[owner].domains;
      auto expected = domains.find(declaration.getSourceDomain());
      if (failed(actual) || expected == domains.end() ||
          expected->second != *actual)
        return reject(declaration,
                      "registered producer clock disagrees with source_domain");
      if (auto logical =
              dyn_cast<acir::LogicalStateOpInterface>(source.storage)) {
        auto name = source.storage->getAttrOfType<StringAttr>("domain");
        auto &producerDomains =
            plan.occurrences[source.storageOwnerOccurrence].domains;
        auto found = name ? producerDomains.find(name.getValue())
                          : producerDomains.end();
        if (found == producerDomains.end() || found->second != *actual)
          return reject(source.storage,
                        "logical producer domain disagrees with its clock");
      }
      source.parentSource = *actual;
      if (!producers
               .emplace(source.storageOwnerOccurrence, source.storage,
                        source.bitOffset)
               .second)
        return reject(
            declaration,
            "one registered bit cannot declare multiple clock identities");
    }
    SmallVector<unsigned char> marks(plan.sources.size(), 0);
    for (unsigned id = 0; id < plan.sources.size(); ++id)
      if (failed(order(id, marks, 0)))
        return failure();
    llvm::stable_sort(plan.eventOrder, [&](unsigned a, unsigned b) {
      return plan.sources[a].waveDepth < plan.sources[b].waveDepth;
    });
    return std::move(plan);
  }

private:
  HardwareAnalysis analysis;
  detail::SourceCheckBudget budget;
  HardwareClockSourcePlan plan;
  DenseSet<Operation *> active;
  LogicalResult reject(Operation *op, StringRef reason) {
    return op->emitOpError() << "clock source: " << reason;
  }
  LogicalResult collect(ModuleOp module, HardwareBindings bindings,
                        std::optional<unsigned> parent, InstanceOp allocation,
                        unsigned depth) {
    if (depth >= 128 || !active.insert(module).second)
      return reject(module, "recursive or excessive owner hierarchy");
    if (failed(budget.debit(128, module)))
      return failure();
    if (module.getBody().empty() || module.getBody().front().empty())
      return reject(module, "clock owner requires a body");
    unsigned owner = plan.occurrences.size();
    HardwareClockOccurrence scope;
    scope.definition = module;
    scope.bindings = std::move(bindings);
    scope.parent = parent;
    scope.allocation = allocation;
    if (parent) {
      auto occurrence = allocation->getAttrOfType<DictionaryAttr>("occurrence");
      if (failed(detail::verifyOccurrence(
              occurrence, [&] { return allocation.emitOpError(); })))
        return failure();
      scope.instancePath = plan.occurrences[*parent].instancePath;
      scope.instancePath.push_back(occurrence);
    }
    plan.occurrences.push_back(std::move(scope));
    DenseSet<Attribute> childIdentities;
    for (auto &op : module.getBody().front()) {
      if (failed(budget.debit(16, &op)))
        return failure();
      if (isa<CollectionOp>(op))
        return reject(
            &op,
            "registered-derived profile requires scalar instance hierarchy");
      if (auto state = dyn_cast<acir::LogicalStateOpInterface>(op);
          state && isa<TableType>(state.getStateQ().getType()))
        return reject(&op,
                      "registered-derived profile requires scalar storage");
      if (isa<QueueOp, CDCRefinementOp>(op))
        return reject(&op,
                      "registered-derived profile requires register storage");
      if (op.walk([](SourceObserveOp) { return WalkResult::interrupt(); })
              .wasInterrupted())
        return reject(&op,
                      "registered-derived profile does not support observations");
      auto instance = dyn_cast<InstanceOp>(op);
      if (!instance)
        continue;
      auto *callee = analysis.resolveCallee(instance);
      auto child = dyn_cast_or_null<ModuleOp>(callee);
      if (!child) {
        auto kind = analysis.getPrimitiveKind(callee);
        if (kind != "dff" && kind != "dffe")
          return reject(instance,
                        "registered-derived profile requires register storage");
        continue;
      }
      if (!childIdentities.insert(instance->getAttr("occurrence")).second)
        return reject(instance,
                      "duplicate child occurrence would alias clock owners");
      if (child.getBody().empty() ||
          instance.getInputs().size() !=
              child.getBody().front().getNumArguments())
        return reject(instance, "child clock owner signature mismatch");
      auto bound =
          analysis.bindInstance(instance, plan.occurrences[owner].bindings);
      if (failed(bound))
        return failure();
      plan.occurrences[owner].children[instance] = plan.occurrences.size();
      if (failed(collect(child, *bound, owner, instance, depth + 1)))
        return failure();
    }
    active.erase(module);
    return success();
  }
  LogicalResult bindDomains() {
    for (auto &scope : plan.occurrences) {
      auto formals =
          scope.definition->getAttrOfType<ArrayAttr>("ac.domain_params");
      if (!formals)
        return reject(scope.definition, "clock owner requires domain formals");
      auto map = scope.parent ? scope.allocation->getAttrOfType<DictionaryAttr>(
                                    "ac.domain_bindings")
                              : DictionaryAttr();
      DenseSet<StringAttr> names;
      for (auto raw : formals) {
        if (failed(budget.debit(8, scope.definition)))
          return failure();
        auto name = dyn_cast<StringAttr>(raw);
        if (!name || name.getValue().empty() || !names.insert(name).second)
          return reject(scope.definition, "invalid or duplicate domain formal");
        if (scope.domains.count(name.getValue())) {
          if (map && map.get(name.getValue()))
            return reject(
                scope.allocation,
                "local derived domain cannot have an inherited binding");
          continue;
        }
        auto target =
            map ? map.getAs<StringAttr>(name.getValue()) : StringAttr();
        if (!scope.parent || !target)
          return reject(scope.definition, "unbound domain formal");
        auto &parent = plan.occurrences[*scope.parent];
        auto found = parent.domains.find(target.getValue());
        if (found == parent.domains.end())
          return reject(scope.allocation, "unknown parent domain binding");
        scope.domains[name.getValue()] = found->second;
      }
      if (scope.domains.size() != names.size())
        return reject(scope.definition,
                      "clock source domain is absent from domain formals");
      if (map)
        for (auto row : map)
          if (!names.contains(StringAttr::get(scope.definition.getContext(),
                                              row.getName().getValue())) ||
              !isa<StringAttr>(row.getValue()))
            return reject(scope.allocation,
                          "unknown or malformed domain binding");
    }
    return success();
  }
  LogicalResult order(unsigned id, SmallVectorImpl<unsigned char> &marks,
                      unsigned depth) {
    if (failed(budget.debit(8, plan.root)))
      return failure();
    if (marks[id] == 2)
      return success();
    if (marks[id] == 1)
      return reject(plan.sources[id].declaration,
                    "cyclic registered clock source dependency");
    if (depth >= 256)
      return reject(plan.root,
                    "source dependency depth exceeds supported bound");
    marks[id] = 1;
    auto &source = plan.sources[id];
    if (source.parentSource) {
      if (failed(order(*source.parentSource, marks, depth + 1)))
        return failure();
      source.waveDepth = plan.sources[*source.parentSource].waveDepth + 1;
      plan.maxWaveDepth = std::max(plan.maxWaveDepth, source.waveDepth);
    }
    marks[id] = 2;
    plan.eventOrder.push_back(id);
    return success();
  }
};
} // namespace

FailureOr<HardwareClockSourcePlan>
getHardwareClockSourcePlan(mlir::ModuleOp package,
                           HardwareSourceCheckLimits limits) {
  return SourcePlanner(package, limits).run();
}
FailureOr<unsigned>
resolveHardwareClockSource(const HardwareClockSourcePlan &plan, Value value,
                           unsigned owner, HardwareSourceCheckLimits limits,
                           bool diagnose) {
  if (!plan.root)
    return failure();
  HardwareAnalysis analysis(plan.root->getParentOfType<mlir::ModuleOp>());
  detail::SourceCheckBudget budget(limits);
  budget.setPhase("clock source identity forwarding");
  return SourceResolver(plan, analysis, budget, diagnose).clock(value, owner);
}
ArrayAttr serializeHardwareClockSources(const HardwareClockSourcePlan &plan) {
  Builder b(plan.root->getContext());
  auto path = [&](unsigned owner) {
    auto &entries = plan.occurrences[owner].instancePath;
    return b.getArrayAttr(
        SmallVector<Attribute>(entries.begin(), entries.end()));
  };
  auto identity = [&](unsigned id) {
    return getHardwareClockSourceIdentity(plan, id);
  };
  SmallVector<Attribute> rows;
  for (unsigned id : plan.eventOrder) {
    auto &source = plan.sources[id];
    NamedAttrList row;
    row.append("identity", identity(id));
    row.append("wave_depth", b.getI64IntegerAttr(source.waveDepth));
    row.append("initial_level", b.getI64IntegerAttr(source.initialLevel));
    if (source.parentSource) {
      row.append("parent", identity(*source.parentSource));
      row.append("storage_owner_path", path(source.storageOwnerOccurrence));
      row.append("storage_occurrence", source.storage->getAttr("occurrence"));
      row.append("bit_offset", b.getI64IntegerAttr(source.bitOffset));
    }
    rows.push_back(row.getDictionary(b.getContext()));
  }
  return b.getArrayAttr(rows);
}
DictionaryAttr
getHardwareClockSourceIdentity(const HardwareClockSourcePlan &plan,
                               unsigned sourceID) {
  Builder b(plan.root->getContext());
  auto &source = plan.sources[sourceID];
  if (source.rootInput)
    return b.getDictionaryAttr(
        {b.getNamedAttr("kind", b.getStringAttr("root_input")),
         b.getNamedAttr("input", b.getI64IntegerAttr(*source.rootInput))});
  auto &path = plan.occurrences[source.ownerOccurrence].instancePath;
  return b.getDictionaryAttr(
      {b.getNamedAttr("kind", b.getStringAttr("registered")),
       b.getNamedAttr("owner_path", b.getArrayAttr(SmallVector<Attribute>(
                                        path.begin(), path.end()))),
       b.getNamedAttr("occurrence", source.declaration->getAttr("occurrence"))});
}
} // namespace acir::ac
