#include "HardwareEmitCommon.h"
#include "../Dialect/ACIR/ACIRSourceContracts.h"
#include "../Dialect/ACIR/HardwareSourceChecks.h"
#include "FinalEmissionNames.h"
#include "mlir/IR/Builders.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/Path.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <functional>
#include <limits>
#include <tuple>
using namespace mlir;
namespace acir::compiler {
namespace {
FailureOr<std::string> identifier(StringRef raw, Operation *site) {
  auto value = legalizeIdentifier(raw, [&] { return site->emitOpError(); });
  if (failed(value))
    return failure();
  return *value;
}
FailureOr<std::string> qualified(StringRef raw, Operation *site,
                                 StringRef separator) {
  SmallVector<StringRef> parts;
  raw.split(parts, '.', -1, true);
  std::string output;
  for (auto part : parts) {
    if (part.empty())
      return site->emitOpError() << "empty hardware symbol component";
    auto item = identifier(part, site);
    if (failed(item))
      return failure();
    if (!output.empty())
      output += separator.str();
    output += *item;
  }
  return output;
}
bool closedStatic(Attribute attribute, const ac::HardwareBindings &bindings,
                  const HardwareEmitContext *context = nullptr,
                  Operation *site = nullptr) {
  if (auto expression = dyn_cast_or_null<ac::StaticExprAttr>(attribute))
    return closedStatic(expression.getTree(), bindings, context, site);
  if (auto dictionary = dyn_cast_or_null<DictionaryAttr>(attribute)) {
    auto kind = dictionary.getAs<StringAttr>("kind");
    if (context && kind && kind.getValue() == "select") {
      auto condition = dictionary.getAs<ac::StaticExprAttr>("condition");
      if (closedStatic(condition, bindings, context, site)) {
        auto value =
            context->analysis.evaluateStatic(condition, bindings, site);
        if (succeeded(value))
          if (auto boolean = dyn_cast<BoolAttr>(*value))
            return closedStatic(dictionary.getAs<ac::StaticExprAttr>(
                                    boolean.getValue() ? "yes" : "no"),
                                bindings, context, site);
      }
    }
    if (context && kind && kind.getValue() == "binary") {
      auto opcode = dictionary.getAs<StringAttr>("operator");
      if (opcode &&
          (opcode.getValue() == "and_bool" || opcode.getValue() == "or_bool")) {
        auto lhs = dictionary.getAs<ac::StaticExprAttr>("lhs");
        if (closedStatic(lhs, bindings, context, site)) {
          auto value = context->analysis.evaluateStatic(lhs, bindings, site);
          if (succeeded(value))
            if (auto boolean = dyn_cast<BoolAttr>(*value))
              if ((opcode.getValue() == "and_bool" && !boolean.getValue()) ||
                  (opcode.getValue() == "or_bool" && boolean.getValue()))
                return true;
        }
      }
    }
    if (kind && kind.getValue() == "reference") {
      auto ref = dictionary.getAs<DictionaryAttr>("ref");
      if (!bindings.owner || !ref || !ref.getAs<FlatSymbolRefAttr>("owner") ||
          ref.getAs<FlatSymbolRefAttr>("owner").getValue() !=
              SymbolTable::getSymbolName(bindings.owner).getValue())
        return false;
      auto name = ref.getAs<StringAttr>("name");
      auto found = name ? bindings.integers.find(name.getValue())
                        : bindings.integers.end();
      return found != bindings.integers.end() &&
             closedStatic(found->second, {}, context, site);
    }
    if (kind && kind.getValue() == "type_width") {
      auto type = dictionary.getAs<TypeAttr>("type");
      if (!type)
        return false;
      Type value = type.getValue();
      if (auto formal = dyn_cast<ac::TypeParamType>(value)) {
        if (!bindings.owner ||
            formal.getOwner().getValue() !=
                SymbolTable::getSymbolName(bindings.owner).getValue())
          return false;
        auto found = bindings.types.find(formal.getName().getValue());
        if (found == bindings.types.end() ||
            isa<ac::TypeParamType>(found->second))
          return false;
        value = found->second;
      }
      if (auto bits = dyn_cast<ac::BitsType>(value))
        return closedStatic(bits.getWidth(), bindings, context, site);
      return isa<ac::StructType, ac::EnumType>(value);
    }
    for (auto field : dictionary)
      if (!closedStatic(field.getValue(), bindings, context, site))
        return false;
  }
  if (auto array = dyn_cast_or_null<ArrayAttr>(attribute))
    for (auto item : array)
      if (!closedStatic(item, bindings, context, site))
        return false;
  return true;
}
FailureOr<std::string>
staticTree(HardwareEmitContext const &context, DictionaryAttr tree,
           Operation *site, bool rtl,
           const ac::HardwareBindings *bindings = nullptr) {
  auto kind = tree.getAs<StringAttr>("kind");
  if (!kind)
    return site->emitOpError() << "static expression has no kind";
  StringRef tag = kind.getValue();
  ac::HardwareBindings empty;
  auto &scope = bindings ? *bindings : empty;
  if (closedStatic(tree, scope, &context, site)) {
    auto value = context.analysis.evaluateStatic(
        ac::StaticExprAttr::get(tree.getContext(), tree), scope, site);
    if (failed(value))
      return failure();
    if (auto integer = dyn_cast<ac::MathIntAttr>(*value))
      return integer.getCanonicalValue().str();
    if (auto boolean = dyn_cast<BoolAttr>(*value))
      return std::string(boolean.getValue() ? "1" : "0");
  }

  if (tag == "literal") {
    auto data = tree.getAs<DictionaryAttr>("value");
    auto value = data ? data.get("value") : Attribute();
    if (auto integer = dyn_cast_or_null<ac::MathIntAttr>(value))
      return integer.getCanonicalValue().str();
    if (auto boolean = dyn_cast_or_null<BoolAttr>(value))
      return std::string(boolean.getValue() ? "1" : "0");
    return site->emitOpError() << "unsupported static literal";
  }
  if (tag == "reference") {
    auto ref = tree.getAs<DictionaryAttr>("ref");
    auto name = ref ? ref.getAs<StringAttr>("name") : StringAttr();
    if (!name)
      return site->emitOpError() << "static reference has no name";
    if (bindings && bindings->owner && ref.getAs<FlatSymbolRefAttr>("owner") &&
        ref.getAs<FlatSymbolRefAttr>("owner").getValue() ==
            SymbolTable::getSymbolName(bindings->owner).getValue()) {
      auto found = bindings->integers.find(name.getValue());
      if (found != bindings->integers.end()) {
        if (auto integer = dyn_cast<ac::MathIntAttr>(found->second))
          return integer.getCanonicalValue().str();
        if (auto expression = dyn_cast<ac::StaticExprAttr>(found->second))
          return staticTree(context, expression.getTree(), site, rtl, bindings);
      }
    }
    return context.rtlFormalName(name.getValue(), site);
  }
  if (tag == "type_width") {
    auto type = tree.getAs<TypeAttr>("type");
    if (!type)
      return site->emitOpError() << "type_width has no type";
    auto payload = type.getValue();
    if (bindings) {
      auto resolved = context.analysis.resolveType(payload, *bindings, site);
      if (failed(resolved))
        return failure();
      payload = *resolved;
    }
    auto rendered = context.rtlType(payload, site);
    if (failed(rendered))
      return failure();
    return "$bits(" + *rendered + ")";
  }
  if (tag == "select") {
    auto a = staticTree(context,
                        tree.getAs<ac::StaticExprAttr>("condition").getTree(),
                        site, rtl, bindings);
    auto b =
        staticTree(context, tree.getAs<ac::StaticExprAttr>("yes").getTree(),
                   site, rtl, bindings);
    auto c = staticTree(context, tree.getAs<ac::StaticExprAttr>("no").getTree(),
                        site, rtl, bindings);
    if (failed(a) || failed(b) || failed(c))
      return failure();
    return "((" + *a + ") ? (" + *b + ") : (" + *c + "))";
  }
  auto opcode = tree.getAs<StringAttr>("operator");
  if (!opcode)
    return site->emitOpError() << "static operator is absent";
  auto symbol = llvm::StringSwitch<StringRef>(opcode.getValue())
                    .Case("neg", "-")
                    .Case("invert", "~")
                    .Case("not", "!")
                    .Case("add", "+")
                    .Case("sub", "-")
                    .Case("mul", "*")
                    .Case("floordiv", "/")
                    .Case("mod", "%")
                    .Case("and_bits", "&")
                    .Case("or_bits", "|")
                    .Case("xor_bits", "^")
                    .Case("shl", "<<")
                    .Case("shr", ">>")
                    .Case("eq", "==")
                    .Case("ne", "!=")
                    .Case("lt", "<")
                    .Case("le", "<=")
                    .Case("gt", ">")
                    .Case("ge", ">=")
                    .Case("and_bool", "&&")
                    .Case("or_bool", "||")
                    .Default("");
  if (opcode.getValue() == "to_int")
    symbol = "+";
  if (symbol.empty())
    return site->emitOpError() << "unsupported static operator";
  if (tag == "unary") {
    auto a =
        staticTree(context, tree.getAs<ac::StaticExprAttr>("operand").getTree(),
                   site, rtl, bindings);
    if (failed(a))
      return failure();
    if (opcode.getValue() == "to_int")
      return *a;
    auto operand =
        rtl ? "$signed(64'( " + *a + " ))" : "std::int64_t(" + *a + ")";
    return "(" + symbol.str() + "(" + operand + "))";
  }
  if (tag != "binary")
    return site->emitOpError() << "unsupported static expression";
  auto a = staticTree(context, tree.getAs<ac::StaticExprAttr>("lhs").getTree(),
                      site, rtl, bindings);
  auto b = staticTree(context, tree.getAs<ac::StaticExprAttr>("rhs").getTree(),
                      site, rtl, bindings);
  if (failed(a) || failed(b))
    return failure();
  auto signedValue = [&](const std::string &value) {
    return rtl ? "$signed(64'(" + value + "))" : "std::int64_t(" + value + ")";
  };
  if (opcode.getValue() == "floordiv" || opcode.getValue() == "mod")
    return (rtl ? "pycircuit_types::pyc_" : "pyc_") + opcode.getValue().str() +
           "(" + signedValue(*a) + ", " + signedValue(*b) + ")";
  if (opcode.getValue() == "shl")
    return rtl ? "(" + signedValue(*a) + " <<< " + signedValue(*b) + ")"
               : "(" + signedValue(*a) + " * (std::int64_t{1} << " +
                     signedValue(*b) + "))";
  if (opcode.getValue() == "shr")
    return (rtl ? "(" + signedValue(*a) + " >>> " + signedValue(*b) + ")"
                : "pyc_floordiv(" + signedValue(*a) + ", (std::int64_t{1} << " +
                      signedValue(*b) + "))");
  return "(" + signedValue(*a) + " " + symbol.str() + " " + signedValue(*b) +
         ")";
}

// ---------------------------------------------------------------------------
// Native observation plan.
//
// `ac.observe` is already produced by the source importers; this plan is the
// only thing native emission adds. It turns every *occurrence* of every
// `ac.observe` site into one `gfsim::ObservationDescriptor` and records the
// slot arithmetic the generated `Work()` needs.
//
// Layout. For a module definition D let S(D) be the number of descriptors one
// lane of D contributes and W(D) = S(D) + sum(shape(c) * W(c)) over its child
// occurrences, i.e. the number of descriptors one lane of D contributes
// together with its whole instance subtree. An occurrence O with L lanes owns
// the contiguous block [base, base + L*W(D)). Lane `l` of O stages site `s`
// into `base + l*S(D) + s`, and the i-th child occurrence of D starts at
// `base + L * (S(D) + sum over j < i of shape_j * W(D_j))`. Every occurrence of
// a definition therefore shares one generated code path, and the parent hands
// its children a runtime base; no occurrence identity is baked into a shared
// class.
//
// Descriptor order. `ObservationSlots::Configure` requires the table to be
// strictly increasing in (ownerKey, registrationKey, siteKey, stableOrdinal,
// kind). Keys are assigned in exactly the block order above: `ownerKey`
// advances per (occurrence, lane) and `stableOrdinal` is the slot index, so the
// generated order is already sorted and stays globally unique.
// ---------------------------------------------------------------------------
FailureOr<ac::ModuleOp> observationRoot(HardwareEmitContext &context) {
  auto systems = context.package.getOps<ac::SystemOp>();
  if (!llvm::hasSingleElement(systems))
    return context.package.emitOpError() << "one system required";
  auto system = *systems.begin();
  auto callee = system.getEntry().getAs<FlatSymbolRefAttr>("callee");
  auto root = callee ? dyn_cast_or_null<ac::ModuleOp>(
                           context.analysis.lookupDefinition(callee.getValue()))
                     : ac::ModuleOp();
  if (!root)
    return system.emitOpError() << "root module is unresolved";
  return root;
}
FailureOr<ObservationEmissionPlan>
buildObservationPlan(HardwareEmitContext &context) {
  ObservationEmissionPlan plan;
  // Full hardware validation is performed by prepareImpl before emission.
  // Without observations, no descriptor geometry needs to be instantiated.
  bool hasObservations = false;
  context.package.walk([&](ac::SourceObserveOp) { hasObservations = true; });
  if (!hasObservations)
    return plan;
  auto root = observationRoot(context);
  if (failed(root))
    return failure();
  llvm::DenseMap<Attribute, uint64_t> registrationKeys;
  llvm::DenseMap<Attribute, uint64_t> identityKeys;
  uint64_t nextSiteKey = 0;
  llvm::SetVector<Operation *> reached;
  std::function<LogicalResult(ac::ModuleOp, const ac::HardwareBindings &)>
      collect = [&](ac::ModuleOp definition,
                    const ac::HardwareBindings &bindings) -> LogicalResult {
    if (!reached.insert(definition))
      return success();
    if (failed(ac::detail::verifyModuleObservationIdentities(definition)))
      return failure();
    struct Group {
      ac::SourceObserveOp observe;
      uint64_t registrationRank, ordinal, identityRank, valueCount;
    };
    SmallVector<Group> groups;
    SmallVector<ObservationEmissionSite> sites;
    for (Operation &op : definition.getBody().front()) {
      auto observe = dyn_cast<ac::SourceObserveOp>(op);
      if (!observe)
        continue;
      auto identity = op.getAttrOfType<DictionaryAttr>("ac.observation_id");
      auto key = [&](Attribute value,
                     llvm::DenseMap<Attribute, uint64_t> &table) {
        auto found = table.find(value);
        if (found == table.end())
          found = table.try_emplace(value, table.size()).first;
        return found->second;
      };
      uint64_t registrationKey =
          key(identity.get("registration"), registrationKeys);
      uint64_t identityKey = key(identity, identityKeys);
      groups.push_back({observe, registrationKey,
                        static_cast<uint64_t>(
                            identity.getAs<IntegerAttr>("ordinal").getInt()),
                        identityKey,
                        std::max<std::size_t>(1, observe.getValues().size())});
    }
    llvm::sort(groups, [](const Group &a, const Group &b) {
      return std::tie(a.registrationRank, a.ordinal, a.identityRank) <
             std::tie(b.registrationRank, b.ordinal, b.identityRank);
    });
    for (const auto &group : groups) {
      auto observe = group.observe;
      uint64_t siteKey = nextSiteKey++;
      // A site with no hardware value operand still publishes once, which is
      // the same placeholder rule the source importer applies to a
      // literal-only `log`.
      bool gauge = observe.getKind() == "report";
      for (uint64_t index = 0; index < group.valueCount; ++index)
        sites.push_back({observe, static_cast<unsigned>(index),
                         index < observe.getValues().size(), gauge,
                         group.registrationRank, siteKey});
    }
    plan.definitions[definition] = std::move(sites);
    const auto &parent = bindings;
    for (Operation &op : definition.getBody().front()) {
      if (!isa<ac::InstanceOp, ac::CollectionOp>(op))
        continue;
      auto *callee =
          isa<ac::InstanceOp>(op)
              ? context.analysis.resolveCallee(cast<ac::InstanceOp>(op))
              : context.analysis.resolveCallee(cast<ac::CollectionOp>(op));
      auto child = dyn_cast_or_null<ac::ModuleOp>(callee);
      uint64_t shape = 1;
      if (auto collection = dyn_cast<ac::CollectionOp>(op)) {
        auto extents =
            context.analysis.resolveShape(collection.getShape(), parent, &op);
        if (failed(extents))
          return failure();
        for (uint64_t extent : *extents) {
          if (extent != 0 &&
              shape > std::numeric_limits<uint64_t>::max() / extent)
            return op.emitOpError()
                   << "native observation lane count overflows";
          shape *= extent;
        }
      }
      auto bindings =
          isa<ac::InstanceOp>(op)
              ? context.analysis.bindInstance(cast<ac::InstanceOp>(op), parent)
              : context.analysis.bindInstance(cast<ac::CollectionOp>(op),
                                              parent);
      if (failed(bindings))
        return failure();
      auto *site = &plan.children[definition].emplace_back(
          ObservationEmissionChild{&op, child, shape});
      (void)site;
      if (child &&
          failed(collect(child, cast<ac::HardwareBindings>(*bindings))))
        return failure();
    }
    return success();
  };
  auto rootBindings = context.rootBindings();
  if (failed(rootBindings) || failed(collect(*root, *rootBindings)))
    return failure();
  llvm::DenseSet<Operation *> active;
  std::function<FailureOr<uint64_t>(Operation *)> width =
      [&](Operation *definition) -> FailureOr<uint64_t> {
    if (!definition)
      return uint64_t{0};
    if (auto found = plan.subtreeWidth.find(definition);
        found != plan.subtreeWidth.end())
      return found->second;
    if (!active.insert(definition).second)
      return definition->emitOpError()
             << "recursive instantiation is not supported by native "
                "observation emission";
    uint64_t total = plan.definitions[definition].size();
    for (const ObservationEmissionChild &child : plan.children[definition]) {
      auto nested = width(child.definition);
      if (failed(nested))
        return failure();
      if (child.shape != 0 &&
          *nested > std::numeric_limits<uint64_t>::max() / child.shape)
        return definition->emitOpError()
               << "native observation descriptor count overflows";
      uint64_t block = child.shape * *nested;
      if (block > std::numeric_limits<uint64_t>::max() - total)
        return definition->emitOpError()
               << "native observation descriptor count overflows";
      total += block;
    }
    active.erase(definition);
    plan.subtreeWidth[definition] = total;
    return total;
  };
  if (failed(width(*root)))
    return failure();
  uint64_t ownerKey = 0;
  using InstancePath = SmallVector<std::pair<Operation *, uint64_t>>;
  std::function<LogicalResult(ac::ModuleOp, ArrayRef<InstancePath>,
                              const ac::HardwareBindings &)>
      layout = [&](ac::ModuleOp definition, ArrayRef<InstancePath> paths,
                   const ac::HardwareBindings &bindings) -> LogicalResult {
    uint64_t lanes = paths.size();
    auto sites = plan.definitions.find(definition);
    uint64_t units = sites == plan.definitions.end() ? 0 : sites->second.size();
    llvm::DenseMap<Operation *, SmallVector<ac::InstanceOp>> logicalSuffixes;
    if (sites != plan.definitions.end())
      for (const auto &site : sites->second) {
        if (logicalSuffixes.contains(site.op))
          continue;
        auto logical = ac::detail::resolveObservationOwner(
            context.analysis, cast<ac::SourceObserveOp>(site.op), bindings, {});
        if (failed(logical))
          return failure();
        logicalSuffixes.try_emplace(site.op, std::move(logical->path));
      }
    for (uint64_t lane = 0; lane < lanes; ++lane) {
      if (sites != plan.definitions.end())
        for (const ObservationEmissionSite &site : sites->second) {
          if (plan.table.size() >= std::numeric_limits<std::uint32_t>::max())
            return definition.emitOpError()
                   << "native observation ordinal exceeds u32";
          InstancePath logicalPath(paths[lane]);
          for (auto instance : logicalSuffixes.find(site.op)->second)
            logicalPath.push_back({instance, 0});
          plan.occurrences.push_back(
              {site, plan.table.size(), paths[lane], std::move(logicalPath)});
          plan.table.push_back({plan.table.size(), ownerKey,
                                site.registrationKey, site.siteKey,
                                site.gauge ? 1u : 0u});
          if (!site.gauge)
            ++plan.events;
        }
      ++ownerKey;
    }
    auto &bases = plan.childBaseUnits[definition];
    for (const ObservationEmissionChild &child : plan.children[definition]) {
      // Geometry remains definition-local. Actual binding-dependent shape
      // changes cannot reuse the first layout for an observed subtree.
      if (auto collection = dyn_cast<ac::CollectionOp>(child.op);
          collection && plan.subtreeWidth.lookup(child.definition) != 0) {
        auto extents = context.analysis.resolveShape(collection.getShape(),
                                                     bindings, collection);
        if (failed(extents))
          return failure();
        uint64_t shape = 1;
        for (auto extent : *extents) {
          if (extent != 0 &&
              shape > std::numeric_limits<uint64_t>::max() / extent)
            return collection.emitOpError()
                   << "native observation lane count overflows";
          shape *= extent;
        }
        if (shape != child.shape)
          return collection.emitOpError()
                 << "binding-dependent observation geometry is unsupported";
      }
      bases.push_back(units);
      uint64_t block = child.shape * plan.subtreeWidth.lookup(child.definition);
      if (block > std::numeric_limits<uint64_t>::max() - units)
        return definition.emitOpError()
               << "native observation descriptor count overflows";
      units += block;
      if (child.definition) {
        SmallVector<InstancePath> childPaths;
        for (const auto &path : paths)
          for (uint64_t lane = 0; lane < child.shape; ++lane) {
            childPaths.push_back(path);
            childPaths.back().emplace_back(child.op, lane);
          }
        auto childBindings =
            isa<ac::InstanceOp>(child.op)
                ? context.analysis.bindInstance(cast<ac::InstanceOp>(child.op),
                                                bindings)
                : context.analysis.bindInstance(
                      cast<ac::CollectionOp>(child.op), bindings);
        if (failed(childBindings) ||
            failed(layout(child.definition, childPaths, *childBindings)))
          return failure();
      }
    }
    return success();
  };
  const InstancePath rootPath;
  if (failed(layout(*root, ArrayRef<InstancePath>(rootPath), *rootBindings)))
    return failure();
  for (std::size_t index = 1; index < plan.table.size(); ++index) {
    const auto &previous = plan.table[index - 1];
    const auto &current = plan.table[index];
    if (!(std::tie(previous[1], previous[2], previous[3], previous[0]) <
          std::tie(current[1], current[2], current[3], current[0])))
      return context.package.emitOpError()
             << "native observation descriptors are not strictly ordered";
    if (previous[0] == current[0])
      return context.package.emitOpError()
             << "native observation stable ordinals are not unique";
  }
  return plan;
}
} // namespace
HardwareEmitContext::HardwareEmitContext(ModuleOp package,
                                         ac::HardwareAnalysis &analysis)
    : package(package), analysis(analysis) {}
LogicalResult HardwareEmitContext::prepare() { return prepareImpl(false); }
LogicalResult HardwareEmitContext::prepareNativeChecks() {
  return prepareImpl(true);
}
const ac::HardwareSourceCheckPlan &HardwareEmitContext::sourceChecks() const {
  assert(sourceChecks_ && "emission context must be prepared");
  return *sourceChecks_;
}
bool HardwareEmitContext::managesChecks() {
  return isSystem() || (sourceChecks_ && !sourceChecks_->checks.empty());
}
LogicalResult HardwareEmitContext::prepareImpl(bool allowSourceChecks) {
  sourceChecks_.reset();
  names_.clear();
  if (!package)
    return failure();
  // Building the plan includes full package verification. Retain that result
  // instead of verifying and expanding the same occurrence closure twice.
  auto checks = analysis.getSourceCheckPlan();
  if (failed(checks))
    return failure();
  // The C++ native observation plan prepares through `prepareNativeChecks()`
  // and then re-closes the one instrumentation op it still cannot emit. This
  // shared gate stays closed for `ac.observe` because the Verilog target has no
  // observation channel yet, and a silently dropped observation is
  // exactly what the fail-closed contract forbids.
  auto unsupported = package.walk([&](Operation *op) {
    if ((allowSourceChecks || !isa<ac::SourceObserveOp>(op)) &&
        (allowSourceChecks || !isa<ac::SourceExpectOp>(op)))
      return WalkResult::advance();
    op->emitOpError() << "hardware instrumentation emission is not implemented";
    return WalkResult::interrupt();
  });
  if (unsupported.wasInterrupted())
    return failure();
  auto root = rootBindings();
  if (failed(root))
    return failure();
  auto checkInteger = [&](Attribute value, Operation *site) -> LogicalResult {
    auto integer = dyn_cast<ac::MathIntAttr>(value);
    if (!integer)
      return success();
    llvm::APSInt number(integer.getCanonicalValue());
    if (llvm::APSInt::compareValues(number,
                                    llvm::APSInt("-9223372036854775808")) < 0 ||
        llvm::APSInt::compareValues(number,
                                    llvm::APSInt("9223372036854775807")) > 0)
      return site->emitOpError()
             << "symbolic static arithmetic exceeds the emitted signed 64-bit "
                "domain; closed expressions are evaluated exactly";
    return success();
  };
  std::function<LogicalResult(Attribute, const ac::HardwareBindings &,
                              Operation *, bool)>
      inspect;
  inspect = [&](Attribute attribute, const ac::HardwareBindings &bindings,
                Operation *site, bool symbolic) -> LogicalResult {
    if (auto expression = dyn_cast_or_null<ac::StaticExprAttr>(attribute)) {
      bool demand = symbolic || !isClosedStatic(expression);
      if (!demand)
        return success();
      auto value = analysis.evaluateStatic(expression, bindings, site);
      if (failed(value) || failed(checkInteger(*value, site)))
        return failure();
      auto tree = expression.getTree();
      auto op = tree.getAs<StringAttr>("operator");
      if (op && (op.getValue() == "shl" || op.getValue() == "shr")) {
        auto count = analysis.evaluateStatic(
            tree.getAs<ac::StaticExprAttr>("rhs"), bindings, site);
        if (failed(count))
          return failure();
        auto integer = dyn_cast<ac::MathIntAttr>(*count);
        if (!integer || llvm::APSInt::compareValues(
                            llvm::APSInt(integer.getCanonicalValue()),
                            llvm::APSInt("63")) >= 0)
          return site->emitOpError() << "symbolic static shift count must be "
                                        "below 63 in both emitted targets";
      }
      if (op && (op.getValue() == "floordiv" || op.getValue() == "mod")) {
        auto lhs = analysis.evaluateStatic(
                 tree.getAs<ac::StaticExprAttr>("lhs"), bindings, site),
             rhs = analysis.evaluateStatic(
                 tree.getAs<ac::StaticExprAttr>("rhs"), bindings, site);
        if (failed(lhs) || failed(rhs))
          return failure();
        if (cast<ac::MathIntAttr>(*lhs).getCanonicalValue() ==
                "-9223372036854775808" &&
            cast<ac::MathIntAttr>(*rhs).getCanonicalValue() == "-1")
          return site->emitOpError() << "symbolic static division exceeds the "
                                        "emitted signed 64-bit domain";
      }
      auto kind = tree.getAs<StringAttr>("kind");
      if (kind && kind.getValue() == "select") {
        auto condition = tree.getAs<ac::StaticExprAttr>("condition");
        if (failed(inspect(condition, bindings, site, true)))
          return failure();
        auto selected = analysis.evaluateStatic(condition, bindings, site);
        if (failed(selected))
          return failure();
        return inspect(tree.getAs<ac::StaticExprAttr>(
                           cast<BoolAttr>(*selected).getValue() ? "yes" : "no"),
                       bindings, site, true);
      }
      if (op && (op.getValue() == "and_bool" || op.getValue() == "or_bool")) {
        auto lhs = tree.getAs<ac::StaticExprAttr>("lhs");
        if (failed(inspect(lhs, bindings, site, true)))
          return failure();
        auto selected = analysis.evaluateStatic(lhs, bindings, site);
        if (failed(selected))
          return failure();
        bool value = cast<BoolAttr>(*selected).getValue();
        if ((op.getValue() == "and_bool" && !value) ||
            (op.getValue() == "or_bool" && value))
          return success();
        return inspect(tree.getAs<ac::StaticExprAttr>("rhs"), bindings, site,
                       true);
      }
      return inspect(tree, bindings, site, true);
    }
    if (auto type = dyn_cast_or_null<TypeAttr>(attribute)) {
      if (auto function = dyn_cast<FunctionType>(type.getValue())) {
        for (Type value : function.getInputs())
          if (failed(inspect(TypeAttr::get(value), bindings, site, false)))
            return failure();
        for (Type value : function.getResults())
          if (failed(inspect(TypeAttr::get(value), bindings, site, false)))
            return failure();
      }
      if (auto table = dyn_cast<ac::TableType>(type.getValue())) {
        if (failed(inspect(table.getShape(), bindings, site, false)))
          return failure();
        return inspect(TypeAttr::get(table.getElementType()), bindings, site,
                       false);
      }
      if (auto bits = dyn_cast<ac::BitsType>(type.getValue()))
        return inspect(bits.getWidth(), bindings, site, false);
    }
    if (auto dict = dyn_cast_or_null<DictionaryAttr>(attribute))
      for (auto field : dict)
        if (field.getName() != "origin" && field.getName() != "location" &&
            failed(inspect(field.getValue(), bindings, site, symbolic)))
          return failure();
    if (auto array = dyn_cast_or_null<ArrayAttr>(attribute))
      for (auto child : array)
        if (failed(inspect(child, bindings, site, symbolic)))
          return failure();
    return success();
  };
  auto checkWidth = [&](Type type, const ac::HardwareBindings &bindings,
                        Operation *site,
                        uint64_t multiplicity) -> LogicalResult {
    if (!isa<ac::BitsType, ac::StructType, ac::EnumType, ac::TypeParamType,
             ac::TableType>(type))
      return success();
    auto width = analysis.getPackedWidth(type, bindings, site);
    if (failed(width))
      return failure();
    if (*width > std::numeric_limits<unsigned>::max() / multiplicity)
      return site->emitOpError()
             << "hardware payload plane including enclosing collections "
                "exceeds the current Runtime bit-width capacity";
    return success();
  };
  std::function<LogicalResult(ac::ModuleOp, const ac::HardwareBindings &,
                              uint64_t)>
      capabilities;
  llvm::DenseSet<Operation *> checkedConcreteDefinitions;
  capabilities = [&](ac::ModuleOp module, const ac::HardwareBindings &bindings,
                     uint64_t multiplicity) -> LogicalResult {
    for (auto &value : bindings.integers) {
      auto actual = value.second;
      if (auto expr = dyn_cast<ac::StaticExprAttr>(actual)) {
        auto evaluated = analysis.evaluateStatic(expr, bindings, module);
        if (failed(evaluated))
          return failure();
        actual = *evaluated;
      }
      if (failed(checkInteger(actual, module)))
        return failure();
    }
    auto result = module.walk([&](Operation *op) {
      for (auto attr : op->getAttrs())
        if (failed(inspect(attr.getValue(), bindings, op, false)))
          return WalkResult::interrupt();
      for (Type type : op->getResultTypes())
        if (failed(inspect(TypeAttr::get(type), bindings, op, false)))
          return WalkResult::interrupt();
      for (Type type : op->getOperandTypes())
        if (failed(inspect(TypeAttr::get(type), bindings, op, false)))
          return WalkResult::interrupt();
      return WalkResult::advance();
    });
    if (result.wasInterrupted())
      return failure();
    for (Type type : module.getFunctionType().getInputs())
      if (failed(checkWidth(type, bindings, module, multiplicity)))
        return failure();
    for (Type type : module.getFunctionType().getResults())
      if (failed(checkWidth(type, bindings, module, multiplicity)))
        return failure();
    auto widths = module.walk([&](Operation *op) {
      for (Type type : op->getResultTypes())
        if (failed(checkWidth(type, bindings, op, multiplicity)))
          return WalkResult::interrupt();
      return WalkResult::advance();
    });
    if (widths.wasInterrupted())
      return failure();
    for (auto &op : module.getBody().front()) {
      if (auto queue = dyn_cast<ac::QueueOp>(op)) {
        auto config = analysis.resolveQueue(queue, bindings);
        if (failed(config))
          return failure();
        if (config->depth > uint64_t(std::numeric_limits<int64_t>::max()))
          return op.emitOpError()
                 << "queue depth exceeds emitted signed 64-bit indexing";
        if (!multiplicity ||
            config->tokenCardinality >
                std::numeric_limits<uint64_t>::max() / multiplicity)
          return op.emitOpError() << "queue family token geometry overflows";
        auto elementWidth =
            analysis.getPackedWidth(config->tokenElementType, bindings, &op);
        if (failed(elementWidth))
          return failure();
        // Tables hold independent scalar wires: round each scalar first, not
        // the packed token. These bytes estimate storage, never hardware
        // layout.
        uint64_t scalarBytes =
            ((*elementWidth - 1) / 64 + 1) * 3 * sizeof(uint64_t);
        if (config->tokenCardinality >
            std::numeric_limits<uint64_t>::max() / scalarBytes)
          return op.emitOpError() << "queue token representation overflows";
        uint64_t tokenBytes = config->tokenCardinality * scalarBytes;
        uint64_t capacity = std::numeric_limits<std::ptrdiff_t>::max();
        // Six control planes (144), Current/Pending metadata (32/8), and a
        // completion word (8) per lane; 128 reserves the shared owner. Besides
        // D slots, retain Pending, two data pin planes and two token buffers.
        // Delayed availability adds D u64 deadlines plus tick, maturity cursor
        // and eligible count (24). Pending timing flags use its existing
        // reserve.
        uint64_t laneCapacity = (capacity - 128) / multiplicity;
        bool delayed = config->availabilityLatency > 1;
        uint64_t fixedBytes = delayed ? 216 : 192;
        if (laneCapacity <= fixedBytes)
          return op.emitOpError()
                 << "queue storage including enclosing collections exceeds "
                    "the current Runtime object capacity";
        uint64_t remaining = laneCapacity - fixedBytes;
        if (delayed) {
          if (config->depth > remaining / sizeof(uint64_t))
            return op.emitOpError()
                   << "queue storage including enclosing collections exceeds "
                      "the current Runtime object capacity";
          remaining -= config->depth * sizeof(uint64_t);
        }
        // Subtract reserved token copies before comparing depth: neither D+5
        // nor the combined payload/timing/family allocation can overflow.
        uint64_t tokenCapacity = remaining / tokenBytes;
        if (tokenCapacity < 5 || config->depth > tokenCapacity - 5)
          return op.emitOpError()
                 << "queue storage including enclosing collections exceeds "
                    "the current Runtime object capacity";
        continue;
      }
      if (!isa<ac::InstanceOp, ac::CollectionOp>(op))
        continue;
      auto child =
          isa<ac::InstanceOp>(op)
              ? analysis.bindInstance(cast<ac::InstanceOp>(op), bindings)
              : analysis.bindInstance(cast<ac::CollectionOp>(op), bindings);
      if (failed(child))
        return failure();
      for (const auto &binding : child->integers) {
        Attribute value = binding.second;
        if (auto expression = dyn_cast<ac::StaticExprAttr>(value)) {
          auto evaluated = analysis.evaluateStatic(expression, *child, &op);
          if (failed(evaluated))
            return failure();
          value = *evaluated;
        }
        if (failed(checkInteger(value, &op)))
          return failure();
      }
      uint64_t childMultiplicity = multiplicity;
      if (auto collection = dyn_cast<ac::CollectionOp>(op)) {
        auto shape =
            analysis.resolveShape(collection.getShape(), bindings, &op);
        if (failed(shape))
          return failure();
        for (uint64_t extent : *shape) {
          if (childMultiplicity > std::numeric_limits<uint64_t>::max() / extent)
            return op.emitOpError() << "enclosing collection multiplicity "
                                       "overflows target storage indexing";
          childMultiplicity *= extent;
        }
      }
      // Storage arrays are not port planes. Check their concrete host-index
      // capacity too, before a generated std::array bound can overflow.
      auto primitive = analysis.getPrimitiveKind(child->owner);
      if (primitive == "sync_mem" || primitive == "sync_mem_dp" ||
          primitive == "byte_mem") {
        Attribute rawDepth = child->integers.lookup("DEPTH");
        if (auto expr = dyn_cast<ac::StaticExprAttr>(rawDepth)) {
          auto value = analysis.evaluateStatic(expr, *child, &op);
          if (failed(value))
            return failure();
          rawDepth = *value;
        }
        auto integer = dyn_cast<ac::MathIntAttr>(rawDepth);
        if (!integer)
          return op.emitOpError() << "memory depth is unresolved";
        llvm::APSInt depthValue(integer.getCanonicalValue());
        if (depthValue.isNegative() || depthValue.isZero() ||
            depthValue.getActiveBits() > 63)
          return op.emitOpError()
                 << "memory depth exceeds target storage indexing";
        auto typeName = cast<StringAttr>(
            child->owner->getAttrOfType<ArrayAttr>("type_parameters")[0]);
        auto bits = analysis.getPackedWidth(
            child->types.lookup(typeName.getValue()), *child, &op);
        if (failed(bits))
          return failure();
        uint64_t wordBytes = ((*bits - 1) / 64 + 1) * 8;
        uint64_t entryBytes = primitive == "byte_mem" ? 1 : wordBytes;
        uint64_t readPorts = primitive == "sync_mem_dp" ? 2 : 1;
        // Upper bound for the fixed mask/read/pending bookkeeping in the
        // owning kernels; this is a resource estimate, not packed bit layout.
        uint64_t overhead =
            128 + wordBytes * (3 * readPorts + 2) + ((*bits - 1) / 8 + 1);
        uint64_t capacity = std::numeric_limits<std::ptrdiff_t>::max();
        uint64_t perLaneLimit = capacity / childMultiplicity;
        if (overhead >= perLaneLimit ||
            depthValue.getZExtValue() > (perLaneLimit - overhead) / entryBytes)
          return op.emitOpError()
                 << "memory storage including enclosing collections exceeds "
                    "the current Runtime object capacity";
      }
      if (auto def = dyn_cast<ac::ModuleOp>(child->owner))
        if (failed(capabilities(def, *child, childMultiplicity)))
          return failure();
    }
    if (module.getParameters().empty() && module.getTypeParameters().empty())
      checkedConcreteDefinitions.insert(module);
    return success();
  };
  if (failed(capabilities(cast<ac::ModuleOp>(root->owner), *root, 1)))
    return failure();
  // Every non-generic definition emits a concrete scalar wrapper, even when the
  // selected root does not instantiate it. Validate that emitted closure too;
  // unbound generic definitions must not acquire facts from their defaults.
  for (auto definition : analysis.getDefinitions()) {
    if (!definition.getParameters().empty() ||
        !definition.getTypeParameters().empty() ||
        checkedConcreteDefinitions.contains(definition))
      continue;
    ac::HardwareBindings bindings;
    bindings.owner = definition;
    if (failed(capabilities(definition, bindings, 1)))
      return failure();
  }
  llvm::StringSet<> rtl, rtlTypes;
  // Every nominal declaration is emitted, even when no executable wire uses
  // it. Check the same target capacity and spelling rules for that closure.
  for (Operation &op : package.getBody()->getOperations()) {
    Type type;
    if (auto enumeration = dyn_cast<ac::EnumOp>(op))
      type = ac::EnumType::get(op.getContext(), enumeration.getSymNameAttr());
    else if (auto record = dyn_cast<ac::StructOp>(op))
      type = ac::StructType::get(op.getContext(), record.getSymNameAttr());
    else
      continue;
    if (failed(checkWidth(type, {}, &op, 1)))
      return failure();
    auto rtlName = rtlType(type, &op);
    if (failed(rtlName))
      return failure();
    // RTL typedefs share pycircuit_types, separately from global module names.
    if (!rtlTypes.insert(*rtlName).second)
      return op.emitOpError() << "emitted hardware name collision";
  }
  for (auto definition : analysis.getDefinitions()) {
    Operation *site = definition;
    auto symbol = definition.getSymName();
    auto r = qualified(symbol, site, "_");
    if (failed(r))
      return failure();
    EmitNames names;
    names.rtl = "ac_" + *r;
    auto owner = sourceOwnerComponents(definition.getSourceOwner(),
                                       [&] { return site->emitOpError(); });
    if (failed(owner))
      return failure();
    auto v = sourcePathName(owner->filePath, ".v",
                            [&] { return site->emitOpError(); });
    if (failed(v))
      return failure();
    names.verilog = *v;
    if (!rtl.insert(names.rtl).second)
      return site->emitOpError() << "emitted hardware name collision";
    // Owners may intentionally share a basename; grouping checks unequal
    // owners.
    names_.try_emplace(site, std::move(names));
  }
  sourceChecks_ = std::move(*checks);
  return success();
}
const EmitNames &HardwareEmitContext::names(Operation *definition) const {
  return names_.find(definition)->second;
}
FailureOr<std::string>
HardwareEmitContext::rtlFormalName(StringRef name, Operation *site) const {
  auto legalized = identifier(name, site);
  if (failed(legalized))
    return failure();
  return "pyc_parameter_" + *legalized;
}
bool HardwareEmitContext::isClosedStatic(
    ac::StaticExprAttr expr, const ac::HardwareBindings &bindings) const {
  return closedStatic(expr, bindings, this,
                      bindings.owner ? bindings.owner : package.operator->());
}
FailureOr<std::string>
HardwareEmitContext::staticRtlValue(ac::StaticExprAttr expr,
                                    Operation *site) const {
  return staticTree(*this, expr.getTree(), site, true);
}
FailureOr<std::string>
HardwareEmitContext::staticRtlValue(ac::StaticExprAttr expr,
                                    const ac::HardwareBindings &bindings,
                                    Operation *site) const {
  return staticTree(*this, expr.getTree(), site, true, &bindings);
}
FailureOr<std::string> HardwareEmitContext::unsignedStaticValue(
    ac::StaticExprAttr expression, Operation *site, bool rtl,
    const ac::HardwareBindings &bindings) const {
  if (isClosedStatic(expression, bindings)) {
    auto value = analysis.evaluateStatic(expression, bindings, site);
    if (failed(value))
      return failure();
    auto integer = dyn_cast<ac::MathIntAttr>(*value);
    if (!integer)
      return site->emitOpError()
             << "unsigned static value requires a positive integer fitting u64";
    llvm::APSInt number(integer.getCanonicalValue());
    if (number.isNegative() || number.isZero() || number.getActiveBits() > 64)
      return site->emitOpError()
             << "unsigned static value requires a positive integer fitting u64";
    auto decimal = integer.getCanonicalValue().str();
    return rtl ? "64'd" + decimal : decimal + "ULL";
  }
  // Queue analysis proves demanded symbolic values positive; prepare retains
  // the signed arithmetic/actual limits for those expressions. Preserve formal
  // names here and convert only the resulting value to the unsigned parameter.
  auto text = staticTree(*this, expression.getTree(), site, rtl, &bindings);
  if (failed(text))
    return failure();
  return rtl ? "$unsigned(64'(" + *text + "))" : "std::uint64_t(" + *text + ")";
}
FailureOr<std::string> HardwareEmitContext::rtlType(Type type,
                                                    Operation *site) const {
  if (auto cached = rtlTypeCache_.find(type); cached != rtlTypeCache_.end())
    return cached->second;
  FailureOr<std::string> rendered = [&]() -> FailureOr<std::string> {
    if (auto bits = dyn_cast<ac::BitsType>(type)) {
      auto width = staticTree(*this, bits.getWidth().getTree(), site, true);
      if (failed(width))
        return failure();
      return "logic [(" + *width + ")-1:0]";
    }
    if (auto table = dyn_cast<ac::TableType>(type)) {
      auto width = payloadWidth(table, site, true);
      if (failed(width))
        return failure();
      return "logic [(" + *width + ")-1:0]";
    }
    if (auto param = dyn_cast<ac::TypeParamType>(type))
      return rtlFormalName(param.getName().getValue(), site);
    if (auto enumeration = dyn_cast<ac::EnumType>(type)) {
      if (failed(analysis.resolveEnum(enumeration, site)))
        return failure();
      auto q = qualified(enumeration.getName().getValue(), site, "_");
      if (failed(q))
        return failure();
      return "pycircuit_types::ac_" + *q + "_t";
    }
    if (auto record = dyn_cast<ac::StructType>(type)) {
      auto q = qualified(record.getName().getValue(), site, "_");
      if (failed(q))
        return failure();
      return "pycircuit_types::ac_" + *q + "_t";
    }
    return site->emitOpError() << "unsupported RTL hardware type";
  }();
  if (succeeded(rendered))
    rtlTypeCache_[type] = *rendered;
  return rendered;
}
} // namespace acir::compiler
namespace acir::compiler {
FailureOr<ac::HardwareBindings> HardwareEmitContext::rootBindings() const {
  auto rootPackage = package;
  auto system = *rootPackage.getOps<ac::SystemOp>().begin();
  auto entry = system.getEntry();
  auto *root = analysis.lookupDefinition(
      entry.getAs<FlatSymbolRefAttr>("callee").getValue());
  ac::HardwareBindings bindings;
  bindings.owner = root;
  auto types = entry.getAs<ArrayAttr>("type_arguments");
  for (auto [name, actual] :
       llvm::zip(root->getAttrOfType<ArrayAttr>("type_parameters"), types))
    bindings.types[cast<StringAttr>(name).getValue()] =
        cast<TypeAttr>(actual).getValue();
  auto actuals = entry.getAs<ArrayAttr>("parameters");
  for (auto [index, raw] :
       llvm::enumerate(root->getAttrOfType<ArrayAttr>("parameters"))) {
    auto formal = cast<DictionaryAttr>(raw);
    auto expression = index < actuals.size()
                          ? cast<ac::StaticExprAttr>(actuals[index])
                          : formal.getAs<ac::StaticExprAttr>("default");
    auto value = analysis.evaluateStatic(expression, bindings, system);
    if (failed(value))
      return failure();
    bindings.integers[formal.getAs<StringAttr>("name").getValue()] = *value;
  }
  return bindings;
}
FailureOr<std::string> HardwareEmitContext::rootRtlParameters() const {
  auto bindings = rootBindings();
  if (failed(bindings))
    return failure();
  auto *root = bindings->owner;
  SmallVector<std::string> arguments;
  for (auto raw : root->getAttrOfType<ArrayAttr>("parameters")) {
    auto formal = cast<DictionaryAttr>(raw);
    auto name = formal.getAs<StringAttr>("name");
    auto id = rtlFormalName(name.getValue(), root);
    if (failed(id))
      return failure();
    auto value =
        cast<ac::MathIntAttr>(bindings->integers.lookup(name.getValue()))
            .getCanonicalValue();
    arguments.push_back("." + *id + "(" + value.str() + ")");
  }
  for (auto raw : root->getAttrOfType<ArrayAttr>("type_parameters")) {
    auto name = cast<StringAttr>(raw);
    auto id = rtlFormalName(name.getValue(), root);
    auto type = rtlType(bindings->types.lookup(name.getValue()), root);
    if (failed(id) || failed(type))
      return failure();
    arguments.push_back("." + *id + "(" + *type + ")");
  }
  return arguments.empty() ? std::string()
                           : " #( " + join(arguments, ", ") + " )";
}
} // namespace acir::compiler

namespace acir::compiler {
FailureOr<std::string>
HardwareEmitContext::shapeSize(ArrayAttr shape, Operation *site, bool rtl,
                               const ac::HardwareBindings &bindings) const {
  std::string result = "1";
  for (Attribute raw : shape) {
    auto text = staticTree(*this, cast<ac::StaticExprAttr>(raw).getTree(), site,
                           rtl, &bindings);
    if (failed(text))
      return failure();
    result = "(" + result + " * (" + *text + "))";
  }
  return result;
}
FailureOr<std::string>
HardwareEmitContext::payloadWidth(Type type, Operation *site, bool rtl,
                                  const ac::HardwareBindings &bindings) const {
  auto resolved = analysis.resolveType(type, bindings, site);
  if (failed(resolved))
    return failure();
  type = *resolved;
  if (auto table = dyn_cast<ac::TableType>(type)) {
    auto count = shapeSize(table.getShape(), site, rtl, bindings);
    auto element = payloadWidth(table.getElementType(), site, rtl, bindings);
    if (failed(count) || failed(element))
      return failure();
    return "(" + *count + " * (" + *element + "))";
  }
  if (auto bits = dyn_cast<ac::BitsType>(type))
    return staticTree(*this, bits.getWidth().getTree(), site, rtl, &bindings);
  if (rtl && isa<ac::StructType, ac::EnumType>(type)) {
    // Nominal layouts are verified by common analysis. Render their finite
    // width directly: $bits(package::type) is not portable in packed ranges.
    auto width = analysis.getPackedWidth(type, bindings, site);
    if (failed(width))
      return failure();
    return std::to_string(*width);
  }
  auto text = rtlType(type, site);
  if (failed(text))
    return failure();
  return "$bits(" + *text + ")";
}
FailureOr<std::string>
HardwareEmitContext::viewIndex(ac::TableViewOp view, StringRef ordinal,
                               bool rtl,
                               const ac::HardwareBindings &bindings) const {
  auto input = cast<ac::TableType>(view.getInput().getType());
  auto output = cast<ac::TableType>(view.getResult().getType());
  auto kind = view.getKind();
  if (kind == "reshape")
    return ordinal.str();
  auto render = [&](Attribute raw) {
    return staticTree(*this, cast<ac::StaticExprAttr>(raw).getTree(), view, rtl,
                      &bindings);
  };
  auto renderedShape =
      [&](ArrayAttr shape) -> FailureOr<SmallVector<std::string>> {
    SmallVector<std::string> result;
    for (auto raw : shape) {
      auto text = render(raw);
      if (failed(text))
        return failure();
      result.push_back(*text);
    }
    return result;
  };
  auto inShape = renderedShape(input.getShape());
  auto outShape = renderedShape(output.getShape());
  if (failed(inShape) || failed(outShape))
    return failure();
  auto strides = [](ArrayRef<std::string> shape) {
    SmallVector<std::string> result(shape.size(), "1");
    for (size_t i = shape.size(); i > 1; --i)
      result[i - 2] = "(" + result[i - 1] + " * (" + shape[i - 1] + "))";
    return result;
  };
  auto inStride = strides(*inShape), outStride = strides(*outShape);
  SmallVector<std::string> coordinates;
  for (size_t i = 0; i < outShape->size(); ++i)
    coordinates.push_back("((" + ordinal.str() + " / (" + outStride[i] +
                          ")) % (" + (*outShape)[i] + "))");
  auto parameters = view.getParameters();
  std::string result = "0";
  for (size_t axis = 0; axis < inShape->size(); ++axis) {
    std::string coordinate;
    if (kind == "slice") {
      auto offset = render(parameters.getAs<ArrayAttr>("offsets")[axis]);
      auto stride = render(parameters.getAs<ArrayAttr>("strides")[axis]);
      if (failed(offset) || failed(stride))
        return failure();
      // A single output coordinate is always zero; omit a potentially huge
      // (but unobservable) static stride rather than narrowing it in C++.
      auto size = cast<ac::StaticExprAttr>(output.getShape()[axis]);
      bool singleton = false;
      if (isClosedStatic(size, bindings)) {
        auto n = analysis.evaluateStatic(size, bindings, view);
        singleton = succeeded(n) &&
                    cast<ac::MathIntAttr>(*n).getCanonicalValue() == "1";
      }
      coordinate = singleton ? *offset
                             : "(" + *offset + " + " + coordinates[axis] +
                                   " * (" + *stride + "))";
    } else if (kind == "transpose") {
      coordinate = "0";
      auto axes = parameters.getAs<ArrayAttr>("axes");
      for (size_t j = 0; j < axes.size(); ++j) {
        auto mapped = render(axes[j]);
        if (failed(mapped))
          return failure();
        coordinate += " + ((" + *mapped + ") == " + std::to_string(axis) +
                      " ? " + coordinates[j] + " : 0)";
      }
      coordinate = "(" + coordinate + ")";
    } else if (kind == "rotate") {
      auto selected = render(parameters.get("axis"));
      auto offset = render(parameters.get("offset"));
      if (failed(selected) || failed(offset))
        return failure();
      std::string shift;
      bool closedShape = llvm::all_of(input.getShape(), [&](Attribute raw) {
        return isClosedStatic(cast<ac::StaticExprAttr>(raw), bindings);
      });
      if (closedShape &&
          isClosedStatic(cast<ac::StaticExprAttr>(parameters.get("offset")),
                         bindings) &&
          isClosedStatic(cast<ac::StaticExprAttr>(parameters.get("axis")),
                         bindings)) {
        auto source = analysis.getViewSourceOrdinal(view, 0, bindings);
        auto shape = analysis.getTableShape(input, bindings, view);
        if (failed(source) || failed(shape))
          return failure();
        uint64_t stride = 1;
        for (size_t j = axis + 1; j < shape->size(); ++j)
          stride *= (*shape)[j];
        shift = std::to_string((*source / stride) % (*shape)[axis]);
      } else {
        auto offsetExpr = cast<ac::StaticExprAttr>(parameters.get("offset"));
        if (isClosedStatic(offsetExpr, bindings)) {
          auto value = analysis.evaluateStatic(offsetExpr, bindings, view);
          if (failed(value))
            return failure();
          llvm::APSInt integer(
              cast<ac::MathIntAttr>(*value).getCanonicalValue());
          bool negative = integer.isNegative();
          llvm::APInt magnitude = negative
                                      ? -integer.sext(integer.getBitWidth() + 1)
                                      : llvm::APInt(integer);
          if (rtl) {
            llvm::SmallString<256> decimal;
            magnitude.toString(decimal, 10, false);
            auto remainder =
                "(" + std::to_string(std::max(1u, magnitude.getActiveBits())) +
                "'d" + decimal.str().str() + " % (" + (*inShape)[axis] + "))";
            shift = negative ? "(" + remainder + " == 0 ? 0 : (" +
                                   (*inShape)[axis] + ")-" + remainder + ")"
                             : remainder;
          } else {
            SmallVector<std::string> words;
            for (unsigned word = 0;
                 word < std::max(1u, (magnitude.getActiveBits() + 63) / 64);
                 ++word)
              words.push_back(std::to_string(magnitude.getRawData()[word]) +
                              "ULL");
            shift = "pyc_mod_words(std::array<std::uint64_t," +
                    std::to_string(words.size()) + ">{" + join(words, ",") +
                    "}, " + (negative ? "true" : "false") + ", " +
                    (*inShape)[axis] + ")";
          }
        } else
          shift = (rtl ? "pycircuit_types::pyc_mod(" : "pyc_mod(") + *offset +
                  ", " + (*inShape)[axis] + ")";
      }
      auto rotated = "((" + coordinates[axis] + " + (" + shift + ")) % (" +
                     (*inShape)[axis] + "))";
      coordinate = "((" + *selected + ") == " + std::to_string(axis) + " ? " +
                   rotated + " : " + coordinates[axis] + ")";
    } else
      return view.emitOpError() << "unverified table view kind";
    result += " + (" + coordinate + ") * (" + inStride[axis] + ")";
  }
  return "(" + result + ")";
}
} // namespace acir::compiler

namespace acir::compiler {
bool HardwareEmitContext::isSystem() {
  auto entries = package.getOps<ac::SystemOp>();
  if (!llvm::hasSingleElement(entries))
    return false;
  auto callee =
      (*entries.begin()).getEntry().getAs<FlatSymbolRefAttr>("callee");
  auto *root = callee ? analysis.lookupDefinition(callee.getValue()) : nullptr;
  auto kind =
      root ? root->getAttrOfType<StringAttr>("ac.root_kind") : StringAttr();
  return kind && kind.getValue() == "system";
}
FailureOr<const ObservationEmissionPlan *> HardwareEmitContext::observationPlan() {
  if (!observationPlanPrepared_) {
    auto plan = buildObservationPlan(*this);
    if (succeeded(plan))
      observationPlan_.emplace(std::move(*plan));
    observationPlanPrepared_ = true;
  }
  if (!observationPlan_)
    return failure();
  return &*observationPlan_;
}

FailureOr<ObservationEmissionPlan>
observationEmissionPlan(HardwareEmitContext &context) {
  auto plan = context.observationPlan();
  if (failed(plan))
    return failure();
  return **plan;
}
namespace {
FailureOr<llvm::json::Value> legacyMetadataValue(Attribute value, Operation *site) {
  if (auto item = dyn_cast<StringAttr>(value))
    return llvm::json::Value(item.getValue());
  if (auto item = dyn_cast<FlatSymbolRefAttr>(value))
    return llvm::json::Value(item.getValue());
  if (auto item = dyn_cast<BoolAttr>(value))
    return llvm::json::Value(item.getValue());
  if (auto item = dyn_cast<IntegerAttr>(value)) {
    if (item.getValue().getActiveBits() > 64)
      return site->emitOpError() << "emission metadata integer exceeds u64";
    return llvm::json::Value(item.getValue().getZExtValue());
  }
  if (auto item = dyn_cast<ac::MathIntAttr>(value))
    return llvm::json::Value(item.getCanonicalValue());
  if (auto items = dyn_cast<ArrayAttr>(value)) {
    llvm::json::Array result;
    for (Attribute item : items) {
      auto converted = legacyMetadataValue(item, site);
      if (failed(converted))
        return failure();
      result.push_back(std::move(*converted));
    }
    return llvm::json::Value(std::move(result));
  }
  if (auto items = dyn_cast<DictionaryAttr>(value)) {
    llvm::json::Object result;
    for (auto item : items) {
      auto converted = legacyMetadataValue(item.getValue(), site);
      if (failed(converted))
        return failure();
      result[item.getName().strref()] = std::move(*converted);
    }
    return llvm::json::Value(std::move(result));
  }
  return site->emitOpError() << "unsupported emission metadata attribute";
}
} // namespace
} // namespace acir::compiler
