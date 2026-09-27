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

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <vector>

namespace acir::compiler {
namespace {

struct TemporaryDirectory {
  llvm::SmallString<256> path;
  TemporaryDirectory() {
    EXPECT_FALSE(llvm::sys::fs::createUniqueDirectory("acir-namespace", path));
  }
  ~TemporaryDirectory() { llvm::sys::fs::remove_directories(path); }
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

int emitTransport(llvm::StringRef source, llvm::StringRef root,
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

int compileUnit(llvm::StringRef capture, llvm::StringRef sourcePath,
                llvm::StringRef body, llvm::StringRef interface,
                llvm::StringRef log, llvm::ArrayRef<std::string> headers = {}) {
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

struct RegistryResult {
  bool accepted;
  std::string diagnostic;
};

RegistryResult checkRegistry(llvm::ArrayRef<mlir::ModuleOp> headers) {
  std::string diagnostic;
  mlir::ModuleOp first = headers.front();
  mlir::MLIRContext *context = first.getContext();
  mlir::ScopedDiagnosticHandler capture(context, [&](mlir::Diagnostic &value) {
    llvm::raw_string_ostream stream(diagnostic);
    value.print(stream);
    return mlir::success();
  });
  auto location = mlir::UnknownLoc::get(context);
  auto emit = [location]() -> mlir::InFlightDiagnostic {
    return mlir::emitError(location);
  };
  return {mlir::succeeded(SourceHeaderRegistry::create(headers, emit)),
          diagnostic};
}

RegistryResult checkRegistry(mlir::ModuleOp header) {
  llvm::SmallVector<mlir::ModuleOp> headers{header};
  return checkRegistry(headers);
}

mlir::DictionaryAttr withField(mlir::Builder &builder,
                               mlir::DictionaryAttr dictionary,
                               llvm::StringRef name, mlir::Attribute value) {
  llvm::SmallVector<mlir::NamedAttribute> fields(dictionary.begin(),
                                                 dictionary.end());
  bool replaced = false;
  for (mlir::NamedAttribute &field : fields)
    if (field.getName() == name) {
      field = builder.getNamedAttr(name, value);
      replaced = true;
    }
  if (!replaced)
    fields.push_back(builder.getNamedAttr(name, value));
  return builder.getDictionaryAttr(fields);
}

mlir::DictionaryAttr withoutField(mlir::Builder &builder,
                                  mlir::DictionaryAttr dictionary,
                                  llvm::StringRef name) {
  llvm::SmallVector<mlir::NamedAttribute> fields;
  for (mlir::NamedAttribute field : dictionary)
    if (field.getName() != name)
      fields.push_back(field);
  return builder.getDictionaryAttr(fields);
}

void replaceFirstEntry(mlir::ModuleOp module, llvm::StringRef attribute,
                       mlir::DictionaryAttr replacement,
                       mlir::Builder &builder) {
  auto entries = module->getAttrOfType<mlir::ArrayAttr>(attribute);
  llvm::SmallVector<mlir::Attribute> values(entries.begin(), entries.end());
  values[0] = replacement;
  module->setAttr(attribute, builder.getArrayAttr(values));
}

std::vector<std::pair<std::string, std::string>>
bindingTable(mlir::ArrayAttr entries) {
  std::vector<std::pair<std::string, std::string>> table;
  for (mlir::Attribute raw : entries) {
    auto entry = mlir::cast<mlir::DictionaryAttr>(raw);
    table.emplace_back(
        entry.getAs<mlir::StringAttr>("name").getValue(),
        entry.getAs<mlir::FlatSymbolRefAttr>("target").getValue());
  }
  return table;
}

class NamespaceContractsTest : public ::testing::Test {
protected:
  NamespaceContractsTest() : context(dialects) {
    dialects.insert<ac::ACIRDialect, mlir::arith::ArithDialect,
                    mlir::func::FuncDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    root = temporary.child("source");
    ASSERT_FALSE(llvm::sys::fs::create_directories(root));
    packetSource = root + "/packet.py";
    consumerSource = root + "/consumer.py";
    writeFile(packetSource,
              readFile(ACIR_TEST_REPO_ROOT
                       "/tests/system/fixtures/u01_source_units/packet.py"));
    writeFile(consumerSource,
              readFile(ACIR_TEST_REPO_ROOT
                       "/tests/system/fixtures/u01_source_units/consumer.py"));
    packetTransport = temporary.child("packet.transport.mlir");
    consumerTransport = temporary.child("consumer.transport.mlir");
    packetBody = temporary.child("packet.body.mlir");
    packetInterface = temporary.child("packet.interface.mlir");
    consumerBody = temporary.child("consumer.body.mlir");
    consumerInterface = temporary.child("consumer.interface.mlir");
    ASSERT_EQ(emitTransport(packetSource, root, packetTransport,
                            temporary.child("packet-python.log")),
              0);
    ASSERT_EQ(emitTransport(consumerSource, root, consumerTransport,
                            temporary.child("consumer-python.log")),
              0);
    ASSERT_EQ(compileUnit(packetTransport, "packet.py", packetBody,
                          packetInterface, temporary.child("packet.log")),
              0);
    ASSERT_FALSE(llvm::sys::fs::remove(packetSource));
    ASSERT_FALSE(llvm::sys::fs::remove(packetBody));
    ASSERT_EQ(compileUnit(consumerTransport, "consumer.py", consumerBody,
                          consumerInterface, temporary.child("consumer.log"),
                          {packetInterface}),
              0);
    packetHeader = parse(packetInterface, context);
    consumerHeader = parse(consumerInterface, context);
    consumerBodyModule = parse(consumerBody, context);
    ASSERT_TRUE(packetHeader && consumerHeader && consumerBodyModule);
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context;
  TemporaryDirectory temporary;
  std::string root, packetSource, consumerSource;
  std::string packetTransport, consumerTransport;
  std::string packetBody, packetInterface, consumerBody, consumerInterface;
  mlir::OwningOpRef<mlir::ModuleOp> packetHeader, consumerHeader,
      consumerBodyModule;
};

TEST_F(NamespaceContractsTest, GeneratedHeadersMatchIndependentBindingTables) {
  using Pair = std::pair<std::string, std::string>;
  const std::vector<Pair> packetExpected = {
      {"Modes", "demo.packet.Modes"},
      {"Pair", "demo.packet.Pair"},
      {"Request", "demo.packet.Request"},
      {"Required", "demo.packet.Required"},
      {"Word", "demo.packet.Word"}};
  const std::vector<Pair> consumerImports = {{"Pair", "demo.packet.Pair"},
                                             {"Request", "demo.packet.Request"},
                                             {"Word", "demo.packet.Word"}};
  EXPECT_EQ(bindingTable(
                (*packetHeader)->getAttrOfType<mlir::ArrayAttr>("ac.exports")),
            packetExpected);
  EXPECT_EQ(
      bindingTable((*consumerHeader)
                       ->getAttrOfType<mlir::ArrayAttr>("ac.import_bindings")),
      consumerImports);
  EXPECT_EQ((*consumerHeader)->getAttr("ac.exports"),
            (*consumerBodyModule)->getAttr("ac.exports"));
  EXPECT_EQ((*consumerHeader)->getAttr("ac.import_bindings"),
            (*consumerBodyModule)->getAttr("ac.import_bindings"));
  EXPECT_EQ((*consumerHeader)->getAttr("ac.interfaces"),
            (*consumerBodyModule)->getAttr("ac.interfaces"));
}

TEST_F(NamespaceContractsTest,
       MandatoryArraysAndClosedExportRecordsAreEnforced) {
  auto missing = clone(*packetHeader);
  missing->getOperation()->removeAttr("ac.exports");
  EXPECT_FALSE(checkRegistry(*missing).accepted);

  auto wrongType = clone(*packetHeader);
  wrongType->getOperation()->setAttr("ac.import_bindings",
                                     mlir::StringAttr::get(&context, "bad"));
  EXPECT_FALSE(checkRegistry(*wrongType).accepted);

  auto open = clone(*packetHeader);
  mlir::Builder builder(&context);
  auto entries =
      open->getOperation()->getAttrOfType<mlir::ArrayAttr>("ac.exports");
  llvm::SmallVector<mlir::Attribute> values(entries.begin(), entries.end());
  values[0] = withField(builder, mlir::cast<mlir::DictionaryAttr>(values[0]),
                        "extra", builder.getUnitAttr());
  open->getOperation()->setAttr("ac.exports", builder.getArrayAttr(values));
  EXPECT_FALSE(checkRegistry(*open).accepted);

  auto openImport = clone(*consumerHeader);
  auto imports = openImport->getOperation()->getAttrOfType<mlir::ArrayAttr>(
      "ac.import_bindings");
  llvm::SmallVector<mlir::Attribute> importValues(imports.begin(),
                                                  imports.end());
  importValues[0] =
      withField(builder, mlir::cast<mlir::DictionaryAttr>(importValues[0]),
                "extra", builder.getUnitAttr());
  openImport->getOperation()->setAttr("ac.import_bindings",
                                      builder.getArrayAttr(importValues));
  llvm::SmallVector<mlir::ModuleOp> headers{*packetHeader, *openImport};
  EXPECT_FALSE(checkRegistry(headers).accepted);
}

TEST_F(NamespaceContractsTest, NamespaceSitesMustBelongToTheHeaderOwner) {
  auto header = clone(*packetHeader);
  mlir::Builder builder(&context);
  auto entries =
      header->getOperation()->getAttrOfType<mlir::ArrayAttr>("ac.exports");
  llvm::SmallVector<mlir::Attribute> values(entries.begin(), entries.end());
  auto entry = mlir::cast<mlir::DictionaryAttr>(values[0]);
  auto site = entry.getAs<mlir::DictionaryAttr>("site");
  auto location = site.getAs<mlir::DictionaryAttr>("location");
  location =
      withField(builder, location, "path", builder.getStringAttr("other.py"));
  site = withField(builder, site, "location", location);
  values[0] = withField(builder, entry, "site", site);
  header->getOperation()->setAttr("ac.exports", builder.getArrayAttr(values));
  RegistryResult result = checkRegistry(*header);
  EXPECT_FALSE(result.accepted);
  EXPECT_NE(result.diagnostic.find("owner path"), std::string::npos);
}

TEST_F(NamespaceContractsTest, ExportBindingsRejectEveryMalformedFieldShape) {
  mlir::Builder builder(&context);
  auto entries = (*packetHeader)->getAttrOfType<mlir::ArrayAttr>("ac.exports");
  auto valid = mlir::cast<mlir::DictionaryAttr>(entries[0]);
  llvm::SmallVector<std::pair<std::string, mlir::DictionaryAttr>> cases;
  for (llvm::StringRef field : {"name", "target", "site"})
    cases.push_back(
        {("missing-" + field).str(), withoutField(builder, valid, field)});
  cases.push_back({"wrong-name", withField(builder, valid, "name",
                                           builder.getBoolAttr(true))});
  cases.push_back({"wrong-target", withField(builder, valid, "target",
                                             builder.getStringAttr("bad"))});
  cases.push_back({"wrong-site", withField(builder, valid, "site",
                                           builder.getStringAttr("bad"))});
  cases.push_back({"unknown-field",
                   withField(builder, valid, "extra", builder.getUnitAttr())});
  for (const auto &[label, replacement] : cases) {
    SCOPED_TRACE(label);
    auto header = clone(*packetHeader);
    replaceFirstEntry(*header, "ac.exports", replacement, builder);
    RegistryResult result = checkRegistry(*header);
    EXPECT_FALSE(result.accepted);
    EXPECT_FALSE(result.diagnostic.empty());
  }
}

TEST_F(NamespaceContractsTest, ImportBindingsRejectEveryMalformedFieldShape) {
  mlir::Builder builder(&context);
  auto entries =
      (*consumerHeader)->getAttrOfType<mlir::ArrayAttr>("ac.import_bindings");
  auto valid = mlir::cast<mlir::DictionaryAttr>(entries[0]);
  llvm::SmallVector<std::pair<std::string, mlir::DictionaryAttr>> cases;
  for (llvm::StringRef field : {"source", "name", "target", "site"})
    cases.push_back(
        {("missing-" + field).str(), withoutField(builder, valid, field)});
  for (llvm::StringRef field : {"source", "name", "target", "site"})
    cases.push_back(
        {("wrong-" + field).str(),
         withField(builder, valid, field, builder.getStringAttr("bad"))});
  cases.push_back({"unknown-field",
                   withField(builder, valid, "extra", builder.getUnitAttr())});
  for (const auto &[label, replacement] : cases) {
    SCOPED_TRACE(label);
    auto header = clone(*consumerHeader);
    replaceFirstEntry(*header, "ac.import_bindings", replacement, builder);
    llvm::SmallVector<mlir::ModuleOp> headers{*packetHeader, *header};
    RegistryResult result = checkRegistry(headers);
    EXPECT_FALSE(result.accepted);
    EXPECT_FALSE(result.diagnostic.empty());
  }
}

TEST_F(NamespaceContractsTest, NamespaceSiteAndPathComponentsAreClosed) {
  mlir::Builder builder(&context);
  auto entries = (*packetHeader)->getAttrOfType<mlir::ArrayAttr>("ac.exports");
  auto entry = mlir::cast<mlir::DictionaryAttr>(entries[0]);
  auto site = entry.getAs<mlir::DictionaryAttr>("site");
  llvm::SmallVector<std::pair<std::string, mlir::Attribute>> siteCases = {
      {"missing-path", withoutField(builder, site, "ast_path")},
      {"missing-location", withoutField(builder, site, "location")},
      {"wrong-path",
       withField(builder, site, "ast_path", builder.getStringAttr("bad"))},
      {"wrong-location",
       withField(builder, site, "location", builder.getStringAttr("bad"))},
      {"unknown-field",
       withField(builder, site, "extra", builder.getUnitAttr())},
  };
  auto path = site.getAs<mlir::ArrayAttr>("ast_path");
  auto component = mlir::cast<mlir::DictionaryAttr>(path[0]);
  llvm::SmallVector<std::pair<std::string, mlir::Attribute>> pathCases = {
      {"missing-kind", withoutField(builder, component, "kind")},
      {"missing-name", withoutField(builder, component, "name")},
      {"wrong-kind",
       withField(builder, component, "kind", builder.getBoolAttr(true))},
      {"unknown-component-field",
       withField(builder, component, "extra", builder.getUnitAttr())},
      {"non-dictionary-component", builder.getStringAttr("bad")},
  };
  for (const auto &[label, malformedSite] : siteCases) {
    SCOPED_TRACE(label);
    auto header = clone(*packetHeader);
    replaceFirstEntry(*header, "ac.exports",
                      withField(builder, entry, "site", malformedSite),
                      builder);
    EXPECT_FALSE(checkRegistry(*header).accepted);
  }
  for (const auto &[label, malformedComponent] : pathCases) {
    SCOPED_TRACE(label);
    llvm::SmallVector<mlir::Attribute> components(path.begin(), path.end());
    components[0] = malformedComponent;
    auto malformedSite =
        withField(builder, site, "ast_path", builder.getArrayAttr(components));
    auto header = clone(*packetHeader);
    replaceFirstEntry(*header, "ac.exports",
                      withField(builder, entry, "site", malformedSite),
                      builder);
    EXPECT_FALSE(checkRegistry(*header).accepted);
  }
}

TEST_F(NamespaceContractsTest, Utf8NamesAndNumericPathIndicesUseExactOrdering) {
  mlir::Builder builder(&context);
  auto exports = (*packetHeader)->getAttrOfType<mlir::ArrayAttr>("ac.exports");
  const std::pair<llvm::StringRef, llvm::StringRef> expected[] = {
      {"z", "demo.packet.Word"},
      {"\xC3\xA9", "demo.packet.Pair"},
      {"\xCE\xB1", "demo.packet.Request"}};
  llvm::SmallVector<mlir::Attribute> renamed;
  for (auto [index, binding] : llvm::enumerate(expected)) {
    auto entry = mlir::cast<mlir::DictionaryAttr>(exports[index]);
    entry =
        withField(builder, entry, "name", builder.getStringAttr(binding.first));
    entry = withField(builder, entry, "target",
                      mlir::FlatSymbolRefAttr::get(&context, binding.second));
    renamed.push_back(entry);
  }
  auto provider = clone(*packetHeader);
  provider->getOperation()->setAttr("ac.exports",
                                    builder.getArrayAttr(renamed));
  RegistryResult result = checkRegistry(*provider);
  ASSERT_TRUE(result.accepted) << result.diagnostic;
  auto location = mlir::UnknownLoc::get(&context);
  auto emit = [location]() -> mlir::InFlightDiagnostic {
    return mlir::emitError(location);
  };
  llvm::SmallVector<mlir::ModuleOp> providerOnly{*provider};
  auto registry = SourceHeaderRegistry::create(providerOnly, emit);
  ASSERT_TRUE(mlir::succeeded(registry));
  for (const auto &[name, target] : expected)
    EXPECT_EQ(registry->lookupExport("demo.packet", name).getValue(), target);

  auto makeIndexed = [&](uint64_t value) {
    auto imports =
        (*consumerHeader)->getAttrOfType<mlir::ArrayAttr>("ac.import_bindings");
    auto binding = mlir::cast<mlir::DictionaryAttr>(imports[0]);
    auto site = binding.getAs<mlir::DictionaryAttr>("site");
    auto path = site.getAs<mlir::ArrayAttr>("ast_path");
    llvm::SmallVector<mlir::Attribute> components(path.begin(), path.end());
    auto last = mlir::cast<mlir::DictionaryAttr>(components.back());
    components.back() =
        withField(builder, last, "value", builder.getI64IntegerAttr(value));
    return withField(
        builder, binding, "site",
        withField(builder, site, "ast_path", builder.getArrayAttr(components)));
  };
  auto consumer = clone(*consumerHeader);
  consumer->getOperation()->setAttr(
      "ac.import_bindings",
      builder.getArrayAttr({makeIndexed(2), makeIndexed(10)}));
  llvm::SmallVector<mlir::ModuleOp> forward{*packetHeader, *consumer};
  llvm::SmallVector<mlir::ModuleOp> reverse{*consumer, *packetHeader};
  EXPECT_TRUE(checkRegistry(forward).accepted);
  EXPECT_TRUE(checkRegistry(reverse).accepted);
  consumer->getOperation()->setAttr(
      "ac.import_bindings",
      builder.getArrayAttr({makeIndexed(10), makeIndexed(2)}));
  EXPECT_FALSE(checkRegistry(forward).accepted);
}

TEST_F(NamespaceContractsTest, OrderingAndStaleProviderBindingsAreRejected) {
  mlir::Builder builder(&context);
  auto unordered = clone(*packetHeader);
  auto exports =
      unordered->getOperation()->getAttrOfType<mlir::ArrayAttr>("ac.exports");
  llvm::SmallVector<mlir::Attribute> reversed(exports.begin(), exports.end());
  std::reverse(reversed.begin(), reversed.end());
  unordered->getOperation()->setAttr("ac.exports",
                                     builder.getArrayAttr(reversed));
  EXPECT_FALSE(checkRegistry(*unordered).accepted);

  auto stale = clone(*consumerHeader);
  auto imports = stale->getOperation()->getAttrOfType<mlir::ArrayAttr>(
      "ac.import_bindings");
  llvm::SmallVector<mlir::Attribute> values(imports.begin(), imports.end());
  auto first = mlir::cast<mlir::DictionaryAttr>(values[0]);
  values[0] =
      withField(builder, first, "target",
                mlir::FlatSymbolRefAttr::get(&context, "demo.packet.Request"));
  stale->getOperation()->setAttr("ac.import_bindings",
                                 builder.getArrayAttr(values));
  llvm::SmallVector<mlir::ModuleOp> headers{*packetHeader, *stale};
  RegistryResult result = checkRegistry(headers);
  EXPECT_FALSE(result.accepted);
  EXPECT_NE(result.diagnostic.find("stale import binding"), std::string::npos);
}

} // namespace
} // namespace acir::compiler
