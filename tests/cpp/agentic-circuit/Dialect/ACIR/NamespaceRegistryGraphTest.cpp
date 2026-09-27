#include "Compiler/SourceUnit.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
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

struct GraphTemporaryDirectory {
  llvm::SmallString<256> path;
  GraphTemporaryDirectory() {
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("acir-namespace-graph", path));
  }
  ~GraphTemporaryDirectory() { llvm::sys::fs::remove_directories(path); }
  std::string child(llvm::StringRef name) const {
    llvm::SmallString<256> result(path);
    llvm::sys::path::append(result, name);
    return result.str().str();
  }
};

void graphWrite(llvm::StringRef path, llvm::StringRef contents) {
  std::error_code error;
  llvm::raw_fd_ostream output(path, error);
  ASSERT_FALSE(error);
  output << contents;
}

std::string graphRead(llvm::StringRef path) {
  auto buffer = llvm::MemoryBuffer::getFile(path);
  EXPECT_TRUE(static_cast<bool>(buffer));
  return buffer ? buffer.get()->getBuffer().str() : std::string{};
}

int graphRun(llvm::StringRef program, const std::vector<std::string> &owned,
             llvm::StringRef log) {
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const std::string &argument : owned)
    arguments.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {std::nullopt,
                                                                   log, log};
  return llvm::sys::ExecuteAndWait(program, arguments, std::nullopt, redirects);
}

int graphEmit(llvm::StringRef source, llvm::StringRef root,
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
  return graphRun(ACIR_TEST_PYTHON,
                  {ACIR_TEST_PYTHON, "-c", script.str(), ACIR_TEST_REPO_ROOT,
                   source.str(), root.str(), output.str()},
                  log);
}

int graphCompile(llvm::StringRef capture, llvm::StringRef sourcePath,
                 llvm::StringRef body, llvm::StringRef interface,
                 llvm::StringRef log,
                 llvm::ArrayRef<std::string> headers = {}) {
  std::vector<std::string> arguments = {ACIR_TEST_SOURCE_UNIT_HARNESS,
                                        "--capture",
                                        capture.str(),
                                        "--package",
                                        "demo",
                                        "--path",
                                        sourcePath.str()};
  for (const std::string &header : headers) {
    arguments.push_back("--header");
    arguments.push_back(header);
  }
  arguments.insert(arguments.end(), {"--body-out", body.str(),
                                     "--interface-out", interface.str()});
  return graphRun(ACIR_TEST_SOURCE_UNIT_HARNESS, arguments, log);
}

mlir::OwningOpRef<mlir::ModuleOp> graphParse(llvm::StringRef path,
                                             mlir::MLIRContext &context) {
  return mlir::parseSourceFile<mlir::ModuleOp>(path, &context);
}

mlir::OwningOpRef<mlir::ModuleOp> graphClone(mlir::ModuleOp module) {
  return mlir::OwningOpRef<mlir::ModuleOp>(
      mlir::cast<mlir::ModuleOp>(module->clone()));
}

struct GraphResult {
  bool accepted;
  std::string diagnostic;
};

GraphResult graphCheck(llvm::ArrayRef<mlir::ModuleOp> headers) {
  mlir::ModuleOp first = headers.front();
  std::string diagnostic;
  mlir::ScopedDiagnosticHandler capture(
      first.getContext(), [&](mlir::Diagnostic &value) {
        llvm::raw_string_ostream stream(diagnostic);
        value.print(stream);
        return mlir::success();
      });
  auto location = mlir::UnknownLoc::get(first.getContext());
  auto emit = [location]() -> mlir::InFlightDiagnostic {
    return mlir::emitError(location);
  };
  return {mlir::succeeded(SourceHeaderRegistry::create(headers, emit)),
          diagnostic};
}

GraphResult graphCheck(mlir::ModuleOp header) {
  llvm::SmallVector<mlir::ModuleOp> headers{header};
  return graphCheck(headers);
}

mlir::DictionaryAttr graphWithField(mlir::Builder &builder,
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

void graphReplaceFirst(mlir::ModuleOp module, llvm::StringRef attribute,
                       mlir::DictionaryAttr replacement,
                       mlir::Builder &builder) {
  auto entries = module->getAttrOfType<mlir::ArrayAttr>(attribute);
  llvm::SmallVector<mlir::Attribute> values(entries.begin(), entries.end());
  values[0] = replacement;
  module->setAttr(attribute, builder.getArrayAttr(values));
}

class NamespaceRegistryGraphTest : public ::testing::Test {
protected:
  NamespaceRegistryGraphTest() : context(dialects) {
    dialects.insert<ac::ACIRDialect, mlir::arith::ArithDialect,
                    mlir::func::FuncDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    root = temporary.child("source");
    ASSERT_FALSE(llvm::sys::fs::create_directories(root));
    std::string packetSource = root + "/packet.py";
    std::string consumerSource = root + "/consumer.py";
    graphWrite(packetSource,
               graphRead(ACIR_TEST_REPO_ROOT
                         "/tests/system/fixtures/u01_source_units/packet.py"));
    graphWrite(
        consumerSource,
        graphRead(ACIR_TEST_REPO_ROOT
                  "/tests/system/fixtures/u01_source_units/consumer.py"));
    std::string packetTransport = temporary.child("packet.transport.mlir");
    std::string consumerTransport = temporary.child("consumer.transport.mlir");
    packetInterface = temporary.child("packet.interface.mlir");
    consumerInterface = temporary.child("consumer.interface.mlir");
    ASSERT_EQ(graphEmit(packetSource, root, packetTransport,
                        temporary.child("packet-python.log")),
              0);
    ASSERT_EQ(graphEmit(consumerSource, root, consumerTransport,
                        temporary.child("consumer-python.log")),
              0);
    ASSERT_EQ(graphCompile(packetTransport, "packet.py",
                           temporary.child("packet.body.mlir"), packetInterface,
                           temporary.child("packet.log")),
              0);
    ASSERT_EQ(graphCompile(consumerTransport, "consumer.py",
                           temporary.child("consumer.body.mlir"),
                           consumerInterface, temporary.child("consumer.log"),
                           {packetInterface}),
              0);
    packet = graphParse(packetInterface, context);
    consumer = graphParse(consumerInterface, context);
    ASSERT_TRUE(packet && consumer);
  }

  mlir::OwningOpRef<mlir::ModuleOp> compileAlias(llvm::StringRef stem) {
    std::string source = root + "/" + stem.str() + ".py";
    graphWrite(
        source,
        "from typing import Annotated\nValue = Annotated[int, range(4)]\n");
    std::string transport = temporary.child((stem + ".transport.mlir").str());
    std::string body = temporary.child((stem + ".body.mlir").str());
    std::string interface = temporary.child((stem + ".interface.mlir").str());
    EXPECT_EQ(graphEmit(source, root, transport,
                        temporary.child((stem + ".python.log").str())),
              0);
    EXPECT_EQ(graphCompile(transport, (stem + ".py").str(), body, interface,
                           temporary.child((stem + ".log").str())),
              0);
    return graphParse(interface, context);
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context;
  GraphTemporaryDirectory temporary;
  std::string root, packetInterface, consumerInterface;
  mlir::OwningOpRef<mlir::ModuleOp> packet, consumer;
};

TEST_F(NamespaceRegistryGraphTest,
       CompleteCyclicImportBindingsAcceptHeaderPermutation) {
  auto a = compileAlias("a");
  auto b = compileAlias("b");
  ASSERT_TRUE(a && b);
  mlir::Builder builder(&context);
  auto aOwner =
      a->getOperation()->getAttrOfType<mlir::DictionaryAttr>("ac.source_owner");
  auto bOwner =
      b->getOperation()->getAttrOfType<mlir::DictionaryAttr>("ac.source_owner");
  auto aExport = mlir::cast<mlir::DictionaryAttr>(
      a->getOperation()->getAttrOfType<mlir::ArrayAttr>("ac.exports")[0]);
  auto bExport = mlir::cast<mlir::DictionaryAttr>(
      b->getOperation()->getAttrOfType<mlir::ArrayAttr>("ac.exports")[0]);
  auto import = [&](mlir::DictionaryAttr source, llvm::StringRef target,
                    mlir::DictionaryAttr site) {
    return builder.getDictionaryAttr({
        builder.getNamedAttr("source", source),
        builder.getNamedAttr("name", builder.getStringAttr("Value")),
        builder.getNamedAttr("target",
                             mlir::FlatSymbolRefAttr::get(&context, target)),
        builder.getNamedAttr("site", site),
    });
  };
  a->getOperation()->setAttr("ac.interfaces",
                             builder.getArrayAttr({aOwner, bOwner}));
  b->getOperation()->setAttr("ac.interfaces",
                             builder.getArrayAttr({bOwner, aOwner}));
  a->getOperation()->setAttr(
      "ac.import_bindings", builder.getArrayAttr({import(
                                bOwner, "demo.b.Value",
                                aExport.getAs<mlir::DictionaryAttr>("site"))}));
  b->getOperation()->setAttr(
      "ac.import_bindings", builder.getArrayAttr({import(
                                aOwner, "demo.a.Value",
                                bExport.getAs<mlir::DictionaryAttr>("site"))}));
  llvm::SmallVector<mlir::ModuleOp> forward{*a, *b};
  llvm::SmallVector<mlir::ModuleOp> reverse{*b, *a};
  EXPECT_TRUE(graphCheck(forward).accepted);
  EXPECT_TRUE(graphCheck(reverse).accepted);
}

TEST_F(NamespaceRegistryGraphTest, ImportSnapshotWithoutAuthorityIsRejected) {
  auto header = graphClone(*packet);
  mlir::Operation &alias = *header->getBody()->begin();
  alias.setAttr("ac.declaration_role",
                mlir::StringAttr::get(&context, "import_snapshot"));
  GraphResult result = graphCheck(*header);
  EXPECT_FALSE(result.accepted);
  EXPECT_NE(result.diagnostic.find("snapshot"), std::string::npos);
}

TEST_F(NamespaceRegistryGraphTest,
       DanglingAndRecordConstructorExportsAreRejected) {
  mlir::Builder builder(&context);
  auto entries = (*packet)->getAttrOfType<mlir::ArrayAttr>("ac.exports");
  auto first = mlir::cast<mlir::DictionaryAttr>(entries[0]);
  for (const auto &[label, target] :
       std::array<std::pair<llvm::StringRef, llvm::StringRef>, 2>{
           {{"dangling", "demo.packet.Missing"},
            {"constructor", "demo.packet.Request.__init__"}}}) {
    SCOPED_TRACE(label.str());
    auto header = graphClone(*packet);
    graphReplaceFirst(
        *header, "ac.exports",
        graphWithField(builder, first, "target",
                       mlir::FlatSymbolRefAttr::get(&context, target)),
        builder);
    GraphResult result = graphCheck(*header);
    EXPECT_FALSE(result.accepted);
    EXPECT_FALSE(result.diagnostic.empty());
  }
}

TEST_F(NamespaceRegistryGraphTest,
       SameProviderAndNameWithDifferentTargetIsRejected) {
  mlir::Builder builder(&context);
  auto header = graphClone(*consumer);
  auto entries = header->getOperation()->getAttrOfType<mlir::ArrayAttr>(
      "ac.import_bindings");
  auto valid = mlir::cast<mlir::DictionaryAttr>(entries[0]);
  auto conflicting = graphWithField(
      builder, valid, "target",
      mlir::FlatSymbolRefAttr::get(&context, "demo.packet.Request"));
  header->getOperation()->setAttr("ac.import_bindings",
                                  builder.getArrayAttr({valid, conflicting}));
  llvm::SmallVector<mlir::ModuleOp> headers{*packet, *header};
  GraphResult result = graphCheck(headers);
  EXPECT_FALSE(result.accepted);
  EXPECT_FALSE(result.diagnostic.empty());
}

} // namespace
} // namespace acir::compiler
