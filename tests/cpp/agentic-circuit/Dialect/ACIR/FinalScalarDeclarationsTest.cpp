#include "Compiler/FinalCppSourceParts.h"
#include "Compiler/FinalProgram.h"
#include "Dialect/ACIR/ACIRHardwareClosure.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/AsmParser/AsmParser.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace acir::compiler {
namespace {

class FinalScalarDeclarationsTest : public ::testing::Test {
protected:
  mlir::MLIRContext context;
  llvm::SmallString<256> directory;
  std::optional<FinalProgram> program;

  std::string child(llvm::StringRef name) const {
    llvm::SmallString<256> result(directory);
    llvm::sys::path::append(result, name);
    return result.str().str();
  }

  auto emitError() {
    return [&]() -> mlir::InFlightDiagnostic {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
  }

  void SetUp() override {
    context.loadDialect<ac::ACIRDialect, mlir::arith::ArithDialect>();
    ASSERT_FALSE(llvm::sys::fs::createUniqueDirectory(
        "final-scalar-declarations", directory));
    static constexpr llvm::StringLiteral script = R"py(
import subprocess
import sys
from pathlib import Path
repo, output, compiler = Path(sys.argv[1]), Path(sys.argv[2]), sys.argv[3]
sys.path[:0] = [str(repo / "python/semantic-core/src"),
                str(repo / "python/pycircuit/src"), str(repo)]
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport
root = output / "source"
root.mkdir()
sources = {
    "root.py": "from typing import Annotated\nfrom pycircuit import module\n"
               "Word = Annotated[int, range(256)]\nUnused = 7\n"
               "@module\ndef Root():\n    state: Word = 0\n",
    "empty.py": '"""Empty declaration source."""\n',
}
for relative, text in sources.items():
    source = root / relative
    source.write_text(text, encoding="utf-8")
    capture = _capture_source_file(source, source_root=root)
    stem = source.stem
    transport = output / (stem + ".transport.mlir")
    transport.write_text(_emit_source_transport(capture), encoding="utf-8")
    subprocess.run([compiler, "--capture", str(transport), "--package", "decl",
                    "--path", relative, "--body-out", str(output / (stem + ".ac")),
                    "--interface-out", str(output / (stem + ".interface.ac"))], check=True)
)py";
    std::vector<std::string> arguments = {
        ACIR_TEST_PYTHON,      "-c",
        script.str(),          ACIR_TEST_REPO_ROOT,
        directory.str().str(), ACIR_TEST_SOURCE_UNIT_HARNESS};
    llvm::SmallVector<llvm::StringRef> refs;
    for (const std::string &argument : arguments)
      refs.push_back(argument);
    const std::string log = child("source.log");
    const std::array<std::optional<llvm::StringRef>, 3> redirects = {
        std::nullopt, log, log};
    int status = llvm::sys::ExecuteAndWait(ACIR_TEST_PYTHON, refs, std::nullopt,
                                           redirects);
    auto capturedLog = llvm::MemoryBuffer::getFile(log);
    ASSERT_EQ(status, 0) << (capturedLog ? capturedLog.get()->getBuffer().str()
                                         : log);
    auto body =
        mlir::parseSourceFile<mlir::ModuleOp>(child("root.ac"), &context);
    auto header = mlir::parseSourceFile<mlir::ModuleOp>(
        child("root.interface.ac"), &context);
    auto emptyBody =
        mlir::parseSourceFile<mlir::ModuleOp>(child("empty.ac"), &context);
    auto emptyHeader = mlir::parseSourceFile<mlir::ModuleOp>(
        child("empty.interface.ac"), &context);
    ASSERT_TRUE(body && header && emptyBody && emptyHeader);
    llvm::SmallVector<SourceLinkUnit> units = {{*body, *header},
                                               {*emptyBody, *emptyHeader}};
    auto analysis = buildFinalProgram(units, emitError());
    ASSERT_TRUE(mlir::succeeded(analysis));
    auto ready = materializeFinalProgram(std::move(*analysis), emitError());
    ASSERT_TRUE(mlir::succeeded(ready));
    program.emplace(std::move(*ready));
    ASSERT_TRUE(mlir::succeeded(verifyFinalProgram(*program, emitError())));
  }

  void TearDown() override {
    program.reset();
    if (!directory.empty())
      llvm::sys::fs::remove_directories(directory);
  }

  void expectFrozenRejectionButFreshAcceptance() {
    // These edits remain valid final artifacts. Their rejection must come from
    // frozen-object integrity, not a malformed op or source-authentication
    // claim.
    ASSERT_TRUE(mlir::succeeded(mlir::verify(program->hardware())));
    ASSERT_TRUE(mlir::succeeded(ac::verifyFinalHardware(program->hardware())));
    EXPECT_TRUE(mlir::failed(verifyFinalProgram(*program, emitError())));
    EXPECT_TRUE(mlir::failed(emitFinalCpp(*program, emitError())));
    EXPECT_TRUE(mlir::failed(emitFinalVerilog(*program, emitError())));
    EXPECT_TRUE(mlir::failed(emitFinalCppSourceParts(*program, emitError())));
    std::string text;
    llvm::raw_string_ostream stream(text);
    program->hardware().print(stream);
    stream.flush();
    mlir::MLIRContext fresh;
    fresh.loadDialect<ac::ACIRDialect, mlir::arith::ArithDialect>();
    auto parsed = mlir::parseSourceString<mlir::ModuleOp>(text, &fresh);
    ASSERT_TRUE(parsed);
    auto error = [&]() -> mlir::InFlightDiagnostic {
      return mlir::emitError(mlir::UnknownLoc::get(&fresh));
    };
    auto reconstructed = buildFinalProgramFromHardware(*parsed, error);
    EXPECT_TRUE(mlir::succeeded(reconstructed));
  }
};

TEST_F(FinalScalarDeclarationsTest,
       FrozenValueEditIsRejectedWithoutInventingSourceAuthentication) {
  ac::ConstantOp declaration;
  program->hardware().walk([&](ac::ConstantOp op) {
    if (op.getSymName() == "decl.root.Unused")
      declaration = op;
  });
  ASSERT_TRUE(declaration) << "producer must retain unused owned constants";
  auto value = mlir::parseAttribute(
      R"attr({kind = "integer", value = #ac.math_int<8>})attr", &context);
  ASSERT_TRUE(value);
  declaration->setAttr("value", value);
  expectFrozenRejectionButFreshAcceptance();
}

TEST_F(FinalScalarDeclarationsTest,
       FrozenUnusedAliasDeletionIsRejectedButFreshFinalIsValid) {
  ac::TypeAliasOp declaration;
  program->hardware().walk([&](ac::TypeAliasOp op) {
    if (op.getSymName() == "decl.root.Word")
      declaration = op;
  });
  ASSERT_TRUE(declaration) << "producer must retain owned aliases";
  declaration.erase();
  expectFrozenRejectionButFreshAcceptance();
}

TEST_F(FinalScalarDeclarationsTest,
       FrozenEmptyUnitDeletionIsRejectedButFreshFinalIsValid) {
  mlir::ModuleOp empty;
  for (mlir::ModuleOp unit :
       program->hardware().getBody()->getOps<mlir::ModuleOp>()) {
    auto owner = unit->getAttrOfType<mlir::DictionaryAttr>("ac.source_owner");
    if (owner && owner.getAs<mlir::StringAttr>("path").getValue() == "empty.py")
      empty = unit;
  }
  ASSERT_TRUE(empty)
      << "producer must retain explicitly supplied empty sources";
  ASSERT_TRUE(empty.getBody()->empty());
  empty.erase();
  expectFrozenRejectionButFreshAcceptance();
}

} // namespace
} // namespace acir::compiler
