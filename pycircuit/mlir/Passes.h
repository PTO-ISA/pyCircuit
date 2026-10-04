#pragma once
#include "mlir/Pass/Pass.h"
#include <memory>
namespace acir {
std::unique_ptr<mlir::Pass> createAnalyzeResourcesPass();
std::unique_ptr<mlir::Pass> createLowerGFSimPass();
std::unique_ptr<mlir::Pass> createConvertToEmitCPass();
void registerPasses();
int compileMain(int argc, char **argv);
} // namespace acir
