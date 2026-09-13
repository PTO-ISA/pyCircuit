#include "acir/Transforms/Passes.h"

#include "Analysis/ModelAnalysisInternal.h"
#include "acir/Dialect/ACIR/ACIROps.h"

#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/SHA256.h"
#include "llvm/Support/raw_ostream.h"

#include <optional>

using namespace mlir;

namespace acir {
namespace {

constexpr llvm::StringLiteral kProfile = "private_transform_tuple_v1";

struct ProjectionPlan {
  ac::TransformOp producer;
  ac::TransformOp consumer;
  ac::StructType logicalType;
  SmallVector<StringAttr> fields;
  SmallVector<Type> fieldTypes;
  SmallVector<ac::VarGetOp> reads;
  TupleType carrierType;
  std::string fingerprint;
};

bool isOrdinaryTransform(ac::TransformOp transform) {
  if (transform.getInputs().size() != 1 || transform.getOutputs().size() != 1 ||
      transform->hasAttr("ac.payload_projection_forbidden"))
    return false;
  if (transform->hasAttr("ac.rule_definition")) {
    auto footprints = transform->getAttrOfType<ArrayAttr>("ac.rule_footprints");
    auto state = transform->getAttrOfType<ArrayAttr>("ac.rule_state_accesses");
    if (!footprints || !footprints.empty() || !state || !state.empty())
      return false;
  }
  auto input = cast<ac::QueueType>(transform.getInputs().front().getType());
  auto output = cast<ac::QueueType>(transform.getOutputs().front().getType());
  return input.getLanes() == 1 && input.getRate() == 1 &&
         output.getLanes() == 1 && output.getRate() == 1;
}

FailureOr<ArrayAttr> structFields(Operation *anchor, ac::StructType type) {
  Operation *declaration =
      SymbolTable::lookupNearestSymbolFrom(anchor, type.getName());
  auto structure = dyn_cast_or_null<ac::StructOp>(declaration);
  if (!structure)
    return failure();
  return structure.getFields();
}

LogicalResult appendDescriptor(llvm::raw_ostream &stream, Operation *anchor,
                               Type type,
                               llvm::SmallPtrSetImpl<Operation *> &active) {
  if (auto structure = dyn_cast<ac::StructType>(type)) {
    Operation *declaration =
        SymbolTable::lookupNearestSymbolFrom(anchor, structure.getName());
    auto record = dyn_cast_or_null<ac::StructOp>(declaration);
    if (!record || !active.insert(record).second)
      return failure();
    stream << "struct(" << structure.getName() << "){";
    for (Attribute raw : record.getFields()) {
      auto field = cast<DictionaryAttr>(raw);
      StringRef name = field.getAs<StringAttr>("name").getValue();
      stream << name.size() << ':' << name << '=';
      if (failed(appendDescriptor(stream, anchor,
                                  field.getAs<TypeAttr>("type").getValue(),
                                  active)))
        return failure();
      stream << ';';
    }
    stream << '}';
    active.erase(record);
    return success();
  }
  if (auto enumeration = dyn_cast<ac::EnumType>(type)) {
    Operation *declaration =
        SymbolTable::lookupNearestSymbolFrom(anchor, enumeration.getName());
    auto record = dyn_cast_or_null<ac::EnumOp>(declaration);
    if (!record)
      return failure();
    stream << "enum(" << enumeration.getName() << ")";
    for (Attribute item : record.getEnumerants())
      stream << cast<StringAttr>(item).getValue() << ';';
    if (record.getValuesAttr())
      for (Attribute item : record.getValuesAttr())
        stream << cast<IntegerAttr>(item).getValue().getZExtValue() << ';';
    stream << "width="
           << (record.getEncodingWidthAttr()
                   ? record.getEncodingWidthAttr().getInt()
                   : -1);
    return success();
  }
  if (auto tuple = dyn_cast<TupleType>(type)) {
    stream << "tuple(";
    for (Type element : tuple.getTypes()) {
      if (failed(appendDescriptor(stream, anchor, element, active)))
        return failure();
      stream << ';';
    }
    stream << ')';
    return success();
  }
  if (auto array = dyn_cast<ac::ValueArrayType>(type)) {
    stream << "array(" << array.getLength() << ',';
    if (failed(
            appendDescriptor(stream, anchor, array.getElementType(), active)))
      return failure();
    stream << ')';
    return success();
  }
  stream << type;
  return success();
}

std::string projectionFingerprint(Operation *anchor, ac::StructType logicalType,
                                  ArrayRef<StringAttr> fields,
                                  TupleType carrierType) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  stream << "version=1\nprofile=" << kProfile << "\nlogical=";
  llvm::SmallPtrSet<Operation *, 8> active;
  if (failed(appendDescriptor(stream, anchor, logicalType, active)))
    return {};
  stream << "\nfields=";
  for (StringAttr field : fields)
    stream << field.getValue().size() << ':' << field.getValue() << ';';
  stream << "\ncarrier=";
  stream << Type(carrierType);
  llvm::SHA256 sha;
  sha.update(stream.str());
  return "sha256:" + llvm::toHex(sha.final(), /*LowerCase=*/true);
}

FailureOr<ProjectionPlan> planProjection(ac::TransformOp producer) {
  if (!isOrdinaryTransform(producer) ||
      !isa<ModuleOp>(producer->getParentOp()) ||
      producer->hasAttr("ac.payload_projections_out"))
    return failure();
  Value edge = producer.getOutputs().front();
  if (!edge.hasOneUse())
    return failure();
  auto consumer = dyn_cast<ac::TransformOp>(*edge.getUsers().begin());
  if (!consumer || !isOrdinaryTransform(consumer) ||
      consumer->hasAttr("ac.payload_projections_in") ||
      consumer.getInputs().front() != edge)
    return failure();
  auto queueType = cast<ac::QueueType>(edge.getType());
  auto logicalType = dyn_cast<ac::StructType>(queueType.getElementType());
  if (!logicalType)
    return failure();

  BlockArgument argument = consumer.getBody().front().getArgument(0);
  SmallVector<ac::VarGetOp> reads;
  llvm::SmallDenseSet<llvm::StringRef> demanded;
  for (OpOperand &use : argument.getUses()) {
    auto get = dyn_cast<ac::VarGetOp>(use.getOwner());
    if (!get || get.getRecord() != argument)
      return failure();
    reads.push_back(get);
    demanded.insert(get.getField());
  }
  if (reads.empty())
    return failure();
  FailureOr<ArrayAttr> declarationFields = structFields(producer, logicalType);
  if (failed(declarationFields) || demanded.size() >= declarationFields->size())
    return failure();

  ProjectionPlan plan{producer, consumer, logicalType};
  llvm::DenseMap<llvm::StringRef, unsigned> ordinals;
  for (Attribute raw : *declarationFields) {
    auto field = cast<DictionaryAttr>(raw);
    StringAttr name = field.getAs<StringAttr>("name");
    if (!demanded.contains(name.getValue()))
      continue;
    ordinals[name.getValue()] = plan.fields.size();
    plan.fields.push_back(name);
    plan.fieldTypes.push_back(field.getAs<TypeAttr>("type").getValue());
  }
  if (plan.fields.size() != demanded.size())
    return failure();
  for (ac::VarGetOp read : reads)
    if (!ordinals.contains(read.getField()))
      return failure();
  plan.reads = std::move(reads);
  plan.carrierType = TupleType::get(producer.getContext(), plan.fieldTypes);
  plan.fingerprint = projectionFingerprint(producer, logicalType, plan.fields,
                                           plan.carrierType);
  if (plan.fingerprint.empty())
    return failure();
  return plan;
}

DictionaryAttr projectionEvidence(OpBuilder &builder,
                                  const ProjectionPlan &plan) {
  SmallVector<Attribute> fields(plan.fields.begin(), plan.fields.end());
  return builder.getDictionaryAttr({
      builder.getNamedAttr("ordinal", builder.getI64IntegerAttr(0)),
      builder.getNamedAttr("version", builder.getI64IntegerAttr(1)),
      builder.getNamedAttr("profile", builder.getStringAttr(kProfile)),
      builder.getNamedAttr("logical_type", TypeAttr::get(plan.logicalType)),
      builder.getNamedAttr("kept_fields", builder.getArrayAttr(fields)),
      builder.getNamedAttr("fingerprint",
                           builder.getStringAttr(plan.fingerprint)),
  });
}

void applyProjection(ProjectionPlan &plan) {
  MLIRContext *context = plan.producer.getContext();
  auto outputType =
      cast<ac::QueueType>(plan.producer.getOutputs()[0].getType());
  auto carrierQueue = ac::QueueType::get(
      context, plan.carrierType, outputType.getLanes(), outputType.getRate());
  auto carrierVar = ac::VarType::get(context, plan.carrierType);
  auto yielded = cast<ac::TransformYieldOp>(
      plan.producer.getBody().front().getTerminator());
  Value logicalValue = yielded.getValues().front();
  OpBuilder producerBuilder(yielded);
  SmallVector<Value> projected;
  for (auto [field, fieldType] :
       llvm::zip_equal(plan.fields, plan.fieldTypes)) {
    OperationState getState(yielded.getLoc(), ac::VarGetOp::getOperationName());
    getState.addOperands(logicalValue);
    getState.addTypes(ac::VarType::get(context, fieldType));
    getState.addAttribute("field", field);
    projected.push_back(producerBuilder.create(getState)->getResult(0));
  }
  OperationState tupleState(yielded.getLoc(),
                            ac::VarTupleOp::getOperationName());
  tupleState.addOperands(projected);
  tupleState.addTypes(carrierVar);
  Value carrier = producerBuilder.create(tupleState)->getResult(0);
  yielded.getValuesMutable().assign(carrier);
  plan.producer.getOutputs()[0].setType(carrierQueue);

  BlockArgument argument = plan.consumer.getBody().front().getArgument(0);
  argument.setType(carrierVar);
  llvm::DenseMap<llvm::StringRef, unsigned> ordinals;
  for (auto [index, field] : llvm::enumerate(plan.fields))
    ordinals[field.getValue()] = index;
  for (ac::VarGetOp read : plan.reads) {
    OpBuilder builder(read);
    OperationState elementState(read.getLoc(),
                                ac::VarElementOp::getOperationName());
    elementState.addOperands(argument);
    elementState.addTypes(read.getResult().getType());
    elementState.addAttribute(
        "index", builder.getI64IntegerAttr(ordinals[read.getField()]));
    Value replacement = builder.create(elementState)->getResult(0);
    if (Attribute provenance = read->getAttr("ac.source_provenance"))
      replacement.getDefiningOp()->setAttr("ac.source_provenance", provenance);
    read.getResult().replaceAllUsesWith(replacement);
    read.erase();
  }

  OpBuilder builder(context);
  ArrayAttr evidence =
      builder.getArrayAttr({projectionEvidence(builder, plan)});
  plan.producer->setAttr("ac.payload_projections_out", evidence);
  plan.consumer->setAttr("ac.payload_projections_in", evidence);
}

LogicalResult
verifyProjectionEvidence(ac::TransformOp producer,
                         llvm::SmallPtrSetImpl<Operation *> &seen) {
  auto outputs =
      producer->getAttrOfType<ArrayAttr>("ac.payload_projections_out");
  if (!producer->hasAttr("ac.payload_projections_out"))
    return success();
  if (!outputs)
    return producer.emitOpError(
        "private payload projection output evidence must be an array");
  if (!isOrdinaryTransform(producer) || !isa<ModuleOp>(producer->getParentOp()))
    return producer.emitOpError(
        "private payload projection producer is outside the admitted profile");
  if (outputs.size() != 1 || producer.getOutputs().size() != 1)
    return producer.emitOpError(
        "private payload projection requires one output evidence record");
  auto evidence = dyn_cast<DictionaryAttr>(outputs[0]);
  auto ordinal =
      evidence ? evidence.getAs<IntegerAttr>("ordinal") : IntegerAttr();
  auto version =
      evidence ? evidence.getAs<IntegerAttr>("version") : IntegerAttr();
  auto profile =
      evidence ? evidence.getAs<StringAttr>("profile") : StringAttr();
  auto logical =
      evidence ? evidence.getAs<TypeAttr>("logical_type") : TypeAttr();
  auto fields =
      evidence ? evidence.getAs<ArrayAttr>("kept_fields") : ArrayAttr();
  auto fingerprint =
      evidence ? evidence.getAs<StringAttr>("fingerprint") : StringAttr();
  if (!evidence || evidence.size() != 6 || !ordinal || ordinal.getInt() != 0 ||
      !version || version.getInt() != 1 || !profile ||
      profile.getValue() != kProfile || !logical || !fields || fields.empty() ||
      !fingerprint)
    return producer.emitOpError(
        "private payload projection evidence is malformed");
  auto logicalType = dyn_cast<ac::StructType>(logical.getValue());
  auto carrier = dyn_cast<TupleType>(
      cast<ac::QueueType>(producer.getOutputs()[0].getType()).getElementType());
  FailureOr<ArrayAttr> declared = logicalType
                                      ? structFields(producer, logicalType)
                                      : FailureOr<ArrayAttr>();
  if (!logicalType || !carrier || failed(declared) ||
      fields.size() >= declared->size() || fields.size() != carrier.size())
    return producer.emitOpError(
        "private payload projection logical/carrier types are inconsistent");
  SmallVector<StringAttr> kept;
  SmallVector<Type> keptTypes;
  size_t cursor = 0;
  for (Attribute raw : *declared) {
    auto field = cast<DictionaryAttr>(raw);
    if (cursor == fields.size())
      break;
    auto requested = dyn_cast<StringAttr>(fields[cursor]);
    if (!requested)
      return producer.emitOpError("kept field names must be strings");
    auto declaredName = field.getAs<StringAttr>("name");
    if (declaredName != requested)
      continue;
    kept.push_back(requested);
    keptTypes.push_back(field.getAs<TypeAttr>("type").getValue());
    ++cursor;
  }
  if (cursor != fields.size())
    return producer.emitOpError(
        "kept fields must be unique and preserve declaration order");
  for (auto [actual, expected] : llvm::zip_equal(carrier.getTypes(), keptTypes))
    if (actual != expected)
      return producer.emitOpError(
          "carrier elements must exactly match retained field types");
  if (fingerprint.getValue() !=
      projectionFingerprint(producer, logicalType, kept, carrier))
    return producer.emitOpError(
        "private payload projection fingerprint mismatch");

  Value edge = producer.getOutputs()[0];
  if (!edge.hasOneUse())
    return producer.emitOpError(
        "private payload projection edge must have one consumer");
  auto consumer = dyn_cast<ac::TransformOp>(*edge.getUsers().begin());
  if (!consumer || consumer.getInputs().size() != 1 ||
      consumer.getInputs()[0] != edge || !isOrdinaryTransform(consumer) ||
      !isa<ModuleOp>(consumer->getParentOp()))
    return producer.emitOpError(
        "private payload projection consumer must be one Transform input");
  auto inputs = consumer->getAttrOfType<ArrayAttr>("ac.payload_projections_in");
  if (!inputs || inputs.size() != 1 || inputs[0] != evidence)
    return consumer.emitOpError(
        "private payload projection input/output evidence must match");
  BlockArgument argument = consumer.getBody().front().getArgument(0);
  for (OpOperand &use : argument.getUses()) {
    auto element = dyn_cast<ac::VarElementOp>(use.getOwner());
    if (!element || element.getAggregate() != argument ||
        element.getIndex() < 0 ||
        static_cast<size_t>(element.getIndex()) >= kept.size())
      return consumer.emitOpError(
          "private carrier may be consumed only by retained tuple elements");
  }
  seen.insert(consumer);
  return success();
}

#define GEN_PASS_DEF_PRUNEINTERNALPAYLOADSPASS
#include "acir/Transforms/Passes.h.inc"

struct PruneInternalPayloadsPass
    : impl::PruneInternalPayloadsPassBase<PruneInternalPayloadsPass> {
  void runOnOperation() override {
    ModuleOp model = getOperation();
    if (model->hasAttr("ac.topology_frozen"))
      return;
    if (auto disabled =
            model->getAttrOfType<BoolAttr>("ac.disable_payload_pruning")) {
      model->removeAttr("ac.disable_payload_pruning");
      if (disabled.getValue())
        return;
    }
    if (detail::hasTopologyFreezeEvidence(model)) {
      model.emitError(
          "internal payload pruning rejects partial freeze evidence");
      return signalPassFailure();
    }
    auto kind = model->getAttrOfType<StringAttr>("ac.model_kind");
    if (!kind || kind.getValue() != "queue_graph" ||
        !model->getAttrOfType<StringAttr>("ac.system") ||
        !model.getOps<ac::SystemOp>().empty() ||
        !model.getOps<ac::ModuleOp>().empty())
      return;
    while (true) {
      std::optional<ProjectionPlan> selected;
      model.walk([&](ac::TransformOp transform) {
        FailureOr<ProjectionPlan> plan = planProjection(transform);
        if (succeeded(plan))
          selected = std::move(*plan);
      });
      if (!selected)
        break;
      applyProjection(*selected);
    }
  }
};

} // namespace

LogicalResult verifyInternalPayloadProjections(ModuleOp model) {
  llvm::SmallPtrSet<Operation *, 8> matchedConsumers;
  LogicalResult result = success();
  bool hasEvidence = false;
  model.walk([&](Operation *operation) {
    const bool projected = operation->hasAttr("ac.payload_projections_in") ||
                           operation->hasAttr("ac.payload_projections_out");
    hasEvidence |= projected;
    if (projected && !isa<ac::TransformOp>(operation))
      result = operation->emitOpError(
          "payload projection evidence is legal only on ac.transform");
  });
  if (failed(result))
    return failure();
  auto kind = model->getAttrOfType<StringAttr>("ac.model_kind");
  if (hasEvidence && (!kind || kind.getValue() != "queue_graph" ||
                      !model->getAttrOfType<StringAttr>("ac.system") ||
                      !model.getOps<ac::SystemOp>().empty() ||
                      !model.getOps<ac::ModuleOp>().empty()))
    return model.emitError(
        "payload projection evidence requires one flat QueueGraph system");
  model.walk([&](ac::TransformOp transform) {
    if (failed(result))
      return WalkResult::interrupt();
    if (failed(verifyProjectionEvidence(transform, matchedConsumers))) {
      result = failure();
      return WalkResult::interrupt();
    }
    return WalkResult::advance();
  });
  if (failed(result))
    return failure();
  model.walk([&](ac::TransformOp transform) {
    if (transform->hasAttr("ac.payload_projections_in") &&
        !matchedConsumers.contains(transform.getOperation()))
      result = transform.emitOpError(
          "orphan private payload projection input evidence");
  });
  return result;
}

std::unique_ptr<Pass> createPruneInternalPayloadsPass() {
  return std::make_unique<PruneInternalPayloadsPass>();
}

} // namespace acir
