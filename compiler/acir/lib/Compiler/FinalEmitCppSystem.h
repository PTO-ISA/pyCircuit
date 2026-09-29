#ifndef ACIR_LIB_COMPILER_FINALEMITCPPSYSTEM_H
#define ACIR_LIB_COMPILER_FINALEMITCPPSYSTEM_H
#include "FinalProgram.h"
#include "llvm/Support/raw_ostream.h"
namespace acir::compiler {
mlir::LogicalResult emitFinalCppSystem(const FinalProgram &program,
                                       llvm::raw_ostream &out,
                                       ac::detail::EmitError emitError);
}
#endif
