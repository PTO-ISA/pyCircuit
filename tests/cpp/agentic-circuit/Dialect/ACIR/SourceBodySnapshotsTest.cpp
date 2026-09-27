#include "Compiler/SourceUnit.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/APSInt.h"
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

struct SnapshotTemporaryDirectory {
  llvm::SmallString<256> path;
  SnapshotTemporaryDirectory() {
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("acir-body-snapshot", path));
  }
  ~SnapshotTemporaryDirectory() { llvm::sys::fs::remove_directories(path); }
  std::string child(llvm::StringRef name) const {
    llvm::SmallString<256> result(path);
    llvm::sys::path::append(result, name);
    return result.str().str();
  }
};

void snapshotWrite(llvm::StringRef path, llvm::StringRef contents) {
  std::error_code error;
  llvm::raw_fd_ostream output(path, error);
  ASSERT_FALSE(error);
  output << contents;
}

std::string snapshotRead(llvm::StringRef path) {
  auto buffer = llvm::MemoryBuffer::getFile(path);
  EXPECT_TRUE(static_cast<bool>(buffer));
  return buffer ? buffer.get()->getBuffer().str() : std::string{};
}

int snapshotRun(llvm::StringRef program, const std::vector<std::string> &owned,
                llvm::StringRef log) {
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const std::string &argument : owned)
    arguments.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {std::nullopt,
                                                                   log, log};
  return llvm::sys::ExecuteAndWait(program, arguments, std::nullopt, redirects);
}

int snapshotEmit(llvm::StringRef source, llvm::StringRef root,
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
  return snapshotRun(ACIR_TEST_PYTHON,
                     {ACIR_TEST_PYTHON, "-c", script.str(), ACIR_TEST_REPO_ROOT,
                      source.str(), root.str(), output.str()},
                     log);
}

int snapshotCompile(llvm::StringRef capture, llvm::StringRef path,
                    llvm::StringRef body, llvm::StringRef interface,
                    llvm::StringRef log,
                    llvm::ArrayRef<std::string> headers = {}) {
  std::vector<std::string> arguments = {ACIR_TEST_SOURCE_UNIT_HARNESS,
                                        "--capture",
                                        capture.str(),
                                        "--package",
                                        "demo",
                                        "--path",
                                        path.str()};
  for (const std::string &header : headers) {
    arguments.push_back("--header");
    arguments.push_back(header);
  }
  arguments.insert(arguments.end(), {"--body-out", body.str(),
                                     "--interface-out", interface.str()});
  return snapshotRun(ACIR_TEST_SOURCE_UNIT_HARNESS, arguments, log);
}

mlir::OwningOpRef<mlir::ModuleOp> snapshotParse(llvm::StringRef path,
                                                mlir::MLIRContext &context) {
  return mlir::parseSourceFile<mlir::ModuleOp>(path, &context);
}

mlir::OwningOpRef<mlir::ModuleOp> snapshotClone(mlir::ModuleOp module) {
  return mlir::OwningOpRef<mlir::ModuleOp>(
      mlir::cast<mlir::ModuleOp>(module->clone()));
}

std::string printed(mlir::ModuleOp module) {
  std::string result;
  llvm::raw_string_ostream stream(result);
  module.print(stream);
  return result;
}

mlir::DictionaryAttr snapshotWithField(mlir::Builder &builder,
                                       mlir::DictionaryAttr dictionary,
                                       llvm::StringRef name,
                                       mlir::Attribute value) {
  llvm::SmallVector<mlir::NamedAttribute> fields(dictionary.begin(),
                                                 dictionary.end());
  for (mlir::NamedAttribute &field : fields)
    if (field.getName() == name) {
      field = builder.getNamedAttr(name, value);
      return builder.getDictionaryAttr(fields);
    }
  fields.push_back(builder.getNamedAttr(name, value));
  return builder.getDictionaryAttr(fields);
}

ac::DffeOp findDffe(mlir::ModuleOp module, llvm::StringRef name) {
  ac::DffeOp result;
  module.walk([&](ac::DffeOp candidate) {
    auto candidateName = candidate->getAttrOfType<mlir::StringAttr>("name");
    if (candidateName && candidateName.getValue() == name)
      result = candidate;
  });
  return result;
}

template <typename OpTy>
OpTy findSymbol(mlir::ModuleOp module, llvm::StringRef name) {
  return mlir::dyn_cast_or_null<OpTy>(
      mlir::SymbolTable::lookupSymbolIn(module, name));
}

class SourceBodySnapshotsTest : public ::testing::Test {
protected:
  SourceBodySnapshotsTest() : context(dialects) {
    dialects.insert<ac::ACIRDialect, mlir::arith::ArithDialect,
                    mlir::func::FuncDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    root = temporary.child("source");
    ASSERT_FALSE(llvm::sys::fs::create_directories(root));
    std::string packet = root + "/packet.py";
    std::string probe = root + "/accumulator_probe.py";
    std::string probeRoot = root + "/probe_root.py";
    snapshotWrite(
        packet,
        snapshotRead(
            ACIR_TEST_REPO_ROOT
            "/tests/integration/pycircuit/fixtures/migration_c1/packet.py"));
    snapshotWrite(probe, R"py(from pycircuit import module, rule
from .packet import Request, Word

@module
class AccumulatorProbe:
    def __init__(self, request: Request, result: Word):
        self.request = request
        self.result = result
        self.total: Word = 0
        self.result = self.forward(self.request)

    @rule
    def forward(self, item: Request) -> Word:
        return item.value
)py");
    snapshotWrite(probeRoot, R"py(from pycircuit import module
from .packet import Request, Word
from .accumulator_probe import AccumulatorProbe

@module
class ProbeRoot:
    def __init__(self):
        self.request: Request = Request(3, True)
        self.result: Word = 0
        self.child = AccumulatorProbe(self.request, self.result)
)py");
    std::string packetTransport = temporary.child("packet.transport.mlir");
    std::string probeTransport = temporary.child("probe.transport.mlir");
    std::string rootTransport = temporary.child("root.transport.mlir");
    std::string packetInterface = temporary.child("packet.interface.mlir");
    std::string probeInterface = temporary.child("probe.interface.mlir");
    std::string probeBody = temporary.child("probe.body.mlir");
    std::string rootBody = temporary.child("root.body.mlir");
    ASSERT_EQ(snapshotEmit(packet, root, packetTransport,
                           temporary.child("packet-python.log")),
              0);
    ASSERT_EQ(snapshotEmit(probe, root, probeTransport,
                           temporary.child("probe-python.log")),
              0);
    ASSERT_EQ(snapshotEmit(probeRoot, root, rootTransport,
                           temporary.child("root-python.log")),
              0);
    ASSERT_EQ(snapshotCompile(packetTransport, "packet.py",
                              temporary.child("packet.body.mlir"),
                              packetInterface, temporary.child("packet.log")),
              0);
    ASSERT_EQ(snapshotCompile(probeTransport, "accumulator_probe.py", probeBody,
                              probeInterface, temporary.child("probe.log"),
                              {packetInterface}),
              0);
    ASSERT_EQ(snapshotCompile(rootTransport, "probe_root.py", rootBody,
                              temporary.child("root.interface.mlir"),
                              temporary.child("root.log"),
                              {packetInterface, probeInterface}),
              0);
    packetHeader = snapshotParse(packetInterface, context);
    owningHeader = snapshotParse(probeInterface, context);
    body = snapshotParse(probeBody, context);
    resetBody = snapshotParse(rootBody, context);
    ASSERT_TRUE(packetHeader && owningHeader && body && resetBody);
    canonicalHeader = printed(*owningHeader);
  }

  bool verify(mlir::ModuleOp candidate,
              llvm::ArrayRef<mlir::ModuleOp> providers) {
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
    auto registry = SourceHeaderRegistry::create(providers, emit);
    EXPECT_TRUE(mlir::succeeded(registry)) << diagnostic;
    if (mlir::failed(registry))
      return false;
    return mlir::succeeded(
        registry->verifyBodySnapshots(candidate, *owningHeader, emit));
  }

  void expectRejects(mlir::OwningOpRef<mlir::ModuleOp> candidate) {
    llvm::SmallVector<mlir::ModuleOp> providers{*packetHeader};
    EXPECT_FALSE(verify(*candidate, providers));
    EXPECT_EQ(printed(*owningHeader), canonicalHeader);
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context;
  SnapshotTemporaryDirectory temporary;
  std::string root, canonicalHeader;
  mlir::OwningOpRef<mlir::ModuleOp> packetHeader, owningHeader, body;
  mlir::OwningOpRef<mlir::ModuleOp> resetBody;
};

TEST_F(SourceBodySnapshotsTest, CanonicalBodyMatchesSuppliedAuthorities) {
  llvm::SmallVector<mlir::ModuleOp> providers{*packetHeader};
  EXPECT_TRUE(verify(*body, providers));
}

TEST_F(SourceBodySnapshotsTest, SourceStageAndUnitKindAreFailClosed) {
  for (llvm::StringRef attribute : {"ac.stage", "ac.unit_kind"}) {
    for (bool missing : {true, false}) {
      auto candidate = snapshotClone(*body);
      if (missing)
        candidate->getOperation()->removeAttr(attribute);
      else
        candidate->getOperation()->setAttr(
            attribute, mlir::StringAttr::get(&context, "bogus"));
      EXPECT_TRUE(mlir::failed(mlir::verify(*candidate)));
    }
  }

  for (llvm::StringRef attribute : {"ac.stage", "ac.unit_kind"}) {
    auto header = snapshotClone(*owningHeader);
    header->getOperation()->removeAttr(attribute);
    llvm::SmallVector<mlir::ModuleOp> headers{*packetHeader, *header};
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
    EXPECT_TRUE(mlir::failed(SourceHeaderRegistry::create(headers, emit)));
  }
  EXPECT_EQ(printed(*owningHeader), canonicalHeader);
}

TEST_F(SourceBodySnapshotsTest, ResetExpressionsAreContextuallyVerified) {
  mlir::Builder builder(&context);
  {
    auto candidate = snapshotClone(*resetBody);
    auto state = findDffe(*candidate, "result");
    ASSERT_TRUE(state);
    auto initial =
        state->getAttrOfType<mlir::DictionaryAttr>("ac.initial_value");
    auto expression = initial.getAs<ac::StaticExprAttr>("value");
    auto tree = expression.getTree();
    tree = snapshotWithField(
        builder, tree, "value",
        builder.getDictionaryAttr({
            builder.getNamedAttr("kind", builder.getStringAttr("bool")),
            builder.getNamedAttr("value", builder.getBoolAttr(false)),
        }));
    state->setAttr("ac.initial_value",
                   snapshotWithField(builder, initial, "value",
                                     ac::StaticExprAttr::get(&context, tree)));
    EXPECT_TRUE(mlir::failed(mlir::verify(*candidate)));
  }

  enum class Mutation { WrongCallee, Duplicate, Unknown, Nominal };
  for (Mutation mutation : {Mutation::WrongCallee, Mutation::Duplicate,
                            Mutation::Unknown, Mutation::Nominal}) {
    auto candidate = snapshotClone(*resetBody);
    auto state = findDffe(*candidate, "request");
    ASSERT_TRUE(state);
    if (mutation == Mutation::Nominal) {
      auto logical =
          state->getAttrOfType<mlir::DictionaryAttr>("ac.logical_element");
      state->setAttr("ac.logical_element",
                     snapshotWithField(builder, logical, "symbol",
                                       mlir::FlatSymbolRefAttr::get(
                                           &context, "demo.packet.Word")));
    } else {
      auto initial =
          state->getAttrOfType<mlir::DictionaryAttr>("ac.initial_value");
      auto expression = initial.getAs<ac::StaticExprAttr>("value");
      auto tree = expression.getTree();
      if (mutation == Mutation::WrongCallee) {
        tree = snapshotWithField(
            builder, tree, "callee",
            mlir::FlatSymbolRefAttr::get(&context, "demo.packet.Word"));
      } else {
        auto arguments = tree.getAs<mlir::ArrayAttr>("arguments");
        llvm::SmallVector<mlir::Attribute> values(arguments.begin(),
                                                  arguments.end());
        auto second = mlir::cast<mlir::DictionaryAttr>(values[1]);
        second = snapshotWithField(builder, second, "kind",
                                   builder.getStringAttr("keyword"));
        second = snapshotWithField(
            builder, second, "name",
            builder.getStringAttr(mutation == Mutation::Duplicate ? "value"
                                                                  : "missing"));
        values[1] = second;
        tree = snapshotWithField(builder, tree, "arguments",
                                 builder.getArrayAttr(values));
      }
      state->setAttr(
          "ac.initial_value",
          snapshotWithField(builder, initial, "value",
                            ac::StaticExprAttr::get(&context, tree)));
    }
    EXPECT_TRUE(mlir::failed(mlir::verify(*candidate)));
  }
}

TEST_F(SourceBodySnapshotsTest, ConstructorBodyTamperingIsRejected) {
  auto candidate = snapshotClone(*body);
  auto constructor = findSymbol<mlir::func::FuncOp>(
      *candidate, "demo.packet.Request.__init__");
  ASSERT_TRUE(constructor);
  mlir::OpBuilder builder(&context);
  builder.setInsertionPoint(&constructor.getBody().front().back());
  builder.create<mlir::arith::ConstantOp>(
      constructor.getLoc(), builder.getI1Type(), builder.getBoolAttr(false));
  expectRejects(std::move(candidate));
}

TEST_F(SourceBodySnapshotsTest,
       ConstructorParameterDefaultTamperingIsRejected) {
  auto candidate = snapshotClone(*body);
  auto constructor = findSymbol<mlir::func::FuncOp>(
      *candidate, "demo.packet.Request.__init__");
  ASSERT_TRUE(constructor);
  mlir::Builder builder(&context);
  auto parameters =
      constructor->getAttrOfType<mlir::ArrayAttr>("ac.parameters");
  llvm::SmallVector<mlir::Attribute> values(parameters.begin(),
                                            parameters.end());
  auto parameter = mlir::cast<mlir::DictionaryAttr>(values[0]);
  auto defaultValue = parameter.getAs<mlir::DictionaryAttr>("default");
  auto value = defaultValue.getAs<mlir::DictionaryAttr>("value");
  value = snapshotWithField(
      builder, value, "value",
      ac::MathIntAttr::get(&context, llvm::APSInt(llvm::APInt(64, 1), true)));
  defaultValue = snapshotWithField(builder, defaultValue, "value", value);
  values[0] = snapshotWithField(builder, parameter, "default", defaultValue);
  constructor->setAttr("ac.parameters", builder.getArrayAttr(values));
  expectRejects(std::move(candidate));
}

TEST_F(SourceBodySnapshotsTest, RecordFieldOrderAndTypeTamperingAreRejected) {
  mlir::Builder builder(&context);
  for (bool wrongType : {false, true}) {
    auto candidate = snapshotClone(*body);
    auto record = findSymbol<ac::StructOp>(*candidate, "demo.packet.Request");
    ASSERT_TRUE(record);
    auto fields = record.getFields();
    llvm::SmallVector<mlir::Attribute> values(fields.begin(), fields.end());
    if (wrongType) {
      auto field = mlir::cast<mlir::DictionaryAttr>(values[0]);
      auto boolean = builder.getDictionaryAttr({
          builder.getNamedAttr("kind", builder.getStringAttr("bool")),
          builder.getNamedAttr("storage",
                               mlir::TypeAttr::get(builder.getI1Type())),
      });
      values[0] = snapshotWithField(builder, field, "type", boolean);
    } else {
      std::reverse(values.begin(), values.end());
    }
    record.setFieldsAttr(builder.getArrayAttr(values));
    expectRejects(std::move(candidate));
  }
}

TEST_F(SourceBodySnapshotsTest, SnapshotRoleAndOwnerTamperingAreRejected) {
  mlir::Builder builder(&context);
  for (bool wrongOwner : {false, true}) {
    auto candidate = snapshotClone(*body);
    auto alias = findSymbol<ac::TypeAliasOp>(*candidate, "demo.packet.Word");
    ASSERT_TRUE(alias);
    if (wrongOwner) {
      auto owner =
          alias->getAttrOfType<mlir::DictionaryAttr>("ac.source_owner");
      alias->setAttr("ac.source_owner",
                     snapshotWithField(builder, owner, "path",
                                       builder.getStringAttr("other.py")));
    } else {
      alias->setAttr("ac.declaration_role",
                     builder.getStringAttr("definition"));
    }
    expectRejects(std::move(candidate));
  }
}

TEST_F(SourceBodySnapshotsTest, MissingProviderHeaderRejectsRetainedSnapshots) {
  llvm::SmallVector<mlir::ModuleOp> noProviders;
  EXPECT_FALSE(verify(*body, noProviders));
  EXPECT_EQ(printed(*owningHeader), canonicalHeader);
}

} // namespace
} // namespace acir::compiler
