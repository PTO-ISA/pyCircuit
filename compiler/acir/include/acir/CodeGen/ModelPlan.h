#ifndef ACIR_CODEGEN_MODELPLAN_H
#define ACIR_CODEGEN_MODELPLAN_H

#include "acir/CodeGen/Manifest.h"

#include "mlir/IR/BuiltinOps.h"
#include "llvm/Support/Error.h"

#include <compare>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace acir::codegen {

enum class TypeKind {
  Accessor,
  Implementation,
  Interface,
  Packet,
  Policy,
  Protocol,
  Provider,
  Resource,
  Role,
  Schema,
  TimeDomain,
  Value,
  Wake,
  Payload,
};

struct TypePlan {
  std::string symbol;
  TypeKind kind;
  std::string cppType;
  Fingerprint fingerprint;
};

struct TimeDomainPlan {
  std::string name;
  uint64_t period = 1;
  uint64_t phase = 0;
  uint64_t tickScale = 1;
};

enum class RuntimeObjectKind { External, Process };

struct RuntimeObjectPlan {
  uint32_t objectId = 0;
  uint32_t activationId = 0;
  std::string targetSymbol;
  std::string hierarchyPath;
  std::vector<uint64_t> indices;
  RuntimeObjectKind objectKind = RuntimeObjectKind::External;
  bool traceOwner = false;
  std::string workThunk;
  std::string xferThunk;
  std::string resetThunk;
  std::string validateThunk;
};

struct ActivationEdgePlan {
  uint32_t sourceId = 0;
  uint32_t targetId = 0;

  auto operator<=>(const ActivationEdgePlan &) const = default;
};

struct SourceMapPlan {
  std::string stableIdentity;
  std::string source;
};

enum class BindingEffect { Pure, Stateful };
enum class ParameterMappingKind {
  TemplateArgument,
  ConstexprArgument,
  ConstructorConstant,
};

struct ParameterPlan {
  std::string name;
  std::string acirType;
  std::string cppType;
  llvm::json::Value canonicalValue = nullptr;
  uint32_t ordinal = 0;
  ParameterMappingKind mapping = ParameterMappingKind::ConstructorConstant;
};

struct EntryPointPlan {
  std::string pure;
  std::string reset;
  std::string validate;
  std::string work;
  std::string xfer;
};

struct PortBindingPlan {
  std::string accessor;
  std::string cardinality;
  std::string delegation;
  std::string direction;
  std::string interfaceType;
  std::string ownership;
  std::string payload;
  std::string protocol;
  std::string role;
  std::string timeDomain;
};

struct ResourceBindingPlan {
  std::string accessor;
  std::string delegation;
  std::string mode;
  std::string ownership;
  std::string resource;
  std::string role;
  std::string timeDomain;
};

struct ResultBindingPlan {
  std::string name;
  std::string cppType;
};

struct ActivationSourcePlan {
  std::string name;
  std::string kind;
};

struct BindingPlan {
  std::string symbol;
  std::string bindingId;
  BindingEffect effect = BindingEffect::Pure;
  std::string header;
  std::string target;
  std::string cppSymbol;
  std::string conceptName;
  std::string cppType;
  std::string implementation;
  std::string provider;
  std::string componentSchema;
  Fingerprint recordFingerprint;
  Fingerprint componentSchemaFingerprint;
  Fingerprint providerImplementationFingerprint;
  EntryPointPlan entryPoints;
  std::vector<llvm::json::Value> constructorArguments;
  std::string ownershipKind;
  std::string ownershipPlacement;
  std::vector<ParameterPlan> parameters;
  std::vector<PortBindingPlan> ports;
  std::vector<ResourceBindingPlan> resources;
  std::vector<ResultBindingPlan> results;
  std::vector<ActivationSourcePlan> activationSources;
};

enum class PlacementKind {
  GeneratedModule,
  ExternalStateful,
  HomogeneousArray,
};

struct PlacementPlan {
  PlacementKind kind = PlacementKind::ExternalStateful;
  std::string symbol;
  std::string memberName;
  std::string target;
  std::string resultValue;
  Fingerprint specializationFingerprint;
  std::vector<uint64_t> shape;
  std::vector<llvm::json::Value> staticArguments;
};

enum class ProjectionKind { Element, Port, Resource };

struct ProjectionPlan {
  ProjectionKind kind = ProjectionKind::Element;
  std::string resultValue;
  std::string baseValue;
  std::vector<uint64_t> indices;
  std::string accessor;
  std::string resultType;
};

struct BindPlan {
  std::string sourceValue;
  std::string targetValue;
  std::string kind;
};

struct ExpressionPlan {
  std::string resultValue;
  std::string callee;
  std::vector<std::string> arguments;
  std::string resultType;
};

struct ExportPlan {
  std::string symbol;
  std::string sourceValue;
  std::string resultValue;
  std::string role;
  std::string resultType;
};

struct CapturePlan {
  std::string name;
  std::string sourceValue;
  std::string type;
};

struct LiveSlotPlan {
  std::string name;
  std::string type;
};

struct LiveLoadPlan {
  std::string resultValue;
  std::string slot;
  std::string type;
};
struct LiveStorePlan {
  std::string sourceValue;
  std::string slot;
};
struct ConstantPlan {
  std::string resultValue;
  std::string resultType;
  llvm::json::Value canonicalValue;
};
struct ArithmeticPlan {
  std::string operationName;
  std::vector<std::string> arguments;
  std::vector<std::string> results;
  std::vector<std::string> resultTypes;
  std::string predicate;
};
struct IndexPlan {
  std::string operationName;
  std::vector<std::string> arguments;
  std::vector<std::string> results;
  std::vector<std::string> resultTypes;
  std::string predicate;
};
struct InlineCallPlan {
  std::string callee;
  std::vector<std::string> arguments;
  std::vector<std::string> results;
  std::vector<std::string> resultTypes;
};
struct InvokePlan {
  std::string callee;
  std::vector<std::string> arguments;
  std::vector<std::string> results;
  std::vector<std::string> resultTypes;
};
using ProcessOperationPlan =
    std::variant<ConstantPlan, ArithmeticPlan, IndexPlan, LiveLoadPlan,
                 LiveStorePlan, InlineCallPlan, InvokePlan>;

struct ContinuePlan {
  std::string targetPc;
};
struct SuspendPlan {
  std::string wakeValue;
  std::string targetPc;
};
struct TerminatePlan {
  std::string status;
};
struct BlockArgumentPlan {
  std::string name;
  std::string type;
};
struct BranchPlan {
  uint32_t targetBlock = 0;
  std::vector<std::string> arguments;
};
struct ConditionalBranchPlan {
  std::string condition;
  uint32_t trueBlock = 0;
  std::vector<std::string> trueArguments;
  uint32_t falseBlock = 0;
  std::vector<std::string> falseArguments;
};
using ProcessTerminatorPlan =
    std::variant<ContinuePlan, SuspendPlan, TerminatePlan>;
using BlockTerminatorPlan =
    std::variant<BranchPlan, ConditionalBranchPlan, ContinuePlan, SuspendPlan,
                 TerminatePlan>;

struct PcBlockPlan {
  uint32_t ordinal = 0;
  std::vector<BlockArgumentPlan> arguments;
  std::vector<ProcessOperationPlan> operations;
  BlockTerminatorPlan terminator = BranchPlan{};
};

struct PcStatePlan {
  uint32_t ordinal = 0;
  std::string name;
  std::vector<ProcessOperationPlan> operations;
  ProcessTerminatorPlan terminator = ContinuePlan{};
  std::vector<PcBlockPlan> blocks;
};

struct ProcessPlan {
  std::string symbol;
  std::string className;
  Fingerprint specializationFingerprint;
  std::string entryPc;
  uint64_t fairnessWork = 0;
  std::vector<CapturePlan> captures;
  std::vector<LiveSlotPlan> liveSlots;
  std::vector<PcStatePlan> states;
};

struct ModulePlan {
  std::string symbol;
  std::string className;
  Fingerprint specializationFingerprint;
  std::vector<PlacementPlan> placements;
  std::vector<ProjectionPlan> projections;
  std::vector<BindPlan> binds;
  std::vector<ExpressionPlan> expressions;
  std::vector<ProcessPlan> processes;
  std::vector<ExportPlan> exports;
  std::vector<std::string> returnValues;
};

struct ModelPlan {
  std::string modelSymbol;
  std::string rootSymbol;
  std::string contractEpoch;
  Fingerprint frozenAcirFingerprint;
  Fingerprint bindingLockFingerprint;
  Fingerprint providerFingerprint;
  Fingerprint profileFingerprint;
  Fingerprint toolchainFingerprint;
  Fingerprint schemaSetFingerprint;
  std::vector<std::string> constructionOrder;
  std::vector<std::string> destructionOrder;
  std::vector<TypePlan> types;
  std::vector<TimeDomainPlan> timeDomains;
  std::vector<BindingPlan> bindings;
  std::vector<ModulePlan> modules;
  std::vector<RuntimeObjectPlan> runtimeObjects;
  std::vector<ActivationEdgePlan> activationEdges;
  std::vector<SourceMapPlan> sourceMap;
};

llvm::Expected<ModelPlan> buildModelPlan(mlir::ModuleOp canonicalACSim);
llvm::Error validateModelPlan(const ModelPlan &plan);

} // namespace acir::codegen

#endif // ACIR_CODEGEN_MODELPLAN_H
