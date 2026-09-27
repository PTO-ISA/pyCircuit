#include "Compiler/SourceUnit.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
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

struct ReviewTemporaryDirectory {
  llvm::SmallString<256> path;
  ReviewTemporaryDirectory() {
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("acir-source-review", path));
  }
  ~ReviewTemporaryDirectory() { llvm::sys::fs::remove_directories(path); }
  std::string child(llvm::StringRef name) const {
    llvm::SmallString<256> result(path);
    llvm::sys::path::append(result, name);
    return result.str().str();
  }
};

void reviewWrite(llvm::StringRef path, llvm::StringRef contents) {
  std::error_code error;
  llvm::raw_fd_ostream output(path, error);
  ASSERT_FALSE(error);
  output << contents;
}

std::string reviewRead(llvm::StringRef path) {
  auto buffer = llvm::MemoryBuffer::getFile(path);
  EXPECT_TRUE(static_cast<bool>(buffer));
  return buffer ? buffer.get()->getBuffer().str() : std::string{};
}

int reviewRun(llvm::StringRef program,
              const std::vector<std::string> &ownedArguments,
              llvm::StringRef log) {
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const std::string &argument : ownedArguments)
    arguments.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {std::nullopt,
                                                                   log, log};
  return llvm::sys::ExecuteAndWait(program, arguments, std::nullopt, redirects);
}

int reviewEmit(llvm::StringRef source, llvm::StringRef root,
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
captured = _capture_source_file(source, source_root=Path(sys.argv[3]))
Path(sys.argv[4]).write_text(_emit_source_transport(captured), encoding="utf-8")
)py";
  return reviewRun(ACIR_TEST_PYTHON,
                   {ACIR_TEST_PYTHON, "-c", script.str(), ACIR_TEST_REPO_ROOT,
                    source.str(), root.str(), output.str()},
                   log);
}

int reviewCompile(llvm::StringRef capture, llvm::StringRef path,
                  llvm::StringRef body, llvm::StringRef interface,
                  llvm::StringRef log,
                  llvm::ArrayRef<std::string> headers = {}) {
  std::vector<std::string> arguments = {
      ACIR_TEST_SOURCE_UNIT_HARNESS,
      "--capture",
      capture.str(),
      "--package",
      "demo",
      "--path",
      path.str(),
      "--body-out",
      body.str(),
      "--interface-out",
      interface.str(),
  };
  for (const std::string &header : headers) {
    arguments.push_back("--header");
    arguments.push_back(header);
  }
  return reviewRun(ACIR_TEST_SOURCE_UNIT_HARNESS, arguments, log);
}

mlir::OwningOpRef<mlir::ModuleOp> reviewClone(mlir::ModuleOp module) {
  return mlir::OwningOpRef<mlir::ModuleOp>(
      mlir::cast<mlir::ModuleOp>(module->clone()));
}

ac::StructOp reviewRecord(mlir::ModuleOp module, llvm::StringRef suffix) {
  ac::StructOp result;
  module.walk([&](ac::StructOp candidate) {
    if (candidate.getSymName().ends_with(suffix))
      result = candidate;
  });
  return result;
}

mlir::func::FuncOp reviewHelper(mlir::ModuleOp module, llvm::StringRef suffix) {
  mlir::func::FuncOp result;
  module.walk([&](mlir::func::FuncOp candidate) {
    if (candidate.getSymName().ends_with(suffix))
      result = candidate;
  });
  return result;
}

struct ReviewResult {
  bool passed;
  std::string diagnostic;
};

ReviewResult reviewRegistry(llvm::ArrayRef<mlir::ModuleOp> headers) {
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
  auto registry = SourceHeaderRegistry::create(headers, emit);
  return {mlir::succeeded(registry), diagnostic};
}

mlir::DictionaryAttr replaceDefinition(mlir::Builder &builder,
                                       mlir::DictionaryAttr occurrence,
                                       llvm::StringRef definition) {
  auto site = occurrence.getAs<mlir::DictionaryAttr>("site");
  llvm::SmallVector<mlir::NamedAttribute> siteFields(site.begin(), site.end());
  for (mlir::NamedAttribute &field : siteFields)
    if (field.getName() == "definition")
      field = builder.getNamedAttr(
          "definition",
          mlir::FlatSymbolRefAttr::get(builder.getContext(), definition));
  llvm::SmallVector<mlir::NamedAttribute> fields(occurrence.begin(),
                                                 occurrence.end());
  for (mlir::NamedAttribute &field : fields)
    if (field.getName() == "site")
      field =
          builder.getNamedAttr("site", builder.getDictionaryAttr(siteFields));
  return builder.getDictionaryAttr(fields);
}

mlir::DictionaryAttr replaceLocation(mlir::Builder &builder,
                                     mlir::DictionaryAttr location,
                                     llvm::StringRef path, int64_t line) {
  llvm::SmallVector<mlir::NamedAttribute> fields(location.begin(),
                                                 location.end());
  for (mlir::NamedAttribute &field : fields) {
    if (field.getName() == "path")
      field = builder.getNamedAttr("path", builder.getStringAttr(path));
    if (field.getName() == "line" || field.getName() == "end_line")
      field = builder.getNamedAttr(field.getName(),
                                   builder.getI64IntegerAttr(line));
  }
  return builder.getDictionaryAttr(fields);
}

llvm::SmallVector<std::pair<std::string, std::optional<uint64_t>>>
astPath(mlir::DictionaryAttr occurrence) {
  auto site = occurrence.getAs<mlir::DictionaryAttr>("site");
  llvm::SmallVector<std::pair<std::string, std::optional<uint64_t>>> result;
  for (mlir::Attribute raw : site.getAs<mlir::ArrayAttr>("ast_path")) {
    auto component = mlir::cast<mlir::DictionaryAttr>(raw);
    auto kind = component.getAs<mlir::StringAttr>("kind").getValue();
    if (kind == "field")
      result.push_back(
          {component.getAs<mlir::StringAttr>("name").getValue().str(),
           std::nullopt});
    else
      result.push_back({{},
                        component.getAs<mlir::IntegerAttr>("value")
                            .getValue()
                            .getZExtValue()});
  }
  return result;
}

class SourceUnitReviewTest : public ::testing::Test {
protected:
  SourceUnitReviewTest() : context(dialects) {
    dialects.insert<ac::ACIRDialect, mlir::arith::ArithDialect,
                    mlir::func::FuncDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    root = temporary.child("source");
    ASSERT_FALSE(llvm::sys::fs::create_directories(root));
    std::string packet = root + "/packet.py";
    std::string consumer = root + "/consumer.py";
    reviewWrite(
        packet,
        reviewRead(ACIR_TEST_REPO_ROOT
                   "/tests/system/fixtures/u01_source_units/packet.py"));
    reviewWrite(
        consumer,
        reviewRead(ACIR_TEST_REPO_ROOT
                   "/tests/system/fixtures/u01_source_units/consumer.py"));
    std::string packetTransport = temporary.child("packet.transport.mlir");
    std::string consumerTransport = temporary.child("consumer.transport.mlir");
    packetInterface = temporary.child("packet.interface.mlir");
    consumerInterface = temporary.child("consumer.interface.mlir");
    ASSERT_EQ(reviewEmit(packet, root, packetTransport,
                         temporary.child("packet-python.log")),
              0);
    ASSERT_EQ(reviewEmit(consumer, root, consumerTransport,
                         temporary.child("consumer-python.log")),
              0);
    ASSERT_EQ(reviewCompile(packetTransport, "packet.py",
                            temporary.child("packet.body.mlir"),
                            packetInterface,
                            temporary.child("packet-compile.log")),
              0);
    ASSERT_EQ(reviewCompile(
                  consumerTransport, "consumer.py",
                  temporary.child("consumer.body.mlir"), consumerInterface,
                  temporary.child("consumer-compile.log"), {packetInterface}),
              0);
    packetHeader =
        mlir::parseSourceFile<mlir::ModuleOp>(packetInterface, &context);
    consumerHeader =
        mlir::parseSourceFile<mlir::ModuleOp>(consumerInterface, &context);
    ASSERT_TRUE(packetHeader && consumerHeader);
  }

  void expectRejects(mlir::ModuleOp packet, mlir::ModuleOp consumer) {
    llvm::SmallVector<mlir::ModuleOp> headers{packet, consumer};
    ReviewResult result = reviewRegistry(headers);
    EXPECT_FALSE(result.passed);
    EXPECT_FALSE(result.diagnostic.empty());
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context;
  ReviewTemporaryDirectory temporary;
  std::string root;
  std::string packetInterface;
  std::string consumerInterface;
  mlir::OwningOpRef<mlir::ModuleOp> packetHeader;
  mlir::OwningOpRef<mlir::ModuleOp> consumerHeader;
};

TEST_F(SourceUnitReviewTest,
       OriginDefinitionsAreCheckedAtEveryDeclarationLayer) {
  mlir::Builder builder(&context);
  for (llvm::StringRef layer : {"record", "field", "helper", "parameter"}) {
    auto packet = reviewClone(*packetHeader);
    auto record = reviewRecord(*packet, ".Request");
    auto helper = reviewHelper(*packet, ".Request.__init__");
    if (layer == "record")
      record->setAttr(
          "ac.origin",
          replaceDefinition(
              builder, record->getAttrOfType<mlir::DictionaryAttr>("ac.origin"),
              "demo.packet.Other"));
    if (layer == "field") {
      llvm::SmallVector<mlir::Attribute> fields(record.getFields().begin(),
                                                record.getFields().end());
      auto field = mlir::cast<mlir::DictionaryAttr>(fields.front());
      llvm::SmallVector<mlir::NamedAttribute> attrs(field.begin(), field.end());
      for (mlir::NamedAttribute &attr : attrs)
        if (attr.getName() == "origin")
          attr = builder.getNamedAttr(
              "origin",
              replaceDefinition(
                  builder, mlir::cast<mlir::DictionaryAttr>(attr.getValue()),
                  "demo.packet.Other"));
      fields.front() = builder.getDictionaryAttr(attrs);
      record->setAttr("fields", builder.getArrayAttr(fields));
    }
    if (layer == "helper")
      helper->setAttr(
          "ac.origin",
          replaceDefinition(
              builder, helper->getAttrOfType<mlir::DictionaryAttr>("ac.origin"),
              "demo.packet.Other"));
    if (layer == "parameter") {
      auto parameters = helper->getAttrOfType<mlir::ArrayAttr>("ac.parameters");
      llvm::SmallVector<mlir::Attribute> values(parameters.begin(),
                                                parameters.end());
      auto parameter = mlir::cast<mlir::DictionaryAttr>(values.front());
      llvm::SmallVector<mlir::NamedAttribute> attrs(parameter.begin(),
                                                    parameter.end());
      for (mlir::NamedAttribute &attr : attrs)
        if (attr.getName() == "origin")
          attr = builder.getNamedAttr(
              "origin",
              replaceDefinition(
                  builder, mlir::cast<mlir::DictionaryAttr>(attr.getValue()),
                  "demo.packet.Other"));
      values.front() = builder.getDictionaryAttr(attrs);
      helper->setAttr("ac.parameters", builder.getArrayAttr(values));
    }
    expectRejects(*packet, *consumerHeader);
  }
}

TEST_F(SourceUnitReviewTest, ForgedBindingOrderAndDefaultGapAreRejected) {
  mlir::Builder builder(&context);
  for (llvm::StringRef mutation : {"binding", "default"}) {
    auto packet = reviewClone(*packetHeader);
    auto helper = reviewHelper(*packet, ".Request.__init__");
    auto parameters = helper->getAttrOfType<mlir::ArrayAttr>("ac.parameters");
    llvm::SmallVector<mlir::Attribute> values(parameters.begin(),
                                              parameters.end());
    size_t index = mutation == "binding" ? 0 : 1;
    auto parameter = mlir::cast<mlir::DictionaryAttr>(values[index]);
    llvm::SmallVector<mlir::NamedAttribute> attrs(parameter.begin(),
                                                  parameter.end());
    for (mlir::NamedAttribute &attr : attrs) {
      if (mutation == "binding" && attr.getName() == "binding")
        attr = builder.getNamedAttr("binding",
                                    builder.getStringAttr("keyword_only"));
      if (mutation == "default" && attr.getName() == "default")
        attr = builder.getNamedAttr(
            "default", builder.getDictionaryAttr({builder.getNamedAttr(
                           "present", builder.getBoolAttr(false))}));
    }
    values[index] = builder.getDictionaryAttr(attrs);
    helper->setAttr("ac.parameters", builder.getArrayAttr(values));
    expectRejects(*packet, *consumerHeader);
  }
}

TEST_F(SourceUnitReviewTest,
       SnapshotComparisonNormalizesLocationsButRejectsBodyChanges) {
  mlir::Builder builder(&context);
  llvm::SmallVector<mlir::ModuleOp> original{*packetHeader, *consumerHeader};
  ReviewResult baseline = reviewRegistry(original);
  ASSERT_TRUE(baseline.passed) << baseline.diagnostic;

  auto relocated = reviewClone(*consumerHeader);
  auto snapshot = reviewHelper(*relocated, ".Pair.__init__");
  ASSERT_TRUE(snapshot);
  snapshot->setLoc(mlir::FileLineColLoc::get(&context, "elsewhere.py", 99, 7));
  llvm::SmallVector<mlir::ModuleOp> relocatedHeaders{*packetHeader, *relocated};
  ReviewResult accepted = reviewRegistry(relocatedHeaders);
  EXPECT_TRUE(accepted.passed) << accepted.diagnostic;

  auto reordered = reviewClone(*consumerHeader);
  snapshot = reviewHelper(*reordered, ".Pair.__init__");
  ac::StructCreateOp create;
  snapshot.walk([&](ac::StructCreateOp candidate) { create = candidate; });
  ASSERT_TRUE(create);
  mlir::Value first = create.getValues()[0];
  create->setOperand(0, create.getValues()[1]);
  create->setOperand(1, first);
  expectRejects(*packetHeader, *reordered);

  auto changedBody = reviewClone(*consumerHeader);
  snapshot = reviewHelper(*changedBody, ".Pair.__init__");
  mlir::Block &entry = snapshot.getBody().front();
  mlir::OpBuilder at(entry.getTerminator());
  at.create<mlir::arith::ConstantOp>(snapshot.getLoc(), builder.getI1Type(),
                                     builder.getBoolAttr(false));
  expectRejects(*packetHeader, *changedBody);
}

TEST_F(SourceUnitReviewTest,
       ForwardReferenceToMalformedRecordDiagnosesWithoutCrash) {
  auto packet = reviewClone(*packetHeader);
  auto record = reviewRecord(*packet, ".Request");
  ASSERT_TRUE(record);
  record->moveAfter(&packet->getBody()->back());
  mlir::Builder builder(&context);
  auto fields = record.getFields();
  llvm::SmallVector<mlir::Attribute> changed(fields.begin(), fields.end());
  auto field = mlir::cast<mlir::DictionaryAttr>(changed.front());
  llvm::SmallVector<mlir::NamedAttribute> attrs;
  for (mlir::NamedAttribute attr : field)
    if (attr.getName() != "type")
      attrs.push_back(attr);
  changed.front() = builder.getDictionaryAttr(attrs);
  record->setAttr("fields", builder.getArrayAttr(changed));

  std::string diagnostic;
  mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &value) {
    llvm::raw_string_ostream stream(diagnostic);
    value.print(stream);
    return mlir::success();
  });
  EXPECT_TRUE(mlir::failed(mlir::verify(*packet)));
  EXPECT_FALSE(diagnostic.empty());
}

TEST_F(SourceUnitReviewTest,
       ImporterOriginsMatchKnownFixtureAstPathsAndProviderSource) {
  auto record = reviewRecord(*packetHeader, ".Request");
  auto helper = reviewHelper(*packetHeader, ".Request.__init__");
  ASSERT_TRUE(record && helper);
  auto fields = record.getFields();
  ASSERT_EQ(fields.size(), 2u);
  for (auto [index, raw] : llvm::enumerate(fields)) {
    auto field = mlir::cast<mlir::DictionaryAttr>(raw);
    EXPECT_EQ(field.getAs<mlir::DictionaryAttr>("location")
                  .getAs<mlir::StringAttr>("path")
                  .getValue(),
              "packet.py");
    auto path = astPath(field.getAs<mlir::DictionaryAttr>("origin"));
    ASSERT_EQ(path.size(), 2u);
    EXPECT_EQ(path[0], (std::pair<std::string, std::optional<uint64_t>>{
                           "body", std::nullopt}));
    EXPECT_EQ(path[1].second, index);
  }

  auto parameters = helper->getAttrOfType<mlir::ArrayAttr>("ac.parameters");
  ASSERT_EQ(parameters.size(), 2u);
  for (auto [index, raw] : llvm::enumerate(parameters)) {
    auto parameter = mlir::cast<mlir::DictionaryAttr>(raw);
    EXPECT_EQ(parameter.getAs<mlir::DictionaryAttr>("location")
                  .getAs<mlir::StringAttr>("path")
                  .getValue(),
              "packet.py");
    auto path = astPath(parameter.getAs<mlir::DictionaryAttr>("origin"));
    ASSERT_EQ(path.size(), 3u);
    EXPECT_EQ(path[0].first, "args");
    EXPECT_EQ(path[1].first, "args");
    EXPECT_EQ(path[2].second, index + 1);
  }
}

TEST_F(
    SourceUnitReviewTest,
    RawAndSnapshotLocationsRetainProviderPathButIgnoreDiagnosticCoordinates) {
  mlir::Builder builder(&context);
  auto changeFirstParameterLocation = [&](mlir::ModuleOp module,
                                          llvm::StringRef path, int64_t line,
                                          bool remove) {
    auto helper = reviewHelper(module, ".Request.__init__");
    auto parameters = helper->getAttrOfType<mlir::ArrayAttr>("ac.parameters");
    llvm::SmallVector<mlir::Attribute> values(parameters.begin(),
                                              parameters.end());
    auto parameter = mlir::cast<mlir::DictionaryAttr>(values.front());
    llvm::SmallVector<mlir::NamedAttribute> attrs;
    for (mlir::NamedAttribute attr : parameter) {
      if (attr.getName() != "location") {
        attrs.push_back(attr);
        continue;
      }
      if (!remove)
        attrs.push_back(builder.getNamedAttr(
            "location",
            replaceLocation(builder,
                            mlir::cast<mlir::DictionaryAttr>(attr.getValue()),
                            path, line)));
    }
    values.front() = builder.getDictionaryAttr(attrs);
    helper->setAttr("ac.parameters", builder.getArrayAttr(values));
  };

  auto rawMismatch = reviewClone(*packetHeader);
  auto record = reviewRecord(*rawMismatch, ".Request");
  llvm::SmallVector<mlir::Attribute> fields(record.getFields().begin(),
                                            record.getFields().end());
  auto field = mlir::cast<mlir::DictionaryAttr>(fields.front());
  llvm::SmallVector<mlir::NamedAttribute> fieldAttrs(field.begin(),
                                                     field.end());
  for (mlir::NamedAttribute &attr : fieldAttrs)
    if (attr.getName() == "location")
      attr = builder.getNamedAttr(
          "location",
          replaceLocation(builder,
                          mlir::cast<mlir::DictionaryAttr>(attr.getValue()),
                          "consumer.py", 1));
  fields.front() = builder.getDictionaryAttr(fieldAttrs);
  record->setAttr("fields", builder.getArrayAttr(fields));
  expectRejects(*rawMismatch, *consumerHeader);

  auto rawParameterMismatch = reviewClone(*packetHeader);
  changeFirstParameterLocation(*rawParameterMismatch, "consumer.py", 1, false);
  expectRejects(*rawParameterMismatch, *consumerHeader);

  auto snapshotPathMismatch = reviewClone(*consumerHeader);
  auto snapshot = reviewRecord(*snapshotPathMismatch, ".Request");
  fields.assign(snapshot.getFields().begin(), snapshot.getFields().end());
  field = mlir::cast<mlir::DictionaryAttr>(fields.front());
  fieldAttrs.assign(field.begin(), field.end());
  for (mlir::NamedAttribute &attr : fieldAttrs)
    if (attr.getName() == "location")
      attr = builder.getNamedAttr(
          "location",
          replaceLocation(builder,
                          mlir::cast<mlir::DictionaryAttr>(attr.getValue()),
                          "consumer.py", 1));
  fields.front() = builder.getDictionaryAttr(fieldAttrs);
  snapshot->setAttr("fields", builder.getArrayAttr(fields));
  expectRejects(*packetHeader, *snapshotPathMismatch);

  auto snapshotParameterPathMismatch = reviewClone(*consumerHeader);
  changeFirstParameterLocation(*snapshotParameterPathMismatch, "consumer.py", 1,
                               false);
  expectRejects(*packetHeader, *snapshotParameterPathMismatch);

  auto snapshotCoordinates = reviewClone(*consumerHeader);
  snapshot = reviewRecord(*snapshotCoordinates, ".Request");
  fields.assign(snapshot.getFields().begin(), snapshot.getFields().end());
  field = mlir::cast<mlir::DictionaryAttr>(fields.front());
  fieldAttrs.assign(field.begin(), field.end());
  for (mlir::NamedAttribute &attr : fieldAttrs)
    if (attr.getName() == "location")
      attr = builder.getNamedAttr(
          "location",
          replaceLocation(builder,
                          mlir::cast<mlir::DictionaryAttr>(attr.getValue()),
                          "packet.py", 777));
  fields.front() = builder.getDictionaryAttr(fieldAttrs);
  snapshot->setAttr("fields", builder.getArrayAttr(fields));
  llvm::SmallVector<mlir::ModuleOp> accepted{*packetHeader,
                                             *snapshotCoordinates};
  ReviewResult acceptedResult = reviewRegistry(accepted);
  EXPECT_TRUE(acceptedResult.passed) << acceptedResult.diagnostic;

  auto snapshotMissing = reviewClone(*consumerHeader);
  changeFirstParameterLocation(*snapshotMissing, "packet.py", 1, true);
  expectRejects(*packetHeader, *snapshotMissing);
}

} // namespace
} // namespace acir::compiler
