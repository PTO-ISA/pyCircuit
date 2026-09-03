#include "acir/Transforms/Passes.h"

#include "acir/Analysis/ProcessStatePlan.h"

#include "mlir/Pass/Pass.h"

using namespace mlir;

namespace acir {
namespace {
#define GEN_PASS_DEF_LOWERPROCESSSTATEPASS
#include "acir/Transforms/Passes.h.inc"

struct LowerProcessStatePass
    : impl::LowerProcessStatePassBase<LowerProcessStatePass> {
  using Base = impl::LowerProcessStatePassBase<LowerProcessStatePass>;
  using Base::Base;

  void runOnOperation() final {
    ProcessStateLimits limits;
    auto plans = planProcessState(getOperation(), limits);
    if (failed(plans)) {
      signalPassFailure();
      return;
    }
    if (failed(verifyProcessStatePlan(*plans, limits))) {
      signalPassFailure();
      return;
    }
  }
};

} // namespace

std::unique_ptr<Pass> createLowerProcessStatePass() {
  return std::make_unique<LowerProcessStatePass>();
}

} // namespace acir
