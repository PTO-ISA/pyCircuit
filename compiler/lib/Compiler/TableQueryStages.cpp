#include "PythonImportContext.h"
#include "TableQueryLowering.h"
#include "mlir/IR/IRMapping.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/Support/MathExtras.h"

using namespace mlir;
namespace acir::compiler::detail {
FailureOr<SmallVector<Value>> TableQueryLowering::map(
    ValueRange tables, ValueRange captures, ArrayRef<uint64_t> sizes,
    TypeRange elements,
    const std::function<FailureOr<SmallVector<Value>>(OpBuilder &, ValueRange)>
        &body) {
  uint64_t rows = 1;
  for (uint64_t size : sizes) {
    if (!size ||
        rows > TableQueryBudget::limit(TableQueryResource::Work) / size)
      return emitError(loc) << "Table query shape product budget exhausted";
    rows *= size;
  }
  auto domain = shape(sizes);
  SmallVector<Type> results;
  for (Type type : elements)
    results.push_back(ac::TableType::get(at.getContext(), domain, type));
  SmallVector<Value> operands(tables);
  llvm::append_range(operands, captures);
  // Type one scalar description in existing module/rule lookup scope. It is
  // discarded; only the fully reserved map is published in the source graph.
  auto temporary = createSourceOperation(
      at, loc, ac::RuleOp::getOperationName(), {}, {}, {}, 1);
  auto block = new Block();
  temporary->getRegion(0).push_back(block);
  auto cleanup = llvm::scope_exit([&] {
    for (Value value : block->getArguments())
      hooks.forgetFacts(value);
    for (Operation &operation : *block)
      for (Value value : operation.getResults())
        hooks.forgetFacts(value);
    temporary->erase();
  });
  SmallVector<Value> arguments;
  arguments.push_back(block->addArgument(
      hooks.bits(std::max(1u, llvm::Log2_64_Ceil(rows))), loc));
  for (Value table : tables)
    arguments.push_back(block->addArgument(
        cast<ac::TableType>(table.getType()).getElementType(), loc));
  for (Value capture : captures) {
    auto argument = block->addArgument(capture.getType(), loc);
    hooks.copyFacts(capture, argument);
    arguments.push_back(argument);
  }
  auto nested = OpBuilder::atBlockEnd(block);
  bool previousTemplate = std::exchange(scalarTemplate, true);
  auto restore = llvm::scope_exit([&] { scalarTemplate = previousTemplate; });
  auto values = body(nested, arguments);
  if (failed(values) || failedCharge)
    return failure();
  createSourceOperation(nested, loc, ac::YieldOp::getOperationName(), *values,
                        {}, {});
  scalarTemplate = previousTemplate;
  TableQueryCharge reservation;
  bool fits =
      TableQueryBudget::add(reservation, TableQueryResource::Operations, 1) &&
      TableQueryBudget::add(reservation, TableQueryResource::Slots,
                            operands.size() + results.size() +
                                arguments.size());
  for (Type result : results) {
    auto weight = words(result);
    fits &= succeeded(weight) &&
            TableQueryBudget::addProduct(reservation,
                                         TableQueryResource::PayloadWords,
                                         succeeded(weight) ? *weight : 0, 3);
  }
  for (Value capture : captures) {
    auto weight = words(capture.getType());
    fits &= succeeded(weight) &&
            TableQueryBudget::addProduct(reservation,
                                         TableQueryResource::PayloadWords,
                                         succeeded(weight) ? *weight : 0, 3);
  }
  for (Operation &operation : *block)
    fits &= describe(&operation, rows, reservation);
  std::string reason;
  if (!fits || !budget.reserve(reservation, reason))
    return emitError(loc)
           << (fits ? reason
                    : "Table query replicated scalar/storage budget exhausted");
  // All work, scalar intermediates, table planes, slots and constants are
  // reserved before any row-domain operation or scalar-template clone.
  auto operation = createSourceOperation(
      at, loc, ac::TableMapOp::getOperationName(), operands, results,
      {at.getNamedAttr("shape", domain),
       at.getNamedAttr("operandSegmentSizes",
                       at.getDenseI32ArrayAttr({int32_t(tables.size()),
                                                int32_t(captures.size())}))},
      1);
  auto published = new Block();
  operation->getRegion(0).push_back(published);
  IRMapping mapping;
  for (Value argument : arguments) {
    auto copied = published->addArgument(argument.getType(), loc);
    mapping.map(argument, copied);
    hooks.copyFacts(argument, copied);
  }
  auto target = OpBuilder::atBlockEnd(published);
  for (Operation &instruction : *block) {
    auto copy = target.clone(instruction, mapping);
    for (auto [original, result] :
         llvm::zip(instruction.getResults(), copy->getResults()))
      hooks.copyFacts(original, result);
  }
  return SmallVector<Value>(operation->getResults());
}
FailureOr<SmallVector<Value>> TableQueryLowering::stage(Block &scalar,
                                                        Value row,
                                                        Value receiver,
                                                        ValueRange outputs) {
  auto size = analysis.getTableSize(cast<ac::TableType>(receiver.getType()), {},
                                    package);
  if (failed(size))
    return failure();
  for (Operation &operation : scalar)
    if (!isa<ac::TableGetOp, ac::BitsConstantOp, ac::BitsUnaryOp,
             ac::BitsBinaryOp, ac::BitsCompareOp, ac::BitsSelectOp,
             ac::BitsConcatOp, ac::BitsExtractOp, ac::BitsResizeOp,
             ac::StructCreateOp, ac::StructGetOp, ac::EnumCreateOp,
             ac::EnumToBitsOp, ac::EnumFromBitsOp, ac::ValueMergeOp>(operation))
      return operation.emitError()
             << "Table query scalar template is unsupported";
  llvm::DenseMap<Value, Value> tables, scalars;
  llvm::DenseSet<Value> dependent;
  tables[row] = receiver;
  dependent.insert(row);
  SmallVector<Operation *> pending;
  auto flush = [&]() -> LogicalResult {
    if (pending.empty())
      return success();
    llvm::DenseSet<Value> internal, seen;
    for (Operation *operation : pending)
      for (Value result : operation->getResults())
        internal.insert(result);
    SmallVector<Value> tableInputs, captures, originals;
    for (Operation *operation : pending)
      for (Value input : operation->getOperands())
        if (!internal.contains(input) && seen.insert(input).second) {
          originals.push_back(input);
          if (dependent.contains(input))
            tableInputs.push_back(tables.lookup(input));
          else
            captures.push_back(scalars.lookup(input) ? scalars.lookup(input)
                                                     : input);
        }
    llvm::DenseSet<Operation *> stageOperations(pending.begin(), pending.end());
    SmallVector<Type> types;
    SmallVector<Value> results;
    for (Operation *operation : pending)
      for (Value result : operation->getResults()) {
        bool live = llvm::is_contained(outputs, result) ||
                    llvm::any_of(result.getUsers(), [&](Operation *user) {
                      return !stageOperations.contains(user);
                    });
        if (live) {
          types.push_back(result.getType());
          results.push_back(result);
        }
      }
    if (results.empty()) {
      TableQueryCharge reservation;
      bool fits = true;
      for (Operation *operation : pending)
        fits &= describe(operation, *size, reservation);
      std::string reason;
      if (!fits || !budget.reserve(reservation, reason))
        return emitError(loc)
               << (fits ? reason : "Table query scalar work budget exhausted");
      pending.clear();
      return success();
    }
    auto mapped =
        map(tableInputs, captures, {*size}, types,
            [&](OpBuilder &nested,
                ValueRange arguments) -> FailureOr<SmallVector<Value>> {
              IRMapping mapping;
              unsigned tableSlot = 1, captureSlot = 1 + tableInputs.size();
              for (Value original : originals) {
                auto argument =
                    arguments[dependent.contains(original) ? tableSlot++
                                                           : captureSlot++];
                mapping.map(original, argument);
                hooks.copyFacts(original, argument);
              }
              for (Operation *operation : pending) {
                auto copy = nested.clone(*operation, mapping);
                for (auto [original, result] :
                     llvm::zip(operation->getResults(), copy->getResults()))
                  hooks.copyFacts(original, result);
              }
              SmallVector<Value> yielded;
              for (Value result : results)
                yielded.push_back(mapping.lookup(result));
              return yielded;
            });
    if (failed(mapped))
      return failure();
    for (auto [original, result] : llvm::zip(results, *mapped))
      tables[original] = result;
    pending.clear();
    return success();
  };
  for (Operation &operation : scalar) {
    bool depends = llvm::any_of(operation.getOperands(), [&](Value value) {
      return dependent.contains(value);
    });
    if (auto get = dyn_cast<ac::TableGetOp>(operation); get && depends) {
      if (failed(flush()))
        return failure();
      Value source = scalars.lookup(get.getInput());
      if (!source)
        source = get.getInput();
      if (dependent.contains(get.getInput()))
        return emitError(loc)
               << "Table query gather receiver must be row independent";
      auto gathered = gather(source, tables.lookup(get.getIndex()));
      if (failed(gathered))
        return failure();
      tables[get.getValue()] = *gathered;
      dependent.insert(get.getValue());
    } else if (depends) {
      if (!isa<ac::BitsConstantOp, ac::BitsUnaryOp, ac::BitsBinaryOp,
               ac::BitsCompareOp, ac::BitsSelectOp, ac::BitsConcatOp,
               ac::BitsExtractOp, ac::BitsResizeOp, ac::StructCreateOp,
               ac::StructGetOp, ac::EnumCreateOp, ac::EnumToBitsOp,
               ac::EnumFromBitsOp, ac::ValueMergeOp>(operation))
        return operation.emitError()
               << "Table query scalar stage is unsupported";
      pending.push_back(&operation);
      for (Value result : operation.getResults())
        dependent.insert(result);
    } else {
      if (failed(charge(&operation)))
        return failure();
      IRMapping mapping;
      for (Value input : operation.getOperands())
        mapping.map(input,
                    scalars.lookup(input) ? scalars.lookup(input) : input);
      auto copy = at.clone(operation, mapping);
      for (auto [original, result] :
           llvm::zip(operation.getResults(), copy->getResults())) {
        scalars[original] = result;
        hooks.copyFacts(original, result);
      }
    }
  }
  if (failed(flush()))
    return failure();
  SmallVector<Value> result;
  for (Value output : outputs) {
    if (dependent.contains(output))
      result.push_back(tables.lookup(output));
    else {
      auto repeated = splat(
          scalars.lookup(output) ? scalars.lookup(output) : output, {*size});
      if (failed(repeated))
        return failure();
      result.push_back(*repeated);
    }
  }
  return result;
}
} // namespace acir::compiler::detail
