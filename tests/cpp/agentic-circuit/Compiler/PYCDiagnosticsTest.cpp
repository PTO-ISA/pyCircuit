#include "acir/Compiler/Driver.h"
#include "pyc/Support/Diagnostics.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/MLIRContext.h"
#include "gtest/gtest.h"

#include <set>
#include <string>

namespace {

TEST(PYCDiagnosticsTest, RegistryContainsNoConflictingCodes) {
  std::set<llvm::StringRef> unique;
  for (llvm::StringRef code : pyc::registeredDiagnosticCodes()) {
    EXPECT_TRUE(code.starts_with("PYC"));
    EXPECT_TRUE(unique.insert(code).second) << "duplicate code: " << code.str();
  }
  EXPECT_TRUE(pyc::isRegisteredDiagnosticCode("PYC926"));
  EXPECT_TRUE(pyc::isRegisteredDiagnosticCode("PYC932"));
}

TEST(PYCDiagnosticsTest, StructuredCodeDoesNotDependOnRenderedMessage) {
  mlir::MLIRContext context;
  std::optional<std::string> capturedCode;
  mlir::ScopedDiagnosticHandler handler(
      &context, [&](mlir::Diagnostic &diagnostic) {
        capturedCode =
            acir::compiler::detail::diagnosticCodeFromMetadata(diagnostic);
        EXPECT_NE(diagnostic.str().find("AC-FAKE-999"), std::string::npos);
        return mlir::success();
      });

  mlir::ModuleOp module =
      mlir::ModuleOp::create(mlir::UnknownLoc::get(&context));
  pyc::emitError(module, "PYC932")
      << "rendered text contains misleading AC-FAKE-999";

  ASSERT_TRUE(capturedCode.has_value());
  EXPECT_EQ(*capturedCode, "PYC932");
}

TEST(PYCDiagnosticsTest, DiagnosticWithoutMetadataHasNoStructuredCode) {
  mlir::MLIRContext context;
  mlir::Diagnostic diagnostic(mlir::UnknownLoc::get(&context),
                              mlir::DiagnosticSeverity::Error);
  diagnostic << "AC-MESSAGE-ONLY-123 must not be parsed";
  EXPECT_FALSE(acir::compiler::detail::diagnosticCodeFromMetadata(diagnostic)
                   .has_value());
}

} // namespace
