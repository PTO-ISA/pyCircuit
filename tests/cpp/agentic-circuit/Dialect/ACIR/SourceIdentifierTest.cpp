#include "Compiler/SourceIdentifier.h"

#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/MLIRContext.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <string>

namespace acir::compiler::detail {
namespace {

struct CheckResult {
  bool accepted;
  std::string diagnostic;
};

CheckResult checkIdentifier(llvm::StringRef name, bool binding) {
  mlir::MLIRContext context;
  std::string diagnostic;
  mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &value) {
    llvm::raw_string_ostream stream(diagnostic);
    value.print(stream);
    return mlir::success();
  });
  auto location = mlir::UnknownLoc::get(&context);
  auto emit = [location]() -> mlir::InFlightDiagnostic {
    return mlir::emitError(location);
  };
  return {mlir::succeeded(verifyPythonAstIdentifier(name, binding, emit)),
          diagnostic};
}

TEST(SourceIdentifierTest, FixedUnicode16CasesHaveExpectedResults) {
  struct Case {
    const char *name;
    bool binding;
    bool expected;
  };
  const Case cases[] = {
      {"ascii_name", true, true},
      {"q\xCC\x81", true, true},          // q + COMBINING ACUTE
      {"\xC3\xA9", true, true},           // NFC e-acute
      {"e\xCC\x81", true, false},         // canonically decomposed
      {"q\xCC\x95\xCC\x80", true, false}, // reordered combining marks
      {"\xEF\xBD\x83\xEF\xBD\x8C\xEF\xBD\x81\xEF\xBD\x93\xEF\xBD\x93", true,
       false},                              // fullwidth class
      {"\xEA\xB0\x80", true, true},         // Hangul syllable GA
      {"\xF0\x90\x90\x80name", true, true}, // DESERET CAPITAL LONG I
      {"class", true, true},
      {"__debug__", false, true},
      {"__debug__", true, false},
      {"None", false, false},
      {"True", false, false},
      {"False", false, false},
  };
  for (const Case &test : cases) {
    SCOPED_TRACE(test.name);
    CheckResult result = checkIdentifier(test.name, test.binding);
    EXPECT_EQ(result.accepted, test.expected) << result.diagnostic;
    if (!test.expected)
      EXPECT_FALSE(result.diagnostic.empty());
  }
}

TEST(SourceIdentifierTest, MalformedUtf8IsRejectedWithDiagnostics) {
  const std::string malformed[] = {
      std::string("\xC0\xAF", 2),     // overlong slash
      std::string("\xED\xA0\x80", 3), // encoded surrogate U+D800
      std::string("\xF4\x90\x80\x80", 4),
      std::string("\xE2\x82", 2),
      std::string("\x80", 1),
  };
  for (const std::string &name : malformed) {
    CheckResult result = checkIdentifier(name, true);
    EXPECT_FALSE(result.accepted);
    EXPECT_FALSE(result.diagnostic.empty());
  }
}

} // namespace
} // namespace acir::compiler::detail
