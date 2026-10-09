#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <string>

namespace {
namespace ac = acir::ac;
using namespace mlir;

class HardwareSourceCheckOccurrencesTest : public ::testing::Test {
protected:
  MLIRContext context;
  OpBuilder b{&context};
  OwningOpRef<mlir::ModuleOp> package;
  ac::ModuleOp root, leaf, localReset, unreachable;
  ac::InstanceOp earlier, later, ownReset;
  ac::CollectionOp lanes;
  SmallVector<Operation *> storage;
  std::string diagnostics;

  void SetUp() override { context.loadDialect<ac::ACIRDialect>(); }
  Location loc() { return FileLineColLoc::get(&context, "owners.py", 1, 1); }
  NamedAttribute f(StringRef name, Attribute value) {
    return b.getNamedAttr(name, value);
  }
  DictionaryAttr d(std::initializer_list<NamedAttribute> fields) {
    return b.getDictionaryAttr(fields);
  }
  DictionaryAttr span() {
    return d({f("path", b.getStringAttr("owners.py")),
              f("line", b.getI64IntegerAttr(1)),
              f("column", b.getI64IntegerAttr(1)),
              f("end_line", b.getI64IntegerAttr(1)),
              f("end_column", b.getI64IntegerAttr(2))});
  }
  DictionaryAttr occurrence(StringRef symbol, unsigned index) {
    auto step = d({f("kind", b.getStringAttr("index")),
                   f("value", b.getI64IntegerAttr(index))});
    return d(
        {f("site", d({f("definition", FlatSymbolRefAttr::get(&context, symbol)),
                      f("ast_path", b.getArrayAttr({step}))})),
         f("expansion", b.getArrayAttr({}))});
  }
  ac::StaticExprAttr expression(StringRef symbol, StringRef value) {
    std::string text =
        "#ac.static_expr<{" + value.str() +
        ", origin = {site = {definition = @" + symbol.str() +
        ", ast_path = []}, expansion = []}, location = {path = \"owners.py\", "
        "line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 "
        ": i64}}>";
    return cast<ac::StaticExprAttr>(parseAttribute(text, &context));
  }
  ac::StaticExprAttr literal(unsigned value, StringRef symbol = "Root") {
    return expression(symbol,
                      "kind = \"literal\", value = {kind = \"integer\", "
                      "value = #ac.math_int<" +
                          std::to_string(value) + ">}");
  }
  Type bits(unsigned width) {
    return ac::BitsType::get(&context, literal(width));
  }
  Type resultType() {
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
  Value constant(unsigned value, unsigned width = 1) {
    return op(ac::BitsConstantOp::getOperationName(), {}, {bits(width)},
              {f("value", literal(value))})
        ->getResult(0);
  }
  ac::ModuleOp module(StringRef name, bool reset = false) {
    b.setInsertionPointToEnd(package->getBody());
    OperationState state(loc(), ac::ModuleOp::getOperationName());
    SmallVector<Type> inputs;
    SmallVector<Attribute> names;
    if (reset) {
      inputs = {bits(1), bits(1)};
      names = {b.getStringAttr("pyc_clk"), b.getStringAttr("pyc_rst")};
    }
    state.addAttributes(
        {f("sym_name", b.getStringAttr(name)),
         f("source_owner", d({f("package", b.getStringAttr("")),
                              f("path", b.getStringAttr("owners.py"))})),
         f("parameters", b.getArrayAttr({})),
         f("type_parameters", b.getArrayAttr({})),
         f("input_names", b.getArrayAttr(names)),
         f("output_names", b.getArrayAttr({b.getStringAttr("result")})),
         f("function_type",
           TypeAttr::get(b.getFunctionType(inputs, {resultType()}))),
         f("ac.return_form", b.getStringAttr("single")),
         f("ac.parameters", b.getArrayAttr({})),
         f("ac.result_constraints",
           b.getArrayAttr({d({f("kind", b.getStringAttr("hardware")),
                              f("type", TypeAttr::get(resultType())),
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
    auto record =
        op(ac::StructCreateOp::getOperationName(), {value}, {resultType()});
    op(ac::YieldOp::getOperationName(), {record->getResult(0)});
    return result;
  }
  void check(ac::ModuleOp owner) {
    b.setInsertionPoint(owner.getBody().front().getTerminator());
    auto registration = occurrence(owner.getSymName(), 7);
    auto id = d({f("registration", registration),
                 f("check", occurrence(owner.getSymName(), 13)),
                 f("obligation", b.getI64IntegerAttr(5))});
    OperationState state(loc(), ac::RuleOp::getOperationName());
    state.addTypes({bits(1), bits(1)});
    state.addAttributes(
        {f("name", b.getStringAttr("check")), f("occurrence", registration),
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
    op(ac::SourceExpectOp::getOperationName(), rule.getResults(), {},
       {f("kind", b.getStringAttr("assert")), f("location", span()),
        f("ac.check_id", id)});
  }
  ac::InstanceOp instance(ac::ModuleOp owner, ac::ModuleOp child,
                          unsigned index, StringRef name,
                          ValueRange inputs = {}) {
    b.setInsertionPoint(owner.getBody().front().getTerminator());
    return cast<ac::InstanceOp>(
        op(ac::InstanceOp::getOperationName(), inputs, {resultType()},
           {f("callee", FlatSymbolRefAttr::get(&context, child.getSymName())),
            f("instance_name", b.getStringAttr(name)),
            f("parameters", b.getArrayAttr({})),
            f("type_arguments", b.getArrayAttr({})),
            f("occurrence", occurrence(owner.getSymName(), index))}));
  }
  void build(bool reset = true, bool withStorage = false) {
    package = mlir::ModuleOp::create(loc());
    b.setInsertionPointToEnd(package->getBody());
    op(ac::StructOp::getOperationName(), {}, {},
       {f("sym_name", b.getStringAttr("Output")),
        f("fields", b.getArrayAttr({d({f("name", b.getStringAttr("flag")),
                                       f("type", TypeAttr::get(bits(1)))})}))});
    leaf = module("Leaf");
    check(leaf);
    localReset = module("LocalReset", true);
    check(localReset);
    unreachable = module("Unreachable");
    check(unreachable);
    root = module("Root", reset);
    // Text order and descriptive names deliberately disagree with occurrence
    // identity. Names cannot choose order or reset authority.
    later = instance(root, leaf, 19, "aaa");
    earlier = instance(root, leaf, 2, "zzz");
    b.setInsertionPoint(root.getBody().front().getTerminator());
    auto clk = reset ? root.getBody().front().getArgument(0) : constant(0);
    auto rst = reset ? root.getBody().front().getArgument(1) : constant(0);
    ownReset = instance(root, localReset, 23, "own_reset", {clk, rst});
    b.setInsertionPoint(root.getBody().front().getTerminator());
    auto shape = b.getArrayAttr({literal(2), literal(3)});
    auto table = ac::TableType::get(&context, shape, resultType());
    lanes = cast<ac::CollectionOp>(
        op(ac::CollectionOp::getOperationName(), {}, {table},
           {f("callee", FlatSymbolRefAttr::get(&context, leaf.getSymName())),
            f("instance_name", b.getStringAttr("family")),
            f("parameters", b.getArrayAttr({})),
            f("type_arguments", b.getArrayAttr({})), f("shape", shape),
            f("occurrence", occurrence("Root", 29))}));
    if (withStorage)
      addStorage(leaf);
    b.setInsertionPointToEnd(package->getBody());
    op(ac::SystemOp::getOperationName(), {}, {},
       {f("domain", b.getStringAttr("default")),
        f("entry", d({f("callee", FlatSymbolRefAttr::get(&context, "Root")),
                      f("parameters", b.getArrayAttr({})),
                      f("type_arguments", b.getArrayAttr({}))}))});
  }
  ac::ModuleImportOp primitive(StringRef kind) {
    b.setInsertionPointToEnd(package->getBody());
    std::string symbol = "Storage_" + kind.str();
    Type payload = ac::TypeParamType::get(
        &context, FlatSymbolRefAttr::get(&context, symbol),
        b.getStringAttr("T"));
    auto address = ac::BitsType::get(
        &context,
        expression(
            symbol,
            "kind = \"reference\", ref = {kind = \"parameter\", owner = @" +
                symbol + ", name = \"ADDR_WIDTH\"}"));
    SmallVector<StringRef> pins, outputs;
    if (kind == "dff") {
      pins = {"clk", "rst", "d", "init"};
      outputs = {"q"};
    }
    if (kind == "dffe") {
      pins = {"clk", "rst", "en", "d", "init"};
      outputs = {"q"};
    }
    if (kind == "byte_mem") {
      pins = {"clk", "rst", "raddr", "wvalid", "waddr", "wdata", "wstrb"};
      outputs = {"rdata"};
    }
    if (kind == "sync_mem") {
      pins = {"clk",    "rst",   "ren",   "raddr",
              "wvalid", "waddr", "wdata", "wstrb"};
      outputs = {"rdata"};
    }
    if (kind == "sync_mem_dp") {
      pins = {"clk",    "rst",    "ren0",  "raddr0", "ren1",
              "raddr1", "wvalid", "waddr", "wdata",  "wstrb"};
      outputs = {"rdata0", "rdata1"};
    }
    SmallVector<Type> types;
    SmallVector<Attribute> names, outputNames, params, dependencies;
    bool memory = kind.contains("mem");
    for (StringRef pin : pins) {
      names.push_back(b.getStringAttr(pin));
      types.push_back(pin == "d" || pin == "init" || pin == "wdata" ? payload
                      : pin.contains("addr")                        ? address
                                                                    : bits(1));
    }
    for (auto [i, name] : llvm::enumerate(outputs)) {
      outputNames.push_back(b.getStringAttr(name));
      dependencies.push_back(ac::getOutputDependencyAttr(
          b, i,
          kind == "byte_mem" ? ArrayRef<unsigned>{2} : ArrayRef<unsigned>{}));
    }
    if (memory)
      for (StringRef name : {"ADDR_WIDTH", "DEPTH"})
        params.push_back(ac::getStaticParameterAttr(b, name));
    return cast<ac::ModuleImportOp>(op(
        ac::ModuleImportOp::getOperationName(), {}, {},
        {f("sym_name", b.getStringAttr(symbol)),
         f("source_owner", d({f("package", b.getStringAttr("gfsim")),
                              f("path", b.getStringAttr(kind.str() + ".py"))})),
         f("parameters", b.getArrayAttr(params)),
         f("type_parameters", b.getArrayAttr({b.getStringAttr("T")})),
         f("function_type",
           TypeAttr::get(b.getFunctionType(
               types, SmallVector<Type>(outputs.size(), payload)))),
         f("input_names", b.getArrayAttr(names)),
         f("output_names", b.getArrayAttr(outputNames)),
         f("primitive_kind", b.getStringAttr(kind)),
         f("dependency_summary", b.getArrayAttr(dependencies))}));
  }
  void addStorage(ac::ModuleOp owner) {
    storage.clear();
    for (StringRef kind :
         {"dff", "dffe", "byte_mem", "sync_mem", "sync_mem_dp"}) {
      auto imported = primitive(kind);
      b.setInsertionPoint(owner.getBody().front().getTerminator());
      SmallVector<Value> inputs;
      for (Attribute raw : imported.getInputNames()) {
        StringRef name = cast<StringAttr>(raw).getValue();
        bool data = name == "d" || name == "init" || name == "wdata";
        inputs.push_back(constant(0, data ? 8 : 1));
      }
      storage.push_back(op(
          ac::InstanceOp::getOperationName(), inputs,
          SmallVector<Type>(imported.getOutputNames().size(), bits(8)),
          {f("callee", FlatSymbolRefAttr::get(&context, imported.getSymName())),
           f("instance_name", b.getStringAttr(kind)),
           f("parameters", kind.contains("mem")
                               ? b.getArrayAttr({literal(1), literal(2)})
                               : b.getArrayAttr({})),
           f("type_arguments", b.getArrayAttr({TypeAttr::get(bits(8))})),
           f("occurrence",
             occurrence(owner.getSymName(), 40 + storage.size()))}));
    }
    b.setInsertionPoint(owner.getBody().front().getTerminator());
    auto zero = constant(0), data = constant(0, 8);
    storage.push_back(op(
        ac::QueueOp::getOperationName(), {zero, zero, zero, data, zero},
        {bits(1), bits(1), bits(8)},
        {f("instance_name", b.getStringAttr("queue")), f("depth", literal(3)),
         f("ready_policy", b.getStringAttr("local_occupancy")),
         f("availability_latency", literal(2)),
         f("head_read_latency", literal(0)),
         f("empty_flow", b.getBoolAttr(false)),
         f("read_during_write", b.getStringAttr("old")),
         f("reset_policy", b.getStringAttr("sync_high_empty")),
         f("empty_data", b.getStringAttr("zero")),
         f("occurrence", occurrence(owner.getSymName(), 49))}));
  }
  FailureOr<ac::HardwareSourceCheckPlan> plan() {
    diagnostics.clear();
    ScopedDiagnosticHandler capture(&context, [&](Diagnostic &error) {
      llvm::raw_string_ostream out(diagnostics);
      error.print(out);
      return success();
    });
    return ac::HardwareAnalysis(*package).getSourceCheckPlan();
  }
};

TEST_F(HardwareSourceCheckOccurrencesTest,
       ActualPathsExpandUnusedChecksAndRowMajorLanes) {
  build();
  ASSERT_TRUE(succeeded(mlir::verify(*package)));
  auto p = plan();
  ASSERT_TRUE(succeeded(p)) << diagnostics;
  ASSERT_EQ(p->owners.size(), 10u);
  ASSERT_EQ(p->checks.size(), 9u);
  EXPECT_EQ(p->owners[0].definition, root);
  EXPECT_TRUE(p->owners[0].path.empty());
  EXPECT_FALSE(p->owners[0].parent);
  SmallVector<Operation *> allocations{earlier, later, ownReset, lanes, lanes,
                                       lanes,   lanes, lanes,    lanes};
  for (unsigned i = 1; i < 10; ++i) {
    const auto &owner = p->owners[i];
    EXPECT_EQ(owner.ordinal, i);
    EXPECT_EQ(owner.parent, std::optional<uint64_t>(0));
    ASSERT_EQ(owner.path.size(), 1u);
    EXPECT_EQ(owner.path[0].allocation, allocations[i - 1]);
    if (i < 4)
      EXPECT_TRUE(owner.path[0].coordinates.empty());
    else
      EXPECT_EQ(owner.path[0].coordinates,
                (SmallVector<uint64_t>{(i - 4) / 3, (i - 4) % 3}));
    EXPECT_NE(owner.definition, unreachable);
    auto definition = owner.definition;
    EXPECT_EQ(owner.bindings.owner, definition.getOperation());
    EXPECT_EQ(p->checks[i - 1].ownerOccurrence, i);
    EXPECT_EQ(p->checks[i - 1].ordinal, i - 1);
    const auto &binding = p->bindings[p->checks[i - 1].bindingIndex];
    EXPECT_EQ(binding.definition, owner.definition);
    EXPECT_FALSE(binding.message);
  }
  EXPECT_TRUE(p->commits.empty());
  EXPECT_TRUE(
      succeeded(ac::HardwareAnalysis(*package).verifySourceCheckPlan(*p)));
  auto independent =
      ac::HardwareAnalysis(*package).getIndependentWorkSubtrees(root);
  ASSERT_TRUE(succeeded(independent));
  EXPECT_TRUE(independent->empty())
      << "checked subtrees must stay on the checked schedule";
}

TEST_F(HardwareSourceCheckOccurrencesTest,
       ResetAuthorityFollowsDomainAndActualOwner) {
  build();
  auto p = plan();
  ASSERT_TRUE(succeeded(p)) << diagnostics;
  for (const auto &check : p->checks) {
    EXPECT_EQ(check.reset.kind, ac::HardwareCheckResetKind::PhysicalReset);
    EXPECT_EQ(check.reset.inputOrdinal, std::optional<unsigned>(1));
    EXPECT_EQ(check.reset.ownerOccurrence,
              std::optional<uint64_t>(
                  p->owners[check.ownerOccurrence].definition == localReset
                      ? check.ownerOccurrence
                      : 0));
  }
  auto altered = *p;
  altered.checks[0].reset.ownerOccurrence = 3;
  EXPECT_TRUE(
      failed(ac::HardwareAnalysis(*package).verifySourceCheckPlan(altered)));
  altered = *p;
  altered.checks[0].reset.inputOrdinal = 0;
  EXPECT_TRUE(
      failed(ac::HardwareAnalysis(*package).verifySourceCheckPlan(altered)));
  // A changed actual binding still identifies the child's own authoritative
  // reset pin, rather than guessing an ancestor input from spelling or value.
  ownReset->setOperand(1, root.getBody().front().getArgument(0));
  auto rebound = plan();
  ASSERT_TRUE(succeeded(rebound)) << diagnostics;
  for (const auto &check : rebound->checks)
    if (rebound->owners[check.ownerOccurrence].definition == localReset) {
      EXPECT_EQ(check.reset.ownerOccurrence,
                std::optional<uint64_t>(check.ownerOccurrence));
      EXPECT_EQ(check.reset.inputOrdinal, std::optional<unsigned>(1));
    }
  build(false);
  p = plan();
  ASSERT_TRUE(succeeded(p)) << diagnostics;
  for (const auto &check : p->checks) {
    bool physical = p->owners[check.ownerOccurrence].definition == localReset;
    EXPECT_EQ(check.reset.kind,
              physical ? ac::HardwareCheckResetKind::PhysicalReset
                       : ac::HardwareCheckResetKind::NoPhysicalReset);
    EXPECT_EQ(check.reset.ownerOccurrence.has_value(), physical);
    EXPECT_EQ(check.reset.inputOrdinal.has_value(), physical);
  }
}

TEST_F(HardwareSourceCheckOccurrencesTest,
       CommitClosureContainsEveryActualTrustedLeaf) {
  build(true, true);
  ASSERT_TRUE(succeeded(mlir::verify(*package)));
  auto p = plan();
  ASSERT_TRUE(succeeded(p)) << diagnostics;
  ASSERT_EQ(p->commits.size(),
            48u); // Eight actual Leaf owners, six leaves each.
  SmallVector<StringRef> kinds{"dff",      "dffe",        "byte_mem",
                               "sync_mem", "sync_mem_dp", "fifo"};
  for (const auto &owner : p->owners) {
    if (owner.definition != leaf)
      continue;
    for (auto [index, allocation] : llvm::enumerate(storage)) {
      unsigned matches = 0;
      for (const auto &endpoint : p->commits)
        if (endpoint.ownerOccurrence == owner.ordinal &&
            endpoint.allocation == allocation) {
          ++matches;
          EXPECT_EQ(endpoint.primitiveKind.getValue(), kinds[index]);
          EXPECT_TRUE(endpoint.coordinates.empty());
        }
      EXPECT_EQ(matches, 1u)
          << "each actual endpoint participates exactly once";
    }
  }
  auto altered = *p;
  altered.commits.pop_back();
  EXPECT_TRUE(
      failed(ac::HardwareAnalysis(*package).verifySourceCheckPlan(altered)));
  altered = *p;
  altered.commits.push_back(altered.commits.front());
  EXPECT_TRUE(
      failed(ac::HardwareAnalysis(*package).verifySourceCheckPlan(altered)));
  altered = *p;
  altered.commits[0].primitiveKind = b.getStringAttr("fifo");
  EXPECT_TRUE(
      failed(ac::HardwareAnalysis(*package).verifySourceCheckPlan(altered)));
  altered = *p;
  altered.commits[0].ownerOccurrence = 0;
  EXPECT_TRUE(
      failed(ac::HardwareAnalysis(*package).verifySourceCheckPlan(altered)));
  altered = *p;
  altered.owners[4].path[0].coordinates = {1, 0};
  EXPECT_TRUE(
      failed(ac::HardwareAnalysis(*package).verifySourceCheckPlan(altered)));
}

TEST_F(HardwareSourceCheckOccurrencesTest,
       UncheckedOwnersStillJoinCommitClosure) {
  build();
  addStorage(root);
  auto p = plan();
  ASSERT_TRUE(succeeded(p)) << diagnostics;
  ASSERT_EQ(p->commits.size(), storage.size());
  for (Operation *allocation : storage) {
    unsigned matches = 0;
    for (const auto &endpoint : p->commits)
      if (endpoint.allocation == allocation) {
        ++matches;
        EXPECT_EQ(endpoint.ownerOccurrence, 0u);
        EXPECT_TRUE(endpoint.coordinates.empty());
      }
    EXPECT_EQ(matches, 1u);
  }
  for (const auto &check : p->checks)
    EXPECT_NE(check.ownerOccurrence, 0u);
}

TEST_F(HardwareSourceCheckOccurrencesTest,
       NumericEquivalentAllocationOccurrencesCannotDuplicateOwners) {
  build();
  ASSERT_TRUE(succeeded(plan())) << diagnostics;
  auto original = earlier.getOccurrence();
  auto site = original.getAs<DictionaryAttr>("site");
  auto path = site.getAs<ArrayAttr>("ast_path");
  auto component = cast<DictionaryAttr>(path[0]);
  NamedAttrList alternateComponent(component);
  alternateComponent.set("value", b.getI32IntegerAttr(2));
  NamedAttrList alternateSite(site);
  alternateSite.set(
      "ast_path", b.getArrayAttr({alternateComponent.getDictionary(&context)}));
  NamedAttrList alternateOccurrence(original);
  alternateOccurrence.set("site", alternateSite.getDictionary(&context));
  later->setAttr("occurrence", alternateOccurrence.getDictionary(&context));
  EXPECT_TRUE(failed(plan())) << diagnostics;
}
} // namespace
