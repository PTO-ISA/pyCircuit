#include "HardwareSourceChecks.h"
#include "ACIRSourceContracts.h"
#include "HardwareSampledAdmission.h"
#include "HardwareSourceCheckPreflight.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/ScopeExit.h"
#include <algorithm>
#include <functional>
#include <limits>
#include <map>
#include <set>

using namespace mlir;
namespace acir::ac {
namespace {
// Author observers are distinguished from compiler-internal case equality by
// retained body provenance. Helpers are expanded into this definition before
// publication; the source-unit contract authenticates their actual bodies.
// Follow every use, including branch control, before any generic folding can
// remove a diagnostic/functional mixed use. Field paths and ordinary operation
// dependencies come from the same analysis as hardware scheduling.
LogicalResult verifyObserverUses(const HardwareAnalysis &analysis,
                                 ModuleOp definition,
                                 const HardwareBindings &bindings,
                                 detail::SourceCheckBudget &budget) {
  using Budget = detail::SourceCheckBudget;
  llvm::DenseMap<Value, SmallVector<FieldPath, 0>> seen;
  SmallVector<WireEndpoint, 0> pending;
  auto debit = [&](uint64_t units, Operation *site) {
    return budget.debit(units, site);
  };
  auto push = [&](Value value, ArrayRef<StringAttr> path,
                  Operation *site) -> LogicalResult {
    if (failed(debit(Budget::add(1, path.size()), site)))
      return failure();
    auto &paths = seen[value];
    for (const auto &prior : paths) {
      if (failed(debit(Budget::add(1, path.size()), site)))
        return failure();
      if (ArrayRef<StringAttr>(prior) == path)
        return success();
    }
    if (failed(
            debit(Budget::bytes(Budget::add(
                      4 * sizeof(WireEndpoint),
                      Budget::multiply(4 * sizeof(StringAttr), path.size()))),
                  site)))
      return failure();
    paths.emplace_back(path.begin(), path.end());
    pending.push_back({value, FieldPath(path.begin(), path.end())});
    return success();
  };
  auto results = [&](Operation *operation) -> LogicalResult {
    for (Value result : operation->getResults()) {
      auto paths =
          analysis.getFieldPaths(result.getType(), bindings, operation, debit);
      if (failed(paths))
        return failure();
      for (const auto &path : *paths)
        if (failed(push(result, path, operation)))
          return failure();
    }
    return success();
  };
  auto scan = definition.walk([&](Operation *operation) {
    if (failed(debit(1, operation)))
      return WalkResult::interrupt();
    auto marker = operation->getAttr("ac.verification_observer");
    if (!marker)
      return WalkResult::advance();
    auto name = dyn_cast<StringAttr>(marker);
    auto compare = dyn_cast<BitsCompareOp>(operation);
    auto origin = operation->getAttrOfType<DictionaryAttr>("ac.origin");
    if (!name ||
        (name.getValue() != "case_eq" && name.getValue() != "is_known") ||
        !compare || compare.getPredicate() != "case_eq" || !origin ||
        failed(detail::verifyOccurrence(
            origin, [&] { return operation->emitOpError(); }))) {
      operation->emitOpError() << "four-state observer requires authenticated "
                                  "source comparison provenance";
      return WalkResult::interrupt();
    }
    return failed(results(operation)) ? WalkResult::interrupt()
                                      : WalkResult::advance();
  });
  if (scan.wasInterrupted())
    return failure();
  auto reject = [](Operation *operation) -> LogicalResult {
    return operation->emitOpError()
           << "four-state observer is verification-only; its data or control "
              "dependency reaches a functional use";
  };
  auto diagnosticBranch = [&](Operation *branch) -> LogicalResult {
    auto checked = branch->walk([&](Operation *operation) {
      if (failed(debit(1, operation)))
        return WalkResult::interrupt();
      if (operation == branch)
        return WalkResult::advance();
      // SourceExpect's own verifier authenticates authored assertions and the
      // generated range/knownness obligations. They are all diagnostic sinks.
      if (isa<SourceExpectOp, SourceObserveOp, YieldOp, SourcePredicateOp>(
              operation) ||
          operation->getName().getStringRef() == "scf.yield" ||
          operation->getName().getStringRef() == "scf.if" ||
          isa<SourceBranchOp>(operation) ||
          detail::isCombinationalHardwareOperation(operation))
        return WalkResult::advance();
      (void)reject(operation);
      return WalkResult::interrupt();
    });
    if (checked.wasInterrupted())
      return failure();
    // An unchanged SSA yield in both arms has no control dependence.
    for (auto [index, result] : llvm::enumerate(branch->getResults())) {
      bool unchanged = branch->getNumRegions() == 2;
      Value common;
      for (Region &region : branch->getRegions()) {
        if (!region.hasOneBlock() || region.front().empty()) {
          unchanged = false;
          break;
        }
        Operation *terminator = region.front().getTerminator();
        if (index >= terminator->getNumOperands()) {
          unchanged = false;
          break;
        }
        Value yielded = terminator->getOperand(index);
        if (common && common != yielded)
          unchanged = false;
        common = yielded;
      }
      if (unchanged)
        continue;
      auto paths =
          analysis.getFieldPaths(result.getType(), bindings, branch, debit);
      if (failed(paths))
        return failure();
      for (const auto &path : *paths)
        if (failed(push(result, path, branch)))
          return failure();
    }
    return success();
  };
  // Memoize only direct dependency edges within this immutable invocation.
  // Common Struct/ValueMerge/Table rules preserve unrelated field authority.
  struct ResultEdges {
    WireEndpoint output;
    SmallVector<WireEndpoint> inputs;
  };
  llvm::DenseMap<Operation *, SmallVector<ResultEdges, 0>> edges;
  auto propagate = [&](Operation *operation,
                       const WireEndpoint &input) -> LogicalResult {
    auto found = edges.find(operation);
    if (found == edges.end()) {
      SmallVector<ResultEdges, 0> described;
      for (Value result : operation->getResults()) {
        auto paths = analysis.getFieldPaths(result.getType(), bindings,
                                            operation, debit);
        if (failed(paths))
          return failure();
        for (const auto &path : *paths) {
          auto dependencies =
              analysis.getDependencies(result, path, bindings, debit);
          if (failed(dependencies))
            return failure();
          uint64_t storage = sizeof(ResultEdges);
          for (const auto &endpoint : *dependencies)
            storage = Budget::add(
                storage, Budget::add(sizeof(WireEndpoint),
                                     Budget::multiply(endpoint.path.size(),
                                                      sizeof(StringAttr))));
          if (failed(debit(Budget::bytes(Budget::multiply(4, storage)),
                           operation)))
            return failure();
          described.push_back({{result, path}, std::move(*dependencies)});
        }
      }
      found = edges.try_emplace(operation, std::move(described)).first;
    }
    for (const auto &edge : found->second)
      for (const auto &dependency : edge.inputs) {
        if (failed(debit(Budget::add(1, dependency.path.size()), operation)))
          return failure();
        if (dependency == input &&
            failed(push(edge.output.value, edge.output.path, operation)))
          return failure();
      }
    return success();
  };
  while (!pending.empty()) {
    WireEndpoint endpoint = pending.pop_back_val();
    Value value = endpoint.value;
    for (OpOperand &use : value.getUses()) {
      Operation *user = use.getOwner();
      unsigned index = use.getOperandNumber();
      if (failed(debit(1, user)))
        return failure();
      if (isa<SourceExpectOp>(user)) {
        if (index < 2)
          continue;
        return reject(user);
      }
      if (isa<SourceObserveOp>(user))
        continue;
      if (isa<SourceBranchOp>(user) ||
          user->getName().getStringRef() == "scf.if") {
        if (failed(diagnosticBranch(user)))
          return failure();
        continue;
      }
      if (isa<RuleOp, SourceTransactionOp, SourceMethodOp>(user)) {
        unsigned argument = index;
        if (isa<SourceTransactionOp>(user)) {
          if (index < 2)
            return reject(user);
          argument -= 2;
        } else if (auto method = dyn_cast<SourceMethodOp>(user)) {
          // A Method declaration's operands bind lexical captures, not authored
          // hardware call arguments. Its regions put those after formals.
          argument += method.getFunctionType().getNumInputs();
        }
        for (Region &region : user->getRegions()) {
          if (!region.hasOneBlock() ||
              argument >= region.front().getNumArguments())
            return reject(user);
          if (failed(push(region.front().getArgument(argument), endpoint.path,
                          user)))
            return failure();
        }
        continue;
      }
      if (isa<YieldOp>(user) || user->getName().getStringRef() == "scf.yield") {
        Operation *owner = user->getParentOp();
        if (isa<ModuleOp, SourceMethodOp>(owner))
          return reject(user);
        if (auto transaction = dyn_cast<SourceTransactionOp>(owner))
          if (user->getParentRegion() == &transaction.getGuard())
            return reject(user);
        if (!isa<RuleOp, SourceTransactionOp, SourceBranchOp, TableMapOp,
                 TableMatchOp>(owner) &&
            owner->getName().getStringRef() != "scf.if")
          return reject(user);
        if (index >= owner->getNumResults())
          return reject(user);
        if (failed(push(owner->getResult(index), endpoint.path, user)))
          return failure();
        continue;
      }
      if (isa<SourcePredicateOp>(user)) {
        if (failed(push(user->getResult(0), {}, user)))
          return failure();
        continue;
      }
      // Index/admission/resource/call operands remain functional boundaries.
      if ((isa<TableGetOp>(user) && index != 0) || isa<TableIndexOp>(user))
        return reject(user);
      if (detail::isCombinationalHardwareOperation(user)) {
        if (failed(propagate(user, endpoint)))
          return failure();
        // The common dependency analysis specifies Table region bindings too.
        for (Region &region : user->getRegions())
          for (Block &block : region)
            for (BlockArgument argument : block.getArguments()) {
              auto paths = analysis.getFieldPaths(argument.getType(), bindings,
                                                  user, debit);
              if (failed(paths))
                return failure();
              for (const auto &path : *paths) {
                auto dependencies =
                    analysis.getDependencies(argument, path, bindings, debit);
                if (failed(dependencies))
                  return failure();
                for (const auto &dependency : *dependencies)
                  if (dependency == endpoint &&
                      failed(push(argument, path, user)))
                    return failure();
              }
            }
        continue;
      }
      return reject(user);
    }
  }
  return success();
}

LogicalResult predicate(const HardwareAnalysis &analysis, Value value,
                        const HardwareBindings &bindings, Operation *site) {
  if (!isa<BitsType>(value.getType()))
    return site->emitOpError() << "source check operand requires one-bit Bits";
  auto width = analysis.getPackedWidth(value.getType(), bindings, site);
  if (failed(width))
    return failure();
  if (*width != 1)
    return site->emitOpError() << "source check operand must have width one";
  return success();
}
FailureOr<Value> carrier(const HardwareAnalysis &analysis, Value value,
                         const HardwareBindings &bindings,
                         detail::SourceCheckBudget &budget) {
  llvm::DenseSet<Value> seen;
  while (auto extract = value.getDefiningOp<BitsExtractOp>()) {
    if (failed(budget.debit(1 + detail::SourceCheckBudget::bytes(sizeof(Value)),
                            extract)))
      return failure();
    if (!seen.insert(value).second)
      return extract.emitOpError() << "cyclic source check carrier association";
    if (failed(analysis.verifyResolvedOperation(extract, bindings)))
      return failure();
    auto low = analysis.evaluateStatic(extract.getLow(), bindings, extract);
    auto input = analysis.getPackedWidth(extract.getInput().getType(), bindings,
                                         extract);
    auto output = analysis.getPackedWidth(value.getType(), bindings, extract);
    if (failed(low) || failed(input) || failed(output))
      return failure();
    auto integer = dyn_cast<MathIntAttr>(*low);
    if (!integer || !llvm::APSInt(integer.getCanonicalValue()).isZero() ||
        *input != *output)
      break;
    value = extract.getInput();
  }
  return value;
}
// Debit every metadata node and string traversal. The iterative work stack
// retains capacity while visiting siblings, and prepays only actual growth.
LogicalResult payEffectMetadata(
    Attribute attribute, Operation *site,
    const std::function<LogicalResult(uint64_t, Operation *)> &debit) {
  using Budget = detail::SourceCheckBudget;
  auto pay = [&](uint64_t units) {
    return debit ? debit(units, site) : success();
  };
  if (failed(pay(1)))
    return failure();
  if (!attribute)
    return success();
  if (auto text = dyn_cast<StringAttr>(attribute))
    return pay(Budget::bytes(text.getValue().size()));
  if (!isa<ArrayAttr, DictionaryAttr>(attribute))
    return success();
  SmallVector<Attribute, 0> pending;
  auto push = [&](Attribute value) -> LogicalResult {
    if (failed(pay(1)))
      return failure();
    if (pending.size() == pending.capacity()) {
      uint64_t capacity = Budget::multiply(
          std::max(uint64_t(1), uint64_t(pending.capacity())), 2);
      uint64_t storage =
          Budget::multiply(capacity - pending.capacity(), sizeof(Attribute));
      if (capacity == std::numeric_limits<uint64_t>::max() ||
          storage == std::numeric_limits<uint64_t>::max() ||
          capacity > pending.max_size())
        return site->emitOpError()
               << "source effect metadata exceeds host stack capacity";
      if (failed(pay(Budget::bytes(storage))))
        return failure();
      pending.reserve(capacity);
    }
    pending.push_back(value);
    return success();
  };
  auto children = [&](Attribute value) -> LogicalResult {
    if (auto array = dyn_cast<ArrayAttr>(value)) {
      for (Attribute item : array)
        if (failed(push(item)))
          return failure();
    } else if (auto dictionary = dyn_cast<DictionaryAttr>(value)) {
      for (auto item : dictionary)
        if (failed(push(item.getName())) || failed(push(item.getValue())))
          return failure();
    }
    return success();
  };
  if (failed(children(attribute)))
    return failure();
  while (!pending.empty()) {
    Attribute value = pending.pop_back_val();
    if (auto text = dyn_cast_or_null<StringAttr>(value)) {
      if (failed(pay(Budget::bytes(text.getValue().size()))))
        return failure();
    } else if (failed(children(value)))
      return failure();
  }
  return success();
}
// Only statement arms extend a callable body. Other region owners (including
// guards and pure helpers) never acquire source-effect placement by ancestry.
Operation *sourceEffectCallable(Operation *operation) {
  Region *region = operation->getParentRegion();
  Operation *parent = operation->getParentOp();
  while (auto branch = dyn_cast_or_null<SourceBranchOp>(parent)) {
    if (branch->getNumRegions() != 2)
      return nullptr;
    if (region != &branch.getThenRegion() && region != &branch.getElseRegion())
      return nullptr;
    region = branch->getParentRegion();
    parent = branch->getParentOp();
  }
  return isa_and_nonnull<SourceMethodOp, SourceTransactionOp>(parent) &&
                 parent->getNumRegions() == 2 && region == &parent->getRegion(1)
             ? parent
             : nullptr;
}
int compareCheckWithinRegistration(DictionaryAttr lhs, DictionaryAttr rhs) {
  for (StringRef key : {"check", "obligation"})
    if (int order =
            detail::compareClosedSourceStructure(lhs.get(key), rhs.get(key)))
      return order;
  return 0;
}
} // namespace
LogicalResult
detail::verifySourceObservationStructure(SourceObserveOp observe) {
  auto error = [&] { return observe.emitOpError(); };
  auto kind = observe->getAttrOfType<StringAttr>("kind");
  auto spec = observe->getAttrOfType<DictionaryAttr>("spec");
  auto *parent = sourceEffectCallable(observe);
  bool source = parent != nullptr;
  if (!source)
    parent = observe->getParentOp();
  if (source) {
    if (!kind ||
        !llvm::is_contained(ArrayRef<StringRef>{"log", "report"},
                            kind.getValue()) ||
        observe->hasAttr("ac.observation_id") ||
        observe->hasAttr("ac.check_id"))
      return error()
             << "source observation requires statement Method/transaction "
                "body placement, log/report kind and no final identity";
    if (failed(verifySourceEffectMetadata(
            observe->getAttrOfType<DictionaryAttr>("occurrence"),
            observe->getAttrOfType<DictionaryAttr>("ac.location"),
            parent->getParentOp(), error)))
      return failure();
  } else {
    if (!isa<ModuleOp>(parent) || !kind ||
        !llvm::is_contained(ArrayRef<StringRef>{"print", "log", "report"},
                            kind.getValue()))
      return error() << "observation requires module placement and known kind";
    if (failed(verifyObservationID(
            observe->getAttrOfType<DictionaryAttr>("ac.observation_id"),
            error)) ||
        failed(verifySourceSpan(
            observe->getAttrOfType<DictionaryAttr>("ac.location"), error)))
      return failure();
  }
  if (observe->getNumOperands() == 0 ||
      !isa<BitsType, TypeParamType>(observe->getOperand(0).getType()))
    return error() << "observation path requires scalar Bits";
  HardwareBindings bindings;
  bindings.owner = source ? parent->getParentOp() : parent;
  if (source &&
      failed(predicate(
          HardwareAnalysis(observe->getParentOfType<mlir::ModuleOp>()),
          observe->getOperand(0), bindings, observe)))
    return failure();
  for (auto operand : observe->getOperands().drop_front())
    if (!isa<BitsType, EnumType, TypeParamType>(operand.getType()))
      return error() << "observation value requires a scalar hardware payload";
  return verifySourceObservationSpec(kind, spec, observe->getNumOperands() - 1,
                                     error);
}

LogicalResult detail::verifySourceObservationSpec(StringAttr kind,
                                                  DictionaryAttr spec,
                                                  uint64_t valueCount,
                                                  EmitError error) {
  if (!kind || !spec)
    return error() << "observation requires a known kind and closed spec";
  auto text = [&](StringRef key, bool nonempty = false) {
    auto value = spec.getAs<StringAttr>(key);
    return value && (!nonempty || !value.getValue().empty());
  };
  if (kind.getValue() == "report") {
    if (spec.size() != 1 || !text("name", true) || valueCount != 1)
      return error() << "report requires a nonempty name and one scalar value";
    return success();
  }
  if (spec.size() != 3 ||
      (kind.getValue() == "print" && (!text("sep") || !text("end"))) ||
      (kind.getValue() == "log" &&
       (!text("event", true) || !text("level") ||
        !llvm::is_contained(
            ArrayRef<StringRef>{"debug", "info", "warning", "error"},
            spec.getAs<StringAttr>("level").getValue()))))
    return error() << "observation spec has invalid closed fields";
  auto items = spec.getAs<ArrayAttr>("items");
  if (!items)
    return error() << "observation items must be an array";
  uint64_t next = 0;
  for (auto raw : items) {
    auto item = dyn_cast<DictionaryAttr>(raw);
    auto itemKind = item ? item.getAs<StringAttr>("kind") : StringAttr();
    if (!item || item.size() != 2 || !itemKind)
      return error() << "observation item requires exact kind and payload";
    if (itemKind.getValue() == "literal") {
      if (!item.getAs<StringAttr>("text"))
        return error() << "literal observation item requires text";
    } else if (itemKind.getValue() == "value") {
      auto ordinal = decodeU32(item.getAs<IntegerAttr>("ordinal"),
                               "observation item ordinal", error);
      if (failed(ordinal) || *ordinal != next++)
        return error()
               << "observation items must cover values exactly in order";
    } else {
      return error() << "observation item kind must be literal or value";
    }
  }
  if (next != valueCount)
    return error() << "observation items must cover values exactly in order";
  return success();
}

LogicalResult detail::verifyModuleObservationIdentities(ModuleOp definition) {
  SmallVector<ObservationLocalKey, 0> keys;
  for (auto observe : definition.getBody().front().getOps<SourceObserveOp>()) {
    if (failed(verifySourceObservationStructure(observe)))
      return failure();
    auto identity = observe->getAttrOfType<DictionaryAttr>("ac.observation_id");
    ObservationLocalKey key{identity, identity.getAs<DictionaryAttr>("owner"),
                            observe.getKind() == "report"
                                ? observe.getSpec().getAs<StringAttr>("name")
                                : StringAttr()};
    for (const auto &prior : keys) {
      if (prior.identity == key.identity)
        return observe.emitOpError() << "duplicate full ObservationID";
      if (key.reportName && prior.reportName == key.reportName &&
          prior.owner == key.owner)
        return observe.emitOpError()
               << "report name must be unique within logical owner: "
               << key.reportName.getValue();
    }
    keys.push_back(key);
  }
  return success();
}

#include "HardwareTransactionReturns.inc"

LogicalResult detail::verifyRoutedRegistration(
    const HardwareAnalysis &analysis, RuleOp rule,
    const std::function<LogicalResult(uint64_t, Operation *)> &debit) {
  auto error = [&] { return rule.emitOpError(); };
  auto pay = [&](uint64_t units) {
    return debit ? debit(units, rule) : success();
  };
  auto metadata = [&](Attribute value) {
    return payEffectMetadata(value, rule, debit);
  };
  auto path = rule->getAttrOfType<ArrayAttr>("ac.registration_path");
  auto original = rule->getAttrOfType<DictionaryAttr>("ac.lexical_registration");
  auto lexical = rule->getAttrOfType<FlatSymbolRefAttr>("ac.registration_owner");
  auto physical = dyn_cast<ModuleOp>(rule->getParentOp());
  if (!physical || !rule->hasAttr("ac.sampled_admission") || !path ||
      path.empty() || path.size() > 128 || !original || !lexical)
    return error() << "routed registration requires its complete lexical identity";
  for (Attribute value : {Attribute(path), Attribute(original),
                         Attribute(lexical), rule->getAttr("occurrence")})
    if (failed(metadata(value)))
      return failure();
  if (failed(verifyOccurrence(original, error)))
    return failure();
  Builder builder(rule.getContext());
  SmallVector<Attribute> expansion, prefix, invocation;
  ModuleOp current = physical;
  auto appendExpansion = [&](DictionaryAttr occurrence,
                             ModuleOp callee) -> LogicalResult {
    auto frames = occurrence.getAs<ArrayAttr>("expansion");
    if (failed(pay(SourceCheckBudget::multiply(frames.size() + 1,
                                               1 + sizeof(Attribute)))))
      return failure();
    llvm::append_range(expansion, frames);
    expansion.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("kind", builder.getStringAttr("call")),
        builder.getNamedAttr("site", occurrence.get("site")),
        builder.getNamedAttr("callee",
                             FlatSymbolRefAttr::get(callee.getSymNameAttr()))}));
    return success();
  };
  for (Attribute raw : path) {
    auto occurrence = dyn_cast<DictionaryAttr>(raw);
    if (!occurrence || failed(verifyOccurrence(occurrence, error)) ||
        failed(verifyOriginDefinition(
            occurrence, FlatSymbolRefAttr::get(current.getSymNameAttr()),
            "routed registration allocation", error)))
      return failure();
    InstanceOp selected;
    for (auto instance : current.getBody().front().getOps<InstanceOp>()) {
      if (failed(pay(1)) || failed(metadata(instance.getOccurrence())))
        return failure();
      if (instance.getOccurrence() != occurrence)
        continue;
      if (selected)
        return error() << "routed registration allocation is ambiguous";
      selected = instance;
    }
    auto child = selected
                     ? dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(selected))
                     : ModuleOp();
    if (!child || !child.getBody().hasOneBlock() ||
        !analysis.getPrimitiveKind(child).empty())
      return error() << "routed registration path must identify ordinary instances";
    if (failed(appendExpansion(occurrence, child)) ||
        failed(pay(SourceCheckBudget::multiply(prefix.size() + 1, 4))))
      return failure();
    prefix.push_back(occurrence);
    invocation.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("call", occurrence),
        builder.getNamedAttr("receiver", builder.getDictionaryAttr({
            builder.getNamedAttr("instance_path", builder.getArrayAttr(prefix))}))}));
    current = child;
  }
  if (lexical.getValue() != current.getSymName() ||
      failed(verifyOriginDefinition(original, lexical,
                                    "routed lexical registration", error)) ||
      failed(appendExpansion(original, current)))
    return error() << "routed registration lexical owner differs from its allocation";
  auto expected = builder.getDictionaryAttr({
      builder.getNamedAttr("site", cast<DictionaryAttr>(path[0]).get("site")),
      builder.getNamedAttr("expansion", builder.getArrayAttr(expansion))});
  if (rule->getAttr("occurrence") != expected)
    return error() << "routed registration identity differs from its lexical path";

  // Check and observation identities must use the same lexical prefix. The
  // ordinary effect-owner proof subsequently resolves every remaining frame
  // and its physical domain independently.
  SmallVector<Attribute> pending;
  for (StringRef key : {"ac.sampled_admission", "ac.required_checks"})
    if (Attribute attr = rule->getAttr(key)) {
      if (failed(metadata(attr)))
        return failure();
      pending.push_back(attr);
    }
  while (!pending.empty()) {
    Attribute attr = pending.pop_back_val();
    if (failed(pay(1)))
      return failure();
    if (auto dict = dyn_cast<DictionaryAttr>(attr)) {
      if (Attribute raw = dict.get("invocation")) {
        auto frames = dyn_cast<ArrayAttr>(raw);
        if (!frames || frames.size() < invocation.size() ||
            !llvm::equal(frames.getValue().take_front(invocation.size()), invocation))
          return error() << "routed invocation omits or changes its lexical allocation prefix";
      }
      if (failed(pay(SourceCheckBudget::multiply(dict.size(), 2))))
        return failure();
      for (NamedAttribute field : dict)
        pending.push_back(field.getValue());
    } else if (auto array = dyn_cast<ArrayAttr>(attr)) {
      if (failed(pay(SourceCheckBudget::multiply(array.size(), 2))))
        return failure();
      llvm::append_range(pending, array);
    }
  }
  return success();
}

FailureOr<detail::ResolvedSourceEffectOwner> detail::resolveSourceEffectOwner(
    const HardwareAnalysis &analysis, ModuleOp physical,
    const HardwareBindings &physicalBindings, DictionaryAttr registration,
    DictionaryAttr site, ArrayAttr invocation, DictionaryAttr location,
    Operation *diagnosticSite,
    const std::function<LogicalResult(uint64_t, Operation *)> &debit) {
  using Budget = SourceCheckBudget;
  auto pay = [&](uint64_t cost, Operation *operation) {
    return debit ? debit(cost, operation) : success();
  };
  auto metadata = [&](Attribute attr) {
    return payEffectMetadata(attr, diagnosticSite, debit);
  };
  auto error = [&] { return diagnosticSite->emitOpError(); };
  for (Attribute attr : {Attribute(registration), Attribute(site),
                         Attribute(invocation), Attribute(location)})
    if (failed(metadata(attr)))
      return failure();
  if (!physical || physicalBindings.owner != physical ||
      physical->getParentOp() != analysis.getPackage().getOperation() ||
      !physical.getBody().hasOneBlock())
    return error()
           << "source effect bindings require its analyzed physical Module";
  if (failed(verifyOccurrence(registration, error)) ||
      failed(verifyOccurrence(site, error)) ||
      failed(verifySourceSpan(location, error)) || !invocation)
    return failure();
  for (Attribute raw : invocation) {
    auto frame = dyn_cast<DictionaryAttr>(raw);
    if (!frame || frame.size() != 2 ||
        failed(verifyOccurrence(frame.getAs<DictionaryAttr>("call"), error)) ||
        failed(verifyOwnerRef(frame.getAs<DictionaryAttr>("receiver"), error)))
      return error()
             << "source effect invocation requires exact call/receiver frames";
  }
  if (failed(pay(Budget::add(Budget::bytes(sizeof(ResolvedSourceEffectOwner)),
                             Budget::bindings(physicalBindings)),
                 diagnosticSite)))
    return failure();
  ResolvedSourceEffectOwner result{physical, physicalBindings, {}};
  auto origin = [&](DictionaryAttr occurrence, ModuleOp module,
                    StringRef description) -> LogicalResult {
    if (failed(metadata(occurrence)))
      return failure();
    return verifyOriginDefinition(
        occurrence, FlatSymbolRefAttr::get(module.getSymNameAttr()),
        description, [&] { return diagnosticSite->emitOpError(); });
  };
  if (failed(origin(registration, physical, "source effect registration")))
    return failure();
  // Each receiver is relative to the physical routing definition. A statically
  // bound Module reference may cross to a sibling; the preceding frame still
  // authenticates the call's lexical origin before the new path is resolved.
  // Restart at the routing root so aliases and distinct allocations keep their
  // actual identity rather than being interpreted as a suffix of the caller.
  auto descend = [&](ArrayAttr path) -> LogicalResult {
    if (failed(pay(Budget::bindings(physicalBindings), diagnosticSite)))
      return failure();
    result.terminal = physical;
    result.bindings = physicalBindings;
    result.path.clear();
    for (size_t index = 0; index < path.size(); ++index) {
      InstanceOp found;
      for (auto &op : result.terminal.getBody().front()) {
        if (!isa<InstanceOp, CollectionOp>(op))
          continue;
        if (failed(pay(1, &op)) || failed(metadata(op.getAttr("occurrence"))))
          return failure();
        if (failed(
                verifyOccurrence(op.getAttrOfType<DictionaryAttr>("occurrence"),
                                 [&] { return op.emitOpError(); })))
          return failure();
        if (op.getAttr("occurrence") != path[index])
          continue;
        if (isa<CollectionOp>(op) || found)
          return diagnosticSite->emitOpError()
                 << "source effect owner requires unique ordinary instances";
        found = cast<InstanceOp>(op);
      }
      auto child =
          found ? dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(found))
                : ModuleOp();
      if (!child || !child.getBody().hasOneBlock() ||
          !analysis.getPrimitiveKind(child).empty())
        return diagnosticSite->emitOpError()
               << "source effect owner path must resolve ordinary definitions";
      if (failed(origin(found->getAttrOfType<DictionaryAttr>("occurrence"),
                        result.terminal, "source effect owner allocation")))
        return failure();
      if (failed(pay(Budget::add(Budget::bytes(sizeof(InstanceOp)),
                                 Budget::bindings(result.bindings)),
                     diagnosticSite)))
        return failure();
      auto bindings = analysis.bindInstance(found, result.bindings);
      if (failed(bindings))
        return failure();
      if (failed(pay(Budget::bindings(*bindings), diagnosticSite)))
        return failure();
      result.path.push_back(found);
      result.terminal = child;
      result.bindings = std::move(*bindings);
    }
    return success();
  };
  for (auto raw : invocation) {
    auto frame = cast<DictionaryAttr>(raw);
    if (failed(origin(frame.getAs<DictionaryAttr>("call"), result.terminal,
                      "source effect invocation call")) ||
        failed(descend(frame.getAs<DictionaryAttr>("receiver")
                           .getAs<ArrayAttr>("instance_path"))))
      return failure();
  }
  if (failed(origin(site, result.terminal, "source effect site")))
    return failure();
  auto sourceOwner =
      result.terminal->getAttrOfType<DictionaryAttr>("source_owner");
  if (failed(metadata(sourceOwner)) ||
      failed(verifySourceOwner(sourceOwner,
                               [&] { return diagnosticSite->emitOpError(); })))
    return failure();
  if (location.getAs<StringAttr>("path") !=
      sourceOwner.getAs<StringAttr>("path"))
    return diagnosticSite->emitOpError()
           << "source effect span must match logical owner SourceOwner";
  return std::move(result);
}

FailureOr<detail::ResolvedSourceEffectOwner> detail::resolveObservationOwner(
    const HardwareAnalysis &analysis, SourceObserveOp observe,
    const HardwareBindings &physicalBindings,
    const std::function<LogicalResult(uint64_t, Operation *)> &debit) {
  using Budget = SourceCheckBudget;
  auto pay = [&](uint64_t cost) {
    return debit ? debit(cost, observe) : success();
  };
  // Pay actual terminal shape/spec validation before using its typed identity.
  for (StringRef key : {"ac.observation_id", "ac.location", "spec", "kind"})
    if (failed(payEffectMetadata(observe->getAttr(key), observe, debit)))
      return failure();
  if (failed(pay(
          Budget::add(8, Budget::multiply(2, observe->getNumOperands())))) ||
      failed(verifySourceObservationStructure(observe)) ||
      failed(predicate(analysis, observe->getOperand(0), physicalBindings,
                       observe)))
    return failure();
  auto id = observe->getAttrOfType<DictionaryAttr>("ac.observation_id");
  return resolveSourceEffectOwner(
      analysis, dyn_cast<ModuleOp>(observe->getParentOp()), physicalBindings,
      id.getAs<DictionaryAttr>("registration"),
      id.getAs<DictionaryAttr>("site"), id.getAs<ArrayAttr>("invocation"),
      observe->getAttrOfType<DictionaryAttr>("ac.location"), observe, debit);
}

FailureOr<SmallVector<HardwareCheckBinding>>
detail::definitionSourceChecks(const HardwareAnalysis &analysis,
                               ModuleOp definition,
                               const HardwareBindings &bindings,
                               detail::SampledAdmissionValidation &validation,
                               detail::SourceCheckBudget &budget) {
  if (failed(validation.verifyInvocation(analysis, budget)))
    return failure();
  if (!definition)
    return analysis.getPackage().emitOpError()
           << "source checks require a Module definition";
  if (definition->getNumRegions() != 1 || !definition.getBody().hasOneBlock())
    return definition.emitOpError()
           << "source checks require one definition graph block";
  if (failed(verifyObserverUses(analysis, definition, bindings, budget)))
    return failure();
  SmallVector<HardwareCheckBinding> result;
  // Keep registration ordering once per registration. IDs within a group
  // have already proved that registration and compare only the remaining
  // semantic fields. Integer widths/signedness still use the closed-source
  // comparator rather than Attribute pointer identity.
  struct CheckLess {
    bool operator()(DictionaryAttr lhs, DictionaryAttr rhs) const {
      return compareCheckWithinRegistration(lhs, rhs) < 0;
    }
  };
  struct RegistrationLess {
    bool operator()(DictionaryAttr lhs, DictionaryAttr rhs) const {
      return detail::compareClosedSourceStructure(lhs, rhs) < 0;
    }
  };
  using CheckMap = std::map<DictionaryAttr, size_t, CheckLess>;
  struct CheckGroup {
    CheckMap identities;
    RuleOp sampled;
    SmallVector<size_t, 0> direct;
  };
  std::map<DictionaryAttr, CheckGroup, RegistrationLess> registrations;
  llvm::DenseMap<Operation *, detail::SampledAdmissionView> sampledRules;
  auto prepared = validation.prepare(definition, bindings);
  if (failed(prepared))
    return failure();
  if (definition->hasAttr("ac.transaction_wiring")) {
    auto wiring = validation.wiring(definition, bindings);
    if (failed(wiring))
      return failure();
  }
  for (auto rule : definition.getBody().front().getOps<RuleOp>()) {
    auto error = [&] { return rule.emitOpError(); };
    if (failed(mlir::verify(rule)) ||
        failed(detail::verifyOccurrence(rule.getOccurrence(), error)))
      return failure();
    auto required = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
    if (rule->hasAttr("ac.required_checks") && !required)
      return error() << "ac.required_checks must be an array";
    if (rule->hasAttr("ac.sampled_admission")) {
      auto sampled = validation.verify(rule, bindings);
      if (failed(sampled))
        return failure();
      if (!required || required.size() != detail::SourceCheckBudget::add(
                                              sampled->controls.size(),
                                              sampled->bodyChecks.size()))
        return error() << "sampled Rule requires exactly its ordered "
                          "guard/assert obligations";
      if (failed(budget.debit(
              detail::SourceCheckBudget::bytes(
                  sizeof(std::pair<Operation *, detail::SampledAdmissionView>)),
              rule)))
        return failure();
      sampledRules.try_emplace(rule, std::move(*sampled));
    }
    if (!required)
      continue;
    if (required.size() > rule.getNumResults() / 2)
      return error()
             << "source check suffix requires two results per obligation";
    uint64_t first = rule.getNumResults() - 2 * required.size();
    // Sampled Rules may append private state-entry witnesses after checks.
    // The authenticated inventory, rather than the total result count, owns
    // the check base in that case.
    if (auto sampled = sampledRules.find(rule); sampled != sampledRules.end())
      first = sampled->second.checkBase;
    CheckGroup *group = nullptr;
    if (!required.empty()) {
      auto [position, inserted] =
          registrations.try_emplace(rule.getOccurrence());
      group = &position->second;
      if (inserted && sampledRules.count(rule)) {
        group->sampled = rule;
        if (failed(budget.debit(
                detail::SourceCheckBudget::multiply(
                    required.size(),
                    detail::SourceCheckBudget::bytes(sizeof(size_t))),
                rule)))
          return failure();
        group->direct.reserve(required.size());
      } else if (group->sampled) {
        // Several Rules may share a semantic registration. Such groups use
        // the general structural map, preserving cross-Rule duplicate checks
        // and the same global ordering as ordinary Rules.
        for (size_t index : group->direct)
          group->identities.emplace(result[index].checkID, index);
        group->direct.clear();
        group->sampled = {};
      }
    }
    for (auto [index, raw] : llvm::enumerate(required)) {
      auto record = dyn_cast<DictionaryAttr>(raw);
      auto id = record ? record.getAs<DictionaryAttr>("id") : DictionaryAttr();
      auto kind = record ? record.getAs<StringAttr>("kind") : StringAttr();
      auto location =
          record ? record.getAs<DictionaryAttr>("location") : DictionaryAttr();
      if (!record || record.size() != 3 || !kind ||
          !llvm::is_contained(
              ArrayRef<StringRef>{"assert", "range", "guard_known"},
              kind.getValue()))
        return error() << "RequiredCheck requires exact id, kind and location";
      if (failed(detail::verifyCheckID(id, error)) ||
          failed(detail::verifySourceSpan(location, error)))
        return failure();
      if (detail::compareClosedSourceStructure(id.get("registration"),
                                               rule.getOccurrence()) != 0)
        return error() << "RequiredCheck registration must equal owning rule "
                          "occurrence";
      uint64_t conditionResult = first + 2 * index;
      uint64_t pathResult = conditionResult + 1;
      auto sampled = sampledRules.find(rule);
      if (kind.getValue() == "guard_known") {
        if (sampled == sampledRules.end())
          return error() << "guard_known requires sampled admission";
        if (index >= sampled->second.controls.size())
          return error() << "guard_known is outside sampled control inventory";
        const auto &guard = sampled->second.controls[index];
        if (id != guard.id || conditionResult != guard.rawResult ||
            pathResult != guard.reachedResult)
          return error() << "guard_known differs from sampled control suffix";
      } else if (sampled != sampledRules.end()) {
        uint64_t controlCount = sampled->second.controls.size();
        if (kind.getValue() != "assert" || index < controlCount ||
            index - controlCount >= sampled->second.bodyChecks.size())
          return error() << "sampled admission only permits ordered "
                            "guard_known/assert checks";
        const auto &body = sampled->second.bodyChecks[index - controlCount];
        if (id != body.id || conditionResult != body.conditionResult ||
            pathResult != body.pathResult)
          return error() << "assert differs from sampled body check suffix";
      }
      if (failed(budget.debit(detail::SourceCheckBudget::bytes(
                                  sizeof(HardwareCheckBinding) +
                                  sizeof(std::pair<DictionaryAttr, size_t>) +
                                  3 * sizeof(void *)),
                              rule)))
        return failure();
      if (group->sampled) {
        // validation.verify above proves each ID's obligation equals its
        // RequiredCheck row ordinal, so this is an exact, duplicate-free key.
        group->direct.push_back(result.size());
      } else if (!group->identities.emplace(id, result.size()).second)
        return error() << "duplicate full source CheckID";
      if (failed(predicate(analysis, rule.getResult(conditionResult), bindings,
                           rule)) ||
          failed(
              predicate(analysis, rule.getResult(pathResult), bindings, rule)))
        return failure();
      result.push_back({definition,
                        rule,
                        {},
                        id,
                        kind,
                        location,
                        {},
                        {},
                        {},
                        index,
                        conditionResult,
                        pathResult});
    }
  }
  for (auto expect : definition.getBody().front().getOps<SourceExpectOp>()) {
    if (failed(detail::verifySourceExpectOperation(analysis, expect, bindings)))
      return failure();
    auto id = expect->getAttrOfType<DictionaryAttr>("ac.check_id");
    auto registration =
        registrations.find(id.getAs<DictionaryAttr>("registration"));
    if (registration == registrations.end())
      return expect.emitOpError()
             << "orphan source check without RequiredCheck owner";
    auto &group = registration->second;
    size_t foundIndex;
    if (group.sampled) {
      auto ordinal = detail::decodeU64(id.getAs<IntegerAttr>("obligation"),
                                       "CheckID obligation",
                                       [&] { return expect.emitOpError(); });
      if (failed(ordinal))
        return failure();
      if (*ordinal >= group.direct.size() ||
          compareCheckWithinRegistration(
              id, result[group.direct[*ordinal]].checkID) != 0)
        return expect.emitOpError()
               << "orphan source check without RequiredCheck owner";
      foundIndex = group.direct[*ordinal];
    } else {
      auto position = group.identities.find(id);
      if (position == group.identities.end())
        return expect.emitOpError()
               << "orphan source check without RequiredCheck owner";
      foundIndex = position->second;
    }
    auto &found = result[foundIndex];
    if (found.expect)
      return expect.emitOpError() << "duplicate expect for source CheckID";
    if (found.kind != expect.getKindAttr() ||
        found.location != expect.getLocationAttr())
      return expect.emitOpError()
             << "expect kind/location differs from RequiredCheck";
    auto condition = carrier(analysis, expect.getCondition(), bindings, budget);
    auto path = carrier(analysis, expect.getPath(), bindings, budget);
    if (failed(condition) || failed(path))
      return failure();
    if (*condition != found.rule.getResult(found.conditionResult) ||
        *path != found.rule.getResult(found.pathResult))
      return expect.emitOpError()
             << "expect operands must match owning rule check suffix";
    if (found.kind.getValue() == "guard_known") {
      auto sampled = sampledRules.find(found.rule);
      if (sampled == sampledRules.end())
        return expect.emitOpError() << "guard_known has no sampled Rule";
      detail::HardwareIdentityStopPolicy stops{
          detail::HardwareIdentityTraversal::DefinitionLocal,
          (*prepared)->getBoundaries()};
      const Value captures[] = {sampled->second.clock, sampled->second.reset};
      for (auto [actual, capture] : llvm::zip(expect.getSampling(), captures)) {
        auto lhs = detail::makeIdentityEndpoint(actual, (*prepared)->getScope(),
                                                analysis, budget);
        auto rhs = detail::makeIdentityEndpoint(
            capture, (*prepared)->getScope(), analysis, budget);
        if (failed(lhs) || failed(rhs))
          return failure();
        auto normalizedActual =
            detail::normalizeIdentityEndpoint(*lhs, stops, analysis, budget);
        auto normalizedCapture =
            detail::normalizeIdentityEndpoint(*rhs, stops, analysis, budget);
        if (failed(normalizedActual) || failed(normalizedCapture))
          return failure();
        if (normalizedActual->value != normalizedCapture->value ||
            normalizedActual->resolvedType != normalizedCapture->resolvedType)
          return expect.emitOpError()
                 << "expect sampling pins differ from owning Rule captures";
      }
    }
    found.expect = expect;
    found.condition = expect.getCondition();
    found.path = expect.getPath();
    found.message = expect->getAttrOfType<StringAttr>("ac.message");
  }
  for (const auto &binding : result)
    if (!binding.expect)
      return definition.emitOpError() << "RequiredCheck has no matching expect";
  // Reuse the structural map order instead of sorting/recomparing the IDs.
  SmallVector<HardwareCheckBinding> ordered;
  if (failed(budget.debit(detail::SourceCheckBudget::multiply(
                              result.size(), detail::SourceCheckBudget::bytes(
                                                 sizeof(HardwareCheckBinding))),
                          definition)))
    return failure();
  ordered.reserve(result.size());
  // Structural order establishes registration ordering and uniqueness. Within
  // each sampled registration replay the authenticated RequiredCheck order.
  llvm::DenseSet<Operation *> emitted;
  if (failed(budget.debit(
          detail::SourceCheckBudget::multiply(
              sampledRules.size(),
              detail::SourceCheckBudget::bytes(sizeof(Operation *))),
          definition)))
    return failure();
  for (const auto &[registration, group] : registrations) {
    if (group.sampled) {
      for (size_t index : group.direct)
        ordered.push_back(result[index]);
      continue;
    }
    for (const auto &[id, index] : group.identities) {
      const auto &binding = result[index];
      if (!sampledRules.count(binding.rule)) {
        ordered.push_back(binding);
        continue;
      }
      if (!emitted.insert(binding.rule).second)
        continue;
      const auto &sampled = sampledRules.find(binding.rule)->second;
      uint64_t count = detail::SourceCheckBudget::add(
          sampled.controls.size(), sampled.bodyChecks.size());
      if (index < binding.requiredIndex)
        return definition.emitOpError()
               << "sampled check ordering lost its first obligation";
      uint64_t first = index - binding.requiredIndex;
      if (first > result.size() || count > result.size() - first)
        return definition.emitOpError()
               << "sampled check ordering exceeds catalog";
      for (uint64_t ordinal = 0; ordinal < count; ++ordinal) {
        if (failed(budget.debit(1, definition)))
          return failure();
        const auto &found = result[first + ordinal];
        if (found.rule != binding.rule || found.requiredIndex != ordinal)
          return definition.emitOpError()
                 << "sampled check ordering lost an obligation";
        ordered.push_back(found);
      }
    }
  }
  return ordered;
}
bool detail::sameHardwareBindings(const HardwareBindings &lhs,
                                  const HardwareBindings &rhs) {
  if (lhs.owner != rhs.owner || lhs.integers.size() != rhs.integers.size() ||
      lhs.types.size() != rhs.types.size())
    return false;
  for (const auto &entry : lhs.integers) {
    auto found = rhs.integers.find(entry.first());
    if (found == rhs.integers.end() || found->second != entry.second)
      return false;
  }
  for (const auto &entry : lhs.types) {
    auto found = rhs.types.find(entry.first());
    if (found == rhs.types.end() || found->second != entry.second)
      return false;
  }
  return true;
}
namespace detail {
LogicalResult verifySourceExpectOperation(const HardwareAnalysis &analysis,
                                          SourceExpectOp expect,
                                          const HardwareBindings &bindings) {
  auto error = [&] { return expect.emitOpError(); };
  auto *parent = sourceEffectCallable(expect);
  bool source = parent != nullptr;
  if (!source)
    parent = expect->getParentOp();
  if (source) {
    if (expect.getKind() != "assert" || expect->hasAttr("ac.check_id") ||
        expect->hasAttr("ac.observation_id") || !expect.getSampling().empty())
      return error()
             << "source assertion requires statement Method/transaction "
                "body placement, assert kind, no sampling and no final "
                "identity";
    if (failed(verifySourceEffectMetadata(
            expect->getAttrOfType<DictionaryAttr>("occurrence"),
            expect.getLocationAttr(), parent->getParentOp(), error)))
      return failure();
  } else {
    if (!isa<ModuleOp>(parent) ||
        !llvm::is_contained(
            ArrayRef<StringRef>{"assert", "range", "guard_known"},
            expect.getKind()))
      return error() << "check requires module placement and known kind";
    if (failed(verifyCheckID(
            expect->getAttrOfType<DictionaryAttr>("ac.check_id"), error)) ||
        failed(verifySourceSpan(expect.getLocationAttr(), error)))
      return failure();
  }
  if (expect->hasAttr("ac.message") &&
      !expect->getAttrOfType<StringAttr>("ac.message"))
    return error() << "source check message must be a static string";
  if (expect.getSampling().size() !=
      (expect.getKind() == "guard_known" ? 2u : 0u))
    return error() << "guard_known requires clock/reset sampling operands; "
                      "assert/range require no sampling operands";
  for (Value pin : expect.getSampling())
    if (failed(predicate(analysis, pin, bindings, expect)))
      return failure();
  if (failed(predicate(analysis, expect.getCondition(), bindings, expect)) ||
      failed(predicate(analysis, expect.getPath(), bindings, expect)))
    return failure();
  return success();
}
FailureOr<HardwareBindings>
resolveHardwareRootBindings(const HardwareAnalysis &analysis) {
  auto package = analysis.getPackage();
  auto systems = package.getOps<SystemOp>();
  if (!llvm::hasSingleElement(systems))
    return package.emitOpError()
           << "executable hardware package requires exactly one ac.system";
  auto system = *systems.begin();
  auto entry = system.getEntry();
  auto callee = entry.getAs<FlatSymbolRefAttr>("callee");
  auto root = callee ? dyn_cast_or_null<ModuleOp>(
                           analysis.lookupDefinition(callee.getValue()))
                     : ModuleOp();
  if (!root)
    return system.emitOpError()
           << "system entry must resolve to module definition";
  HardwareBindings bindings;
  bindings.owner = root;
  auto parameters = entry.getAs<ArrayAttr>("parameters");
  auto types = entry.getAs<ArrayAttr>("type_arguments");
  if (!parameters)
    parameters = ArrayAttr::get(package.getContext(), {});
  if (!types)
    types = ArrayAttr::get(package.getContext(), {});
  if (types.size() != root.getTypeParameters().size() ||
      parameters.size() > root.getParameters().size())
    return system.emitOpError() << "system entry parameter arity mismatch";
  for (auto [name, type] : llvm::zip(root.getTypeParameters(), types)) {
    auto attr = dyn_cast<TypeAttr>(type);
    if (!attr)
      return system.emitOpError() << "system type argument must be TypeAttr";
    bindings.types[cast<StringAttr>(name).getValue()] = attr.getValue();
    if (failed(analysis.getPackedWidth(attr.getValue(), {}, system)))
      return failure();
  }
  for (auto [index, raw] : llvm::enumerate(root.getParameters())) {
    auto formal = cast<DictionaryAttr>(raw);
    auto expr = index < parameters.size()
                    ? dyn_cast<StaticExprAttr>(parameters[index])
                    : formal.getAs<StaticExprAttr>("default");
    if (!expr)
      return system.emitOpError() << "missing root integer parameter";
    auto value = analysis.evaluateStatic(expr, bindings, system);
    if (failed(value))
      return failure();
    if (!isa<MathIntAttr>(*value))
      return system.emitOpError()
             << "root integer parameter requires math integer";
    bindings.integers[formal.getAs<StringAttr>("name").getValue()] = *value;
  }
  return bindings;
}
} // namespace detail

thread_local detail::NativeSourceCheckScope
    *detail::NativeSourceCheckScope::active = nullptr;

detail::NativeSourceCheckScope::NativeSourceCheckScope(
    const HardwareAnalysis &analysis, SampledAdmissionValidation &validation,
    SourceCheckBudget &budget, Definitions &definitions)
    : previous(active), analysis(analysis), validation(validation),
      budget(budget), definitions(definitions) {
  active = this;
}
detail::NativeSourceCheckScope::~NativeSourceCheckScope() { active = previous; }

std::optional<LogicalResult>
detail::NativeSourceCheckScope::verify(ModuleOp definition) {
  if (!active || !definition ||
      definition->getParentOp() != active->analysis.getPackage().getOperation())
    return std::nullopt;
  auto &scope = *active;
  if (failed(scope.budget.debit(1, definition)))
    return failure();
  // No transform can run inside this synchronous MLIR verification scope.
  // This catalog holds current default-binding checks, never an edited-IR or
  // occurrence verdict; bound occurrence checks still use their existing owner.
  if (scope.definitions.contains(definition))
    return success();
  HardwareBindings bindings;
  bindings.owner = definition;
  auto checks = definitionSourceChecks(scope.analysis, definition, bindings,
                                       scope.validation, scope.budget);
  if (failed(checks) ||
      failed(scope.budget.debit(
          SourceCheckBudget::bytes(sizeof(Definitions::value_type)),
          definition)))
    return failure();
  scope.definitions.try_emplace(definition, std::move(*checks));
  return success();
}

LogicalResult
HardwareAnalysis::verifyDefinitionSourceChecks(ModuleOp definition) const {
  if (!definition || definition->getParentOp() != package.getOperation())
    return package.emitOpError()
           << "source check definition must belong to analyzed package";
  if (auto shared = detail::NativeSourceCheckScope::verify(definition))
    return *shared;
  HardwareBindings bindings;
  bindings.owner = definition;
  detail::SourceCheckBudget budget(HardwareSourceCheckLimits{});
  budget.setPhase("definition source checks");
  detail::SampledAdmissionValidation validation(*this, budget);
  return success(succeeded(detail::definitionSourceChecks(
      *this, definition, bindings, validation, budget)));
}

FailureOr<HardwareSourceCheckPlan>
HardwareAnalysis::getSourceCheckPlan(HardwareSourceCheckLimits limits) const {
  return detail::buildSourceCheckPlan(*this, limits);
}

LogicalResult HardwareAnalysis::verifySourceCheckPlan(
    const HardwareSourceCheckPlan &plan,
    HardwareSourceCheckLimits limits) const {
  if (!package)
    return failure();
  // Recompute from current SSA, attributes and bindings; no saved plan grants
  // authority to an edited package or a removed check/commit endpoint.
  HardwareAnalysis current(package);
  auto fresh = current.getSourceCheckPlan(limits);
  if (failed(fresh))
    return failure();
  auto mismatch = [&] {
    return package.emitOpError()
           << "source check plan differs from current hardware package";
  };
  if (plan.resetSemantics != fresh->resetSemantics ||
      plan.bindings.size() != fresh->bindings.size() ||
      plan.owners.size() != fresh->owners.size() ||
      plan.checks.size() != fresh->checks.size() ||
      plan.commits.size() != fresh->commits.size() ||
      plan.samplingBindings.size() != fresh->samplingBindings.size() ||
      plan.samplingOccurrences.size() != fresh->samplingOccurrences.size())
    return mismatch();
  for (auto [lhs, rhs] : llvm::zip(plan.bindings, fresh->bindings))
    if (lhs.definition != rhs.definition || lhs.rule != rhs.rule ||
        lhs.expect != rhs.expect || lhs.checkID != rhs.checkID ||
        lhs.kind != rhs.kind || lhs.location != rhs.location ||
        lhs.message != rhs.message || lhs.condition != rhs.condition ||
        lhs.path != rhs.path || lhs.requiredIndex != rhs.requiredIndex ||
        lhs.conditionResult != rhs.conditionResult ||
        lhs.pathResult != rhs.pathResult ||
        lhs.samplingBindingIndex != rhs.samplingBindingIndex)
      return mismatch();
  for (auto [lhs, rhs] : llvm::zip(plan.owners, fresh->owners)) {
    if (lhs.definition != rhs.definition || lhs.parent != rhs.parent ||
        lhs.ordinal != rhs.ordinal ||
        !detail::sameHardwareBindings(lhs.bindings, rhs.bindings) ||
        lhs.path.size() != rhs.path.size())
      return mismatch();
    for (auto [left, right] : llvm::zip(lhs.path, rhs.path))
      if (left.allocation != right.allocation ||
          left.coordinates != right.coordinates)
        return mismatch();
  }
  for (auto [lhs, rhs] : llvm::zip(plan.checks, fresh->checks))
    if (lhs.bindingIndex != rhs.bindingIndex ||
        lhs.ownerOccurrence != rhs.ownerOccurrence ||
        lhs.ordinal != rhs.ordinal || lhs.reset.kind != rhs.reset.kind ||
        lhs.reset.ownerOccurrence != rhs.reset.ownerOccurrence ||
        lhs.reset.inputOrdinal != rhs.reset.inputOrdinal ||
        lhs.samplingOccurrenceIndex != rhs.samplingOccurrenceIndex)
      return mismatch();
  for (auto [lhs, rhs] : llvm::zip(plan.commits, fresh->commits))
    if (lhs.ownerOccurrence != rhs.ownerOccurrence ||
        lhs.allocation != rhs.allocation ||
        lhs.coordinates != rhs.coordinates ||
        lhs.primitiveKind != rhs.primitiveKind || lhs.form != rhs.form)
      return mismatch();
  for (auto [lhs, rhs] :
       llvm::zip(plan.samplingBindings, fresh->samplingBindings))
    if (lhs.definition != rhs.definition || lhs.clock != rhs.clock ||
        lhs.reset != rhs.reset || lhs.localOrdinal != rhs.localOrdinal)
      return mismatch();
  for (auto [lhs, rhs] :
       llvm::zip(plan.samplingOccurrences, fresh->samplingOccurrences))
    if (lhs.bindingIndex != rhs.bindingIndex ||
        lhs.ownerOccurrence != rhs.ownerOccurrence ||
        lhs.ordinal != rhs.ordinal)
      return mismatch();
  return success();
}

FailureOr<HardwarePhysicalProjection> HardwareAnalysis::getPhysicalProjection(
    const HardwareSourceCheckPlan &plan,
    HardwareSourceCheckLimits limits) const {
  if (failed(verifySourceCheckPlan(plan, limits)))
    return failure();
  if (plan.owners.empty())
    return package.emitOpError() << "physical RTL requires a root owner";
  auto root = plan.owners.front().definition;
  auto kind = root->getAttrOfType<StringAttr>("ac.root_kind");
  if (kind && kind.getValue() == "system")
    return root.emitOpError()
           << "physical RTL requires an ordinary Module, not a closed System";
  auto domain = root->getAttrOfType<DictionaryAttr>("ac.domain_inputs");
  auto clock = domain ? domain.getAs<IntegerAttr>("clock") : IntegerAttr();
  auto reset = domain ? domain.getAs<IntegerAttr>("reset") : IntegerAttr();
  auto layout = getModulePortLayout(root);
  if (failed(layout))
    return failure();
  if (!clock || !reset || clock.getValue().getActiveBits() > 32 ||
      reset.getValue().getActiveBits() > 32 || clock == reset ||
      clock.getValue().getZExtValue() >= layout->sourceInputCount ||
      reset.getValue().getZExtValue() >= layout->sourceInputCount)
    return root.emitOpError()
           << "physical RTL requires one actual root clock/reset pair";
  unsigned clk = clock.getValue().getZExtValue(),
           rst = reset.getValue().getZExtValue();
  detail::SourceCheckBudget budget(limits);
  budget.setPhase("physical RTL projection");
  // Identity-only transport is sufficient for domain proof. Uniform Tables
  // preserve identity through Collection slicing and layout permutations.
  // Derived expressions deliberately have no domain identity.
  std::function<FailureOr<unsigned>(Value, uint64_t, unsigned)> identity;
  identity = [&](Value value, uint64_t owner,
                 unsigned depth) -> FailureOr<unsigned> {
    auto &occurrence = plan.owners[owner];
    Operation *site = value.getDefiningOp();
    if (!site)
      site = value.getParentBlock()->getParentOp();
    if (depth > 256 || failed(budget.debit(1, site)))
      return site->emitOpError()
             << "physical domain identity exceeds bounded traversal";
    if (auto argument = dyn_cast<BlockArgument>(value)) {
      auto *parent = argument.getOwner()->getParentOp();
      if (auto rule = dyn_cast<RuleOp>(parent))
        return identity(rule->getOperand(argument.getArgNumber()), owner,
                        depth + 1);
      if (parent == occurrence.definition) {
        if (!occurrence.parent)
          return argument.getArgNumber() == clk   ? 1u
                 : argument.getArgNumber() == rst ? 2u
                                                  : 0u;
        auto *allocation = occurrence.path.back().allocation;
        return identity(allocation->getOperand(argument.getArgNumber()),
                        *occurrence.parent, depth + 1);
      }
      return 0u;
    }
    if (auto result = dyn_cast<OpResult>(value)) {
      if (auto rule = dyn_cast<RuleOp>(result.getOwner()))
        return identity(cast<YieldOp>(rule.getBody().front().back())
                            .getValues()[result.getResultNumber()],
                        owner, depth + 1);
    }
    if (auto extract = value.getDefiningOp<BitsExtractOp>()) {
      auto low = evaluateStatic(extract.getLow(), occurrence.bindings, extract);
      auto inWidth = getPackedWidth(extract.getInput().getType(),
                                    occurrence.bindings, extract);
      auto outWidth =
          getPackedWidth(value.getType(), occurrence.bindings, extract);
      if (failed(low) || failed(inWidth) || failed(outWidth))
        return failure();
      auto integer = dyn_cast<MathIntAttr>(*low);
      if (integer && llvm::APSInt(integer.getCanonicalValue()).isZero() &&
          *inWidth == *outWidth)
        return identity(extract.getInput(), owner, depth + 1);
    }
    if (auto splat = value.getDefiningOp<TableSplatOp>())
      return identity(splat.getInput(), owner, depth + 1);
    if (auto create = value.getDefiningOp<TableCreateOp>()) {
      unsigned uniform = 0;
      for (Value element : create->getOperands()) {
        auto part = identity(element, owner, depth + 1);
        if (failed(part))
          return failure();
        if (!*part || (uniform && uniform != *part))
          return 0u;
        uniform = *part;
      }
      return uniform;
    }
    return 0u;
  };
  auto pair = [&](Value clockValue, Value resetValue, uint64_t owner,
                  Operation *site) -> LogicalResult {
    auto c = identity(clockValue, owner, 0), r = identity(resetValue, owner, 0);
    if (failed(c) || failed(r))
      return failure();
    if (*c != 1 || *r != 2)
      return site->emitOpError()
             << "physical RTL requires all sampling and storage to use the "
                "actual root clock/reset pair";
    return success();
  };
  llvm::DenseSet<Operation *> reachable;
  for (const auto &owner : plan.owners)
    reachable.insert(owner.definition);
  for (const auto &check : plan.bindings)
    if (reachable.contains(check.definition) &&
        check.kind.getValue() != "guard_known")
      return check.expect->emitOpError()
             << "physical RTL requires managed execution for residual "
             << check.kind.getValue() << " checks";
  for (const auto &sampling : plan.samplingOccurrences) {
    const auto &binding = plan.samplingBindings[sampling.bindingIndex];
    if (failed(pair(binding.clock, binding.reset, sampling.ownerOccurrence,
                    binding.definition)))
      return failure();
  }
  for (auto [owner, occurrence] : llvm::enumerate(plan.owners)) {
    auto definition = occurrence.definition;
    auto observations = definition.getBody().front().getOps<SourceObserveOp>();
    if (!observations.empty())
      return definition.emitOpError()
             << "physical RTL does not support observations";
    // Include stateless and readonly transactions even if no storage/check
    // occurrence happens to retain their domain in the managed plan.
    for (auto rule : definition.getBody().front().getOps<RuleOp>()) {
      if (!rule->hasAttr("ac.sampled_admission"))
        continue;
      auto view = detail::parseSampledAdmission(rule, *this,
                                                occurrence.bindings, budget);
      if (failed(view) || failed(pair(view->clock, view->reset, owner, rule)))
        return failure();
      for (const auto &control : view->controls)
        if (control.kind != detail::SampledControlKind::CallableGuard &&
            control.kind != detail::SampledControlKind::BranchSelect)
          return rule.emitOpError() << "physical RTL requires managed "
                                       "execution for non-guard controls";
    }
    for (Operation &op : definition.getBody().front()) {
      if (!isa<InstanceOp, CollectionOp>(op))
        continue;
      auto *callee = isa<InstanceOp>(op)
                         ? resolveCallee(cast<InstanceOp>(op))
                         : resolveCallee(cast<CollectionOp>(op));
      auto primitive = getPrimitiveKind(callee);
      if (primitive.empty())
        continue;
      if (primitive != "dff" && primitive != "dffe")
        return op.emitOpError()
               << "physical RTL supports only DFF/DFFE storage primitives";
      if (op.getNumOperands() < 2 ||
          failed(pair(op.getOperand(0), op.getOperand(1), owner, &op)))
        return failure();
      unsigned initialIndex = primitive == "dff" ? 3 : 4;
      if (op.getNumOperands() <= initialIndex ||
          failed(detail::verifyClosedStateInitializer(
              *this, op.getOperand(initialIndex), occurrence.bindings, &op)))
        return failure();
    }
    auto uses = definition.walk([&](Operation *op) {
      for (auto [index, operand] : llvm::enumerate(op->getOperands())) {
        auto id = identity(operand, owner, 0);
        if (failed(id))
          return WalkResult::interrupt();
        if (*id != 1)
          continue;
        bool transport = isa<TableSplatOp>(op);
        if (auto create = dyn_cast<TableCreateOp>(op)) {
          auto result = identity(create.getResult(), owner, 0);
          if (failed(result))
            return WalkResult::interrupt();
          transport = *result == 1;
        }
        if (auto extract = dyn_cast<BitsExtractOp>(op)) {
          auto result = identity(extract.getResult(), owner, 0);
          if (failed(result))
            return WalkResult::interrupt();
          transport = *result == 1;
        }
        if (auto rule = dyn_cast<RuleOp>(op)) {
          auto metadata =
              rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
          auto capture = metadata ? metadata.getAs<IntegerAttr>("clock_capture")
                                  : IntegerAttr();
          transport = capture && index == capture.getValue().getZExtValue();
        }
        if (isa<InstanceOp, CollectionOp>(op)) {
          auto *callee = isa<InstanceOp>(op)
                             ? resolveCallee(cast<InstanceOp>(op))
                             : resolveCallee(cast<CollectionOp>(op));
          auto primitive = getPrimitiveKind(callee);
          // Ordinary child arguments are checked in their actual occurrence.
          transport =
              primitive.empty() ||
              ((primitive == "dff" || primitive == "dffe") && index == 0);
        }
        if (auto expect = dyn_cast<SourceExpectOp>(op))
          transport = expect.getKind() == "guard_known" && index == 2;
        if (!transport) {
          op->emitOpError() << "physical RTL forbids clock use as data or "
                               "combinational control";
          return WalkResult::interrupt();
        }
      }
      return WalkResult::advance();
    });
    if (uses.wasInterrupted())
      return failure();
  }
  return HardwarePhysicalProjection{root, clk, rst};
}
} // namespace acir::ac
