#include "ACIRSourceContracts.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/ScopeExit.h"

#include <functional>
#include <limits>
#include <optional>

using namespace mlir;
namespace acir::ac {
namespace {

FailureOr<ResolvedQueueConfig> checkQueue(const HardwareAnalysis &analysis,
                                          QueueOp queue,
                                          const HardwareBindings &bindings,
                                          bool requireResolved) {
  auto error = [&] { return queue.emitOpError(); };
  auto owner = dyn_cast_or_null<ModuleOp>(queue->getParentOp());
  if (!owner || !owner.getBody().hasOneBlock())
    return error() << "queue requires direct module placement";
  auto ownerName = owner->getAttrOfType<StringAttr>("sym_name");
  if (!ownerName || ownerName.getValue().empty())
    return error() << "queue requires a named lexical module owner";
  // Signature verification establishes the scope walkers' inventory
  // precondition without recursing into this operation through verifyRegions().
  if (failed(owner.verify()))
    return failure();
  if (bindings.owner && bindings.owner != owner.getOperation())
    return error() << "queue bindings must belong to its owning module";
  if (queue->getNumOperands() != 5 || queue->getNumResults() != 3 ||
      queue->getNumRegions() != 0)
    return error() << "queue requires five inputs and three outputs";

  const SmallVector<StringRef> names{
      "instance_name",     "depth",
      "ready_policy",      "availability_latency",
      "head_read_latency", "empty_flow",
      "read_during_write", "reset_policy",
      "empty_data",        "occurrence"};
  for (auto attr : queue->getAttrs())
    if (!llvm::is_contained(names, attr.getName().getValue()))
      return error() << "unknown queue attribute '" << attr.getName() << "'";
  for (auto name : names)
    if (!queue->hasAttr(name))
      return error() << "missing queue attribute '" << name << "'";

  auto instanceName = queue->getAttrOfType<StringAttr>("instance_name");
  if (!instanceName || instanceName.getValue().empty() ||
      instanceName.getValue().contains('\0'))
    return error() << "queue requires a nonempty instance_name";
  for (Operation &other : owner.getBody().front())
    if (&other != queue.getOperation() &&
        isa<InstanceOp, CollectionOp, QueueOp>(other) &&
        other.getAttrOfType<StringAttr>("instance_name") == instanceName)
      return error() << "instance_name must be unique within parent";
  auto occurrence = queue->getAttrOfType<DictionaryAttr>("occurrence");
  if (!occurrence)
    return error() << "queue occurrence requires a dictionary";
  if (failed(detail::verifyOccurrence(occurrence, error)))
    return failure();

  ResolvedQueueConfig config{};
  auto policy = queue->getAttrOfType<StringAttr>("ready_policy");
  if (!policy || (policy.getValue() != "local_occupancy" &&
                  policy.getValue() != "downstream_pop"))
    return error()
           << "queue ready_policy must be local_occupancy or downstream_pop";
  config.readyPolicy = policy.getValue() == "local_occupancy"
                           ? QueueReadyPolicy::LocalOccupancy
                           : QueueReadyPolicy::DownstreamPop;
  auto emptyFlow = queue->getAttrOfType<BoolAttr>("empty_flow");
  if (!emptyFlow || emptyFlow.getValue())
    return error() << "queue requires empty_flow=false";
  config.emptyFlow = false;
  for (auto [name, expected] :
       {std::pair<StringRef, StringRef>{"read_during_write", "old"},
        {"reset_policy", "sync_high_empty"},
        {"empty_data", "zero"}}) {
    auto value = queue->getAttrOfType<StringAttr>(name);
    if (!value || value.getValue() != expected)
      return error() << "queue requires " << name << "='" << expected << "'";
  }
  config.readDuringWrite = queue.getReadDuringWriteAttr();
  config.resetPolicy = queue.getResetPolicyAttr();
  config.emptyData = queue.getEmptyDataAttr();

  // Check original syntax, not substituted forwarded actuals, against the
  // lexical owner. Both branches of a static choice must be well scoped.
  std::function<LogicalResult(Type)> syntax;
  syntax = [&](Type type) -> LogicalResult {
    if (auto bits = dyn_cast<BitsType>(type))
      return BitsType::verify(error, bits.getWidth());
    if (auto table = dyn_cast<TableType>(type)) {
      if (failed(TableType::verify(error, table.getShape(),
                                   table.getElementType())))
        return failure();
      return syntax(table.getElementType());
    }
    if (auto formal = dyn_cast<TypeParamType>(type))
      return TypeParamType::verify(error, formal.getOwner(), formal.getName());
    if (auto record = dyn_cast<StructType>(type))
      return StructType::verify(error, record.getName());
    if (auto enumeration = dyn_cast<EnumType>(type))
      return EnumType::verify(error, enumeration.getName());
    return error() << "queue requires finite hardware payload types";
  };
  for (Type type : queue->getOperandTypes())
    if (failed(syntax(type)) ||
        failed(detail::verifyHardwareTypeScope(type, owner)))
      return failure();
  for (Type type : queue->getResultTypes())
    if (failed(syntax(type)) ||
        failed(detail::verifyHardwareTypeScope(type, owner)))
      return failure();
  for (const auto &actual : bindings.types)
    if (failed(syntax(actual.second)))
      return failure();
  auto integer = [&](StringRef name,
                     bool positive) -> FailureOr<std::optional<uint64_t>> {
    auto expression = queue->getAttrOfType<StaticExprAttr>(name);
    if (!expression)
      return error() << "queue " << name << " requires a scoped StaticExpr";
    if (failed(StaticExprAttr::verify(error, expression.getTree())) ||
        failed(detail::verifyHardwareAttributeScope(expression, owner)))
      return failure();
    if (!requireResolved && !analysis.isStaticEvaluable(expression, bindings))
      return std::optional<uint64_t>{};
    auto value = analysis.evaluateStatic(expression, bindings, queue);
    if (failed(value))
      return failure();
    auto number = dyn_cast<MathIntAttr>(*value);
    if (!number)
      return error() << "queue " << name << " requires an integer";
    llvm::APSInt n(number.getCanonicalValue());
    if (n.isNegative() || (positive && n.isZero()) || n.getActiveBits() > 64)
      return error() << "queue " << name << " must be "
                     << (positive ? "positive" : "nonnegative")
                     << " and fit u64";
    return std::optional<uint64_t>{n.getZExtValue()};
  };
  auto depth = integer("depth", true);
  auto availability = integer("availability_latency", true);
  auto headRead = integer("head_read_latency", false);
  if (failed(depth) || failed(availability) || failed(headRead))
    return failure();
  if (*headRead && **headRead != 0)
    return error() << "queue head_read_latency must equal zero";
  if (*depth)
    config.depth = **depth;
  if (*availability)
    config.availabilityLatency = **availability;
  if (*headRead)
    config.headReadLatency = **headRead;

  // Timing geometry depends only on depth and latency, not the payload type.
  // Validate it even for declarations whose payload is still symbolic.
  if (*depth) {
    config.pointerWidth =
        std::max(1u, llvm::APInt(64, config.depth - 1).getActiveBits());
    config.countWidth = llvm::APInt(64, config.depth).getActiveBits();
  }
  if (*availability && config.availabilityLatency > 1) {
    config.timestampWidth =
        llvm::APInt(64, config.availabilityLatency - 1).getActiveBits();
    if (*depth) {
      const uint64_t maxBits = std::numeric_limits<uint64_t>::max();
      if (config.depth > maxBits / config.timestampWidth)
        return error()
               << "queue logical timing storage bit count overflows u64";
      const uint64_t deadlineBits = config.depth * config.timestampWidth;
      // Each width is at most 64, so this metadata sum is at most 192.
      const uint64_t metadataBits = uint64_t(config.timestampWidth) +
                                    config.pointerWidth + config.countWidth;
      if (deadlineBits > maxBits - metadataBits)
        return error()
               << "queue logical timing storage bit count overflows u64";
      config.timingStorageBitCount = deadlineBits + metadataBits;
    }
  }

  // Validate each constituent even when another dimension remains symbolic.
  llvm::DenseSet<Type> activeTypes;
  std::function<FailureOr<bool>(Type)> finite;
  finite = [&](Type type) -> FailureOr<bool> {
    if (!activeTypes.insert(type).second)
      return error() << "cyclic queue payload type binding";
    llvm::scope_exit cleanup([&] { activeTypes.erase(type); });
    auto resolved = analysis.resolveType(type, bindings, queue);
    if (failed(resolved))
      return failure();
    if (isa<TypeParamType>(*resolved)) {
      if (requireResolved)
        return error() << "queue payload requires a resolved finite type";
      return false;
    }
    if (auto table = dyn_cast<TableType>(*resolved)) {
      if (failed(TableType::verify(error, table.getShape(),
                                   table.getElementType())))
        return failure();
      bool shapeClosed = analysis.isStaticEvaluable(table.getShape(), bindings);
      if ((requireResolved || shapeClosed) &&
          failed(analysis.getTableSize(table, bindings, queue)))
        return failure();
      auto elementClosed = finite(table.getElementType());
      if (failed(elementClosed))
        return failure();
      return (requireResolved || shapeClosed) && *elementClosed;
    }
    if (auto bits = dyn_cast<BitsType>(*resolved)) {
      if (failed(StaticExprAttr::verify(error, bits.getWidth().getTree())))
        return failure();
      if (!requireResolved &&
          !analysis.isStaticEvaluable(bits.getWidth(), bindings))
        return false;
    }
    if (failed(analysis.getPackedWidth(*resolved, bindings, queue)))
      return failure();
    return true;
  };
  for (Value control : SmallVector<Value>{
           queue->getOperand(0), queue->getOperand(1), queue->getOperand(2),
           queue->getOperand(4), queue->getResult(0), queue->getResult(1)}) {
    if (!isa<BitsType>(control.getType()))
      return error() << "queue controls must be bits";
    auto closed = finite(control.getType());
    if (failed(closed))
      return failure();
    if (*closed) {
      auto width = analysis.getPackedWidth(control.getType(), bindings, queue);
      if (failed(width))
        return failure();
      if (*width != 1)
        return error() << "queue controls must have width one";
    }
  }
  auto input =
      analysis.resolveType(queue.getInData().getType(), bindings, queue);
  auto output =
      analysis.resolveType(queue.getOutData().getType(), bindings, queue);
  if (failed(input) || failed(output))
    return failure();
  if (!areEquivalentHardwareTypes(*input, *output))
    return error() << "queue input and output payload types must match";
  auto payloadClosed = finite(*input);
  if (failed(payloadClosed))
    return failure();
  config.payloadType = *input;
  if (!*payloadClosed)
    return config;

  auto width = analysis.getPackedWidth(*input, bindings, queue);
  if (failed(width))
    return failure();
  config.packedWidth = *width;
  config.tokenCardinality = 1;
  Type element = *input;
  while (auto table = dyn_cast<TableType>(element)) {
    auto shape = analysis.getTableShape(table, bindings, queue);
    if (failed(shape))
      return failure();
    for (uint64_t extent : *shape) {
      if (config.tokenCardinality >
          uint64_t(std::numeric_limits<int64_t>::max()) / extent)
        return error() << "queue token cardinality exceeds signed 64 bits";
      config.tokenCardinality *= extent;
      config.tokenShape.push_back(extent);
    }
    element = table.getElementType();
  }
  config.tokenElementType = element;
  if (*depth) {
    if (config.depth > std::numeric_limits<uint64_t>::max() / *width)
      return error() << "queue logical storage bit count overflows u64";
    config.storageBitCount = config.depth * *width;
    if (config.storageBitCount >
        std::numeric_limits<uint64_t>::max() - config.timingStorageBitCount)
      return error()
             << "queue combined logical storage bit count overflows u64";
  }
  return config;
}

} // namespace

LogicalResult QueueOp::verify() {
  auto package = (*this)->getParentOfType<mlir::ModuleOp>();
  if (!package)
    return emitOpError() << "queue requires hardware package placement";
  HardwareBindings scope;
  scope.owner = (*this)->getParentOp();
  return HardwareAnalysis(package).verifyQueueOperation(*this, scope);
}

LogicalResult
HardwareAnalysis::verifyQueueOperation(QueueOp queue,
                                       const HardwareBindings &bindings,
                                       bool requireResolved) const {
  return success(
      succeeded(checkQueue(*this, queue, bindings, requireResolved)));
}

FailureOr<ResolvedQueueConfig>
HardwareAnalysis::resolveQueue(QueueOp queue,
                               const HardwareBindings &bindings) const {
  return checkQueue(*this, queue, bindings, true);
}

} // namespace acir::ac
