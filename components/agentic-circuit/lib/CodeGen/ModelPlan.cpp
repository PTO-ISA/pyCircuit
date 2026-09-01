#include "acir/CodeGen/ModelPlan.h"

#include "ModelPlanInternal.h"

#include "acir/Dialect/ACSim/ACSimOps.h"

#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Operation.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Error.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <optional>
#include <system_error>
#include <utility>

namespace acir::codegen {
namespace {

llvm::Error planError(const llvm::Twine &code, const llvm::Twine &message) {
  return llvm::createStringError(
      std::make_error_code(std::errc::invalid_argument), code + ": " + message);
}

std::optional<TypeKind> parseTypeKind(llvm::StringRef kind) {
  return llvm::StringSwitch<std::optional<TypeKind>>(kind)
      .Case("accessor", TypeKind::Accessor)
      .Case("implementation", TypeKind::Implementation)
      .Case("interface", TypeKind::Interface)
      .Case("packet", TypeKind::Packet)
      .Case("policy", TypeKind::Policy)
      .Case("protocol", TypeKind::Protocol)
      .Case("provider", TypeKind::Provider)
      .Case("resource", TypeKind::Resource)
      .Case("role", TypeKind::Role)
      .Case("schema", TypeKind::Schema)
      .Case("time_domain", TypeKind::TimeDomain)
      .Case("value", TypeKind::Value)
      .Case("wake", TypeKind::Wake)
      .Case("payload", TypeKind::Payload)
      .Default(std::nullopt);
}

std::string symbolRefString(mlir::SymbolRefAttr symbol) {
  std::string result = symbol.getRootReference().getValue().str();
  for (mlir::FlatSymbolRefAttr nested : symbol.getNestedReferences()) {
    result += "::";
    result += nested.getValue();
  }
  return result;
}

RuntimeObjectKind inferRuntimeObjectKind(acsim::ModelOp model,
                                         mlir::SymbolRefAttr target) {
  auto nested = target.getNestedReferences();
  if (nested.size() != 1)
    return RuntimeObjectKind::External;

  const llvm::StringRef moduleName = target.getRootReference().getValue();
  const llvm::StringRef memberName = nested.front().getValue();
  for (mlir::Operation &operation : model.getBody().front()) {
    auto module = mlir::dyn_cast<acsim::ModuleOp>(operation);
    if (!module || module.getSymName() != moduleName)
      continue;
    for (mlir::Operation &member : module.getBody().front()) {
      auto process = mlir::dyn_cast<acsim::ProcessOp>(member);
      if (process && process.getSymName() == memberName)
        return RuntimeObjectKind::Process;
    }
  }
  return RuntimeObjectKind::External;
}

const BindingPlan *findBinding(const ModelPlan &plan, llvm::StringRef symbol) {
  auto found = std::find_if(
      plan.bindings.begin(), plan.bindings.end(),
      [&](const BindingPlan &binding) { return binding.symbol == symbol; });
  return found == plan.bindings.end() ? nullptr : &*found;
}

const ModulePlan *findModule(const ModelPlan &plan, llvm::StringRef symbol) {
  auto found = std::find_if(
      plan.modules.begin(), plan.modules.end(),
      [&](const ModulePlan &module) { return module.symbol == symbol; });
  return found == plan.modules.end() ? nullptr : &*found;
}

bool isTraceSourceBinding(const ModelPlan &plan, const BindingPlan &binding) {
  if (binding.cppSymbol == "gfsim::TraceSource" ||
      llvm::StringRef(binding.cppSymbol).starts_with("gfsim::TraceSource<") ||
      binding.cppSymbol == "gfsim::ShowcaseTraceSource" ||
      binding.cppSymbol == "gfsim::NpuTraceSource")
    return true;
  auto schema = std::find_if(plan.types.begin(), plan.types.end(),
                             [&](const TypePlan &type) {
                               return type.symbol == binding.componentSchema;
                             });
  return schema != plan.types.end() && schema->cppType == "ac.TraceSource";
}

bool isTraceOwner(const ModelPlan &plan,
                  const RuntimeObjectPlan &runtimeObject) {
  const ModulePlan *module = findModule(plan, plan.rootSymbol);
  if (!module)
    return false;
  llvm::SmallVector<llvm::StringRef> segments;
  llvm::StringRef(runtimeObject.hierarchyPath).split(segments, '.');
  if (segments.size() < 2)
    return false;
  for (size_t index = 1; index < segments.size(); ++index) {
    const llvm::StringRef symbol =
        segments[index].take_until([](char value) { return value == '['; });
    auto placement =
        std::find_if(module->placements.begin(), module->placements.end(),
                     [&](const PlacementPlan &candidate) {
                       return candidate.symbol == symbol;
                     });
    if (placement == module->placements.end())
      return false;
    llvm::StringRef target = placement->target;
    target = target.take_until([](char value) { return value == ':'; });
    if (index + 1 == segments.size()) {
      const BindingPlan *binding = findBinding(plan, target);
      return binding && isTraceSourceBinding(plan, *binding);
    }
    module = findModule(plan, target);
    if (!module)
      return false;
  }
  return false;
}

llvm::Expected<uint32_t> checkedId(uint64_t value, llvm::StringRef field) {
  if (value > std::numeric_limits<uint32_t>::max())
    return planError("ACLOWER-DISPATCH", field + " exceeds uint32_t");
  return static_cast<uint32_t>(value);
}

llvm::Expected<std::vector<std::string>> stringArray(mlir::ArrayAttr values,
                                                     llvm::StringRef field) {
  std::vector<std::string> result;
  result.reserve(values.size());
  for (mlir::Attribute value : values) {
    auto string = mlir::dyn_cast<mlir::StringAttr>(value);
    if (!string)
      return planError("ACLOWER-FINGERPRINT",
                       field + " contains a non-string value");
    result.push_back(string.getValue().str());
  }
  return result;
}

llvm::Expected<Fingerprint> fingerprintField(mlir::DictionaryAttr fields,
                                             llvm::StringRef name) {
  auto value = fields.getAs<mlir::StringAttr>(name);
  if (!value || !isValidFingerprint(value.getValue()))
    return planError("ACLOWER-FINGERPRINT",
                     "model fingerprint '" + name + "' is invalid");
  return value.getValue().str();
}

} // namespace

llvm::Error validateModelPlan(const ModelPlan &plan) {
  if (plan.modelSymbol.empty() || plan.rootSymbol.empty())
    return planError("ACLOWER-FINGERPRINT",
                     "model and root symbols must be non-empty");
  if (plan.contractEpoch != "0.4")
    return planError("ACLOWER-FINGERPRINT", "model contract epoch must be 0.4");
  for (const Fingerprint *fingerprint :
       {&plan.frozenAcirFingerprint, &plan.bindingLockFingerprint,
        &plan.providerFingerprint, &plan.profileFingerprint,
        &plan.toolchainFingerprint, &plan.schemaSetFingerprint}) {
    if (!isValidFingerprint(*fingerprint))
      return planError("ACLOWER-FINGERPRINT",
                       "model contains an invalid input fingerprint");
  }
  if (plan.constructionOrder.size() != plan.destructionOrder.size())
    return planError("ACLOWER-OWNERSHIP",
                     "construction and destruction orders differ in size");
  if (!std::equal(plan.constructionOrder.rbegin(),
                  plan.constructionOrder.rend(), plan.destructionOrder.begin()))
    return planError("ACLOWER-OWNERSHIP",
                     "destruction order must reverse construction order");

  llvm::StringRef previousType;
  for (const TypePlan &type : plan.types) {
    if (type.symbol.empty() || type.cppType.empty() ||
        !isValidFingerprint(type.fingerprint))
      return planError("ACLOWER-TYPE-MISMATCH", "type plan is incomplete");
    if (!previousType.empty() && previousType >= type.symbol)
      return planError("ACLOWER-TYPE-MISMATCH",
                       "type plans are not strictly symbol-sorted");
    previousType = type.symbol;
  }

  auto isDomainName = [](llvm::StringRef name) {
    if (name.empty() ||
        !(std::isalpha(static_cast<unsigned char>(name.front())) ||
          name.front() == '_'))
      return false;
    return llvm::all_of(name.drop_front(), [](char character) {
      return std::isalnum(static_cast<unsigned char>(character)) ||
             character == '_' || character == '.' || character == '-';
    });
  };
  llvm::StringRef previousDomain;
  for (const TimeDomainPlan &domain : plan.timeDomains) {
    if (!isDomainName(domain.name) || domain.period == 0 ||
        domain.tickScale == 0 ||
        (!previousDomain.empty() && previousDomain >= domain.name))
      return planError("ACLOWER-TIME-DOMAIN",
                       "time-domain plans are not canonical and complete");
    previousDomain = domain.name;
  }

  for (size_t index = 0; index < plan.runtimeObjects.size(); ++index) {
    const RuntimeObjectPlan &object = plan.runtimeObjects[index];
    if (object.objectId != index || object.activationId != index)
      return planError("ACLOWER-DISPATCH",
                       "runtime object and activation IDs must be dense");
    if (object.targetSymbol.empty() || object.hierarchyPath.empty() ||
        object.workThunk.empty() || object.xferThunk.empty() ||
        object.resetThunk.empty() || object.validateThunk.empty())
      return planError("ACLOWER-DISPATCH",
                       "runtime dispatch row is incomplete");
  }

  std::optional<ActivationEdgePlan> previousEdge;
  for (const ActivationEdgePlan &edge : plan.activationEdges) {
    if (edge.sourceId >= plan.runtimeObjects.size() ||
        edge.targetId >= plan.runtimeObjects.size())
      return planError("ACLOWER-ACTIVATION",
                       "activation edge references an unknown dense ID");
    if (previousEdge && *previousEdge >= edge)
      return planError("ACLOWER-ACTIVATION",
                       "activation edges are not sorted and unique");
    previousEdge = edge;
  }
  return detail::validateModelDetails(plan);
}

llvm::Expected<ModelPlan> buildModelPlan(mlir::ModuleOp canonicalACSim) {
  if (!canonicalACSim ||
      mlir::failed(acsim::verifyCanonicalACSimFile(canonicalACSim)))
    return planError("ACLOWER-FINGERPRINT",
                     "input is not verified canonical ACSim");

  acsim::ModelOp model;
  unsigned modelCount = 0;
  canonicalACSim.walk([&](acsim::ModelOp candidate) {
    model = candidate;
    ++modelCount;
  });
  if (modelCount != 1)
    return planError("ACLOWER-FINGERPRINT",
                     "input must contain exactly one ACSim model");

  ModelPlan plan;
  plan.modelSymbol = model.getSymName().str();
  plan.rootSymbol = model.getRoot().str();
  plan.contractEpoch = model.getContractEpoch().str();

  auto construction =
      stringArray(model.getConstructionOrder(), "construction_order");
  if (!construction)
    return construction.takeError();
  plan.constructionOrder = std::move(*construction);
  auto destruction =
      stringArray(model.getDestructionOrder(), "destruction_order");
  if (!destruction)
    return destruction.takeError();
  plan.destructionOrder = std::move(*destruction);

  auto readFingerprint = [&](llvm::StringRef name,
                             Fingerprint &destination) -> llvm::Error {
    auto value = fingerprintField(model.getFingerprints(), name);
    if (!value)
      return value.takeError();
    destination = std::move(*value);
    return llvm::Error::success();
  };
  if (auto error = readFingerprint("frozen_acir", plan.frozenAcirFingerprint))
    return std::move(error);
  if (auto error = readFingerprint("binding_lock", plan.bindingLockFingerprint))
    return std::move(error);
  if (auto error = readFingerprint("provider", plan.providerFingerprint))
    return std::move(error);
  if (auto error = readFingerprint("profile", plan.profileFingerprint))
    return std::move(error);
  if (auto error = readFingerprint("toolchain", plan.toolchainFingerprint))
    return std::move(error);
  if (auto error = readFingerprint("schema_set", plan.schemaSetFingerprint))
    return std::move(error);

  for (mlir::Operation &operation : model.getBody().front()) {
    if (auto type = mlir::dyn_cast<acsim::TypeOp>(operation)) {
      auto kind = parseTypeKind(type.getKind());
      if (!kind)
        return planError("ACLOWER-TYPE-MISMATCH",
                         "unknown closed ACSim type kind");
      plan.types.push_back({type.getSymName().str(), *kind,
                            type.getCppName().str(),
                            type.getFingerprint().str()});
      if (*kind == TypeKind::TimeDomain) {
        mlir::IntegerAttr period = type.getPeriodAttr();
        mlir::IntegerAttr phase = type.getPhaseAttr();
        mlir::IntegerAttr tickScale = type.getTickScaleAttr();
        if (period || phase || tickScale) {
          if (!period || !phase || !tickScale || period.getInt() <= 0 ||
              phase.getInt() < 0 || tickScale.getInt() <= 0)
            return planError("ACLOWER-TIME-DOMAIN",
                             "time-domain runtime metadata is incomplete");
          plan.timeDomains.push_back(
              {type.getSymName().str(), static_cast<uint64_t>(period.getInt()),
               static_cast<uint64_t>(phase.getInt()),
               static_cast<uint64_t>(tickScale.getInt())});
        }
      }
      continue;
    }

    if (auto dispatch = mlir::dyn_cast<acsim::DispatchOp>(operation)) {
      auto objectId = checkedId(dispatch.getObjectId(), "object_id");
      if (!objectId)
        return objectId.takeError();
      auto activationId =
          checkedId(dispatch.getActivationId(), "activation_id");
      if (!activationId)
        return activationId.takeError();
      RuntimeObjectPlan object;
      object.objectId = *objectId;
      object.activationId = *activationId;
      object.targetSymbol = symbolRefString(dispatch.getTargetAttr());
      object.hierarchyPath = dispatch.getPath().str();
      object.objectKind =
          inferRuntimeObjectKind(model, dispatch.getTargetAttr());
      object.workThunk = dispatch.getWork().str();
      object.xferThunk = dispatch.getXfer().str();
      object.resetThunk = dispatch.getReset().str();
      object.validateThunk = dispatch.getValidate().str();
      for (int64_t index : dispatch.getIndices()) {
        if (index < 0)
          return planError("ACLOWER-ARRAY",
                           "dispatch index cannot be negative");
        object.indices.push_back(static_cast<uint64_t>(index));
      }
      plan.runtimeObjects.push_back(std::move(object));
      continue;
    }

    if (auto activate = mlir::dyn_cast<acsim::ActivateOp>(operation)) {
      auto source = activate.getSource().getDefiningOp<acsim::DispatchOp>();
      auto target = activate.getTarget().getDefiningOp<acsim::DispatchOp>();
      if (!source || !target)
        return planError("ACLOWER-ACTIVATION",
                         "activation operands must resolve to dispatch rows");
      auto sourceId = checkedId(source.getActivationId(), "activation source");
      if (!sourceId)
        return sourceId.takeError();
      auto targetId = checkedId(target.getObjectId(), "activation target");
      if (!targetId)
        return targetId.takeError();
      plan.activationEdges.push_back({*sourceId, *targetId});
    }
  }

  std::sort(plan.types.begin(), plan.types.end(),
            [](const TypePlan &lhs, const TypePlan &rhs) {
              return lhs.symbol < rhs.symbol;
            });
  std::sort(plan.runtimeObjects.begin(), plan.runtimeObjects.end(),
            [](const RuntimeObjectPlan &lhs, const RuntimeObjectPlan &rhs) {
              return lhs.objectId < rhs.objectId;
            });
  std::sort(plan.timeDomains.begin(), plan.timeDomains.end(),
            [](const TimeDomainPlan &lhs, const TimeDomainPlan &rhs) {
              return lhs.name < rhs.name;
            });
  std::sort(plan.activationEdges.begin(), plan.activationEdges.end());

  if (auto error = detail::populateModelDetails(model, plan))
    return std::move(error);
  for (RuntimeObjectPlan &runtimeObject : plan.runtimeObjects)
    runtimeObject.traceOwner = isTraceOwner(plan, runtimeObject);
  if (auto error = validateModelPlan(plan))
    return std::move(error);
  return plan;
}

} // namespace acir::codegen
