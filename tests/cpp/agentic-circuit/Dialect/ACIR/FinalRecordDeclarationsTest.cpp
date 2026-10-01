#include "Compiler/FinalCppSourceParts.h"
#include "Compiler/FinalProgram.h"
#include "Compiler/FinalVerilogSourceParts.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Attributes.h"
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

class FinalRecordDeclarationsTest : public ::testing::Test {
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
    context.loadDialect<ac::ACIRDialect, mlir::arith::ArithDialect,
                        mlir::func::FuncDialect>();
    ASSERT_FALSE(llvm::sys::fs::createUniqueDirectory(
        "final-record-declarations", directory));
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
    "provider.py": "from typing import Annotated\n"
                  "Word = Annotated[int, range(256)]\n"
                  "class Pair:\n    lo: Word\n    hi: bool\n"
                  "    def __init__(self, hi: bool = False, lo: Word = 3):\n"
                  "        self.lo = lo\n        self.hi = hi\n",
    "empty.py": '"""Empty declaration source."""\n',
    "root.py": "from .provider import Pair, Word\n"
               "from pycircuit import module\n"
               "@module\ndef Root():\n    state: Word = 0\n",
}
interfaces = {}
for relative, text in sources.items():
    source = root / relative
    source.write_text(text, encoding="utf-8")
    capture = _capture_source_file(source, source_root=root)
    stem = source.stem
    transport = output / (stem + ".transport.mlir")
    transport.write_text(_emit_source_transport(capture), encoding="utf-8")
    command = [compiler, "--capture", str(transport), "--package", "decl",
               "--path", relative]
    if relative == "root.py":
        command += ["--header", interfaces["provider.py"]]
    command += ["--body-out", str(output / (stem + ".ac")),
                "--interface-out", str(output / (stem + ".interface.ac"))]
    subprocess.run(command, check=True)
    interfaces[relative] = str(output / (stem + ".interface.ac"))
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
    auto provider =
        mlir::parseSourceFile<mlir::ModuleOp>(child("provider.ac"), &context);
    auto providerHeader = mlir::parseSourceFile<mlir::ModuleOp>(
        child("provider.interface.ac"), &context);
    auto empty =
        mlir::parseSourceFile<mlir::ModuleOp>(child("empty.ac"), &context);
    auto emptyHeader = mlir::parseSourceFile<mlir::ModuleOp>(
        child("empty.interface.ac"), &context);
    ASSERT_TRUE(body && header && provider && providerHeader && empty &&
                emptyHeader);
    llvm::SmallVector<SourceLinkUnit> units = {
        {*provider, *providerHeader}, {*empty, *emptyHeader}, {*body, *header}};
    auto analysis = buildFinalProgram(units, emitError());
    ASSERT_TRUE(mlir::succeeded(analysis));
    EXPECT_EQ(analysis->state(), FinalProgramState::AnalysisClosed);
    EXPECT_TRUE(mlir::succeeded(verifyFinalProgram(*analysis, emitError())));
    EXPECT_TRUE(mlir::failed(emitFinalCpp(*analysis, emitError())));
    EXPECT_TRUE(mlir::failed(emitFinalVerilog(*analysis, emitError())));
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

  mlir::LogicalResult freshVerify() {
    std::string text;
    llvm::raw_string_ostream stream(text);
    program->hardware().print(stream);
    stream.flush();
    mlir::MLIRContext fresh;
    fresh.loadDialect<ac::ACIRDialect, mlir::arith::ArithDialect>();
    auto parsed = mlir::parseSourceString<mlir::ModuleOp>(text, &fresh);
    if (!parsed)
      return mlir::failure();
    auto error = [&]() -> mlir::InFlightDiagnostic {
      return mlir::emitError(mlir::UnknownLoc::get(&fresh));
    };
    auto reconstructed = buildFinalProgramFromHardware(*parsed, error);
    return mlir::succeeded(reconstructed) ? mlir::success() : mlir::failure();
  }

  void expectFrozenEditButValidStandaloneFinal() {
    ASSERT_TRUE(mlir::succeeded(mlir::verify(program->hardware())));
    EXPECT_TRUE(mlir::failed(verifyFinalProgram(*program, emitError())));
    EXPECT_TRUE(mlir::failed(emitFinalCpp(*program, emitError())));
    EXPECT_TRUE(mlir::failed(emitFinalVerilog(*program, emitError())));
    EXPECT_TRUE(mlir::failed(emitFinalCppSourceParts(*program, emitError())));
    EXPECT_TRUE(mlir::failed(emitFinalVerilogParts(*program, emitError())));
    EXPECT_TRUE(
        mlir::failed(emitFinalVerilogSourceParts(*program, emitError())));
    EXPECT_TRUE(mlir::succeeded(freshVerify()));
  }
};

TEST_F(FinalRecordDeclarationsTest,
       FrozenUnusedRecordFieldRenameIsRejectedButFreshFinalIsValid) {
  ac::StructOp record;
  program->hardware().walk([&](ac::StructOp op) {
    if (op.getSymName() == "decl.provider.Pair")
      record = op;
  });
  ASSERT_TRUE(record) << "all source-owned records must survive projection";
  auto fields = record.getFields();
  ASSERT_EQ(fields.size(), 2u);
  auto first = mlir::cast<mlir::DictionaryAttr>(fields[0]);
  mlir::Builder builder(&context);
  llvm::SmallVector<mlir::NamedAttribute> changedFirst;
  for (mlir::NamedAttribute attribute : first)
    changedFirst.push_back(
        attribute.getName() == "name"
            ? builder.getNamedAttr("name", builder.getStringAttr("renamed"))
            : attribute);
  llvm::SmallVector<mlir::Attribute> changedFields = {
      builder.getDictionaryAttr(changedFirst), fields[1]};
  record->setAttr("fields", builder.getArrayAttr(changedFields));
  expectFrozenEditButValidStandaloneFinal();
}

TEST_F(FinalRecordDeclarationsTest,
       FrozenUnusedRecordDeletionIsRejectedButFreshFinalIsValid) {
  ac::StructOp record;
  program->hardware().walk([&](ac::StructOp op) {
    if (op.getSymName() == "decl.provider.Pair")
      record = op;
  });
  ASSERT_TRUE(record);
  record.erase();
  expectFrozenEditButValidStandaloneFinal();
}

TEST_F(FinalRecordDeclarationsTest,
       FrozenRecordOperationLocationEditIsRejectedButFreshFinalIsValid) {
  ac::StructOp record;
  program->hardware().walk([&](ac::StructOp op) {
    if (op.getSymName() == "decl.provider.Pair")
      record = op;
  });
  ASSERT_TRUE(record);
  record->setLoc(mlir::UnknownLoc::get(&context));
  expectFrozenEditButValidStandaloneFinal();
}

TEST_F(FinalRecordDeclarationsTest,
       FrozenRecordConstructorMutationIsRejectedByFrozenAndFreshReaders) {
  ac::StructOp record;
  program->hardware().walk([&](ac::StructOp op) {
    if (op.getSymName() == "decl.provider.Pair")
      record = op;
  });
  ASSERT_TRUE(record);
  record->setAttr("constructor", mlir::FlatSymbolRefAttr::get(
                                     &context, "decl.provider.Other.__init__"));
  EXPECT_TRUE(mlir::failed(verifyFinalProgram(*program, emitError())));
  EXPECT_TRUE(mlir::failed(freshVerify()));
}

} // namespace
} // namespace acir::compiler
