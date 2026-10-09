#include "Compiler/SourceUnit.h"
#include "Compiler/SourceLink.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/SymbolTable.h"
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

// Foundation tests use a legitimate existing implementation provider and an
// unused Struct. Real source Enum/declaration-only cases require later S3 and
// belong to the public-source lane; no handmade header is counted as authority.
namespace {
namespace ac = acir::ac;
namespace compiler = acir::compiler;
using namespace mlir;

struct TemporaryDirectory {
  llvm::SmallString<256> path;
  TemporaryDirectory() {
    EXPECT_FALSE(llvm::sys::fs::createUniqueDirectory("enum-source-authority", path));
  }
  ~TemporaryDirectory() { llvm::sys::fs::remove_directories(path); }
  std::string child(StringRef name) const {
    llvm::SmallString<256> result(path); llvm::sys::path::append(result, name);
    return result.str().str();
  }
};
const char *providerSource = R"py(
import pycircuit as ac
@ac.struct
class Unused:
    flag: ac.u1
@ac.module
def Top(value: bool) -> {"out": bool}:
    return {"out": value}
)py";
const char *consumerSource = R"py(
from authority.provider import Top as Child
from pycircuit import module, rule
@module
def Parent(value: bool) -> {"out": bool}:
    child = Child()
    @rule
    def bind():
        child(value=value)
    bind()
    return {"out": child.out}
)py";

class EnumSourceUnitAuthorityTest : public ::testing::Test {
protected:
  MLIRContext context;
  TemporaryDirectory scratch;
  std::string diagnostics;
  void SetUp() override {
    context.loadDialect<ac::ACIRDialect, arith::ArithDialect, func::FuncDialect>();
  }
  DictionaryAttr owner(StringRef file) {
    Builder b(&context);
    return b.getDictionaryAttr({b.getNamedAttr("package", b.getStringAttr("authority")),
                                b.getNamedAttr("path", b.getStringAttr(file))});
  }
  auto error() { return [&] { return emitError(UnknownLoc::get(&context)); }; }
  std::optional<compiler::SourceUnitArtifacts>
  compile(StringRef filename, StringRef source, ArrayRef<mlir::ModuleOp> headers = {}) {
    auto path = scratch.child(filename), transportPath = path + ".transport.mlir";
    std::error_code ec;
    { llvm::raw_fd_ostream out(path, ec); EXPECT_FALSE(ec); if (ec) return {}; out << source; }
    static constexpr llvm::StringLiteral script = R"py(
import sys
from pathlib import Path
repo, source, root, output = map(Path, sys.argv[1:])
sys.path.insert(0, str(repo / "python"))
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport
capture = _capture_source_file(source, source_root=root)
output.write_text(_emit_source_transport(capture), encoding="utf-8")
)py";
    std::vector<std::string> owned{PYCIRCUIT_TEST_PYTHON, "-c", script.str(),
        PYCIRCUIT_TEST_REPO_ROOT, path, scratch.path.str().str(), transportPath};
    SmallVector<StringRef> args; for (const auto &value : owned) args.push_back(value);
    std::string log = path + ".capture.log";
    const std::array<std::optional<StringRef>, 3> redirects{std::nullopt, log, log};
    int status = llvm::sys::ExecuteAndWait(PYCIRCUIT_TEST_PYTHON, args, std::nullopt, redirects);
    EXPECT_EQ(status, 0); if (status) return {};
    auto transport = parseSourceFile<mlir::ModuleOp>(transportPath, &context);
    EXPECT_TRUE(transport); if (!transport) return {};
    auto registry = compiler::SourceHeaderRegistry::create(&context, headers, error());
    EXPECT_TRUE(succeeded(registry)); if (failed(registry)) return {};
    auto unit = compiler::compilePythonSourceUnit(*transport, owner(filename), *registry, error());
    EXPECT_TRUE(succeeded(unit)); if (failed(unit)) return {};
    return std::move(*unit);
  }
  OwningOpRef<mlir::ModuleOp> clone(mlir::ModuleOp original) {
    return OwningOpRef<mlir::ModuleOp>(cast<mlir::ModuleOp>(original->clone()));
  }
  ac::StructOp record(mlir::ModuleOp unit) {
    return cast<ac::StructOp>(SymbolTable::lookupSymbolIn(unit, "authority.provider.Unused"));
  }
  void removeNominalExport(mlir::ModuleOp unit) {
    Builder b(&context); SmallVector<Attribute> retained;
    for (Attribute raw : unit->getAttrOfType<ArrayAttr>("ac.exports")) {
      auto row = cast<DictionaryAttr>(raw);
      if (row.getAs<FlatSymbolRefAttr>("target").getValue() != "authority.provider.Unused")
        retained.push_back(raw);
    }
    unit->setAttr("ac.exports", b.getArrayAttr(retained));
  }
  template <typename Action> void rejected(Action action, StringRef guard) {
    diagnostics.clear();
    ScopedDiagnosticHandler handler(&context, [&](Diagnostic &d) {
      llvm::raw_string_ostream out(diagnostics); d.print(out); return success();
    });
    EXPECT_TRUE(failed(action()));
    EXPECT_NE(diagnostics.find(guard.str()), std::string::npos) << diagnostics;
  }
};

TEST_F(EnumSourceUnitAuthorityTest, GenuineProviderHasBidirectionalNominalCoverage) {
  auto provider = compile("provider.py", providerSource); ASSERT_TRUE(provider);
  auto registry = compiler::SourceHeaderRegistry::create(&context, {*provider->interface}, error());
  ASSERT_TRUE(succeeded(registry));
  ASSERT_TRUE(succeeded(registry->verifyBodySnapshots(*provider->body, *provider->interface, error())));
  auto body = clone(*provider->body), header = clone(*provider->interface);
  // Keep both real artifact namespaces consistent: retained header ownership,
  // rather than an exported dangling name, must expose the deleted definition.
  removeNominalExport(*body); removeNominalExport(*header); record(*body).erase();
  auto coverage = compiler::SourceHeaderRegistry::create(&context, {*header}, error());
  ASSERT_TRUE(succeeded(coverage));
  rejected([&] { return coverage->verifyBodySnapshots(*body, *header, error()); },
           "owning header nominal declaration has no matching body definition");
  auto mismatched = clone(*provider->body); Builder b(&context);
  auto originalField = cast<DictionaryAttr>(record(*mismatched).getFields()[0]);
  auto oldType = originalField.getAs<TypeAttr>("type");
  record(*mismatched)->setAttr("fields", b.getArrayAttr({b.getDictionaryAttr({
      b.getNamedAttr("name", b.getStringAttr("changed")), b.getNamedAttr("type", oldType)})}));
  rejected([&] { return registry->verifyBodySnapshots(*mismatched, *provider->interface, error()); },
           "source body declaration differs from its owning header");
}

TEST_F(EnumSourceUnitAuthorityTest, RealOwnerMutationAndMissingProviderReject) {
  auto provider = compile("provider.py", providerSource); ASSERT_TRUE(provider);
  auto consumer = compile("consumer.py", consumerSource, {*provider->interface}); ASSERT_TRUE(consumer);
  auto registry = compiler::SourceHeaderRegistry::create(&context,
      {*provider->interface, *consumer->interface}, error()); ASSERT_TRUE(succeeded(registry));
  auto changed = clone(*provider->body);
  record(*changed)->setAttr("ac.source_owner", owner("consumer.py"));
  rejected([&] { return registry->verifyBodySnapshots(*changed, *provider->interface, error()); },
           "source definition has a foreign SourceOwner");
  rejected([&] { return compiler::admitSourceLinkUnits(
      {{*consumer->body, *consumer->interface}}, error()); }, "header");
  auto deleted = clone(*provider->body), header = clone(*provider->interface);
  removeNominalExport(*deleted); removeNominalExport(*header); record(*deleted).erase();
  rejected([&] { return compiler::admitSourceLinkUnits(
      {{*consumer->body, *consumer->interface}, {*deleted, *header}}, error()); },
           "owning header nominal declaration has no matching body definition");
}

TEST_F(EnumSourceUnitAuthorityTest, ActualProviderWinsEquivalentSnapshotsInEitherOrder) {
  auto provider = compile("provider.py", providerSource); ASSERT_TRUE(provider);
  auto consumer = compile("consumer.py", consumerSource, {*provider->interface}); ASSERT_TRUE(consumer);
  Builder b(&context);
  // Retain a clone of the real compiled unused declaration to isolate link's
  // provider/snapshot ordering rule before S3 expands importer retention.
  for (mlir::ModuleOp unit : {*consumer->body, *consumer->interface}) {
    Operation *snapshot = SymbolTable::lookupSymbolIn(unit, "authority.provider.Unused");
    if (!snapshot) {
      snapshot = record(*provider->body)->clone();
      unit.getBody()->push_back(snapshot);
    }
    snapshot->setAttr("ac.declaration_role", b.getStringAttr("import_snapshot"));
    snapshot->setLoc(UnknownLoc::get(&context));
  }
  for (bool providerFirst : {false, true}) {
    std::vector<compiler::SourceLinkUnit> units;
    compiler::SourceLinkUnit p{*provider->body, *provider->interface},
        c{*consumer->body, *consumer->interface};
    if (providerFirst) units = {p, c}; else units = {c, p};
    auto linked = compiler::linkHardwareUnits(units, "authority.consumer.Parent", error());
    ASSERT_TRUE(succeeded(linked));
    auto selected = record(**linked), actual = record(*provider->body);
    EXPECT_EQ(selected->getAttr("ac.declaration_role"), b.getStringAttr("definition"));
    EXPECT_EQ(selected->getAttr("ac.source_owner"), actual->getAttr("ac.source_owner"));
    EXPECT_EQ(selected->getAttr("ac.origin"), actual->getAttr("ac.origin"));
    EXPECT_EQ(selected.getLoc(), actual.getLoc());
    auto rows = ac::collectFinalSourceUnits(**linked, error());
    ASSERT_TRUE(succeeded(rows)); ASSERT_EQ(rows->size(), 2u);
    EXPECT_EQ((*rows)[0].owner, owner("consumer.py"));
    EXPECT_EQ((*rows)[1].owner, owner("provider.py"));
  }
}
} // namespace
