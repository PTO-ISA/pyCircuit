#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <functional>
#include <string>

namespace {
namespace ac = acir::ac;
using namespace mlir;

// This native fixture represents the approved carrier contract directly. It
// needs neither Python capture nor a selected system for definition validation.
class HardwareSourceChecksTest : public ::testing::Test {
protected:
  MLIRContext context;
  OpBuilder builder{&context};
  OwningOpRef<mlir::ModuleOp> package;
  ac::ModuleOp definition;
  ac::RuleOp rule;
  SmallVector<ac::SourceExpectOp> expects;
  std::string diagnostics;

  void SetUp() override { context.loadDialect<ac::ACIRDialect>(); }
  Location loc() { return FileLineColLoc::get(&context, "checks.py", 9, 3); }
  DictionaryAttr dict(std::initializer_list<NamedAttribute> fields) {
    return builder.getDictionaryAttr(fields);
  }
  NamedAttribute field(StringRef name, Attribute value) {
    return builder.getNamedAttr(name, value);
  }
  DictionaryAttr occurrence(uint64_t index, StringRef symbol = "Checks") {
    auto component = dict({field("kind", builder.getStringAttr("index")),
                           field("value", builder.getI64IntegerAttr(index))});
    auto site =
        dict({field("definition", FlatSymbolRefAttr::get(&context, symbol)),
              field("ast_path", builder.getArrayAttr({component}))});
    return dict(
        {field("site", site), field("expansion", builder.getArrayAttr({}))});
  }
  DictionaryAttr span() {
    return dict({field("path", builder.getStringAttr("checks.py")),
                 field("line", builder.getI64IntegerAttr(9)),
                 field("column", builder.getI64IntegerAttr(3)),
                 field("end_line", builder.getI64IntegerAttr(9)),
                 field("end_column", builder.getI64IntegerAttr(18))});
  }
  DictionaryAttr id(uint64_t obligation, uint64_t registration = 2) {
    return dict({field("registration", occurrence(registration)),
                 field("check", occurrence(4)),
                 field("obligation", builder.getI64IntegerAttr(obligation))});
  }
  DictionaryAttr with(DictionaryAttr old, StringRef name, Attribute value) {
    NamedAttrList fields(old);
    fields.set(name, value);
    return fields.getDictionary(&context);
  }
  ac::StaticExprAttr literal(unsigned value) {
    std::string text =
        "#ac.static_expr<{kind = \"literal\", value = {kind = "
        "\"integer\", value = #ac.math_int<" +
        std::to_string(value) +
        ">}, origin = {site = {definition = @Checks, ast_path = []}, "
        "expansion = []}, location = {path = \"checks.py\", line = 9 : i64, "
        "column = 3 : i64, end_line = 9 : i64, end_column = 18 : i64}}>";
    return cast<ac::StaticExprAttr>(parseAttribute(text, &context));
  }
  Type bits(unsigned width) {
    return ac::BitsType::get(&context, literal(width));
  }
  Operation *op(StringRef name, ValueRange inputs = {}, TypeRange results = {},
                ArrayRef<NamedAttribute> attributes = {}) {
    OperationState state(loc(), name);
    state.addOperands(inputs);
    state.addTypes(results);
    state.addAttributes(attributes);
    return builder.create(state);
  }
  void build(unsigned prefix = 1, bool equalValues = false,
             bool selected = false, StringRef kind = "assert") {
    package = mlir::ModuleOp::create(loc());
    builder.setInsertionPointToStart(package->getBody());
    OperationState state(loc(), ac::ModuleOp::getOperationName());
    state.addAttributes(
        {field("sym_name", builder.getStringAttr("Checks")),
         field("source_owner",
               dict({field("package", builder.getStringAttr("")),
                     field("path", builder.getStringAttr("checks.py"))})),
         field("parameters", builder.getArrayAttr({})),
         field("type_parameters", builder.getArrayAttr({})),
         field("input_names",
               builder.getArrayAttr(
                   {builder.getStringAttr("a"), builder.getStringAttr("b")})),
         field("output_names", builder.getArrayAttr({})),
         field("function_type", TypeAttr::get(builder.getFunctionType(
                                    {bits(1), bits(1)}, {})))});
    state.addRegion();
    definition = cast<ac::ModuleOp>(builder.create(state));
    auto *body = new Block;
    definition.getBody().push_back(body);
    body->addArgument(bits(1), loc());
    body->addArgument(bits(1), loc());
    builder.setInsertionPointToEnd(body);
    SmallVector<Attribute> required;
    // Same registration and check occurrence, distinct noncontiguous obligation
    // slots. Array position, rather than slot number, owns the suffix pair.
    for (unsigned obligation : {3, 11})
      required.push_back(dict({field("id", id(obligation)),
                               field("kind", builder.getStringAttr(kind)),
                               field("location", span())}));
    OperationState calculation(loc(), ac::RuleOp::getOperationName());
    calculation.addOperands(body->getArguments());
    calculation.addTypes(SmallVector<Type>(prefix + 4, bits(1)));
    calculation.addAttributes(
        {field("name", builder.getStringAttr("compute")),
         field("occurrence", occurrence(2)),
         field("ac.required_checks", builder.getArrayAttr(required))});
    calculation.addRegion();
    rule = cast<ac::RuleOp>(builder.create(calculation));
    auto *ruleBody = new Block;
    rule.getBody().push_back(ruleBody);
    ruleBody->addArgument(bits(1), loc());
    ruleBody->addArgument(bits(1), loc());
    builder.setInsertionPointToEnd(ruleBody);
    auto first = ruleBody->getArgument(0), second = ruleBody->getArgument(1);
    SmallVector<Value> yielded(prefix, first);
    yielded.append({first, equalValues ? first : second,
                    equalValues ? first : second, first});
    op(ac::YieldOp::getOperationName(), yielded);
    builder.setInsertionPointToEnd(body);
    expects.clear();
    for (unsigned j = 0; j < 2; ++j) {
      auto *expect = op(
          ac::SourceExpectOp::getOperationName(),
          {rule.getResult(prefix + 2 * j), rule.getResult(prefix + 2 * j + 1)},
          {},
          {field("kind", builder.getStringAttr(kind)),
           field("location", span()), field("ac.check_id", id(j ? 11 : 3)),
           field("ac.message", builder.getStringAttr("independent oracle"))});
      expects.push_back(cast<ac::SourceExpectOp>(expect));
    }
    op(ac::YieldOp::getOperationName());
    if (selected) {
      builder.setInsertionPointToEnd(package->getBody());
      op(ac::SystemOp::getOperationName(), {}, {},
         {field("domain", builder.getStringAttr("default")),
          field(
              "entry",
              dict({field("callee", FlatSymbolRefAttr::get(&context, "Checks")),
                    field("parameters", builder.getArrayAttr({})),
                    field("type_arguments", builder.getArrayAttr({}))}))});
    }
  }
  bool verifyDefinition() {
    diagnostics.clear();
    ScopedDiagnosticHandler handler(&context, [&](Diagnostic &d) {
      llvm::raw_string_ostream stream(diagnostics);
      d.print(stream);
      return success();
    });
    return succeeded(
        ac::HardwareAnalysis(*package).verifyDefinitionSourceChecks(
            definition));
  }
  template <typename Mutation>
  void rejects(StringRef label, Mutation mutation) {
    SCOPED_TRACE(label.str());
    build();
    ASSERT_TRUE(verifyDefinition()) << diagnostics;
    mutation();
    EXPECT_FALSE(verifyDefinition()) << diagnostics;
    EXPECT_FALSE(diagnostics.empty());
  }
};

TEST_F(HardwareSourceChecksTest, DefinitionLocalSuffixKeepsEarlierResults) {
  for (unsigned prefix : {0, 1, 7}) {
    SCOPED_TRACE(prefix);
    build(prefix);
    ASSERT_TRUE(succeeded(mlir::verify(*package)));
    ASSERT_TRUE(verifyDefinition()) << diagnostics;
    EXPECT_TRUE(package->getOps<ac::SystemOp>().empty());
    EXPECT_EQ(expects[0].getCondition(), rule.getResult(prefix));
    EXPECT_EQ(expects[1].getPath(), rule.getResult(prefix + 3));
  }
}

TEST_F(HardwareSourceChecksTest, EqualYieldedValuesRetainDistinctCarriers) {
  build(0, true);
  ASSERT_TRUE(verifyDefinition()) << diagnostics;
  auto yield = cast<ac::YieldOp>(rule.getBody().front().back());
  EXPECT_EQ(yield.getValues()[0], yield.getValues()[1]);
  EXPECT_NE(expects[0].getCondition(), expects[0].getPath());
  expects[0]->setOperand(1, rule.getResult(0));
  EXPECT_FALSE(verifyDefinition()) << diagnostics;
}

TEST_F(HardwareSourceChecksTest, DefinitionValidationNeedsNoProviderBody) {
  build();
  builder.setInsertionPointToEnd(package->getBody());
  op(ac::ModuleImportOp::getOperationName(), {}, {},
     {field("sym_name", builder.getStringAttr("Provider")),
      field("source_owner",
            dict({field("package", builder.getStringAttr("")),
                  field("path", builder.getStringAttr("provider.py"))})),
      field("parameters", builder.getArrayAttr({})),
      field("type_parameters", builder.getArrayAttr({})),
      field("input_names", builder.getArrayAttr({})),
      field("output_names", builder.getArrayAttr({})),
      field("function_type", TypeAttr::get(builder.getFunctionType({}, {}))),
      field("dependency_summary", builder.getArrayAttr({}))});
  builder.setInsertionPoint(definition.getBody().front().getTerminator());
  op(ac::InstanceOp::getOperationName(), {}, {},
     {field("callee", FlatSymbolRefAttr::get(&context, "Provider")),
      field("instance_name", builder.getStringAttr("external")),
      field("parameters", builder.getArrayAttr({})),
      field("type_arguments", builder.getArrayAttr({})),
      field("occurrence", occurrence(31))});
  ASSERT_TRUE(succeeded(mlir::verify(*package)));
  EXPECT_TRUE(verifyDefinition()) << diagnostics;
}

TEST_F(HardwareSourceChecksTest, ExactBijectionAndMetadataRejectTampering) {
  rejects("missing", [&] { expects[0]->erase(); });
  rejects("duplicate", [&] {
    auto *copy = expects[0]->clone();
    expects[0]->getBlock()->getOperations().insert(expects[0]->getIterator(),
                                                   copy);
  });
  rejects("orphan", [&] { rule->removeAttr("ac.required_checks"); });
  rejects("missing ID", [&] { expects[0]->removeAttr("ac.check_id"); });
  rejects("wrong-typed requirements", [&] {
    rule->setAttr("ac.required_checks", builder.getStringAttr("missing"));
  });
  rejects("wrong-typed message", [&] {
    expects[0]->setAttr("ac.message", builder.getI64IntegerAttr(1));
  });
  rejects("wrong kind",
          [&] { expects[0]->setAttr("kind", builder.getStringAttr("range")); });
  rejects("unknown kind",
          [&] { expects[0]->setAttr("kind", builder.getStringAttr("probe")); });
  rejects("different span", [&] {
    expects[0]->setAttr(
        "location", with(span(), "end_column", builder.getI64IntegerAttr(19)));
  });
  rejects("malformed span", [&] {
    expects[0]->setAttr("location",
                        with(span(), "line", builder.getI64IntegerAttr(0)));
  });
  rejects("cross registration",
          [&] { expects[0]->setAttr("ac.check_id", id(3, 8)); });
  rejects("foreign check owner", [&] {
    expects[0]->setAttr("ac.check_id",
                        with(id(3), "check", occurrence(4, "Foreign")));
  });
  rejects("coherently foreign registration", [&] {
    auto foreign = id(3, 8);
    expects[0]->setAttr("ac.check_id", foreign);
    auto required = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
    rule->setAttr("ac.required_checks",
                  builder.getArrayAttr(
                      {with(cast<DictionaryAttr>(required[0]), "id", foreign),
                       required[1]}));
  });
  rejects("duplicate full ID", [&] {
    expects[1]->setAttr("ac.check_id", id(3));
    auto required = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
    rule->setAttr("ac.required_checks",
                  builder.getArrayAttr({required[0], required[0]}));
  });
  rejects("numeric-equivalent obligation carriers", [&] {
    auto alternate = with(id(3), "obligation", builder.getI32IntegerAttr(3));
    expects[1]->setAttr("ac.check_id", alternate);
    auto required = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
    rule->setAttr("ac.required_checks",
                  builder.getArrayAttr(
                      {required[0], with(cast<DictionaryAttr>(required[1]),
                                         "id", alternate)}));
  });
  rejects("condition redirect",
          [&] { expects[0]->setOperand(0, rule.getResult(0)); });
  rejects("path redirect",
          [&] { expects[0]->setOperand(1, rule.getResult(4)); });
  rejects("swapped pair", [&] {
    expects[0]->setOperands({rule.getResult(2), rule.getResult(1)});
  });
  rejects("swapped suffix pairs", [&] {
    expects[0]->setOperands({rule.getResult(3), rule.getResult(4)});
  });
  rejects("insufficient suffix", [&] {
    auto required = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
    rule->setAttr(
        "ac.required_checks",
        builder.getArrayAttr({required[0], required[1], required[0]}));
  });
  rejects("predicate width", [&] { rule.getResult(1).setType(bits(2)); });
  rejects("path width", [&] { rule.getResult(2).setType(bits(8)); });
}

TEST_F(HardwareSourceChecksTest,
       OnlyVerifiedIdentityExtractionForwardsCarriers) {
  build();
  builder.setInsertionPoint(expects[0]);
  auto *identity =
      op(ac::BitsExtractOp::getOperationName(), {rule.getResult(1)}, {bits(1)},
         {field("low", literal(0))});
  expects[0]->setOperand(0, identity->getResult(0));
  ASSERT_TRUE(verifyDefinition()) << diagnostics;
  identity->setAttr("low", literal(1));
  EXPECT_FALSE(verifyDefinition()) << diagnostics;
  rejects("nonidentity width extraction", [&] {
    builder.setInsertionPoint(expects[0]);
    auto *extract =
        op(ac::BitsExtractOp::getOperationName(), {rule.getResult(1)},
           {bits(2)}, {field("low", literal(0))});
    expects[0]->setOperand(0, extract->getResult(0));
  });
  rejects("same value select", [&] {
    builder.setInsertionPoint(expects[0]);
    auto value = rule.getResult(1);
    auto *select = op(ac::BitsSelectOp::getOperationName(),
                      {value, value, value}, {bits(1)});
    expects[0]->setOperand(0, select->getResult(0));
  });
  rejects("resize carrier", [&] {
    builder.setInsertionPoint(expects[0]);
    auto *resize =
        op(ac::BitsResizeOp::getOperationName(), {rule.getResult(1)}, {bits(1)},
           {field("mode", builder.getStringAttr("trunc"))});
    expects[0]->setOperand(0, resize->getResult(0));
  });
  rejects("another calculation carrier", [&] {
    auto *other = rule->clone();
    rule->getBlock()->getOperations().insert(rule->getIterator(), other);
    other->setAttr("name", builder.getStringAttr("other"));
    other->setAttr("occurrence", occurrence(8));
    other->removeAttr("ac.required_checks");
    expects[0]->setOperand(0, other->getResult(1));
  });
}

TEST_F(HardwareSourceChecksTest,
       PlanRecordsExactBindingAndRevalidatesMutations) {
  build(1, false, true);
  // Equal registration/check occurrences force the final obligation component
  // to order sites even when expect placement presents the opposite order.
  expects[1]->moveBefore(expects[0]);
  ac::HardwareAnalysis analysis(*package);
  auto plan = analysis.getSourceCheckPlan();
  ASSERT_TRUE(succeeded(plan));
  ASSERT_TRUE(succeeded(analysis.verifySourceCheckPlan(*plan)));
  ASSERT_EQ(plan->bindings.size(), 2u);
  ASSERT_EQ(plan->checks.size(), 2u);
  EXPECT_TRUE(plan->hasChecks());
  for (unsigned j = 0; j < 2; ++j) {
    const auto &binding = plan->bindings[j];
    EXPECT_EQ(binding.definition, definition);
    EXPECT_EQ(binding.rule, rule);
    EXPECT_EQ(binding.expect, expects[j]);
    EXPECT_EQ(binding.checkID, id(j ? 11 : 3));
    EXPECT_EQ(binding.kind.getValue(), "assert");
    EXPECT_EQ(binding.location, span());
    ASSERT_TRUE(binding.message)
        << "the supplied static message must be retained";
    EXPECT_EQ(binding.message.getValue(), "independent oracle");
    EXPECT_EQ(binding.requiredIndex, j);
    EXPECT_EQ(binding.conditionResult, 1 + 2 * j);
    EXPECT_EQ(binding.pathResult, 2 + 2 * j);
    EXPECT_EQ(binding.condition, rule.getResult(1 + 2 * j));
    EXPECT_EQ(binding.path, rule.getResult(2 + 2 * j));
    EXPECT_EQ(plan->checks[j].reset.kind,
              ac::HardwareCheckResetKind::NoPhysicalReset);
    EXPECT_FALSE(plan->checks[j].reset.ownerOccurrence);
    EXPECT_FALSE(plan->checks[j].reset.inputOrdinal);
  }
  auto rejected = [&](StringRef label, auto mutate) {
    SCOPED_TRACE(label.str());
    auto altered = *plan;
    mutate(altered);
    EXPECT_TRUE(failed(analysis.verifySourceCheckPlan(altered)));
  };
  rejected("missing binding", [](auto &p) { p.bindings.pop_back(); });
  rejected("missing actual check", [](auto &p) { p.checks.pop_back(); });
  rejected("extra check",
           [](auto &p) { p.checks.push_back(p.checks.front()); });
  rejected("requiredIndex out of range",
           [](auto &p) { p.bindings[0].requiredIndex = 91; });
  rejected("condition carrier",
           [&](auto &p) { p.bindings[0].condition = rule.getResult(0); });
  rejected("suffix position", [](auto &p) { ++p.bindings[0].conditionResult; });
  rejected("obligation", [&](auto &p) { p.bindings[0].checkID = id(19); });
  rejected("owner ordinal", [](auto &p) { p.checks[0].ownerOccurrence = 83; });
  rejected("binding ordinal", [](auto &p) { p.checks[0].bindingIndex = 83; });
  rejected("reset kind without authority", [](auto &p) {
    p.checks[0].reset.kind = ac::HardwareCheckResetKind::PhysicalReset;
  });
  rejected("no-reset forged input",
           [](auto &p) { p.checks[0].reset.inputOrdinal = 0; });
  rejected("no-reset forged owner",
           [](auto &p) { p.checks[0].reset.ownerOccurrence = 0; });
  rejected("owner path", [&](auto &p) {
    p.owners[0].path.push_back({expects[0].getOperation(), {}});
  });
  expects[0]->setOperand(0, rule.getResult(0));
  EXPECT_TRUE(failed(analysis.verifySourceCheckPlan(*plan)))
      << "a plan cannot authorize the package after carrier mutation";
}

TEST_F(HardwareSourceChecksTest, CheckOnlyComputationCyclesFailClosed) {
  build(1, false, true);
  ac::HardwareAnalysis good(*package);
  ASSERT_TRUE(succeeded(good.getSourceCheckPlan()));
  builder.setInsertionPoint(rule.getBody().front().getTerminator());
  auto input = rule.getBody().front().getArgument(0);
  auto *left = op(ac::BitsUnaryOp::getOperationName(), {input}, {bits(1)},
                  {field("opcode", builder.getStringAttr("not"))});
  auto *right = op(ac::BitsUnaryOp::getOperationName(), {left->getResult(0)},
                   {bits(1)}, {field("opcode", builder.getStringAttr("not"))});
  left->setOperand(0, right->getResult(0));
  rule.getBody().front().getTerminator()->setOperand(1, left->getResult(0));
  EXPECT_TRUE(failed(ac::HardwareAnalysis(*package).getSourceCheckPlan()));
}

TEST_F(HardwareSourceChecksTest, RangeUsesTheSameBooleanCarrierContract) {
  build(1, false, true, "range");
  ASSERT_TRUE(verifyDefinition()) << diagnostics;
  auto plan = ac::HardwareAnalysis(*package).getSourceCheckPlan();
  ASSERT_TRUE(succeeded(plan));
  EXPECT_EQ(plan->bindings.front().kind.getValue(), "range");
  rule.getResult(1).setType(bits(8));
  EXPECT_FALSE(verifyDefinition()) << diagnostics;
}
} // namespace
