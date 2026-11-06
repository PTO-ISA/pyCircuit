#include "SourceUnit.h"

#include "PythonImportRecords.h"
#include "SourceRuleWrites.h"
#include "SourceMemoryBindings.h"
#include "pycircuit/Transforms/Passes.h"

#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;

namespace acir::compiler {

FailureOr<OwningOpRef<ModuleOp>>
compilePythonACIR(ModuleOp transport, DictionaryAttr sourceOwner,
                        const SourceHeaderRegistry &headers,
                        ac::detail::EmitError emitError) {
  if (failed(ac::detail::verifySourceOwner(sourceOwner, emitError)))
    return failure();
  OwningOpRef<ModuleOp> body(cast<ModuleOp>(transport->clone()));
  PassManager lowering(transport.getContext());
  lowering.addPass(detail::createAnalyzeRuleWritesPass());
  lowering.addPass(detail::createInferSourceBindingsPass(sourceOwner, headers));
  lowering.addPass(detail::createLowerPythonSourcePass(sourceOwner, headers));
  lowering.addPass(acir::createVerifyACIRPass());
  if (failed(lowering.run(*body)))
    return failure();
  return std::move(body);
}

FailureOr<SourceUnitArtifacts>
compilePythonSourceUnit(ModuleOp transport, DictionaryAttr sourceOwner,
                        const SourceHeaderRegistry &headers,
                        ac::detail::EmitError emitError,
                        bool retainSourceImport) {
  auto captured = compilePythonACIR(transport, sourceOwner, headers, emitError);
  if (failed(captured))
    return failure();
  auto body = std::move(*captured);
  std::optional<std::string> sourceImport;
  PassManager lowering(transport.getContext());
  acir::buildACIRToSourcePipeline(lowering);
  lowering.addPass(acir::createAnalyzeSourceMethodSummariesPass());
  if (failed(lowering.run(*body)))
    return failure();
  bool transactions = false;
  body->walk([&](Operation *operation) {
    if (isa<ac::SourceStateOp, ac::SourceStateReadOp, ac::SourceStateWriteOp,
            ac::SourceMethodOp, ac::SourceMethodCallOp,
            ac::SourceTransactionOp>(operation))
      transactions = true;
    if (auto imported = dyn_cast<ac::ModuleImportOp>(operation))
      transactions |= imported.getSemanticKind() == "transactions";
  });
  if (transactions && retainSourceImport)
    return emitError()
           << "source-import diagnostic requires lowered hardware; "
              "source transaction effects are retained until link";
  if (!transactions) {
    if (retainSourceImport) {
      sourceImport.emplace();
      llvm::raw_string_ostream stream(*sourceImport);
      body->print(stream, OpPrintingFlags().enableDebugInfo());
      stream << '\n';
    }
    PassManager hardware(transport.getContext());
    hardware.addPass(acir::createSimplifyRecordWiresPass());
    if (failed(hardware.run(*body)))
      return failure();
  }
  OwningOpRef<ModuleOp> interface(cast<ModuleOp>(body->clone()));
  PassManager pipeline(transport.getContext());
  pipeline.addPass(acir::createExtractSourceInterfacePass());
  if (failed(pipeline.run(*interface)) ||
      failed(headers.verifyBodySnapshots(*body, *interface, emitError)))
    return failure();
  return SourceUnitArtifacts{std::move(body), std::move(interface),
                             std::move(sourceImport)};
}

} // namespace acir::compiler
