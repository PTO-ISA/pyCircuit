#ifndef ACIR_CODEGEN_QUEUEGRAPHGENERATOR_H
#define ACIR_CODEGEN_QUEUEGRAPHGENERATOR_H

#include "acir/CodeGen/QueueGraphPlan.h"
#include "llvm/Support/Error.h"

#include <string>

namespace acir::codegen {

llvm::Expected<std::string> generateQueueGraphCpp(const QueueGraphPlan &plan);

} // namespace acir::codegen

#endif // ACIR_CODEGEN_QUEUEGRAPHGENERATOR_H
