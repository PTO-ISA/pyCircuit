#include "Compiler/PythonImportRecords.h"
#include "Compiler/SourceMemoryBindings.h"
#include "Compiler/SourceRuleWrites.h"
#include "Compiler/SourceUnit.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/AsmParser/AsmParser.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
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

// These tests cover capture, typed effects, and source publication authority.
// They do not execute a design or establish final transaction lowering.
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
  SmallVector<ac::InstanceOp> queueInstances(ModuleOp file) {
    SmallVector<ac::InstanceOp> result;
    file.walk([&](ac::InstanceOp instance) {
      if (ac::isQueueInstance(instance))
        result.push_back(instance);
    });
    return result;
  }
  template <typename Op> SmallVector<Op> direct(Region &region) {
    SmallVector<Op> result;
    for (auto operation : region.front().getOps<Op>())
      result.push_back(operation);
    return result;
  }
  ac::SourceMethodOp entryMethod(ac::SourceTransactionOp transaction) {
    auto entry = ac::getSourceRegistrationEntry(transaction);
    EXPECT_TRUE(succeeded(entry)) << diagnostics;
    if (failed(entry)) return {};
    EXPECT_TRUE(entry->has_value());
    return *entry ? (**entry).method : ac::SourceMethodOp();
  }
  Region &executionBody(ac::SourceTransactionOp transaction) {
    auto entry = entryMethod(transaction);
    return entry ? entry.getBody() : transaction.getBody();
  }
  Region &executionGuard(ac::SourceTransactionOp transaction) {
    auto entry = entryMethod(transaction);
    return entry ? entry.getGuard() : transaction.getGuard();
  }
  ac::SourceMethodOp namedMethod(ModuleOp package, StringRef name) {
    for (auto method : operations<ac::SourceMethodOp>(package))
      if (method.getSymName() == name) return method;
    return {};
  }
  void compilerAvailability(ac::SourceTransactionOp transaction) {
    auto owner = transaction->getParentOfType<ac::ModuleOp>();
    auto policy = owner->getAttrOfType<ArrayAttr>("ac.result_availability");
    ASSERT_TRUE(policy);
    ASSERT_EQ(policy.size(), owner.getFunctionType().getNumResults());
    for (Attribute raw : policy) {
      auto mode = dyn_cast<StringAttr>(raw);
      ASSERT_TRUE(mode);
      EXPECT_EQ(mode.getValue(), "compiler");
    }
  }
  void managedResult(StringRef text, bool total = false) {
    auto file = typed(text);
    ASSERT_TRUE(file) << diagnostics;
    ac::SourceMethodSummariesAnalysis summaries(file->getOperation());
    ASSERT_TRUE(summaries.isValid()) << diagnostics;
    unsigned valued = 0;
    for (auto transaction : operations<ac::SourceTransactionOp>(*file))
      if (transaction.getNumResults()) {
        ++valued;
        EXPECT_EQ(summaries.isTotal(transaction), total);
        compilerAvailability(transaction);
      }
    EXPECT_NE(valued, 0u);
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
  DictionaryAttr changed(DictionaryAttr dictionary, StringRef field, Attribute value) {
    NamedAttrList fields(dictionary);
    if (value) fields.set(field, value);
    else fields.erase(field);
    return fields.getDictionary(&context);
  }
  DictionaryAttr changedRow(DictionaryAttr dictionary, StringRef field,
                            unsigned ordinal, DictionaryAttr value) {
    auto array = dictionary.getAs<ArrayAttr>(field);
    SmallVector<Attribute> rows(array.begin(), array.end());
    rows[ordinal] = value;
    return changed(dictionary, field, Builder(&context).getArrayAttr(rows));
  }
  unsigned rowIndex(DictionaryAttr dictionary, StringRef section, StringRef name) {
    for (auto [i, raw] : llvm::enumerate(dictionary.getAs<ArrayAttr>(section)))
      if (cast<DictionaryAttr>(raw).getAs<StringAttr>("name").getValue() == name)
        return i;
    ADD_FAILURE() << "missing summary row " << name.str();
    return 0;
  }
};

TEST_F(SourceTransactionCaptureTest, OrdinaryHelpersInlineIntoRuleAndMethodBodies) {
  auto file = typed(R"py(import pycircuit as ac
@ac.module
def Owner():
    count: ac.u8 = 0
    bias = 1
    def increment(value: ac.u8) -> ac.u8:
        typed_bias: ac.u8 = bias
        intermediate: ac.u8 = value + typed_bias
        return intermediate
    @ac.method
    def add(__pyc_helper_snapshot_0: ac.u8) -> ac.u8:
        count = increment(count + __pyc_helper_snapshot_0)
        return count
    @ac.rule
    def advance() -> None:
        count = increment(count)
    advance()
)py", "helpers.py");
  ASSERT_TRUE(file);
  auto methods = operations<ac::SourceMethodOp>(*file);
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(methods.size(), 2u);
  ASSERT_EQ(transactions.size(), 1u);
  for (auto *scope : {namedMethod(*file, "add").getOperation(),
                      entryMethod(transactions[0]).getOperation()}) {
    unsigned calls = 0;
    scope->walk([&](mlir::func::CallOp) { ++calls; });
    EXPECT_EQ(calls, 0u);
  }
  EXPECT_FALSE(operations<ac::BitsBinaryOp>(*file).empty());
}

TEST_F(SourceTransactionCaptureTest, OrdinaryHelperLoopsInlineIntoRuleAndMethodBodies) {
  auto file = typed(R"py(import pycircuit as ac
def step(value: ac.u8) -> ac.u8:
    for ordinal in range(2):
        delta: ac.u8 = ordinal
        value = value + delta
    return value
@ac.module
def Owner():
    count: ac.u8 = 0
    bias = 3
    def shifted(value: ac.u8) -> ac.u8:
        result = value
        typed_bias: ac.u8 = bias
        for ordinal in range(2):
            if result < typed_bias:
                result = step(result)
            else:
                result = result ^ typed_bias
        return result
    bias = 7
    @ac.method
    def add(value: ac.u8) -> ac.u8:
        count = shifted(value)
        return count
    @ac.rule
    def advance() -> None:
        count = shifted(count)
    advance()
)py", "helper_loops.py");
  ASSERT_TRUE(file);
  auto methods = operations<ac::SourceMethodOp>(*file);
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(methods.size(), 2u);
  ASSERT_EQ(transactions.size(), 1u);
  // The helper is ordinary combinational SSA. Only the caller owns the write;
  // helper calls do not create an autonomous registration or a state owner.
  EXPECT_EQ(operations<ac::SourceStateOp>(*file).size(), 1u);
  for (auto *scope : {namedMethod(*file, "add").getOperation(),
                      entryMethod(transactions[0]).getOperation()}) {
    unsigned calls = 0, writes = 0;
    scope->walk([&](mlir::func::CallOp) { ++calls; });
    scope->walk([&](ac::SourceStateWriteOp) { ++writes; });
    EXPECT_EQ(calls, 0u);
    EXPECT_EQ(writes, 1u);
  }
}

TEST_F(SourceTransactionCaptureTest, HelperLoopCallsRetainDistinctIterationOccurrences) {
  auto file = typed(R"py(import pycircuit as ac
def leaf(value: ac.u8) -> ac.u8:
    return value + 5
def inner(value: ac.u8) -> ac.u8:
    for second in range(2):
        value = leaf(value)
    return value
def outer(value: ac.u8) -> ac.u8:
    for first in range(3):
        value = inner(value)
    return value
@ac.struct
class Result:
    value: ac.u8
@ac.module
def Top(value: ac.u8) -> Result:
    return Result(value=outer(value))
)py", "loop_origins.py");
  ASSERT_TRUE(file);
  std::array<bool, 6> occurrences{};
  unsigned constants = 0;
  for (auto definition : operations<ac::ModuleOp>(*file)) {
    definition.walk([&](ac::BitsConstantOp operation) {
      auto expression = operation->getAttrOfType<ac::StaticExprAttr>("value");
      if (!expression) return;
      auto origin = expression.getTree().getAs<DictionaryAttr>("origin");
      if (!origin) return;
      auto site = origin.getAs<DictionaryAttr>("site");
      if (!site || site.getAs<FlatSymbolRefAttr>("definition").getValue() !=
                       "loop_origins.leaf") return;
      ++constants;
      std::optional<unsigned> first, second;
      for (Attribute raw : origin.getAs<ArrayAttr>("expansion")) {
        auto frame = cast<DictionaryAttr>(raw);
        if (frame.getAs<StringAttr>("kind").getValue() != "iteration") continue;
        auto loopSite = frame.getAs<DictionaryAttr>("site");
        auto symbol = loopSite.getAs<FlatSymbolRefAttr>("definition");
        auto ordinal = frame.getAs<IntegerAttr>("ordinal");
        ASSERT_TRUE(ordinal);
        if (symbol.getValue() == "loop_origins.outer") {
          EXPECT_FALSE(first.has_value());
          first = ordinal.getUInt();
        }
        if (symbol.getValue() == "loop_origins.inner") {
          EXPECT_FALSE(second.has_value());
          second = ordinal.getUInt();
        }
      }
      ASSERT_TRUE(first.has_value());
      ASSERT_TRUE(second.has_value());
      ASSERT_LT(*first, 3u);
      ASSERT_LT(*second, 2u);
      unsigned index = *first * 2 + *second;
      EXPECT_FALSE(occurrences[index]);
      occurrences[index] = true;
    });
  }
  EXPECT_EQ(constants, 6u);
  for (bool observed : occurrences) EXPECT_TRUE(observed);
}

TEST_F(SourceTransactionCaptureTest, HelperLoopProviderPublishesTypedBodiesForIndependentConsumers) {
  auto provider = capture("loop_provider.py", R"py(import pycircuit as ac
def fold(values: ac.table[3, ac.u8]) -> ac.u8:
    result: ac.u8 = 0
    for index in range(3):
        result = result + values[index]
    return result
)py");
  ASSERT_TRUE(provider);
  auto empty = compiler::SourceHeaderRegistry::create(&context, {}, error());
  ASSERT_TRUE(succeeded(empty));
  auto published = compiler::compilePythonSourceUnit(
      *provider, owner("loop_provider.py"), *empty, error());
  ASSERT_TRUE(succeeded(published)) << diagnostics;
  ASSERT_TRUE(succeeded(compiler::verifyIntrinsicSourceUnitPair(
      *published->body, *published->interface, error()))) << diagnostics;
  // Consumer capture imports syntax only; its native compilation sees the
  // provider's published header rather than importing/executing provider Python.
  auto consumer = capture("loop_consumer.py", R"py(import pycircuit as ac
from loop_provider import fold
@ac.struct
class Result:
    value: ac.u8
@ac.module
def Top(values: ac.table[3, ac.u8]) -> Result:
    return Result(value=fold(values))
)py");
  ASSERT_TRUE(consumer);
  auto supplied = compiler::SourceHeaderRegistry::create(
      &context, {*published->interface}, error());
  ASSERT_TRUE(succeeded(supplied)) << diagnostics;
  auto compiled = compiler::compilePythonSourceUnit(
      *consumer, owner("loop_consumer.py"), *supplied, error());
  ASSERT_TRUE(succeeded(compiled)) << diagnostics;
  ASSERT_TRUE(succeeded(supplied->verifyBodySnapshots(
      *compiled->body, *compiled->interface, error()))) << diagnostics;
  auto definitions = operations<ac::ModuleOp>(*compiled->body);
  ASSERT_EQ(definitions.size(), 1u);
  unsigned calls = 0;
  definitions[0].walk([&](mlir::func::CallOp) { ++calls; });
  EXPECT_EQ(calls, 0u);
  EXPECT_TRUE(operations<ac::SourceStateOp>(*compiled->body).empty());
  EXPECT_TRUE(operations<ac::SourceTransactionOp>(*compiled->body).empty());
}

TEST_F(SourceTransactionCaptureTest, DeferredNestedHelpersKeepLexicalScopeAndHiddenCaptureIdentity) {
  auto file = typed(R"py(import pycircuit as ac
@ac.struct
class Result:
    formal: ac.u8
    local: ac.u8
    forward: ac.u8
@ac.module
def Top(value: ac.u8) -> Result:
    offset = value + 1
    def base(arg: ac.u8) -> ac.u8:
        return arg + offset
    def caller(__pyc_capture_0: ac.u8) -> ac.u8:
        return base(__pyc_capture_0)
    def local(arg: ac.u8) -> ac.u8:
        __pyc_capture_0 = arg
        return base(__pyc_capture_0)
    def first(arg: ac.u8) -> ac.u8:
        return later(arg)
    def later(arg: ac.u8) -> ac.u8:
        return arg + 1
    offset = value + 7
    return Result(formal=caller(value), local=local(value), forward=first(value))
)py", "hidden_capture.py");
  ASSERT_TRUE(file);
  for (auto definition : operations<ac::ModuleOp>(*file)) {
    unsigned calls = 0;
    definition->walk([&](mlir::func::CallOp) { ++calls; });
    EXPECT_EQ(calls, 0u);
  }
}

TEST_F(SourceTransactionCaptureTest, HelperBindingsMayShadowRuleNamesAsOrdinaryValues) {
  auto file = typed(R"py(import pycircuit as ac
@ac.struct
class Result:
    formal: ac.u8
    local: ac.u8
@ac.rule
def legacy(value: ac.u8) -> ac.u8:
    return value
@ac.module
def Top(value: ac.u8) -> Result:
    def formal(legacy: ac.u8) -> ac.u8:
        return legacy + 1
    def local(arg: ac.u8) -> ac.u8:
        legacy = arg
        return legacy + 1
    return Result(formal=formal(value), local=local(value))
)py", "rule_shadow.py");
  ASSERT_TRUE(file);
}

TEST_F(SourceTransactionCaptureTest, OrdinaryModuleResultWiresCanBeCapturedAsPayloadSnapshots) {
  auto file = typed(R"py(import pycircuit as ac
@ac.struct
class View:
    value: ac.u8
@ac.module
def Child(value: ac.u8) -> View:
    return View(value=value + 1)
@ac.module
def Top(value: ac.u8) -> View:
    wire = Child(value)
    def helper(arg: ac.u8) -> ac.u8:
        return wire.value + arg
    return View(value=helper(value))
)py", "wire_capture.py");
  ASSERT_TRUE(file);
  EXPECT_EQ(operations<ac::InstanceOp>(*file).size(), 1u);
}

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
  ASSERT_EQ(methods.size(), 4u);
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
  auto outer = direct<ac::SourceMethodCallOp>(executionBody(transactions[0]));
  ASSERT_EQ(outer.size(), 2u);
  EXPECT_TRUE(outer[0]->isBeforeInBlock(outer[1]));
  EXPECT_EQ(outer[0].getInstancePath(), outer[1].getInstancePath());
  ASSERT_EQ(outer[0].getInstancePath().size(), 1u);
  unsigned localCalls = 0;
  for (auto method : methods)
    for (auto call : direct<ac::SourceMethodCallOp>(method.getBody())) {
      if (!call.getInstancePath().empty()) {
        EXPECT_EQ(method.getSymName(), "advance");
        EXPECT_EQ(call.getInstancePath(), outer[0].getInstancePath());
        continue;
      }
      ++localCalls;
      EXPECT_EQ(method.getSymName(), "relay");
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
  auto calls = direct<ac::SourceMethodCallOp>(executionBody(transactions[0]));
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
    EXPECT_EQ(operations<ac::SourceMethodOp>(*file).size(), 4u);
    EXPECT_EQ(operations<ac::SourceTransactionOp>(*file).size(), 1u);
    EXPECT_EQ(operations<ac::SourceMethodCallOp>(*file).size(), 4u);
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
  EXPECT_EQ(operations<ac::SourceMethodOp>(*file).size(), 4u);
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
  ASSERT_EQ(methods.size(), 2u);
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
       GuardedNestedRuleClassifiesWithoutDummyMethodOrState) {
  auto guarded = typed(R"py(import pycircuit as ac
@ac.module
def GuardOnly(enabled: ac.u1):
    @ac.rule(guard=lambda: enabled)
    def tick() -> None:
        pass
    tick()
)py",
                       "guard_only.py");
  ASSERT_TRUE(guarded);
  auto transactions = operations<ac::SourceTransactionOp>(*guarded);
  ASSERT_EQ(transactions.size(), 1u);
  EXPECT_EQ(operations<ac::SourceMethodOp>(*guarded).size(), 1u);
  EXPECT_TRUE(operations<ac::SourceStateOp>(*guarded).empty());
  EXPECT_EQ(transactions.front().getName(), "tick");
  EXPECT_EQ(transactions.front().getNumResults(), 0u);
  EXPECT_TRUE(transactions.front().getClk());
  EXPECT_TRUE(transactions.front().getRst());
  auto owner = transactions.front()->getParentOfType<ac::ModuleOp>();
  ASSERT_TRUE(owner);
  auto domain = owner->getAttrOfType<DictionaryAttr>("ac.domain_inputs");
  ASSERT_TRUE(domain);
  EXPECT_TRUE(domain.getAs<IntegerAttr>("clock"));
  EXPECT_TRUE(domain.getAs<IntegerAttr>("reset"));

  auto ordinary = typed(R"py(import pycircuit as ac
@ac.module
def Pure(enabled: ac.u1) -> {"out": ac.u1}:
    @ac.rule
    def tick() -> None:
        pass
    tick()
    return {"out": enabled}
)py",
                        "ordinary_rule.py");
  ASSERT_TRUE(ordinary);
  EXPECT_EQ(operations<ac::SourceTransactionOp>(*ordinary).size(), 1u);
  EXPECT_EQ(operations<ac::SourceMethodOp>(*ordinary).size(), 1u);
  EXPECT_TRUE(operations<ac::RuleOp>(*ordinary).empty());
}

TEST_F(SourceTransactionCaptureTest,
       GuardedNestedRuleRegistrationArgumentsRemainUnsupported) {
  reject(R"py(import pycircuit as ac
@ac.module
def GuardOnly(enabled: ac.u1):
    @ac.rule(guard=lambda: enabled)
    def tick() -> None:
        pass
    tick(enabled)
)py",
         "argument");
}

TEST_F(SourceTransactionCaptureTest,
       GuardLocationsUseExactLambdaAndBareFunctionDefinitionSpans) {
  auto file = typed(R"py(import pycircuit as ac
@ac.module
def Owner():
    enabled = True
    @ac.method(guard=lambda: enabled)
    def explicit_method() -> None:
        pass
    @ac.method
    def bare_method() -> None:
        pass
    @ac.rule(guard=lambda: enabled)
    def explicit_rule() -> None:
        pass
    @ac.rule
    def bare_rule() -> None:
        pass
    explicit_rule()
    bare_rule()
)py",
                    "guard_spans.py");
  ASSERT_TRUE(file);
  auto expectSpan = [&](Operation *callable, unsigned line, unsigned column,
                        unsigned endLine, unsigned endColumn) {
    auto span = callable->getAttrOfType<DictionaryAttr>("ac.guard_location");
    ASSERT_TRUE(span);
    EXPECT_EQ(span.size(), 5u);
    EXPECT_EQ(span.getAs<StringAttr>("path").getValue(), "guard_spans.py");
    EXPECT_EQ(span.getAs<IntegerAttr>("line").getInt(), line);
    EXPECT_EQ(span.getAs<IntegerAttr>("column").getInt(), column);
    EXPECT_EQ(span.getAs<IntegerAttr>("end_line").getInt(), endLine);
    EXPECT_EQ(span.getAs<IntegerAttr>("end_column").getInt(), endColumn);
  };
  auto methods = operations<ac::SourceMethodOp>(*file);
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(methods.size(), 4u);
  ASSERT_EQ(transactions.size(), 2u);
  for (auto method : methods) {
    if (method.getSymName() == "explicit_method")
      expectSpan(method, 5, 22, 5, 37);
    else if (method.getSymName() == "bare_method")
      expectSpan(method, 9, 5, 10, 13);
    else if (method.getSymName() == "explicit_rule")
      expectSpan(method, 11, 20, 11, 35);
    else if (method.getSymName() == "bare_rule")
      expectSpan(method, 15, 5, 16, 13);
    else
      ADD_FAILURE() << "unexpected method " << method.getSymName().str();
  }
  for (auto transaction : transactions) {
    if (transaction.getName() == "explicit_rule")
      expectSpan(transaction, 11, 20, 11, 35);
    else if (transaction.getName() == "bare_rule")
      expectSpan(transaction, 15, 5, 16, 13);
    else
      ADD_FAILURE() << "unexpected transaction " << transaction.getName().str();
  }
}

TEST_F(SourceTransactionCaptureTest,
       UnitCompilationPublishesTypedEffectsWithoutMutatingCapture) {
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
  ASSERT_TRUE(succeeded(result)) << diagnostics;
  EXPECT_EQ(print(), before);
  EXPECT_TRUE(succeeded(mlir::verify(*captured)));
  EXPECT_FALSE(operations<ac::SourceMethodOp>(*result->body).empty());
  EXPECT_FALSE(operations<ac::SourceTransactionOp>(*result->body).empty());
  EXPECT_TRUE(operations<ac::SourceMethodOp>(*result->interface).empty());
  EXPECT_TRUE(operations<ac::SourceTransactionOp>(*result->interface).empty());
  unsigned transactionProviders = 0;
  for (auto declaration : result->interface->getOps<ac::ModuleImportOp>()) {
    if (declaration.getSymName().ends_with("Counter") ||
        declaration.getSymName().ends_with("Parent")) {
      ++transactionProviders;
      EXPECT_EQ(declaration.getSemanticKind(), "transactions");
      EXPECT_TRUE(declaration.getTransactionSummaryAttr());
      EXPECT_FALSE(declaration.getDependencySummaryAttr());
    } else {
      EXPECT_EQ(declaration.getSemanticKind(), "hardware");
      EXPECT_TRUE(declaration.getDependencySummaryAttr());
      EXPECT_FALSE(declaration.getTransactionSummaryAttr());
      EXPECT_TRUE(headers->isTrustedBuiltin(declaration));
    }
  }
  EXPECT_EQ(transactionProviders, 2u);
  EXPECT_TRUE(succeeded(compiler::verifyIntrinsicSourceUnitPair(
      *result->body, *result->interface, error()))) << diagnostics;
  diagnostics.clear();
  EXPECT_TRUE(failed(ac::verifyHardwarePackage(*result->body)));
  EXPECT_NE(StringRef(diagnostics).lower().find("requires final hardware"),
            std::string::npos) << diagnostics;
}

TEST_F(SourceTransactionCaptureTest, TypedPublicationDoesNotRelabelSourceImportAsHardware) {
  auto captured = capture("design.py", counterSource());
  ASSERT_TRUE(captured);
  std::string before;
  llvm::raw_string_ostream stream(before);
  captured->print(stream);
  auto headers = compiler::SourceHeaderRegistry::create(&context, {}, error());
  ASSERT_TRUE(succeeded(headers));
  diagnostics.clear();
  auto result = compiler::compilePythonSourceUnit(*captured, owner("design.py"),
                                                *headers, error(), /*retainSourceImport=*/true);
  EXPECT_TRUE(failed(result));
  EXPECT_NE(StringRef(diagnostics).lower().find("hardware"), std::string::npos) << diagnostics;
  std::string after;
  llvm::raw_string_ostream output(after);
  captured->print(output);
  EXPECT_EQ(before, after);
}

TEST_F(SourceTransactionCaptureTest, NativePairRederivesEveryPublishedMethodProviderFact) {
  auto captured = capture("design.py", counterSource());
  ASSERT_TRUE(captured);
  auto headers = compiler::SourceHeaderRegistry::create(&context, {}, error());
  ASSERT_TRUE(succeeded(headers));
  auto compiled = compiler::compilePythonSourceUnit(*captured, owner("design.py"), *headers, error());
  ASSERT_TRUE(succeeded(compiled)) << diagnostics;
  Builder b(&context);
  for (unsigned mutation = 0; mutation != 8; ++mutation) {
    SCOPED_TRACE(mutation);
    OwningOpRef<ModuleOp> header(cast<ModuleOp>((*compiled->interface)->clone()));
    ac::ModuleImportOp provider;
    for (auto declaration : header->getOps<ac::ModuleImportOp>())
      if (declaration.getSymName().ends_with("Counter")) provider = declaration;
    ASSERT_TRUE(provider);
    auto summary = provider.getTransactionSummaryAttr();
    ASSERT_TRUE(summary);
    auto states = summary.getAs<ArrayAttr>("owners");
    ASSERT_FALSE(states.empty());
    auto state = cast<DictionaryAttr>(states[0]);
    auto methods = summary.getAs<ArrayAttr>("methods");
    unsigned addIndex = rowIndex(summary, "methods", "add");
    unsigned relayIndex = rowIndex(summary, "methods", "relay");
    auto add = cast<DictionaryAttr>(methods[addIndex]);
    auto relay = cast<DictionaryAttr>(methods[relayIndex]);
    switch (mutation) {
    case 0:
      summary = changedRow(summary, "owners", 0, changed(state, "name", b.getStringAttr("different")));
      break;
    case 1: {
      auto domain = state.getAs<DictionaryAttr>("domain");
      auto endpoint = b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("constant")),
                                           b.getNamedAttr("value", b.getBoolAttr(true))});
      auto changedDomain = changed(domain, "clock", endpoint);
      if (changedDomain == domain) changedDomain = changed(domain, "reset", endpoint);
      ASSERT_NE(changedDomain, domain);
      summary = changedRow(summary, "owners", 0, changed(state, "domain", changedDomain));
      break;
    }
    case 2: {
      auto effects = add.getAs<ArrayAttr>("body_effects");
      ASSERT_GT(effects.size(), 1u);
      SmallVector<Attribute> reordered(effects.begin(), effects.end());
      std::swap(reordered[0], reordered[1]);
      summary = changedRow(summary, "methods", addIndex, changed(add, "body_effects", b.getArrayAttr(reordered)));
      break;
    }
    case 3: {
      auto effect = cast<DictionaryAttr>(add.getAs<ArrayAttr>("body_effects")[0]);
      auto path = b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("constant")),
                                       b.getNamedAttr("value", b.getBoolAttr(false))});
      add = changedRow(add, "body_effects", 0, changed(effect, "path", path));
      summary = changedRow(summary, "methods", addIndex, add);
      break;
    }
    case 4: {
      auto guard = add.getAs<DictionaryAttr>("guard");
      auto fact = b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("unknown"))});
      summary = changedRow(summary, "methods", addIndex, changed(add, "guard", changed(guard, "fact", fact)));
      break;
    }
    case 5: {
      auto result = cast<DictionaryAttr>(add.getAs<ArrayAttr>("results")[0]);
      auto dependencies = b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("known")),
                                               b.getNamedAttr("arguments", b.getArrayAttr({})),
                                               b.getNamedAttr("captures", b.getArrayAttr({}))});
      add = changedRow(add, "results", 0, changed(result, "dependencies", dependencies));
      summary = changedRow(summary, "methods", addIndex, add);
      break;
    }
    case 6: {
      auto call = cast<DictionaryAttr>(relay.getAs<ArrayAttr>("body_effects")[0]);
      call = changed(call, "callee", SymbolRefAttr::get(&context, provider.getSymName(), {FlatSymbolRefAttr::get(&context, "read")}));
      call = changed(call, "arguments", b.getArrayAttr({}));
      relay = changedRow(relay, "body_effects", 0, call);
      summary = changedRow(summary, "methods", relayIndex, relay);
      break;
    }
    case 7: {
      auto parameter = cast<DictionaryAttr>(add.getAs<ArrayAttr>("ac.parameters")[0]);
      auto constraint = parameter.getAs<DictionaryAttr>("constraint");
      constraint = changed(constraint, "source_kind", b.getStringAttr("integer"));
      constraint = changed(constraint, "domain", parseAttribute("#ac.source_domain<{kind = \"integer\", lower = #ac.math_int<0>, upper = #ac.math_int<256>}>", &context));
      parameter = changed(parameter, "constraint", constraint);
      add = changedRow(add, "ac.parameters", 0, parameter);
      summary = changedRow(summary, "methods", addIndex, add);
      break;
    }
    }
    ASSERT_NE(summary, provider.getTransactionSummaryAttr());
    provider->setAttr("transaction_summary", summary);
    diagnostics.clear();
    ASSERT_TRUE(succeeded(mlir::verify(*header))) << diagnostics;
    // There is no receipt/hash wrapper here: invoke the actual native pair
    // entry with the unchanged provider body and the forged declaration.
    EXPECT_TRUE(failed(compiler::verifyIntrinsicSourceUnitPair(*compiled->body, *header, error())));
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(SourceTransactionCaptureTest, OwningTransactionDeclarationsCannotReplaceTheirProviderBodies) {
  auto captured = capture("design.py", counterSource());
  ASSERT_TRUE(captured);
  auto headers = compiler::SourceHeaderRegistry::create(&context, {}, error());
  ASSERT_TRUE(succeeded(headers));
  auto compiled = compiler::compilePythonSourceUnit(*captured, owner("design.py"), *headers, error());
  ASSERT_TRUE(succeeded(compiled)) << diagnostics;
  auto registry = compiler::SourceHeaderRegistry::create(&context, {*compiled->interface}, error());
  ASSERT_TRUE(succeeded(registry)) << diagnostics;
  for (bool snapshots : {false, true}) {
    SCOPED_TRACE(snapshots);
    OwningOpRef<ModuleOp> missing(cast<ModuleOp>((*compiled->body)->clone()));
    for (auto definition : llvm::make_early_inc_range(missing->getOps<ac::ModuleOp>())) {
      if (snapshots) {
        auto declaration = cast<ac::ModuleImportOp>(SymbolTable::lookupSymbolIn(*compiled->interface, definition.getSymName()));
        auto *copy = declaration->clone();
        copy->setAttr("ac.declaration_role", Builder(&context).getStringAttr("import_snapshot"));
        definition->getBlock()->getOperations().insert(definition->getIterator(), copy);
      }
      definition->erase();
    }
    (*missing)->setAttr("ac.unit_kind", Builder(&context).getStringAttr("declarations"));
    diagnostics.clear();
    EXPECT_TRUE(failed(compiler::verifyIntrinsicSourceUnitPair(*missing, *compiled->interface, error())));
    EXPECT_FALSE(diagnostics.empty());
    diagnostics.clear();
    EXPECT_TRUE(failed(registry->verifyBodySnapshots(*missing, *compiled->interface, error())));
    EXPECT_FALSE(diagnostics.empty());
  }
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
  auto calls = direct<ac::SourceMethodCallOp>(executionBody(transactions[0]));
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
       GuardedPayloadUsesCompilerManagedAvailability) {
  auto sourceText = replaceOnce(
      counterSource(), "@ac.method\n    def add",
      "@ac.method(guard=lambda delta: count >= delta)\n    def add");
  managedResult(sourceText);
  managedResult(replaceOnce(sourceText, "after = left.relay(delta)",
                     "after = left.add(delta)"),
         false);
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
  managedResult(childChain);
  managedResult(replaceOnce(counterSource(), "@ac.rule\n    def advance",
                     "@ac.rule(guard=lambda: delta != 0)\n    def advance"),
         false);
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

TEST_F(SourceTransactionCaptureTest,
       MethodChecksAndObservationsPreserveSourceOrderAndSnapshots) {
  auto file = typed(R"py(import pycircuit as ac
@ac.module
def Meter():
    count: ac.u8 = 0
    @ac.method
    def read() -> ac.u8:
        return count
    @ac.method
    def add(delta: ac.u8) -> ac.u8:
        count = count + delta
        return count
    @ac.method
    def instrument(delta: ac.u8) -> ac.u8:
        before = read()
        assert before != delta
        ac.log("info", "step", "before", before, add(delta))
        assert before != delta, ""
        ac.log("debug", "literal", "fixed")
        ac.report("after", count)
        return before
    @ac.rule
    def run() -> None:
        unused = instrument(1)
    run()
)py");
  ASSERT_TRUE(file) << diagnostics;
  ac::SourceMethodOp instrument;
  for (auto method : operations<ac::SourceMethodOp>(*file))
    if (method.getSymName() == "instrument")
      instrument = method;
  ASSERT_TRUE(instrument);

  auto calls = direct<ac::SourceMethodCallOp>(instrument.getBody());
  auto checks = direct<ac::SourceExpectOp>(instrument.getBody());
  auto observations = direct<ac::SourceObserveOp>(instrument.getBody());
  ASSERT_EQ(calls.size(), 2u);
  ASSERT_EQ(checks.size(), 2u);
  ASSERT_EQ(observations.size(), 3u);
  EXPECT_TRUE(calls[0]->isBeforeInBlock(checks[0]));
  EXPECT_TRUE(checks[0]->isBeforeInBlock(calls[1]));
  EXPECT_TRUE(calls[1]->isBeforeInBlock(observations[0]));
  EXPECT_TRUE(observations[0]->isBeforeInBlock(checks[1]));
  EXPECT_TRUE(checks[1]->isBeforeInBlock(observations[1]));
  EXPECT_TRUE(observations[1]->isBeforeInBlock(observations[2]));
  EXPECT_EQ(calls[0].getCalleeAttr().getLeafReference(), "read");
  EXPECT_EQ(calls[1].getCalleeAttr().getLeafReference(), "add");
  EXPECT_NE(calls[1].getPath(), checks[0].getCondition());

  EXPECT_FALSE(checks[0]->hasAttr("ac.message"));
  auto empty = checks[1]->getAttrOfType<StringAttr>("ac.message");
  ASSERT_TRUE(empty);
  EXPECT_TRUE(empty.getValue().empty());
  for (auto check : checks) {
    EXPECT_EQ(check.getKind(), "assert");
    EXPECT_TRUE(check.getSampling().empty());
    EXPECT_TRUE(check->hasAttr("occurrence"));
    EXPECT_FALSE(check->hasAttr("ac.check_id"));
    EXPECT_TRUE(check.getPath().getDefiningOp<ac::BitsConstantOp>());
  }

  EXPECT_EQ(observations[0].getKind(), "log");
  EXPECT_EQ(observations[0].getValues().size(), 2u);
  EXPECT_EQ(observations[1].getKind(), "log");
  EXPECT_TRUE(observations[1].getValues().empty());
  EXPECT_EQ(observations[2].getKind(), "report");
  EXPECT_EQ(observations[2].getValues().size(), 1u);
  for (auto observe : observations) {
    EXPECT_TRUE(observe->hasAttr("occurrence"));
    EXPECT_TRUE(observe->hasAttr("ac.location"));
    EXPECT_FALSE(observe->hasAttr("ac.observation_id"));
    EXPECT_TRUE(observe.getPath().getDefiningOp<ac::BitsConstantOp>());
  }

  // The first value remains the SSA captured before add(delta). Later source
  // reads cannot retroactively replace that operand with the candidate state.
  EXPECT_EQ(observations[0].getValues()[0], calls[0].getResult(0));
  EXPECT_EQ(observations[0].getValues()[1], calls[1].getResult(0));
  EXPECT_NE(observations[2].getValues()[0], observations[0].getValues()[0]);
}

TEST_F(SourceTransactionCaptureTest,
       BareRuleConditionalObservationsSelectTypedTransactionsThroughIfArms) {
  const char *source = R"py(import pycircuit as ac
from pycircuit import report as sample, log as record
@ac.module
def Top(flag: ac.u1, value: ac.u8):
    @ac.rule
    def inspect() -> None:
        BODY
    inspect()
)py";
  const std::pair<StringRef, unsigned> cases[] = {
      {"if flag:\n            ac.report(\"selected\", value)", 1},
      {"if False:\n            ac.log(\"info\", \"dead\", value)", 1},
      {"if flag:\n            record(\"info\", \"outer\", value)\n"
       "        elif value == 0:\n            sample(\"zero\", value)\n"
       "        else:\n            if value == 1:\n"
       "                ac.report(\"one\", value)", 3}};
  for (auto [body, branches] : cases) {
    SCOPED_TRACE(body.str());
    auto file = typed(replaceOnce(source, "BODY", body));
    ASSERT_TRUE(file) << diagnostics;
    EXPECT_EQ(operations<ac::SourceMethodOp>(*file).size(), 1u);
    auto transactions = operations<ac::SourceTransactionOp>(*file);
    ASSERT_EQ(transactions.size(), 1u);
    EXPECT_EQ(transactions.front().getName(), "inspect");
    EXPECT_EQ(operations<ac::SourceBranchOp>(*file).size(), branches);
    auto observations = operations<ac::SourceObserveOp>(*file);
    ASSERT_EQ(observations.size(), branches);
    for (auto observation : observations) {
      EXPECT_TRUE(observation->getParentOfType<ac::SourceBranchOp>());
      EXPECT_EQ(observation->getParentOfType<ac::SourceMethodOp>(),
                entryMethod(transactions.front()));
      EXPECT_TRUE(observation.getPath().getDefiningOp<ac::BitsConstantOp>());
      EXPECT_TRUE(observation->hasAttr("occurrence"));
      EXPECT_TRUE(observation->hasAttr("ac.location"));
      EXPECT_EQ(observation.getValues().size(), 1u);
    }
  }
}

TEST_F(SourceTransactionCaptureTest,
       SameNameAndShadowedCallsCannotSeedIntrinsicObservationTransactions) {
  const char *source = R"py(import pycircuit as ac
IMPORT
@ac.module
def Top(flag: ac.u1, value: ac.u8):
    DECLARATION
    @ac.rule
    def inspect() -> None:
        if flag:
            CALL
    inspect()
)py";
  for (auto [declaration, call] :
       {std::pair<StringRef, StringRef>(
            "def report(arg: ac.u8) -> ac.u8:\n        return arg", "report(value)"),
        {"def log(arg: ac.u8) -> ac.u8:\n        return arg", "log(value)"},
        {"def sample(arg: ac.u8) -> ac.u8:\n        return arg", "sample(value)"}}) {
    auto text = replaceOnce(source, "IMPORT",
                            call.starts_with("sample")
                                ? "from pycircuit import report as sample"
                                : "");
    text = replaceOnce(text, "DECLARATION", declaration);
    text = replaceOnce(text, "CALL", call);
    auto captured = capture("nonintrinsic.py", text);
    ASSERT_TRUE(captured);
    diagnostics.clear();
    auto status = lower(*captured, "nonintrinsic.py");
    if (call.starts_with("sample")) {
      EXPECT_TRUE(failed(status)); // The imported alias cannot be rebound.
      EXPECT_FALSE(diagnostics.empty());
    } else {
      ASSERT_TRUE(succeeded(status)) << diagnostics;
      EXPECT_EQ(operations<ac::SourceTransactionOp>(*captured).size(), 1u);
      EXPECT_EQ(operations<ac::SourceBranchOp>(*captured).size(), 1u);
      EXPECT_TRUE(operations<func::CallOp>(*captured).empty());
    }
    EXPECT_TRUE(operations<ac::SourceObserveOp>(*captured).empty());
    // A same-spelled helper is never an observation intrinsic.
    EXPECT_EQ(diagnostics.find("report requires"), std::string::npos);
    EXPECT_EQ(diagnostics.find("log requires"), std::string::npos);
  }
}

TEST_F(SourceTransactionCaptureTest,
       BareConditionalObservationDoesNotAdmitExpressionSelectorOrHelperEffects) {
  const char *source = R"py(import pycircuit as ac
@ac.module
def Top(flag: ac.u1, value: ac.u8):
    def helper(arg: ac.u8) -> ac.u8:
        HELPER
    @ac.rule
    def inspect() -> None:
        if flag:
            ac.report("selected", value)
        BODY
    inspect()
)py";
  for (StringRef body : {
           "saved = ac.report(\"value\", value)",
           "saved = ac.log(\"info\", \"value\", value) if flag else value",
           "if ac.report(\"selector\", value):\n            pass"}) {
    auto text = replaceOnce(source, "HELPER", "return arg");
    auto captured = capture("effect_placement.py", replaceOnce(text, "BODY", body));
    ASSERT_TRUE(captured);
    diagnostics.clear();
    EXPECT_TRUE(failed(lower(*captured, "effect_placement.py")));
    EXPECT_FALSE(diagnostics.empty());
  }
  auto helper = replaceOnce(source, "HELPER", "return ac.report(\"hidden\", arg)");
  helper = replaceOnce(helper, "BODY", "saved = helper(value)");
  auto captured = capture("helper_effect.py", helper);
  ASSERT_TRUE(captured);
  diagnostics.clear();
  EXPECT_TRUE(failed(lower(*captured, "helper_effect.py")));
  EXPECT_FALSE(diagnostics.empty());
  auto guarded = replaceOnce(source, "HELPER", "return arg");
  guarded = replaceOnce(guarded, "BODY", "pass");
  guarded = replaceOnce(guarded, "@ac.rule",
                        "@ac.rule(guard=lambda: ac.report(\"guard\", value))");
  reject(guarded, "guard");
}

TEST_F(SourceTransactionCaptureTest,
       AggregateIfMatchAndObservationsUseSharedTransactionBody) {
  const char *source = R"py(import pycircuit as ac
@ac.struct
class Pair:
    left: ac.u8
    right: ac.u8
@ac.struct
class View:
    value: ac.u8
@ac.module
def Top(flag: ac.u1, entries: ac.table[2, ac.u8]) -> View:
    @ac.rule
    def inspect() -> View:
        pair = Pair(left=entries[0], right=entries[1])
        if flag:
            pair = Pair(left=pair.left + 1, right=pair.right)
            assert pair.right == entries[1], "table payload snapshot"
        else:
            assert pair.left == entries[0]
        match flag:
            case 0:
                assert entries[0] == entries[0]
            case _:
                assert pair.right == entries[1]
        OBSERVATION
        return View(value=pair.left)
    return inspect()
)py";
  for (StringRef observation : {"pass", "ac.report(\"value\", entries[0])",
                               "ac.log(\"info\", \"value\", entries[0])"}) {
    SCOPED_TRACE(observation.str());
    auto file = typed(replaceOnce(source, "OBSERVATION", observation));
    ASSERT_TRUE(file) << diagnostics;
    auto transactions = operations<ac::SourceTransactionOp>(*file);
    ASSERT_EQ(transactions.size(), 1u);
    EXPECT_TRUE(entryMethod(transactions.front()));
    EXPECT_EQ(operations<ac::SourceBranchOp>(*file).size(), 2u);
    EXPECT_TRUE(operations<ac::RuleOp>(*file).empty());
    compilerAvailability(transactions.front());
    EXPECT_EQ(operations<ac::SourceExpectOp>(*file).size(), 4u);
    EXPECT_EQ(operations<ac::SourceObserveOp>(*file).size(),
              observation == "pass" ? 0u : 1u);
  }
}

TEST_F(SourceTransactionCaptureTest,
       MethodInstrumentationRejectsDynamicMetadataAndMalformedSpecs) {
  const char *base = R"py(import pycircuit as ac
@ac.module
def Meter():
    count: ac.u8 = 0
    @ac.method
    def sample(delta: ac.u8) -> ac.u8:
        BODY
        return count
    @ac.rule
    def run() -> None:
        unused = sample(1)
    run()
)py";
  for (auto [body, category] :
       {std::pair<StringRef, StringRef>("assert count != delta, delta",
                                       "message"),
        {"ac.log(\"verbose\", \"event\", count)", "log"},
        {"ac.log(\"info\", \"\", count)", "log"},
        {"ac.report(\"\", count)", "report"},
        {"ac.report(\"name\", count, delta)", "report"}}) {
    reject(replaceOnce(base, "BODY", body), category);
  }
}

TEST_F(SourceTransactionCaptureTest,
       InvalidObservationMetadataRejectsBeforePayloadCallsAreLowered) {
  auto captured = capture("invalid_observation.py", R"py(import pycircuit as ac
@ac.module
def Meter():
    count: ac.u8 = 0
    @ac.method
    def add(delta: ac.u8) -> ac.u8:
        count = count + delta
        return count
    @ac.method
    def sample(delta: ac.u8) -> ac.u8:
        ac.log("verbose", "event", add(delta))
        return count
    @ac.rule
    def run() -> None:
        unused = sample(1)
    run()
)py");
  ASSERT_TRUE(captured);
  diagnostics.clear();
  EXPECT_TRUE(failed(lower(*captured, "invalid_observation.py")));
  EXPECT_NE(StringRef(diagnostics).lower().find("log"), std::string::npos)
      << diagnostics;
  EXPECT_TRUE(operations<ac::SourceMethodCallOp>(*captured).empty());
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

TEST_F(SourceTransactionCaptureTest,
       RuleDeclarationsHaveDistinctOuterRegistrationsAndNestedInvocations) {
  auto repeated = typed(replaceOnce(counterSource(), "    return advance()",
                                    "    first = advance()\n    return advance()"));
  ASSERT_TRUE(repeated) << diagnostics;
  auto registrations = operations<ac::SourceTransactionOp>(*repeated);
  ASSERT_EQ(registrations.size(), 2u);
  EXPECT_EQ(registrations[0].getDeclaration(), registrations[1].getDeclaration());
  EXPECT_NE(registrations[0].getOccurrence(), registrations[1].getOccurrence());
  // These are distinct outer transactions. Shared-writer scheduling is checked
  // later by the state/EHR analysis, not by forbidding repeated declarations.
  reject(replaceOnce(counterSource(), "    return advance()", "    pass"),
         "return");
  auto nested =
      replaceOnce(counterSource(), "        before = left.read()",
                  "        inner()\n        before = left.read()");
  nested = replaceOnce(nested, "    @ac.rule\n    def advance",
                       "    @ac.rule\n    def inner() -> None:\n        pass\n "
                       "   @ac.rule\n    def advance");
  auto called = typed(nested);
  ASSERT_TRUE(called) << diagnostics;
  auto outer = operations<ac::SourceTransactionOp>(*called);
  ASSERT_EQ(outer.size(), 1u);
  unsigned innerCalls = 0;
  executionBody(outer[0]).walk([&](ac::SourceMethodCallOp call) {
    innerCalls += call.getCalleeAttr().getLeafReference() == "inner";
  });
  EXPECT_EQ(innerCalls, 1u);
  reject(replaceOnce(nested, "        inner()", "        ignored = inner()"),
         "result");
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

TEST_F(SourceTransactionCaptureTest, EffectfulStatementBranchesRetainActualCalls) {
  auto file = typed(replaceOnce(
      counterSource(), "        after = left.relay(delta)",
      "        if delta != 0:\n            after = left.relay(delta)\n"
      "        else:\n            after = before"));
  ASSERT_TRUE(file) << diagnostics;
  auto branches = operations<ac::SourceBranchOp>(*file);
  ASSERT_EQ(branches.size(), 1u);
  auto calls = direct<ac::SourceMethodCallOp>(branches[0].getThenRegion());
  ASSERT_EQ(calls.size(), 1u);
  EXPECT_EQ(calls[0].getCalleeAttr().getLeafReference(), "relay");
  EXPECT_TRUE(direct<ac::SourceMethodCallOp>(branches[0].getElseRegion()).empty());
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(transactions.size(), 1u);
  compilerAvailability(transactions.front());
}

TEST_F(SourceTransactionCaptureTest,
       FiniteEffectfulForPreservesOuterTransactionAndIterationIdentity) {
  auto file = typed(replaceOnce(
      counterSource(), "        after = left.relay(delta)",
      "        after = before\n        for index in range(2):\n"
      "            after = left.relay(delta)"));
  ASSERT_TRUE(file) << diagnostics;
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(transactions.size(), 1u);
  SmallVector<ac::SourceMethodCallOp> calls;
  executionBody(transactions[0]).walk([&](ac::SourceMethodCallOp call) {
    if (call.getCalleeAttr().getLeafReference() == "relay")
      calls.push_back(call);
  });
  ASSERT_EQ(calls.size(), 2u);
  ASSERT_EQ(calls[0]->getBlock(), calls[1]->getBlock());
  EXPECT_TRUE(calls[0]->isBeforeInBlock(calls[1]));
  EXPECT_NE(calls[0].getOccurrence(), calls[1].getOccurrence());
  for (auto call : calls) {
    auto expansion = call.getOccurrence().getAs<ArrayAttr>("expansion");
    ASSERT_TRUE(expansion);
    EXPECT_FALSE(expansion.empty());
  }
  file->walk([&](Operation *operation) {
    EXPECT_NE(operation->getName().getDialectNamespace(), "scf");
  });
}

TEST_F(SourceTransactionCaptureTest,
       StatementBranchesPreserveFixedBooleanElifPassAndMissingElse) {
  auto file = typed(R"py(import pycircuit as ac
@ac.module
def Owner():
    count: ac.u8 = 5
    @ac.method
    def choose(fixed: ac.u1, logical: bool) -> None:
        if fixed:
            count = count + 1
        if logical:
            count = count + 2
        elif fixed:
            pass
        else:
            count = count + 3
    @ac.rule
    def step() -> None:
        choose(0, True)
    step()
)py",
                    "statement_branches.py");
  ASSERT_TRUE(file);
  auto branches = operations<ac::SourceBranchOp>(*file);
  ASSERT_EQ(branches.size(), 3u);
  EXPECT_EQ(operations<ac::SourceStateWriteOp>(*file).size(), 3u);
  ac::HardwareAnalysis hardware(*file);
  for (auto branch : branches) {
    ac::HardwareBindings bindings;
    bindings.owner = branch->getParentOfType<ac::ModuleOp>();
    auto width = hardware.getPackedWidth(branch.getCondition().getType(),
                                         bindings, branch);
    ASSERT_TRUE(succeeded(width));
    EXPECT_EQ(*width, 1u);
    EXPECT_TRUE(branch.getConditionLocation());
    EXPECT_TRUE(branch.getOccurrence());
    ASSERT_TRUE(branch.getThenRegion().hasOneBlock());
    ASSERT_TRUE(branch.getElseRegion().hasOneBlock());
    EXPECT_TRUE(
        isa<ac::YieldOp>(branch.getThenRegion().front().getTerminator()));
    EXPECT_TRUE(
        isa<ac::YieldOp>(branch.getElseRegion().front().getTerminator()));
  }
  unsigned emptyArms = 0;
  for (auto branch : branches)
    for (Region *arm : {&branch.getThenRegion(), &branch.getElseRegion()})
      if (arm->front().getOperations().size() == 1)
        ++emptyArms;
  EXPECT_EQ(emptyArms, 2u) << "pass and the missing else remain explicit arms";
}

TEST_F(SourceTransactionCaptureTest,
       ScalarBranchJoinsUseCompleteIntegerRepresentationsAndAnnotations) {
  auto file = typed(R"py(import pycircuit as ac
from typing import Annotated
@ac.module
def Owner():
    narrow: ac.u8 = 0
    wide: ac.bits[72] = 0
    @ac.method
    def choose(fixed: ac.u1, logical: bool) -> None:
        if logical:
            signed = 0 - 1
        else:
            signed = 2
        narrow = signed + 2
        if fixed:
            large = 1 << 70
        else:
            large = 3
        wide = large
        bounded: Annotated[int, range(1 << 3)] = 0
        if fixed:
            bounded = 7
        else:
            bounded = 1
        narrow = bounded
    @ac.rule
    def step() -> None:
        choose(0, True)
    step()
)py",
                    "numeric_branch_joins.py");
  ASSERT_TRUE(file);
  auto branches = operations<ac::SourceBranchOp>(*file);
  ASSERT_EQ(branches.size(), 3u);
  bool sawSmall = false, sawWide = false;
  ac::HardwareAnalysis hardware(*file);
  for (auto branch : branches) {
    ASSERT_EQ(branch.getNumResults(), 1u);
    ac::HardwareBindings bindings;
    bindings.owner = branch->getParentOfType<ac::ModuleOp>();
    auto width = hardware.getPackedWidth(branch.getResult(0).getType(),
                                         bindings, branch);
    ASSERT_TRUE(succeeded(width));
    sawSmall |= *width <= 8;
    sawWide |= *width >= 71;
  }
  EXPECT_TRUE(sawSmall);
  EXPECT_TRUE(sawWide);
  EXPECT_EQ(operations<ac::SourceStateWriteOp>(*file).size(), 3u);

  auto overflow =
      capture("numeric_branch_overflow.py", R"py(import pycircuit as ac
from typing import Annotated
@ac.module
def Owner():
    count: ac.u8 = 0
    @ac.method
    def choose(flag: ac.u1) -> None:
        local: Annotated[int, range(1 << 3)] = 0
        if flag:
            local = 8
        else:
            local = 1
        count = local
    @ac.rule
    def step() -> None:
        choose(0)
    step()
)py");
  ASSERT_TRUE(overflow);
  diagnostics.clear();
  EXPECT_TRUE(failed(lower(*overflow, "numeric_branch_overflow.py")));
  EXPECT_NE(StringRef(diagnostics).lower().find("bound"), std::string::npos)
      << diagnostics;
}

TEST_F(SourceTransactionCaptureTest,
       BranchLocalsRequireDefiniteAssignmentAndNominalEscape) {
  reject(R"py(import pycircuit as ac
@ac.module
def Owner():
    count: ac.u8 = 0
    @ac.method
    def choose(flag: ac.u1) -> None:
        if flag:
            selected = count
        else:
            pass
        count = selected
    @ac.rule
    def step() -> None:
        choose(0)
    step()
)py",
         "selected");

  auto joined = typed(R"py(import pycircuit as ac
@ac.struct
class Packet:
    value: ac.u8
@ac.module
def Owner():
    count: ac.u8 = 0
    @ac.method
    def choose(flag: ac.u1) -> None:
        packet = Packet(value=count)
        if flag:
            packet = Packet(value=count + 1)
        else:
            pass
        count = packet.value
    @ac.rule
    def step() -> None:
        choose(0)
    step()
)py",
                      "nominal_escape.py");
  ASSERT_TRUE(joined);
  auto joins = operations<ac::SourceBranchOp>(*joined);
  ASSERT_EQ(joins.size(), 1u);
  ASSERT_EQ(joins[0].getNumResults(), 1u);
  EXPECT_TRUE(isa<ac::StructType>(joins[0].getResult(0).getType()));
}

TEST_F(SourceTransactionCaptureTest,
       SelectorsConditionalExpressionsAndEarlyReturnsRemainClosed) {
  constexpr StringLiteral sourceText = R"py(import pycircuit as ac
@ac.module
def Owner():
    count: ac.u8 = 0
    @ac.method
    def predicate() -> ac.u1:
        return count[:1]
    @ac.method
    def update() -> None:
        count = count + 1
    @ac.rule
    def step() -> None:
        BODY
    step()
)py";
  const std::pair<StringRef, StringRef> cases[] = {
      {"if predicate():\n            pass", "branch"},
      {"value = 1 if predicate() else 0", "branch"},
      {"if False:\n            return", "return"}};
  for (auto [body, category] : cases)
    reject(replaceOnce(sourceText.str(), "BODY", body), category);
}

TEST_F(SourceTransactionCaptureTest,
       DeadBranchFiniteLoopRetainsCallsAndIterationIdentity) {
  auto file = typed(R"py(import pycircuit as ac
@ac.module
def Owner():
    count: ac.u8 = 0
    @ac.rule
    def update() -> None:
        count = count + 1
    @ac.rule
    def step() -> None:
        if False:
            for value in range(2):
                update()
    step()
)py", "dead_loop.py");
  ASSERT_TRUE(file) << diagnostics;
  auto branches = operations<ac::SourceBranchOp>(*file);
  ASSERT_FALSE(branches.empty());
  for (auto branch : branches) {
    SmallVector<ac::SourceMethodCallOp> calls;
    branch->getRegion(0).walk(
        [&](ac::SourceMethodCallOp call) { calls.push_back(call); });
    ASSERT_EQ(calls.size(), 2u);
    EXPECT_NE(calls[0].getOccurrence(), calls[1].getOccurrence());
    for (auto call : calls) {
      auto expansion = call.getOccurrence().getAs<ArrayAttr>("expansion");
      ASSERT_TRUE(expansion);
      EXPECT_FALSE(expansion.empty());
    }
  }
}

TEST_F(SourceTransactionCaptureTest,
       ConditionalStatementsRetainDeadAndDynamicArmCallAndEffectInventories) {
  for (StringRef selector : {"False", "True", "flag"}) {
    auto source = R"py(import pycircuit as ac
@ac.module
def Owner():
    count: ac.u8 = 5
    @ac.method
    def update(amount: ac.u8) -> ac.u8:
        count = count + amount
        return count
    @ac.method
    def choose(flag: ac.u1) -> None:
        saved = update(1)
        if SELECTOR:
            selected = update(2)
            assert saved != 0
            ac.log("info", "then", selected)
        else:
            selected = update(4)
            ac.report("else", selected)
        count = count + 3
@ac.module
def Parent(flag: ac.u1):
    owner = Owner()
    @ac.rule
    def step() -> None:
        owner.choose(flag)
    step()
)py";
    auto file = typed(replaceOnce(source, "SELECTOR", selector));
    ASSERT_TRUE(file) << diagnostics;
    auto branches = operations<ac::SourceBranchOp>(*file);
    ASSERT_EQ(branches.size(), 1u);
    EXPECT_EQ(operations<ac::SourceMethodCallOp>(*file).size(), 5u);
    EXPECT_EQ(operations<ac::SourceExpectOp>(*file).size(), 1u);
    EXPECT_EQ(operations<ac::SourceObserveOp>(*file).size(), 2u);
    for (auto &arm : branches.front()->getRegions()) {
      unsigned calls = 0;
      arm.walk([&](ac::SourceMethodCallOp) { ++calls; });
      EXPECT_EQ(calls, 1u);
    }
  }
}

TEST_F(SourceTransactionCaptureTest,
       MethodResultSelectorsRetainUnknownAuthorityWithManagedResults) {
  const char *source = R"py(import pycircuit as ac
@ac.struct
class View:
    value: ac.u8
@ac.module
def Owner():
    count: ac.u8 = 5
    @ac.method
    def predicate() -> ac.u1:
        return 0
    @ac.method
    def update() -> None:
        count = count + 1
@ac.module
def Parent() RESULT:
    owner = Owner()
    @ac.rule
    def step() RETURN:
        selected = owner.predicate()
        if selected:
            owner.update()
        YIELD
    REGISTER
)py";
  auto valued = replaceOnce(source, "RESULT", "-> View");
  valued = replaceOnce(valued, "RETURN", "-> View");
  valued = replaceOnce(valued, "YIELD", "return View(value=1)");
  valued = replaceOnce(valued, "REGISTER", "return step()");
  // Existing source fact/rowFact publication does not infer a Method's body
  // result as a selector literal, even when that body returns zero.
  managedResult(valued);
  auto noResult = replaceOnce(source, "RESULT", "");
  noResult = replaceOnce(noResult, "RETURN", "-> None");
  noResult = replaceOnce(noResult, "YIELD", "pass");
  noResult = replaceOnce(noResult, "REGISTER", "step()");
  auto file = typed(noResult);
  ASSERT_TRUE(file) << diagnostics;
  ac::SourceMethodSummariesAnalysis summaries(file->getOperation());
  ASSERT_TRUE(summaries.isValid());
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(transactions.size(), 1u);
  EXPECT_FALSE(summaries.isTotal(transactions.front()));
}

TEST_F(SourceTransactionCaptureTest,
       DeadArmsStillValidateMethodClosureSignaturesTypesAndRecursion) {
  const char *source = R"py(import pycircuit as ac
@ac.module
def Owner():
    count: ac.u8 = 0
    @ac.method
    def update(amount: ac.u8) -> None:
        count = count + amount
    @ac.rule
    def step() -> None:
        if False:
            CALL
    step()
)py";
  for (auto [call, category] :
       {std::pair<StringRef, StringRef>("missing(1)", "direct Method call"),
        {"update()", "argument"},
        {"update(True)", "type"}})
    reject(replaceOnce(source, "CALL", call), category);
  auto recursive = replaceOnce(source, "count = count + amount", "update(amount)");
  reject(replaceOnce(recursive, "CALL", "update(1)"), "recursive");
}

TEST_F(SourceTransactionCaptureTest,
       PureConditionalExpressionRemainsStraightLineWithAggregateLocals) {
  auto file = typed(R"py(import pycircuit as ac
@ac.struct
class Packet:
    left: ac.u8
    right: ac.u8
@ac.module
def Owner():
    count: ac.u8 = 0
    @ac.method
    def choose(flag: ac.u1) -> None:
        packet = Packet(left=count, right=count + 1)
        selected = packet.left if flag else packet.right
        count = selected
    @ac.rule
    def step() -> None:
        choose(0)
    step()
)py",
                    "pure_select.py");
  ASSERT_TRUE(file);
  EXPECT_TRUE(operations<ac::SourceBranchOp>(*file).empty());
  EXPECT_EQ(operations<ac::BitsSelectOp>(*file).size(), 1u);
  EXPECT_EQ(operations<ac::SourceStateWriteOp>(*file).size(), 1u);
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
  EXPECT_EQ(operations<ac::SourceMethodOp>(*file).size(), 5u);
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(transactions.size(), 1u);
  ASSERT_EQ(transactions[0].getNumResults(), 1u);
  EXPECT_TRUE(isa<ac::StructType>(transactions[0].getResult(0).getType()));
}

TEST_F(SourceTransactionCaptureTest,
       LogicalIntegerMethodResultAndConditionalCallsFailClosed) {
  for (auto [expression, diagnostic] :
       {std::pair<StringRef, StringRef>(
            "not owner.value()", "Boolean source condition, not Integer"),
        {"1 if owner.value() else 0", "Method calls are forbidden in branch tests and conditional expressions"}}) {
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
    EXPECT_NE(diagnostics.find(diagnostic.str()), std::string::npos)
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
  auto file = typed(R"py(import pycircuit as ac
@ac.struct
class View:
    value: ac.u8
@ac.rule(guard=lambda value: value != 0)
def select(value: ac.u8) -> View:
    return View(value=value)
@ac.module
def Parent(value: ac.u8) -> View:
    return select(value)
)py", "guarded.py");
  ASSERT_TRUE(file) << diagnostics;
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(transactions.size(), 1u);
  EXPECT_TRUE(operations<ac::RuleOp>(*file).empty());
  EXPECT_EQ(direct<ac::BitsCompareOp>(executionGuard(transactions.front())).size(), 1u);
  ac::SourceMethodSummariesAnalysis summaries(file->getOperation());
  ASSERT_TRUE(summaries.isValid());
  EXPECT_FALSE(summaries.isTotal(transactions.front()));
  compilerAvailability(transactions.front());
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
  ASSERT_TRUE(succeeded(semantic.run(*file))) << diagnostics;
  ac::SourceMethodSummariesAnalysis summaries(file->getOperation());
  ASSERT_TRUE(summaries.isValid());
  EXPECT_FALSE(summaries.isTotal(transactions.front()));
  compilerAvailability(transactions.front());
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
  auto valued = typed(perCallGuardSource(/*secondValue=*/true));
  ASSERT_TRUE(valued);
  ac::SourceMethodSummariesAnalysis valuedSummaries(valued->getOperation());
  ASSERT_TRUE(valuedSummaries.isValid());
  unsigned valuedTotal = 0, valuedBlocked = 0;
  for (auto transaction : operations<ac::SourceTransactionOp>(*valued)) {
    ASSERT_EQ(transaction.getNumResults(), 1u);
    compilerAvailability(transaction);
    valuedSummaries.isTotal(transaction) ? ++valuedTotal : ++valuedBlocked;
  }
  EXPECT_EQ(valuedTotal, 1u);
  EXPECT_EQ(valuedBlocked, 1u);
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
    managedResult(valued);
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
  managedResult(sourceText);
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
  const std::string fixtures[] = {std::string(integer), wrapping, boolean};
  const char *expected[] = {
      "conditional requires a Boolean source condition, not Integer",
      "integer interval is not proven within destination bounds",
      "mathematical arithmetic requires known Integer kind and finite interval"};
  for (unsigned i = 0; i < 3; ++i) {
    auto captured = capture("authority.py", fixtures[i]);
    ASSERT_TRUE(captured);
    diagnostics.clear();
    // Storage is admitted, but logical Integer/Boolean owners cannot become
    // wrapping fixed Bits or predicates merely because they share a carrier.
    EXPECT_TRUE(failed(lower(*captured, "authority.py")));
    EXPECT_NE(diagnostics.find(expected[i]), std::string::npos)
        << diagnostics;
    EXPECT_EQ(diagnostics.find("requires explicitly fixed Bits annotation"),
              std::string::npos);
    EXPECT_EQ(diagnostics.find("unexpected operation"), std::string::npos);
  }
}

TEST_F(SourceTransactionCaptureTest,
       NamedModuleResultsKeepValueAndCallableInstanceIdentity) {
  const char *source = R"py(import pycircuit as ac
@ac.struct
class Packet:
    data: ac.u8
@ac.module
def Single(value: ac.u8) -> Packet:
    @ac.rule
    def read() -> ac.u8:
        return value + 2
    return Packet(data=value + 1)
@ac.module
def Multiple(value: ac.u8) -> tuple[Packet, ac.u8]:
    @ac.rule
    def read() -> ac.u8:
        return value + 3
    return Packet(data=value + 4), value + 5
@ac.module
def NoOutputs(value: ac.u8) -> None:
    @ac.rule
    def read() -> ac.u8:
        return value + 6
@ac.module
def Parent(value: ac.u8) -> tuple[ac.u8, ac.u8, ac.u8, ac.u8, ac.u8, ac.u8]:
    single = Single(value)
    alias = single
    multiple = Multiple(value)
    packet, extra = multiple
    empty = NoOutputs(value)
    @ac.rule
    def inspect() -> tuple[ac.u8, ac.u8, ac.u8, ac.u8, ac.u8, ac.u8]:
        return single.data, alias.read(), packet.data, extra, multiple.read(), empty.read()
    a, b, c, d, e, f = inspect()
    return a, b, c, d, e, f
)py";
  auto file = typed(source, "handles.py");
  ASSERT_TRUE(file) << diagnostics;
  auto instances = operations<ac::InstanceOp>(*file);
  EXPECT_EQ(instances.size(), 3u);
  auto registrations = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(registrations.size(), 1u);
  llvm::SmallVector<ac::SourceMethodCallOp> calls;
  executionBody(registrations[0]).walk(
      [&](ac::SourceMethodCallOp call) { calls.push_back(call); });
  ASSERT_EQ(calls.size(), 3u);
  llvm::SmallVector<std::string> receivers;
  for (auto call : calls) {
    ASSERT_EQ(call.getInstancePath().size(), 1u);
    receivers.push_back(
        cast<StringAttr>(call.getInstancePath()[0]).getValue().str());
  }
  EXPECT_TRUE(llvm::is_contained(receivers, "single"));
  EXPECT_TRUE(llvm::is_contained(receivers, "multiple"));
  EXPECT_TRUE(llvm::is_contained(receivers, "empty"));
  EXPECT_FALSE(llvm::is_contained(receivers, "alias"));
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
  ASSERT_EQ(methods.size(), 2u);
  auto echo = namedMethod(*file, "echo");
  ASSERT_TRUE(echo);
  ASSERT_EQ(transactions.size(), 1u);
  EXPECT_TRUE(echo.getCaptures().empty());
  auto &guard = echo.getGuard().front();
  auto &body = echo.getBody().front();
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
  managedResult(shadowedLocalFormalSource(/*value=*/false));
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
    for (StringRef name : {"ac.parameters", "ac.result_constraints",
                           "ac.return_form", "ac.guard_location"}) {
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
    auto guardLocation =
        after->getAttrOfType<DictionaryAttr>("ac.guard_location");
    ASSERT_TRUE(guardLocation);
    EXPECT_EQ(guardLocation.size(), 5u);
    EXPECT_EQ(guardLocation.getAs<StringAttr>("path"),
              sourceOwner.getAs<StringAttr>("path"));
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


TEST_F(SourceTransactionCaptureTest, PublishedHelperBodyAndContractRemainBoundToTheirProvider) {
  auto captured = capture("helper_provider.py", R"py(import pycircuit as ac
def identity(value: ac.u8) -> ac.u8:
    return value
)py");
  ASSERT_TRUE(captured);
  auto headers = compiler::SourceHeaderRegistry::create(&context, {}, error());
  ASSERT_TRUE(succeeded(headers));
  auto compiled = compiler::compilePythonSourceUnit(*captured, owner("helper_provider.py"), *headers, error());
  ASSERT_TRUE(succeeded(compiled)) << diagnostics;
  for (unsigned mutation = 0; mutation != 4; ++mutation) {
    OwningOpRef<ModuleOp> header(cast<ModuleOp>((*compiled->interface)->clone()));
    mlir::func::FuncOp helper;
    for (auto candidate : header->getOps<mlir::func::FuncOp>())
      if (candidate->getAttrOfType<StringAttr>("ac.helper_kind") == StringAttr::get(&context, "value")) helper = candidate;
    ASSERT_TRUE(helper);
    Builder b(&context);
    if (mutation == 0) {
      OpBuilder operations(&context);
      operations.setInsertionPoint(helper.getBody().front().getTerminator());
      auto argument = helper.getBody().front().getArgument(0);
      OperationState invert(helper.getLoc(), ac::BitsUnaryOp::getOperationName());
      invert.addOperands(argument);
      invert.addTypes(helper.getFunctionType().getResult(0));
      invert.addAttribute("opcode", b.getStringAttr("not"));
      auto *operation = operations.create(invert);
      helper.getBody().front().getTerminator()->setOperand(0, operation->getResult(0));
      ASSERT_TRUE(succeeded(mlir::verify(*header))) << diagnostics;
    }
    if (mutation == 1) {
      auto byte = cast<ac::BitsType>(helper.getFunctionType().getInput(0));
      auto width = byte.getWidth();
      auto value = width.getTree().getAs<DictionaryAttr>("value");
      ASSERT_TRUE(value);
      auto one = changed(value, "value", parseAttribute("#ac.math_int<1>", &context));
      auto bit = ac::BitsType::get(&context, ac::StaticExprAttr::get(&context, changed(width.getTree(), "value", one)));
      helper.setType(b.getFunctionType({bit}, {bit}));
      helper.getBody().front().getArgument(0).setType(bit);
      auto parameter = cast<DictionaryAttr>(helper->getAttrOfType<ArrayAttr>("ac.parameters")[0]);
      auto constraint = parameter.getAs<DictionaryAttr>("constraint");
      auto bound = changed(constraint, "type", TypeAttr::get(bit));
      helper->setAttr("ac.parameters", b.getArrayAttr({changed(parameter, "constraint", bound)}));
      helper->setAttr("ac.result_constraints", b.getArrayAttr({bound}));
      helper.getBody().getBlocks().clear();
      auto *block = new Block;
      helper.getBody().push_back(block);
      auto input = block->addArgument(bit, helper.getLoc());
      OpBuilder operations(&context);
      operations.setInsertionPointToEnd(block);
      operations.create<mlir::func::ReturnOp>(helper.getLoc(), input);
      ASSERT_TRUE(succeeded(mlir::verify(*header))) << diagnostics;
    }
    if (mutation == 2) helper->setAttr("ac.capture_types", b.getArrayAttr({TypeAttr::get(helper.getFunctionType().getInput(0))}));
    if (mutation == 3) helper->setAttr("ac.source_owner", owner("foreign.py"));
    diagnostics.clear();
    EXPECT_TRUE(failed(compiler::verifyIntrinsicSourceUnitPair(*compiled->body, *header, error())));
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(SourceTransactionCaptureTest, HelperOnlyProviderCannotPublishWithoutItsOwningFunctionBody) {
  auto captured = capture("helper_provider.py", R"py(import pycircuit as ac
def identity(value: ac.u8) -> ac.u8:
    return value
)py");
  ASSERT_TRUE(captured);
  auto headers = compiler::SourceHeaderRegistry::create(&context, {}, error());
  ASSERT_TRUE(succeeded(headers));
  auto compiled = compiler::compilePythonSourceUnit(*captured, owner("helper_provider.py"), *headers, error());
  ASSERT_TRUE(succeeded(compiled)) << diagnostics;
  auto registry = compiler::SourceHeaderRegistry::create(&context, {*compiled->interface}, error());
  ASSERT_TRUE(succeeded(registry)) << diagnostics;
  for (bool snapshot : {false, true}) {
    OwningOpRef<ModuleOp> body(cast<ModuleOp>((*compiled->body)->clone()));
    for (auto helper : llvm::make_early_inc_range(body->getOps<mlir::func::FuncOp>())) {
      if (snapshot) helper->setAttr("ac.declaration_role", Builder(&context).getStringAttr("import_snapshot"));
      else helper->erase();
    }
    diagnostics.clear();
    EXPECT_TRUE(failed(compiler::verifyIntrinsicSourceUnitPair(*body, *compiled->interface, error())));
    EXPECT_FALSE(diagnostics.empty());
    diagnostics.clear();
    EXPECT_TRUE(failed(registry->verifyBodySnapshots(*body, *compiled->interface, error())));
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(SourceTransactionCaptureTest, PureHelpersMaySupplyGuardsButCannotHideEffects) {
  constexpr StringLiteral sourceText = R"py(import pycircuit as ac
@ac.module
def Owner():
    count: ac.u8 = 1
    def predicate(value: ac.u8) -> bool:
        return value != 0
    @ac.method(guard=lambda delta: predicate(delta))
    def guarded(delta: ac.u8) -> None:
        count = delta
    @ac.rule(guard=lambda: predicate(count))
    def inspect() -> None:
        pass
    inspect()
)py";
  auto file = typed(sourceText, "helper_guard.py");
  ASSERT_TRUE(file);
  for (auto method : operations<ac::SourceMethodOp>(*file))
    EXPECT_FALSE(direct<ac::BitsCompareOp>(method.getGuard()).empty());
  for (auto transaction : operations<ac::SourceTransactionOp>(*file))
    EXPECT_FALSE(direct<ac::BitsCompareOp>(executionGuard(transaction)).empty());
  for (StringRef body : {"return ac.report(value)", "return guarded(value)", "allocated = Owner()\n        return True"}) {
    std::string text = sourceText.str();
    auto offset = text.find("return value != 0");
    text.replace(offset, std::string("return value != 0").size(), body.str());
    auto captured = capture("bad_guard.py", text);
    ASSERT_TRUE(captured);
    diagnostics.clear();
    EXPECT_TRUE(failed(lower(*captured, "bad_guard.py")));
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(SourceTransactionCaptureTest,
       CapturedIntegerCrossesFixedInitializerLocalFormalAndOwnerWriteBoundaries) {
  auto file = typed(R"py(import pycircuit as ac
@ac.module
def Owner():
    bias = 3
    count: ac.u8 = bias
    @ac.method
    def put(value: ac.u8) -> ac.u8:
        local: ac.u8 = bias
        count = bias
        count = local
        return value
    @ac.rule
    def step() -> None:
        unused = put(bias)
    step()
)py", "fixed_boundaries.py");
  ASSERT_TRUE(file);
  auto states = operations<ac::SourceStateOp>(*file);
  auto methods = operations<ac::SourceMethodOp>(*file);
  auto writes = operations<ac::SourceStateWriteOp>(*file);
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(transactions.size(), 1u);
  auto calls = direct<ac::SourceMethodCallOp>(executionBody(transactions.front()));
  auto put = namedMethod(*file, "put");
  ASSERT_TRUE(put);
  ASSERT_EQ(states.size(), 1u);
  ASSERT_EQ(methods.size(), 2u);
  ASSERT_EQ(writes.size(), 2u);
  ASSERT_EQ(calls.size(), 1u);
  ac::HardwareAnalysis hardware(*file);
  ac::HardwareBindings bindings;
  bindings.owner = states[0]->getParentOfType<ac::ModuleOp>();
  auto fixedByteBoundary = [&](Type actual, Type expected, Operation *site) {
    ASSERT_TRUE(isa<ac::BitsType>(actual));
    ASSERT_TRUE(isa<ac::BitsType>(expected));
    // Physical widths retain source-linked expression provenance. The native
    // boundary contract compares hardware meaning, not uniqued type handles.
    EXPECT_TRUE(ac::areEquivalentHardwareTypes(actual, expected));
    auto actualWidth = hardware.getPackedWidth(actual, bindings, site);
    auto expectedWidth = hardware.getPackedWidth(expected, bindings, site);
    ASSERT_TRUE(succeeded(actualWidth));
    ASSERT_TRUE(succeeded(expectedWidth));
    EXPECT_EQ(*actualWidth, 8u);
    EXPECT_EQ(*expectedWidth, 8u);
  };
  fixedByteBoundary(states[0].getInitial().getType(),
                    states[0].getQ().getType(), states[0]);
  for (auto write : writes) {
    EXPECT_EQ(write.getState(), states[0].getInstanceName());
    fixedByteBoundary(write.getValue().getType(), states[0].getQ().getType(),
                      write);
  }
  ASSERT_EQ(calls[0].getArguments().size(), 1u);
  fixedByteBoundary(calls[0].getArguments()[0].getType(),
                    put.getFunctionType().getInput(0), calls[0]);
  auto parameters = put->getAttrOfType<ArrayAttr>("ac.parameters");
  auto parameter = cast<DictionaryAttr>(parameters[0]);
  EXPECT_EQ(parameter.getAs<DictionaryAttr>("constraint")
                .getAs<StringAttr>("source_kind")
                .getValue(),
            "fixed_bits");
}

TEST_F(SourceTransactionCaptureTest,
       FixedBindingBoundariesKeepBooleanIntegerAndFixedKindsDistinct) {
  const std::string positive = R"py(import pycircuit as ac
@ac.module
def Owner():
    bias = 3
    count: ac.u8 = bias
    @ac.method
    def put(value: ac.u8) -> ac.u8:
        local: ac.u8 = bias
        count = bias
        return value
    @ac.rule
    def step() -> None:
        unused = put(bias)
    step()
)py";
  const std::string cases[] = {
      replaceOnce(positive, "count: ac.u8 = bias", "count: ac.u8 = True"),
      replaceOnce(positive, "local: ac.u8 = bias", "local: ac.u8 = True"),
      replaceOnce(positive, "count = bias", "count = True"),
      replaceOnce(positive, "put(bias)", "put(True)"),
      replaceOnce(positive, "count: ac.u8 = bias", "count: ac.u8 = 256")};
  for (const auto &sourceText : cases) {
    auto file = capture("wrong_fixed_boundary.py", sourceText);
    ASSERT_TRUE(file);
    diagnostics.clear();
    EXPECT_TRUE(failed(lower(*file, "wrong_fixed_boundary.py")));
    EXPECT_FALSE(diagnostics.empty());
  }
  // Fixed Bits does not become logical Boolean merely because its width is1.
  auto wrongLogicalKind = capture("wrong_logical_kind.py", R"py(import pycircuit as ac
@ac.module
def Owner():
    count: ac.u8 = 0
    @ac.method
    def require_bool(value: bool) -> None:
        pass
    @ac.rule
    def step() -> None:
        value: ac.u1 = 1
        require_bool(value)
    step()
)py");
  ASSERT_TRUE(wrongLogicalKind);
  diagnostics.clear();
  EXPECT_TRUE(failed(lower(*wrongLogicalKind, "wrong_logical_kind.py")));
  EXPECT_FALSE(diagnostics.empty());
  for (StringRef annotation : {StringRef("bool"),
                               StringRef("Annotated[int, range(1 << 1)]")}) {
    auto localKind = capture("wrong_local_kind.py",
        std::string(R"py(import pycircuit as ac
from typing import Annotated
@ac.module
def Owner():
    count: ac.u8 = 0
    @ac.method
    def consume(value: ac.u1) -> None:
        local: )py") + annotation.str() + R"py( = value
    @ac.rule
    def step() -> None:
        actual: ac.u1 = 1
        consume(actual)
    step()
)py");
    ASSERT_TRUE(localKind);
    diagnostics.clear();
    EXPECT_TRUE(failed(lower(*localKind, "wrong_local_kind.py")));
    EXPECT_NE(diagnostics.find("source kind"), std::string::npos) << diagnostics;
  }
}

TEST_F(SourceTransactionCaptureTest,
       LocalNominalFieldReconstructionAndJoinsPreserveSnapshotsAndRootType) {
  auto file = typed(R"py(import pycircuit as ac
from enum import Enum
@ac.encoding(width=3)
class Tag(Enum):
    IDLE = 0
    BUSY = 5
@ac.struct
class Leaf:
    payload: ac.u8
    tag: Tag
@ac.struct
class Packet:
    left: Leaf
    right: Leaf
@ac.module
def Owner():
    count: ac.u8 = 1
    @ac.method
    def choose(flag: ac.u1, incoming: Packet) -> Packet:
        local = incoming
        saved = local
        saved_leaf = local.left
        local.left.payload = count
        local.right = Leaf(payload=7, tag=Tag.IDLE)
        if flag:
            local.left.tag = Tag.BUSY
            if flag:
                local.right.payload = 13
        else:
            local.right.payload = 11
        tag = Tag.IDLE
        if flag:
            tag = Tag.BUSY
        local.right.tag = tag
        count = saved.left.payload + saved_leaf.payload
        return local
    @ac.rule
    def step() -> Packet:
        source = Packet(left=Leaf(payload=2, tag=Tag.IDLE),
                        right=Leaf(payload=3, tag=Tag.BUSY))
        first = choose(1, source)
        second = choose(0, first)
        snapshot = second
        second.left = first.right
        return snapshot
    step()
)py",
                    "nominal_fields.py");
  ASSERT_TRUE(file);
  auto branches = operations<ac::SourceBranchOp>(*file);
  ASSERT_EQ(branches.size(), 3u);
  EXPECT_TRUE(isa<ac::StructType>(branches[0].getResult(0).getType()));
  EXPECT_TRUE(isa<ac::StructType>(branches[1].getResult(0).getType()));
  EXPECT_TRUE(isa<ac::EnumType>(branches[2].getResult(0).getType()));
  EXPECT_EQ(operations<ac::SourceStateOp>(*file).size(), 1u);
  EXPECT_EQ(operations<ac::SourceStateWriteOp>(*file).size(), 1u);
  EXPECT_EQ(operations<ac::SourceMethodCallOp>(*file).size(), 3u);
  EXPECT_TRUE(operations<ac::BitsSelectOp>(*file).empty())
      << "nominal branch hardware must be materialized after source analysis";
}

TEST_F(SourceTransactionCaptureTest,
       LocalNominalAssignmentsRejectForeignTypesAndInvalidTargets) {
  const std::string prefix = R"py(import pycircuit as ac
from enum import Enum
@ac.encoding(width=2)
class Tag(Enum):
    ZERO = 0
    ONE = 1
@ac.encoding(width=2)
class ForeignTag(Enum):
    ZERO = 0
    ONE = 1
@ac.struct
class Leaf:
    data: ac.u8
    tag: Tag
@ac.struct
class ForeignLeaf:
    data: ac.u8
    tag: Tag
@ac.struct
class Packet:
    leaf: Leaf
@ac.module
def Owner():
    count: ac.u8 = 0
    @ac.method
    def choose(flag: ac.u1) -> None:
        packet = Packet(leaf=Leaf(data=1, tag=Tag.ZERO))
)py";
  for (StringRef statement :
       {"packet.leaf = ForeignLeaf(data=2, tag=Tag.ZERO)",
        "packet.leaf.tag = ForeignTag.ONE", "packet.leaf.data = 256",
        "packet.absent = 0", "packet.leaf.data.missing = 0",
        "packet.leaf.data[0] = 1", "packet.leaf.data += 1",
        "packet.leaf.data: ac.u8 = 1", "missing.leaf.data = 1", "Leaf.data = 1",
           "count.data = 1",
           "choose.flag = 1",
           "Tag.ZERO = Tag.ONE",
        "if flag:\n            packet.leaf = ForeignLeaf(data=2, tag=Tag.ZERO)",
        "if flag:\n            created = packet\n        created.leaf.data = "
        "2"}) {
    SCOPED_TRACE(statement.str());
    auto text = prefix + "        " + statement.str() + R"py(
    @ac.rule
    def step() -> None:
        choose(0)
    step()
)py";
    auto file = capture("invalid_nominal_target.py", text);
    ASSERT_TRUE(file);
    diagnostics.clear();
    EXPECT_TRUE(failed(lower(*file, "invalid_nominal_target.py")));
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(SourceTransactionCaptureTest,
       UnchangedTableContainingCaptureRemainsLegalButReconstructionDoesNot) {
  const std::string text = R"py(import pycircuit as ac
@ac.struct
class Packet:
    lanes: ac.table[2, ac.u8]
    tag: ac.u8
@ac.module
def Owner():
    @ac.method
    def choose(flag: ac.u1, packet: Packet) -> Packet:
        local = packet
        if flag:
            pass
        return local
@ac.module
def Top(packet: Packet) -> Packet:
    owner = Owner()
    @ac.rule
    def step() -> Packet:
        return owner.choose(0, packet)
    return step()
)py";
  ASSERT_TRUE(typed(text, "unchanged_table_capture.py"));
  for (StringRef statement :
       {"local.tag = 2", "local = packet\n            local.tag = 2"}) {
    auto changed = capture("changed_table_capture.py",
                           replaceOnce(text, "            pass",
                                       "            " + statement.str()));
    ASSERT_TRUE(changed);
    diagnostics.clear();
    EXPECT_TRUE(failed(lower(*changed, "changed_table_capture.py")));
    EXPECT_FALSE(diagnostics.empty());
  }
}


// Independent callable-output coverage: each position has its own source kind
// and nominal identity; an equal packed representation cannot replace it.
static constexpr llvm::StringLiteral callableTupleSource = R"py(import pycircuit as ac
@ac.struct
class Packet:
    value: ac.u8
@ac.module
def Owner():
    count: ac.u8 = 3
    @ac.method
    def split(delta: ac.u8, enabled: bool) -> tuple[ac.u8, bool, Packet]:
        count = count + delta
        return count, enabled, Packet(value=count)
    @ac.method
    def singleton(packet: Packet) -> tuple[Packet]:
        return (packet,)
@ac.module
def Top(value: ac.u8, flag: bool) -> tuple[ac.u8, bool, Packet]:
    owner = Owner()
    @ac.rule
    def step(delta: ac.u8, enabled: bool) -> tuple[ac.u8, bool, Packet]:
        word, logical, packet = owner.split(delta, enabled)
        saved, = owner.singleton(packet)
        return word, logical, saved
    word, logical, packet = step(enabled=flag, delta=value)
    return word, logical, packet
)py";

TEST_F(SourceTransactionCaptureTest,
       TypedRuleArgumentsAndMethodTuplesKeepOrderedHeterogeneousResults) {
  auto file = typed(callableTupleSource, "callable_tuple.py");
  ASSERT_TRUE(file);
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(transactions.size(), 1u);
  auto transaction = transactions.front();
  ASSERT_EQ(transaction.getNumResults(), 3u);
  EXPECT_TRUE(isa<ac::StructType>(transaction.getResult(2).getType()));
  auto parent = transaction->getParentOfType<ac::ModuleOp>();
  ASSERT_EQ(parent.getFunctionType().getNumResults(), 3u);
  auto output = cast<ac::YieldOp>(parent.getBody().front().getTerminator());
  for (unsigned i = 0; i != 3; ++i)
    EXPECT_EQ(output.getOperand(i), transaction.getResult(i));
  auto calls = direct<ac::SourceMethodCallOp>(executionBody(transaction));
  ASSERT_EQ(calls.size(), 2u);
  EXPECT_EQ(calls[0].getNumResults(), 3u);
  ASSERT_EQ(calls[1].getNumResults(), 1u);
  EXPECT_EQ(calls[1].getResult(0).getType(), transaction.getResult(2).getType());
  auto methods = operations<ac::SourceMethodOp>(*file);
  ASSERT_EQ(methods.size(), 3u);
  for (auto method : methods) {
    EXPECT_EQ(method->getAttrOfType<StringAttr>("ac.return_form").getValue(),
              "tuple");
    EXPECT_EQ(method->getAttrOfType<ArrayAttr>("ac.result_constraints").size(),
              method.getFunctionType().getNumResults());
  }
  // A keyword permutation must still bind the declared positional contracts.
  ASSERT_GE(transaction.getCaptures().size(), 2u);
  EXPECT_TRUE(ac::areEquivalentHardwareTypes(
      transaction.getCaptures()[0].getType(),
      parent.getFunctionType().getInput(0)));
  EXPECT_TRUE(ac::areEquivalentHardwareTypes(
      transaction.getCaptures()[1].getType(),
      parent.getFunctionType().getInput(1)));
}

TEST_F(SourceTransactionCaptureTest,
       BehavioralTopLevelAndNestedRulesUnpackTuplesIncludingSingletons) {
  auto file = typed(R"py(import pycircuit as ac
@ac.struct
class Packet:
    value: ac.u8
@ac.rule
def split(value: ac.u8, flag: bool) -> tuple[Packet, bool, ac.u8]:
    return Packet(value=value + 2), flag, value
@ac.rule
def singleton(value: ac.u8) -> tuple[ac.u8]:
    return (value,)
@ac.module
def Top(value: ac.u8, flag: bool) -> tuple[Packet, bool, ac.u8]:
    @ac.rule
    def local(delta: ac.u8) -> tuple[ac.u8]:
        return (delta + 1,)
    packet, logical, unchanged = split(value, flag)
    saved, = singleton(unchanged)
    changed, = local(saved)
    return packet, logical, changed
)py", "behavioral_tuple.py");
  ASSERT_TRUE(file);
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(transactions.size(), 3u);
  for (auto transaction : transactions) EXPECT_TRUE(entryMethod(transaction));
  ASSERT_EQ(operations<ac::SourceMethodOp>(*file).size(), 3u);
  for (StringRef name : {"split", "singleton", "local"})
    EXPECT_TRUE(namedMethod(*file, name));
  auto definitions = operations<ac::ModuleOp>(*file);
  ASSERT_EQ(definitions.size(), 1u);
  EXPECT_EQ(definitions[0].getFunctionType().getNumResults(), 3u);
  EXPECT_TRUE(operations<ac::RuleOp>(*file).empty());
}

TEST_F(SourceTransactionCaptureTest,
       BehavioralTupleRuleKeywordsPreserveOwnerActualsAndRejectInvalidBindings) {
  const std::string source = R"py(import pycircuit as ac
@ac.rule
def update(delta: ac.u8, count: ac.u8) -> tuple[ac.u8, ac.u8]:
    before = count
    count = count + delta
    return before, count
@ac.module
def Top(value: ac.u8) -> tuple[ac.u8, ac.u8]:
    counter: ac.u8 = 5
    before, after = update(count=counter, delta=value)
    return before, after
)py";
  auto file = typed(source, "behavioral_tuple_owner.py");
  ASSERT_TRUE(file);
  auto states = operations<ac::SourceStateOp>(*file);
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(states.size(), 1u);
  ASSERT_EQ(transactions.size(), 1u);
  EXPECT_TRUE(namedMethod(*file, "update"));
  auto actual = entryMethod(transactions.front());
  ASSERT_TRUE(actual);
  auto visibility = actual->getAttrOfType<StringAttr>("sym_visibility");
  ASSERT_TRUE(visibility);
  EXPECT_EQ(visibility.getValue(), "private");
  auto writes = direct<ac::SourceStateWriteOp>(executionBody(transactions.front()));
  ASSERT_EQ(writes.size(), 1u);
  EXPECT_EQ(writes[0].getState(), states[0].getInstanceName());
  EXPECT_TRUE(operations<ac::InstanceOp>(*file).empty());
  for (StringRef replacement : {
           "before, after = update(value, count=counter, delta=value)",
           "before, after = update(count=counter, absent=value)",
           "before, after = update(count=counter)"}) {
    SCOPED_TRACE(replacement.str());
    auto invalid = capture("bad_tuple_owner_arguments.py", replaceOnce(source,
        "before, after = update(count=counter, delta=value)", replacement));
    ASSERT_TRUE(invalid);
    diagnostics.clear();
    EXPECT_TRUE(failed(lower(*invalid, "bad_tuple_owner_arguments.py")));
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(SourceTransactionCaptureTest,
       TupleRuleKeywordArgumentsCannotAliasTwoWritableFormals) {
  auto file = capture("aliased_tuple_owners.py", R"py(import pycircuit as ac
@ac.rule
def update(left: ac.u8, right: ac.u8) -> tuple[ac.u8, ac.u8]:
    left = left + 1
    right = right + 2
    return left, right
@ac.module
def Top() -> tuple[ac.u8, ac.u8]:
    counter: ac.u8 = 5
    first, second = update(right=counter, left=counter)
    return first, second
)py");
  ASSERT_TRUE(file);
  diagnostics.clear();
  EXPECT_TRUE(failed(lower(*file, "aliased_tuple_owners.py")));
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(SourceTransactionCaptureTest,
       CallableTuplesRejectArityTargetsAndPerResultKindOrNominalMismatch) {
  for (auto replacement : {
           "word = owner.split(delta, enabled)",
           "word, logical = owner.split(delta, enabled)",
           "word, word, packet = owner.split(delta, enabled)",
           "word, (logical, packet) = owner.split(delta, enabled)",
           "word, *logical, packet = owner.split(delta, enabled)",
           "word, logical, packet.value = owner.split(delta, enabled)",
           "[word, logical, packet] = owner.split(delta, enabled)",
           "word, logical, packet = other = owner.split(delta, enabled)"}) {
    SCOPED_TRACE(replacement);
    auto text = replaceOnce(callableTupleSource.str(),
        "word, logical, packet = owner.split(delta, enabled)", replacement);
    auto file = capture("bad_tuple_target.py", text);
    ASSERT_TRUE(file);
    diagnostics.clear();
    EXPECT_TRUE(failed(lower(*file, "bad_tuple_target.py"))) << replacement;
    EXPECT_FALSE(diagnostics.empty());
  }
  for (auto replacement : {
           "word, logical, packet = step(enabled=value, delta=flag)",
           "word, logical, packet = step(value)",
           "word, logical, packet = step(value, flag, value)",
           "word, logical, packet = step(value, enabled=flag, delta=value)",
           "word, logical, packet = step(value, absent=flag)",
           "value, logical, packet = step(value, flag)"}) {
    SCOPED_TRACE(replacement);
    auto text = replaceOnce(callableTupleSource.str(),
        "word, logical, packet = step(enabled=flag, delta=value)", replacement);
    auto file = capture("bad_rule_argument.py", text);
    ASSERT_TRUE(file);
    diagnostics.clear();
    EXPECT_TRUE(failed(lower(*file, "bad_rule_argument.py"))) << replacement;
    EXPECT_FALSE(diagnostics.empty());
  }
  for (auto replacement : {
           "return count, count, Packet(value=count)",
           "return count, enabled, count",
           "return count, enabled",
           "return count",
           "return [count, enabled, Packet(value=count)]"}) {
    SCOPED_TRACE(replacement);
    auto text = replaceOnce(callableTupleSource.str(),
        "return count, enabled, Packet(value=count)", replacement);
    auto file = capture("bad_tuple_return.py", text);
    ASSERT_TRUE(file);
    diagnostics.clear();
    EXPECT_TRUE(failed(lower(*file, "bad_tuple_return.py"))) << replacement;
    EXPECT_FALSE(diagnostics.empty());
  }
  // bool and u1 share a one-bit carrier but retain different source kinds.
  for (auto text : {
           replaceOnce(callableTupleSource.str(),
               "-> tuple[ac.u8, bool, Packet]:",
               "-> tuple[ac.u8, ac.u1, Packet]:"),
           replaceOnce(callableTupleSource.str(), "flag: bool", "flag: ac.u1"),
           replaceOnce(callableTupleSource.str(),
               "        word, logical, packet = owner.split(delta, enabled)",
               "        logical: ac.u1 = 0\n"
               "        word, logical, packet = owner.split(delta, enabled)")}) {
    auto file = capture("wrong_tuple_logical_kind.py", text);
    ASSERT_TRUE(file);
    diagnostics.clear();
    EXPECT_TRUE(failed(lower(*file, "wrong_tuple_logical_kind.py")));
    EXPECT_FALSE(diagnostics.empty());
  }
  auto foreign = replaceOnce(callableTupleSource.str(),
      "@ac.module\ndef Owner():",
      "@ac.struct\nclass ForeignPacket:\n    value: ac.u8\n@ac.module\ndef Owner():");
  foreign = replaceOnce(foreign, "return count, enabled, Packet(value=count)",
                        "return count, enabled, ForeignPacket(value=count)");
  auto file = capture("foreign_tuple_return.py", foreign);
  ASSERT_TRUE(file);
  diagnostics.clear();
  EXPECT_TRUE(failed(lower(*file, "foreign_tuple_return.py")));
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(SourceTransactionCaptureTest,
       SingletonMethodAndRuleTuplesRequireExplicitUnpacking) {
  for (auto replacement : {"saved = owner.singleton(packet)",
                            "saved, extra = owner.singleton(packet)"}) {
    SCOPED_TRACE(replacement);
    auto file = capture("bad_singleton_call.py",
        replaceOnce(callableTupleSource.str(), "saved, = owner.singleton(packet)",
                    replacement));
    ASSERT_TRUE(file);
    diagnostics.clear();
    EXPECT_TRUE(failed(lower(*file, "bad_singleton_call.py")));
    EXPECT_FALSE(diagnostics.empty());
  }
  auto file = capture("bad_singleton_return.py",
      replaceOnce(callableTupleSource.str(), "return (packet,)", "return packet"));
  ASSERT_TRUE(file);
  diagnostics.clear();
  EXPECT_TRUE(failed(lower(*file, "bad_singleton_return.py")));
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(SourceTransactionCaptureTest,
       TypedRuleSingletonRetainsTupleShapeAtRegistrationAndReturn) {
  const std::string source = R"py(import pycircuit as ac
@ac.module
def Owner():
    @ac.method
    def read() -> ac.u8:
        return 8
@ac.module
def Top() -> tuple[ac.u8]:
    owner = Owner()
    @ac.rule
    def step(delta: ac.u8) -> tuple[ac.u8]:
        old = owner.read()
        return (old + delta,)
    value, = step(1)
    return (value,)
)py";
  auto file = typed(source, "singleton_rule.py");
  ASSERT_TRUE(file);
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(transactions.size(), 1u);
  EXPECT_EQ(transactions[0].getNumResults(), 1u);
  for (auto text : {
           replaceOnce(source, "value, = step(1)", "value = step(1)"),
           replaceOnce(source, "value, = step(1)", "value, extra = step(1)"),
           replaceOnce(source, "return (old + delta,)", "return old + delta")}) {
    auto invalid = capture("bad_rule_singleton.py", text);
    ASSERT_TRUE(invalid);
    diagnostics.clear();
    EXPECT_TRUE(failed(lower(*invalid, "bad_rule_singleton.py")));
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(SourceTransactionCaptureTest,
       TupleRuleResultsRequireDirectUnpackingAndCompilerAvailability) {
  auto forwarded = capture("forwarded_rule_tuple.py", R"py(import pycircuit as ac
@ac.rule
def split(value: ac.u8) -> tuple[ac.u8, ac.u8]:
    return value, value + 1
@ac.module
def Top(value: ac.u8) -> tuple[ac.u8, ac.u8]:
    return split(value)
)py");
  ASSERT_TRUE(forwarded);
  diagnostics.clear();
  EXPECT_TRUE(failed(lower(*forwarded, "forwarded_rule_tuple.py")));
  EXPECT_FALSE(diagnostics.empty());
  auto guarded = capture("guarded_rule_tuple.py",
      replaceOnce(callableTupleSource.str(), "    @ac.rule\n    def step",
                  "    @ac.rule(guard=lambda delta, enabled: enabled)\n"
                  "    def step"));
  ASSERT_TRUE(guarded);
  diagnostics.clear();
  ASSERT_TRUE(succeeded(lower(*guarded, "guarded_rule_tuple.py"))) << diagnostics;
  auto transactions = operations<ac::SourceTransactionOp>(*guarded);
  ASSERT_EQ(transactions.size(), 1u);
  ASSERT_EQ(transactions[0].getNumResults(), 3u);
  compilerAvailability(transactions.front());
  ac::SourceMethodSummariesAnalysis summaries(guarded->getOperation());
  ASSERT_TRUE(summaries.isValid());
  EXPECT_FALSE(summaries.isTotal(transactions.front()));
}

TEST_F(SourceTransactionCaptureTest,
       TypedTupleRuleOwnerFormalsUseActualStorageAndPreserveSnapshots) {
  const std::string source = R"py(import pycircuit as ac
@ac.module
def Top(delta: ac.u8) -> tuple[ac.u8, ac.u8]:
    count: ac.u8 = 5
    @ac.rule(guard=lambda amount, target: True)
    def update(amount: ac.u8, target: ac.u8) -> tuple[ac.u8, ac.u8]:
        before = target
        target = target + amount
        return before, target
    first, second = update(target=count, amount=delta)
    return first, second
)py";
  auto file = typed(source, "typed_tuple_owner.py");
  ASSERT_TRUE(file);
  auto states = operations<ac::SourceStateOp>(*file);
  ASSERT_EQ(states.size(), 1u);
  auto writes = operations<ac::SourceStateWriteOp>(*file);
  ASSERT_EQ(writes.size(), 1u);
  EXPECT_EQ(writes[0].getState(), states[0].getInstanceName());
  auto transactions = operations<ac::SourceTransactionOp>(*file);
  ASSERT_EQ(transactions.size(), 1u);
  auto yield = cast<ac::YieldOp>(executionBody(transactions[0]).front().getTerminator());
  ASSERT_EQ(yield.getNumOperands(), 2u);
  EXPECT_NE(yield.getOperand(0), yield.getOperand(1));
  auto wrongType = capture("wrong_typed_owner_formal.py",
      replaceOnce(source, "target: ac.u8", "target: ac.u7"));
  ASSERT_TRUE(wrongType);
  diagnostics.clear();
  EXPECT_TRUE(failed(lower(*wrongType, "wrong_typed_owner_formal.py")));
  EXPECT_FALSE(diagnostics.empty());
  auto alias = capture("aliased_typed_owner_formals.py", R"py(import pycircuit as ac
@ac.module
def Top() -> tuple[ac.u8, ac.u8]:
    count: ac.u8 = 5
    @ac.rule(guard=lambda left, right: True)
    def update(left: ac.u8, right: ac.u8) -> tuple[ac.u8, ac.u8]:
        left = left + 1
        right = right + 2
        return left, right
    first, second = update(right=count, left=count)
    return first, second
)py");
  ASSERT_TRUE(alias);
  diagnostics.clear();
  EXPECT_TRUE(failed(lower(*alias, "aliased_typed_owner_formals.py")));
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(SourceTransactionCaptureTest,
       TupleMethodBodyAndPublishedSummaryRejectReturnShapeAndCountTampering) {
  auto captured = capture("callable_tuple.py", callableTupleSource);
  ASSERT_TRUE(captured);
  auto headers = compiler::SourceHeaderRegistry::create(&context, {}, error());
  ASSERT_TRUE(succeeded(headers));
  auto compiled = compiler::compilePythonSourceUnit(
      *captured, owner("callable_tuple.py"), *headers, error());
  ASSERT_TRUE(succeeded(compiled)) << diagnostics;
  ASSERT_TRUE(succeeded(compiler::verifyIntrinsicSourceUnitPair(
      *compiled->body, *compiled->interface, error()))) << diagnostics;
  Builder b(&context);
  for (bool summary : {false, true}) {
    for (unsigned mutation = 0; mutation != 3; ++mutation) {
      SCOPED_TRACE(summary);
      SCOPED_TRACE(mutation);
      OwningOpRef<ModuleOp> bodyCopy(
          cast<ModuleOp>((*compiled->body)->clone()));
      OwningOpRef<ModuleOp> headerCopy(
          cast<ModuleOp>((*compiled->interface)->clone()));
      StringRef methodName = mutation == 2 ? "singleton" : "split";
      if (summary) {
        ac::ModuleImportOp provider;
        for (auto declaration : headerCopy->getOps<ac::ModuleImportOp>())
          if (declaration.getSymName().ends_with("Owner")) provider = declaration;
        ASSERT_TRUE(provider);
        auto contract = provider.getTransactionSummaryAttr();
        unsigned index = rowIndex(contract, "methods", methodName);
        auto row = cast<DictionaryAttr>(contract.getAs<ArrayAttr>("methods")[index]);
        row = mutation == 1
            ? changed(row, "ac.result_constraints", b.getArrayAttr({}))
            : changed(row, "ac.return_form", b.getStringAttr("single"));
        provider->setAttr("transaction_summary",
                          changedRow(contract, "methods", index, row));
      } else {
        ac::SourceMethodOp method;
        for (auto candidate : operations<ac::SourceMethodOp>(*bodyCopy))
          if (candidate.getSymName() == methodName) method = candidate;
        ASSERT_TRUE(method);
        method->setAttr(mutation == 1 ? "ac.result_constraints" : "ac.return_form",
                        mutation == 1 ? Attribute(b.getArrayAttr({}))
                                      : Attribute(b.getStringAttr("single")));
      }
      diagnostics.clear();
      if (mutation != 2) {
        EXPECT_TRUE(failed(mlir::verify(summary ? *headerCopy : *bodyCopy)));
      } else {
        // A singleton scalar form is structurally valid, but it is a different
        // authored callable contract from the tuple in the unchanged peer.
        ASSERT_TRUE(succeeded(mlir::verify(summary ? *headerCopy : *bodyCopy)))
            << diagnostics;
        EXPECT_TRUE(failed(compiler::verifyIntrinsicSourceUnitPair(
            *bodyCopy, *headerCopy, error())));
      }
      EXPECT_FALSE(diagnostics.empty());
    }
  }
}

TEST_F(SourceTransactionCaptureTest, NamedQueueHandlesRetainAliasesDirectionsAndRawCaptures) {
  auto file = typed(R"py(import pycircuit as ac
@ac.struct
class View:
    ready: ac.u1
    valid: ac.u1
    data: ac.u8
@ac.module
def Top(valid: ac.u1, data: ac.u8, take: ac.u1) -> View:
    source = ac.queue[ac.u8](in_valid=valid, in_data=data, depth=3, latency=2)
    dest = ac.queue[ac.u8](out_ready=take, depth=2)
    alias = source
    @ac.rule
    def relay() -> None:
        raw = source.valid
        observed = alias.peek()
        value = alias.pop()
        dest.push(value)
    @ac.rule
    def tick() -> None:
        relay()
    tick()
    return View(ready=source.ready, valid=dest.valid, data=dest.data)
)py", "queue_handles.py");
  ASSERT_TRUE(file);
  auto queues = queueInstances(*file);
  ASSERT_EQ(queues.size(), 2u);
  auto source = queues[0], dest = queues[1];
  EXPECT_EQ(source.getInstanceName(), "source");
  EXPECT_EQ(dest.getInstanceName(), "dest");
  EXPECT_EQ(source.getCallee(), "pycircuit.__builtins__.fifo_downstream_pop");
  EXPECT_EQ(dest.getCallee(), "pycircuit.__builtins__.fifo_downstream_pop");
  EXPECT_FALSE(source->hasAttr("ac.source_unbound_push"));
  EXPECT_TRUE(source->hasAttr("ac.source_unbound_pop"));
  EXPECT_TRUE(dest->hasAttr("ac.source_unbound_push"));
  EXPECT_FALSE(dest->hasAttr("ac.source_unbound_pop"));
  SmallVector<ac::SourceMethodCallOp> pops, peeks, pushes;
  for (auto call : operations<ac::SourceMethodCallOp>(*file)) {
    StringRef name = call.getCalleeAttr().getLeafReference().getValue();
    if (name == "pop")
      pops.push_back(call);
    if (name == "peek")
      peeks.push_back(call);
    if (name == "push")
      pushes.push_back(call);
  }
  ASSERT_EQ(pops.size(), 1u);
  ASSERT_EQ(peeks.size(), 1u);
  ASSERT_EQ(pushes.size(), 1u);
  EXPECT_EQ(cast<StringAttr>(pops[0].getInstancePath()[0]).getValue(), "source");
  EXPECT_EQ(cast<StringAttr>(peeks[0].getInstancePath()[0]).getValue(), "source");
  EXPECT_EQ(cast<StringAttr>(pushes[0].getInstancePath()[0]).getValue(), "dest");
  EXPECT_TRUE(identityTransport(pushes[0].getArguments()[0],
                                pops[0].getResults()[0], *file));
  EXPECT_TRUE(operations<ac::QueueOp>(*file).empty());
  EXPECT_TRUE(operations<ac::SourceQueuePopOp>(*file).empty());
  EXPECT_TRUE(operations<ac::SourceQueuePeekOp>(*file).empty());
  EXPECT_TRUE(operations<ac::SourceQueuePushOp>(*file).empty());
  bool capturedRaw = false;
  for (auto method : operations<ac::SourceMethodOp>(*file))
    for (auto call : direct<ac::SourceMethodCallOp>(method.getBody()))
      if (call.getCalleeAttr().getLeafReference().getValue() == "pop")
        capturedRaw |=
            llvm::is_contained(method.getCaptures(), source.getOutputs()[1]);
  EXPECT_TRUE(capturedRaw);
}

TEST_F(SourceTransactionCaptureTest, NamedAndLegacyQueuesKeepDistinctDefaultPolicies) {
  auto file = typed(R"py(import pycircuit as ac
@ac.struct
class View:
    ready: ac.u1
    valid: ac.u1
    data: ac.u8
@ac.module
def Top(valid: ac.u1, data: ac.u8, take: ac.u1) -> View:
    named = ac.queue[ac.u8](in_valid=valid, in_data=data, out_ready=take, depth=2)
    ready, available, value = ac.queue[ac.u8](valid, data, take, depth=2)
    return View(ready=named.ready & ready, valid=named.valid & available, data=named.data ^ value)
)py", "queue_policies.py");
  ASSERT_TRUE(file);
  auto queues = queueInstances(*file);
  ASSERT_EQ(queues.size(), 2u);
  EXPECT_EQ(queues[0].getCallee(),
            "pycircuit.__builtins__.fifo_downstream_pop");
  EXPECT_EQ(queues[1].getCallee(), "pycircuit.__builtins__.fifo_local");
  EXPECT_FALSE(queues[1]->hasAttr("ac.payload_constraint"));
}

TEST_F(SourceTransactionCaptureTest, QueueEffectsRejectGuardsAndRawDirectionMixing) {
  reject(R"py(import pycircuit as ac
@ac.module
def Top() -> None:
    fifo = ac.queue[ac.u8](depth=2)
    @ac.rule(guard=lambda: fifo.peek() == ac.u8(0))
    def bad() -> None:
        fifo.pop()
    bad()
)py", "guard cannot call a Method");
  reject(R"py(import pycircuit as ac
@ac.module
def Top(valid: ac.u1, data: ac.u8) -> None:
    fifo = ac.queue[ac.u8](in_valid=valid, in_data=data, depth=2)
    @ac.rule
    def bad() -> None:
        fifo.push(data)
    bad()
)py", "conflicts with an externally bound raw input");
  reject(R"py(import pycircuit as ac
@ac.module
def Top(valid: ac.u1) -> None:
    fifo = ac.queue[ac.u8](in_valid=valid, depth=2)
    @ac.rule
    def idle() -> None:
        pass
    idle()
)py", "in_valid and in_data together");
}

TEST_F(SourceTransactionCaptureTest, QueueCaptureGrowthKeepsPhysicalDomainValues) {
  auto file = typed(R"py(import pycircuit as ac
from enum import Enum
@ac.encoding(width=3, kind="explicit")
class Tag(Enum):
    FIVE = 5
    SIX = 6
@ac.struct
class Packet:
    tag: Tag
    wide: ac.bits[130]
@ac.module
def Top() -> None:
    flags = ac.queue[bool](depth=2)
    fixed = ac.queue[ac.u1](depth=2)
    tags = ac.queue[Tag](depth=2)
    packets = ac.queue[Packet](depth=2)
    lanes = ac.queue[ac.table[3, Tag]](depth=2)
    @ac.rule
    def idle() -> None:
        pass
    idle()
)py", "queue_carriers.py");
  ASSERT_TRUE(file);
  auto queues = queueInstances(*file);
  ASSERT_EQ(queues.size(), 5u);
  ac::InstanceOp flags, fixed;
  for (auto queue : queues) {
    if (queue.getInstanceName() == "flags")
      flags = queue;
    if (queue.getInstanceName() == "fixed")
      fixed = queue;
  }
  ASSERT_TRUE(flags && fixed);
  auto flagsConstraint =
      flags->getAttrOfType<DictionaryAttr>("ac.payload_constraint");
  auto fixedConstraint =
      fixed->getAttrOfType<DictionaryAttr>("ac.payload_constraint");
  ASSERT_TRUE(flagsConstraint && fixedConstraint);
  EXPECT_EQ(flagsConstraint.getAs<StringAttr>("source_kind").getValue(),
            "boolean");
  EXPECT_EQ(fixedConstraint.getAs<StringAttr>("source_kind").getValue(),
            "fixed_bits");
  EXPECT_TRUE(ac::areEquivalentHardwareTypes(
      cast<TypeAttr>(flags.getTypeArguments()[0]).getValue(),
      cast<TypeAttr>(fixed.getTypeArguments()[0]).getValue()));
  auto module = queues.front()->getParentOfType<ac::ModuleOp>();
  auto &block = module.getBody().front();
  ASSERT_EQ(block.getNumArguments(), 2u);
  for (auto queue : queues) {
    EXPECT_EQ(queue.getInputs()[0], block.getArgument(0));
    EXPECT_EQ(queue.getInputs()[1], block.getArgument(1));
  }
}

TEST_F(SourceTransactionCaptureTest,
       PartialWritesRetainSelectedRhsAndStatementIndex) {
  auto file = typed(R"py(import pycircuit as ac
@ac.struct
class Cell:
    data: ac.u8
    tag: ac.u4
@ac.module
def Top(index: ac.u1, data: ac.u8, flag: ac.u1) -> Cell:
    packet: Cell = Cell()
    entries = ac.table[2, Cell](init=0)
    @ac.rule
    def selected(target: ac.table[2, Cell], slot: ac.u1, value: ac.u8) -> None:
        target[slot].data = value
        slot = slot ^ 1
    @ac.rule
    def drive() -> Cell:
        if flag:
            packet.data = data
            entries[index].data = data
        else:
            packet.tag = 1
        return packet
    return drive()
)py",
                    "selected_writes.py");
  ASSERT_TRUE(file) << diagnostics;
  auto drive = namedMethod(*file, "drive");
  ASSERT_TRUE(drive);
  unsigned fields = 0, indexed = 0;
  drive.getBody().walk([&](ac::SourceStateWriteOp write) {
    EXPECT_TRUE(write->getParentOfType<ac::SourceBranchOp>());
    EXPECT_TRUE(isa<ac::BitsType>(write.getValue().getType()));
    if (write.getState() == "packet") {
      EXPECT_EQ(write.getSelectors().size(), 1u);
      EXPECT_TRUE(write.getIndices().empty());
      ++fields;
    } else if (write.getState() == "entries") {
      EXPECT_EQ(write.getSelectors().size(), 2u);
      ASSERT_EQ(write.getIndices().size(), 1u);
      auto blockArg = dyn_cast<BlockArgument>(write.getIndices()[0]);
      EXPECT_TRUE(blockArg);
      ++indexed;
    }
  });
  EXPECT_EQ(fields, 2u);
  EXPECT_EQ(indexed, 1u);
  auto selected = namedMethod(*file, "selected");
  ASSERT_TRUE(selected);
  auto writes = operations<ac::SourceFormalWriteOp>(*file);
  ASSERT_EQ(writes.size(), 2u);
  ac::SourceFormalWriteOp write;
  for (auto candidate : writes)
    if (candidate.getFormalAttr().getInt() == 0)
      write = candidate;
  ASSERT_TRUE(write);
  EXPECT_EQ(write->getParentOfType<ac::SourceMethodOp>(), selected);
  EXPECT_EQ(write.getSelectors().size(), 2u);
  ASSERT_EQ(write.getIndices().size(), 1u);
  EXPECT_EQ(write.getIndices()[0], selected.getBody().front().getArgument(1));
  EXPECT_TRUE(isa<ac::TableType>(write.getCandidate().getType()));
  EXPECT_TRUE(isa<ac::BitsType>(write.getPayload().getType()));
  ac::SourceMethodSummariesAnalysis summaries(file->getOperation());
  EXPECT_TRUE(summaries.isValid()) << diagnostics;
}

TEST_F(SourceTransactionCaptureTest, ModuleLocalSelectedUpdateReadsCommittedStateIndex) {
  auto file = typed(R"py(import pycircuit as ac
@ac.struct
class Cell:
    value: ac.u4
@ac.struct
class Result:
    old: ac.u4
    edited: ac.u4
@ac.module
def Top(data: ac.u4) -> Result:
    entries = ac.table[2, Cell](init=Cell(value=3))
    index: ac.u1 = 0
    @ac.rule
    def advance() -> None:
        index = index + 1
    advance()
    snapshot = entries
    saved = snapshot
    snapshot[index].value = data
    return Result(old=saved[index].value, edited=snapshot[index].value)
)py", "module_local_index.py");
  ASSERT_TRUE(file) << diagnostics;
  EXPECT_EQ(operations<ac::SourceStateOp>(*file).size(), 2u);
  EXPECT_EQ(operations<ac::SourceStateWriteOp>(*file).size(), 1u);
  EXPECT_EQ(operations<ac::TableMapOp>(*file).size(), 1u);
}

TEST_F(SourceTransactionCaptureTest, PureHelperUsesSharedFixedResultBinder) {
  auto file = typed(R"py(import pycircuit as ac
def encode(value: ac.u8) -> ac.u3:
    index, valid, conflict = ac.onehot_encode(value)
    return index
@ac.module
def Top(value: ac.u8) -> ac.u3:
    @ac.rule
    def run() -> ac.u3:
        return encode(value)
    return run()
)py", "helper_fixed_results.py");
  ASSERT_TRUE(file) << diagnostics;
  EXPECT_TRUE(succeeded(verify(*file))) << diagnostics;
}

TEST_F(SourceTransactionCaptureTest,
       PureHelperTableCallbacksKeepParametersLocalAndCaptureOuterValues) {
  auto file = typed(R"py(import pycircuit as ac
@ac.struct
class Samples:
    words: ac.table[2, ac.u8]
@ac.struct
class Result:
    first: ac.u8
    last: ac.u8
@ac.module
def Top(offset: ac.u8, shadow: ac.u8) -> Result:
    def mapped(values: Samples) -> Result:
        shifted = values.words.map(lambda shadow, peer: shadow + peer + offset, values.words)
        first_index, first_valid = shifted.first(where=lambda shadow: shadow != 0)
        best_index, best_valid = shifted.argmin(where=lambda shadow: shadow != 0, key=lambda shadow: shadow)
        return Result(first=shifted[first_index], last=shifted[best_index])
    return mapped(Samples(words=(1, 2)))
)py", "helper_table_callbacks.py");
  ASSERT_TRUE(file) << diagnostics;
  unsigned helpers = 0;
  for (auto helper : operations<func::FuncOp>(*file)) {
    if (helper->getAttrOfType<StringAttr>("ac.helper_kind") !=
        StringAttr::get(&context, "value")) continue;
    ++helpers;
    auto captures = helper->getAttrOfType<ArrayAttr>("ac.capture_types");
    ASSERT_TRUE(captures);
    EXPECT_EQ(captures.size(), 1u); // offset; callback shadow is not captured.
    EXPECT_EQ(helper.getNumArguments(), 2u); // Samples plus offset.
  }
  EXPECT_EQ(helpers, 1u);
  EXPECT_TRUE(succeeded(verify(*file))) << diagnostics;
}

TEST_F(SourceTransactionCaptureTest,
       PureHelperTableCallbackAdmissionCannotHideEffectsOrLambdaValues) {
  constexpr StringLiteral sourceText = R"py(import pycircuit as ac
@ac.struct
class Samples:
    words: ac.table[2, ac.u8]
@ac.struct
class Result:
    value: ac.u8

def mapped(values: Samples) -> Result:
    updated = values.words.map(lambda row: row + 1)
    return Result(value=updated[0])
@ac.module
def Top() -> Result:
    return mapped(Samples(words=(1, 2)))
)py";
  for (StringRef replacement : {
           "updated = values.words.map(lambda row: ac.report(row))",
           "updated = values.words.map(lambda row=1: row)",
           "updated = values.words.map(lambda row: row, lambda peer: peer)",
           "callback = lambda row: row\n    updated = values.words.map(callback)"}) {
    std::string text = sourceText.str();
    auto offset = text.find("updated = values.words.map(lambda row: row + 1)");
    ASSERT_NE(offset, std::string::npos);
    text.replace(offset, std::string("updated = values.words.map(lambda row: row + 1)").size(),
                 replacement.str());
    auto captured = capture("bad_helper_table_callback.py", text);
    ASSERT_TRUE(captured);
    diagnostics.clear();
    EXPECT_TRUE(failed(lower(*captured, "bad_helper_table_callback.py")))
        << replacement.str();
    EXPECT_FALSE(diagnostics.empty()) << replacement.str();
  }
}

TEST_F(SourceTransactionCaptureTest, TypedRuleModuleReusesStandardMemoryBinding) {
  auto file = typed(R"py(import pycircuit as ac
@ac.module
def Top(data: ac.u8, index: ac.u2) -> ac.u8:
    state: ac.u8 = 0
    raw = ac.sync_mem[ac.u8](True, index, False, index, data, 0, depth=4)
    @ac.rule
    def update() -> None:
        state = raw
    update()
    return state
)py", "typed_rule_memory.py");
  ASSERT_TRUE(file) << diagnostics;
  EXPECT_EQ(operations<ac::InstanceOp>(*file).size(), 1u);
  EXPECT_EQ(operations<ac::SourceStateOp>(*file).size(), 1u);
  EXPECT_EQ(operations<ac::SourceStateWriteOp>(*file).size(), 1u);
  EXPECT_TRUE(succeeded(verify(*file))) << diagnostics;
}

TEST_F(SourceTransactionCaptureTest,
       GlobalUnannotatedStatefulRuleCalledOnlyFromLocalRuleUsesTransactionBinding) {
  auto file = typed(R"py(import pycircuit as ac
@ac.rule
def install(target, index: ac.u1, value: ac.u8) -> ac.u8:
    old = target[index]
    target[index] = value
    return old
@ac.module
def Top(index: ac.u1, value: ac.u8) -> ac.u8:
    entries = ac.table[2, ac.u8](init=0)
    @ac.rule
    def transfer() -> ac.u8:
        return install(entries, index, value)
    return transfer()
)py", "nested_global_stateful_rule.py");
  ASSERT_TRUE(file) << diagnostics;
  EXPECT_EQ(operations<ac::SourceTransactionOp>(*file).size(), 1u);
  EXPECT_EQ(operations<ac::SourceStateWriteOp>(*file).size(), 1u);
  EXPECT_GE(operations<ac::SourceMethodCallOp>(*file).size(), 2u);
  EXPECT_TRUE(succeeded(verify(*file))) << diagnostics;
}

TEST_F(SourceTransactionCaptureTest,
       GlobalRuleNestedCallAdmissionRetainsPureScopeAndShadowingRestrictions) {
  constexpr StringLiteral sourceText = R"py(import pycircuit as ac
@ac.rule
def install(target, index: ac.u1, value: ac.u8) -> ac.u8:
    old = target[index]
    target[index] = value
    return old
@ac.struct
class Samples:
    words: ac.table[2, ac.u8]
@ac.module
def Top(index: ac.u1, value: ac.u8) -> ac.u8:
    entries = ac.table[2, ac.u8](init=0)
    BODY
)py";
  for (StringRef body : {
           "def pure(arg: ac.u8) -> ac.u8:\n        return install(entries, index, arg)\n    return pure(value)",
           "samples = Samples(words=(1, 2))\n    mapped = samples.words.map(lambda arg: install(entries, index, arg))\n    return mapped[0]",
           "@ac.rule\n    def transfer(install: ac.u8) -> ac.u8:\n        return install(entries, index, value)\n    return transfer(value)"}) {
    std::string text = sourceText.str();
    auto offset = text.find("BODY");
    ASSERT_NE(offset, std::string::npos);
    text.replace(offset, 4, body.str());
    auto captured = capture("bad_nested_global_rule.py", text);
    ASSERT_TRUE(captured);
    diagnostics.clear();
    EXPECT_TRUE(failed(lower(*captured, "bad_nested_global_rule.py")))
        << body.str();
    EXPECT_FALSE(diagnostics.empty()) << body.str();
  }
}

} // namespace
