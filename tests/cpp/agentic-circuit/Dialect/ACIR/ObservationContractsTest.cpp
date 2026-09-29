#include "Compiler/ObservationGraph.h"
#include "acir/Dialect/ACIR/ACIRAttributes.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "acir/Dialect/ACIR/ACIRTypes.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <array>
#include <cstdlib>
#include <initializer_list>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace acir::ac {
namespace {

using namespace acir::compiler;

struct TemporaryDirectory {
  llvm::SmallString<256> path;
  TemporaryDirectory() {
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("acir-observation-graph", path));
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
sys.path[:0] = [str(root / "python/semantic-core/src"), str(root / "python/pycircuit/src"), str(root)]
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport
source = Path(sys.argv[2])
Path(sys.argv[4]).write_text(_emit_source_transport(_capture_source_file(source, source_root=Path(sys.argv[3]))), encoding="utf-8")
)py";
  return run(ACIR_TEST_PYTHON,
             {ACIR_TEST_PYTHON, "-c", script.str(), ACIR_TEST_REPO_ROOT,
              source.str(), root.str(), output.str()},
             log);
}

int compileCapture(llvm::StringRef capture, llvm::StringRef sourcePath,
                   llvm::StringRef body, llvm::StringRef header,
                   llvm::StringRef log) {
  return run(ACIR_TEST_SOURCE_UNIT_HARNESS,
             {ACIR_TEST_SOURCE_UNIT_HARNESS, "--capture", capture.str(),
              "--package", "verify", "--path", sourcePath.str(), "--body-out",
              body.str(), "--interface-out", header.str()},
             log);
}

using Field = std::pair<llvm::StringRef, mlir::Attribute>;

mlir::DictionaryAttr dictionary(mlir::Builder &builder,
                                std::initializer_list<Field> fields) {
  llvm::SmallVector<mlir::NamedAttribute> attributes;
  for (const auto &[name, value] : fields)
    attributes.push_back(builder.getNamedAttr(name, value));
  return builder.getDictionaryAttr(attributes);
}

mlir::IntegerAttr u32(mlir::Builder &builder, uint32_t value) {
  return builder.getIntegerAttr(builder.getI32Type(), value);
}

mlir::IntegerAttr u64(mlir::Builder &builder, uint64_t value) {
  return builder.getIntegerAttr(builder.getI64Type(), value);
}

mlir::DictionaryAttr occurrence(mlir::Builder &builder, uint64_t index) {
  auto path = builder.getArrayAttr(
      {dictionary(builder, {{"kind", builder.getStringAttr("index")},
                            {"value", u64(builder, index)}})});
  return dictionary(
      builder,
      {{"site",
        dictionary(builder, {{"definition",
                              mlir::FlatSymbolRefAttr::get(builder.getContext(),
                                                           "verify.Observed")},
                             {"ast_path", path}})},
       {"expansion", builder.getArrayAttr({})}});
}

mlir::DictionaryAttr sourceOwner(mlir::Builder &builder) {
  return dictionary(builder, {{"package", builder.getStringAttr("verify")},
                              {"path", builder.getStringAttr("observed.py")}});
}

mlir::DictionaryAttr logicalBool(mlir::Builder &builder) {
  return dictionary(builder,
                    {{"kind", builder.getStringAttr("bool")},
                     {"storage", mlir::TypeAttr::get(builder.getI1Type())}});
}

mlir::DictionaryAttr logicalU8(mlir::Builder &builder,
                               bool signedDomain = false) {
  return dictionary(
      builder,
      {{"kind", builder.getStringAttr("integer")},
       {"storage", mlir::TypeAttr::get(builder.getI8Type())},
       {"lower",
        MathIntAttr::get(builder.getContext(),
                         llvm::APSInt(llvm::APInt(9, signedDomain ? -128 : 0,
                                                  /*isSigned=*/signedDomain),
                                      /*isUnsigned=*/!signedDomain))},
       {"upper",
        MathIntAttr::get(builder.getContext(),
                         llvm::APSInt(llvm::APInt(9, signedDomain ? 128 : 256,
                                                  /*isSigned=*/signedDomain),
                                      /*isUnsigned=*/!signedDomain))},
       {"interpretation",
        builder.getStringAttr(signedDomain ? "signed" : "unsigned")}});
}

mlir::DictionaryAttr valueConstraint(mlir::Builder &builder,
                                     mlir::DictionaryAttr logical) {
  return dictionary(
      builder, {{"kind", builder.getStringAttr("logical")}, {"type", logical}});
}

mlir::DictionaryAttr formal(mlir::Builder &builder) {
  return dictionary(builder, {{"kind", builder.getStringAttr("formal")},
                              {"parameter", builder.getStringAttr("value")},
                              {"ordinal", builder.getUnitAttr()}});
}

mlir::DictionaryAttr valueID(mlir::Builder &builder,
                             mlir::DictionaryAttr origin) {
  return dictionary(builder, {{"origin", origin}, {"slot", u32(builder, 0)}});
}

mlir::DictionaryAttr observationID(mlir::Builder &builder,
                                   mlir::DictionaryAttr registration,
                                   mlir::DictionaryAttr site) {
  return dictionary(builder, {{"registration", registration}, {"site", site}});
}

mlir::DictionaryAttr item(mlir::Builder &builder) {
  return dictionary(builder, {{"kind", builder.getStringAttr("value")},
                              {"ordinal", u32(builder, 0)}});
}

mlir::DictionaryAttr spec(mlir::Builder &builder, llvm::StringRef kind) {
  if (kind == "print")
    return dictionary(builder,
                      {{"items", builder.getArrayAttr({item(builder)})},
                       {"sep", builder.getStringAttr(" ")},
                       {"end", builder.getStringAttr("\n")}});
  if (kind == "log")
    return dictionary(builder,
                      {{"level", builder.getStringAttr("info")},
                       {"event", builder.getStringAttr("sample")},
                       {"items", builder.getArrayAttr({item(builder)})}});
  return dictionary(builder, {{"name", builder.getStringAttr("occupancy")}});
}

struct ObservationGraph {
  mlir::OwningOpRef<mlir::ModuleOp> container;
  ModuleOp module;
  RuleOp rule;
  SourceReadOp read;
  mlir::Operation *observe;
};

ObservationGraph buildGraph(mlir::MLIRContext &context, llvm::StringRef kind,
                            bool signedReport = false) {
  mlir::OpBuilder builder(&context);
  auto location = mlir::FileLineColLoc::get(&context, "observed.py", 9, 5);
  auto container = mlir::ModuleOp::create(location);
  container->setAttr("ac.stage", builder.getStringAttr("source"));
  container->setAttr("ac.unit_kind", builder.getStringAttr("implementation"));
  container->setAttr("ac.source_owner", sourceOwner(builder));
  builder.setInsertionPointToStart(container.getBody());

  auto logical = kind == "report" ? logicalU8(builder, signedReport)
                                  : logicalBool(builder);
  auto physical = kind == "report" ? mlir::Type(builder.getI8Type())
                                   : mlir::Type(builder.getI1Type());
  auto registration = occurrence(builder, 2);
  auto readOrigin = occurrence(builder, 3);
  auto observeSite = occurrence(builder, 4);
  auto identity = observationID(builder, registration, observeSite);
  auto valueIdentity = valueID(builder, readOrigin);
  auto observationSpec = spec(builder, kind);
  auto required =
      dictionary(builder, {{"id", identity},
                           {"kind", builder.getStringAttr(kind)},
                           {"spec", observationSpec},
                           {"values", builder.getArrayAttr({valueIdentity})}});

  auto port = dictionary(
      builder,
      {{"parameter", builder.getStringAttr("value")},
       {"ordinal", builder.getUnitAttr()},
       {"role", builder.getStringAttr("current")},
       {"type", logical},
       {"origin", occurrence(builder, 1)},
       {"location",
        dictionary(builder, {{"path", builder.getStringAttr("observed.py")},
                             {"line", u64(builder, 4)},
                             {"column", u64(builder, 14)},
                             {"end_line", u64(builder, 4)},
                             {"end_column", u64(builder, 19)}})}});
  mlir::OperationState moduleState(location, ModuleOp::getOperationName());
  moduleState.addAttribute("name", builder.getStringAttr("Observed"));
  moduleState.addAttribute("sym_name",
                           builder.getStringAttr("verify.Observed"));
  moduleState.addAttribute("ac.source_owner", sourceOwner(builder));
  moduleState.addAttribute("ac.origin", occurrence(builder, 0));
  moduleState.addAttribute("ac.ports", builder.getArrayAttr({port}));
  moduleState.addAttribute("ac.control_ports",
                           dictionary(builder,
                                      {{"clock", builder.getI32IntegerAttr(0)},
                                       {"reset", builder.getI32IntegerAttr(1)}}));
  moduleState.addRegion();
  auto module = mlir::cast<ModuleOp>(builder.create(moduleState));
  auto *moduleBody = new mlir::Block();
  module.getBody().push_back(moduleBody);
  moduleBody->addArgument(builder.getI1Type(), location);
  moduleBody->addArgument(builder.getI1Type(), location);
  moduleBody->addArgument(RegType::get(&context, physical), location);

  builder.setInsertionPointToEnd(moduleBody);
  mlir::OperationState ruleState(location, RuleOp::getOperationName());
  ruleState.addOperands(moduleBody->getArgument(2));
  ruleState.addAttribute("name", builder.getStringAttr("observe"));
  ruleState.addAttribute("registration", registration);
  ruleState.addAttribute("operandSegmentSizes",
                         builder.getDenseI32ArrayAttr({1, 0}));
  ruleState.addAttribute("ac.source_owner", sourceOwner(builder));
  ruleState.addAttribute("ac.origin", occurrence(builder, 5));
  ruleState.addAttribute("ac.input_bindings",
                         builder.getArrayAttr({formal(builder)}));
  ruleState.addAttribute("ac.output_bindings", builder.getArrayAttr({}));
  ruleState.addAttribute("ac.input_types", builder.getArrayAttr({logical}));
  ruleState.addAttribute("ac.output_types", builder.getArrayAttr({}));
  ruleState.addAttribute("ac.required_observations",
                         builder.getArrayAttr({required}));
  ruleState.addRegion();
  auto rule = mlir::cast<RuleOp>(builder.create(ruleState));
  auto *ruleBody = new mlir::Block();
  rule.getBody().push_back(ruleBody);
  ruleBody->addArgument(physical, location);
  builder.setInsertionPointToEnd(ruleBody);

  mlir::OperationState readState(location, SourceReadOp::getOperationName());
  readState.addOperands(ruleBody->getArgument(0));
  readState.addTypes(physical);
  readState.addAttribute("ac.origin", readOrigin);
  auto read = mlir::cast<SourceReadOp>(builder.create(readState));
  auto path = mlir::arith::ConstantOp::create(
      builder, location, builder.getI1Type(), builder.getBoolAttr(true));
  mlir::OperationState observeState(location, "ac.observe");
  observeState.addOperands({path.getResult(), read.getResult()});
  observeState.addAttribute("kind", builder.getStringAttr(kind));
  observeState.addAttribute("ac.observation_id", identity);
  observeState.addAttribute("ac.value_ids",
                            builder.getArrayAttr({valueIdentity}));
  observeState.addAttribute(
      "ac.value_constraints",
      builder.getArrayAttr({valueConstraint(builder, logical)}));
  observeState.addAttribute("spec", observationSpec);
  mlir::Operation *observe = builder.create(observeState);
  builder.create(mlir::OperationState(location, YieldOp::getOperationName()));
  builder.setInsertionPointToEnd(moduleBody);
  builder.create(mlir::OperationState(location, YieldOp::getOperationName()));
  return {std::move(container), module, rule, read, observe};
}

struct Verification {
  bool passed;
  std::string diagnostic;
};

Verification verify(mlir::MLIRContext &context, mlir::Operation *operation) {
  std::string diagnostic;
  mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &value) {
    llvm::raw_string_ostream(diagnostic) << value;
    return mlir::success();
  });
  return {mlir::succeeded(mlir::verify(operation)), diagnostic};
}

class ObservationContractsTest : public ::testing::Test {
protected:
  ObservationContractsTest() {
    context.loadDialect<ACIRDialect, mlir::arith::ArithDialect>();
  }

  mlir::OwningOpRef<mlir::ModuleOp> clone(mlir::ModuleOp module) {
    return mlir::OwningOpRef<mlir::ModuleOp>(
        mlir::cast<mlir::ModuleOp>(module->clone()));
  }

  ::testing::AssertionResult requireBaseline(const ObservationGraph &graph) {
    Verification result = verify(context, *graph.container);
    if (result.passed)
      return ::testing::AssertionSuccess();
    return ::testing::AssertionFailure()
           << "W09 RED: approved source ac.observe schema is unavailable\n"
           << result.diagnostic;
  }

  mlir::MLIRContext context;
};

class ObservationGraphContractsTest : public ::testing::Test {
protected:
  ObservationGraphContractsTest() : context(dialects) {
    dialects.insert<ACIRDialect, mlir::arith::ArithDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }
  void SetUp() override {
    sourceRoot = temporary.child("source");
    ASSERT_FALSE(llvm::sys::fs::create_directories(sourceRoot));
    std::string source = sourceRoot + "/observed.py";
    writeFile(source, R"py(from pycircuit import module, rule

@module
def Observed(source: bool, sink: bool):
    @rule
    def sample():
        nonlocal sink
        sink = source
        return
    sample()
)py");
    std::string capture = temporary.child("observed.transport.mlir");
    std::string bodyPath = temporary.child("observed.body.mlir");
    std::string headerPath = temporary.child("observed.interface.mlir");
    ASSERT_EQ(
        emitCapture(source, sourceRoot, capture, temporary.child("python.log")),
        0);
    ASSERT_EQ(compileCapture(capture, "observed.py", bodyPath, headerPath,
                             temporary.child("compile.log")),
              0);
    body = mlir::parseSourceFile<mlir::ModuleOp>(bodyPath, &context);
    header = mlir::parseSourceFile<mlir::ModuleOp>(headerPath, &context);
    ASSERT_TRUE(body && header);
    addObservation(*body);
  }

  void addObservation(mlir::ModuleOp sourceBody) {
    auto module = *sourceBody.getOps<ModuleOp>().begin();
    auto rule = *module.getBody().front().getOps<RuleOp>().begin();
    mlir::Builder builder(&context);
    auto logical = logicalBool(builder);
    auto registration = rule.getRegistrationAttr();
    auto site = occurrence(builder, 44);
    auto secondSite = occurrence(builder, 46);
    auto identity = observationID(builder, registration, site);
    auto secondIdentity = observationID(builder, registration, secondSite);
    auto origin = occurrence(builder, 45);
    auto valueID = valueIDFor(builder, origin);
    auto observationSpec = spec(builder, "log");
    auto required =
        dictionary(builder, {{"id", identity},
                             {"kind", builder.getStringAttr("log")},
                             {"spec", observationSpec},
                             {"values", builder.getArrayAttr({valueID})}});
    auto secondRequired =
        dictionary(builder, {{"id", secondIdentity},
                             {"kind", builder.getStringAttr("log")},
                             {"spec", observationSpec},
                             {"values", builder.getArrayAttr({valueID})}});
    rule->setAttr("ac.required_observations",
                  builder.getArrayAttr({required, secondRequired}));
    mlir::Block &body = rule.getBody().front();
    mlir::OpBuilder opBuilder(&body, body.getTerminator()->getIterator());
    mlir::OperationState readState(rule.getLoc(),
                                   SourceReadOp::getOperationName());
    readState.addOperands(body.getArgument(0));
    readState.addTypes(body.getArgument(0).getType());
    auto read = mlir::cast<SourceReadOp>(opBuilder.create(readState));
    read->setAttr("ac.origin", origin);
    auto path = mlir::arith::ConstantOp::create(opBuilder, rule.getLoc(),
                                                opBuilder.getI1Type(),
                                                opBuilder.getBoolAttr(true));
    mlir::OperationState state(rule.getLoc(), "ac.observe");
    state.addOperands({path.getResult(), read.getResult()});
    state.addAttribute("kind", opBuilder.getStringAttr("log"));
    state.addAttribute("ac.observation_id", identity);
    state.addAttribute("ac.value_ids", opBuilder.getArrayAttr({valueID}));
    state.addAttribute(
        "ac.value_constraints",
        opBuilder.getArrayAttr({valueConstraint(opBuilder, logical)}));
    state.addAttribute("spec", observationSpec);
    mlir::Operation *first = opBuilder.create(state);
    mlir::Operation *second = first->clone();
    second->setAttr("ac.observation_id", secondIdentity);
    body.getOperations().insert(body.getTerminator()->getIterator(), second);
  }

  mlir::DictionaryAttr valueIDFor(mlir::Builder &builder,
                                  mlir::DictionaryAttr origin) {
    return dictionary(builder, {{"origin", origin}, {"slot", u32(builder, 0)}});
  }

  std::pair<std::optional<acir::compiler::ModuleGraph>, std::string> build() {
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
    auto registry = SourceHeaderRegistry::create(headers, emit);
    if (mlir::failed(registry))
      return {std::nullopt, diagnostic};
    llvm::SmallVector<SourceLinkUnit> units{{*body, *header}};
    auto graph = buildSourceModuleGraph(units, *registry, emit);
    if (mlir::failed(graph))
      return {std::nullopt, diagnostic};
    return {std::move(*graph), diagnostic};
  }

  std::pair<bool, std::string> verify(acir::compiler::ObservationGraph &graph) {
    std::string diagnostic;
    mlir::ScopedDiagnosticHandler capture(
        &context, [&](mlir::Diagnostic &value) {
          llvm::raw_string_ostream(diagnostic) << value;
          return mlir::success();
        });
    auto emit = [&]() -> mlir::InFlightDiagnostic {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
    return {mlir::succeeded(verifySourceObservations(graph, emit)), diagnostic};
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context{dialects};
  TemporaryDirectory temporary;
  std::string sourceRoot;
  mlir::OwningOpRef<mlir::ModuleOp> body, header;
};

TEST_F(ObservationGraphContractsTest,
       BuiltObservationGraphVerifiesAndRejectsMutation) {
  auto built = build();
  ASSERT_TRUE(built.first.has_value()) << built.second;
  auto modules = std::move(*built.first);
  std::string diagnostic;
  auto emit = [&]() -> mlir::InFlightDiagnostic {
    return mlir::emitError(mlir::UnknownLoc::get(&context));
  };
  auto observations = buildSourceObservationGraph(modules, emit);
  ASSERT_TRUE(mlir::succeeded(observations)) << diagnostic;
  ASSERT_EQ(observations->bindings.size(), 2u);
  auto baseline = verify(*observations);
  ASSERT_TRUE(baseline.first) << baseline.second;

  auto expectBindingMutationRejected = [&](auto mutate, llvm::StringRef name) {
    auto mutated = *observations;
    mutate(mutated.bindings.front());
    auto rejected = verify(mutated);
    EXPECT_FALSE(rejected.first) << name.str() << " mutation was accepted";
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
        binding.ownerRef =
            dictionary(builder, {{"package", builder.getStringAttr("wrong")},
                                 {"path", builder.getStringAttr("wrong.py")}});
      },
      "ownerRef");
  expectBindingMutationRejected([](auto &binding) { binding.rule = {}; },
                                "rule");
  expectBindingMutationRejected(
      [&](auto &binding) { binding.registration = occurrence(builder, 999); },
      "registration");
  expectBindingMutationRejected(
      [&](auto &binding) {
        binding.observationID = observationID(builder, occurrence(builder, 2),
                                              occurrence(builder, 999));
      },
      "observationID");
  expectBindingMutationRejected(
      [&](auto &binding) { binding.kind = builder.getStringAttr("print"); },
      "kind");
  expectBindingMutationRejected(
      [&](auto &binding) { binding.spec = dictionary(builder, {}); }, "spec");
  expectBindingMutationRejected(
      [&](auto &binding) { binding.valueIDs = builder.getArrayAttr({}); },
      "valueIDs");
  expectBindingMutationRejected(
      [&](auto &binding) {
        binding.valueConstraints = builder.getArrayAttr({});
      },
      "valueConstraints");
  expectBindingMutationRejected(
      [](auto &binding) { binding.path = binding.values.front(); }, "path");
  expectBindingMutationRejected(
      [](auto &binding) { binding.values.front() = binding.path; }, "values");
  expectBindingMutationRejected([](auto &binding) { binding.observe = {}; },
                                "observe");

  auto rule = observations->bindings.front().rule;
  auto required =
      rule->getAttrOfType<mlir::ArrayAttr>("ac.required_observations");
  rule->setAttr("ac.required_observations",
                builder.getArrayAttr({required[1], required[0]}));
  EXPECT_FALSE(verify(*observations).first)
      << "reordered required list was accepted";
  rule->setAttr("ac.required_observations", required);

  auto observe = observations->bindings.front().observe.getOperation();
  mlir::Value originalValue = observe->getOperand(1);
  mlir::OpBuilder opBuilder(observe);
  auto redirected = mlir::arith::ConstantOp::create(
      opBuilder, observe->getLoc(), opBuilder.getI1Type(),
      opBuilder.getBoolAttr(false));
  observe->setOperand(1, redirected.getResult());
  EXPECT_FALSE(verify(*observations).first)
      << "same-type SSA redirect was accepted";
  observe->setOperand(1, originalValue);

  mlir::Operation *duplicate = observe->clone();
  rule.getBody().front().getOperations().insert(
      rule.getBody().front().getTerminator()->getIterator(), duplicate);
  EXPECT_FALSE(verify(*observations).first)
      << "duplicate actual op was accepted";
  duplicate->erase();

  observe->erase();
  EXPECT_FALSE(verify(*observations).first) << "deleted actual op was accepted";
}

TEST_F(ObservationContractsTest, CanonicalPrintLogAndReportParseAndVerify) {
  for (llvm::StringRef kind : {"print", "log", "report"}) {
    SCOPED_TRACE(kind.str());
    ObservationGraph graph = buildGraph(context, kind);
    ASSERT_TRUE(requireBaseline(graph));
    std::string text;
    llvm::raw_string_ostream(text) << *graph.container;
    auto reparsed = mlir::parseSourceString<mlir::ModuleOp>(text, &context);
    ASSERT_TRUE(reparsed);
    EXPECT_TRUE(verify(context, *reparsed).passed);
  }
}

TEST_F(ObservationContractsTest, RequiredObservationRejectsDeleteAndDuplicate) {
  ObservationGraph graph = buildGraph(context, "log");
  ASSERT_TRUE(requireBaseline(graph));

  auto deleted = clone(*graph.container);
  mlir::Operation *observe = nullptr;
  deleted->walk([&](mlir::Operation *operation) {
    if (operation->getName().getStringRef() == "ac.observe")
      observe = operation;
  });
  ASSERT_NE(observe, nullptr);
  observe->erase();
  EXPECT_FALSE(verify(context, *deleted).passed);

  auto duplicated = clone(*graph.container);
  observe = nullptr;
  duplicated->walk([&](mlir::Operation *operation) {
    if (operation->getName().getStringRef() == "ac.observe")
      observe = operation;
  });
  ASSERT_NE(observe, nullptr);
  observe->getBlock()->getOperations().insert(observe->getIterator(),
                                              observe->clone());
  EXPECT_FALSE(verify(context, *duplicated).passed);
}

TEST_F(ObservationContractsTest, RedirectedValueAndWrongPathAreRejected) {
  ObservationGraph graph = buildGraph(context, "log");
  ASSERT_TRUE(requireBaseline(graph));
  auto redirected = clone(*graph.container);
  mlir::Operation *observe = nullptr;
  redirected->walk([&](mlir::Operation *operation) {
    if (operation->getName().getStringRef() == "ac.observe")
      observe = operation;
  });
  ASSERT_NE(observe, nullptr);
  mlir::OpBuilder builder(observe);
  auto forged = mlir::arith::ConstantOp::create(builder, observe->getLoc(),
                                                builder.getI1Type(),
                                                builder.getBoolAttr(false));
  observe->setOperand(1, forged.getResult());
  EXPECT_FALSE(verify(context, *redirected).passed);

  auto wrongPath = clone(*graph.container);
  observe = nullptr;
  wrongPath->walk([&](mlir::Operation *operation) {
    if (operation->getName().getStringRef() == "ac.observe")
      observe = operation;
  });
  ASSERT_NE(observe, nullptr);
  observe->setOperand(0, observe->getOperand(1));
  EXPECT_FALSE(verify(context, *wrongPath).passed);
}

TEST_F(ObservationContractsTest, WrongScopeIdentitySpecAndRequiredListReject) {
  ObservationGraph graph = buildGraph(context, "print");
  ASSERT_TRUE(requireBaseline(graph));
  mlir::Builder builder(&context);

  auto wrongID = clone(*graph.container);
  mlir::Operation *observe = nullptr;
  wrongID->walk([&](mlir::Operation *operation) {
    if (operation->getName().getStringRef() == "ac.observe")
      observe = operation;
  });
  observe->setAttr(
      "ac.observation_id",
      observationID(builder, occurrence(builder, 99), occurrence(builder, 4)));
  EXPECT_FALSE(verify(context, *wrongID).passed);

  auto wrongSpec = clone(*graph.container);
  wrongSpec->walk([&](mlir::Operation *operation) {
    if (operation->getName().getStringRef() == "ac.observe")
      operation->setAttr("spec", dictionary(builder, {}));
  });
  EXPECT_FALSE(verify(context, *wrongSpec).passed);

  auto wrongValues = clone(*graph.container);
  wrongValues->walk([&](mlir::Operation *operation) {
    if (operation->getName().getStringRef() == "ac.observe")
      operation->setAttr("ac.value_ids", builder.getArrayAttr({}));
  });
  EXPECT_FALSE(verify(context, *wrongValues).passed);

  auto missingRequired = clone(*graph.container);
  auto module = *missingRequired->getOps<ModuleOp>().begin();
  auto rule = *module.getBody().front().getOps<RuleOp>().begin();
  rule->setAttr("ac.required_observations", builder.getArrayAttr({}));
  EXPECT_FALSE(verify(context, *missingRequired).passed);
}

TEST_F(ObservationContractsTest, ReportRequiresNonnegativeIntegerDomain) {
  ObservationGraph valid = buildGraph(context, "report");
  ASSERT_TRUE(requireBaseline(valid));
  ObservationGraph negative = buildGraph(context, "report", true);
  EXPECT_FALSE(verify(context, *negative.container).passed);
}

} // namespace
} // namespace acir::ac
