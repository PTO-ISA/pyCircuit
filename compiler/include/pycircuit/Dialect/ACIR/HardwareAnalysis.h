#ifndef PYCIRCUIT_DIALECT_ACIR_HARDWAREANALYSIS_H
#define PYCIRCUIT_DIALECT_ACIR_HARDWAREANALYSIS_H

#include "mlir/IR/BuiltinOps.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include <cstdint>
#include <optional>
#include <utility>

namespace acir::ac {

/// Compare hardware payload types without source origin/location provenance.
/// Nominal struct/enum and type-parameter owner identities remain significant.
bool areEquivalentHardwareTypes(mlir::Type lhs, mlir::Type rhs);

/// Source physical prefix and final-only compiler-generated port suffix.
/// Counts include original hidden domain ports. Ordinary instances always bind
/// the full signature; defaults apply only to the selected System occurrence.
struct ModulePortLayout {
  unsigned sourceInputCount;
  unsigned sourceOutputCount;
  mlir::ArrayAttr rootInputDefaults;
  /// Source-port ordinal -> physical Bits1 companion, or -1 when total.
  llvm::SmallVector<int64_t> inputValidity;
  llvm::SmallVector<int64_t> outputValidity;
  /// Private lexical payload export -> physical availability companion.
  llvm::SmallVector<std::pair<int64_t, int64_t>> routedOutputValidity;
};
/// Validate the closed ac.internal_ports contract. Without it, every physical
/// port belongs to the source boundary and rootInputDefaults is an empty array.
mlir::FailureOr<ModulePortLayout>
getModulePortLayout(mlir::Operation *declaration);

struct EnumMember {
  mlir::StringAttr name;
  MathIntAttr code;
};
/// Validated declaration data shared by analysis and both emitters.
struct EnumDefinitionView {
  uint64_t width;
  mlir::StringAttr encoding;
  llvm::SmallVector<EnumMember> members;
};

enum class QueueReadyPolicy { LocalOccupancy, DownstreamPop };
/// Recognize the canonical FIFO allocation; schema and bindings are verified
/// by the same instance and primitive checks used for other native leaves.
bool isQueueInstance(InstanceOp instance);
/// Derive the existing FIFO configuration from its canonical callee and
/// ordinary static arguments. No duplicated configuration is stored on the IR.
mlir::FailureOr<mlir::DictionaryAttr>
getQueueConfiguration(InstanceOp instance);
/// The same derivation for authenticated declaration-only instance rows.
mlir::FailureOr<mlir::DictionaryAttr>
getQueueConfiguration(mlir::FlatSymbolRefAttr callee,
                      mlir::ArrayAttr parameters,
                      mlir::Operation *diagnosticSite);
struct ResolvedQueueConfig {
  mlir::Type payloadType;
  mlir::Type tokenElementType;
  llvm::SmallVector<uint64_t> tokenShape;
  uint64_t tokenCardinality;
  uint64_t packedWidth;
  uint64_t depth;
  unsigned pointerWidth;
  unsigned countWidth;
  QueueReadyPolicy readyPolicy;
  uint64_t availabilityLatency;
  uint64_t headReadLatency;
  bool emptyFlow;
  mlir::StringAttr readDuringWrite;
  mlir::StringAttr resetPolicy;
  mlir::StringAttr emptyData;
  uint64_t storageBitCount;
  // Zero for latency one or an unresolved latency. Otherwise bit_width(L - 1).
  unsigned timestampWidth;
  // D*K + K + pointerWidth + countWidth, separate from payload storage.
  // Zero for latency one or an unresolved depth/latency; checked before use.
  uint64_t timingStorageBitCount;
};

/// Bindings describe an occurrence of an existing definition, not cloned IR.
struct HardwareBindings {
  mlir::Operation *owner = nullptr;
  llvm::StringMap<mlir::Attribute> integers;
  llvm::StringMap<mlir::Type> types;
};

struct HardwareCheckSamplingBinding {
  ModuleOp definition;
  mlir::Value clock, reset;
  uint64_t localOrdinal = 0;
};
struct HardwareCheckSamplingOccurrence {
  uint64_t bindingIndex = 0, ownerOccurrence = 0, ordinal = 0;
};

/// Definition-local obligations and their actual linked owner occurrences.
/// These views retain original IR; they must be revalidated after mutations.
struct HardwareCheckBinding {
  ModuleOp definition;
  RuleOp rule;
  SourceExpectOp expect;
  mlir::DictionaryAttr checkID;
  mlir::StringAttr kind;
  mlir::DictionaryAttr location;
  mlir::StringAttr message;
  mlir::Value condition, path;
  uint64_t requiredIndex = 0, conditionResult = 0, pathResult = 0;
  std::optional<uint64_t> samplingBindingIndex;
};
struct HardwareCheckOccurrenceStep {
  mlir::Operation *allocation = nullptr;
  llvm::SmallVector<uint64_t> coordinates;
};
struct HardwareCheckOwnerOccurrence {
  ModuleOp definition;
  llvm::SmallVector<HardwareCheckOccurrenceStep> path;
  HardwareBindings bindings;
  std::optional<uint64_t> parent;
  uint64_t ordinal = 0;
};
enum class HardwareCheckResetKind { NoPhysicalReset, PhysicalReset };
struct HardwareCheckResetContext {
  HardwareCheckResetKind kind = HardwareCheckResetKind::NoPhysicalReset;
  std::optional<uint64_t> ownerOccurrence;
  std::optional<unsigned> inputOrdinal;
};
struct HardwareResolvedSourceCheck {
  uint64_t bindingIndex = 0, ownerOccurrence = 0;
  HardwareCheckResetContext reset;
  uint64_t ordinal = 0;
  std::optional<uint64_t> samplingOccurrenceIndex;
};
struct HardwareCheckCommitEndpoint {
  uint64_t ownerOccurrence = 0;
  mlir::Operation *allocation = nullptr;
  llvm::SmallVector<uint64_t> coordinates;
  mlir::StringAttr primitiveKind;
};
struct HardwareSourceCheckLimits {
  uint64_t maxWorkUnits = 4 * 1024 * 1024;
};
struct HardwareSourceCheckPlan {
  llvm::SmallVector<HardwareCheckBinding> bindings;
  llvm::SmallVector<HardwareCheckOwnerOccurrence> owners;
  llvm::SmallVector<HardwareResolvedSourceCheck> checks;
  llvm::SmallVector<HardwareCheckCommitEndpoint> commits;
  llvm::SmallVector<HardwareCheckSamplingBinding> samplingBindings;
  llvm::SmallVector<HardwareCheckSamplingOccurrence> samplingOccurrences;
  bool hasChecks() const { return !checks.empty(); }
};
/// Read-only physical-edge admission over the authenticated check inventory.
struct HardwarePhysicalProjection {
  ModuleOp root;
  unsigned clockInput = 0, resetInput = 0;
};
using FieldPath = llvm::SmallVector<mlir::StringAttr>;
struct PackedLeaf {
  FieldPath path;
  mlir::Type type;
  uint64_t low = 0;
  uint64_t width = 0;
};
struct PortEndpoint {
  unsigned port = 0;
  FieldPath path;
  bool operator==(const PortEndpoint &other) const {
    return port == other.port && path == other.path;
  }
};
struct OutputDependency {
  PortEndpoint output;
  llvm::SmallVector<PortEndpoint> inputs;
};

/// Canonical source-interface endpoint serialization shared with revalidation.
mlir::ArrayAttr
serializeOutputDependencies(mlir::MLIRContext *context,
                            llvm::ArrayRef<OutputDependency> dependencies);
struct WireEndpoint {
  mlir::Value value;
  FieldPath path;
  bool operator==(const WireEndpoint &other) const {
    return value == other.value && path == other.path;
  }
};
struct WireStep {
  WireEndpoint output;
  mlir::Operation *operation = nullptr;
};
struct ModuleAnalysis {
  llvm::SmallVector<WireStep> wireSchedule;
  llvm::SmallVector<mlir::Operation *> storageWork;
  /// Existing operations, in dependency order. Storage output reads are cuts;
  /// their sink input connections must be populated after combinational Work.
  llvm::SmallVector<mlir::Operation *> schedule;
  llvm::SmallVector<OutputDependency> dependencies;
};

/// All facts refer to the one verified module package and its original SSA.
class HardwareAnalysis {
public:
  explicit HardwareAnalysis(mlir::ModuleOp package);
  explicit HardwareAnalysis(mlir::Operation *operation)
      : HardwareAnalysis(mlir::cast<mlir::ModuleOp>(operation)) {}
  mlir::ModuleOp getPackage() const { return package; }
  llvm::SmallVector<ModuleOp> getDefinitions() const;
  llvm::SmallVector<StructOp> getStructs() const;
  mlir::Operation *lookupDefinition(llvm::StringRef name) const;
  StructOp lookupStruct(StructType type) const;
  EnumOp lookupEnum(EnumType type) const;
  mlir::FailureOr<EnumDefinitionView> resolveEnum(EnumType type,
                                                  mlir::Operation *site) const;
  mlir::Operation *resolveCallee(InstanceOp instance) const;
  mlir::Operation *resolveCallee(CollectionOp collection) const;
  mlir::FailureOr<llvm::SmallVector<uint64_t>>
  resolveShape(mlir::ArrayAttr shape, const HardwareBindings &bindings,
               mlir::Operation *site) const;
  mlir::FailureOr<llvm::SmallVector<uint64_t>>
  getTableShape(TableType table, const HardwareBindings &bindings,
                mlir::Operation *site) const;
  mlir::FailureOr<uint64_t> getTableSize(TableType table,
                                         const HardwareBindings &bindings,
                                         mlir::Operation *site) const;
  mlir::FailureOr<uint64_t>
  getViewSourceOrdinal(TableViewOp view, uint64_t ordinal,
                       const HardwareBindings &bindings = {}) const;
  llvm::StringRef getPrimitiveKind(mlir::Operation *definition) const;
  mlir::FailureOr<mlir::Attribute>
  evaluateStatic(StaticExprAttr expression, const HardwareBindings &bindings,
                 mlir::Operation *site) const;
  bool isStaticEvaluable(mlir::Attribute expression,
                         const HardwareBindings &bindings) const;
  mlir::FailureOr<mlir::Type> resolveType(mlir::Type type,
                                          const HardwareBindings &bindings,
                                          mlir::Operation *site) const;
  mlir::FailureOr<uint64_t> getPackedWidth(mlir::Type type,
                                           const HardwareBindings &bindings,
                                           mlir::Operation *site) const;
  /// Shape-only paths retain opaque type formals and symbolic bit widths.
  mlir::FailureOr<llvm::SmallVector<FieldPath>>
  getFieldPaths(mlir::Type type, const HardwareBindings &bindings,
                mlir::Operation *site) const;
  mlir::FailureOr<llvm::SmallVector<OutputDependency>>
  getImportDependencies(ModuleImportOp imported,
                        const HardwareBindings &bindings = {}) const;
  mlir::FailureOr<llvm::SmallVector<PackedLeaf>>
  getLeaves(mlir::Type type, const HardwareBindings &bindings,
            mlir::Operation *site) const;
  mlir::FailureOr<HardwareBindings>
  bindInstance(InstanceOp instance, const HardwareBindings &parent = {}) const;
  mlir::FailureOr<HardwareBindings>
  bindInstance(CollectionOp collection,
               const HardwareBindings &parent = {}) const;
  mlir::FailureOr<HardwareBindings>
  bindCallee(mlir::Operation *callee, mlir::ArrayAttr actualParameters,
             mlir::ArrayAttr actualTypeArguments,
             const HardwareBindings &parent,
             mlir::Operation *diagnosticSite) const;
  mlir::FailureOr<ModuleAnalysis>
  analyzeModule(ModuleOp definition,
                const HardwareBindings &bindings = {}) const;
  mlir::FailureOr<llvm::SmallVector<WireEndpoint>>
  getDependencies(mlir::Value value, llvm::ArrayRef<mlir::StringAttr> path,
                  const HardwareBindings &bindings = {}) const;
  mlir::LogicalResult
  verifyResolvedOperation(mlir::Operation *operation,
                          const HardwareBindings &bindings) const;
  mlir::LogicalResult verifyImport(ModuleImportOp imported) const;
  mlir::LogicalResult
  verifyEnumValueOperation(mlir::Operation *operation,
                           const HardwareBindings &bindings,
                           bool requireResolved = false) const;
  mlir::LogicalResult verifyQueueOperation(InstanceOp operation,
                                           const HardwareBindings &bindings,
                                           bool requireResolved = false) const;
  mlir::FailureOr<ResolvedQueueConfig>
  resolveQueue(InstanceOp operation, const HardwareBindings &bindings) const;
  mlir::LogicalResult verifyInstance(InstanceOp instance,
                                     const HardwareBindings &parent = {}) const;
  mlir::LogicalResult verifyInstance(CollectionOp collection,
                                     const HardwareBindings &parent = {}) const;
  mlir::LogicalResult
  verifyCollectionOperation(mlir::Operation *operation,
                            const HardwareBindings &bindings,
                            bool requireResolved = false) const;
  /// Normalize the complete writer tuples together before building merge IR.
  mlir::LogicalResult normalizeMergeOperands(
      mlir::Type baseType, llvm::SmallVectorImpl<mlir::ArrayAttr> &paths,
      llvm::SmallVectorImpl<mlir::Value> &guards,
      llvm::SmallVectorImpl<mlir::Value> &values,
      const HardwareBindings &bindings, mlir::Operation *site) const;
  /// Direct occurrence subtrees whose inputs can be prepared without another
  /// occurrence's Work. Results identify existing IR; no cloned task graph.
  mlir::FailureOr<llvm::SmallVector<mlir::Operation *>>
  getIndependentWorkSubtrees(ModuleOp definition) const;
  mlir::FailureOr<HardwareSourceCheckPlan>
  getSourceCheckPlan(HardwareSourceCheckLimits limits = {}) const;
  mlir::LogicalResult
  verifySourceCheckPlan(const HardwareSourceCheckPlan &plan,
                        HardwareSourceCheckLimits limits = {}) const;
  mlir::FailureOr<HardwarePhysicalProjection>
  getPhysicalProjection(const HardwareSourceCheckPlan &plan,
                        HardwareSourceCheckLimits limits = {}) const;
  mlir::LogicalResult verifyDefinitionSourceChecks(ModuleOp definition) const;
  mlir::LogicalResult verify() const;

private:
  mlir::FailureOr<HardwareBindings>
  bindOccurrence(mlir::Operation *operation,
                 const HardwareBindings &parent) const;
  mlir::LogicalResult verifyOccurrence(mlir::Operation *operation,
                                       const HardwareBindings &parent) const;
  mutable mlir::ModuleOp package;
  llvm::StringMap<mlir::Operation *> symbols;
  llvm::SmallVector<ModuleOp> definitions;
  llvm::SmallVector<StructOp> structures;
  mutable llvm::DenseMap<mlir::Type, llvm::SmallVector<PackedLeaf>> layouts;
  mutable llvm::SmallVector<std::pair<mlir::Attribute, mlir::Operation *>>
      activeStatic;
};

/// Verify common hardware shape, binding, dependency and execution contracts.
mlir::LogicalResult verifyHardwarePackage(mlir::ModuleOp package);

} // namespace acir::ac
#endif
