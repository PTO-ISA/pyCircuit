#include "Compiler/SourceLink.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"

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

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <vector>

namespace acir::compiler {
namespace {

struct LinkTemporaryDirectory {
  llvm::SmallString<256> path;
  LinkTemporaryDirectory() {
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("acir-source-link", path));
  }
  ~LinkTemporaryDirectory() { llvm::sys::fs::remove_directories(path); }
  std::string child(llvm::StringRef name) const {
    llvm::SmallString<256> result(path);
    llvm::sys::path::append(result, name);
    return result.str().str();
  }
};

void writeFile(llvm::StringRef path, llvm::StringRef contents) {
  std::error_code error;
  llvm::raw_fd_ostream output(path, error);
  ASSERT_FALSE(error);
  output << contents;
}

std::string readFile(llvm::StringRef path) {
  auto buffer = llvm::MemoryBuffer::getFile(path);
  EXPECT_TRUE(static_cast<bool>(buffer));
  return buffer ? buffer.get()->getBuffer().str() : std::string{};
}

int run(llvm::StringRef program, const std::vector<std::string> &owned,
        llvm::StringRef log) {
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const std::string &argument : owned)
    arguments.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {std::nullopt,
                                                                   log, log};
  return llvm::sys::ExecuteAndWait(program, arguments, std::nullopt, redirects);
}

int emitCapture(llvm::StringRef source, llvm::StringRef root,
                llvm::StringRef output, llvm::StringRef log) {
  static constexpr llvm::StringLiteral script = R"py(
import sys
from pathlib import Path
root = Path(sys.argv[1])
sys.path[:0] = [str(root / "python/semantic-core/src"),
                str(root / "python/pycircuit/src"), str(root)]
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport
source = Path(sys.argv[2])
capture = _capture_source_file(source, source_root=Path(sys.argv[3]))
Path(sys.argv[4]).write_text(_emit_source_transport(capture), encoding="utf-8")
)py";
  return run(ACIR_TEST_PYTHON,
             {ACIR_TEST_PYTHON, "-c", script.str(), ACIR_TEST_REPO_ROOT,
              source.str(), root.str(), output.str()},
             log);
}

int compileUnit(llvm::StringRef capture, llvm::StringRef path,
                llvm::StringRef body, llvm::StringRef header,
                llvm::StringRef log,
                llvm::ArrayRef<std::string> dependencies = {}) {
  std::vector<std::string> arguments = {ACIR_TEST_SOURCE_UNIT_HARNESS,
                                        "--capture",
                                        capture.str(),
                                        "--package",
                                        "demo",
                                        "--path",
                                        path.str()};
  for (const std::string &dependency : dependencies) {
    arguments.push_back("--header");
    arguments.push_back(dependency);
  }
  arguments.insert(arguments.end(),
                   {"--body-out", body.str(), "--interface-out", header.str()});
  return run(ACIR_TEST_SOURCE_UNIT_HARNESS, arguments, log);
}

mlir::OwningOpRef<mlir::ModuleOp> parse(llvm::StringRef path,
                                        mlir::MLIRContext &context) {
  return mlir::parseSourceFile<mlir::ModuleOp>(path, &context);
}

mlir::OwningOpRef<mlir::ModuleOp> clone(mlir::ModuleOp module) {
  return mlir::OwningOpRef<mlir::ModuleOp>(
      mlir::cast<mlir::ModuleOp>(module->clone()));
}

template <typename OpTy>
OpTy findSymbol(mlir::ModuleOp module, llvm::StringRef name) {
  return mlir::dyn_cast_or_null<OpTy>(
      mlir::SymbolTable::lookupSymbolIn(module, name));
}

class SourceLinkAdmissionTest : public ::testing::Test {
protected:
  SourceLinkAdmissionTest() : context(dialects) {
    dialects.insert<ac::ACIRDialect, mlir::arith::ArithDialect,
                    mlir::func::FuncDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    sourceRoot = temporary.child("source");
    ASSERT_FALSE(llvm::sys::fs::create_directories(sourceRoot));
    const std::string packetSource = sourceRoot + "/packet.py";
    const std::string probeSource = sourceRoot + "/accumulator_probe.py";
    const std::string rootSource = sourceRoot + "/probe_root.py";
    writeFile(
        packetSource,
        readFile(
            ACIR_TEST_REPO_ROOT
            "/tests/integration/pycircuit/fixtures/source_language/packet.py"));
    writeFile(probeSource, R"py(from pycircuit import module, rule
from .packet import Request, Word

@module
def AccumulatorProbe(request: Request, result: Word):
    total: Word = 0

    @rule
    def forward():
        nonlocal result
        result = request.value
        return

    forward()
)py");
    writeFile(rootSource, R"py(from pycircuit import module
from .packet import Request, Word
from .accumulator_probe import AccumulatorProbe

@module
def ProbeRoot():
    request: Request = Request(3, True)
    result: Word = 0
    child = AccumulatorProbe(request, result)
)py");

    const std::string packetCapture = temporary.child("packet.transport.mlir");
    const std::string probeCapture = temporary.child("probe.transport.mlir");
    const std::string rootCapture = temporary.child("root.transport.mlir");
    packetBodyPath = temporary.child("packet.body.mlir");
    packetHeaderPath = temporary.child("packet.interface.mlir");
    probeBodyPath = temporary.child("probe.body.mlir");
    probeHeaderPath = temporary.child("probe.interface.mlir");
    rootBodyPath = temporary.child("root.body.mlir");
    rootHeaderPath = temporary.child("root.interface.mlir");

    ASSERT_EQ(emitCapture(packetSource, sourceRoot, packetCapture,
                          temporary.child("packet-python.log")),
              0);
    ASSERT_EQ(emitCapture(probeSource, sourceRoot, probeCapture,
                          temporary.child("probe-python.log")),
              0);
    ASSERT_EQ(emitCapture(rootSource, sourceRoot, rootCapture,
                          temporary.child("root-python.log")),
              0);
    ASSERT_EQ(compileUnit(packetCapture, "packet.py", packetBodyPath,
                          packetHeaderPath, temporary.child("packet.log")),
              0);
    ASSERT_EQ(compileUnit(probeCapture, "accumulator_probe.py", probeBodyPath,
                          probeHeaderPath, temporary.child("probe.log"),
                          {packetHeaderPath}),
              0);
    ASSERT_EQ(compileUnit(rootCapture, "probe_root.py", rootBodyPath,
                          rootHeaderPath, temporary.child("root.log"),
                          {packetHeaderPath, probeHeaderPath}),
              0);

    packetBody = parse(packetBodyPath, context);
    packetHeader = parse(packetHeaderPath, context);
    probeBody = parse(probeBodyPath, context);
    probeHeader = parse(probeHeaderPath, context);
    rootBody = parse(rootBodyPath, context);
    rootHeader = parse(rootHeaderPath, context);
    ASSERT_TRUE(packetBody && packetHeader && probeBody && probeHeader &&
                rootBody && rootHeader);

    // Admission consumes only the compiled units. The original Python sources
    // are deliberately unavailable at link time.
    ASSERT_FALSE(llvm::sys::fs::remove_directories(sourceRoot));
  }

  std::pair<bool, std::string> admit(llvm::ArrayRef<SourceLinkUnit> units) {
    std::string diagnostic;
    mlir::ScopedDiagnosticHandler capture(
        &context, [&](mlir::Diagnostic &value) {
          llvm::raw_string_ostream stream(diagnostic);
          value.print(stream);
          return mlir::success();
        });
    auto location = mlir::UnknownLoc::get(&context);
    auto emit = [location]() -> mlir::InFlightDiagnostic {
      return mlir::emitError(location);
    };
    return {mlir::succeeded(admitSourceLinkUnits(units, emit)), diagnostic};
  }

  llvm::SmallVector<SourceLinkUnit> canonicalUnits() {
    return {{*packetBody, *packetHeader},
            {*probeBody, *probeHeader},
            {*rootBody, *rootHeader}};
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context;
  LinkTemporaryDirectory temporary;
  std::string sourceRoot;
  std::string packetBodyPath, packetHeaderPath, probeBodyPath, probeHeaderPath;
  std::string rootBodyPath, rootHeaderPath;
  mlir::OwningOpRef<mlir::ModuleOp> packetBody, packetHeader, probeBody;
  mlir::OwningOpRef<mlir::ModuleOp> probeHeader, rootBody, rootHeader;
};

TEST_F(SourceLinkAdmissionTest, CompleteUnitsAreOrderIndependent) {
  auto units = canonicalUnits();
  EXPECT_TRUE(admit(units).first);
  std::reverse(units.begin(), units.end());
  EXPECT_TRUE(admit(units).first);
  std::rotate(units.begin(), units.begin() + 1, units.end());
  EXPECT_TRUE(admit(units).first);
}

TEST_F(SourceLinkAdmissionTest, MissingOrDuplicatePairsAreRejected) {
  auto units = canonicalUnits();
  units[0].body = {};
  EXPECT_FALSE(admit(units).first);

  units = canonicalUnits();
  units[0].header = {};
  EXPECT_FALSE(admit(units).first);

  units = canonicalUnits();
  units.push_back(units.front());
  auto duplicate = admit(units);
  EXPECT_FALSE(duplicate.first);
  EXPECT_NE(duplicate.second.find("duplicate source link SourceOwner"),
            std::string::npos);

  units = canonicalUnits();
  units[2].body = units[1].body;
  EXPECT_FALSE(admit(units).first);
}

TEST_F(SourceLinkAdmissionTest, MissingOrChangedOwnersAreRejected) {
  auto missingHeaderOwner = clone(*probeHeader);
  missingHeaderOwner->getOperation()->removeAttr("ac.source_owner");
  auto units = canonicalUnits();
  units[1].header = *missingHeaderOwner;
  EXPECT_FALSE(admit(units).first);

  auto missingBodyOwner = clone(*probeBody);
  missingBodyOwner->getOperation()->removeAttr("ac.source_owner");
  units = canonicalUnits();
  units[1].body = *missingBodyOwner;
  EXPECT_FALSE(admit(units).first);

  for (llvm::StringRef attribute : {"ac.stage", "ac.unit_kind"}) {
    auto changedHeader = clone(*probeHeader);
    changedHeader->getOperation()->setAttr(
        attribute, mlir::StringAttr::get(&context, "bogus"));
    units = canonicalUnits();
    units[1].header = *changedHeader;
    EXPECT_FALSE(admit(units).first);

    auto changedBody = clone(*probeBody);
    changedBody->getOperation()->setAttr(
        attribute, mlir::StringAttr::get(&context, "bogus"));
    units = canonicalUnits();
    units[1].body = *changedBody;
    EXPECT_FALSE(admit(units).first);
  }
}

TEST_F(SourceLinkAdmissionTest, StaleNamespaceBindingIsRejected) {
  auto staleRootHeader = clone(*rootHeader);
  mlir::Builder builder(&context);
  auto bindings =
      staleRootHeader->getOperation()->getAttrOfType<mlir::ArrayAttr>(
          "ac.import_bindings");
  ASSERT_FALSE(bindings.empty());
  llvm::SmallVector<mlir::Attribute> changed(bindings.begin(), bindings.end());
  bool changedProbeBinding = false;
  for (mlir::Attribute &raw : changed) {
    auto binding = mlir::cast<mlir::DictionaryAttr>(raw);
    auto name = binding.getAs<mlir::StringAttr>("name");
    if (!name || name.getValue() != "AccumulatorProbe")
      continue;
    llvm::SmallVector<mlir::NamedAttribute> fields(binding.begin(),
                                                   binding.end());
    for (mlir::NamedAttribute &field : fields)
      if (field.getName() == "target")
        field = builder.getNamedAttr(
            "target",
            mlir::FlatSymbolRefAttr::get(&context, "demo.packet.Request"));
    raw = builder.getDictionaryAttr(fields);
    changedProbeBinding = true;
  }
  ASSERT_TRUE(changedProbeBinding);
  staleRootHeader->getOperation()->setAttr("ac.import_bindings",
                                           builder.getArrayAttr(changed));
  auto units = canonicalUnits();
  units[2].header = *staleRootHeader;
  auto result = admit(units);
  EXPECT_FALSE(result.first);
  EXPECT_NE(
      result.second.find("body/header metadata differs for ac.import_bindings"),
      std::string::npos)
      << result.second;
}

TEST_F(SourceLinkAdmissionTest, BodyMetadataAndDeclarationSnapshotsMustMatch) {
  auto changedMetadata = clone(*rootBody);
  changedMetadata->getOperation()->setAttr("ac.exports",
                                           mlir::ArrayAttr::get(&context, {}));
  auto units = canonicalUnits();
  units[2].body = *changedMetadata;
  EXPECT_FALSE(admit(units).first);

  auto changedSnapshot = clone(*probeBody);
  auto alias =
      findSymbol<ac::TypeAliasOp>(*changedSnapshot, "demo.packet.Word");
  ASSERT_TRUE(alias);
  alias->setAttr("ac.declaration_role",
                 mlir::StringAttr::get(&context, "definition"));
  units = canonicalUnits();
  units[1].body = *changedSnapshot;
  auto result = admit(units);
  EXPECT_FALSE(result.first);
  EXPECT_FALSE(result.second.empty());
}

} // namespace
} // namespace acir::compiler
