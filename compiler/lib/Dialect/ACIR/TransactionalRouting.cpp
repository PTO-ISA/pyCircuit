#include "pycircuit/Dialect/ACIR/TransactionalRouting.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;
namespace acir::ac {
namespace {
using Kind = TransactionalStepKind;
constexpr StringLiteral sourceKey = "ac.transactional_source";
constexpr StringLiteral topKey = "ac.transactional_top";
constexpr StringLiteral versionKey = "ac.transactional_provenance_version";
constexpr StringLiteral stepKey = "ac.transactional_step";
constexpr StringLiteral storageName = "__pyc_transactional_dffe";

Operation *make(OpBuilder &b, Location loc, StringRef name, ValueRange operands,
                TypeRange results, ArrayRef<NamedAttribute> attrs = {}) {
  OperationState state(loc, name);
  state.addOperands(operands);
  state.addTypes(results);
  state.addAttributes(attrs);
  return b.create(state);
}
DictionaryAttr occurrence(Builder &b, StringRef top, unsigned ordinal) {
  auto index = b.getDictionaryAttr(
      {b.getNamedAttr("kind", b.getStringAttr("index")),
       b.getNamedAttr("value", b.getI64IntegerAttr(ordinal))});
  auto site = b.getDictionaryAttr(
      {b.getNamedAttr("definition",
                      FlatSymbolRefAttr::get(b.getContext(), top)),
       b.getNamedAttr("ast_path", b.getArrayAttr({index}))});
  return b.getDictionaryAttr({b.getNamedAttr("site", site),
                              b.getNamedAttr("expansion", b.getArrayAttr({}))});
}
DictionaryAttr syntheticSpan(Builder &b) {
  return b.getDictionaryAttr(
      {b.getNamedAttr("path", b.getStringAttr("__transactional__.py")),
       b.getNamedAttr("line", b.getI64IntegerAttr(1)),
       b.getNamedAttr("column", b.getI64IntegerAttr(1)),
       b.getNamedAttr("end_line", b.getI64IntegerAttr(1)),
       b.getNamedAttr("end_column", b.getI64IntegerAttr(2))});
}
BitsType predicateType(Builder &b, StringRef top) {
  auto literal = b.getDictionaryAttr(
      {b.getNamedAttr("kind", b.getStringAttr("integer")),
       b.getNamedAttr(
           "value", MathIntAttr::get(b.getContext(),
                                     llvm::APSInt(llvm::APInt(64, 1), true)))});
  return BitsType::get(
      b.getContext(),
      StaticExprAttr::get(
          b.getContext(),
          b.getDictionaryAttr(
              {b.getNamedAttr("kind", b.getStringAttr("literal")),
               b.getNamedAttr("value", literal),
               b.getNamedAttr("origin", occurrence(b, top, 0)),
               b.getNamedAttr("location", syntheticSpan(b))})));
}
DictionaryAttr sourceOccurrence(Operation *op, Builder &b, StringRef top,
                                unsigned ordinal) {
  for (StringRef key : {"occurrence", "ac.origin"})
    if (auto value = op->getAttrOfType<DictionaryAttr>(key))
      return value;
  return occurrence(b, top, ordinal);
}
DictionaryAttr checkID(const TransactionalAnalysis &p, Builder &b,
                       const TransactionalStep &step, unsigned ordinal) {
  auto top = p.getTop().getSymName();
  return b.getDictionaryAttr(
      {b.getNamedAttr("registration", occurrence(b, top, 0)),
       b.getNamedAttr("check",
                      sourceOccurrence(step.operation, b, top, ordinal + 1)),
       b.getNamedAttr("obligation", b.getI64IntegerAttr(ordinal))});
}
ArrayAttr sourceOwners(const TransactionalAnalysis &p, Builder &b) {
  if (auto owners = p.getPackage()->getAttrOfType<ArrayAttr>("ac.source_units"))
    return owners;
  SmallVector<Attribute> owners;
  for (const auto &instance : p.getInstances()) {
    auto owner = instance.definition->getAttr("source_owner");
    if (!llvm::is_contained(owners, owner))
      owners.push_back(owner);
  }
  return b.getArrayAttr(owners);
}
std::string instanceSymbol(const TransactionalAnalysis &p, unsigned id);
std::string physicalSymbol(const TransactionalAnalysis &p, unsigned id) {
  return id == 0 ? p.getTop().getSymName().str() : instanceSymbol(p, id);
}
DictionaryAttr physicalOccurrence(DictionaryAttr source, Builder &b,
                                  StringRef symbol) {
  NamedAttrList occurrence(source);
  NamedAttrList site(source.getAs<DictionaryAttr>("site"));
  site.set("definition", FlatSymbolRefAttr::get(b.getContext(), symbol));
  occurrence.set("site", site.getDictionary(b.getContext()));
  return occurrence.getDictionary(b.getContext());
}
DictionaryAttr storageOccurrence(const TransactionalAnalysis &p, Builder &b,
                                unsigned resource) {
  auto &owner = p.getResources()[resource];
  return physicalOccurrence(
      sourceOccurrence(owner.allocation, b, p.getTop().getSymName(), resource + 1),
      b, physicalSymbol(p, owner.instance));
}
DictionaryAttr ownerRef(const TransactionalAnalysis &p, Builder &b,
                        unsigned instance) {
  SmallVector<Attribute> path;
  for (unsigned id = instance; id != 0; id = p.getInstances()[id].parent)
    path.push_back(physicalOccurrence(
        sourceOccurrence(p.getInstances()[id].declaration, b,
                         p.getTop().getSymName(), id),
        b, physicalSymbol(p, p.getInstances()[id].parent)));
  std::reverse(path.begin(), path.end());
  return b.getDictionaryAttr(
      {b.getNamedAttr("instance_path", b.getArrayAttr(path))});
}
DictionaryAttr observationID(const TransactionalAnalysis &p, Builder &b,
                             const TransactionalStep &step, unsigned ordinal) {
  auto site = physicalOccurrence(
      sourceOccurrence(step.operation, b, p.getTop().getSymName(), ordinal), b,
      physicalSymbol(p, step.instance));
  auto registration = p.getRules()[step.rule].registration;
  SmallVector<Attribute> invocation;
  unsigned caller = 0;
  for (auto frame : step.invocation) {
    invocation.push_back(b.getDictionaryAttr(
        {b.getNamedAttr("call", physicalOccurrence(
                                    sourceOccurrence(frame.call, b,
                                                     p.getTop().getSymName(),
                                                     ordinal),
                                    b, physicalSymbol(p, caller))),
         b.getNamedAttr("receiver", ownerRef(p, b, frame.instance))}));
    caller = frame.instance;
  }
  return b.getDictionaryAttr(
      {b.getNamedAttr("registration", physicalOccurrence(
                                            sourceOccurrence(registration, b,
                                                             p.getTop().getSymName(),
                                                             ordinal),
                                            b, p.getTop().getSymName())),
       b.getNamedAttr("site", site),
       b.getNamedAttr("owner", ownerRef(p, b, step.instance)),
       b.getNamedAttr("ordinal", b.getI64IntegerAttr(ordinal)),
       b.getNamedAttr("invocation", b.getArrayAttr(invocation))});
}
SmallVector<unsigned> subtreeResources(const TransactionalAnalysis &p,
                                       unsigned instance) {
  SmallVector<unsigned> resources(p.getInstances()[instance].resources);
  for (unsigned child : p.getInstances()[instance].children)
    llvm::append_range(resources, subtreeResources(p, child));
  return resources;
}
std::string instanceSymbol(const TransactionalAnalysis &p, unsigned id) {
  return p.getTop().getSymName().str() + "__pyc_instance_" + std::to_string(id);
}
SmallVector<NamedAttribute> envelopeAttributes(const TransactionalAnalysis &p,
                                             Builder &b, unsigned id,
                                             BitsType bit) {
  SmallVector<Type> inputs{bit, bit}, outputs;
  SmallVector<std::string> inputNames{"pyc_clk", "pyc_rst"}, outputNames;
  for (unsigned resource : subtreeResources(p, id)) {
    auto type = p.getResources()[resource].type.getPayload();
    inputs.append({type, bit});
    inputNames.push_back("d_" + std::to_string(resource));
    inputNames.push_back("en_" + std::to_string(resource));
    outputs.push_back(type);
    outputNames.push_back("q_" + std::to_string(resource));
  }
  SmallVector<Attribute> inputStrings, outputStrings;
  for (auto &name : inputNames)
    inputStrings.push_back(b.getStringAttr(name));
  for (auto &name : outputNames)
    outputStrings.push_back(b.getStringAttr(name));
  SmallVector<NamedAttribute> attrs{
      b.getNamedAttr("sym_name", b.getStringAttr(instanceSymbol(p, id))),
      b.getNamedAttr("source_owner",
                     p.getInstances()[id].definition->getAttr("source_owner")),
      b.getNamedAttr("parameters", b.getArrayAttr({})),
      b.getNamedAttr("type_parameters", b.getArrayAttr({})),
      b.getNamedAttr("function_type",
                     TypeAttr::get(b.getFunctionType(inputs, outputs))),
      b.getNamedAttr("input_names", b.getArrayAttr(inputStrings)),
      b.getNamedAttr("output_names", b.getArrayAttr(outputStrings)),
      b.getNamedAttr("ac.domain_inputs",
                     b.getDictionaryAttr(
                         {b.getNamedAttr("clock", b.getI64IntegerAttr(0)),
                          b.getNamedAttr("reset", b.getI64IntegerAttr(1))}))};
  if (inputs.size() > 2) {
    SmallVector<Attribute> defaults(inputs.size() - 2, b.getI64IntegerAttr(0));
    attrs.push_back(b.getNamedAttr(
        "ac.internal_ports",
        b.getDictionaryAttr(
            {b.getNamedAttr("source_inputs", b.getI64IntegerAttr(2)),
             b.getNamedAttr("source_outputs", b.getI64IntegerAttr(0)),
             b.getNamedAttr("root_inputs", b.getArrayAttr(defaults))})));
  }
  return attrs;
}
SmallVector<NamedAttribute> childAttributes(const TransactionalAnalysis &p,
                                          Builder &b, unsigned id) {
  auto source = p.getInstances()[id].declaration;
  return {
      b.getNamedAttr("instance_name", source.getInstanceNameAttr()),
      b.getNamedAttr("occurrence", physicalOccurrence(
                                       source.getOccurrence(), b,
                                       physicalSymbol(p, p.getInstances()[id].parent))),
      b.getNamedAttr("callee", FlatSymbolRefAttr::get(
                                   b.getContext(), instanceSymbol(p, id))),
      b.getNamedAttr("parameters", b.getArrayAttr({})),
      b.getNamedAttr("type_arguments", b.getArrayAttr({}))};
}
struct Observation {
  const TransactionalStep *step;
  Value path;
  SmallVector<Value> payloads;
};
struct Check {
  const TransactionalStep *step;
  Value condition, path;
};

class Router {
public:
  Router(const TransactionalAnalysis &plan, TransactionalDebit debit)
      : p(plan), debit(std::move(debit)), b(plan.getPackage().getContext()),
        loc(plan.getPackage().getLoc()), top(plan.getTop().getSymName()),
        bit(predicateType(b, top)) {}
  FailureOr<OwningOpRef<mlir::ModuleOp>> run();

private:
  const TransactionalAnalysis &p;
  TransactionalDebit debit;
  OpBuilder b;
  Location loc;
  StringRef top;
  BitsType bit;
  SmallVector<Value> values, q, data, enables;
  SmallVector<Check> checks;
  SmallVector<Observation> observations;
  Value zero, one;
  LogicalResult preserveHierarchy(mlir::ModuleOp package, ModuleOp root);
  Value literal(bool value) {
    return make(b, loc, BitsConstant4Op::getOperationName(), {}, TypeRange{bit},
                {b.getNamedAttr("pattern", b.getStringAttr(value ? "1" : "0"))})
        ->getResult(0);
  }
  Value binary(StringRef code, Value lhs, Value rhs) {
    return make(b, loc, BitsBinaryOp::getOperationName(), {lhs, rhs},
                TypeRange{bit},
                {b.getNamedAttr("opcode", b.getStringAttr(code))})
        ->getResult(0);
  }
  void sequence(const TransactionalSequence &seq, Value guard,
                bool effects = false) {
    for (const auto &owned : seq.steps) {
      const auto &step = *owned;
      bool payload = step.kind == Kind::Pure || step.kind == Kind::StateRead;
      if (payload == effects)
        continue;
      if (step.kind == Kind::Pure) {
        IRMapping map;
        for (auto [operand, id] :
             llvm::zip(step.operation->getOperands(), step.operands))
          map.map(operand, values[id]);
        auto *clone = b.clone(*step.operation, map);
        clone->setAttr(stepKey, b.getI64IntegerAttr(step.id));
        for (auto [id, result] : llvm::zip(step.results, clone->getResults()))
          values[id] = result;
      } else if (step.kind == Kind::StateRead) {
        values[step.results.front()] = q[step.resource];
      } else if (step.kind == Kind::StateWrite) {
        auto id = step.resource;
        data[id] = make(b, loc, BitsSelectOp::getOperationName(),
                        {guard, values[step.operands.front()], data[id]},
                        TypeRange{data[id].getType()})
                       ->getResult(0);
        enables[id] = binary("or", enables[id], guard);
      } else if (step.kind == Kind::Observe) {
        Observation observation{
            &step, binary("and", guard, values[step.activation]), {}};
        for (auto id : step.operands)
          observation.payloads.push_back(values[id]);
        observations.push_back(std::move(observation));
      } else {
        checks.push_back({&step, values[step.operands.front()],
                          binary("and", guard, values[step.activation])});
      }
    }
  }
};

FailureOr<OwningOpRef<mlir::ModuleOp>> Router::run() {
  if (failed(debit(256 + p.getValues().size() * 32, p.getPackage())))
    return failure();
  auto package = OwningOpRef<mlir::ModuleOp>(mlir::ModuleOp::create(loc));
  std::string snapshot;
  llvm::raw_string_ostream stream(snapshot);
  p.getPackage().print(stream);
  if (failed(debit(snapshot.size(), p.getPackage())))
    return failure();
  (*package)->setAttr(sourceKey, b.getStringAttr(snapshot));
  (*package)->setAttr(topKey, b.getStringAttr(top));
  (*package)->setAttr(versionKey, b.getI64IntegerAttr(1));
  (*package)->setAttr("ac.stage", b.getStringAttr("routed"));
  (*package)->setAttr("ac.routed_contract", b.getI64IntegerAttr(4));
  auto owner = p.getTop()->getAttrOfType<DictionaryAttr>("source_owner");
  (*package)->setAttr("ac.source_units", sourceOwners(p, b));
  b.setInsertionPointToEnd(package->getBody());
  auto param = TypeParamType::get(
      b.getContext(), FlatSymbolRefAttr::get(b.getContext(), storageName),
      b.getStringAttr("T"));
  auto endpoint =
      b.getDictionaryAttr({b.getNamedAttr("port", b.getI64IntegerAttr(0)),
                           b.getNamedAttr("path", b.getArrayAttr({}))});
  auto dependency =
      b.getDictionaryAttr({b.getNamedAttr("output", endpoint),
                           b.getNamedAttr("inputs", b.getArrayAttr({}))});
  make(b, loc, ModuleImportOp::getOperationName(), {}, {},
       {b.getNamedAttr("sym_name", b.getStringAttr(storageName)),
        b.getNamedAttr("source_owner", owner),
        b.getNamedAttr("semantic_kind", b.getStringAttr("hardware")),
        b.getNamedAttr("primitive_kind", b.getStringAttr("dffe")),
        b.getNamedAttr("parameters", b.getArrayAttr({})),
        b.getNamedAttr("type_parameters",
                       b.getArrayAttr({b.getStringAttr("T")})),
        b.getNamedAttr("function_type",
                       TypeAttr::get(b.getFunctionType(
                           {bit, bit, bit, param, param}, {param}))),
        b.getNamedAttr("input_names",
                       b.getStrArrayAttr({"clk", "rst", "en", "d", "init"})),
        b.getNamedAttr("output_names", b.getStrArrayAttr({"q"})),
        b.getNamedAttr("dependency_summary", b.getArrayAttr({dependency}))});
  SmallVector<Type> outputs;
  for (unsigned id : p.getExports())
    outputs.push_back(p.getResources()[id].type.getPayload());
  OperationState moduleState(loc, ModuleOp::getOperationName());
  moduleState.addAttributes(
      {b.getNamedAttr("sym_name", b.getStringAttr(top)),
       b.getNamedAttr("source_owner", owner),
       b.getNamedAttr("parameters", b.getArrayAttr({})),
       b.getNamedAttr("type_parameters", b.getArrayAttr({})),
       b.getNamedAttr("function_type",
                      TypeAttr::get(b.getFunctionType({bit, bit}, outputs))),
       b.getNamedAttr("input_names", b.getStrArrayAttr({"pyc_clk", "pyc_rst"})),
       b.getNamedAttr("output_names", p.getTop().getOutputNames()),
       b.getNamedAttr("ac.domain_inputs",
                      b.getDictionaryAttr(
                          {b.getNamedAttr("clock", b.getI64IntegerAttr(0)),
                           b.getNamedAttr("reset", b.getI64IntegerAttr(1))}))});
  moduleState.addRegion()->push_back(new Block());
  auto module = cast<ModuleOp>(b.create(moduleState));
  for (StringRef key : {"ac.root_kind", "ac.origin"})
    if (auto attr = p.getTop()->getAttr(key))
      module->setAttr(key, attr);
  auto &body = module.getBody().front();
  body.addArgument(bit, loc);
  body.addArgument(bit, loc);
  b.setInsertionPointToEnd(&body);
  values.resize(p.getValues().size());
  for (auto [id, resource] : llvm::enumerate(p.getResources())) {
    q.push_back(
        make(b, loc, SourceStateOp::getOperationName(),
             {body.getArgument(0), body.getArgument(1)},
             TypeRange{resource.type.getPayload()},
             {b.getNamedAttr("instance_name",
                             resource.allocation->getAttr("instance_name")),
              b.getNamedAttr("occurrence", sourceOccurrence(resource.allocation,
                                                            b, top, id + 1))})
            ->getResult(0));
  }
  for (const auto &owner : p.getInstances())
    sequence(owner.comb, {});
  for (auto [id, resource] : llvm::enumerate(p.getResources()))
    q[id].getDefiningOp()->insertOperands(2, values[resource.initial]);
  SmallVector<Value> captures(q);
  SmallVector<unsigned> captured;
  for (auto [id, value] : llvm::enumerate(values))
    if (value) {
      captures.push_back(value);
      captured.push_back(id);
    }
  OperationState combState(loc, RuleOp::getOperationName());
  combState.addOperands(captures);
  combState.addAttributes(
      {b.getNamedAttr("name", b.getStringAttr("transactions")),
       b.getNamedAttr("occurrence", occurrence(b, top, 0))});
  combState.addRegion()->push_back(new Block());
  // Result types are assigned once proposals and check suffix are known.
  auto *comb = b.create(combState);
  auto &work = comb->getRegion(0).front();
  for (Value value : captures)
    work.addArgument(value.getType(), loc);
  auto outerQ = q;
  for (unsigned i = 0; i < q.size(); ++i)
    q[i] = work.getArgument(i);
  for (auto [i, id] : llvm::enumerate(captured))
    values[id] = work.getArgument(q.size() + i);
  b.setInsertionPointToEnd(&work);
  zero = literal(false);
  one = literal(true);
  data = q;
  enables.assign(q.size(), zero);
  for (const auto &rule : p.getRules()) {
    sequence(rule.guard, one);
    sequence(rule.body, one);
    Value guard =
        rule.guard.yields.empty() ? one : values[rule.guard.yields.front()];
    for (auto term : rule.path) {
      Value condition = values[term.value];
      if (!term.positive)
        condition = binary("xor", condition, one);
      guard = binary("and", guard, condition);
    }
    for (unsigned requirement : rule.requirements)
      guard = binary("and", guard, values[requirement]);
    sequence(rule.body, guard, true);
  }
  SmallVector<Value> yields;
  for (unsigned i = 0; i < q.size(); ++i) {
    yields.push_back(data[i]);
    yields.push_back(enables[i]);
  }
  for (const auto &observation : observations) {
    yields.push_back(observation.path);
    llvm::append_range(yields, observation.payloads);
  }
  unsigned observationCount = yields.size() - 2 * q.size();
  SmallVector<Attribute> required;
  for (auto [id, check] : llvm::enumerate(checks)) {
    yields.push_back(check.condition);
    yields.push_back(check.path);
    required.push_back(b.getDictionaryAttr(
        {b.getNamedAttr("id", checkID(p, b, *check.step, id)),
         b.getNamedAttr("kind", b.getStringAttr("assert")),
         b.getNamedAttr("location",
                        check.step->operation->getAttr("location"))}));
  }
  make(b, loc, YieldOp::getOperationName(), yields, {});
  // MLIR operations cannot grow their result list: replace the provisional op.
  b.setInsertionPoint(comb);
  OperationState complete(loc, RuleOp::getOperationName());
  complete.addOperands(captures);
  for (Value value : yields)
    complete.addTypes(value.getType());
  complete.addAttributes(comb->getAttrs());
  complete.addAttribute("ac.required_checks", b.getArrayAttr(required));
  complete.addRegion();
  auto *finished = b.create(complete);
  finished->getRegion(0).takeBody(comb->getRegion(0));
  comb->erase();
  b.setInsertionPointToEnd(&body);
  for (unsigned i = 0; i < q.size(); ++i)
    make(
        b, loc, StateUpdateOp::getOperationName(),
        {outerQ[i], finished->getResult(2 * i), finished->getResult(2 * i + 1)},
        {});
  for (auto [id, check] : llvm::enumerate(checks)) {
    unsigned offset = 2 * q.size() + observationCount + 2 * id;
    auto *expect = make(
        b, check.step->operation->getLoc(), SourceExpectOp::getOperationName(),
        {finished->getResult(offset), finished->getResult(offset + 1)}, {},
        {b.getNamedAttr("kind", b.getStringAttr("assert")),
         b.getNamedAttr("location", check.step->operation->getAttr("location")),
         b.getNamedAttr("ac.check_id", checkID(p, b, *check.step, id))});
    if (auto message = check.step->operation->getAttr("ac.message"))
      expect->setAttr("ac.message", message);
  }
  unsigned observationOffset = 2 * q.size();
  for (auto [id, observation] : llvm::enumerate(observations)) {
    SmallVector<Value> operands{finished->getResult(observationOffset++)};
    for (unsigned i = 0; i < observation.payloads.size(); ++i)
      operands.push_back(finished->getResult(observationOffset++));
    auto source = cast<SourceObserveOp>(observation.step->operation);
    make(b, source.getLoc(), SourceObserveOp::getOperationName(), operands, {},
         {b.getNamedAttr("kind", source.getKindAttr()),
          b.getNamedAttr("spec", source.getSpec()),
          b.getNamedAttr("ac.location", source->getAttr("ac.location")),
          b.getNamedAttr("ac.observation_id",
                         observationID(p, b, *observation.step, id))});
  }
  SmallVector<Value> exports;
  for (unsigned id : p.getExports())
    exports.push_back(outerQ[id]);
  make(b, loc, YieldOp::getOperationName(), exports, {});
  b.setInsertionPointToEnd(package->getBody());
  auto *system =
      make(b, loc, SystemOp::getOperationName(), {}, {},
           {b.getNamedAttr(
                "entry",
                b.getDictionaryAttr(
                    {b.getNamedAttr(
                         "callee", FlatSymbolRefAttr::get(b.getContext(), top)),
                     b.getNamedAttr("parameters", b.getArrayAttr({})),
                     b.getNamedAttr("type_arguments", b.getArrayAttr({}))})),
            b.getNamedAttr("domain", b.getStringAttr("default"))});
  for (auto sourceSystem : p.getPackage().getOps<SystemOp>())
    if (auto policy = sourceSystem->getAttr("ac.execution_policy"))
      system->setAttr("ac.execution_policy", policy);
  if (failed(preserveHierarchy(*package, module)))
    return failure();
  return std::move(package);
}

// Transaction queries are statically specialized in the root computation.
// Storage remains in real source-owner instances; generated proposal wires
// cross the hierarchy and share the outer transaction's final admission.
LogicalResult Router::preserveHierarchy(mlir::ModuleOp package, ModuleOp root) {
  if (p.getInstances().size() == 1)
    return success();
  SmallVector<SourceStateOp> storage(root.getBody().front().getOps<SourceStateOp>());
  SmallVector<StateUpdateOp> updates(root.getBody().front().getOps<StateUpdateOp>());
  SmallVector<ModuleOp> envelopes(p.getInstances().size());
  for (unsigned id = 1; id < p.getInstances().size(); ++id) {
    if (failed(debit(64 + subtreeResources(p, id).size() * 32,
                     p.getInstances()[id].definition)))
      return failure();
    b.setInsertionPointToEnd(package.getBody());
    OperationState state(loc, ModuleOp::getOperationName());
    state.addAttributes(envelopeAttributes(p, b, id, bit));
    state.addRegion()->push_back(new Block());
    envelopes[id] = cast<ModuleOp>(b.create(state));
    auto &body = envelopes[id].getBody().front();
    for (Type type : envelopes[id].getFunctionType().getInputs())
      body.addArgument(type, loc);
  }
  for (unsigned id = 0; id < p.getInstances().size(); ++id) {
    auto module = id == 0 ? root : envelopes[id];
    auto &body = module.getBody().front();
    b.setInsertionPointToEnd(&body);
    if (id == 0)
      b.setInsertionPoint(body.getTerminator());
    auto resources = subtreeResources(p, id);
    llvm::DenseMap<unsigned, unsigned> slots;
    for (auto [slot, resource] : llvm::enumerate(resources))
      slots[resource] = slot;
    llvm::DenseMap<unsigned, Value> endpoints;
    IRMapping initialValues;
    std::function<Value(Value)> cloneInitial = [&](Value value) -> Value {
      if (auto mapped = initialValues.lookupOrNull(value))
        return mapped;
      auto *source = value.getDefiningOp();
      for (Value operand : source->getOperands())
        cloneInitial(operand);
      auto *clone = b.clone(*source, initialValues);
      clone->removeAttr(stepKey);
      for (auto [original, copied] : llvm::zip(source->getResults(), clone->getResults()))
        initialValues.map(original, copied);
      return initialValues.lookup(value);
    };
    for (unsigned resource : p.getInstances()[id].resources) {
      if (id == 0)
        continue;
      unsigned port = 2 + 2 * slots[resource];
      auto original = storage[resource];
      auto *local = make(
          b, original.getLoc(), SourceStateOp::getOperationName(),
          {body.getArgument(0), body.getArgument(1),
           cloneInitial(original->getOperand(2))},
          original->getResultTypes(), original->getAttrs());
      local->setAttr("occurrence", storageOccurrence(p, b, resource));
      endpoints[resource] = local->getResult(0);
      make(b, loc, StateUpdateOp::getOperationName(),
           {local->getResult(0), body.getArgument(port),
            body.getArgument(port + 1)}, {});
    }
    for (unsigned child : p.getInstances()[id].children) {
      SmallVector<Value> operands{body.getArgument(0), body.getArgument(1)};
      SmallVector<Type> results;
      auto childResources = subtreeResources(p, child);
      for (unsigned resource : childResources) {
        if (id == 0) {
          operands.append({updates[resource].getCandidate(),
                           updates[resource].getIntent()});
        } else {
          unsigned port = 2 + 2 * slots[resource];
          operands.append({body.getArgument(port), body.getArgument(port + 1)});
        }
        results.push_back(p.getResources()[resource].type.getPayload());
      }
      auto *instance = make(b, loc, InstanceOp::getOperationName(), operands,
                            results, childAttributes(p, b, child));
      for (auto [resource, result] : llvm::zip(childResources, instance->getResults())) {
        if (id == 0)
          storage[resource]->getResult(0).replaceAllUsesWith(result);
        else
          endpoints[resource] = result;
      }
    }
    if (id != 0) {
      SmallVector<Value> outputs;
      for (unsigned resource : resources)
        outputs.push_back(endpoints[resource]);
      make(b, loc, YieldOp::getOperationName(), outputs, {});
    }
  }
  for (auto [resource, source] : llvm::enumerate(storage))
    if (p.getResources()[resource].instance != 0) {
      updates[resource].erase();
      source.erase();
    }
  return success();
}

// Replay does not call Router or construct an expected hardware graph. It binds
// source values to actual graph endpoints, checks every source operation and
// proposal edge, and accounts for every operation in both executable regions.
class Replay {
public:
  Replay(const TransactionalAnalysis &plan, mlir::ModuleOp candidate,
         TransactionalDebit debit)
      : p(plan), package(candidate), debit(std::move(debit)),
        b(candidate.getContext()), top(plan.getTop().getSymName()),
        bit(predicateType(b, top)) {}
  LogicalResult run();

private:
  struct Write {
    Value guard, payload;
  };
  const TransactionalAnalysis &p;
  mlir::ModuleOp package;
  TransactionalDebit debit;
  Builder b;
  StringRef top;
  BitsType bit;
  ModuleOp module;
  RuleOp comb;
  SmallVector<Value> values, q, initials, proposalData, proposalEnable;
  SmallVector<ModuleOp> ownerModules;
  LogicalResult inspectOwner(unsigned id, ModuleOp owner,
                             ArrayRef<Value> arguments,
                             ArrayRef<Value> exposed);
  LogicalResult initializer(Value actual, Value expected, Block *owner);
  SmallVector<SmallVector<Write>> writes;
  SmallVector<std::pair<const TransactionalStep *, Value>> assertions,
      observations;
  llvm::DenseMap<unsigned, Operation *> pure;
  llvm::DenseSet<std::pair<Value, Value>> initializers;
  llvm::DenseSet<Operation *> claimed;
  Value zero, one;
  bool final = false;
  LogicalResult reject(StringRef reason) {
    return package.emitError()
           << "transactional source correspondence: " << reason;
  }
  LogicalResult claim(Operation *op) {
    if (!op ||
        failed(debit(8 + op->getNumOperands() + op->getNumResults(), op)))
      return failure();
    claimed.insert(op);
    return success();
  }
  bool attrs(Operation *op, ArrayRef<NamedAttribute> expected) {
    NamedAttrList fields(expected);
    if (isa<ModuleOp>(op))
      for (StringRef key : {"ac.root_kind", "ac.origin"})
        if (auto attr = p.getTop()->getAttr(key))
          fields.set(key, attr);
    if (isa<SystemOp>(op))
      for (auto sourceSystem : p.getPackage().getOps<SystemOp>())
        if (auto policy = sourceSystem->getAttr("ac.execution_policy"))
          fields.set("ac.execution_policy", policy);
    return op->getAttrDictionary() == fields.getDictionary(b.getContext());
  }
  bool literal(Value value, StringRef pattern) {
    auto op = value.getDefiningOp<BitsConstant4Op>();
    return op && op.getResult().getType() == bit &&
           op.getPattern() == pattern &&
           attrs(op, {b.getNamedAttr("pattern", b.getStringAttr(pattern))}) &&
           succeeded(claim(op));
  }
  bool binary(Value value, StringRef opcode, Value lhs, Value rhs) {
    auto op = value.getDefiningOp<BitsBinaryOp>();
    return op && op->getNumOperands() == 2 && op->getOperand(0) == lhs &&
           op->getOperand(1) == rhs && value.getType() == bit &&
           attrs(op, {b.getNamedAttr("opcode", b.getStringAttr(opcode))}) &&
           succeeded(claim(op));
  }
  LogicalResult sequence(const TransactionalSequence &seq, Value guard,
                         Block *block, bool effects = false) {
    for (const auto &owned : seq.steps) {
      const auto &step = *owned;
      bool payload = step.kind == Kind::Pure || step.kind == Kind::StateRead;
      if (payload == effects)
        continue;
      if (failed(debit(8 + step.operands.size(), step.operation)))
        return failure();
      if (step.kind == Kind::Pure) {
        auto found = pure.find(step.id);
        if (found == pure.end())
          return reject("missing pure source operation");
        auto *op = found->second;
        NamedAttrList expected(step.operation->getAttrs());
        expected.set(stepKey, b.getI64IntegerAttr(step.id));
        if (op->getBlock() != block ||
            op->getName() != step.operation->getName() ||
            op->getAttrDictionary() != expected.getDictionary(b.getContext()) ||
            op->getResultTypes() != step.operation->getResultTypes() ||
            op->getNumOperands() != step.operands.size() || op->getNumRegions())
          return reject("pure source operation differs");
        for (auto [actual, id] : llvm::zip(op->getOperands(), step.operands))
          if (actual != values[id])
            return reject(
                "pure source operand is not its old-state/source value");
        for (auto [id, actual] : llvm::zip(step.results, op->getResults()))
          values[id] = actual;
        if (failed(claim(op)))
          return failure();
      } else if (step.kind == Kind::StateRead)
        values[step.results.front()] = q[step.resource];
      else if (step.kind == Kind::StateWrite)
        writes[step.resource].push_back({guard, values[step.operands.front()]});
      else if (step.kind == Kind::Observe)
        observations.push_back({&step, guard});
      else
        assertions.push_back({&step, guard});
    }
    return success();
  }
};

LogicalResult Replay::initializer(Value actual, Value expected, Block *owner) {
  if (initializers.contains({actual, expected}))
    return success();
  auto *op = actual.getDefiningOp();
  auto *source = expected.getDefiningOp();
  if (!op || !source || op->getBlock() != owner || op->getNumRegions() ||
      op->getName() != source->getName() ||
      op->getResultTypes() != source->getResultTypes() ||
      op->getNumOperands() != source->getNumOperands() ||
      cast<OpResult>(actual).getResultNumber() != cast<OpResult>(expected).getResultNumber())
    return reject("owner-local State initializer DAG differs");
  NamedAttrList attributes(source->getAttrs());
  attributes.erase(stepKey);
  if (op->getAttrDictionary() != attributes.getDictionary(b.getContext()))
    return reject("owner-local State initializer attributes differ");
  for (auto [operand, original] : llvm::zip(op->getOperands(), source->getOperands()))
    if (failed(initializer(operand, original, owner)))
      return failure();
  initializers.insert({actual, expected});
  return claim(op);
}

LogicalResult Replay::inspectOwner(unsigned id, ModuleOp owner,
                                    ArrayRef<Value> arguments,
                                    ArrayRef<Value> exposed) {
  ownerModules[id] = owner;
  if (id != 0 && owner->getAttrDictionary() !=
                     NamedAttrList(envelopeAttributes(p, b, id, bit))
                         .getDictionary(b.getContext()))
    return reject("storage-owner Module signature/identity differs");
  if (!owner.getBody().hasOneBlock())
    return reject("storage-owner body differs");
  auto &body = owner.getBody().front();
  if (body.getNumArguments() != arguments.size())
    return reject("storage-owner input binding differs");
  auto resources = subtreeResources(p, id);
  llvm::DenseMap<unsigned, unsigned> slots;
  for (auto [slot, resource] : llvm::enumerate(resources))
    slots[resource] = slot;
  SmallVector<Operation *> storage;
  SmallVector<StateUpdateOp> updates;
  SmallVector<InstanceOp> children;
  for (Operation &op : body) {
    if (isa<SourceStateOp>(op))
      storage.push_back(&op);
    if (auto instance = dyn_cast<InstanceOp>(op)) {
      if (instance.getCallee() == storageName)
        storage.push_back(&op);
      else
        children.push_back(instance);
    }
    if (auto update = dyn_cast<StateUpdateOp>(op))
      updates.push_back(update);
  }
  const auto &source = p.getInstances()[id];
  if (storage.size() != source.resources.size() ||
      updates.size() != (final ? 0 : storage.size()) ||
      children.size() != source.children.size())
    return reject("storage-owner executable inventory differs");
  llvm::DenseMap<unsigned, Value> localQ;
  for (auto [index, resourceID] : llvm::enumerate(source.resources)) {
    auto *op = storage[index];
    const auto &resource = p.getResources()[resourceID];
    if (op->getNumResults() != 1 ||
        op->getResult(0).getType() != resource.type.getPayload())
      return reject("State payload differs");
    NamedAttrList expected(
        {b.getNamedAttr("instance_name",
                        resource.allocation->getAttr("instance_name")),
         b.getNamedAttr("occurrence", storageOccurrence(p, b, resourceID))});
    if (final) {
      expected.append("callee", FlatSymbolRefAttr::get(b.getContext(), storageName));
      expected.append("parameters", b.getArrayAttr({}));
      expected.append("type_arguments",
                      b.getArrayAttr({TypeAttr::get(resource.type.getPayload())}));
    }
    if ((final ? !isa<InstanceOp>(op) : !isa<SourceStateOp>(op)) ||
        op->getNumOperands() != (final ? 5 : 3) ||
        op->getAttrDictionary() != expected.getDictionary(b.getContext()) ||
        op->getOperand(0) != body.getArgument(0) ||
        op->getOperand(1) != body.getArgument(1))
      return reject("owner-local State identity/clock/reset differs");
    Value data, enable, initial = op->getOperand(final ? 4 : 2);
    if (final) {
      enable = op->getOperand(2);
      data = op->getOperand(3);
    } else {
      auto update = updates[index];
      if (update->getNumOperands() != 3 || !update->getAttrs().empty() ||
          update.getState() != op->getResult(0))
        return reject("owner-local State update differs");
      data = update.getCandidate();
      enable = update.getIntent();
      if (failed(claim(update)))
        return failure();
    }
    localQ[resourceID] = op->getResult(0);
    if (id == 0) {
      q[resourceID] = op->getResult(0);
      proposalData[resourceID] = data;
      proposalEnable[resourceID] = enable;
      initials[resourceID] = initial;
    } else {
      unsigned slot = slots[resourceID], port = 2 + 2 * slot;
      if (data != body.getArgument(port) || enable != body.getArgument(port + 1))
        return reject("owner-local proposal port differs");
      q[resourceID] = exposed[slot];
      proposalData[resourceID] = arguments[port];
      proposalEnable[resourceID] = arguments[port + 1];
      initials[resourceID] = initial;
    }
    if (failed(claim(op)))
      return failure();
  }
  for (auto [index, childID] : llvm::enumerate(source.children)) {
    auto child = children[index];
    if (!attrs(child, childAttributes(p, b, childID)))
      return reject("source child instance identity differs");
    ModuleOp definition;
    for (auto candidate : package.getOps<ModuleOp>())
      if (candidate.getSymName() == instanceSymbol(p, childID))
        definition = candidate;
    auto childResources = subtreeResources(p, childID);
    if (!definition || child.getNumOperands() != 2 + 2 * childResources.size() ||
        child.getNumResults() != childResources.size() ||
        child->getOperand(0) != body.getArgument(0) ||
        child->getOperand(1) != body.getArgument(1))
      return reject("source child instance interface differs");
    SmallVector<Value> childArguments{arguments[0], arguments[1]}, childExposed;
    for (auto [slot, resource] : llvm::enumerate(childResources)) {
      unsigned port = 2 + 2 * slot;
      if (child->getResult(slot).getType() != p.getResources()[resource].type.getPayload())
        return reject("child committed-Q result type differs");
      localQ[resource] = child->getResult(slot);
      if (id == 0) {
        childArguments.append({child->getOperand(port), child->getOperand(port + 1)});
        childExposed.push_back(child->getResult(slot));
      } else {
        unsigned parentSlot = slots[resource], parentPort = 2 + 2 * parentSlot;
        for (unsigned field = 0; field < 2; ++field) {
          if (child->getOperand(port + field) != body.getArgument(parentPort + field))
            return reject("proposal forwarding edge differs");
          childArguments.push_back(arguments[parentPort + field]);
        }
        childExposed.push_back(exposed[parentSlot]);
      }
    }
    if (failed(claim(child)) ||
        failed(inspectOwner(childID, definition, childArguments, childExposed)))
      return failure();
  }
  if (id != 0) {
    auto yield = dyn_cast<YieldOp>(body.getTerminator());
    if (!yield || yield.getValues().size() != resources.size())
      return reject("storage-owner committed-Q export inventory differs");
    for (auto [slot, resource] : llvm::enumerate(resources))
      if (yield.getValues()[slot] != localQ[resource])
        return reject("storage-owner committed-Q forwarding edge differs");
    if (failed(claim(yield)))
      return failure();
  }
  return success();
}

LogicalResult Replay::run() {
  auto stage = package->getAttrOfType<StringAttr>("ac.stage");
  final = stage && stage.getValue() == "final";
  if (!stage || (!final && stage.getValue() != "routed"))
    return reject("expected routed or final stage");
  auto modules = package.getOps<ModuleOp>();
  auto imports = package.getOps<ModuleImportOp>();
  auto systems = package.getOps<SystemOp>();
  if (std::distance(modules.begin(), modules.end()) != p.getInstances().size() ||
      !llvm::hasSingleElement(imports) ||
      !llvm::hasSingleElement(systems))
    return reject("source instance/storage/System inventory differs");
  for (auto candidate : modules)
    if (candidate.getSymName() == top)
      module = candidate;
  if (!module)
    return reject("missing selected root Module");
  auto imported = *imports.begin();
  auto system = *systems.begin();
  auto owner = p.getTop()->getAttrOfType<DictionaryAttr>("source_owner");
  SmallVector<Type> outputTypes;
  for (unsigned id : p.getExports())
    outputTypes.push_back(p.getResources()[id].type.getPayload());
  auto signature = b.getFunctionType({bit, bit}, outputTypes);
  if (!attrs(module,
             {b.getNamedAttr("sym_name", b.getStringAttr(top)),
              b.getNamedAttr("source_owner", owner),
              b.getNamedAttr("parameters", b.getArrayAttr({})),
              b.getNamedAttr("type_parameters", b.getArrayAttr({})),
              b.getNamedAttr("function_type", TypeAttr::get(signature)),
              b.getNamedAttr("input_names",
                             b.getStrArrayAttr({"pyc_clk", "pyc_rst"})),
              b.getNamedAttr("output_names", p.getTop().getOutputNames()),
              b.getNamedAttr(
                  "ac.domain_inputs",
                  b.getDictionaryAttr(
                      {b.getNamedAttr("clock", b.getI64IntegerAttr(0)),
                       b.getNamedAttr("reset", b.getI64IntegerAttr(1))}))}))
    return reject(
        "Module interface/owner differs from selected source exports");
  auto param = TypeParamType::get(
      b.getContext(), FlatSymbolRefAttr::get(b.getContext(), storageName),
      b.getStringAttr("T"));
  auto endpoint =
      b.getDictionaryAttr({b.getNamedAttr("port", b.getI64IntegerAttr(0)),
                           b.getNamedAttr("path", b.getArrayAttr({}))});
  auto dependency =
      b.getDictionaryAttr({b.getNamedAttr("output", endpoint),
                           b.getNamedAttr("inputs", b.getArrayAttr({}))});
  if (!attrs(
          imported,
          {b.getNamedAttr("sym_name", b.getStringAttr(storageName)),
           b.getNamedAttr("source_owner", owner),
           b.getNamedAttr("semantic_kind", b.getStringAttr("hardware")),
           b.getNamedAttr("primitive_kind", b.getStringAttr("dffe")),
           b.getNamedAttr("parameters", b.getArrayAttr({})),
           b.getNamedAttr("type_parameters",
                          b.getArrayAttr({b.getStringAttr("T")})),
           b.getNamedAttr("function_type",
                          TypeAttr::get(b.getFunctionType(
                              {bit, bit, bit, param, param}, {param}))),
           b.getNamedAttr("input_names",
                          b.getStrArrayAttr({"clk", "rst", "en", "d", "init"})),
           b.getNamedAttr("output_names", b.getStrArrayAttr({"q"})),
           b.getNamedAttr("dependency_summary", b.getArrayAttr({dependency}))}))
    return reject("storage declaration differs");
  if (!attrs(system,
             {b.getNamedAttr(
                  "entry",
                  b.getDictionaryAttr(
                      {b.getNamedAttr("callee", FlatSymbolRefAttr::get(
                                                    b.getContext(), top)),
                       b.getNamedAttr("parameters", b.getArrayAttr({})),
                       b.getNamedAttr("type_arguments", b.getArrayAttr({}))})),
              b.getNamedAttr("domain", b.getStringAttr("default"))}))
    return reject("System entry differs");
  if (module.getBody().empty() || !module.getBody().hasOneBlock())
    return reject("Module body differs");
  auto &body = module.getBody().front();
  if (body.getNumArguments() != 2 ||
      body.getArgumentTypes() != signature.getInputs())
    return reject("clock/reset interface differs");
  auto combs = body.getOps<RuleOp>();
  if (!llvm::hasSingleElement(combs))
    return reject("missing or duplicated transaction computation");
  comb = *combs.begin();
  if (!comb.getBody().hasOneBlock())
    return reject("transaction body differs");
  auto &work = comb.getBody().front();
  unsigned resourceCount = p.getResources().size();
  q.resize(resourceCount);
  initials.resize(resourceCount);
  proposalData.resize(resourceCount);
  proposalEnable.resize(resourceCount);
  writes.resize(resourceCount);
  values.resize(p.getValues().size());
  ownerModules.resize(p.getInstances().size());
  SmallVector<Value> rootArguments(body.getArguments());
  if (failed(inspectOwner(0, module, rootArguments, {})))
    return failure();
  auto collect = [&](Block &block) -> LogicalResult {
    for (Operation &op : block)
      if (op.hasAttr(stepKey)) {
        auto id = op.getAttrOfType<IntegerAttr>(stepKey);
        if (!id || !id.getType().isSignlessInteger(64) ||
            id.getValue().isNegative() || id.getValue().getActiveBits() > 32 ||
            !pure.try_emplace(unsigned(id.getInt()), &op).second)
          return reject("invalid/duplicate source operation locator");
      }
    return success();
  };
  if (failed(collect(body)) || failed(collect(work)))
    return failure();
  for (const auto &owner : p.getInstances())
    if (failed(sequence(owner.comb, {}, &body)))
      return failure();
  for (auto [id, resource] : llvm::enumerate(p.getResources())) {
    if (resource.instance == 0) {
      if (initials[id] != values[resource.initial])
        return reject("State initialization differs from source");
    } else if (failed(initializer(initials[id], values[resource.initial],
                                  &ownerModules[resource.instance].getBody().front())))
      return failure();
  }
  SmallVector<Value> captures(q);
  SmallVector<unsigned> captured;
  for (auto [id, value] : llvm::enumerate(values))
    if (value) {
      captures.push_back(value);
      captured.push_back(id);
    }
  if (comb->getOperands() != ValueRange(captures) ||
      work.getNumArguments() != captures.size())
    return reject("old-Q/module captures differ");
  for (auto [index, value] : llvm::enumerate(captures))
    if (work.getArgument(index).getType() != value.getType())
      return reject("capture payload differs");
  auto outerQ = q;
  for (unsigned id = 0; id < q.size(); ++id)
    q[id] = work.getArgument(id);
  for (auto [index, id] : llvm::enumerate(captured))
    values[id] = work.getArgument(q.size() + index);
  if (work.getOperations().size() < 3)
    return reject("missing control constants");
  auto it = work.begin();
  if (it->getNumResults() != 1)
    return reject("missing false constant");
  zero = it->getResult(0);
  ++it;
  if (it->getNumResults() != 1)
    return reject("missing true constant");
  one = it->getResult(0);
  if (!literal(zero, "0") || !literal(one, "1"))
    return reject("control constants differ");
  for (const auto &rule : p.getRules()) {
    if (failed(sequence(rule.guard, one, &work)) ||
        failed(sequence(rule.body, one, &work)))
      return failure();
    Value guard =
        rule.guard.yields.empty() ? one : values[rule.guard.yields.front()];
    auto findBinary = [&](StringRef opcode, Value lhs, Value rhs) -> Value {
      for (Operation &op : work) {
        if (failed(debit(4, &op)))
          return {};
        if (op.getNumResults() == 1 && !claimed.contains(&op) &&
            binary(op.getResult(0), opcode, lhs, rhs))
          return op.getResult(0);
      }
      return {};
    };
    for (auto term : rule.path) {
      Value condition = values[term.value];
      if (!term.positive)
        condition = findBinary("xor", condition, one);
      if (!condition)
        return reject("Module branch polarity differs");
      guard = findBinary("and", guard, condition);
      if (!guard)
        return reject("Module registration path differs");
    }
    for (unsigned requirement : rule.requirements) {
      guard = findBinary("and", guard, values[requirement]);
      if (!guard)
        return reject("Method admission guard differs");
    }
    if (failed(sequence(rule.body, guard, &work, true)))
      return failure();
  }
  auto yield = dyn_cast<YieldOp>(work.getTerminator());
  unsigned prefix = 2 * q.size();
  unsigned observationCount = 0;
  for (auto observation : observations)
    observationCount += 1 + observation.first->operands.size();
  if (!yield ||
      yield.getValues().size() !=
          prefix + 2 * assertions.size() + observationCount ||
      comb->getResultTypes() != yield->getOperandTypes())
    return reject("proposal/check result inventory differs");
  for (unsigned id = 0; id < q.size(); ++id) {
    Value data = yield.getValues()[2 * id],
          enable = yield.getValues()[2 * id + 1];
    for (const auto &write : llvm::reverse(writes[id])) {
      auto select = data.getDefiningOp<BitsSelectOp>();
      auto join = enable.getDefiningOp<BitsBinaryOp>();
      if (!select || select->getNumOperands() != 3 ||
          !select->getAttrs().empty() || select->getOperand(0) != write.guard ||
          select->getOperand(1) != write.payload ||
          select->getResult(0).getType() != q[id].getType() || !join ||
          !binary(enable, "or", join->getOperand(0), write.guard) ||
          failed(claim(select)))
        return reject("write proposal/guard differs");
      data = select->getOperand(2);
      enable = join->getOperand(0);
    }
    if (data != q[id] || enable != zero)
      return reject("hold base is not old Q/false");
    if (proposalEnable[id] != comb->getResult(2 * id + 1) ||
        proposalData[id] != comb->getResult(2 * id))
      return reject("owner data/enable endpoints differ");
  }
  SmallVector<Attribute> required;
  SmallVector<SourceExpectOp> expects(body.getOps<SourceExpectOp>());
  if (expects.size() != assertions.size())
    return reject("assertion inventory differs");
  for (auto [id, assertion] : llvm::enumerate(assertions)) {
    const auto &step = *assertion.first;
    unsigned offset = prefix + observationCount + 2 * id;
    if (yield.getValues()[offset] != values[step.operands.front()] ||
        !binary(yield.getValues()[offset + 1], "and", assertion.second,
                values[step.activation]))
      return reject("assertion condition/reachability differs");
    auto location = step.operation->getAttr("location");
    auto identity = checkID(p, b, step, id);
    required.push_back(
        b.getDictionaryAttr({b.getNamedAttr("id", identity),
                             b.getNamedAttr("kind", b.getStringAttr("assert")),
                             b.getNamedAttr("location", location)}));
    NamedAttrList expected({b.getNamedAttr("kind", b.getStringAttr("assert")),
                            b.getNamedAttr("location", location),
                            b.getNamedAttr("ac.check_id", identity)});
    if (auto message = step.operation->getAttr("ac.message"))
      expected.set("ac.message", message);
    auto expect = expects[id];
    if (expect->getAttrDictionary() != expected.getDictionary(b.getContext()) ||
        expect->getNumOperands() != 2 ||
        expect.getCondition() != comb->getResult(offset) ||
        expect.getPath() != comb->getResult(offset + 1))
      return reject("physical assertion differs");
    if (failed(claim(expect)))
      return failure();
  }
  SmallVector<SourceObserveOp> observed(body.getOps<SourceObserveOp>());
  if (observed.size() != observations.size())
    return reject("observation inventory differs");
  unsigned offset = prefix;
  for (auto [id, observation] : llvm::enumerate(observations)) {
    const auto &step = *observation.first;
    auto source = cast<SourceObserveOp>(step.operation);
    auto actual = observed[id];
    if (!binary(yield.getValues()[offset], "and", observation.second,
                values[step.activation]) ||
        actual.getPath() != comb->getResult(offset++))
      return reject("observation activation differs");
    if (actual.getValues().size() != step.operands.size() ||
        !attrs(actual,
               {b.getNamedAttr("kind", source.getKindAttr()),
                b.getNamedAttr("spec", source.getSpec()),
                b.getNamedAttr("ac.location", source->getAttr("ac.location")),
                b.getNamedAttr("ac.observation_id",
                               observationID(p, b, step, id))}))
      return reject("observation source identity/spec differs");
    for (auto [operand, value] : llvm::zip(step.operands, actual.getValues())) {
      if (yield.getValues()[offset] != values[operand] ||
          value != comb->getResult(offset++))
        return reject("observation payload differs");
    }
    if (failed(claim(actual)))
      return failure();
  }
  if (!attrs(comb,
             {b.getNamedAttr("name", b.getStringAttr("transactions")),
              b.getNamedAttr("occurrence", occurrence(b, top, 0)),
              b.getNamedAttr("ac.required_checks", b.getArrayAttr(required))}))
    return reject("assertion ownership inventory differs");
  auto exported = dyn_cast<YieldOp>(body.getTerminator());
  if (!exported || exported.getValues().size() != p.getExports().size())
    return reject("export inventory differs");
  for (auto [index, resource] : llvm::enumerate(p.getExports()))
    if (exported.getValues()[index] != outerQ[resource])
      return reject("export does not preserve State identity");
  for (Operation *op :
       {comb.getOperation(), yield.getOperation(), exported.getOperation()})
    if (failed(claim(op)))
      return failure();
  for (auto owner : ownerModules)
    for (Operation &op : owner.getBody().front())
      if (!claimed.contains(&op))
        return reject("unaccounted executable operation");
  for (Operation &op : work)
    if (!claimed.contains(&op))
      return reject("unaccounted transaction computation");
  for (Operation &op : package.getBody()->getOperations())
    if (!llvm::is_contained(ownerModules, &op) && &op != imported && &op != system)
      return reject("unaccounted package operation");
  if (package->getAttr("ac.source_units") != sourceOwners(p, b))
    return reject("source owner inventory differs");
  return success();
}
} // namespace

FailureOr<OwningOpRef<mlir::ModuleOp>>
routeTransactionalPackage(mlir::ModuleOp source, StringRef top,
                          TransactionalDebit debit) {
  if (!debit)
    return source.emitError(
        "transactional routing requires a verification budget");
  auto plan = TransactionalAnalysis::create(source, top, debit);
  if (failed(plan))
    return failure();
  auto candidate = Router(**plan, debit).run();
  if (failed(candidate) || failed(Replay(**plan, **candidate, debit).run()))
    return failure();
  return std::move(*candidate);
}

LogicalResult verifyTransactionalRouting(mlir::ModuleOp package,
                                         TransactionalDebit debit) {
  if (!debit)
    return package.emitError(
        "transactional replay requires a verification budget");
  auto source = package->getAttrOfType<StringAttr>(sourceKey);
  auto top = package->getAttrOfType<StringAttr>(topKey);
  auto version = package->getAttrOfType<IntegerAttr>(versionKey);
  if (!source || !top || !version || !version.getType().isSignlessInteger(64) ||
      version.getInt() != 1)
    return package.emitError("transactional Routed contract requires versioned "
                             "source provenance and top");
  if (failed(debit(source.getValue().size() + 128, package)))
    return failure();
  auto parsed = parseSourceString<mlir::ModuleOp>(
      source.getValue(), ParserConfig(package.getContext()));
  if (!parsed)
    return package.emitError("transactional source snapshot cannot be parsed");
  auto plan = TransactionalAnalysis::create(*parsed, top.getValue(), debit);
  if (failed(plan))
    return failure();
  return Replay(**plan, package, debit).run();
}
} // namespace acir::ac
