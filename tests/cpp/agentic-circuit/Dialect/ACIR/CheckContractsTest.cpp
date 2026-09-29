#include "Compiler/CheckGraph.h"
#include "Compiler/ModuleGraph.h"
#include "Compiler/ObservationGraph.h"
#include "Compiler/ProposalGraph.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACIR/ACIROps.h"

#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
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
#include <initializer_list>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace acir::ac {
namespace {

using Field = std::pair<llvm::StringRef, mlir::Attribute>;

mlir::DictionaryAttr dictionary(mlir::Builder &builder,
                                std::initializer_list<Field> fields) {
  llvm::SmallVector<mlir::NamedAttribute> attributes;
  for (const auto &[name, value] : fields)
    attributes.push_back(builder.getNamedAttr(name, value));
  return builder.getDictionaryAttr(attributes);
}

mlir::IntegerAttr u64(mlir::Builder &builder, uint64_t value) {
  return builder.getIntegerAttr(builder.getI64Type(), value);
}

mlir::DictionaryAttr occurrence(mlir::Builder &builder, uint64_t index) {
  auto path = builder.getArrayAttr(
      {dictionary(builder, {{"kind", builder.getStringAttr("index")},
                            {"value", u64(builder, index)}})});
  auto site = dictionary(
      builder, {{"definition", mlir::FlatSymbolRefAttr::get(
                                   builder.getContext(), "verify.Checks")},
                {"ast_path", path}});
  return dictionary(builder,
                    {{"site", site}, {"expansion", builder.getArrayAttr({})}});
}

mlir::DictionaryAttr location(mlir::Builder &builder) {
  return dictionary(builder, {{"path", builder.getStringAttr("checks.py")},
                              {"line", u64(builder, 9)},
                              {"column", u64(builder, 9)},
                              {"end_line", u64(builder, 9)},
                              {"end_column", u64(builder, 28)}});
}

mlir::DictionaryAttr checkID(mlir::Builder &builder, uint64_t registration,
                             uint64_t site, uint64_t obligation = 0) {
  return dictionary(builder,
                    {{"registration", occurrence(builder, registration)},
                     {"check", occurrence(builder, site)},
                     {"obligation", u64(builder, obligation)}});
}

bool expectIsRegistered(mlir::MLIRContext &context) {
  return static_cast<bool>(
      mlir::RegisteredOperationName::lookup("ac.expect", &context));
}

struct CheckGraph {
  mlir::OwningOpRef<mlir::ModuleOp> container;
  RuleOp rule;
  mlir::Operation *read = nullptr;
  mlir::Operation *expect = nullptr;
};

CheckGraph buildCheckGraph(mlir::MLIRContext &context) {
  mlir::OpBuilder builder(&context);
  auto loc = mlir::FileLineColLoc::get(&context, "checks.py", 9, 9);
  auto container = mlir::ModuleOp::create(loc);
  container->setAttr("ac.stage", builder.getStringAttr("source"));
  container->setAttr("ac.unit_kind", builder.getStringAttr("implementation"));
  auto owner =
      dictionary(builder, {{"package", builder.getStringAttr("verify")},
                           {"path", builder.getStringAttr("checks.py")}});
  container->setAttr("ac.source_owner", owner);
  builder.setInsertionPointToStart(container.getBody());

  mlir::OperationState moduleState(loc, ModuleOp::getOperationName());
  moduleState.addAttribute("name", builder.getStringAttr("Checks"));
  moduleState.addAttribute("sym_name", builder.getStringAttr("verify.Checks"));
  moduleState.addAttribute("ac.source_owner", owner);
  moduleState.addAttribute("ac.origin", occurrence(builder, 0));
  auto logicalBool = dictionary(
      builder, {{"kind", builder.getStringAttr("bool")},
                {"storage", mlir::TypeAttr::get(builder.getI1Type())}});
  auto span = location(builder);
  auto port = dictionary(builder, {{"parameter", builder.getStringAttr("flag")},
                                   {"ordinal", builder.getUnitAttr()},
                                   {"role", builder.getStringAttr("current")},
                                   {"type", logicalBool},
                                   {"origin", occurrence(builder, 1)},
                                   {"location", span}});
  moduleState.addAttribute("ac.ports", builder.getArrayAttr({port}));
  moduleState.addAttribute("ac.control_ports",
                           dictionary(builder,
                                      {{"clock", builder.getI32IntegerAttr(0)},
                                       {"reset", builder.getI32IntegerAttr(1)}}));
  moduleState.addRegion();
  auto module = mlir::cast<ModuleOp>(builder.create(moduleState));
  auto *moduleBody = new mlir::Block();
  module.getBody().push_back(moduleBody);
  moduleBody->addArgument(builder.getI1Type(), loc);
  moduleBody->addArgument(builder.getI1Type(), loc);
  moduleBody->addArgument(RegType::get(&context, builder.getI1Type()), loc);

  builder.setInsertionPointToEnd(moduleBody);
  mlir::OperationState ruleState(loc, RuleOp::getOperationName());
  ruleState.addOperands(moduleBody->getArgument(2));
  ruleState.addAttribute("name", builder.getStringAttr("check"));
  auto registration = occurrence(builder, 2);
  ruleState.addAttribute("registration", registration);
  ruleState.addAttribute("operandSegmentSizes",
                         builder.getDenseI32ArrayAttr({1, 0}));
  ruleState.addAttribute("ac.source_owner", owner);
  ruleState.addAttribute("ac.origin", occurrence(builder, 3));
  auto stateRef =
      dictionary(builder, {{"kind", builder.getStringAttr("formal")},
                           {"parameter", builder.getStringAttr("flag")},
                           {"ordinal", builder.getUnitAttr()}});
  ruleState.addAttribute("ac.input_bindings", builder.getArrayAttr({stateRef}));
  ruleState.addAttribute("ac.output_bindings", builder.getArrayAttr({}));
  ruleState.addAttribute("ac.input_types", builder.getArrayAttr({logicalBool}));
  ruleState.addAttribute("ac.output_types", builder.getArrayAttr({}));
  auto id = checkID(builder, 2, 4);
  auto required =
      dictionary(builder, {{"id", id},
                           {"kind", builder.getStringAttr("assert")},
                           {"location", span}});
  ruleState.addAttribute("ac.required_checks",
                         builder.getArrayAttr({required}));
  ruleState.addRegion();
  auto rule = mlir::cast<RuleOp>(builder.create(ruleState));
  auto *ruleBody = new mlir::Block();
  rule.getBody().push_back(ruleBody);
  ruleBody->addArgument(builder.getI1Type(), loc);
  builder.setInsertionPointToEnd(ruleBody);

  mlir::OperationState readState(loc, SourceReadOp::getOperationName());
  readState.addOperands(ruleBody->getArgument(0));
  readState.addTypes(builder.getI1Type());
  readState.addAttribute("ac.origin", occurrence(builder, 5));
  mlir::Operation *read = builder.create(readState);
  auto path = mlir::arith::ConstantOp::create(builder, loc, builder.getI1Type(),
                                              builder.getBoolAttr(true));
  mlir::OperationState expectState(loc, "ac.expect");
  expectState.addOperands({read->getResult(0), path.getResult()});
  expectState.addAttribute("kind", builder.getStringAttr("assert"));
  expectState.addAttribute("ac.check_id", id);
  expectState.addAttribute("location", span);
  mlir::Operation *expect = builder.create(expectState);
  builder.create(mlir::OperationState(loc, YieldOp::getOperationName()));
  builder.setInsertionPointToEnd(moduleBody);
  builder.create(mlir::OperationState(loc, YieldOp::getOperationName()));
  return {std::move(container), rule, read, expect};
}

class CheckContractsTest : public ::testing::Test {
protected:
  CheckContractsTest() {
    context.loadDialect<ACIRDialect, mlir::arith::ArithDialect>();
  }

  void requireCapability() {
    ASSERT_TRUE(expectIsRegistered(context))
        << "W09 RED: ac.expect is not registered; source assert requires "
           "condition,path plus CheckID/RequiredCheck verification";
  }

  mlir::OwningOpRef<mlir::ModuleOp> clone(mlir::ModuleOp module) {
    return mlir::OwningOpRef<mlir::ModuleOp>(
        mlir::cast<mlir::ModuleOp>(module->clone()));
  }

  mlir::MLIRContext context;
};

TEST_F(CheckContractsTest, CanonicalAssertCheckParsesAndVerifies) {
  requireCapability();
  if (!expectIsRegistered(context))
    return;
  CheckGraph graph = buildCheckGraph(context);
  EXPECT_TRUE(mlir::succeeded(mlir::verify(*graph.container)));
}

TEST_F(CheckContractsTest, RequiredCheckRejectsDeleteAndDuplicate) {
  requireCapability();
  if (!expectIsRegistered(context))
    return;
  CheckGraph graph = buildCheckGraph(context);
  auto deleted = clone(*graph.container);
  mlir::Operation *expect = nullptr;
  deleted->walk([&](mlir::Operation *operation) {
    if (operation->getName().getStringRef() == "ac.expect")
      expect = operation;
  });
  ASSERT_NE(expect, nullptr);
  expect->erase();
  EXPECT_TRUE(mlir::failed(mlir::verify(*deleted)));

  auto duplicate = clone(*graph.container);
  expect = nullptr;
  duplicate->walk([&](mlir::Operation *operation) {
    if (operation->getName().getStringRef() == "ac.expect")
      expect = operation;
  });
  ASSERT_NE(expect, nullptr);
  expect->getBlock()->getOperations().insert(expect->getIterator(),
                                             expect->clone());
  EXPECT_TRUE(mlir::failed(mlir::verify(*duplicate)));
}

TEST_F(CheckContractsTest, RedirectedConditionAndPathReject) {
  requireCapability();
  if (!expectIsRegistered(context))
    return;
  CheckGraph graph = buildCheckGraph(context);
  auto redirected = clone(*graph.container);
  mlir::Operation *expect = nullptr;
  redirected->walk([&](mlir::Operation *operation) {
    if (operation->getName().getStringRef() == "ac.expect")
      expect = operation;
  });
  ASSERT_NE(expect, nullptr);
  expect->setOperand(0, expect->getOperand(1));
  EXPECT_TRUE(mlir::failed(mlir::verify(*redirected)));

  auto wrongPath = clone(*graph.container);
  expect = nullptr;
  wrongPath->walk([&](mlir::Operation *operation) {
    if (operation->getName().getStringRef() == "ac.expect")
      expect = operation;
  });
  ASSERT_NE(expect, nullptr);
  expect->setOperand(1, expect->getOperand(0));
  EXPECT_TRUE(mlir::failed(mlir::verify(*wrongPath)));
}

TEST_F(CheckContractsTest, ScopeKindLocationAndRequiredMutationsReject) {
  requireCapability();
  if (!expectIsRegistered(context))
    return;
  CheckGraph graph = buildCheckGraph(context);
  mlir::Builder builder(&context);
  for (llvm::StringRef mutation : {"scope", "kind", "location", "required"}) {
    SCOPED_TRACE(mutation.str());
    auto mutated = clone(*graph.container);
    mlir::Operation *expect = nullptr;
    mutated->walk([&](mlir::Operation *operation) {
      if (operation->getName().getStringRef() == "ac.expect")
        expect = operation;
    });
    auto module = *mutated->getOps<ModuleOp>().begin();
    auto rule = *module.getBody().front().getOps<RuleOp>().begin();
    ASSERT_NE(expect, nullptr);
    if (mutation == "scope")
      expect->setAttr("ac.check_id", checkID(builder, 99, 4));
    else if (mutation == "kind")
      expect->setAttr("kind", builder.getStringAttr("range"));
    else if (mutation == "location")
      expect->setAttr(
          "location",
          dictionary(builder, {{"path", builder.getStringAttr("x.py")},
                               {"line", u64(builder, 1)},
                               {"column", u64(builder, 1)},
                               {"end_line", u64(builder, 1)},
                               {"end_column", u64(builder, 2)}}));
    else
      rule->setAttr("ac.required_checks", builder.getArrayAttr({}));
    EXPECT_TRUE(mlir::failed(mlir::verify(*mutated)));
  }
}

TEST(CheckGraphClosureTest, PrivateCheckGraphApiIsRequired) {
  SUCCEED() << "Compiler/CheckGraph.h is available for binding tests";
}

struct TemporaryDirectory {
  llvm::SmallString<256> path;
  TemporaryDirectory() {
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("acir-check-graph", path));
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

int compileCapture(llvm::StringRef capture, llvm::StringRef sourcePath,
                   llvm::StringRef body, llvm::StringRef header,
                   llvm::StringRef log,
                   llvm::ArrayRef<std::string> dependencies = {}) {
  std::vector<std::string> arguments = {ACIR_TEST_SOURCE_UNIT_HARNESS,
                                        "--capture",
                                        capture.str(),
                                        "--package",
                                        "verify",
                                        "--path",
                                        sourcePath.str()};
  for (const std::string &dependency : dependencies)
    arguments.insert(arguments.end(), {"--header", dependency});
  arguments.insert(arguments.end(),
                   {"--body-out", body.str(), "--interface-out", header.str()});
  return run(ACIR_TEST_SOURCE_UNIT_HARNESS, arguments, log);
}

class PythonCheckGraphTest : public ::testing::Test {
protected:
  PythonCheckGraphTest() : context(dialects) {
    dialects.insert<ACIRDialect, mlir::arith::ArithDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    root = temporary.child("source");
    ASSERT_FALSE(llvm::sys::fs::create_directories(root));
    std::string source = root + "/checked.py";
    writeFile(source, R"py(from pycircuit import module, rule, log

@module
def Checked():
    flag: bool = True
    value: bool = True
    sink: bool = False

    @rule
    def step():
        nonlocal sink
        assert flag, "flag"
        assert value, "value"
        log("info", "sample", value)
        sink = value
        return

    step()
)py");
    std::string capture = temporary.child("checked.transport.mlir");
    bodyPath = temporary.child("checked.body.mlir");
    headerPath = temporary.child("checked.interface.mlir");
    ASSERT_EQ(emitCapture(source, root, capture, temporary.child("python.log")),
              0);
    ASSERT_EQ(compileCapture(capture, "checked.py", bodyPath, headerPath,
                             temporary.child("compile.log")),
              0);
    body = mlir::parseSourceFile<mlir::ModuleOp>(bodyPath, &context);
    header = mlir::parseSourceFile<mlir::ModuleOp>(headerPath, &context);
    ASSERT_TRUE(body && header);
  }

  std::pair<std::optional<compiler::ModuleGraph>, std::string>
  buildModules(mlir::ModuleOp selectedBody = {}) {
    std::string diagnostic;
    mlir::ScopedDiagnosticHandler capture(
        &context, [&](mlir::Diagnostic &value) {
          llvm::raw_string_ostream(diagnostic) << value;
          return mlir::success();
        });
    auto emit = [&]() -> mlir::InFlightDiagnostic {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
    llvm::SmallVector<mlir::ModuleOp> headers{*header};
    auto registry = compiler::SourceHeaderRegistry::create(headers, emit);
    if (mlir::failed(registry))
      return {std::nullopt, diagnostic};
    llvm::SmallVector<compiler::SourceLinkUnit> units{
        {selectedBody ? selectedBody : *body, *header}};
    if (mlir::failed(compiler::admitSourceLinkUnits(units, emit)))
      return {std::nullopt, diagnostic};
    auto modules = compiler::buildSourceModuleGraph(units, *registry, emit);
    if (mlir::failed(modules))
      return {std::nullopt, diagnostic};
    return {std::move(*modules), diagnostic};
  }

  std::pair<bool, std::string> verifyChecks(compiler::CheckGraph &checks) {
    std::string diagnostic;
    mlir::ScopedDiagnosticHandler capture(
        &context, [&](mlir::Diagnostic &value) {
          llvm::raw_string_ostream(diagnostic) << value;
          return mlir::success();
        });
    auto emit = [&]() -> mlir::InFlightDiagnostic {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
    return {mlir::succeeded(compiler::verifySourceChecks(checks, emit)),
            diagnostic};
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context{dialects};
  TemporaryDirectory temporary;
  std::string root, bodyPath, headerPath;
  mlir::OwningOpRef<mlir::ModuleOp> body, header;
};

TEST_F(PythonCheckGraphTest, BindingFieldsAndActualBijectionRejectMutations) {
  auto built = buildModules();
  ASSERT_TRUE(built.first.has_value()) << built.second;
  compiler::ModuleGraph modules = std::move(*built.first);
  std::string diagnostic;
  auto emit = [&]() -> mlir::InFlightDiagnostic {
    return mlir::emitError(mlir::UnknownLoc::get(&context));
  };
  auto checks = compiler::buildSourceCheckGraph(modules, emit);
  ASSERT_TRUE(mlir::succeeded(checks));
  ASSERT_EQ(checks->bindings.size(), 2u);
  ASSERT_TRUE(verifyChecks(*checks).first);

  auto expectBindingMutationRejected = [&](auto mutate, llvm::StringRef name) {
    auto mutated = *checks;
    mutate(mutated.bindings.front());
    auto result = verifyChecks(mutated);
    EXPECT_FALSE(result.first) << name.str() << " mutation was accepted";
  };
  mlir::Builder builder(&context);
  expectBindingMutationRejected([](auto &binding) { ++binding.stableOrdinal; },
                                "stableOrdinal");
  expectBindingMutationRejected([](auto &binding) { ++binding.requiredIndex; },
                                "requiredIndex");
  expectBindingMutationRejected([](auto &binding) { binding.owner = nullptr; },
                                "owner");
  expectBindingMutationRejected(
      [&](auto &binding) {
        binding.ownerRef = dictionary(
            builder, {{"instance_path",
                       builder.getArrayAttr({occurrence(builder, 999)})}});
      },
      "ownerRef");
  expectBindingMutationRejected([](auto &binding) { binding.rule = {}; },
                                "rule");
  expectBindingMutationRejected([](auto &binding) { binding.expect = {}; },
                                "expect");
  expectBindingMutationRejected(
      [&](auto &binding) { binding.registration = occurrence(builder, 999); },
      "registration");
  expectBindingMutationRejected(
      [&](auto &binding) { binding.checkID = checkID(builder, 999, 998); },
      "checkID");
  expectBindingMutationRejected(
      [&](auto &binding) { binding.kind = builder.getStringAttr("range"); },
      "kind");
  expectBindingMutationRejected(
      [&](auto &binding) {
        binding.location =
            dictionary(builder, {{"path", builder.getStringAttr("other.py")},
                                 {"line", u64(builder, 1)},
                                 {"column", u64(builder, 1)},
                                 {"end_line", u64(builder, 1)},
                                 {"end_column", u64(builder, 2)}});
      },
      "location");
  expectBindingMutationRejected(
      [](auto &binding) { binding.condition = binding.path; }, "condition");
  expectBindingMutationRejected(
      [](auto &binding) { binding.path = binding.condition; }, "path");

  auto rule = checks->bindings.front().rule;
  auto required = rule->getAttrOfType<mlir::ArrayAttr>("ac.required_checks");
  rule->setAttr("ac.required_checks",
                builder.getArrayAttr({required[1], required[0]}));
  EXPECT_FALSE(verifyChecks(*checks).first)
      << "required check reorder was accepted";
  rule->setAttr("ac.required_checks", required);

  auto expect = checks->bindings.front().expect;
  mlir::Value originalCondition = expect.getCondition();
  mlir::Value originalPath = expect.getPath();
  expect->setOperand(0, originalPath);
  EXPECT_FALSE(verifyChecks(*checks).first)
      << "same-type condition redirect was accepted";
  expect->setOperand(0, originalCondition);
  expect->setOperand(1, originalCondition);
  EXPECT_FALSE(verifyChecks(*checks).first)
      << "same-type path redirect was accepted";
  expect->setOperand(1, originalPath);

  mlir::Operation *duplicate = expect->clone();
  rule.getBody().front().getOperations().insert(
      rule.getBody().front().getTerminator()->getIterator(), duplicate);
  EXPECT_FALSE(verifyChecks(*checks).first)
      << "duplicate actual check was accepted";
  duplicate->erase();
  expect->erase();
  EXPECT_FALSE(verifyChecks(*checks).first)
      << "deleted actual check was accepted";
}

TEST_F(PythonCheckGraphTest,
       HeaderBodyCheckProposalObservationAndLinkClosureSucceeds) {
  auto built = buildModules();
  ASSERT_TRUE(built.first.has_value()) << built.second;
  compiler::ModuleGraph modules = std::move(*built.first);
  std::string diagnostic;
  mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &value) {
    llvm::raw_string_ostream(diagnostic) << value;
    return mlir::success();
  });
  auto emit = [&]() -> mlir::InFlightDiagnostic {
    return mlir::emitError(mlir::UnknownLoc::get(&context));
  };
  auto checks = compiler::buildSourceCheckGraph(modules, emit);
  ASSERT_TRUE(mlir::succeeded(checks)) << diagnostic;
  ASSERT_TRUE(mlir::succeeded(compiler::verifySourceChecks(*checks, emit)))
      << diagnostic;
  auto proposals = compiler::buildSourceProposalGraph(modules, &*checks, emit);
  ASSERT_TRUE(mlir::succeeded(proposals)) << diagnostic;
  EXPECT_FALSE(proposals->globalPermitAlways);
  for (const compiler::StateProposals &state : proposals->states)
    EXPECT_FALSE(state.commit.permitAlways);
  EXPECT_TRUE(
      mlir::succeeded(compiler::verifySourceProposals(*proposals, emit)))
      << diagnostic;
  auto observations = compiler::buildSourceObservationGraph(modules, emit);
  ASSERT_TRUE(mlir::succeeded(observations)) << diagnostic;
  EXPECT_TRUE(
      mlir::succeeded(compiler::verifySourceObservations(*observations, emit)))
      << diagnostic;

  auto missingChecks = *proposals;
  missingChecks.checks = nullptr;
  missingChecks.globalPermitAlways = false;
  for (compiler::StateProposals &state : missingChecks.states)
    state.commit.permitAlways = false;
  EXPECT_TRUE(
      mlir::failed(compiler::verifySourceProposals(missingChecks, emit)))
      << "checks were deleted while forged false permit was accepted";
}

TEST_F(PythonCheckGraphTest, LinkRejectsMutatedAssertConditionPathAndRequired) {
  for (llvm::StringRef mutation : {"condition", "path", "required"}) {
    SCOPED_TRACE(mutation.str());
    auto mutated = mlir::OwningOpRef<mlir::ModuleOp>(
        mlir::cast<mlir::ModuleOp>((*body)->clone()));
    auto module = *mutated->getOps<ModuleOp>().begin();
    auto rule = *module.getBody().front().getOps<RuleOp>().begin();
    auto expect = *rule.getBody().front().getOps<SourceExpectOp>().begin();
    if (mutation == "condition")
      expect->setOperand(0, expect.getPath());
    else if (mutation == "path")
      expect->setOperand(1, expect.getCondition());
    else
      rule->setAttr("ac.required_checks",
                    mlir::Builder(&context).getArrayAttr({}));
    auto rejected = buildModules(*mutated);
    EXPECT_FALSE(rejected.first.has_value())
        << mutation.str() << " mutation passed source link";
    EXPECT_FALSE(rejected.second.empty());
  }
}

TEST(TwoInstanceObservationGraphTest,
     OwnerRefAndStableOrdinalIgnoreUnitInputOrder) {
  TemporaryDirectory temporary;
  std::string root = temporary.child("source");
  ASSERT_FALSE(llvm::sys::fs::create_directories(root));
  std::string leafSource = root + "/leaf.py";
  std::string parentSource = root + "/parent.py";
  writeFile(leafSource, R"py(from pycircuit import module, rule, log

@module
def Leaf(source: bool, sink: bool):
    @rule
    def sample():
        nonlocal sink
        log("info", "same-site", source)
        sink = source
        return
    sample()
)py");
  writeFile(parentSource, R"py(from pycircuit import module
from .leaf import Leaf

@module
def Parent(source: bool, sink: bool):
    relay: bool = False
    left = Leaf(source, relay)
    right = Leaf(relay, sink)
)py");
  std::string leafCapture = temporary.child("leaf.transport.mlir");
  std::string parentCapture = temporary.child("parent.transport.mlir");
  std::string leafBodyPath = temporary.child("leaf.body.mlir");
  std::string leafHeaderPath = temporary.child("leaf.interface.mlir");
  std::string parentBodyPath = temporary.child("parent.body.mlir");
  std::string parentHeaderPath = temporary.child("parent.interface.mlir");
  ASSERT_EQ(emitCapture(leafSource, root, leafCapture,
                        temporary.child("leaf-python.log")),
            0);
  ASSERT_EQ(emitCapture(parentSource, root, parentCapture,
                        temporary.child("parent-python.log")),
            0);
  ASSERT_EQ(compileCapture(leafCapture, "leaf.py", leafBodyPath, leafHeaderPath,
                           temporary.child("leaf.log")),
            0);
  ASSERT_EQ(compileCapture(parentCapture, "parent.py", parentBodyPath,
                           parentHeaderPath, temporary.child("parent.log"),
                           {leafHeaderPath}),
            0);

  mlir::DialectRegistry dialects;
  dialects.insert<ACIRDialect, mlir::arith::ArithDialect>();
  mlir::MLIRContext context(dialects);
  context.loadAllAvailableDialects();
  auto leafBody = mlir::parseSourceFile<mlir::ModuleOp>(leafBodyPath, &context);
  auto leafHeader =
      mlir::parseSourceFile<mlir::ModuleOp>(leafHeaderPath, &context);
  auto parentBody =
      mlir::parseSourceFile<mlir::ModuleOp>(parentBodyPath, &context);
  auto parentHeader =
      mlir::parseSourceFile<mlir::ModuleOp>(parentHeaderPath, &context);
  ASSERT_TRUE(leafBody && leafHeader && parentBody && parentHeader);

  auto build = [&](bool reversed) {
    std::string diagnostic;
    mlir::ScopedDiagnosticHandler capture(
        &context, [&](mlir::Diagnostic &value) {
          llvm::raw_string_ostream(diagnostic) << value;
          return mlir::success();
        });
    auto emit = [&]() -> mlir::InFlightDiagnostic {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
    llvm::SmallVector<mlir::ModuleOp> headers{*leafHeader, *parentHeader};
    auto registry = compiler::SourceHeaderRegistry::create(headers, emit);
    EXPECT_TRUE(mlir::succeeded(registry)) << diagnostic;
    llvm::SmallVector<compiler::SourceLinkUnit> units;
    if (reversed)
      units.append({{*parentBody, *parentHeader}, {*leafBody, *leafHeader}});
    else
      units.append({{*leafBody, *leafHeader}, {*parentBody, *parentHeader}});
    auto modules = compiler::buildSourceModuleGraph(units, *registry, emit);
    EXPECT_TRUE(mlir::succeeded(modules)) << diagnostic;
    return compiler::buildSourceObservationGraph(*modules, emit);
  };

  auto forward = build(false);
  auto reversed = build(true);
  ASSERT_TRUE(mlir::succeeded(forward));
  ASSERT_TRUE(mlir::succeeded(reversed));
  ASSERT_EQ(forward->bindings.size(), 2u);
  ASSERT_EQ(reversed->bindings.size(), 2u);
  EXPECT_NE(forward->bindings[0].ownerRef, forward->bindings[1].ownerRef);
  for (size_t index = 0; index < 2; ++index) {
    EXPECT_EQ(forward->bindings[index].stableOrdinal, index);
    EXPECT_EQ(reversed->bindings[index].stableOrdinal, index);
    EXPECT_EQ(forward->bindings[index].ownerRef,
              reversed->bindings[index].ownerRef);
    EXPECT_EQ(forward->bindings[index].observationID,
              reversed->bindings[index].observationID);
  }
}

} // namespace
} // namespace acir::ac
