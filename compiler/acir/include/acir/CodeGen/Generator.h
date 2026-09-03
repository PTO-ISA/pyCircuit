#ifndef ACIR_CODEGEN_GENERATOR_H
#define ACIR_CODEGEN_GENERATOR_H

#include "acir/CodeGen/ModelPlan.h"

#include "llvm/Support/Error.h"

#include <string>
#include <vector>

namespace acir::codegen {

struct GeneratedFile {
  std::string relativePath;
  std::string content;
  Fingerprint fingerprint;
};

struct SourceBundle {
  Fingerprint sourceFingerprint;
  Fingerprint buildFingerprint;
  std::vector<GeneratedFile> files;
};

llvm::Expected<SourceBundle> generateModelSources(const ModelPlan &plan);
llvm::Error validateSourceBundle(const ModelPlan &plan,
                                 const SourceBundle &bundle);

} // namespace acir::codegen

#endif // ACIR_CODEGEN_GENERATOR_H
