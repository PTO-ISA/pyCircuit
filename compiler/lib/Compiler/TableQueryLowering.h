#ifndef ACIR_LIB_COMPILER_TABLEQUERYLOWERING_H
#define ACIR_LIB_COMPILER_TABLEQUERYLOWERING_H

#include "mlir/IR/Builders.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/SmallVector.h"
#include <array>
#include <functional>
#include <string>

namespace acir::compiler::detail {

enum class TableQueryResource : unsigned {
  Nesting,
  Occurrences,
  Work,
  Operations,
  Slots,
  PayloadWords,
  ConstantBytes,
  Count
};
struct TableQueryCharge {
  std::array<uint64_t, static_cast<unsigned>(TableQueryResource::Count)>
      amounts{};
};
// Private source-invocation ledger. A reservation is atomic across all
// counters.
class TableQueryBudget {
public:
  static uint64_t limit(TableQueryResource resource);
  uint64_t used(TableQueryResource resource) const;
  bool reserve(const TableQueryCharge &charge, std::string &reason);
  bool reserveProduct(TableQueryResource resource, uint64_t a, uint64_t b,
                      std::string &reason);
  bool enter(std::string &reason);
  void leave();
  static bool add(TableQueryCharge &charge, TableQueryResource resource,
                  uint64_t amount);
  static bool addProduct(TableQueryCharge &charge, TableQueryResource resource,
                         uint64_t a, uint64_t b);

private:
  TableQueryCharge spent;
};

// Private non-debiting storage measurement shared by importer envelopes. The
// optional visit hook preserves the existing Table-query type-walk charges.
mlir::FailureOr<uint64_t> measureTableStorageWords(
    mlir::Type type, ac::HardwareAnalysis &analysis, mlir::ModuleOp package,
    mlir::Location location, unsigned depth = 0,
    const std::function<mlir::LogicalResult()> &visit = {});

struct TableQueryHooks {
  std::function<ac::StaticExprAttr(uint64_t)> literal;
  std::function<mlir::Type(uint64_t)> bits;
  std::function<mlir::FailureOr<mlir::Value>(mlir::Value, mlir::Value,
                                             mlir::Value, mlir::OpBuilder &)>
      select;
  std::function<mlir::Value(mlir::Value, mlir::Type, mlir::OpBuilder &)>
      identity;
  std::function<void(mlir::Value, mlir::Value)> copyFacts;
  std::function<void(mlir::Value)> forgetFacts;
};

// Logical bounds prepaid by the named fixed callback's first reservation.
// Observation is integrated with the existing stage/clone loops, not a replay.
struct TableQueryStageEnvelope {
  uint64_t scalarOperations, rowArguments, mapArguments, factValues;
  uint64_t mappingSlots;
  uint64_t hoists = 0, pending = 0, inputs = 0, captures = 0, arguments = 0;
  uint64_t observedValues = 0, mappingEntries = 0;
};

// Consumes already typed SSA. The owning importer is the only syntax lowerer.
class TableQueryLowering {
public:
  TableQueryLowering(mlir::OpBuilder &builder, mlir::Location location,
                     mlir::ModuleOp package, TableQueryBudget &budget,
                     TableQueryHooks hooks);
  mlir::LogicalResult capture(mlir::Value value);
  mlir::FailureOr<llvm::SmallVector<mlir::Value>>
  stage(mlir::Block &scalar, mlir::Value row, mlir::Value receiver,
        mlir::ValueRange outputs);
  mlir::FailureOr<llvm::SmallVector<mlir::Value>>
  stage(mlir::Block &scalar, mlir::ValueRange rows, mlir::ValueRange receivers,
        mlir::ValueRange outputs, bool mapAggregates = false,
        TableQueryStageEnvelope *envelope = nullptr);
  mlir::FailureOr<mlir::Value> fold(mlir::Value table, llvm::StringRef kind);
  mlir::FailureOr<mlir::Value> count(mlir::Value table, mlir::Type result);
  mlir::FailureOr<llvm::SmallVector<mlir::Value>> choose(mlir::Value predicates,
                                                         mlir::Value keys = {});
  // Describe already reserved scalar storage without spending the ledger a
  // second time. The importer owns the reservation and consistency audit.
  bool describeWithoutReservation(mlir::Operation *operation, uint64_t rows,
                                  TableQueryCharge &charge);

private:
  mlir::FailureOr<uint64_t> words(mlir::Type type, unsigned depth = 0,
                                  bool debitWork = true);
  bool describe(mlir::Operation *operation, uint64_t rows,
                TableQueryCharge &charge, bool debitWork = true);
  mlir::LogicalResult charge(mlir::Operation *operation, uint64_t rows = 1);
  mlir::FailureOr<mlir::Value> gather(mlir::Value table, mlir::Value indices);
  mlir::FailureOr<llvm::SmallVector<mlir::Value>>
  reduce(mlir::ValueRange tables, uint64_t outer, uint64_t extent, bool argmin);
  mlir::FailureOr<llvm::SmallVector<mlir::Value>>
  combine(mlir::ValueRange left, mlir::ValueRange right, bool argmin);
  mlir::FailureOr<llvm::SmallVector<mlir::Value>>
  map(mlir::ValueRange tables, mlir::ValueRange captures,
      llvm::ArrayRef<uint64_t> shape, mlir::TypeRange elements,
      const std::function<mlir::FailureOr<llvm::SmallVector<mlir::Value>>(
          mlir::OpBuilder &, mlir::ValueRange)> &body,
      TableQueryStageEnvelope *envelope = nullptr);
  mlir::FailureOr<mlir::Value>
  view(mlir::Value input, llvm::StringRef kind, llvm::ArrayRef<uint64_t> shape,
       llvm::ArrayRef<mlir::NamedAttribute> parameters);
  mlir::FailureOr<mlir::Value> splat(mlir::Value input,
                                     llvm::ArrayRef<uint64_t> shape);
  mlir::ArrayAttr shape(llvm::ArrayRef<uint64_t> sizes);
  mlir::Operation *op(mlir::OpBuilder &at, llvm::StringRef name,
                      mlir::ValueRange inputs, mlir::TypeRange outputs,
                      llvm::ArrayRef<mlir::NamedAttribute> attrs = {},
                      unsigned regions = 0);
  mlir::Value constant(mlir::OpBuilder &at, uint64_t value, mlir::Type type);
  mlir::Value binary(mlir::OpBuilder &at, llvm::StringRef code, mlir::Value a,
                     mlir::Value b);
  mlir::Value compare(mlir::OpBuilder &at, llvm::StringRef code, mlir::Value a,
                      mlir::Value b);
  mlir::Value widen(mlir::OpBuilder &at, mlir::Value value, mlir::Type type);
  mlir::OpBuilder &at;
  mlir::Location loc;
  ac::HardwareAnalysis analysis;
  mlir::ModuleOp package;
  TableQueryBudget &budget;
  TableQueryHooks hooks;
  bool failedCharge = false;
  bool scalarTemplate = false;
};
} // namespace acir::compiler::detail
#endif
