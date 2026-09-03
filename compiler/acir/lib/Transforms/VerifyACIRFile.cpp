#include "acir/Transforms/Passes.h"

#include "Dialect/ACIR/ProcessLowerability.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "acir/Dialect/ACIR/ACIRResources.h"
#include "acir/Dialect/ACSim/ACSimOps.h"

namespace acir {
namespace {

class VerifyACIRFilePass final
    : public mlir::PassWrapper<VerifyACIRFilePass,
                               mlir::OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(VerifyACIRFilePass)

  llvm::StringRef getArgument() const override { return "verify-ac-file"; }
  llvm::StringRef getDescription() const override {
    return "Verify the Agentic Circuit epoch and whole-file legality";
  }

  void runOnOperation() override {
    mlir::ModuleOp module = getOperation();
    if (mlir::failed(ac::preflightRawModelStructure(module))) {
      signalPassFailure();
      return;
    }
    auto epoch = module->getAttrOfType<mlir::StringAttr>("ac.contract_epoch");
    if (!epoch || epoch.getValue() != "0.4") {
      module.emitError(
          "expected top-level 'ac.contract_epoch' string attribute equal to "
          "\"0.4\"");
      signalPassFailure();
      return;
    }
    if (mlir::failed(acsim::verifyCanonicalACSimFile(module))) {
      signalPassFailure();
      return;
    }

    mlir::WalkResult result = module.walk([&](mlir::Operation *operation) {
      if (mlir::failed(ac::verifyTopologyTypeUses(operation)))
        return mlir::WalkResult::interrupt();
      auto rejectChannel = [&](mlir::Attribute attribute) {
        return attribute && attribute
                                .walk([](ac::ChannelType) {
                                  return mlir::WalkResult::interrupt();
                                })
                                .wasInterrupted();
      };
      for (mlir::NamedAttribute attribute : operation->getAttrs()) {
        if (mlir::isa<ac::PortOp>(operation) && attribute.getName() == "type")
          continue;
        if (rejectChannel(attribute.getValue())) {
          operation->emitError("channel type is only permitted in an "
                               "ac.interface channel declaration");
          return mlir::WalkResult::interrupt();
        }
      }
      if ((!mlir::isa<ac::PortOp>(operation) &&
           rejectChannel(operation->getPropertiesAsAttribute())) ||
          rejectChannel(mlir::LocationAttr(operation->getLoc()))) {
        operation->emitError("channel type is only permitted in an "
                             "ac.interface channel declaration");
        return mlir::WalkResult::interrupt();
      }
      for (mlir::Type type : operation->getOperandTypes())
        if (ac::containsChannelType(type)) {
          operation->emitError("channel type is only permitted in an "
                               "ac.interface channel declaration");
          return mlir::WalkResult::interrupt();
        }
      for (mlir::Type type : operation->getResultTypes())
        if (ac::containsChannelType(type)) {
          operation->emitError("channel type is only permitted in an "
                               "ac.interface channel declaration");
          return mlir::WalkResult::interrupt();
        }
      for (mlir::Region &region : operation->getRegions())
        for (mlir::Block &block : region)
          for (mlir::BlockArgument argument : block.getArguments())
            if (ac::containsChannelType(argument.getType())) {
              operation->emitError("channel type is only permitted in an "
                                   "ac.interface channel declaration");
              return mlir::WalkResult::interrupt();
            }
      return mlir::WalkResult::advance();
    });
    if (result.wasInterrupted())
      signalPassFailure();
  }
};

} // namespace

std::unique_ptr<mlir::Pass> createVerifyACIRFilePass() {
  return std::make_unique<VerifyACIRFilePass>();
}

} // namespace acir
