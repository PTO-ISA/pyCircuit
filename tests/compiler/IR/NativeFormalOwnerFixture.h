#ifndef PYCIRCUIT_TESTS_COMPILER_IR_NATIVEFORMALOWNERFIXTURE_H
#define PYCIRCUIT_TESTS_COMPILER_IR_NATIVEFORMALOWNERFIXTURE_H

#include "Compiler/SourceUnit.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/STLExtras.h"
#include "pycircuit/Transforms/Passes.h"

namespace acir::test {

// This constructs native common IR, not a Python authoring route. Python's
// same-source owner specialization intentionally removes these proposal pairs.
// Start from a genuine generic/by-value callable, then bind its existing
// candidate/intent result suffix to explicitly declared physical state.
inline mlir::LogicalResult bindNativeFormalOwners(
    ac::SourceMethodCallOp call, llvm::ArrayRef<llvm::StringRef> owners) {
  auto module = call->getParentOfType<ac::ModuleOp>();
  auto file = module->getParentOfType<mlir::ModuleOp>();
  auto method = mlir::dyn_cast_or_null<ac::SourceMethodOp>(
      mlir::SymbolTable::lookupSymbolIn(file, call.getCalleeAttr()));
  auto proposals = method
                       ? method->getAttrOfType<mlir::ArrayAttr>("ac.formal_proposals")
                       : mlir::ArrayAttr();
  if (!method || !proposals || owners.size() != call.getArguments().size() ||
      call->getNumResults() < 2 * proposals.size())
    return call.emitOpError() << "native owner fixture requires a generic proposal signature";
  mlir::OpBuilder builder(call);
  llvm::SmallVector<mlir::Attribute> bindings;
  for (auto [index, name] : llvm::enumerate(owners)) {
    if (name.empty())
      continue;
    ac::SourceStateOp owner;
    for (auto state : module.getBody().front().getOps<ac::SourceStateOp>())
      if (state.getInstanceName() == name)
        owner = state;
    if (!owner || !ac::areEquivalentHardwareTypes(
                      owner.getQ().getType(), call.getArguments()[index].getType()))
      return call.emitOpError() << "native owner fixture lacks the exact declared state";
    mlir::OperationState read(call.getLoc(), ac::SourceStateReadOp::getOperationName());
    read.addOperands(call.getPath());
    read.addTypes(owner.getQ().getType());
    read.addAttribute("state", builder.getStringAttr(name));
    read.addAttribute("selectors", builder.getArrayAttr({}));
    read.addAttribute("occurrence", call.getOccurrence());
    call->setOperand(index + 1, builder.create(read)->getResult(0));
    if (llvm::is_contained(proposals, builder.getI64IntegerAttr(index)))
      bindings.push_back(builder.getDictionaryAttr({
          builder.getNamedAttr("formal", builder.getI64IntegerAttr(index)),
          builder.getNamedAttr("state", builder.getStringAttr(name))}));
  }
  call->setAttr("ac.formal_owner_bindings", builder.getArrayAttr(bindings));
  builder.setInsertionPointAfter(call);
  unsigned publicResults = call->getNumResults() - 2 * proposals.size();
  for (auto [ordinal, raw] : llvm::enumerate(proposals)) {
    unsigned formal = mlir::cast<mlir::IntegerAttr>(raw).getInt();
    if (owners[formal].empty())
      continue;
    mlir::OperationState write(call.getLoc(), ac::SourceStateWriteOp::getOperationName());
    write.addOperands({call->getResult(publicResults + 2 * ordinal + 1),
                       call->getResult(publicResults + 2 * ordinal)});
    write.addAttribute("state", builder.getStringAttr(owners[formal]));
    write.addAttribute("selectors", builder.getArrayAttr({}));
    write.addAttribute("occurrence", call.getOccurrence());
    builder.create(write);
  }
  return mlir::success();
}

// Native construction changes effects. Recompute summaries and publish a new
// interface before linking; the original Python-produced header is discarded.
inline mlir::LogicalResult publishNativeFormalOwnerFixture(
    compiler::SourceUnitArtifacts &unit, mlir::MLIRContext &context) {
  mlir::PassManager summaries(&context);
  summaries.addPass(createAnalyzeSourceMethodSummariesPass());
  if (mlir::failed(summaries.run(*unit.body)))
    return mlir::failure();
  unit.interface = mlir::OwningOpRef<mlir::ModuleOp>(
      mlir::cast<mlir::ModuleOp>(unit.body->clone()));
  mlir::PassManager publication(&context);
  publication.addPass(createExtractSourceInterfacePass());
  return publication.run(*unit.interface);
}

} // namespace acir::test
#endif
