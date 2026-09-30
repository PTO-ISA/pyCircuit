#ifndef ACIR_LIB_COMPILER_FINALEMITVERILOG_H
#define ACIR_LIB_COMPILER_FINALEMITVERILOG_H

#include "FinalProgram.h"
#include "FinalVerilogSourceParts.h"

namespace acir::compiler {

mlir::FailureOr<FinalVerilogEmission>
emitFinalVerilogPartsBody(const FinalProgram &program,
                          ac::detail::EmitError emitError);

mlir::FailureOr<FinalVerilogSourceParts>
emitFinalVerilogSourcePartsBody(const FinalProgram &program,
                                ac::detail::EmitError emitError);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_FINALEMITVERILOG_H
