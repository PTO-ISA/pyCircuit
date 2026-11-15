#include "HardwareCDCGraph.h"
#include "SourceCheckBudget.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/SymbolTable.h"
#include "pycircuit/Dialect/ACIR/HardwareCDC.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/MathExtras.h"

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
  Type bits(unsigned width) {
    return BitsType::get(b.getContext(), literal(width));
  }
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
  Ref slice(Ref input, unsigned width) {
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
                 b.getNamedAttr("implementation", b.getI64IntegerAttr(1))}))};
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
    if (auto table = dyn_cast<TableType>(type(state)))
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
  Equations(Operation *source, StringRef kind, std::optional<uint64_t> depth,
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
      uint64_t depth = *circuit.depth;
      if (depth < 2 || !llvm::isPowerOf2_64(depth))
        return source->emitOpError("selected CDC FIFO implementation supports "
                                   "power-of-two depth >= 2");
      unsigned addressWidth = llvm::Log2_64(depth),
               pointerWidth = addressWidth + 1;
      auto pointerType = bits(pointerWidth);
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
                     constant(pointerType, uint64_t(3) << (pointerWidth - 2))));
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
      auto writeAddress = slice(wb, addressWidth),
           readAddress = slice(rb, addressWidth);
      auto payloadZero = zero(payloadType);
      auto payloadInitial = payloadZero;
      Ref head = payloadZero;
      for (uint64_t slot = 0; slot < depth; ++slot) {
        if (!valid)
          return failure();
        auto address = constant(bits(addressWidth), slot);
        auto bank =
            storage(("payload." + Twine(slot)).str(), payloadType, writeClock,
                    reset, writeEdge, payloadInitial, false);
        drive(bank, Ref(pushPayload),
              binary("and", push, equal(writeAddress, address)));
        head = merge(binary("and", readAvailable, equal(readAddress, address)),
                     bank, head);
      }
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
    if (layout->isFIFO() && !layout->depth)
      return source->emitOpError("CDC recipe requires a resolved lexical DEPTH");
    SmallVector<Value> inputs{layout->sourceClock(), layout->targetClock(),
                              layout->reset(), layout->input()};
    if (layout->isFIFO()) {
      inputs.push_back(layout->pushIntent());
      inputs.push_back(layout->popIntent());
    }
    return Equations(source, layout->kind, layout->depth, inputs, budget).run();
  }
  auto kind = source->getAttrOfType<StringAttr>("logical_kind");
  if (!isa<CDCRefinementOp>(source) || !kind)
    return failure();
  std::optional<uint64_t> depth;
  if (kind.getValue() == "async_fifo")
    depth = source->getAttrOfType<IntegerAttr>("depth").getInt();
  SmallVector<Value> inputs;
  llvm::append_range(inputs, source->getOperands().take_front(
                                kind.getValue() == "async_fifo" ? 6 : 4));
  return Equations(source, kind.getValue(), depth, inputs, budget).run();
}
Value resolve(Ref ref, const SmallVector<Operation *, 0> &nodes,
              const IRMapping &mapping) {
  return ref.external ? mapping.lookup(ref.external)
                      : nodes[ref.node]->getResult(ref.result);
}
} // namespace detail::cdc

namespace {
LogicalResult verifyFinalHardwareCDCComponent(Operation *witness, bool authenticateIntents = true) {
  auto reject = [&]() {
    return witness->emitOpError("invalid CDC component refinement witness");
  };
  if (failed(verifyCDCRefinementContract(witness)))
    return failure();
  auto package = witness->getParentOfType<mlir::ModuleOp>();
  if (authenticateIntents && failed(verifyCDCRefinementIntents(
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
      implementation.getInt() != 1 ||
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
  for (auto [equation, actual] : llvm::zip(equations->nodes, nodes))
    if (!llvm::equal(actual->getResultTypes(), equation.types))
      return reject();
  llvm::DenseSet<Value> exposed;
  unsigned outputBase = reg ? 4 : 6;
  for (auto [i, ref] : llvm::enumerate(equations->outputs)) {
    auto output = resolve(ref, nodes, identity);
    if (output != witness->getOperand(outputBase + i))
      return reject();
    exposed.insert(output);
  }
  for (auto [equation, actual] : llvm::zip(equations->nodes, nodes)) {
    if (actual->getName().getStringRef() != equation.operation ||
        actual->getAttrDictionary() !=
            equation.attributes.getDictionary(witness->getContext()) ||
        !llvm::equal(actual->getResultTypes(), equation.types) ||
        actual->getNumOperands() != equation.inputs.size() ||
        actual->getNumRegions() || actual->getNumSuccessors())
      return reject();
    for (auto [ref, input] : llvm::zip(equation.inputs, actual->getOperands()))
      if (resolve(ref, nodes, identity) != input)
        return reject();
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
getCDCComponentOwners(mlir::ModuleOp package, bool authenticateIntents) {
  LogicalResult valid = success();
  DenseMap<Operation *, Operation *> covered;
  package.walk([&](Operation *op) {
    if (op->getName().getStringRef() != "ac.cdc.refinement" || failed(valid))
      return;
    if (failed(verifyFinalHardwareCDCComponent(op, authenticateIntents))) {
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
FailureOr<DenseMap<Operation *, Operation *>>
getVerifiedCDCComponentOwners(mlir::ModuleOp package) {
  return getCDCComponentOwners(package, true);
}
FailureOr<DenseMap<Operation *, Operation *>>
detail::cdc::getCDCComponentRoutingOwners(mlir::ModuleOp package) {
  return getCDCComponentOwners(package, false);
}
LogicalResult verifyFinalHardwareCDCComponents(mlir::ModuleOp package) {
  return success(succeeded(getVerifiedCDCComponentOwners(package)));
}
} // namespace acir::ac
