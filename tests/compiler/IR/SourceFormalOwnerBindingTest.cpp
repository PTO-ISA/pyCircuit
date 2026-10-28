#include "NativeFormalOwnerFixture.h"
#include "Compiler/SourceLink.h"
#include "Compiler/SourceUnit.h"
#include "Dialect/ACIR/HardwareSourceCheckPreflight.h"
#include "Transforms/LowerTransactions.h"
#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceTransactionStateAnalysis.h"
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
namespace ac = acir::ac;
namespace compiler = acir::compiler;

// Generic callable bodies come from production Python capture. Owning formal
// edges are then explicit native common-IR fixtures, with freshly published
// interfaces. A separate test covers Python owner specialization itself.
class SourceFormalOwnerBindingTest : public ::testing::Test {
protected:
  MLIRContext context;
  Builder builder{&context};
  llvm::SmallString<256> directory;
  std::string diagnostics;
  std::unique_ptr<ScopedDiagnosticHandler> handler;

  void SetUp() override {
    context.loadDialect<ac::ACIRDialect>();
    ASSERT_FALSE(llvm::sys::fs::createUniqueDirectory("formal-owner", directory));
    handler = std::make_unique<ScopedDiagnosticHandler>(
        &context, [&](Diagnostic &diagnostic) {
          llvm::raw_string_ostream stream(diagnostics);
          diagnostic.print(stream);
          return success();
        });
  }
  void TearDown() override { llvm::sys::fs::remove_directories(directory); }
  auto error() { return [&] { return emitError(builder.getUnknownLoc()); }; }
  std::string path(StringRef name) {
    llvm::SmallString<256> result(directory);
    llvm::sys::path::append(result, name);
    return result.str().str();
  }
  OwningOpRef<ModuleOp> fixture(bool nativeOwners = true) {
    std::error_code ec;
    {
      llvm::raw_fd_ostream stream(path("formal.py"), ec);
      EXPECT_FALSE(ec);
      if (ec) return {};
      stream << R"py(import pycircuit as ac
@ac.rule
def alter(value: ac.u8) -> None:
    value = value + 3
@ac.rule
def forward(value: ac.u8) -> None:
    value = value + 2
    alter(value)
@ac.rule
def pair(left: ac.u8, right: ac.u8) -> None:
    left = left + 1
    right = right + 2
)py";
      // Only the by-value form supplies generic proposal signatures. The
      // native fixture below binds selected calls to physical owners.
      stream << (nativeOwners ? R"py(@ac.module
def Top(input_left: ac.u8, input_right: ac.u8):
    left: ac.u8 = 11
    right: ac.u8 = 29
    @ac.rule
    def run() -> None:
        snapshot = left
        forward(snapshot)
        forward(input_left)
        pair(input_left, input_right)
)py" : R"py(@ac.module
def Top():
    left: ac.u8 = 11
    right: ac.u8 = 29
    @ac.rule
    def run() -> None:
        snapshot = left
        forward(snapshot)
        forward(left)
        pair(left, right)
)py");
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
    std::vector<std::string> owned{PYCIRCUIT_TEST_PYTHON, "-c", script.str(),
        PYCIRCUIT_TEST_REPO_ROOT, path("formal.py"), directory.str().str(),
        path("capture.mlir")};
    SmallVector<StringRef> args;
    for (auto &arg : owned) args.push_back(arg);
    std::string log = path("capture.log");
    std::array<std::optional<StringRef>, 3> redirects{std::nullopt, log, log};
    int status = llvm::sys::ExecuteAndWait(PYCIRCUIT_TEST_PYTHON, args,
                                          std::nullopt, redirects);
    auto output = llvm::MemoryBuffer::getFile(log);
    EXPECT_EQ(status, 0) << (output ? output.get()->getBuffer().str() : log);
    if (status) return {};
    auto captured = parseSourceFile<ModuleOp>(path("capture.mlir"), &context);
    if (!captured) return {};
    auto headers = compiler::SourceHeaderRegistry::create(&context, {}, error());
    if (failed(headers)) return {};
    auto owner = builder.getDictionaryAttr({
        builder.getNamedAttr("package", builder.getStringAttr("")),
        builder.getNamedAttr("path", builder.getStringAttr("formal.py"))});
    auto unit = compiler::compilePythonSourceUnit(*captured, owner, *headers, error());
    EXPECT_TRUE(succeeded(unit)) << diagnostics;
    if (failed(unit)) return {};
    if (nativeOwners) {
      unsigned forwards = 0, bound = 0;
      bool valid = true;
      unit->body->walk([&](ac::SourceMethodCallOp call) {
        auto name = call.getCalleeAttr().getLeafReference().getValue();
        if (name == "forward" && ++forwards == 2) {
          valid &= succeeded(acir::test::bindNativeFormalOwners(call, {"left"}));
          ++bound;
        } else if (name == "pair") {
          valid &= succeeded(acir::test::bindNativeFormalOwners(call, {"left", "right"}));
          ++bound;
        }
      });
      EXPECT_EQ(bound, 2u);
      EXPECT_TRUE(valid) << diagnostics;
      if (!valid || bound != 2 ||
          failed(acir::test::publishNativeFormalOwnerFixture(*unit, context)))
        return {};
    }
    compiler::SourceLinkStages stages;
    auto final = compiler::linkSourceUnits({{*unit->body, *unit->interface}},
                                             "formal.Top", error(), compiler::SourceLinkTarget::Hardware, &stages);
    EXPECT_TRUE(succeeded(final)) << diagnostics;
    if (failed(final)) return {};
    return parseSourceString<ModuleOp>(stages.beforeTransactions, &context);
  }
  // This stage intentionally stops before link/materialization: these cases
  // exercise the native owner/history verifier on production-captured effects.
  OwningOpRef<ModuleOp> selectedFixture(bool table, unsigned extent = 2) {
    std::string type = table ? "ac.table[" + std::to_string(extent) + ", Cell]" : "Box";
    std::string selected = table ? "[index].field" : ".inner.field";
    // Persistent Tables use the existing element-image initializer. These
    // tests authenticate assignment reconstruction, independently of reset data.
    std::string initial = table
        ? "Cell(field=1, sibling=2)"
        : "Box(inner=Cell(field=1, sibling=2), tail=3)";
    std::string source = R"py(import pycircuit as ac
@ac.struct
class Cell:
    field: ac.u4
    sibling: ac.u4
@ac.struct
class Box:
    inner: Cell
    tail: ac.u4
@ac.rule
def edit(left: TYPE, right: TYPE, index: INDEX, rhs: ac.u4) -> None:
    leftSELECTED = rhs
    leftSELECTED = rhs + 1
    rightSELECTED = rhs + 2
@ac.rule
def relay(left: TYPE, right: TYPE, index: INDEX, rhs: ac.u4) -> None:
    edit(left, right, index, rhs)
@ac.rule
def replace(left: TYPE, right: TYPE) -> None:
    left = right
@ac.module
def Top(index: INDEX, rhs: ac.u4, input_left: TYPE, input_right: TYPE):
    left: TYPE = INITIAL
    right: TYPE = INITIAL
    @ac.rule
    def run() -> None:
        saved = left
        relay(saved, input_right, index, rhs)
        relay(input_left, input_right, index, rhs)
        relay(input_left, input_right, index, rhs)
        replace(input_left, input_right)
)py";
    auto substitute = [&](StringRef key, StringRef value) {
      size_t at = 0;
      while ((at = source.find(key.str(), at)) != std::string::npos) {
        source.replace(at, key.size(), value.str());
        at += value.size();
      }
    };
    substitute("INDEX", table && extent == 2 ? "ac.u1" : "ac.u4");
    substitute("TYPE", type);
    substitute("SELECTED", selected);
    substitute("INITIAL", initial);
    std::error_code ec;
    {
      llvm::raw_fd_ostream stream(path("selected.py"), ec);
      EXPECT_FALSE(ec);
      if (ec) return {};
      stream << source;
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
    std::vector<std::string> owned{PYCIRCUIT_TEST_PYTHON, "-c", script.str(),
        PYCIRCUIT_TEST_REPO_ROOT, path("selected.py"), directory.str().str(),
        path("selected-capture.mlir")};
    SmallVector<StringRef> args;
    for (auto &arg : owned) args.push_back(arg);
    std::string log = path("selected-capture.log");
    std::array<std::optional<StringRef>, 3> redirects{std::nullopt, log, log};
    int status = llvm::sys::ExecuteAndWait(PYCIRCUIT_TEST_PYTHON, args,
                                          std::nullopt, redirects);
    auto output = llvm::MemoryBuffer::getFile(log);
    EXPECT_EQ(status, 0) << (output ? output.get()->getBuffer().str() : log);
    if (status) return {};
    auto captured = parseSourceFile<ModuleOp>(path("selected-capture.mlir"), &context);
    if (!captured) return {};
    auto headers = compiler::SourceHeaderRegistry::create(&context, {}, error());
    if (failed(headers)) return {};
    auto owner = builder.getDictionaryAttr({
        builder.getNamedAttr("package", builder.getStringAttr("")),
        builder.getNamedAttr("path", builder.getStringAttr("selected.py"))});
    auto unit = compiler::compilePythonSourceUnit(*captured, owner, *headers, error());
    EXPECT_TRUE(succeeded(unit)) << diagnostics;
    if (failed(unit)) return {};
    unsigned relays = 0, bound = 0;
    bool valid = true;
    unit->body->walk([&](ac::SourceMethodCallOp call) {
      auto name = call.getCalleeAttr().getLeafReference().getValue();
      if (name == "relay") {
        valid &= succeeded(acir::test::bindNativeFormalOwners(
            call, {relays++ ? "left" : "", "right", "", ""}));
        ++bound;
      } else if (name == "replace") {
        valid &= succeeded(acir::test::bindNativeFormalOwners(call, {"left", "right"}));
        ++bound;
      }
    });
    EXPECT_EQ(bound, 4u);
    EXPECT_TRUE(valid) << diagnostics;
    if (!valid || bound != 4 ||
        failed(acir::test::publishNativeFormalOwnerFixture(*unit, context)))
      return {};
    return std::move(unit->body);
  }
  SmallVector<ac::SourceFormalWriteOp> selectedWrites(ModuleOp file) {
    SmallVector<ac::SourceFormalWriteOp> writes;
    for (auto marker : all<ac::SourceFormalWriteOp>(file))
      if (!marker.getPrevious() && !marker.getSelectors().empty())
        writes.push_back(marker);
    return writes;
  }
  void rejectsSelectedCandidate(ModuleOp file, StringRef expected = "selected formal candidate") {
    diagnostics.clear();
    // Mutations must preserve native schemas and reach formal candidate
    // authentication, rather than merely provoke a generic IR type error.
    ASSERT_TRUE(succeeded(verify(file))) << diagnostics;
    diagnostics.clear();
    ac::SourceTransactionStateAnalysis analysis(file);
    EXPECT_FALSE(analysis.isValid());
    EXPECT_NE(diagnostics.find(expected.str()), std::string::npos)
        << diagnostics;
  }
  template <typename Op> SmallVector<Op> all(ModuleOp file) {
    SmallVector<Op> result;
    file.walk([&](Op op) { result.push_back(op); });
    return result;
  }
  ac::SourceMethodCallOp ownerCall(ModuleOp file) {
    for (auto call : all<ac::SourceMethodCallOp>(file)) {
      auto rows = call->getAttrOfType<ArrayAttr>("ac.formal_owner_bindings");
      if (rows && rows.size() == 1 &&
          cast<DictionaryAttr>(rows[0]).get("state")) return call;
    }
    return {};
  }
  ac::SourceStateOp state(ModuleOp file, StringRef name) {
    for (auto op : all<ac::SourceStateOp>(file))
      if (op->getAttrOfType<StringAttr>("instance_name").getValue() == name)
        return op;
    return {};
  }
  DictionaryAttr changed(DictionaryAttr row, StringRef key, Attribute value) {
    NamedAttrList attrs(row);
    attrs.set(key, value);
    return attrs.getDictionary(&context);
  }
  void rejects(ModuleOp file) {
    diagnostics.clear();
    ac::SourceTransactionStateAnalysis analysis(file);
    EXPECT_FALSE(analysis.isValid());
    EXPECT_FALSE(diagnostics.empty());
  }
};

TEST_F(SourceFormalOwnerBindingTest, PythonOwnerCallsSpecializeWithoutNativeOwnerEdges) {
  auto file = fixture(false);
  ASSERT_TRUE(file) << diagnostics;
  ac::SourceTransactionStateAnalysis analysis(*file);
  ASSERT_TRUE(analysis.isValid()) << diagnostics;
  unsigned directWrites = 0, physicalBindings = 0;
  file->walk([&](ac::SourceStateWriteOp) { ++directWrites; });
  file->walk([&](ac::SourceMethodCallOp call) {
    if (auto bindings = call->getAttrOfType<ArrayAttr>("ac.formal_owner_bindings"))
      for (auto raw : bindings)
        physicalBindings += bool(cast<DictionaryAttr>(raw).get("state"));
  });
  EXPECT_GE(directWrites, 2u);
  EXPECT_EQ(physicalBindings, 0u);
  bool owningStateHistory = false;
  for (auto transaction : all<ac::SourceTransactionOp>(*file)) {
    auto *plan = analysis.lookup(transaction);
    ASSERT_TRUE(plan);
    owningStateHistory |= !plan->writes.empty();
  }
  EXPECT_TRUE(owningStateHistory);
}

TEST_F(SourceFormalOwnerBindingTest, ActualSSAAndPhysicalOwnerAuthenticateBindings) {
  auto original = fixture();
  ASSERT_TRUE(original) << diagnostics;
  for (unsigned mutation = 0; mutation < 7; ++mutation) {
    SCOPED_TRACE(mutation);
    OwningOpRef<ModuleOp> file(cast<ModuleOp>(original->clone()));
    auto call = ownerCall(*file);
    ASSERT_TRUE(call);
    auto rows = call->getAttrOfType<ArrayAttr>("ac.formal_owner_bindings");
    auto row = cast<DictionaryAttr>(rows[0]);
    auto left = state(*file, "left");
    auto right = state(*file, "right");
    ASSERT_TRUE(left && right);
    if (mutation == 0) call->removeAttr("ac.formal_owner_bindings");
    if (mutation == 1)
      call->setAttr("ac.formal_owner_bindings", builder.getArrayAttr({
          changed(row, "state", builder.getStringAttr("right"))}));
    if (mutation == 2) {
      auto constraint = left->getAttrOfType<DictionaryAttr>("ac.payload_constraint");
      ASSERT_TRUE(constraint);
      constraint = changed(constraint, "source_kind", builder.getStringAttr("integer"));
      constraint = changed(constraint, "domain", parseAttribute(
          "#ac.source_domain<{kind = \"integer\", lower = #ac.math_int<0>, upper = #ac.math_int<256>}>", &context));
      left->setAttr("ac.payload_constraint", constraint);
    }
    if (mutation == 3) left->setOperand(0, left->getOperand(1));
    if (mutation == 4) {
      // A valid one-bit endpoint cannot become an eight-bit physical owner.
      left.getResult().setType(left->getOperand(0).getType());
    }
    if (mutation == 5) {
      // Metadata presence cannot turn a saved, by-value snapshot into ownership.
      ac::SourceMethodCallOp valueCall;
      for (auto candidate : all<ac::SourceMethodCallOp>(*file))
        if (candidate.getCalleeAttr() == call.getCalleeAttr() &&
            candidate->getParentOp() == call->getParentOp() &&
            !candidate->hasAttr("ac.formal_owner_bindings")) valueCall = candidate;
      ASSERT_TRUE(valueCall);
      valueCall->setAttr("ac.formal_owner_bindings", rows);
    }
    if (mutation == 6) {
      ac::SourceMethodCallOp pair;
      for (auto candidate : all<ac::SourceMethodCallOp>(*file)) {
        auto bindings = candidate->getAttrOfType<ArrayAttr>("ac.formal_owner_bindings");
        if (bindings && bindings.size() == 2) pair = candidate;
      }
      ASSERT_TRUE(pair);
      auto bindings = pair->getAttrOfType<ArrayAttr>("ac.formal_owner_bindings");
      auto first = cast<DictionaryAttr>(bindings[0]);
      auto second = cast<DictionaryAttr>(bindings[1]);
      pair->setOperand(2, pair->getOperand(1));
      pair->setAttr("ac.formal_owner_bindings", builder.getArrayAttr({first,
          changed(second, "state", first.get("state"))}));
    }
    rejects(*file);
  }
}

TEST_F(SourceFormalOwnerBindingTest, ForwardedSelectionPreviousAndProposalSlotsAreExact) {
  auto original = fixture();
  ASSERT_TRUE(original) << diagnostics;
  for (unsigned mutation = 0; mutation < 4; ++mutation) {
    SCOPED_TRACE(mutation);
    OwningOpRef<ModuleOp> file(cast<ModuleOp>(original->clone()));
    ac::SourceFormalWriteOp marker;
    for (auto candidate : all<ac::SourceFormalWriteOp>(*file))
      if (candidate.getPrevious()) { marker = candidate; break; }
    ASSERT_TRUE(marker);
    auto select = marker.getCandidate().getDefiningOp<ac::BitsSelectOp>();
    ASSERT_TRUE(select);
    if (mutation == 0) marker->setOperand(3, marker.getPayload());
    if (mutation == 1) select->setOperand(1, marker.getPrevious());
    if (mutation == 2) select->setOperand(2, marker.getPayload());
    if (mutation == 3) {
      auto proposal = marker.getPayload().getDefiningOp<ac::SourceMethodCallOp>();
      ASSERT_TRUE(proposal);
      marker->setOperand(1, proposal.getPath());
    }
    rejects(*file);
  }
}

TEST_F(SourceFormalOwnerBindingTest, ByValueFramesStayNonOwningAndReplayRejectsDowngrading) {
  auto file = fixture();
  ASSERT_TRUE(file) << diagnostics;
  ac::SourceTransactionStateAnalysis analysis(*file);
  ASSERT_TRUE(analysis.isValid()) << diagnostics;
  bool owned = false, byValue = false;
  for (auto transaction : all<ac::SourceTransactionOp>(*file)) {
    auto *plan = analysis.lookup(transaction);
    ASSERT_TRUE(plan);
    for (const auto &write : plan->formalWrites) {
      if (write.owner) owned = true;
      else { byValue = true; EXPECT_FALSE(write.write); }
    }
  }
  EXPECT_TRUE(owned);
  EXPECT_TRUE(byValue);
  ac::HardwareSourceCheckLimits limits;
  ac::detail::SourceCheckBudget preparationBudget(limits);
  auto prepared = acir::detail::prepareTransactionLowering(*file, analysis, preparationBudget);
  ASSERT_TRUE(succeeded(prepared)) << diagnostics;
  ac::detail::SourceCheckBudget materializationBudget(limits);
  auto candidate = acir::detail::materializeTransactionLowering(*prepared, materializationBudget);
  ASSERT_TRUE(succeeded(candidate)) << diagnostics;
  ac::detail::SourceCheckBudget initialProof(limits);
  ASSERT_TRUE(succeeded(acir::detail::verifyTransactionMaterialization(
      *prepared, *candidate, initialProof))) << diagnostics;
  auto call = ownerCall(*file);
  ASSERT_TRUE(call);
  auto bindings = call->getAttrOfType<ArrayAttr>("ac.formal_owner_bindings");
  for (bool empty : {false, true}) {
    SCOPED_TRACE(empty);
    if (empty) call->setAttr("ac.formal_owner_bindings", builder.getArrayAttr({}));
    else call->removeAttr("ac.formal_owner_bindings");
    diagnostics.clear();
    ac::detail::SourceCheckBudget proofBudget(limits);
    EXPECT_TRUE(failed(acir::detail::verifyTransactionMaterialization(
        *prepared, *candidate, proofBudget)));
    EXPECT_FALSE(diagnostics.empty());
    call->setAttr("ac.formal_owner_bindings", bindings);
  }
}

TEST_F(SourceFormalOwnerBindingTest, PartialFormalCandidatesUseCurrentOwningAndByValueFrames) {
  for (bool table : {false, true}) {
    SCOPED_TRACE(table);
    auto file = selectedFixture(table);
    ASSERT_TRUE(file) << diagnostics;
    ac::SourceTransactionStateAnalysis analysis(*file);
    ASSERT_TRUE(analysis.isValid()) << diagnostics;
    auto markers = selectedWrites(*file);
    ASSERT_EQ(markers.size(), 3u);
    EXPECT_NE(markers[0].getCandidate(), markers[1].getCandidate());
    bool owned = false, byValue = false;
    unsigned reached = 0;
    for (auto transaction : all<ac::SourceTransactionOp>(*file)) {
      auto *plan = analysis.lookup(transaction);
      ASSERT_TRUE(plan);
      for (const auto &write : plan->formalWrites) {
        if (write.operation->getAttrOfType<ArrayAttr>("selectors").empty()) continue;
        ++reached;
        if (write.owner) owned = true;
        else { byValue = true; EXPECT_FALSE(write.write); }
      }
    }
    EXPECT_GE(reached, 9u); // Three writes reached through three nested calls.
    EXPECT_TRUE(owned);
    EXPECT_TRUE(byValue);
  }
}

TEST_F(SourceFormalOwnerBindingTest, TableFormalCandidateAuthenticatesAssignmentTimeIndexAndRHS) {
  // A four-bit index needs sixteen entries for a valid generic source.
  // This dedicated shape keeps index/RHS substitutions type-correct.
  auto original = selectedFixture(true, 16);
  ASSERT_TRUE(original) << diagnostics;
  ac::SourceTransactionStateAnalysis baseline(*original);
  ASSERT_TRUE(baseline.isValid()) << diagnostics;
  for (unsigned mutation = 0; mutation != 6; ++mutation) {
    SCOPED_TRACE(mutation);
    OwningOpRef<ModuleOp> file(cast<ModuleOp>(original->clone()));
    auto markers = selectedWrites(*file);
    ASSERT_EQ(markers.size(), 3u);
    auto marker = markers[0];
    auto map = marker.getCandidate().getDefiningOp<ac::TableMapOp>();
    ASSERT_TRUE(map);
    auto &body = map.getBody().front();
    auto merge = cast<ac::YieldOp>(body.getTerminator()).getOperand(0)
                     .getDefiningOp<ac::ValueMergeOp>();
    ASSERT_TRUE(merge);
    auto compare = merge.getGuards()[0].getDefiningOp<ac::BitsCompareOp>();
    ASSERT_TRUE(compare);
    if (mutation == 0) {
      // Equal logical widths make this a schema-valid index/RHS substitution.
      auto splat = map.getTables()[1].getDefiningOp<ac::TableSplatOp>();
      ASSERT_TRUE(splat);
      splat->setOperand(0, marker.getPayload());
    }
    if (mutation == 1) {
      auto splat = map.getTables()[2].getDefiningOp<ac::TableSplatOp>();
      ASSERT_TRUE(splat);
      splat->setOperand(0, marker.getIndices()[0]);
    }
    if (mutation == 2) {
      compare->setAttr("predicate", builder.getStringAttr("case_eq"));
    }
    if (mutation == 3) {
      merge->setAttr("paths", builder.getArrayAttr({builder.getArrayAttr({builder.getStringAttr("sibling")})}));
    }
    if (mutation == 4) {
      marker->setAttr("selectors", builder.getArrayAttr({
          builder.getDictionaryAttr({builder.getNamedAttr("index", builder.getI64IntegerAttr(0))}),
          builder.getDictionaryAttr({builder.getNamedAttr("field", builder.getStringAttr("sibling"))})}));
    }
    if (mutation == 5) {
      // Change both compare operands to their one-bit ordinal domain; the
      // four-bit source index's high bits must remain part of equality.
      OpBuilder b(compare);
      auto makeExtract = [&](Value input) {
        OperationState state(compare.getLoc(), ac::BitsExtractOp::getOperationName());
        state.addOperands(input);
        auto width = cast<ac::BitsType>(body.getArgument(0).getType()).getWidth();
        NamedAttrList tree(width.getTree());
        auto integer = cast<DictionaryAttr>(tree.get("value"));
        tree.set("value", changed(integer, "value", ac::MathIntAttr::get(&context, llvm::APSInt("1"))));
        state.addTypes(ac::BitsType::get(&context, ac::StaticExprAttr::get(&context, tree.getDictionary(&context))));
        tree.set("value", changed(integer, "value", ac::MathIntAttr::get(&context, llvm::APSInt("0"))));
        state.addAttribute("low", ac::StaticExprAttr::get(&context, tree.getDictionary(&context)));
        return b.create(state)->getResult(0);
      };
      compare->setOperands({makeExtract(body.getArgument(0)), makeExtract(body.getArgument(2))});
    }
    rejectsSelectedCandidate(*file);
  }
}

TEST_F(SourceFormalOwnerBindingTest, SelectedFormalCandidatesRejectStaleOrForeignReconstruction) {
  for (bool table : {false, true}) {
    auto original = selectedFixture(table);
    ASSERT_TRUE(original) << diagnostics;
    ac::SourceTransactionStateAnalysis baseline(*original);
    ASSERT_TRUE(baseline.isValid()) << diagnostics;
    for (unsigned mutation = 0; mutation != 3; ++mutation) {
      SCOPED_TRACE(::testing::Message() << "table=" << table << " mutation=" << mutation);
      OwningOpRef<ModuleOp> file(cast<ModuleOp>(original->clone()));
      auto markers = selectedWrites(*file);
      ASSERT_EQ(markers.size(), 3u);
      if (mutation == 0) {
        if (table) {
          auto first = markers[0].getCandidate().getDefiningOp<ac::TableMapOp>();
          auto second = markers[1].getCandidate().getDefiningOp<ac::TableMapOp>();
          ASSERT_TRUE(first && second);
          second->setOperand(0, first.getTables()[0]);
        } else {
          auto first = markers[0].getCandidate().getDefiningOp<ac::StructCreateOp>();
          auto second = markers[1].getCandidate().getDefiningOp<ac::StructCreateOp>();
          ASSERT_TRUE(first && second);
          // Restore the old tail projection instead of the current candidate.
          second->setOperand(1, first.getValues()[1]);
        }
      }
      if (mutation == 1) {
        // A later formal may not reuse an earlier formal's reconstructed value.
        markers[2]->setOperand(2, markers[1].getCandidate());
      }
      if (mutation == 2) {
        if (table) {
          auto map = markers[0].getCandidate().getDefiningOp<ac::TableMapOp>();
          ASSERT_TRUE(map);
          auto &body = map.getBody().front();
          auto merge = cast<ac::YieldOp>(body.getTerminator()).getOperand(0).getDefiningOp<ac::ValueMergeOp>();
          ASSERT_TRUE(merge);
          OpBuilder b(merge);
          OperationState state(merge.getLoc(), ac::StructCreateOp::getOperationName());
          state.addOperands({body.getArgument(3), body.getArgument(3)});
          state.addTypes(body.getArgument(1).getType());
          auto clobbered = b.create(state)->getResult(0);
          merge->setOperand(0, clobbered);
        } else {
          auto create = markers[0].getCandidate().getDefiningOp<ac::StructCreateOp>();
          ASSERT_TRUE(create);
          create->setOperand(1, markers[0].getPayload());
        }
      }
      rejectsSelectedCandidate(*file);
    }
  }
}

TEST_F(SourceFormalOwnerBindingTest, WholeFormalCandidateMustBeActualRHS) {
  auto file = selectedFixture(false);
  ASSERT_TRUE(file) << diagnostics;
  ac::SourceTransactionStateAnalysis baseline(*file);
  ASSERT_TRUE(baseline.isValid()) << diagnostics;
  ac::SourceFormalWriteOp whole;
  for (auto marker : all<ac::SourceFormalWriteOp>(*file))
    if (!marker.getPrevious() && marker.getSelectors().empty()) whole = marker;
  ASSERT_TRUE(whole);
  auto method = whole->getParentOfType<ac::SourceMethodOp>();
  ASSERT_TRUE(method);
  whole->setOperand(2, method.getBody().front().getArgument(0));
  rejectsSelectedCandidate(*file, "whole formal assignment candidate");
}
} // namespace
