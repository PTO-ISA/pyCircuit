#include "mlir/Pass/Pass.h"
#include "pycircuit/Dialect/ACIR/RecordConstructors.h"
#include "pycircuit/Transforms/Passes.h"
using namespace mlir;
namespace acir {
namespace {
class NormalizeRecordConstructorsPass
    : public PassWrapper<NormalizeRecordConstructorsPass,
                         OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(NormalizeRecordConstructorsPass)
  StringRef getArgument() const final {
    return "ac-normalize-record-constructors";
  }
  StringRef getDescription() const final {
    return "Complete source record constructors in declaration order";
  }
  void runOnOperation() final {
    if (failed(ac::normalizeRecordConstructors(getOperation())))
      signalPassFailure();
  }
};
} // namespace
std::unique_ptr<mlir::Pass> createNormalizeRecordConstructorsPass() {
  return std::make_unique<NormalizeRecordConstructorsPass>();
}
} // namespace acir
