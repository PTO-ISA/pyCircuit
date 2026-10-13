#include "Dialect/ACIR/HardwareSampledAdmission.h"
#include "Dialect/ACIR/HardwareSourceCheckPreflight.h"
#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <string>

namespace {
namespace ac = acir::ac;
namespace detail = acir::ac::detail;
using namespace mlir;

class HardwareSampledAdmissionTest : public ::testing::Test {
protected:
  MLIRContext context;
  OpBuilder b{&context};
  OwningOpRef<mlir::ModuleOp> package;
  ac::ModuleOp definition;
  ac::RuleOp rule;
  ac::SourceExpectOp expect;
  SmallVector<ac::SourceExpectOp> expects;
  ac::BitsCompareOp known;
  std::string diagnostics;

  void SetUp() override { context.loadDialect<ac::ACIRDialect>(); }
  Location loc() { return FileLineColLoc::get(&context, "sampled.py", 7, 5); }
  NamedAttribute f(StringRef name, Attribute value) {
    return b.getNamedAttr(name, value);
  }
  DictionaryAttr d(std::initializer_list<NamedAttribute> fields) {
    return b.getDictionaryAttr(fields);
  }
  DictionaryAttr occurrence(uint64_t index, StringRef symbol = "Sampled") {
    auto step = d({f("kind", b.getStringAttr("index")),
                   f("value", b.getI64IntegerAttr(index))});
    return d(
        {f("site", d({f("definition", FlatSymbolRefAttr::get(&context, symbol)),
                      f("ast_path", b.getArrayAttr({step}))})),
         f("expansion", b.getArrayAttr({}))});
  }
  DictionaryAttr span() {
    return d({f("path", b.getStringAttr("sampled.py")),
              f("line", b.getI64IntegerAttr(7)),
              f("column", b.getI64IntegerAttr(5)),
              f("end_line", b.getI64IntegerAttr(7)),
              f("end_column", b.getI64IntegerAttr(19))});
  }
  DictionaryAttr checkID(uint64_t ordinal = 0) {
    return d({f("registration", occurrence(2)), f("check", occurrence(5)),
              f("obligation", b.getI64IntegerAttr(ordinal))});
  }
  ac::StaticExprAttr literal(uint64_t value) {
    std::string text =
        "#ac.static_expr<{kind = \"literal\", value = {kind = \"integer\", "
        "value = #ac.math_int<" +
        std::to_string(value) +
        ">}, origin = {site = {definition = @Sampled, ast_path = []}, "
        "expansion = []}, location = {path = \"sampled.py\", line = 7 : "
        "i64, column = 5 : i64, end_line = 7 : i64, end_column = 19 : "
        "i64}}>";
    return cast<ac::StaticExprAttr>(parseAttribute(text, &context));
  }
  Type bit() { return ac::BitsType::get(&context, literal(1)); }
  Operation *op(StringRef name, ValueRange operands = {},
                TypeRange results = {}, ArrayRef<NamedAttribute> attrs = {}) {
    OperationState state(loc(), name);
    state.addOperands(operands);
    state.addTypes(results);
    state.addAttributes(attrs);
    return b.create(state);
  }
  Value constant(uint64_t value) {
    return op(ac::BitsConstantOp::getOperationName(), {}, {bit()},
              {f("value", literal(value))})
        ->getResult(0);
  }
  DictionaryAttr sampled(ArrayRef<Attribute> guards, uint64_t fire = 0,
                         uint64_t clock = 0, uint64_t reset = 1) {
    return d({f("clock_capture", b.getI64IntegerAttr(clock)),
              f("reset_capture", b.getI64IntegerAttr(reset)),
              f("fire_result", b.getI64IntegerAttr(fire)),
              f("guards", b.getArrayAttr(guards))});
  }
  DictionaryAttr guard(uint64_t ordinal = 0, int64_t predecessor = -1,
                       ArrayAttr invocation = {}) {
    if (!invocation)
      invocation = b.getArrayAttr({});
    return d({f("id", checkID(ordinal)), f("invocation", invocation),
              f("predecessor", b.getI64IntegerAttr(predecessor))});
  }
  ArrayAttr invocation(uint64_t ordinal) {
    if (!ordinal)
      return b.getArrayAttr({});
    return b.getArrayAttr(
        {d({f("call", occurrence(10 + ordinal)),
            f("receiver", d({f("instance_path", b.getArrayAttr({}))}))})});
  }
  void build(bool valued = false, std::optional<uint64_t> literalGuard = {},
             unsigned guardCount = 1) {
    package = mlir::ModuleOp::create(loc());
    package->getOperation()->setAttr("ac.stage", b.getStringAttr("final"));
    b.setInsertionPointToStart(package->getBody());
    OperationState moduleState(loc(), ac::ModuleOp::getOperationName());
    moduleState.addAttributes(
        {f("sym_name", b.getStringAttr("Sampled")),
         f("source_owner", d({f("package", b.getStringAttr("")),
                              f("path", b.getStringAttr("sampled.py"))})),
         f("parameters", b.getArrayAttr({})),
         f("type_parameters", b.getArrayAttr({})),
         f("input_names", b.getArrayAttr({b.getStringAttr("enabled"),
                                          b.getStringAttr("pyc_clk"),
                                          b.getStringAttr("pyc_rst")})),
         f("output_names", b.getArrayAttr({})),
         f("function_type",
           TypeAttr::get(b.getFunctionType({bit(), bit(), bit()}, {}))),
         f("ac.domain_inputs", d({f("clock", b.getI64IntegerAttr(1)),
                                  f("reset", b.getI64IntegerAttr(2))}))});
    moduleState.addRegion();
    definition = cast<ac::ModuleOp>(b.create(moduleState));
    auto *body = new Block;
    definition.getBody().push_back(body);
    for (unsigned i = 0; i < 3; ++i)
      body->addArgument(bit(), loc());

    b.setInsertionPointToEnd(body);
    uint64_t ordinary = valued ? 1 : 0;
    uint64_t fire = ordinary;
    SmallVector<Attribute> required, guardRows;
    for (unsigned ordinal = 0; ordinal < guardCount; ++ordinal) {
      auto id = checkID(ordinal);
      required.push_back(
          d({f("id", id), f("kind", b.getStringAttr("guard_known")),
             f("location", span())}));
      guardRows.push_back(guard(ordinal, static_cast<int64_t>(ordinal) - 1,
                                invocation(ordinal)));
    }
    OperationState ruleState(loc(), ac::RuleOp::getOperationName());
    ruleState.addOperands(
        {body->getArgument(1), body->getArgument(2), body->getArgument(0)});
    ruleState.addTypes(SmallVector<Type>(ordinary + 1 + 2 * guardCount, bit()));
    ruleState.addAttributes(
        {f("name", b.getStringAttr("tick")), f("occurrence", occurrence(2)),
         f("ac.required_checks", b.getArrayAttr(required)),
         f("ac.sampled_admission", sampled(guardRows, fire))});
    ruleState.addRegion();
    rule = cast<ac::RuleOp>(b.create(ruleState));
    auto *ruleBody = new Block;
    rule.getBody().push_back(ruleBody);
    for (unsigned i = 0; i < 3; ++i)
      ruleBody->addArgument(bit(), loc());
    b.setInsertionPointToEnd(ruleBody);
    Value raw =
        literalGuard ? constant(*literalGuard) : ruleBody->getArgument(2);
    Value one = constant(1);
    known = cast<ac::BitsCompareOp>(
        op(ac::BitsCompareOp::getOperationName(), {raw, one}, {bit()},
           {f("predicate", b.getStringAttr("case_eq"))}));
    SmallVector<Value> reached = {one};
    Value prefix = known.getResult();
    for (unsigned ordinal = 1; ordinal < guardCount; ++ordinal) {
      reached.push_back(prefix);
      prefix = op(ac::BitsBinaryOp::getOperationName(), {prefix, known},
                  {bit()}, {f("opcode", b.getStringAttr("and"))})
                   ->getResult(0);
    }
    SmallVector<Value> yielded;
    if (valued)
      yielded.push_back(raw);
    yielded.push_back(prefix);
    for (unsigned ordinal = 0; ordinal < guardCount; ++ordinal)
      yielded.append({raw, reached[ordinal]});
    op(ac::YieldOp::getOperationName(), yielded);

    b.setInsertionPointToEnd(body);
    expects.clear();
    for (unsigned ordinal = 0; ordinal < guardCount; ++ordinal) {
      auto result = fire + 1 + 2 * ordinal;
      expects.push_back(cast<ac::SourceExpectOp>(
          op(ac::SourceExpectOp::getOperationName(),
             {rule.getResult(result), rule.getResult(result + 1),
              body->getArgument(1), body->getArgument(2)},
             {},
             {f("kind", b.getStringAttr("guard_known")), f("location", span()),
              f("ac.check_id", checkID(ordinal))})));
    }
    expect = expects.front();
    op(ac::YieldOp::getOperationName());
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
  LogicalResult verifyAdmission(uint64_t budgetLimit = 1 << 20) {
    ac::HardwareAnalysis analysis(*package);
    ac::HardwareBindings bindings;
    bindings.owner = definition;
    detail::SourceCheckBudget budget({budgetLimit});
    auto view = detail::parseSampledAdmission(rule, analysis, bindings, budget);
    if (failed(view))
      return failure();
    return detail::proveSampledAdmission(*view, analysis, bindings, budget);
  }
  LogicalResult verifyDefinition() {
    return ac::HardwareAnalysis(*package).verifyDefinitionSourceChecks(
        definition);
  }
  template <typename Mutation>
  void rejects(StringRef label, Mutation mutation, StringRef diagnostic = {}) {
    SCOPED_TRACE(label.str());
    build();
    ASSERT_TRUE(succeeded(capture([&] { return verifyAdmission(); })))
        << diagnostics;
    mutation();
    EXPECT_TRUE(failed(capture([&] { return verifyAdmission(); })))
        << "mutation unexpectedly admitted: " << label.str();
    EXPECT_FALSE(diagnostics.empty());
    if (!diagnostic.empty())
      EXPECT_NE(diagnostics.find(diagnostic.str()), std::string::npos)
          << diagnostics;
  }
};

TEST_F(HardwareSampledAdmissionTest,
       LocalPrefixProofCoversStatefreeReadonlyLiteralAndValuedShapes) {
  // This is the local prefix proof only. Package wiring separately owns the
  // rule that ordinary results require total literal-true guards.
  for (bool valued : {false, true})
    for (std::optional<uint64_t> guardValue :
         {std::optional<uint64_t>(), std::optional<uint64_t>(0),
          std::optional<uint64_t>(1)}) {
      SCOPED_TRACE(::testing::Message()
                   << "valued=" << valued << " literal="
                   << (guardValue ? std::to_string(*guardValue) : "dynamic"));
      build(valued, guardValue);
      EXPECT_TRUE(succeeded(capture([&] { return verifyAdmission(); })))
          << diagnostics;
    }
}

TEST_F(HardwareSampledAdmissionTest,
       OrderedTwoGuardPrefixUsesSharedKnownOneDAG) {
  build(false, std::nullopt, 2);
  ASSERT_TRUE(succeeded(capture([&] { return verifyAdmission(); })))
      << diagnostics;
  auto yield = cast<ac::YieldOp>(rule.getBody().front().back());
  ASSERT_EQ(yield.getNumOperands(), 5u);
  auto shared = yield.getOperand(0).getDefiningOp<ac::BitsBinaryOp>();
  ASSERT_TRUE(shared);
  EXPECT_EQ(shared.getLhs(), shared.getRhs());
  EXPECT_EQ(yield.getOperand(4), shared.getLhs());

  auto rejectsTwo = [&](StringRef label, auto mutation) {
    SCOPED_TRACE(label.str());
    build(false, std::nullopt, 2);
    ASSERT_TRUE(succeeded(capture([&] { return verifyAdmission(); })))
        << diagnostics;
    mutation();
    EXPECT_TRUE(failed(capture([&] { return verifyAdmission(); })))
        << "invalid two-guard prefix admitted: " << label.str();
  };
  rejectsTwo("missing earlier known prefix", [&] {
    auto current = cast<ac::YieldOp>(rule.getBody().front().back());
    current->setOperand(4, current->getOperand(2));
  });
  rejectsTwo("wrong predecessor", [&] {
    auto metadata = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    SmallVector<Attribute> rows(metadata.getAs<ArrayAttr>("guards").begin(),
                                metadata.getAs<ArrayAttr>("guards").end());
    NamedAttrList changed(cast<DictionaryAttr>(rows[1]));
    changed.set("predecessor", b.getI64IntegerAttr(-1));
    rows[1] = changed.getDictionary(&context);
    NamedAttrList fields(metadata);
    fields.set("guards", b.getArrayAttr(rows));
    rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
  });
  rejectsTwo("guard order", [&] {
    auto metadata = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    SmallVector<Attribute> rows(metadata.getAs<ArrayAttr>("guards").begin(),
                                metadata.getAs<ArrayAttr>("guards").end());
    std::swap(rows[0], rows[1]);
    NamedAttrList fields(metadata);
    fields.set("guards", b.getArrayAttr(rows));
    rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
  });
  rejectsTwo("duplicate guard id", [&] {
    auto metadata = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    SmallVector<Attribute> rows(metadata.getAs<ArrayAttr>("guards").begin(),
                                metadata.getAs<ArrayAttr>("guards").end());
    NamedAttrList changed(cast<DictionaryAttr>(rows[1]));
    changed.set("id", checkID(0));
    rows[1] = changed.getDictionary(&context);
    NamedAttrList fields(metadata);
    fields.set("guards", b.getArrayAttr(rows));
    rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
  });
  rejectsTwo("duplicate invocation", [&] {
    auto metadata = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    SmallVector<Attribute> rows(metadata.getAs<ArrayAttr>("guards").begin(),
                                metadata.getAs<ArrayAttr>("guards").end());
    NamedAttrList changed(cast<DictionaryAttr>(rows[1]));
    changed.set("invocation", b.getArrayAttr({}));
    rows[1] = changed.getDictionary(&context);
    NamedAttrList fields(metadata);
    fields.set("guards", b.getArrayAttr(rows));
    rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
  });
}

TEST_F(HardwareSampledAdmissionTest,
       ExpectSamplingOperandsAreExclusiveToGuardKnown) {
  auto rejectsDefinition = [&](StringRef label, auto mutation) {
    SCOPED_TRACE(label.str());
    build();
    ASSERT_TRUE(succeeded(capture([&] { return verifyDefinition(); })))
        << diagnostics;
    mutation();
    EXPECT_TRUE(failed(capture([&] { return verifyDefinition(); })))
        << "invalid sampling operands admitted: " << label.str();
  };
  rejectsDefinition("missing reset sampling pin", [&] {
    expect->setOperands(expect->getOperands().drop_back());
  });
  rejectsDefinition("extra sampling pin", [&] {
    SmallVector<Value> operands(expect->getOperands());
    operands.push_back(definition.getBody().front().getArgument(1));
    expect->setOperands(operands);
  });
  for (StringRef kind : {"assert", "range"}) {
    SCOPED_TRACE(kind.str());
    build();
    rule->removeAttr("ac.sampled_admission");
    expect->setAttr("kind", b.getStringAttr(kind));
    auto required = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
    auto row = cast<DictionaryAttr>(required[0]);
    NamedAttrList changed(row);
    changed.set("kind", b.getStringAttr(kind));
    rule->setAttr("ac.required_checks",
                  b.getArrayAttr({changed.getDictionary(&context)}));
    expect->setOperands({rule.getResult(1), rule.getResult(2)});
    ASSERT_TRUE(succeeded(capture([&] { return verifyDefinition(); })))
        << diagnostics;
    expect->insertOperands(2, definition.getBody().front().getArgument(1));
    EXPECT_TRUE(failed(capture([&] { return verifyDefinition(); })))
        << kind.str() << " incorrectly accepted a sampling operand";
  }
}

TEST_F(HardwareSampledAdmissionTest,
       SampledAttributesRequireTheirExactFinalPlacement) {
  build();
  ASSERT_TRUE(succeeded(capture([&] { return mlir::verify(*package); })))
      << diagnostics;
  auto sampledMetadata = rule->getAttr("ac.sampled_admission");
  definition->setAttr("ac.sampled_admission", sampledMetadata);
  EXPECT_TRUE(failed(capture([&] { return mlir::verify(*package); })));

  build();
  rule->setAttr("ac.transaction_wiring", b.getDictionaryAttr({}));
  EXPECT_TRUE(failed(capture([&] { return mlir::verify(*package); })));

  build();
  package->getOperation()->setAttr("ac.stage", b.getStringAttr("source"));
  EXPECT_TRUE(failed(capture([&] { return mlir::verify(*package); })));

  build();
  rule->removeAttr("ac.sampled_admission");
  definition->setAttr("ac.transaction_wiring", b.getDictionaryAttr({}));
  package->getOperation()->setAttr("ac.stage", b.getStringAttr("source"));
  EXPECT_TRUE(failed(capture([&] { return mlir::verify(*package); })));
}

TEST_F(HardwareSampledAdmissionTest, ExactClosedSchemaAndIndexTypesFailClosed) {
  rejects("missing guards", [&] {
    NamedAttrList fields(
        rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission"));
    fields.erase("guards");
    rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
  });
  rejects("extra field", [&] {
    NamedAttrList fields(
        rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission"));
    fields.set("unknown", b.getUnitAttr());
    rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
  });
  for (StringRef field : {"clock_capture", "reset_capture", "fire_result"}) {
    rejects((Twine("i32 ") + field).str(), [&] {
      NamedAttrList fields(
          rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission"));
      fields.set(field, b.getI32IntegerAttr(0));
      rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
    });
    rejects((Twine("negative ") + field).str(), [&] {
      NamedAttrList fields(
          rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission"));
      fields.set(field, b.getI64IntegerAttr(-1));
      rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
    });
  }
  rejects("capture out of range", [&] {
    NamedAttrList fields(
        rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission"));
    fields.set("clock_capture", b.getI64IntegerAttr(3));
    rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
  });
  rejects("result out of range", [&] {
    NamedAttrList fields(
        rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission"));
    fields.set("fire_result", b.getI64IntegerAttr(3));
    rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
  });
  rejects("empty guards", [&] {
    NamedAttrList fields(
        rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission"));
    fields.set("guards", b.getArrayAttr({}));
    rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
  });
}

TEST_F(HardwareSampledAdmissionTest,
       GuardOrderIdentityAndResultLayoutAreAuthoritative) {
  rejects("first predecessor", [&] {
    auto metadata = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    auto row = cast<DictionaryAttr>(metadata.getAs<ArrayAttr>("guards")[0]);
    NamedAttrList changed(row);
    changed.set("predecessor", b.getI64IntegerAttr(0));
    NamedAttrList fields(metadata);
    fields.set("guards", b.getArrayAttr({changed.getDictionary(&context)}));
    rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
  });
  rejects("guard extra field", [&] {
    auto metadata = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    auto row = cast<DictionaryAttr>(metadata.getAs<ArrayAttr>("guards")[0]);
    NamedAttrList changed(row);
    changed.set("extra", b.getUnitAttr());
    NamedAttrList fields(metadata);
    fields.set("guards", b.getArrayAttr({changed.getDictionary(&context)}));
    rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
  });
  rejects("wrong obligation ordinal", [&] {
    auto metadata = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    auto row = cast<DictionaryAttr>(metadata.getAs<ArrayAttr>("guards")[0]);
    NamedAttrList changed(row);
    changed.set("id", checkID(4));
    NamedAttrList fields(metadata);
    fields.set("guards", b.getArrayAttr({changed.getDictionary(&context)}));
    rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
  });
  rejects("wrong suffix id", [&] {
    auto required = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
    auto row = cast<DictionaryAttr>(required[0]);
    NamedAttrList changed(row);
    changed.set("id", checkID(1));
    rule->setAttr("ac.required_checks",
                  b.getArrayAttr({changed.getDictionary(&context)}));
  });
  rejects("malformed invocation frame", [&] {
    auto metadata = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    auto row = cast<DictionaryAttr>(metadata.getAs<ArrayAttr>("guards")[0]);
    NamedAttrList changed(row);
    changed.set("invocation", b.getArrayAttr({b.getStringAttr("frame")}));
    NamedAttrList fields(metadata);
    fields.set("guards", b.getArrayAttr({changed.getDictionary(&context)}));
    rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
  });
}

TEST_F(HardwareSampledAdmissionTest,
       ExactCaseEqualityAndPhysicalCapturesProveThePrefix) {
  rejects("ordinary equality",
          [&] { known->setAttr("predicate", b.getStringAttr("eq")); });
  rejects("raw guard used as fire", [&] {
    auto yield = cast<ac::YieldOp>(rule.getBody().front().back());
    yield->setOperand(0, rule.getBody().front().getArgument(2));
  });
  rejects("wrong reached", [&] {
    auto yield = cast<ac::YieldOp>(rule.getBody().front().back());
    yield->setOperand(2, rule.getBody().front().getArgument(2));
  });
}

TEST_F(HardwareSampledAdmissionTest, BudgetRejectsBeforeLateProofWork) {
  build();
  EXPECT_TRUE(failed(capture([&] { return verifyAdmission(1); })));
  EXPECT_NE(diagnostics.find("source-check plan exceeds analysis work budget"),
            std::string::npos)
      << diagnostics;
  EXPECT_TRUE(rule->hasAttr("ac.sampled_admission"));
  EXPECT_EQ(expect->getNumOperands(), 4u);
}

} // namespace
