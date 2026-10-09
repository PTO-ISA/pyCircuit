#ifndef ACIR_LIB_COMPILER_PYTHONIMPORTRECORDS_H
#define ACIR_LIB_COMPILER_PYTHONIMPORTRECORDS_H

#include "PythonImportAST.h"
#include "SourceUnit.h"
#include "mlir/Pass/Pass.h"
#include <memory>

namespace acir::compiler::detail {

std::unique_ptr<mlir::Pass>
createLowerPythonSourcePass(mlir::DictionaryAttr owner,
                            const SourceHeaderRegistry &headers);

} // namespace acir::compiler::detail

#endif // ACIR_LIB_COMPILER_PYTHONIMPORTRECORDS_H
