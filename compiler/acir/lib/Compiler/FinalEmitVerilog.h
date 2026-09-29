#ifndef ACIR_LIB_COMPILER_FINALEMITVERILOG_H
#define ACIR_LIB_COMPILER_FINALEMITVERILOG_H

#include "FinalProgram.h"

namespace acir::compiler {

mlir::FailureOr<std::string>
emitFinalVerilogBody(const FinalProgram &program,
                     ac::detail::EmitError emitError);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_FINALEMITVERILOG_H
