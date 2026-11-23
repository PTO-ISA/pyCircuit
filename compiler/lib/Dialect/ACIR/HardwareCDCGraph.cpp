#include "HardwareCDCGraph.h"
#include "SourceCheckBudget.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/SymbolTable.h"
#include "pycircuit/Dialect/ACIR/HardwareCDC.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;
namespace acir::ac {
using detail::cdc::Circuit;
using detail::cdc::Node;
using detail::cdc::Ref;
using detail::cdc::getCircuit;
using detail::cdc::registerSymbol;
using detail::cdc::resolve;

namespace {
// Derive a fresh declarative graph for each caller. Producer-owned values or
// maps never supply correspondence or standalone-verification evidence.
class Equations {
  Circuit circuit;
  Builder b;
  HardwareAnalysis analysis;
  detail::SourceCheckBudget &budget;
  bool valid = true;
  BitsType bit;
  Ref zeroBit, oneBit;
  llvm::SmallVector<Value> inputs;
  StaticExprAttr literal(uint64_t n) {
    if (failed(budget.debit(128, circuit.source)))
      valid = false;
    auto provenance = bit.getWidth().getTree();
    auto number = MathIntAttr::get(b.getContext(),
                                   llvm::APSInt(llvm::APInt(64, n), true));
    return StaticExprAttr::get(
        b.getContext(),
        b.getDictionaryAttr(
            {b.getNamedAttr("kind", b.getStringAttr("literal")),
             b.getNamedAttr(
                 "value",
                 b.getDictionaryAttr(
                     {b.getNamedAttr("kind", b.getStringAttr("integer")),
                      b.getNamedAttr("value", number)})),
             b.getNamedAttr("origin", circuit.source->getAttr("occurrence")),
             b.getNamedAttr("location", provenance.get("location"))}));
  }
  StaticExprAttr expression(StringRef kind, ArrayRef<NamedAttribute> fields) {
    if (failed(budget.debit(128 + 32 * fields.size(), circuit.source)))
      valid = false;
    NamedAttrList tree(fields);
    tree.set("kind", b.getStringAttr(kind));
    tree.set("origin", circuit.source->getAttr("occurrence"));
    tree.set("location", bit.getWidth().getTree().get("location"));
    return StaticExprAttr::get(b.getContext(), tree.getDictionary(b.getContext()));
  }
  StaticExprAttr arithmetic(StringRef op, StaticExprAttr lhs, StaticExprAttr rhs) {
    return expression("binary", {b.getNamedAttr("operator", b.getStringAttr(op)),
        b.getNamedAttr("lhs", lhs), b.getNamedAttr("rhs", rhs)});
  }
  StaticExprAttr select(StaticExprAttr condition, StaticExprAttr yes, StaticExprAttr no) {
    return expression("select", {b.getNamedAttr("condition", condition),
        b.getNamedAttr("yes", yes), b.getNamedAttr("no", no)});
  }
  StaticExprAttr addressWidth() {
    SmallVector<StaticExprAttr> terms;
    auto depth = cast<StaticExprAttr>(circuit.depth);
    // Admitted D = 2^n, 1 <= n <= 63: k=63 is always false.
    // Omitting it keeps generated thresholds in the signed-64 static domain.
    for (unsigned k = 0; k < 63; ++k)
      terms.push_back(select(arithmetic("gt", depth, literal(uint64_t(1) << k)),
                             literal(1), literal(0)));
    while (terms.size() > 1) {
      SmallVector<StaticExprAttr> next;
      for (unsigned i = 0; i < terms.size(); i += 2)
        next.push_back(i + 1 < terms.size()
                           ? arithmetic("add", terms[i], terms[i + 1])
                           : terms[i]);
      terms = std::move(next);
    }
    return select(arithmetic("gt", depth, literal(1)), terms[0], literal(1));
  }
  Type bits(StaticExprAttr width) { return BitsType::get(b.getContext(), width); }
  Ref node(StringRef name, TypeRange types, ArrayRef<Ref> inputs = {},
           ArrayRef<NamedAttribute> attrs = {}) {
    if (failed(budget.debit(128 + 32 * inputs.size(), circuit.source)))
      valid = false;
    unsigned index = circuit.nodes.size();
    Node entry;
    entry.operation = name.str();
    llvm::append_range(entry.types, types);
    llvm::append_range(entry.inputs, inputs);
    entry.attributes.assign(attrs);
    circuit.nodes.push_back(std::move(entry));
    return Ref(index);
  }
  Type type(Ref value) {
    return value.external ? value.external.getType()
                          : circuit.nodes[value.node].types[value.result];
  }
  Ref constant(Type type, StaticExprAttr value) {
    return node(BitsConstantOp::getOperationName(), {type}, {},
                {b.getNamedAttr("value", value)});
  }
  Ref constant(Type type, uint64_t value) {
    return node(BitsConstantOp::getOperationName(), {type}, {},
                {b.getNamedAttr("value", literal(value))});
  }
  Ref binary(StringRef opcode, Ref lhs, Ref rhs) {
    return node(BitsBinaryOp::getOperationName(), {type(lhs)}, {lhs, rhs},
                {b.getNamedAttr("opcode", b.getStringAttr(opcode))});
  }
  Ref invert(Ref input) {
    return node(BitsUnaryOp::getOperationName(), {type(input)}, {input},
                {b.getNamedAttr("opcode", b.getStringAttr("not"))});
  }
  Ref equal(Ref lhs, Ref rhs, StringRef predicate = "eq") {
    return node(BitsCompareOp::getOperationName(), {bit}, {lhs, rhs},
                {b.getNamedAttr("predicate", b.getStringAttr(predicate))});
  }
  Ref slice(Ref input, StaticExprAttr width) {
    return node(BitsExtractOp::getOperationName(), {bits(width)}, {input},
                {b.getNamedAttr("low", literal(0))});
  }
  Ref merge(Ref condition, Ref yes, Ref no) {
    auto result = node(
        ValueMergeOp::getOperationName(), {type(no), bit}, {no, condition, yes},
        {b.getNamedAttr("paths", b.getArrayAttr({b.getArrayAttr({})})),
         b.getNamedAttr("operandSegmentSizes",
                        b.getDenseI32ArrayAttr({1, 1, 1}))});
    return result;
  }
  Ref zero(Type type) {
    if (!valid)
      return zeroBit;
    if (auto bits = dyn_cast<BitsType>(type))
      return constant(bits, 0);
    if (auto enumeration = dyn_cast<EnumType>(type)) {
      auto declaration = analysis.resolveEnum(enumeration, circuit.source);
      if (failed(declaration)) {
        valid = false;
        return zeroBit;
      }
      auto carrier = BitsType::get(b.getContext(), literal(declaration->width));
      // Membership is deliberately unused: raw zero need not be a member.
      return node(EnumFromBitsOp::getOperationName(), {type, bit},
                  {constant(carrier, 0)});
    }
    if (auto record = dyn_cast<StructType>(type)) {
      SmallVector<Ref> fields;
      for (auto raw : analysis.lookupStruct(record).getFields()) {
        auto field = cast<DictionaryAttr>(raw);
        fields.push_back(zero(field.getAs<TypeAttr>("type").getValue()));
      }
      return node(StructCreateOp::getOperationName(), {type}, fields);
    }
    if (auto table = dyn_cast<TableType>(type))
      return node(TableSplatOp::getOperationName(), {type},
                  {zero(table.getElementType())},
                  {b.getNamedAttr("shape", table.getShape())});
    // Preserve a shared generic body; never substitute its first actual here.
    assert(isa<TypeParamType>(type));
    return node(ValueZeroOp::getOperationName(), {type});
  }
  Ref storage(StringRef role, Type type, Ref clock, Ref reset, StringRef edge,
              Ref initial, bool resettable = true) {
    auto name =
        circuit.source->getAttrOfType<StringAttr>("instance_name").getValue();
    auto resourceOccurrence =
        circuit.source->getAttrOfType<DictionaryAttr>("occurrence");
    NamedAttrList occurrence(resourceOccurrence.getValue());
    NamedAttrList site(
        resourceOccurrence.getAs<DictionaryAttr>("site").getValue());
    auto originalPath = cast<ArrayAttr>(site.get("ast_path"));
    SmallVector<Attribute> path(originalPath.begin(), originalPath.end());
    path.push_back(b.getDictionaryAttr(
        {b.getNamedAttr("kind", b.getStringAttr("field")),
         b.getNamedAttr("name", b.getStringAttr(("__cdc." + role).str()))}));
    site.set("ast_path", b.getArrayAttr(path));
    occurrence.set("site", site.getDictionary(b.getContext()));
    SmallVector<NamedAttribute> attrs{
        b.getNamedAttr("instance_name",
                       b.getStringAttr((name + ".__cdc." + role).str())),
        b.getNamedAttr("callee",
                       FlatSymbolRefAttr::get(b.getContext(), registerSymbol)),
        b.getNamedAttr("parameters", b.getArrayAttr({})),
        b.getNamedAttr(
            "type_arguments",
            b.getArrayAttr({TypeAttr::get(
                isa<TableType>(type) ? cast<TableType>(type).getElementType()
                                     : type)})),
        b.getNamedAttr("occurrence", occurrence.getDictionary(b.getContext())),
        b.getNamedAttr("edge", b.getStringAttr(edge)),
        b.getNamedAttr("reset_kind",
                       b.getStringAttr(resettable ? "async" : "none")),
        b.getNamedAttr(
            "init_kind",
            b.getStringAttr(resettable ? "explicit" : "unspecified")),
        b.getNamedAttr(
            "ac.cdc_component",
            b.getDictionaryAttr(
                {b.getNamedAttr("resource",
                                circuit.source->getAttr("occurrence")),
                 b.getNamedAttr("role", b.getStringAttr(role)),
                 b.getNamedAttr("implementation", b.getI64IntegerAttr(2))}))};
    if (resettable)
      attrs.push_back(b.getNamedAttr(
          "reset_polarity", circuit.source->getAttr("reset_polarity")));
    if (auto table = dyn_cast<TableType>(type)) {
      attrs.push_back(b.getNamedAttr("shape", table.getShape()));
      auto splat = [&](Ref input) {
        return node(TableSplatOp::getOperationName(),
                    {TableType::get(b.getContext(), table.getShape(),
                                    this->type(input))},
                    {input}, {b.getNamedAttr("shape", table.getShape())});
      };
      return node(CollectionOp::getOperationName(), {type},
                  {splat(clock), splat(reset), splat(oneBit), initial, initial,
                   initial},
                  attrs);
    }
    return node(InstanceOp::getOperationName(), {type},
                {clock, reset, oneBit, initial, initial, initial}, attrs);
  }
  void drive(Ref state, Ref data, Ref enable) {
    if (auto table = dyn_cast<TableType>(type(state)); table && !isa<TableType>(type(enable)))
      enable =
          node(TableSplatOp::getOperationName(),
               {TableType::get(b.getContext(), table.getShape(), type(enable))},
               {enable}, {b.getNamedAttr("shape", table.getShape())});
    circuit.nodes[state.node].inputs[2] = enable;
    circuit.nodes[state.node].inputs[3] = data;
  }
  Ref sync(StringRef role, Ref input, Ref clock, Ref reset, StringRef edge) {
    auto initial = zero(type(input));
    auto first = storage((role + ".stage1").str(), type(input), clock, reset,
                         edge, initial);
    auto second = storage((role + ".stage2").str(), type(input), clock, reset,
                          edge, initial);
    drive(first, input, oneBit);
    drive(second, first, oneBit);
    return second;
  }

public:
  Equations(Operation *source, StringRef kind, Attribute depth,
            ArrayRef<Value> inputs, detail::SourceCheckBudget &budget)
      : circuit{source, kind.str(), depth, {}, {}}, b(source->getContext()),
        analysis(source->getParentOfType<mlir::ModuleOp>()), budget(budget),
        inputs(inputs) {
    bit = cast<BitsType>(source->getOperand(0).getType());
    zeroBit = constant(bit, 0);
    oneBit = constant(bit, 1);
  }
  FailureOr<Circuit> run() {
    auto *source = circuit.source;
    Ref reset(inputs[2]);
    if (circuit.kind == "async_reg") {
      Ref clock(inputs[1]);
      auto edge = source->getAttrOfType<StringAttr>("target_edge").getValue();
      auto released = sync("release", oneBit, clock, reset, edge);
      auto first = storage("data.stage1", bit, clock, reset, edge, zeroBit);
      auto second = storage("data.stage2", bit, clock, reset, edge, zeroBit);
      drive(first, merge(released, Ref(inputs[3]), zeroBit), oneBit);
      drive(second, merge(released, first, zeroBit), oneBit);
      circuit.outputs.push_back(second);
    } else {
      auto depth = cast<StaticExprAttr>(circuit.depth);
      if (failed(verifyCDCDepth(analysis, depth, {}, source)))
        return failure();
      auto address = addressWidth();
      auto pointerType = bits(arithmetic("add", address, literal(1)));
      auto pointerZero = constant(pointerType, 0),
           pointerOne = constant(pointerType, 1);
      Ref writeClock(inputs[0]), readClock(inputs[1]);
      auto writeEdge =
          source->getAttrOfType<StringAttr>("write_edge").getValue();
      auto readEdge = source->getAttrOfType<StringAttr>("read_edge").getValue();
      auto writeReleased =
          sync("write.release", oneBit, writeClock, reset, writeEdge);
      auto readReleased =
          sync("read.release", oneBit, readClock, reset, readEdge);
      auto peerReadReleased = sync("write.peer_release", readReleased,
                                   writeClock, reset, writeEdge);
      auto peerWriteReleased =
          sync("read.peer_release", writeReleased, readClock, reset, readEdge);
      auto wb = storage("write.binary", pointerType, writeClock, reset,
                        writeEdge, pointerZero);
      auto wg = storage("write.gray", pointerType, writeClock, reset, writeEdge,
                        pointerZero);
      auto rb = storage("read.binary", pointerType, readClock, reset, readEdge,
                        pointerZero);
      auto rg = storage("read.gray", pointerType, readClock, reset, readEdge,
                        pointerZero);
      auto syncedRead = sync("write.pointer", rg, writeClock, reset, writeEdge);
      auto syncedWrite = sync("read.pointer", wg, readClock, reset, readEdge);
      auto full = equal(
          wg, binary("xor", syncedRead,
                     constant(pointerType, arithmetic("shl", literal(3),
                         arithmetic("sub", address, literal(1))))));
      auto empty = equal(rg, syncedWrite);
      auto writeAvailable = binary(
          "and", binary("and", writeReleased, peerReadReleased), invert(full));
      auto readAvailable = binary(
          "and", binary("and", readReleased, peerWriteReleased), invert(empty));
      Value pushPayload = inputs[3];
      Value pushIntent = inputs[4];
      Value popIntent = inputs[5];
      Type payloadType = pushPayload.getType();
      auto asserted =
          source->getAttrOfType<StringAttr>("reset_polarity").getValue() ==
                  "high"
              ? oneBit
              : zeroBit;
      auto resetInactive = invert(equal(reset, asserted, "case_eq"));
      auto push = binary("and", binary("and", Ref(pushIntent), writeAvailable),
                         resetInactive);
      auto pop = binary("and", binary("and", Ref(popIntent), readAvailable),
                        resetInactive);
      auto nextWrite = binary("add", wb, pointerOne),
           nextRead = binary("add", rb, pointerOne);
      drive(wb, nextWrite, push);
      drive(wg, binary("xor", nextWrite, binary("lshr", nextWrite, pointerOne)),
            push);
      drive(rb, nextRead, pop);
      drive(rg, binary("xor", nextRead, binary("lshr", nextRead, pointerOne)),
            pop);
      auto writeAddress = slice(wb, address),
           readAddress = slice(rb, address);
      auto payloadZero = zero(payloadType);
      auto shape = b.getArrayAttr({depth});
      auto tableType = TableType::get(b.getContext(), shape, payloadType);
      auto splat = [&](Ref input) {
        return node(TableSplatOp::getOperationName(),
                    {TableType::get(b.getContext(), shape, type(input))}, {input},
                    {b.getNamedAttr("shape", shape)});
      };
      auto bank = storage("payload", tableType, writeClock, reset, writeEdge,
                          splat(payloadZero), false);
      auto enables = node(TableMapOp::getOperationName(),
          {TableType::get(b.getContext(), shape, bit)}, {writeAddress, push},
          {b.getNamedAttr("shape", shape),
           b.getNamedAttr("operandSegmentSizes", b.getDenseI32ArrayAttr({0, 2}))});
      auto &map = circuit.nodes[enables.node];
      map.arguments = {bits(address), bits(address), bit};
      if (failed(budget.debit(512, source))) valid = false;
      Node compare;
      compare.operation = BitsCompareOp::getOperationName().str();
      compare.types = {bit};
      compare.inputs = {Ref::blockArgument(0), Ref::blockArgument(1)};
      compare.attributes.set("predicate", b.getStringAttr("eq"));
      map.body.push_back(std::move(compare));
      Node gate;
      gate.operation = BitsBinaryOp::getOperationName().str();
      gate.types = {bit};
      gate.inputs = {Ref(0), Ref::blockArgument(2)};
      gate.attributes.set("opcode", b.getStringAttr("and"));
      map.body.push_back(std::move(gate));
      Node yield;
      yield.operation = YieldOp::getOperationName().str();
      yield.inputs = {Ref(1)};
      map.body.push_back(std::move(yield));
      drive(bank, splat(Ref(pushPayload)), enables);
      auto selected = node(TableGetOp::getOperationName(), {payloadType, bit},
                           {bank, readAddress});
      auto head = merge(readAvailable, selected, payloadZero);
      circuit.outputs = {head, writeAvailable, readAvailable};
    }
    if (!valid)
      return failure();
    return std::move(circuit);
  }
};
} // namespace

namespace detail::cdc {
FailureOr<Circuit> getCircuit(Operation *source, SourceCheckBudget &budget) {
  if (auto instance = dyn_cast<InstanceOp>(source)) {
    auto layout = getCDCPrimitiveLayout(instance);
    if (failed(layout))
      return failure();
    SmallVector<Value> inputs{layout->sourceClock(), layout->targetClock(),
                              layout->reset(), layout->input()};
    if (layout->isFIFO()) {
      inputs.push_back(layout->pushIntent());
      inputs.push_back(layout->popIntent());
    }
    return Equations(source, layout->kind, layout->depthExpression, inputs, budget).run();
  }
  auto kind = source->getAttrOfType<StringAttr>("logical_kind");
  if (!isa<CDCRefinementOp>(source) || !kind)
    return failure();
  Attribute depth = source->getAttr("depth");
  SmallVector<Value> inputs;
  llvm::append_range(inputs, source->getOperands().take_front(
                                kind.getValue() == "async_fifo" ? 6 : 4));
  return Equations(source, kind.getValue(), depth, inputs, budget).run();
}
Value resolve(Ref ref, const SmallVector<Operation *, 0> &nodes,
              const IRMapping &mapping, Block *block) {
  if (ref.argument) return block->getArgument(ref.result);
  return ref.external ? mapping.lookup(ref.external)
                      : nodes[ref.node]->getResult(ref.result);
}
LogicalResult verifyNodes(ArrayRef<Node> equations,
    const SmallVector<Operation *, 0> &nodes, const IRMapping &mapping,
    Operation *source, SourceCheckBudget &budget, Block *block) {
  auto reject = [&]() { return source->emitOpError("CDC component equation/region correspondence differs"); };
  if (equations.size() != nodes.size()) return reject();
  for (auto [equation, actual] : llvm::zip(equations, nodes)) {
    if (failed(budget.debit(64 + 16 * actual->getNumOperands() +
                           16 * equation.arguments.size(), source))) return failure();
    if (actual->getName().getStringRef() != equation.operation ||
        actual->getAttrDictionary() != equation.attributes.getDictionary(source->getContext()) ||
        !llvm::equal(actual->getResultTypes(), equation.types) ||
        actual->getLoc() != source->getLoc() ||
        actual->getNumOperands() != equation.inputs.size() ||
        actual->getNumRegions() != (equation.body.empty() ? 0u : 1u) ||
        actual->getNumSuccessors()) return reject();
    if (!equation.body.empty()) {
      auto &region = actual->getRegion(0);
      if (!region.hasOneBlock()) return reject();
      auto &body = region.front();
      if (!llvm::equal(body.getArgumentTypes(), equation.arguments)) return reject();
      for (auto argument : body.getArguments())
        if (argument.getLoc() != source->getLoc()) return reject();
    }
  }
  // Storage pins may refer forward. Establish every node's complete result
  // inventory before resolving any reference in this scope, including in each
  // independently checked TableMap body.
  for (auto [equation, actual] : llvm::zip(equations, nodes)) {
    for (auto [ref, input] : llvm::zip(equation.inputs, actual->getOperands()))
      if (resolve(ref, nodes, mapping, block) != input) return reject();
    if (!equation.body.empty()) {
      auto &body = actual->getRegion(0).front();
      SmallVector<Operation *, 0> children;
      for (auto &child : body) children.push_back(&child);
      if (failed(verifyNodes(equation.body, children, mapping, source, budget, &body)))
        return failure();
    }
  }
  return success();
}
} // namespace detail::cdc

namespace {
LogicalResult verifyFinalHardwareCDCComponent(Operation *witness) {
  auto reject = [&]() {
    return witness->emitOpError("invalid CDC component refinement witness");
  };
  if (failed(verifyCDCRefinementContract(witness)))
    return failure();
  auto package = witness->getParentOfType<mlir::ModuleOp>();
  if (failed(verifyCDCRefinementIntents(
          HardwareAnalysis(package), cast<CDCRefinementOp>(witness), {})))
    return failure();
  auto imported = dyn_cast_or_null<ModuleImportOp>(
      SymbolTable::lookupSymbolIn(package, registerSymbol));
  auto version =
      imported ? getRegisterContract(imported) : FailureOr<unsigned>(failure());
  if (!imported || failed(version) || *version != 3 ||
      imported->getAttrOfType<StringAttr>("primitive_kind").getValue() !=
          "dffe" ||
      failed(HardwareAnalysis(package).verifyImport(imported)))
    return reject();
  auto kind = witness->getAttrOfType<StringAttr>("logical_kind");
  auto implementation = witness->getAttrOfType<IntegerAttr>("implementation");
  bool reg = kind && kind.getValue() == "async_reg";
  if (!kind || (!reg && kind.getValue() != "async_fifo") || !implementation ||
      implementation.getInt() != 2 ||
      witness->getNumOperands() != (reg ? 5u : 9u) ||
      witness->getNumResults() || witness->getNumRegions())
    return reject();
  detail::SourceCheckBudget budget(HardwareSourceCheckLimits{});
  auto equations = getCircuit(witness, budget);
  if (failed(equations))
    return failure();
  SmallVector<Operation *, 0> nodes;
  auto cursor = witness->getIterator();
  for (size_t i = 0; i < equations->nodes.size(); ++i) {
    if (cursor == witness->getBlock()->begin())
      return reject();
    nodes.push_back(&*--cursor);
  }
  std::reverse(nodes.begin(), nodes.end());
  IRMapping identity;
  for (auto operand : witness->getOperands())
    identity.map(operand, operand);
  llvm::DenseSet<Operation *> owned(nodes.begin(), nodes.end());
  if (failed(detail::cdc::verifyNodes(equations->nodes, nodes, identity,
                                      witness, budget)))
    return failure();
  llvm::DenseSet<Value> exposed;
  unsigned outputBase = reg ? 4 : 6;
  for (auto [i, ref] : llvm::enumerate(equations->outputs)) {
    auto output = resolve(ref, nodes, identity);
    if (output != witness->getOperand(outputBase + i))
      return reject();
    exposed.insert(output);
  }
  for (auto *actual : nodes) {
    for (auto result : actual->getResults())
      for (auto *user : result.getUsers())
        if (user != witness && !owned.contains(user) &&
            !exposed.contains(result))
          return reject();
  }
  return success();
}
} // namespace
LogicalResult verifyHardwareCDCComponent(Operation *witness) {
  return verifyFinalHardwareCDCComponent(witness);
}
FailureOr<DenseMap<Operation *, Operation *>>
getVerifiedCDCComponentOwners(mlir::ModuleOp package) {
  LogicalResult valid = success();
  DenseMap<Operation *, Operation *> covered;
  package.walk([&](Operation *op) {
    if (op->getName().getStringRef() != "ac.cdc.refinement" || failed(valid))
      return;
    if (failed(verifyFinalHardwareCDCComponent(op))) {
      valid = failure();
      return;
    }
    detail::SourceCheckBudget budget(HardwareSourceCheckLimits{});
    auto equations = getCircuit(op, budget);
    if (failed(equations)) {
      valid = failure();
      return;
    }
    auto cursor = op->getIterator();
    for (size_t i = 0; i < equations->nodes.size(); ++i) {
      auto *node = &*--cursor;
      if (!covered.try_emplace(node, op).second) {
        op->emitOpError("overlapping CDC component ownership");
        valid = failure();
      }
    }
  });
  package.walk([&](Operation *op) {
    if (op->hasAttr("ac.cdc_component") && !covered.count(op)) {
      op->emitOpError(
          "CDC component lacks a verified complete refinement witness");
      valid = failure();
    }
  });
  if (failed(valid))
    return failure();
  return covered;
}
LogicalResult verifyFinalHardwareCDCComponents(mlir::ModuleOp package) {
  return success(succeeded(getVerifiedCDCComponentOwners(package)));
}
} // namespace acir::ac
