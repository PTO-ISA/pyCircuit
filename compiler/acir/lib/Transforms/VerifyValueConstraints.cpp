#include "acir/Transforms/Passes.h"

#include "acir/Analysis/VariableAnalysis.h"
#include "acir/Analysis/WriterArbitrationAnalysis.h"
#include "acir/Dialect/ACIR/ACIROps.h"

#include "mlir/IR/SymbolTable.h"

#include <limits>

using namespace mlir;

namespace acir {
namespace {

template <typename Declaration>
Declaration resolveFlatDeclaration(Operation *operation,
                                   FlatSymbolRefAttr reference) {
  for (Operation *ancestor = operation->getParentOp(); ancestor;
       ancestor = ancestor->getParentOp()) {
    for (Region &region : ancestor->getRegions())
      for (Block &block : region)
        for (Declaration declaration : block.getOps<Declaration>())
          if (declaration.getSymName() == reference.getValue())
            return declaration;
  }
  return {};
}

std::string constraintText(const ValueConstraint &constraint) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  constraint.print(stream);
  return text;
}

ValueConstraint typeConstraint(Type type) {
  if (auto variable = dyn_cast<ac::VarType>(type))
    type = variable.getElementType();
  if (auto range = dyn_cast<ac::RangeType>(type))
    return ValueConstraint::closedInterval(range.getLower(), range.getUpper());
  auto integer = dyn_cast<IntegerType>(type);
  if (!integer || !integer.isSignless() || integer.getWidth() == 0 ||
      integer.getWidth() > 64)
    return ValueConstraint::unknown();
  const unsigned width = integer.getWidth();
  const uint64_t upper = width == 64 ? std::numeric_limits<uint64_t>::max()
                                     : (uint64_t{1} << width) - 1;
  return ValueConstraint::closedInterval(0, upper);
}

LogicalResult verifyIndex(ACDataFlowAnalyzer &analysis, Operation *operation,
                          Value index, uint64_t extent, StringRef resource) {
  if (extent == 0)
    return operation->emitOpError() << resource << " extent must be positive";
  if (auto constant = index.getDefiningOp<ac::VarConstantOp>()) {
    auto value = dyn_cast<IntegerAttr>(constant.getValue());
    if (value && value.getValue().getZExtValue() < extent)
      return success();
    return operation->emitOpError()
           << "constant " << resource << " index is out of range";
  }
  ValueConstraint constraint = analysis.lookupConstraint(index);
  if (constraint.kind == ValueConstraintKind::Unknown)
    constraint = typeConstraint(index.getType());
  if (constraint.provesWithin(0, extent - 1))
    return success();
  return operation->emitOpError()
         << "cannot prove " << resource << " index is within [0, " << extent - 1
         << "]; inferred " << constraintText(constraint);
}

FailureOr<uint64_t> shapedEntries(Operation *operation,
                                  DenseI64ArrayAttr shape) {
  uint64_t entries = 1;
  if (!shape || shape.asArrayRef().empty())
    return failure();
  for (int64_t extent : shape.asArrayRef()) {
    if (extent <= 0 || entries > std::numeric_limits<uint64_t>::max() /
                                     static_cast<uint64_t>(extent)) {
      operation->emitOpError("shaped ac.var extent product overflows");
      return failure();
    }
    entries *= static_cast<uint64_t>(extent);
  }
  return entries;
}

LogicalResult verifyTableAccessIndex(ACDataFlowAnalyzer &analysis,
                                     Operation *operation, Value index,
                                     ac::TableOp table, StringRef resource) {
  if (auto flattened = index.getDefiningOp<ac::TableIndexOp>()) {
    if (flattened.getTableAttr() !=
        FlatSymbolRefAttr::get(operation->getContext(), table.getSymName()))
      return operation->emitOpError(
          "flattened Table index belongs to another Table");
    return success();
  }
  if (auto selection = index.getDefiningOp<ac::TableChooseOp>()) {
    const int64_t count = selection.getCountAttr().getInt();
    if (count <= 0 || selection.getResults().size() != 2 * count ||
        !llvm::is_contained(selection.getResults().take_front(count), index) ||
        selection.getTableAttr() !=
            FlatSymbolRefAttr::get(operation->getContext(), table.getSymName()))
      return operation->emitOpError(
          "TableChoice index belongs to another Table");
    return success();
  }
  return verifyIndex(analysis, operation, index, table.getEntries(), resource);
}

#define GEN_PASS_DEF_VERIFYVALUECONSTRAINTSPASS
#include "acir/Transforms/Passes.h.inc"

struct VerifyValueConstraintsPass
    : impl::VerifyValueConstraintsPassBase<VerifyValueConstraintsPass> {
  void runOnOperation() override {
    if (failed(verifyValueConstraints(getOperation())))
      signalPassFailure();
  }
};

} // namespace

LogicalResult verifyValueConstraints(ModuleOp model) {
  ACDataFlowAnalyzer analysis(model.getOperation());
  if (failed(analysis.run()))
    return model.emitError("AC value constraint analysis failed");

  LogicalResult result = success();
  model.walk([&](Operation *operation) {
    if (failed(result))
      return WalkResult::interrupt();
    if (auto read = dyn_cast<ac::VarReadElementOp>(operation)) {
      auto variable =
          resolveFlatDeclaration<ac::VarDeclOp>(read, read.getVariableAttr());
      if (!variable || !variable.getShapeAttr())
        return WalkResult::advance();
      auto entries = shapedEntries(read, variable.getShapeAttr());
      result = failed(entries) ? failure()
                               : verifyIndex(analysis, read, read.getIndex(),
                                             *entries, "shaped ac.var");
    } else if (auto refine = dyn_cast<ac::VarRangeRefineOp>(operation)) {
      auto target = dyn_cast<ac::RangeType>(
          cast<ac::VarType>(refine.getResult().getType()).getElementType());
      if (target && !analysis.provesWithin(refine.getInput(), target.getLower(),
                                           target.getUpper())) {
        ValueConstraint constraint =
            analysis.lookupConstraint(refine.getInput());
        result = refine.emitOpError()
                 << "cannot prove strict range refinement is within ["
                 << target.getLower() << ", " << target.getUpper()
                 << "]; inferred " << constraintText(constraint);
      }
    } else if (auto element = dyn_cast<ac::VarDynamicElementOp>(operation)) {
      auto array = dyn_cast<ac::ValueArrayType>(
          cast<ac::VarType>(element.getAggregate().getType()).getElementType());
      if (array)
        result = verifyIndex(analysis, element, element.getIndex(),
                             static_cast<uint64_t>(array.getLength()),
                             "value_array");
    } else if (auto update = dyn_cast<ac::VarWithElementOp>(operation)) {
      auto array = dyn_cast<ac::ValueArrayType>(
          cast<ac::VarType>(update.getAggregate().getType()).getElementType());
      if (array)
        result = verifyIndex(analysis, update, update.getIndex(),
                             static_cast<uint64_t>(array.getLength()),
                             "value_array update");
    } else if (auto assign = dyn_cast<ac::VarAssignElementOp>(operation)) {
      auto variable = resolveFlatDeclaration<ac::VarDeclOp>(
          assign, assign.getVariableAttr());
      if (!variable || !variable.getShapeAttr())
        return WalkResult::advance();
      auto entries = shapedEntries(assign, variable.getShapeAttr());
      result = failed(entries)
                   ? failure()
                   : verifyIndex(analysis, assign, assign.getIndex(), *entries,
                                 "shaped ac.var");
    } else if (auto flattened = dyn_cast<ac::TableIndexOp>(operation)) {
      auto table = resolveFlatDeclaration<ac::TableOp>(
          flattened, flattened.getTableAttr());
      if (!table)
        return WalkResult::advance();
      SmallVector<int64_t> shape;
      if (auto rawShape = table.getShape())
        shape.assign(rawShape->begin(), rawShape->end());
      else
        shape.push_back(table.getEntries());
      if (shape.size() != flattened.getCoordinates().size())
        return WalkResult::advance();
      for (auto [axis, values] : llvm::enumerate(
               llvm::zip_equal(flattened.getCoordinates(), shape))) {
        auto [coordinate, extent] = values;
        std::string resource = "Table coordinate axis " + std::to_string(axis);
        result = verifyIndex(analysis, flattened, coordinate, extent, resource);
        if (failed(result))
          break;
      }
    } else if (auto match = dyn_cast<ac::VarMatchOp>(operation)) {
      if (!match.getRow())
        return WalkResult::advance();
      auto variable =
          resolveFlatDeclaration<ac::VarDeclOp>(match, match.getVariableAttr());
      auto shape = variable ? variable.getShapeAttr() : DenseI64ArrayAttr();
      if (shape && shape.asArrayRef().size() == 2)
        result = verifyIndex(analysis, match, match.getRow(),
                             shape.asArrayRef().front(), "ac.var row");
    } else if (auto read = dyn_cast<ac::TableGetOp>(operation)) {
      auto table =
          resolveFlatDeclaration<ac::TableOp>(read, read.getTableAttr());
      if (table)
        result = verifyTableAccessIndex(analysis, read, read.getIndex(), table,
                                        "Table");
    } else if (auto read = dyn_cast<ac::TableReadOp>(operation)) {
      auto table =
          resolveFlatDeclaration<ac::TableOp>(read, read.getTableAttr());
      if (table && read.getAddress().hasOneBlock())
        result = verifyTableAccessIndex(
            analysis, read,
            cast<ac::TableYieldOp>(read.getAddress().front().getTerminator())
                .getValue(),
            table, "Table read address");
    } else if (auto write = dyn_cast<ac::TableWriteOp>(operation)) {
      auto table =
          resolveFlatDeclaration<ac::TableOp>(write, write.getTableAttr());
      if (table && write.getAddress().hasOneBlock())
        result = verifyTableAccessIndex(
            analysis, write,
            cast<ac::TableYieldOp>(write.getAddress().front().getTerminator())
                .getValue(),
            table, "Table write address");
    } else if (auto proposal = dyn_cast<ac::TableProposeOp>(operation)) {
      auto table = resolveFlatDeclaration<ac::TableOp>(proposal,
                                                       proposal.getTableAttr());
      if (table)
        result = verifyTableAccessIndex(analysis, proposal, proposal.getIndex(),
                                        table, "Table");
    } else if (auto snapshot = dyn_cast<ac::StateSnapshotOp>(operation)) {
      if (!snapshot.getIndex())
        return WalkResult::advance();
      auto table = resolveFlatDeclaration<ac::TableOp>(snapshot,
                                                       snapshot.getTableAttr());
      if (table)
        result = verifyTableAccessIndex(analysis, snapshot, snapshot.getIndex(),
                                        table, "Table snapshot");
    }
    return succeeded(result) ? WalkResult::advance() : WalkResult::interrupt();
  });
  if (failed(result))
    return result;

  return analyzeWriterArbitration(model, analysis);
}

std::unique_ptr<Pass> createVerifyValueConstraintsPass() {
  return std::make_unique<VerifyValueConstraintsPass>();
}

} // namespace acir
