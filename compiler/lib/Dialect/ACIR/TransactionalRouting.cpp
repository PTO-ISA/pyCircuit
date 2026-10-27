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
  return b.getArrayAttr({p.getTop()->getAttr("source_owner")});
}
DictionaryAttr observationID(const TransactionalAnalysis &p, Builder &b,
                             const TransactionalStep &step, unsigned ordinal) {
  auto site = step.operation->getAttrOfType<DictionaryAttr>("occurrence");
  auto registration = p.getRules()[step.rule].registration;
  auto call = registration->getAttrOfType<DictionaryAttr>("occurrence");
  if (!call)
    call = occurrence(b, p.getTop().getSymName(), ordinal);
  return b.getDictionaryAttr(
      {b.getNamedAttr("registration", call), b.getNamedAttr("site", site),
       b.getNamedAttr("owner", b.getDictionaryAttr({b.getNamedAttr(
                                   "instance_path", b.getArrayAttr({}))})),
       b.getNamedAttr("ordinal", b.getI64IntegerAttr(ordinal)),
       b.getNamedAttr("invocation", b.getArrayAttr({}))});
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
  sequence(p.getInstances().front().comb, {});
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
  return std::move(package);
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
  SmallVector<Value> values, q, initials;
  SmallVector<SmallVector<Write>> writes;
  SmallVector<std::pair<const TransactionalStep *, Value>> assertions,
      observations;
  llvm::DenseMap<unsigned, Operation *> pure;
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

LogicalResult Replay::run() {
  auto stage = package->getAttrOfType<StringAttr>("ac.stage");
  final = stage && stage.getValue() == "final";
  if (!stage || (!final && stage.getValue() != "routed"))
    return reject("expected routed or final stage");
  auto modules = package.getOps<ModuleOp>();
  auto imports = package.getOps<ModuleImportOp>();
  auto systems = package.getOps<SystemOp>();
  if (!llvm::hasSingleElement(modules) || !llvm::hasSingleElement(imports) ||
      !llvm::hasSingleElement(systems))
    return reject("closed State graph requires one Module, storage declaration "
                  "and System");
  module = *modules.begin();
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
  SmallVector<Operation *> storage;
  SmallVector<StateUpdateOp> updates;
  for (Operation &op : body) {
    if (isa<SourceStateOp, InstanceOp>(op))
      storage.push_back(&op);
    if (auto update = dyn_cast<StateUpdateOp>(op))
      updates.push_back(update);
  }
  if (storage.size() != p.getResources().size() ||
      updates.size() != (final ? 0 : storage.size()))
    return reject("storage/update inventory differs");
  q.resize(storage.size());
  initials.resize(storage.size());
  writes.resize(storage.size());
  values.resize(p.getValues().size());
  for (auto [id, op] : llvm::enumerate(storage)) {
    const auto &resource = p.getResources()[id];
    if (op->getNumResults() != 1 ||
        op->getResult(0).getType() != resource.type.getPayload())
      return reject("State payload differs");
    if (final) {
      if (!isa<InstanceOp>(op) || op->getNumOperands() != 5 ||
          !attrs(op,
                 {b.getNamedAttr("instance_name",
                                 resource.allocation->getAttr("instance_name")),
                  b.getNamedAttr(
                      "occurrence",
                      sourceOccurrence(resource.allocation, b, top, id + 1)),
                  b.getNamedAttr("callee", FlatSymbolRefAttr::get(
                                               b.getContext(), storageName)),
                  b.getNamedAttr("parameters", b.getArrayAttr({})),
                  b.getNamedAttr("type_arguments",
                                 b.getArrayAttr({TypeAttr::get(
                                     resource.type.getPayload())}))}))
        return reject("physical State identity differs");
      initials[id] = op->getOperand(4);
    } else {
      if (!isa<SourceStateOp>(op) || op->getNumOperands() != 3 ||
          !attrs(op,
                 {b.getNamedAttr("instance_name",
                                 resource.allocation->getAttr("instance_name")),
                  b.getNamedAttr(
                      "occurrence",
                      sourceOccurrence(resource.allocation, b, top, id + 1))}))
        return reject("logical State identity differs");
      initials[id] = op->getOperand(2);
    }
    if (op->getOperand(0) != body.getArgument(0) ||
        op->getOperand(1) != body.getArgument(1))
      return reject("State clock/reset differs");
    q[id] = op->getResult(0);
    if (failed(claim(op)))
      return failure();
  }
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
  if (failed(collect(body)) || failed(collect(work)) ||
      failed(sequence(p.getInstances().front().comb, {}, &body)))
    return failure();
  for (auto [id, resource] : llvm::enumerate(p.getResources()))
    if (initials[id] != values[resource.initial])
      return reject("State initialization differs from source");
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
    if (final) {
      if (storage[id]->getOperand(2) != comb->getResult(2 * id + 1) ||
          storage[id]->getOperand(3) != comb->getResult(2 * id))
        return reject("final data/enable endpoints differ");
    } else {
      auto update = updates[id];
      if (update->getNumOperands() != 3 || !update->getAttrs().empty() ||
          update.getState() != outerQ[id] ||
          update.getCandidate() != comb->getResult(2 * id) ||
          update.getIntent() != comb->getResult(2 * id + 1))
        return reject("routed data/enable endpoints differ");
      if (failed(claim(update)))
        return failure();
    }
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
  for (Block *block : {&body, &work})
    for (Operation &op : *block)
      if (!claimed.contains(&op))
        return reject("unaccounted executable operation");
  for (Operation &op : package.getBody()->getOperations())
    if (&op != module && &op != imported && &op != system)
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
