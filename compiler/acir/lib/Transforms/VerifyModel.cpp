#include "acir/Transforms/Passes.h"

#include "acir/Analysis/ModelAnalysis.h"

using namespace mlir;

namespace acir {
namespace {
#define GEN_PASS_DEF_VERIFYMODELPASS
#include "acir/Transforms/Passes.h.inc"

struct VerifyModelPass : impl::VerifyModelPassBase<VerifyModelPass> {
  void runOnOperation() override {
    if (failed(verifyModel(getOperation())))
      signalPassFailure();
  }
};
} // namespace

std::unique_ptr<Pass> createVerifyModelPass() {
  return std::make_unique<VerifyModelPass>();
}

} // namespace acir
