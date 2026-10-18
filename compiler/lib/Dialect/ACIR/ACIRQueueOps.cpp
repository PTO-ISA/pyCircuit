#include "ACIRSourceContracts.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceIdentifier.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/ScopeExit.h"

#include <functional>
#include <limits>
#include <optional>

using namespace mlir;
namespace acir::ac {
namespace {

FailureOr<ResolvedQueueConfig> checkQueueConfiguration(
    const HardwareAnalysis &analysis, Operation *queue, Operation *owner,
    DictionaryAttr configAttributes, TypeRange payloadTypes,
    TypeRange controlTypes, const HardwareBindings &bindings,
    bool requireResolved) {
  auto error = [&] { return queue->emitOpError(); };
  ResolvedQueueConfig config{};
  auto policy = configAttributes.getAs<StringAttr>("ready_policy");
  if (!policy || (policy.getValue() != "local_occupancy" &&
                  policy.getValue() != "downstream_pop"))
    return error()
           << "queue ready_policy must be local_occupancy or downstream_pop";
  config.readyPolicy = policy.getValue() == "local_occupancy"
                           ? QueueReadyPolicy::LocalOccupancy
                           : QueueReadyPolicy::DownstreamPop;
  auto emptyFlow = configAttributes.getAs<BoolAttr>("empty_flow");
  if (!emptyFlow || emptyFlow.getValue())
    return error() << "queue requires empty_flow=false";
  config.emptyFlow = false;
  for (auto [name, expected] :
       {std::pair<StringRef, StringRef>{"read_during_write", "old"},
        {"reset_policy", "sync_high_empty"},
        {"empty_data", "zero"}}) {
    auto value = configAttributes.getAs<StringAttr>(name);
    if (!value || value.getValue() != expected)
      return error() << "queue requires " << name << "='" << expected << "'";
  }
  config.readDuringWrite = configAttributes.getAs<StringAttr>("read_during_write");
  config.resetPolicy = configAttributes.getAs<StringAttr>("reset_policy");
  config.emptyData = configAttributes.getAs<StringAttr>("empty_data");

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
  SmallVector<Type> scopedTypes(payloadTypes.begin(), payloadTypes.end());
  llvm::append_range(scopedTypes, controlTypes);
  for (Type type : scopedTypes)
    if (failed(syntax(type)) ||
        failed(detail::verifyHardwareTypeScope(type, owner)))
      return failure();
  for (const auto &actual : bindings.types)
    if (failed(syntax(actual.second)))
      return failure();
  auto integer = [&](StringRef name,
                     bool positive) -> FailureOr<std::optional<uint64_t>> {
    auto expression = configAttributes.getAs<StaticExprAttr>(name);
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
  for (Type control : controlTypes) {
    auto resolvedControl = analysis.resolveType(control, bindings, queue);
    if (failed(resolvedControl))
      return failure();
    if (!isa<BitsType>(*resolvedControl))
      return error() << "queue controls must be bits";
    auto closed = finite(*resolvedControl);
    if (failed(closed))
      return failure();
    if (*closed) {
      auto width = analysis.getPackedWidth(*resolvedControl, bindings, queue);
      if (failed(width))
        return failure();
      if (*width != 1)
        return error() << "queue controls must have width one";
    }
  }
  auto input =
      analysis.resolveType(payloadTypes[0], bindings, queue);
  auto output =
      analysis.resolveType(payloadTypes[1], bindings, queue);
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

FailureOr<ResolvedQueueConfig> checkQueue(const HardwareAnalysis &analysis,
                                          InstanceOp queue,
                                          const HardwareBindings &bindings,
                                          bool requireResolved) {
  auto error = [&] { return queue.emitOpError(); };
  auto owner = dyn_cast_or_null<ModuleOp>(queue->getParentOp());
  if (!owner || !owner.getBody().hasOneBlock())
    return error() << "queue requires direct module placement";
  auto ownerName = owner->getAttrOfType<StringAttr>("sym_name");
  if (!ownerName || ownerName.getValue().empty())
    return error() << "queue requires a named lexical module owner";
  if (bindings.owner && bindings.owner != owner.getOperation())
    return error() << "queue bindings must belong to its owning module";
  if (!isQueueInstance(queue))
    return error() << "queue verification requires a canonical FIFO instance";
  for (StringRef retired : {"depth", "ready_policy", "availability_latency",
                            "head_read_latency", "empty_flow",
                            "read_during_write", "reset_policy",
                            "empty_data"})
    if (queue->hasAttr(retired))
      return error() << "canonical FIFO configuration cannot duplicate retired '"
                     << retired << "' queue authority";
  if (queue->getNumOperands() != 5 || queue->getNumResults() != 4 ||
      queue->getNumRegions() != 0)
    return error() << "queue requires five inputs and four outputs";
  auto imported = dyn_cast_or_null<ModuleImportOp>(analysis.resolveCallee(queue));
  StringRef expectedKind = queue.getCalleeAttr().getValue() ==
                                   "pycircuit.__builtins__.fifo_local"
                               ? "fifo_local" : "fifo_downstream_pop";
  if (!imported || analysis.getPrimitiveKind(imported) != expectedKind ||
      failed(analysis.verifyImport(imported)))
    return error() << "canonical FIFO requires its trusted builtin import";
  auto queueBindings = analysis.bindInstance(queue, bindings);
  if (failed(queueBindings))
    return failure();
  auto payloadBinding = queueBindings->types.find("T");
  if (payloadBinding == queueBindings->types.end())
    return error() << "canonical FIFO requires bound payload type T";

  auto instanceName = queue->getAttrOfType<StringAttr>("instance_name");
  if (!instanceName || instanceName.getValue().empty() ||
      instanceName.getValue().contains('\0'))
    return error() << "queue requires a nonempty instance_name";
  for (Operation &other : owner.getBody().front())
    if (&other != queue.getOperation() &&
        isa<SourceStateOp, InstanceOp, CollectionOp, QueueOp>(other) &&
        other.getAttrOfType<StringAttr>("instance_name") == instanceName)
      return error() << "instance_name must be unique within parent";
  auto occurrence = queue->getAttrOfType<DictionaryAttr>("occurrence");
  if (!occurrence)
    return error() << "queue occurrence requires a dictionary";
  if (failed(detail::verifyOccurrence(occurrence, error)))
    return failure();

  if (failed(detail::verifyOriginDefinition(
          occurrence, FlatSymbolRefAttr::get(ownerName), "queue allocation", error)))
    return failure();

  bool source = false;
  for (StringRef name : {"ac.source_unbound_push", "ac.source_unbound_pop"}) {
    if (Attribute raw = queue->getAttr(name)) {
      auto flag = dyn_cast<BoolAttr>(raw);
      if (!flag || !flag.getValue())
        return error() << name << " requires BoolAttr true";
      source = true;
      Value pin = name == "ac.source_unbound_push" ? queue.getInputs()[2]
                                                  : queue.getInputs()[4];
      auto constant = pin.getDefiningOp<BitsConstantOp>();
      auto literal = constant
                         ? analysis.evaluateStatic(constant.getValue(), bindings, queue)
                         : FailureOr<Attribute>(failure());
      auto number = succeeded(literal) ? dyn_cast<MathIntAttr>(*literal)
                                       : MathIntAttr();
      if (!number || !llvm::APSInt(number.getCanonicalValue()).isZero())
        return error() << "unbound source queue direction requires a zero placeholder";
    }
  }
  auto package = queue->getParentOfType<mlir::ModuleOp>();
  auto stage = package ? package->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
  if (stage && stage.getValue() == "final" &&
      (source || queue->hasAttr("ac.payload_constraint")))
    return error() << "source queue ownership annotations cannot survive final lowering";
  auto constraint = queue->getAttrOfType<DictionaryAttr>("ac.payload_constraint");
  if ((source || queue->hasAttr("ac.payload_constraint")) &&
      (!constraint || failed(detail::verifySourceCallHardwareConstraint(
                          constraint, queue.getOutputs()[2].getType(), analysis,
                          owner, queue, "queue payload"))))
    return error() << "source queue requires its declared payload constraint";
  auto config = getQueueConfiguration(queue);
  if (failed(config))
    return failure();
  SmallVector<Type> controls{queue.getInputs()[0].getType(),
                             queue.getInputs()[1].getType(),
                             queue.getInputs()[2].getType(),
                             queue.getInputs()[4].getType(),
                             queue.getOutputs()[0].getType(),
                             queue.getOutputs()[1].getType(),
                             queue.getOutputs()[3].getType()};
  auto resolved = checkQueueConfiguration(
      analysis, queue, owner, *config,
      TypeRange{queue.getInputs()[3].getType(), queue.getOutputs()[2].getType()},
      controls, bindings, requireResolved);
  if (failed(resolved))
    return failure();
  if (!areEquivalentHardwareTypes(resolved->payloadType,
                                  payloadBinding->second))
    return error() << "canonical FIFO payload pins must match bound T";
  return resolved;
}

} // namespace

bool isQueueInstance(InstanceOp instance) {
  if (!instance)
    return false;
  auto callee = instance.getCalleeAttr();
  if (!callee)
    return false;
  return callee.getValue() == "pycircuit.__builtins__.fifo_local" ||
         callee.getValue() == "pycircuit.__builtins__.fifo_downstream_pop";
}

FailureOr<DictionaryAttr> getQueueConfiguration(InstanceOp instance) {
  if (!isQueueInstance(instance))
    return instance.emitOpError()
           << "queue configuration requires a canonical FIFO instance";
  return getQueueConfiguration(instance.getCalleeAttr(),
                               instance.getParameters(), instance);
}

FailureOr<DictionaryAttr>
getQueueConfiguration(FlatSymbolRefAttr callee, ArrayAttr parameters,
                      Operation *diagnosticSite) {
  if (!diagnosticSite)
    return failure();
  if (!callee ||
      (callee.getValue() != "pycircuit.__builtins__.fifo_local" &&
       callee.getValue() !=
           "pycircuit.__builtins__.fifo_downstream_pop"))
    return diagnosticSite->emitOpError()
           << "queue configuration requires a canonical FIFO callee";
  if (!parameters || parameters.size() != 2)
    return diagnosticSite->emitOpError()
           << "canonical FIFO instance requires explicit DEPTH and "
              "AVAILABILITY_LATENCY arguments";
  auto depth = dyn_cast<StaticExprAttr>(parameters[0]);
  auto latency = dyn_cast<StaticExprAttr>(parameters[1]);
  if (!depth || !latency)
    return diagnosticSite->emitOpError()
           << "canonical FIFO arguments require StaticExpr values";
  Builder b(diagnosticSite->getContext());
  auto origin = depth.getTree().getAs<DictionaryAttr>("origin");
  auto location = depth.getTree().getAs<DictionaryAttr>("location");
  if (!origin || !location)
    return diagnosticSite->emitOpError()
           << "canonical FIFO DEPTH requires source provenance";
  auto zeroTree = b.getDictionaryAttr({
      b.getNamedAttr("kind", b.getStringAttr("literal")),
      b.getNamedAttr("origin", origin), b.getNamedAttr("location", location),
      b.getNamedAttr(
          "value", b.getDictionaryAttr({
                       b.getNamedAttr("kind", b.getStringAttr("integer")),
                       b.getNamedAttr(
                           "value", MathIntAttr::get(
                                        diagnosticSite->getContext(),
                                        llvm::APSInt::getUnsigned(0)))}))});
  auto headRead =
      StaticExprAttr::get(diagnosticSite->getContext(), zeroTree);
  StringRef readyPolicy = callee.getValue() ==
                                  "pycircuit.__builtins__.fifo_local"
                              ? "local_occupancy"
                              : "downstream_pop";
  return b.getDictionaryAttr({
      b.getNamedAttr("depth", depth),
      b.getNamedAttr("ready_policy", b.getStringAttr(readyPolicy)),
      b.getNamedAttr("availability_latency", latency),
      b.getNamedAttr("head_read_latency", headRead),
      b.getNamedAttr("empty_flow", b.getBoolAttr(false)),
      b.getNamedAttr("read_during_write", b.getStringAttr("old")),
      b.getNamedAttr("reset_policy", b.getStringAttr("sync_high_empty")),
      b.getNamedAttr("empty_data", b.getStringAttr("zero"))});
}

LogicalResult verifySourceQueueConfiguration(Operation *owner,
                                               DictionaryAttr config,
                                               Type payload, Operation *site) {
  const SmallVector<StringRef> fields{
      "depth", "ready_policy", "availability_latency", "head_read_latency",
      "empty_flow", "read_during_write", "reset_policy", "empty_data"};
  if (!config || config.size() != fields.size())
    return site->emitOpError() << "queue configuration has invalid fields";
  for (StringRef field : fields)
    if (!config.get(field))
      return site->emitOpError() << "queue configuration is missing " << field;
  HardwareAnalysis analysis(owner->getParentOfType<mlir::ModuleOp>());
  HardwareBindings bindings;
  bindings.owner = owner;
  return success(succeeded(checkQueueConfiguration(
      analysis, site, owner, config, TypeRange{payload, payload}, {}, bindings,
      false)));
}

LogicalResult SourceQueuePopOp::verify() {
  return emitOpError()
         << "retired dedicated queue effects are not admitted; use a native "
            "FIFO Method call";
}
LogicalResult SourceQueuePeekOp::verify() {
  return emitOpError()
         << "retired dedicated queue effects are not admitted; use a native "
            "FIFO Method call";
}
LogicalResult SourceQueuePushOp::verify() {
  return emitOpError()
         << "retired dedicated queue effects are not admitted; use a native "
            "FIFO Method call";
}

LogicalResult QueueOp::verify() {
  return emitOpError()
         << "retired ac.queue is not admitted; use a canonical FIFO instance";
}

LogicalResult
HardwareAnalysis::verifyQueueOperation(InstanceOp queue,
                                       const HardwareBindings &bindings,
                                       bool requireResolved) const {
  return success(
      succeeded(checkQueue(*this, queue, bindings, requireResolved)));
}

FailureOr<ResolvedQueueConfig>
HardwareAnalysis::resolveQueue(InstanceOp queue,
                               const HardwareBindings &bindings) const {
  return checkQueue(*this, queue, bindings, true);
}

} // namespace acir::ac
