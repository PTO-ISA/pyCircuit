#include "ConcreteDesignPlan.h"

#include "Dialect/ACIR/HardwareSourceChecks.h"
#include "FinalEmissionNames.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "pycircuit/Dialect/HWIR/HWIROps.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/xxhash.h"

#include <algorithm>
#include <limits>

using namespace mlir;

namespace acir::compiler {
namespace {

static hwir::ModuleOp asHWIRModule(Operation *operation) {
  return dyn_cast_or_null<hwir::ModuleOp>(operation);
}
static FunctionType moduleType(Operation *operation) {
  return cast<FunctionType>(
      cast<TypeAttr>(operation->getAttr("function_type")).getValue());
}
static ArrayAttr moduleParameters(Operation *operation) {
  return operation->getAttrOfType<ArrayAttr>("parameters");
}
static ArrayAttr moduleTypeParameters(Operation *operation) {
  return operation->getAttrOfType<ArrayAttr>("type_parameters");
}
static ArrayAttr moduleInputNames(Operation *operation) {
  return operation->getAttrOfType<ArrayAttr>("input_names");
}
static ArrayAttr moduleOutputNames(Operation *operation) {
  return operation->getAttrOfType<ArrayAttr>("output_names");
}
static Region &moduleBody(Operation *operation) {
  return operation->getRegion(0);
}
static DictionaryAttr moduleSourceOwner(Operation *operation) {
  return operation->getAttrOfType<DictionaryAttr>("source_owner");
}

std::string specializationName(const ConcreteModuleKey &key) {
  StringRef symbol = SymbolTable::getSymbolName(key.definition).getValue();
  std::string canonical = symbol.str();
  for (const ConcreteIntegerBinding &binding : key.integers) {
    canonical += "|i:" + binding.name.getValue().str() + "=";
    canonical += binding.canonicalValue;
  }
  for (const ConcreteTypeBinding &binding : key.types) {
    canonical += "|t:" + binding.name.getValue().str() + "=";
    canonical += binding.canonicalForm;
  }
  std::string hash;
  llvm::raw_string_ostream hashStream(hash);
  hashStream << llvm::format_hex_no_prefix(llvm::xxHash64(canonical), 16,
                                           /*Upper=*/false);
  return "__h" + hash;
}

std::string canonicalModuleKey(const ConcreteModuleKey &key) {
  std::string canonical =
      SymbolTable::getSymbolName(key.definition).getValue().str();
  for (const ConcreteIntegerBinding &binding : key.integers)
    canonical +=
        "|i:" + binding.name.getValue().str() + "=" + binding.canonicalValue;
  for (const ConcreteTypeBinding &binding : key.types)
    canonical +=
        "|t:" + binding.name.getValue().str() + "=" + binding.canonicalForm;
  return canonical;
}

uint64_t shapeProduct(ArrayRef<uint64_t> shape) {
  uint64_t product = 1;
  for (uint64_t extent : shape)
    product *= extent;
  return product;
}

uint64_t shapeIndexWidth(uint64_t size) {
  return std::max<uint64_t>(1, llvm::APInt(64, size - 1).getActiveBits());
}

bool isApprovedStructuralOp(Operation *operation) {
  if (operation->getName().getDialectNamespace() == "hwir")
    return true;
  static const llvm::StringSet<> approved = [] {
    llvm::StringSet<> result;
    for (StringRef name :
         {"ac.module",        "ac.rule",           "ac.yield",
          "ac.instance",      "ac.collection",     "ac.queue",
          "ac.constant",      "ac.bits.constant",  "ac.bits.constant4",
          "ac.bits.unary",
          "ac.bits.binary",   "ac.bits.compare",   "ac.bits.select",
          "ac.bits.concat",   "ac.bits.extract",   "ac.bits.resize",
          "ac.math.constant", "ac.math.from_bits", "ac.math.compare",
          "ac.math.binary",   "ac.math.to_bits",   "ac.value.merge",
          "ac.struct.create", "ac.struct.get",     "ac.enum.create",
          "ac.enum.to_bits",  "ac.enum.from_bits", "ac.table.create",
          "ac.table.splat",   "ac.table.map",      "ac.table.view",
          "ac.table.index",   "ac.table.get",      "ac.table.match",
          "ac.table.choose",  "ac.table.fold",     "ac.state",
          "ac.state.read",    "ac.state.write",    "ac.expect",
          "ac.observe"})
      result.insert(name);
    return result;
  }();
  return approved.contains(operation->getName().getStringRef());
}

bool isKnownPrimitive(StringRef kind) {
  return llvm::is_contained(ArrayRef<StringRef>{"dff", "dffe", "fifo",
                                                "byte_mem", "sync_mem",
                                                "sync_mem_dp"},
                            kind);
}

class Builder {
public:
  Builder(ModuleOp package, ac::HardwareAnalysis &analysis)
      : package(package), analysis(analysis) {
    plan.package = package;
  }

  FailureOr<ConcreteDesignPlan> build() {
    FailureOr<ac::HardwareBindings> root = failure();
    if (package->hasAttr("hwir.contract")) {
      auto systems = package.getOps<hwir::SystemOp>();
      if (!llvm::hasSingleElement(systems))
        return package.emitOpError()
               << "structural C++ requires exactly one hwir.system";
      auto system = *systems.begin();
      auto callee = system.getEntry().getAs<FlatSymbolRefAttr>("callee");
      Operation *definition =
          callee ? analysis.lookupDefinition(callee.getValue()) : nullptr;
      if (!isa_and_nonnull<hwir::ModuleOp>(definition))
        return system.emitOpError()
               << "system entry must resolve to an HWIR module";
      root = analysis.bindCallee(
          definition, system.getEntry().getAs<ArrayAttr>("parameters"),
          system.getEntry().getAs<ArrayAttr>("type_arguments"), {}, system);
    } else {
      root = ac::detail::resolveHardwareRootBindings(analysis);
    }
    if (failed(root))
      return failure();
    if (failed(requireConcreteBindings(*root, root->owner)))
      return failure();
    if (failed(preflightResources(root->owner, *root, 1)))
      return failure();
    auto rootIndex = visit(root->owner, *root);
    if (failed(rootIndex))
      return package.emitOpError()
             << "failed to materialize the concrete root module";
    plan.rootModule = *rootIndex;
    plan.root = plan.modules[*rootIndex].key;
    if (failed(buildSystemPlan()))
      return package.emitOpError() << "failed to build the concrete system plan";
    materializeDesignLifecycle();
    buildManifest();
    if (failed(buildSourceUnits()))
      return package.emitOpError()
             << "failed to build concrete source-unit ownership";
    if (failed(buildNamedTypes()))
      return package.emitOpError() << "failed to build concrete named types";
    if (failed(buildRetainedEffects()))
      return package.emitOpError() << "failed to build retained effects";
    auto sourceCheckCount = countSourceChecks(plan.rootModule);
    if (failed(sourceCheckCount))
      return package.emitOpError() << "failed to count retained source checks";
    plan.sourceCheckCount = *sourceCheckCount;
    if (failed(buildDependencyGraph()))
      return package.emitOpError()
             << "failed to build the concrete dependency graph";
    // Construction-only bindings and analysis schedules must not remain an
    // alternate semantic input to the renderer after all concrete records have
    // been materialized.
    for (ConcreteModulePlan &module : plan.modules) {
      module.bindings = {};
      module.schedule = {};
    }
    if (failed(verifyConcreteDesignPlan(plan)))
      return failure();
    return std::move(plan);
  }

private:
  LogicalResult preflightResources(Operation *definition,
                                   const ac::HardwareBindings &bindings,
                                   uint64_t multiplicity) {
    auto checkType = [&](Type type, Operation *site) -> LogicalResult {
      auto width = analysis.getPackedWidth(type, bindings, site);
      if (failed(width))
        return failure();
      if (!multiplicity ||
          *width > std::numeric_limits<unsigned>::max() / multiplicity)
        return site->emitOpError()
               << "hardware payload plane including enclosing collections "
                  "exceeds the current Runtime bit-width capacity";
      return success();
    };
    auto checkMemory = [&](Operation *site,
                           const ac::HardwareBindings &child,
                           uint64_t childMultiplicity) -> LogicalResult {
      StringRef primitive = analysis.getPrimitiveKind(child.owner);
      if (primitive != "sync_mem" && primitive != "sync_mem_dp" &&
          primitive != "byte_mem")
        return success();
      auto depth = dyn_cast_or_null<ac::MathIntAttr>(
          child.integers.lookup("DEPTH"));
      if (!depth)
        return site->emitOpError() << "memory depth is unresolved";
      llvm::APSInt depthValue(depth.getCanonicalValue());
      if (depthValue.isNegative() || depthValue.isZero() ||
          depthValue.getActiveBits() > 63)
        return site->emitOpError()
               << "memory depth exceeds target storage indexing";
      Type payload = child.types.lookup("T");
      if (!payload)
        return site->emitOpError()
               << "memory payload type is unresolved";
      auto bits = analysis.getPackedWidth(payload, child, site);
      if (failed(bits))
        return failure();
      const uint64_t wordBytes = ((*bits - 1) / 64 + 1) * 8;
      const uint64_t entryBytes = primitive == "byte_mem" ? 1 : wordBytes;
      const uint64_t readPorts = primitive == "sync_mem_dp" ? 2 : 1;
      // Upper bound for the fixed mask/read/pending bookkeeping in the
      // owning kernels; this is a resource estimate, not packed bit layout.
      const uint64_t overhead =
          128 + wordBytes * (3 * readPorts + 2) + ((*bits - 1) / 8 + 1);
      const uint64_t capacity = std::numeric_limits<std::ptrdiff_t>::max();
      if (!childMultiplicity || childMultiplicity > capacity / overhead)
        return site->emitOpError()
               << "memory storage including enclosing collections exceeds "
                  "the current Runtime object capacity";
      const uint64_t perLaneLimit = capacity / childMultiplicity;
      if (overhead >= perLaneLimit ||
          depthValue.getZExtValue() >
              (perLaneLimit - overhead) / entryBytes)
        return site->emitOpError()
               << "memory storage including enclosing collections exceeds "
                  "the current Runtime object capacity";
      return success();
    };
    for (Operation &operation : moduleBody(definition).front()) {
      for (Type type : operation.getResultTypes())
        if (failed(checkType(type, &operation)))
          return failure();
      if (auto queue = dyn_cast<hwir::QueueOp>(operation)) {
        auto config = resolveHWIRQueue(queue, bindings);
        if (failed(config))
          return failure();
        auto elementWidth = analysis.getPackedWidth(
            config->tokenElementType, bindings, queue);
        if (failed(elementWidth))
          return failure();
        const uint64_t scalarBytes =
            ((*elementWidth - 1) / 64 + 1) * 3 * sizeof(uint64_t);
        if (config->tokenCardinality > UINT64_MAX / scalarBytes)
          return queue.emitOpError() << "queue token representation overflows";
        const uint64_t tokenBytes = config->tokenCardinality * scalarBytes;
        const uint64_t capacity = std::numeric_limits<std::ptrdiff_t>::max();
        if (!multiplicity || multiplicity > capacity / 128)
          return queue.emitOpError()
                 << "queue storage including enclosing collections exceeds "
                    "the current Runtime object capacity";
        uint64_t laneCapacity = (capacity - 128) / multiplicity;
        const bool delayed = config->availabilityLatency > 1;
        const uint64_t fixedBytes = delayed ? 216 : 192;
        if (laneCapacity <= fixedBytes)
          return queue.emitOpError()
                 << "queue storage including enclosing collections exceeds "
                    "the current Runtime object capacity";
        uint64_t remaining = laneCapacity - fixedBytes;
        if (delayed) {
          if (config->depth > remaining / sizeof(uint64_t))
            return queue.emitOpError()
                   << "queue storage including enclosing collections exceeds "
                      "the current Runtime object capacity";
          remaining -= config->depth * sizeof(uint64_t);
        }
        const uint64_t tokenCapacity = remaining / tokenBytes;
        if (tokenCapacity < 5 || config->depth > tokenCapacity - 5)
          return queue.emitOpError()
                 << "queue storage including enclosing collections exceeds "
                    "the current Runtime object capacity";
      }
      hwir::InstanceOp instance = dyn_cast<hwir::InstanceOp>(operation);
      hwir::CollectionOp collection = dyn_cast<hwir::CollectionOp>(operation);
      if (instance || collection) {
        Operation *site = instance ? instance.getOperation()
                                   : collection.getOperation();
        Operation *callee = analysis.lookupDefinition(
            instance ? instance.getCallee() : collection.getCallee());
        auto child = analysis.bindCallee(
            callee,
            instance ? instance.getParametersAttr()
                     : collection.getParametersAttr(),
            instance ? instance.getTypeArgumentsAttr()
                     : collection.getTypeArgumentsAttr(),
            bindings, site);
        if (failed(child))
          return failure();
        uint64_t childMultiplicity = multiplicity;
        if (collection) {
          auto shape = analysis.resolveShape(collection.getShape(), bindings,
                                             collection);
          if (failed(shape))
            return failure();
        for (uint64_t extent : *shape) {
          if (!extent || childMultiplicity > UINT64_MAX / extent)
            return collection.emitOpError()
                   << "collection occurrence multiplicity overflows";
          childMultiplicity *= extent;
        }
        }
        if (failed(checkMemory(site, *child, childMultiplicity)))
          return failure();
        if (isa<hwir::ModuleOp>(child->owner) &&
            failed(preflightResources(child->owner, *child,
                                      childMultiplicity)))
          return failure();
      }
    }
    return success();
  }

  LogicalResult buildSystemPlan() {
    ConcreteModulePlan &root = plan.modules[plan.rootModule];
    auto kind = root.key.definition->getAttrOfType<StringAttr>(
        package->hasAttr("hwir.contract") ? "hwir.root_kind" : "ac.root_kind");
    if (!kind)
      return success();
    if (kind.getValue() != "system")
      return root.key.definition->emitOpError()
             << "structural C++ root_kind must be system";
    auto domain = root.key.definition->getAttrOfType<DictionaryAttr>(
        package->hasAttr("hwir.contract") ? "hwir.domain_inputs"
                                           : "ac.domain_inputs");
    auto clock = domain ? domain.getAs<IntegerAttr>("clock") : IntegerAttr();
    auto reset = domain ? domain.getAs<IntegerAttr>("reset") : IntegerAttr();
    if (!domain || domain.size() != 2 || !clock || !reset ||
        clock.getValue().getActiveBits() > 64 ||
        reset.getValue().getActiveBits() > 64)
      return root.key.definition->emitOpError()
             << "structural C++ system requires concrete clock/reset inputs";
    const uint64_t clockIndex = clock.getValue().getZExtValue();
    const uint64_t resetIndex = reset.getValue().getZExtValue();
    if (clockIndex == resetIndex || clockIndex >= root.inputs.size() ||
        resetIndex >= root.inputs.size() || root.inputs.size() != 2 ||
        !root.outputs.empty() ||
        clockIndex > std::numeric_limits<unsigned>::max() ||
        resetIndex > std::numeric_limits<unsigned>::max())
      return root.key.definition->emitOpError()
             << "structural C++ system must be closed except for one clock "
                "and one reset";
    plan.system = ConcreteSystemPlan{static_cast<unsigned>(clockIndex),
                                     static_cast<unsigned>(resetIndex)};
    return success();
  }

  LogicalResult buildSourceUnits() {
    auto units = ac::collectFinalSourceUnits(
        package, [&] { return package.emitOpError(); });
    if (failed(units))
      return failure();
    llvm::DenseMap<Attribute, unsigned> indices;
    for (const ac::FinalSourceUnitView &unit : *units) {
      unsigned index = plan.sourceUnits.size();
      indices.try_emplace(unit.owner, index);
      plan.sourceUnits.push_back({unit.owner, {}, {}});
    }
    llvm::SmallVector<unsigned, 0> moduleUnits(plan.modules.size());
    for (auto [moduleIndex, module] : llvm::enumerate(plan.modules)) {
      auto owner = moduleSourceOwner(module.key.definition);
      auto found = indices.find(owner);
      if (found == indices.end())
        return module.key.definition->emitOpError()
               << "structural C++ module owner is absent from final source "
                  "units";
      moduleUnits[moduleIndex] = found->second;
      plan.sourceUnits[found->second].moduleIndices.push_back(moduleIndex);
    }
    for (auto [moduleIndex, module] : llvm::enumerate(plan.modules)) {
      unsigned owner = moduleUnits[moduleIndex];
      for (const ConcreteChildPlan &child : module.children) {
        if (child.kind != ConcreteChildKind::Module || !child.moduleIndex)
          continue;
        unsigned dependency = moduleUnits[*child.moduleIndex];
        if (dependency != owner)
          plan.sourceUnits[owner].dependencies.push_back(dependency);
      }
    }
    for (ConcreteSourceUnitPlan &unit : plan.sourceUnits) {
      llvm::sort(unit.moduleIndices);
      llvm::sort(unit.dependencies);
      unit.dependencies.erase(
          std::unique(unit.dependencies.begin(), unit.dependencies.end()),
          unit.dependencies.end());
    }
    return success();
  }

  FailureOr<uint64_t> countSourceChecks(unsigned moduleIndex) {
    if (moduleIndex >= plan.modules.size())
      return failure();
    uint64_t count =
        llvm::count_if(plan.checks, [&](const ConcreteCheckPlan &check) {
          return check.moduleIndex == moduleIndex;
        });
    for (const ConcreteChildPlan &child : plan.modules[moduleIndex].children) {
      if (child.kind != ConcreteChildKind::Module)
        continue;
      if (!child.moduleIndex)
        return child.occurrence->emitOpError()
               << "structural C++ check count has an unresolved child";
      auto nested = countSourceChecks(*child.moduleIndex);
      if (failed(nested))
        return failure();
      if (*nested > std::numeric_limits<uint64_t>::max() - count)
        return child.occurrence->emitOpError()
               << "structural C++ source-check occurrence count overflows";
      count += *nested;
    }
    return count;
  }

  FailureOr<std::string> runtimeTypeName(Type type,
                                         const ac::HardwareBindings &bindings,
                                         Operation *site) {
    if (auto bits = dyn_cast<ac::BitsType>(type)) {
      auto width = analysis.getPackedWidth(type, bindings, site);
      if (failed(width))
        return failure();
      return (Twine("gfsim::Bits<") + Twine(*width) + ">").str();
    }
    if (auto table = dyn_cast<ac::TableType>(type)) {
      auto element = runtimeTypeName(table.getElementType(), bindings, site);
      if (failed(element))
        return failure();
      auto shape = analysis.getTableShape(table, bindings, site);
      if (failed(shape))
        return failure();
      std::string result = "gfsim::table<" + *element;
      for (uint64_t extent : *shape)
        result += ", " + std::to_string(extent);
      return result + ">";
    }
    auto qualified = [&](StringRef raw) -> FailureOr<std::string> {
      SmallVector<StringRef> parts;
      raw.split(parts, '.', -1, true);
      std::string result;
      for (StringRef part : parts) {
        auto name =
            legalizeIdentifier(part, [&] { return site->emitOpError(); });
        if (failed(name))
          return failure();
        if (!result.empty())
          result += "::";
        result += *name;
      }
      return result;
    };
    if (auto record = dyn_cast<ac::StructType>(type)) {
      auto name = qualified(record.getName().getValue());
      return failed(name) ? FailureOr<std::string>(failure())
                          : FailureOr<std::string>("::" + *name);
    }
    if (auto enumeration = dyn_cast<ac::EnumType>(type)) {
      if (failed(analysis.resolveEnum(enumeration, site)))
        return failure();
      auto name = qualified(enumeration.getName().getValue());
      return failed(name) ? FailureOr<std::string>(failure())
                          : FailureOr<std::string>("::" + *name);
    }
    return site->emitOpError()
           << "structural C++ encountered an unsupported runtime type";
  }

  void materializeDesignLifecycle() {
    const unsigned root = plan.rootModule;
    plan.lifecycle.work.push_back(
        {ConcreteLifecycleStepKind::InvokeRootWork, root, std::nullopt});
    plan.lifecycle.check.push_back(
        {ConcreteLifecycleStepKind::WholeSystemCheck, root, std::nullopt});
    plan.lifecycle.xfer.push_back(
        {ConcreteLifecycleStepKind::InvokeRootXfer, root, std::nullopt});
    plan.lifecycle.discard.push_back(
        {ConcreteLifecycleStepKind::InvokeRootDiscard, root, std::nullopt});
    plan.lifecycle.reset.push_back(
        {ConcreteLifecycleStepKind::InvokeRootReset, root, std::nullopt});
  }

  FailureOr<ConcreteTypeBinding>
  normalizeTypeBinding(StringAttr name, Type type,
                       const ac::HardwareBindings &bindings, Operation *site) {
    auto resolved = analysis.resolveType(type, bindings, site);
    if (failed(resolved) || isa<ac::TypeParamType>(*resolved))
      return failure();
    ConcreteTypeBinding result;
    result.name = name;
    result.resolvedType = *resolved;
    auto width = analysis.getPackedWidth(*resolved, bindings, site);
    if (failed(width) || *width == 0)
      return failure();
    result.packedWidth = *width;
    if (auto bits = dyn_cast<ac::BitsType>(*resolved)) {
      result.kind = ConcreteTypeKind::Bits;
      result.canonicalForm = "bits:" + std::to_string(*width);
    } else if (auto table = dyn_cast<ac::TableType>(*resolved)) {
      result.kind = ConcreteTypeKind::Table;
      auto shape = analysis.getTableShape(table, bindings, site);
      if (failed(shape))
        return failure();
      result.shape.assign(shape->begin(), shape->end());
      auto element =
          normalizeTypeBinding(name, table.getElementType(), bindings, site);
      if (failed(element))
        return failure();
      result.canonicalForm = "table[";
      for (uint64_t extent : result.shape)
        result.canonicalForm += std::to_string(extent) + ",";
      result.canonicalForm += "]:" + element->canonicalForm;
    } else if (auto record = dyn_cast<ac::StructType>(*resolved)) {
      result.kind = ConcreteTypeKind::Struct;
      result.canonicalForm = "struct:" + record.getName().getValue().str() +
                             ":" + std::to_string(*width);
    } else if (auto enumeration = dyn_cast<ac::EnumType>(*resolved)) {
      result.kind = ConcreteTypeKind::Enum;
      result.canonicalForm = "enum:" + enumeration.getName().getValue().str() +
                             ":" + std::to_string(*width);
    } else {
      return site->emitOpError()
             << "structural C++ cannot normalize a non-hardware type";
    }
    return result;
  }

  FailureOr<ConcreteModuleKey>
  normalizeConcreteModuleKey(Operation *definition,
                             const ac::HardwareBindings &bindings,
                             Operation *site) {
    ConcreteModuleKey key;
    key.definition = definition;
    for (Attribute raw : moduleParameters(definition)) {
      auto name = cast<DictionaryAttr>(raw).getAs<StringAttr>("name");
      auto value = dyn_cast_or_null<ac::MathIntAttr>(
          bindings.integers.lookup(name.getValue()));
      if (!value)
        return site->emitOpError()
               << "structural C++ requires concrete integer binding '"
               << name.getValue() << "'";
      key.integers.push_back({name, value.getCanonicalValue().str()});
    }
    for (Attribute raw : moduleTypeParameters(definition)) {
      auto name = cast<StringAttr>(raw);
      Type type = bindings.types.lookup(name.getValue());
      if (!type)
        return site->emitOpError()
               << "structural C++ requires concrete type binding '"
               << name.getValue() << "'";
      auto normalized = normalizeTypeBinding(name, type, bindings, site);
      if (failed(normalized))
        return failure();
      key.types.push_back(std::move(*normalized));
    }
    return key;
  }

  FailureOr<ConcretePrimitiveConfig>
  primitiveConfig(StringAttr kind, const ac::HardwareBindings &bindings,
                  Operation *site,
                  std::optional<ac::ResolvedQueueConfig> queue = std::nullopt) {
    ConcretePrimitiveConfig config;
    config.kind = kind;
    config.queue = std::move(queue);
    for (const auto &item : bindings.integers) {
      auto value = dyn_cast<ac::MathIntAttr>(item.getValue());
      if (!value)
        return site->emitOpError()
               << "structural C++ primitive has unresolved integer binding '"
               << item.getKey() << "'";
      config.integers.push_back(
          {StringAttr::get(package.getContext(), item.getKey()),
           value.getCanonicalValue().str()});
    }
    llvm::sort(config.integers, [](const auto &lhs, const auto &rhs) {
      return lhs.name.getValue() < rhs.name.getValue();
    });
    for (const auto &item : bindings.types) {
      auto name = StringAttr::get(package.getContext(), item.getKey());
      auto type = normalizeTypeBinding(name, item.getValue(), bindings, site);
      if (failed(type))
        return failure();
      config.types.push_back(std::move(*type));
    }
    llvm::sort(config.types, [](const auto &lhs, const auto &rhs) {
      return lhs.name.getValue() < rhs.name.getValue();
    });
    auto payload = [&]() -> FailureOr<const ConcreteTypeBinding *> {
      for (const ConcreteTypeBinding &type : config.types)
        if (type.name.getValue() == "T")
          return &type;
      return site->emitOpError()
             << "structural C++ primitive requires concrete payload type T";
    };
    auto integer = [&](StringRef name) -> FailureOr<StringRef> {
      for (const ConcreteIntegerBinding &value : config.integers)
        if (value.name.getValue() == name)
          return StringRef(value.canonicalValue);
      return site->emitOpError()
             << "structural C++ primitive requires concrete parameter '" << name
             << "'";
    };
    auto cppPayload = [&]() -> FailureOr<std::string> {
      auto type = payload();
      if (failed(type))
        return failure();
      return runtimeTypeName((*type)->resolvedType, bindings, site);
    };
    if (kind.getValue() == "dff" || kind.getValue() == "dffe") {
      auto type = cppPayload();
      if (failed(type))
        return failure();
      config.runtimeCppType =
          (Twine("gfsim::") + kind.getValue() + "<" + *type + ">").str();
      config.runtimeHeader = "gfsim/dff.h";
      config.inputMembers =
          kind.getValue() == "dff"
              ? SmallVector<std::string, 8>{"clk", "rst", "d", "init"}
              : SmallVector<std::string, 8>{"clk", "rst", "en", "d", "init"};
      config.outputMembers = {"q"};
      config.outputIsTemporal = {true};
      config.outputRefreshInputs = {{}};
      config.hasWork = true;
    } else if (kind.getValue() == "byte_mem" || kind.getValue() == "sync_mem" ||
               kind.getValue() == "sync_mem_dp") {
      auto type = cppPayload();
      auto addressWidth = integer("ADDR_WIDTH");
      auto depth = integer("DEPTH");
      if (failed(type) || failed(addressWidth) || failed(depth))
        return failure();
      config.runtimeCppType =
          (Twine("gfsim::") + kind.getValue() + "<" + *type + ", " +
           *addressWidth + ", " + *depth + ">")
              .str();
      config.runtimeHeader = kind.getValue() == "byte_mem" ? "gfsim/byte_mem.h"
                                                           : "gfsim/sync_mem.h";
      if (kind.getValue() == "sync_mem_dp") {
        config.inputMembers = {"clk",    "rst",    "ren0",  "raddr0", "ren1",
                               "raddr1", "wvalid", "waddr", "wdata",  "wstrb"};
        config.outputMembers = {"rdata0", "rdata1"};
        config.outputIsTemporal = {true, true};
        config.outputRefreshInputs = {{}, {}};
      } else if (kind.getValue() == "sync_mem") {
        config.inputMembers = {"clk",    "rst",   "ren",   "raddr",
                               "wvalid", "waddr", "wdata", "wstrb"};
        config.outputMembers = {"rdata"};
        config.outputIsTemporal = {true};
        config.outputRefreshInputs = {{}};
      } else {
        config.inputMembers = {"clk",   "rst",   "raddr", "wvalid",
                               "waddr", "wdata", "wstrb"};
        config.outputMembers = {"rdata"};
        // byte_mem is an asynchronous address-dependent read. It must run
        // after its current address input is driven and is not a temporal cut.
        config.outputIsTemporal = {false};
        config.outputRefreshInputs = {{2}};
        config.hasPureOutputRefresh = true;
      }
      config.hasWork = true;
    } else if (kind.getValue() == "fifo") {
      if (!config.queue)
        return site->emitOpError()
               << "structural C++ fifo requires resolved queue configuration";
      // The FIFO primitive's T is its complete token type.  It is a distinct
      // binding namespace from an enclosing generic module's T; in
      // particular, queue<table<T, ...>> must not silently inherit the table
      // element type and truncate every stored token to one lane.
      llvm::erase_if(config.types, [](const ConcreteTypeBinding &type) {
        return type.name.getValue() == "T";
      });
      auto payloadBinding = normalizeTypeBinding(
          StringAttr::get(package.getContext(), "T"),
          config.queue->payloadType, bindings, site);
      if (failed(payloadBinding))
        return failure();
      config.types.push_back(std::move(*payloadBinding));
      llvm::sort(config.types, [](const auto &lhs, const auto &rhs) {
        return lhs.name.getValue() < rhs.name.getValue();
      });
      auto type = runtimeTypeName(config.queue->payloadType, bindings, site);
      if (failed(type))
        return failure();
      StringRef policy =
          config.queue->readyPolicy == ac::QueueReadyPolicy::LocalOccupancy
              ? "LocalOccupancy"
              : "DownstreamPop";
      config.runtimeCppType =
          (Twine("gfsim::fifo<") + *type + ", " + Twine(config.queue->depth) +
           ", gfsim::QueueReadyPolicy::" + policy + ", " +
           Twine(config.queue->availabilityLatency) + ">")
              .str();
      config.runtimeHeader = "gfsim/fifo.h";
      config.inputMembers = {"clk", "rst", "in_valid", "in_data", "out_ready"};
      config.outputMembers = {"in_ready", "out_valid", "out_data"};
      // FIFO valid/data are committed-state views. Local-occupancy ready is
      // also state-only; downstream-pop ready additionally depends on the
      // current out_ready pin and therefore cannot cross the Work boundary.
      config.outputIsTemporal = {config.queue->readyPolicy ==
                                     ac::QueueReadyPolicy::LocalOccupancy,
                                 true, true};
      config.outputRefreshInputs = {
          config.queue->readyPolicy == ac::QueueReadyPolicy::LocalOccupancy
              ? llvm::SmallVector<unsigned, 1>{}
              : llvm::SmallVector<unsigned, 1>{4},
          {}, {}};
      config.hasPureOutputRefresh = true;
      config.hasWork = true;
    }
    return config;
  }

  LogicalResult buildDependencyGraph() {
    auto &graph = plan.dependencies;
    auto isUsedOperand = [](Operation *consumer, unsigned operand) {
      if (!isa<ac::RuleOp, hwir::RuleOp>(consumer) ||
          consumer->getRegion(0).empty() ||
          operand >= consumer->getRegion(0).front().getNumArguments())
        return true;
      // Source lowering closes a rule over every value visible at its
      // definition site.  Unused captures are ABI slots, not combinational
      // dependencies.  The rule body is the semantic authority for whether
      // the corresponding operand participates in the result.
      return !consumer->getRegion(0).front().getArgument(operand).use_empty();
    };

    // Expand nodes per concrete hierarchy occurrence, not merely per module
    // specialization. Several instances may deliberately reuse one
    // specialization (and therefore the same definition-local Operation
    // pointers); folding those instances onto one node turns an ordinary
    // chain such as child0 -> child1 into a false self-cycle.
    using OccurrenceNodes = llvm::DenseMap<Operation *, unsigned>;
    std::function<OccurrenceNodes(unsigned)> expandOccurrence =
        [&](unsigned moduleIndex) -> OccurrenceNodes {
      ConcreteModulePlan &module = plan.modules[moduleIndex];
      OccurrenceNodes nodes;
      for (Operation *operation : module.schedule.schedule) {
        unsigned node = graph.nodes.size();
        graph.nodes.push_back({moduleIndex, operation});
        nodes[operation] = node;
      }
      llvm::DenseSet<Operation *> expandedModuleInstances;
      for (const ConcreteChildPlan &child : module.children)
        if (child.kind == ConcreteChildKind::Module && child.moduleIndex &&
            child.occurrence &&
            isa<ac::InstanceOp, hwir::InstanceOp>(child.occurrence))
          expandedModuleInstances.insert(child.occurrence);

      auto addEdge = [&](const OccurrenceNodes &fromNodes, Operation *from,
                         const OccurrenceNodes &toNodes, Operation *to) {
        auto fromIt = fromNodes.find(from);
        auto toIt = toNodes.find(to);
        if (fromIt != fromNodes.end() && toIt != toNodes.end())
          graph.edges.push_back({fromIt->second, toIt->second});
      };

      for (Operation *consumer : module.schedule.schedule) {
        for (auto [operandIndex, operand] :
             llvm::enumerate(consumer->getOperands())) {
          if (!isUsedOperand(consumer, operandIndex))
            continue;
          Operation *producer = operand.getDefiningOp();
          if (!producer)
            continue;
          // Ordinary module instances are expanded across their input/output
          // boundary below. Treating the instance op itself as a monolithic
          // combinational node would reconnect temporal outputs to next-state
          // inputs and manufacture a cycle around a stateful child.
          if (expandedModuleInstances.contains(producer) ||
              expandedModuleInstances.contains(consumer))
            continue;
          auto producerIt = nodes.find(producer);
          auto consumerIt = nodes.find(consumer);
          auto producerOrder = llvm::find(module.schedule.schedule, producer);
          auto consumerOrder = llvm::find(module.schedule.schedule, consumer);
          // A producer after its consumer is a temporal cut (for example a
          // queue head or register Q), not a combinational edge. Shared
          // HardwareAnalysis has already validated that cut.
          if (producerIt != nodes.end() && consumerIt != nodes.end() &&
              producerOrder != module.schedule.schedule.end() &&
              consumerOrder != module.schedule.schedule.end() &&
              producerOrder < consumerOrder)
            graph.edges.push_back({producerIt->second, consumerIt->second});
        }
      }

      // Fold every ordinary module occurrence across its own boundary. Block
      // arguments connect to parent producers; yielded values connect to
      // parent users of this particular instance result.
      for (const ConcreteChildPlan &child : module.children) {
        if (child.kind != ConcreteChildKind::Module || !child.moduleIndex ||
            !child.occurrence)
          continue;
        Operation *instance = child.occurrence;
        if (!isa<ac::InstanceOp, hwir::InstanceOp>(instance))
          continue;
        ConcreteModulePlan &nested = plan.modules[*child.moduleIndex];
        OccurrenceNodes nestedNodes = expandOccurrence(*child.moduleIndex);
        Block &body = moduleBody(nested.key.definition).front();
        for (Operation *consumer : nested.schedule.schedule)
          for (Value operand : consumer->getOperands())
            if (auto blockArg = dyn_cast<BlockArgument>(operand)) {
              unsigned ordinal = blockArg.getArgNumber();
              if (ordinal < instance->getNumOperands())
                if (Operation *producer =
                        instance->getOperand(ordinal).getDefiningOp())
                  addEdge(nodes, producer, nestedNodes, consumer);
            }
        Operation *yield = &body.back();
        if (!isa<ac::YieldOp, hwir::YieldOp>(yield) ||
            !nested.schedule.storageWork.empty())
          continue;
        for (auto [result, value] : llvm::enumerate(yield->getOperands())) {
          if (result < nested.outputIsTemporal.size() &&
              nested.outputIsTemporal[result])
            continue;
          Operation *producer = value.getDefiningOp();
          if (!producer)
            continue;
          for (Operation *consumer : module.schedule.schedule) {
            for (auto [operandIndex, operand] :
                 llvm::enumerate(consumer->getOperands())) {
              if (!isUsedOperand(consumer, operandIndex))
                continue;
              auto instanceResult = dyn_cast<OpResult>(operand);
              if (operand.getDefiningOp() == instance && instanceResult &&
                  instanceResult.getResultNumber() == result)
                addEdge(nestedNodes, producer, nodes, consumer);
            }
          }
        }
      }
      return nodes;
    };

    (void)expandOccurrence(plan.rootModule);

    // This graph is retained for diagnostics and source attribution. It must
    // not become a second scheduler: definition-level SSA cannot precisely
    // encode temporal cuts through stateful module outputs. The value-level
    // materializeSchedule topological sort above is the semantic admission
    // authority and already fails when no executable ordering exists.
    return success();
  }

  Value flattenRegionValue(Value value) {
    if (auto argument = dyn_cast<BlockArgument>(value)) {
      Operation *owner = argument.getOwner()->getParentOp();
      if (owner && !isa<ac::ModuleOp, hwir::ModuleOp>(owner) &&
          argument.getArgNumber() < owner->getNumOperands())
        return flattenRegionValue(owner->getOperand(argument.getArgNumber()));
    }
    if (auto result = dyn_cast<OpResult>(value)) {
      Operation *owner = result.getOwner();
      for (OpOperand &use : result.getUses()) {
        Operation *yield = use.getOwner();
        if (!isa<ac::YieldOp, hwir::YieldOp>(yield))
          continue;
        Operation *owner = yield->getParentOp();
        if (owner && !isa<ac::ModuleOp, hwir::ModuleOp>(owner) &&
            use.getOperandNumber() < owner->getNumResults())
          return flattenRegionValue(owner->getResult(use.getOperandNumber()));
      }
    }
    return value;
  }

  FailureOr<ConcreteValueId> valueId(const ConcreteModulePlan &module,
                                     Value value, Operation *site) {
    value = flattenRegionValue(value);
    auto moduleValues = valueIds.find(canonicalModuleKey(module.key));
    if (moduleValues == valueIds.end())
      return site->emitOpError()
             << "structural C++ module has no value-ID namespace";
    auto found = moduleValues->second.find(value);
    if (found != moduleValues->second.end())
      return found->second;
    return site->emitOpError()
           << "structural C++ value is outside the module value plan";
  }

  ConcreteValueId addSyntheticValue(
      ConcreteModulePlan &module, ConcreteValueKind kind, ConcreteTypePlan type,
      std::string cppName,
      std::optional<ConcreteValueId> availabilityParent = std::nullopt,
      std::optional<uint64_t> tableLane = std::nullopt) {
    ConcreteValueId id = module.values.size();
    module.values.push_back(
        {id, kind, std::move(type), std::move(cppName), availabilityParent,
         tableLane});
    return id;
  }

  // Maps one approved scalar compute operation to its renderable plan. The
  // caller supplies the concrete input/output value IDs; this returns only the
  // opcode/immediate payload. Shared by module schedules and per-lane table.map
  // unrolling so both render through the exact same compute kind set.
  FailureOr<ConcreteComputePlan> computePlan(Operation *operation,
                                             ConcreteModulePlan &module) {
    if (auto constant = dyn_cast<ac::BitsConstantOp>(operation)) {
      auto integer = analysis.evaluateStatic(constant.getValue(),
                                             module.bindings, operation);
      if (failed(integer))
        return failure();
      auto value = dyn_cast<ac::MathIntAttr>(*integer);
      if (!value)
        return operation->emitOpError()
               << "structural C++ constant is not an integer";
      llvm::APSInt parsed(value.getCanonicalValue());
      unsigned width =
          concreteWidth(operation->getResult(0).getType(), module, operation);
      if (!width)
        return failure();
      llvm::APInt number = parsed.isSigned() ? parsed.sextOrTrunc(width)
                                             : parsed.zextOrTrunc(width);
      ConcreteComputePlan compute;
      compute.kind = ConcreteComputeKind::BitsConstant;
      for (unsigned i = 0; i < number.getNumWords(); ++i)
        compute.constantWords.push_back(number.getRawData()[i]);
      return compute;
    }
    if (auto constant = dyn_cast<ac::BitsConstant4Op>(operation)) {
      unsigned width =
          concreteWidth(operation->getResult(0).getType(), module, operation);
      if (!width || constant.getPattern().size() != width)
        return operation->emitOpError()
               << "structural C++ constant4 has an invalid concrete width";
      llvm::APInt value(width, 0), known(width, 0), z(width, 0);
      for (auto [sourceIndex, symbol] :
           llvm::enumerate(constant.getPattern())) {
        unsigned bit = width - 1 - sourceIndex;
        if (symbol == '0' || symbol == '1') {
          known.setBit(bit);
          if (symbol == '1')
            value.setBit(bit);
        } else if (symbol == 'z') {
          z.setBit(bit);
        }
      }
      ConcreteComputePlan compute;
      compute.kind = ConcreteComputeKind::BitsConstant4;
      for (unsigned word = 0; word < value.getNumWords(); ++word) {
        compute.constantWords.push_back(value.getRawData()[word]);
        compute.knownWords.push_back(known.getRawData()[word]);
        compute.zWords.push_back(z.getRawData()[word]);
      }
      return compute;
    }
    if (auto unary = dyn_cast<ac::BitsUnaryOp>(operation)) {
      return ConcreteComputePlan{
          ConcreteComputeKind::BitsUnary, "bit_not", {}, {}, std::nullopt};
    }
    if (auto binary = dyn_cast<ac::BitsBinaryOp>(operation)) {
      return ConcreteComputePlan{
          ConcreteComputeKind::BitsBinary,
          llvm::StringSwitch<StringRef>(binary.getOpcode())
              .Case("and", "bit_and")
              .Case("or", "bit_or")
              .Case("xor", "bit_xor")
              .Default(binary.getOpcode())
              .str(),
          {},
          {},
          std::nullopt};
    }
    if (auto compare = dyn_cast<ac::BitsCompareOp>(operation)) {
      return ConcreteComputePlan{ConcreteComputeKind::BitsCompare,
                                 compare.getPredicate().upper(),
                                 {},
                                 {},
                                 std::nullopt};
    }
    if (isa<ac::BitsSelectOp>(operation))
      return ConcreteComputePlan{
          ConcreteComputeKind::BitsSelect, "select", {}, {}, std::nullopt};
    if (isa<ac::BitsConcatOp>(operation))
      return ConcreteComputePlan{
          ConcreteComputeKind::BitsConcat, "concat", {}, {}, std::nullopt};
    if (auto extract = dyn_cast<ac::BitsExtractOp>(operation)) {
      auto value =
          analysis.evaluateStatic(extract.getLow(), module.bindings, operation);
      auto integer = succeeded(value) ? dyn_cast<ac::MathIntAttr>(*value)
                                      : ac::MathIntAttr();
      if (!integer || integer.getCanonicalValue().starts_with("-"))
        return operation->emitOpError()
               << "structural C++ extract requires a concrete low bound";
      uint64_t low = 0;
      if (integer.getCanonicalValue().getAsInteger(10, low))
        return operation->emitOpError()
               << "structural C++ extract low bound exceeds uint64";
      return ConcreteComputePlan{
          ConcreteComputeKind::BitsExtract, "extract", {}, {}, low};
    }
    if (auto resize = dyn_cast<ac::BitsResizeOp>(operation)) {
      return ConcreteComputePlan{ConcreteComputeKind::BitsResize,
                                 resize.getMode().str(),
                                 {},
                                 {},
                                 std::nullopt};
    }
    if (isa<ac::StructCreateOp>(operation))
      return ConcreteComputePlan{
          ConcreteComputeKind::BitsConcat, "concat", {}, {}, std::nullopt};
    if (auto get = dyn_cast<ac::StructGetOp>(operation)) {
      auto declaration = analysis.lookupStruct(get.getValue().getType());
      if (!declaration)
        return operation->emitOpError()
               << "structural C++ cannot resolve struct projection";
      uint64_t low = 0;
      bool found = false;
      for (Attribute raw : llvm::reverse(declaration.getFields())) {
        auto field = cast<DictionaryAttr>(raw);
        if (field.getAs<StringAttr>("name") == get.getFieldAttr()) {
          found = true;
          break;
        }
        auto width =
            analysis.getPackedWidth(field.getAs<TypeAttr>("type").getValue(),
                                    module.bindings, operation);
        if (failed(width))
          return failure();
        low += *width;
      }
      if (!found)
        return operation->emitOpError()
               << "structural C++ cannot locate struct field";
      return ConcreteComputePlan{
          ConcreteComputeKind::BitsExtract, "extract", {}, {}, low};
    }
    if (auto create = dyn_cast<ac::EnumCreateOp>(operation)) {
      auto view = analysis.resolveEnum(create.getResult().getType(), operation);
      if (failed(view))
        return failure();
      const ac::EnumMember *member = nullptr;
      for (const ac::EnumMember &candidate : view->members)
        if (candidate.name == create.getMemberAttr())
          member = &candidate;
      if (!member)
        return operation->emitOpError()
               << "structural C++ cannot resolve enum member";
      llvm::APSInt parsed(member->code.getCanonicalValue());
      llvm::APInt number = parsed.zextOrTrunc(view->width);
      ConcreteComputePlan compute;
      compute.kind = ConcreteComputeKind::BitsConstant;
      for (unsigned i = 0; i < number.getNumWords(); ++i)
        compute.constantWords.push_back(number.getRawData()[i]);
      return compute;
    }
    if (isa<ac::EnumToBitsOp>(operation))
      return ConcreteComputePlan{ConcreteComputeKind::PackedIdentity,
                                 "identity",
                                 {},
                                 {},
                                 std::nullopt};
    if (auto fromBits = dyn_cast<ac::EnumFromBitsOp>(operation)) {
      auto view = analysis.resolveEnum(fromBits.getValue().getType(), fromBits);
      if (failed(view))
        return failure();
      ConcreteComputePlan compute;
      compute.kind = ConcreteComputeKind::EnumFromBits;
      for (const ac::EnumMember &member : view->members) {
        llvm::APSInt parsed(member.code.getCanonicalValue());
        llvm::APInt number = parsed.zextOrTrunc(view->width);
        llvm::SmallVector<uint64_t, 2> words;
        for (unsigned index = 0; index < number.getNumWords(); ++index)
          words.push_back(number.getRawData()[index]);
        compute.enumCodes.push_back(std::move(words));
      }
      return compute;
    }
    if (isa<ac::TableSplatOp, hwir::TableSplatOp>(operation)) {
      ConcreteComputePlan compute{
          ConcreteComputeKind::BitsConcat, "splat", {}, {}, std::nullopt};
      auto inputType = concreteType(operation->getOperand(0).getType(),
                                    module.bindings, operation);
      auto outputType = concreteType(operation->getResult(0).getType(),
                                     module.bindings, operation);
      if (failed(inputType) || failed(outputType) ||
          inputType->packedWidth == 0 ||
          outputType->packedWidth % inputType->packedWidth != 0)
        return operation->emitOpError()
               << "structural C++ table splat has incompatible packed widths";
      compute.immediate = outputType->packedWidth / inputType->packedWidth;
      return compute;
    }
    if (auto view = dyn_cast<hwir::TableViewOp>(operation)) {
      ConcreteComputePlan compute{ConcreteComputeKind::TableView, view.getKind().str(), {}, {}, std::nullopt};
      auto shape = analysis.getTableShape(view.getInput().getType(), module.bindings, view);
      auto outShape = analysis.getTableShape(view.getResult().getType(), module.bindings, view);
      if (failed(shape) || failed(outShape))
        return failure();
      for (uint64_t lane = 0, count = shapeProduct(*outShape); lane < count; ++lane) {
        auto mapped = [&]() -> FailureOr<uint64_t> {
          // Reuse the shared ACIR view mapping through a temporary operation-shaped
          // calculation; the HWIR operation carries the same canonical attributes.
          SmallVector<uint64_t> coords(outShape->size()), source(shape->size());
          uint64_t ordinal = lane;
          for (size_t i = outShape->size(); i-- > 0;) { coords[i] = ordinal % (*outShape)[i]; ordinal /= (*outShape)[i]; }
          source = coords;
          auto params = view.getParameters();
          if (view.getKind() == "reshape")
            return lane;
          else if (view.getKind() == "slice") {
            auto offsets = params.getAs<ArrayAttr>("offsets"); auto strides = params.getAs<ArrayAttr>("strides");
            if (!offsets || !strides) return failure();
            for (unsigned i = 0; i < source.size(); ++i) {
              auto o = dyn_cast<ac::StaticExprAttr>(offsets[i]); auto s = dyn_cast<ac::StaticExprAttr>(strides[i]);
              if (!o || !s) return failure();
              auto ov = analysis.evaluateStatic(o, module.bindings, view); auto sv = analysis.evaluateStatic(s, module.bindings, view);
              if (failed(ov) || failed(sv)) return failure();
              auto oi = dyn_cast<ac::MathIntAttr>(*ov); auto si = dyn_cast<ac::MathIntAttr>(*sv);
              if (!oi || !si) return failure();
              llvm::APSInt offsetValue(oi.getCanonicalValue());
              llvm::APSInt strideValue(si.getCanonicalValue());
              if (offsetValue.isNegative() || strideValue.isNegative() ||
                  offsetValue.getActiveBits() > 64 ||
                  strideValue.getActiveBits() > 64)
                return failure();
              source[i] = offsetValue.getZExtValue() +
                          coords[i] * strideValue.getZExtValue();
            }
          } else if (view.getKind() == "transpose") {
            auto axes = params.getAs<ArrayAttr>("axes");
            if (!axes || axes.size() != source.size())
              return view.emitOpError() << "table transpose has invalid axes";
            for (unsigned i = 0; i < source.size(); ++i) {
              auto axis = dyn_cast<ac::StaticExprAttr>(axes[i]);
              auto value = axis ? analysis.evaluateStatic(
                                      axis, module.bindings, view)
                                : FailureOr<Attribute>(failure());
              auto integer = succeeded(value)
                                 ? dyn_cast<ac::MathIntAttr>(*value)
                                 : ac::MathIntAttr();
              if (!integer)
                return view.emitOpError()
                       << "table transpose axis is not concrete";
              llvm::APSInt parsed(integer.getCanonicalValue());
              if (parsed.isNegative() || parsed.getActiveBits() > 64 ||
                  parsed.getZExtValue() >= source.size())
                return view.emitOpError()
                       << "table transpose axis is out of range";
              source[parsed.getZExtValue()] = coords[i];
            }
          } else if (view.getKind() == "rotate") {
            auto axis = dyn_cast<ac::StaticExprAttr>(params.get("axis"));
            auto offset = dyn_cast<ac::StaticExprAttr>(params.get("offset"));
            auto axisValue = axis ? analysis.evaluateStatic(
                                        axis, module.bindings, view)
                                  : FailureOr<Attribute>(failure());
            auto offsetValue = offset ? analysis.evaluateStatic(
                                            offset, module.bindings, view)
                                      : FailureOr<Attribute>(failure());
            auto axisInteger = succeeded(axisValue)
                                   ? dyn_cast<ac::MathIntAttr>(*axisValue)
                                   : ac::MathIntAttr();
            auto offsetInteger = succeeded(offsetValue)
                                     ? dyn_cast<ac::MathIntAttr>(*offsetValue)
                                     : ac::MathIntAttr();
            if (!axisInteger || !offsetInteger)
              return view.emitOpError()
                     << "table rotate parameters are not concrete";
            llvm::APSInt axisParsed(axisInteger.getCanonicalValue());
            if (axisParsed.isNegative() || axisParsed.getActiveBits() > 64 ||
                axisParsed.getZExtValue() >= source.size())
              return view.emitOpError() << "table rotate axis is out of range";
            const uint64_t ax = axisParsed.getZExtValue();
            const uint64_t extent = (*shape)[ax];
            llvm::APSInt shift(offsetInteger.getCanonicalValue());
            // Static offsets are mathematical integers and can be wider than
            // a host int64. Reduce them in arbitrary precision before
            // converting the bounded lane offset to uint64.
            const unsigned bits = std::max<unsigned>(
                65, shift.getBitWidth() + unsigned(shift.isUnsigned()));
            llvm::APInt modulus(bits, extent);
            llvm::APInt extended = shift.isUnsigned()
                                       ? shift.zextOrTrunc(bits)
                                       : shift.sextOrTrunc(bits);
            llvm::APInt remainder = extended.srem(modulus);
            if (remainder.isNegative())
              remainder += modulus;
            source[ax] = (coords[ax] + remainder.getZExtValue()) % extent;
          } else {
            return view.emitOpError() << "unsupported table view kind '"
                                      << view.getKind() << "'";
          }
          uint64_t mappedOrdinal = 0; for (unsigned i = 0; i < source.size(); ++i) mappedOrdinal = mappedOrdinal * (*shape)[i] + source[i]; return mappedOrdinal;
        }();
        if (failed(mapped)) return failure();
        compute.laneMap.push_back(*mapped);
      }
      return compute;
    }
    if (auto index = dyn_cast<hwir::TableIndexOp>(operation)) {
      ConcreteComputePlan compute{ConcreteComputeKind::TableIndex, "index", {}, {}, std::nullopt};
      auto shape = analysis.resolveShape(index.getShape(), module.bindings, index);
      if (failed(shape)) return failure();
      compute.laneMap.append(shape->begin(), shape->end());
      return compute;
    }
    if (isa<hwir::TableMatchOp>(operation))
      return ConcreteComputePlan{ConcreteComputeKind::TableMatch, "match", {}, {}, std::nullopt};
    if (auto choose = dyn_cast<hwir::TableChooseOp>(operation))
      return ConcreteComputePlan{ConcreteComputeKind::TableChoose, choose.getOrder().str(), {}, {},
                                 static_cast<uint64_t>(choose.getCount())};
    if (auto fold = dyn_cast<hwir::TableFoldOp>(operation))
      return ConcreteComputePlan{ConcreteComputeKind::TableFold, fold.getKind().str(), {}, {}, std::nullopt};
    if (auto merge = dyn_cast<hwir::ValueMergeOp>(operation)) {
      ConcreteComputePlan compute{ConcreteComputeKind::ValueMerge, "merge", {}, {}, std::nullopt};
      auto baseType = concreteType(merge.getBase().getType(), module.bindings,
                                   merge);
      if (failed(baseType))
        return failure();
      for (Attribute raw : merge.getPaths()) {
        auto path = cast<ArrayAttr>(raw);
        uint64_t low = std::numeric_limits<uint64_t>::max(), high = 0;
        for (const auto &field : baseType->fields) {
          if (path.size() > field.path.size()) continue;
          bool prefix = true;
          for (auto [i, component] : llvm::enumerate(path))
            prefix &= cast<StringAttr>(component).getValue() == field.path[i];
          if (!prefix) continue;
          low = std::min(low, field.low); high = std::max(high, field.low + field.width);
        }
        if (path.empty()) { low = 0; high = concreteWidth(merge.getBase().getType(), module, merge); }
        if (low == std::numeric_limits<uint64_t>::max() || high <= low)
          return merge.emitOpError() << "structural C++ cannot resolve merge field path";
        compute.laneMap.push_back(low); compute.laneMap.push_back(high - low);
      }
      return compute;
    }
    if (isa<ac::TableCreateOp, hwir::TableCreateOp>(operation))
      return ConcreteComputePlan{
          ConcreteComputeKind::TableCreate, "create", {}, {}, std::nullopt};
    if (isa<ac::TableGetOp, hwir::TableGetOp>(operation))
      return ConcreteComputePlan{
          ConcreteComputeKind::TableGet, "get", {}, {}, std::nullopt};
    return operation->emitOpError()
           << "structural C++ has no approved compute mapping";
  }

  unsigned concreteWidth(Type type, ConcreteModulePlan &module,
                         Operation *site) {
    auto width = analysis.getPackedWidth(type, module.bindings, site);
    if (failed(width))
      return 0;
    return *width;
  }

  LogicalResult materializeValues(unsigned index) {
    auto &module = plan.modules[index];
    auto &moduleValueIds = valueIds[canonicalModuleKey(module.key)];
    Block &body = moduleBody(module.key.definition).front();
    auto addValue = [&](Value source, ConcreteValueKind kind,
                        StringRef prefix) -> LogicalResult {
      source = flattenRegionValue(source);
      if (moduleValueIds.contains(source))
        return success();
      auto type = concreteType(source.getType(), module.bindings,
                               module.key.definition);
      if (failed(type))
        return failure();
      ConcreteValueId id = module.values.size();
      module.values.push_back({id, kind, *type, (prefix + Twine(id)).str()});
      moduleValueIds.try_emplace(source, id);
      return success();
    };

    for (BlockArgument argument : body.getArguments())
      if (failed(addValue(argument, ConcreteValueKind::ModuleInput, "input_")))
        return failure();
    auto walk = body.walk([&](Operation *operation) {
      for (Value result : operation->getResults())
        if (failed(
                addValue(result, ConcreteValueKind::OperationResult, "value_")))
          return WalkResult::interrupt();
      return WalkResult::advance();
    });
    if (walk.wasInterrupted())
      return failure();

    auto signature = moduleType(module.key.definition);
    ArrayAttr inputNames = moduleInputNames(module.key.definition);
    for (auto [ordinal, item] :
         llvm::enumerate(llvm::zip_equal(signature.getInputs(), inputNames))) {
      auto [type, rawName] = item;
      auto concrete =
          concreteType(type, module.bindings, module.key.definition);
      if (failed(concrete))
        return failure();
      auto cppName =
          legalizeIdentifier(cast<StringAttr>(rawName).getValue(), [&] {
            return module.key.definition->emitOpError();
          });
      if (failed(cppName))
        return failure();
      module.inputs.push_back({cast<StringAttr>(rawName), *concrete,
                               static_cast<ConcreteValueId>(ordinal),
                               std::move(*cppName)});
    }

    Operation *yield = &body.back();
    if (!isa<ac::YieldOp, hwir::YieldOp>(yield))
      return module.key.definition->emitOpError()
             << "structural C++ module has no output yield";
    ArrayAttr outputNames = moduleOutputNames(module.key.definition);
    for (auto [type, rawName, value] : llvm::zip_equal(
             signature.getResults(), outputNames, yield->getOperands())) {
      auto concrete =
          concreteType(type, module.bindings, module.key.definition);
      auto id = valueId(module, value, yield);
      auto cppName =
          legalizeIdentifier(cast<StringAttr>(rawName).getValue(), [&] {
            return module.key.definition->emitOpError();
          });
      if (failed(concrete) || failed(id) || failed(cppName))
        return failure();
      module.outputs.push_back(
          {cast<StringAttr>(rawName), *concrete, *id, std::move(*cppName)});
    }
    return success();
  }

  // Expands one closed table.map into flat per-lane compute steps. The body
  // receives the lane ordinal plus the element of each table operand for the
  // current lane (and any scalar captures); each result is assigned to the
  // corresponding lane element of the result table via a PackedIdentity step.
  // A source aggregate update can place another table.map in the body. In that
  // case `enclosingIds` is the outer lane's value environment: the nested map
  // consumes those local table values, publishes synthetic result tables back
  // into the same environment, and recursively owns its lane barriers.
  template <typename TableMapT>
  LogicalResult unrollTableMap(
      TableMapT map, ConcreteModulePlan &module,
      llvm::DenseMap<Value, ConcreteValueId> *enclosingIds = nullptr) {
    auto shape = analysis.resolveShape(map.getShape(), module.bindings, map);
    if (failed(shape))
      return failure();
    uint64_t lanes = shapeProduct(*shape);
    if (lanes == 0)
      return map.emitOpError() << "table.map has a non-positive shape";
    uint64_t ordinalWidth = shapeIndexWidth(lanes);

    llvm::SmallVector<ConcreteValueId> tableOperands;
    llvm::SmallVector<ConcreteTypePlan> elementTypes;
    for (Value operand : map.getTables()) {
      FailureOr<ConcreteValueId> id = failure();
      if (enclosingIds) {
        auto found = enclosingIds->find(operand);
        if (found == enclosingIds->end())
          return map.emitOpError()
                 << "nested table.map table operand is not closed over the "
                    "enclosing lane";
        id = found->second;
      } else {
        id = valueId(module, operand, map);
        if (failed(id))
          return failure();
      }
      auto table = cast<ac::TableType>(operand.getType());
      auto element = concreteType(table.getElementType(), module.bindings, map);
      if (failed(element))
        return failure();
      tableOperands.push_back(*id);
      elementTypes.push_back(std::move(*element));
    }
    llvm::SmallVector<ConcreteValueId> resultTables;
    llvm::SmallVector<ConcreteTypePlan> resultElementTypes;
    for (Value result : map.getResults()) {
      auto table = cast<ac::TableType>(result.getType());
      auto element = concreteType(table.getElementType(), module.bindings, map);
      if (failed(element))
        return failure();
      ConcreteValueId id = 0;
      if (enclosingIds) {
        auto type = concreteType(result.getType(), module.bindings, map);
        if (failed(type))
          return failure();
        id = addSyntheticValue(
            module, ConcreteValueKind::OperationResult, *type,
            ("map_table_" + Twine(module.values.size())).str());
        enclosingIds->try_emplace(result, id);
      } else {
        auto mapped = valueId(module, result, map);
        if (failed(mapped))
          return failure();
        id = *mapped;
      }
      resultTables.push_back(id);
      resultElementTypes.push_back(std::move(*element));
    }

    Block &body = map.getBody().front();
    auto arguments = body.getArguments();
    size_t expectedArgs = 1 + map.getTables().size() + map.getCaptures().size();
    if (arguments.size() != expectedArgs)
      return map.emitOpError()
             << "table.map body arity does not match operands";
    Operation *yield = &body.back();
    llvm::SmallVector<llvm::SmallVector<ConcreteValueId, 4>, 2> resultElements(
        resultTables.size());

    auto constantStep = [&](uint64_t value, unsigned width,
                            ConcreteValueId output) {
      llvm::APInt number(width, value);
      ConcreteComputePlan compute;
      compute.kind = ConcreteComputeKind::BitsConstant;
      for (unsigned w = 0; w < number.getNumWords(); ++w)
        compute.constantWords.push_back(number.getRawData()[w]);
      ConcreteScheduleStep step;
      step.kind = ConcreteScheduleStepKind::ComputeValue;
      step.operation = map;
      step.outputs.push_back(output);
      step.compute = std::move(compute);
      module.concreteSchedule.steps.push_back(std::move(step));
    };

    for (uint64_t lane = 0; lane < lanes; ++lane) {
      llvm::DenseMap<Value, ConcreteValueId> localIds;
      auto ordinal = concreteType(arguments[0].getType(), module.bindings, map);
      if (failed(ordinal))
        return failure();
      ConcreteValueId ordinalId = addSyntheticValue(
          module, ConcreteValueKind::OperationResult, *ordinal,
          ("map_ordinal_" + Twine(module.values.size()) + "_" + Twine(lane))
              .str());
      constantStep(lane, ordinal->packedWidth, ordinalId);
      localIds[arguments[0]] = ordinalId;

      for (auto [k, tableId] : llvm::enumerate(tableOperands)) {
        ConcreteValueId elementId = addSyntheticValue(
            module, ConcreteValueKind::TableElement, elementTypes[k],
            (Twine(module.values[tableId].cppName) + ".element(" + Twine(lane) +
             ")")
                .str(),
            tableId, lane);
        localIds[arguments[1 + k]] = elementId;
      }
      for (auto [c, capture] : llvm::enumerate(map.getCaptures())) {
        ConcreteValueId id = 0;
        if (enclosingIds) {
          auto found = enclosingIds->find(capture);
          if (found == enclosingIds->end())
            return map.emitOpError()
                   << "nested table.map capture is not closed over the "
                      "enclosing lane";
          id = found->second;
        } else {
          auto mapped = valueId(module, capture, map);
          if (failed(mapped))
            return failure();
          id = *mapped;
        }
        localIds[arguments[1 + map.getTables().size() + c]] = id;
      }

      for (Operation &nested : body.without_terminator()) {
        if (auto nestedMap = dyn_cast<ac::TableMapOp>(&nested)) {
          if (failed(unrollTableMap(nestedMap, module, &localIds)))
            return failure();
          continue;
        }
        if (auto nestedMap = dyn_cast<hwir::TableMapOp>(&nested)) {
          if (failed(unrollTableMap(nestedMap, module, &localIds)))
            return failure();
          continue;
        }
        ConcreteScheduleStep step;
        step.kind = ConcreteScheduleStepKind::ComputeValue;
        step.operation = &nested;
        for (Value operand : nested.getOperands()) {
          auto found = localIds.find(operand);
          if (found == localIds.end())
            return nested.emitOpError()
                   << "table.map body operand is not closed over lane values";
          step.inputs.push_back(found->second);
        }
        for (Value result : nested.getResults()) {
          auto type = concreteType(result.getType(), module.bindings, &nested);
          if (failed(type))
            return failure();
          ConcreteValueId id = addSyntheticValue(
              module, ConcreteValueKind::OperationResult, *type,
              ("map_value_" + Twine(lane) + "_" + Twine(module.values.size()))
                  .str());
          step.outputs.push_back(id);
          localIds[result] = id;
        }
        auto compute = computePlan(&nested, module);
        if (failed(compute))
          return failure();
        step.compute = std::move(*compute);
        module.concreteSchedule.steps.push_back(std::move(step));
      }

      for (auto [j, yieldValue] : llvm::enumerate(yield->getOperands())) {
        auto found = localIds.find(yieldValue);
        if (found == localIds.end())
          return map.emitOpError()
                 << "table.map yield is not closed over lane values";
        ConcreteValueId resultElementId = addSyntheticValue(
            module, ConcreteValueKind::TableElement, resultElementTypes[j],
            (Twine(module.values[resultTables[j]].cppName) + ".element(" +
             Twine(lane) + ")")
                .str(),
            resultTables[j], lane);
        resultElements[j].push_back(resultElementId);
        ConcreteScheduleStep step;
        step.kind = ConcreteScheduleStepKind::ComputeValue;
        step.operation = map;
        step.inputs.push_back(found->second);
        step.outputs.push_back(resultElementId);
        step.compute = ConcreteComputePlan{ConcreteComputeKind::PackedIdentity,
                                           "identity",
                                           {},
                                           {},
                                           std::nullopt};
        module.concreteSchedule.steps.push_back(std::move(step));
      }
    }
    // Lane assignments mutate element views. Publish the whole-table SSA
    // value only after every lane of that result has been written.
    for (auto [result, elements] :
         llvm::zip_equal(resultTables, resultElements)) {
      ConcreteScheduleStep barrier;
      barrier.kind = ConcreteScheduleStepKind::AvailabilityBarrier;
      barrier.operation = map;
      llvm::append_range(barrier.inputs, elements);
      barrier.outputs.push_back(result);
      module.concreteSchedule.steps.push_back(std::move(barrier));
    }
    return success();
  }

  template <typename TableMatchT>
  LogicalResult unrollTableMatch(TableMatchT match, ConcreteModulePlan &module) {
    auto shape = analysis.getTableShape(match.getInput().getType(), module.bindings, match);
    if (failed(shape)) return failure();
    uint64_t lanes = shapeProduct(*shape);
    auto maskId = valueId(module, match.getMask(), match);
    if (failed(maskId)) return failure();
    auto inputId = valueId(module, match.getInput(), match);
    if (failed(inputId)) return failure();
    Block &body = match.getBody().front();
    if (body.empty() || !isa<hwir::YieldOp>(body.back()))
      return match.emitOpError() << "table.match body has no hwir.yield";
    auto yield = cast<hwir::YieldOp>(body.back());
    llvm::SmallVector<ConcreteValueId> predicates;
    predicates.reserve(lanes);
    for (uint64_t lane = 0; lane < lanes; ++lane) {
      llvm::DenseMap<Value, ConcreteValueId> local;
      auto element = concreteType(body.getArgument(0).getType(), module.bindings, match);
      if (failed(element)) return failure();
      local[body.getArgument(0)] = addSyntheticValue(
          module, ConcreteValueKind::TableElement, *element,
          (Twine(module.values[*inputId].cppName) + ".element(" + Twine(lane) +
           ")")
              .str(),
          *inputId, lane);
      for (auto [i, capture] : llvm::enumerate(match.getCaptures())) {
        auto id = valueId(module, capture, match); if (failed(id)) return failure();
        local[body.getArgument(i + 1)] = *id;
      }
      for (Operation &nested : body.without_terminator()) {
        ConcreteScheduleStep step;
        step.kind = ConcreteScheduleStepKind::ComputeValue;
        step.operation = &nested;
        for (Value operand : nested.getOperands()) {
          auto found = local.find(operand);
          if (found == local.end()) return nested.emitOpError() << "table.match body operand is not closed";
          step.inputs.push_back(found->second);
        }
        for (Value result : nested.getResults()) {
          auto type = concreteType(result.getType(), module.bindings, &nested);
          if (failed(type)) return failure();
          auto id = addSyntheticValue(module, ConcreteValueKind::OperationResult, *type,
                                      (Twine("match_value_") + Twine(lane) + "_" + Twine(module.values.size())).str());
          step.outputs.push_back(id); local[result] = id;
        }
        auto compute = computePlan(&nested, module); if (failed(compute)) return failure();
        step.compute = std::move(*compute); module.concreteSchedule.steps.push_back(std::move(step));
      }
      auto found = local.find(yield.getValues().front());
      if (found == local.end()) return match.emitOpError() << "table.match yield is not closed";
      predicates.push_back(found->second);
    }
    ConcreteScheduleStep aggregate;
    aggregate.kind = ConcreteScheduleStepKind::ComputeValue;
    aggregate.operation = match;
    aggregate.inputs.append(predicates.begin(), predicates.end());
    aggregate.outputs.push_back(*maskId);
    aggregate.compute = ConcreteComputePlan{ConcreteComputeKind::TableMatch, "match", {}, {}, std::nullopt};
    module.concreteSchedule.steps.push_back(std::move(aggregate));
    return success();
  }

  LogicalResult materializeSchedule(unsigned index) {
    auto &module = plan.modules[index];
    llvm::DenseSet<Operation *> fieldReads;
    llvm::DenseSet<Operation *> ruleInternals;
    llvm::DenseMap<Operation *, llvm::SmallVector<Operation *, 2>> ruleSteps;
    for (Operation &operation : moduleBody(module.key.definition).front())
      if (isa<ac::RuleOp, hwir::RuleOp>(operation))
        operation.walk([&](Operation *nested) {
          if (nested != &operation &&
              !isa<ac::YieldOp, hwir::YieldOp>(nested)) {
            ruleInternals.insert(nested);
            ruleSteps[&operation].push_back(nested);
          }
        });
    // ac.table.map is IsolatedFromAbove: its body is unrolled separately by
    // unrollTableMap, so never emit its body ops as flat compute steps. The
    // walk is post-order and has already descended into those regions, so drop
    // any operation enclosed by a table.map here.
    for (auto &entry : ruleSteps)
      llvm::erase_if(entry.second, [](Operation *op) {
        for (Operation *parent = op->getParentOp(); parent;
             parent = parent->getParentOp())
          if (isa<ac::TableMapOp, hwir::TableMapOp, hwir::TableMatchOp>(parent))
            return true;
        return false;
      });
    // A Rule is one analyzed schedule node, but its region operations still
    // need a concrete renderer order. Region order is not dependency order
    // after source lowering (an instance may precede the SSA that drives it),
    // so materialize a stable local topological order here. Outputs of storage
    // work are temporal cuts and may be read before that primitive samples its
    // next-state inputs later in the same Work.
    llvm::DenseSet<Operation *> storageWork;
    storageWork.insert(module.schedule.storageWork.begin(),
                       module.schedule.storageWork.end());
    for (auto &[rule, steps] : ruleSteps) {
      llvm::DenseSet<Operation *> remaining(steps.begin(), steps.end());
      llvm::DenseSet<Operation *> emitted;
      llvm::SmallVector<Operation *, 2> ordered;
      ordered.reserve(steps.size());
      while (!remaining.empty()) {
        Operation *next = nullptr;
        for (Operation *candidate : steps) {
          if (!remaining.contains(candidate))
            continue;
          bool ready = true;
          for (Value operand : candidate->getOperands()) {
            Operation *producer = operand.getDefiningOp();
            if (producer && remaining.contains(producer) &&
                !storageWork.contains(producer) &&
                !emitted.contains(producer)) {
              ready = false;
              break;
            }
          }
          if (ready) {
            next = candidate;
            break;
          }
        }
        if (!next)
          return rule->emitOpError()
                 << "structural C++ rule body has an unresolved "
                    "combinational dependency cycle";
        remaining.erase(next);
        emitted.insert(next);
        ordered.push_back(next);
      }
      steps = std::move(ordered);
    }
    llvm::SmallVector<Operation *> emissionSchedule;
    llvm::DenseSet<Operation *> scheduledRules;
    for (Operation *operation : module.schedule.schedule) {
      if (isa<ac::SourceExpectOp, ac::SourceObserveOp>(operation))
        continue;
      if (isa<ac::RuleOp, hwir::RuleOp>(operation)) {
        scheduledRules.insert(operation);
        if (auto steps = ruleSteps.find(operation); steps != ruleSteps.end())
          llvm::append_range(emissionSchedule, steps->second);
        continue;
      }
      if (!ruleInternals.contains(operation))
        emissionSchedule.push_back(operation);
    }
    // Observation/check payload rules may be excluded from the hardware work
    // schedule because their consumers are retained effects. They still own
    // ordinary value computations required before staging/precheck.
    for (Operation &operation : moduleBody(module.key.definition).front())
      if (isa<ac::RuleOp, hwir::RuleOp>(operation) &&
          !scheduledRules.contains(&operation))
        if (auto steps = ruleSteps.find(&operation); steps != ruleSteps.end())
          llvm::append_range(emissionSchedule, steps->second);

    for (Operation *operation : emissionSchedule) {
      llvm::SmallVector<unsigned> matchedChildren;
      for (auto [childIndex, child] : llvm::enumerate(module.children))
        if (child.occurrence == operation)
          matchedChildren.push_back(static_cast<unsigned>(childIndex));
      if (!matchedChildren.empty()) {
        for (unsigned childIndex : matchedChildren) {
          const ConcreteChildPlan &child = module.children[childIndex];
          const bool ordinary = child.kind == ConcreteChildKind::Module;
          for (const ConcretePortBinding &input : child.inputs) {
            ConcreteScheduleStep step;
            step.kind = ordinary
                            ? ConcreteScheduleStepKind::DriveChildInput
                            : ConcreteScheduleStepKind::DrivePrimitiveInput;
            step.operation = operation;
            step.childIndex = childIndex;
            step.portIndex = input.port;
            step.inputs.push_back(input.value);
            module.concreteSchedule.steps.push_back(std::move(step));
          }
          ConcreteScheduleStep invoke;
          invoke.kind = ordinary
                            ? ConcreteScheduleStepKind::InvokeChildWork
                            : ConcreteScheduleStepKind::InvokePrimitiveWork;
          invoke.operation = operation;
          invoke.childIndex = childIndex;
          module.concreteSchedule.steps.push_back(std::move(invoke));
          for (const ConcretePortBinding &output : child.outputs) {
            // Aggregate child outputs can have independent leaf dependencies.
            // Project a leaf directly from the child output when every use of
            // the result is a StructGet chain. This is important for graphs
            // such as `a = Child(b.pair.left)`: reading the complete aggregate
            // would require unrelated fields and can create a false cycle.
            bool projected = false;
            if (ordinary && child.moduleIndex && !child.laneIndex) {
              Value result = operation->getResult(output.port);
              auto childOutput = plan.modules[*child.moduleIndex]
                                     .outputs[output.port]
                                     .type;
              auto dependencies = plan.modules[*child.moduleIndex]
                                      .key.definition
                                      ->getAttrOfType<ArrayAttr>(
                                          "hwir.output_dependencies");
              if (!dependencies)
                return operation->emitOpError()
                       << "HWIR child lacks output dependency facts";
              std::function<LogicalResult(Value, ArrayRef<StringRef>)> visit =
                  [&](Value value, ArrayRef<StringRef> prefix) {
                    bool sawUse = false;
                    for (OpOperand &use : value.getUses()) {
                      auto get = dyn_cast<ac::StructGetOp>(use.getOwner());
                      if (!get || get.getValue() != value)
                        return failure();
                      sawUse = true;
                      SmallVector<StringRef, 4> path(prefix.begin(),
                                                     prefix.end());
                      path.push_back(get.getFieldAttr());
                      bool nested = false;
                      for (OpOperand &nestedUse : get.getResult().getUses())
                        if (isa<ac::StructGetOp>(nestedUse.getOwner())) {
                          nested = true;
                          break;
                        }
                      if (nested) {
                        if (failed(visit(get.getResult(), path)))
                          return failure();
                        fieldReads.insert(get);
                        continue;
                      }
                      const ConcreteTypePlan::PackedField *field = nullptr;
                      for (const auto &candidate : childOutput.fields) {
                        if (candidate.path.size() != path.size())
                          continue;
                        bool matches = true;
                        for (auto [i, component] : llvm::enumerate(path))
                          matches &= candidate.path[i] == component.str();
                        if (matches) {
                          field = &candidate;
                          break;
                        }
                      }
                      if (!field) {
                        get.emitOpError()
                            << "HWIR child output field lacks packed layout";
                        return failure();
                      }
                      SmallVector<unsigned, 2> refreshInputs;
                      bool covered = false;
                      for (Attribute raw : dependencies) {
                        auto row = dyn_cast<DictionaryAttr>(raw);
                        auto endpoint = row
                                            ? row.getAs<DictionaryAttr>("output")
                                            : DictionaryAttr();
                        auto port = endpoint
                                        ? endpoint.getAs<IntegerAttr>("port")
                                        : IntegerAttr();
                        auto outputPath = endpoint
                                              ? endpoint.getAs<ArrayAttr>("path")
                                              : ArrayAttr();
                        auto inputs = row ? row.getAs<ArrayAttr>("inputs")
                                          : ArrayAttr();
                        if (!port || !outputPath || !inputs ||
                            port.getValue().getZExtValue() != output.port ||
                            outputPath.size() != path.size())
                          continue;
                        bool matches = true;
                        for (auto [i, component] : llvm::enumerate(path)) {
                          auto name = dyn_cast<StringAttr>(outputPath[i]);
                          matches &= name && name.getValue() == component;
                        }
                        if (!matches)
                          continue;
                        covered = true;
                        for (Attribute inputRaw : inputs) {
                          auto input = dyn_cast<DictionaryAttr>(inputRaw);
                          auto inputPort = input
                                                ? input.getAs<IntegerAttr>("port")
                                                : IntegerAttr();
                          if (!inputPort ||
                              inputPort.getValue().getActiveBits() > 32) {
                            operation->emitOpError()
                                << "HWIR child has malformed dependency facts";
                            return failure();
                          }
                          unsigned ordinal = inputPort.getValue().getZExtValue();
                          if (!llvm::is_contained(refreshInputs, ordinal))
                            refreshInputs.push_back(ordinal);
                        }
                      }
                      if (!covered) {
                        get.emitOpError()
                            << "HWIR child output field lacks dependency facts";
                        return failure();
                      }
                      auto outputId = valueId(module, get.getResult(), get);
                      if (failed(outputId))
                        return failure();
                      ConcreteScheduleStep fieldStep;
                      fieldStep.kind =
                          ConcreteScheduleStepKind::ReadChildOutputField;
                      fieldStep.operation = get;
                      fieldStep.childIndex = childIndex;
                      fieldStep.portIndex = output.port;
                      fieldStep.fieldLow = field->low;
                      fieldStep.refreshInputPorts = std::move(refreshInputs);
                      fieldStep.outputs.push_back(*outputId);
                      module.concreteSchedule.steps.push_back(
                          std::move(fieldStep));
                      fieldReads.insert(get);
                    }
                    return success();
                  };
              for (OpOperand &use : result.getUses()) {
                auto get = dyn_cast<ac::StructGetOp>(use.getOwner());
                if (!get || get.getValue() != result)
                  continue;
                SmallVector<StringRef, 4> empty;
                if (succeeded(visit(result, empty))) {
                  projected = true;
                  break;
                }
              }
            }
            if (projected)
              continue;
            ConcreteScheduleStep step;
            step.kind = ordinary
                            ? ConcreteScheduleStepKind::ReadChildOutput
                            : ConcreteScheduleStepKind::ReadPrimitiveOutput;
            step.operation = operation;
            step.childIndex = childIndex;
            step.portIndex = output.port;
            step.outputs.push_back(output.value);
            module.concreteSchedule.steps.push_back(std::move(step));
          }
        }
        if (isa<ac::CollectionOp, hwir::CollectionOp>(operation)) {
          const auto &first = module.children[matchedChildren.front()];
          for (unsigned port = 0; port < first.outputs.size(); ++port) {
            ConcreteValueId firstLane = first.outputs[port].value;
            if (!module.values[firstLane].availabilityParent)
              return operation->emitOpError()
                     << "structural C++ collection output has no parent "
                        "table";
            ConcreteScheduleStep barrier;
            barrier.kind = ConcreteScheduleStepKind::AvailabilityBarrier;
            barrier.operation = operation;
            for (unsigned childIndex : matchedChildren) {
              const auto &binding = module.children[childIndex].outputs[port];
              if (module.values[binding.value].availabilityParent !=
                  module.values[firstLane].availabilityParent)
                return operation->emitOpError()
                       << "structural C++ collection lane outputs disagree "
                          "on their parent table";
              barrier.inputs.push_back(binding.value);
            }
            barrier.outputs.push_back(
                *module.values[firstLane].availabilityParent);
            module.concreteSchedule.steps.push_back(std::move(barrier));
          }
        }
        continue;
      }
      if (fieldReads.contains(operation))
        continue;
      if (auto map = dyn_cast<ac::TableMapOp>(operation)) {
        if (failed(unrollTableMap(map, module)))
          return failure();
        continue;
      }
      if (auto map = dyn_cast<hwir::TableMapOp>(operation)) {
        if (failed(unrollTableMap(map, module)))
          return failure();
        continue;
      }
      if (auto match = dyn_cast<hwir::TableMatchOp>(operation)) {
        if (failed(unrollTableMatch(match, module)))
          return failure();
        continue;
      }
      ConcreteScheduleStep step;
      step.kind = ConcreteScheduleStepKind::ComputeValue;
      step.operation = operation;
      bool forwardedStructField = false;
      if (auto get = dyn_cast<ac::StructGetOp>(operation)) {
        if (auto create = get.getValue().getDefiningOp<ac::StructCreateOp>()) {
          auto declaration = analysis.lookupStruct(create.getResult().getType());
          if (!declaration)
            return get.emitOpError()
                   << "cannot resolve struct.create field forwarding";
          for (auto [fieldIndex, raw] :
               llvm::enumerate(declaration.getFields())) {
            auto field = cast<DictionaryAttr>(raw);
            if (field.getAs<StringAttr>("name") != get.getFieldAttr())
              continue;
            auto id = valueId(module, create.getValues()[fieldIndex], get);
            if (failed(id))
              return failure();
            step.inputs.push_back(*id);
            step.compute = ConcreteComputePlan{
                ConcreteComputeKind::PackedIdentity, "identity", {}, {},
                std::nullopt};
            forwardedStructField = true;
            break;
          }
          if (!forwardedStructField)
            return get.emitOpError()
                   << "cannot locate struct.create forwarding field";
        }
      }
      if (!forwardedStructField)
        for (Value operand : operation->getOperands()) {
          auto id = valueId(module, operand, operation);
          if (failed(id))
            return failure();
          step.inputs.push_back(*id);
        }
      for (Value result : operation->getResults()) {
        auto id = valueId(module, result, operation);
        if (failed(id))
          return failure();
        step.outputs.push_back(*id);
      }
      if (!forwardedStructField) {
        auto compute = computePlan(operation, module);
        if (failed(compute))
          return failure();
        step.compute = std::move(*compute);
      }
      module.concreteSchedule.steps.push_back(std::move(step));
    }
    // Rule results and the values yielded by their regions can retain
    // distinct concrete IDs. Make every such forwarding edge explicit so a
    // child input that consumes the rule result becomes available as soon as
    // the yielded computation (or captured input) is available.
    for (Operation &operation : moduleBody(module.key.definition).front()) {
      if (!isa<ac::RuleOp, hwir::RuleOp>(operation) ||
          operation.getRegion(0).empty())
        continue;
      Operation *yield = &operation.getRegion(0).front().back();
      if (!isa<ac::YieldOp, hwir::YieldOp>(yield))
        continue;
      for (auto [result, yielded] :
           llvm::zip_equal(operation.getResults(), yield->getOperands())) {
        auto input = valueId(module, yielded, yield);
        auto output = valueId(module, result, &operation);
        if (failed(input) || failed(output))
          return failure();
        if (*input == *output)
          continue;
        ConcreteScheduleStep step;
        step.kind = ConcreteScheduleStepKind::ComputeValue;
        step.operation = &operation;
        step.inputs.push_back(*input);
        step.outputs.push_back(*output);
        step.compute = ConcreteComputePlan{ConcreteComputeKind::PackedIdentity,
                                           "identity",
                                           {},
                                           {},
                                           std::nullopt};
        module.concreteSchedule.steps.push_back(std::move(step));
      }
    }
    // Expand the coarse analysis schedule into its value-level executable
    // order. Rule results have already been flattened to their yielded values,
    // which permits independent input computations to precede a child Work
    // even when another result of the same Rule reads that child's output.
    auto pending = std::move(module.concreteSchedule.steps);
    llvm::SmallVector<ConcreteScheduleStep, 0> ordered;
    llvm::DenseSet<ConcreteValueId> available;
    auto markAvailable = [&](ConcreteValueId root) {
      llvm::SmallVector<ConcreteValueId, 4> worklist{root};
      while (!worklist.empty()) {
        ConcreteValueId value = worklist.pop_back_val();
        if (!available.insert(value).second)
          continue;
        for (const ConcreteValuePlan &candidate : module.values)
          if (candidate.availabilityParent == value)
            worklist.push_back(candidate.id);
      }
    };
    for (const ConcreteValuePlan &value : module.values)
      if (value.kind == ConcreteValueKind::ModuleInput)
        markAvailable(value.id);
    llvm::SmallVector<llvm::SmallVector<bool, 8>, 0> drivenPorts;
    drivenPorts.reserve(module.children.size());
    for (const ConcreteChildPlan &child : module.children)
      drivenPorts.emplace_back(child.inputs.size(), false);
    llvm::SmallVector<unsigned, 0> driven(module.children.size());
    llvm::SmallVector<unsigned, 0> driveCounts(module.children.size());
    llvm::SmallVector<bool, 0> invoked(module.children.size(), false);
    for (const ConcreteScheduleStep &step : pending)
      if ((step.kind == ConcreteScheduleStepKind::DriveChildInput ||
           step.kind == ConcreteScheduleStepKind::DrivePrimitiveInput) &&
          step.childIndex)
        ++driveCounts[*step.childIndex];
    llvm::SmallVector<bool, 0> done(pending.size(), false);
    while (ordered.size() != pending.size()) {
      std::optional<unsigned> selected;
      for (auto [stepIndex, step] : llvm::enumerate(pending)) {
        if (done[stepIndex])
          continue;
        bool ready = llvm::all_of(step.inputs, [&](ConcreteValueId input) {
          return available.contains(input);
        });
        if (ready &&
            (step.kind == ConcreteScheduleStepKind::InvokeChildWork ||
             step.kind == ConcreteScheduleStepKind::InvokePrimitiveWork))
          ready = step.childIndex &&
                  driven[*step.childIndex] == driveCounts[*step.childIndex];
        if (ready && step.kind ==
                         ConcreteScheduleStepKind::ReadChildOutputField) {
          ready = step.childIndex &&
                  llvm::all_of(step.refreshInputPorts, [&](unsigned input) {
                    return input < drivenPorts[*step.childIndex].size() &&
                           drivenPorts[*step.childIndex][input];
                  });
        }
        if (ready &&
            (step.kind == ConcreteScheduleStepKind::ReadChildOutput ||
             step.kind == ConcreteScheduleStepKind::ReadPrimitiveOutput)) {
          bool temporalRead = false;
          if (step.childIndex && step.portIndex) {
            const ConcreteChildPlan &child = module.children[*step.childIndex];
            if (child.kind == ConcreteChildKind::Module && child.moduleIndex) {
              const auto &temporal =
                  plan.modules[*child.moduleIndex].outputIsTemporal;
              temporalRead = *step.portIndex < temporal.size() &&
                             temporal[*step.portIndex];
            } else if (child.primitive) {
              const auto &temporal = child.primitive->outputIsTemporal;
              temporalRead = *step.portIndex < temporal.size() &&
                             temporal[*step.portIndex];
            }
          }
          if (temporalRead) {
            ready = true;
          } else if (step.kind ==
                         ConcreteScheduleStepKind::ReadPrimitiveOutput &&
                     step.childIndex && step.portIndex) {
            const ConcreteChildPlan &child =
                module.children[*step.childIndex];
            ready = child.primitive &&
                    *step.portIndex <
                        child.primitive->outputRefreshInputs.size() &&
                    llvm::all_of(
                        child.primitive
                            ->outputRefreshInputs[*step.portIndex],
                        [&](unsigned input) {
                          return input < drivenPorts[*step.childIndex].size() &&
                                 drivenPorts[*step.childIndex][input];
                        });
          } else {
            ready = step.childIndex && invoked[*step.childIndex];
          }
        }
        if (ready) {
          selected = stepIndex;
          break;
        }
      }
      if (!selected) {
        auto diagnostic = module.key.definition->emitOpError()
                          << "structural C++ concrete steps have an unresolved "
                             "value or child lifecycle dependency";
        unsigned reported = 0;
        for (auto [stepIndex, step] : llvm::enumerate(pending)) {
          if (done[stepIndex] || reported == 16)
            continue;
          ++reported;
          diagnostic << "; pending step " << static_cast<unsigned>(step.kind);
          if (step.childIndex)
            diagnostic << " for child " << *step.childIndex << " (driven "
                       << driven[*step.childIndex] << "/"
                       << driveCounts[*step.childIndex] << ", invoked "
                       << invoked[*step.childIndex] << ")";
          for (ConcreteValueId input : step.inputs)
            if (!available.contains(input)) {
              diagnostic << " missing " << module.values[input].cppName << "["
                         << input << "]";
              auto moduleValues = valueIds.find(canonicalModuleKey(module.key));
              if (moduleValues != valueIds.end())
                for (const auto &[value, id] : moduleValues->second) {
                  if (id != input)
                    continue;
                  if (auto result = dyn_cast<OpResult>(value)) {
                    diagnostic << " produced by "
                               << result.getOwner()->getName() << " result "
                               << result.getResultNumber();
                    if (auto rule = dyn_cast<ac::RuleOp>(result.getOwner())) {
                      if (auto yield = dyn_cast<ac::YieldOp>(
                              rule.getBody().front().back())) {
                        Value yielded =
                            yield.getValues()[result.getResultNumber()];
                        diagnostic << " yielding ";
                        if (auto yieldedArgument =
                                dyn_cast<BlockArgument>(yielded))
                          diagnostic << "block argument "
                                     << yieldedArgument.getArgNumber();
                        else if (auto yieldedResult =
                                     dyn_cast<OpResult>(yielded))
                          diagnostic << yieldedResult.getOwner()->getName()
                                     << " result "
                                     << yieldedResult.getResultNumber();
                        auto yieldedId = valueId(module, yielded, yield);
                        if (succeeded(yieldedId))
                          diagnostic << " as value " << *yieldedId;
                      }
                    }
                  } else if (auto argument = dyn_cast<BlockArgument>(value))
                    diagnostic << " from block argument "
                               << argument.getArgNumber();
                  break;
                }
            }
          if (step.childIndex) {
            const ConcreteChildPlan &child = module.children[*step.childIndex];
            diagnostic << " temporal outputs [";
            ArrayRef<uint8_t> temporal;
            if (child.moduleIndex)
              temporal = plan.modules[*child.moduleIndex].outputIsTemporal;
            else if (child.primitive)
              temporal = child.primitive->outputIsTemporal;
            for (auto [index, flag] : llvm::enumerate(temporal))
              diagnostic << (index ? "," : "") << static_cast<unsigned>(flag);
            diagnostic << "]";
          }
        }
        if (pending.size() - ordered.size() > reported)
          diagnostic << "; ... and "
                     << (pending.size() - ordered.size() - reported)
                     << " more pending steps";
        return failure();
      }
      done[*selected] = true;
      ConcreteScheduleStep step = std::move(pending[*selected]);
      if ((step.kind == ConcreteScheduleStepKind::DriveChildInput ||
           step.kind == ConcreteScheduleStepKind::DrivePrimitiveInput) &&
          step.childIndex) {
        ++driven[*step.childIndex];
        if (step.portIndex &&
            *step.portIndex < drivenPorts[*step.childIndex].size())
          drivenPorts[*step.childIndex][*step.portIndex] = true;
      }
      if ((step.kind == ConcreteScheduleStepKind::InvokeChildWork ||
           step.kind == ConcreteScheduleStepKind::InvokePrimitiveWork) &&
          step.childIndex)
        invoked[*step.childIndex] = true;
      for (ConcreteValueId output : step.outputs)
        markAvailable(output);
      ordered.push_back(std::move(step));
    }
    module.concreteSchedule.steps = std::move(ordered);
    Operation *yield = &moduleBody(module.key.definition).front().back();
    for (auto [port, output] : llvm::enumerate(module.outputs)) {
      ConcreteScheduleStep step;
      step.kind = ConcreteScheduleStepKind::MaterializeOutput;
      step.operation = yield;
      step.portIndex = static_cast<unsigned>(port);
      step.inputs.push_back(output.value);
      module.concreteSchedule.steps.push_back(std::move(step));

      // A packed aggregate output is also published one top-level field at a
      // time. This projection is what RefreshPureOutputs uses when different
      // fields have different input/temporal dependencies.
      Value yielded = moduleBody(module.key.definition)
                          .front()
                          .back()
                          .getOperand(port);
      yielded = flattenRegionValue(yielded);
      auto create = yielded.getDefiningOp<ac::StructCreateOp>();
      auto declaration =
          create ? analysis.lookupStruct(create.getResult().getType())
                 : ac::StructOp();
      if (!create || !declaration)
        continue;
      uint64_t low = 0;
      ArrayAttr fields = declaration.getFields();
      for (size_t reverseIndex = fields.size(); reverseIndex != 0;
           --reverseIndex) {
        size_t fieldIndex = reverseIndex - 1;
        Attribute rawField = fields[fieldIndex];
        auto field = cast<DictionaryAttr>(rawField);
        auto width = analysis.getPackedWidth(
            field.getAs<TypeAttr>("type").getValue(), module.bindings,
            create);
        if (failed(width))
          return failure();
        auto input = valueId(module, create.getValues()[fieldIndex], create);
        if (failed(input))
          return failure();
        ConcreteScheduleStep fieldStep;
        fieldStep.kind = ConcreteScheduleStepKind::MaterializeOutput;
        fieldStep.operation = &moduleBody(module.key.definition).front().back();
        fieldStep.portIndex = static_cast<unsigned>(port);
        fieldStep.fieldLow = low;
        fieldStep.inputs.push_back(*input);
        module.concreteSchedule.steps.push_back(std::move(fieldStep));
        low += *width;
      }
    }
    // Derive the state-only projection from the executable schedule. No
    // module input, drive, or Work step is admitted. The fixed point includes
    // constants and pure computations fed exclusively by committed temporal
    // child outputs, then records which public outputs are safe temporal cuts.
    module.outputIsTemporal.assign(module.outputs.size(), false);
    llvm::DenseSet<ConcreteValueId> temporalValues;
    auto markDerivedAvailable = [&](llvm::DenseSet<ConcreteValueId> &values,
                                    ConcreteValueId root) {
      llvm::SmallVector<ConcreteValueId, 4> worklist{root};
      while (!worklist.empty()) {
        ConcreteValueId value = worklist.pop_back_val();
        if (!values.insert(value).second)
          continue;
        for (const ConcreteValuePlan &candidate : module.values)
          if (candidate.availabilityParent == value)
            worklist.push_back(candidate.id);
      }
    };
    llvm::SmallVector<bool, 0> refreshDone(module.concreteSchedule.steps.size(),
                                           false);
    bool progress = true;
    while (progress) {
      progress = false;
      for (auto [stepIndex, step] :
           llvm::enumerate(module.concreteSchedule.steps)) {
        if (refreshDone[stepIndex])
          continue;
        bool admit = false;
        switch (step.kind) {
        case ConcreteScheduleStepKind::ReadChildOutput:
          if (step.childIndex && step.portIndex) {
            const ConcreteChildPlan &child = module.children[*step.childIndex];
            if (child.moduleIndex) {
              const auto &temporal =
                  plan.modules[*child.moduleIndex].outputIsTemporal;
              admit = *step.portIndex < temporal.size() &&
                      temporal[*step.portIndex];
            }
          }
          break;
        case ConcreteScheduleStepKind::ReadPrimitiveOutput:
          if (step.childIndex && step.portIndex) {
            const ConcreteChildPlan &child = module.children[*step.childIndex];
            if (child.primitive) {
              const auto &temporal = child.primitive->outputIsTemporal;
              admit = *step.portIndex < temporal.size() &&
                      temporal[*step.portIndex];
            }
          }
          break;
        case ConcreteScheduleStepKind::ComputeValue:
        case ConcreteScheduleStepKind::AvailabilityBarrier:
          admit = llvm::all_of(step.inputs, [&](ConcreteValueId input) {
            return temporalValues.contains(input);
          });
          break;
        case ConcreteScheduleStepKind::MaterializeOutput:
          admit = llvm::all_of(step.inputs, [&](ConcreteValueId input) {
            return temporalValues.contains(input);
          });
          break;
        default:
          break;
        }
        if (!admit)
          continue;
        refreshDone[stepIndex] = true;
        progress = true;
        for (ConcreteValueId output : step.outputs)
          markDerivedAvailable(temporalValues, output);
        if (step.kind == ConcreteScheduleStepKind::MaterializeOutput &&
            !step.fieldLow && step.portIndex &&
            *step.portIndex < module.outputIsTemporal.size())
          module.outputIsTemporal[*step.portIndex] = true;
        module.temporalRefresh.steps.push_back(step);
      }
    }
    // Build the non-sampling combinational projection used across ordinary
    // module boundaries. Module inputs are potential values here; a parent
    // invokes this method only after the dependency-summary ports for the
    // requested field have actually been driven.
    llvm::DenseSet<ConcreteValueId> pureValues;
    for (const ConcreteValuePlan &value : module.values)
      if (value.kind == ConcreteValueKind::ModuleInput)
        markDerivedAvailable(pureValues, value.id);
    llvm::SmallVector<bool, 0> pureDone(module.concreteSchedule.steps.size(),
                                        false);
    llvm::SmallVector<llvm::SmallVector<bool, 8>, 0> pureDriven;
    pureDriven.reserve(module.children.size());
    for (const ConcreteChildPlan &child : module.children)
      pureDriven.emplace_back(child.inputs.size(), false);
    progress = true;
    while (progress) {
      progress = false;
      for (auto [stepIndex, step] :
           llvm::enumerate(module.concreteSchedule.steps)) {
        if (pureDone[stepIndex])
          continue;
        bool admit = llvm::all_of(step.inputs, [&](ConcreteValueId input) {
          return pureValues.contains(input);
        });
        if (!admit)
          continue;
        switch (step.kind) {
        case ConcreteScheduleStepKind::ComputeValue:
        case ConcreteScheduleStepKind::AvailabilityBarrier:
        case ConcreteScheduleStepKind::DriveChildInput:
        case ConcreteScheduleStepKind::DrivePrimitiveInput:
        case ConcreteScheduleStepKind::MaterializeOutput:
          break;
        case ConcreteScheduleStepKind::ReadChildOutputField:
          admit = step.childIndex &&
                  llvm::all_of(step.refreshInputPorts, [&](unsigned input) {
                    return input < pureDriven[*step.childIndex].size() &&
                           pureDriven[*step.childIndex][input];
                  });
          break;
        case ConcreteScheduleStepKind::ReadChildOutput:
          if (step.childIndex && step.portIndex) {
            const ConcreteChildPlan &child = module.children[*step.childIndex];
            const auto &temporal =
                plan.modules[*child.moduleIndex].outputIsTemporal;
            admit = *step.portIndex < temporal.size() &&
                    temporal[*step.portIndex];
          } else {
            admit = false;
          }
          break;
        case ConcreteScheduleStepKind::ReadPrimitiveOutput:
          if (step.childIndex && step.portIndex) {
            const ConcreteChildPlan &child = module.children[*step.childIndex];
            const auto &temporal = child.primitive->outputIsTemporal;
            admit = (*step.portIndex < temporal.size() &&
                     temporal[*step.portIndex]) ||
                    (*step.portIndex <
                         child.primitive->outputRefreshInputs.size() &&
                     llvm::all_of(
                         child.primitive
                             ->outputRefreshInputs[*step.portIndex],
                         [&](unsigned input) {
                           return input < pureDriven[*step.childIndex].size() &&
                                  pureDriven[*step.childIndex][input];
                         }));
          } else {
            admit = false;
          }
          break;
        case ConcreteScheduleStepKind::InvokeChildWork:
        case ConcreteScheduleStepKind::InvokePrimitiveWork:
          admit = false;
          break;
        }
        if (!admit)
          continue;
        pureDone[stepIndex] = true;
        progress = true;
        if ((step.kind == ConcreteScheduleStepKind::DriveChildInput ||
             step.kind == ConcreteScheduleStepKind::DrivePrimitiveInput) &&
            step.childIndex && step.portIndex &&
            *step.portIndex < pureDriven[*step.childIndex].size())
          pureDriven[*step.childIndex][*step.portIndex] = true;
        for (ConcreteValueId output : step.outputs)
          markDerivedAvailable(pureValues, output);
        module.pureOutputRefresh.steps.push_back(step);
      }
    }
    llvm::DenseSet<ConcreteValueId> resetValues;
    for (const ConcreteValuePlan &value : module.values)
      if (value.kind == ConcreteValueKind::ModuleInput)
        markDerivedAvailable(resetValues, value.id);
    llvm::SmallVector<bool, 0> resetDone(module.concreteSchedule.steps.size(),
                                         false);
    progress = true;
    while (progress) {
      progress = false;
      for (auto [stepIndex, step] :
           llvm::enumerate(module.concreteSchedule.steps)) {
        if (resetDone[stepIndex])
          continue;
        const bool eligible =
            step.kind == ConcreteScheduleStepKind::ComputeValue ||
            step.kind == ConcreteScheduleStepKind::AvailabilityBarrier ||
            step.kind == ConcreteScheduleStepKind::DriveChildInput ||
            step.kind == ConcreteScheduleStepKind::DrivePrimitiveInput;
        if (!eligible || !llvm::all_of(step.inputs, [&](ConcreteValueId input) {
              return resetValues.contains(input);
            }))
          continue;
        resetDone[stepIndex] = true;
        progress = true;
        for (ConcreteValueId output : step.outputs)
          markDerivedAvailable(resetValues, output);
        module.resetPreparation.steps.push_back(step);
      }
    }
    return success();
  }

  void materializeModuleLifecycle(unsigned index) {
    auto &module = plan.modules[index];
    for (auto [childIndex, child] : llvm::enumerate(module.children)) {
      const bool ordinary = child.kind == ConcreteChildKind::Module;
      module.lifecycle.xfer.push_back(
          {ordinary ? ConcreteLifecycleStepKind::InvokeChildXfer
                    : ConcreteLifecycleStepKind::InvokePrimitiveXfer,
           index, static_cast<unsigned>(childIndex)});
      module.lifecycle.discard.push_back(
          {ordinary ? ConcreteLifecycleStepKind::InvokeChildDiscard
                    : ConcreteLifecycleStepKind::InvokePrimitiveDiscard,
           index, static_cast<unsigned>(childIndex)});
      module.lifecycle.reset.push_back(
          {ordinary ? ConcreteLifecycleStepKind::InvokeChildReset
                    : ConcreteLifecycleStepKind::InvokePrimitiveReset,
           index, static_cast<unsigned>(childIndex)});
    }
  }

  // Materializes the specialization source map after the closure is complete.
  // Entries mirror `plan.modules` in construction order and tie each stable
  // C++ class name back to its HWIR definition and concrete arguments.
  void buildManifest() {
    plan.manifest.reserve(plan.modules.size());
    for (const ConcreteModulePlan &module : plan.modules) {
      ConcreteModuleManifest entry;
      entry.cppName = module.cppName;
      StringRef symbol =
          SymbolTable::getSymbolName(module.key.definition).getValue();
      entry.definition = (llvm::Twine("@") + symbol).str();
      for (const ConcreteIntegerBinding &binding : module.key.integers)
        entry.integerArguments.push_back(
            {binding.name.getValue().str(), binding.canonicalValue});
      for (const ConcreteTypeBinding &binding : module.key.types)
        entry.typeArguments.push_back(
            {binding.name.getValue().str(), binding.canonicalForm});
      plan.manifest.push_back(std::move(entry));
    }
  }

  LogicalResult buildNamedTypes() {
    llvm::StringSet<> seen;
    for (Operation &operation : package.getBody()->getOperations()) {
      Type type;
      ConcreteTypeKind kind;
      StringRef rawName;
      if (auto record = dyn_cast<ac::StructOp>(operation)) {
        rawName = record.getSymName();
        type =
            ac::StructType::get(package.getContext(), record.getSymNameAttr());
        kind = ConcreteTypeKind::Struct;
      } else if (auto enumeration = dyn_cast<ac::EnumOp>(operation)) {
        rawName = enumeration.getSymName();
        type = ac::EnumType::get(package.getContext(),
                                 enumeration.getSymNameAttr());
        kind = ConcreteTypeKind::Enum;
      } else {
        continue;
      }
      if (!seen.insert(rawName).second)
        continue;
      auto width = analysis.getPackedWidth(type, {}, &operation);
      auto cppName = runtimeTypeName(type, {}, &operation);
      if (failed(width) || failed(cppName) || *width == 0)
        return failure();
      plan.namedTypes.push_back({kind, *cppName, *width});
    }
    return success();
  }

  LogicalResult buildRetainedEffects() {
    llvm::DenseMap<Attribute, uint64_t> registrationKeys;
    llvm::DenseMap<Attribute, uint64_t> identityKeys;
    uint64_t nextSiteKey = 0;
    auto intern = [](Attribute value,
                     llvm::DenseMap<Attribute, uint64_t> &map) {
      auto found = map.find(value);
      if (found == map.end())
        found = map.try_emplace(value, map.size()).first;
      return found->second;
    };
    for (auto [moduleIndex, module] : llvm::enumerate(plan.modules)) {
      for (auto expect : moduleBody(module.key.definition)
                             .front()
                             .getOps<ac::SourceExpectOp>()) {
        auto condition = valueId(module, expect.getCondition(), expect);
        auto path = valueId(module, expect.getPath(), expect);
        if (failed(condition) || failed(path))
          return failure();
        mlir::Builder metadataBuilder(expect.getContext());
        Attribute location = expect.getLocation();
        Attribute checkId = expect->getAttr("ac.check_id");
        auto locationJson = emissionMetadataJson(
            location ? location : metadataBuilder.getDictionaryAttr({}), expect);
        auto checkIdJson = emissionMetadataJson(
            checkId ? checkId : metadataBuilder.getDictionaryAttr({}), expect);
        if (failed(locationJson) || failed(checkIdJson))
          return failure();
        auto message = expect->getAttrOfType<StringAttr>("ac.message");
        ConcreteCheckPlan check{
            static_cast<unsigned>(moduleIndex), *condition, *path,
            expect.getKind().str(), {},
            message ? message.getValue().str()
                    : ("source " + expect.getKind() + " check failed").str(),
            std::move(*locationJson), std::move(*checkIdJson)};
        plan.checks.push_back(std::move(check));
      }
      for (auto expect :
           moduleBody(module.key.definition).front().getOps<hwir::ExpectOp>()) {
        auto condition = valueId(module, expect.getCondition(), expect);
        auto path = valueId(module, expect.getPath(), expect);
        if (failed(condition) || failed(path))
          return failure();
        mlir::Builder metadataBuilder(expect.getContext());
        Attribute location = expect.getLocation().value_or(DictionaryAttr());
        Attribute checkId = expect->getAttr("ac.check_id");
        auto locationJson = emissionMetadataJson(
            location ? location : metadataBuilder.getDictionaryAttr({}), expect);
        auto checkIdJson = emissionMetadataJson(
            checkId ? checkId : metadataBuilder.getDictionaryAttr({}), expect);
        if (failed(locationJson) || failed(checkIdJson))
          return failure();
        auto message = expect->getAttrOfType<StringAttr>("ac.message");
        ConcreteCheckPlan check{
            static_cast<unsigned>(moduleIndex), *condition, *path,
            expect.getKind().str(), {},
            message ? message.getValue().str()
                    : ("source " + expect.getKind() + " check failed").str(),
            std::move(*locationJson), std::move(*checkIdJson)};
        for (Value sample : expect.getSampling()) {
          auto id = valueId(module, sample, expect);
          if (failed(id))
            return failure();
          check.sampling.push_back(*id);
        }
        plan.checks.push_back(std::move(check));
      }
      struct ObservationGroup {
        ac::SourceObserveOp observe;
        uint64_t registrationKey = 0;
        uint64_t ordinal = 0;
        uint64_t identityKey = 0;
      };
      llvm::SmallVector<ObservationGroup, 0> groups;
      for (auto observe : moduleBody(module.key.definition)
                              .front()
                              .getOps<ac::SourceObserveOp>()) {
        auto identity =
            observe->getAttrOfType<DictionaryAttr>("ac.observation_id");
        if (failed(ac::detail::verifyObservationID(
                identity, [&] { return observe.emitOpError(); })))
          return failure();
        groups.push_back(
            {observe, intern(identity.get("registration"), registrationKeys),
             static_cast<uint64_t>(
                 identity.getAs<IntegerAttr>("ordinal").getInt()),
             intern(identity, identityKeys)});
      }
      if (groups.empty()) {
        for (auto observe : moduleBody(module.key.definition)
                                .front()
                                .getOps<hwir::ObserveOp>()) {
          auto identity =
              observe->getAttrOfType<DictionaryAttr>("ac.observation_id");
          if (failed(ac::detail::verifyObservationID(
                  identity, [&] { return observe.emitOpError(); })))
            return failure();
          auto path = valueId(module, observe.getPath(), observe);
          auto registrationJson =
              emissionMetadataJson(identity.get("registration"), observe);
          mlir::Builder metadataBuilder(observe.getContext());
          auto siteJson = emissionMetadataJson(
              metadataBuilder.getDictionaryAttr(
                  {metadataBuilder.getNamedAttr("site", identity.get("site")),
                   metadataBuilder.getNamedAttr("invocation",
                                                identity.get("invocation")),
                   metadataBuilder.getNamedAttr("ordinal",
                                                identity.get("ordinal"))}),
              observe);
          auto specJson = emissionMetadataJson(
              observe.getSpecAttr()
                  ? Attribute(observe.getSpecAttr())
                  : Attribute(DictionaryAttr::get(observe.getContext())),
              observe);
          if (failed(path) || failed(registrationJson) || failed(siteJson) ||
              failed(specJson))
            return failure();
          const uint64_t registrationKey =
              intern(identity.get("registration"), registrationKeys);
          auto logicalSuffix = resolveHWIRObservationOwner(
              cast<hwir::ModuleOp>(module.key.definition.operation), identity,
              observe);
          if (failed(logicalSuffix))
            return failure();
          const uint32_t groupSize = static_cast<uint32_t>(
              std::max<std::size_t>(1, observe.getValues().size()));
          const uint64_t siteKey = nextSiteKey++;
          for (uint32_t groupIndex = 0; groupIndex < groupSize; ++groupIndex) {
            const bool carriesValue = groupIndex < observe.getValues().size();
            Value value = carriesValue ? observe.getValues()[groupIndex]
                                       : observe.getPath();
            auto id = valueId(module, value, observe);
            auto width = analysis.getPackedWidth(value.getType(),
                                                 module.bindings, observe);
            if (failed(id) || failed(width))
              return failure();
            ConcreteObservationSitePlan site;
            site.path = *path;
            site.value = *id;
            site.carriesValue = carriesValue;
            site.kind = observe.getKind() == "report"
                            ? ConcreteObservationKind::Gauge
                            : ConcreteObservationKind::Event;
            site.groupIndex = groupIndex;
            site.groupSize = groupSize;
            site.payloadWidth = *width;
            site.registrationKey = registrationKey;
            site.siteKey = siteKey;
            site.kindName = observe.getKind().str();
            site.registrationJson = *registrationJson;
            site.siteJson = *siteJson;
            site.specJson = *specJson;
            if (auto spec = observe.getSpecAttr())
              if (auto name = spec.getAs<StringAttr>("name"))
                site.reportName = name.getValue().str();
            site.logicalOwnerSuffix = *logicalSuffix;
            plan.modules[moduleIndex].observationSites.push_back(
                std::move(site));
          }
        }
      }
      llvm::sort(groups, [](const ObservationGroup &left,
                            const ObservationGroup &right) {
        return std::tie(left.registrationKey, left.ordinal, left.identityKey) <
               std::tie(right.registrationKey, right.ordinal,
                        right.identityKey);
      });
      for (const ObservationGroup &group : groups) {
        ac::SourceObserveOp observe = group.observe;
        auto path = valueId(module, observe.getPath(), observe);
        if (failed(path))
          return failure();
        auto identity =
            observe->getAttrOfType<DictionaryAttr>("ac.observation_id");
        auto registrationJson =
            emissionMetadataJson(identity.get("registration"), observe);
        auto siteJson = observationSiteMetadataJson(observe);
        auto specJson = emissionMetadataJson(observe.getSpec(), observe);
        if (failed(registrationJson) || failed(siteJson) || failed(specJson))
          return failure();
        auto logical = ac::detail::resolveObservationOwner(analysis, observe,
                                                           module.bindings, {});
        if (failed(logical))
          return failure();
        llvm::SmallVector<std::string, 2> logicalSuffix;
        for (ac::InstanceOp instance : logical->path)
          logicalSuffix.push_back(instance.getInstanceName().str());
        const std::size_t rawGroupSize =
            std::max<std::size_t>(1, observe.getValues().size());
        if (rawGroupSize > std::numeric_limits<uint32_t>::max())
          return observe.emitOpError()
                 << "structural C++ observation group exceeds u32";
        const uint32_t groupSize = static_cast<uint32_t>(rawGroupSize);
        const bool gauge = observe.getKind() == "report";
        const uint64_t siteKey = nextSiteKey++;
        for (uint32_t groupIndex = 0; groupIndex < groupSize; ++groupIndex) {
          const bool carriesValue = groupIndex < observe.getValues().size();
          Value value = carriesValue ? observe.getValues()[groupIndex]
                                     : observe.getPath();
          auto id = valueId(module, value, observe);
          auto width = analysis.getPackedWidth(value.getType(), module.bindings,
                                               observe);
          if (failed(id) || failed(width) || *width == 0 ||
              *width > std::numeric_limits<uint32_t>::max())
            return failure();
          if (gauge && *width > 64)
            return observe.emitOpError()
                   << "structural C++ numeric report requires at most 64 bits";
          ConcreteObservationSitePlan site;
          site.path = *path;
          site.value = *id;
          site.carriesValue = carriesValue;
          site.kind = gauge ? ConcreteObservationKind::Gauge
                            : ConcreteObservationKind::Event;
          site.groupIndex = groupIndex;
          site.groupSize = groupSize;
          site.payloadWidth = *width;
          site.registrationKey = group.registrationKey;
          site.siteKey = siteKey;
          site.kindName = observe.getKind().str();
          site.registrationJson = *registrationJson;
          site.siteJson = *siteJson;
          site.specJson = *specJson;
          if (auto name = observe.getSpec().getAs<StringAttr>("name"))
            site.reportName = name.getValue().str();
          site.logicalOwnerSuffix = logicalSuffix;
          plan.modules[moduleIndex].observationSites.push_back(std::move(site));
        }
      }
    }
    llvm::SmallVector<std::optional<uint64_t>, 0> widths(plan.modules.size());
    std::function<FailureOr<uint64_t>(unsigned)> subtreeWidth =
        [&](unsigned moduleIndex) -> FailureOr<uint64_t> {
      if (moduleIndex >= plan.modules.size())
        return failure();
      if (widths[moduleIndex])
        return *widths[moduleIndex];
      uint64_t width = plan.modules[moduleIndex].observationSites.size();
      for (const ConcreteChildPlan &child :
           plan.modules[moduleIndex].children) {
        if (child.kind != ConcreteChildKind::Module)
          continue;
        if (!child.moduleIndex)
          return failure();
        auto nested = subtreeWidth(*child.moduleIndex);
        if (failed(nested) ||
            *nested > std::numeric_limits<uint64_t>::max() - width)
          return failure();
        width += *nested;
      }
      widths[moduleIndex] = width;
      plan.modules[moduleIndex].observationSubtreeWidth = width;
      return width;
    };
    if (failed(subtreeWidth(plan.rootModule)))
      return failure();

    uint64_t ownerKey = 0;
    std::function<LogicalResult(unsigned, StringRef)> layout =
        [&](unsigned moduleIndex, StringRef instancePath) -> LogicalResult {
      if (moduleIndex >= plan.modules.size())
        return failure();
      const ConcreteModulePlan &module = plan.modules[moduleIndex];
      for (auto [localIndex, site] : llvm::enumerate(module.observationSites)) {
        if (plan.observations.size() >= std::numeric_limits<uint32_t>::max())
          return module.key.definition->emitOpError()
                 << "structural C++ observation ordinal exceeds u32";
        std::string logicalPath = instancePath.str();
        for (StringRef name : site.logicalOwnerSuffix)
          logicalPath += "/" + name.str();
        ConcreteObservationPlan occurrence;
        occurrence.stableOrdinal = plan.observations.size();
        occurrence.ownerKey = ownerKey;
        occurrence.registrationKey = site.registrationKey;
        occurrence.siteKey = site.siteKey;
        occurrence.kind = site.kind;
        occurrence.groupIndex = site.groupIndex;
        occurrence.groupSize = site.groupSize;
        occurrence.payloadWidth = site.payloadWidth;
        occurrence.moduleIndex = moduleIndex;
        occurrence.localSiteIndex = localIndex;
        occurrence.physicalInstancePath = instancePath.str();
        occurrence.instancePath = std::move(logicalPath);
        occurrence.kindName = site.kindName;
        occurrence.registrationJson = site.registrationJson;
        occurrence.siteJson = site.siteJson;
        occurrence.specJson = site.specJson;
        occurrence.carriesValue = site.carriesValue;
        occurrence.reportName = site.reportName;
        if (site.kind == ConcreteObservationKind::Event)
          ++plan.observationEventCapacity;
        plan.observations.push_back(std::move(occurrence));
      }
      ++ownerKey;
      for (const ConcreteChildPlan &child : module.children) {
        if (child.kind != ConcreteChildKind::Module)
          continue;
        if (!child.moduleIndex)
          return failure();
        std::string childPath = instancePath.str();
        childPath += "/" + child.instanceName.getValue().str();
        if (failed(layout(*child.moduleIndex, childPath)))
          return failure();
      }
      return success();
    };
    return layout(plan.rootModule, "root");
  }

  LogicalResult requireConcreteStatic(ac::StaticExprAttr expression,
                                      const ac::HardwareBindings &bindings,
                                      Operation *site, bool positive) {
    auto value = analysis.evaluateStatic(expression, bindings, site);
    if (failed(value))
      return failure();
    auto integer = dyn_cast<ac::MathIntAttr>(*value);
    if (!integer)
      return site->emitOpError()
             << "structural C++ requires a concrete integer expression";
    // MathIntAttr is a canonical mathematical spelling. Constructing APSInt
    // directly from a positive spelling such as "1" may choose a one-bit
    // signed representation and therefore misclassify it as negative.
    StringRef spelling = integer.getCanonicalValue();
    if (positive && (spelling == "0" || spelling.starts_with("-")))
      return site->emitOpError()
             << "structural C++ requires a positive concrete dimension";
    return success();
  }

  FailureOr<ConcreteTypePlan> concreteType(Type type,
                                           const ac::HardwareBindings &bindings,
                                           Operation *site) {
    auto resolved = analysis.resolveType(type, bindings, site);
    if (failed(resolved))
      return failure();
    type = *resolved;
    if (isa<ac::TypeParamType>(type))
      return site->emitOpError()
             << "structural C++ requires every type parameter to be bound";
    if (auto bits = dyn_cast<ac::BitsType>(type)) {
      if (failed(requireConcreteStatic(bits.getWidth(), bindings, site, true)))
        return failure();
    } else if (auto table = dyn_cast<ac::TableType>(type)) {
      for (Attribute raw : table.getShape())
        if (failed(requireConcreteStatic(cast<ac::StaticExprAttr>(raw),
                                         bindings, site, true)))
          return failure();
      if (failed(concreteType(table.getElementType(), bindings, site)))
        return failure();
    } else if (!isa<ac::StructType, ac::EnumType>(type)) {
      return site->emitOpError()
             << "structural C++ encountered a non-hardware concrete type";
    }
    auto width = analysis.getPackedWidth(type, bindings, site);
    if (failed(width) || *width == 0)
      return failure();
    // gfsim::Bits and hardware_traits expose their packed width as an
    // `unsigned` template argument.  Reject an otherwise finite HWIR type at
    // the concrete backend boundary before source emission can instantiate a
    // truncated or non-representable runtime carrier.
    if (*width > std::numeric_limits<unsigned>::max())
      return site->emitOpError()
             << "hardware payload plane including enclosing collections "
                "exceeds the current Runtime bit-width capacity";
    auto cppType = runtimeTypeName(type, bindings, site);
    if (failed(cppType))
      return failure();
    ConcreteTypePlan result{type, *width};
    result.cppTypeName = *cppType;
    auto leaves = analysis.getLeaves(type, bindings, site);
    if (failed(leaves))
      return failure();
    for (const ac::PackedLeaf &leaf : *leaves) {
      ConcreteTypePlan::PackedField field;
      for (StringAttr component : leaf.path)
        field.path.push_back(component.getValue().str());
      field.low = leaf.low;
      field.width = leaf.width;
      result.fields.push_back(std::move(field));
    }
    if (isa<ac::BitsType>(type))
      result.runtimeKind = ConcreteTypePlan::RuntimeKind::Bits;
    else if (auto table = dyn_cast<ac::TableType>(type)) {
      result.runtimeKind = ConcreteTypePlan::RuntimeKind::Table;
      auto shape = analysis.getTableShape(table, bindings, site);
      if (failed(shape))
        return failure();
      result.shape.assign(shape->begin(), shape->end());
    } else if (isa<ac::StructType>(type))
      result.runtimeKind = ConcreteTypePlan::RuntimeKind::Struct;
    else
      result.runtimeKind = ConcreteTypePlan::RuntimeKind::Enum;
    return result;
  }

  LogicalResult requireConcreteBindings(const ac::HardwareBindings &bindings,
                                        Operation *site) {
    if (!bindings.owner)
      return site->emitOpError()
             << "structural C++ occurrence has no bound definition";
    auto parameters = bindings.owner->getAttrOfType<ArrayAttr>("parameters");
    auto typeParameters =
        bindings.owner->getAttrOfType<ArrayAttr>("type_parameters");
    if (!parameters || !typeParameters)
      return site->emitOpError()
             << "structural C++ bound definition has no parameter schema";
    for (Attribute raw : parameters) {
      auto name = cast<DictionaryAttr>(raw).getAs<StringAttr>("name");
      if (!isa_and_nonnull<ac::MathIntAttr>(
              bindings.integers.lookup(name.getValue())))
        return site->emitOpError()
               << "structural C++ requires concrete integer binding '"
               << name.getValue() << "'";
    }
    for (Attribute raw : typeParameters) {
      auto name = cast<StringAttr>(raw);
      Type type = bindings.types.lookup(name.getValue());
      if (!type)
        return site->emitOpError()
               << "structural C++ requires concrete type binding '"
               << name.getValue() << "'";
      if (failed(concreteType(type, bindings, site)))
        return failure();
    }
    return success();
  }

  LogicalResult bindPorts(ConcreteModulePlan &module, ConcreteChildPlan &child,
                          Operation *occurrence) {
    for (auto [port, value] : llvm::enumerate(occurrence->getOperands())) {
      auto id = valueId(module, value, occurrence);
      if (failed(id))
        return failure();
      child.inputs.push_back(
          {static_cast<unsigned>(port), *id, ("input_" + Twine(port)).str()});
    }
    for (auto [port, value] : llvm::enumerate(occurrence->getResults())) {
      auto id = valueId(module, value, occurrence);
      if (failed(id))
        return failure();
      child.outputs.push_back(
          {static_cast<unsigned>(port), *id, ("output_" + Twine(port)).str()});
    }
    return success();
  }

  FailureOr<ac::ResolvedQueueConfig>
  resolveHWIRQueue(hwir::QueueOp queue, const ac::HardwareBindings &bindings) {
    auto integer = [&](ac::StaticExprAttr expression, StringRef name,
                       bool positive) -> FailureOr<uint64_t> {
      auto value = analysis.evaluateStatic(expression, bindings, queue);
      auto number = succeeded(value) ? dyn_cast<ac::MathIntAttr>(*value)
                                     : ac::MathIntAttr();
      if (!number || number.getCanonicalValue().starts_with("-"))
        return queue.emitOpError()
               << "structural C++ queue " << name
               << " requires a nonnegative concrete integer";
      uint64_t result = 0;
      if (number.getCanonicalValue().getAsInteger(10, result) ||
          (positive && result == 0))
        return queue.emitOpError()
               << "structural C++ queue " << name
               << (positive ? " must be positive and fit u64"
                            : " must fit u64");
      return result;
    };

    auto depth = integer(queue.getDepth(), "depth", true);
    auto availability =
        integer(queue.getAvailabilityLatency(), "availability_latency", true);
    auto headRead =
        integer(queue.getHeadReadLatency(), "head_read_latency", false);
    auto payload =
        analysis.resolveType(queue.getInData().getType(), bindings, queue);
    if (failed(depth) || failed(availability) || failed(headRead) ||
        failed(payload))
      return failure();
    if (*headRead != 0)
      return queue.emitOpError()
             << "structural C++ queue head_read_latency must equal zero";
    ac::ResolvedQueueConfig config{};
    config.payloadType = *payload;
    config.depth = *depth;
    config.availabilityLatency = *availability;
    config.headReadLatency = *headRead;
    config.readyPolicy = queue.getReadyPolicy() == "local_occupancy"
                             ? ac::QueueReadyPolicy::LocalOccupancy
                             : ac::QueueReadyPolicy::DownstreamPop;
    config.emptyFlow = queue.getEmptyFlow();
    config.readDuringWrite = queue.getReadDuringWriteAttr();
    config.resetPolicy = queue.getResetPolicyAttr();
    config.emptyData = queue.getEmptyDataAttr();
    config.pointerWidth =
        std::max(1u, llvm::APInt(64, config.depth - 1).getActiveBits());
    config.countWidth = llvm::APInt(64, config.depth).getActiveBits();

    auto width = analysis.getPackedWidth(*payload, bindings, queue);
    if (failed(width) || *width == 0)
      return failure();
    config.packedWidth = *width;
    config.tokenCardinality = 1;
    Type element = *payload;
    while (auto table = dyn_cast<ac::TableType>(element)) {
      auto shape = analysis.getTableShape(table, bindings, queue);
      if (failed(shape))
        return failure();
      for (uint64_t extent : *shape) {
        if (extent == 0 ||
            config.tokenCardinality >
                uint64_t(std::numeric_limits<int64_t>::max()) / extent)
          return queue.emitOpError()
                 << "structural C++ queue token cardinality is invalid";
        config.tokenCardinality *= extent;
        config.tokenShape.push_back(extent);
      }
      element = table.getElementType();
    }
    config.tokenElementType = element;

    const uint64_t max = std::numeric_limits<uint64_t>::max();
    if (config.depth > max / config.packedWidth)
      return queue.emitOpError()
             << "structural C++ queue storage size overflows u64";
    config.storageBitCount = config.depth * config.packedWidth;
    if (config.availabilityLatency > 1) {
      config.timestampWidth =
          llvm::APInt(64, config.availabilityLatency - 1).getActiveBits();
      if (config.depth > max / config.timestampWidth)
        return queue.emitOpError()
               << "structural C++ queue timing storage overflows u64";
      uint64_t deadlines = config.depth * config.timestampWidth;
      uint64_t metadata = uint64_t(config.timestampWidth) +
                          config.pointerWidth + config.countWidth;
      if (deadlines > max - metadata)
        return queue.emitOpError()
               << "structural C++ queue timing storage overflows u64";
      config.timingStorageBitCount = deadlines + metadata;
    }
    if (config.storageBitCount > max - config.timingStorageBitCount)
      return queue.emitOpError()
             << "structural C++ queue combined storage overflows u64";
    return config;
  }

  FailureOr<llvm::SmallVector<std::string, 2>>
  resolveHWIRObservationOwner(hwir::ModuleOp physical, DictionaryAttr identity,
                              Operation *site) {
    llvm::SmallVector<std::string, 2> names;
    llvm::SmallVector<DictionaryAttr, 2> occurrences;
    hwir::ModuleOp terminal = physical;
    auto invocation = identity.getAs<ArrayAttr>("invocation");
    if (!invocation)
      return site->emitOpError()
             << "HWIR observation identity has no invocation path";
    for (Attribute rawFrame : invocation) {
      auto frame = dyn_cast<DictionaryAttr>(rawFrame);
      auto receiver =
          frame ? frame.getAs<DictionaryAttr>("receiver") : DictionaryAttr();
      auto path =
          receiver ? receiver.getAs<ArrayAttr>("instance_path") : ArrayAttr();
      if (!path || path.size() < occurrences.size())
        return site->emitOpError()
               << "HWIR observation receiver path is malformed";
      for (size_t index = 0; index < occurrences.size(); ++index)
        if (path[index] != occurrences[index])
          return site->emitOpError()
                 << "HWIR observation receiver path does not descend";
      for (size_t index = occurrences.size(); index < path.size(); ++index) {
        auto occurrence = dyn_cast<DictionaryAttr>(path[index]);
        hwir::InstanceOp found;
        for (auto instance :
             terminal.getBody().front().getOps<hwir::InstanceOp>()) {
          if (instance.getOccurrenceAttr() != occurrence)
            continue;
          if (found)
            return site->emitOpError()
                   << "HWIR observation owner occurrence is ambiguous";
          found = instance;
        }
        Operation *definition =
            found ? analysis.lookupDefinition(found.getCallee()) : nullptr;
        auto child = dyn_cast_or_null<hwir::ModuleOp>(definition);
        if (!found || !child || !analysis.getPrimitiveKind(definition).empty())
          return site->emitOpError()
                 << "HWIR observation owner path does not resolve an ordinary "
                    "module instance";
        occurrences.push_back(occurrence);
        names.push_back(found.getInstanceName().str());
        terminal = child;
      }
    }
    return names;
  }

  LogicalResult bindCollectionLanePorts(ConcreteModulePlan &module,
                                        ConcreteChildPlan &child,
                                        Operation *collection, uint64_t lane) {
    for (auto [port, value] : llvm::enumerate(collection->getOperands())) {
      auto tableId = valueId(module, value, collection);
      auto table = dyn_cast<ac::TableType>(value.getType());
      if (failed(tableId) || !table)
        return collection->emitOpError()
               << "structural C++ collection input must be a concrete table";
      auto element =
          concreteType(table.getElementType(), module.bindings, collection);
      if (child.kind == ConcreteChildKind::Module && child.moduleIndex &&
          port < plan.modules[*child.moduleIndex].inputs.size())
        element = plan.modules[*child.moduleIndex].inputs[port].type;
      if (failed(element))
        return failure();
      ConcreteValueId laneValue =
          addSyntheticValue(module, ConcreteValueKind::TableElement, *element,
                            (Twine(module.values[*tableId].cppName) +
                             ".element(" + Twine(lane) + ")")
                                .str(),
                            tableId, lane);
      child.inputs.push_back({static_cast<unsigned>(port), laneValue,
                              ("input_" + Twine(port)).str()});
    }
    for (auto [port, value] : llvm::enumerate(collection->getResults())) {
      auto tableId = valueId(module, value, collection);
      auto table = dyn_cast<ac::TableType>(value.getType());
      if (failed(tableId) || !table)
        return collection->emitOpError()
               << "structural C++ collection output must be a concrete table";
      auto element =
          concreteType(table.getElementType(), module.bindings, collection);
      if (child.kind == ConcreteChildKind::Module && child.moduleIndex &&
          port < plan.modules[*child.moduleIndex].outputs.size())
        element = plan.modules[*child.moduleIndex].outputs[port].type;
      if (failed(element))
        return failure();
      ConcreteValueId laneValue =
          addSyntheticValue(module, ConcreteValueKind::TableElement, *element,
                            (Twine(module.values[*tableId].cppName) +
                             ".element(" + Twine(lane) + ")")
                                .str(),
                            tableId, lane);
      child.outputs.push_back({static_cast<unsigned>(port), laneValue,
                               ("output_" + Twine(port)).str()});
    }
    return success();
  }

  std::optional<unsigned> find(const ConcreteModuleKey &key) {
    auto found = moduleIndices.find(canonicalModuleKey(key));
    if (found != moduleIndices.end())
      return found->second;
    return std::nullopt;
  }

  FailureOr<unsigned> visit(Operation *definition,
                            const ac::HardwareBindings &bindings) {
    auto key = normalizeConcreteModuleKey(definition, bindings, definition);
    if (failed(key))
      return failure();
    if (auto existing = find(*key)) {
      if (active.contains(*existing))
        return definition->emitOpError()
               << "structural C++ does not support recursive module "
                  "specializations";
      return *existing;
    }
    if (failed(requireConcreteBindings(bindings, definition)))
      return failure();

    unsigned index = plan.modules.size();
    auto symbol = SymbolTable::getSymbolName(definition).getValue();
    auto name =
        legalizeIdentifier(symbol, [&] { return definition->emitOpError(); });
    if (failed(name))
      return failure();
    ConcreteModulePlan module;
    module.key = *key;
    module.bindings = bindings;
    module.cppName = *name + specializationName(*key);
    FunctionType boundaryType = moduleType(definition);
    module.publicInputCount = boundaryType.getNumInputs();
    module.publicOutputCount = boundaryType.getNumResults();
    if (auto interface =
            definition->getAttrOfType<DictionaryAttr>("hwir.interface")) {
      auto inputs = interface.getAs<IntegerAttr>("public_inputs");
      auto outputs = interface.getAs<IntegerAttr>("public_outputs");
      auto defaults = interface.getAs<ArrayAttr>("root_input_defaults");
      if (!inputs || !outputs || !defaults ||
          inputs.getValue().getActiveBits() > 64 ||
          outputs.getValue().getActiveBits() > 64)
        return definition->emitOpError()
               << "HWIR interface is not concrete";
      module.publicInputCount = inputs.getValue().getZExtValue();
      module.publicOutputCount = outputs.getValue().getZExtValue();
      for (Attribute raw : defaults) {
        auto value = dyn_cast<IntegerAttr>(raw);
        if (!value || value.getValue().getActiveBits() > 64)
          return definition->emitOpError()
                 << "HWIR root input default is not a bounded integer";
        module.rootInputDefaults.push_back(value.getValue().getZExtValue());
      }
    }
    auto domain = definition->getAttrOfType<DictionaryAttr>(
        package->hasAttr("hwir.contract") ? "hwir.domain_inputs"
                                           : "ac.domain_inputs");
    if (domain && !domain.empty()) {
      auto clock = domain.getAs<IntegerAttr>("clock");
      auto reset = domain.getAs<IntegerAttr>("reset");
      if (!clock || !reset || clock.getValue().getActiveBits() > 64 ||
          reset.getValue().getActiveBits() > 64)
        return definition->emitOpError() << "hardware domain is not concrete";
      module.clockInput = clock.getValue().getZExtValue();
      module.resetInput = reset.getValue().getZExtValue();
    }
    auto rootKind = definition->getAttrOfType<StringAttr>(
        package->hasAttr("hwir.contract") ? "hwir.root_kind"
                                           : "ac.root_kind");
    module.closedSystem = rootKind && rootKind.getValue() == "system";
    if (module.publicInputCount > boundaryType.getNumInputs() ||
        module.publicOutputCount > boundaryType.getNumResults() ||
        module.rootInputDefaults.size() !=
            boundaryType.getNumInputs() - module.publicInputCount ||
        (module.clockInput && *module.clockInput >= boundaryType.getNumInputs()) ||
        (module.resetInput && *module.resetInput >= boundaryType.getNumInputs()) ||
        (module.clockInput && module.resetInput &&
         *module.clockInput == *module.resetInput))
      return definition->emitOpError()
             << "HWIR interface/domain is inconsistent with its signature";
    plan.modules.push_back(std::move(module));
    moduleIndices.try_emplace(canonicalModuleKey(plan.modules.back().key),
                              index);
    active.insert(index);
    llvm::scope_exit leaveActive([&] { active.erase(index); });

    auto signature = moduleType(definition);
    FailureOr<ac::ModuleAnalysis> schedule = failure();
    if (auto acDefinition = dyn_cast<ac::ModuleOp>(definition))
      schedule = analysis.analyzeModule(acDefinition, bindings);
    else {
      ac::ModuleAnalysis direct;
      llvm::SmallVector<std::pair<uint64_t, Operation *>> ranked;
      for (Operation &operation : moduleBody(definition).front()) {
        if (isa<hwir::YieldOp, hwir::ExpectOp, hwir::ObserveOp>(operation))
          continue;
        auto rank = operation.getAttrOfType<IntegerAttr>("hwir.schedule");
        ranked.push_back({rank ? rank.getValue().getZExtValue()
                               : std::numeric_limits<uint64_t>::max(),
                          &operation});
        if (operation.hasAttr("hwir.temporal_cut") ||
            isa<hwir::QueueOp>(operation))
          direct.storageWork.push_back(&operation);
      }
      llvm::stable_sort(ranked, [](const auto &left, const auto &right) {
        return left.first < right.first;
      });
      for (auto [rank, operation] : ranked)
        direct.schedule.push_back(operation);
      schedule = std::move(direct);
    }
    if (failed(schedule))
      return definition->emitOpError()
             << "failed to analyze the concrete module schedule";
    // Do not infer a cross-instance cycle from coarse schedule/source order.
    // Final HWIR may place a child before the register or stateful child whose
    // committed output drives it. Once every child has a concrete primitive or
    // module plan, materializeSchedule classifies those temporal outputs and
    // its value/lifecycle topological sort rejects only unresolved
    // combinational dependencies.
    plan.modules[index].schedule = std::move(*schedule);
    if (failed(materializeValues(index)))
      return definition->emitOpError()
             << "failed to materialize concrete module values";

    LogicalResult approved = success();
    definition->walk([&](Operation *operation) {
      if (failed(approved) || operation == definition)
        return WalkResult::interrupt();
      if (!isApprovedStructuralOp(operation)) {
        approved = operation->emitOpError()
                   << "structural C++ has no approved operation mapping";
        return WalkResult::interrupt();
      }
      return WalkResult::advance();
    });
    if (failed(approved))
      return failure();

    for (Operation &operation : moduleBody(definition).front()) {
      if (auto collection = dyn_cast<hwir::CollectionOp>(operation)) {
        auto shape =
            analysis.resolveShape(collection.getShape(), bindings, collection);
        auto childBindings = analysis.bindCallee(
            analysis.lookupDefinition(collection.getCallee()),
            collection.getParametersAttr(), collection.getTypeArgumentsAttr(),
            bindings, collection);
        if (failed(shape) || failed(childBindings) ||
            failed(requireConcreteBindings(*childBindings, collection)))
          return failure();
        uint64_t lanes = 1;
        for (uint64_t extent : *shape) {
          if (extent == 0 ||
              lanes > std::numeric_limits<uint64_t>::max() / extent)
            return collection.emitOpError()
                   << "structural C++ collection lane count is invalid";
          lanes *= extent;
        }
        FunctionType childType = moduleType(childBindings->owner);
        for (Type type : llvm::concat<const Type>(childType.getInputs(),
                                                  childType.getResults())) {
          auto width =
              analysis.getPackedWidth(type, *childBindings, collection);
          if (failed(width))
            return failure();
          if (*width > std::numeric_limits<unsigned>::max() / lanes)
            return collection.emitOpError()
                   << "hardware payload plane including enclosing collections "
                      "exceeds the current Runtime bit-width capacity";
        }
        StringRef primitive = analysis.getPrimitiveKind(childBindings->owner);
        std::optional<unsigned> childModuleIndex;
        if (primitive.empty()) {
          if (!isa<hwir::ModuleOp>(childBindings->owner))
            return collection.emitOpError()
                   << "structural C++ collection callee is not an HWIR module";
          auto visited = visit(childBindings->owner, *childBindings);
          if (failed(visited))
            return failure();
          childModuleIndex = visited;
        } else if (!isKnownPrimitive(primitive)) {
          return collection.emitOpError()
                 << "structural C++ does not support collection primitive '"
                 << primitive << "'";
        }
        auto baseName = legalizeIdentifier(collection.getInstanceName(), [&] {
          return collection.emitOpError();
        });
        if (failed(baseName))
          return failure();
        for (uint64_t lane = 0; lane < lanes; ++lane) {
          ConcreteChildPlan child;
          child.occurrence = collection;
          child.laneIndex = lane;
          child.instanceName = StringAttr::get(
              package.getContext(),
              (Twine(collection.getInstanceName()) + "[" + Twine(lane) + "]")
                  .str());
          child.cppName = (Twine("pyc_") + *baseName + "_" + Twine(lane)).str();
          child.primitiveKind =
              StringAttr::get(package.getContext(), primitive);
          if (primitive.empty()) {
            child.kind = ConcreteChildKind::Module;
            child.moduleIndex = childModuleIndex;
            child.calleeKey = plan.modules[*childModuleIndex].key;
          }
          if (failed(bindCollectionLanePorts(plan.modules[index], child,
                                             collection, lane)))
            return failure();
          if (primitive.empty()) {
          } else {
            child.kind = ConcreteChildKind::Primitive;
            auto config = primitiveConfig(child.primitiveKind, *childBindings,
                                          collection);
            if (failed(config))
              return failure();
            child.primitive = std::move(*config);
          }
          plan.modules[index].children.push_back(std::move(child));
        }
        continue;
      }
      if (auto collection = dyn_cast<ac::CollectionOp>(operation)) {
        auto shape =
            analysis.resolveShape(collection.getShape(), bindings, collection);
        auto childBindings = analysis.bindInstance(collection, bindings);
        if (failed(shape) || failed(childBindings) ||
            failed(requireConcreteBindings(*childBindings, collection)))
          return failure();
        uint64_t lanes = 1;
        for (uint64_t extent : *shape) {
          if (extent == 0 ||
              lanes > std::numeric_limits<uint64_t>::max() / extent)
            return collection.emitOpError()
                   << "structural C++ collection lane count is invalid";
          lanes *= extent;
        }
        FunctionType childType = moduleType(childBindings->owner);
        for (Type type : llvm::concat<const Type>(childType.getInputs(),
                                                  childType.getResults())) {
          auto width =
              analysis.getPackedWidth(type, *childBindings, collection);
          if (failed(width))
            return failure();
          if (*width > std::numeric_limits<unsigned>::max() / lanes)
            return collection.emitOpError()
                   << "hardware payload plane including enclosing collections "
                      "exceeds the current Runtime bit-width capacity";
        }
        auto childDefinition = dyn_cast<ac::ModuleOp>(childBindings->owner);
        StringRef primitive = analysis.getPrimitiveKind(childBindings->owner);
        std::optional<unsigned> childModuleIndex;
        if (primitive.empty()) {
          if (!childDefinition)
            return collection.emitOpError()
                   << "structural C++ collection callee is not a module";
          auto visited = visit(childDefinition, *childBindings);
          if (failed(visited))
            return failure();
          childModuleIndex = visited;
        } else if (!isKnownPrimitive(primitive)) {
          return collection.emitOpError()
                 << "structural C++ does not support collection primitive '"
                 << primitive << "'";
        }
        auto baseName = legalizeIdentifier(collection.getInstanceName(), [&] {
          return collection.emitOpError();
        });
        if (failed(baseName))
          return failure();
        for (uint64_t lane = 0; lane < lanes; ++lane) {
          ConcreteChildPlan child;
          child.occurrence = collection;
          child.laneIndex = lane;
          child.instanceName = StringAttr::get(
              package.getContext(),
              (Twine(collection.getInstanceName()) + "[" + Twine(lane) + "]")
                  .str());
          child.cppName = (Twine("pyc_") + *baseName + "_" + Twine(lane)).str();
          child.primitiveKind =
              StringAttr::get(package.getContext(), primitive);
          if (failed(bindCollectionLanePorts(plan.modules[index], child,
                                             collection, lane)))
            return failure();
          if (primitive.empty()) {
            child.kind = ConcreteChildKind::Module;
            child.moduleIndex = childModuleIndex;
            child.calleeKey = plan.modules[*childModuleIndex].key;
          } else {
            child.kind = ConcreteChildKind::Primitive;
            auto config = primitiveConfig(child.primitiveKind, *childBindings,
                                          collection);
            if (failed(config))
              return failure();
            child.primitive = std::move(*config);
          }
          plan.modules[index].children.push_back(std::move(child));
        }
        continue;
      }
      if (auto instance = dyn_cast<hwir::InstanceOp>(operation)) {
        auto childBindings = analysis.bindCallee(
            analysis.lookupDefinition(instance.getCallee()),
            instance.getParametersAttr(), instance.getTypeArgumentsAttr(),
            bindings, instance);
        if (failed(childBindings) ||
            failed(requireConcreteBindings(*childBindings, instance)))
          return failure();
        ConcreteChildPlan child;
        child.occurrence = instance;
        child.instanceName = instance.getInstanceNameAttr();
        auto childName = legalizeIdentifier(child.instanceName.getValue(), [&] {
          return instance.emitOpError();
        });
        if (failed(childName) ||
            failed(bindPorts(plan.modules[index], child, instance)))
          return failure();
        child.cppName = "pyc_" + *childName;
        child.primitiveKind =
            StringAttr::get(package.getContext(),
                            analysis.getPrimitiveKind(childBindings->owner));
        if (!child.primitiveKind.getValue().empty()) {
          if (!isKnownPrimitive(child.primitiveKind.getValue()))
            return instance.emitOpError()
                   << "structural C++ does not support primitive kind '"
                   << child.primitiveKind.getValue() << "'";
          child.kind = ConcreteChildKind::Primitive;
          auto primitive =
              primitiveConfig(child.primitiveKind, *childBindings, instance);
          if (failed(primitive))
            return failure();
          child.primitive = std::move(*primitive);
        } else {
          if (!isa<hwir::ModuleOp>(childBindings->owner))
            return instance.emitOpError()
                   << "ordinary HWIR instance has no module definition";
          auto childIndex = visit(childBindings->owner, *childBindings);
          if (failed(childIndex))
            return failure();
          child.kind = ConcreteChildKind::Module;
          child.moduleIndex = childIndex;
          child.calleeKey = plan.modules[*childIndex].key;
        }
        plan.modules[index].children.push_back(std::move(child));
        continue;
      }
      if (auto queue = dyn_cast<hwir::QueueOp>(operation)) {
        auto config = resolveHWIRQueue(queue, bindings);
        if (failed(config) || config->depth == 0 ||
            config->tokenCardinality == 0 || config->packedWidth == 0 ||
            config->storageBitCount == 0 || config->availabilityLatency == 0)
          return failure();
        ConcreteChildPlan child;
        child.kind = ConcreteChildKind::Queue;
        child.occurrence = queue;
        child.instanceName = queue.getInstanceNameAttr();
        auto childName = legalizeIdentifier(
            queue.getInstanceName(), [&] { return queue.emitOpError(); });
        if (failed(childName))
          return failure();
        child.cppName = "pyc_" + *childName;
        child.primitiveKind = StringAttr::get(package.getContext(), "fifo");
        child.queue = std::move(*config);
        auto primitive =
            primitiveConfig(child.primitiveKind, bindings, queue, child.queue);
        if (failed(primitive))
          return failure();
        child.primitive = std::move(*primitive);
        if (failed(bindPorts(plan.modules[index], child, queue)))
          return failure();
        plan.modules[index].children.push_back(std::move(child));
        continue;
      }
      if (auto queue = dyn_cast<ac::QueueOp>(operation)) {
        auto config = analysis.resolveQueue(queue, bindings);
        auto name = queue->getAttrOfType<StringAttr>("instance_name");
        if (failed(config) || !name)
          return failure();
        if (config->depth == 0 || config->tokenCardinality == 0 ||
            config->packedWidth == 0 || config->storageBitCount == 0 ||
            config->availabilityLatency == 0)
          return queue.emitOpError()
                 << "structural C++ requires concrete queue depth, latency, "
                    "payload shape and storage size";
        ConcreteChildPlan child;
        child.kind = ConcreteChildKind::Queue;
        child.occurrence = queue;
        child.instanceName = name;
        auto childName = legalizeIdentifier(
            name.getValue(), [&] { return queue.emitOpError(); });
        if (failed(childName))
          return failure();
        child.cppName = "pyc_" + *childName;
        child.primitiveKind = StringAttr::get(package.getContext(), "fifo");
        child.queue = std::move(*config);
        auto primitive =
            primitiveConfig(child.primitiveKind, bindings, queue, child.queue);
        if (failed(primitive))
          return failure();
        child.primitive = std::move(*primitive);
        if (failed(bindPorts(plan.modules[index], child, queue)))
          return failure();
        plan.modules[index].children.push_back(std::move(child));
        continue;
      }
      auto instance = dyn_cast<ac::InstanceOp>(operation);
      if (!instance)
        continue;
      auto childBindings = analysis.bindInstance(instance, bindings);
      if (failed(childBindings) ||
          failed(requireConcreteBindings(*childBindings, instance)))
        return failure();
      for (Type type : instance->getOperandTypes())
        if (failed(concreteType(type, bindings, instance)))
          return failure();
      for (Type type : instance->getResultTypes())
        if (failed(concreteType(type, bindings, instance)))
          return failure();

      ConcreteChildPlan child;
      child.occurrence = instance;
      child.instanceName = instance.getInstanceNameAttr();
      auto childName = legalizeIdentifier(child.instanceName.getValue(), [&] {
        return instance.emitOpError();
      });
      if (failed(childName))
        return failure();
      child.cppName = "pyc_" + *childName;
      if (failed(bindPorts(plan.modules[index], child, instance)))
        return failure();
      child.primitiveKind =
          StringAttr::get(package.getContext(),
                          analysis.getPrimitiveKind(childBindings->owner));
      if (!child.primitiveKind.getValue().empty()) {
        StringRef kind = child.primitiveKind.getValue();
        if (!isKnownPrimitive(kind))
          return instance.emitOpError()
                 << "structural C++ does not support primitive kind '" << kind
                 << "'";
        if (kind == "byte_mem" || kind == "sync_mem" || kind == "sync_mem_dp") {
          for (StringRef parameter :
               {StringRef("ADDR_WIDTH"), StringRef("DEPTH")})
            if (!isa_and_nonnull<ac::MathIntAttr>(
                    childBindings->integers.lookup(parameter)))
              return instance.emitOpError()
                     << "structural C++ requires concrete memory parameter '"
                     << parameter << "'";
        }
        child.kind = ConcreteChildKind::Primitive;
        auto primitive =
            primitiveConfig(child.primitiveKind, *childBindings, instance);
        if (failed(primitive))
          return failure();
        child.primitive = std::move(*primitive);
      } else {
        auto childDefinition = dyn_cast<ac::ModuleOp>(childBindings->owner);
        if (!childDefinition)
          return instance.emitOpError()
                 << "structural C++ ordinary instance has no module definition";
        auto childIndex = visit(childDefinition, *childBindings);
        if (failed(childIndex))
          return failure();
        child.kind = ConcreteChildKind::Module;
        child.moduleIndex = childIndex;
        child.calleeKey = plan.modules[*childIndex].key;
      }
      plan.modules[index].children.push_back(std::move(child));
    }

    if (failed(materializeSchedule(index)))
      return definition->emitOpError()
             << "failed to materialize the concrete module schedule";
    materializeModuleLifecycle(index);
    return index;
  }

  ModuleOp package;
  ac::HardwareAnalysis &analysis;
  ConcreteDesignPlan plan;
  llvm::DenseSet<unsigned> active;
  llvm::StringMap<unsigned> moduleIndices;
  llvm::StringMap<llvm::DenseMap<Value, ConcreteValueId>> valueIds;
};

} // namespace

FailureOr<ConcreteDesignPlan>
buildConcreteDesignPlan(ModuleOp package, ac::HardwareAnalysis &analysis) {
  if (!package)
    return failure();
  return Builder(package, analysis).build();
}

LogicalResult verifyConcreteDesignPlan(const ConcreteDesignPlan &plan) {
  if (plan.rootModule >= plan.modules.size())
    return plan.package->emitError()
           << "structural C++ plan has an invalid root module index";
  llvm::SmallVector<unsigned, 0> ownership(plan.modules.size(), 0);
  for (auto [unitIndex, unit] : llvm::enumerate(plan.sourceUnits)) {
    if (!unit.sourceOwner)
      return plan.package->emitError()
             << "structural C++ source unit has no owner";
    for (unsigned moduleIndex : unit.moduleIndices) {
      if (moduleIndex >= plan.modules.size() || ++ownership[moduleIndex] != 1)
        return plan.package->emitError()
               << "structural C++ module source ownership is invalid";
      if (plan.modules[moduleIndex].key.definition->getAttr("source_owner") !=
          unit.sourceOwner)
        return plan.package->emitError()
               << "structural C++ module source owner does not match its "
                  "unit";
    }
    for (unsigned dependency : unit.dependencies)
      if (dependency >= plan.sourceUnits.size() || dependency == unitIndex)
        return plan.package->emitError()
               << "structural C++ source-unit dependency is invalid";
  }
  if (llvm::any_of(ownership, [](unsigned count) { return count != 1; }))
    return plan.package->emitError()
           << "structural C++ source-unit inventory is incomplete";
  for (auto [moduleIndex, module] : llvm::enumerate(plan.modules)) {
    for (auto [valueIndex, value] : llvm::enumerate(module.values))
      if (value.id != valueIndex || value.type.packedWidth == 0 ||
          value.type.cppTypeName.empty() ||
          (value.availabilityParent &&
           (*value.availabilityParent >= module.values.size() ||
            value.kind != ConcreteValueKind::TableElement)))
        return module.key.definition->emitOpError()
               << "structural C++ plan has an invalid concrete value";
    for (const ConcreteChildPlan &child : module.children) {
      if (!child.occurrence)
        return module.key.definition->emitOpError()
               << "structural C++ plan has a child without an occurrence";
      if (child.kind == ConcreteChildKind::Module) {
        if (!child.moduleIndex || *child.moduleIndex >= plan.modules.size())
          return child.occurrence->emitOpError()
                 << "structural C++ plan has an invalid module child";
      } else {
        if (!child.primitive || child.primitive->runtimeCppType.empty())
          return child.occurrence->emitOpError()
                 << "structural C++ has no approved primitive mapping";
        if (child.primitive->inputMembers.size() != child.inputs.size() ||
            child.primitive->outputMembers.size() != child.outputs.size() ||
            child.primitive->outputIsTemporal.size() != child.outputs.size() ||
            child.primitive->outputRefreshInputs.size() !=
                child.outputs.size())
          return child.occurrence->emitOpError()
                 << "structural C++ primitive plan has an invalid pin schema";
      }
      for (const ConcretePortBinding &binding : child.inputs)
        if (binding.value >= module.values.size())
          return child.occurrence->emitOpError()
                 << "structural C++ child input has an invalid value ID";
      for (const ConcretePortBinding &binding : child.outputs)
        if (binding.value >= module.values.size())
          return child.occurrence->emitOpError()
                 << "structural C++ child output has an invalid value ID";
    }
    for (const ConcreteScheduleStep &step : module.concreteSchedule.steps) {
      for (ConcreteValueId value : step.inputs)
        if (value >= module.values.size())
          return step.operation->emitOpError()
                 << "structural C++ schedule input has an invalid value ID";
      for (ConcreteValueId value : step.outputs)
        if (value >= module.values.size())
          return step.operation->emitOpError()
                 << "structural C++ schedule output has an invalid value ID";
      if (step.childIndex && *step.childIndex >= module.children.size())
        return step.operation->emitOpError()
               << "structural C++ schedule has an invalid child index";
      if (step.kind != ConcreteScheduleStepKind::ComputeValue)
        continue;
      if (!step.compute ||
          ((step.compute->kind == ConcreteComputeKind::TableGet ||
            step.compute->kind == ConcreteComputeKind::TableChoose ||
            step.compute->kind == ConcreteComputeKind::ValueMerge ||
            step.compute->kind == ConcreteComputeKind::EnumFromBits)
               ? (step.compute->kind == ConcreteComputeKind::TableChoose
                      ? step.outputs.size() != *step.compute->immediate * 2
                      : step.outputs.size() != 2)
               : step.outputs.size() != 1))
        return step.operation->emitOpError()
               << "structural C++ has no approved compute mapping";
      const ConcreteComputePlan &compute = *step.compute;
      switch (compute.kind) {
      case ConcreteComputeKind::BitsConstant:
        if (!step.inputs.empty() || compute.constantWords.empty())
          return step.operation->emitOpError()
                 << "structural C++ has an invalid constant mapping";
        break;
      case ConcreteComputeKind::BitsConstant4:
        if (!step.inputs.empty() || compute.constantWords.empty() ||
            compute.constantWords.size() != compute.knownWords.size() ||
            compute.constantWords.size() != compute.zWords.size())
          return step.operation->emitOpError()
                 << "structural C++ has an invalid four-state constant mapping";
        break;
      case ConcreteComputeKind::PackedIdentity:
        if (step.inputs.size() != 1)
          return step.operation->emitOpError()
                 << "structural C++ has an invalid packed identity mapping";
        break;
      case ConcreteComputeKind::EnumFromBits:
        if (step.inputs.size() != 1 || step.outputs.size() != 2 ||
            compute.enumCodes.empty())
          return step.operation->emitOpError()
                 << "structural C++ has an invalid enum-from-bits mapping";
        break;
      case ConcreteComputeKind::TableSplat:
        if (step.inputs.size() != 1 || step.outputs.size() != 1 ||
            !compute.immediate || *compute.immediate == 0)
          return step.operation->emitOpError()
                 << "structural C++ has an invalid table splat mapping";
        break;
      case ConcreteComputeKind::TableCreate:
        if (step.inputs.empty() || step.outputs.size() != 1)
          return step.operation->emitOpError()
                 << "structural C++ has an invalid table create mapping";
        break;
      case ConcreteComputeKind::TableGet:
        if (step.inputs.size() != 2 || step.outputs.size() != 2)
          return step.operation->emitOpError()
                 << "structural C++ has an invalid table get mapping";
        break;
      case ConcreteComputeKind::TableView:
        if (step.inputs.size() != 1 || step.outputs.size() != 1 ||
            compute.laneMap.empty())
          return step.operation->emitOpError() << "structural C++ has an invalid table view mapping";
        break;
      case ConcreteComputeKind::TableIndex:
        if (step.inputs.empty() || step.outputs.size() != 1 ||
            compute.laneMap.size() != step.inputs.size())
          return step.operation->emitOpError() << "structural C++ has an invalid table index mapping";
        break;
      case ConcreteComputeKind::TableMatch:
        if (step.inputs.empty() || step.outputs.size() != 1)
          return step.operation->emitOpError() << "structural C++ has an invalid table match mapping";
        break;
      case ConcreteComputeKind::TableChoose:
        if (step.inputs.size() != 2 || !compute.immediate ||
            step.outputs.size() != *compute.immediate * 2)
          return step.operation->emitOpError() << "structural C++ has an invalid table choose mapping";
        break;
      case ConcreteComputeKind::TableFold:
        if (step.inputs.size() != 1 || step.outputs.size() != 1)
          return step.operation->emitOpError() << "structural C++ has an invalid table fold mapping";
        break;
      case ConcreteComputeKind::ValueMerge:
        if (step.outputs.size() != 2 || step.inputs.empty() ||
            (step.inputs.size() - 1) % 2 ||
            compute.laneMap.size() != step.inputs.size() - 1)
          return step.operation->emitOpError() << "structural C++ has an invalid value merge mapping";
        break;
      case ConcreteComputeKind::BitsUnary:
      case ConcreteComputeKind::BitsBinary:
      case ConcreteComputeKind::BitsCompare:
      case ConcreteComputeKind::BitsSelect:
      case ConcreteComputeKind::BitsConcat:
      case ConcreteComputeKind::BitsExtract:
      case ConcreteComputeKind::BitsResize:
        break;
      }
      if (compute.kind == ConcreteComputeKind::BitsUnary &&
          (step.inputs.size() != 1 || compute.opcode != "bit_not"))
        return step.operation->emitOpError()
               << "structural C++ has an invalid unary mapping";
      if (compute.kind == ConcreteComputeKind::BitsBinary &&
          (step.inputs.size() != 2 ||
           !llvm::is_contained(ArrayRef<StringRef>{"bit_and", "bit_or",
                                                   "bit_xor", "add", "sub",
                                                   "mul", "udiv", "urem", "shl",
                                                   "lshr", "ashr"},
                               compute.opcode)))
        return step.operation->emitOpError()
               << "structural C++ has no approved binary mapping for '"
               << compute.opcode << "'";
      if (compute.kind == ConcreteComputeKind::BitsCompare &&
          (step.inputs.size() != 2 ||
           !llvm::is_contained(ArrayRef<StringRef>{"EQ", "NE", "CASE_EQ", "ULT",
                                                   "ULE", "UGT", "UGE", "SLT",
                                                   "SLE", "SGT", "SGE"},
                               compute.opcode)))
        return step.operation->emitOpError()
               << "structural C++ has an invalid compare mapping";
      if (compute.kind == ConcreteComputeKind::BitsSelect &&
          step.inputs.size() != 3)
        return step.operation->emitOpError()
               << "structural C++ has an invalid select mapping";
      if (compute.kind == ConcreteComputeKind::BitsConcat &&
          step.inputs.empty())
        return step.operation->emitOpError()
               << "structural C++ has an invalid concat mapping";
      if (compute.kind == ConcreteComputeKind::BitsExtract &&
          (step.inputs.size() != 1 || !compute.immediate))
        return step.operation->emitOpError()
               << "structural C++ has an invalid extract mapping";
      if (compute.kind == ConcreteComputeKind::BitsResize &&
          (step.inputs.size() != 1 ||
           !llvm::is_contained(ArrayRef<StringRef>{"trunc", "zext", "sext"},
                               compute.opcode)))
        return step.operation->emitOpError()
               << "structural C++ has an invalid resize mapping";
    }
    for (std::size_t begin = 0; begin < module.observationSites.size();) {
      const ConcreteObservationSitePlan &first = module.observationSites[begin];
      if (first.groupIndex != 0 || first.groupSize == 0 ||
          first.groupSize > module.observationSites.size() - begin)
        return module.key.definition->emitOpError()
               << "structural C++ observation plan has an invalid group";
      for (std::size_t member = 0; member < first.groupSize; ++member) {
        const ConcreteObservationSitePlan &site =
            module.observationSites[begin + member];
        if (site.path >= module.values.size() ||
            site.value >= module.values.size() || site.payloadWidth == 0 ||
            site.payloadWidth > std::numeric_limits<uint32_t>::max() ||
            site.groupIndex != member || site.groupSize != first.groupSize ||
            site.kind != first.kind ||
            site.registrationKey != first.registrationKey ||
            site.siteKey != first.siteKey || site.kindName.empty() ||
            site.registrationJson.empty() || site.siteJson.empty() ||
            site.specJson.empty() ||
            (site.kind == ConcreteObservationKind::Gauge &&
             (site.groupSize != 1 || site.payloadWidth > 64)))
          return module.key.definition->emitOpError()
                 << "structural C++ observation site is incomplete";
      }
      begin += first.groupSize;
    }
    (void)moduleIndex;
  }
  uint64_t eventCapacity = 0;
  for (auto [ordinal, occurrence] : llvm::enumerate(plan.observations)) {
    if (ordinal > std::numeric_limits<uint32_t>::max() ||
        occurrence.stableOrdinal != ordinal ||
        occurrence.moduleIndex >= plan.modules.size() ||
        occurrence.localSiteIndex >=
            plan.modules[occurrence.moduleIndex].observationSites.size() ||
        occurrence.physicalInstancePath.empty() ||
        occurrence.instancePath.empty())
      return plan.package->emitError()
             << "structural C++ observation descriptor is incomplete";
    const ConcreteObservationSitePlan &site =
        plan.modules[occurrence.moduleIndex]
            .observationSites[occurrence.localSiteIndex];
    if (occurrence.registrationKey != site.registrationKey ||
        occurrence.siteKey != site.siteKey || occurrence.kind != site.kind ||
        occurrence.groupIndex != site.groupIndex ||
        occurrence.groupSize != site.groupSize ||
        occurrence.payloadWidth != site.payloadWidth ||
        occurrence.kindName != site.kindName ||
        occurrence.registrationJson != site.registrationJson ||
        occurrence.siteJson != site.siteJson ||
        occurrence.specJson != site.specJson ||
        occurrence.carriesValue != site.carriesValue ||
        occurrence.reportName != site.reportName)
      return plan.package->emitError()
             << "structural C++ observation descriptor disagrees with its "
                "local site";
    if (ordinal != 0) {
      const ConcreteObservationPlan &previous = plan.observations[ordinal - 1];
      if (!(std::tie(previous.ownerKey, previous.registrationKey,
                     previous.siteKey, previous.stableOrdinal) <
            std::tie(occurrence.ownerKey, occurrence.registrationKey,
                     occurrence.siteKey, occurrence.stableOrdinal)))
        return plan.package->emitError()
               << "structural C++ observation descriptors are not strictly "
                  "ordered";
    }
    if (occurrence.kind == ConcreteObservationKind::Event)
      ++eventCapacity;
  }
  if (eventCapacity != plan.observationEventCapacity ||
      plan.observations.size() !=
          plan.modules[plan.rootModule].observationSubtreeWidth)
    return plan.package->emitError()
           << "structural C++ observation inventory is inconsistent";
  if (plan.system) {
    const ConcreteModulePlan &root = plan.modules[plan.rootModule];
    if (root.inputs.size() != 2 || !root.outputs.empty() ||
        plan.system->clockInput >= root.inputs.size() ||
        plan.system->resetInput >= root.inputs.size() ||
        plan.system->clockInput == plan.system->resetInput)
      return plan.package->emitError()
             << "structural C++ system runner plan is incomplete";
  }
  return success();
}

} // namespace acir::compiler
