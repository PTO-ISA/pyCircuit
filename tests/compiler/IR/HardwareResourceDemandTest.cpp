// Independent resource-demand authentication starts from production-captured
// source. Serialized admission rows never supply the expected demand graph.
#include "Compiler/SourceLink.h"
#include "Compiler/SourceUnit.h"
#include "Transforms/LowerTransactions.h"
#include "Dialect/ACIR/HardwareSampledAdmission.h"
#include "Dialect/ACIR/HardwareQueueResources.h"
#include "Dialect/ACIR/HardwareSourceCheckPreflight.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceTransactionStateAnalysis.h"
#include "pycircuit/Transforms/Passes.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/Pass.h"
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
namespace ac = acir::ac;
namespace compiler = acir::compiler;
using namespace mlir;

class HardwareResourceDemandTest : public ::testing::Test {
protected:
  MLIRContext context;
  Builder attrs{&context};
  llvm::SmallString<256> directory;
  std::string diagnostics;
  std::unique_ptr<ScopedDiagnosticHandler> handler;
  void SetUp() override {
    context.loadDialect<ac::ACIRDialect>();
    ASSERT_FALSE(
        llvm::sys::fs::createUniqueDirectory("resource-demand-proof", directory));
    handler =
        std::make_unique<ScopedDiagnosticHandler>(&context, [&](Diagnostic &d) {
          llvm::raw_string_ostream out(diagnostics);
          d.print(out);
          diagnostics += '\n';
          return success();
        });
  }
  void TearDown() override { llvm::sys::fs::remove_directories(directory); }
  std::string path(StringRef file) {
    llvm::SmallString<256> result(directory);
    llvm::sys::path::append(result, file);
    return result.str().str();
  }
  auto error() {
    return [&] { return emitError(attrs.getUnknownLoc()); };
  }
  OwningOpRef<ModuleOp> fixture(bool pureState = false, bool localPath = false,
                               bool prospective = false, bool rawBound = false,
                               unsigned wrapper = 0,
                               compiler::SourceLinkStages *stages = nullptr) {
    std::string fixtureName = wrapper ? "resource_demand_wrapper.py"
        : rawBound ? "resource_demand_raw_bound_bits1.py"
        : prospective ? "resource_demand_prospective.py"
        : localPath ? "resource_demand_local_path.py"
        : pureState ? "resource_demand_pure_state.py" : "resource_demand.py";
    static constexpr llvm::StringLiteral script = R"py(
import sys
from pathlib import Path
repo, output = map(Path, sys.argv[1:3])
sys.path.insert(0, str(repo / "python"))
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport
name = sys.argv[3]
if name == "resource_demand_prospective.py":
    fixtures = repo / "tests/system/fixtures/source_system"
    text = (fixtures / "rule_queue_hierarchy_provider.py").read_text()
    text += "\n" + (fixtures / "rule_queue_hierarchy_bench.py").read_text().replace(
        "from checks.rule_queue_hierarchy_provider import HierarchyRelay\n", "").replace(
        "import pycircuit as ac\n", "")
else:
    original = repo / "tests/compiler/lit/Source/Inputs/source_check_execution" / name
    text = original.read_text()
    if name == "resource_demand_wrapper.py" and sys.argv[4] == "2":
        assert text.count("child.push(7)") == 1
        text = text.replace("child.push(7)", "value = child.take()")
source = output.with_name(name)
source.write_text(text)
output.write_text(_emit_source_transport(_capture_source_file(source, source_root=source.parent)), encoding="utf-8")
)py";
    std::vector<std::string> owned{
        PYCIRCUIT_TEST_PYTHON, "-c",
        script.str(),          PYCIRCUIT_TEST_REPO_ROOT,
        path("capture.mlir"), fixtureName, std::to_string(wrapper)};
    SmallVector<StringRef> arguments;
    for (auto &value : owned)
      arguments.push_back(value);
    auto log = path("capture.log");
    std::array<std::optional<StringRef>, 3> redirects{std::nullopt, log, log};
    int status =
        llvm::sys::ExecuteAndWait(owned[0], arguments, std::nullopt, redirects);
    auto output = llvm::MemoryBuffer::getFile(log);
    EXPECT_EQ(status, 0) << (output ? output.get()->getBuffer().str() : log);
    if (status)
      return {};
    auto captured = parseSourceFile<ModuleOp>(path("capture.mlir"), &context);
    if (!captured)
      return {};
    auto headers =
        compiler::SourceHeaderRegistry::create(&context, {}, error());
    if (failed(headers))
      return {};
    auto owner = attrs.getDictionaryAttr(
        {attrs.getNamedAttr("package", attrs.getStringAttr("execution")),
         attrs.getNamedAttr("path", attrs.getStringAttr(fixtureName))});
    auto unit =
        compiler::compilePythonSourceUnit(*captured, owner, *headers, error());
    EXPECT_TRUE(succeeded(unit)) << diagnostics;
    if (failed(unit))
      return {};
    if (localPath) {
      // Python intentionally has no raw Queue-path argument. Exercise the
      // admitted typed source IR with an unused conditional peek, and publish
      // a newly derived interface through the ordinary owners before link.
      unsigned changed = 0;
      unit->body->walk([&](ac::SourceMethodCallOp call) {
        if (call.getCalleeAttr().getLeafReference().getValue() != "peek")
          return;
        auto method = call->getParentOfType<ac::SourceMethodOp>();
        if (!method || method.getBody().front().getNumArguments() == 0)
          return;
        call->setOperand(0, method.getBody().front().getArgument(0));
        ++changed;
      });
      EXPECT_EQ(changed, 1u);
      if (changed != 1)
        return {};
      PassManager summaries(&context);
      summaries.addPass(acir::createAnalyzeSourceMethodSummariesPass());
      if (failed(summaries.run(*unit->body)))
        return {};
      unit->interface = OwningOpRef<ModuleOp>(cast<ModuleOp>(unit->body->clone()));
      PassManager publication(&context);
      publication.addPass(acir::createExtractSourceInterfacePass());
      if (failed(publication.run(*unit->interface)))
        return {};
    }
    auto final =
        compiler::linkSourceUnits({{*unit->body, *unit->interface}},
                                    wrapper ? "execution.resource_demand_wrapper.Top"
                  : rawBound ? "execution.resource_demand_raw_bound_bits1.Top"
                  : prospective ? "execution.resource_demand_prospective.ExerciseQueueHierarchy"
                  : localPath ? "execution.resource_demand_local_path.Top"
                  : pureState ? "execution.resource_demand_pure_state.Top"
                              : "execution.resource_demand.Top", error(),
                                 compiler::SourceLinkTarget::Hardware, stages);
    EXPECT_TRUE(succeeded(final)) << diagnostics;
    return succeeded(final) ? std::move(*final) : OwningOpRef<ModuleOp>{};
  }
  struct StagedQueue {
    OwningOpRef<ModuleOp> original;
    std::unique_ptr<ac::SourceTransactionStateAnalysis> analysis;
    acir::detail::PreparedTransactionLowering prepared;
    acir::detail::MaterializedTransactionLowering candidate;
  };
  std::unique_ptr<StagedQueue> stageQueue(bool prospective, bool rawBound) {
    compiler::SourceLinkStages stages;
    auto linked = fixture(false, false, prospective, rawBound, 0, &stages);
    if (!linked) return {};
    // Re-enter the existing native seam from the actual retained source stage;
    // no separately authored queue graph supplies the expected product.
    auto original = parseSourceString<ModuleOp>(stages.beforeTransactions, &context);
    if (!original) return {};
    auto analysis = std::make_unique<ac::SourceTransactionStateAnalysis>(
        original->getOperation());
    EXPECT_TRUE(analysis->isValid()) << diagnostics;
    if (!analysis->isValid()) return {};
    ac::HardwareSourceCheckLimits limits;
    ac::detail::SourceCheckBudget preparationBudget(limits);
    auto prepared = acir::detail::prepareTransactionLowering(
        *original, *analysis, preparationBudget);
    EXPECT_TRUE(succeeded(prepared)) << diagnostics;
    if (failed(prepared)) return {};
    ac::detail::SourceCheckBudget materializationBudget(limits);
    auto candidate = acir::detail::materializeTransactionLowering(
        *prepared, materializationBudget);
    EXPECT_TRUE(succeeded(candidate)) << diagnostics;
    if (failed(candidate)) return {};
    auto staged = std::make_unique<StagedQueue>();
    staged->original = std::move(original);
    staged->analysis = std::move(analysis);
    staged->prepared = std::move(*prepared);
    staged->candidate = std::move(*candidate);
    return staged;
  }
  LogicalResult verifyDirect(StagedQueue &staged) {
    ac::detail::SourceCheckBudget budget(ac::HardwareSourceCheckLimits{});
    diagnostics.clear();
    return acir::detail::verifyTransactionMaterialization(
        staged.prepared, staged.candidate, budget);
  }
  std::string ir(ModuleOp file) {
    std::string text;
    llvm::raw_string_ostream output(text);
    file.print(output);
    return text;
  }
  std::vector<Operation *> objects(ModuleOp file) {
    std::vector<Operation *> result;
    file.walk([&](Operation *operation) { result.push_back(operation); });
    return result;
  }
  void directRejectsWithoutMutation(StagedQueue &staged) {
    auto file = staged.candidate.module.get();
    ac::HardwareAnalysis analysis(file);
    bool parsed = true;
    file.walk([&](ac::RuleOp rule) {
      if (!rule->hasAttr("ac.sampled_admission")) return;
      ac::HardwareBindings bindings;
      bindings.owner = rule->getParentOp();
      ac::detail::SourceCheckBudget budget(ac::HardwareSourceCheckLimits{});
      parsed &= succeeded(ac::detail::parseSampledAdmission(
          rule, analysis, bindings, budget));
    });
    ASSERT_TRUE(parsed) << diagnostics;
    auto sourceText = ir(*staged.original), candidateText = ir(file);
    auto sourceObjects = objects(*staged.original), candidateObjects = objects(file);
    EXPECT_TRUE(failed(verifyDirect(staged)));
    EXPECT_FALSE(diagnostics.empty());
    EXPECT_EQ(sourceText, ir(*staged.original));
    EXPECT_EQ(candidateText, ir(file));
    EXPECT_EQ(sourceObjects, objects(*staged.original));
    EXPECT_EQ(candidateObjects, objects(file));
  }
  LogicalResult verifyFinal(ModuleOp file) {
    ac::HardwareAnalysis analysis(file);
    auto plan = analysis.getSourceCheckPlan();
    if (failed(plan))
      return failure();
    EXPECT_FALSE(plan->checks.empty());
    EXPECT_GE(plan->commits.size(), 1u);
    return analysis.verifySourceCheckPlan(*plan);
  }
  struct Graph {
    ac::RuleOp rule;
    Value demand, mayExecute, fire;
    SmallVector<ac::BitsBinaryOp> reached;
    SmallVector<Value> selectors, predicates;
  };
  static SmallVector<Operation *> operations(Value root) {
    SmallVector<Operation *> result;
    SmallVector<Value> pending{root};
    llvm::DenseSet<Operation *> seen;
    while (!pending.empty()) {
      Operation *operation = pending.pop_back_val().getDefiningOp();
      if (!operation || !seen.insert(operation).second)
        continue;
      result.push_back(operation);
      llvm::append_range(pending, operation->getOperands());
    }
    return result;
  }
  Graph graph(ModuleOp file) {
    ac::RuleOp selected;
    file.walk([&](ac::RuleOp rule) {
      auto row = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
      auto resources = row ? row.getAs<ArrayAttr>("resources") : ArrayAttr();
      if (!selected && resources && resources.size() >= 3)
        selected = rule;
    });
    if (!selected)
      return {};
    ac::HardwareAnalysis analysis(file);
    ac::HardwareBindings bindings;
    bindings.owner = selected->getParentOp();
    ac::detail::SourceCheckBudget budget({1 << 22});
    auto view = ac::detail::parseSampledAdmission(selected, analysis, bindings,
                                                budget);
    EXPECT_TRUE(succeeded(view)) << diagnostics;
    if (failed(view))
      return {};
    Graph result;
    result.rule = selected;
    result.fire = view->fire;
    for (const auto &control : view->controls) {
      auto wrapped = control.reached.getDefiningOp<ac::BitsBinaryOp>();
      EXPECT_TRUE(wrapped && wrapped.getOpcode() == "and");
      if (!wrapped || wrapped.getOpcode() != "and")
        return {};
      if (!result.mayExecute)
        result.mayExecute = wrapped.getRhs();
      EXPECT_EQ(wrapped.getRhs(), result.mayExecute);
      result.reached.push_back(wrapped);
      if (control.kind == ac::detail::SampledControlKind::BranchSelect)
        result.selectors.push_back(control.raw);
    }
    auto invert = result.mayExecute.getDefiningOp<ac::BitsUnaryOp>();
    auto compare = invert ? invert.getInput().getDefiningOp<ac::BitsCompareOp>()
                          : ac::BitsCompareOp();
    EXPECT_TRUE(invert && invert.getOpcode() == "not" && compare &&
                compare.getPredicate() == "case_eq");
    if (!invert || !compare)
      return {};
    result.demand = compare.getLhs();
    for (const auto &resource : view->resources) {
      auto raw = view->controls[resource.control].raw;
      auto select = raw.getDefiningOp<ac::BitsSelectOp>();
      EXPECT_TRUE(select);
      if (select && !llvm::is_contained(result.predicates, select.getTrueValue()))
        result.predicates.push_back(select.getTrueValue());
    }
    return result;
  }
  void rejects(ModuleOp file) {
    // Keep the serialized schema valid. The owning final proof must reject the
    // changed SSA, rather than a misspelled metadata field or malformed row.
    ac::HardwareAnalysis analysis(file);
    bool parsed = true;
    file.walk([&](ac::RuleOp rule) {
      if (!rule->hasAttr("ac.sampled_admission"))
        return;
      ac::HardwareBindings bindings;
      bindings.owner = rule->getParentOp();
      ac::detail::SourceCheckBudget budget({1 << 22});
      parsed &= succeeded(ac::detail::parseSampledAdmission(
          rule, analysis, bindings, budget));
    });
    ASSERT_TRUE(parsed) << diagnostics;
    diagnostics.clear();
    EXPECT_TRUE(failed(verifyFinal(file)));
    EXPECT_FALSE(diagnostics.empty());
  }
  Value constant(OpBuilder &b, Type type, unsigned value) {
    auto bits = cast<ac::BitsType>(type);
    NamedAttrList tree(bits.getWidth().getTree());
    tree.set("value", attrs.getDictionaryAttr({
                          attrs.getNamedAttr("kind", attrs.getStringAttr("integer")),
                          attrs.getNamedAttr("value", ac::MathIntAttr::get(
                              &context, llvm::APSInt(std::to_string(value))))}));
    OperationState state(b.getUnknownLoc(), ac::BitsConstantOp::getOperationName());
    state.addTypes(type);
    state.addAttribute("value", ac::StaticExprAttr::get(
                                    &context, tree.getDictionary(&context)));
    return b.create(state)->getResult(0);
  }
  Value compare(OpBuilder &b, Value lhs, Value rhs) {
    OperationState state(b.getUnknownLoc(), ac::BitsCompareOp::getOperationName());
    state.addOperands({lhs, rhs});
    state.addTypes(lhs.getType());
    state.addAttribute("predicate", attrs.getStringAttr("case_eq"));
    return b.create(state)->getResult(0);
  }
  static ac::BitsBinaryOp leaf(const Graph &graph) {
    for (Operation *operation : operations(graph.demand)) {
      auto binary = dyn_cast<ac::BitsBinaryOp>(operation);
      if (binary && binary.getOpcode() == "and" &&
          llvm::is_contained(graph.predicates, binary.getLhs()))
        return binary;
    }
    return {};
  }
  static ac::BitsSelectOp branch(const Graph &graph) {
    for (Operation *operation : operations(graph.demand)) {
      auto select = dyn_cast<ac::BitsSelectOp>(operation);
      if (select && llvm::is_contained(graph.selectors, select.getCondition()))
        return select;
    }
    return {};
  }
};

TEST_F(HardwareResourceDemandTest,
       TypedUnusedConditionalAccessRetainsItsRawLocalPathChoice) {
  auto file = fixture(false, true);
  ASSERT_TRUE(file) << diagnostics;
  EXPECT_TRUE(succeeded(verifyFinal(*file))) << diagnostics;
  bool found = false;
  file->walk([&](ac::RuleOp rule) {
    auto sampled = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    auto resources = sampled ? sampled.getAs<ArrayAttr>("resources") : ArrayAttr();
    if (!resources || resources.empty())
      return;
    auto yielded = cast<ac::YieldOp>(rule.getBody().front().getTerminator());
    auto controls = sampled.getAs<ArrayAttr>("controls");
    for (Attribute raw : controls)
      EXPECT_NE(cast<DictionaryAttr>(raw).getAs<StringAttr>("kind").getValue(),
                "branch_select");
    auto first = sampled.getAs<IntegerAttr>("fire_result").getInt() + 2;
    auto wrapper = yielded.getOperand(first).getDefiningOp<ac::BitsBinaryOp>();
    ASSERT_TRUE(wrapper);
    auto inverse = wrapper.getRhs().getDefiningOp<ac::BitsUnaryOp>();
    ASSERT_TRUE(inverse);
    auto tested = inverse.getInput().getDefiningOp<ac::BitsCompareOp>();
    ASSERT_TRUE(tested);
    for (Operation *operation : operations(tested.getLhs())) {
      auto selected = dyn_cast<ac::BitsSelectOp>(operation);
      if (selected && !selected.getCondition().getDefiningOp<ac::BitsCompareOp>() &&
          !selected.getCondition().getDefiningOp<ac::BitsConstantOp>())
        found = true;
    }
  });
  EXPECT_TRUE(found);
}

TEST_F(HardwareResourceDemandTest,
       PureStateBranchRetainsPathQualifiedAvailabilityWithoutEffectSelection) {
  auto file = fixture(true);
  ASSERT_TRUE(file) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*file))) << diagnostics;
  bool found = false;
  file->walk([&](ac::RuleOp rule) {
    auto sampled = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    auto resources = sampled ? sampled.getAs<ArrayAttr>("resources") : ArrayAttr();
    if (!resources || resources.empty())
      return;
    bool availability = false, selection = false;
    for (Attribute raw : sampled.getAs<ArrayAttr>("controls")) {
      auto kind = cast<DictionaryAttr>(raw).getAs<StringAttr>("kind").getValue();
      availability |= kind == "value_available";
      selection |= kind == "branch_select";
    }
    EXPECT_FALSE(selection);
    found |= availability;
  });
  EXPECT_TRUE(found);
}

TEST_F(HardwareResourceDemandTest,
       ProductionDemandUsesRawBranchesBareResourcesAndPrivateReachedGates) {
  auto file = fixture();
  ASSERT_TRUE(file) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*file))) << diagnostics;
  auto actual = graph(*file);
  ASSERT_TRUE(actual.rule);
  EXPECT_GE(actual.predicates.size(), 3u);
  EXPECT_TRUE(leaf(actual));
  EXPECT_TRUE(branch(actual));
  EXPECT_FALSE(llvm::is_contained(operations(actual.fire),
                                 actual.mayExecute.getDefiningOp()));
  for (OpOperand &use : actual.mayExecute.getUses())
    EXPECT_TRUE(llvm::is_contained(actual.reached,
                                   dyn_cast<ac::BitsBinaryOp>(use.getOwner())));
}

TEST_F(HardwareResourceDemandTest, DiagnosticGateCannotDriveFireOrQueueProposal) {
  auto original = fixture();
  ASSERT_TRUE(original) << diagnostics;
  for (bool fire : {false, true}) {
    SCOPED_TRACE(fire);
    OwningOpRef<ModuleOp> file(cast<ModuleOp>(original->clone()));
    auto actual = graph(*file);
    ASSERT_TRUE(actual.rule);
    auto metadata = actual.rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    auto yielded = cast<ac::YieldOp>(actual.rule.getBody().front().getTerminator());
    unsigned slot = metadata.getAs<IntegerAttr>("fire_result").getInt();
    if (!fire) {
      auto proposals = metadata.getAs<ArrayAttr>("queue_proposals");
      ASSERT_TRUE(proposals && !proposals.empty());
      slot = cast<DictionaryAttr>(proposals[0]).getAs<IntegerAttr>("path").getInt();
    }
    yielded->setOperand(slot, actual.mayExecute);
    rejects(*file);
  }
}

TEST_F(HardwareResourceDemandTest, ForgedZeroDemandCannotSuppressAllChecks) {
  auto file = fixture();
  ASSERT_TRUE(file) << diagnostics;
  auto actual = graph(*file);
  ASSERT_TRUE(actual.rule);
  auto inverted = actual.mayExecute.getDefiningOp<ac::BitsUnaryOp>();
  auto tested = inverted.getInput().getDefiningOp<ac::BitsCompareOp>();
  ASSERT_TRUE(tested);
  OpBuilder b(tested);
  tested->setOperand(0, constant(b, tested.getLhs().getType(), 0));
  rejects(*file);
}

TEST_F(HardwareResourceDemandTest, EveryReachedControlRequiresItsDemandWrapper) {
  auto original = fixture();
  ASSERT_TRUE(original) << diagnostics;
  for (bool first : {false, true}) {
    SCOPED_TRACE(first);
    OwningOpRef<ModuleOp> file(cast<ModuleOp>(original->clone()));
    auto actual = graph(*file);
    ASSERT_TRUE(actual.rule);
    auto wrapper = first ? actual.reached.front() : actual.reached.back();
    wrapper->getResult(0).replaceAllUsesWith(wrapper.getLhs());
    rejects(*file);
  }
}

TEST_F(HardwareResourceDemandTest, ForeignMayExecuteCannotReplaceOneWrapper) {
  auto file = fixture();
  ASSERT_TRUE(file) << diagnostics;
  auto actual = graph(*file);
  ASSERT_TRUE(actual.rule);
  auto wrapper = actual.reached.back();
  OpBuilder b(wrapper);
  auto one = constant(b, wrapper.getLhs().getType(), 1);
  auto zero = constant(b, wrapper.getLhs().getType(), 0);
  wrapper->setOperand(1, compare(b, one, zero));
  rejects(*file);
}

TEST_F(HardwareResourceDemandTest, ResourceLeafCannotUseSiblingQueueOrBeOmitted) {
  auto original = fixture();
  ASSERT_TRUE(original) << diagnostics;
  for (bool omit : {false, true}) {
    SCOPED_TRACE(omit);
    OwningOpRef<ModuleOp> file(cast<ModuleOp>(original->clone()));
    auto actual = graph(*file);
    ASSERT_TRUE(actual.rule);
    auto resource = leaf(actual);
    ASSERT_TRUE(resource);
    if (omit)
      resource->getResult(0).replaceAllUsesWith(resource.getRhs());
    else {
      Value sibling;
      for (Value predicate : actual.predicates)
        if (predicate != resource.getLhs())
          sibling = predicate;
      ASSERT_TRUE(sibling);
      resource->setOperand(0, sibling);
    }
    rejects(*file);
  }
}

TEST_F(HardwareResourceDemandTest, RawBranchCannotBecomeKnownArmSelection) {
  auto file = fixture();
  ASSERT_TRUE(file) << diagnostics;
  auto actual = graph(*file);
  ASSERT_TRUE(actual.rule);
  auto selected = branch(actual);
  ASSERT_TRUE(selected);
  OpBuilder b(selected);
  auto one = constant(b, selected.getCondition().getType(), 1);
  selected->setOperand(0, compare(b, selected.getCondition(), one));
  rejects(*file);
}

TEST_F(HardwareResourceDemandTest, GuardBypassAndResourceContinuationStayExact) {
  auto original = fixture();
  ASSERT_TRUE(original) << diagnostics;
  for (bool bypass : {false, true}) {
    SCOPED_TRACE(bypass);
    OwningOpRef<ModuleOp> file(cast<ModuleOp>(original->clone()));
    auto actual = graph(*file);
    ASSERT_TRUE(actual.rule);
    ac::BitsSelectOp guarded;
    for (Operation *operation : operations(actual.demand)) {
      auto choice = dyn_cast<ac::BitsSelectOp>(operation);
      auto predicate = choice
                           ? choice.getCondition().getDefiningOp<ac::BitsCompareOp>()
                           : ac::BitsCompareOp();
      if (predicate && predicate.getPredicate() == "case_eq" &&
          !llvm::is_contained(actual.selectors, choice.getCondition())) {
        guarded = choice;
        break;
      }
    }
    ASSERT_TRUE(guarded);
    if (bypass) {
      OpBuilder b(guarded);
      guarded->setOperand(2, constant(b, guarded.getFalseValue().getType(), 0));
    } else {
      auto arm = leaf(actual);
      ASSERT_TRUE(arm);
      // Adding the same resource ancestry again is not an authenticated source
      // continuation, even when a two-state Boolean identity would hide it.
      arm->setOperand(1, arm.getLhs());
    }
    rejects(*file);
  }
}


TEST_F(HardwareResourceDemandTest,
       ProspectiveHierarchyRetainsTwoInstancesAndSummaryOnlyDefinitions) {
  auto file = fixture(false, false, true);
  ASSERT_TRUE(file) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*file))) << diagnostics;
  unsigned summaryOnly = 0;
  bool unselected = false, twoInstances = false;
  ac::HardwareAnalysis analysis(*file);
  for (auto module : file->getBody()->getOps<ac::ModuleOp>()) {
    unselected |= module.getSymName().contains("UnselectedQueueHierarchy");
    auto queue = module->getAttrOfType<DictionaryAttr>("ac.queue_wiring");
    auto summaries = queue ? queue.getAs<ArrayAttr>("pop_summaries") : ArrayAttr();
    if (summaries && !summaries.empty() &&
        module.getBody().front().getOps<ac::RuleOp>().empty()) {
      auto transaction = module->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
      ASSERT_TRUE(transaction);
      EXPECT_EQ(transaction.size(), 3u);
      for (StringRef key : {"states", "incoming", "writers"}) {
        auto rows = transaction.getAs<ArrayAttr>(key);
        ASSERT_TRUE(rows);
        EXPECT_TRUE(rows.empty());
      }
      ++summaryOnly;
    }
    if (module.getSymName().contains("ExerciseQueueHierarchy")) {
      SmallVector<Operation *> callees;
      for (auto instance : module.getBody().front().getOps<ac::InstanceOp>()) {
        auto callee = analysis.resolveCallee(instance);
        if (callee && llvm::is_contained(callees, callee))
          twoInstances = true;
        callees.push_back(callee);
      }
    }
  }
  EXPECT_GT(summaryOnly, 0u);
  EXPECT_TRUE(unselected);
  EXPECT_TRUE(twoInstances);
}

TEST_F(HardwareResourceDemandTest,
       ProspectiveSummaryOnlyMetadataAndPrivateUsesAreAuthenticated) {
  auto original = fixture(false, false, true);
  ASSERT_TRUE(original) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*original))) << diagnostics;
  for (bool leak : {false, true}) {
    SCOPED_TRACE(leak);
    OwningOpRef<ModuleOp> file(cast<ModuleOp>(original->clone()));
    ac::ModuleOp selected;
    for (auto module : file->getBody()->getOps<ac::ModuleOp>()) {
      auto wiring = module->getAttrOfType<DictionaryAttr>("ac.queue_wiring");
      auto summaries = wiring ? wiring.getAs<ArrayAttr>("pop_summaries") : ArrayAttr();
      if (summaries && !summaries.empty() &&
          module.getBody().front().getOps<ac::RuleOp>().empty()) {
        selected = module;
        break;
      }
    }
    ASSERT_TRUE(selected);
    if (!leak)
      selected->removeAttr("ac.transaction_wiring");
    else {
      auto wiring = selected->getAttrOfType<DictionaryAttr>("ac.queue_wiring");
      auto summary = cast<DictionaryAttr>(wiring.getAs<ArrayAttr>("pop_summaries")[0]);
      unsigned output = summary.getAs<IntegerAttr>("output").getInt();
      auto yield = cast<ac::YieldOp>(selected.getBody().front().getTerminator());
      OpBuilder b(yield);
      OperationState state(b.getUnknownLoc(), ac::BitsUnaryOp::getOperationName());
      state.addOperands(yield.getOperand(output));
      state.addTypes(yield.getOperand(output).getType());
      state.addAttribute("opcode", attrs.getStringAttr("not"));
      b.create(state);
    }
    rejects(*file);
  }
}

TEST_F(HardwareResourceDemandTest,
       ProspectiveExternalCreditCannotAliasAnotherActualInstance) {
  auto file = fixture(false, false, true);
  ASSERT_TRUE(file) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*file))) << diagnostics;
  bool changed = false;
  for (auto module : file->getBody()->getOps<ac::ModuleOp>()) {
    auto wiring = module->getAttrOfType<DictionaryAttr>("ac.queue_wiring");
    auto external = wiring ? wiring.getAs<ArrayAttr>("external_pops") : ArrayAttr();
    if (!external)
      continue;
    for (unsigned i = 0; i < external.size() && !changed; ++i)
      for (unsigned j = i + 1; j < external.size() && !changed; ++j) {
        auto a = cast<DictionaryAttr>(external[i]), b = cast<DictionaryAttr>(external[j]);
        if (a.get("registration") != b.get("registration") || a.get("queue") == b.get("queue"))
          continue;
        for (auto rule : module.getBody().front().getOps<ac::RuleOp>()) {
          if (rule->getAttr("occurrence") != a.get("registration"))
            continue;
          unsigned left = a.getAs<IntegerAttr>("capture").getInt();
          unsigned right = b.getAs<IntegerAttr>("capture").getInt();
          Value saved = rule.getOperand(left);
          rule->setOperand(left, rule.getOperand(right));
          rule->setOperand(right, saved);
          changed = true;
        }
      }
  }
  ASSERT_TRUE(changed);
  rejects(*file);
}

TEST_F(HardwareResourceDemandTest,
       ProspectiveExternalInventoryCannotBeMissingEmptyOrDuplicate) {
  auto original = fixture(false, false, true);
  ASSERT_TRUE(original) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*original))) << diagnostics;
  for (unsigned mutation = 0; mutation != 3; ++mutation) {
    SCOPED_TRACE(mutation);
    OwningOpRef<ModuleOp> file(cast<ModuleOp>(original->clone()));
    bool changed = false;
    for (auto module : file->getBody()->getOps<ac::ModuleOp>()) {
      auto wiring = module->getAttrOfType<DictionaryAttr>("ac.queue_wiring");
      auto external = wiring ? wiring.getAs<ArrayAttr>("external_pops") : ArrayAttr();
      if (!external || external.empty())
        continue;
      NamedAttrList row(wiring);
      if (mutation == 0)
        row.erase("external_pops");
      else if (mutation == 1)
        row.set("external_pops", attrs.getArrayAttr({}));
      else {
        SmallVector<Attribute> duplicate(external.begin(), external.end());
        duplicate.push_back(external[0]);
        row.set("external_pops", attrs.getArrayAttr(duplicate));
      }
      module->setAttr("ac.queue_wiring", row.getDictionary(&context));
      changed = true;
      break;
    }
    ASSERT_TRUE(changed);
    rejects(*file);
  }
}


TEST_F(HardwareResourceDemandTest,
       ProspectiveCrossLevelPopExclusionRequiresBothActualSelectorWitnesses) {
  auto file = fixture(false, false, true);
  ASSERT_TRUE(file) << diagnostics;
  auto wiring = [&]() -> LogicalResult {
    ac::HardwareAnalysis analysis(*file);
    SmallVector<std::pair<ac::ModuleOp, ac::HardwareBindings>> scopes;
    for (auto module : file->getBody()->getOps<ac::ModuleOp>()) {
      ac::HardwareBindings bindings;
      bindings.owner = module;
      scopes.push_back({module, bindings});
    }
    ac::detail::SourceCheckBudget budget({1 << 22});
    return ac::detail::verifyQueueResourceWiring(analysis, scopes, budget);
  };
  ASSERT_TRUE(succeeded(wiring())) << diagnostics;
  bool changed = false;
  file->walk([&](ac::RuleOp rule) {
    if (changed)
      return;
    auto admission = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    auto controls = admission ? admission.getAs<ArrayAttr>("controls") : ArrayAttr();
    auto proposals = admission ? admission.getAs<ArrayAttr>("queue_proposals") : ArrayAttr();
    if (!controls || !proposals)
      return;
    std::optional<unsigned> selection;
    for (auto [i, control] : llvm::enumerate(controls))
      if (cast<DictionaryAttr>(control).getAs<StringAttr>("kind").getValue() == "branch_select")
        selection = i;
    if (!selection)
      return;
    SmallVector<Attribute> rows(proposals.begin(), proposals.end());
    for (unsigned i = 0; i < rows.size(); ++i) {
      auto row = cast<DictionaryAttr>(rows[i]);
      auto id = row.getAs<DictionaryAttr>("queue");
      auto owner = id.getAs<DictionaryAttr>("owner");
      if (row.getAs<StringAttr>("kind").getValue() != "pop" ||
          !owner.getAs<ArrayAttr>("instance_path").empty())
        continue;
      NamedAttrList proposal(row);
      proposal.set("excluded_controls", attrs.getArrayAttr({attrs.getI64IntegerAttr(*selection)}));
      rows[i] = proposal.getDictionary(&context);
      NamedAttrList updated(admission);
      updated.set("queue_proposals", attrs.getArrayAttr(rows));
      rule->setAttr("ac.sampled_admission", updated.getDictionary(&context));
      changed = true;
      break;
    }
  });
  ASSERT_TRUE(changed);
  diagnostics.clear();
  EXPECT_TRUE(failed(wiring()));
  EXPECT_NE(diagnostics.find("actual-instance proposals lack opposite paths"),
            std::string::npos) << diagnostics;
}

TEST_F(HardwareResourceDemandTest,
       PublicBits1PayloadMayEqualIndependentlyAuthoredRawBoundTake) {
  auto file = fixture(false, false, false, true);
  ASSERT_TRUE(file) << diagnostics;
  EXPECT_TRUE(succeeded(verifyFinal(*file))) << diagnostics;
  bool exactSharedPublicValue = false;
  for (auto module : file->getBody()->getOps<ac::ModuleOp>()) {
    if (module.getBody().front().getNumArguments() == 0)
      continue;
    for (auto queue : module.getBody().front().getOps<ac::InstanceOp>())
      if (ac::isQueueInstance(queue))
        exactSharedPublicValue |=
            queue.getInputs()[4] == module.getBody().front().getArgument(0);
  }
  EXPECT_TRUE(exactSharedPublicValue);
}

TEST_F(HardwareResourceDemandTest,
       PrivateRawBoundSummaryOutputCannotSupplyBits1Payload) {
  auto file = fixture(false, false, false, true);
  ASSERT_TRUE(file) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*file))) << diagnostics;
  ac::HardwareAnalysis analysis(*file);
  bool changed = false;
  for (auto module : file->getBody()->getOps<ac::ModuleOp>()) {
    Value summary;
    ac::InstanceOp sourceInstance;
    ac::ModuleOp sourceCallee;
    for (auto instance : module.getBody().front().getOps<ac::InstanceOp>()) {
      auto child = dyn_cast_or_null<ac::ModuleOp>(analysis.resolveCallee(instance));
      auto wiring = child ? child->getAttrOfType<DictionaryAttr>("ac.queue_wiring")
                          : DictionaryAttr();
      auto summaries = wiring ? wiring.getAs<ArrayAttr>("pop_summaries") : ArrayAttr();
      if (!summaries || summaries.empty())
        continue;
      unsigned output = cast<DictionaryAttr>(summaries[0]).getAs<IntegerAttr>("output").getInt();
      summary = instance.getResult(output);
      sourceInstance = instance;
      sourceCallee = child;
      break;
    }
    if (!summary)
      continue;
    for (auto rule : module.getBody().front().getOps<ac::RuleOp>()) {
      auto admission = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
      auto proposals = admission ? admission.getAs<ArrayAttr>("queue_proposals") : ArrayAttr();
      if (!proposals)
        continue;
      auto yield = cast<ac::YieldOp>(rule.getBody().front().getTerminator());
      for (Attribute raw : proposals) {
        auto proposal = cast<DictionaryAttr>(raw);
        auto data = proposal.getAs<IntegerAttr>("data");
        if (!data)
          continue;
        auto argument = dyn_cast<BlockArgument>(yield.getOperand(data.getInt()));
        if (!argument || argument.getOwner() != &rule.getBody().front())
          continue;
        // Locate the authored payload use by its source queue-push occurrence,
        // rather than treating equal Bits1 values as interchangeable roles.
        bool sourceUse = false;
        for (auto require : rule.getBody().front().getOps<ac::ValueRequireOp>())
          sourceUse |= require.getPayload() == argument &&
                       require.getOccurrence() == proposal.getAs<DictionaryAttr>("occurrence");
        auto capture = dyn_cast<OpResult>(rule.getOperand(argument.getArgNumber()));
        auto childYield = cast<ac::YieldOp>(sourceCallee.getBody().front().getTerminator());
        auto names = sourceCallee.getInputNames();
        std::optional<unsigned> publicInput;
        for (auto [i, name] : llvm::enumerate(names))
          if (cast<StringAttr>(name).getValue() == "x")
            publicInput = i;
        if (!sourceUse || !capture || capture.getOwner() != sourceInstance ||
            !publicInput || childYield.getOperand(capture.getResultNumber()) !=
                                sourceCallee.getBody().front().getArgument(*publicInput))
          continue;
        EXPECT_NE(capture, summary);
        EXPECT_TRUE(ac::areEquivalentHardwareTypes(argument.getType(), summary.getType()));
        rule->setOperand(argument.getArgNumber(), summary);
        changed = true;
        break;
      }
      if (changed)
        break;
    }
    if (changed)
      break;
  }
  ASSERT_TRUE(changed);
  rejects(*file);
}

TEST_F(HardwareResourceDemandTest,
       PureWrapperMayLeaveSharedChildPushSuffixDisabledWithoutParentWiring) {
  auto file = fixture(false, false, false, false, 1);
  ASSERT_TRUE(file) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*file))) << diagnostics;
  ac::ModuleOp wrapper;
  for (auto module : file->getBody()->getOps<ac::ModuleOp>())
    if (module.getSymName().contains("WrapperB"))
      wrapper = module;
  ASSERT_TRUE(wrapper);
  EXPECT_FALSE(wrapper->hasAttr("ac.queue_wiring"));
  auto instances = wrapper.getBody().front().getOps<ac::InstanceOp>();
  ASSERT_FALSE(instances.empty());
  auto instance = *instances.begin();
  ac::HardwareAnalysis analysis(*file);
  auto child = dyn_cast_or_null<ac::ModuleOp>(analysis.resolveCallee(instance));
  ASSERT_TRUE(child);
  auto wiring = child->getAttrOfType<DictionaryAttr>("ac.queue_wiring");
  ASSERT_TRUE(wiring);
  auto incoming = wiring.getAs<ArrayAttr>("incoming");
  ASSERT_TRUE(incoming && !incoming.empty());
  for (Attribute raw : incoming)
    EXPECT_EQ(cast<DictionaryAttr>(raw).getAs<StringAttr>("kind").getValue(), "push");
}

TEST_F(HardwareResourceDemandTest,
       ForgedPopSummaryCannotClaimAnExistingRoutedPublicPayloadExport) {
  auto file = fixture(false, false, false, true);
  ASSERT_TRUE(file) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*file))) << diagnostics;
  bool changed = false;
  for (auto module : file->getBody()->getOps<ac::ModuleOp>()) {
    auto wiring = module->getAttrOfType<DictionaryAttr>("ac.queue_wiring");
    auto summaries = wiring ? wiring.getAs<ArrayAttr>("pop_summaries") : ArrayAttr();
    auto routed = module->getAttrOfType<ArrayAttr>("ac.routed_output_validity");
    if (!summaries || summaries.empty() || !routed)
      continue;
    auto yield = cast<ac::YieldOp>(module.getBody().front().getTerminator());
    std::optional<unsigned> publicInput;
    for (auto [i, name] : llvm::enumerate(module.getInputNames()))
      if (cast<StringAttr>(name).getValue() == "x")
        publicInput = i;
    if (!publicInput)
      continue;
    for (Attribute raw : routed) {
      auto route = cast<DictionaryAttr>(raw);
      auto payload = route.getAs<IntegerAttr>("payload");
      if (!payload || yield.getOperand(payload.getInt()) !=
                          module.getBody().front().getArgument(*publicInput))
        continue;
      SmallVector<Attribute> rows(summaries.begin(), summaries.end());
      NamedAttrList summary(cast<DictionaryAttr>(rows[0]));
      EXPECT_NE(summary.get("output"), payload);
      summary.set("output", payload);
      rows[0] = summary.getDictionary(&context);
      NamedAttrList updated(wiring);
      updated.set("pop_summaries", attrs.getArrayAttr(rows));
      module->setAttr("ac.queue_wiring", updated.getDictionary(&context));
      changed = true;
      break;
    }
    if (changed)
      break;
  }
  ASSERT_TRUE(changed);
  rejects(*file);
}

TEST_F(HardwareResourceDemandTest,
       PublicRawBoundAliasStillRequiresItsOwnPayloadAvailability) {
  auto original = fixture(false, false, false, true);
  ASSERT_TRUE(original) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*original))) << diagnostics;
  for (bool omit : {false, true}) {
    SCOPED_TRACE(omit);
    OwningOpRef<ModuleOp> file(cast<ModuleOp>(original->clone()));
    bool changed = false;
    for (auto module : file->getBody()->getOps<ac::ModuleOp>()) {
      auto routed = module->getAttrOfType<ArrayAttr>("ac.routed_output_validity");
      if (!routed || routed.empty())
        continue;
      if (omit)
        module->removeAttr("ac.routed_output_validity");
      else {
        auto yield = cast<ac::YieldOp>(module.getBody().front().getTerminator());
        unsigned validity = cast<DictionaryAttr>(routed[0]).getAs<IntegerAttr>("validity").getInt();
        OpBuilder b(yield);
        yield->setOperand(validity, constant(b, yield.getOperand(validity).getType(), 0));
      }
      changed = true;
      break;
    }
    ASSERT_TRUE(changed);
    rejects(*file);
  }
}

TEST_F(HardwareResourceDemandTest,
       EnabledChildPushStillRequiresParentQueueWiring) {
  auto file = fixture(false, false, false, false, 1);
  ASSERT_TRUE(file) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*file))) << diagnostics;
  bool changed = false;
  ac::HardwareAnalysis analysis(*file);
  for (auto module : file->getBody()->getOps<ac::ModuleOp>()) {
    if (!module.getSymName().contains("WrapperB"))
      continue;
    ASSERT_FALSE(module->hasAttr("ac.queue_wiring"));
    for (auto instance : module.getBody().front().getOps<ac::InstanceOp>()) {
      auto child = dyn_cast_or_null<ac::ModuleOp>(analysis.resolveCallee(instance));
      auto wiring = child ? child->getAttrOfType<DictionaryAttr>("ac.queue_wiring") : DictionaryAttr();
      auto incoming = wiring ? wiring.getAs<ArrayAttr>("incoming") : ArrayAttr();
      if (!incoming || incoming.empty())
        continue;
      auto row = cast<DictionaryAttr>(incoming[0]);
      unsigned path = row.getAs<IntegerAttr>("path").getInt();
      OpBuilder b(instance);
      instance->setOperand(path, constant(b, instance.getOperand(path).getType(), 1));
      changed = true;
      break;
    }
  }
  ASSERT_TRUE(changed);
  rejects(*file);
}

TEST_F(HardwareResourceDemandTest, ChildPopSuffixStillRequiresParentQueueWiring) {
  auto file = fixture(false, false, false, false, 2);
  ASSERT_TRUE(file) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*file))) << diagnostics;
  bool changed = false;
  for (auto module : file->getBody()->getOps<ac::ModuleOp>()) {
    if (!module.getSymName().contains("WrapperB"))
      continue;
    auto wiring = module->getAttrOfType<DictionaryAttr>("ac.queue_wiring");
    ASSERT_TRUE(wiring);
    auto incoming = wiring.getAs<ArrayAttr>("incoming");
    ASSERT_TRUE(incoming && !incoming.empty());
    EXPECT_EQ(cast<DictionaryAttr>(incoming[0]).getAs<StringAttr>("kind").getValue(), "pop");
    module->removeAttr("ac.queue_wiring");
    changed = true;
    break;
  }
  ASSERT_TRUE(changed);
  rejects(*file);
}
TEST_F(HardwareResourceDemandTest,
       DirectMaterializationRejectsPrivateQueueEscapeAndEqualWidthPayloadAlias) {
  for (bool payloadAlias : {false, true}) {
    SCOPED_TRACE(payloadAlias);
    auto staged = stageQueue(false, true);
    ASSERT_TRUE(staged) << diagnostics;
    ASSERT_TRUE(succeeded(verifyDirect(*staged))) << diagnostics;
    bool changed = false;
    staged->candidate.module->walk([&](ac::RuleOp rule) {
      if (changed) return;
      auto admission = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
      auto proposals = admission ? admission.getAs<ArrayAttr>("queue_proposals")
                                 : ArrayAttr();
      if (!proposals) return;
      auto yield = cast<ac::YieldOp>(rule.getBody().front().getTerminator());
      for (Attribute raw : proposals) {
        auto proposal = cast<DictionaryAttr>(raw);
        auto data = proposal.getAs<IntegerAttr>("data");
        auto path = proposal.getAs<IntegerAttr>("path");
        if (!data || !path) continue;
        auto payload = yield.getOperand(data.getInt());
        auto enable = yield.getOperand(path.getInt());
        ASSERT_TRUE(ac::areEquivalentHardwareTypes(payload.getType(), enable.getType()));
        ASSERT_NE(payload, enable);
        if (payloadAlias) {
          // Both are Bits1, but a private enable is not authored payload.
          yield->setOperand(data.getInt(), enable);
        } else {
          OpBuilder b(rule->getBlock()->getTerminator());
          OperationState leak(rule.getLoc(), ac::BitsUnaryOp::getOperationName());
          leak.addOperands(rule.getResult(path.getInt()));
          leak.addTypes(enable.getType());
          leak.addAttribute("opcode", attrs.getStringAttr("not"));
          b.create(leak);
        }
        changed = true;
        break;
      }
    });
    ASSERT_TRUE(changed);
    directRejectsWithoutMutation(*staged);
  }
}

TEST_F(HardwareResourceDemandTest,
       DirectMaterializationRejectsProspectiveCorruptionInSelectedAndUnselectedDefinitions) {
  for (bool unselected : {false, true}) {
    SCOPED_TRACE(unselected);
    auto staged = stageQueue(true, false);
    ASSERT_TRUE(staged) << diagnostics;
    ASSERT_TRUE(succeeded(verifyDirect(*staged))) << diagnostics;
    ac::HardwareAnalysis analysis(*staged->candidate.module);
    bool changed = false;
    for (auto module : staged->candidate.module->getOps<ac::ModuleOp>()) {
      if (!module.getSymName().contains(unselected ? "UnselectedQueueHierarchy"
                                                 : "ExerciseQueueHierarchy"))
        continue;
      for (auto rule : module.getBody().front().getOps<ac::RuleOp>()) {
        if (!rule->hasAttr("ac.sampled_admission")) continue;
        ac::HardwareBindings bindings;
        bindings.owner = module;
        ac::detail::SourceCheckBudget budget(ac::HardwareSourceCheckLimits{});
        auto view = ac::detail::parseSampledAdmission(rule, analysis, bindings, budget);
        ASSERT_TRUE(succeeded(view)) << diagnostics;
        for (const auto &resource : view->resources) {
          if (!resource.prospective) continue;
          auto decision = view->controls[resource.control].raw
                              .getDefiningOp<ac::BitsSelectOp>();
          ASSERT_TRUE(decision);
          ASSERT_NE(decision.getTrueValue(), decision.getFalseValue());
          // Preserve the schema and raw path, but bypass the genuine capacity/
          // committed-valid/pop-credit predicate with its own bypass value.
          decision->setOperand(1, decision.getFalseValue());
          changed = true;
          break;
        }
        if (changed) break;
      }
      if (changed) break;
    }
    ASSERT_TRUE(changed);
    directRejectsWithoutMutation(*staged);
  }
}

} // namespace
