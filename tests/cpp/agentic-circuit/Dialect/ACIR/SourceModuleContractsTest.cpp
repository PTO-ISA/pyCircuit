#include "Compiler/PythonImportRules.h"
#include "Compiler/SourceUnit.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringSet.h"
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

TEST(SourceRuleAnalysisTest, InputIndicesAreResolvedThroughBoundArguments) {
  auto makePlan = [] {
    detail::RulePlan plan;
    plan.arguments.push_back({"formal", 2, {}, {}});
    plan.arguments.push_back({"same", 2, {}, {}});
    plan.arguments.push_back({"other", 0, {}, {}});
    plan.arguments.push_back({"alias", 2, {}, {}});
    return plan;
  };

  detail::RulePlan formalFirst = makePlan();
  EXPECT_EQ(detail::bindRuleInput(formalFirst, 0), 0u);
  EXPECT_EQ(detail::bindRuleInput(formalFirst, 1), 0u);
  EXPECT_EQ(detail::bindRuleInput(formalFirst, 3), 0u);
  EXPECT_EQ(detail::bindRuleInput(formalFirst, 2), 1u);
  EXPECT_EQ(formalFirst.inputs, (llvm::SmallVector<size_t>{0, 2}));

  detail::RulePlan memberFirst = makePlan();
  EXPECT_EQ(detail::bindRuleInput(memberFirst, 1), 0u);
  EXPECT_EQ(detail::bindRuleInput(memberFirst, 0), 0u);
  EXPECT_EQ(detail::bindRuleInput(memberFirst, 2), 1u);
  EXPECT_EQ(memberFirst.inputs, (llvm::SmallVector<size_t>{1, 2}));
  EXPECT_EQ(detail::rulePlanInputSlot(memberFirst, 2), 0u);
  EXPECT_EQ(detail::rulePlanInputSlot(memberFirst, 0), 1u);
}

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

void replaceRuleBodyWithIdentity(mlir::Operation *rule,
                                 mlir::MLIRContext &context) {
  mlir::Block &body = rule->getRegion(0).front();
  body.getArgument(0).setType(mlir::IntegerType::get(&context, 8));
  while (!body.empty())
    body.back().erase();
  mlir::OpBuilder builder(&context);
  builder.setInsertionPointToEnd(&body);
  auto enabled = builder.create<mlir::arith::ConstantOp>(
      mlir::UnknownLoc::get(&context), builder.getI1Type(),
      builder.getBoolAttr(true));
  mlir::OperationState yield(mlir::UnknownLoc::get(&context),
                             ac::YieldOp::getOperationName());
  yield.addOperands({body.getArgument(0), enabled.getResult()});
  builder.create(yield);
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
def AccumulatorProbe(request: Request, result: Word):
    total: Word = 0

    @rule
    def forward():
        nonlocal result
        result = request.value
        return

    forward()
)py");
    moduleWrite(rootSource, R"py(from pycircuit import module
from .packet import Request, Word
from .accumulator_probe import AccumulatorProbe

@module
def ProbeRoot():
    request: Request = Request(3, True)
    result: Word = 0
    child = AccumulatorProbe(request, result)
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

TEST_F(SourceModuleContractsTest,
       LexicalAssignmentTargetsReadIndicesButNotStoredBaseIdentities) {
  std::string source = root + "/target_reads.py";
  std::string transportPath = temporary.child("target-reads.transport.mlir");
  moduleWrite(source, R"py(def scan(item):
    items[index] = item
    packet.field.leaf = item
    (left, items[tuple_index]) = item
    [right, items[list_index]] = item
    value = loaded.value
)py");
  ASSERT_EQ(moduleEmit(source, root, transportPath,
                       temporary.child("target-reads-python.log")),
            0);
  auto transport = moduleParse(transportPath, context);
  ASSERT_TRUE(transport);
  auto emit = [&]() -> mlir::InFlightDiagnostic {
    return mlir::emitError(mlir::UnknownLoc::get(&context));
  };
  auto captured = detail::readSingleCapture(*transport, emit);
  ASSERT_TRUE(mlir::succeeded(captured));
  detail::AstNode function = captured->module.item("body", 0);
  ASSERT_EQ(function.kind(), "FunctionDef");

  llvm::StringSet<> formals;
  llvm::StringSet<> members;
  llvm::StringSet<> targetFormals;
  llvm::StringSet<> targetMembers;
  auto formalRead = [&](llvm::StringRef name, const detail::AstNode &) {
    formals.insert(name);
  };
  auto memberRead = [&](llvm::StringRef name, const detail::AstNode &) {
    members.insert(name);
  };
  mlir::ArrayAttr statements = function.array("body");
  for (size_t index = 0; index < statements.size(); ++index) {
    detail::AstNode statement = function.item("body", index);
    detail::collectRuleStatementReads(statement, formalRead, memberRead);
    mlir::ArrayAttr targets = statement.array("targets");
    for (size_t targetIndex = 0; targetIndex < targets.size(); ++targetIndex)
      detail::collectRuleExpressionReads(
          statement.item("targets", targetIndex),
          [&](llvm::StringRef name, const detail::AstNode &) {
            targetFormals.insert(name);
          },
          [&](llvm::StringRef name, const detail::AstNode &) {
            targetMembers.insert(name);
          });
  }

  for (llvm::StringRef read :
       {"item", "index", "tuple_index", "list_index", "loaded"})
    EXPECT_TRUE(formals.contains(read)) << read.str();
  for (llvm::StringRef stored : {"items", "packet", "left", "right"})
    EXPECT_FALSE(formals.contains(stored)) << stored.str();
  EXPECT_TRUE(members.empty());
  for (llvm::StringRef read : {"index", "tuple_index", "list_index"})
    EXPECT_TRUE(targetFormals.contains(read)) << read.str();
  for (llvm::StringRef stored : {"items", "packet", "left", "right", "loaded"})
    EXPECT_FALSE(targetFormals.contains(stored)) << stored.str();
  EXPECT_TRUE(targetMembers.empty());
}

TEST_F(SourceModuleContractsTest, RetiredClassSelfModuleIsAHardError) {
  std::string source = root + "/retired_class.py";
  std::string transport = temporary.child("retired-class.transport.mlir");
  std::string log = temporary.child("retired-class.log");
  moduleWrite(source, R"py(from pycircuit import module
from .packet import Word

@module
class Retired:
    def __init__(self):
        self.value: Word = 0
)py");
  ASSERT_EQ(moduleEmit(source, root, transport,
                       temporary.child("retired-class-python.log")),
            0);
  EXPECT_NE(moduleCompile(transport, "retired_class.py",
                          temporary.child("retired-class.body.mlir"),
                          temporary.child("retired-class.interface.mlir"), log,
                          {packetInterface}),
            0);
  EXPECT_NE(moduleRead(log).find("class/self authoring has been retired"),
            std::string::npos);
}

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
    ASSERT_EQ(path.size(), 3u);
    EXPECT_EQ(mlir::cast<mlir::DictionaryAttr>(path[2])
                  .getAs<mlir::IntegerAttr>("value")
                  .getInt(),
              argumentIndex);
    auto location = parameter.getAs<mlir::DictionaryAttr>("location");
    EXPECT_EQ(location.getAs<mlir::StringAttr>("path").getValue(),
              "accumulator_probe.py");
    EXPECT_EQ(location.getAs<mlir::IntegerAttr>("line").getInt(), 5);
  };
  checkParameter(0, "request", 0);
  checkParameter(1, "result", 1);
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
  mlir::Operation *state = findOperation(*accumulatorImplementation, "ac.reg");
  mlir::Operation *rule = findOperation(*accumulatorImplementation, "ac.rule");
  ASSERT_NE(module, nullptr);
  ASSERT_NE(state, nullptr);
  ASSERT_NE(rule, nullptr);

  EXPECT_EQ(state->getAttrOfType<mlir::StringAttr>("name").getValue(), "total");
  EXPECT_EQ(state->getAttrOfType<mlir::StringAttr>("ac.domain").getValue(),
            "default");
  EXPECT_TRUE(state->getAttrOfType<mlir::DictionaryAttr>("ac.logical_element"));
  EXPECT_TRUE(state->getAttrOfType<mlir::DictionaryAttr>("ac.declaration"));
  ASSERT_EQ(state->getNumOperands(), 2u);
  mlir::Block &moduleBody = module->getRegion(0).front();
  EXPECT_EQ(state->getOperand(0), moduleBody.getArgument(0));
  EXPECT_EQ(state->getOperand(1), moduleBody.getArgument(1));
  EXPECT_TRUE(mlir::isa<ac::RegType>(state->getResult(0).getType()));
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
  EXPECT_EQ(rule->getNumResults(), 2u);
  ASSERT_EQ(rule->getNumRegions(), 1u);
  ASSERT_TRUE(rule->getRegion(0).hasOneBlock());
  mlir::Block &body = rule->getRegion(0).front();
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
  EXPECT_EQ(instance->getNumOperands(), 4u);
  EXPECT_EQ(instance->getNumResults(), 2u);
  auto *module = findOperation(*rootImplementation, "ac.module");
  ASSERT_NE(module, nullptr);
  mlir::Block &moduleBody = module->getRegion(0).front();
  EXPECT_EQ(instance->getOperand(0), moduleBody.getArgument(0));
  EXPECT_EQ(instance->getOperand(1), moduleBody.getArgument(1));
}

TEST_F(SourceModuleContractsTest, RegMetadataIsMandatoryAndClosed) {
  for (llvm::StringRef attribute :
       {"ac.logical_element", "ac.initial_value", "ac.shape", "ac.domain",
        "ac.declaration"}) {
    SCOPED_TRACE(attribute.str());
    auto implementation = moduleClone(*accumulatorImplementation);
    mlir::Operation *state = findOperation(*implementation, "ac.reg");
    ASSERT_NE(state, nullptr);
    state->removeAttr(attribute);
    EXPECT_TRUE(mlir::failed(mlir::verify(*implementation)));
  }
  auto implementation = moduleClone(*accumulatorImplementation);
  mlir::Operation *state = findOperation(*implementation, "ac.reg");
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
  rule->getRegion(0).front().back().eraseOperand(1);
  EXPECT_TRUE(mlir::failed(mlir::verify(*implementation)));

  implementation = moduleClone(*accumulatorImplementation);
  rule = findOperation(*implementation, "ac.rule");
  rule->getRegion(0).front().addArgument(mlir::IntegerType::get(&context, 1),
                                         mlir::UnknownLoc::get(&context));
  EXPECT_TRUE(mlir::failed(mlir::verify(*implementation)));

  implementation = moduleClone(*accumulatorImplementation);
  rule = findOperation(*implementation, "ac.rule");
  rule->getRegion(0).emplaceBlock();
  EXPECT_TRUE(mlir::failed(mlir::verify(*implementation)));

  implementation = moduleClone(*accumulatorImplementation);
  rule = findOperation(*implementation, "ac.rule");
  mlir::OpBuilder builder(rule);
  mlir::OperationState retired(rule->getLoc(), ac::RuleOp::getOperationName());
  retired.addOperands(rule->getOperands());
  retired.addTypes(rule->getResultTypes());
  retired.addAttributes(rule->getAttrs());
  for (unsigned index = 0; index < 4; ++index)
    retired.addRegion();
  builder.create(retired);
  EXPECT_TRUE(mlir::failed(mlir::verify(*implementation)));
}

TEST_F(SourceModuleContractsTest,
       RuleInputsAreBoundToTheirActualFormalOrOwnedAuthority) {
  mlir::Builder builder(&context);
  EXPECT_TRUE(mlir::succeeded(mlir::verify(*accumulatorImplementation)));

  {
    auto implementation = moduleClone(*accumulatorImplementation);
    mlir::Operation *rule = findOperation(*implementation, "ac.rule");
    auto bindings = rule->getAttrOfType<mlir::ArrayAttr>("ac.input_bindings");
    auto binding = mlir::cast<mlir::DictionaryAttr>(bindings[0]);
    binding = replaceField(builder, binding, "parameter",
                           builder.getStringAttr("result"));
    rule->setAttr("ac.input_bindings", builder.getArrayAttr({binding}));
    EXPECT_TRUE(mlir::failed(mlir::verify(*implementation)));
  }

  {
    auto implementation = moduleClone(*accumulatorImplementation);
    mlir::Operation *rule = findOperation(*implementation, "ac.rule");
    auto outputBindings =
        rule->getAttrOfType<mlir::ArrayAttr>("ac.output_bindings");
    auto outputTypes = rule->getAttrOfType<mlir::ArrayAttr>("ac.output_types");
    rule->setOperand(0, rule->getOperand(1));
    rule->setAttr("ac.input_bindings",
                  builder.getArrayAttr({outputBindings[0]}));
    rule->setAttr("ac.input_types", builder.getArrayAttr({outputTypes[0]}));
    replaceRuleBodyWithIdentity(rule, context);
    EXPECT_TRUE(mlir::failed(mlir::verify(*implementation)));
  }

  auto owned = moduleClone(*accumulatorImplementation);
  mlir::Operation *rule = findOperation(*owned, "ac.rule");
  mlir::Operation *state = findOperation(*owned, "ac.reg");
  ASSERT_NE(rule, nullptr);
  ASSERT_NE(state, nullptr);
  auto ownedBinding = builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("owned")),
      builder.getNamedAttr(
          "declaration",
          state->getAttrOfType<mlir::DictionaryAttr>("ac.declaration")),
      builder.getNamedAttr("element", builder.getArrayAttr({})),
  });
  rule->setOperand(0, state->getResult(0));
  rule->setAttr("ac.input_bindings", builder.getArrayAttr({ownedBinding}));
  rule->setAttr("ac.input_types",
                builder.getArrayAttr({state->getAttr("ac.logical_element")}));
  replaceRuleBodyWithIdentity(rule, context);
  EXPECT_TRUE(mlir::succeeded(mlir::verify(*owned)));

  auto declaration = ownedBinding.getAs<mlir::DictionaryAttr>("declaration");
  auto site = declaration.getAs<mlir::DictionaryAttr>("site");
  site = replaceField(builder, site, "definition",
                      mlir::FlatSymbolRefAttr::get(&context, "demo.Other"));
  declaration = replaceField(builder, declaration, "site", site);
  ownedBinding =
      replaceField(builder, ownedBinding, "declaration", declaration);
  rule->setAttr("ac.input_bindings", builder.getArrayAttr({ownedBinding}));
  EXPECT_TRUE(mlir::failed(mlir::verify(*owned)));
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
