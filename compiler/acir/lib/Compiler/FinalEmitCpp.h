#ifndef ACIR_LIB_COMPILER_FINALEMITCPP_H
#define ACIR_LIB_COMPILER_FINALEMITCPP_H

#include "FinalProgram.h"

namespace acir::compiler {

mlir::FailureOr<std::string> emitFinalCppBody(const FinalProgram &program,
                                              ac::detail::EmitError emitError);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_FINALEMITCPP_H
