#include "SourceUnit.h"
#include "PythonImportRecords.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "pycircuit/Transforms/Passes.h"
#include "llvm/Support/raw_ostream.h"
using namespace mlir;
namespace acir::compiler {
FailureOr<SourceUnitArtifacts>
compilePythonSourceUnit(ModuleOp transport, DictionaryAttr sourceOwner,
                        const SourceHeaderRegistry &headers,
                        ac::detail::EmitError emitError,
                        bool retainSourceImport) {
  auto imported =
      importPythonSource(transport, sourceOwner, headers, emitError);
  if (failed(imported))
    return failure();
  OwningOpRef<ModuleOp> body = std::move(*imported);
  std::optional<std::string> capture;
  if (retainSourceImport) {
    capture.emplace();
    llvm::raw_string_ostream stream(*capture);
    body->print(stream, OpPrintingFlags().enableDebugInfo());
    stream << '\n';
  }
  OwningOpRef<ModuleOp> interface(cast<ModuleOp>(body->clone()));
  PassManager pipeline(transport.getContext());
  pipeline.addPass(acir::createExtractSourceInterfacePass());
  if (failed(pipeline.run(*interface)) ||
      failed(headers.verifyBodySnapshots(*body, *interface, emitError)))
    return failure();
  return SourceUnitArtifacts{std::move(body), std::move(interface),
                             std::move(capture)};
}
} // namespace acir::compiler
