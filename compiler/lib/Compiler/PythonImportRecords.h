#ifndef ACIR_LIB_COMPILER_PYTHONIMPORTRECORDS_H
#define ACIR_LIB_COMPILER_PYTHONIMPORTRECORDS_H
#include "SourceUnit.h"
namespace acir::compiler {
mlir::FailureOr<mlir::OwningOpRef<mlir::ModuleOp>>
importPythonSource(mlir::ModuleOp transport, mlir::DictionaryAttr sourceOwner,
                   const SourceHeaderRegistry &headers,
                   ac::detail::EmitError emitError);
}
#endif
