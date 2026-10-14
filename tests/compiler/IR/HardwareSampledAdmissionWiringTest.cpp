#include "Compiler/HardwareEmitCommon.h"
#include "Dialect/ACIR/HardwareSampledAdmission.h"
#include "Dialect/ACIR/HardwareSourceCheckPreflight.h"
#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <string>

namespace {
namespace ac = acir::ac;
namespace detail = acir::ac::detail;
using namespace mlir;

class HardwareSampledAdmissionWiringTest : public ::testing::Test {
protected:
  MLIRContext context;
  OpBuilder b{&context};
  OwningOpRef<mlir::ModuleOp> package;
  ac::ModuleOp definition, proofRoot;
  ac::ModuleImportOp dffe;
  ac::InstanceOp storage;
  ac::RuleOp rule;
  ac::SourceExpectOp expect;
  Value zero, one;
  std::string diagnostics;

  void SetUp() override { context.loadDialect<ac::ACIRDialect>(); }
  Location loc() { return FileLineColLoc::get(&context, "wiring.py", 4, 2); }
  NamedAttribute f(StringRef name, Attribute value) {
    return b.getNamedAttr(name, value);
  }
  DictionaryAttr d(std::initializer_list<NamedAttribute> fields) {
    return b.getDictionaryAttr(fields);
  }
  DictionaryAttr occurrence(uint64_t index, StringRef symbol = "Owner") {
    auto step = d({f("kind", b.getStringAttr("index")),
                   f("value", b.getI64IntegerAttr(index))});
    return d(
        {f("site", d({f("definition", FlatSymbolRefAttr::get(&context, symbol)),
                      f("ast_path", b.getArrayAttr({step}))})),
         f("expansion", b.getArrayAttr({}))});
  }
  DictionaryAttr span() {
    return d({f("path", b.getStringAttr("wiring.py")),
              f("line", b.getI64IntegerAttr(4)),
              f("column", b.getI64IntegerAttr(2)),
              f("end_line", b.getI64IntegerAttr(4)),
              f("end_column", b.getI64IntegerAttr(21))});
  }
  DictionaryAttr checkID() {
    return d({f("registration", occurrence(7)), f("check", occurrence(8)),
              f("obligation", b.getI64IntegerAttr(0))});
  }
  DictionaryAttr stateID(uint64_t declaration = 20, ArrayAttr ownerPath = {}) {
    if (!ownerPath)
      ownerPath = b.getArrayAttr({});
    return d({f("owner", d({f("instance_path", ownerPath)})),
              f("declaration", occurrence(declaration)),
              f("element", b.getArrayAttr({}))});
  }
  ac::StaticExprAttr literal(uint64_t value, StringRef symbol = "Owner") {
    std::string text =
        "#ac.static_expr<{kind = \"literal\", value = {kind = \"integer\", "
        "value = #ac.math_int<" +
        std::to_string(value) + ">}, origin = {site = {definition = @" +
        symbol.str() +
        ", ast_path = []}, expansion = []}, location = {path = "
        "\"wiring.py\", line = 4 : i64, column = 2 : i64, end_line = 4 : "
        "i64, end_column = 21 : i64}}>";
    return cast<ac::StaticExprAttr>(parseAttribute(text, &context));
  }
  ac::StaticExprAttr parameterReference(StringRef name) {
    auto text = "#ac.static_expr<{kind = \"reference\", ref = {kind = "
                "\"parameter\", owner = @Owner, name = \"" +
                name.str() +
                "\"}, origin = {site = {definition = @Owner, ast_path = "
                "[]}, expansion = []}, location = {path = \"wiring.py\", "
                "line = 4 : i64, column = 2 : i64, end_line = 4 : i64, "
                "end_column = 21 : i64}}>";
    return cast<ac::StaticExprAttr>(parseAttribute(text, &context));
  }
  Type bit() { return ac::BitsType::get(&context, literal(1)); }
  void setDomainType(Type type) {
    auto &body = definition.getBody().front();
    body.getArgument(1).setType(type);
    body.getArgument(2).setType(type);
    SmallVector<Type> inputs(definition.getFunctionType().getInputs());
    inputs[1] = type;
    inputs[2] = type;
    definition->setAttr(
        "function_type",
        TypeAttr::get(b.getFunctionType(
            inputs, definition.getFunctionType().getResults())));
    rule.getBody().front().getArgument(0).setType(type);
    rule.getBody().front().getArgument(1).setType(type);
  }
  void setStatePayloadType(Type type) {
    auto &body = definition.getBody().front();
    auto initial = body.addArgument(type, loc());
    SmallVector<Type> inputs(definition.getFunctionType().getInputs());
    inputs.push_back(type);
    definition->setAttr(
        "function_type",
        TypeAttr::get(b.getFunctionType(
            inputs, definition.getFunctionType().getResults())));
    SmallVector<Attribute> names(definition.getInputNames().getValue());
    names.push_back(b.getStringAttr("initial"));
    definition->setAttr("input_names", b.getArrayAttr(names));
    storage->getResult(0).setType(type);
    storage->setAttr("type_arguments", b.getArrayAttr({TypeAttr::get(type)}));
    storage->setOperand(4, initial);
    rule.getBody().front().getArgument(3).setType(type);
    rule.getBody().front().getTerminator()->setOperand(
        0, rule.getBody().front().getArgument(2));
  }
  Operation *op(StringRef name, ValueRange operands = {},
                TypeRange results = {}, ArrayRef<NamedAttribute> attrs = {}) {
    OperationState state(loc(), name);
    state.addOperands(operands);
    state.addTypes(results);
    state.addAttributes(attrs);
    return b.create(state);
  }
  Value constant(uint64_t value, StringRef symbol = "Owner") {
    return op(ac::BitsConstantOp::getOperationName(), {}, {bit()},
              {f("value", literal(value, symbol))})
        ->getResult(0);
  }
  DictionaryAttr wiring(ArrayRef<Attribute> states,
                        ArrayRef<Attribute> incoming,
                        ArrayRef<Attribute> writers) {
    return d({f("states", b.getArrayAttr(states)),
              f("incoming", b.getArrayAttr(incoming)),
              f("writers", b.getArrayAttr(writers))});
  }
  DictionaryAttr writer(ArrayRef<Attribute> candidates = {}) {
    return d({f("registration", occurrence(7)),
              f("ordinary_results", b.getI64IntegerAttr(0)),
              f("candidates", b.getArrayAttr(candidates))});
  }
  DictionaryAttr candidate(DictionaryAttr state, uint64_t result = 0) {
    return d({f("state", state), f("result", b.getI64IntegerAttr(result))});
  }
  DictionaryAttr incoming(DictionaryAttr state, uint64_t data,
                          uint64_t enable) {
    return d({f("state", state), f("data", b.getI64IntegerAttr(data)),
              f("enable", b.getI64IntegerAttr(enable))});
  }
  void addPrimitive() {
    b.setInsertionPointToEnd(package->getBody());
    Type payload = ac::TypeParamType::get(
        &context, FlatSymbolRefAttr::get(&context, "storage"),
        b.getStringAttr("T"));
    OperationState state(loc(), ac::ModuleImportOp::getOperationName());
    state.addAttributes(
        {f("semantic_kind", b.getStringAttr("hardware")),
         f("sym_name", b.getStringAttr("storage")),
         f("source_owner", d({f("package", b.getStringAttr("gfsim")),
                              f("path", b.getStringAttr("dffe.py"))})),
         f("parameters", b.getArrayAttr({})),
         f("type_parameters", b.getArrayAttr({b.getStringAttr("T")})),
         f("function_type",
           TypeAttr::get(b.getFunctionType(
               {bit(), bit(), bit(), payload, payload}, {payload}))),
         f("input_names",
           b.getArrayAttr({b.getStringAttr("clk"), b.getStringAttr("rst"),
                           b.getStringAttr("en"), b.getStringAttr("d"),
                           b.getStringAttr("init")})),
         f("output_names", b.getArrayAttr({b.getStringAttr("q")})),
         f("primitive_kind", b.getStringAttr("dffe")),
         f("dependency_summary",
           b.getArrayAttr({d({f("output", d({f("port", b.getI64IntegerAttr(0)),
                                             f("path", b.getArrayAttr({}))})),
                              f("inputs", b.getArrayAttr({}))})}))});
    dffe = cast<ac::ModuleImportOp>(b.create(state));
  }
  void build(bool withState = true) {
    package = mlir::ModuleOp::create(loc());
    package->getOperation()->setAttr("ac.stage", b.getStringAttr("final"));
    addPrimitive();
    b.setInsertionPointToEnd(package->getBody());
    OperationState state(loc(), ac::ModuleOp::getOperationName());
    state.addAttributes(
        {f("sym_name", b.getStringAttr("Owner")),
         f("source_owner", d({f("package", b.getStringAttr("")),
                              f("path", b.getStringAttr("wiring.py"))})),
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
    state.addRegion();
    definition = cast<ac::ModuleOp>(b.create(state));
    proofRoot = definition;
    auto *body = new Block;
    definition.getBody().push_back(body);
    for (unsigned i = 0; i < 3; ++i)
      body->addArgument(bit(), loc());
    b.setInsertionPointToEnd(body);
    zero = constant(0);
    one = constant(1);
    if (withState) {
      storage = cast<ac::InstanceOp>(
          op(ac::InstanceOp::getOperationName(),
             {body->getArgument(1), body->getArgument(2), zero, zero, zero},
             {bit()},
             {f("callee", FlatSymbolRefAttr::get(&context, "storage")),
              f("instance_name", b.getStringAttr("state")),
              f("parameters", b.getArrayAttr({})),
              f("type_arguments", b.getArrayAttr({TypeAttr::get(bit())})),
              f("occurrence", occurrence(20))}));
    }

    auto id = checkID();
    OperationState ruleState(loc(), ac::RuleOp::getOperationName());
    SmallVector<Value> captures = {body->getArgument(1), body->getArgument(2),
                                   body->getArgument(0)};
    if (withState)
      captures.push_back(storage.getResult(0));
    ruleState.addOperands(captures);
    ruleState.addTypes(SmallVector<Type>(withState ? 4 : 3, bit()));
    uint64_t fire = withState ? 1 : 0;
    ruleState.addAttributes(
        {f("name", b.getStringAttr("tick")), f("occurrence", occurrence(7)),
         f("ac.required_checks",
           b.getArrayAttr(
               {d({f("id", id), f("kind", b.getStringAttr("guard_known")),
                   f("location", span())})})),
         f("ac.sampled_admission",
           d({f("clock_capture", b.getI64IntegerAttr(0)),
              f("reset_capture", b.getI64IntegerAttr(1)),
              f("fire_result", b.getI64IntegerAttr(fire)),
              f("body_checks", b.getArrayAttr({})),
              f("observations", b.getArrayAttr({})),
              f("guards",
                b.getArrayAttr(
                    {d({f("id", id), f("invocation", b.getArrayAttr({})),
                        f("predecessor", b.getI64IntegerAttr(-1))})}))}))});
    ruleState.addRegion();
    rule = cast<ac::RuleOp>(b.create(ruleState));
    auto *ruleBody = new Block;
    rule.getBody().push_back(ruleBody);
    for (Value capture : captures)
      ruleBody->addArgument(capture.getType(), loc());
    b.setInsertionPointToEnd(ruleBody);
    Value ruleOne = constant(1);
    auto compare = op(ac::BitsCompareOp::getOperationName(),
                      {ruleBody->getArgument(2), ruleOne}, {bit()},
                      {f("predicate", b.getStringAttr("case_eq"))});
    SmallVector<Value> yielded;
    if (withState)
      yielded.push_back(ruleBody->getArgument(3));
    yielded.append({compare->getResult(0), ruleBody->getArgument(2), ruleOne});
    op(ac::YieldOp::getOperationName(), yielded);
    if (withState)
      storage->setOperands({body->getArgument(1), body->getArgument(2),
                            rule.getResult(1), rule.getResult(0), zero});
    b.setInsertionPointToEnd(body);
    expect = cast<ac::SourceExpectOp>(
        op(ac::SourceExpectOp::getOperationName(),
           {rule.getResult(fire + 1), rule.getResult(fire + 2),
            body->getArgument(1), body->getArgument(2)},
           {},
           {f("kind", b.getStringAttr("guard_known")), f("location", span()),
            f("ac.check_id", id)}));
    op(ac::YieldOp::getOperationName());
    SmallVector<Attribute> states, candidates;
    if (withState) {
      states.push_back(occurrence(20));
      candidates.push_back(candidate(stateID()));
    }
    definition->setAttr("ac.transaction_wiring",
                        wiring(states, {}, {writer(candidates)}));
    b.setInsertionPointToEnd(package->getBody());
    op(ac::SystemOp::getOperationName(), {}, {},
       {f("domain", b.getStringAttr("default")),
        f("entry", d({f("callee", FlatSymbolRefAttr::get(&context, "Owner")),
                      f("parameters", b.getArrayAttr({})),
                      f("type_arguments", b.getArrayAttr({}))}))});
  }
  SmallVector<ac::InstanceOp> expandToThreeCandidates() {
    if (!storage) {
      ADD_FAILURE() << "three-candidate fixture requires stateful build";
      return {};
    }
    auto &body = definition.getBody().front();
    SmallVector<ac::InstanceOp> states{storage};
    for (uint64_t declaration : {21u, 22u}) {
      auto *copy = storage->clone();
      copy->setAttr("instance_name",
                    b.getStringAttr("state_" + std::to_string(declaration)));
      copy->setAttr("occurrence", occurrence(declaration));
      body.getOperations().insert(rule->getIterator(), copy);
      states.push_back(cast<ac::InstanceOp>(copy));
    }

    NamedAttrList attributes(rule->getAttrs());
    NamedAttrList sampled(
        rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission"));
    sampled.set("fire_result", b.getI64IntegerAttr(3));
    attributes.set("ac.sampled_admission", sampled.getDictionary(&context));
    b.setInsertionPoint(rule);
    OperationState state(loc(), ac::RuleOp::getOperationName());
    SmallVector<Value> captures(rule.getOperands());
    captures.push_back(states[1].getResult(0));
    captures.push_back(states[2].getResult(0));
    state.addOperands(captures);
    state.addTypes(SmallVector<Type>(6, bit()));
    state.addAttributes(attributes);
    state.addRegion();
    auto replacement = cast<ac::RuleOp>(b.create(state));
    auto *replacementBody = new Block;
    replacement.getBody().push_back(replacementBody);
    for (Value capture : captures)
      replacementBody->addArgument(capture.getType(), loc());
    b.setInsertionPointToEnd(replacementBody);
    auto knownOne = constant(1);
    auto known = cast<ac::BitsCompareOp>(
        op(ac::BitsCompareOp::getOperationName(),
           {replacementBody->getArgument(2), knownOne}, {bit()},
           {f("predicate", b.getStringAttr("case_eq"))}));
    op(ac::YieldOp::getOperationName(),
       {replacementBody->getArgument(3), replacementBody->getArgument(4),
        replacementBody->getArgument(5), known.getResult(),
        replacementBody->getArgument(2), knownOne});

    for (auto [index, instance] : llvm::enumerate(states))
      instance->setOperands({body.getArgument(1), body.getArgument(2),
                             replacement.getResult(3),
                             replacement.getResult(index), zero});
    expect->setOperands({replacement.getResult(4), replacement.getResult(5),
                         body.getArgument(1), body.getArgument(2)});
    rule.erase();
    rule = replacement;
    definition->setAttr(
        "ac.transaction_wiring",
        wiring({occurrence(20), occurrence(21), occurrence(22)}, {},
               {writer({candidate(stateID(20), 0), candidate(stateID(21), 1),
                        candidate(stateID(22), 2)})}));
    return states;
  }
  ac::RuleOp addIllegalRuleCapture(ValueRange captures,
                                   StringRef name = "illegal_capture") {
    auto &body = definition.getBody().front();
    b.setInsertionPoint(body.getTerminator());
    OperationState state(loc(), ac::RuleOp::getOperationName());
    state.addOperands(captures);
    state.addAttribute("name", b.getStringAttr(name));
    state.addAttribute("occurrence", occurrence(90));
    state.addRegion();
    auto captured = cast<ac::RuleOp>(b.create(state));
    auto *capturedBody = new Block;
    captured.getBody().push_back(capturedBody);
    for (Value value : captures)
      capturedBody->addArgument(value.getType(), loc());
    b.setInsertionPointToEnd(capturedBody);
    op(ac::YieldOp::getOperationName());
    return captured;
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
  LogicalResult verifyWiring(uint64_t limit = 1 << 22) {
    ac::HardwareAnalysis analysis(*package);
    SmallVector<std::pair<ac::ModuleOp, ac::HardwareBindings>> scopes;
    for (auto module : package->getOps<ac::ModuleOp>()) {
      ac::HardwareBindings bindings;
      bindings.owner = module;
      scopes.emplace_back(module, std::move(bindings));
    }
    detail::SourceCheckBudget budget({limit});
    detail::SampledAdmissionValidation validation(analysis, budget);
    return detail::verifyTransactionWiring(analysis, scopes, validation,
                                           budget);
  }
  void makeExternalIncomingHold() {
    storage->setOperands({definition.getBody().front().getArgument(1),
                          definition.getBody().front().getArgument(2), zero,
                          zero, zero});
    expect.erase();
    rule.erase();
    auto &body = definition.getBody().front();
    auto data = body.addArgument(bit(), loc());
    auto enable = body.addArgument(bit(), loc());
    storage->setOperand(2, enable);
    storage->setOperand(3, data);
    definition->setAttr("function_type",
                        TypeAttr::get(b.getFunctionType(
                            {bit(), bit(), bit(), bit(), bit()}, {})));
    definition->setAttr(
        "input_names",
        b.getArrayAttr({b.getStringAttr("enabled"), b.getStringAttr("pyc_clk"),
                        b.getStringAttr("pyc_rst"),
                        b.getStringAttr("__ac_transaction_d_0"),
                        b.getStringAttr("__ac_transaction_e_0")}));
    definition->setAttr(
        "ac.internal_ports",
        d({f("source_inputs", b.getI64IntegerAttr(3)),
           f("source_outputs", b.getI64IntegerAttr(0)),
           f("root_inputs", b.getArrayAttr({b.getI64IntegerAttr(0),
                                            b.getI64IntegerAttr(0)}))}));
    definition->setAttr(
        "ac.transaction_wiring",
        wiring({occurrence(20)}, {incoming(stateID(), 3, 4)}, {}));

    auto system = *package->getOps<ac::SystemOp>().begin();
    b.setInsertionPoint(system);
    OperationState parentState(loc(), ac::ModuleOp::getOperationName());
    parentState.addAttributes(
        {f("sym_name", b.getStringAttr("Parent")),
         f("source_owner", d({f("package", b.getStringAttr("")),
                              f("path", b.getStringAttr("parent.py"))})),
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
    parentState.addRegion();
    proofRoot = cast<ac::ModuleOp>(b.create(parentState));
    auto *parentBody = new Block;
    proofRoot.getBody().push_back(parentBody);
    for (unsigned i = 0; i < 3; ++i)
      parentBody->addArgument(bit(), loc());
    b.setInsertionPointToEnd(parentBody);
    Value parentZero = constant(0);
    op(ac::InstanceOp::getOperationName(),
       {parentBody->getArgument(0), parentBody->getArgument(1),
        parentBody->getArgument(2), parentZero, parentZero},
       {},
       {f("callee", FlatSymbolRefAttr::get(&context, "Owner")),
        f("instance_name", b.getStringAttr("child")),
        f("parameters", b.getArrayAttr({})),
        f("type_arguments", b.getArrayAttr({})),
        f("occurrence", occurrence(31, "Parent"))});
    op(ac::YieldOp::getOperationName());
    NamedAttrList entry(system.getEntry());
    entry.set("callee", FlatSymbolRefAttr::get(&context, "Parent"));
    system->setAttr("entry", entry.getDictionary(&context));
  }
  void exportValueThroughParent(Value exported, bool privateValue) {
    definition->setAttr("function_type", TypeAttr::get(b.getFunctionType(
                                             {bit(), bit(), bit()}, {bit()})));
    definition->setAttr(
        "output_names",
        b.getArrayAttr({b.getStringAttr(
            privateValue ? "__ac_transaction_private_0" : "guard")}));
    if (privateValue)
      definition->setAttr("ac.internal_ports",
                          d({f("source_inputs", b.getI64IntegerAttr(3)),
                             f("source_outputs", b.getI64IntegerAttr(0)),
                             f("root_inputs", b.getArrayAttr({}))}));
    else
      definition->removeAttr("ac.internal_ports");
    definition.getBody().front().getTerminator()->setOperands(exported);

    auto system = *package->getOps<ac::SystemOp>().begin();
    b.setInsertionPoint(system);
    OperationState parentState(loc(), ac::ModuleOp::getOperationName());
    parentState.addAttributes(
        {f("sym_name", b.getStringAttr("Parent")),
         f("source_owner", d({f("package", b.getStringAttr("")),
                              f("path", b.getStringAttr("parent.py"))})),
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
    parentState.addRegion();
    proofRoot = cast<ac::ModuleOp>(b.create(parentState));
    auto *parentBody = new Block;
    proofRoot.getBody().push_back(parentBody);
    for (unsigned i = 0; i < 3; ++i)
      parentBody->addArgument(bit(), loc());
    b.setInsertionPointToEnd(parentBody);
    op(ac::InstanceOp::getOperationName(), parentBody->getArguments(), {bit()},
       {f("callee", FlatSymbolRefAttr::get(&context, "Owner")),
        f("instance_name", b.getStringAttr("child")),
        f("parameters", b.getArrayAttr({})),
        f("type_arguments", b.getArrayAttr({})),
        f("occurrence", occurrence(31, "Parent"))});
    op(ac::YieldOp::getOperationName());
    NamedAttrList entry(system.getEntry());
    entry.set("callee", FlatSymbolRefAttr::get(&context, "Parent"));
    system->setAttr("entry", entry.getDictionary(&context));
  }
  void exportUnusedPrivateValue(Value exported) {
    exportValueThroughParent(exported, true);
  }
  ac::RuleOp addValuedParentGuardedBy(Value predicate, uint64_t index = 0) {
    auto &body = proofRoot.getBody().front();
    auto registration = 40 + 2 * index;
    auto location = d({f("path", b.getStringAttr("parent.py")),
                       f("line", b.getI64IntegerAttr(4)),
                       f("column", b.getI64IntegerAttr(2)),
                       f("end_line", b.getI64IntegerAttr(4)),
                       f("end_column", b.getI64IntegerAttr(21))});
    auto id = d({f("registration", occurrence(registration, "Parent")),
                 f("check", occurrence(registration + 1, "Parent")),
                 f("obligation", b.getI64IntegerAttr(0))});
    b.setInsertionPoint(body.getTerminator());
    OperationState state(loc(), ac::RuleOp::getOperationName());
    state.addOperands({body.getArgument(1), body.getArgument(2), predicate});
    state.addTypes(SmallVector<Type>(4, bit()));
    state.addAttributes(
        {f("name", b.getStringAttr("valued_" + std::to_string(index))),
         f("occurrence", occurrence(registration, "Parent")),
         f("ac.required_checks",
           b.getArrayAttr(
               {d({f("id", id), f("kind", b.getStringAttr("guard_known")),
                   f("location", location)})})),
         f("ac.sampled_admission",
           d({f("clock_capture", b.getI64IntegerAttr(0)),
              f("reset_capture", b.getI64IntegerAttr(1)),
              f("fire_result", b.getI64IntegerAttr(1)),
              f("body_checks", b.getArrayAttr({})),
              f("observations", b.getArrayAttr({})),
              f("guards",
                b.getArrayAttr(
                    {d({f("id", id), f("invocation", b.getArrayAttr({})),
                        f("predecessor", b.getI64IntegerAttr(-1))})}))}))});
    state.addRegion();
    auto valued = cast<ac::RuleOp>(b.create(state));
    auto *ruleBody = new Block;
    valued.getBody().push_back(ruleBody);
    for (Value capture : valued.getOperands())
      ruleBody->addArgument(capture.getType(), loc());
    b.setInsertionPointToEnd(ruleBody);
    auto knownOne = constant(1, "Parent");
    auto admitted = op(ac::BitsCompareOp::getOperationName(),
                       {ruleBody->getArgument(2), knownOne}, {bit()},
                       {f("predicate", b.getStringAttr("case_eq"))});
    op(ac::YieldOp::getOperationName(),
       {ruleBody->getArgument(2), admitted->getResult(0),
        ruleBody->getArgument(2), knownOne});
    b.setInsertionPoint(body.getTerminator());
    op(ac::SourceExpectOp::getOperationName(),
       {valued.getResult(2), valued.getResult(3), body.getArgument(1),
        body.getArgument(2)},
       {},
       {f("kind", b.getStringAttr("guard_known")), f("location", location),
        f("ac.check_id", id)});
    SmallVector<Attribute> writers;
    if (auto metadata =
            proofRoot->getAttrOfType<DictionaryAttr>("ac.transaction_wiring"))
      llvm::append_range(writers,
                         metadata.getAs<ArrayAttr>("writers").getValue());
    writers.push_back(d({f("registration", occurrence(registration, "Parent")),
                         f("ordinary_results", b.getI64IntegerAttr(1)),
                         f("candidates", b.getArrayAttr({}))}));
    proofRoot->setAttr("ac.transaction_wiring", wiring({}, {}, writers));
    return valued;
  }
  Value makePlainCheckedPrivateValue(StringRef kind) {
    expect.erase();
    rule.erase();
    definition->removeAttr("ac.transaction_wiring");
    auto id = checkID();
    auto &body = definition.getBody().front();
    b.setInsertionPoint(body.getTerminator());
    OperationState state(loc(), ac::RuleOp::getOperationName());
    state.addOperands(one);
    state.addTypes({bit(), bit()});
    state.addAttributes(
        {f("name", b.getStringAttr("plain_check")),
         f("occurrence", occurrence(7)),
         f("ac.required_checks",
           b.getArrayAttr({d({f("id", id), f("kind", b.getStringAttr(kind)),
                              f("location", span())})}))});
    state.addRegion();
    rule = cast<ac::RuleOp>(b.create(state));
    auto *ruleBody = new Block;
    rule.getBody().push_back(ruleBody);
    auto value = ruleBody->addArgument(bit(), loc());
    b.setInsertionPointToEnd(ruleBody);
    op(ac::YieldOp::getOperationName(), {value, value});
    b.setInsertionPoint(body.getTerminator());
    expect = cast<ac::SourceExpectOp>(
        op(ac::SourceExpectOp::getOperationName(), rule.getResults(), {},
           {f("kind", b.getStringAttr(kind)), f("location", span()),
            f("ac.check_id", id)}));
    return rule.getResult(0);
  }
  DictionaryAttr indexedCheckID(uint64_t registration, uint64_t check) {
    return d({f("registration", occurrence(registration)),
              f("check", occurrence(check)),
              f("obligation", b.getI64IntegerAttr(0))});
  }
  ac::RuleOp addIndependentSampledRule(uint64_t index) {
    auto registration = 100 + 2 * index;
    auto id = indexedCheckID(registration, registration + 1);
    auto *copy = rule->clone();
    copy->setAttr("name", b.getStringAttr("sampled_" + std::to_string(index)));
    copy->setAttr("occurrence", occurrence(registration));
    copy->setAttr("ac.required_checks",
                  b.getArrayAttr({d({f("id", id),
                                     f("kind", b.getStringAttr("guard_known")),
                                     f("location", span())})}));
    NamedAttrList sampled(
        copy->getAttrOfType<DictionaryAttr>("ac.sampled_admission"));
    auto guards = cast<ArrayAttr>(sampled.get("guards"));
    NamedAttrList guard(cast<DictionaryAttr>(guards[0]));
    guard.set("id", id);
    sampled.set("guards", b.getArrayAttr({guard.getDictionary(&context)}));
    copy->setAttr("ac.sampled_admission", sampled.getDictionary(&context));
    definition.getBody().front().getOperations().insert(
        definition.getBody().front().getTerminator()->getIterator(), copy);
    auto sampledRule = cast<ac::RuleOp>(copy);
    auto *expectCopy = expect->clone();
    expectCopy->setOperands({sampledRule.getResult(1), sampledRule.getResult(2),
                             definition.getBody().front().getArgument(1),
                             definition.getBody().front().getArgument(2)});
    expectCopy->setAttr("ac.check_id", id);
    definition.getBody().front().getOperations().insert(
        definition.getBody().front().getTerminator()->getIterator(),
        expectCopy);
    auto metadata =
        definition->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
    SmallVector<Attribute> writers(
        metadata.getAs<ArrayAttr>("writers").getValue());
    writers.push_back(writer({}));
    NamedAttrList changed(cast<DictionaryAttr>(writers.back()));
    changed.set("registration", occurrence(registration));
    writers.back() = changed.getDictionary(&context);
    NamedAttrList wiringFields(metadata);
    wiringFields.set("writers", b.getArrayAttr(writers));
    definition->setAttr("ac.transaction_wiring",
                        wiringFields.getDictionary(&context));
    return sampledRule;
  }
  ac::RuleOp addPlainCheck(uint64_t index) {
    auto registration = 300 + 2 * index;
    auto id = indexedCheckID(registration, registration + 1);
    auto &body = definition.getBody().front();
    b.setInsertionPoint(body.getTerminator());
    OperationState state(loc(), ac::RuleOp::getOperationName());
    state.addOperands(one);
    state.addTypes({bit(), bit()});
    state.addAttributes(
        {f("name", b.getStringAttr("plain_" + std::to_string(index))),
         f("occurrence", occurrence(registration)),
         f("ac.required_checks",
           b.getArrayAttr({d({f("id", id), f("kind", b.getStringAttr("assert")),
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
       {f("kind", b.getStringAttr("assert")), f("location", span()),
        f("ac.check_id", id)});
    return checked;
  }
  void growGuardIdentityChain(unsigned length) {
    auto *yielded = rule.getBody().front().getTerminator();
    auto compare = *rule.getBody().front().getOps<ac::BitsCompareOp>().begin();
    Value value = yielded->getOperand(1);
    b.setInsertionPoint(compare);
    for (unsigned i = 0; i < length; ++i)
      value = op(ac::BitsExtractOp::getOperationName(), {value}, {bit()},
                 {f("low", literal(0))})
                  ->getResult(0);
    compare->setOperand(0, value);
    yielded->setOperand(1, value);
  }
  void makeSharedPrefixGuards(unsigned count) {
    ASSERT_GT(count, 0u);
    expect.erase();
    rule.erase();
    auto &body = definition.getBody().front();
    SmallVector<Attribute> required, guards;
    auto guardID = [&](unsigned ordinal) {
      return d({f("registration", occurrence(7)), f("check", occurrence(8)),
                f("obligation", b.getI64IntegerAttr(ordinal))});
    };
    auto invocation = [&](unsigned ordinal) {
      if (!ordinal)
        return b.getArrayAttr({});
      return b.getArrayAttr(
          {d({f("call", occurrence(500 + ordinal)),
              f("receiver", d({f("instance_path", b.getArrayAttr({}))}))})});
    };
    for (unsigned i = 0; i < count; ++i) {
      auto id = guardID(i);
      required.push_back(
          d({f("id", id), f("kind", b.getStringAttr("guard_known")),
             f("location", span())}));
      guards.push_back(
          d({f("id", id), f("invocation", invocation(i)),
             f("predecessor", b.getI64IntegerAttr(i ? int64_t(i - 1) : -1))}));
    }
    b.setInsertionPoint(body.getTerminator());
    OperationState state(loc(), ac::RuleOp::getOperationName());
    state.addOperands(
        {body.getArgument(1), body.getArgument(2), body.getArgument(0)});
    state.addTypes(SmallVector<Type>(1 + 2 * count, bit()));
    state.addAttributes({f("name", b.getStringAttr("tick")),
                         f("occurrence", occurrence(7)),
                         f("ac.required_checks", b.getArrayAttr(required)),
                         f("ac.sampled_admission",
                           d({f("clock_capture", b.getI64IntegerAttr(0)),
                              f("reset_capture", b.getI64IntegerAttr(1)),
                              f("fire_result", b.getI64IntegerAttr(0)),
                              f("body_checks", b.getArrayAttr({})),
                              f("observations", b.getArrayAttr({})),
                              f("guards", b.getArrayAttr(guards))}))});
    state.addRegion();
    rule = cast<ac::RuleOp>(b.create(state));
    auto *ruleBody = new Block;
    rule.getBody().push_back(ruleBody);
    for (Value capture : rule.getOperands())
      ruleBody->addArgument(capture.getType(), loc());
    b.setInsertionPointToEnd(ruleBody);
    Value knownOne = constant(1);
    auto known = op(ac::BitsCompareOp::getOperationName(),
                    {ruleBody->getArgument(2), knownOne}, {bit()},
                    {f("predicate", b.getStringAttr("case_eq"))})
                     ->getResult(0);
    Value prefix = knownOne;
    SmallVector<Value> suffix;
    for (unsigned i = 0; i < count; ++i) {
      suffix.push_back(ruleBody->getArgument(2));
      suffix.push_back(prefix);
      prefix = op(ac::BitsBinaryOp::getOperationName(), {prefix, known},
                  {bit()}, {f("opcode", b.getStringAttr("and"))})
                   ->getResult(0);
    }
    SmallVector<Value> yielded{prefix};
    llvm::append_range(yielded, suffix);
    op(ac::YieldOp::getOperationName(), yielded);
    b.setInsertionPoint(body.getTerminator());
    for (unsigned i = 0; i < count; ++i)
      op(ac::SourceExpectOp::getOperationName(),
         {rule.getResult(1 + 2 * i), rule.getResult(2 + 2 * i),
          body.getArgument(1), body.getArgument(2)},
         {},
         {f("kind", b.getStringAttr("guard_known")), f("location", span()),
          f("ac.check_id", guardID(i))});
    definition->setAttr("ac.transaction_wiring", wiring({}, {}, {writer({})}));
  }
  void expectDefaultPlanAndTinyBudget(unsigned checks,
                                      unsigned samplingBindings) {
    auto before = printPackage();
    ac::HardwareAnalysis analysis(*package);
    auto plan = capture([&] { return analysis.getSourceCheckPlan(); });
    ASSERT_TRUE(succeeded(plan)) << diagnostics;
    EXPECT_EQ(plan->checks.size(), checks);
    EXPECT_EQ(plan->samplingBindings.size(), samplingBindings);
    auto tiny = capture([&] {
      return analysis.getSourceCheckPlan(ac::HardwareSourceCheckLimits{1});
    });
    EXPECT_TRUE(failed(tiny));
    EXPECT_NE(
        diagnostics.find("source-check plan exceeds analysis work budget"),
        std::string::npos)
        << diagnostics;
    EXPECT_EQ(printPackage(), before);
    auto repeated = capture([&] { return analysis.getSourceCheckPlan(); });
    ASSERT_TRUE(succeeded(repeated)) << diagnostics;
    EXPECT_EQ(repeated->checks.size(), checks);
    EXPECT_EQ(repeated->samplingBindings.size(), samplingBindings);
  }
  void instantiateSampledOwnerTwice() {
    auto system = *package->getOps<ac::SystemOp>().begin();
    b.setInsertionPoint(system);
    OperationState parentState(loc(), ac::ModuleOp::getOperationName());
    parentState.addAttributes(
        {f("sym_name", b.getStringAttr("Parent")),
         f("source_owner", d({f("package", b.getStringAttr("")),
                              f("path", b.getStringAttr("parent.py"))})),
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
    parentState.addRegion();
    proofRoot = cast<ac::ModuleOp>(b.create(parentState));
    auto *parentBody = new Block;
    proofRoot.getBody().push_back(parentBody);
    for (unsigned i = 0; i < 3; ++i)
      parentBody->addArgument(bit(), loc());
    b.setInsertionPointToEnd(parentBody);
    for (unsigned index : {31u, 32u})
      op(ac::InstanceOp::getOperationName(), parentBody->getArguments(), {},
         {f("callee", FlatSymbolRefAttr::get(&context, "Owner")),
          f("instance_name", b.getStringAttr(index == 31 ? "left" : "right")),
          f("parameters", b.getArrayAttr({})),
          f("type_arguments", b.getArrayAttr({})),
          f("occurrence", occurrence(index, "Parent"))});
    op(ac::YieldOp::getOperationName());
    NamedAttrList entry(system.getEntry());
    entry.set("callee", FlatSymbolRefAttr::get(&context, "Parent"));
    system->setAttr("entry", entry.getDictionary(&context));
  }
  void makeValuedStatefree(bool literalTrueGuard) {
    ASSERT_FALSE(storage);
    OperationState state(loc(), ac::RuleOp::getOperationName());
    state.addOperands(rule.getOperands());
    state.addTypes(SmallVector<Type>(4, bit()));
    NamedAttrList attrs(rule->getAttrs());
    NamedAttrList sampled(
        rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission"));
    sampled.set("fire_result", b.getI64IntegerAttr(1));
    attrs.set("ac.sampled_admission", sampled.getDictionary(&context));
    state.addAttributes(attrs);
    state.addRegion();
    b.setInsertionPoint(rule);
    auto replacement = cast<ac::RuleOp>(b.create(state));
    auto *body = new Block;
    replacement.getBody().push_back(body);
    for (Value capture : replacement.getOperands())
      body->addArgument(capture.getType(), loc());
    b.setInsertionPointToEnd(body);
    Value knownOne = constant(1);
    Value predicate = literalTrueGuard ? knownOne : body->getArgument(2);
    auto admitted =
        op(ac::BitsCompareOp::getOperationName(), {predicate, knownOne},
           {bit()}, {f("predicate", b.getStringAttr("case_eq"))});
    op(ac::YieldOp::getOperationName(),
       {body->getArgument(2), admitted->getResult(0), predicate, knownOne});
    expect->setOperands({replacement.getResult(2), replacement.getResult(3),
                         definition.getBody().front().getArgument(1),
                         definition.getBody().front().getArgument(2)});
    rule.erase();
    rule = replacement;
    definition->setAttr(
        "ac.transaction_wiring",
        wiring({}, {},
               {d({f("registration", occurrence(7)),
                   f("ordinary_results", b.getI64IntegerAttr(1)),
                   f("candidates", b.getArrayAttr({}))})}));
  }
  template <typename Mutation>
  void rejects(StringRef label, Mutation mutation, StringRef diagnostic = {}) {
    SCOPED_TRACE(label.str());
    build();
    ASSERT_TRUE(succeeded(capture([&] { return verifyWiring(); })))
        << diagnostics;
    mutation();
    EXPECT_TRUE(failed(capture([&] { return verifyWiring(); })))
        << "mutation unexpectedly admitted: " << label.str();
    EXPECT_FALSE(diagnostics.empty());
    if (!diagnostic.empty())
      EXPECT_NE(diagnostics.find(diagnostic.str()), std::string::npos)
          << diagnostics;
  }
};

TEST_F(HardwareSampledAdmissionWiringTest,
       StatefulAndStatefreeWritersUseTheSameClosedCompanion) {
  for (bool stateful : {false, true}) {
    SCOPED_TRACE(stateful);
    build(stateful);
    EXPECT_TRUE(succeeded(capture([&] { return verifyWiring(); })))
        << diagnostics;
  }
}

TEST_F(HardwareSampledAdmissionWiringTest,
       PreparedBoundariesRetainStatefulCandidateFireAndGuardSuffixes) {
  build(true);
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
  auto contains = [&](Value value) {
    return llvm::any_of(
        (**prepared).getBoundaries(),
        [&](const auto &endpoint) { return endpoint.value == value; });
  };
  ASSERT_EQ(rule->getNumResults(), 4u);
  for (Value value : rule->getResults())
    EXPECT_TRUE(contains(value));
}

TEST_F(HardwareSampledAdmissionWiringTest,
       PreparedPointersStayStableAcrossImmutableContextGrowth) {
  build(false);
  exportValueThroughParent(one, false);
  auto child = *proofRoot.getBody().front().getOps<ac::InstanceOp>().begin();
  auto parentRule = addValuedParentGuardedBy(child.getResult(0));
  ac::HardwareAnalysis analysis(*package);
  ac::HardwareBindings ownerBindings;
  ownerBindings.owner = definition;
  ac::HardwareBindings parentBindings;
  parentBindings.owner = proofRoot;
  detail::SourceCheckBudget budget({1 << 20});
  detail::SampledAdmissionValidation validation(analysis, budget);
  auto ownerPrepared =
      capture([&] { return validation.prepare(definition, ownerBindings); });
  ASSERT_TRUE(succeeded(ownerPrepared)) << diagnostics;
  ASSERT_NE(*ownerPrepared, nullptr);
  auto parentPrepared =
      capture([&] { return validation.prepare(proofRoot, parentBindings); });
  ASSERT_TRUE(succeeded(parentPrepared)) << diagnostics;
  ASSERT_NE(*parentPrepared, nullptr);
  EXPECT_NE(*ownerPrepared, *parentPrepared);
  auto ownerAgain =
      capture([&] { return validation.prepare(definition, ownerBindings); });
  ASSERT_TRUE(succeeded(ownerAgain)) << diagnostics;
  EXPECT_EQ(*ownerAgain, *ownerPrepared);
  EXPECT_TRUE(succeeded(capture([&] {
    return validation.verify(rule, ownerBindings);
  }))) << diagnostics;
  EXPECT_TRUE(succeeded(capture([&] {
    return validation.verify(parentRule, parentBindings);
  }))) << diagnostics;
  EXPECT_TRUE(succeeded(capture([&] {
    return validation.verifyInvocation(analysis, budget);
  }))) << diagnostics;
}

TEST_F(HardwareSampledAdmissionWiringTest,
       ThreeCandidatesRetainEveryDataRouteAndOneSharedFire) {
  build(true);
  auto states = expandToThreeCandidates();
  ASSERT_EQ(states.size(), 3u);
  for (auto [index, instance] : llvm::enumerate(states)) {
    EXPECT_EQ(instance->getOperand(2), rule.getResult(3));
    EXPECT_EQ(instance->getOperand(3), rule.getResult(index));
  }
  EXPECT_TRUE(succeeded(capture([&] { return verifyWiring(); })))
      << diagnostics;
}

TEST_F(HardwareSampledAdmissionWiringTest,
       LateSharedFireRuleCaptureRejectsWithAllPhysicalRoutesIntact) {
  build(true);
  auto states = expandToThreeCandidates();
  ASSERT_EQ(states.size(), 3u);
  addIllegalRuleCapture({rule.getResult(3)});
  for (auto [index, instance] : llvm::enumerate(states)) {
    EXPECT_EQ(instance->getOperand(2), rule.getResult(3));
    EXPECT_EQ(instance->getOperand(3), rule.getResult(index));
  }
  EXPECT_TRUE(failed(capture([&] { return verifyWiring(); }))) << diagnostics;
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(HardwareSampledAdmissionWiringTest,
       LateSharedFireIdentityForkWithoutDeadEndRejects) {
  build(true);
  auto states = expandToThreeCandidates();
  ASSERT_EQ(states.size(), 3u);
  auto &body = definition.getBody().front();
  b.setInsertionPoint(body.getTerminator());
  auto identity = [&](Value value) {
    return op(ac::BitsExtractOp::getOperationName(), {value}, {bit()},
              {f("low", literal(0))})
        ->getResult(0);
  };
  auto trunk = identity(rule.getResult(3));
  auto left = identity(trunk);
  auto right = identity(trunk);
  addIllegalRuleCapture({left, right}, "illegal_fork");
  for (auto [index, instance] : llvm::enumerate(states)) {
    EXPECT_EQ(instance->getOperand(2), rule.getResult(3));
    EXPECT_EQ(instance->getOperand(3), rule.getResult(index));
  }
  EXPECT_TRUE(failed(capture([&] { return verifyWiring(); }))) << diagnostics;
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(HardwareSampledAdmissionWiringTest,
       LateSharedFireIdentityForkWithDeadEndsRejects) {
  build(true);
  auto states = expandToThreeCandidates();
  ASSERT_EQ(states.size(), 3u);
  auto &body = definition.getBody().front();
  b.setInsertionPoint(body.getTerminator());
  auto identity = [&](Value value) {
    return op(ac::BitsExtractOp::getOperationName(), {value}, {bit()},
              {f("low", literal(0))})
        ->getResult(0);
  };
  auto trunk = identity(rule.getResult(3));
  identity(trunk);
  identity(trunk);
  for (auto [index, instance] : llvm::enumerate(states)) {
    EXPECT_EQ(instance->getOperand(2), rule.getResult(3));
    EXPECT_EQ(instance->getOperand(3), rule.getResult(index));
  }
  EXPECT_TRUE(failed(capture([&] { return verifyWiring(); }))) << diagnostics;
  EXPECT_NE(diagnostics.find("private transaction/check route has a dead end"),
            std::string::npos)
      << diagnostics;
}

TEST_F(HardwareSampledAdmissionWiringTest,
       EmptyCompanionWithoutAnyFinalWitnessFailsClosed) {
  build(false);
  expect.erase();
  rule.erase();
  definition->setAttr("ac.transaction_wiring", wiring({}, {}, {}));
  EXPECT_TRUE(failed(capture([&] { return verifyWiring(); }))) << diagnostics;
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(HardwareSampledAdmissionWiringTest,
       CompanionAndWriterSchemasRejectMissingExtraAndDuplicateRows) {
  rejects("missing companion field", [&] {
    NamedAttrList fields(
        definition->getAttrOfType<DictionaryAttr>("ac.transaction_wiring"));
    fields.erase("incoming");
    definition->setAttr("ac.transaction_wiring",
                        fields.getDictionary(&context));
  });
  rejects("extra companion field", [&] {
    NamedAttrList fields(
        definition->getAttrOfType<DictionaryAttr>("ac.transaction_wiring"));
    fields.set("extra", b.getUnitAttr());
    definition->setAttr("ac.transaction_wiring",
                        fields.getDictionary(&context));
  });
  rejects(
      "missing writer",
      [&] {
        definition->setAttr("ac.transaction_wiring",
                            wiring({occurrence(20)}, {}, {}));
      },
      "missing its transaction writer row");
  rejects(
      "duplicate writer",
      [&] {
        auto metadata =
            definition->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
        auto row = metadata.getAs<ArrayAttr>("writers")[0];
        NamedAttrList fields(metadata);
        fields.set("writers", b.getArrayAttr({row, row}));
        definition->setAttr("ac.transaction_wiring",
                            fields.getDictionary(&context));
      },
      "duplicate transaction writer");
  rejects(
      "wrong ordinary result type",
      [&] {
        auto metadata =
            definition->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
        auto row =
            cast<DictionaryAttr>(metadata.getAs<ArrayAttr>("writers")[0]);
        NamedAttrList changed(row);
        changed.set("ordinary_results", b.getI32IntegerAttr(0));
        NamedAttrList fields(metadata);
        fields.set("writers",
                   b.getArrayAttr({changed.getDictionary(&context)}));
        definition->setAttr("ac.transaction_wiring",
                            fields.getDictionary(&context));
      },
      "nonnegative signless i64");
}

TEST_F(HardwareSampledAdmissionWiringTest,
       CandidateAndPhysicalEnableMustReachTheirDeclaredSlots) {
  rejects(
      "candidate slot skips zero",
      [&] {
        auto metadata =
            definition->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
        auto row =
            cast<DictionaryAttr>(metadata.getAs<ArrayAttr>("writers")[0]);
        NamedAttrList changed(row);
        changed.set("candidates", b.getArrayAttr({candidate(stateID(), 1)}));
        NamedAttrList fields(metadata);
        fields.set("writers",
                   b.getArrayAttr({changed.getDictionary(&context)}));
        definition->setAttr("ac.transaction_wiring",
                            fields.getDictionary(&context));
      },
      "candidate result");
  rejects("D bypasses candidate", [&] { storage->setOperand(3, zero); });
  rejects("E bypasses fire", [&] { storage->setOperand(2, one); });
  rejects("clock uses unrelated pin", [&] {
    storage->setOperand(0, definition.getBody().front().getArgument(0));
  });
  rejects("reset uses unrelated pin", [&] {
    storage->setOperand(1, definition.getBody().front().getArgument(0));
  });
  rejects("sampled clock aliases reset", [&] {
    NamedAttrList fields(
        rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission"));
    fields.set("clock_capture", b.getI64IntegerAttr(1));
    rule->setAttr("ac.sampled_admission", fields.getDictionary(&context));
  });
}

TEST_F(HardwareSampledAdmissionWiringTest,
       LocalProofSuccessDoesNotBypassPhysicalOrPrivateRejection) {
  for (bool privateEscape : {false, true}) {
    SCOPED_TRACE(privateEscape ? "private escape" : "physical route");
    build(true);
    if (privateEscape)
      addIllegalRuleCapture({rule.getResult(1)});
    else
      storage->setOperand(3, zero);
    ac::HardwareAnalysis analysis(*package);
    ac::HardwareBindings bindings;
    bindings.owner = definition;
    detail::SourceCheckBudget budget({1 << 20});
    detail::SampledAdmissionValidation validation(analysis, budget);
    ASSERT_TRUE(succeeded(capture([&] {
      return validation.verify(rule, bindings);
    }))) << diagnostics;
    SmallVector<std::pair<ac::ModuleOp, ac::HardwareBindings>> scopes{
        {definition, bindings}};
    EXPECT_TRUE(failed(capture([&] {
      return detail::verifyTransactionWiring(analysis, scopes, validation,
                                             budget);
    })));
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(HardwareSampledAdmissionWiringTest,
       StateIdentityInventoryAndPrivateValuesCannotBeForged) {
  rejects("missing state inventory", [&] {
    auto metadata =
        definition->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
    NamedAttrList fields(metadata);
    fields.set("states", b.getArrayAttr({}));
    definition->setAttr("ac.transaction_wiring",
                        fields.getDictionary(&context));
  });
  rejects("duplicate state inventory", [&] {
    auto metadata =
        definition->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
    NamedAttrList fields(metadata);
    fields.set("states", b.getArrayAttr({occurrence(20), occurrence(20)}));
    definition->setAttr("ac.transaction_wiring",
                        fields.getDictionary(&context));
  });
  rejects("foreign candidate owner", [&] {
    auto metadata =
        definition->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
    auto row = cast<DictionaryAttr>(metadata.getAs<ArrayAttr>("writers")[0]);
    NamedAttrList changed(row);
    changed.set("candidates", b.getArrayAttr({candidate(stateID(99))}));
    NamedAttrList fields(metadata);
    fields.set("writers", b.getArrayAttr({changed.getDictionary(&context)}));
    definition->setAttr("ac.transaction_wiring",
                        fields.getDictionary(&context));
  });
  rejects("candidate escapes through unrelated capture",
          [&] { rule->insertOperands(0, rule.getResult(0)); });
  rejects("candidate has unused identity branch", [&] {
    b.setInsertionPoint(definition.getBody().front().getTerminator());
    op(ac::BitsExtractOp::getOperationName(), {rule.getResult(0)}, {bit()},
       {f("low", literal(0))});
  });
  rejects("check suffix has unused identity branch", [&] {
    b.setInsertionPoint(definition.getBody().front().getTerminator());
    op(ac::BitsExtractOp::getOperationName(), {rule.getResult(2)}, {bit()},
       {f("low", literal(0))});
  });
}

TEST_F(HardwareSampledAdmissionWiringTest,
       HiddenPrivateOutputMustTerminateAtItsDeclaredPhysicalRoute) {
  build();
  exportUnusedPrivateValue(rule.getResult(0));
  EXPECT_TRUE(failed(capture([&] { return verifyWiring(); }))) << diagnostics;
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(HardwareSampledAdmissionWiringTest,
       UnwrittenDirectStateRequiresThePhysicalHoldPair) {
  build();
  storage->setOperands({definition.getBody().front().getArgument(1),
                        definition.getBody().front().getArgument(2), zero,
                        storage.getResult(0), zero});
  expect.erase();
  rule.erase();
  definition->setAttr("ac.transaction_wiring",
                      wiring({occurrence(20)}, {}, {}));
  EXPECT_TRUE(succeeded(capture([&] { return verifyWiring(); })))
      << diagnostics;
  storage->setOperand(3, zero);
  EXPECT_TRUE(failed(capture([&] { return verifyWiring(); }))) << diagnostics;
  storage->setOperand(3, storage.getResult(0));
  storage->setOperand(3, one);
  EXPECT_TRUE(failed(capture([&] { return verifyWiring(); }))) << diagnostics;
}

TEST_F(HardwareSampledAdmissionWiringTest,
       SelectedRootMayDriveAnUnusedExternalPairWithKnownZeros) {
  build();
  makeExternalIncomingHold();
  EXPECT_TRUE(succeeded(capture([&] { return verifyWiring(); })))
      << diagnostics;
  auto layout = capture([&] { return ac::getModulePortLayout(definition); });
  ASSERT_TRUE(succeeded(layout)) << diagnostics;
  ASSERT_EQ(layout->rootInputDefaults.size(), 2u);
  EXPECT_EQ(layout->rootInputDefaults[0], b.getI64IntegerAttr(0));
  EXPECT_EQ(layout->rootInputDefaults[1], b.getI64IntegerAttr(0));
}

TEST_F(HardwareSampledAdmissionWiringTest,
       StateResolutionRejectsMismatchedNonNullBindingOwner) {
  build();
  ac::HardwareAnalysis analysis(*package);
  ac::HardwareBindings bindings;
  bindings.owner = dffe.getOperation();
  detail::SourceCheckBudget budget({1 << 20});
  auto resolved = capture([&] {
    return detail::resolveTransactionState(definition, stateID(), analysis,
                                           bindings, budget);
  });
  EXPECT_TRUE(failed(resolved));
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(HardwareSampledAdmissionWiringTest, ProofBudgetRejectsWithoutMutation) {
  build();
  auto before = printPackage();
  auto metadata = definition->getAttr("ac.transaction_wiring");
  EXPECT_TRUE(failed(capture([&] { return verifyWiring(1); })));
  EXPECT_NE(diagnostics.find("source-check plan exceeds analysis work budget"),
            std::string::npos)
      << diagnostics;
  EXPECT_EQ(definition->getAttr("ac.transaction_wiring"), metadata);
  EXPECT_EQ(storage->getOperand(2), rule.getResult(1));
  EXPECT_EQ(printPackage(), before);
}

TEST_F(HardwareSampledAdmissionWiringTest,
       RepeatedExactRootScopesShareOneBoundedSuccessfulInvocation) {
  build(true);
  ac::HardwareAnalysis analysis(*package);
  ac::HardwareBindings bindings;
  bindings.owner = definition;
  SmallVector<std::pair<ac::ModuleOp, ac::HardwareBindings>> scopes;
  for (unsigned i = 0; i < 64; ++i)
    scopes.emplace_back(definition, bindings);
  detail::SourceCheckBudget budget({1 << 20});
  detail::SampledAdmissionValidation validation(analysis, budget);
  EXPECT_TRUE(succeeded(capture([&] {
    return detail::verifyTransactionWiring(analysis, scopes, validation,
                                           budget);
  }))) << diagnostics;
}

TEST_F(HardwareSampledAdmissionWiringTest,
       ExactRootScopeReuseDoesNotSkipMismatchedBindingOwner) {
  build(true);
  ac::HardwareAnalysis analysis(*package);
  ac::HardwareBindings valid;
  valid.owner = definition;
  ac::HardwareBindings invalid;
  invalid.owner = dffe.getOperation();
  SmallVector<std::pair<ac::ModuleOp, ac::HardwareBindings>> scopes{
      {definition, valid}, {definition, invalid}};
  detail::SourceCheckBudget budget({1 << 20});
  detail::SampledAdmissionValidation validation(analysis, budget);
  EXPECT_TRUE(failed(capture([&] {
    return detail::verifyTransactionWiring(analysis, scopes, validation,
                                           budget);
  })));
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(HardwareSampledAdmissionWiringTest,
       ExactRootScopeReuseDistinguishesIntegerAndTypeBindings) {
  for (bool typeParameter : {false, true}) {
    SCOPED_TRACE(typeParameter ? "type parameter" : "integer parameter");
    build(true);
    ac::HardwareBindings valid, invalid;
    valid.owner = definition;
    invalid.owner = definition;
    if (typeParameter) {
      definition->setAttr("type_parameters",
                          b.getArrayAttr({b.getStringAttr("T")}));
      setStatePayloadType(ac::TypeParamType::get(
          &context, FlatSymbolRefAttr::get(&context, "Owner"),
          b.getStringAttr("T")));
      valid.types["T"] = bit();
      invalid.types["T"] = ac::BitsType::get(&context, literal(2));
    } else {
      definition->setAttr("parameters",
                          b.getArrayAttr({ac::getStaticParameterAttr(b, "N")}));
      setDomainType(ac::BitsType::get(&context, parameterReference("N")));
      valid.integers["N"] = ac::MathIntAttr::get(&context, llvm::APSInt("1"));
      invalid.integers["N"] = ac::MathIntAttr::get(&context, llvm::APSInt("2"));
    }
    ac::HardwareAnalysis analysis(*package);
    SmallVector<std::pair<ac::ModuleOp, ac::HardwareBindings>> validScope{
        {definition, valid}};
    detail::SourceCheckBudget validBudget({1 << 20});
    detail::SampledAdmissionValidation validValidation(analysis, validBudget);
    ASSERT_TRUE(succeeded(capture([&] {
      return detail::verifyTransactionWiring(analysis, validScope,
                                             validValidation, validBudget);
    }))) << diagnostics;

    SmallVector<std::pair<ac::ModuleOp, ac::HardwareBindings>> mixedScopes{
        {definition, valid}, {definition, invalid}};
    detail::SourceCheckBudget invalidBudget({1 << 20});
    detail::SampledAdmissionValidation invalidValidation(analysis,
                                                         invalidBudget);
    EXPECT_TRUE(failed(capture([&] {
      return detail::verifyTransactionWiring(analysis, mixedScopes,
                                             invalidValidation, invalidBudget);
    })));
    EXPECT_FALSE(diagnostics.empty());
    EXPECT_EQ(
        diagnostics.find("source-check plan exceeds analysis work budget"),
        std::string::npos)
        << diagnostics;
    if (typeParameter)
      EXPECT_NE(diagnostics.find("transaction candidate/route payload differs "
                                 "from actual state"),
                std::string::npos)
          << diagnostics;
  }
}

TEST_F(HardwareSampledAdmissionWiringTest,
       RepeatedSelectedRootDoesNotHideUnselectedInvalidDefinition) {
  build(true);
  auto system = *package->getOps<ac::SystemOp>().begin();
  auto *copy = definition->clone();
  copy->setAttr("sym_name", b.getStringAttr("UnselectedBad"));
  NamedAttrList malformed(
      copy->getAttrOfType<DictionaryAttr>("ac.transaction_wiring"));
  malformed.erase("writers");
  copy->setAttr("ac.transaction_wiring", malformed.getDictionary(&context));
  package->getBody()->getOperations().insert(system->getIterator(), copy);
  auto bad = cast<ac::ModuleOp>(copy);
  ac::HardwareAnalysis analysis(*package);
  ac::HardwareBindings selectedBindings;
  selectedBindings.owner = definition;
  ac::HardwareBindings badBindings;
  badBindings.owner = bad;
  SmallVector<std::pair<ac::ModuleOp, ac::HardwareBindings>> scopes;
  for (unsigned i = 0; i < 32; ++i)
    scopes.emplace_back(definition, selectedBindings);
  scopes.emplace_back(bad, badBindings);
  detail::SourceCheckBudget budget({1 << 20});
  detail::SampledAdmissionValidation validation(analysis, budget);
  EXPECT_TRUE(failed(capture([&] {
    return detail::verifyTransactionWiring(analysis, scopes, validation,
                                           budget);
  })));
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(HardwareSampledAdmissionWiringTest,
       DefaultBudgetIgnoresUnrelatedSharedCombinationalDAG) {
  build(false);
  auto *terminator = definition.getBody().front().getTerminator();
  b.setInsertionPoint(terminator);
  Value shared = one;
  for (unsigned i = 0; i < 160; ++i)
    shared = op(ac::BitsBinaryOp::getOperationName(), {shared, one}, {bit()},
                {f("opcode", b.getStringAttr("and"))})
                 ->getResult(0);
  auto plan = capture(
      [&] { return ac::HardwareAnalysis(*package).getSourceCheckPlan(); });
  ASSERT_TRUE(succeeded(plan)) << diagnostics;
  ASSERT_EQ(plan->samplingBindings.size(), 1u);
  ASSERT_EQ(plan->samplingOccurrences.size(), 1u);
  ASSERT_EQ(plan->checks.size(), 1u);
  EXPECT_EQ(plan->checks.front().samplingOccurrenceIndex, 0u);
}

TEST_F(HardwareSampledAdmissionWiringTest,
       DefaultBudgetAdmitsIndependentSmallSampledRules) {
  build(false);
  constexpr unsigned additionalRules = 10;
  for (unsigned i = 0; i < additionalRules; ++i)
    addIndependentSampledRule(i);
  expectDefaultPlanAndTinyBudget(1 + additionalRules, 1);
}

TEST_F(HardwareSampledAdmissionWiringTest,
       IdentityChainGrowthRemainsBoundedAndAtomic) {
  build(false);
  growGuardIdentityChain(64);
  expectDefaultPlanAndTinyBudget(1, 1);
}

TEST_F(HardwareSampledAdmissionWiringTest,
       PrivateStopInventoryGrowthRemainsBoundedAndAtomic) {
  build(false);
  constexpr unsigned additionalChecks = 16;
  for (unsigned i = 0; i < additionalChecks; ++i)
    addPlainCheck(i);
  expectDefaultPlanAndTinyBudget(1 + additionalChecks, 1);
}

TEST_F(HardwareSampledAdmissionWiringTest,
       SharedPrefixDagGrowthRemainsBoundedAndAtomic) {
  build(false);
  constexpr unsigned guardCount = 8;
  makeSharedPrefixGuards(guardCount);
  expectDefaultPlanAndTinyBudget(guardCount, 1);
}

TEST_F(HardwareSampledAdmissionWiringTest,
       SavedPlanRevalidatesSamplingVectorsIndicesAndPins) {
  build(false);
  ac::HardwareAnalysis analysis(*package);
  auto plan = capture([&] { return analysis.getSourceCheckPlan(); });
  ASSERT_TRUE(succeeded(plan)) << diagnostics;
  ASSERT_TRUE(succeeded(capture([&] {
    return analysis.verifySourceCheckPlan(*plan);
  }))) << diagnostics;
  ASSERT_EQ(plan->bindings.size(), 1u);
  ASSERT_EQ(plan->checks.size(), 1u);
  ASSERT_EQ(plan->samplingBindings.size(), 1u);
  ASSERT_EQ(plan->samplingOccurrences.size(), 1u);
  ASSERT_EQ(plan->bindings[0].samplingBindingIndex, 0u);
  ASSERT_EQ(plan->checks[0].samplingOccurrenceIndex, 0u);

  auto rejectsPlan = [&](StringRef label, auto mutate) {
    SCOPED_TRACE(label.str());
    auto altered = *plan;
    mutate(altered);
    EXPECT_TRUE(failed(capture([&] {
      return analysis.verifySourceCheckPlan(altered);
    }))) << "stale sampling plan unexpectedly admitted: "
         << label.str();
  };
  rejectsPlan("missing sampling binding",
              [](auto &altered) { altered.samplingBindings.clear(); });
  rejectsPlan("missing sampling occurrence",
              [](auto &altered) { altered.samplingOccurrences.clear(); });
  rejectsPlan("binding optional index", [](auto &altered) {
    altered.bindings[0].samplingBindingIndex.reset();
  });
  rejectsPlan("check optional index", [](auto &altered) {
    altered.checks[0].samplingOccurrenceIndex.reset();
  });
  rejectsPlan("sampling clock", [](auto &altered) {
    altered.samplingBindings[0].clock = altered.samplingBindings[0].reset;
  });
  rejectsPlan("sampling local ordinal", [](auto &altered) {
    ++altered.samplingBindings[0].localOrdinal;
  });
  rejectsPlan("occurrence binding", [](auto &altered) {
    ++altered.samplingOccurrences[0].bindingIndex;
  });
  rejectsPlan("occurrence owner", [](auto &altered) {
    ++altered.samplingOccurrences[0].ownerOccurrence;
  });
  rejectsPlan("occurrence ordinal",
              [](auto &altered) { ++altered.samplingOccurrences[0].ordinal; });
}

TEST_F(HardwareSampledAdmissionWiringTest,
       OrdinaryResultsRequirePackageLevelLiteralTrueTotality) {
  build(false);
  makeValuedStatefree(false);
  EXPECT_TRUE(failed(capture([&] { return verifyWiring(); }))) << diagnostics;
  EXPECT_NE(diagnostics.find("ordinary transaction results require all guards "
                             "proven literal true"),
            std::string::npos)
      << diagnostics;

  build(false);
  makeValuedStatefree(true);
  EXPECT_TRUE(succeeded(capture([&] { return verifyWiring(); })))
      << diagnostics;
}

TEST_F(HardwareSampledAdmissionWiringTest,
       ValuedTotalityFollowsPublicOrdinaryChildRoutesOnlyForKnownTrue) {
  for (unsigned kind = 0; kind < 4; ++kind) {
    SCOPED_TRACE(kind);
    build(false);
    Value exported;
    if (kind == 0)
      exported = one;
    else if (kind == 1)
      exported = zero;
    else if (kind == 2)
      exported = definition.getBody().front().getArgument(0);
    else {
      b.setInsertionPoint(definition.getBody().front().getTerminator());
      exported = op(ac::BitsCompareOp::getOperationName(),
                    {definition.getBody().front().getArgument(0), zero},
                    {bit()}, {f("predicate", b.getStringAttr("eq"))})
                     ->getResult(0);
    }
    exportValueThroughParent(exported, false);
    auto child = *proofRoot.getBody().front().getOps<ac::InstanceOp>().begin();
    addValuedParentGuardedBy(child.getResult(0));
    auto result = capture([&] { return verifyWiring(); });
    if (kind == 0) {
      EXPECT_TRUE(succeeded(result)) << diagnostics;
    } else {
      EXPECT_TRUE(failed(result)) << diagnostics;
      EXPECT_NE(diagnostics.find("ordinary transaction results require all "
                                 "guards proven literal true"),
                std::string::npos)
          << diagnostics;
    }
  }
}

TEST_F(HardwareSampledAdmissionWiringTest,
       ActualInstanceScopesDoNotShareValuedGuardProofsForOneChildSsa) {
  for (bool dynamicRight : {false, true}) {
    SCOPED_TRACE(dynamicRight);
    build(false);
    exportValueThroughParent(definition.getBody().front().getArgument(0),
                             false);
    auto &parentBody = proofRoot.getBody().front();
    auto left = *parentBody.getOps<ac::InstanceOp>().begin();
    b.setInsertionPoint(left);
    auto parentOne = constant(1, "Parent");
    auto parentZero = constant(0, "Parent");
    left->setAttr("instance_name", b.getStringAttr("left"));
    left->setOperand(0, parentOne);
    addValuedParentGuardedBy(left.getResult(0), 0);
    ASSERT_TRUE(succeeded(capture([&] { return verifyWiring(); })))
        << diagnostics;

    auto *copy = left->clone();
    copy->setAttr("instance_name", b.getStringAttr("right"));
    copy->setAttr("occurrence", occurrence(32, "Parent"));
    copy->setOperand(0, dynamicRight ? parentBody.getArgument(0) : parentZero);
    parentBody.getOperations().insert(parentBody.getTerminator()->getIterator(),
                                      copy);
    auto right = cast<ac::InstanceOp>(copy);
    addValuedParentGuardedBy(right.getResult(0), 1);
    EXPECT_TRUE(failed(capture([&] { return verifyWiring(); }))) << diagnostics;
    EXPECT_NE(diagnostics.find("ordinary transaction results require all "
                               "guards proven literal true"),
              std::string::npos)
        << diagnostics;
  }
}

TEST_F(HardwareSampledAdmissionWiringTest,
       KnownTruePrivateCandidateCannotLaunderValuedTotality) {
  build(true);
  llvm::SmallVector<Value> captures(rule.getOperands());
  captures.push_back(one);
  rule->setOperands(captures);
  rule.getBody().front().addArgument(one.getType(), loc());
  rule.getBody().front().getTerminator()->setOperand(
      0, rule.getBody().front().getArguments().back());
  exportValueThroughParent(rule.getResult(0), true);
  auto child = *proofRoot.getBody().front().getOps<ac::InstanceOp>().begin();
  addValuedParentGuardedBy(child.getResult(0));
  EXPECT_TRUE(failed(capture([&] { return verifyWiring(); }))) << diagnostics;
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(HardwareSampledAdmissionWiringTest,
       PlainChildCheckSuffixCannotLaunderValuedTotality) {
  for (StringRef kind : {"assert", "range"}) {
    SCOPED_TRACE(kind.str());
    build(false);
    auto privateValue = makePlainCheckedPrivateValue(kind);
    exportValueThroughParent(privateValue, false);
    auto child = *proofRoot.getBody().front().getOps<ac::InstanceOp>().begin();
    addValuedParentGuardedBy(child.getResult(0));
    EXPECT_TRUE(failed(capture([&] { return verifyWiring(); }))) << diagnostics;
    EXPECT_FALSE(diagnostics.empty());
  }
}

TEST_F(HardwareSampledAdmissionWiringTest,
       ReusedDefinitionCreatesDistinctActualSamplingHistories) {
  build(false);
  instantiateSampledOwnerTwice();
  auto plan = capture(
      [&] { return ac::HardwareAnalysis(*package).getSourceCheckPlan(); });
  ASSERT_TRUE(succeeded(plan)) << diagnostics;
  ASSERT_EQ(plan->samplingBindings.size(), 1u);
  ASSERT_EQ(plan->samplingOccurrences.size(), 2u);
  ASSERT_EQ(plan->checks.size(), 2u);
  EXPECT_EQ(plan->samplingOccurrences[0].bindingIndex, 0u);
  EXPECT_EQ(plan->samplingOccurrences[1].bindingIndex, 0u);
  EXPECT_NE(plan->samplingOccurrences[0].ownerOccurrence,
            plan->samplingOccurrences[1].ownerOccurrence);
  EXPECT_NE(plan->samplingOccurrences[0].ordinal,
            plan->samplingOccurrences[1].ordinal);
  ASSERT_TRUE(plan->checks[0].samplingOccurrenceIndex);
  ASSERT_TRUE(plan->checks[1].samplingOccurrenceIndex);
  EXPECT_NE(*plan->checks[0].samplingOccurrenceIndex,
            *plan->checks[1].samplingOccurrenceIndex);
}

TEST_F(HardwareSampledAdmissionWiringTest,
       NativeEmitterPreparationAdmitsSampledLifecycleOnlyThroughItsOwnedGate) {
  build(false);
  auto prepare = [&](bool native) {
    ac::HardwareAnalysis analysis(*package);
    acir::compiler::HardwareEmitContext emitter(*package, analysis);
    return native ? emitter.prepareNativeChecks() : emitter.prepare();
  };
  EXPECT_TRUE(failed(capture([&] { return prepare(false); })));
  EXPECT_NE(diagnostics.find("hardware instrumentation emission is not "
                             "implemented"),
            std::string::npos)
      << diagnostics;
  EXPECT_TRUE(succeeded(capture([&] { return prepare(true); }))) << diagnostics;
}

} // namespace
