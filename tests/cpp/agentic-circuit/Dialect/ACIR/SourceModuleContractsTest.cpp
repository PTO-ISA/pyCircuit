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

struct ModuleTemporaryDirectory {
  llvm::SmallString<256> path;
  ModuleTemporaryDirectory() {
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("acir-source-module", path));
  }
  ~ModuleTemporaryDirectory() { llvm::sys::fs::remove_directories(path); }
  std::string child(llvm::StringRef name) const {
    llvm::SmallString<256> result(path);
    llvm::sys::path::append(result, name);
    return result.str().str();
  }
};

void moduleWrite(llvm::StringRef path, llvm::StringRef contents) {
  std::error_code error;
  llvm::raw_fd_ostream output(path, error);
  ASSERT_FALSE(error);
  output << contents;
}

std::string moduleRead(llvm::StringRef path) {
  auto buffer = llvm::MemoryBuffer::getFile(path);
  EXPECT_TRUE(static_cast<bool>(buffer));
  return buffer ? buffer.get()->getBuffer().str() : std::string{};
}

int moduleRun(llvm::StringRef program,
              const std::vector<std::string> &ownedArguments,
              llvm::StringRef log) {
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const std::string &argument : ownedArguments)
    arguments.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {std::nullopt,
                                                                   log, log};
  return llvm::sys::ExecuteAndWait(program, arguments, std::nullopt, redirects);
}

int moduleEmit(llvm::StringRef source, llvm::StringRef root,
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
  return moduleRun(ACIR_TEST_PYTHON,
                   {ACIR_TEST_PYTHON, "-c", script.str(), ACIR_TEST_REPO_ROOT,
                    source.str(), root.str(), output.str()},
                   log);
}

int moduleCompile(llvm::StringRef capture, llvm::StringRef sourcePath,
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
  return moduleRun(ACIR_TEST_SOURCE_UNIT_HARNESS, arguments, log);
}

mlir::OwningOpRef<mlir::ModuleOp> moduleParse(llvm::StringRef path,
                                              mlir::MLIRContext &context) {
  return mlir::parseSourceFile<mlir::ModuleOp>(path, &context);
}

mlir::OwningOpRef<mlir::ModuleOp> moduleClone(mlir::ModuleOp module) {
  return mlir::OwningOpRef<mlir::ModuleOp>(
      mlir::cast<mlir::ModuleOp>(module->clone()));
}

mlir::Operation *findOperation(mlir::ModuleOp module, llvm::StringRef name) {
  mlir::Operation *found = nullptr;
  module.walk([&](mlir::Operation *operation) {
    if (operation->getName().getStringRef() == name) {
      EXPECT_EQ(found, nullptr) << "duplicate operation " << name.str();
      found = operation;
    }
  });
  return found;
}

mlir::DictionaryAttr replaceField(mlir::Builder &builder,
                                  mlir::DictionaryAttr dictionary,
                                  llvm::StringRef name, mlir::Attribute value) {
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

struct RegistryResult {
  bool accepted;
  std::string diagnostic;
};

RegistryResult moduleRegistry(llvm::ArrayRef<mlir::ModuleOp> headers) {
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

class SourceModuleContractsTest : public ::testing::Test {
protected:
  SourceModuleContractsTest() : context(dialects) {
    dialects.insert<ac::ACIRDialect, mlir::arith::ArithDialect,
                    mlir::func::FuncDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    root = temporary.child("source");
    ASSERT_FALSE(llvm::sys::fs::create_directories(root));
    std::string fixtureRoot = ACIR_TEST_REPO_ROOT
        "/tests/integration/pycircuit/fixtures/migration_c1";
    std::string packetSource = root + "/packet.py";
    std::string accumulatorSource = root + "/accumulator_probe.py";
    std::string rootSource = root + "/probe_root.py";
    moduleWrite(packetSource, moduleRead(fixtureRoot + "/packet.py"));
    moduleWrite(accumulatorSource, R"py(from pycircuit import module, rule
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
    moduleWrite(rootSource, R"py(from pycircuit import module
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
    std::string accumulatorTransport =
        temporary.child("accumulator.transport.mlir");
    std::string rootTransport = temporary.child("probe-root.transport.mlir");
    packetInterface = temporary.child("packet.interface.mlir");
    accumulatorInterface = temporary.child("accumulator.interface.mlir");
    accumulatorBody = temporary.child("accumulator.body.mlir");
    rootBody = temporary.child("probe-root.body.mlir");
    ASSERT_EQ(moduleEmit(packetSource, root, packetTransport,
                         temporary.child("packet-python.log")),
              0);
    ASSERT_EQ(moduleEmit(accumulatorSource, root, accumulatorTransport,
                         temporary.child("accumulator-python.log")),
              0);
    ASSERT_EQ(moduleEmit(rootSource, root, rootTransport,
                         temporary.child("probe-root-python.log")),
              0);
    ASSERT_EQ(moduleCompile(packetTransport, "packet.py",
                            temporary.child("packet.body.mlir"),
                            packetInterface, temporary.child("packet.log")),
              0);
    ASSERT_EQ(moduleCompile(accumulatorTransport, "accumulator_probe.py",
                            accumulatorBody, accumulatorInterface,
                            temporary.child("accumulator.log"),
                            {packetInterface}),
              0);
    ASSERT_EQ(moduleCompile(rootTransport, "probe_root.py", rootBody,
                            temporary.child("probe-root.interface.mlir"),
                            temporary.child("probe-root.log"),
                            {packetInterface, accumulatorInterface}),
              0);
    packetHeader = moduleParse(packetInterface, context);
    accumulatorHeader = moduleParse(accumulatorInterface, context);
    accumulatorImplementation = moduleParse(accumulatorBody, context);
    rootImplementation = moduleParse(rootBody, context);
    ASSERT_TRUE(packetHeader && accumulatorHeader &&
                accumulatorImplementation && rootImplementation);
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context;
  ModuleTemporaryDirectory temporary;
  std::string root, packetInterface, accumulatorInterface, accumulatorBody,
      rootBody;
  mlir::OwningOpRef<mlir::ModuleOp> packetHeader, accumulatorHeader,
      accumulatorImplementation, rootImplementation;
};

TEST_F(SourceModuleContractsTest, HeaderPublishesOneClosedModuleSignature) {
  mlir::Builder builder(&context);
  mlir::Operation *declaration =
      findOperation(*accumulatorHeader, "ac.module.import");
  ASSERT_NE(declaration, nullptr);
  EXPECT_EQ(declaration->getNumOperands(), 0u);
  EXPECT_EQ(declaration->getNumResults(), 0u);
  EXPECT_EQ(declaration->getNumRegions(), 0u);
  EXPECT_TRUE(declaration->getAttrOfType<mlir::StringAttr>("sym_name"));
  EXPECT_TRUE(
      declaration->getAttrOfType<mlir::DictionaryAttr>("ac.source_owner"));
  EXPECT_TRUE(declaration->getAttrOfType<mlir::DictionaryAttr>("ac.origin"));
  EXPECT_TRUE(
      declaration->getAttrOfType<mlir::StringAttr>("ac.declaration_role"));
  auto contract =
      declaration->getAttrOfType<mlir::DictionaryAttr>("ac.contract");
  ASSERT_TRUE(contract);
  auto parameters = contract.getAs<mlir::ArrayAttr>("parameters");
  auto connections = contract.getAs<mlir::ArrayAttr>("connections");
  ASSERT_TRUE(parameters && connections);
  ASSERT_EQ(parameters.size(), 2u);
  ASSERT_EQ(connections.size(), 2u);
  EXPECT_EQ(mlir::cast<mlir::DictionaryAttr>(parameters[0])
                .getAs<mlir::StringAttr>("name")
                .getValue(),
            "request");
  EXPECT_EQ(mlir::cast<mlir::DictionaryAttr>(parameters[1])
                .getAs<mlir::StringAttr>("name")
                .getValue(),
            "result");
  auto checkParameter = [&](size_t index, llvm::StringRef name,
                            uint64_t argumentIndex) {
    auto parameter = mlir::cast<mlir::DictionaryAttr>(parameters[index]);
    EXPECT_EQ(parameter.getAs<mlir::StringAttr>("name").getValue(), name);
    EXPECT_EQ(parameter.getAs<mlir::StringAttr>("binding").getValue(),
              "positional_or_keyword");
    EXPECT_EQ(parameter.getAs<mlir::StringAttr>("category").getValue(),
              "connection");
    auto defaultValue = parameter.getAs<mlir::DictionaryAttr>("default");
    ASSERT_TRUE(defaultValue);
    EXPECT_FALSE(defaultValue.getAs<mlir::BoolAttr>("present").getValue());
    auto origin = parameter.getAs<mlir::DictionaryAttr>("origin");
    auto site = origin.getAs<mlir::DictionaryAttr>("site");
    EXPECT_EQ(site.getAs<mlir::FlatSymbolRefAttr>("definition").getValue(),
              "demo.accumulator_probe.AccumulatorProbe");
    auto path = site.getAs<mlir::ArrayAttr>("ast_path");
    ASSERT_EQ(path.size(), 5u);
    EXPECT_EQ(mlir::cast<mlir::DictionaryAttr>(path[0])
                  .getAs<mlir::StringAttr>("name")
                  .getValue(),
              "body");
    EXPECT_EQ(mlir::cast<mlir::DictionaryAttr>(path[1])
                  .getAs<mlir::IntegerAttr>("value")
                  .getInt(),
              0);
    EXPECT_EQ(mlir::cast<mlir::DictionaryAttr>(path[4])
                  .getAs<mlir::IntegerAttr>("value")
                  .getInt(),
              argumentIndex);
    auto location = parameter.getAs<mlir::DictionaryAttr>("location");
    EXPECT_EQ(location.getAs<mlir::StringAttr>("path").getValue(),
              "accumulator_probe.py");
    EXPECT_EQ(location.getAs<mlir::IntegerAttr>("line").getInt(), 6);
  };
  checkParameter(0, "request", 1);
  checkParameter(1, "result", 2);
  auto requestType = mlir::cast<mlir::DictionaryAttr>(parameters[0])
                         .getAs<mlir::DictionaryAttr>("type");
  EXPECT_EQ(requestType.getAs<mlir::StringAttr>("kind").getValue(), "record");
  EXPECT_EQ(requestType.getAs<mlir::FlatSymbolRefAttr>("symbol").getValue(),
            "demo.packet.Request");
  auto resultType = mlir::cast<mlir::DictionaryAttr>(parameters[1])
                        .getAs<mlir::DictionaryAttr>("type");
  EXPECT_EQ(resultType.getAs<mlir::StringAttr>("kind").getValue(), "integer");
  EXPECT_EQ(resultType.getAs<mlir::TypeAttr>("storage").getValue(),
            builder.getIntegerType(8));
  EXPECT_FALSE(resultType.get("symbol"));

  auto checkConnection = [&](size_t index, llvm::StringRef name, bool read,
                             bool write) {
    auto connection = mlir::cast<mlir::DictionaryAttr>(connections[index]);
    EXPECT_EQ(connection.getAs<mlir::StringAttr>("parameter").getValue(), name);
    auto elements = connection.getAs<mlir::ArrayAttr>("elements");
    ASSERT_EQ(elements.size(), 1u);
    auto effect = mlir::cast<mlir::DictionaryAttr>(elements[0]);
    EXPECT_TRUE(effect.getAs<mlir::UnitAttr>("ordinal"));
    EXPECT_EQ(effect.getAs<mlir::BoolAttr>("read").getValue(), read);
    EXPECT_EQ(effect.getAs<mlir::BoolAttr>("write").getValue(), write);
    EXPECT_EQ(effect.getAs<mlir::StringAttr>("precision").getValue(), "exact");
    auto origins = effect.getAs<mlir::ArrayAttr>("origins");
    EXPECT_FALSE(origins.empty());
  };
  checkConnection(0, "request", true, false);
  checkConnection(1, "result", false, true);
}

TEST_F(SourceModuleContractsTest, RegistryAcceptsTheOwningModuleAuthority) {
  llvm::SmallVector<mlir::ModuleOp> headers{*packetHeader, *accumulatorHeader};
  RegistryResult result = moduleRegistry(headers);
  EXPECT_TRUE(result.accepted) << result.diagnostic;
}

TEST_F(SourceModuleContractsTest, ProbeBodyHasOneOwnedStateAndRegisteredRule) {
  mlir::Operation *module =
      findOperation(*accumulatorImplementation, "ac.module");
  mlir::Operation *state = findOperation(*accumulatorImplementation, "ac.dffe");
  mlir::Operation *rule = findOperation(*accumulatorImplementation, "ac.rule");
  ASSERT_NE(module, nullptr);
  ASSERT_NE(state, nullptr);
  ASSERT_NE(rule, nullptr);

  EXPECT_EQ(state->getAttrOfType<mlir::StringAttr>("name").getValue(), "total");
  EXPECT_EQ(state->getAttrOfType<mlir::StringAttr>("ac.domain").getValue(),
            "default");
  EXPECT_TRUE(state->getAttrOfType<mlir::DictionaryAttr>("ac.logical_element"));
  EXPECT_TRUE(state->getAttrOfType<mlir::DictionaryAttr>("ac.declaration"));
  auto shape = state->getAttrOfType<mlir::ArrayAttr>("ac.shape");
  ASSERT_TRUE(shape);
  EXPECT_TRUE(shape.empty());
  auto initial = state->getAttrOfType<mlir::DictionaryAttr>("ac.initial_value");
  ASSERT_TRUE(initial);
  EXPECT_EQ(initial.getAs<mlir::StringAttr>("kind").getValue(), "scalar");
  EXPECT_TRUE(initial.getAs<ac::StaticExprAttr>("value"));

  EXPECT_EQ(rule->getAttrOfType<mlir::StringAttr>("name").getValue(),
            "forward");
  EXPECT_EQ(rule->getNumOperands(), 2u);
  EXPECT_EQ(rule->getNumResults(), 0u);
  ASSERT_EQ(rule->getNumRegions(), 4u);
  EXPECT_TRUE(rule->getRegion(0).empty());
  EXPECT_TRUE(rule->getRegion(1).empty());
  EXPECT_TRUE(rule->getRegion(3).empty());
  ASSERT_TRUE(rule->getRegion(2).hasOneBlock());
  mlir::Block &body = rule->getRegion(2).front();
  EXPECT_EQ(body.getNumArguments(), 1u);
  auto *yield = &body.back();
  EXPECT_EQ(yield->getName().getStringRef(), "ac.yield");
  EXPECT_EQ(yield->getNumOperands(), 2u);
  EXPECT_TRUE(yield->getOperand(1).getType().isInteger(1));
}

TEST_F(SourceModuleContractsTest, ProbeRootUsesSourceInstanceContract) {
  mlir::Operation *instance = findOperation(*rootImplementation, "ac.instance");
  ASSERT_NE(instance, nullptr);
  EXPECT_TRUE(instance->getAttrOfType<mlir::FlatSymbolRefAttr>("callee"));
  EXPECT_TRUE(instance->getAttrOfType<mlir::ArrayAttr>("ac.static_args"));
  EXPECT_TRUE(instance->getAttrOfType<mlir::DictionaryAttr>("ac.origin"));
  EXPECT_EQ(instance->getNumOperands(), 2u);
  EXPECT_EQ(instance->getNumResults(), 0u);
}

TEST_F(SourceModuleContractsTest, DffeMetadataIsMandatoryAndClosed) {
  for (llvm::StringRef attribute :
       {"ac.logical_element", "ac.initial_value", "ac.shape", "ac.domain",
        "ac.declaration"}) {
    SCOPED_TRACE(attribute.str());
    auto implementation = moduleClone(*accumulatorImplementation);
    mlir::Operation *state = findOperation(*implementation, "ac.dffe");
    ASSERT_NE(state, nullptr);
    state->removeAttr(attribute);
    EXPECT_TRUE(mlir::failed(mlir::verify(*implementation)));
  }
  auto implementation = moduleClone(*accumulatorImplementation);
  mlir::Operation *state = findOperation(*implementation, "ac.dffe");
  state->setAttr("ac.initial_value", mlir::StringAttr::get(&context, "bad"));
  EXPECT_TRUE(mlir::failed(mlir::verify(*implementation)));
}

TEST_F(SourceModuleContractsTest, RuleShapeAndBindingsAreMandatory) {
  for (llvm::StringRef attribute : {"ac.input_bindings", "ac.output_bindings",
                                    "ac.input_types", "ac.output_types"}) {
    SCOPED_TRACE(attribute.str());
    auto implementation = moduleClone(*accumulatorImplementation);
    mlir::Operation *rule = findOperation(*implementation, "ac.rule");
    ASSERT_NE(rule, nullptr);
    rule->removeAttr(attribute);
    EXPECT_TRUE(mlir::failed(mlir::verify(*implementation)));
  }

  auto implementation = moduleClone(*accumulatorImplementation);
  mlir::Operation *rule = findOperation(*implementation, "ac.rule");
  rule->getRegion(2).front().back().eraseOperand(1);
  EXPECT_TRUE(mlir::failed(mlir::verify(*implementation)));

  implementation = moduleClone(*accumulatorImplementation);
  rule = findOperation(*implementation, "ac.rule");
  rule->getRegion(2).front().addArgument(mlir::IntegerType::get(&context, 1),
                                         mlir::UnknownLoc::get(&context));
  EXPECT_TRUE(mlir::failed(mlir::verify(*implementation)));

  implementation = moduleClone(*accumulatorImplementation);
  rule = findOperation(*implementation, "ac.rule");
  rule->getRegion(0).emplaceBlock();
  EXPECT_TRUE(mlir::failed(mlir::verify(*implementation)));
}

TEST_F(SourceModuleContractsTest, InstanceCalleeAndStaticArgsAreMandatory) {
  auto implementation = moduleClone(*rootImplementation);
  mlir::Operation *instance = findOperation(*implementation, "ac.instance");
  ASSERT_NE(instance, nullptr);
  instance->setAttr("callee", mlir::StringAttr::get(&context, "bad"));
  EXPECT_TRUE(mlir::failed(mlir::verify(*implementation)));

  implementation = moduleClone(*rootImplementation);
  instance = findOperation(*implementation, "ac.instance");
  instance->removeAttr("ac.static_args");
  EXPECT_TRUE(mlir::failed(mlir::verify(*implementation)));
}

TEST_F(SourceModuleContractsTest, MissingAndMalformedContractsAreRejected) {
  for (bool wrongType : {false, true}) {
    auto header = moduleClone(*accumulatorHeader);
    mlir::Operation *declaration = findOperation(*header, "ac.module.import");
    ASSERT_NE(declaration, nullptr);
    if (wrongType)
      declaration->setAttr("ac.contract",
                           mlir::StringAttr::get(&context, "bad"));
    else
      declaration->removeAttr("ac.contract");
    EXPECT_TRUE(mlir::failed(mlir::verify(*header)));
  }
}

TEST_F(SourceModuleContractsTest, SignatureAndWriterShapesAreRejected) {
  mlir::Builder builder(&context);
  auto header = moduleClone(*accumulatorHeader);
  mlir::Operation *declaration = findOperation(*header, "ac.module.import");
  ASSERT_NE(declaration, nullptr);
  auto contract =
      declaration->getAttrOfType<mlir::DictionaryAttr>("ac.contract");
  auto connections = contract.getAs<mlir::ArrayAttr>("connections");
  llvm::SmallVector<mlir::Attribute> values(connections.begin(),
                                            connections.end());
  auto first = mlir::cast<mlir::DictionaryAttr>(values[0]);
  auto effects = first.getAs<mlir::ArrayAttr>("elements");
  llvm::SmallVector<mlir::Attribute> effectValues(effects.begin(),
                                                  effects.end());
  auto effect = mlir::cast<mlir::DictionaryAttr>(effectValues[0]);
  effectValues[0] =
      replaceField(builder, effect, "write", builder.getStringAttr("bad"));
  values[0] = replaceField(builder, first, "elements",
                           builder.getArrayAttr(effectValues));
  contract = replaceField(builder, contract, "connections",
                          builder.getArrayAttr(values));
  declaration->setAttr("ac.contract", contract);
  EXPECT_TRUE(mlir::failed(mlir::verify(*header)));

  header = moduleClone(*accumulatorHeader);
  declaration = findOperation(*header, "ac.module.import");
  contract = declaration->getAttrOfType<mlir::DictionaryAttr>("ac.contract");
  connections = contract.getAs<mlir::ArrayAttr>("connections");
  values.assign(connections.begin(), connections.end());
  first = mlir::cast<mlir::DictionaryAttr>(values[0]);
  values[0] = replaceField(builder, first, "parameter",
                           builder.getStringAttr("missing"));
  declaration->setAttr("ac.contract",
                       replaceField(builder, contract, "connections",
                                    builder.getArrayAttr(values)));
  EXPECT_TRUE(mlir::failed(mlir::verify(*header)));
}

TEST_F(SourceModuleContractsTest, EffectOriginsUseNumericAstPathOrdering) {
  mlir::Builder builder(&context);
  auto header = moduleClone(*accumulatorHeader);
  mlir::Operation *declaration = findOperation(*header, "ac.module.import");
  auto contract =
      declaration->getAttrOfType<mlir::DictionaryAttr>("ac.contract");
  auto connections = contract.getAs<mlir::ArrayAttr>("connections");
  auto connection = mlir::cast<mlir::DictionaryAttr>(connections[0]);
  auto elements = connection.getAs<mlir::ArrayAttr>("elements");
  auto effect = mlir::cast<mlir::DictionaryAttr>(elements[0]);
  auto origins = effect.getAs<mlir::ArrayAttr>("origins");
  ASSERT_FALSE(origins.empty());
  auto base = mlir::cast<mlir::DictionaryAttr>(origins[0]);
  auto withIndex = [&](uint64_t value) {
    auto site = base.getAs<mlir::DictionaryAttr>("site");
    auto path = site.getAs<mlir::ArrayAttr>("ast_path");
    llvm::SmallVector<mlir::Attribute> components(path.begin(), path.end());
    components.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("kind", builder.getStringAttr("index")),
        builder.getNamedAttr("value", builder.getI64IntegerAttr(value)),
    }));
    return replaceField(builder, base, "site",
                        replaceField(builder, site, "ast_path",
                                     builder.getArrayAttr(components)));
  };
  auto install = [&](mlir::ArrayAttr orderedOrigins) {
    auto updatedEffect =
        replaceField(builder, effect, "origins", orderedOrigins);
    auto updatedConnection = replaceField(
        builder, connection, "elements", builder.getArrayAttr({updatedEffect}));
    return replaceField(
        builder, contract, "connections",
        builder.getArrayAttr({updatedConnection, connections[1]}));
  };
  declaration->setAttr("ac.contract", install(builder.getArrayAttr(
                                          {withIndex(2), withIndex(10)})));
  EXPECT_TRUE(mlir::succeeded(mlir::verify(*header)));
  declaration->setAttr("ac.contract", install(builder.getArrayAttr(
                                          {withIndex(10), withIndex(2)})));
  EXPECT_TRUE(mlir::failed(mlir::verify(*header)));
}

TEST_F(SourceModuleContractsTest, OwnerAndSnapshotTamperingAreRejected) {
  mlir::Builder builder(&context);
  auto ownerHeader = moduleClone(*accumulatorHeader);
  mlir::Operation *declaration =
      findOperation(*ownerHeader, "ac.module.import");
  ASSERT_NE(declaration, nullptr);
  auto owner =
      declaration->getAttrOfType<mlir::DictionaryAttr>("ac.source_owner");
  declaration->setAttr(
      "ac.source_owner",
      replaceField(builder, owner, "path", builder.getStringAttr("other.py")));
  EXPECT_TRUE(mlir::failed(mlir::verify(*ownerHeader)));

  auto snapshot = moduleClone(*accumulatorHeader);
  declaration = findOperation(*snapshot, "ac.module.import");
  declaration->setAttr("ac.declaration_role",
                       builder.getStringAttr("import_snapshot"));
  llvm::SmallVector<mlir::ModuleOp> headers{*packetHeader, *snapshot};
  RegistryResult result = moduleRegistry(headers);
  EXPECT_FALSE(result.accepted);
  EXPECT_FALSE(result.diagnostic.empty());
}

} // namespace
} // namespace acir::compiler
