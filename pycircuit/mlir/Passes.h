#pragma once
#include "mlir/Pass/Pass.h"
namespace acir {
int compileMain(int argc, char **argv);
std::unique_ptr<mlir::Pass> createLowerAnalysisPass();
void registerACIRPasses();
}
