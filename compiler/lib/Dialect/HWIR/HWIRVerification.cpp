#include "pycircuit/Dialect/HWIR/HWIRVerification.h"

#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/HWIR/HWIRDialect.h"

#include "mlir/IR/SymbolTable.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassRegistry.h"
#include "llvm/ADT/StringSet.h"

using namespace mlir;

namespace acir::hwir {
namespace {

/// Admitted combinational computation shared with ACIR. Every operation here
/// is a pure payload computation with no package or stage requirement; source
/// state, transactions, methods and retained effects are intentionally absent.
bool isAdmittedComputation(Operation *operation) {
  return isa<ac::BitsConstantOp, ac::BitsConstant4Op, ac::BitsUnaryOp,
             ac::BitsBinaryOp, ac::BitsCompareOp, ac::BitsSelectOp,
             ac::BitsConcatOp, ac::BitsExtractOp, ac::BitsResizeOp,
             ac::StructCreateOp, ac::StructGetOp, ac::EnumCreateOp,
             ac::EnumToBitsOp, ac::EnumFromBitsOp>(operation);
}

LogicalResult verifyModuleBody(ModuleOp module) {
  Block &block = module.getBody().front();
  llvm::StringSet<> occurrenceNames;
  for (Operation &operation : block) {
    StringRef occurrence;
    if (auto instance = dyn_cast<InstanceOp>(operation))
      occurrence = instance.getInstanceName();
    else if (auto collection = dyn_cast<CollectionOp>(operation))
      occurrence = collection.getInstanceName();
    if (!occurrence.empty() && !occurrenceNames.insert(occurrence).second)
      return operation.emitOpError()
             << "duplicate occurrence name '" << occurrence << "'";
    if (isa<InstanceOp, CollectionOp, QueueOp, RuleOp, ExpectOp, ObserveOp,
            TableCreateOp, TableSplatOp, TableMapOp, TableViewOp,
            TableIndexOp, TableGetOp, TableMatchOp, TableChooseOp,
            TableFoldOp, ValueMergeOp, YieldOp>(operation))
      continue;
    if (isAdmittedComputation(&operation))
      continue;
    return operation.emitOpError()
           << "operation is not permitted in an HWIR module body";
  }

  for (auto rule : block.getOps<RuleOp>()) {
    for (Operation &operation : rule.getBody().front()) {
      if (isa<YieldOp, TableCreateOp, TableSplatOp, TableMapOp, TableViewOp,
              TableIndexOp, TableGetOp, TableMatchOp, TableChooseOp,
              TableFoldOp, ValueMergeOp>(operation) ||
          isAdmittedComputation(&operation))
        continue;
      return operation.emitOpError()
             << "operation is not permitted in an HWIR rule body";
    }
  }
  return success();
}

} // namespace

LogicalResult verifyHWIRPackage(mlir::ModuleOp package) {
  if (auto contract = package->getAttr("hwir.contract")) {
    auto version = dyn_cast<IntegerAttr>(contract);
    if (!version || !version.getType().isSignlessInteger(64) ||
        (version.getValue() != 1 && version.getValue() != 2))
      return package.emitOpError() << "unsupported hwir.contract version";
  }

  for (Operation &operation : package.getBody()->getOperations()) {
    if (isa<ac::StructOp, ac::EnumOp, ac::ModuleImportOp, ModuleOp, SystemOp>(
            operation))
      continue;
    return operation.emitOpError()
           << "operation outside closed HWIR package";
  }

  llvm::StringSet<> symbols;
  for (Operation &operation : package.getBody()->getOperations()) {
    auto name = operation.getAttrOfType<StringAttr>("sym_name");
    if (!name)
      continue;
    if (!symbols.insert(name.getValue()).second)
      return operation.emitOpError()
             << "duplicate definition '" << name.getValue() << "'";
  }

  auto contract = package->getAttrOfType<IntegerAttr>("hwir.contract");
  for (auto module : package.getOps<ModuleOp>()) {
    if (contract && contract.getValue() == 2 &&
        !module->getAttrOfType<ArrayAttr>("hwir.output_dependencies"))
      return module.emitOpError()
             << "HWIR v2 requires output dependency facts";
    if (failed(module.verify()) || failed(verifyModuleBody(module)))
      return failure();
  }

  unsigned systems = 0;
  for (auto system : package.getOps<SystemOp>()) {
    ++systems;
    auto entry = system.getEntry();
    auto callee = entry.getAs<FlatSymbolRefAttr>("callee");
    if (!callee)
      continue;
    Operation *target = SymbolTable::lookupSymbolIn(package, callee);
    if (!isa_and_nonnull<ModuleOp>(target))
      return system.emitOpError()
             << "entry does not resolve to an HWIR module definition";
  }
  if (package->hasAttr("hwir.contract") && systems != 1)
    return package.emitOpError()
           << "a hardware package requires exactly one hwir.system";
  return success();
}

namespace {

class HWIRVerifyPass
    : public PassWrapper<HWIRVerifyPass, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(HWIRVerifyPass)
  StringRef getArgument() const final { return "hwir-verify"; }
  StringRef getDescription() const final {
    return "Verify the closed HWIR hardware package";
  }
  void runOnOperation() final {
    if (failed(verifyHWIRPackage(getOperation())))
      signalPassFailure();
    else
      markAllAnalysesPreserved();
  }
};

} // namespace

std::unique_ptr<mlir::Pass> createHWIRVerifyPass() {
  return std::make_unique<HWIRVerifyPass>();
}

void registerHWIRPasses() {
  registerPass([] { return createHWIRVerifyPass(); });
}

} // namespace acir::hwir
