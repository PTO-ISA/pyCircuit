#include "Compiler/SourceUnit.h"
#include "acir/Dialect/ACIR/ACIRAttributes.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APSInt.h"
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

struct TemporaryDirectory {
  llvm::SmallString<256> path;

  TemporaryDirectory() {
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("acir-source-unit", path));
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

struct ProcessResult {
  int status;
  std::string output;
};

ProcessResult runProcess(llvm::StringRef program,
                         const std::vector<std::string> &ownedArguments,
                         llvm::StringRef logPath) {
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const std::string &argument : ownedArguments)
    arguments.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {
      std::nullopt, logPath, logPath};
  int status =
      llvm::sys::ExecuteAndWait(program, arguments, std::nullopt, redirects);
  return {status, readFile(logPath)};
}

ProcessResult emitTransport(llvm::StringRef source, llvm::StringRef sourceRoot,
                            llvm::StringRef output, llvm::StringRef logPath) {
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
  return runProcess(ACIR_TEST_PYTHON,
                    {ACIR_TEST_PYTHON, "-c", script.str(), ACIR_TEST_REPO_ROOT,
                     source.str(), sourceRoot.str(), output.str()},
                    logPath);
}

ProcessResult compileUnit(llvm::StringRef capture, llvm::StringRef sourcePath,
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
      sourcePath.str(),
      "--body-out",
      body.str(),
      "--interface-out",
      interface.str(),
  };
  for (const std::string &header : headers) {
    arguments.push_back("--header");
    arguments.push_back(header);
  }
  return runProcess(ACIR_TEST_SOURCE_UNIT_HARNESS, arguments, log);
}

mlir::OwningOpRef<mlir::ModuleOp> parseModule(llvm::StringRef path,
                                              mlir::MLIRContext &context) {
  return mlir::parseSourceFile<mlir::ModuleOp>(path, &context);
}

mlir::OwningOpRef<mlir::ModuleOp> cloneModule(mlir::ModuleOp module) {
  return mlir::OwningOpRef<mlir::ModuleOp>(
      mlir::cast<mlir::ModuleOp>(module->clone()));
}

ac::StructOp findRecord(mlir::ModuleOp module, llvm::StringRef suffix) {
  ac::StructOp result;
  module.walk([&](ac::StructOp candidate) {
    if (candidate.getSymName().ends_with(suffix))
      result = candidate;
  });
  return result;
}

mlir::func::FuncOp findHelper(mlir::ModuleOp module, llvm::StringRef suffix) {
  mlir::func::FuncOp result;
  module.walk([&](mlir::func::FuncOp candidate) {
    if (candidate.getSymName().ends_with(suffix))
      result = candidate;
  });
  return result;
}

struct RegistryResult {
  bool passed;
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
  auto emitError = [location]() -> mlir::InFlightDiagnostic {
    return mlir::emitError(location);
  };
  auto registry = SourceHeaderRegistry::create(headers, emitError);
  return {mlir::succeeded(registry), diagnostic};
}

RegistryResult checkRegistry(mlir::ModuleOp header) {
  llvm::SmallVector<mlir::ModuleOp> headers{header};
  return checkRegistry(headers);
}

void expectRegistryRejects(mlir::ModuleOp header) {
  RegistryResult result = checkRegistry(header);
  EXPECT_FALSE(result.passed);
  EXPECT_FALSE(result.diagnostic.empty());
}

mlir::DictionaryAttr owner(mlir::MLIRContext &context, llvm::StringRef path) {
  mlir::Builder builder(&context);
  return builder.getDictionaryAttr({
      builder.getNamedAttr("package", builder.getStringAttr("demo")),
      builder.getNamedAttr("path", builder.getStringAttr(path)),
  });
}

class SourceUnitHeaderTest : public ::testing::Test {
protected:
  SourceUnitHeaderTest() : context(dialects) {
    dialects.insert<ac::ACIRDialect, mlir::arith::ArithDialect,
                    mlir::func::FuncDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    sourceRoot = temporary.child("source");
    ASSERT_FALSE(llvm::sys::fs::create_directories(sourceRoot));
    packetSource = sourceRoot + "/packet.py";
    consumerSource = sourceRoot + "/consumer.py";
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
    auto emitted = emitTransport(packetSource, sourceRoot, packetTransport,
                                 temporary.child("packet-python.log"));
    ASSERT_EQ(emitted.status, 0) << emitted.output;
    emitted = emitTransport(consumerSource, sourceRoot, consumerTransport,
                            temporary.child("consumer-python.log"));
    ASSERT_EQ(emitted.status, 0) << emitted.output;
    auto compiled =
        compileUnit(packetTransport, "packet.py", packetBody, packetInterface,
                    temporary.child("packet-compile.log"));
    ASSERT_EQ(compiled.status, 0) << compiled.output;
    ASSERT_FALSE(llvm::sys::fs::remove(packetSource));
    ASSERT_FALSE(llvm::sys::fs::remove(packetBody));
    compiled = compileUnit(
        consumerTransport, "consumer.py", consumerBody, consumerInterface,
        temporary.child("consumer-compile.log"), {packetInterface});
    ASSERT_EQ(compiled.status, 0) << compiled.output;
    packetHeader = parseModule(packetInterface, context);
    consumerHeader = parseModule(consumerInterface, context);
    ASSERT_TRUE(packetHeader);
    ASSERT_TRUE(consumerHeader);
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context;
  TemporaryDirectory temporary;
  std::string sourceRoot;
  std::string packetSource;
  std::string consumerSource;
  std::string packetTransport;
  std::string consumerTransport;
  std::string packetBody;
  std::string packetInterface;
  std::string consumerBody;
  std::string consumerInterface;
  mlir::OwningOpRef<mlir::ModuleOp> packetHeader;
  mlir::OwningOpRef<mlir::ModuleOp> consumerHeader;
};

TEST_F(SourceUnitHeaderTest, ExtraOrdinaryModuleMetadataIsAccepted) {
  auto header = cloneModule(*packetHeader);
  header->getOperation()->setAttr(
      "sym_name", mlir::StringAttr::get(&context, "packet_header"));
  header->getOperation()->setAttr("test.note",
                                  mlir::StringAttr::get(&context, "ordinary"));
  RegistryResult result = checkRegistry(*header);
  EXPECT_TRUE(result.passed) << result.diagnostic;
}

TEST_F(SourceUnitHeaderTest, RequiredEnvelopeAttributesAreMandatoryAndTyped) {
  for (llvm::StringRef name :
       {"ac.source_owner", "ac.unit_kind", "ac.stage", "ac.interfaces"}) {
    auto missing = cloneModule(*packetHeader);
    missing->getOperation()->removeAttr(name);
    expectRegistryRejects(*missing);
    auto malformed = cloneModule(*packetHeader);
    malformed->getOperation()->setAttr(name, mlir::UnitAttr::get(&context));
    expectRegistryRejects(*malformed);
  }
}

TEST_F(SourceUnitHeaderTest,
       DefinitionsUseEnclosingOwnerAndSnapshotsKeepAuthority) {
  RegistryResult packet = checkRegistry(*packetHeader);
  llvm::SmallVector<mlir::ModuleOp> headers{*packetHeader, *consumerHeader};
  RegistryResult consumer = checkRegistry(headers);
  ASSERT_TRUE(packet.passed) << packet.diagnostic;
  ASSERT_TRUE(consumer.passed) << consumer.diagnostic;

  auto definitionMismatch = cloneModule(*packetHeader);
  findRecord(*definitionMismatch, ".Request")
      ->setAttr("ac.source_owner", owner(context, "other.py"));
  expectRegistryRejects(*definitionMismatch);

  auto snapshotMismatch = cloneModule(*consumerHeader);
  auto snapshot = findRecord(*snapshotMismatch, ".Request");
  ASSERT_TRUE(snapshot);
  ASSERT_EQ(snapshot->getAttrOfType<mlir::StringAttr>("ac.declaration_role")
                .getValue(),
            "import_snapshot");
  EXPECT_EQ(snapshot->getAttrOfType<mlir::DictionaryAttr>("ac.source_owner"),
            owner(context, "packet.py"));
  snapshot->setAttr("ac.source_owner", owner(context, "consumer.py"));
  llvm::SmallVector<mlir::ModuleOp> mismatched{*packetHeader,
                                               *snapshotMismatch};
  RegistryResult mismatch = checkRegistry(mismatched);
  EXPECT_FALSE(mismatch.passed);
  EXPECT_FALSE(mismatch.diagnostic.empty());
}

TEST_F(SourceUnitHeaderTest, DeclarationAndConstructorKeysAreExact) {
  for (llvm::StringRef name :
       {"ac.source_owner", "ac.origin", "ac.declaration_role", "fields",
        "constructor"}) {
    auto header = cloneModule(*packetHeader);
    auto record = findRecord(*header, ".Request");
    ASSERT_TRUE(record->hasAttr(name)) << name.str();
    record->removeAttr(name);
    expectRegistryRejects(*header);
  }
  for (llvm::StringRef name :
       {"ac.source_owner", "ac.origin", "ac.declaration_role", "ac.helper_kind",
        "ac.parameters", "ac.return_form", "ac.result_constraints",
        "ac.check_templates", "ac.record"}) {
    auto header = cloneModule(*packetHeader);
    auto helper = findHelper(*header, ".Request.__init__");
    ASSERT_TRUE(helper->hasAttr(name)) << name.str();
    helper->removeAttr(name);
    expectRegistryRejects(*header);
  }
}

TEST_F(SourceUnitHeaderTest, ParameterMetadataAndPhysicalSignatureMustAgree) {
  auto missingConstraintKind = cloneModule(*packetHeader);
  auto helper = findHelper(*missingConstraintKind, ".Request.__init__");
  auto parameters = helper->getAttrOfType<mlir::ArrayAttr>("ac.parameters");
  llvm::SmallVector<mlir::Attribute> changed(parameters.begin(),
                                             parameters.end());
  auto parameter = mlir::cast<mlir::DictionaryAttr>(changed.front());
  auto constraint = parameter.getAs<mlir::DictionaryAttr>("constraint");
  llvm::SmallVector<mlir::NamedAttribute> constraintFields;
  for (mlir::NamedAttribute field : constraint)
    if (field.getName() != "kind")
      constraintFields.push_back(field);
  mlir::Builder builder(&context);
  llvm::SmallVector<mlir::NamedAttribute> parameterFields;
  for (mlir::NamedAttribute field : parameter)
    parameterFields.push_back(
        field.getName() == "constraint"
            ? builder.getNamedAttr("constraint",
                                   builder.getDictionaryAttr(constraintFields))
            : field);
  changed.front() = builder.getDictionaryAttr(parameterFields);
  helper->setAttr("ac.parameters", builder.getArrayAttr(changed));
  expectRegistryRejects(*missingConstraintKind);

  auto badSignature = cloneModule(*packetHeader);
  helper = findHelper(*badSignature, ".Request.__init__");
  auto type = helper.getFunctionType();
  llvm::SmallVector<mlir::Type> inputs(type.getInputs().drop_back());
  helper.setType(builder.getFunctionType(inputs, type.getResults()));
  expectRegistryRejects(*badSignature);
}

TEST_F(SourceUnitHeaderTest, TypeDefaultNominalReturnAndImpureTamperingFail) {
  mlir::Builder builder(&context);
  auto badType = cloneModule(*packetHeader);
  auto record = findRecord(*badType, ".Request");
  auto fields = record.getFields();
  llvm::SmallVector<mlir::Attribute> changedFields(fields.begin(),
                                                   fields.end());
  auto field = mlir::cast<mlir::DictionaryAttr>(changedFields.front());
  llvm::SmallVector<mlir::NamedAttribute> fieldAttrs(field.begin(),
                                                     field.end());
  for (mlir::NamedAttribute &attribute : fieldAttrs)
    if (attribute.getName() == "type")
      attribute = builder.getNamedAttr(
          "type",
          builder.getDictionaryAttr({
              builder.getNamedAttr("kind", builder.getStringAttr("bool")),
              builder.getNamedAttr("storage",
                                   mlir::TypeAttr::get(builder.getI1Type())),
          }));
  changedFields.front() = builder.getDictionaryAttr(fieldAttrs);
  record->setAttr("fields", builder.getArrayAttr(changedFields));
  expectRegistryRejects(*badType);

  auto badDefault = cloneModule(*packetHeader);
  auto defaultHelper = findHelper(*badDefault, ".Request.__init__");
  auto parameters =
      defaultHelper->getAttrOfType<mlir::ArrayAttr>("ac.parameters");
  llvm::SmallVector<mlir::Attribute> changedParameters(parameters.begin(),
                                                       parameters.end());
  for (size_t index = 0; index < changedParameters.size(); ++index) {
    auto parameter = mlir::cast<mlir::DictionaryAttr>(changedParameters[index]);
    if (parameter.getAs<mlir::StringAttr>("name").getValue() != "value")
      continue;
    llvm::APSInt number(llvm::APInt(9, 256), /*isUnsigned=*/true);
    auto value = builder.getDictionaryAttr({
        builder.getNamedAttr("kind", builder.getStringAttr("integer")),
        builder.getNamedAttr("value", ac::MathIntAttr::get(&context, number)),
    });
    auto defaultValue = builder.getDictionaryAttr({
        builder.getNamedAttr("present", builder.getBoolAttr(true)),
        builder.getNamedAttr("value", value),
    });
    llvm::SmallVector<mlir::NamedAttribute> attributes(parameter.begin(),
                                                       parameter.end());
    for (mlir::NamedAttribute &attribute : attributes)
      if (attribute.getName() == "default")
        attribute = builder.getNamedAttr("default", defaultValue);
    changedParameters[index] = builder.getDictionaryAttr(attributes);
  }
  defaultHelper->setAttr("ac.parameters",
                         builder.getArrayAttr(changedParameters));
  expectRegistryRejects(*badDefault);

  auto badNominal = cloneModule(*packetHeader);
  findHelper(*badNominal, ".Request.__init__")
      ->setAttr("ac.record",
                mlir::FlatSymbolRefAttr::get(&context, "demo.packet.Required"));
  expectRegistryRejects(*badNominal);

  auto badReturn = cloneModule(*packetHeader);
  auto returnHelper = findHelper(*badReturn, ".Request.__init__");
  auto returnType = returnHelper.getFunctionType();
  returnHelper.setType(builder.getFunctionType(
      returnType.getInputs(), {builder.getI8Type(), builder.getI1Type()}));
  expectRegistryRejects(*badReturn);

  auto pureArithmetic = cloneModule(*packetHeader);
  auto pureHelper = findHelper(*pureArithmetic, ".Request.__init__");
  mlir::Block &entry = pureHelper.getBody().front();
  mlir::OpBuilder at(entry.getTerminator());
  mlir::Value path = entry.getArguments().back();
  at.create<mlir::arith::AndIOp>(pureHelper.getLoc(), path, path);
  RegistryResult pure = checkRegistry(*pureArithmetic);
  EXPECT_TRUE(pure.passed) << pure.diagnostic;

  auto recursive = cloneModule(*packetHeader);
  auto recursiveHelper = findHelper(*recursive, ".Request.__init__");
  mlir::Block &recursiveEntry = recursiveHelper.getBody().front();
  mlir::OpBuilder recursiveBuilder(recursiveEntry.getTerminator());
  recursiveBuilder.create<mlir::func::CallOp>(
      recursiveHelper.getLoc(), recursiveHelper, recursiveEntry.getArguments());
  expectRegistryRejects(*recursive);
}

TEST_F(SourceUnitHeaderTest, ConstructorAndConsumerUseDeclaredFieldOrder) {
  auto constructor = findHelper(*packetHeader, ".Request.__init__");
  ASSERT_TRUE(constructor);
  auto parameters =
      constructor->getAttrOfType<mlir::ArrayAttr>("ac.parameters");
  ASSERT_EQ(parameters.size(), 2u);
  size_t validIndex = 0;
  size_t valueIndex = 0;
  for (auto [index, raw] : llvm::enumerate(parameters)) {
    auto parameter = mlir::cast<mlir::DictionaryAttr>(raw);
    auto name = parameter.getAs<mlir::StringAttr>("name").getValue();
    if (name == "valid")
      validIndex = index;
    if (name == "value")
      valueIndex = index;
  }
  ac::StructCreateOp created;
  constructor.walk([&](ac::StructCreateOp candidate) { created = candidate; });
  ASSERT_TRUE(created);
  ASSERT_EQ(created.getValues().size(), 2u);
  mlir::Block &entry = constructor.getBody().front();
  EXPECT_EQ(created.getValues()[0], entry.getArgument(valueIndex));
  EXPECT_EQ(created.getValues()[1], entry.getArgument(validIndex));

  auto keyword = findHelper(*consumerHeader, ".keyword_request");
  auto defaults = findHelper(*consumerHeader, ".default_request");
  auto reader = findHelper(*consumerHeader, ".read_value");
  ASSERT_TRUE(keyword && defaults && reader);
  ac::StructGetOp read;
  reader.walk([&](ac::StructGetOp candidate) { read = candidate; });
  ASSERT_TRUE(read);
  EXPECT_EQ(read.getField(), "value");
  auto expectRequestCall = [&](mlir::func::FuncOp helper, int64_t expectedValid,
                               int64_t expectedValue) {
    mlir::func::CallOp call;
    helper.walk([&](mlir::func::CallOp candidate) { call = candidate; });
    ASSERT_TRUE(call);
    EXPECT_EQ(call.getCallee(), "demo.packet.Request.__init__");
    ASSERT_EQ(call.getOperands().size(), 3u);
    auto valid = call.getOperand(0).getDefiningOp<mlir::arith::ConstantOp>();
    auto value = call.getOperand(1).getDefiningOp<mlir::arith::ConstantOp>();
    ASSERT_TRUE(valid && value);
    EXPECT_EQ(mlir::cast<mlir::IntegerAttr>(valid.getValue())
                  .getValue()
                  .getZExtValue(),
              static_cast<uint64_t>(expectedValid));
    EXPECT_EQ(mlir::cast<mlir::IntegerAttr>(value.getValue())
                  .getValue()
                  .getZExtValue(),
              static_cast<uint64_t>(expectedValue));
  };
  expectRequestCall(keyword, 1, 7);
  expectRequestCall(defaults, 0, 0);

  auto pairConstructor = findHelper(*packetHeader, ".Pair.__init__");
  ASSERT_TRUE(pairConstructor);
  auto pairParameters =
      pairConstructor->getAttrOfType<mlir::ArrayAttr>("ac.parameters");
  ASSERT_EQ(pairParameters.size(), 2u);
  size_t leftIndex = 0;
  size_t rightIndex = 0;
  for (auto [index, raw] : llvm::enumerate(pairParameters)) {
    auto parameter = mlir::cast<mlir::DictionaryAttr>(raw);
    auto name = parameter.getAs<mlir::StringAttr>("name").getValue();
    if (name == "left")
      leftIndex = index;
    if (name == "right")
      rightIndex = index;
  }
  ac::StructCreateOp pairCreated;
  pairConstructor.walk(
      [&](ac::StructCreateOp candidate) { pairCreated = candidate; });
  ASSERT_TRUE(pairCreated);
  mlir::Block &pairEntry = pairConstructor.getBody().front();
  EXPECT_EQ(pairCreated.getValues()[0], pairEntry.getArgument(leftIndex));
  EXPECT_EQ(pairCreated.getValues()[1], pairEntry.getArgument(rightIndex));

  auto expectPairConstants = [&](llvm::StringRef helperSuffix,
                                 int64_t expectedLeft, int64_t expectedRight) {
    auto helper = findHelper(*consumerHeader, helperSuffix);
    ASSERT_TRUE(helper);
    mlir::func::CallOp call;
    helper.walk([&](mlir::func::CallOp candidate) { call = candidate; });
    ASSERT_TRUE(call);
    EXPECT_EQ(call.getCallee(), "demo.packet.Pair.__init__");
    ASSERT_EQ(call.getOperands().size(), 3u);
    auto right = call.getOperand(0).getDefiningOp<mlir::arith::ConstantOp>();
    auto left = call.getOperand(1).getDefiningOp<mlir::arith::ConstantOp>();
    ASSERT_TRUE(left && right);
    EXPECT_EQ(mlir::cast<mlir::IntegerAttr>(left.getValue())
                  .getValue()
                  .getZExtValue(),
              static_cast<uint64_t>(expectedLeft));
    EXPECT_EQ(mlir::cast<mlir::IntegerAttr>(right.getValue())
                  .getValue()
                  .getZExtValue(),
              static_cast<uint64_t>(expectedRight));
  };
  expectPairConstants(".keyword_pair", 10, 20);
  expectPairConstants(".default_pair", 1, 2);

  auto readLeft = findHelper(*consumerHeader, ".read_left");
  ASSERT_TRUE(readLeft);
  ac::StructGetOp leftRead;
  readLeft.walk([&](ac::StructGetOp candidate) { leftRead = candidate; });
  ASSERT_TRUE(leftRead);
  EXPECT_EQ(leftRead.getField(), "left");
}

TEST_F(SourceUnitHeaderTest,
       ConstructorAndParameterOriginsUseConstructorRelativePaths) {
  auto constructor = findHelper(*packetHeader, ".Request.__init__");
  ASSERT_TRUE(constructor);
  auto constructorOrigin =
      constructor->getAttrOfType<mlir::DictionaryAttr>("ac.origin");
  auto constructorSite = constructorOrigin.getAs<mlir::DictionaryAttr>("site");
  auto constructorDefinition =
      constructorSite.getAs<mlir::FlatSymbolRefAttr>("definition");
  EXPECT_EQ(constructorDefinition.getValue(), "demo.packet.Request.__init__");
  EXPECT_TRUE(constructorSite.getAs<mlir::ArrayAttr>("ast_path").empty());

  auto parameters =
      constructor->getAttrOfType<mlir::ArrayAttr>("ac.parameters");
  ASSERT_EQ(parameters.size(), 2u);
  for (mlir::Attribute raw : parameters) {
    auto parameter = mlir::cast<mlir::DictionaryAttr>(raw);
    auto origin = parameter.getAs<mlir::DictionaryAttr>("origin");
    auto site = origin.getAs<mlir::DictionaryAttr>("site");
    EXPECT_EQ(site.getAs<mlir::FlatSymbolRefAttr>("definition"),
              constructorDefinition);
    auto path = site.getAs<mlir::ArrayAttr>("ast_path");
    EXPECT_FALSE(path.empty());
  }
}

} // namespace
} // namespace acir::compiler
