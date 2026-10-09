#include "SourceUnit.h"

#include "PythonImportRecords.h"
#include "SourceRuleWrites.h"
#include "SourceMemoryBindings.h"
#include "pycircuit/Transforms/Passes.h"

#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassInstrumentation.h"
#include "mlir/Pass/PassManager.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;

namespace acir::compiler {
namespace {

struct SourceImportCapture {
  unsigned count = 0;
  std::optional<std::string> text;
};

class SourceImportInstrumentation final : public PassInstrumentation {
public:
  SourceImportInstrumentation(Pass *simplify, Operation *body,
                              SourceImportCapture &capture)
      : simplify(simplify), body(body), capture(capture) {}

  void runBeforePass(Pass *pass, Operation *operation) override {
    if (pass != simplify || operation != body)
      return;
    if (++capture.count != 1)
      return;
    capture.text.emplace();
    llvm::raw_string_ostream stream(*capture.text);
    operation->print(stream, OpPrintingFlags().enableDebugInfo());
    stream << '\n';
  }

private:
  Pass *simplify;
  Operation *body;
  SourceImportCapture &capture;
};

} // namespace

FailureOr<SourceUnitArtifacts>
compilePythonSourceUnit(ModuleOp transport, DictionaryAttr sourceOwner,
                        const SourceHeaderRegistry &headers,
                        ac::detail::EmitError emitError,
                        bool retainSourceImport) {
  if (failed(ac::detail::verifySourceOwner(sourceOwner, emitError)))
    return failure();
  OwningOpRef<ModuleOp> body(cast<ModuleOp>(transport->clone()));
  // Capture state outlives both the run and the manager's instrumentation.
  SourceImportCapture capture;
  PassManager lowering(transport.getContext());
  lowering.addPass(detail::createAnalyzeRuleWritesPass());
  lowering.addPass(detail::createInferSourceBindingsPass(sourceOwner, headers));
  lowering.addPass(detail::createLowerPythonSourcePass(sourceOwner, headers));
  auto simplify = acir::createSimplifyRecordWiresPass();
  if (retainSourceImport)
    lowering.addInstrumentation(std::make_unique<SourceImportInstrumentation>(
        simplify.get(), body->getOperation(), capture));
  lowering.addPass(std::move(simplify));
  if (failed(lowering.run(*body)))
    return failure();
  if (retainSourceImport && (capture.count != 1 || !capture.text))
    return emitError() << "source-import diagnostic boundary was not captured "
                          "exactly once";
  OwningOpRef<ModuleOp> interface(cast<ModuleOp>(body->clone()));
  PassManager pipeline(transport.getContext());
  pipeline.addPass(acir::createExtractSourceInterfacePass());
  if (failed(pipeline.run(*interface)) ||
      failed(headers.verifyBodySnapshots(*body, *interface, emitError)))
    return failure();
  return SourceUnitArtifacts{std::move(body), std::move(interface),
                             std::move(capture.text)};
}

} // namespace acir::compiler
