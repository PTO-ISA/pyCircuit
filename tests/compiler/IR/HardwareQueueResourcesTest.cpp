// Independent final-IR resource/admission/route authentication tests.
#include "Dialect/ACIR/HardwareQueueResources.h"
#include "Dialect/ACIR/HardwareSourceCheckPreflight.h"
#include "Compiler/SourceUnit.h"
#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

namespace {
namespace ac = acir::ac;
namespace compiler = acir::compiler;
namespace detail = acir::ac::detail;
using namespace mlir;

class HardwareQueueResourcesTest : public ::testing::Test {
protected:
  MLIRContext context;
  OpBuilder b{&context};
  OwningOpRef<mlir::ModuleOp> package;
  ac::ModuleOp module;
  ac::RuleOp rule;
  ac::InstanceOp queue, otherQueue;
  SmallVector<ac::SourceExpectOp> expects;
  SmallVector<Value> raw, reached, acceptance, enables, rawPaths;
  Value fire, one, zero, data, readiness;
  std::string diagnostics;
  void SetUp() override { context.loadDialect<ac::ACIRDialect>(); }
  Location loc() {
    return FileLineColLoc::get(&context, "queue-proof.py", 4, 3);
  }
  NamedAttribute f(StringRef key, Attribute value) {
    return b.getNamedAttr(key, value);
  }
  DictionaryAttr d(std::initializer_list<NamedAttribute> values) {
    return b.getDictionaryAttr(values);
  }
  DictionaryAttr occurrence(unsigned index) {
    return d(
        {f("site",
           d({f("definition", FlatSymbolRefAttr::get(&context, "QueueProof")),
              f("ast_path", b.getArrayAttr({d(
                                {f("kind", b.getStringAttr("index")),
                                 f("value", b.getI64IntegerAttr(index))})}))})),
         f("expansion", b.getArrayAttr({}))});
  }
  DictionaryAttr owner() { return d({f("instance_path", b.getArrayAttr({}))}); }
  DictionaryAttr id(unsigned index = 1) {
    return d({f("owner", owner()), f("declaration", occurrence(index))});
  }
  DictionaryAttr span() {
    return d({f("path", b.getStringAttr("queue-proof.py")),
              f("line", b.getI64IntegerAttr(4)),
              f("column", b.getI64IntegerAttr(3)),
              f("end_line", b.getI64IntegerAttr(4)),
              f("end_column", b.getI64IntegerAttr(15))});
  }
  DictionaryAttr checkID(unsigned index) {
    return d({f("registration", occurrence(2)),
              f("check", occurrence(10 + index)),
              f("obligation", b.getI64IntegerAttr(index))});
  }
  ac::StaticExprAttr literal(unsigned value) {
    return cast<ac::StaticExprAttr>(
        parseAttribute("#ac.static_expr<{kind = \"literal\", value = {kind = "
                       "\"integer\", value = #ac.math_int<" +
                           std::to_string(value) +
                           ">}, origin = {site = {definition = @QueueProof, "
                           "ast_path = []}, expansion = []}, location = {path "
                           "= \"queue-proof.py\", line = 4 : i64, column = 3 : "
                           "i64, end_line = 4 : i64, end_column = 15 : i64}}>",
                       &context));
  }
  Type bits(unsigned width = 1) {
    return ac::BitsType::get(&context, literal(width));
  }
  ac::StaticExprAttr parameterReference(StringRef name) {
    NamedAttrList tree(literal(1).getTree());
    tree.set("kind", b.getStringAttr("reference"));
    tree.erase("value");
    tree.set("ref",
             d({f("kind", b.getStringAttr("parameter")),
                f("owner", FlatSymbolRefAttr::get(&context, "QueueProof")),
                f("name", b.getStringAttr(name))}));
    return ac::StaticExprAttr::get(&context, tree.getDictionary(&context));
  }
  void setDomainType(Type type) {
    auto &body = module.getBody().front();
    SmallVector<Type> inputs(module.getFunctionType().getInputs());
    for (unsigned index : {3u, 4u}) {
      body.getArgument(index).setType(type);
      inputs[index] = type;
      rule.getBody().front().getArgument(index - 3).setType(type);
    }
    module->setAttr("function_type",
                    TypeAttr::get(b.getFunctionType(
                        inputs, module.getFunctionType().getResults())));
  }
  Operation *op(StringRef name, ValueRange operands = {},
                TypeRange results = {}, ArrayRef<NamedAttribute> attrs = {}) {
    OperationState state(loc(), name);
    state.addOperands(operands);
    state.addTypes(results);
    state.addAttributes(attrs);
    return b.create(state);
  }
  Value constant(unsigned value, unsigned width = 1) {
    return op(ac::BitsConstantOp::getOperationName(), {}, {bits(width)},
              {f("value", literal(value))})
        ->getResult(0);
  }
  Value binary(StringRef opcode, Value a, Value c) {
    return op(ac::BitsBinaryOp::getOperationName(), {a, c}, {bits()},
              {f("opcode", b.getStringAttr(opcode))})
        ->getResult(0);
  }
  Value invert(Value a) {
    return op(ac::BitsUnaryOp::getOperationName(), {a}, {bits()},
              {f("opcode", b.getStringAttr("not"))})
        ->getResult(0);
  }
  Value select(Value a, Value yes, Value no) {
    return op(ac::BitsSelectOp::getOperationName(), {a, yes, no},
              {yes.getType()})
        ->getResult(0);
  }
  Value known(Value a, Value expected) {
    return op(ac::BitsCompareOp::getOperationName(), {a, expected}, {bits()},
              {f("predicate", b.getStringAttr("case_eq"))})
        ->getResult(0);
  }
  DictionaryAttr arm(bool whenTrue) {
    return d({f("selection", b.getI64IntegerAttr(1)),
              f("when_true", b.getBoolAttr(whenTrue))});
  }
  DictionaryAttr control(unsigned index, StringRef kind,
                         DictionaryAttr path = {}) {
    if (!path)
      path = d({});
    return d({f("id", checkID(index)), f("kind", b.getStringAttr(kind)),
              f("path", path), f("invocation", b.getArrayAttr({})),
              f("predecessor", b.getI64IntegerAttr(int64_t(index) - 1))});
  }
  DictionaryAttr resource(unsigned index, bool push, bool replacement = false) {
    NamedAttrList row(d({f("control", b.getI64IntegerAttr(index)),
              f("kind", b.getStringAttr(push ? "queue_ready" : "queue_valid")),
              f("owner", owner()), f("declaration", occurrence(1)),
              f("occurrence", occurrence(10 + index)), f("location", span())}));
    if (push) {
      NamedAttrList prospective(d({f("capacity", b.getI64IntegerAttr(4))}));
      if (replacement) {
        prospective.set("valid", b.getI64IntegerAttr(3));
        prospective.set("external_pop", b.getI64IntegerAttr(7));
        prospective.set("prior_pops", b.getArrayAttr({b.getI64IntegerAttr(0)}));
      }
      row.set("prospective", prospective.getDictionary(&context));
    }
    return row.getDictionary(&context);
  }
  DictionaryAttr proposal(unsigned controlIndex, unsigned start, bool push,
                          std::optional<unsigned> excluded = {}) {
    NamedAttrList row(
        d({f("queue", id()), f("kind", b.getStringAttr(push ? "push" : "pop")),
           f("occurrence", occurrence(10 + controlIndex)),
           f("location", span()), f("raw_path", b.getI64IntegerAttr(start)),
           f("path", b.getI64IntegerAttr(start + 1))}));
    row.set("resource_control", b.getI64IntegerAttr(controlIndex));
    if (push)
      row.set("data", b.getI64IntegerAttr(start + 2));
    if (excluded)
      row.set("excluded_controls",
              b.getArrayAttr({b.getI64IntegerAttr(*excluded)}));
    return row.getDictionary(&context);
  }
  DictionaryAttr route(unsigned start, bool push) {
    NamedAttrList row(
        d({f("queue", id()), f("kind", b.getStringAttr(push ? "push" : "pop")),
           f("path", b.getI64IntegerAttr(start + 1))}));
    if (push)
      row.set("data", b.getI64IntegerAttr(start + 2));
    return row.getDictionary(&context);
  }
  ac::InstanceOp allocation(unsigned index, Value clk, Value rst,
                            Value payload) {
    return cast<ac::InstanceOp>(
        op(ac::InstanceOp::getOperationName(), {clk, rst, zero, payload, zero},
           {bits(), bits(), payload.getType(), bits()},
           {f("instance_name", b.getStringAttr(index == 1 ? "fifo" : "other")),
            f("callee", FlatSymbolRefAttr::get(
                            &context,
                            "pycircuit.__builtins__.fifo_downstream_pop")),
            f("parameters", b.getArrayAttr({literal(3), literal(1)})),
            f("type_arguments",
              b.getArrayAttr({TypeAttr::get(payload.getType())})),
            f("occurrence", occurrence(index))}));
  }
  // Dynamic selectors and payloads remain ordinary captured ports, including
  // X/Z. The test constructs its expected prefix independently from serialized
  // rows.
  void build(bool branches = false, bool replacement = false,
             unsigned payloadWidth = 8) {
    raw.clear();
    reached.clear();
    acceptance.clear();
    enables.clear();
    rawPaths.clear();
    expects.clear();
    package = mlir::ModuleOp::create(loc());
    package->getOperation()->setAttr("ac.stage", b.getStringAttr("final"));
    b.setInsertionPointToEnd(package->getBody());
    auto registry = compiler::SourceHeaderRegistry::create(
        &context, {}, [&] { return emitError(loc()); });
    ASSERT_TRUE(succeeded(registry));
    for (StringRef kind : {"fifo_local", "fifo_downstream_pop"}) {
      auto builtin = registry->lookupBuiltin(kind);
      ASSERT_TRUE(builtin);
      b.insert(builtin.declaration->clone());
    }
    SmallVector<Type> inputs{bits(), bits(payloadWidth), bits(), bits(),
                             bits()};
    if (replacement)
      inputs.push_back(bits());
    OperationState state(loc(), ac::ModuleOp::getOperationName());
    state.addAttributes(
        {f("sym_name", b.getStringAttr("QueueProof")),
         f("source_owner", d({f("package", b.getStringAttr("")),
                              f("path", b.getStringAttr("queue-proof.py"))})),
         f("parameters", b.getArrayAttr({})),
         f("type_parameters", b.getArrayAttr({})),
         f("input_names",
           b.getArrayAttr({b.getStringAttr("guard"), b.getStringAttr("data"),
                           b.getStringAttr("selector"),
                           b.getStringAttr("pyc_clk"),
                           b.getStringAttr("pyc_rst")})),
         f("output_names", b.getArrayAttr({})),
         f("function_type", TypeAttr::get(b.getFunctionType(inputs, {}))),
         f("ac.domain_inputs", d({f("clock", b.getI64IntegerAttr(3)),
                                  f("reset", b.getI64IntegerAttr(4))}))});
    state.addRegion();
    module = cast<ac::ModuleOp>(b.create(state));
    if (replacement) {
      module->setAttr("input_names", b.getArrayAttr(
          {b.getStringAttr("guard"), b.getStringAttr("data"),
           b.getStringAttr("selector"), b.getStringAttr("pyc_clk"),
           b.getStringAttr("pyc_rst"), b.getStringAttr("incoming_pop")}));
      module->setAttr("ac.internal_ports", d(
          {f("source_inputs", b.getI64IntegerAttr(5)),
           f("source_outputs", b.getI64IntegerAttr(0)),
           f("root_inputs", b.getArrayAttr({b.getI64IntegerAttr(0)}))}));
    }
    auto *body = new Block;
    module.getBody().push_back(body);
    for (Type type : inputs)
      body->addArgument(type, loc());
    b.setInsertionPointToEnd(body);
    zero = constant(0);
    data = body->getArgument(1);
    queue = allocation(1, body->getArgument(3), body->getArgument(4), data);
    if (!replacement)
      queue->setAttr(
          "callee", FlatSymbolRefAttr::get(
                        &context, "pycircuit.__builtins__.fifo_local"));
    otherQueue =
        allocation(3, body->getArgument(3), body->getArgument(4), data);
    unsigned count = branches ? 4 : 3;
    unsigned start = 1 + 2 * count;
    SmallVector<Attribute> controls{control(0, "callable_guard")}, resources,
        proposals, routes, required;
    if (branches) {
      controls.push_back(control(1, "branch_select"));
      controls.push_back(control(2, "resource_available", arm(true)));
      controls.push_back(control(3, "resource_available", arm(false)));
      resources = {resource(2, false), resource(3, false)};
      proposals = {proposal(2, start, false, 3),
                   proposal(3, start + 2, false, 2)};
      routes = {route(start, false), route(start + 2, false)};
    } else {
      controls.push_back(control(1, "resource_available"));
      resources.push_back(resource(1, false));
      controls.push_back(control(2, "resource_available"));
      resources.push_back(resource(2, true, replacement));
      proposals = {
          proposal(1, start, false),
          proposal(2, start + 2, true)};
      routes = {route(start, false), route(start + 2, true)};
    }
    for (unsigned i = 0; i != count; ++i)
      required.push_back(
          d({f("id", checkID(i)), f("kind", b.getStringAttr("guard_known")),
             f("location", span())}));
    auto metadata = d({f("clock_capture", b.getI64IntegerAttr(0)),
                       f("reset_capture", b.getI64IntegerAttr(1)),
                       f("fire_result", b.getI64IntegerAttr(0)),
                       f("controls", b.getArrayAttr(controls)),
                       f("body_checks", b.getArrayAttr({})),
                       f("observations", b.getArrayAttr({})),
                       f("resources", b.getArrayAttr(resources)),
                       f("queue_proposals", b.getArrayAttr(proposals))});
    SmallVector<Type> resultTypes(start + (branches ? 4 : 5), bits());
    if (!branches)
      resultTypes.back() = bits(payloadWidth);
    OperationState ruleState(loc(), ac::RuleOp::getOperationName());
    ruleState.addOperands({body->getArgument(3), body->getArgument(4),
                           body->getArgument(0), queue.getOutputs()[1],
                           queue.getOutputs()[3], data, body->getArgument(2)});
    if (replacement)
      ruleState.addOperands(body->getArgument(5));
    ruleState.addTypes(resultTypes);
    ruleState.addAttributes({f("name", b.getStringAttr("tick")),
                             f("occurrence", occurrence(2)),
                             f("ac.required_checks", b.getArrayAttr(required)),
                             f("ac.sampled_admission", metadata)});
    ruleState.addRegion();
    rule = cast<ac::RuleOp>(b.create(ruleState));
    auto *rb = new Block;
    rule.getBody().push_back(rb);
    for (Value operand : rule.getOperands())
      rb->addArgument(operand.getType(), loc());
    b.setInsertionPointToEnd(rb);
    one = constant(1);
    Value localZero = constant(0);
    raw.push_back(rb->getArgument(2));
    reached.push_back(one);
    acceptance.push_back(known(raw[0], one));
    if (branches) {
      auto selector = rb->getArgument(6);
      auto inverse = invert(selector);
      raw.push_back(selector);
      reached.push_back(acceptance[0]);
      auto selected = known(selector, one),
           unselected = known(selector, localZero);
      acceptance.push_back(binary("or", selected, unselected));
      auto prefix = binary("and", acceptance[0], acceptance[1]);
      auto truePath = binary("and", binary("and", one, selector), one);
      auto falsePath = binary("and", binary("and", one, inverse), one);
      raw.push_back(select(truePath, rb->getArgument(3), one));
      reached.push_back(binary("and", prefix, selected));
      acceptance.push_back(select(selected, known(raw[2], one), one));
      raw.push_back(select(falsePath, rb->getArgument(3), one));
      reached.push_back(
          binary("and", binary("and", prefix, acceptance[2]), unselected));
      acceptance.push_back(select(unselected, known(raw[3], one), one));
      fire = binary("and", binary("and", prefix, acceptance[2]), acceptance[3]);
      rawPaths = {truePath, falsePath};
      enables = {binary("and", binary("and", prefix, acceptance[2]), truePath),
                 binary("and", binary("and", prefix, acceptance[3]), falsePath)};
    } else {
      Value prefix = acceptance[0];
      for (unsigned i = 1; i != count; ++i) {
        Value predicate = rb->getArgument(i == 1 ? 3 : 4);
        if (i == 2 && replacement)
          predicate = binary("or", rb->getArgument(4),
              binary("and", rb->getArgument(3),
                  binary("or", rawPaths.front(), rb->getArgument(7))));
        if (i == 2)
          readiness = predicate;
        raw.push_back(select(binary("and", one, one),
                             predicate, one));
        if (i == 1)
          rawPaths.push_back(raw.back().getDefiningOp<ac::BitsSelectOp>().getCondition());
        reached.push_back(prefix);
        acceptance.push_back(known(raw.back(), one));
        prefix = binary("and", prefix, acceptance.back());
      }
      fire = prefix;
      auto popPath = raw[1].getDefiningOp<ac::BitsSelectOp>().getCondition();
      auto pushPath = raw[2].getDefiningOp<ac::BitsSelectOp>().getCondition();
      rawPaths = {popPath, pushPath};
      enables = {binary("and", fire, rawPaths[0]),
                 binary("and", fire, rawPaths[1])};
    }
    // Independently reconstruct the canonical diagnostic-only demand graph.
    // Resource leaves retain even a literal authored path as an actual select;
    // both source branch arms share the same continuation SSA.
    auto resourceDemand = [&](Value predicate, Value continuation) {
      return select(one, binary("and", predicate, continuation), continuation);
    };
    Value demand;
    if (branches) {
      auto whenTrue = resourceDemand(rb->getArgument(3), one);
      auto whenFalse = resourceDemand(rb->getArgument(3), one);
      demand = select(rb->getArgument(6), whenTrue, whenFalse);
    } else {
      Value tail = resourceDemand(readiness, one);
      demand = resourceDemand(rb->getArgument(3), tail);
    }
    demand = select(known(raw[0], one), demand, one);
    auto mayExecute = invert(known(demand, localZero));
    for (Value &path : reached)
      path = binary("and", path, mayExecute);
    SmallVector<Value> yielded{fire};
    for (unsigned i = 0; i != count; ++i)
      yielded.append({raw[i], reached[i]});
    yielded.append({rawPaths[0], enables[0], rawPaths[1], enables[1]});
    if (!branches)
      yielded.push_back(rb->getArgument(5));
    op(ac::YieldOp::getOperationName(), yielded);
    b.setInsertionPointToEnd(body);
    Value popDrive = branches ? binary("or", rule.getResult(start + 1),
                                           rule.getResult(start + 3))
                              : rule.getResult(start + 1);
    if (replacement)
      popDrive = binary("or", body->getArgument(5), popDrive);
    queue->setOperand(4, popDrive);
    if (!branches) {
      queue->setOperand(2, rule.getResult(start + 3));
      queue->setOperand(3, rule.getResult(start + 4));
    }
    for (unsigned i = 0; i != count; ++i)
      expects.push_back(cast<ac::SourceExpectOp>(
          op(ac::SourceExpectOp::getOperationName(),
             {rule.getResult(1 + 2 * i), rule.getResult(2 + 2 * i),
              body->getArgument(3), body->getArgument(4)},
             {},
             {f("kind", b.getStringAttr("guard_known")), f("location", span()),
              f("ac.check_id", checkID(i))})));
    op(ac::YieldOp::getOperationName());
    SmallVector<Attribute> allocations;
    for (unsigned index : {1u, 3u})
      allocations.push_back(d({f("declaration", occurrence(index)),
                               f("unbound_push", b.getBoolAttr(true)),
                               f("unbound_pop", b.getBoolAttr(true))}));
    NamedAttrList queueWiring(
        d({f("queues", b.getArrayAttr(allocations)),
           f("incoming", replacement ? b.getArrayAttr({d(
                {f("queue", id()), f("kind", b.getStringAttr("pop")),
                 f("path", b.getI64IntegerAttr(5))})}) : b.getArrayAttr({})),
           f("writers",
             b.getArrayAttr({d({f("registration", occurrence(2)),
                                f("proposals", b.getArrayAttr(routes))})}))}));
    if (replacement)
      queueWiring.set("external_pops", b.getArrayAttr({d(
          {f("registration", occurrence(2)), f("queue", id()),
           f("capture", b.getI64IntegerAttr(7))})}));
    module->setAttr("ac.queue_wiring", queueWiring.getDictionary(&context));
    module->setAttr(
        "ac.transaction_wiring",
        d({f("states", b.getArrayAttr({})), f("incoming", b.getArrayAttr({})),
           f("writers",
             b.getArrayAttr({d({f("registration", occurrence(2)),
                                f("ordinary_results", b.getI64IntegerAttr(0)),
                                f("candidates", b.getArrayAttr({}))})}))}));
    b.setInsertionPointToEnd(package->getBody());
    op(ac::SystemOp::getOperationName(), {}, {},
       {f("domain", b.getStringAttr("default")),
        f("entry",
          d({f("callee", FlatSymbolRefAttr::get(&context, "QueueProof")),
             f("parameters", b.getArrayAttr({})),
             f("type_arguments", b.getArrayAttr({}))}))});
  }
  template <typename Function> auto capture(Function fn) {
    diagnostics.clear();
    ScopedDiagnosticHandler handler(&context, [&](Diagnostic &diagnostic) {
      llvm::raw_string_ostream out(diagnostics);
      diagnostic.print(out);
      diagnostics += '\n';
      return success();
    });
    return fn();
  }
  LogicalResult wiring(uint64_t limit = 1 << 22) {
    ac::HardwareAnalysis analysis(*package);
    ac::HardwareBindings bindings;
    bindings.owner = module;
    detail::SourceCheckBudget budget({limit});
    return detail::verifyQueueResourceWiring(analysis, {{module, bindings}},
                                             budget);
  }
  LogicalResult admission() {
    ac::HardwareAnalysis analysis(*package);
    ac::HardwareBindings bindings;
    bindings.owner = module;
    detail::SourceCheckBudget budget({1 << 22});
    detail::SampledAdmissionValidation validation(analysis, budget);
    return success(succeeded(validation.verify(rule, bindings)));
  }
  LogicalResult source() {
    ac::HardwareAnalysis analysis(*package);
    auto plan = analysis.getSourceCheckPlan();
    if (succeeded(plan))
      EXPECT_EQ(plan->checks.size(), expects.size());
    return success(succeeded(plan) &&
                   succeeded(analysis.verifySourceCheckPlan(*plan)));
  }
  LogicalResult parsed() {
    ac::HardwareAnalysis analysis(*package);
    ac::HardwareBindings bindings;
    bindings.owner = module;
    detail::SourceCheckBudget budget({1 << 22});
    return success(succeeded(
        detail::parseSampledAdmission(rule, analysis, bindings, budget)));
  }
  void changeRow(StringRef attribute, StringRef array, unsigned index,
                 StringRef field, Attribute replacement) {
    Operation *target = attribute == "ac.sampled_admission"
                            ? rule.getOperation()
                            : module.getOperation();
    NamedAttrList metadata(target->getAttrOfType<DictionaryAttr>(attribute));
    auto rows = cast<ArrayAttr>(metadata.get(array));
    SmallVector<Attribute> updated(rows.begin(), rows.end());
    NamedAttrList row(cast<DictionaryAttr>(updated[index]));
    row.set(field, replacement);
    updated[index] = row.getDictionary(&context);
    metadata.set(array, b.getArrayAttr(updated));
    target->setAttr(attribute, metadata.getDictionary(&context));
  }
  std::string print() {
    std::string value;
    llvm::raw_string_ostream out(value);
    package->print(out);
    return value;
  }
};

TEST_F(HardwareQueueResourcesTest,
       ActualQueueResourcesAndPrivatePushPopSuffixPass) {
  for (bool replacement : {false, true}) {
    SCOPED_TRACE(replacement);
    build(false, replacement);
    auto before = print();
    EXPECT_TRUE(succeeded(capture([&] { return parsed(); }))) << diagnostics;
    EXPECT_TRUE(succeeded(capture([&] { return wiring(); }))) << diagnostics;
    EXPECT_TRUE(succeeded(capture([&] { return admission(); }))) << diagnostics;
    EXPECT_TRUE(succeeded(capture([&] { return source(); }))) << diagnostics;
    EXPECT_EQ(before, print());
  }
}

TEST_F(HardwareQueueResourcesTest,
       OppositeActualBranchPathsRetainSelectorKnownness) {
  build(true);
  EXPECT_TRUE(succeeded(capture([&] { return parsed(); }))) << diagnostics;
  EXPECT_TRUE(succeeded(capture([&] { return wiring(); }))) << diagnostics;
  EXPECT_TRUE(succeeded(capture([&] { return admission(); }))) << diagnostics;
  b.setInsertionPoint(rule.getBody().front().getTerminator());
  // Replacing the known-zero/known-one OR by tautological raw selector logic
  // would turn X/Z into an unknown prefix. It must not prove knownness.
  acceptance[1].getDefiningOp()->setOperand(
      0, rule.getBody().front().getArgument(6));
  EXPECT_TRUE(succeeded(capture([&] { return parsed(); }))) << diagnostics;
  EXPECT_TRUE(failed(capture([&] { return admission(); })));
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(HardwareQueueResourcesTest,
       CompoundBranchSelectorRetainsExactOppositePathIdentity) {
  build(true);
  b.setInsertionPoint(rule);
  auto compound = binary("and", module.getBody().front().getArgument(0),
                         module.getBody().front().getArgument(2));
  rule->setOperand(6, compound);
  // Normalization sees the actual conjunction, while the false arm is its
  // complement as a whole. Factoring may not lose that selector identity.
  EXPECT_TRUE(succeeded(capture([&] { return parsed(); }))) << diagnostics;
  EXPECT_TRUE(succeeded(capture([&] { return wiring(); }))) << diagnostics;
  EXPECT_TRUE(succeeded(capture([&] { return admission(); }))) << diagnostics;
}

TEST_F(HardwareQueueResourcesTest,
       OddAndEvenNotSelectorsRetainKnownnessForOppositeQueuePaths) {
  for (unsigned depth : {1u, 2u, 3u, 4u}) {
    SCOPED_TRACE(depth);
    build(true);
    b.setInsertionPoint(rule);
    Value selector = module.getBody().front().getArgument(2);
    for (unsigned i = 0; i != depth; ++i)
      selector = invert(selector);
    rule->setOperand(6, selector);
    auto before = print();
    EXPECT_TRUE(succeeded(capture([&] { return parsed(); }))) << diagnostics;
    EXPECT_TRUE(succeeded(capture([&] { return wiring(); }))) << diagnostics;
    EXPECT_TRUE(succeeded(capture([&] { return admission(); }))) << diagnostics;
    EXPECT_EQ(before, print());
  }
}

TEST_F(HardwareQueueResourcesTest,
       MaskedSelectorDoesNotProveItsComponentKnownForQueueDisjointness) {
  build(true);
  b.setInsertionPoint(rule);
  auto &body = module.getBody().front();
  rule->setOperand(6, binary("and", body.getArgument(0), body.getArgument(2)));
  ASSERT_TRUE(succeeded(capture([&] { return wiring(); }))) << diagnostics;
  // A known zero conjunction can mask an X/Z selector. Forge opposite raw
  // witnesses for that component while retaining only the conjunction's check.
  // The queue and its physical scope remain identical for both proposals.
  rule->insertOperands(rule.getNumOperands(), body.getArgument(2));
  auto component = rule.getBody().front().addArgument(bits(), loc());
  auto yield = rule.getBody().front().getTerminator();
  b.setInsertionPoint(yield);
  yield->setOperand(9, component);
  yield->setOperand(11, invert(component));
  ASSERT_TRUE(succeeded(capture([&] { return parsed(); }))) << diagnostics;
  EXPECT_TRUE(failed(capture([&] { return wiring(); })));
  EXPECT_NE(diagnostics.find("queue port conflict"), std::string::npos)
      << diagnostics;
}

TEST_F(HardwareQueueResourcesTest,
       RepeatedExactRootScopesFitTheDefaultBudgetWithoutChangingIR) {
  build(false, true);
  ac::HardwareAnalysis analysis(*package);
  ac::HardwareBindings bindings;
  bindings.owner = module;
  SmallVector<std::pair<ac::ModuleOp, ac::HardwareBindings>> scopes;
  for (unsigned i = 0; i != 1024; ++i)
    scopes.push_back({module, bindings});
  detail::SourceCheckBudget budget(ac::HardwareSourceCheckLimits{});
  auto before = print();
  EXPECT_TRUE(succeeded(capture([&] {
    return detail::verifyQueueResourceWiring(analysis, scopes, budget);
  }))) << diagnostics;
  EXPECT_EQ(before, print());
}

TEST_F(HardwareQueueResourcesTest,
       ExactRootReuseChecksDifferentOwnerIntegerAndTypeBindings) {
  for (unsigned mutation = 0; mutation != 3; ++mutation) {
    SCOPED_TRACE(mutation);
    build();
    ac::HardwareBindings valid, invalid;
    valid.owner = module;
    invalid.owner = module;
    if (mutation == 0) {
      invalid.owner = queue;
    } else if (mutation == 1) {
      module->setAttr("parameters",
                      b.getArrayAttr({ac::getStaticParameterAttr(b, "N")}));
      setDomainType(ac::BitsType::get(&context, parameterReference("N")));
      valid.integers["N"] = ac::MathIntAttr::get(&context, llvm::APSInt("1"));
      invalid.integers["N"] = ac::MathIntAttr::get(&context, llvm::APSInt("2"));
    } else {
      module->setAttr("type_parameters",
                      b.getArrayAttr({b.getStringAttr("T")}));
      setDomainType(ac::TypeParamType::get(
          &context, FlatSymbolRefAttr::get(&context, "QueueProof"),
          b.getStringAttr("T")));
      valid.types["T"] = bits();
      invalid.types["T"] = bits(2);
    }
    ac::HardwareAnalysis analysis(*package);
    detail::SourceCheckBudget validBudget(ac::HardwareSourceCheckLimits{});
    ASSERT_TRUE(succeeded(capture([&] {
      return detail::verifyQueueResourceWiring(analysis, {{module, valid}},
                                               validBudget);
    }))) << diagnostics;
    detail::SourceCheckBudget mixedBudget(ac::HardwareSourceCheckLimits{});
    EXPECT_TRUE(failed(capture([&] {
      return detail::verifyQueueResourceWiring(
          analysis, {{module, valid}, {module, valid}, {module, invalid}},
          mixedBudget);
    })));
    EXPECT_FALSE(diagnostics.empty());
    EXPECT_EQ(diagnostics.find("analysis work budget"), std::string::npos)
        << diagnostics;
  }
}

TEST_F(HardwareQueueResourcesTest,
       RepeatedRootDoesNotHideADifferentMalformedModule) {
  build();
  auto *copy = module->clone();
  copy->setAttr("sym_name", b.getStringAttr("OtherQueueProof"));
  copy->setAttr("ac.queue_wiring", b.getStringAttr("malformed"));
  package->getBody()->getOperations().push_back(copy);
  auto other = cast<ac::ModuleOp>(copy);
  ac::HardwareAnalysis analysis(*package);
  ac::HardwareBindings selected, different;
  selected.owner = module;
  different.owner = other;
  detail::SourceCheckBudget budget(ac::HardwareSourceCheckLimits{});
  EXPECT_TRUE(failed(capture([&] {
    return detail::verifyQueueResourceWiring(
        analysis, {{module, selected}, {module, selected}, {other, different}},
        budget);
  })));
  EXPECT_NE(diagnostics.find("queue wiring requires a dictionary"),
            std::string::npos)
      << diagnostics;
}

TEST_F(HardwareQueueResourcesTest,
       FreshPublicInvocationsCheckMetadataMutationAndCorrectedRetry) {
  build(false, true);
  ac::HardwareAnalysis analysis(*package);
  ac::HardwareBindings bindings;
  bindings.owner = module;
  auto prove = [&] {
    detail::SourceCheckBudget budget(ac::HardwareSourceCheckLimits{});
    return detail::verifyQueueResourceWiring(analysis, {{module, bindings}},
                                             budget);
  };
  auto uses = [&] {
    detail::SourceCheckBudget budget(ac::HardwareSourceCheckLimits{});
    return detail::getVerifiedQueueResourceUses(analysis, budget);
  };
  auto before = print();
  ASSERT_TRUE(succeeded(capture(prove))) << diagnostics;
  ASSERT_TRUE(succeeded(capture(uses))) << diagnostics;
  // The same analysis must read the current ownership metadata on each public
  // invocation. An authored direction cannot receive the earlier private route.
  changeRow("ac.queue_wiring", "queues", 0, "unbound_pop",
            b.getBoolAttr(false));
  auto malformed = print();
  EXPECT_TRUE(failed(capture(prove)));
  EXPECT_NE(diagnostics.find("queue proposal targets an authored direction"),
            std::string::npos)
      << diagnostics;
  EXPECT_TRUE(failed(capture(uses)));
  EXPECT_NE(diagnostics.find("queue proposal targets an authored direction"),
            std::string::npos)
      << diagnostics;
  EXPECT_EQ(malformed, print());
  changeRow("ac.queue_wiring", "queues", 0, "unbound_pop", b.getBoolAttr(true));
  EXPECT_TRUE(succeeded(capture(prove))) << diagnostics;
  EXPECT_TRUE(succeeded(capture(uses))) << diagnostics;
  EXPECT_EQ(before, print());
}

TEST_F(HardwareQueueResourcesTest,
       PhysicalResourcesRejectDifferentQueueAndDomainAndBypass) {
  for (unsigned mutation = 0; mutation != 5; ++mutation) {
    SCOPED_TRACE(mutation);
    build();
    ASSERT_TRUE(succeeded(capture([&] { return wiring(); }))) << diagnostics;
    auto selector = cast<ac::BitsSelectOp>(raw[1].getDefiningOp());
    if (mutation == 0)
      rule->setOperand(3, otherQueue.getOutputs()[1]);
    if (mutation == 1)
      rule->setOperand(0, module.getBody().front().getArgument(0));
    if (mutation == 2)
      rule->setOperand(1, module.getBody().front().getArgument(0));
    if (mutation == 3) {
      b.setInsertionPoint(selector);
      selector->setOperand(2, constant(0));
    }
    if (mutation == 4) {
      b.setInsertionPoint(selector);
      selector->setOperand(0, constant(0));
    }
    ASSERT_TRUE(succeeded(capture([&] { return parsed(); }))) << diagnostics;
    EXPECT_TRUE(failed(capture([&] { return wiring(); })));
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(HardwareQueueResourcesTest,
       PrivateRoutesRejectPublicFanoutAndEnableAsDataAlias) {
  for (bool alias : {false, true}) {
    SCOPED_TRACE(alias);
    build(false, false, alias ? 1 : 8);
    ASSERT_TRUE(succeeded(capture([&] { return wiring(); }))) << diagnostics;
    b.setInsertionPoint(module.getBody().front().getTerminator());
    if (alias)
      queue->setOperand(3, rule.getResult(10));
    else
      op(ac::BitsUnaryOp::getOperationName(), {rule.getResult(8)}, {bits()},
         {f("opcode", b.getStringAttr("not"))});
    ASSERT_TRUE(succeeded(capture([&] { return parsed(); }))) << diagnostics;
    EXPECT_TRUE(failed(capture([&] { return wiring(); })));
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(HardwareQueueResourcesTest,
       RawPathSuffixCannotEscapeThroughExtraUseOrPublicOutput) {
  for (bool output : {false, true}) {
    SCOPED_TRACE(output);
    build();
    ASSERT_TRUE(succeeded(capture([&] { return wiring(); }))) << diagnostics;
    auto yield = cast<ac::YieldOp>(module.getBody().front().getTerminator());
    b.setInsertionPoint(yield);
    if (output) {
      yield->setOperands({rule.getResult(7)});
      module->setAttr("function_type",
                      TypeAttr::get(b.getFunctionType(
                          module.getFunctionType().getInputs(), {bits()})));
      module->setAttr("output_names",
                      b.getArrayAttr({b.getStringAttr("leaked_path")}));
    } else {
      op(ac::BitsUnaryOp::getOperationName(), {rule.getResult(7)}, {bits()},
         {f("opcode", b.getStringAttr("not"))});
    }
    ASSERT_TRUE(succeeded(capture([&] { return parsed(); }))) << diagnostics;
    EXPECT_TRUE(failed(capture([&] { return wiring(); })));
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(HardwareQueueResourcesTest,
       EnableCannotBeReexportedThroughPayloadSuffixAlias) {
  build(false, false, 1);
  ASSERT_TRUE(succeeded(capture([&] { return wiring(); }))) << diagnostics;
  // Both external suffix slots have Bits1. The payload slot is an identity
  // alias of the private enable, so comparing payload endpoints alone is not
  // enough to distinguish the enable and data transport roles.
  auto yielded = cast<ac::YieldOp>(rule.getBody().front().getTerminator());
  yielded->setOperand(11, yielded->getOperand(10));
  ASSERT_TRUE(succeeded(capture([&] { return parsed(); }))) << diagnostics;
  EXPECT_TRUE(failed(capture([&] { return wiring(); })));
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(HardwareQueueResourcesTest, QueueEnableMustUseExactProposalRoute) {
  build();
  ASSERT_TRUE(succeeded(capture([&] { return wiring(); }))) << diagnostics;
  queue->setOperand(4, module.getBody().front().getArgument(0));
  ASSERT_TRUE(succeeded(capture([&] { return parsed(); }))) << diagnostics;
  EXPECT_TRUE(failed(capture([&] { return wiring(); })));
  EXPECT_NE(diagnostics.find("physical enable"), std::string::npos)
      << diagnostics;
}

TEST_F(HardwareQueueResourcesTest, CommonAdmissionCannotBeOmittedFromProposal) {
  build();
  ASSERT_TRUE(succeeded(capture([&] { return admission(); }))) << diagnostics;
  b.setInsertionPoint(rule.getBody().front().getTerminator());
  enables[0].getDefiningOp()->setOperand(
      0, binary("and", acceptance[1], acceptance[2]));
  ASSERT_TRUE(succeeded(capture([&] { return parsed(); }))) << diagnostics;
  ASSERT_TRUE(succeeded(capture([&] { return wiring(); }))) << diagnostics;
  EXPECT_TRUE(failed(capture([&] { return admission(); })));
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(HardwareQueueResourcesTest,
       CommonControlCannotBeListedAsOppositeArmExclusion) {
  build(true);
  ASSERT_TRUE(succeeded(capture([&] { return admission(); }))) << diagnostics;
  changeRow("ac.sampled_admission", "queue_proposals", 0, "excluded_controls",
            b.getArrayAttr({b.getI64IntegerAttr(0), b.getI64IntegerAttr(3)}));
  ASSERT_TRUE(succeeded(capture([&] { return parsed(); }))) << diagnostics;
  EXPECT_TRUE(failed(capture([&] { return admission(); })));
  EXPECT_NE(diagnostics.find("opposite-arm SSA witness"), std::string::npos)
      << diagnostics;
}

TEST_F(HardwareQueueResourcesTest,
       SameDirectionProposalsRequireActualOppositePaths) {
  build(true);
  ASSERT_TRUE(succeeded(capture([&] { return wiring(); }))) << diagnostics;
  auto yielded = cast<ac::YieldOp>(rule.getBody().front().getTerminator());
  yielded->setOperand(11,
                      rawPaths[0]); // second proposal raw path, exact suffix
  ASSERT_TRUE(succeeded(capture([&] { return parsed(); }))) << diagnostics;
  EXPECT_TRUE(failed(capture([&] { return wiring(); })));
  EXPECT_NE(diagnostics.find("actual-instance proposals lack opposite paths"), std::string::npos)
      << diagnostics;
}

TEST_F(HardwareQueueResourcesTest,
       ReplacementCapacityRejectsFuturePopAndLocalOccupancy) {
  build(false, true);
  ASSERT_TRUE(succeeded(capture([&] { return wiring(); }))) << diagnostics;
  changeRow("ac.sampled_admission", "resources", 1, "prospective",
            d({f("capacity", b.getI64IntegerAttr(4)),
               f("valid", b.getI64IntegerAttr(3)),
               f("external_pop", b.getI64IntegerAttr(7)),
               f("prior_pops", b.getArrayAttr({b.getI64IntegerAttr(1)}))}));
  EXPECT_TRUE(failed(capture([&] { return wiring(); })));
  EXPECT_FALSE(diagnostics.empty());
  build(false, true);
  ASSERT_TRUE(succeeded(capture([&] { return wiring(); }))) << diagnostics;
  queue->setAttr(
      "callee", FlatSymbolRefAttr::get(
                    &context, "pycircuit.__builtins__.fifo_local"));
  ASSERT_TRUE(succeeded(capture([&] { return parsed(); }))) << diagnostics;
  EXPECT_TRUE(failed(capture([&] { return wiring(); })));
  EXPECT_NE(diagnostics.find("local_occupancy cannot receive prospective pop credit"),
            std::string::npos)
      << diagnostics;
}

TEST_F(HardwareQueueResourcesTest, RetiredReadyFromPopCannotReplacePushResource) {
  build(false, true);
  ASSERT_TRUE(succeeded(capture([&] { return wiring(); }))) << diagnostics;
  changeRow("ac.sampled_admission", "queue_proposals", 1, "ready_from_pop",
            b.getI64IntegerAttr(0));
  EXPECT_TRUE(failed(capture([&] { return parsed(); })));
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(HardwareQueueResourcesTest,
       ProspectiveCapacityRequiresCommittedEndpointsAndRawCompleteCredit) {
  for (unsigned mutation = 0; mutation != 7; ++mutation) {
    SCOPED_TRACE(mutation);
    build(false, true);
    ASSERT_TRUE(succeeded(capture([&] { return wiring(); }))) << diagnostics;
    auto capacity = readiness.getDefiningOp<ac::BitsBinaryOp>();
    auto validCredit = capacity.getRhs().getDefiningOp<ac::BitsBinaryOp>();
    auto credit = validCredit.getRhs().getDefiningOp<ac::BitsBinaryOp>();
    b.setInsertionPoint(rule.getBody().front().getTerminator());
    if (mutation == 0)
      rule->setOperand(4, queue.getOutputs()[0]);
    if (mutation == 1)
      rule->setOperand(3, otherQueue.getOutputs()[1]);
    if (mutation == 2)
      capacity->setOperand(0, one);
    if (mutation == 3)
      validCredit->setOperand(0, one);
    if (mutation == 4)
      credit->setOperand(0, enables[0]);
    if (mutation == 5)
      credit->setOperand(1, one);
    if (mutation == 6)
      credit->setOperand(0, known(rawPaths[0], one));
    ASSERT_TRUE(succeeded(capture([&] { return parsed(); }))) << diagnostics;
    EXPECT_TRUE(failed(capture([&] { return wiring(); })));
  }
}

TEST_F(HardwareQueueResourcesTest,
       ExternalCreditExcludesEveryProposalOfItsOwnRegistration) {
  build(false, true);
  ASSERT_TRUE(succeeded(capture([&] { return wiring(); }))) << diagnostics;
  b.setInsertionPoint(rule);
  rule->setOperand(7, binary("or", module.getBody().front().getArgument(5),
                             rule.getResult(8)));
  ASSERT_TRUE(succeeded(capture([&] { return parsed(); }))) << diagnostics;
  EXPECT_TRUE(failed(capture([&] { return wiring(); })));
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(HardwareQueueResourcesTest,
       ProspectiveFormulaAndDedicatedExternalCaptureCannotLeak) {
  for (unsigned mutation = 0; mutation != 3; ++mutation) {
    SCOPED_TRACE(mutation);
    build(false, true);
    ASSERT_TRUE(succeeded(capture([&] { return wiring(); }))) << diagnostics;
    b.setInsertionPoint(rule.getBody().front().getTerminator());
    Value leaked = mutation == 0 ? rule.getBody().front().getArgument(7)
                    : mutation == 1 ? readiness
                                    : readiness.getDefiningOp<ac::BitsBinaryOp>().getRhs();
    invert(leaked);
    ASSERT_TRUE(succeeded(capture([&] { return parsed(); }))) << diagnostics;
    EXPECT_TRUE(failed(capture([&] { return wiring(); })));
    EXPECT_NE(diagnostics.find("private prospective Queue control escapes"),
              std::string::npos) << diagnostics;
  }
}

TEST_F(HardwareQueueResourcesTest,
       ProspectivePriorInventoryCannotOmitDuplicateOrReorderPops) {
  for (unsigned mutation = 0; mutation != 4; ++mutation) {
    SCOPED_TRACE(mutation);
    build(false, true);
    ASSERT_TRUE(succeeded(capture([&] { return wiring(); }))) << diagnostics;
    SmallVector<Attribute> prior;
    if (mutation == 1)
      prior = {b.getI64IntegerAttr(0), b.getI64IntegerAttr(0)};
    if (mutation == 2)
      prior = {b.getI64IntegerAttr(1), b.getI64IntegerAttr(0)};
    if (mutation == 3)
      prior = {b.getI64IntegerAttr(2)};
    changeRow("ac.sampled_admission", "resources", 1, "prospective",
              d({f("capacity", b.getI64IntegerAttr(4)),
                 f("valid", b.getI64IntegerAttr(3)),
                 f("external_pop", b.getI64IntegerAttr(7)),
                 f("prior_pops", b.getArrayAttr(prior))}));
    EXPECT_TRUE(failed(capture([&] { return wiring(); })));
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(HardwareQueueResourcesTest,
       InvalidIncomingOffsetAndPayloadTypeFailPhysicalOwner) {
  build();
  ASSERT_TRUE(succeeded(capture([&] { return wiring(); }))) << diagnostics;
  // Adding an incoming row cannot alias the source's public arguments.
  NamedAttrList wiringRow(
      module->getAttrOfType<DictionaryAttr>("ac.queue_wiring"));
  wiringRow.set(
      "incoming",
      b.getArrayAttr({d({f("queue", id(3)), f("kind", b.getStringAttr("pop")),
                         f("path", b.getI64IntegerAttr(0))})}));
  module->setAttr("ac.queue_wiring", wiringRow.getDictionary(&context));
  EXPECT_TRUE(failed(capture([&] { return wiring(); })));
  EXPECT_NE(diagnostics.find("state proposal suffix"), std::string::npos)
      << diagnostics;
  build();
  ASSERT_TRUE(succeeded(capture([&] { return wiring(); }))) << diagnostics;
  b.setInsertionPoint(queue);
  queue->setOperand(3, constant(0, 9));
  ASSERT_TRUE(succeeded(capture([&] { return parsed(); }))) << diagnostics;
  EXPECT_TRUE(failed(capture([&] { return wiring(); })));
  EXPECT_NE(diagnostics.find("payload type differs"), std::string::npos)
      << diagnostics;
}

TEST_F(HardwareQueueResourcesTest,
       BudgetFailureDoesNotMutateAndFreshProofRetries) {
  build();
  auto before = print();
  EXPECT_TRUE(failed(capture([&] { return wiring(32); })));
  EXPECT_NE(diagnostics.find("analysis work budget"), std::string::npos)
      << diagnostics;
  EXPECT_EQ(before, print());
  EXPECT_TRUE(succeeded(capture([&] { return wiring(); }))) << diagnostics;
  EXPECT_EQ(before, print());
}
} // namespace
