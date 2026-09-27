#ifndef ACIR_LIB_COMPILER_PYTHONIMPORTRECORDS_H
#define ACIR_LIB_COMPILER_PYTHONIMPORTRECORDS_H

#include "PythonImportAST.h"
#include "SourceUnit.h"

namespace acir::compiler::detail {

mlir::FailureOr<SourceUnitArtifacts>
lowerRecordSourceUnit(const CapturedSource &source, mlir::DictionaryAttr owner,
                      const SourceHeaderRegistry &headers,
                      ac::detail::EmitError emitError);

} // namespace acir::compiler::detail

#endif // ACIR_LIB_COMPILER_PYTHONIMPORTRECORDS_H
