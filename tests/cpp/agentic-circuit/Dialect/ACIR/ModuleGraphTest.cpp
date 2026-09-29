#include "Compiler/ModuleGraph.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
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
        llvm::sys::fs::createUniqueDirectory("acir-module-graph", path));
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

mlir::OwningOpRef<mlir::ModuleOp> clone(mlir::ModuleOp module) {
  return mlir::OwningOpRef<mlir::ModuleOp>(
      mlir::cast<mlir::ModuleOp>(module->clone()));
}

struct GraphAttempt {
  std::optional<ModuleGraph> graph;
  std::string diagnostic;
};

class ModuleGraphTest : public ::testing::Test {
protected:
  ModuleGraphTest() : context(dialects) {
    dialects.insert<ac::ACIRDialect, mlir::arith::ArithDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    root = temporary.child("source");
    ASSERT_FALSE(llvm::sys::fs::create_directories(root));
    std::string leafSource = root + "/leaf.py";
    std::string parentSource = root + "/parent.py";
    writeFile(leafSource, R"py(from pycircuit import module, rule

@module
def Leaf(source: bool, sink: bool):
    hidden: bool = False

    @rule
    def forward():
        nonlocal hidden, sink
        hidden = source
        sink = hidden
        return

    forward()
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
    leafHeaderPath = temporary.child("leaf.interface.mlir");
    std::string parentBodyPath = temporary.child("parent.body.mlir");
    std::string parentHeaderPath = temporary.child("parent.interface.mlir");
    ASSERT_EQ(emitCapture(leafSource, root, leafCapture,
                          temporary.child("leaf-python.log")),
              0);
    ASSERT_EQ(emitCapture(parentSource, root, parentCapture,
                          temporary.child("parent-python.log")),
              0);
    ASSERT_EQ(compileCapture(leafCapture, "leaf.py", leafBodyPath,
                             leafHeaderPath, temporary.child("leaf.log")),
              0);
    ASSERT_EQ(compileCapture(parentCapture, "parent.py", parentBodyPath,
                             parentHeaderPath, temporary.child("parent.log"),
                             {leafHeaderPath}),
              0);
    leafBody = mlir::parseSourceFile<mlir::ModuleOp>(leafBodyPath, &context);
    leafHeader =
        mlir::parseSourceFile<mlir::ModuleOp>(leafHeaderPath, &context);
    parentBody =
        mlir::parseSourceFile<mlir::ModuleOp>(parentBodyPath, &context);
    parentHeader =
        mlir::parseSourceFile<mlir::ModuleOp>(parentHeaderPath, &context);
    ASSERT_TRUE(leafBody && leafHeader && parentBody && parentHeader);

    // Link consumes compiled units only. Parent compilation received only the
    // child header; neither step may recover semantics from child Python.
    ASSERT_FALSE(llvm::sys::fs::remove_directories(root));
  }

  bool admit(llvm::ArrayRef<SourceLinkUnit> units) {
    std::string diagnostic;
    mlir::ScopedDiagnosticHandler capture(
        &context, [&](mlir::Diagnostic &value) {
          llvm::raw_string_ostream(diagnostic) << value;
          return mlir::success();
        });
    auto emit = [&]() -> mlir::InFlightDiagnostic {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
    return mlir::succeeded(admitSourceLinkUnits(units, emit));
  }

  llvm::SmallVector<SourceLinkUnit> units(mlir::ModuleOp parent = {}) {
    return {{*leafBody, *leafHeader},
            {parent ? parent : *parentBody, *parentHeader}};
  }

  GraphAttempt build(llvm::ArrayRef<SourceLinkUnit> graphUnits) {
    GraphAttempt attempt;
    mlir::ScopedDiagnosticHandler capture(
        &context, [&](mlir::Diagnostic &value) {
          llvm::raw_string_ostream(attempt.diagnostic) << value;
          return mlir::success();
        });
    auto emit = [&]() -> mlir::InFlightDiagnostic {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
    llvm::SmallVector<mlir::ModuleOp> headers;
    for (SourceLinkUnit unit : graphUnits)
      headers.push_back(unit.header);
    auto registry = SourceHeaderRegistry::create(headers, emit);
    if (mlir::failed(registry))
      return attempt;
    auto graph = buildSourceModuleGraph(graphUnits, *registry, emit);
    if (mlir::succeeded(graph))
      attempt.graph.emplace(std::move(*graph));
    return attempt;
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context;
  TemporaryDirectory temporary;
  std::string root, leafHeaderPath;
  mlir::OwningOpRef<mlir::ModuleOp> leafBody, leafHeader, parentBody,
      parentHeader;
};

TEST_F(ModuleGraphTest, HeaderOnlyParentAndCompleteBodiesAreAdmitted) {
  EXPECT_TRUE(admit(units()));
}

mlir::DictionaryAttr aliasFor(const InstanceView &view,
                              llvm::StringRef parameter) {
  for (const auto &[state, id] : view.formalAliases) {
    auto reference = mlir::cast<mlir::DictionaryAttr>(state);
    auto name = reference.getAs<mlir::StringAttr>("parameter");
    if (name && name.getValue() == parameter)
      return id;
  }
  return {};
}

TEST_F(ModuleGraphTest, GraphHasRootOrderedChildrenOwnersAndPostOrder) {
  GraphAttempt result = build(units());
  ASSERT_TRUE(result.graph.has_value()) << result.diagnostic;
  ModuleGraph &graph = *result.graph;
  ASSERT_NE(graph.root, nullptr);
  ASSERT_EQ(graph.views.size(), 3u);
  EXPECT_EQ(graph.root->definition.getValue(), "verify.parent.Parent");
  EXPECT_EQ(graph.root->parent, nullptr);
  ASSERT_EQ(graph.root->children.size(), 2u);
  EXPECT_EQ(graph.root->children[0]->placement.getName(), "left");
  EXPECT_EQ(graph.root->children[1]->placement.getName(), "right");
  EXPECT_EQ(graph.root->children[0]->parent, graph.root);
  EXPECT_EQ(graph.root->children[1]->parent, graph.root);
  EXPECT_NE(graph.root->children[0]->owner, graph.root->children[1]->owner);
  ASSERT_EQ(graph.postOrder().size(), 3u);
  EXPECT_EQ(graph.postOrder()[0], graph.root->children[0]);
  EXPECT_EQ(graph.postOrder()[1], graph.root->children[1]);
  EXPECT_EQ(graph.postOrder()[2], graph.root);
}

TEST_F(ModuleGraphTest, GraphOwnsThreeStatesAndRelayIsOneStateID) {
  GraphAttempt result = build(units());
  ASSERT_TRUE(result.graph.has_value()) << result.diagnostic;
  ModuleGraph &graph = *result.graph;
  ASSERT_EQ(graph.root->children.size(), 2u);
  InstanceView *left = graph.root->children[0];
  InstanceView *right = graph.root->children[1];
  ASSERT_EQ(graph.root->ownedStateIDs.size(), 1u);
  ASSERT_EQ(left->ownedStateIDs.size(), 1u);
  ASSERT_EQ(right->ownedStateIDs.size(), 1u);
  llvm::DenseSet<mlir::Attribute> states;
  states.insert(graph.root->ownedStateIDs.front());
  states.insert(left->ownedStateIDs.front());
  states.insert(right->ownedStateIDs.front());
  EXPECT_EQ(states.size(), 3u);
  auto leftSink = aliasFor(*left, "sink");
  auto rightSource = aliasFor(*right, "source");
  ASSERT_TRUE(leftSink && rightSource);
  EXPECT_EQ(leftSink, graph.root->ownedStateIDs.front());
  EXPECT_EQ(rightSource, graph.root->ownedStateIDs.front());
}

TEST_F(ModuleGraphTest, BadControlAndRecursiveCalleeAreRejected) {
  auto badControl = clone(*parentBody);
  auto module = *badControl->getOps<ac::ModuleOp>().begin();
  auto instance = *module.getBody().front().getOps<ac::InstanceOp>().begin();
  instance.getClockMutable().set(module.getBody().front().getArgument(1));
  GraphAttempt result = build(units(*badControl));
  EXPECT_FALSE(result.graph.has_value());
  EXPECT_NE(result.diagnostic.find("controls do not identity-forward"),
            std::string::npos)
      << result.diagnostic;

  auto recursive = clone(*parentBody);
  module = *recursive->getOps<ac::ModuleOp>().begin();
  instance = *module.getBody().front().getOps<ac::InstanceOp>().begin();
  instance.setCalleeAttr(
      mlir::FlatSymbolRefAttr::get(&context, "verify.parent.Parent"));
  result = build(units(*recursive));
  EXPECT_FALSE(result.graph.has_value());
  EXPECT_NE(result.diagnostic.find("requires one zero-parent root"),
            std::string::npos)
      << result.diagnostic;
}

TEST_F(ModuleGraphTest, ForeignAndMissingOrRepeatedActualsAreRejected) {
  auto foreign = clone(*parentBody);
  auto module = *foreign->getOps<ac::ModuleOp>().begin();
  auto instance = *module.getBody().front().getOps<ac::InstanceOp>().begin();
  auto leaf = *leafBody->getOps<ac::ModuleOp>().begin();
  auto hidden = *leaf.getBody().front().getOps<ac::RegOp>().begin();
  instance.getInputsMutable().assign(hidden.getState());
  GraphAttempt result = build(units(*foreign));
  EXPECT_FALSE(result.graph.has_value());
  EXPECT_NE(result.diagnostic.find("leaks private or foreign state"),
            std::string::npos)
      << result.diagnostic;

  auto missing = clone(*parentBody);
  module = *missing->getOps<ac::ModuleOp>().begin();
  instance = *module.getBody().front().getOps<ac::InstanceOp>().begin();
  instance.getInputsMutable().assign(mlir::ValueRange{});
  result = build(units(*missing));
  EXPECT_FALSE(result.graph.has_value());
  EXPECT_NE(result.diagnostic.find("current port has no actual"),
            std::string::npos)
      << result.diagnostic;

  auto repeated = clone(*parentBody);
  module = *repeated->getOps<ac::ModuleOp>().begin();
  instance = *module.getBody().front().getOps<ac::InstanceOp>().begin();
  mlir::Value input = instance.getInputs().front();
  instance.getInputsMutable().assign(mlir::ValueRange{input, input});
  result = build(units(*repeated));
  EXPECT_FALSE(result.graph.has_value());
  EXPECT_NE(result.diagnostic.find("actual arity exceeds"), std::string::npos)
      << result.diagnostic;
}

} // namespace
} // namespace acir::compiler
