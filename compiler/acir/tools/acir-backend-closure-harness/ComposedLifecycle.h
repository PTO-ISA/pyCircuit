#ifndef ACIR_COMPOSED_LIFECYCLE_H
#define ACIR_COMPOSED_LIFECYCLE_H
#include "Compiler/FinalProgram.h"
mlir::FailureOr<std::string>
executeComposedLifecycle(mlir::MLIRContext &context, uint64_t maxTicks,
                         acir::ac::detail::EmitError error);
#endif
