#include "Compiler/SourceIdentifier.h"

#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/MLIRContext.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "gtest/gtest.h"

#include <array>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace acir::compiler::detail {
namespace {

bool acceptsIdentifier(llvm::StringRef name, bool binding) {
  mlir::MLIRContext context;
  mlir::ScopedDiagnosticHandler discard(
      &context, [](mlir::Diagnostic &) { return mlir::success(); });
  auto location = mlir::UnknownLoc::get(&context);
  auto emit = [location]() -> mlir::InFlightDiagnostic {
    return mlir::emitError(location);
  };
  return mlir::succeeded(verifyPythonAstIdentifier(name, binding, emit));
}

std::string readFile(llvm::StringRef path) {
  auto buffer = llvm::MemoryBuffer::getFile(path);
  EXPECT_TRUE(static_cast<bool>(buffer));
  return buffer ? buffer.get()->getBuffer().trim().str() : std::string{};
}

std::string runRecipePython(llvm::StringRef script,
                            llvm::ArrayRef<std::string> arguments) {
  llvm::SmallString<256> directory;
  EXPECT_FALSE(llvm::sys::fs::createUniqueDirectory("acir-oracle", directory));
  llvm::SmallString<256> output(directory);
  llvm::sys::path::append(output, "oracle.txt");
  std::vector<std::string> owned{ACIR_UNICODE_RECIPE_PYTHON, "-c",
                                 script.str()};
  owned.insert(owned.end(), arguments.begin(), arguments.end());
  llvm::SmallVector<llvm::StringRef> refs;
  for (const std::string &argument : owned)
    refs.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {
      std::nullopt, output.str(), output.str()};
  int status = llvm::sys::ExecuteAndWait(ACIR_UNICODE_RECIPE_PYTHON, refs,
                                         std::nullopt, redirects);
  EXPECT_EQ(status, 0) << readFile(output);
  std::string result = readFile(output);
  llvm::sys::fs::remove_directories(directory);
  return result;
}

bool pythonAccepts(llvm::StringRef name, bool binding) {
  static constexpr llvm::StringLiteral script = R"py(
import sys, unicodedata
s = sys.argv[1]
binding = sys.argv[2] == "1"
accepted = (s.isidentifier() and unicodedata.normalize("NFKC", s) == s
            and s not in {"None", "True", "False"}
            and not (binding and s == "__debug__"))
print("1" if accepted else "0")
)py";
  return runRecipePython(script, {name.str(), binding ? "1" : "0"}) == "1";
}

std::string encodeUtf8(uint32_t codepoint) {
  std::string result;
  if (codepoint <= 0x7F) {
    result.push_back(static_cast<char>(codepoint));
  } else if (codepoint <= 0x7FF) {
    result.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
    result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else {
    result.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
    result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
    result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  }
  return result;
}

TEST(SourceIdentifierOracleTest, RecipeVersionAndFixedAstCasesMatch) {
  static constexpr llvm::StringLiteral versionScript = R"py(
import sys, unicodedata
print(f"{sys.implementation.name}|{sys.version_info.major}.{sys.version_info.minor}.{sys.version_info.micro}|{unicodedata.unidata_version}")
)py";
  EXPECT_EQ(runRecipePython(versionScript, {}), "cpython|3.14.6|16.0.0");

  struct Case {
    const char *name;
    bool binding;
  };
  const Case cases[] = {{"q\xCC\x81", true},  {"\xC3\xA9", true},
                        {"e\xCC\x81", true},  {"class", true},
                        {"__debug__", false}, {"__debug__", true},
                        {"None", false}};
  for (const Case &test : cases) {
    SCOPED_TRACE(test.name);
    EXPECT_EQ(acceptsIdentifier(test.name, test.binding),
              pythonAccepts(test.name, test.binding));
  }

  static constexpr llvm::StringLiteral astScript = R"py(
import ast, sys
tree = ast.parse(sys.argv[1] + " = 1\n", mode="exec")
compile(tree, "<identifier-oracle>", "exec")
print(tree.body[0].targets[0].id)
)py";
  EXPECT_EQ(runRecipePython(astScript, {"q\xCC\x81"}), "q\xCC\x81");
  EXPECT_EQ(runRecipePython(astScript, {"e\xCC\x81"}), "\xC3\xA9");
  EXPECT_EQ(
      runRecipePython(
          astScript,
          {"\xEF\xBD\x83\xEF\xBD\x8C\xEF\xBD\x81\xEF\xBD\x93\xEF\xBD\x93"}),
      "class");
}

TEST(SourceIdentifierOracleTest, BoundedBatchHasNoMismatches) {
  const std::string identifiers[] = {
      "_",          "_private",      "alpha9",       "\xCE\xB1",
      "\xD0\x96",   "\xE4\xB8\xAD",  "\xEA\xB0\x81", "\xF0\x90\x90\xA8",
      "x\xCC\xA7",  "x\xE2\x80\x8C", "9start",       "has-dash",
      "with space", "\xEF\xBC\xA1",  "\xC2\xB5",     "\xE2\x84\xAA",
  };
  for (const std::string &name : identifiers)
    for (bool binding : {false, true}) {
      SCOPED_TRACE(name);
      EXPECT_EQ(acceptsIdentifier(name, binding), pythonAccepts(name, binding));
    }
}

TEST(SourceIdentifierOracleTest, ExhaustiveBmpSingleCharactersMatch) {
  static constexpr llvm::StringLiteral script = R"py(
import unicodedata
accepted = []
for cp in range(0x10000):
    if 0xD800 <= cp <= 0xDFFF:
        continue
    s = chr(cp)
    if s.isidentifier() and unicodedata.normalize("NFKC", s) == s:
        accepted.append(f"{cp:x}")
print(",".join(accepted))
)py";
  std::set<uint32_t> expected;
  llvm::SmallVector<llvm::StringRef> fields;
  std::string ownedOutput = runRecipePython(script, {});
  llvm::StringRef(ownedOutput).split(fields, ',', -1, false);
  for (llvm::StringRef field : fields) {
    uint32_t value = 0;
    ASSERT_FALSE(field.getAsInteger(16, value));
    expected.insert(value);
  }
  for (uint32_t codepoint = 0; codepoint < 0x10000; ++codepoint) {
    if (codepoint >= 0xD800 && codepoint <= 0xDFFF)
      continue;
    ASSERT_EQ(acceptsIdentifier(encodeUtf8(codepoint), false),
              expected.contains(codepoint))
        << "codepoint " << codepoint;
  }
}

} // namespace
} // namespace acir::compiler::detail
