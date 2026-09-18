#ifndef ACIR_CODEGEN_QUEUEGRAPHGENERATOR_H
#define ACIR_CODEGEN_QUEUEGRAPHGENERATOR_H

#include "acir/CodeGen/QueueGraphPlan.h"
#include "llvm/Support/Error.h"

#include <string>
#include <vector>

namespace acir::codegen {

llvm::Expected<std::string> generateQueueGraphCpp(const QueueGraphPlan &plan);

/// Emit deterministic, verifier-derived static cost data for a generated
/// QueueGraph model. The report carries structural names and source provenance
/// without deriving content identity.
llvm::Expected<std::string> generateQueueGraphCostReport(
    const QueueGraphPlan &plan, llvm::StringRef sdkProductVersion,
    llvm::StringRef sdkSourceRevision);

struct QueueGraphBundleOptions {
  std::string sdkProductVersion;
  std::string sdkSourceRevision;
};

struct QueueGraphGeneratedFile {
  std::string relativePath;
  std::string content;
};

/// Generate the closed runtime-consumer source set from a verified QueueGraph
/// plan. This API never imports or evaluates frontend Python.
llvm::Expected<std::vector<QueueGraphGeneratedFile>>
generateQueueGraphModelBundle(const QueueGraphPlan &plan,
                              const QueueGraphBundleOptions &options);

} // namespace acir::codegen

#endif // ACIR_CODEGEN_QUEUEGRAPHGENERATOR_H
