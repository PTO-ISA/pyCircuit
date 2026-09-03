#ifndef ACIR_CODEGEN_PROCESSGENERATOR_H
#define ACIR_CODEGEN_PROCESSGENERATOR_H

#include "acir/CodeGen/Generator.h"

namespace acir::codegen::detail {

llvm::Expected<GeneratedFile> generateProcessHeader(const ModelPlan &plan,
                                                    const ProcessPlan &process);

llvm::Expected<GeneratedFile> generateProcessSource(const ModelPlan &plan,
                                                    const ProcessPlan &process);

} // namespace acir::codegen::detail

#endif // ACIR_CODEGEN_PROCESSGENERATOR_H
