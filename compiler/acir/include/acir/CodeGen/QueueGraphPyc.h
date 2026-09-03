#ifndef ACIR_CODEGEN_QUEUEGRAPHPYC_H
#define ACIR_CODEGEN_QUEUEGRAPHPYC_H

#include "acir/CodeGen/QueueGraphPlan.h"
#include "llvm/Support/Error.h"

#include <string>

namespace acir::codegen {

llvm::Expected<std::string> generateQueueGraphPyc(const QueueGraphPlan &plan);

} // namespace acir::codegen

#endif // ACIR_CODEGEN_QUEUEGRAPHPYC_H
