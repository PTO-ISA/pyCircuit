#include "Dialect/ACIR/HardwareSampledAdmission.h"
#include "Dialect/ACIR/HardwareSourceCheckPreflight.h"
#include "Dialect/ACIR/HardwareSourceChecks.h"
#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <functional>
#include <limits>
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
  SmallVector<ac::SourceExpectOp> bodyExpects;
  SmallVector<ac::SourceObserveOp> observations;
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
  DictionaryAttr occurrenceIndex(IntegerAttr index) {
    return d({f("site",
                d({f("definition", FlatSymbolRefAttr::get(&context, "Sampled")),
                   f("ast_path",
                     b.getArrayAttr({d({f("kind", b.getStringAttr("index")),
                                        f("value", index)})}))})),
              f("expansion", b.getArrayAttr({}))});
  }
  ArrayAttr invocationIndex(IntegerAttr index, bool receiver = false) {
    auto call = receiver ? occurrence(12) : occurrenceIndex(index);
    auto path = receiver ? b.getArrayAttr({occurrenceIndex(index)})
                         : b.getArrayAttr({});
    return b.getArrayAttr(
        {d({f("call", call), f("receiver", d({f("instance_path", path)}))})});
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
  DictionaryAttr observationID(uint64_t ordinal, uint64_t site) {
    return d({f("registration", occurrence(2)), f("site", occurrence(site)),
              f("ordinal", b.getI64IntegerAttr(ordinal)),
              f("invocation", b.getArrayAttr({})),
              f("owner", d({f("instance_path", b.getArrayAttr({}))}))});
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
              f("controls", b.getArrayAttr(guards)),
              f("body_checks", b.getArrayAttr({})),
              f("observations", b.getArrayAttr({}))});
  }
  DictionaryAttr guard(uint64_t ordinal = 0, int64_t predecessor = -1,
                       ArrayAttr invocation = {}) {
    if (!invocation)
      invocation = b.getArrayAttr({});
    return d({f("id", checkID(ordinal)), f("invocation", invocation),
              f("kind", b.getStringAttr("callable_guard")),
              f("path", b.getDictionaryAttr({})),
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
  void buildMixedEffects() {
    build();
    auto oldRule = rule;
    auto &moduleBody = definition.getBody().front();
    auto guardID = checkID(0), firstCheckID = checkID(1),
         secondCheckID = checkID(2);
    auto firstObservationID = observationID(0, 20);
    auto secondObservationID = observationID(1, 21);
    auto guardRows =
        oldRule->getAttrOfType<DictionaryAttr>("ac.sampled_admission")
            .getAs<ArrayAttr>("controls");
    auto metadata = d(
        {f("clock_capture", b.getI64IntegerAttr(0)),
         f("reset_capture", b.getI64IntegerAttr(1)),
         f("fire_result", b.getI64IntegerAttr(0)), f("controls", guardRows),
         f("body_checks",
           b.getArrayAttr(
               {d({f("id", firstCheckID), f("invocation", b.getArrayAttr({})),
                   f("path", b.getDictionaryAttr({}))}),
                d({f("id", secondCheckID), f("invocation", b.getArrayAttr({})),
                   f("path", b.getDictionaryAttr({}))})})),
         f("observations",
           b.getArrayAttr({d({f("id", firstObservationID),
                              f("path", b.getDictionaryAttr({})),
                              f("value_count", b.getI64IntegerAttr(0))}),
                           d({f("id", secondObservationID),
                              f("path", b.getDictionaryAttr({})),
                              f("value_count", b.getI64IntegerAttr(3))})}))});
    auto required = b.getArrayAttr(
        {d({f("id", guardID), f("kind", b.getStringAttr("guard_known")),
            f("location", span())}),
         d({f("id", firstCheckID), f("kind", b.getStringAttr("assert")),
            f("location", span())}),
         d({f("id", secondCheckID), f("kind", b.getStringAttr("assert")),
            f("location", span())})});

    b.setInsertionPoint(oldRule);
    OperationState state(loc(), ac::RuleOp::getOperationName());
    state.addOperands(oldRule.getOperands());
    state.addTypes(SmallVector<Type>(12, bit()));
    state.addAttributes({f("name", oldRule.getNameAttr()),
                         f("occurrence", oldRule.getOccurrence()),
                         f("ac.required_checks", required),
                         f("ac.sampled_admission", metadata)});
    state.addRegion();
    rule = cast<ac::RuleOp>(b.create(state));
    auto *body = new Block;
    rule.getBody().push_back(body);
    for (auto argument : oldRule.getBody().front().getArguments())
      body->addArgument(argument.getType(), argument.getLoc());
    b.setInsertionPointToEnd(body);
    auto raw = body->getArgument(2);
    auto one = constant(1);
    auto sampled = cast<ac::BitsCompareOp>(
        op(ac::BitsCompareOp::getOperationName(), {raw, one}, {bit()},
           {f("predicate", b.getStringAttr("case_eq"))}));
    auto fire = sampled.getResult();
    op(ac::YieldOp::getOperationName(),
       {fire, fire, fire, raw, one, raw, raw, one, raw, fire, one, fire});

    for (auto terminal : expects)
      terminal.erase();
    oldRule.erase();
    b.setInsertionPoint(moduleBody.getTerminator());
    expects.clear();
    expect = cast<ac::SourceExpectOp>(
        op(ac::SourceExpectOp::getOperationName(),
           {rule.getResult(6), rule.getResult(7), moduleBody.getArgument(1),
            moduleBody.getArgument(2)},
           {},
           {f("kind", b.getStringAttr("guard_known")), f("location", span()),
            f("ac.check_id", guardID)}));
    expects.push_back(expect);
    bodyExpects.clear();
    bodyExpects.push_back(cast<ac::SourceExpectOp>(
        op(ac::SourceExpectOp::getOperationName(),
           {rule.getResult(8), rule.getResult(9)}, {},
           {f("kind", b.getStringAttr("assert")), f("location", span()),
            f("ac.check_id", firstCheckID)})));
    bodyExpects.push_back(cast<ac::SourceExpectOp>(
        op(ac::SourceExpectOp::getOperationName(),
           {rule.getResult(10), rule.getResult(11)}, {},
           {f("kind", b.getStringAttr("assert")), f("location", span()),
            f("ac.check_id", secondCheckID),
            f("ac.message", b.getStringAttr("second"))})));
    auto literalItem = d({f("kind", b.getStringAttr("literal")),
                          f("text", b.getStringAttr("only"))});
    auto valueItem = [&](unsigned ordinal) {
      return d({f("kind", b.getStringAttr("value")),
                f("ordinal", b.getI32IntegerAttr(ordinal))});
    };
    observations.clear();
    observations.push_back(cast<ac::SourceObserveOp>(
        op(ac::SourceObserveOp::getOperationName(), {rule.getResult(1)}, {},
           {f("kind", b.getStringAttr("log")),
            f("spec", d({f("level", b.getStringAttr("debug")),
                         f("event", b.getStringAttr("literal")),
                         f("items", b.getArrayAttr({literalItem}))})),
            f("ac.observation_id", firstObservationID),
            f("ac.location", span())})));
    observations.push_back(cast<ac::SourceObserveOp>(
        op(ac::SourceObserveOp::getOperationName(),
           {rule.getResult(2), rule.getResult(3), rule.getResult(4),
            rule.getResult(5)},
           {},
           {f("kind", b.getStringAttr("log")),
            f("spec", d({f("level", b.getStringAttr("info")),
                         f("event", b.getStringAttr("values")),
                         f("items", b.getArrayAttr({valueItem(0), valueItem(1),
                                                    valueItem(2)}))})),
            f("ac.observation_id", secondObservationID),
            f("ac.location", span())})));
    definition->setAttr(
        "ac.transaction_wiring",
        wiring({d({f("registration", occurrence(2)),
                   f("ordinary_results", b.getI64IntegerAttr(0)),
                   f("candidates", b.getArrayAttr({}))})}));
    b.setInsertionPointToEnd(package->getBody());
    op(ac::SystemOp::getOperationName(), {}, {},
       {f("domain", b.getStringAttr("default")),
        f("entry", d({f("callee", mlir::FlatSymbolRefAttr::get(
                                      &context, definition.getSymName())),
                      f("parameters", b.getArrayAttr({})),
                      f("type_arguments", b.getArrayAttr({}))}))});
  }
  struct ConditionalValues {
    Value selector, one, zero, knownZero, knownOne, knownEither;
    Value thenPath, elsePath, thenBypass, elseBypass, fire;
  } conditional;

  Value binary(StringRef opcode, Value lhs, Value rhs) {
    return op(ac::BitsBinaryOp::getOperationName(), {lhs, rhs}, {bit()},
              {f("opcode", b.getStringAttr(opcode))})
        ->getResult(0);
  }
  Value caseEqual(Value raw, Value literalValue) {
    return op(ac::BitsCompareOp::getOperationName(), {raw, literalValue},
              {bit()}, {f("predicate", b.getStringAttr("case_eq"))})
        ->getResult(0);
  }
  Value select(Value path, Value accepted, Value fallback) {
    return op(ac::BitsSelectOp::getOperationName(), {path, accepted, fallback},
              {bit()})
        ->getResult(0);
  }
  DictionaryAttr arm(bool whenTrue, uint64_t selection = 1) {
    return d({f("selection", b.getI64IntegerAttr(selection)),
              f("when_true", b.getBoolAttr(whenTrue))});
  }
  void setConditionalFire(Value fire) {
    auto yielded = cast<ac::YieldOp>(rule.getBody().front().back());
    yielded->setOperand(0, fire);
    for (unsigned slot : {1u, 2u, 15u, 17u})
      yielded.getOperand(slot).getDefiningOp<ac::BitsBinaryOp>()->setOperand(
          0, fire);
    conditional.fire = fire;
  }
  void changeRow(StringRef inventory, unsigned index, StringRef field,
                 Attribute value) {
    NamedAttrList metadata(
        rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission"));
    auto original = cast<ArrayAttr>(metadata.get(inventory));
    SmallVector<Attribute> rows(original.begin(), original.end());
    NamedAttrList row(cast<DictionaryAttr>(rows[index]));
    if (value)
      row.set(field, value);
    else
      row.erase(field);
    rows[index] = row.getDictionary(&context);
    metadata.set(inventory, b.getArrayAttr(rows));
    rule->setAttr("ac.sampled_admission", metadata.getDictionary(&context));
  }
  // Four controls deliberately use two distinct index spaces: root guard 0,
  // selection 1, then guard 2 and else guard 3. Checks start at control
  // count 4. Both effect kinds retain dedicated private result slots.
  void buildConditional(std::optional<uint64_t> literalSelector = {}) {
    buildMixedEffects();
    auto old = rule;
    NamedAttrList metadata(
        old->getAttrOfType<DictionaryAttr>("ac.sampled_admission"));
    SmallVector<Attribute> controls, required;
    for (unsigned i = 0; i < 4; ++i) {
      auto path = i < 2 ? b.getDictionaryAttr({}) : arm(i == 2);
      controls.push_back(
          d({f("kind",
               b.getStringAttr(i == 1 ? "branch_select" : "callable_guard")),
             f("id", checkID(i)),
             f("invocation", i < 2 ? b.getArrayAttr({}) : invocation(i)),
             f("predecessor", b.getI64IntegerAttr(int64_t(i) - 1)),
             f("path", path)}));
      required.push_back(
          d({f("id", checkID(i)), f("kind", b.getStringAttr("guard_known")),
             f("location", span())}));
    }
    metadata.set("controls", b.getArrayAttr(controls));
    SmallVector<Attribute> checks, observed;
    for (unsigned i = 0; i < 2; ++i) {
      checks.push_back(
          d({f("id", checkID(4 + i)), f("invocation", b.getArrayAttr({})),
             f("path", arm(i == 0))}));
      required.push_back(
          d({f("id", checkID(4 + i)), f("kind", b.getStringAttr("assert")),
             f("location", span())}));
      observed.push_back(d({f("id", observationID(i, 20 + i)),
                            f("value_count", b.getI64IntegerAttr(i ? 3 : 0)),
                            f("path", arm(i == 0))}));
    }
    metadata.set("body_checks", b.getArrayAttr(checks));
    metadata.set("observations", b.getArrayAttr(observed));
    b.setInsertionPoint(old);
    OperationState state(loc(), ac::RuleOp::getOperationName());
    state.addOperands(old.getOperands());
    state.addTypes(SmallVector<Type>(18, bit()));
    state.addAttributes(
        {f("name", old.getNameAttr()), f("occurrence", old.getOccurrence()),
         f("ac.required_checks", b.getArrayAttr(required)),
         f("ac.sampled_admission", metadata.getDictionary(&context))});
    state.addRegion();
    rule = cast<ac::RuleOp>(b.create(state));
    auto *body = new Block;
    rule.getBody().push_back(body);
    for (auto argument : old.getBody().front().getArguments())
      body->addArgument(argument.getType(), argument.getLoc());
    b.setInsertionPointToEnd(body);
    conditional.one = constant(1);
    conditional.zero = constant(0);
    conditional.selector =
        literalSelector ? constant(*literalSelector) : body->getArgument(2);
    conditional.knownZero = caseEqual(conditional.selector, conditional.zero);
    conditional.knownOne = caseEqual(conditional.selector, conditional.one);
    conditional.knownEither =
        binary("or", conditional.knownZero, conditional.knownOne);
    auto root = caseEqual(conditional.one, conditional.one);
    auto selected = binary("and", root, conditional.knownEither);
    conditional.thenPath = conditional.knownOne;
    conditional.elsePath = conditional.knownZero;
    auto thenReached = binary("and", selected, conditional.thenPath);
    auto accepted = caseEqual(conditional.one, conditional.one);
    conditional.thenBypass =
        select(conditional.thenPath, accepted, conditional.one);
    auto afterThen = binary("and", selected, conditional.thenBypass);
    auto elseReached = binary("and", afterThen, conditional.elsePath);
    conditional.elseBypass =
        select(conditional.elsePath, accepted, conditional.one);
    conditional.fire = binary("and", afterThen, conditional.elseBypass);
    auto thenEffect = binary("and", conditional.fire, conditional.thenPath);
    auto elseEffect = binary("and", conditional.fire, conditional.elsePath);
    op(ac::YieldOp::getOperationName(),
       {conditional.fire, thenEffect, elseEffect, conditional.selector,
        conditional.one, conditional.selector, conditional.one, conditional.one,
        conditional.selector, root, conditional.one, thenReached,
        conditional.one, elseReached, conditional.one, thenEffect,
        conditional.one, elseEffect});

    for (auto terminal : expects)
      terminal.erase();
    expects.clear();
    auto &moduleBody = definition.getBody().front();
    b.setInsertionPoint(moduleBody.getTerminator());
    for (unsigned i = 0; i < 4; ++i)
      expects.push_back(cast<ac::SourceExpectOp>(
          op(ac::SourceExpectOp::getOperationName(),
             {rule.getResult(6 + 2 * i), rule.getResult(7 + 2 * i),
              moduleBody.getArgument(1), moduleBody.getArgument(2)},
             {},
             {f("kind", b.getStringAttr("guard_known")), f("location", span()),
              f("ac.check_id", checkID(i))})));
    expect = expects.front();
    for (unsigned i = 0; i < 2; ++i) {
      bodyExpects[i]->setOperands(
          {rule.getResult(14 + 2 * i), rule.getResult(15 + 2 * i)});
      bodyExpects[i]->setAttr("ac.check_id", checkID(4 + i));
    }
    observations[0]->setOperand(0, rule.getResult(1));
    observations[1]->setOperands({rule.getResult(2), rule.getResult(3),
                                  rule.getResult(4), rule.getResult(5)});
    old.erase();
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
  std::string printPackage() {
    std::string text;
    llvm::raw_string_ostream stream(text);
    package->print(stream);
    stream << '\n';
    return text;
  }
  DictionaryAttr indexedCheckID(uint64_t registration, uint64_t check,
                                uint64_t obligation = 0) {
    return d({f("registration", occurrence(registration)),
              f("check", occurrence(check)),
              f("obligation", b.getI64IntegerAttr(obligation))});
  }
  DictionaryAttr wiring(ArrayRef<Attribute> writers) {
    return d({f("states", b.getArrayAttr({})),
              f("incoming", b.getArrayAttr({})),
              f("writers", b.getArrayAttr(writers))});
  }
  ac::RuleOp cloneSampledRule(uint64_t index) {
    auto registration = 20 + 2 * index;
    auto id = indexedCheckID(registration, registration + 1);
    auto *copy = rule->clone();
    copy->setAttr("name", b.getStringAttr("tick_" + std::to_string(index)));
    copy->setAttr("occurrence", occurrence(registration));
    copy->setAttr("ac.required_checks",
                  b.getArrayAttr({d({f("id", id),
                                     f("kind", b.getStringAttr("guard_known")),
                                     f("location", span())})}));
    NamedAttrList sampledFields(
        copy->getAttrOfType<DictionaryAttr>("ac.sampled_admission"));
    auto rows = cast<ArrayAttr>(sampledFields.get("controls"));
    NamedAttrList guardFields(cast<DictionaryAttr>(rows[0]));
    guardFields.set("id", id);
    sampledFields.set("controls",
                      b.getArrayAttr({guardFields.getDictionary(&context)}));
    copy->setAttr("ac.sampled_admission",
                  sampledFields.getDictionary(&context));
    definition.getBody().front().getOperations().insert(
        definition.getBody().front().getTerminator()->getIterator(), copy);
    auto clonedRule = cast<ac::RuleOp>(copy);
    auto *expectCopy = expect->clone();
    expectCopy->setOperands({clonedRule.getResult(1), clonedRule.getResult(2),
                             definition.getBody().front().getArgument(1),
                             definition.getBody().front().getArgument(2)});
    expectCopy->setAttr("ac.check_id", id);
    definition.getBody().front().getOperations().insert(
        definition.getBody().front().getTerminator()->getIterator(),
        expectCopy);
    return clonedRule;
  }
  ac::RuleOp addOrdinaryCheck(StringRef kind, uint64_t index) {
    auto registration = 40 + 2 * index;
    auto id = indexedCheckID(registration, registration + 1);
    auto &body = definition.getBody().front();
    b.setInsertionPoint(body.getTerminator());
    OperationState state(loc(), ac::RuleOp::getOperationName());
    state.addOperands(body.getArgument(0));
    state.addTypes({bit(), bit()});
    state.addAttributes(
        {f("name", b.getStringAttr((Twine(kind) + "_" + Twine(index)).str())),
         f("occurrence", occurrence(registration)),
         f("ac.required_checks",
           b.getArrayAttr({d({f("id", id), f("kind", b.getStringAttr(kind)),
                              f("location", span())})}))});
    state.addRegion();
    auto checked = cast<ac::RuleOp>(b.create(state));
    auto *checkedBody = new Block;
    checked.getBody().push_back(checkedBody);
    auto value = checkedBody->addArgument(bit(), loc());
    b.setInsertionPointToEnd(checkedBody);
    op(ac::YieldOp::getOperationName(), {value, value});
    b.setInsertionPoint(body.getTerminator());
    op(ac::SourceExpectOp::getOperationName(), checked.getResults(), {},
       {f("kind", b.getStringAttr(kind)), f("location", span()),
        f("ac.check_id", id)});
    return checked;
  }
  ac::ModuleOp addPlainDefinition() {
    b.setInsertionPointToEnd(package->getBody());
    OperationState state(loc(), ac::ModuleOp::getOperationName());
    state.addAttributes(
        {f("sym_name", b.getStringAttr("Other")),
         f("source_owner", d({f("package", b.getStringAttr("")),
                              f("path", b.getStringAttr("other.py"))})),
         f("parameters", b.getArrayAttr({})),
         f("type_parameters", b.getArrayAttr({})),
         f("input_names", b.getArrayAttr({})),
         f("output_names", b.getArrayAttr({})),
         f("function_type", TypeAttr::get(b.getFunctionType({}, {})))});
    state.addRegion();
    auto other = cast<ac::ModuleOp>(b.create(state));
    auto *body = new Block;
    other.getBody().push_back(body);
    b.setInsertionPointToEnd(body);
    op(ac::YieldOp::getOperationName());
    return other;
  }
  LogicalResult verifyAdmission(uint64_t budgetLimit = 1 << 20) {
    ac::HardwareAnalysis analysis(*package);
    ac::HardwareBindings bindings;
    bindings.owner = definition;
    detail::SourceCheckBudget budget({budgetLimit});
    detail::SampledAdmissionValidation validation(analysis, budget);
    auto view = validation.verify(rule, bindings);
    if (failed(view))
      return failure();
    return validation.verifyInvocation(analysis, budget);
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
       SampledAndGuardWitnessesCannotBorrowValidityOnlyPortExemption) {
  for (unsigned mutation = 0; mutation < 3; ++mutation) {
    SCOPED_TRACE(mutation);
    build(false, 1);
    auto &body = definition.getBody().front();
    body.addArgument(bit(), loc());
    definition->setAttr("function_type", TypeAttr::get(b.getFunctionType({bit(), bit(), bit(), bit()}, {})));
    definition->setAttr("input_names", b.getStrArrayAttr({"enabled", "pyc_clk", "pyc_rst", "available"}));
    definition->setAttr("ac.internal_ports", d({
        f("source_inputs", b.getI64IntegerAttr(3)),
        f("source_outputs", b.getI64IntegerAttr(0)),
        f("root_inputs", b.getArrayAttr({b.getI64IntegerAttr(1)}))}));
    definition->setAttr("ac.input_validity", b.getArrayAttr({b.getI64IntegerAttr(3), b.getI64IntegerAttr(-1), b.getI64IntegerAttr(-1)}));
    definition->setAttr("ac.output_validity", b.getArrayAttr({}));
    auto wiringProof = [&] {
      ac::HardwareAnalysis analysis(*package);
      ac::HardwareBindings bindings;
      bindings.owner = definition;
      detail::SourceCheckBudget budget({1 << 22});
      detail::SampledAdmissionValidation validation(analysis, budget);
      return detail::verifyTransactionWiring(analysis, {{definition, bindings}}, validation, budget);
    };
    definition->setAttr("ac.transaction_wiring", wiring({d({
        f("registration", occurrence(2)), f("ordinary_results", b.getI64IntegerAttr(0)),
        f("candidates", b.getArrayAttr({}))})}));
    ASSERT_TRUE(succeeded(capture(wiringProof))) << diagnostics;
    definition->removeAttr("ac.transaction_wiring");
    if (mutation >= 1)
      rule->removeAttr("ac.sampled_admission");
    if (mutation == 2)
      rule->removeAttr("ac.required_checks");
    EXPECT_TRUE(failed(capture(wiringProof)));
    EXPECT_NE(diagnostics.find("transaction_wiring companion"), std::string::npos)
        << diagnostics;
  }
}

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
       SampledCatalogOrdinalIndexPreservesSemanticCheckIdentity) {
  build(false, {}, 2);
  ASSERT_TRUE(succeeded(capture([&] { return verifyDefinition(); }))) << diagnostics;
  NamedAttrList equivalent(expect->getAttrOfType<DictionaryAttr>("ac.check_id"));
  equivalent.set("obligation", b.getIntegerAttr(b.getIntegerType(128), 0));
  expect->setAttr("ac.check_id", b.getDictionaryAttr(equivalent));
  EXPECT_TRUE(succeeded(capture([&] { return verifyDefinition(); }))) << diagnostics;
  equivalent.set("check", occurrence(99));
  expect->setAttr("ac.check_id", b.getDictionaryAttr(equivalent));
  EXPECT_TRUE(failed(capture([&] { return verifyDefinition(); })));
  EXPECT_NE(diagnostics.find("orphan source check"), std::string::npos) << diagnostics;
}

TEST_F(HardwareSampledAdmissionTest,
       OnePreparedContextProvesMultipleRulesInTheSameDefinition) {
  build();
  auto second = cloneSampledRule(0);
  ac::HardwareAnalysis analysis(*package);
  ac::HardwareBindings bindings;
  bindings.owner = definition;
  detail::SourceCheckBudget budget({1 << 20});
  detail::SampledAdmissionValidation validation(analysis, budget);
  auto prepared =
      capture([&] { return validation.prepare(definition, bindings); });
  ASSERT_TRUE(succeeded(prepared)) << diagnostics;
  ASSERT_NE(*prepared, nullptr);
  for (auto candidate : {rule, second}) {
    auto view = capture([&] { return validation.verify(candidate, bindings); });
    ASSERT_TRUE(succeeded(view)) << diagnostics;
  }
  auto repeatedRule =
      capture([&] { return validation.verify(rule, bindings); });
  ASSERT_TRUE(succeeded(repeatedRule)) << diagnostics;
  auto repeatedPrepared =
      capture([&] { return validation.prepare(definition, bindings); });
  ASSERT_TRUE(succeeded(repeatedPrepared)) << diagnostics;
  EXPECT_EQ(*repeatedPrepared, *prepared);
  EXPECT_TRUE(succeeded(capture([&] {
    return validation.verifyInvocation(analysis, budget);
  }))) << diagnostics;
}

TEST_F(HardwareSampledAdmissionTest,
       CachedValidRuleDoesNotAuthenticateDifferentInvalidRule) {
  build();
  auto invalid = cloneSampledRule(0);
  auto *invalidYield = invalid.getBody().front().getTerminator();
  invalidYield->setOperand(0, invalid.getBody().front().getArgument(2));
  ac::HardwareAnalysis analysis(*package);
  ac::HardwareBindings bindings;
  bindings.owner = definition;
  detail::SourceCheckBudget budget({1 << 20});
  detail::SampledAdmissionValidation validation(analysis, budget);
  ASSERT_TRUE(succeeded(capture([&] {
    return validation.verify(rule, bindings);
  }))) << diagnostics;
  ASSERT_TRUE(succeeded(capture([&] {
    return validation.verify(rule, bindings);
  }))) << diagnostics;
  EXPECT_TRUE(
      failed(capture([&] { return validation.verify(invalid, bindings); })));
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(HardwareSampledAdmissionTest,
       ValidationInvocationRejectsDifferentAnalysisAndBudgetIdentity) {
  build();
  ac::HardwareAnalysis analysis(*package);
  ac::HardwareAnalysis otherAnalysis(*package);
  ac::HardwareBindings bindings;
  bindings.owner = definition;
  detail::SourceCheckBudget budget({1 << 20});
  detail::SourceCheckBudget otherBudget({1 << 20});
  detail::SampledAdmissionValidation validation(analysis, budget);
  ASSERT_TRUE(succeeded(capture([&] {
    return validation.verify(rule, bindings);
  }))) << diagnostics;
  EXPECT_TRUE(failed(capture(
      [&] { return validation.verifyInvocation(otherAnalysis, budget); })));
  EXPECT_FALSE(diagnostics.empty());
  EXPECT_TRUE(failed(capture(
      [&] { return validation.verifyInvocation(analysis, otherBudget); })));
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(HardwareSampledAdmissionTest,
       ValidationPrepareRejectsDefinitionFromForeignPackage) {
  build();
  mlir::OwningOpRef<mlir::ModuleOp> foreignPackage(
      mlir::ModuleOp::create(loc()));
  auto *copy = definition->clone();
  copy->setAttr("sym_name", b.getStringAttr("Foreign"));
  foreignPackage->getBody()->push_back(copy);
  auto foreign = cast<ac::ModuleOp>(copy);
  ac::HardwareAnalysis analysis(*package);
  ac::HardwareBindings bindings;
  bindings.owner = foreign;
  detail::SourceCheckBudget budget({1 << 20});
  detail::SampledAdmissionValidation validation(analysis, budget);
  auto prepared =
      capture([&] { return validation.prepare(foreign, bindings); });
  EXPECT_TRUE(failed(prepared));
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(HardwareSampledAdmissionTest,
       PreparedContextRejectsDifferentDefinitionAndFullBindings) {
  build();
  auto other = addPlainDefinition();
  ac::HardwareAnalysis analysis(*package);
  ac::HardwareBindings bindings;
  bindings.owner = definition;
  detail::SourceCheckBudget prepareBudget({1 << 20});
  auto prepared = capture([&] {
    return detail::prepareSampledBoundaries(definition, analysis, bindings,
                                            prepareBudget);
  });
  ASSERT_TRUE(succeeded(prepared)) << diagnostics;
  ASSERT_TRUE(prepared->has_value());

  ac::HardwareBindings otherBindings;
  otherBindings.owner = other;
  detail::SourceCheckBudget definitionBudget({1 << 20});
  EXPECT_TRUE(failed(capture([&] {
    return (**prepared).verifyFor(other, otherBindings, definitionBudget);
  })));
  EXPECT_FALSE(diagnostics.empty());

  for (bool typeBinding : {false, true}) {
    SCOPED_TRACE(typeBinding ? "type binding" : "integer binding");
    ac::HardwareBindings changedBindings;
    changedBindings.owner = definition;
    if (typeBinding)
      changedBindings.types["T"] = bit();
    else
      changedBindings.integers["N"] = b.getI64IntegerAttr(1);
    detail::SourceCheckBudget bindingsBudget({1 << 20});
    EXPECT_TRUE(failed(capture([&] {
      return (**prepared)
          .verifyFor(definition, changedBindings, bindingsBudget);
    })));
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(HardwareSampledAdmissionTest,
       BoundaryFactoryRejectsMalformedWiringMissingWriterAndInvalidSuffix) {
  auto rejectsFactory = [&](StringRef label, auto mutation) {
    SCOPED_TRACE(label.str());
    build();
    mutation();
    auto before = printPackage();
    ac::HardwareAnalysis analysis(*package);
    ac::HardwareBindings bindings;
    bindings.owner = definition;
    detail::SourceCheckBudget budget({1 << 20});
    auto prepared = capture([&] {
      return detail::prepareSampledBoundaries(definition, analysis, bindings,
                                              budget);
    });
    EXPECT_TRUE(failed(prepared));
    EXPECT_FALSE(diagnostics.empty());
    EXPECT_EQ(printPackage(), before);
  };
  rejectsFactory("malformed wiring", [&] {
    definition->setAttr("ac.transaction_wiring", b.getDictionaryAttr({}));
  });
  rejectsFactory("missing writer", [&] {
    definition->setAttr("ac.transaction_wiring", wiring({}));
  });
  rejectsFactory("invalid suffix", [&] {
    auto required = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
    rule->setAttr("ac.required_checks",
                  b.getArrayAttr({required[0], required[0]}));
  });
}

TEST_F(HardwareSampledAdmissionTest,
       PreparedBoundariesRetainFireGuardAssertAndRangeSuffixes) {
  build();
  auto asserted = addOrdinaryCheck("assert", 0);
  auto ranged = addOrdinaryCheck("range", 1);
  ac::HardwareAnalysis analysis(*package);
  ac::HardwareBindings bindings;
  bindings.owner = definition;
  detail::SourceCheckBudget budget({1 << 20});
  auto prepared = capture([&] {
    return detail::prepareSampledBoundaries(definition, analysis, bindings,
                                            budget);
  });
  ASSERT_TRUE(succeeded(prepared)) << diagnostics;
  ASSERT_TRUE(prepared->has_value());
  EXPECT_EQ((**prepared).getScope().root, definition);
  EXPECT_TRUE((**prepared).getScope().path.empty());
  auto contains = [&](Value value) {
    return llvm::any_of(
        (**prepared).getBoundaries(),
        [&](const auto &endpoint) { return endpoint.value == value; });
  };
  for (Value value : rule->getResults())
    EXPECT_TRUE(contains(value));
  for (Value value : asserted->getResults())
    EXPECT_TRUE(contains(value));
  for (Value value : ranged->getResults())
    EXPECT_TRUE(contains(value));
}

TEST_F(HardwareSampledAdmissionTest,
       OrdinaryDefinitionUsesTheSuccessfulNullPreparedFastPath) {
  build();
  rule->removeAttr("ac.sampled_admission");
  auto required = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
  NamedAttrList requiredFields(cast<DictionaryAttr>(required[0]));
  requiredFields.set("kind", b.getStringAttr("assert"));
  rule->setAttr("ac.required_checks",
                b.getArrayAttr({requiredFields.getDictionary(&context)}));
  expect->setAttr("kind", b.getStringAttr("assert"));
  expect->setOperands({rule.getResult(1), rule.getResult(2)});
  ac::HardwareAnalysis analysis(*package);
  ac::HardwareBindings bindings;
  bindings.owner = definition;
  detail::SourceCheckBudget budget({1 << 20});
  detail::SampledAdmissionValidation validation(analysis, budget);
  auto prepared =
      capture([&] { return validation.prepare(definition, bindings); });
  ASSERT_TRUE(succeeded(prepared)) << diagnostics;
  EXPECT_EQ(*prepared, nullptr);
}

TEST_F(HardwareSampledAdmissionTest,
       BoundaryFactoryTinyBudgetRejectsWithoutIrMutation) {
  build();
  auto before = printPackage();
  ac::HardwareAnalysis analysis(*package);
  ac::HardwareBindings bindings;
  bindings.owner = definition;
  detail::SourceCheckBudget budget({1});
  detail::SampledAdmissionValidation validation(analysis, budget);
  auto prepared =
      capture([&] { return validation.prepare(definition, bindings); });
  EXPECT_TRUE(failed(prepared));
  EXPECT_NE(diagnostics.find("source-check plan exceeds analysis work budget"),
            std::string::npos)
      << diagnostics;
  EXPECT_EQ(printPackage(), before);
}

TEST_F(HardwareSampledAdmissionTest,
       IdentityEndpointRejectsStaleResolvedTypeAndFreshCallRebuilds) {
  build();
  ac::HardwareAnalysis analysis(*package);
  ac::HardwareBindings bindings;
  bindings.owner = definition;
  detail::HardwareIdentityScope scope{definition, bindings, {}};
  detail::SourceCheckBudget initialBudget({1 << 20});
  auto endpoint = capture([&] {
    return detail::makeIdentityEndpoint(
        definition.getBody().front().getArgument(0), scope, analysis,
        initialBudget);
  });
  ASSERT_TRUE(succeeded(endpoint)) << diagnostics;

  auto widened = ac::BitsType::get(&context, literal(2));
  definition.getBody().front().getArgument(0).setType(widened);
  SmallVector<Type> inputs(definition.getFunctionType().getInputs());
  inputs[0] = widened;
  definition->setAttr("function_type",
                      TypeAttr::get(b.getFunctionType(
                          inputs, definition.getFunctionType().getResults())));
  detail::HardwareIdentityStopPolicy stops{
      detail::HardwareIdentityTraversal::DefinitionLocal, {}};
  detail::SourceCheckBudget staleBudget({1 << 20});
  auto stale = capture([&] {
    return detail::normalizeIdentityEndpoint(*endpoint, stops, analysis,
                                             staleBudget);
  });
  EXPECT_TRUE(failed(stale));
  EXPECT_NE(diagnostics.find("stale identity endpoint resolved type"),
            std::string::npos)
      << diagnostics;

  detail::SourceCheckBudget freshBudget({1 << 20});
  auto fresh = capture([&] {
    return detail::makeIdentityEndpoint(
        definition.getBody().front().getArgument(0), scope, analysis,
        freshBudget);
  });
  ASSERT_TRUE(succeeded(fresh)) << diagnostics;
  auto normalized = capture([&] {
    return detail::normalizeIdentityEndpoint(*fresh, stops, analysis,
                                             freshBudget);
  });
  EXPECT_TRUE(succeeded(normalized)) << diagnostics;
  EXPECT_TRUE(
      ac::areEquivalentHardwareTypes(normalized->resolvedType, widened));
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
    SmallVector<Attribute> rows(metadata.getAs<ArrayAttr>("controls").begin(),
                                metadata.getAs<ArrayAttr>("controls").end());
    NamedAttrList changed(cast<DictionaryAttr>(rows[1]));
    changed.set("predecessor", b.getI64IntegerAttr(-1));
    rows[1] = changed.getDictionary(&context);
    NamedAttrList fields(metadata);
    fields.set("controls", b.getArrayAttr(rows));
    rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
  });
  rejectsTwo("guard order", [&] {
    auto metadata = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    SmallVector<Attribute> rows(metadata.getAs<ArrayAttr>("controls").begin(),
                                metadata.getAs<ArrayAttr>("controls").end());
    std::swap(rows[0], rows[1]);
    NamedAttrList fields(metadata);
    fields.set("controls", b.getArrayAttr(rows));
    rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
  });
  rejectsTwo("duplicate guard id", [&] {
    auto metadata = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    SmallVector<Attribute> rows(metadata.getAs<ArrayAttr>("controls").begin(),
                                metadata.getAs<ArrayAttr>("controls").end());
    NamedAttrList changed(cast<DictionaryAttr>(rows[1]));
    changed.set("id", checkID(0));
    rows[1] = changed.getDictionary(&context);
    NamedAttrList fields(metadata);
    fields.set("controls", b.getArrayAttr(rows));
    rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
  });
  rejectsTwo("duplicate invocation", [&] {
    auto metadata = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    SmallVector<Attribute> rows(metadata.getAs<ArrayAttr>("controls").begin(),
                                metadata.getAs<ArrayAttr>("controls").end());
    NamedAttrList changed(cast<DictionaryAttr>(rows[1]));
    changed.set("invocation", b.getArrayAttr({}));
    rows[1] = changed.getDictionary(&context);
    NamedAttrList fields(metadata);
    fields.set("controls", b.getArrayAttr(rows));
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
    EXPECT_TRUE(failed(capture([&] {
      ac::HardwareAnalysis analysis(*package);
      return analysis.getSourceCheckPlan();
    }))) << "invalid sampling operands admitted: "
         << label.str();
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
  rejects("old six-field guard inventory", [&] {
    NamedAttrList metadata(
        rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission"));
    auto rows = cast<ArrayAttr>(metadata.get("controls"));
    NamedAttrList row(cast<DictionaryAttr>(rows[0]));
    row.erase("kind");
    row.erase("path");
    metadata.erase("controls");
    metadata.set("guards", b.getArrayAttr({row.getDictionary(&context)}));
    rule->setAttr("ac.sampled_admission", metadata.getDictionary(&context));
  });
  rejects("old four-field schema", [&] {
    auto metadata = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    rule->setAttr("ac.sampled_admission",
                  d({f("clock_capture", metadata.get("clock_capture")),
                     f("reset_capture", metadata.get("reset_capture")),
                     f("fire_result", metadata.get("fire_result")),
                     f("guards", metadata.get("controls"))}));
  });
  for (StringRef field : {"body_checks", "observations"}) {
    rejects((Twine("missing ") + field).str(), [&] {
      NamedAttrList fields(
          rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission"));
      fields.erase(field);
      rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
    });
  }
  rejects("missing guards", [&] {
    NamedAttrList fields(
        rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission"));
    fields.erase("controls");
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
    fields.set("controls", b.getArrayAttr({}));
    rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
  });
}

TEST_F(HardwareSampledAdmissionTest,
       GuardOrderIdentityAndResultLayoutAreAuthoritative) {
  rejects("first predecessor", [&] {
    auto metadata = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    auto row = cast<DictionaryAttr>(metadata.getAs<ArrayAttr>("controls")[0]);
    NamedAttrList changed(row);
    changed.set("predecessor", b.getI64IntegerAttr(0));
    NamedAttrList fields(metadata);
    fields.set("controls", b.getArrayAttr({changed.getDictionary(&context)}));
    rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
  });
  rejects("guard extra field", [&] {
    auto metadata = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    auto row = cast<DictionaryAttr>(metadata.getAs<ArrayAttr>("controls")[0]);
    NamedAttrList changed(row);
    changed.set("extra", b.getUnitAttr());
    NamedAttrList fields(metadata);
    fields.set("controls", b.getArrayAttr({changed.getDictionary(&context)}));
    rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
  });
  rejects("wrong obligation ordinal", [&] {
    auto metadata = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    auto row = cast<DictionaryAttr>(metadata.getAs<ArrayAttr>("controls")[0]);
    NamedAttrList changed(row);
    changed.set("id", checkID(4));
    NamedAttrList fields(metadata);
    fields.set("controls", b.getArrayAttr({changed.getDictionary(&context)}));
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
    auto row = cast<DictionaryAttr>(metadata.getAs<ArrayAttr>("controls")[0]);
    NamedAttrList changed(row);
    changed.set("invocation", b.getArrayAttr({b.getStringAttr("frame")}));
    NamedAttrList fields(metadata);
    fields.set("controls", b.getArrayAttr({changed.getDictionary(&context)}));
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

TEST_F(HardwareSampledAdmissionTest,
       MixedGuardAssertionsAndObservationsUseExactFinalLayout) {
  buildMixedEffects();
  ASSERT_TRUE(succeeded(capture([&] { return mlir::verify(*package); })))
      << diagnostics;
  ac::HardwareAnalysis analysis(*package);
  ac::HardwareBindings bindings;
  bindings.owner = definition;
  detail::SourceCheckBudget budget({1 << 20});
  auto view = capture([&] {
    return detail::parseSampledAdmission(rule, analysis, bindings, budget);
  });
  ASSERT_TRUE(succeeded(view)) << diagnostics;
  EXPECT_EQ(view->fireResult, 0u);
  EXPECT_TRUE(rule.getResult(view->fireResult).use_empty());
  EXPECT_EQ(view->checkBase, 6u);
  EXPECT_EQ(view->totalResults, 12u);
  ASSERT_EQ(view->controls.size(), 1u);
  ASSERT_EQ(view->bodyChecks.size(), 2u);
  ASSERT_EQ(view->observations.size(), 2u);
  EXPECT_EQ(view->observations[0].pathResult, 1u);
  EXPECT_EQ(view->observations[0].valueCount, 0u);
  EXPECT_EQ(view->observations[1].pathResult, 2u);
  EXPECT_EQ(view->observations[1].firstValueResult, 3u);
  EXPECT_EQ(view->observations[1].valueCount, 3u);
  EXPECT_EQ(view->controls[0].rawResult, 6u);
  EXPECT_EQ(view->controls[0].reachedResult, 7u);
  EXPECT_EQ(view->bodyChecks[0].conditionResult, 8u);
  EXPECT_EQ(view->bodyChecks[0].pathResult, 9u);
  EXPECT_EQ(view->bodyChecks[1].conditionResult, 10u);
  EXPECT_EQ(view->bodyChecks[1].pathResult, 11u);
  auto yielded = mlir::cast<ac::YieldOp>(rule.getBody().front().back());
  EXPECT_EQ(view->fire, yielded.getOperand(0));
  EXPECT_EQ(view->bodyChecks[0].condition, yielded.getOperand(8));
  EXPECT_EQ(view->bodyChecks[0].path, yielded.getOperand(9));
  EXPECT_NE(view->bodyChecks[0].condition, view->fire);
  EXPECT_EQ(view->bodyChecks[0].path, view->fire);
  EXPECT_EQ(view->bodyChecks[1].path, view->fire);
  for (auto terminal : bodyExpects)
    EXPECT_TRUE(terminal.getSampling().empty());
  auto required = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
  ASSERT_EQ(required.size(), 3u);
  EXPECT_EQ(
      cast<DictionaryAttr>(required[0]).getAs<StringAttr>("kind").getValue(),
      "guard_known");
  EXPECT_EQ(
      cast<DictionaryAttr>(required[1]).getAs<StringAttr>("kind").getValue(),
      "assert");
  EXPECT_EQ(
      cast<DictionaryAttr>(required[2]).getAs<StringAttr>("kind").getValue(),
      "assert");
  detail::SampledAdmissionValidation validation(analysis, budget);
  auto prepared = validation.prepare(definition, bindings);
  ASSERT_TRUE(succeeded(prepared)) << diagnostics;
  ASSERT_NE(*prepared, nullptr);
  EXPECT_TRUE(succeeded(detail::proveSampledAdmission(
      *view, **prepared, analysis, bindings, budget)))
      << diagnostics;
  auto checkBindings = capture([&] {
    return detail::definitionSourceChecks(analysis, definition, bindings,
                                          validation, budget);
  });
  ASSERT_TRUE(succeeded(checkBindings)) << diagnostics;
  ASSERT_EQ(checkBindings->size(), 3u);
  EXPECT_EQ((*checkBindings)[0].kind.getValue(), "guard_known");
  EXPECT_EQ((*checkBindings)[0].requiredIndex, 0u);
  EXPECT_FALSE((*checkBindings)[0].samplingBindingIndex);
  for (unsigned index = 1; index < 3; ++index) {
    EXPECT_EQ((*checkBindings)[index].kind.getValue(), "assert");
    EXPECT_EQ((*checkBindings)[index].requiredIndex, index);
    EXPECT_FALSE((*checkBindings)[index].samplingBindingIndex);
  }
  auto fullPlan = capture([&] { return analysis.getSourceCheckPlan(); });
  ASSERT_TRUE(succeeded(fullPlan)) << diagnostics;
  ASSERT_EQ(fullPlan->bindings.size(), 3u);
  ASSERT_TRUE(fullPlan->bindings[0].samplingBindingIndex);
  EXPECT_FALSE(fullPlan->bindings[1].samplingBindingIndex);
  EXPECT_FALSE(fullPlan->bindings[2].samplingBindingIndex);
}

TEST_F(HardwareSampledAdmissionTest,
       BodyEffectSlotsAndTerminalAssociationsFailClosed) {
  auto rejectsMixed = [&](StringRef label,
                          const std::function<void()> &mutate) {
    SCOPED_TRACE(label.str());
    buildMixedEffects();
    mutate();
    EXPECT_TRUE(failed(capture([&] {
      auto plan = ac::HardwareAnalysis(*package).getSourceCheckPlan();
      return success(succeeded(plan));
    }))) << "mutation unexpectedly admitted: "
         << label.str();
    EXPECT_FALSE(diagnostics.empty());
  };
  rejectsMixed("direct fire substituted for asserted path",
               [&] { bodyExpects[0]->setOperand(1, rule.getResult(0)); });
  rejectsMixed("assertion condition and path roles swapped", [&] {
    auto condition = bodyExpects[0].getCondition();
    bodyExpects[0]->setOperand(0, bodyExpects[0].getPath());
    bodyExpects[0]->setOperand(1, condition);
  });
  rejectsMixed("direct fire substituted for observation path",
               [&] { observations[0]->setOperand(0, rule.getResult(0)); });
  rejectsMixed("assertion acquired sampling pins", [&] {
    bodyExpects[0]->insertOperands(
        2, {definition.getBody().front().getArgument(1),
            definition.getBody().front().getArgument(2)});
  });
  rejectsMixed("observation payload roles share one result", [&] {
    observations[1]->setOperand(2, observations[1]->getOperand(1));
  });
  rejectsMixed("observation payload order swapped", [&] {
    auto first = observations[1]->getOperand(1);
    observations[1]->setOperand(1, observations[1]->getOperand(2));
    observations[1]->setOperand(2, first);
  });
  rejectsMixed("missing body assertion terminal",
               [&] { bodyExpects[0].erase(); });
  rejectsMixed("extra body assertion terminal", [&] {
    auto *copy = bodyExpects[0]->clone();
    definition.getBody().front().getOperations().insert(
        definition.getBody().front().getTerminator()->getIterator(), copy);
  });
  rejectsMixed("effect payload escapes into unrelated computation", [&] {
    b.setInsertionPoint(definition.getBody().front().getTerminator());
    op(ac::BitsBinaryOp::getOperationName(),
       {rule.getResult(3), rule.getResult(3)}, {bit()},
       {f("opcode", b.getStringAttr("add"))});
  });
  rejectsMixed("observation value count disagrees", [&] {
    auto metadata = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    SmallVector<Attribute> rows(
        metadata.getAs<ArrayAttr>("observations").begin(),
        metadata.getAs<ArrayAttr>("observations").end());
    NamedAttrList changed(cast<DictionaryAttr>(rows[1]));
    changed.set("value_count", b.getI64IntegerAttr(2));
    rows[1] = changed.getDictionary(&context);
    NamedAttrList fields(metadata);
    fields.set("observations", b.getArrayAttr(rows));
    rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
  });
}

TEST_F(HardwareSampledAdmissionTest,
       EnormousDeclaredEffectCountsRejectBeforeLayoutIteration) {
  buildMixedEffects();
  auto metadata = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
  SmallVector<Attribute> rows(metadata.getAs<ArrayAttr>("observations").begin(),
                              metadata.getAs<ArrayAttr>("observations").end());
  NamedAttrList changed(cast<DictionaryAttr>(rows[1]));
  changed.set("value_count",
              b.getI64IntegerAttr(std::numeric_limits<int64_t>::max()));
  rows[1] = changed.getDictionary(&context);
  NamedAttrList fields(metadata);
  fields.set("observations", b.getArrayAttr(rows));
  rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
  auto before = printPackage();
  EXPECT_TRUE(failed(capture([&] { return verifyAdmission(); })));
  EXPECT_FALSE(diagnostics.empty());
  EXPECT_LT(diagnostics.size(), 4096u);
  EXPECT_EQ(before, printPackage());
}

TEST_F(HardwareSampledAdmissionTest,
       ConditionalControlsAcceptKnownZeroAndOneWithoutErasingBypassNodes) {
  for (std::optional<uint64_t> selector :
       {std::optional<uint64_t>(), std::optional<uint64_t>(0),
        std::optional<uint64_t>(1)}) {
    SCOPED_TRACE(selector ? std::to_string(*selector) : "dynamic");
    buildConditional(selector);
    ASSERT_TRUE(succeeded(capture([&] { return verifyAdmission(); })))
        << diagnostics;
    ac::HardwareAnalysis analysis(*package);
    ac::HardwareBindings bindings;
    bindings.owner = definition;
    detail::SourceCheckBudget budget({1 << 20});
    auto view = capture([&] {
      return detail::parseSampledAdmission(rule, analysis, bindings, budget);
    });
    ASSERT_TRUE(succeeded(view)) << diagnostics;
    ASSERT_EQ(view->controls.size(), 4u);
    EXPECT_EQ(view->controls[1].kind, detail::SampledControlKind::BranchSelect);
    EXPECT_EQ(view->controls[2].kind,
              detail::SampledControlKind::CallableGuard);
    EXPECT_EQ(view->controls[2].rawResult, 10u);
    EXPECT_EQ(view->controls[2].executionPath.selectionControl, 1u);
    EXPECT_TRUE(view->controls[2].executionPath.whenTrue);
    EXPECT_FALSE(view->controls[3].executionPath.whenTrue);
    EXPECT_EQ(view->bodyChecks[0].conditionResult, 14u);
    EXPECT_EQ(view->bodyChecks[1].conditionResult, 16u);
    EXPECT_TRUE(rule.getResult(0).use_empty());
    if (selector) {
      // The same KnownEither value is visited in Prefix and Acceptance modes.
      // Both raw-literal cases must preserve mode-specific validation.
      conditional.thenBypass.getDefiningOp()->setOperand(
          1, conditional.knownEither);
      EXPECT_TRUE(succeeded(capture([&] { return verifyAdmission(); })))
          << diagnostics;
    }
  }
}

TEST_F(HardwareSampledAdmissionTest,
       ConditionalMetadataRejectsMalformedRolesAndInvocationAncestry) {
  auto rejectsConditional = [&](StringRef label, auto mutation) {
    SCOPED_TRACE(label.str());
    buildConditional();
    ASSERT_TRUE(succeeded(capture([&] { return verifyAdmission(); })))
        << diagnostics;
    mutation();
    auto before = printPackage();
    EXPECT_TRUE(failed(capture([&] { return verifyAdmission(); })))
        << "conditional metadata mutation admitted: " << label.str();
    EXPECT_FALSE(diagnostics.empty());
    EXPECT_EQ(before, printPackage());
  };
  rejectsConditional("missing control role",
                     [&] { changeRow("controls", 1, "kind", {}); });
  rejectsConditional("missing path",
                     [&] { changeRow("controls", 2, "path", {}); });
  rejectsConditional("future selection",
                     [&] { changeRow("controls", 2, "path", arm(true, 3)); });
  rejectsConditional("callable used as selection",
                     [&] { changeRow("controls", 2, "path", arm(true, 0)); });
  rejectsConditional("wrong arm polarity",
                     [&] { changeRow("controls", 2, "path", arm(false)); });
  rejectsConditional("integer arm instead of BoolAttr", [&] {
    changeRow("controls", 2, "path",
              d({f("selection", b.getI64IntegerAttr(1)),
                 f("when_true", b.getI64IntegerAttr(1))}));
  });
  rejectsConditional("narrow selection index", [&] {
    changeRow("controls", 2, "path",
              d({f("selection", b.getI32IntegerAttr(1)),
                 f("when_true", b.getBoolAttr(true))}));
  });
  rejectsConditional("missing callable entry", [&] {
    changeRow("controls", 1, "invocation", invocation(99));
  });
  rejectsConditional("missing parent invocation", [&] {
    auto frame = invocation(2)[0];
    changeRow("controls", 2, "invocation", b.getArrayAttr({frame, frame}));
  });
  rejectsConditional("sibling invocation path", [&] {
    changeRow("controls", 1, "kind", b.getStringAttr("callable_guard"));
    changeRow("controls", 1, "invocation", invocation(1));
    changeRow("controls", 2, "kind", b.getStringAttr("branch_select"));
    changeRow("controls", 2, "invocation", invocation(1));
    changeRow("controls", 2, "path", b.getDictionaryAttr({}));
    changeRow("controls", 3, "path", arm(false, 2));
  });
  rejectsConditional("callee selection escapes owning arm", [&] {
    changeRow("controls", 3, "kind", b.getStringAttr("branch_select"));
    changeRow("controls", 3, "invocation", invocation(2));
    changeRow("controls", 3, "path", b.getDictionaryAttr({}));
  });
  rejectsConditional("body effect invocation has no callable", [&] {
    changeRow("body_checks", 0, "invocation", invocation(99));
  });
  rejectsConditional("effect path refers to future callable", [&] {
    changeRow("observations", 0, "path", arm(true, 3));
  });
}

TEST_F(HardwareSampledAdmissionTest,
       BodyInvocationAcceptsPositiveOrdinalsAtDifferentIntegerWidths) {
  for (bool receiver : {false, true})
    for (unsigned width : {16u, 32u, 64u, 128u}) {
      SCOPED_TRACE(::testing::Message()
                   << "receiver=" << receiver << " width=" << width);
      buildConditional();
      changeRow("controls", 2, "invocation",
                invocationIndex(b.getI64IntegerAttr(255), receiver));
      changeRow("body_checks", 0, "invocation",
                invocationIndex(b.getIntegerAttr(b.getIntegerType(width), 255),
                                receiver));
      auto before = printPackage();
      EXPECT_TRUE(succeeded(capture([&] { return verifyAdmission(); })))
          << diagnostics;
      EXPECT_EQ(before, printPackage());
    }
}

TEST_F(HardwareSampledAdmissionTest,
       BodyInvocationRejectsNegativeOrdinalsBeforeNumericIdentityMatching) {
  // Unsigned structural comparison alone would equate -1:i8 with 255:i64.
  // Invocation membership therefore cannot establish closed-schema validity.
  for (bool receiver : {false, true})
    for (unsigned width : {8u, 16u}) {
      SCOPED_TRACE(::testing::Message()
                   << "receiver=" << receiver << " width=" << width);
      buildConditional();
      auto positive = b.getI64IntegerAttr((uint64_t(1) << width) - 1);
      changeRow("controls", 2, "invocation",
                invocationIndex(positive, receiver));
      changeRow("body_checks", 0, "invocation",
                invocationIndex(positive, receiver));
      ASSERT_TRUE(succeeded(capture([&] { return verifyAdmission(); })))
          << diagnostics;
      changeRow("body_checks", 0, "invocation",
                invocationIndex(b.getIntegerAttr(b.getIntegerType(width), -1),
                                receiver));
      auto before = printPackage();
      EXPECT_TRUE(failed(capture([&] { return verifyAdmission(); })));
      EXPECT_NE(diagnostics.find("must be non-negative"), std::string::npos)
          << diagnostics;
      EXPECT_EQ(before, printPackage());
    }
}

TEST_F(HardwareSampledAdmissionTest,
       NumericEquivalentCallableAndSelectionIdentitiesCannotBeDuplicated) {
  for (bool selection : {false, true}) {
    SCOPED_TRACE(selection ? "selection site" : "callable invocation");
    buildConditional();
    ASSERT_TRUE(succeeded(capture([&] { return verifyAdmission(); })))
        << diagnostics;
    if (selection) {
      changeRow("controls", 3, "kind", b.getStringAttr("branch_select"));
      changeRow("controls", 3, "invocation", b.getArrayAttr({}));
      changeRow("controls", 3, "path", b.getDictionaryAttr({}));
      NamedAttrList id(checkID(3));
      id.set("check", occurrenceIndex(b.getI32IntegerAttr(5)));
      changeRow("controls", 3, "id", id.getDictionary(&context));
    } else {
      changeRow("controls", 3, "invocation",
                invocationIndex(b.getI32IntegerAttr(12)));
      changeRow("controls", 3, "path", arm(true));
    }
    auto before = printPackage();
    EXPECT_TRUE(failed(capture([&] { return verifyAdmission(); })));
    EXPECT_NE(diagnostics.find(selection
                                   ? "duplicate sampled selection site"
                                   : "duplicate sampled callable identity"),
              std::string::npos)
        << diagnostics;
    EXPECT_EQ(before, printPackage());
  }
}

TEST_F(HardwareSampledAdmissionTest,
       ConditionalPrefixGrammarRejectsKnownnessAndBypassForgeries) {
  auto rejectsProof = [&](StringRef label, auto mutation,
                          std::optional<uint64_t> selector = {}) {
    SCOPED_TRACE(label.str());
    buildConditional(selector);
    ASSERT_TRUE(succeeded(capture([&] { return verifyAdmission(); })))
        << diagnostics;
    b.setInsertionPoint(rule.getBody().front().getTerminator());
    mutation();
    EXPECT_TRUE(failed(capture([&] { return verifyAdmission(); })))
        << "conditional proof mutation admitted: " << label.str();
    EXPECT_FALSE(diagnostics.empty());
  };
  rejectsProof("known OR tests different raw endpoints", [&] {
    conditional.knownZero.getDefiningOp()->setOperand(
        0, rule.getBody().front().getArgument(0));
  });
  // Both mismatched comparisons reduce to literals, so reduction must not hide
  // the raw-identity obligation in the direct KnownEither matcher.
  for (uint64_t selector : {0u, 1u})
    rejectsProof(
        "literal known OR still authenticates identical raw",
        [&] {
          conditional.knownZero.getDefiningOp()->setOperand(
              0, selector ? conditional.zero : conditional.one);
        },
        selector);
  for (uint64_t selector : {0u, 1u})
    rejectsProof(
        "literal acceptance OR still authenticates identical raw",
        [&] {
          auto zeroTest = caseEqual(
              selector ? conditional.zero : conditional.one, conditional.zero);
          auto oneTest = caseEqual(conditional.selector, conditional.one);
          auto forged = binary("or", zeroTest, oneTest);
          conditional.thenBypass.getDefiningOp()->setOperand(1, forged);
        },
        selector);
  rejectsProof("known one alone cannot certify selector known zero", [&] {
    conditional.knownEither.getDefiningOp()->setOperand(0,
                                                        conditional.knownOne);
  });
  rejectsProof("unknown comparison literal", [&] {
    conditional.knownZero.getDefiningOp()->setOperand(1, constant(2));
  });
  rejectsProof("bypass fallback is false", [&] {
    conditional.thenBypass.getDefiningOp()->setOperand(2, conditional.zero);
  });
  rejectsProof("recursive bypass in selection path", [&] {
    auto recursive =
        select(conditional.thenPath, conditional.knownOne, conditional.one);
    conditional.thenBypass.getDefiningOp()->setOperand(0, recursive);
  });
  rejectsProof("recursive bypass in acceptance", [&] {
    auto recursive =
        select(conditional.thenPath, conditional.knownOne, conditional.one);
    conditional.thenBypass.getDefiningOp()->setOperand(1, recursive);
  });
  rejectsProof("cached Prefix cannot bypass SelectionPath mode", [&] {
    // The then bypass has already been parsed while proving else's reached
    // prefix. Reusing it as an else selection path must still reject.
    conditional.elseBypass.getDefiningOp()->setOperand(0,
                                                       conditional.thenBypass);
  });
  rejectsProof("cached Prefix cannot bypass Acceptance mode", [&] {
    conditional.elseBypass.getDefiningOp()->setOperand(1,
                                                       conditional.thenBypass);
  });
  rejectsProof("active prefix cycle", [&] {
    conditional.fire.getDefiningOp()->setOperand(0, conditional.fire);
  });
}

TEST_F(HardwareSampledAdmissionTest,
       ConditionalEffectsRequireExactFinalFireAndWholeExpectedPath) {
  auto rejectsEffect = [&](StringRef label, auto mutation) {
    SCOPED_TRACE(label.str());
    buildConditional();
    ASSERT_TRUE(succeeded(capture([&] { return verifyAdmission(); })))
        << diagnostics;
    b.setInsertionPoint(rule.getBody().front().getTerminator());
    mutation();
    EXPECT_TRUE(failed(capture([&] { return verifyAdmission(); })))
        << "effect path mutation admitted: " << label.str();
  };
  rejectsEffect("root-equivalent copy of fire loses identity", [&] {
    auto yielded = cast<ac::YieldOp>(rule.getBody().front().back());
    auto effect = yielded.getOperand(15).getDefiningOp<ac::BitsBinaryOp>();
    auto duplicate = binary("and", conditional.fire, conditional.one);
    effect->setOperand(0, duplicate);
  });
  rejectsEffect("canonical AND operands reversed", [&] {
    auto yielded = cast<ac::YieldOp>(rule.getBody().front().back());
    yielded->setOperand(15,
                        binary("and", conditional.thenPath, conditional.fire));
  });
  rejectsEffect("exact F with wrong complete selected path", [&] {
    auto yielded = cast<ac::YieldOp>(rule.getBody().front().back());
    yielded->setOperand(15,
                        binary("and", conditional.fire, conditional.elsePath));
  });
  rejectsEffect("F alone omits arm selection", [&] {
    auto yielded = cast<ac::YieldOp>(rule.getBody().front().back());
    yielded->setOperand(1, conditional.fire);
  });
}

TEST_F(HardwareSampledAdmissionTest,
       EveryValidationQueryReparsesCurrentRuleMetadata) {
  buildConditional();
  ac::HardwareAnalysis analysis(*package);
  ac::HardwareBindings bindings;
  bindings.owner = definition;
  detail::SourceCheckBudget budget({1 << 20});
  detail::SampledAdmissionValidation validation(analysis, budget);
  ASSERT_TRUE(succeeded(capture([&] {
    return validation.verify(rule, bindings);
  }))) << diagnostics;
  changeRow("controls", 1, "kind", b.getStringAttr("unrecognized"));
  EXPECT_TRUE(
      failed(capture([&] { return validation.verify(rule, bindings); })));
}

TEST_F(HardwareSampledAdmissionTest,
       ConditionalBudgetFailureIsAtomicAndFreshContextCanRetry) {
  buildConditional();
  auto before = printPackage();
  for (uint64_t limit : {1u, 32u, 256u}) {
    EXPECT_TRUE(failed(capture([&] { return verifyAdmission(limit); })));
    EXPECT_NE(
        diagnostics.find("source-check plan exceeds analysis work budget"),
        std::string::npos)
        << diagnostics;
    EXPECT_EQ(before, printPackage());
  }
  EXPECT_TRUE(succeeded(capture([&] { return verifyAdmission(); })))
      << diagnostics;
  EXPECT_EQ(before, printPackage());
}

TEST_F(HardwareSampledAdmissionTest,
       PrefixConjunctionPreservesOrderIdempotenceAndLiteralLaws) {
  // Separate select operations with equal path/acceptance are semantically the
  // same admission atom. Different AND association and duplicates preserve F.
  for (unsigned permutation = 0; permutation < 3; ++permutation) {
    SCOPED_TRACE(permutation);
    buildConditional();
    b.setInsertionPoint(rule.getBody().front().getTerminator());
    auto duplicateThen = select(
        conditional.thenPath,
        conditional.thenBypass.getDefiningOp()->getOperand(1), conditional.one);
    SmallVector<Value> factors{conditional.knownEither, conditional.thenBypass,
                               conditional.elseBypass,  duplicateThen,
                               conditional.knownEither, conditional.one};
    if (permutation == 1)
      std::reverse(factors.begin(), factors.end());
    else if (permutation == 2)
      std::rotate(factors.begin(), factors.begin() + 2, factors.end());
    Value fire = factors.front();
    for (auto factor : llvm::drop_begin(factors))
      fire = binary("and", factor, fire);
    setConditionalFire(fire);
    auto before = printPackage();
    EXPECT_TRUE(succeeded(capture([&] { return verifyAdmission(); })))
        << diagnostics;
    EXPECT_EQ(before, printPackage());
  }
  for (uint64_t raw : {0u, 1u}) {
    build(false, raw);
    b.setInsertionPoint(rule.getBody().front().getTerminator());
    auto yielded = cast<ac::YieldOp>(rule.getBody().front().back());
    // Known-zero callable admission is impossible; AND false absorbs every
    // already-valid prefix. A literal one is the identity in either order.
    auto literalPrefix = constant(raw);
    yielded->setOperand(0,
                        binary("and", literalPrefix,
                               binary("and", known.getResult(), constant(1))));
    EXPECT_TRUE(succeeded(capture([&] { return verifyAdmission(); })))
        << diagnostics;
  }
}

TEST_F(HardwareSampledAdmissionTest,
       IdenticalRawEndpointDoesNotMergeKnownZeroOneAndEither) {
  for (unsigned changedTest = 0; changedTest < 3; ++changedTest) {
    SCOPED_TRACE(changedTest);
    buildConditional();
    ASSERT_TRUE(succeeded(capture([&] { return verifyAdmission(); })))
        << diagnostics;
    b.setInsertionPoint(rule.getBody().front().getTerminator());
    auto yielded = cast<ac::YieldOp>(rule.getBody().front().back());
    if (changedTest == 0)
      yielded->setOperand(
          11,
          binary(
              "and",
              yielded.getOperand(11).getDefiningOp<ac::BitsBinaryOp>().getLhs(),
              conditional.knownZero));
    else if (changedTest == 1)
      yielded->setOperand(
          13, binary("and", conditional.fire.getDefiningOp()->getOperand(0),
                     conditional.knownOne));
    else
      conditional.knownEither.getDefiningOp()->setOperand(0,
                                                          conditional.knownOne);
    auto before = printPackage();
    EXPECT_TRUE(failed(capture([&] { return verifyAdmission(); })));
    EXPECT_FALSE(diagnostics.empty());
    EXPECT_EQ(before, printPackage());
  }
}

TEST_F(HardwareSampledAdmissionTest,
       DuplicateConditionalAtomsStillAuthenticateEveryChild) {
  for (unsigned mutation = 0; mutation < 5; ++mutation) {
    SCOPED_TRACE(mutation);
    buildConditional();
    b.setInsertionPoint(rule.getBody().front().getTerminator());
    auto accepted = conditional.thenBypass.getDefiningOp()->getOperand(1);
    auto duplicate = select(conditional.thenPath, accepted, conditional.one);
    setConditionalFire(binary("and", conditional.fire, duplicate));
    ASSERT_TRUE(succeeded(capture([&] { return verifyAdmission(); })))
        << diagnostics;
    // The earlier, distinct select is still present with the same valid key.
    // No interning/cache reuse may make this select's validation optional.
    if (mutation == 0)
      duplicate.getDefiningOp()->setOperand(2, conditional.zero);
    else if (mutation == 1)
      duplicate.getDefiningOp()->setOperand(0, conditional.thenBypass);
    else if (mutation == 2)
      duplicate.getDefiningOp()->setOperand(1, conditional.thenBypass);
    else if (mutation == 3)
      duplicate.getDefiningOp()->setOperand(0, conditional.knownEither);
    else
      duplicate.getDefiningOp()->setOperand(1, conditional.knownZero);
    auto before = printPackage();
    EXPECT_TRUE(failed(capture([&] { return verifyAdmission(); })));
    EXPECT_FALSE(diagnostics.empty());
    EXPECT_EQ(before, printPackage());
  }
}

TEST_F(HardwareSampledAdmissionTest,
       FirstDistinctPrefixMissCannotEnterForeignOwnerScope) {
  for (bool raw : {false, true}) {
    SCOPED_TRACE(raw ? "raw control" : "first reached prefix");
    build();
    auto foreign = addPlainDefinition();
    b.setInsertionPoint(foreign.getBody().front().getTerminator());
    auto foreignOne = constant(1);
    ac::HardwareAnalysis analysis(*package);
    ac::HardwareBindings bindings;
    bindings.owner = definition;
    detail::SourceCheckBudget budget({1 << 20});
    auto prepared = capture([&] {
      return detail::prepareSampledBoundaries(definition, analysis, bindings,
                                              budget);
    });
    ASSERT_TRUE(succeeded(prepared)) << diagnostics;
    ASSERT_TRUE(prepared->has_value());
    auto view = capture([&] {
      return detail::parseSampledAdmission(rule, analysis, bindings, budget);
    });
    ASSERT_TRUE(succeeded(view)) << diagnostics;
    if (raw)
      view->controls[0].raw = foreignOne;
    else
      view->controls[0].reached = foreignOne;
    auto before = printPackage();
    EXPECT_TRUE(failed(capture([&] {
      return detail::proveSampledAdmission(*view, **prepared, analysis,
                                           bindings, budget);
    })));
    EXPECT_NE(diagnostics.find("actual owner scope"), std::string::npos)
        << diagnostics;
    EXPECT_EQ(before, printPackage());
  }
}

TEST_F(HardwareSampledAdmissionTest,
       IdentityStopsRequireCompleteScopeBindingsAndResolvedType) {
  build();
  auto foreign = addPlainDefinition();
  ac::HardwareAnalysis analysis(*package);
  ac::HardwareBindings bindings;
  bindings.owner = definition;
  detail::HardwareIdentityScope scope{definition, bindings, {}};
  detail::SourceCheckBudget endpointBudget({1 << 20});
  auto endpoint = capture([&] {
    return detail::makeIdentityEndpoint(rule.getResult(0), scope, analysis,
                                        endpointBudget);
  });
  ASSERT_TRUE(succeeded(endpoint)) << diagnostics;
  auto expected =
      cast<ac::YieldOp>(rule.getBody().front().back()).getOperand(0);
  for (unsigned mutation = 0; mutation < 5; ++mutation) {
    SCOPED_TRACE(mutation);
    auto stop = *endpoint;
    if (mutation == 1) {
      stop.scope.root = foreign;
      stop.scope.rootBindings.owner = foreign;
    } else if (mutation == 2)
      stop.scope.rootBindings.integers["N"] = b.getI64IntegerAttr(1);
    else if (mutation == 3)
      stop.scope.rootBindings.types["T"] = bit();
    else if (mutation == 4)
      stop.resolvedType = ac::BitsType::get(&context, literal(2));
    detail::HardwareIdentityStopPolicy policy{
        detail::HardwareIdentityTraversal::DefinitionLocal,
        ArrayRef<detail::HardwareIdentityEndpoint>(stop)};
    detail::SourceCheckBudget budget({1 << 20});
    auto before = printPackage();
    auto result = capture([&] {
      return detail::normalizeIdentityEndpoint(*endpoint, policy, analysis,
                                               budget);
    });
    ASSERT_TRUE(succeeded(result)) << diagnostics;
    EXPECT_EQ(result->value, mutation == 0 ? rule.getResult(0) : expected);
    EXPECT_EQ(before, printPackage());
  }
}

TEST_F(HardwareSampledAdmissionTest,
       QueueProposalCannotDropCommonOrSelectedArmAdmission) {
  // Exercise the prefix proof directly with a parsed ordinary admission view.
  // Physical queue identity and the serialized proposal schema have separate
  // tests; this test isolates the mutation which formerly made readiness loops
  // tempting to repair by dropping arbitrary admission terms.
  for (unsigned mutation = 0; mutation != 4; ++mutation) {
    SCOPED_TRACE(mutation);
    buildConditional();
    b.setInsertionPoint(rule.getBody().front().getTerminator());
    Value prefix = binary("and", conditional.knownEither, conditional.thenBypass);
    Value rawPath = conditional.selector;
    if (mutation == 2) prefix = conditional.one; // Drops selector knownness.
    Value enable = binary("and", prefix,
                          mutation == 3 ? conditional.one : rawPath);
    ac::HardwareAnalysis analysis(*package);
    ac::HardwareBindings bindings;
    bindings.owner = definition;
    detail::SourceCheckBudget budget({1 << 20});
    auto parsed = detail::parseSampledAdmission(rule, analysis, bindings, budget);
    ASSERT_TRUE(succeeded(parsed));
    auto prepared = detail::prepareSampledBoundaries(definition, analysis, bindings, budget);
    ASSERT_TRUE(succeeded(prepared));
    ASSERT_TRUE(prepared->has_value());
    detail::SampledQueueProposalView proposal;
    proposal.kind = b.getStringAttr("pop");
    proposal.rawPath = rawPath;
    proposal.path = enable;
    proposal.excludedControls.push_back(mutation == 1 ? 2 : 3);
    parsed->queueProposals.push_back(proposal);
    auto result = capture([&] {
      return detail::proveSampledAdmission(*parsed, **prepared, analysis, bindings, budget);
    });
    EXPECT_EQ(succeeded(result), mutation == 0) << diagnostics;
  }
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
