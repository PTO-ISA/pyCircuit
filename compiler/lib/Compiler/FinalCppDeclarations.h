#ifndef ACIR_LIB_COMPILER_FINALCPPDECLARATIONS_H
#define ACIR_LIB_COMPILER_FINALCPPDECLARATIONS_H

#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "mlir/IR/Operation.h"
#include "llvm/ADT/FunctionExtras.h"
#include "llvm/ADT/StringRef.h"

#include <string>

namespace acir::compiler {

mlir::FailureOr<std::string>
emitCppValueDeclaration(mlir::Operation *operation, llvm::StringRef cppName,
                         llvm::StringRef ownerText,
                         ac::detail::EmitError emitError);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_FINALCPPDECLARATIONS_H
