#ifndef ACIR_CODEGEN_EMITCXX_H
#define ACIR_CODEGEN_EMITCXX_H

#include "acir/CodeGen/Manifest.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Support/LogicalResult.h"

#include <memory>
#include <string>

namespace mlir {
class Pass;
} // namespace mlir

namespace acir::acsim {
class ModelOp;
} // namespace acir::acsim

namespace acir::codegen {

struct EmitCxxOptions {
  std::string outputDir;
  std::string profile = "fast";
  std::string toolchainTarget = "unspecified";
  bool emitMain = true;
};

/// True when `--acsim-output-dir` was supplied to the driver.
bool emitCxxRequested();

/// True when `--acsim-check-cxx-contract` was supplied to the driver.
bool checkCxxContractRequested();

/// Staging directory from `--acsim-output-dir`.
std::string emitCxxOutputDir();

/// Emit deterministic C++20 sources and a build manifest from canonical ACSim.
mlir::FailureOr<BuildManifest> emitCxx(acsim::ModelOp model,
                                       const EmitCxxOptions &options);

/// Locate the unique `acsim.model` in a canonical file and emit C++.
mlir::LogicalResult emitCxxFile(mlir::ModuleOp file,
                                const EmitCxxOptions &options);

std::unique_ptr<mlir::Pass> createEmitCxxPass(EmitCxxOptions options);

std::unique_ptr<mlir::Pass> createCheckCxxContractPass();

} // namespace acir::codegen

#endif // ACIR_CODEGEN_EMITCXX_H
