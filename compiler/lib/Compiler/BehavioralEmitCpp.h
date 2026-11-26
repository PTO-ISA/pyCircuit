#ifndef ACIR_LIB_COMPILER_BEHAVIORALEMITCPP_H
#define ACIR_LIB_COMPILER_BEHAVIORALEMITCPP_H

#include "FinalCppSourceParts.h"

namespace acir::gfir {
struct StructuredProgram;
}

namespace acir::compiler {

mlir::FailureOr<FinalCppSourceParts>
emitBehavioralCpp(mlir::ModuleOp package, const gfir::StructuredProgram &program);

} // namespace acir::compiler
#endif
