#include "pyc/Dialect/PYC/PYCOps.h"
#include "pyc/Dialect/PYC/PYCAttributes.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "acir/Dialect/ACIR/ACIRTypes.h"

#include "pyc/Dialect/PYC/PYCDialect.h"
#include "pyc/Dialect/PYC/PYCTypes.h"
#include "pyc/Generated/SemanticPrimitiveRegistry.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/OpImplementation.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Types.h"
#include "mlir/Support/LogicalResult.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

#include <optional>

using namespace mlir;
using namespace pyc;

acir::ac::DependentArgumentsAttr
pyc::dependentArgumentsFromStatic(acir::ac::StaticArgumentsAttr arguments) {
  MLIRContext *context = arguments.getContext();
  SmallVector<Attribute> converted;
  converted.reserve(arguments.getArguments().size());
  for (auto argument :
       arguments.getArguments().getAsRange<acir::ac::StaticArgumentAttr>()) {
    auto literal = acir::ac::DependentStaticLiteralAttr::get(
        context, argument.getValue());
    auto value = acir::ac::DependentValueAttr::get(context, literal);
    converted.push_back(acir::ac::DependentArgumentAttr::get(
        context, argument.getName(), value));
  }
  return acir::ac::DependentArgumentsAttr::get(
      context, ArrayAttr::get(context, converted));
}

FailureOr<acir::ac::StaticArgumentsAttr>
pyc::staticArgumentsFromDependent(
    acir::ac::DependentArgumentsAttr arguments) {
  MLIRContext *context = arguments.getContext();
  SmallVector<Attribute> converted;
  converted.reserve(arguments.getArguments().size());
  for (auto argument : arguments.getArguments().getAsRange<
           acir::ac::DependentArgumentAttr>()) {
    auto literal = dyn_cast<acir::ac::DependentStaticLiteralAttr>(
        argument.getValue().getValue());
    if (!literal)
      return failure();
    converted.push_back(acir::ac::StaticArgumentAttr::get(
        context, argument.getName(), literal.getValue()));
  }
  return acir::ac::StaticArgumentsAttr::get(
      context, ArrayAttr::get(context, converted));
}

template <typename Element>
static LogicalResult verifyTypedArray(
    llvm::function_ref<InFlightDiagnostic()> emitError, ArrayAttr values,
    StringRef name, bool nonEmpty = false) {
  if (!values || (nonEmpty && values.empty()))
    return emitError() << name << " must be a typed ordered array";
  for (Attribute value : values)
    if (!isa<Element>(value))
      return emitError() << name << " contains an invalid element";
  return success();
}

LogicalResult ProjectionStepAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, Attribute value) {
  return isa<ProjectionFieldAttr, ProjectionTupleElementAttr,
             ProjectionArrayElementAttr>(value)
             ? success()
             : emitError() << "projection step has an unsupported record";
}
LogicalResult ProjectionFieldAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, StringAttr name) {
  return name && !name.empty() ? success()
                               : emitError() << "projection field requires a name";
}
LogicalResult ProjectionPathAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, ArrayAttr steps) {
  return verifyTypedArray<ProjectionStepAttr>(emitError, steps,
                                               "projection path");
}
LogicalResult PackedLeafAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, ProjectionPathAttr path,
    acir::ac::TypeExprAttr logicalType, llvm::APInt lsb, llvm::APInt width) {
  if (!path || !logicalType || lsb.isNegative() || width.isNegative() ||
      width.isZero())
    return emitError() << "packed leaf interval is invalid";
  return success();
}
LogicalResult LayoutAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, llvm::APInt width,
    ArrayAttr leaves) {
  if (width.isNegative() || width.isZero() ||
      failed(verifyTypedArray<PackedLeafAttr>(
                        emitError, leaves, "layout leaves", true)))
    return failure();
  llvm::APInt next(width.getBitWidth(), 0);
  llvm::SmallDenseSet<Attribute> paths;
  for (auto leaf : leaves.getAsRange<PackedLeafAttr>()) {
    unsigned bits = std::max({width.getBitWidth(), next.getBitWidth(),
                              leaf.getLsb().getBitWidth(),
                              leaf.getWidth().getBitWidth()});
    llvm::APInt total = width.zextOrTrunc(bits);
    llvm::APInt offset = next.zextOrTrunc(bits);
    llvm::APInt leafOffset = leaf.getLsb().zextOrTrunc(bits);
    llvm::APInt leafWidth = leaf.getWidth().zextOrTrunc(bits);
    if (leaf.getLsb().isNegative() || leaf.getWidth().isNegative() ||
        leafOffset != offset || leafWidth.ugt(total - offset) ||
        !paths.insert(leaf.getPath()).second)
      return emitError() << "layout leaves must uniquely cover the packed width";
    next = offset + leafWidth;
  }
  unsigned bits = std::max(next.getBitWidth(), width.getBitWidth());
  return next.zextOrTrunc(bits) == width.zextOrTrunc(bits) ? success()
                       : emitError() << "layout leaves do not cover the width";
}
LogicalResult PhysicalPortAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, StringAttr direction,
    uint64_t, TypeAttr type, StringAttr role, IntegerAttr lane,
    LayoutAttr layout) {
  if (!direction || !type || !role ||
      (direction.getValue() != "input" && direction.getValue() != "result"))
    return emitError() << "physical port direction/type is invalid";
  bool laneRole = role.getValue() == "queue_valid" ||
                  role.getValue() == "queue_data";
  bool layoutRole = role.getValue() == "value" ||
                    role.getValue() == "queue_data";
  if ((!laneRole && role.getValue() != "value" &&
       role.getValue() != "queue_ready") ||
      static_cast<bool>(lane) != laneRole ||
      static_cast<bool>(layout) != layoutRole)
    return emitError() << "physical port lane/layout disagrees with its role";
  if (lane && lane.getValue().isNegative())
    return emitError() << "physical port lane must be non-negative";
  return success();
}
LogicalResult LogicalPortMappingAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, StringAttr direction,
    uint64_t, StringAttr name, acir::ac::TypeExprAttr logicalType,
    ArrayAttr carriers, acir::ac::SourceProvenanceAttr provenance) {
  if (!direction || !name || name.empty() || !logicalType || !provenance ||
      (direction.getValue() != "input" && direction.getValue() != "output") ||
      failed(verifyTypedArray<PhysicalPortAttr>(emitError, carriers,
                                                "logical carriers", true)))
    return failure();
  return success();
}
LogicalResult ImplicitControlOriginAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, StringAttr kind,
    StringAttr source, acir::ac::SourceProvenanceAttr provenance) {
  if (!kind || !source || !provenance || source.getValue() != "implicit" ||
      (kind.getValue() != "clock" && kind.getValue() != "reset"))
    return emitError() << "control origin must be implicit clock/reset";
  return success();
}
LogicalResult ControlPortMappingAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, StringAttr kind,
    uint64_t index, TypeAttr type, ImplicitControlOriginAttr origin) {
  bool clock = kind && kind.getValue() == "clock";
  bool reset = kind && kind.getValue() == "reset";
  if ((!clock && !reset) || !type || !origin || origin.getKind() != kind ||
      (clock ? index != 0 || !isa<ClockType>(type.getValue())
             : index != 1 || !isa<ResetType>(type.getValue())))
    return emitError() << "control mapping must use !pyc.clock/!pyc.reset at inputs 0/1";
  return success();
}
LogicalResult ModulePortMappingAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, ArrayAttr controls,
    ArrayAttr logicalPorts, ArrayAttr physicalInputs, ArrayAttr physicalResults) {
  if (failed(verifyTypedArray<ControlPortMappingAttr>(
          emitError, controls, "control mappings", true)) ||
      controls.size() != 2 ||
      failed(verifyTypedArray<LogicalPortMappingAttr>(
          emitError, logicalPorts, "logical mappings")) ||
      failed(verifyTypedArray<PhysicalPortAttr>(
          emitError, physicalInputs, "physical inputs")) ||
      failed(verifyTypedArray<PhysicalPortAttr>(
          emitError, physicalResults, "physical results")))
    return failure();
  return success();
}
LogicalResult ModuleCaseSignatureAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError,
    acir::ac::DependentArgumentsAttr arguments,
    acir::ac::ModuleInterfaceAttr logical, TypeAttr physical,
    ModulePortMappingAttr mapping) {
  if (!arguments || !logical || !physical ||
      !isa<FunctionType>(physical.getValue()) || !mapping)
    return emitError() << "PYC module case signature is incomplete";
  auto functionType = cast<FunctionType>(physical.getValue());
  auto controls = mapping.getControls().getAsRange<ControlPortMappingAttr>();
  if (mapping.getControls().size() != 2)
    return emitError() << "controls must be ordered clock then reset";
  auto controlIt = controls.begin();
  ControlPortMappingAttr clock = *controlIt++;
  ControlPortMappingAttr reset = *controlIt;
  if (clock.getKind().getValue() != "clock" ||
      reset.getKind().getValue() != "reset")
    return emitError() << "controls must be ordered clock then reset";
  if (functionType.getNumInputs() != mapping.getPhysicalInputs().size() + 2 ||
      functionType.getNumResults() != mapping.getPhysicalResults().size())
    return emitError() << "physical mapping arity does not match FunctionType";

  llvm::SmallDenseSet<Attribute> inputCarriers;
  llvm::SmallDenseSet<Attribute> resultCarriers;
  uint64_t expectedInput = 2;
  for (auto port : mapping.getPhysicalInputs().getAsRange<PhysicalPortAttr>()) {
    if (port.getDirection().getValue() != "input")
      return emitError() << "physical input carrier has non-input direction";
    if (port.getIndex() != expectedInput)
      return emitError() << "physical input carriers must be contiguous after controls";
    if (port.getType().getValue() != functionType.getInput(expectedInput))
      return emitError() << "physical input carrier type does not match FunctionType";
    if (!inputCarriers.insert(port).second)
      return emitError() << "physical input carriers must be unique";
    ++expectedInput;
  }
  uint64_t expectedResult = 0;
  for (auto port : mapping.getPhysicalResults().getAsRange<PhysicalPortAttr>()) {
    if (port.getDirection().getValue() != "result")
      return emitError() << "physical result carrier has non-result direction";
    if (port.getIndex() != expectedResult)
      return emitError() << "physical result carriers must be contiguous from zero";
    if (port.getType().getValue() != functionType.getResult(expectedResult))
      return emitError() << "physical result carrier " << expectedResult
                         << " type " << port.getType().getValue()
                         << " does not match FunctionType result "
                         << functionType.getResult(expectedResult);
    if (!resultCarriers.insert(port).second)
      return emitError() << "physical result carriers must be unique";
    ++expectedResult;
  }

  llvm::SmallDenseSet<Attribute> usedInputs;
  llvm::SmallDenseSet<Attribute> usedResults;
  llvm::StringMap<uint64_t> nextLogicalIndex;
  auto useCarrier = [&](PhysicalPortAttr carrier) -> LogicalResult {
    auto &available = carrier.getDirection().getValue() == "input"
                          ? inputCarriers
                          : resultCarriers;
    auto &used = carrier.getDirection().getValue() == "input" ? usedInputs
                                                                : usedResults;
    if (!available.contains(carrier) || !used.insert(carrier).second)
      return emitError() << "logical mapping reuses or invents a physical carrier";
    return success();
  };
  auto logicalPorts = logical.getPorts().getAsRange<acir::ac::InterfacePortAttr>();
  auto mappingPorts =
      mapping.getLogicalPorts().getAsRange<LogicalPortMappingAttr>();
  if (mapping.getLogicalPorts().size() != logical.getPorts().size())
    return emitError() << "mapping must cover every logical port exactly once";
  for (auto [expectedPort, mappingPort] :
       llvm::zip_equal(logicalPorts, mappingPorts)) {
    if (mappingPort.getIndex() != nextLogicalIndex[mappingPort.getDirection()]++ ||
        mappingPort.getDirection() != expectedPort.getDirection() ||
        mappingPort.getName() != expectedPort.getName() ||
        mappingPort.getLogicalType() != expectedPort.getLogicalType() ||
        mappingPort.getProvenance() != expectedPort.getProvenance())
      return emitError() << "logical mapping does not match interface declaration order";
    auto concrete = dyn_cast<acir::ac::TypeExprConcreteAttr>(
        mappingPort.getLogicalType().getValue());
    auto queue = concrete
                     ? dyn_cast<acir::ac::QueueType>(
                           concrete.getType().getValue())
                     : acir::ac::QueueType();
    const bool input = mappingPort.getDirection().getValue() == "input";
    if (!queue) {
      if (!concrete || mappingPort.getCarriers().size() != 1)
        return emitError()
               << "materialized logical value requires one physical carrier";
      auto carrier = cast<PhysicalPortAttr>(mappingPort.getCarriers()[0]);
      if (failed(useCarrier(carrier)))
        return failure();
      auto integer = dyn_cast<IntegerType>(carrier.getType().getValue());
      if (carrier.getRole().getValue() != "value" || carrier.getLane() ||
          !carrier.getLayout() ||
          carrier.getDirection().getValue() != (input ? "input" : "result") ||
          !integer || carrier.getLayout().getWidth().isNegative() ||
          carrier.getLayout().getWidth().getActiveBits() > 64 ||
          integer.getWidth() != carrier.getLayout().getWidth().getZExtValue())
        return emitError()
               << "logical value carrier must have exact direction and layout";
      continue;
    }
    uint64_t lane = 0;
    unsigned readyCount = 0;
    bool expectValid = true;
    bool sawReady = false;
    for (auto carrier :
         mappingPort.getCarriers().getAsRange<PhysicalPortAttr>()) {
      if (failed(useCarrier(carrier)))
        return failure();
      StringRef role = carrier.getRole().getValue();
      if (role == "queue_ready") {
        if (!expectValid || lane != static_cast<uint64_t>(queue.getLanes()) ||
            ++readyCount != 1 || carrier.getLane() || carrier.getLayout() ||
            carrier.getDirection().getValue() != (input ? "result" : "input"))
          return emitError() << "Queue requires one trailing shared opposite-direction ready carrier";
        sawReady = true;
        continue;
      }
      if (sawReady ||
          carrier.getDirection().getValue() != (input ? "input" : "result") ||
          !carrier.getLane() || carrier.getLane().getValue().isNegative() ||
          carrier.getLane().getValue().getZExtValue() != lane)
        return emitError() << "Queue lane carriers have wrong direction or order";
      if (expectValid) {
        if (role != "queue_valid" || carrier.getLayout() ||
            !carrier.getType().getValue().isInteger(1))
          return emitError() << "Queue lane must begin with one-bit valid";
        expectValid = false;
      } else {
        if (role != "queue_data" || !carrier.getLayout())
          return emitError() << "Queue valid must be followed by packed data";
        auto integer = dyn_cast<IntegerType>(carrier.getType().getValue());
        if (!integer || carrier.getLayout().getWidth().isNegative() ||
            carrier.getLayout().getWidth().getActiveBits() > 64 ||
            integer.getWidth() !=
                carrier.getLayout().getWidth().getZExtValue())
          return emitError() << "Queue data layout must exactly match carrier width";
        expectValid = true;
        ++lane;
      }
    }
    if (!expectValid || lane != static_cast<uint64_t>(queue.getLanes()) ||
        readyCount != 1 || queue.getRate() <= 0 ||
        queue.getRate() > queue.getLanes())
      return emitError() << "Queue carrier inventory does not match lanes/rate";
  }
  if (usedInputs.size() != inputCarriers.size() ||
      usedResults.size() != resultCarriers.size())
    return emitError() << "mapping must cover every logical and physical port exactly once";
  return success();
}

static LogicalResult verifyNominalDefinitions(
    Operation *owner, acir::ac::ModuleFamilySchemaAttr schema) {
  auto module = owner->getParentOfType<ModuleOp>();
  if (!module)
    return owner->emitError("family carrier must be contained in a module");
  for (Attribute raw : schema.getNominalDeclarations()) {
    auto reference = cast<FlatSymbolRefAttr>(raw);
    unsigned matches = 0;
    for (acir::ac::TypeScopeOp scope :
         module.getOps<acir::ac::TypeScopeOp>()) {
      for (acir::ac::EnumOp declaration :
           scope.getBody().front().getOps<acir::ac::EnumOp>())
        matches += declaration.getSymName() == reference.getValue();
      for (acir::ac::StructOp declaration :
           scope.getBody().front().getOps<acir::ac::StructOp>())
        matches += declaration.getSymName() == reference.getValue();
    }
    if (matches != 1)
      return owner->emitError(
                 "nominal declaration must resolve to one source-owned typed definition: ")
             << reference.getValue();
  }
  return success();
}

static FailureOr<uint64_t>
verifyPackedLayout(Operation *anchor, Type type, LayoutAttr layout) {
  auto module = anchor->getParentOfType<ModuleOp>();
  if (!module || !layout)
    return failure();
  SmallVector<PackedLeafAttr> expected;
  llvm::SmallDenseSet<Type> active;
  std::function<FailureOr<uint64_t>(Type, SmallVector<Attribute>)> collect =
      [&](Type current, SmallVector<Attribute> steps) -> FailureOr<uint64_t> {
    if (!active.insert(current).second)
      return failure();
    auto finish = [&](FailureOr<uint64_t> result) {
      active.erase(current);
      return result;
    };
    if (auto structure = dyn_cast<acir::ac::StructType>(current)) {
      acir::ac::StructOp declaration;
      for (acir::ac::TypeScopeOp scope :
           module.getOps<acir::ac::TypeScopeOp>())
        for (acir::ac::StructOp candidate :
             scope.getBody().front().getOps<acir::ac::StructOp>())
          if (candidate.getSymName() ==
              structure.getName().getLeafReference().getValue())
            declaration = candidate;
      if (!declaration)
        return finish(failure());
      uint64_t total = 0;
      for (Attribute rawField : declaration.getFields()) {
        auto field = dyn_cast<DictionaryAttr>(rawField);
        auto name = field ? field.getAs<StringAttr>("name") : StringAttr();
        auto fieldType = field ? field.getAs<TypeAttr>("type") : TypeAttr();
        if (!name || !fieldType)
          return finish(failure());
        auto nested = steps;
        nested.push_back(ProjectionStepAttr::get(
            current.getContext(), ProjectionFieldAttr::get(
                                      current.getContext(), name)));
        auto width = collect(fieldType.getValue(), std::move(nested));
        if (failed(width) || total > UINT64_MAX - *width)
          return finish(failure());
        total += *width;
      }
      return finish(total);
    }
    if (auto tuple = dyn_cast<TupleType>(current)) {
      uint64_t total = 0;
      for (auto [index, element] : llvm::enumerate(tuple.getTypes())) {
        auto nested = steps;
        nested.push_back(ProjectionStepAttr::get(
            current.getContext(), ProjectionTupleElementAttr::get(
                                      current.getContext(), index)));
        auto width = collect(element, std::move(nested));
        if (failed(width) || total > UINT64_MAX - *width)
          return finish(failure());
        total += *width;
      }
      return finish(total);
    }
    if (auto array = dyn_cast<acir::ac::ValueArrayType>(current)) {
      uint64_t total = 0;
      for (int64_t index = 0; index < array.getLength(); ++index) {
        auto nested = steps;
        nested.push_back(ProjectionStepAttr::get(
            current.getContext(), ProjectionArrayElementAttr::get(
                                      current.getContext(), index)));
        auto width = collect(array.getElementType(), std::move(nested));
        if (failed(width) || total > UINT64_MAX - *width)
          return finish(failure());
        total += *width;
      }
      return finish(total);
    }
    uint64_t width = 0;
    if (auto integer = dyn_cast<IntegerType>(current))
      width = integer.getWidth();
    else if (auto range = dyn_cast<acir::ac::RangeType>(current))
      width = std::max<uint64_t>(1, llvm::Log2_64_Ceil(range.getUpper() + 1));
    else if (auto enumeration = dyn_cast<acir::ac::EnumType>(current)) {
      acir::ac::EnumOp declaration;
      for (acir::ac::TypeScopeOp scope :
           module.getOps<acir::ac::TypeScopeOp>())
        for (acir::ac::EnumOp candidate :
             scope.getBody().front().getOps<acir::ac::EnumOp>())
          if (candidate.getSymName() ==
              enumeration.getName().getLeafReference().getValue())
            declaration = candidate;
      if (!declaration)
        return finish(failure());
      width = declaration.getEncodingWidthAttr()
                  ? *declaration.getEncodingWidth()
                  : std::max<uint64_t>(
                        1, llvm::Log2_64_Ceil(
                               declaration.getEnumerants().size()));
    } else {
      return finish(failure());
    }
    auto path = ProjectionPathAttr::get(
        current.getContext(), ArrayAttr::get(current.getContext(), steps));
    auto logical = acir::ac::TypeExprAttr::get(
        current.getContext(), acir::ac::TypeExprConcreteAttr::get(
                                  current.getContext(), TypeAttr::get(current)));
    uint64_t lsb = 0;
    for (PackedLeafAttr leaf : expected)
      lsb += leaf.getWidth().getZExtValue();
    expected.push_back(PackedLeafAttr::get(
        current.getContext(), path, logical, APInt(64, lsb), APInt(64, width)));
    return finish(width);
  };
  auto width = collect(type, {});
  if (failed(width) || layout.getWidth().isNegative() ||
      layout.getWidth().getActiveBits() > 64 ||
      *width != layout.getWidth().getZExtValue() ||
      expected.size() != layout.getLeaves().size())
    return failure();
  for (auto [actual, wanted] : llvm::zip_equal(
           layout.getLeaves().getAsRange<PackedLeafAttr>(), expected))
    if (actual.getPath() != wanted.getPath() ||
        actual.getLogicalType() != wanted.getLogicalType() ||
        actual.getLsb().getActiveBits() > 64 ||
        actual.getWidth().getActiveBits() > 64 ||
        actual.getLsb().getZExtValue() != wanted.getLsb().getZExtValue() ||
        actual.getWidth().getZExtValue() != wanted.getWidth().getZExtValue())
      return failure();
  return *width;
}

LogicalResult pyc::FamilyOp::verify() {
  if (!getSource() || !getSchema() || getSource() != getSchema().getSource())
    return emitOpError("source owner must match the complete family schema");
  if (failed(verifyNominalDefinitions(*this, getSchema())))
    return failure();
  if (getBody().empty())
    return emitOpError("requires one family body block");
  auto declared = getSchema().getCases().getCases();
  if (getBody().front().getOperations().size() != declared.size())
    return emitOpError("body count must exactly match the declared finite cases");
  for (Operation &operation : getBody().front())
    if (!isa<pyc::ModuleCaseOp>(operation))
      return operation.emitOpError(
          "PYC family body may contain only pyc.module.case operations");
  return success();
}

LogicalResult pyc::ModuleImportOp::verify() {
  if (!getSource() || !getSchema() || getSource() != getSchema().getSource())
    return emitOpError("source owner must match the complete family schema");
  if (failed(verifyNominalDefinitions(*this, getSchema())))
    return failure();
  auto file = getOperation()->getParentOfType<ModuleOp>();
  for (Attribute rawCase : getSchema().getCases().getCases()) {
    auto materialized = acir::ac::materializeModuleInterface(
        getSchema().getInterface(),
        cast<acir::ac::StaticArgumentsAttr>(rawCase), {}, file);
    if (!materialized)
      return emitOpError()
             << "import case interface is not concretely materializable: "
             << llvm::toString(materialized.takeError());
  }
  return success();
}

LogicalResult pyc::ModuleCaseOp::verify() {
  auto family = dyn_cast_or_null<pyc::FamilyOp>((*this)->getParentOp());
  if (!family)
    return emitOpError("must be a direct child of one pyc.module family");
  if (!getSignature() || !getSourceProvenance() || getBody().empty())
    return emitOpError("requires a typed signature, provenance, and body block");
  bool declared = llvm::any_of(
      family.getSchema().getCases().getCases(), [&](Attribute candidate) {
        return dependentArgumentsFromStatic(
                   cast<acir::ac::StaticArgumentsAttr>(candidate)) ==
               getSignature().getArguments();
      });
  if (!declared)
    return emitOpError(
        "dependent arguments do not select one declared family case");
  auto physical = cast<FunctionType>(getSignature().getPhysical().getValue());
  Block &entry = getBody().front();
  if (!llvm::equal(entry.getArgumentTypes(), physical.getInputs()))
    return emitOpError("body arguments must match the physical case inputs");
  if (entry.empty() || !isa<pyc::ReturnOp>(entry.back()))
    return emitOpError("body must end with pyc.return");
  for (LogicalPortMappingAttr logical : getSignature()
                                             .getMapping()
                                             .getLogicalPorts()
                                             .getAsRange<LogicalPortMappingAttr>()) {
    auto concrete = dyn_cast<acir::ac::TypeExprConcreteAttr>(
        logical.getLogicalType().getValue());
    if (!concrete)
      return emitOpError("logical mapping must be materialized before PYC");
    Type layoutType = concrete.getType().getValue();
    if (auto queue = dyn_cast<acir::ac::QueueType>(layoutType))
      layoutType = queue.getElementType();
    for (PhysicalPortAttr carrier :
         logical.getCarriers().getAsRange<PhysicalPortAttr>())
      if (carrier.getLayout() &&
          failed(verifyPackedLayout(*this, layoutType, carrier.getLayout())))
        return emitOpError(
            "packed layout does not recursively match the logical type");
  }
  return success();
}

LogicalResult pyc::ReturnOp::verify() {
  auto moduleCase = dyn_cast_or_null<pyc::ModuleCaseOp>((*this)->getParentOp());
  if (!moduleCase)
    return emitOpError("must directly terminate one pyc.module.case");
  auto physical =
      cast<FunctionType>(moduleCase.getSignature().getPhysical().getValue());
  if (!llvm::equal(getValues().getTypes(), physical.getResults()))
    return emitOpError("return values must match the physical case results");
  return success();
}

ParseResult ConstantOp::parse(OpAsmParser &parser, OperationState &result) {
  // Parse: `pyc.constant <integer> : <type>`
  SMLoc loc = parser.getCurrentLocation();

  // Parse the literal as an APInt (avoid consuming `: <type>` as part of the
  // attribute).
  APInt v;
  Type type;
  if (parser.parseInteger(v) || parser.parseColonType(type))
    return failure();

  auto intTy = dyn_cast<IntegerType>(type);
  if (!intTy)
    return parser.emitError(loc,
                            "pyc.constant requires an integer result type");

  // Re-type the value to match the result type width.
  if (v.getBitWidth() != (unsigned)intTy.getWidth())
    v = v.zextOrTrunc(intTy.getWidth());

  result.addAttribute("value", IntegerAttr::get(intTy, v));
  result.addTypes(type);
  return success();
}

void ConstantOp::print(OpAsmPrinter &p) {
  p << " " << getValueAttr().getValue().getZExtValue() << " : " << getType();
}

OpFoldResult ConstantOp::fold(FoldAdaptor) { return getValueAttr(); }

static std::optional<llvm::APInt> asIntAttr(Attribute a) {
  if (!a)
    return std::nullopt;
  if (auto ia = dyn_cast<IntegerAttr>(a))
    return ia.getValue();
  return std::nullopt;
}

template <typename Pred> static bool integerConstMatch(Value v, Pred pred) {
  if (!v)
    return false;
  if (auto c = v.getDefiningOp<ConstantOp>())
    return pred(c.getValueAttr().getValue());
  if (auto c = v.getDefiningOp<arith::ConstantOp>()) {
    auto attr = c.getValue();
    if (auto ia = dyn_cast<IntegerAttr>(attr))
      return pred(ia.getValue());
  }
  return false;
}

static bool isConstZero(Value v) {
  return integerConstMatch(v, [](const APInt &x) { return x.isZero(); });
}
static bool isConstOne(Value v) {
  return integerConstMatch(v, [](const APInt &x) { return x.isOne(); });
}
static bool isConstAllOnes(Value v) {
  return integerConstMatch(v, [](const APInt &x) { return x.isAllOnes(); });
}

static IntegerAttr intAttrFor(Type ty, const llvm::APInt &v) {
  auto intTy = dyn_cast<IntegerType>(ty);
  if (!intTy)
    return {};
  llvm::APInt vv = v;
  if (vv.getBitWidth() != intTy.getWidth())
    vv = vv.zextOrTrunc(intTy.getWidth());
  return IntegerAttr::get(intTy, vv);
}

static OpFoldResult foldValueIfResultTypeMatches(Value v, Type resultTy) {
  if (v && v.getType() == resultTy)
    return v;
  return {};
}

OpFoldResult AddOp::fold(FoldAdaptor adaptor) {
  auto outTy = dyn_cast<IntegerType>(getResult().getType());
  if (!outTy) {
    if (isConstZero(getLhs()))
      return foldValueIfResultTypeMatches(getRhs(), getResult().getType());
    if (isConstZero(getRhs()))
      return foldValueIfResultTypeMatches(getLhs(), getResult().getType());
    return {};
  }
  auto a = asIntAttr(adaptor.getLhs());
  auto b = asIntAttr(adaptor.getRhs());
  if (a && b)
    return intAttrFor(outTy, (*a + *b).trunc(outTy.getWidth()));
  if (a && a->isZero())
    return getRhs();
  if (b && b->isZero())
    return getLhs();
  return {};
}

OpFoldResult SubOp::fold(FoldAdaptor adaptor) {
  auto outTy = dyn_cast<IntegerType>(getResult().getType());
  if (!outTy) {
    if (isConstZero(getRhs()))
      return foldValueIfResultTypeMatches(getLhs(), getResult().getType());
    return {};
  }
  auto a = asIntAttr(adaptor.getLhs());
  auto b = asIntAttr(adaptor.getRhs());
  if (a && b)
    return intAttrFor(outTy, (*a - *b).trunc(outTy.getWidth()));
  if (b && b->isZero())
    return getLhs();
  if (getLhs() == getRhs())
    return intAttrFor(outTy, llvm::APInt(outTy.getWidth(), 0));
  return {};
}

OpFoldResult MulOp::fold(FoldAdaptor adaptor) {
  auto outTy = dyn_cast<IntegerType>(getResult().getType());
  if (!outTy) {
    if (isConstZero(getLhs()))
      return foldValueIfResultTypeMatches(getLhs(), getResult().getType());
    if (isConstOne(getLhs()))
      return foldValueIfResultTypeMatches(getRhs(), getResult().getType());
    if (isConstZero(getRhs()))
      return foldValueIfResultTypeMatches(getRhs(), getResult().getType());
    if (isConstOne(getRhs()))
      return foldValueIfResultTypeMatches(getLhs(), getResult().getType());
    return {};
  }
  auto a = asIntAttr(adaptor.getLhs());
  auto b = asIntAttr(adaptor.getRhs());
  if (a && b)
    return intAttrFor(outTy, (*a * *b).trunc(outTy.getWidth()));
  if (a) {
    if (a->isZero())
      return intAttrFor(outTy, llvm::APInt(outTy.getWidth(), 0));
    if (a->isOne())
      return getRhs();
  }
  if (b) {
    if (b->isZero())
      return intAttrFor(outTy, llvm::APInt(outTy.getWidth(), 0));
    if (b->isOne())
      return getLhs();
  }
  return {};
}

OpFoldResult UdivOp::fold(FoldAdaptor adaptor) {
  auto outTy = dyn_cast<IntegerType>(getResult().getType());
  if (!outTy) {
    if (isConstZero(getRhs()))
      return foldValueIfResultTypeMatches(getRhs(), getResult().getType());
    if (isConstOne(getRhs()))
      return foldValueIfResultTypeMatches(getLhs(), getResult().getType());
    if (isConstZero(getLhs()))
      return foldValueIfResultTypeMatches(getLhs(), getResult().getType());
    return {};
  }
  auto a = asIntAttr(adaptor.getLhs());
  auto b = asIntAttr(adaptor.getRhs());
  if (b) {
    if (b->isZero())
      return intAttrFor(outTy, llvm::APInt(outTy.getWidth(), 0));
    if (b->isOne())
      return getLhs();
  }
  if (a && a->isZero())
    return intAttrFor(outTy, llvm::APInt(outTy.getWidth(), 0));
  if (a && b)
    return intAttrFor(outTy, a->udiv(*b).trunc(outTy.getWidth()));
  return {};
}

OpFoldResult UremOp::fold(FoldAdaptor adaptor) {
  auto outTy = dyn_cast<IntegerType>(getResult().getType());
  if (!outTy) {
    if (isConstZero(getRhs()))
      return foldValueIfResultTypeMatches(getRhs(), getResult().getType());
    if (isConstZero(getLhs()))
      return foldValueIfResultTypeMatches(getLhs(), getResult().getType());
    return {};
  }
  auto a = asIntAttr(adaptor.getLhs());
  auto b = asIntAttr(adaptor.getRhs());
  if (b) {
    if (b->isZero())
      return intAttrFor(outTy, llvm::APInt(outTy.getWidth(), 0));
    if (b->isOne())
      return intAttrFor(outTy, llvm::APInt(outTy.getWidth(), 0));
  }
  if (a && a->isZero())
    return intAttrFor(outTy, llvm::APInt(outTy.getWidth(), 0));
  if (a && b)
    return intAttrFor(outTy, a->urem(*b).trunc(outTy.getWidth()));
  return {};
}

OpFoldResult SdivOp::fold(FoldAdaptor adaptor) {
  auto outTy = dyn_cast<IntegerType>(getResult().getType());
  if (!outTy) {
    if (isConstZero(getRhs()))
      return foldValueIfResultTypeMatches(getRhs(), getResult().getType());
    if (isConstOne(getRhs()))
      return foldValueIfResultTypeMatches(getLhs(), getResult().getType());
    if (isConstZero(getLhs()))
      return foldValueIfResultTypeMatches(getLhs(), getResult().getType());
    return {};
  }
  auto a = asIntAttr(adaptor.getLhs());
  auto b = asIntAttr(adaptor.getRhs());
  if (b) {
    if (b->isZero())
      return intAttrFor(outTy, llvm::APInt(outTy.getWidth(), 0));
    if (b->isOne())
      return getLhs();
  }
  if (a && a->isZero())
    return intAttrFor(outTy, llvm::APInt(outTy.getWidth(), 0));
  if (a && b)
    return intAttrFor(outTy, a->sdiv(*b).trunc(outTy.getWidth()));
  return {};
}

OpFoldResult SremOp::fold(FoldAdaptor adaptor) {
  auto outTy = dyn_cast<IntegerType>(getResult().getType());
  if (!outTy) {
    if (isConstZero(getRhs()))
      return foldValueIfResultTypeMatches(getRhs(), getResult().getType());
    if (isConstZero(getLhs()))
      return foldValueIfResultTypeMatches(getLhs(), getResult().getType());
    return {};
  }
  auto a = asIntAttr(adaptor.getLhs());
  auto b = asIntAttr(adaptor.getRhs());
  if (b) {
    if (b->isZero())
      return intAttrFor(outTy, llvm::APInt(outTy.getWidth(), 0));
    if (b->isOne())
      return intAttrFor(outTy, llvm::APInt(outTy.getWidth(), 0));
  }
  if (a && a->isZero())
    return intAttrFor(outTy, llvm::APInt(outTy.getWidth(), 0));
  if (a && b)
    return intAttrFor(outTy, a->srem(*b).trunc(outTy.getWidth()));
  return {};
}

OpFoldResult AndOp::fold(FoldAdaptor adaptor) {
  auto outTy = dyn_cast<IntegerType>(getResult().getType());
  if (!outTy) {
    if (isConstZero(getLhs()))
      return foldValueIfResultTypeMatches(getLhs(), getResult().getType());
    if (isConstAllOnes(getLhs()))
      return foldValueIfResultTypeMatches(getRhs(), getResult().getType());
    if (isConstZero(getRhs()))
      return foldValueIfResultTypeMatches(getRhs(), getResult().getType());
    if (isConstAllOnes(getRhs()))
      return foldValueIfResultTypeMatches(getLhs(), getResult().getType());
    return {};
  }
  auto a = asIntAttr(adaptor.getLhs());
  auto b = asIntAttr(adaptor.getRhs());
  if (a && b)
    return intAttrFor(outTy, (*a & *b).trunc(outTy.getWidth()));
  if (a) {
    if (a->isZero())
      return intAttrFor(outTy, *a);
    if (a->isAllOnes())
      return getRhs();
  }
  if (b) {
    if (b->isZero())
      return intAttrFor(outTy, *b);
    if (b->isAllOnes())
      return getLhs();
  }
  return {};
}

OpFoldResult OrOp::fold(FoldAdaptor adaptor) {
  auto outTy = dyn_cast<IntegerType>(getResult().getType());
  if (!outTy) {
    if (isConstZero(getLhs()))
      return foldValueIfResultTypeMatches(getRhs(), getResult().getType());
    if (isConstAllOnes(getLhs()))
      return foldValueIfResultTypeMatches(getLhs(), getResult().getType());
    if (isConstZero(getRhs()))
      return foldValueIfResultTypeMatches(getLhs(), getResult().getType());
    if (isConstAllOnes(getRhs()))
      return foldValueIfResultTypeMatches(getRhs(), getResult().getType());
    return {};
  }
  auto a = asIntAttr(adaptor.getLhs());
  auto b = asIntAttr(adaptor.getRhs());
  if (a && b)
    return intAttrFor(outTy, (*a | *b).trunc(outTy.getWidth()));
  if (a) {
    if (a->isZero())
      return getRhs();
    if (a->isAllOnes())
      return intAttrFor(outTy, *a);
  }
  if (b) {
    if (b->isZero())
      return getLhs();
    if (b->isAllOnes())
      return intAttrFor(outTy, *b);
  }
  return {};
}

OpFoldResult XorOp::fold(FoldAdaptor adaptor) {
  auto outTy = dyn_cast<IntegerType>(getResult().getType());
  if (!outTy) {
    if (isConstZero(getLhs()))
      return foldValueIfResultTypeMatches(getRhs(), getResult().getType());
    if (isConstZero(getRhs()))
      return foldValueIfResultTypeMatches(getLhs(), getResult().getType());
    return {};
  }
  auto a = asIntAttr(adaptor.getLhs());
  auto b = asIntAttr(adaptor.getRhs());
  if (a && b)
    return intAttrFor(outTy, (*a ^ *b).trunc(outTy.getWidth()));
  if (a && a->isZero())
    return getRhs();
  if (b && b->isZero())
    return getLhs();
  if (getLhs() == getRhs())
    return intAttrFor(outTy, llvm::APInt(outTy.getWidth(), 0));
  return {};
}

OpFoldResult NotOp::fold(FoldAdaptor adaptor) {
  if (auto inner = getIn().getDefiningOp<NotOp>())
    return inner.getIn();
  auto outTy = dyn_cast<IntegerType>(getResult().getType());
  if (!outTy)
    return {};
  auto a = asIntAttr(adaptor.getIn());
  if (a)
    return intAttrFor(outTy, (~(*a)).trunc(outTy.getWidth()));
  return {};
}

OpFoldResult SelectOp::fold(FoldAdaptor adaptor) {
  auto sel = asIntAttr(adaptor.getSel());
  if (sel) {
    if (sel->isZero())
      return getB();
    return getA();
  }
  if (getA() == getB())
    return getA();
  return {};
}

OpFoldResult CmpOp::fold(FoldAdaptor adaptor) {
  if (!isa<IntegerType>(getResult().getType()))
    return {};
  StringRef predicate = getPredicate();
  if (getLhs() == getRhs()) {
    bool value = predicate == "eq";
    return IntegerAttr::get(IntegerType::get(getContext(), 1), value ? 1 : 0);
  }
  auto a = asIntAttr(adaptor.getLhs());
  auto b = asIntAttr(adaptor.getRhs());
  if (a && b) {
    bool value = predicate == "eq"    ? (*a == *b)
                 : predicate == "ult" ? a->ult(*b)
                                      : a->slt(*b);
    return IntegerAttr::get(IntegerType::get(getContext(), 1), value ? 1 : 0);
  }
  return {};
}

OpFoldResult TruncOp::fold(FoldAdaptor adaptor) {
  if (getIn().getType() == getResult().getType())
    return getIn();
  if (auto z = getIn().getDefiningOp<ZextOp>()) {
    if (z.getIn().getType() == getResult().getType())
      return z.getIn();
  }
  if (auto s = getIn().getDefiningOp<SextOp>()) {
    if (s.getIn().getType() == getResult().getType())
      return s.getIn();
  }
  auto a = asIntAttr(adaptor.getIn());
  if (a) {
    auto outTy = dyn_cast<IntegerType>(getResult().getType());
    if (!outTy)
      return {};
    return intAttrFor(getResult().getType(), a->trunc(outTy.getWidth()));
  }
  return {};
}

OpFoldResult ZextOp::fold(FoldAdaptor adaptor) {
  if (getIn().getType() == getResult().getType())
    return getIn();
  auto a = asIntAttr(adaptor.getIn());
  if (a) {
    auto outTy = dyn_cast<IntegerType>(getResult().getType());
    if (!outTy)
      return {};
    return intAttrFor(getResult().getType(), a->zext(outTy.getWidth()));
  }
  return {};
}

OpFoldResult SextOp::fold(FoldAdaptor adaptor) {
  if (getIn().getType() == getResult().getType())
    return getIn();
  auto a = asIntAttr(adaptor.getIn());
  if (a) {
    auto outTy = dyn_cast<IntegerType>(getResult().getType());
    if (!outTy)
      return {};
    return intAttrFor(getResult().getType(), a->sext(outTy.getWidth()));
  }
  return {};
}

OpFoldResult ExtractOp::fold(FoldAdaptor adaptor) {
  auto inTy = dyn_cast<IntegerType>(getIn().getType());
  auto outTy = dyn_cast<IntegerType>(getResult().getType());
  if (!inTy || !outTy)
    return {};
  std::int64_t lsb = getLsbAttr().getInt();
  if (lsb == 0 && outTy.getWidth() == inTy.getWidth())
    return getIn();
  if (auto c = getIn().getDefiningOp<ConcatOp>()) {
    auto cTy = cast<IntegerType>(c.getResult().getType());
    std::int64_t pos = static_cast<std::int64_t>(cTy.getWidth());
    for (Value v : c.getInputs()) {
      auto vTy = cast<IntegerType>(v.getType());
      pos -= static_cast<std::int64_t>(vTy.getWidth());
      if (pos == lsb && vTy.getWidth() == outTy.getWidth())
        return v;
    }
  }
  auto a = asIntAttr(adaptor.getIn());
  if (a) {
    llvm::APInt shifted = a->lshr(static_cast<unsigned>(lsb));
    llvm::APInt sliced = shifted.trunc(outTy.getWidth());
    return intAttrFor(getResult().getType(), sliced);
  }
  return {};
}

static OpFoldResult foldShift(Value input, Attribute inputAttr,
                              Attribute amountAttr, Type resultType,
                              StringRef kind) {
  auto amount = asIntAttr(amountAttr);
  if (!amount)
    return {};
  uint64_t shift = amount->getLimitedValue();
  if (shift == 0)
    return input;
  auto outTy = dyn_cast<IntegerType>(resultType);
  if (!outTy)
    return {};
  auto value = asIntAttr(inputAttr);
  if (shift >= outTy.getWidth()) {
    if (kind != "ashr")
      return intAttrFor(resultType, llvm::APInt(outTy.getWidth(), 0));
    if (!value)
      return {};
    return intAttrFor(resultType,
                      value->isNegative()
                          ? llvm::APInt::getAllOnes(outTy.getWidth())
                          : llvm::APInt(outTy.getWidth(), 0));
  }
  if (!value)
    return {};
  llvm::APInt result = kind == "shl"    ? (*value << shift)
                       : kind == "lshr" ? value->lshr(shift)
                                        : value->ashr(shift);
  return intAttrFor(resultType, result.trunc(outTy.getWidth()));
}

OpFoldResult ShlOp::fold(FoldAdaptor adaptor) {
  return foldShift(getIn(), adaptor.getIn(), adaptor.getAmount(),
                   getResult().getType(), "shl");
}

OpFoldResult LshrOp::fold(FoldAdaptor adaptor) {
  return foldShift(getIn(), adaptor.getIn(), adaptor.getAmount(),
                   getResult().getType(), "lshr");
}

OpFoldResult AshrOp::fold(FoldAdaptor adaptor) {
  return foldShift(getIn(), adaptor.getIn(), adaptor.getAmount(),
                   getResult().getType(), "ashr");
}

OpFoldResult ConcatOp::fold(FoldAdaptor adaptor) {
  if (getInputs().size() == 1)
    return getInputs().front();

  auto outTy = dyn_cast<IntegerType>(getResult().getType());
  if (!outTy)
    return {};
  llvm::APInt acc(outTy.getWidth(), 0);

  bool allConst = true;
  unsigned offset = outTy.getWidth();
  for (auto [v, a] : llvm::zip(getInputs(), adaptor.getInputs())) {
    auto inTy = cast<IntegerType>(v.getType());
    offset -= inTy.getWidth();
    auto av = asIntAttr(a);
    if (!av) {
      allConst = false;
      break;
    }
    llvm::APInt piece = av->zextOrTrunc(inTy.getWidth());
    acc.insertBits(piece, offset);
  }
  if (allConst)
    return intAttrFor(getResult().getType(), acc);

  return {};
}

OpFoldResult AliasOp::fold(FoldAdaptor) {
  // Preserve alias ops that carry a debug name (used for codegen name
  // mangling).
  if (auto nAttr = (*this)->getAttrOfType<StringAttr>("pyc.name"))
    return {};
  return getIn();
}

LogicalResult SelectOp::verify() {
  if (getA().getType() != getB().getType())
    return emitOpError("selected values must have the same integer type");
  if (getResult().getType() != getA().getType())
    return emitOpError("result type must match the selected value type");
  return success();
}

LogicalResult NotOp::verify() {
  if (getIn().getType() != getResult().getType())
    return emitOpError("result type must match input type");
  return success();
}

static LogicalResult verifyIntCast(Operation *op, Type inTyRaw, Type outTyRaw,
                                   bool requireWiden, bool signExtend) {
  (void)signExtend;
  auto inTy = dyn_cast<IntegerType>(inTyRaw);
  auto outTy = dyn_cast<IntegerType>(outTyRaw);
  if (!inTy || !outTy)
    return op->emitOpError("only supports scalar integer types");
  if (requireWiden) {
    if (outTy.getWidth() < inTy.getWidth())
      return op->emitOpError("result width must be >= input width");
  } else {
    if (outTy.getWidth() > inTy.getWidth())
      return op->emitOpError("result width must be <= input width");
  }
  return success();
}

LogicalResult TruncOp::verify() {
  return verifyIntCast(*this, getIn().getType(), getResult().getType(),
                       /*requireWiden=*/false, /*signExtend=*/false);
}

LogicalResult ZextOp::verify() {
  return verifyIntCast(*this, getIn().getType(), getResult().getType(),
                       /*requireWiden=*/true, /*signExtend=*/false);
}

LogicalResult SextOp::verify() {
  return verifyIntCast(*this, getIn().getType(), getResult().getType(),
                       /*requireWiden=*/true, /*signExtend=*/true);
}

LogicalResult ExtractOp::verify() {
  auto inTy = dyn_cast<IntegerType>(getIn().getType());
  auto outTy = dyn_cast<IntegerType>(getResult().getType());
  if (!inTy || !outTy)
    return emitOpError("only supports scalar integer types");
  if (outTy.getWidth() == 0)
    return emitOpError("result width must be > 0");
  std::int64_t lsb = getLsbAttr().getInt();
  if (lsb < 0)
    return emitOpError("lsb must be >= 0");
  if (static_cast<std::uint64_t>(lsb) +
          static_cast<std::uint64_t>(outTy.getWidth()) >
      static_cast<std::uint64_t>(inTy.getWidth()))
    return emitOpError("slice out of range for input type");
  if (auto msbAttr = getMsbAttr()) {
    std::int64_t msb = msbAttr.getInt();
    std::int64_t expected =
        lsb + static_cast<std::int64_t>(outTy.getWidth()) - 1;
    if (msb != expected)
      return emitOpError("msb must equal lsb + result_width - 1 (expected ")
             << expected << ", got " << msb << ")";
  }
  return success();
}

static LogicalResult verifyDynShift(Operation *op, Type inTyRaw, Type amtTyRaw,
                                    Type outTyRaw) {
  if (!isa<IntegerType>(inTyRaw) || !isa<IntegerType>(amtTyRaw) ||
      !isa<IntegerType>(outTyRaw))
    return op->emitOpError("only supports scalar integer types");
  if (outTyRaw != inTyRaw)
    return op->emitOpError("result type must match input type");
  return success();
}

LogicalResult ShlOp::verify() {
  return verifyDynShift(*this, getIn().getType(), getAmount().getType(),
                        getResult().getType());
}

LogicalResult LshrOp::verify() {
  return verifyDynShift(*this, getIn().getType(), getAmount().getType(),
                        getResult().getType());
}

LogicalResult AshrOp::verify() {
  return verifyDynShift(*this, getIn().getType(), getAmount().getType(),
                        getResult().getType());
}

LogicalResult ConcatOp::verify() {
  if (getInputs().empty())
    return emitOpError("requires at least one input");

  auto outTy = dyn_cast<IntegerType>(getResult().getType());
  if (!outTy)
    return emitOpError("only supports integer result types");

  std::uint64_t sum = 0;
  for (Value v : getInputs()) {
    auto ty = dyn_cast<IntegerType>(v.getType());
    if (!ty)
      return emitOpError("only supports integer input types");
    sum += static_cast<std::uint64_t>(ty.getWidth());
  }

  if (sum != static_cast<std::uint64_t>(outTy.getWidth()))
    return emitOpError("result width must equal sum of input widths");

  return success();
}

LogicalResult PriorityEncodeOp::verify() {
  auto inputType = dyn_cast<IntegerType>(getIn().getType());
  auto indexType = dyn_cast<IntegerType>(getIndex().getType());
  if (!inputType || !indexType)
    return emitOpError("input and index result must be integer types");
  const unsigned inputWidth = inputType.getWidth();
  const auto *contract =
      generated::findSemanticPrimitive("pyc.priority_encode.v1");
  if (!contract)
    return emitOpError("semantic primitive is missing from the registry");
  if (!generated::supportsInputWidth(*contract, inputWidth))
    return emitOpError("input width must be in the shared backend range 1..64");
  const unsigned indexWidth =
      generated::outputWidth(*contract, "index", inputWidth);
  if (indexType.getWidth() != indexWidth)
    return emitOpError() << "index result width must be max(1, ceil(log2("
                         << inputWidth << "))) = " << indexWidth;
  if (!generated::enumAllows(*contract, "order", getOrder()))
    return emitOpError("order must be \"low\" or \"high\"");
  return success();
}

LogicalResult PopcountOp::verify() {
  auto inputType = dyn_cast<IntegerType>(getIn().getType());
  auto countType = dyn_cast<IntegerType>(getCount().getType());
  if (!inputType || !countType)
    return emitOpError("input and count result must be integer types");
  const auto *contract = generated::findSemanticPrimitive("pyc.popcount.v1");
  if (!contract)
    return emitOpError("semantic primitive is missing from the registry");
  if (!generated::supportsInputWidth(*contract, inputType.getWidth()))
    return emitOpError("input width must be in the shared backend range 1..64");
  const unsigned expectedWidth =
      generated::outputWidth(*contract, "count", inputType.getWidth());
  if (countType.getWidth() != expectedWidth)
    return emitOpError()
           << "count result width must be max(1, ceil(log2(N+1))) = "
           << expectedWidth;
  return success();
}

LogicalResult CountZerosOp::verify() {
  auto inputType = dyn_cast<IntegerType>(getIn().getType());
  auto countType = dyn_cast<IntegerType>(getCount().getType());
  if (!inputType || !countType)
    return emitOpError("input and count result must be integer types");
  const auto *contract = generated::findSemanticPrimitive("pyc.count_zeros.v1");
  if (!contract)
    return emitOpError("semantic primitive is missing from the registry");
  if (!generated::supportsInputWidth(*contract, inputType.getWidth()))
    return emitOpError("input width must be in the shared backend range 1..64");
  const unsigned expectedWidth =
      generated::outputWidth(*contract, "count", inputType.getWidth());
  if (countType.getWidth() != expectedWidth)
    return emitOpError()
           << "count result width must be max(1, ceil(log2(N+1))) = "
           << expectedWidth;
  if (!generated::enumAllows(*contract, "direction", getDirection()))
    return emitOpError("direction must be \"leading\" or \"trailing\"");
  return success();
}

static bool isRtlIdentifier(llvm::StringRef value) {
  if (value.empty() || !(llvm::isAlpha(value.front()) || value.front() == '_'))
    return false;
  return llvm::all_of(value.drop_front(), [](char c) {
    return llvm::isAlnum(c) || c == '_' || c == '$';
  });
}

LogicalResult RtlCombOp::verify() {
  if (getInputs().empty() || getOutputs().empty())
    return emitOpError(
        "selected combinational RTL requires inputs and outputs");
  auto semantic = (*this)->getAttrOfType<StringAttr>("semantic_id");
  auto implementation = (*this)->getAttrOfType<StringAttr>("implementation_id");
  auto module = (*this)->getAttrOfType<StringAttr>("module");
  auto parameters = (*this)->getAttrOfType<DictionaryAttr>("parameters");
  auto inputPorts = (*this)->getAttrOfType<ArrayAttr>("input_ports");
  auto outputPorts = (*this)->getAttrOfType<ArrayAttr>("output_ports");
  auto sources = (*this)->getAttrOfType<ArrayAttr>("sources");
  if (!semantic || !semantic.getValue().starts_with("pyc.") ||
      semantic.getValue().size() <= 4)
    return emitOpError("semantic_id must be a non-empty pyc.* identifier");
  if (!implementation || implementation.getValue().empty())
    return emitOpError("implementation_id must be non-empty");
  if (!module || !isRtlIdentifier(module.getValue()))
    return emitOpError("module must be a Verilog identifier");
  if (!parameters)
    return emitOpError("parameters must be present");
  for (NamedAttribute parameter : parameters) {
    if (!isRtlIdentifier(parameter.getName().strref()))
      return emitOpError() << "parameter '" << parameter.getName()
                           << "' is not a Verilog identifier";
    auto value = dyn_cast<IntegerAttr>(parameter.getValue());
    if (!value)
      return emitOpError() << "parameter '" << parameter.getName()
                           << "' must be an integer attribute";
    if (value.getInt() < 0)
      return emitOpError() << "parameter '" << parameter.getName()
                           << "' must be non-negative";
  }

  llvm::StringSet<> allPorts;
  auto verifyPorts = [&](ArrayAttr ports, size_t arity,
                         llvm::StringRef kind) -> LogicalResult {
    if (!ports || ports.size() != arity)
      return emitOpError() << kind << "_ports arity must match " << kind
                           << " value arity";
    llvm::StringSet<> seen;
    for (Attribute raw : ports) {
      auto port = dyn_cast<StringAttr>(raw);
      if (!port || !isRtlIdentifier(port.getValue()))
        return emitOpError()
               << kind << "_ports must contain Verilog identifiers";
      if (!seen.insert(port.getValue()).second)
        return emitOpError() << kind << "_ports must be unique";
      if (!allPorts.insert(port.getValue()).second)
        return emitOpError("input and output port names must be disjoint");
    }
    return success();
  };
  if (failed(verifyPorts(inputPorts, getInputs().size(), "input")) ||
      failed(verifyPorts(outputPorts, getOutputs().size(), "output")))
    return failure();

  if (!sources || sources.empty())
    return emitOpError("sources must contain a non-empty dependency closure");
  llvm::StringSet<> sourcePaths;
  for (Attribute raw : sources) {
    auto source = dyn_cast<DictionaryAttr>(raw);
    auto path = source ? source.getAs<StringAttr>("path") : StringAttr();
    auto license = source ? source.getAs<StringAttr>("license") : StringAttr();
    if (!path || !license || license.getValue().empty())
      return emitOpError("each source requires path and license strings");
    llvm::StringRef value = path.getValue();
    bool escapes = false;
    for (auto part = llvm::sys::path::begin(value),
              end = llvm::sys::path::end(value);
         part != end; ++part)
      escapes |= *part == "..";
    if (value.empty() || llvm::sys::path::is_absolute(value) ||
        value.contains("\\") || escapes)
      return emitOpError("source paths must be normalized relative paths");
    if (!sourcePaths.insert(value).second)
      return emitOpError("source paths must be unique");
  }
  return success();
}

LogicalResult AssignOp::verify() {
  if (!getDst().getDefiningOp<WireOp>())
    return emitOpError("dst must be defined by pyc.wire");
  return success();
}

LogicalResult RegOp::verify() {
  auto nextTy = getNext().getType();
  if (getInit().getType() != nextTy)
    return emitOpError("init type must match next type");
  if (getQ().getType() != nextTy)
    return emitOpError("result type must match next type");
  return success();
}

LogicalResult FifoOp::verify() {
  auto inTy = getInData().getType();
  auto outTy = getOutData().getType();
  if (inTy != outTy)
    return emitOpError("out_data type must match in_data type");
  auto depthAttr = (*this)->getAttrOfType<IntegerAttr>("depth");
  if (!depthAttr)
    return emitOpError("requires integer attribute `depth`");
  if (depthAttr.getValue().getSExtValue() <= 0)
    return emitOpError("depth must be > 0");
  return success();
}

LogicalResult ByteMemOp::verify() {
  auto addrTy = dyn_cast<IntegerType>(getRaddr().getType());
  auto waddrTy = dyn_cast<IntegerType>(getWaddr().getType());
  if (!addrTy || !waddrTy)
    return emitOpError("only supports integer address types");
  if (addrTy != waddrTy)
    return emitOpError("waddr type must match raddr type");

  auto dataTy = dyn_cast<IntegerType>(getWdata().getType());
  auto rdataTy = dyn_cast<IntegerType>(getRdata().getType());
  if (!dataTy || !rdataTy)
    return emitOpError("only supports integer data types");
  if (dataTy != rdataTy)
    return emitOpError("rdata type must match wdata type");

  unsigned dataW = dataTy.getWidth();
  if (dataW == 0)
    return emitOpError("data width must be >= 1");

  auto strbTy = dyn_cast<IntegerType>(getWstrb().getType());
  if (!strbTy)
    return emitOpError("only supports integer wstrb types");
  if (strbTy.getWidth() != ((dataW + 7) / 8))
    return emitOpError("wstrb width must be ceil(data_width / 8)");

  auto depthAttr = (*this)->getAttrOfType<IntegerAttr>("depth");
  if (!depthAttr)
    return emitOpError("requires integer attribute `depth` (bytes)");
  if (depthAttr.getValue().getSExtValue() <= 0)
    return emitOpError("depth must be > 0");
  if (auto nameAttr = (*this)->getAttrOfType<StringAttr>("name")) {
    if (nameAttr.getValue().empty())
      return emitOpError("name must be non-empty when provided");
  }

  return success();
}

LogicalResult SyncMemOp::verify() {
  auto addrTy = dyn_cast<IntegerType>(getRaddr().getType());
  auto waddrTy = dyn_cast<IntegerType>(getWaddr().getType());
  if (!addrTy || !waddrTy)
    return emitOpError("only supports integer address types");
  if (addrTy != waddrTy)
    return emitOpError("waddr type must match raddr type");

  auto dataTy = dyn_cast<IntegerType>(getWdata().getType());
  auto rdataTy = dyn_cast<IntegerType>(getRdata().getType());
  if (!dataTy || !rdataTy)
    return emitOpError("only supports integer data types");
  if (dataTy != rdataTy)
    return emitOpError("rdata type must match wdata type");

  unsigned dataW = dataTy.getWidth();
  if (dataW == 0)
    return emitOpError("data width must be >= 1");

  auto strbTy = dyn_cast<IntegerType>(getWstrb().getType());
  if (!strbTy)
    return emitOpError("only supports integer wstrb types");
  if (strbTy.getWidth() != ((dataW + 7) / 8))
    return emitOpError("wstrb width must be ceil(data_width / 8)");

  auto depthAttr = (*this)->getAttrOfType<IntegerAttr>("depth");
  if (!depthAttr)
    return emitOpError("requires integer attribute `depth` (entries)");
  if (depthAttr.getValue().getSExtValue() <= 0)
    return emitOpError("depth must be > 0");
  auto liveWindow = (*this)->getAttrOfType<IntegerAttr>("live_window");
  if (!liveWindow || liveWindow.getInt() != 1)
    return emitOpError(
        "sync_mem requires the static aggressive live_window N=1");

  if (auto nameAttr = (*this)->getAttrOfType<StringAttr>("name")) {
    if (nameAttr.getValue().empty())
      return emitOpError("name must be non-empty when provided");
  }

  return success();
}

LogicalResult SyncMemDPOp::verify() {
  auto addrTy0 = dyn_cast<IntegerType>(getRaddr0().getType());
  auto addrTy1 = dyn_cast<IntegerType>(getRaddr1().getType());
  auto waddrTy = dyn_cast<IntegerType>(getWaddr().getType());
  if (!addrTy0 || !addrTy1 || !waddrTy)
    return emitOpError("only supports integer address types");
  if (addrTy0 != addrTy1 || addrTy0 != waddrTy)
    return emitOpError("raddr0/raddr1/waddr types must match");

  auto dataTy = dyn_cast<IntegerType>(getWdata().getType());
  auto rdataTy0 = dyn_cast<IntegerType>(getRdata0().getType());
  auto rdataTy1 = dyn_cast<IntegerType>(getRdata1().getType());
  if (!dataTy || !rdataTy0 || !rdataTy1)
    return emitOpError("only supports integer data types");
  if (dataTy != rdataTy0 || dataTy != rdataTy1)
    return emitOpError("rdata types must match wdata type");

  unsigned dataW = dataTy.getWidth();
  if (dataW == 0)
    return emitOpError("data width must be >= 1");

  auto strbTy = dyn_cast<IntegerType>(getWstrb().getType());
  if (!strbTy)
    return emitOpError("only supports integer wstrb types");
  if (strbTy.getWidth() != ((dataW + 7) / 8))
    return emitOpError("wstrb width must be ceil(data_width / 8)");

  auto depthAttr = (*this)->getAttrOfType<IntegerAttr>("depth");
  if (!depthAttr)
    return emitOpError("requires integer attribute `depth` (entries)");
  if (depthAttr.getValue().getSExtValue() <= 0)
    return emitOpError("depth must be > 0");
  auto liveWindow = (*this)->getAttrOfType<IntegerAttr>("live_window");
  if (!liveWindow || liveWindow.getInt() != 1)
    return emitOpError(
        "sync_mem_dp requires the static aggressive live_window N=1");

  if (auto nameAttr = (*this)->getAttrOfType<StringAttr>("name")) {
    if (nameAttr.getValue().empty())
      return emitOpError("name must be non-empty when provided");
  }

  return success();
}

LogicalResult AsyncFifoOp::verify() {
  auto inTy = getInData().getType();
  auto outTy = getOutData().getType();
  if (inTy != outTy)
    return emitOpError("out_data type must match in_data type");
  auto depthAttr = (*this)->getAttrOfType<IntegerAttr>("depth");
  if (!depthAttr)
    return emitOpError("requires integer attribute `depth`");
  std::int64_t depth = depthAttr.getValue().getSExtValue();
  if (depth < 2)
    return emitOpError("depth must be >= 2");
  // The async FIFO requires a power-of-two depth for gray-code pointers.
  std::uint64_t d = static_cast<std::uint64_t>(depth);
  if ((d & (d - 1)) != 0)
    return emitOpError("depth must be a power of two");
  return success();
}

LogicalResult CdcSyncOp::verify() {
  auto ty = dyn_cast<IntegerType>(getIn().getType());
  if (!ty)
    return emitOpError("only supports integer types");
  if (ty.getWidth() == 0 || ty.getWidth() > 64)
    return emitOpError("supports widths 1..64");
  auto stagesAttr = (*this)->getAttrOfType<IntegerAttr>("stages");
  if (stagesAttr) {
    if (stagesAttr.getValue().getSExtValue() < 1)
      return emitOpError("stages must be >= 1");
  }
  return success();
}

LogicalResult InstanceOp::verify() {
  auto calleeAttr = getCalleeAttr();
  if (!calleeAttr)
    return emitOpError("requires FlatSymbolRefAttr attribute `callee`");

  auto module = (*this)->getParentOfType<ModuleOp>();
  if (!module)
    return emitOpError("must be contained in an MLIR module");

  Operation *sym = SymbolTable::lookupSymbolIn(module, calleeAttr);
  auto family = dyn_cast_or_null<pyc::FamilyOp>(sym);
  if (!family)
    return emitOpError("callee must reference a pyc.module family");
  pyc::ModuleCaseOp selected;
  for (pyc::ModuleCaseOp candidate :
       family.getBody().front().getOps<pyc::ModuleCaseOp>())
    if (candidate.getSignature().getArguments() == getStaticArgs()) {
      selected = candidate;
      break;
    }
  if (!selected)
    return emitOpError(
        "static arguments do not select one declared family case");

  FunctionType ft = cast<FunctionType>(
      selected.getSignature().getPhysical().getValue());
  if (ft.getNumInputs() != getNumOperands())
    return emitOpError("operand count does not match callee signature");
  if (ft.getNumResults() != getNumResults())
    return emitOpError("result count does not match callee signature");

  auto isBoundaryType = [](Type type) {
    return isa<IntegerType, pyc::ClockType, pyc::ResetType>(type);
  };

  for (auto [i, ty] : llvm::enumerate(ft.getInputs())) {
    if (!isBoundaryType(ty))
      return emitOpError() << "callee input #" << i
                           << " must use a scalar PYC boundary type";
    if (getOperand(i).getType() != ty)
      return emitOpError() << "operand type mismatch at #" << i << ": got "
                           << getOperand(i).getType() << " expected " << ty;
  }
  for (auto [i, ty] : llvm::enumerate(ft.getResults())) {
    if (!isBoundaryType(ty))
      return emitOpError() << "callee result #" << i
                           << " must use a scalar PYC boundary type";
    if (getResult(i).getType() != ty)
      return emitOpError() << "result type mismatch at #" << i << ": got "
                           << getResult(i).getType() << " expected " << ty;
  }

  if (getName().empty())
    return emitOpError("name must be non-empty");

  return success();
}

LogicalResult AssertOp::verify() {
  if (auto m = getMsgAttr()) {
    if (m.getValue().empty())
      return emitOpError("msg must be non-empty when provided");
  }
  const bool architectureAssertion = getObligationIdAttr() ||
                                     getObligationKindAttr() ||
                                     getSeverityAttr() ||
                                     getSamplingKindAttr() ||
                                     getSamplingEdgeAttr() ||
                                     getSampleAnchorAttr() || getSourceAttr() ||
                                     getNdfIdsAttr();
  if (!architectureAssertion)
    return success();
  auto message = getMsgAttr();
  auto id = getObligationIdAttr();
  auto kind = getObligationKindAttr();
  auto severity = getSeverityAttr();
  auto samplingKind = getSamplingKindAttr();
  auto samplingEdge = getSamplingEdgeAttr();
  auto anchor = getSampleAnchorAttr();
  auto source = getSourceAttr();
  auto ndfIds = getNdfIdsAttr();
  if (!id || id.getValue().empty() || !kind || kind.getValue().empty() ||
      !severity || !samplingKind || !samplingEdge || !anchor ||
      anchor.getValue().empty() || !source || source.getValue().empty() ||
      !message || !ndfIds)
    return emitOpError(
        "architecture assertion requires complete ID, kind, severity, "
        "sampling, anchor, source, and message metadata");
  auto isPrintableAscii = [](llvm::StringRef value) {
    return llvm::all_of(value, [](char character) {
      const unsigned byte = static_cast<unsigned char>(character);
      return byte >= 0x20 && byte <= 0x7e;
    });
  };
  for (llvm::StringRef value :
       {message.getValue(), id.getValue(), kind.getValue(),
        severity.getValue(), samplingKind.getValue(), samplingEdge.getValue(),
        anchor.getValue(), source.getValue()})
    if (!isPrintableAscii(value))
      return emitOpError(
          "architecture assertion metadata must be printable ASCII");
  llvm::StringSet<> seenNdfIds;
  for (Attribute raw : ndfIds) {
    auto identifier = dyn_cast<StringAttr>(raw);
    if (!identifier || identifier.getValue().empty() ||
        !isPrintableAscii(identifier.getValue()) ||
        !seenNdfIds.insert(identifier.getValue()).second)
      return emitOpError(
          "architecture assertion NDF IDs must be unique printable strings");
  }
  for (char character : id.getValue())
    if (!(llvm::isAlnum(character) || character == '_' || character == '.' ||
          character == '-' || character == ':' || character == '/'))
      return emitOpError(
          "architecture assertion ID uses invalid structural-name characters");
  llvm::StringSet<> safetyKinds;
  for (llvm::StringRef supported :
       {"mutual_exclusion", "single_writer", "resource_capacity",
        "ready_valid_integrity", "transaction_atomicity", "generation_match",
        "epoch_match", "ordering", "range", "onehot0", "no_partial_commit",
        "no_stale_update", "credit_balance", "pipeline_alignment",
        "no_stale_response"})
    safetyKinds.insert(supported);
  if (!safetyKinds.contains(kind.getValue()))
    return emitOpError(
        "architecture assertion kind must be a closed phase-one safety kind");
  if (severity.getValue() != "error" && severity.getValue() != "fatal")
    return emitOpError("architecture assertion severity must be error or fatal");
  if (samplingKind.getValue() != "pre_publish" ||
      samplingEdge.getValue() != "none")
    return emitOpError(
        "phase-one architecture assertions require pre_publish sampling with "
        "the inherited firing clock");
  return success();
}

LogicalResult CombOp::verify() {
  if (getBody().empty())
    return emitOpError("requires a non-empty region");
  if (!llvm::hasSingleElement(getBody()))
    return emitOpError("requires a single block region");

  Block &b = getBody().front();
  if (b.getNumArguments() != getNumOperands())
    return emitOpError("body block argument count must match comb inputs");

  for (auto [arg, in] : llvm::zip(b.getArguments(), getInputs())) {
    if (!isa<IntegerType>(in.getType()))
      return emitOpError("comb inputs must be scalar integers");
    if (arg.getType() != in.getType())
      return emitOpError(
          "body block argument types must match comb input types");
  }

  auto yield = dyn_cast<YieldOp>(b.getTerminator());
  if (!yield)
    return emitOpError("body must terminate with pyc.yield");

  if (yield.getNumOperands() != getNumResults())
    return emitOpError("pyc.yield operand count must match comb results");

  for (auto [v, r] : llvm::zip(yield.getOperands(), getResults())) {
    if (!isa<IntegerType>(r.getType()))
      return emitOpError("comb results must be scalar integers");
    if (v.getType() != r.getType())
      return emitOpError(
          "pyc.yield operand types must match comb result types");
  }

  return success();
}

//===----------------------------------------------------------------------===//
// Scalar binary op verifiers
//===----------------------------------------------------------------------===//

static LogicalResult verifyScalarBinary(Operation *op, Type lhsTy, Type rhsTy,
                                        Type resultTy, bool compareResult) {
  if (!isa<IntegerType>(lhsTy) || !isa<IntegerType>(rhsTy) ||
      !isa<IntegerType>(resultTy))
    return op->emitOpError("operands and result must be scalar integers");
  if (lhsTy != rhsTy)
    return op->emitOpError("operand integer types must match");
  Type expected =
      compareResult ? Type(IntegerType::get(op->getContext(), 1)) : lhsTy;
  if (resultTy != expected)
    return op->emitOpError("result type must be ") << expected;
  return success();
}

#define DEFINE_VALUE_BINARY_VERIFY(OP)                                         \
  LogicalResult OP::verify() {                                                 \
    return verifyScalarBinary(getOperation(), getLhs().getType(),              \
                              getRhs().getType(), getResult().getType(),       \
                              /*compareResult=*/false);                        \
  }

DEFINE_VALUE_BINARY_VERIFY(AddOp)
DEFINE_VALUE_BINARY_VERIFY(SubOp)
DEFINE_VALUE_BINARY_VERIFY(MulOp)
DEFINE_VALUE_BINARY_VERIFY(UdivOp)
DEFINE_VALUE_BINARY_VERIFY(UremOp)
DEFINE_VALUE_BINARY_VERIFY(SdivOp)
DEFINE_VALUE_BINARY_VERIFY(SremOp)
DEFINE_VALUE_BINARY_VERIFY(AndOp)
DEFINE_VALUE_BINARY_VERIFY(OrOp)
DEFINE_VALUE_BINARY_VERIFY(XorOp)

#undef DEFINE_VALUE_BINARY_VERIFY

LogicalResult CmpOp::verify() {
  StringRef predicate = getPredicate();
  if (predicate != "eq" && predicate != "ult" && predicate != "slt")
    return emitOpError("predicate must be eq, ult, or slt");
  return verifyScalarBinary(getOperation(), getLhs().getType(),
                            getRhs().getType(), getResult().getType(),
                            /*compareResult=*/true);
}

#define GET_OP_CLASSES
#include "pyc/Dialect/PYC/PYCOps.cpp.inc"
