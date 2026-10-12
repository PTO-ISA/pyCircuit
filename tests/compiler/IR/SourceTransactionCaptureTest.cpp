#include "Compiler/PythonImportRecords.h"
#include "Compiler/SourceMemoryBindings.h"
#include "Compiler/SourceRuleWrites.h"
#include "Compiler/SourceUnit.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/PassManager.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"
#include "pycircuit/Transforms/Passes.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace {
using namespace mlir;
namespace compiler = acir::compiler;
namespace source = acir::compiler::detail;
namespace ac = acir::ac;

// These tests stop at verified typed effects. They do not execute a design or
// establish transaction lowering, publication, C++, or Verilog support.
class SourceTransactionCaptureTest : public ::testing::Test {
protected:
  MLIRContext context;
  llvm::SmallString<256> directory;
  std::string diagnostics;
  std::unique_ptr<ScopedDiagnosticHandler> handler;

  void SetUp() override {
    context.loadDialect<ac::ACIRDialect>();
    ASSERT_FALSE(llvm::sys::fs::createUniqueDirectory(
        "source-transaction-capture", directory));
    handler = std::make_unique<ScopedDiagnosticHandler>(
        &context, [&](Diagnostic &diagnostic) {
          llvm::raw_string_ostream stream(diagnostics);
          diagnostic.print(stream);
          return success();
        });
  }
  void TearDown() override {
    if (!directory.empty())
      llvm::sys::fs::remove_directories(directory);
  }
  std::string child(StringRef name) const {
    llvm::SmallString<256> path(directory);
    llvm::sys::path::append(path, name);
    return path.str().str();
  }
  DictionaryAttr owner(StringRef filename) {
    Builder builder(&context);
    return builder.getDictionaryAttr(
        {builder.getNamedAttr("package", builder.getStringAttr("")),
         builder.getNamedAttr("path", builder.getStringAttr(filename))});
  }
  auto error() {
    return [&] { return emitError(UnknownLoc::get(&context)); };
  }

  // The production capture transport is obtained from real Python text, using
  // the same ExecuteAndWait route as SourceMemoryBindingsTest.
  OwningOpRef<ModuleOp> capture(StringRef filename, StringRef text) {
    auto path = child(filename), transport = path + ".capture.mlir";
    std::error_code ec;
    {
      llvm::raw_fd_ostream output(path, ec);
      EXPECT_FALSE(ec);
      if (ec)
        return {};
      output << text;
    }
    static constexpr llvm::StringLiteral script = R"py(
import sys
from pathlib import Path
repo, source, root, output = map(Path, sys.argv[1:])
sys.path.insert(0, str(repo / "python"))
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport
output.write_text(_emit_source_transport(_capture_source_file(source, source_root=root)), encoding="utf-8")
)py";
    std::vector<std::string> owned{PYCIRCUIT_TEST_PYTHON,
                                   "-c",
                                   script.str(),
                                   PYCIRCUIT_TEST_REPO_ROOT,
                                   path,
                                   directory.str().str(),
                                   transport};
    SmallVector<StringRef> arguments;
    for (const auto &value : owned)
      arguments.push_back(value);
    auto log = path + ".capture.log";
    std::array<std::optional<StringRef>, 3> redirects{std::nullopt, log, log};
    int status = llvm::sys::ExecuteAndWait(PYCIRCUIT_TEST_PYTHON, arguments,
                                           std::nullopt, redirects);
    auto output = llvm::MemoryBuffer::getFile(log);
    EXPECT_EQ(status, 0) << (output ? output.get()->getBuffer().str() : log);
    if (status)
      return {};
    return parseSourceFile<ModuleOp>(transport, &context);
  }

  LogicalResult lower(ModuleOp captured, StringRef filename,
                      bool runSummaries = true) {
    auto headers =
        compiler::SourceHeaderRegistry::create(&context, {}, error());
    if (failed(headers))
      return failure();
    PassManager pipeline(&context);
    pipeline.addPass(source::createAnalyzeRuleWritesPass());
    pipeline.addPass(
        source::createInferSourceBindingsPass(owner(filename), *headers));
    pipeline.addPass(
        source::createLowerPythonSourcePass(owner(filename), *headers));
    if (runSummaries)
      pipeline.addPass(acir::createAnalyzeSourceMethodSummariesPass());
    if (failed(pipeline.run(captured)))
      return failure();
    return mlir::verify(captured);
  }
  OwningOpRef<ModuleOp> typed(StringRef text,
                              StringRef filename = "design.py") {
    auto result = capture(filename, text);
    if (!result)
      return {};
    diagnostics.clear();
    auto status = lower(*result, filename);
    EXPECT_TRUE(succeeded(status)) << diagnostics;
    if (failed(status))
      return {};
    return result;
  }
  template <typename Op> SmallVector<Op> operations(ModuleOp file) {
    SmallVector<Op> result;
    file.walk([&](Op operation) { result.push_back(operation); });
    return result;
  }
  template <typename Op> SmallVector<Op> direct(Region &region) {
    SmallVector<Op> result;
    for (auto operation : region.front().getOps<Op>())
      result.push_back(operation);
    return result;
  }
  bool identityTransport(Value result, Value original, ModuleOp package) {
    ac::HardwareAnalysis analysis(package);
    llvm::DenseSet<Value> seen;
    while (result != original) {
      if (!seen.insert(result).second)
        return false;
      auto extract = result.getDefiningOp<ac::BitsExtractOp>();
      if (!extract || failed(mlir::verify(extract)))
        return false;
      ac::HardwareBindings bindings;
      bindings.owner = extract->getParentOfType<ac::ModuleOp>();
      if (failed(analysis.verifyResolvedOperation(extract, bindings)))
        return false;
      auto low = analysis.evaluateStatic(extract.getLow(), bindings, extract);
      auto inputWidth = analysis.getPackedWidth(extract.getInput().getType(),
                                                bindings, extract);
      auto resultWidth =
          analysis.getPackedWidth(result.getType(), bindings, extract);
      if (failed(low) || failed(inputWidth) || failed(resultWidth) ||
          *inputWidth != *resultWidth ||
          cast<ac::MathIntAttr>(*low).getCanonicalValue() != "0")
        return false;
      // Only an entire zero-offset slice is accepted. The existing BitsExtract
      // contract copies value, known and Z planes; arithmetic, resizing,
      // selects and partial slices cannot stand in for the explicit second
      // state read.
      result = extract.getInput();
    }
    return true;
  }
  void reject(StringRef text, StringRef category) {
    auto captured = capture("invalid.py", text);
    ASSERT_TRUE(captured);
    diagnostics.clear();
    EXPECT_TRUE(failed(lower(*captured, "invalid.py"))) << text.str();
    EXPECT_NE(StringRef(diagnostics).lower().find(category.lower()),
              std::string::npos)
        << diagnostics;
  }
};

std::string counterSource(unsigned width = 8, StringRef ownerName = "Counter",
                          StringRef parentName = "Parent", bool two = false) {
  const std::string bits = "ac.bits[" + std::to_string(width) + "]";
  return "import pycircuit as ac\n@ac.struct\nclass View:\n    before: " +
         bits + "\n    after: " + bits + "\n@ac.module\ndef " +
         ownerName.str() + "():\n    count: " + bits +
         " = 0\n"
         "    @ac.method\n    def read() -> " +
         bits +
         " :\n        return count\n"
         "    @ac.method\n    def add(delta: " +
         bits + ") -> " + bits +
         ":\n        next_value = count + delta\n        count = next_value\n  "
         "      return count\n"
         "    @ac.method\n    def relay(delta: " +
         bits + ") -> " + bits +
         ":\n        saved = add(delta)\n        return saved\n"
         "@ac.module\ndef " +
         parentName.str() + "(delta: " + bits +
         ") -> View:\n"
         "    left = " +
         ownerName.str() + "()\n" +
         (two ? "    right = " + ownerName.str() + "()\n" : "") +
         "    @ac.rule\n    def advance() -> View:\n        before = "
         "left.read()\n"
         "        after = left.relay(delta)\n" +
         (two ? "        other = right.add(delta)\n" : "") +
         "        return View(before=before, after=after)\n    return "
         "advance()\n";
}

TEST_F(SourceTransactionCaptureTest, SourceWritesRemainOrderedTypedEffects) {
  auto file = typed(counterSource());
  ASSERT_TRUE(file);
  auto states = operations<ac::SourceStateOp>(*file);
  auto methods = operations<ac::SourceMethodOp>(*file);
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(states.size(), 1u);
  ASSERT_EQ(methods.size(), 3u);
  ASSERT_EQ(transactions.size(), 1u);
  ac::SourceMethodOp writing;
  for (auto method : methods)
    if (!direct<ac::SourceStateWriteOp>(method.getBody()).empty())
      writing = method;
  ASSERT_TRUE(writing);
  auto reads = direct<ac::SourceStateReadOp>(writing.getBody());
  auto writes = direct<ac::SourceStateWriteOp>(writing.getBody());
  ASSERT_EQ(reads.size(), 2u);
  ASSERT_EQ(writes.size(), 1u);
  EXPECT_TRUE(reads[0]->isBeforeInBlock(writes[0]));
  EXPECT_TRUE(writes[0]->isBeforeInBlock(reads[1]));
  EXPECT_EQ(reads[0].getState(), reads[1].getState());
  EXPECT_EQ(reads[1].getState(), states[0].getInstanceName());
  EXPECT_NE(reads[1].getValue(), writes[0].getValue());
  // The producer preserves the second read. State SSA is a later pass.
  auto yield = cast<ac::YieldOp>(writing.getBody().front().getTerminator());
  ASSERT_EQ(yield.getNumOperands(), 1u);
  EXPECT_TRUE(
      identityTransport(yield.getOperand(0), reads[1].getValue(), *file));
}

TEST_F(SourceTransactionCaptureTest, MethodChainsShareTheOuterTransaction) {
  auto file = typed(counterSource());
  ASSERT_TRUE(file);
  auto methods = operations<ac::SourceMethodOp>(*file);
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(transactions.size(), 1u);
  auto outer = direct<ac::SourceMethodCallOp>(transactions[0].getBody());
  ASSERT_EQ(outer.size(), 2u);
  EXPECT_TRUE(outer[0]->isBeforeInBlock(outer[1]));
  EXPECT_EQ(outer[0].getInstancePath(), outer[1].getInstancePath());
  ASSERT_EQ(outer[0].getInstancePath().size(), 1u);
  unsigned localCalls = 0;
  for (auto method : methods)
    for (auto call : direct<ac::SourceMethodCallOp>(method.getBody())) {
      ++localCalls;
      EXPECT_TRUE(call.getInstancePath().empty());
      EXPECT_EQ(call->getParentOfType<ac::ModuleOp>().getSymNameAttr(),
                call.getCalleeAttr().getRootReference());
    }
  EXPECT_EQ(localCalls, 1u);
  EXPECT_EQ(operations<ac::SourceStateOp>(*file).size(), 1u);
  EXPECT_EQ(operations<ac::InstanceOp>(*file).size(), 1u);
}

TEST_F(SourceTransactionCaptureTest,
       OrdinaryStructResultsRetainTheirInterface) {
  auto file = typed(counterSource());
  ASSERT_TRUE(file);
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(transactions.size(), 1u);
  auto transaction = transactions.front();
  auto parent = transaction->getParentOfType<ac::ModuleOp>();
  ASSERT_EQ(parent.getFunctionType().getNumResults(), 1u);
  ASSERT_EQ(transaction.getNumResults(), 1u);
  EXPECT_TRUE(isa<ac::StructType>(transaction.getResult(0).getType()));
  EXPECT_EQ(parent.getFunctionType().getResult(0),
            transaction.getResult(0).getType());
  auto yield = cast<ac::YieldOp>(parent.getBody().front().getTerminator());
  EXPECT_EQ(yield.getOperand(0), transaction.getResult(0));
  EXPECT_EQ(parent.getInputNames().size(), 3u); // delta + compiler clock/reset
  auto state = operations<ac::SourceStateOp>(*file).front();
  EXPECT_TRUE(isa<BlockArgument>(state.getClk()));
  EXPECT_TRUE(isa<BlockArgument>(state.getRst()));
  EXPECT_NE(state.getClk(), state.getRst());
  EXPECT_TRUE(isa<BlockArgument>(transaction.getClk()));
  EXPECT_TRUE(isa<BlockArgument>(transaction.getRst()));
}

TEST_F(SourceTransactionCaptureTest, SeparateInstancesReuseOneOwnerDefinition) {
  auto file = typed(counterSource(13, "Accumulator", "Pair", true), "pair.py");
  ASSERT_TRUE(file);
  auto states = operations<ac::SourceStateOp>(*file);
  auto children = operations<ac::InstanceOp>(*file);
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(states.size(), 1u);
  ASSERT_EQ(children.size(), 2u);
  ASSERT_EQ(transactions.size(), 1u);
  EXPECT_EQ(children[0].getCallee(), children[1].getCallee());
  EXPECT_NE(children[0].getOccurrence(), children[1].getOccurrence());
  auto calls = direct<ac::SourceMethodCallOp>(transactions[0].getBody());
  ASSERT_EQ(calls.size(), 3u);
  EXPECT_EQ(calls[0].getInstancePath(), calls[1].getInstancePath());
  EXPECT_NE(calls[1].getInstancePath(), calls[2].getInstancePath());
  auto stateType = states[0].getQ().getType();
  auto resultType = calls[2].getResult(0).getType();
  ASSERT_TRUE(isa<ac::BitsType>(stateType));
  ASSERT_TRUE(isa<ac::BitsType>(resultType));
  EXPECT_TRUE(ac::areEquivalentHardwareTypes(stateType, resultType));
  ac::HardwareAnalysis analysis(*file);
  auto stateWidth = analysis.getPackedWidth(stateType, {}, states[0]);
  auto resultWidth = analysis.getPackedWidth(resultType, {}, calls[2]);
  ASSERT_TRUE(succeeded(stateWidth));
  ASSERT_TRUE(succeeded(resultWidth));
  EXPECT_EQ(*stateWidth, 13u);
  EXPECT_EQ(*resultWidth, 13u);
  auto occurrence = states[0].getOccurrence();
  EXPECT_EQ(
      occurrence.getAs<DictionaryAttr>("site").getAs<FlatSymbolRefAttr>(
          "definition"),
      FlatSymbolRefAttr::get(
          &context, states[0]->getParentOfType<ac::ModuleOp>().getSymName()));
}

TEST_F(SourceTransactionCaptureTest,
       NamesWidthsAndMarkerAliasesDoNotSelectSemantics) {
  for (auto width : {3u, 11u}) {
    auto file = typed(counterSource(width, "RegisterBank", "Transform"));
    ASSERT_TRUE(file);
    EXPECT_EQ(operations<ac::SourceStateOp>(*file).size(), 1u);
    EXPECT_EQ(operations<ac::SourceMethodOp>(*file).size(), 3u);
    EXPECT_EQ(operations<ac::SourceTransactionOp>(*file).size(), 1u);
    EXPECT_EQ(operations<ac::SourceMethodCallOp>(*file).size(), 3u);
  }
  std::string aliases = counterSource();
  auto replace = [&](StringRef before, StringRef after) {
    std::size_t offset = 0;
    while ((offset = aliases.find(before.str(), offset)) != std::string::npos) {
      aliases.replace(offset, before.size(), after.str());
      offset += after.size();
    }
  };
  replace("import pycircuit as ac",
          "import pycircuit as ac\nfrom pycircuit import module as component, "
          "rule as transaction, method as operation");
  replace("@ac.module", "@component");
  replace("@ac.rule", "@transaction");
  replace("@ac.method", "@operation");
  replace("@operation\n    def add",
          "@operation(guard=lambda delta: True)\n    def add");
  replace("@transaction\n    def advance",
          "@transaction(guard=lambda: True)\n    def advance");
  auto file = typed(aliases);
  ASSERT_TRUE(file);
  EXPECT_EQ(operations<ac::SourceMethodOp>(*file).size(), 3u);
  EXPECT_EQ(operations<ac::SourceTransactionOp>(*file).size(), 1u);
}

TEST_F(SourceTransactionCaptureTest, GuardsHaveOwnerReadsAndMatchingFormals) {
  auto file = typed(R"py(import pycircuit as ac
@ac.module
def Reservoir():
    count: ac.u8 = 0
    @ac.method(guard=lambda amount: count >= amount)
    def take(amount: ac.u8) -> ac.u8:
        count = count - amount
        return count
@ac.module
def Driver(amount: ac.u8):
    reservoir = Reservoir()
    @ac.rule(guard=lambda: amount != 0)
    def step() -> None:
        discarded = reservoir.take(amount)
    step()
)py");
  ASSERT_TRUE(file);
  auto methods = operations<ac::SourceMethodOp>(*file);
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(methods.size(), 1u);
  ASSERT_EQ(transactions.size(), 1u);
  EXPECT_EQ(direct<ac::SourceStateReadOp>(methods[0].getGuard()).size(), 1u);
  EXPECT_EQ(direct<ac::SourceStateReadOp>(methods[0].getBody()).size(), 2u);
  EXPECT_TRUE(direct<ac::SourceMethodCallOp>(methods[0].getGuard()).empty());
  EXPECT_TRUE(direct<ac::SourceStateWriteOp>(methods[0].getGuard()).empty());
  EXPECT_EQ(transactions[0].getNumResults(), 0u);
  EXPECT_TRUE(llvm::equal(methods[0].getGuard().front().getArgumentTypes(),
                          methods[0].getBody().front().getArgumentTypes()));
  auto guard = cast<ac::YieldOp>(methods[0].getGuard().front().getTerminator());
  ASSERT_EQ(guard.getNumOperands(), 1u);
  EXPECT_TRUE(isa<ac::BitsType>(guard.getOperand(0).getType()));
}

TEST_F(SourceTransactionCaptureTest,
       UnitCompilationRejectsWithoutMutatingCapture) {
  auto captured = capture("design.py", counterSource());
  ASSERT_TRUE(captured);
  auto print = [&] {
    std::string text;
    llvm::raw_string_ostream stream(text);
    captured->print(stream);
    return text;
  };
  auto before = print();
  auto headers = compiler::SourceHeaderRegistry::create(&context, {}, error());
  ASSERT_TRUE(succeeded(headers));
  diagnostics.clear();
  auto result = compiler::compilePythonSourceUnit(*captured, owner("design.py"),
                                                  *headers, error());
  EXPECT_TRUE(failed(result));
  EXPECT_NE(diagnostics.find("unlowered"), std::string::npos) << diagnostics;
  EXPECT_EQ(print(), before);
  EXPECT_TRUE(succeeded(mlir::verify(*captured)));
}

std::string replaceOnce(std::string text, StringRef before, StringRef after) {
  auto offset = text.find(before.str());
  EXPECT_NE(offset, std::string::npos);
  if (offset != std::string::npos)
    text.replace(offset, before.size(), after.str());
  return text;
}

TEST_F(SourceTransactionCaptureTest, ImmutableHandlesDoNotCloneState) {
  auto sourceText = replaceOnce(counterSource(), "    @ac.rule\n",
                                "    alias = left\n    @ac.rule\n");
  sourceText =
      replaceOnce(sourceText, "before = left.read()", "before = alias.read()");
  auto file = typed(sourceText);
  ASSERT_TRUE(file);
  EXPECT_EQ(operations<ac::SourceStateOp>(*file).size(), 1u);
  EXPECT_EQ(operations<ac::InstanceOp>(*file).size(), 1u);
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(transactions.size(), 1u);
  auto calls = direct<ac::SourceMethodCallOp>(transactions[0].getBody());
  ASSERT_EQ(calls.size(), 2u);
  EXPECT_EQ(calls[0].getInstancePath(), calls[1].getInstancePath());
}

TEST_F(SourceTransactionCaptureTest, GuardSyntaxAndFormalBindingFailClosed) {
  for (StringRef decorator :
       {"@ac.method(guard=True)", "@ac.method(guard=lambda other: other != 0)",
        "@ac.method(guard=lambda delta, extra: delta != extra)",
        "@ac.method(guard=lambda delta: delta)",
        "@ac.method(guard=lambda delta: True, unknown=True)"}) {
    reject(replaceOnce(counterSource(), "@ac.method\n    def add",
                       decorator.str() + "\n    def add"),
           "guard");
  }
  reject(
      replaceOnce(counterSource(), "@ac.rule\n    def advance",
                  "@ac.rule(guard=lambda delta: delta != 0)\n    def advance"),
      "guard");
}

TEST_F(SourceTransactionCaptureTest,
       GuardedPayloadCannotEscapeAsOrdinaryOutput) {
  auto sourceText = replaceOnce(
      counterSource(), "@ac.method\n    def add",
      "@ac.method(guard=lambda delta: count >= delta)\n    def add");
  reject(sourceText, "none");
  reject(replaceOnce(sourceText, "after = left.relay(delta)",
                     "after = left.add(delta)"),
         "none");
  auto childChain =
      replaceOnce(sourceText, "@ac.module\ndef Parent", R"py(@ac.module
def Wrapper():
    child = Counter()
    @ac.method
    def read() -> ac.bits[8]:
        return child.read()
    @ac.method
    def relay(delta: ac.bits[8]) -> ac.bits[8]:
        return child.add(delta)
@ac.module
def Parent)py");
  childChain =
      replaceOnce(childChain, "    left = Counter()", "    left = Wrapper()");
  reject(childChain, "none");
  reject(replaceOnce(counterSource(), "@ac.rule\n    def advance",
                     "@ac.rule(guard=lambda: delta != 0)\n    def advance"),
         "none");
}

TEST_F(SourceTransactionCaptureTest, GuardsCannotCallMethodsOrCreateEffects) {
  reject(replaceOnce(
             counterSource(), "@ac.method\n    def add",
             "@ac.method(guard=lambda delta: read() != delta)\n    def add"),
         "guard");
  reject(replaceOnce(
             counterSource(), "@ac.method\n    def add",
             "@ac.method(guard=lambda delta: ac.report(count))\n    def add"),
         "guard");
  reject(replaceOnce(
             counterSource(), "@ac.rule\n    def advance",
             "@ac.rule(guard=lambda: left.read() != delta)\n    def advance"),
         "guard");
}

TEST_F(SourceTransactionCaptureTest, OwnerNamesCannotBeShadowed) {
  reject(replaceOnce(counterSource(), "def add(delta: ac.bits[8])",
                     "def add(count: ac.bits[8])"),
         "shadow");
  reject(replaceOnce(
             counterSource(), "next_value = count + delta",
             "count: ac.bits[8] = delta\n        next_value = count + delta"),
         "shadow");
}

TEST_F(SourceTransactionCaptureTest, MarkerAliasesCannotBeRebound) {
  auto alias = replaceOnce(counterSource(), "import pycircuit as ac",
                           "import pycircuit as ac\nfrom pycircuit import "
                           "method as operation\noperation = 1");
  alias = replaceOnce(alias, "@ac.method\n    def read",
                      "@operation\n    def read");
  reject(alias, "shadow");
  auto shadow = replaceOnce(counterSource(), "    count: ac.bits[8] = 0",
                            "    ac = 1\n    count: ac.bits[8] = 0");
  reject(shadow, "shadow");
}

TEST_F(SourceTransactionCaptureTest, RulesRegisterOnceAndCannotNestCalls) {
  reject(replaceOnce(counterSource(), "    return advance()",
                     "    first = advance()\n    return advance()"),
         "regist");
  reject(replaceOnce(counterSource(), "    return advance()", "    pass"),
         "regist");
  auto nested =
      replaceOnce(counterSource(), "        before = left.read()",
                  "        ignored = inner()\n        before = left.read()");
  nested = replaceOnce(nested, "    @ac.rule\n    def advance",
                       "    @ac.rule\n    def inner() -> None:\n        pass\n "
                       "   @ac.rule\n    def advance");
  reject(nested, "rule");
  reject(replaceOnce(counterSource(), "        before = left.read()",
                     "        @ac.rule\n        def nested() -> None:\n"
                     "            pass\n        nested()\n"
                     "        before = left.read()"),
         "rule");
}

TEST_F(SourceTransactionCaptureTest, MethodSignaturesRequireExplicitTypes) {
  reject(replaceOnce(counterSource(), "def add(delta: ac.bits[8])",
                     "def add(delta)"),
         "annotation");
  reject(replaceOnce(counterSource(),
                     "def add(delta: ac.bits[8]) -> ac.bits[8]:",
                     "def add(delta: ac.bits[8]):"),
         "annotation");
}

TEST_F(SourceTransactionCaptureTest, EffectfulControlFlowFailsClosed) {
  reject(replaceOnce(
             counterSource(), "        after = left.relay(delta)",
             "        if delta != 0:\n            after = left.relay(delta)\n  "
             "      else:\n            after = before"),
         "branch");
  reject(replaceOnce(counterSource(), "        after = left.relay(delta)",
                     "        for index in range(2):\n            after = "
                     "left.relay(delta)"),
         "loop");
}

TEST_F(SourceTransactionCaptureTest, MethodCallsRequireDirectTransactionScope) {
  reject(
      replaceOnce(counterSource(), "    @ac.rule\n    def advance",
                  "    outside = left.read()\n    @ac.rule\n    def advance"),
      "rule");
  reject(replaceOnce(counterSource(), "    @ac.rule\n    def advance",
                     "    left.add(delta)\n    @ac.rule\n    def advance"),
         "rule");
  reject(replaceOnce(
             counterSource(), "        before = left.read()",
             "        operation = left.read\n        before = operation()"),
         "method");
  auto imported = replaceOnce(
      counterSource(), "import pycircuit as ac",
      "import pycircuit as ac\nfrom external_owner import ImportedOwner");
  imported = replaceOnce(imported, "    left = Counter()",
                         "    left = ImportedOwner()");
  reject(imported, "import");
}

TEST_F(SourceTransactionCaptureTest,
       MethodOwnersRejectAuthoredPortsAndResults) {
  reject(replaceOnce(counterSource(),
                     "def Counter():", "def Counter(unused: ac.bits[8]):"),
         "parameter");
  auto mixed = replaceOnce(counterSource(),
                           "def Counter():", "def Counter() -> ac.bits[8]:");
  mixed = replaceOnce(mixed, "@ac.module\ndef Parent",
                      "    return count\n@ac.module\ndef Parent");
  reject(mixed, "return");
  reject(replaceOnce(counterSource(), "@ac.module\ndef Parent",
                     "    return count\n@ac.module\ndef Parent"),
         "return");
}

TEST_F(SourceTransactionCaptureTest,
       HandlesAndOwnerAliasesCannotRebindIdentity) {
  auto rebound = replaceOnce(counterSource(8, "Counter", "Parent", true),
                             "    @ac.rule\n    def advance",
                             "    left = right\n    @ac.rule\n    def advance");
  reject(rebound, "rebound");
  auto ownerAlias =
      replaceOnce(counterSource(), "    @ac.rule\n    def advance",
                  "    count = left\n    @ac.rule\n    def advance");
  // Shadow a state declared in this parent, rather than another module's state.
  ownerAlias = replaceOnce(ownerAlias, "    left = Counter()",
                           "    count: ac.bits[8] = 0\n    left = Counter()");
  reject(ownerAlias, "shadow");
}

TEST_F(SourceTransactionCaptureTest,
       UnusedGuardedMethodDoesNotGuardATotalRule) {
  auto unused =
      replaceOnce(counterSource(), "@ac.module\ndef Parent",
                  R"py(    @ac.method(guard=lambda amount: count >= amount)
    def unused(amount: ac.bits[8]) -> ac.bits[8]:
        count = count - amount
        return count
@ac.module
def Parent)py");
  auto file = typed(unused);
  ASSERT_TRUE(file);
  EXPECT_EQ(operations<ac::SourceMethodOp>(*file).size(), 4u);
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(transactions.size(), 1u);
  ASSERT_EQ(transactions[0].getNumResults(), 1u);
  EXPECT_TRUE(isa<ac::StructType>(transactions[0].getResult(0).getType()));
}

TEST_F(SourceTransactionCaptureTest,
       LogicalIntegerMethodResultIsNotAPredicate) {
  for (StringRef expression :
       {"not owner.value()", "1 if owner.value() else 0"}) {
    auto text = R"py(import pycircuit as ac
from typing import Annotated
@ac.module
def Owner():
    count: ac.u8 = 0
    @ac.method
    def value() -> Annotated[int, range(1 << 1)]:
        return 1
@ac.module
def Parent():
    owner = Owner()
    @ac.rule
    def step() -> None:
        result = EXPRESSION
    step()
)py";
    auto captured =
        capture("integer.py", replaceOnce(text, "EXPRESSION", expression));
    ASSERT_TRUE(captured);
    diagnostics.clear();
    EXPECT_TRUE(failed(lower(*captured, "integer.py")));
    EXPECT_NE(diagnostics.find("Boolean source condition, not Integer"),
              std::string::npos)
        << diagnostics;
    EXPECT_EQ(diagnostics.find("unexpected operation"), std::string::npos);
  }
}

TEST_F(SourceTransactionCaptureTest,
       BooleanMethodResultDoesNotBecomeFixedArithmetic) {
  auto captured = capture("boolean.py", R"py(import pycircuit as ac
@ac.module
def Owner():
    count: ac.u8 = 0
    @ac.method
    def ready() -> bool:
        return True
@ac.module
def Parent(value: ac.u1):
    owner = Owner()
    @ac.rule
    def step() -> None:
        flag = owner.ready()
        result = flag + value
    step()
)py");
  ASSERT_TRUE(captured);
  diagnostics.clear();
  EXPECT_TRUE(failed(lower(*captured, "boolean.py")));
  EXPECT_FALSE(diagnostics.empty());
  EXPECT_EQ(diagnostics.find("unexpected operation"), std::string::npos);
}

TEST_F(SourceTransactionCaptureTest, SystemMarkerSurvivesOrFailsExplicitly) {
  auto captured = capture("system.py", R"py(import pycircuit as ac
@ac.module
def Owner():
    count: ac.u8 = 0
    @ac.method
    def read() -> ac.u8:
        return count
@ac.system
def Closed():
    owner = Owner()
    @ac.rule
    def sample() -> None:
        value = owner.read()
    sample()
)py");
  ASSERT_TRUE(captured);
  diagnostics.clear();
  if (failed(lower(*captured, "system.py"))) {
    EXPECT_NE(StringRef(diagnostics).lower().find("system"), std::string::npos)
        << diagnostics;
    return;
  }
  auto transactions = operations<ac::SourceTransactionOp>(*captured);
  ASSERT_EQ(transactions.size(), 1u);
  auto root = transactions[0]->getParentOfType<ac::ModuleOp>();
  auto kind = root->getAttrOfType<StringAttr>("ac.root_kind");
  ASSERT_TRUE(kind);
  EXPECT_EQ(kind.getValue(), "system");
}

TEST_F(SourceTransactionCaptureTest,
       GuardedTopLevelRuleCannotSilentlyBecomePure) {
  auto file = capture("guarded.py", R"py(import pycircuit as ac
@ac.rule(guard=lambda value: value != 0)
def select(value: ac.u8) -> ac.u8:
    return value
@ac.module
def Parent(value: ac.u8) -> ac.u8:
    return select(value)
)py");
  ASSERT_TRUE(file);
  diagnostics.clear();
  auto result = lower(*file, "guarded.py");
  if (succeeded(result)) {
    // A future valid typed route must preserve the guard; pure success fails.
    auto transactions = operations<ac::SourceTransactionOp>(*file);
    ASSERT_FALSE(transactions.empty());
    EXPECT_TRUE(operations<ac::RuleOp>(*file).empty());
    EXPECT_FALSE(
        direct<ac::BitsCompareOp>(transactions.front().getGuard()).empty());
  } else {
    EXPECT_NE(StringRef(diagnostics).lower().find("guard"), std::string::npos)
        << diagnostics;
  }
}

TEST_F(SourceTransactionCaptureTest,
       GuardedPayloadIsStructuralBeforeSemanticAnalysis) {
  auto sourceText = replaceOnce(
      counterSource(), "@ac.method\n    def add",
      "@ac.method(guard=lambda delta: count >= delta)\n    def add");
  auto file = capture("structural.py", sourceText);
  ASSERT_TRUE(file);
  diagnostics.clear();
  ASSERT_TRUE(succeeded(lower(*file, "structural.py", /*runSummaries=*/false)))
      << diagnostics;
  ASSERT_TRUE(succeeded(mlir::verify(*file)));
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(transactions.size(), 1u);
  ASSERT_EQ(transactions[0].getNumResults(), 1u);
  EXPECT_TRUE(isa<ac::StructType>(transactions[0].getResult(0).getType()));
  diagnostics.clear();
  PassManager semantic(&context);
  semantic.addPass(acir::createAnalyzeSourceMethodSummariesPass());
  EXPECT_TRUE(failed(semantic.run(*file)));
  EXPECT_NE(StringRef(diagnostics).lower().find("none"), std::string::npos)
      << diagnostics;
}

TEST_F(SourceTransactionCaptureTest, OrdinaryCapturedTrueGuardsRemainTotal) {
  auto sourceText =
      replaceOnce(counterSource(), "    count: ac.bits[8] = 0",
                  "    count: ac.bits[8] = 0\n    allowed = True");
  sourceText =
      replaceOnce(sourceText, "@ac.method\n    def add",
                  "@ac.method(guard=lambda delta: allowed)\n    def add");
  sourceText = replaceOnce(sourceText, "    @ac.rule\n    def advance",
                           "    permitted = True\n    @ac.rule(guard=lambda: "
                           "permitted)\n    def advance");
  auto file = typed(sourceText);
  ASSERT_TRUE(file);
  ac::SourceMethodSummariesAnalysis summaries(file->getOperation());
  ASSERT_TRUE(summaries.isValid());
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(transactions.size(), 1u);
  EXPECT_TRUE(summaries.isTotal(transactions[0]));
  EXPECT_EQ(transactions[0].getNumResults(), 1u);
}

std::string perCallGuardSource(bool secondValue) {
  return std::string(R"py(import pycircuit as ac
@ac.struct
class View:
    before: ac.u8
    after: ac.u8
@ac.module
def Owner():
    count: ac.u8 = 0
    @ac.method(guard=lambda allow: allow)
    def probe(allow: bool) -> ac.u8:
        return count
@ac.module
def Parent() -> View:
    owner = Owner()
    @ac.rule
    def first() -> View:
        allowed = True
        value = owner.probe(allowed)
        return View(before=value, after=value)
    @ac.rule
    def second() -> )py") +
         (secondValue ? "View" : "None") + R"py(:
        denied = False
        value = owner.probe(denied)
)py" + (secondValue ? "        return View(before=value, after=value)\n" : "") +
         "    second()\n    return first()\n";
}

TEST_F(SourceTransactionCaptureTest,
       SameMethodActualContextsDoNotShareGuardProof) {
  // Visit both call contexts in one package. One has a known-true actual;
  // the other has known-false. The method and receiver are identical.
  auto file = typed(perCallGuardSource(/*secondValue=*/false));
  ASSERT_TRUE(file);
  ac::SourceMethodSummariesAnalysis summaries(file->getOperation());
  ASSERT_TRUE(summaries.isValid());
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(transactions.size(), 2u);
  unsigned total = 0, blocked = 0;
  for (auto transaction : transactions) {
    if (transaction.getNumResults()) {
      EXPECT_TRUE(summaries.isTotal(transaction));
      ++total;
    } else {
      EXPECT_FALSE(summaries.isTotal(transaction));
      ++blocked;
    }
  }
  EXPECT_EQ(total, 1u);
  EXPECT_EQ(blocked, 1u);
  reject(perCallGuardSource(/*secondValue=*/true), "none");
}

TEST_F(SourceTransactionCaptureTest,
       TransitiveBlockingCallsAreLegalForVoidRules) {
  auto sourceText = R"py(import pycircuit as ac
@ac.module
def Owner():
    count: ac.u8 = 0
    @ac.method(guard=lambda: False)
    def denied() -> ac.u8:
        return count
    @ac.method
    def relay() -> ac.u8:
        return denied()
@ac.module
def Parent():
    owner = Owner()
    @ac.rule
    def step() -> None:
        unused = owner.relay()
    step()
)py";
  for (bool directCall : {false, true}) {
    auto fixture =
        directCall ? replaceOnce(sourceText, "owner.relay()", "owner.denied()")
                   : std::string(sourceText);
    auto file = typed(fixture);
    ASSERT_TRUE(file);
    ac::SourceMethodSummariesAnalysis summaries(file->getOperation());
    ASSERT_TRUE(summaries.isValid());
    auto transactions = operations<ac::SourceTransactionOp>(*file);
    ASSERT_EQ(transactions.size(), 1u);
    EXPECT_EQ(transactions[0].getNumResults(), 0u);
    EXPECT_FALSE(summaries.isTotal(transactions[0]));

    auto valued = replaceOnce(fixture, "import pycircuit as ac",
                              R"py(import pycircuit as ac
@ac.struct
class View:
    value: ac.u8)py");
    valued = replaceOnce(valued, "def Parent():", "def Parent() -> View:");
    valued = replaceOnce(valued, "def step() -> None:", "def step() -> View:");
    valued =
        replaceOnce(valued, "    step()",
                    "        return View(value=unused)\n    return step()");
    reject(valued, "none");
  }
}

TEST_F(SourceTransactionCaptureTest, StateInitializerIsNotAGuardConstantProof) {
  auto sourceText = R"py(import pycircuit as ac
@ac.struct
class View:
    value: ac.u8
@ac.module
def Owner():
    ready: ac.u1 = 1
    count: ac.u8 = 0
    @ac.method(guard=lambda: ready)
    def read() -> ac.u8:
        return count
@ac.module
def Parent() -> View:
    owner = Owner()
    @ac.rule
    def step() -> View:
        value = owner.read()
        return View(value=value)
    return step()
)py";
  reject(sourceText, "none");
  auto voidSource =
      replaceOnce(sourceText, "def Parent() -> View:", "def Parent():");
  voidSource =
      replaceOnce(voidSource, "def step() -> View:", "def step() -> None:");
  voidSource =
      replaceOnce(voidSource, "        return View(value=value)\n", "");
  voidSource = replaceOnce(voidSource, "    return step()", "    step()");
  auto file = typed(voidSource);
  ASSERT_TRUE(file);
  ac::SourceMethodSummariesAnalysis summaries(file->getOperation());
  ASSERT_TRUE(summaries.isValid());
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(transactions.size(), 1u);
  EXPECT_FALSE(summaries.isTotal(transactions[0]));
}

TEST_F(SourceTransactionCaptureTest,
       RecursiveMethodsRejectEvenWhenUnusedOrGuardedOff) {
  reject(replaceOnce(counterSource(), "saved = add(delta)",
                     "saved = relay(delta)"),
         "recursive");
  auto unused =
      replaceOnce(counterSource(), "@ac.module\ndef Parent", R"py(    @ac.method
    def cycle(value: ac.bits[8]) -> ac.bits[8]:
        return cycle(value)
@ac.module
def Parent)py");
  reject(unused, "recursive");
  auto guardedOff =
      replaceOnce(unused, "@ac.method\n    def cycle",
                  "@ac.method(guard=lambda value: False)\n    def cycle");
  reject(guardedOff, "recursive");
  auto reachable = replaceOnce(counterSource(), "saved = add(delta)",
                               "saved = relay(delta)");
  reachable =
      replaceOnce(reachable, "@ac.method\n    def relay",
                  "@ac.method(guard=lambda delta: False)\n    def relay");
  reject(reachable, "recursive");
}

TEST_F(SourceTransactionCaptureTest, StatusFieldsDoNotIntroduceImplicitGuards) {
  auto file = typed(R"py(import pycircuit as ac
@ac.struct
class Result:
    retry: ac.u1
    accepted: ac.u1
    value: ac.u8
@ac.module
def Owner():
    count: ac.u8 = 0
    @ac.method
    def attempt() -> Result:
        count = count + 1
        return Result(retry=1, accepted=0, value=count)
@ac.module
def Parent() -> Result:
    owner = Owner()
    @ac.rule
    def step() -> Result:
        return owner.attempt()
    return step()
)py");
  ASSERT_TRUE(file);
  ac::SourceMethodSummariesAnalysis summaries(file->getOperation());
  ASSERT_TRUE(summaries.isValid());
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(transactions.size(), 1u);
  EXPECT_TRUE(summaries.isTotal(transactions[0]));
  EXPECT_EQ(operations<ac::SourceStateWriteOp>(*file).size(), 1u);
  EXPECT_TRUE(isa<ac::StructType>(transactions[0].getResult(0).getType()));
}

TEST_F(SourceTransactionCaptureTest,
       PersistentStateAnnotationsKeepLogicalAuthority) {
  const char *integer = R"py(import pycircuit as ac
from typing import Annotated
@ac.module
def Owner():
    ready: Annotated[int, range(1 << 1)] = 1
    @ac.method(guard=lambda: ready)
    def read() -> ac.u1:
        return 1
@ac.module
def Parent():
    owner = Owner()
    @ac.rule
    def step() -> None:
        unused = owner.read()
    step()
)py";
  auto wrapping =
      replaceOnce(integer, "@ac.method(guard=lambda: ready)", "@ac.method");
  wrapping = replaceOnce(wrapping, "def read() -> ac.u1:",
                         "def read() -> Annotated[int, range(1 << 1)]:");
  wrapping = replaceOnce(wrapping, "        return 1",
                         "        ready = ready + 1\n        return ready");
  auto boolean =
      replaceOnce(integer, "ready: Annotated[int, range(1 << 1)] = 1",
                  "ready: bool = True");
  boolean =
      replaceOnce(boolean, "@ac.method(guard=lambda: ready)", "@ac.method");
  boolean = replaceOnce(boolean, "        return 1",
                        "        ready = ready + 1\n        return ready");
  for (const auto &fixture : {std::string(integer), wrapping, boolean}) {
    auto captured = capture("authority.py", fixture);
    ASSERT_TRUE(captured);
    diagnostics.clear();
    // This bounded producer may reject non-fixed state outright. It may not
    // reinterpret Integer/Boolean owners as wrapping fixed Bits or predicates.
    EXPECT_TRUE(failed(lower(*captured, "authority.py")));
    EXPECT_NE(diagnostics.find("requires explicitly fixed Bits annotation"),
              std::string::npos)
        << diagnostics;
    EXPECT_EQ(diagnostics.find("unexpected operation"), std::string::npos);
  }
}

std::string shadowedLocalFormalSource(bool value) {
  return std::string(R"py(import pycircuit as ac
@ac.struct
class View:
    value: ac.u1
@ac.module
def Owner():
    count: ac.u8 = 0
    enabled = True
    @ac.method(guard=lambda enabled: enabled)
    def echo(enabled: ac.u1) -> ac.u1:
        return enabled
@ac.module
def Parent() -> View:
    owner = Owner()
    @ac.rule
    def step() -> View:
        actual: ac.u1 = )py") +
         (value ? "True" : "False") + R"py(
        result = owner.echo(actual)
        return View(value=result)
    return step()
)py";
}

TEST_F(SourceTransactionCaptureTest,
       FormalShadowsOrdinaryLocalWithoutCapturingIt) {
  auto file = typed(shadowedLocalFormalSource(/*value=*/true));
  ASSERT_TRUE(file);
  auto methods = operations<ac::SourceMethodOp>(*file);
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(methods.size(), 1u);
  ASSERT_EQ(transactions.size(), 1u);
  EXPECT_TRUE(methods[0].getCaptures().empty());
  auto &guard = methods[0].getGuard().front();
  auto &body = methods[0].getBody().front();
  ASSERT_EQ(guard.getNumArguments(), 1u);
  ASSERT_EQ(body.getNumArguments(), 1u);
  auto guardYield = cast<ac::YieldOp>(guard.getTerminator());
  auto bodyYield = cast<ac::YieldOp>(body.getTerminator());
  ASSERT_EQ(guardYield.getNumOperands(), 1u);
  ASSERT_EQ(bodyYield.getNumOperands(), 1u);
  EXPECT_TRUE(
      identityTransport(guardYield.getOperand(0), guard.getArgument(0), *file));
  EXPECT_TRUE(
      identityTransport(bodyYield.getOperand(0), body.getArgument(0), *file));
  ac::SourceMethodSummariesAnalysis summaries(file->getOperation());
  ASSERT_TRUE(summaries.isValid());
  EXPECT_TRUE(summaries.isTotal(transactions[0]));
  reject(shadowedLocalFormalSource(/*value=*/false), "none");
}

TEST_F(SourceTransactionCaptureTest,
       FormalsCannotRedirectCapturedHardwareCalls) {
  auto methodName = replaceOnce(
      counterSource(),
      "def relay(delta: ac.bits[8]) -> ac.bits[8]:\n        saved = add(delta)",
      "def relay(add: ac.bits[8]) -> ac.bits[8]:\n        saved = add(add)");
  reject(methodName, "shadow");
  auto handleName =
      replaceOnce(counterSource(), "@ac.module\ndef Parent", R"py(@ac.module
def Wrapper():
    child = Counter()
    @ac.method
    def read() -> ac.bits[8]:
        return child.read()
    @ac.method
    def relay(child: ac.bits[8]) -> ac.bits[8]:
        return child.add(child)
@ac.module
def Parent)py");
  handleName =
      replaceOnce(handleName, "    left = Counter()", "    left = Wrapper()");
  reject(handleName, "shadow");
}

TEST_F(SourceTransactionCaptureTest,
       LocalBindingsCannotRedirectCapturedHardwareCalls) {
  for (bool annotated : {false, true}) {
    const std::string annotation = annotated ? ": ac.bits[8]" : "";
    auto method = replaceOnce(counterSource(), "        saved = add(delta)",
                              "        add" + annotation +
                                  " = delta\n        saved = add(delta)");
    reject(method, "shadow");

    auto instance = replaceOnce(counterSource(), "        before = left.read()",
                                "        left" + annotation +
                                    " = delta\n        before = left.read()");
    reject(instance, "shadow");

    auto rule = replaceOnce(
        counterSource(), "        before = left.read()",
        "        advance" + annotation +
            " = delta\n"
            "        ignored = advance()\n        before = left.read()");
    reject(rule, "shadow");

    auto methodHandle =
        replaceOnce(counterSource(), "@ac.module\ndef Parent", R"py(@ac.module
def Wrapper():
    child = Counter()
    @ac.method
    def read() -> ac.bits[8]:
        return child.read()
    @ac.method
    def relay(delta: ac.bits[8]) -> ac.bits[8]:
        return child.add(delta)
@ac.module
def Parent)py");
    methodHandle = replaceOnce(methodHandle, "    left = Counter()",
                               "    left = Wrapper()");
    methodHandle = replaceOnce(methodHandle, "        return child.add(delta)",
                               "        child" + annotation +
                                   " = delta\n        return child.add(delta)");
    reject(methodHandle, "shadow");
  }
}

TEST_F(SourceTransactionCaptureTest,
       MethodSourceCallAuthoritySurvivesPrintAndReparse) {
  auto file = typed(R"py(import pycircuit as ac
from typing import Annotated
@ac.struct
class Packet:
    value: ac.u1
@ac.module
def Owner():
    count: ac.u8 = 0
    @ac.method
    def noop() -> None:
        pass
    allowed = True
    @ac.method
    def boolean(value: bool) -> bool:
        return value
    @ac.method
    def integer(value: Annotated[int, range(1 << 1)]) -> Annotated[int, range(1 << 1)]:
        return value
    @ac.method
    def fixed(value: ac.u1) -> ac.u1:
        return value
    @ac.method
    def packet(value: Packet) -> Packet:
        return value
    @ac.method(guard=lambda: allowed)
    def captured() -> None:
        pass
)py",
                    "method_authority.py");
  ASSERT_TRUE(file);
  auto original = operations<ac::SourceMethodOp>(*file);
  ASSERT_EQ(original.size(), 6u);
  std::string text;
  llvm::raw_string_ostream stream(text);
  file->print(stream);
  auto reparsed = parseSourceString<ModuleOp>(text, &context);
  ASSERT_TRUE(reparsed) << diagnostics;
  ASSERT_TRUE(succeeded(mlir::verify(*reparsed))) << diagnostics;
  auto methods = operations<ac::SourceMethodOp>(*reparsed);
  ASSERT_EQ(methods.size(), original.size());
  for (auto [before, after] : llvm::zip(original, methods)) {
    for (StringRef name :
         {"ac.parameters", "ac.result_constraints", "ac.return_form"}) {
      ASSERT_TRUE(before->hasAttr(name));
      EXPECT_EQ(before->getAttr(name), after->getAttr(name));
    }
    auto parameters = after->getAttrOfType<ArrayAttr>("ac.parameters");
    auto results = after->getAttrOfType<ArrayAttr>("ac.result_constraints");
    auto form = after->getAttrOfType<StringAttr>("ac.return_form");
    ASSERT_TRUE(parameters);
    ASSERT_TRUE(results);
    ASSERT_TRUE(form);
    EXPECT_EQ(parameters.size(), after.getFunctionType().getNumInputs());
    EXPECT_EQ(results.size(), after.getFunctionType().getNumResults());
    EXPECT_EQ(form.getValue(), results.empty() ? "none" : "single");
    auto owner = after->getParentOfType<ac::ModuleOp>();
    auto sourceOwner = owner.getSourceOwner();
    for (auto [index, raw] : llvm::enumerate(parameters)) {
      auto row = dyn_cast<DictionaryAttr>(raw);
      ASSERT_TRUE(row);
      EXPECT_EQ(row.size(), 6u);
      EXPECT_EQ(row.getAs<StringAttr>("name"), after.getArgumentNames()[index]);
      ASSERT_TRUE(row.getAs<StringAttr>("binding"));
      EXPECT_EQ(row.getAs<StringAttr>("binding").getValue(),
                "positional_or_keyword");
      auto defaultValue = row.getAs<DictionaryAttr>("default");
      ASSERT_TRUE(defaultValue);
      ASSERT_TRUE(defaultValue.getAs<BoolAttr>("present"));
      EXPECT_FALSE(defaultValue.getAs<BoolAttr>("present").getValue());
      EXPECT_EQ(defaultValue.size(), 1u);
      auto origin = row.getAs<DictionaryAttr>("origin");
      ASSERT_TRUE(origin);
      auto site = origin.getAs<DictionaryAttr>("site");
      ASSERT_TRUE(site);
      EXPECT_EQ(site.getAs<FlatSymbolRefAttr>("definition"),
                FlatSymbolRefAttr::get(&context, owner.getSymName()));
      auto location = row.getAs<DictionaryAttr>("location");
      ASSERT_TRUE(location);
      EXPECT_EQ(location.getAs<StringAttr>("path"),
                sourceOwner.getAs<StringAttr>("path"));
    }
    StringRef name = after.getSymName();
    if (name == "noop" || name == "captured") {
      EXPECT_TRUE(parameters.empty());
      EXPECT_TRUE(results.empty());
      EXPECT_EQ(before.getCaptures().size(), after.getCaptures().size());
      if (name == "captured")
        EXPECT_GT(after.getCaptures().size(), 0u);
      else
        EXPECT_TRUE(after.getCaptures().empty());
      continue;
    }
    ASSERT_EQ(parameters.size(), 1u);
    ASSERT_EQ(results.size(), 1u);
    auto parameterConstraint =
        cast<DictionaryAttr>(parameters[0]).getAs<DictionaryAttr>("constraint");
    auto resultConstraint = dyn_cast<DictionaryAttr>(results[0]);
    ASSERT_TRUE(parameterConstraint);
    ASSERT_TRUE(resultConstraint);
    EXPECT_EQ(parameterConstraint.get("source_kind"),
              resultConstraint.get("source_kind"));
    EXPECT_EQ(parameterConstraint.get("domain"),
              resultConstraint.get("domain"));
    EXPECT_EQ(resultConstraint.getAs<TypeAttr>("type").getValue(),
              after.getFunctionType().getResult(0));
    EXPECT_EQ(parameterConstraint.getAs<StringAttr>("kind").getValue(),
              "hardware");
    EXPECT_EQ(parameterConstraint.getAs<TypeAttr>("type").getValue(),
              after.getFunctionType().getInput(0));
    auto sourceKind = parameterConstraint.getAs<StringAttr>("source_kind");
    ASSERT_TRUE(sourceKind);
    if (name == "packet") {
      EXPECT_EQ(sourceKind.getValue(), "nominal");
      EXPECT_TRUE(isa<ac::StructType>(after.getFunctionType().getInput(0)));
      EXPECT_FALSE(parameterConstraint.get("domain"));
      continue;
    }
    ac::HardwareAnalysis analysis(*reparsed);
    ac::HardwareBindings bindings;
    bindings.owner = owner;
    auto width = analysis.getPackedWidth(after.getFunctionType().getInput(0),
                                         bindings, after);
    ASSERT_TRUE(succeeded(width));
    EXPECT_EQ(*width, 1u);
    if (name == "fixed") {
      EXPECT_EQ(sourceKind.getValue(), "fixed_bits");
      EXPECT_FALSE(parameterConstraint.get("domain"));
    } else {
      auto domain = parameterConstraint.getAs<ac::SourceDomainAttr>("domain");
      ASSERT_TRUE(domain);
      auto fields = domain.getValue();
      if (name == "boolean") {
        EXPECT_EQ(sourceKind.getValue(), "boolean");
        EXPECT_EQ(fields.getAs<StringAttr>("kind").getValue(), "bool");
      } else {
        EXPECT_EQ(name, "integer");
        EXPECT_EQ(sourceKind.getValue(), "integer");
        EXPECT_EQ(fields.getAs<StringAttr>("kind").getValue(), "integer");
        EXPECT_EQ(fields.getAs<ac::MathIntAttr>("lower").getCanonicalValue(),
                  "0");
        EXPECT_EQ(fields.getAs<ac::MathIntAttr>("upper").getCanonicalValue(),
                  "2");
      }
    }
  }
}

TEST_F(SourceTransactionCaptureTest,
       VoidMethodContractRejectsPhantomParametersAndResults) {
  auto file = typed(R"py(import pycircuit as ac
@ac.module
def Owner():
    count: ac.u8 = 0
    @ac.method
    def noop() -> None:
        pass
)py");
  ASSERT_TRUE(file);
  auto methods = operations<ac::SourceMethodOp>(*file);
  ASSERT_EQ(methods.size(), 1u);
  Builder builder(&context);
  for (StringRef attribute :
       {"ac.parameters", "ac.result_constraints", "ac.return_form"}) {
    auto saved = methods[0]->getAttr(attribute);
    ASSERT_TRUE(saved);
    methods[0]->setAttr(
        attribute,
        attribute == "ac.return_form"
            ? Attribute(builder.getStringAttr("single"))
            : Attribute(builder.getArrayAttr({builder.getUnitAttr()})));
    diagnostics.clear();
    EXPECT_TRUE(failed(mlir::verify(*file))) << attribute.str();
    EXPECT_FALSE(diagnostics.empty());
    methods[0]->setAttr(attribute, saved);
  }
  ASSERT_TRUE(succeeded(mlir::verify(*file))) << diagnostics;
}

} // namespace
