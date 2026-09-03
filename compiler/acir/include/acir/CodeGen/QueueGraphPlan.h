#ifndef ACIR_CODEGEN_QUEUEGRAPHPLAN_H
#define ACIR_CODEGEN_QUEUEGRAPHPLAN_H

#include "mlir/IR/BuiltinOps.h"
#include "llvm/Support/Error.h"

#include <cstdint>
#include <string>
#include <vector>

namespace acir::codegen {

struct QueuePayloadFieldPlan {
  std::string name;
  std::string type;
};

struct QueuePayloadPlan {
  std::string name;
  std::vector<QueuePayloadFieldPlan> fields;
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
};

struct QueuePlan {
  std::string name;
  std::string payloadType;
  std::string scope;
  uint64_t depth = 1;
  uint64_t latency = 1;
  uint64_t rate = 1;
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
  std::string slot;
  std::string writeMode;
  uint64_t endpointOrdinal = 0;
  std::string message;
  std::vector<std::string> writeFields;
};

struct MemoryInstancePlan {
  std::string name;
  std::string dataType;
  uint64_t entries = 0;
  uint64_t init = 0;
  uint64_t latency = 1;
  std::string stableId;
  std::string ownerPath;
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

struct TablePlan {
  std::string name;
  std::string entryType;
  uint64_t entries = 0;
  uint64_t init = 0;
  std::string stableId;
  std::string ownerPath;
};

struct TableMatchPlan {
  std::string name;
  std::string table;
  std::string scope;
  std::string resultType;
  std::vector<QueueExpressionPlan> expressions;
  std::string yield;
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
};

struct QueueGraphPlan {
  std::string system;
  std::string specializationFingerprint;
  std::vector<QueuePayloadPlan> payloads;
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
};

llvm::Expected<QueueGraphPlan> buildQueueGraphPlan(mlir::ModuleOp module);
llvm::Error verifyQueueGraphPlan(const QueueGraphPlan &plan);

} // namespace acir::codegen

#endif // ACIR_CODEGEN_QUEUEGRAPHPLAN_H
