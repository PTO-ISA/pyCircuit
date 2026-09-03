#ifndef ACIR_CODEGEN_MODELPLANINTERNAL_H
#define ACIR_CODEGEN_MODELPLANINTERNAL_H

#include "acir/CodeGen/ModelPlan.h"
#include "acir/Dialect/ACSim/ACSimOps.h"

namespace acir::codegen::detail {

llvm::Error populateModelDetails(acsim::ModelOp model, ModelPlan &plan);
llvm::Error validateModelDetails(const ModelPlan &plan);

} // namespace acir::codegen::detail

#endif // ACIR_CODEGEN_MODELPLANINTERNAL_H
