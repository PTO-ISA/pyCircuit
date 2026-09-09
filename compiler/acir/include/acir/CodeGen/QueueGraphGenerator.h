#ifndef ACIR_CODEGEN_QUEUEGRAPHGENERATOR_H
#define ACIR_CODEGEN_QUEUEGRAPHGENERATOR_H

#include "acir/CodeGen/QueueGraphPlan.h"
#include "llvm/Support/Error.h"

#include <string>
#include <vector>

namespace acir::codegen {

llvm::Expected<std::string> generateQueueGraphCpp(const QueueGraphPlan &plan);

struct QueueGraphBundleOptions {
  std::string sdkProductVersion;
  std::string sdkSourceRevision;
};

struct QueueGraphGeneratedFile {
  std::string relativePath;
  std::string content;
};

/// Generate the closed runtime-consumer source set predicted by model-plan v1.
/// The input is an already verified QueueGraph plan; this API never imports or
/// evaluates frontend Python.
llvm::Expected<std::vector<QueueGraphGeneratedFile>>
generateQueueGraphModelBundle(const QueueGraphPlan &plan,
                              const QueueGraphBundleOptions &options);

} // namespace acir::codegen

#endif // ACIR_CODEGEN_QUEUEGRAPHGENERATOR_H
