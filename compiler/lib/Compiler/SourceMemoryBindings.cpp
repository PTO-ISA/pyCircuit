#include "SourceMemoryBindings.h"
#include "SourceDeclarationContext.h"
#include "SourceRuleWrites.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/OperationSupport.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;
namespace acir::compiler::detail {
SourceMemoryBindingsAnalysis::SourceMemoryBindingsAnalysis(Operation *operation)
    : operation(operation), capture(operation->getAttr("ac.python_capture")) {}
SourceMemoryBindingsAnalysis::~SourceMemoryBindingsAnalysis() = default;
bool SourceMemoryBindingsAnalysis::matchesInputs(
    DictionaryAttr owner, const SourceHeaderRegistry &headers) const {
  if (!inputs || inputs->owner != owner ||
      operation->getAttr("ac.python_capture") != capture ||
      inputs->headerStorage.size() != headers.suppliedHeaders().size())
    return false;
  for (auto [snapshot, current] : llvm::zip(inputs->headerStorage,
                                           headers.suppliedHeaders())) {
    ModuleOp currentModule = current;
    if (!OperationEquivalence::isEquivalentTo(
            (*snapshot).getOperation(), currentModule.getOperation(),
            OperationEquivalence::IgnoreLocations))
      return false;
  }
  return true;
}
LogicalResult SourceMemoryBindingsAnalysis::initialize(
    DictionaryAttr owner, const SourceHeaderRegistry &headers,
    const SourceRuleWritesAnalysis &writes) {
  auto error = [&] { return operation->emitError(); };
  if (initialized) {
    if (!matchesInputs(owner, headers))
      return error() << "source binding analysis input identity changed";
    return success(valid);
  }
  initialized = true;
  // A valid capture plan may still require lowered owner-enable proofs.
  if (!writes.isPlanValid() || failed(ac::detail::verifySourceOwner(owner, error)))
    return failure();
  auto source = readSingleCapture(cast<ModuleOp>(operation), error);
  if (failed(source))
    return failure();
  if (owner.getAs<StringAttr>("path").getValue() != source->path)
    return error() << "source owner path must equal the captured source path";
  inputs = std::make_unique<SourceCompilationInputs>();
  inputs->owner = owner;
  SmallVector<ModuleOp> views;
  for (ModuleOp header : headers.suppliedHeaders()) {
    inputs->headerStorage.emplace_back(cast<ModuleOp>(header->clone()));
    views.push_back(*inputs->headerStorage.back());
  }
  auto registry = SourceHeaderRegistry::create(operation->getContext(), views, error);
  if (failed(registry))
    return failure();
  inputs->headers = std::make_unique<SourceHeaderRegistry>(std::move(*registry));
  declarations = createSourceDeclarationContext(*source, *inputs, writes, operation);
  valid = succeeded(declarations->prepare());
  return success(valid);
}
ArrayRef<MemoryCallPlan> SourceMemoryBindingsAnalysis::memoryCalls() const {
  return valid ? declarations->memoryCalls() : ArrayRef<MemoryCallPlan>();
}
ArrayRef<ModuleDomainPlan> SourceMemoryBindingsAnalysis::moduleDomains() const {
  return valid ? declarations->moduleDomains() : ArrayRef<ModuleDomainPlan>();
}
FailureOr<OwningOpRef<ModuleOp>> SourceMemoryBindingsAnalysis::lower(
    DictionaryAttr owner, const SourceHeaderRegistry &headers) {
  if (!initialized || !valid || consumed || !matchesInputs(owner, headers))
    return operation->emitError()
           << "source lowering requires the initialized, unchanged binding plan";
  consumed = true;
  return declarations->consume();
}
namespace {
class InferSourceBindingsPass
    : public PassWrapper<InferSourceBindingsPass, OperationPass<ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(InferSourceBindingsPass)
  InferSourceBindingsPass() = default;
  InferSourceBindingsPass(DictionaryAttr owner, const SourceHeaderRegistry &headers)
      : owner(owner), headers(&headers) {}
  InferSourceBindingsPass(const InferSourceBindingsPass &other)
      : PassWrapper(other), owner(other.owner), headers(other.headers) {}
  StringRef getArgument() const final { return "ac-infer-source-bindings"; }
  StringRef getDescription() const final {
    return "Resolve source allocation geometry, bindings and hidden domains";
  }
  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<ac::ACIRDialect>();
  }
  void runOnOperation() override {
    auto operation = getOperation();
    auto error = [&] { return operation.emitError(); };
    SmallVector<OwningOpRef<ModuleOp>> ownedHeaders;
    std::optional<SourceHeaderRegistry> standaloneRegistry;
    DictionaryAttr admittedOwner = owner;
    const SourceHeaderRegistry *admittedHeaders = headers;
    if (!admittedHeaders) {
      if (package.getValue().empty() || sourcePath.getValue().empty()) {
        error() << "standalone source inference requires package and source-path";
        signalPassFailure();
        return;
      }
      OpBuilder b(operation.getContext());
      admittedOwner = b.getDictionaryAttr({
          b.getNamedAttr("package", b.getStringAttr(package.getValue())),
          b.getNamedAttr("path", b.getStringAttr(sourcePath.getValue()))});
      SmallVector<ModuleOp> views;
      for (const std::string &path : headerFiles) {
        auto parsed = parseSourceFile<ModuleOp>(path, operation.getContext());
        if (!parsed) {
          signalPassFailure();
          return;
        }
        views.push_back(*parsed);
        ownedHeaders.push_back(std::move(parsed));
      }
      auto registry = SourceHeaderRegistry::create(operation.getContext(), views, error);
      if (failed(registry)) {
        signalPassFailure();
        return;
      }
      standaloneRegistry.emplace(std::move(*registry));
      admittedHeaders = &*standaloneRegistry;
    }
    auto &analysis = getAnalysis<SourceMemoryBindingsAnalysis>();
    if (failed(analysis.initialize(admittedOwner, *admittedHeaders,
                                  getAnalysis<SourceRuleWritesAnalysis>()))) {
      signalPassFailure();
      return;
    }
    if (reportPlan) {
      for (const auto &plan : analysis.memoryCalls())
        emitRemark(plan.call.location(operation.getContext(), sourcePath.getValue().empty()
                                          ? admittedOwner.getAs<StringAttr>("path").getValue()
                                          : StringRef(sourcePath.getValue())))
            << "inferred memory " << plan.callee << " depth=" << plan.depth
            << " address-width=" << plan.addressWidth
            << " payload-width=" << plan.payloadWidth
            << " strobe-width=" << plan.strobeWidth;
      for (const auto &plan : analysis.moduleDomains())
        operation.emitRemark() << "inferred module " << plan.symbol
                               << " domain=" << plan.needsDomain
                               << " signature=" << plan.signature;
    }
    markAllAnalysesPreserved();
  }
private:
  DictionaryAttr owner;
  const SourceHeaderRegistry *headers = nullptr;
  Option<std::string> package{*this, "package", llvm::cl::desc("Source package")};
  Option<std::string> sourcePath{*this, "source-path", llvm::cl::desc("Source owner path")};
  ListOption<std::string> headerFiles{*this, "headers", llvm::cl::desc("Published interface paths")};
  Option<bool> reportPlan{*this, "report-plan", llvm::cl::init(false)};
};
} // namespace
std::unique_ptr<Pass> createInferSourceBindingsPass(
    DictionaryAttr owner, const SourceHeaderRegistry &headers) {
  return std::make_unique<InferSourceBindingsPass>(owner, headers);
}
void registerSourceBindingsPass() {
  registerPass([] { return std::make_unique<InferSourceBindingsPass>(); });
}
} // namespace acir::compiler::detail
