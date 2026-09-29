#ifndef ACIR_COMPOSED_FIXTURE_H
#define ACIR_COMPOSED_FIXTURE_H
#include "Compiler/FinalProgram.h"
mlir::FailureOr<acir::compiler::FinalProgram>
buildComposedFixture(mlir::MLIRContext &context, bool pipeline,
                     acir::ac::detail::EmitError error);
#endif
