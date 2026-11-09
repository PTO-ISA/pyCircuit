#ifndef PYCIRCUIT_VERILOG_EMISSION_PLAN_H
#define PYCIRCUIT_VERILOG_EMISSION_PLAN_H

#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"

#include <optional>
#include <string>

namespace acir::compiler {

class HardwareEmitContext;

// Concrete, Verilog-facing structural data. Semantic resolution belongs in
// this plan builder; the renderer retains only SSA net selection and layout.
enum class VerilogValueSource {
  Operand,
  Result,
};

struct VerilogDataPortPlan {
  std::string name;
  VerilogValueSource source;
  unsigned ordinal = 0;
  std::string collectionWidth;
};

enum class VerilogPortDirection {
  Input,
  Output,
};

struct VerilogModulePortPlan {
  VerilogPortDirection direction;
  std::string type;
  std::string name;
};

struct VerilogPortConnectionPlan {
  std::string name;
  std::string expression;
};

struct VerilogOutputAssignmentPlan {
  std::string name;
  mlir::Value value;
};

enum class VerilogCombinationalKind {
  Constant,
  BitsConstant4,
  Unary,
  Binary,
  Compare,
  Select,
  Concat,
  Extract,
  Resize,
  EnumCreate,
  EnumToBits,
  EnumFromBits,
  StructCreate,
  StructGet,
};

struct VerilogCombinationalPlan {
  mlir::Operation *operation = nullptr;
  VerilogCombinationalKind kind;
  llvm::SmallVector<std::string> resultTypes;
  std::string opcode;
  std::string staticValue;
  llvm::SmallVector<std::string> staticValues;
  bool signedOperands = false;
  bool alwaysComb = false;
};

enum class VerilogTableKind {
  Create,
  Splat,
  View,
  Index,
  Get,
  Map,
  Match,
  Choose,
  Fold,
  ValueMerge,
};

enum class VerilogRegionArgumentKind {
  Ordinal,
  TableElement,
  Capture,
};

struct VerilogRegionArgumentPlan {
  mlir::Value argument;
  VerilogRegionArgumentKind kind;
  unsigned operand = 0;
  std::string type;
  std::string elementCount;
  std::string elementWidth;
};

struct VerilogRegionYieldPlan {
  mlir::Value value;
  unsigned result = 0;
  bool tableSlice = false;
  std::string elementCount;
  std::string elementWidth;
};

struct VerilogMergeWritePlan {
  std::string fieldSuffix;
  unsigned guardOperand = 0;
  unsigned valueOperand = 0;
};

struct VerilogTablePlan {
  mlir::Operation *operation = nullptr;
  VerilogTableKind kind;
  llvm::SmallVector<std::string> resultTypes;
  std::string elementCount;
  std::string elementWidth;
  std::string sourceElementCount;
  std::string sourceElementWidth;
  std::string mappedIndex;
  std::string targetWidth;
  llvm::SmallVector<std::string> shape;
  llvm::SmallVector<std::string> operandWidths;
  llvm::SmallVector<VerilogRegionArgumentPlan> regionArguments;
  llvm::SmallVector<VerilogRegionYieldPlan> regionYields;
  llvm::SmallVector<VerilogMergeWritePlan> mergeWrites;
  llvm::SmallVector<std::string> mergePassthroughs;
  llvm::SmallVector<unsigned> mergeEnableOperands;
  std::string mode;
  std::string opcode;
  int64_t count = 0;
  bool conditionalFold = false;
};

struct VerilogParameterPlan {
  std::optional<std::string> name;
  std::string value;
  bool managed = false;
};

enum class VerilogManagedSignal {
  Phase,
  RootCommitOk,
  ResetActive,
  CheckComplete,
  SamplingError,
  LocalError,
};

struct VerilogManagedPortPlan {
  std::string name;
  VerilogManagedSignal signal;
  bool laneIndexed = false;
};

enum class VerilogLifecycleAggregate {
  Error,
  Completion,
  SamplingError,
};

enum class VerilogLifecycleReduction {
  Direct,
  Any,
  All,
};

struct VerilogLifecycleWirePlan {
  VerilogManagedSignal signal;
  std::string suffix;
  bool vector = false;
  VerilogLifecycleAggregate aggregate;
  VerilogLifecycleReduction reduction;
};

struct VerilogOccurrencePlan {
  mlir::Operation *occurrence = nullptr;
  bool collection = false;
  bool primitive = false;
  bool managedPrimitive = false;
  std::string moduleName;
  std::string instanceName;
  llvm::SmallVector<VerilogParameterPlan> parameters;
  std::string collectionCount;
  llvm::SmallVector<VerilogDataPortPlan> dataPorts;
  llvm::SmallVector<VerilogManagedPortPlan> managedPorts;
  llvm::SmallVector<VerilogLifecycleWirePlan> lifecycleWires;
};

struct VerilogQueuePlan {
  mlir::Operation *occurrence = nullptr;
  std::string instanceName;
  std::string payloadType;
  std::string depth;
  std::string availabilityLatency;
  unsigned readyPolicy = 0;
  bool managedParameter = false;
  llvm::SmallVector<VerilogDataPortPlan> dataPorts;
  llvm::SmallVector<VerilogManagedPortPlan> managedPorts;
  llvm::SmallVector<VerilogLifecycleWirePlan> lifecycleWires;
};

struct VerilogModuleEmissionPlan {
  llvm::SmallVector<std::string> parameters;
  llvm::SmallVector<VerilogModulePortPlan> ports;
  llvm::SmallVector<VerilogOutputAssignmentPlan> outputAssignments;
  llvm::DenseMap<mlir::Operation *, VerilogCombinationalPlan> combinational;
  llvm::DenseMap<mlir::Operation *, VerilogTablePlan> tables;
  llvm::DenseMap<mlir::Operation *, VerilogOccurrencePlan> occurrences;
  llvm::DenseMap<mlir::Operation *, VerilogQueuePlan> queues;
};

struct VerilogWrapperEmissionPlan {
  std::string calleeParameters;
  llvm::SmallVector<VerilogModulePortPlan> ports;
  llvm::SmallVector<VerilogPortConnectionPlan> connections;
};

mlir::FailureOr<VerilogModuleEmissionPlan>
buildVerilogModuleEmissionPlan(HardwareEmitContext &context,
                               ac::ModuleOp definition);

mlir::FailureOr<VerilogWrapperEmissionPlan>
buildVerilogWrapperEmissionPlan(HardwareEmitContext &context, ac::ModuleOp root,
                                llvm::StringRef callee);

} // namespace acir::compiler

#endif
