#ifndef ACIR_INITALLPASSES_H
#define ACIR_INITALLPASSES_H

#include "acir/Transforms/Passes.h"
#include "acir/CodeGen/EmitCxx.h"
#include "mlir/Pass/PassRegistry.h"
#include "mlir/Transforms/Passes.h"

#include <memory>

namespace acir {

inline void registerAllPasses() {
  mlir::registerTransformsPasses();
  registerACIRTransformsPasses();
  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return createNormalizeACIRFilePass();
  });
  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return createVerifyACIRFilePass();
  });
  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return codegen::createEmitCxxPass({});
  });
  mlir::registerPass([]() -> std::unique_ptr<mlir::Pass> {
    return codegen::createCheckCxxContractPass();
  });
}

} // namespace acir

#endif // ACIR_INITALLPASSES_H
