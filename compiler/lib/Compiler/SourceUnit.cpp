#include "SourceUnit.h"

#include "PythonImportRecords.h"
#include "SourceRuleWrites.h"
#include "SourceMemoryBindings.h"
#include "pycircuit/Transforms/Passes.h"

#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"

using namespace mlir;

namespace acir::compiler {

FailureOr<SourceUnitArtifacts>
compilePythonSourceUnit(ModuleOp transport, DictionaryAttr sourceOwner,
                        const SourceHeaderRegistry &headers,
                        ac::detail::EmitError emitError) {
  if (failed(ac::detail::verifySourceOwner(sourceOwner, emitError)))
    return failure();
  OwningOpRef<ModuleOp> body(cast<ModuleOp>(transport->clone()));
  PassManager lowering(transport.getContext());
  lowering.addPass(detail::createAnalyzeRuleWritesPass());
  lowering.addPass(detail::createInferSourceBindingsPass(sourceOwner, headers));
  lowering.addPass(detail::createLowerPythonSourcePass(sourceOwner, headers));
  lowering.addPass(acir::createSimplifyRecordWiresPass());
  if (failed(lowering.run(*body)))
    return failure();
  OwningOpRef<ModuleOp> interface(cast<ModuleOp>(body->clone()));
  PassManager pipeline(transport.getContext());
  pipeline.addPass(acir::createExtractSourceInterfacePass());
  if (failed(pipeline.run(*interface)) ||
      failed(headers.verifyBodySnapshots(*body, *interface, emitError)))
    return failure();
  return SourceUnitArtifacts{std::move(body), std::move(interface)};
}

} // namespace acir::compiler
