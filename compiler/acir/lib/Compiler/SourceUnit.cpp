#include "SourceUnit.h"

#include "PythonImportAST.h"
#include "PythonImportRecords.h"

using namespace mlir;

namespace acir::compiler {

FailureOr<SourceUnitArtifacts>
compilePythonSourceUnit(ModuleOp transport, DictionaryAttr sourceOwner,
                        const SourceHeaderRegistry &headers,
                        ac::detail::EmitError emitError) {
  if (failed(ac::detail::verifySourceOwner(sourceOwner, emitError)))
    return failure();
  auto captured = detail::readSingleCapture(transport, emitError);
  if (failed(captured))
    return failure();
  auto result =
      detail::lowerRecordSourceUnit(*captured, sourceOwner, headers, emitError);
  if (failed(result) || failed(headers.verifyBodySnapshots(
                            *result->body, *result->interface, emitError)))
    return failure();
  return result;
}

} // namespace acir::compiler
