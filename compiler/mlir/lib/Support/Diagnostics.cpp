#include "pyc/Support/Diagnostics.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Operation.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/ErrorHandling.h"

namespace pyc {
namespace {

constexpr llvm::StringLiteral kCodes[] = {
    "PYC901", "PYC902", "PYC903", "PYC904", "PYC905", "PYC906", "PYC907",
    "PYC908", "PYC909", "PYC910", "PYC911", "PYC912", "PYC913", "PYC914",
    "PYC915", "PYC916", "PYC917", "PYC918", "PYC920", "PYC921", "PYC922",
    "PYC923", "PYC924", "PYC925", "PYC926", "PYC927", "PYC928", "PYC929",
    "PYC930", "PYC931", "PYC932", "PYC940", "PYC941", "PYC942", "PYC943",
    "PYC944", "PYC945", "PYC946", "PYC947", "PYC951", "PYC952", "PYC953",
    "PYC954", "PYC955", "PYC956", "PYC957", "PYC958", "PYC959", "PYC960",
    "PYC961", "PYC962", "PYC963", "PYC964", "PYC965", "PYC966", "PYC967",
    "PYC968", "PYC969", "PYC970", "PYC971", "PYC972", "PYC973", "PYC974",
    "PYC975", "PYC976", "PYC977", "PYC978", "PYC979", "PYC980", "PYC981",
    "PYC982", "PYC983", "PYC984", "PYC985", "PYC986", "PYC987", "PYC988",
    "PYC989", "PYC990", "PYC991", "PYC992", "PYC993",
};

} // namespace

llvm::ArrayRef<llvm::StringLiteral> registeredDiagnosticCodes() {
  return kCodes;
}

bool isRegisteredDiagnosticCode(llvm::StringRef code) {
  return llvm::is_contained(registeredDiagnosticCodes(), code);
}

mlir::InFlightDiagnostic emitError(mlir::Operation *op, llvm::StringRef code) {
  if (!isRegisteredDiagnosticCode(code))
    llvm::report_fatal_error("PYC diagnostic code is not registered: " + code);
  mlir::InFlightDiagnostic diagnostic = op->emitError();
  diagnostic << "[" << code << "] ";

  mlir::Builder builder(op->getContext());
  mlir::NamedAttribute codeField(builder.getStringAttr(kDiagnosticCodeMetadata),
                                 builder.getStringAttr(code));
  diagnostic.getUnderlyingDiagnostic()->getMetadata().emplace_back(
      mlir::DictionaryAttr::get(op->getContext(), {codeField}));
  return diagnostic;
}

} // namespace pyc
