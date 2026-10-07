#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <cerrno>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <string>
#if GTEST_HAS_DEATH_TEST
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace {
namespace ac = acir::ac;
using namespace mlir;

// All large families are compact declarations. Subprocess success requires
// actual analysis results; timeout, allocation failure and fork failure fail.
class HardwareSourceCheckCapacityTest : public ::testing::Test {
protected:
  MLIRContext context;
  OpBuilder b{&context};
  OwningOpRef<mlir::ModuleOp> package;
  ac::ModuleOp root, leaf;
  ac::SourceExpectOp lastExpect;
  std::string diagnostics;
  static constexpr llvm::StringLiteral budgetError =
      "source-check plan exceeds analysis work budget before occurrence "
      "expansion";

  void SetUp() override { context.loadDialect<ac::ACIRDialect>(); }
  Location loc() { return FileLineColLoc::get(&context, "capacity.py", 1, 1); }
  NamedAttribute f(StringRef name, Attribute value) {
    return b.getNamedAttr(name, value);
  }
  DictionaryAttr d(std::initializer_list<NamedAttribute> fields) {
    return b.getDictionaryAttr(fields);
  }
  DictionaryAttr span() {
    return d({f("path", b.getStringAttr("capacity.py")),
              f("line", b.getI64IntegerAttr(1)),
              f("column", b.getI64IntegerAttr(1)),
              f("end_line", b.getI64IntegerAttr(1)),
              f("end_column", b.getI64IntegerAttr(2))});
  }
  DictionaryAttr occurrence(StringRef owner, unsigned index) {
    auto step = d({f("kind", b.getStringAttr("index")),
                   f("value", b.getI64IntegerAttr(index))});
    return d(
        {f("site", d({f("definition", FlatSymbolRefAttr::get(&context, owner)),
                      f("ast_path", b.getArrayAttr({step}))})),
         f("expansion", b.getArrayAttr({}))});
  }
  ac::StaticExprAttr expression(StringRef owner, StringRef tree) {
    auto text = "#ac.static_expr<{" + tree.str() +
                ", origin = {site = {definition = @" + owner.str() +
                ", ast_path = []}, expansion = []}, location = {path = "
                "\"capacity.py\", line = 1 : i64, column = 1 : i64, "
                "end_line = 1 : i64, end_column = 2 : i64}}>";
    return cast<ac::StaticExprAttr>(parseAttribute(text, &context));
  }
  ac::StaticExprAttr literal(uint64_t value) {
    return expression("Root",
                      "kind = \"literal\", value = {kind = \"integer\", "
                      "value = #ac.math_int<" +
                          std::to_string(value) + ">}");
  }
  Type bits(unsigned width) {
    return ac::BitsType::get(&context, literal(width));
  }
  Type output() {
    return ac::StructType::get(&context, b.getStringAttr("Output"));
  }
  Operation *op(StringRef name, ValueRange inputs = {}, TypeRange results = {},
                ArrayRef<NamedAttribute> attrs = {}) {
    OperationState state(loc(), name);
    state.addOperands(inputs);
    state.addTypes(results);
    state.addAttributes(attrs);
    return b.create(state);
  }
  Value constant(unsigned value, Type type = {}) {
    return op(ac::BitsConstantOp::getOperationName(), {},
              {type ? type : bits(1)}, {f("value", literal(value))})
        ->getResult(0);
  }
  void begin() {
    package = mlir::ModuleOp::create(loc());
    b.setInsertionPointToEnd(package->getBody());
    op(ac::StructOp::getOperationName(), {}, {},
       {f("sym_name", b.getStringAttr("Output")),
        f("fields", b.getArrayAttr({d({f("name", b.getStringAttr("flag")),
                                       f("type", TypeAttr::get(bits(1)))})}))});
  }
  ac::ModuleOp module(StringRef name, bool reset = false) {
    b.setInsertionPointToEnd(package->getBody());
    SmallVector<Type> inputs;
    SmallVector<Attribute> names;
    if (reset) {
      inputs = {bits(1), bits(1)};
      names = {b.getStringAttr("pyc_clk"), b.getStringAttr("pyc_rst")};
    }
    OperationState state(loc(), ac::ModuleOp::getOperationName());
    state.addAttributes(
        {f("sym_name", b.getStringAttr(name)),
         f("source_owner", d({f("package", b.getStringAttr("")),
                              f("path", b.getStringAttr("capacity.py"))})),
         f("parameters", b.getArrayAttr({})),
         f("type_parameters", b.getArrayAttr({})),
         f("input_names", b.getArrayAttr(names)),
         f("output_names", b.getArrayAttr({b.getStringAttr("result")})),
         f("function_type",
           TypeAttr::get(b.getFunctionType(inputs, {output()}))),
         f("ac.return_form", b.getStringAttr("single")),
         f("ac.parameters", b.getArrayAttr({})),
         f("ac.result_constraints",
           b.getArrayAttr({d({f("kind", b.getStringAttr("hardware")),
                              f("type", TypeAttr::get(output())),
                              f("source_kind", b.getStringAttr("nominal"))})})),
         f("ac.domain_inputs", reset ? d({f("clock", b.getI64IntegerAttr(0)),
                                          f("reset", b.getI64IntegerAttr(1))})
                                     : d({}))});
    state.addRegion();
    auto result = cast<ac::ModuleOp>(b.create(state));
    auto *body = new Block;
    result.getBody().push_back(body);
    for (Type input : inputs)
      body->addArgument(input, loc());
    b.setInsertionPointToEnd(body);
    auto value = constant(1);
    auto *record =
        op(ac::StructCreateOp::getOperationName(), {value}, {output()});
    op(ac::YieldOp::getOperationName(), {record->getResult(0)});
    return result;
  }
  void select(ac::ModuleOp selected) {
    root = selected;
    b.setInsertionPointToEnd(package->getBody());
    op(ac::SystemOp::getOperationName(), {}, {},
       {f("domain", b.getStringAttr("default")),
        f("entry",
          d({f("callee", FlatSymbolRefAttr::get(&context, root.getSymName())),
             f("parameters", b.getArrayAttr({})),
             f("type_arguments", b.getArrayAttr({}))}))});
  }
  DictionaryAttr checkID(ac::ModuleOp owner) {
    return d({f("registration", occurrence(owner.getSymName(), 7)),
              f("check", occurrence(owner.getSymName(), 13)),
              f("obligation", b.getI64IntegerAttr(5))});
  }
  void check(ac::ModuleOp owner) {
    b.setInsertionPoint(owner.getBody().front().getTerminator());
    auto id = checkID(owner);
    OperationState state(loc(), ac::RuleOp::getOperationName());
    state.addTypes({bits(1), bits(1)});
    state.addAttributes(
        {f("name", b.getStringAttr("check")),
         f("occurrence", occurrence(owner.getSymName(), 7)),
         f("ac.required_checks",
           b.getArrayAttr({d({f("id", id), f("kind", b.getStringAttr("assert")),
                              f("location", span())})}))});
    state.addRegion();
    auto rule = cast<ac::RuleOp>(b.create(state));
    rule.getBody().push_back(new Block);
    b.setInsertionPointToEnd(&rule.getBody().front());
    auto condition = constant(1), path = constant(1);
    op(ac::YieldOp::getOperationName(), {condition, path});
    b.setInsertionPoint(owner.getBody().front().getTerminator());
    lastExpect = cast<ac::SourceExpectOp>(
        op(ac::SourceExpectOp::getOperationName(), rule.getResults(), {},
           {f("kind", b.getStringAttr("assert")), f("location", span()),
            f("ac.check_id", id),
            f("ac.message", b.getStringAttr("capacity oracle"))}));
  }
  ac::InstanceOp instance(ac::ModuleOp owner, ac::ModuleOp child,
                          unsigned index, ValueRange inputs = {},
                          ArrayAttr parameters = {}) {
    b.setInsertionPoint(owner.getBody().front().getTerminator());
    return cast<ac::InstanceOp>(
        op(ac::InstanceOp::getOperationName(), inputs, {output()},
           {f("callee", FlatSymbolRefAttr::get(&context, child.getSymName())),
            f("instance_name", b.getStringAttr("edge" + std::to_string(index))),
            f("parameters", parameters ? parameters : b.getArrayAttr({})),
            f("type_arguments", b.getArrayAttr({})),
            f("occurrence", occurrence(owner.getSymName(), index))}));
  }
  ac::CollectionOp collection(ac::ModuleOp owner, ac::ModuleOp child,
                              ArrayRef<uint64_t> extents, unsigned index = 11) {
    b.setInsertionPoint(owner.getBody().front().getTerminator());
    SmallVector<Attribute> dimensions;
    for (uint64_t extent : extents)
      dimensions.push_back(literal(extent));
    auto shape = b.getArrayAttr(dimensions);
    return cast<ac::CollectionOp>(op(
        ac::CollectionOp::getOperationName(), {},
        {ac::TableType::get(&context, shape, output())},
        {f("callee", FlatSymbolRefAttr::get(&context, child.getSymName())),
         f("instance_name", b.getStringAttr("family" + std::to_string(index))),
         f("parameters", b.getArrayAttr({})),
         f("type_arguments", b.getArrayAttr({})), f("shape", shape),
         f("occurrence", occurrence(owner.getSymName(), index))}));
  }
  Operation *storage(ac::ModuleOp owner) {
    b.setInsertionPoint(owner.getBody().front().getTerminator());
    auto zero = constant(0), data = constant(0, bits(8));
    return op(ac::QueueOp::getOperationName(), {zero, zero, zero, data, zero},
              {bits(1), bits(1), bits(8)},
              {f("instance_name", b.getStringAttr("state")),
               f("depth", literal(3)),
               f("ready_policy", b.getStringAttr("local_occupancy")),
               f("availability_latency", literal(2)),
               f("head_read_latency", literal(0)),
               f("empty_flow", b.getBoolAttr(false)),
               f("read_during_write", b.getStringAttr("old")),
               f("reset_policy", b.getStringAttr("sync_high_empty")),
               f("empty_data", b.getStringAttr("zero")),
               f("occurrence", occurrence(owner.getSymName(), 49))});
  }
  void family(uint64_t lanes, bool checked = true, unsigned siblings = 1) {
    begin();
    leaf = module("Leaf");
    if (checked)
      check(leaf);
    storage(leaf);
    root = module("Root");
    for (unsigned i = 0; i < siblings; ++i)
      collection(root, leaf, {lanes}, 11 + i);
    select(root);
  }
  template <typename Function> auto capture(Function function) {
    diagnostics.clear();
    ScopedDiagnosticHandler handler(&context, [&](Diagnostic &error) {
      llvm::raw_string_ostream stream(diagnostics);
      error.print(stream);
      return success();
    });
    return function();
  }
  FailureOr<ac::HardwareSourceCheckPlan>
  plan(ac::HardwareSourceCheckLimits limits = {}) {
    return capture([&] {
      return ac::HardwareAnalysis(*package).getSourceCheckPlan(limits);
    });
  }
  LogicalResult finalVerify() {
    return capture([&] { return ac::verifyHardwarePackage(*package); });
  }
  bool rejected(ac::HardwareSourceCheckLimits limits = {}) {
    auto p = plan(limits);
    if (succeeded(p) ||
        diagnostics.find(budgetError.str()) == std::string::npos)
      return false;
    return failed(finalVerify()) &&
           diagnostics.find(budgetError.str()) != std::string::npos;
  }
  bool compactSuccess(size_t expectedBindings = 0) {
    auto p = plan();
    if (failed(p) || p->bindings.size() != expectedBindings ||
        !p->owners.empty() || !p->checks.empty() || !p->commits.empty())
      return false;
    return succeeded(
               ac::HardwareAnalysis(*package).verifySourceCheckPlan(*p)) &&
           succeeded(finalVerify());
  }
#if GTEST_HAS_DEATH_TEST
  static void guard() {
    alarm(15);
    rlimit memory{1024ULL * 1024 * 1024, 1024ULL * 1024 * 1024};
    if (setrlimit(RLIMIT_DATA, &memory) != 0) {
      // Darwin may reject this limit; canonical CTest then enforces tree RSS.
      int limitError = errno;
      const char *marker = std::getenv("PYCIRCUIT_SOURCE_CHECK_RSS_GUARD");
      if ((limitError != EINVAL && limitError != ENOSYS &&
           limitError != ENOTSUP) ||
          !marker || StringRef(marker) != "process-tree-rss-v1")
        std::_Exit(90);
    }
#if defined(__linux__)
    if (setrlimit(RLIMIT_AS, &memory) != 0)
      std::_Exit(91);
#endif
  }
  void finish(bool result) {
    if (!result)
      llvm::errs() << diagnostics << "\ncapacity result mismatch\n";
    std::_Exit(result ? 0 : 92);
  }
#endif
};

#if GTEST_HAS_DEATH_TEST
TEST_F(HardwareSourceCheckCapacityTest,
       CheckedBillionLanesRejectBeforeExpansion) {
  ASSERT_EXIT(
      {
        guard();
        family(1000000000);
        finish(rejected());
      },
      ::testing::ExitedWithCode(0), "");
}

TEST_F(HardwareSourceCheckCapacityTest, NestedModerateFamiliesRejectProduct) {
  ASSERT_EXIT(
      {
        guard();
        begin();
        leaf = module("Leaf");
        check(leaf);
        storage(leaf);
        auto middle = module("Middle");
        collection(middle, leaf, {1000});
        root = module("Root");
        collection(root, middle, {1000});
        select(root);
        finish(rejected());
      },
      ::testing::ExitedWithCode(0), "");
}

TEST_F(HardwareSourceCheckCapacityTest,
       AggregateSharedSiblingsExceedAllowance) {
  ASSERT_EXIT(
      {
        guard();
        // Find an affordable single family without relying on private byte
        // charges. The next doubling must reject, and two equally sized sibling
        // families carry that same expanded payload plus the additional compact
        // graph edge.
        uint64_t affordable = 0;
        for (uint64_t n = 1; n <= 1048576; n *= 2) {
          family(n);
          auto p = plan();
          if (failed(p)) {
            if (diagnostics.find(budgetError.str()) == std::string::npos)
              finish(false);
            break;
          }
          affordable = n;
        }
        if (!affordable)
          finish(false);
        family(affordable, true, 2);
        finish(rejected());
      },
      ::testing::ExitedWithCode(0), "");
}

TEST_F(HardwareSourceCheckCapacityTest,
       ArithmeticOverflowRejectsWithMaximalAllowance) {
  ASSERT_EXIT(
      {
        guard();
        begin();
        leaf = module("Leaf");
        check(leaf);
        auto inner = module("Inner");
        collection(inner, leaf, {1000000});
        auto outer = module("Outer");
        collection(outer, inner, {1000000});
        root = module("Root");
        collection(root, outer, {1000000});
        select(root);
        finish(rejected({std::numeric_limits<uint64_t>::max()}));
      },
      ::testing::ExitedWithCode(0), "");
}

TEST_F(HardwareSourceCheckCapacityTest, UncheckedNestedCountsAreNeverExpanded) {
  ASSERT_EXIT(
      {
        guard();
        begin();
        leaf = module("Leaf");
        storage(leaf);
        auto inner = module("Inner");
        collection(inner, leaf, {1000000});
        auto outer = module("Outer");
        collection(outer, inner, {1000000});
        root = module("Root");
        collection(root, outer, {1000000});
        select(root);
        finish(compactSuccess());
      },
      ::testing::ExitedWithCode(0), "");
}

TEST_F(HardwareSourceCheckCapacityTest, UncheckedBillionLanesStayCompact) {
  ASSERT_EXIT(
      {
        guard();
        family(1000000000, false);
        finish(compactSuccess());
      },
      ::testing::ExitedWithCode(0), "");
}

TEST_F(HardwareSourceCheckCapacityTest, UnreachableChecksDoNotForceExpansion) {
  ASSERT_EXIT(
      {
        guard();
        family(1000000000, false);
        auto unreachable = module("Unreachable");
        check(unreachable);
        auto p = plan();
        bool valid = succeeded(p) && p->bindings.size() == 1 &&
                     p->bindings[0].definition == unreachable &&
                     compactSuccess(1);
        lastExpect->setAttr("ac.message", b.getI64IntegerAttr(9));
        bool malformed = failed(plan()) && failed(finalVerify());
        finish(valid && malformed);
      },
      ::testing::ExitedWithCode(0), "");
}

TEST_F(HardwareSourceCheckCapacityTest,
       CheckedRootRetainsHugeUncheckedStateSibling) {
  ASSERT_EXIT(
      {
        guard();
        family(1000000000, false);
        check(root);
        finish(rejected());
      },
      ::testing::ExitedWithCode(0), "");
}

TEST_F(HardwareSourceCheckCapacityTest, RecursiveBoundHierarchyStillRejects) {
  ASSERT_EXIT(
      {
        guard();
        family(2);
        instance(leaf, leaf, 31);
        finish(failed(plan()) && failed(finalVerify()));
      },
      ::testing::ExitedWithCode(0), "");
}

TEST_F(HardwareSourceCheckCapacityTest,
       LargeUncheckedCombinationalGraphStaysCompact) {
  ASSERT_EXIT(
      {
        guard();
        begin();
        root = module("Root");
        b.setInsertionPoint(root.getBody().front().getTerminator());
        auto input = constant(1);
        for (unsigned i = 0; i < 8192; ++i)
          op(ac::BitsUnaryOp::getOperationName(), {input}, {bits(1)},
             {f("opcode", b.getStringAttr("not"))});
        select(root);
        auto count = [&] {
          auto operations = root.getBody().front().getOps<ac::BitsUnaryOp>();
          return std::distance(operations.begin(), operations.end());
        };
        bool allPresent = count() == 8192;
        finish(allPresent && compactSuccess() && count() == 8192);
      },
      ::testing::ExitedWithCode(0), "");
}

TEST_F(HardwareSourceCheckCapacityTest,
       OrdinaryInstanceDAGValidationIsMemoized) {
  ASSERT_EXIT(
      {
        guard();
        begin();
        auto child = module("Terminal");
        storage(child);
        for (unsigned depth = 0; depth < 28; ++depth) {
          auto parent = module("Level" + std::to_string(depth));
          instance(parent, child, 2);
          instance(parent, child, 3);
          child = parent;
        }
        select(child);
        finish(compactSuccess());
      },
      ::testing::ExitedWithCode(0), "");
}
#else
TEST_F(HardwareSourceCheckCapacityTest, GuardedCasesRequireDeathTestSupport) {
  FAIL() << "capacity gates require death-test subprocess support";
}
#endif

TEST_F(HardwareSourceCheckCapacityTest,
       TinyAndSufficientBudgetPreserveCompleteRecords) {
  begin();
  leaf = module("Leaf");
  check(leaf);
  auto *state = storage(leaf);
  auto ownReset = module("OwnReset", true);
  check(ownReset);
  root = module("Root", true);
  auto lanes = collection(root, leaf, {2, 3});
  auto child =
      instance(root, ownReset, 23, root.getBody().front().getArguments());
  select(root);
  ASSERT_TRUE(succeeded(mlir::verify(*package)));
  EXPECT_TRUE(failed(plan({1})));
  EXPECT_NE(diagnostics.find(budgetError.str()), std::string::npos);
  auto p = plan({65536});
  ASSERT_TRUE(succeeded(p)) << diagnostics;
  ASSERT_EQ(p->bindings.size(), 2u);
  ASSERT_EQ(p->owners.size(), 8u);
  ASSERT_EQ(p->checks.size(), 7u);
  ASSERT_EQ(p->commits.size(), 6u);
  EXPECT_EQ(p->owners[0].definition, root);
  EXPECT_TRUE(p->owners[0].path.empty());
  for (unsigned i = 0; i < 7; ++i) {
    const auto &owner = p->owners[i + 1];
    const auto &resolved = p->checks[i];
    EXPECT_EQ(owner.ordinal, i + 1);
    EXPECT_EQ(owner.parent, std::optional<uint64_t>(0));
    ASSERT_EQ(owner.path.size(), 1u);
    EXPECT_EQ(owner.path[0].allocation,
              i < 6 ? lanes.getOperation() : child.getOperation());
    EXPECT_EQ(owner.path[0].coordinates,
              i < 6 ? (SmallVector<uint64_t>{i / 3, i % 3})
                    : SmallVector<uint64_t>{});
    EXPECT_EQ(owner.definition, i < 6 ? leaf : ownReset);
    EXPECT_EQ(resolved.ordinal, i);
    EXPECT_EQ(resolved.ownerOccurrence, i + 1);
    ASSERT_LT(resolved.bindingIndex, p->bindings.size());
    const auto &binding = p->bindings[resolved.bindingIndex];
    EXPECT_EQ(binding.definition, owner.definition);
    EXPECT_EQ(binding.checkID, checkID(owner.definition));
    EXPECT_EQ(binding.message.getValue(), "capacity oracle");
    EXPECT_EQ(resolved.reset.kind, ac::HardwareCheckResetKind::PhysicalReset);
    EXPECT_EQ(resolved.reset.ownerOccurrence,
              std::optional<uint64_t>(i < 6 ? 0 : 7));
    EXPECT_EQ(resolved.reset.inputOrdinal, std::optional<unsigned>(1));
    if (i < 6) {
      const auto &commit = p->commits[i];
      EXPECT_EQ(commit.ownerOccurrence, i + 1);
      EXPECT_EQ(commit.allocation, state);
      EXPECT_TRUE(commit.coordinates.empty());
      EXPECT_EQ(commit.primitiveKind.getValue(), "fifo");
    }
  }
  ac::HardwareAnalysis analysis(*package);
  EXPECT_TRUE(failed(analysis.verifySourceCheckPlan(*p, {1})));
  EXPECT_TRUE(succeeded(analysis.verifySourceCheckPlan(*p, {65536})));
  EXPECT_TRUE(succeeded(finalVerify())) << diagnostics;
}

TEST_F(HardwareSourceCheckCapacityTest,
       BoundDefinitionCacheDistinguishesActualParameters) {
  begin();
  leaf = module("Generic");
  check(leaf);
  // Parameterized hardware IR uses its existing structural contract. The
  // concrete source-call metadata admits no static parameters and is absent.
  for (StringRef name : {"ac.return_form", "ac.parameters",
                         "ac.result_constraints", "ac.domain_inputs"})
    leaf->removeAttr(name);
  leaf->setAttr("parameters",
                b.getArrayAttr({ac::getStaticParameterAttr(b, "N")}));
  auto width =
      expression("Generic", "kind = \"reference\", ref = {kind = "
                            "\"parameter\", owner = @Generic, name = \"N\"}");
  b.setInsertionPoint(leaf.getBody().front().getTerminator());
  constant(0, ac::BitsType::get(&context, width));
  root = module("Root");
  instance(root, leaf, 2, {}, b.getArrayAttr({literal(8)}));
  auto second = instance(root, leaf, 3, {}, b.getArrayAttr({literal(16)}));
  select(root);
  auto p = plan();
  ASSERT_TRUE(succeeded(p)) << diagnostics;
  ASSERT_EQ(p->owners.size(), 3u);
  ASSERT_EQ(p->checks.size(), 2u);
  for (unsigned i = 0; i < 2; ++i) {
    const auto &scope = p->owners[i + 1].bindings;
    EXPECT_EQ(scope.owner, leaf.getOperation());
    ASSERT_EQ(scope.integers.size(), 1u);
    auto actual = dyn_cast<ac::MathIntAttr>(scope.integers.lookup("N"));
    ASSERT_TRUE(actual);
    EXPECT_EQ(actual.getCanonicalValue(), i ? "16" : "8");
  }
  EXPECT_TRUE(succeeded(finalVerify())) << diagnostics;
  second->setAttr("parameters", b.getArrayAttr({literal(0)}));
  EXPECT_TRUE(failed(plan())) << "a second binding must validate its own width";
  EXPECT_TRUE(failed(finalVerify())) << diagnostics;
}
} // namespace
