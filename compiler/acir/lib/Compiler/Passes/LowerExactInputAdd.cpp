#include "Compiler/ScalarNumericLowering.h"

#include "mlir/Pass/PassRegistry.h"

using namespace mlir;

namespace acir::compiler {
namespace {

class LowerExactInputAddPass final
    : public PassWrapper<LowerExactInputAddPass,
                         OperationPass<mlir::ModuleOp>> {
public:
  StringRef getArgument() const final { return "ac-lower-exact-input-add"; }
  StringRef getDescription() const final {
    return "transactionally lower the verified N0-C1 exact input-add profile";
  }

  void runOnOperation() final {
    if (failed(lowerExactInputAddTransactional(getOperation())))
      signalPassFailure();
  }
};

} // namespace

std::unique_ptr<Pass> createLowerExactInputAddPass() {
  return std::make_unique<LowerExactInputAddPass>();
}

void registerACIRScalarNumericPasses() {
  registerPass(
      []() -> std::unique_ptr<Pass> { return createLowerExactInputAddPass(); });
}

} // namespace acir::compiler
