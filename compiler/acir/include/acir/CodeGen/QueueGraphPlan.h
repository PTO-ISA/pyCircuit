#ifndef ACIR_CODEGEN_QUEUEGRAPHPLAN_H
#define ACIR_CODEGEN_QUEUEGRAPHPLAN_H

#include "mlir/IR/BuiltinOps.h"
#include "acir/Dialect/ACIR/ACIRAttributes.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace acir::codegen {

inline constexpr uint64_t kMaximumPackedValueWidth = 1u << 16;

std::string legalizeQueueGraphIdentifier(llvm::StringRef value);

struct QueuePayloadFieldPlan {
  std::string name;
  std::string type;
  uint64_t width = 0;
};

struct QueuePayloadPlan {
  std::string name;
  std::vector<QueuePayloadFieldPlan> fields;
};

struct QueueEnumPlan {
  std::string name;
  std::vector<std::string> enumerants;
  std::vector<uint64_t> values;
  uint64_t width = 0;
};

struct QueueAggregatePlan {
  std::string type;
  std::string kind;
  std::vector<std::string> elements;
  uint64_t length = 0;
  uint64_t width = 0;
};

struct QueueSourceFramePlan {
  std::string kind;
  std::string file;
  uint64_t line = 0;
  uint64_t column = 0;
  std::string symbol;

  bool operator==(const QueueSourceFramePlan &) const = default;
};

using QueueSourceOriginPlan = std::vector<QueueSourceFramePlan>;

struct QueueSourceProvenancePlan {
  std::vector<QueueSourceOriginPlan> origins;

  bool operator==(const QueueSourceProvenancePlan &) const = default;
};

struct QueueExpressionPlan {
  std::string result;
  std::string kind;
  std::string type;
  std::vector<std::string> operands;
  std::string field;
  std::string predicate;
  std::string literal;
  std::string table;
  std::string slot;
  std::vector<QueueExpressionPlan> nestedExpressions;
  std::vector<std::string> nestedYields;
  uint64_t lsb = 0;
  uint64_t width = 0;
  std::string mask;
  std::string value;
  std::vector<uint64_t> domainAxes;
  std::vector<uint64_t> domainShape;
  std::vector<uint64_t> domainStrides;
  uint64_t domainOffset = 0;
  std::string domainBase;
  bool hasDomainProjection = false;
  QueueSourceProvenancePlan sourceProvenance;
  uint64_t selectionCount = 1;
  uint64_t laneOrdinal = 0;
  std::string keyOrdering;
  uint64_t initialCursor = 0;
  std::string staticTypeTarget;
  uint64_t indexWidth = 0;
};

std::string inlineTableChoiceContractKey(const QueueExpressionPlan &expression);
bool isEffectFreeTableMatchExpression(const QueueExpressionPlan &expression);

struct StateWritePlan {
  std::string table;
  std::string index;
  std::string value;
  std::string present;
  std::string mode;
  std::vector<std::string> fields;
  std::string versionedAction;
  std::string refGeneration;
  std::string refEpoch;
  std::string refAttempt;
  std::string staleObligationId;
};

struct OutputPresencePlan {
  uint64_t ordinal = 0;
  std::string value;
  std::string present;
};

struct StateReservationPlan {
  std::string table;
  std::string index;
  std::string source;
  std::string predicate;
  std::string indexKind;
  std::vector<std::string> fields;
};

struct QueuePayloadProjectionPlan {
  uint64_t version = 0;
  std::string profile;
  std::string logicalType;
  std::vector<std::string> keptFields;
  std::string carrierType;
  uint64_t logicalBits = 0;
  uint64_t carrierBits = 0;
  uint64_t removedBits = 0;

  bool operator==(const QueuePayloadProjectionPlan &) const = default;
};

struct QueuePlan {
  std::string name;
  std::string payloadType;
  std::string scope;
  uint64_t depth = 1;
  uint64_t latency = 1;
  uint64_t rate = 1;
  uint64_t lanes = 1;
  std::vector<uint64_t> laneOrdinals;
  std::optional<QueuePayloadProjectionPlan> payloadProjection;
};

struct QueueRuleResourcePlan {
  std::string kind;
  uint64_t ordinal = 0;
  std::string resource;

  bool operator==(const QueueRuleResourcePlan &) const = default;
};

struct QueueWriterArbitrationPlan {
  std::string owner;
  std::string endpointStableId;
  std::string policy;
  uint64_t declaredRank = 0;
  std::string resolution;

  bool operator==(const QueueWriterArbitrationPlan &) const = default;
};

struct SlotReleaseEffectPlan {
  std::string slot;
  std::string when;

  bool operator==(const SlotReleaseEffectPlan &) const = default;
};

struct QueueBlockPlan {
  std::string kind;
  std::string name;
  std::string scope;
  std::vector<std::string> inputs;
  std::vector<std::string> outputs;
  std::vector<uint64_t> depths;
  std::vector<uint64_t> latencies;
  std::string policy;
  uint64_t maxIterations = 0;
  std::string region;
  std::vector<QueueExpressionPlan> expressions;
  std::vector<std::string> yields;
  uint64_t capacity = 0;
  uint64_t start = 0;
  uint64_t noDependency = 0;
  uint64_t resources = 0;
  uint64_t credits = 0;
  uint64_t entries = 0;
  uint64_t init = 0;
  std::string resultField;
  std::string memoryInstance;
  std::string table;
  std::string tableIndex;
  std::string tableValue;
  std::string slot;
  std::string writeMode;
  uint64_t endpointOrdinal = 0;
  std::string message;
  std::vector<std::string> writeFields;
  uint64_t priority = 0;
  std::string guard;
  std::vector<StateWritePlan> stateWrites;
  std::vector<StateReservationPlan> stateReservations;
  std::vector<SlotReleaseEffectPlan> slotReleases;
  std::vector<OutputPresencePlan> outputPresence;
  std::vector<QueueRuleResourcePlan> activationSources;
  std::vector<QueueRuleResourcePlan> transactionResources;
  std::vector<QueueWriterArbitrationPlan> arbitrationMembership;
  bool hasActivationEvidence = false;
  bool initiallyActive = false;
  uint64_t lexicalOrder = 0;
  std::string provider;
  std::string stableId;
  std::string selection;
  uint64_t selectionCount = 0;
  std::string displayRuleName;
  std::vector<std::string> ndfIds;
  std::vector<std::string> ndfRequires;
  std::string sourceFile;
  uint64_t sourceLine = 0;
  uint64_t sourceColumn = 0;
  QueueSourceProvenancePlan sourceProvenance;
};

struct QueueHelperPlan {
  std::string name;
  std::vector<std::string> inputNames;
  std::vector<std::string> inputTypes;
  std::vector<std::string> resultTypes;
  std::vector<QueueExpressionPlan> expressions;
  std::vector<std::string> yields;
  QueueSourceProvenancePlan sourceProvenance;
};

struct MemoryInstancePlan {
  std::string name;
  std::string dataType;
  uint64_t entries = 0;
  uint64_t init = 0;
  uint64_t latency = 1;
  std::string stableId;
  std::string ownerPath;
  QueueSourceProvenancePlan sourceProvenance;
};

struct MemoryRequestPlan {
  std::string instance;
  std::string name;
  std::string scope;
  std::string input;
  std::string output;
  uint64_t ordinal = 0;
  uint64_t depth = 1;
  std::string resultField;
};

struct TableInitValuePlan {
  std::string kind;
  std::string type;
  std::string value;
  std::vector<std::string> fieldNames;
  std::vector<TableInitValuePlan> elements;

  bool operator==(const TableInitValuePlan &) const = default;
};

struct TablePlan {
  std::string name;
  std::string entryType;
  uint64_t entries = 0;
  uint64_t init = 0;
  std::string stableId;
  std::string ownerPath;
  std::vector<uint64_t> shape;
  std::vector<uint64_t> axisWidths;
  std::string layout;
  uint64_t layoutVersion = 0;
  uint64_t initVersion = 0;
  std::vector<TableInitValuePlan> initImage;
  bool hasTypedSchema = false;
  QueueSourceProvenancePlan sourceProvenance;
  bool versioned = false;
  std::string recoveryDomain;
  std::string identity;
  std::string checkpoint;
  std::string retainedResult;
  uint64_t generationBits = 0;
  uint64_t epochBits = 0;
  uint64_t attemptBits = 0;
  std::string validField;
  std::string generationField;
  std::string epochField;
  std::string attemptField;
  std::string payloadField;
};

struct TableMatchPlan {
  std::string name;
  std::string table;
  std::string scope;
  std::string resultType;
  std::vector<QueueExpressionPlan> expressions;
  std::string yield;
  std::vector<uint64_t> domainAxes;
  std::vector<uint64_t> domainShape;
  std::vector<uint64_t> domainStrides;
  uint64_t domainOffset = 0;
  std::string domainBase;
  bool hasDomainProjection = false;
  QueueSourceProvenancePlan sourceProvenance;
};

struct TableSelectionPlan {
  std::string name;
  std::string table;
  std::string scope;
  std::string match;
  std::string policy;
  std::string indexType;
  std::vector<QueueExpressionPlan> keyExpressions;
  std::string keyYield;
  uint64_t count = 1;
  std::string keyOrdering;
  std::string stableId;
  uint64_t initialCursor = 0;
  QueueSourceProvenancePlan sourceProvenance;
};

struct TableReadPlan {
  std::string table;
  std::string name;
  std::string scope;
  std::string input;
  std::string output;
  uint64_t depth = 1;
  uint64_t latency = 1;
};

struct TableWritePlan {
  std::string table;
  std::string name;
  std::string scope;
  std::string input;
  std::string mode;
  std::vector<std::string> writeFields;
};

struct TableMaskedWritePlan {
  std::string table;
  std::string name;
  std::string scope;
  std::string mode;
  std::vector<std::string> writeFields;
};

struct SlotPlan {
  std::string name;
  std::string payloadType;
  std::string input;
  std::string scope;
  std::string stableId;
  std::string ownerPath;
  QueueSourceProvenancePlan sourceProvenance;
};

struct QueueInterfacePlan {
  std::string name;
  std::string payloadType;
  uint64_t lanes = 1;
  uint64_t rate = 1;
  std::string displayName;
};

struct QueueModuleInstancePlan {
  std::string name;
  std::string definition;
  acir::ac::StaticArgumentsAttr staticArguments;
  std::string scope;
  std::vector<std::string> inputs;
  std::vector<std::string> outputs;
  uint64_t lexicalOrder = 0;
  QueueSourceProvenancePlan sourceProvenance;
};

struct QueueArchitectureObligationPlan {
  std::string module;
  std::string symbol;
  std::string id;
  std::string kind;
  std::string severity;
  std::string status;
  std::string firing;
  std::string conditionRule;
  std::string conditionTable;
  uint64_t conditionRoot = 0;
  std::string activeRule;
  std::optional<uint64_t> activeRoot;
  std::string disableRule;
  std::optional<uint64_t> disableRoot;
  std::string samplingKind;
  std::string samplingEdge;
  std::string sampleAnchor;
  bool monitorOnly = false;
  std::optional<uint64_t> captureLatency;
  uint64_t inputOrdinal = 0;
  uint64_t maximum = 0;
  std::vector<std::string> targets;
  std::string message;
  std::vector<std::string> sourceRules;
  std::vector<std::string> stateOwners;
  std::vector<std::string> ndfIds;
  std::string proofCertificate;
  std::vector<std::string> materializations;
  std::string sampling;
  QueueSourceProvenancePlan sourceProvenance;

  bool operator==(const QueueArchitectureObligationPlan &) const = default;
};

struct QueueProvedObligationElisionPlan {
  std::string module;
  std::string id;
  std::string kind;
  std::string reason;
  std::string leftEndpoint;
  std::string rightEndpoint;
  std::string ownerPath;
  std::string ownerStableId;
  std::string sourceProvenance;
  uint64_t propertyRoot = 0;

  bool operator==(const QueueProvedObligationElisionPlan &) const = default;
};

llvm::Expected<QueueProvedObligationElisionPlan>
buildProvedObligationElision(mlir::Operation *obligation,
                             llvm::StringRef module);

struct QueueArchitectureExpressionNodePlan {
  std::string opcode;
  std::string type;
  std::vector<uint64_t> operands;
  std::string operation;
  std::string predicate;
  uint64_t inputOrdinal = 0;
  uint64_t literal = 0;
  bool hasInputOrdinal = false;
  bool hasLiteral = false;
  std::string attributes;
};

struct QueueArchitectureExpressionScopePlan {
  std::string rule;
  std::string ownerRule;
  std::vector<QueueArchitectureExpressionNodePlan> nodes;
};

enum class QueueActivationNodeKind {
  InterfaceInput,
  InterfaceOutput,
  Queue,
  Block,
  Table,
  Slot,
};

struct QueueActivationNodePlan {
  QueueActivationNodeKind kind = QueueActivationNodeKind::Queue;
  uint64_t index = 0;

  bool operator==(const QueueActivationNodePlan &) const = default;
};

struct QueueActivationEdgePlan {
  QueueActivationNodePlan source;
  QueueActivationNodePlan target;

  bool operator==(const QueueActivationEdgePlan &) const = default;
};

struct QueueGraphPlan;

struct ModuleCasePlan {
  acir::ac::StaticArgumentsAttr arguments;
  mlir::FunctionType concreteSignature;
  acir::ac::SourceProvenanceAttr sourceProvenance;
  acir::ac::ModuleInterfaceAttr materializedInterface;
  std::vector<std::string> queues;
  std::vector<std::string> tables;
  std::vector<std::string> slots;
  std::vector<std::string> rules;
  std::vector<std::string> proofs;
  std::vector<std::string> obligations;
  std::vector<std::string> stateOwners;
  std::shared_ptr<QueueGraphPlan> bodyPlan;
};

struct NominalDefinitionPlan {
  enum class Kind { Enum, Struct };
  Kind kind = Kind::Enum;
  std::string scope;
  std::string name;
  mlir::Attribute scopeLayout;
  acir::ac::StaticParametersAttr parameters;
  mlir::ArrayAttr members;
  mlir::ArrayAttr values;
  mlir::IntegerAttr encodingWidth;
};

struct ModuleFamilyPlan {
  std::string definition;
  acir::ac::SourceOwnerAttr source;
  acir::ac::StaticParametersAttr parameters;
  acir::ac::StaticCasesAttr declaredCases;
  acir::ac::ModuleInterfaceAttr interface;
  mlir::ArrayAttr nominalDeclarations;
  std::vector<NominalDefinitionPlan> nominalDefinitions;
  std::vector<ModuleCasePlan> cases;
};

struct QueueGraphPlan {
  std::string system;
  std::string definition;
  std::string sourceDefinition;
  std::vector<std::string> ndfIds;
  std::vector<std::string> ndfRequires;
  std::string sourceFile;
  uint64_t sourceLine = 0;
  uint64_t sourceColumn = 0;
  std::vector<QueueInterfacePlan> interfaceInputs;
  std::vector<QueueInterfacePlan> interfaceOutputs;
  std::vector<QueueModuleInstancePlan> moduleInstances;
  std::vector<ModuleFamilyPlan> moduleFamilies;
  std::vector<QueueArchitectureExpressionScopePlan>
      architectureExpressionScopes;
  std::vector<QueueArchitectureObligationPlan> architectureObligations;
  std::vector<QueueProvedObligationElisionPlan> provedObligationElisions;
  std::vector<QueueActivationEdgePlan> activationEdges;
  std::vector<QueueActivationEdgePlan> workClosureEdges;
  std::vector<QueueActivationNodePlan> initialActivation;
  std::vector<QueuePayloadPlan> payloads;
  std::vector<QueueEnumPlan> enums;
  std::vector<QueueAggregatePlan> aggregates;
  std::vector<QueueHelperPlan> helpers;
  std::vector<std::string> scopes;
  std::vector<QueuePlan> queues;
  std::vector<QueueBlockPlan> blocks;
  std::vector<MemoryInstancePlan> memoryInstances;
  std::vector<MemoryRequestPlan> memoryRequests;
  std::vector<TablePlan> tables;
  std::vector<TableMatchPlan> tableMatches;
  std::vector<TableSelectionPlan> tableSelections;
  std::vector<TableReadPlan> tableReads;
  std::vector<TableWritePlan> tableWrites;
  std::vector<TableMaskedWritePlan> tableMaskedWrites;
  std::vector<SlotPlan> slots;

  llvm::Expected<std::string> canonicalJson() const;
  llvm::Expected<std::string> sourceMapJson() const;
  llvm::Expected<std::string> moduleManifestJson() const;
};

llvm::Expected<QueueGraphPlan> buildQueueGraphPlan(mlir::ModuleOp module);
llvm::Error resolveQueueWriterPriorities(QueueGraphPlan &plan);
llvm::Error verifyQueueGraphPlan(const QueueGraphPlan &plan);

} // namespace acir::codegen

#endif // ACIR_CODEGEN_QUEUEGRAPHPLAN_H
