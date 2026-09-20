#include "acir/Dialect/ACIR/ACIROps.h"
#include "ACIROpsTestHooks.h"
#include "ProcessLowerability.h"
#include "acir/Dialect/ACIR/ACIRResources.h"
#include "acir/Dialect/ACIR/GraphRegion.h"
#include "acir/Support/PrimitiveWidths.h"

#include "mlir/Dialect/DLTI/DLTI.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Interfaces/FunctionImplementation.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/TypeSwitch.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MathExtras.h"

#include <limits>
#include <optional>
#include <set>

using namespace mlir;

namespace acir::ac {
namespace {

thread_local detail::ProcessLivenessWork *processLivenessWorkCollector =
    nullptr;

bool isPureExpressionOperation(Operation *operation) {
  return isMemoryEffectFree(operation) || isa<func::CallOp>(operation);
}

Operation *lookupGraphSymbol(Operation *from, FlatSymbolRefAttr name) {
  auto file = from->getParentOfType<mlir::ModuleOp>();
  return file ? SymbolTable::lookupSymbolIn(file, name) : nullptr;
}

/// Resolve a possibly nested static symbol reference in the enclosing graph
/// file.
///
/// `ac.module` carries both the Symbol and the SymbolTable trait, so MLIR's
/// lookupNearestSymbolFrom() searches inside the referencing module instead of
/// the builtin.module around it. Static references, nested form included,
/// resolve against the enclosing file like every other graph reference.
Operation *lookupGraphSymbolReference(Operation *from,
                                      SymbolRefAttr reference) {
  auto file = from->getParentOfType<mlir::ModuleOp>();
  return file ? SymbolTable::lookupSymbolIn(file, reference) : nullptr;
}

} // namespace

LogicalResult
WriterPriorityAttr::verify(llvm::function_ref<InFlightDiagnostic()> emitError,
                           int64_t rank) {
  if (rank < 0)
    return emitError() << "writer priority rank must be non-negative";
  return success();
}

LogicalResult StaticIntTypeAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, uint64_t width,
    bool) {
  if (width == 0)
    return emitError() << "static integer width must be positive";
  return success();
}

LogicalResult StaticIntValueAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, StaticIntTypeAttr type,
    IntegerAttr value) {
  if (!type || !value || value.getType().getIntOrFloatBitWidth() != type.getWidth())
    return emitError() << "static integer value must have its exact declared width";
  return success();
}

static bool isStaticParameterType(Attribute value);
static bool isStaticParameterValue(Attribute value);

LogicalResult StaticTypeAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, Attribute value) {
  return isStaticParameterType(value)
             ? success()
             : emitError() << "static type wrapper contains an unsupported kind";
}

LogicalResult StaticValueAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, Attribute value) {
  return isStaticParameterValue(value)
             ? success()
             : emitError() << "static value wrapper contains an unsupported kind";
}

LogicalResult StaticConstraintAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, Attribute value) {
  return isa<OneOfConstraintAttr, IntegerRangeConstraintAttr>(value)
             ? success()
             : emitError() << "static constraint wrapper contains an unsupported kind";
}

static bool staticValueMatchesType(Attribute type, Attribute value) {
  if (isa<StaticBoolTypeAttr>(type))
    return isa<StaticBoolValueAttr>(value);
  if (auto integerType = dyn_cast<StaticIntTypeAttr>(type)) {
    auto integerValue = dyn_cast<StaticIntValueAttr>(value);
    return integerValue && integerValue.getType() == integerType;
  }
  if (auto enumType = dyn_cast<StaticEnumTypeAttr>(type)) {
    auto enumValue = dyn_cast<StaticEnumValueAttr>(value);
    return enumValue &&
           enumValue.getDeclaration() == enumType.getDeclaration();
  }
  auto configType = dyn_cast<StaticConfigTypeAttr>(type);
  auto configValue = dyn_cast<StaticConfigValueAttr>(value);
  if (!configType || !configValue ||
      configValue.getDeclaration() != configType.getDeclaration())
    return false;
  ArrayAttr fields = configType.getFields().getFields();
  ArrayAttr values = configValue.getFields().getFields();
  if (fields.size() != values.size())
    return false;
  for (auto [field, item] : llvm::zip_equal(
           fields.getAsRange<StaticConfigFieldAttr>(),
           values.getAsRange<StaticConfigFieldValueAttr>()))
    if (field.getName() != item.getName() ||
        !staticValueMatchesType(field.getType(), item.getValue()))
      return false;
  return true;
}

static bool staticValueSatisfiesConstraint(Attribute value,
                                           Attribute constraint) {
  if (auto oneOf = dyn_cast<OneOfConstraintAttr>(constraint))
    return llvm::any_of(oneOf.getValues(), [&](Attribute candidate) {
      auto wrapped = dyn_cast<StaticValueAttr>(candidate);
      return wrapped && wrapped.getValue() == value;
    });
  auto range = dyn_cast<IntegerRangeConstraintAttr>(constraint);
  auto integer = dyn_cast<StaticIntValueAttr>(value);
  if (!range || !integer || integer.getType() != range.getMinimum().getType())
    return false;
  const llvm::APInt candidate = integer.getValue().getValue();
  const llvm::APInt minimum = range.getMinimum().getValue().getValue();
  const llvm::APInt maximum = range.getMaximum().getValue().getValue();
  if (integer.getType().getIsSigned())
    return candidate.sge(minimum) && candidate.sle(maximum);
  return candidate.uge(minimum) && candidate.ule(maximum);
}

LogicalResult StaticEnumTypeAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError,
    FlatSymbolRefAttr declaration) {
  if (!declaration || declaration.getValue().empty())
    return emitError() << "static enum type requires a nominal declaration";
  return success();
}

LogicalResult StaticEnumValueAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError,
    FlatSymbolRefAttr declaration, StringAttr member) {
  if (!declaration || declaration.getValue().empty() || !member ||
      member.empty())
    return emitError() << "static enum value requires a nominal declaration and member";
  return success();
}

LogicalResult StaticConfigFieldAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, StringAttr name,
    Attribute type) {
  if (!name || name.empty() || !isStaticParameterType(type))
    return emitError() << "static config field requires a name and closed static type";
  return success();
}

LogicalResult StaticConfigFieldValueAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, StringAttr name,
    Attribute value) {
  if (!name || name.empty() || !isStaticParameterValue(value))
    return emitError() << "static config field value requires a name and typed value";
  return success();
}

LogicalResult StaticConfigTypeAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError,
    FlatSymbolRefAttr declaration, StaticConfigFieldsAttr fields) {
  if (!declaration || declaration.getValue().empty() || !fields)
    return emitError() << "static config type requires a nominal declaration and fields";
  return success();
}

LogicalResult StaticConfigValueAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError,
    FlatSymbolRefAttr declaration, StaticConfigFieldValuesAttr fields) {
  if (!declaration || declaration.getValue().empty() || !fields)
    return emitError() << "static config value requires a nominal declaration and fields";
  return success();
}

template <typename Element>
static LogicalResult verifyAttributeArray(
    llvm::function_ref<InFlightDiagnostic()> emitError, ArrayAttr values,
    StringRef description, bool requireNonEmpty = false) {
  if (!values || (requireNonEmpty && values.empty()))
    return emitError() << description << " must be a non-empty ordered array";
  for (Attribute value : values)
    if (!isa<Element>(value))
      return emitError() << description << " contains an invalid element";
  return success();
}

LogicalResult StaticConfigFieldsAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, ArrayAttr fields) {
  if (failed(verifyAttributeArray<StaticConfigFieldAttr>(emitError, fields,
                                                         "static config fields")))
    return failure();
  llvm::StringSet<> names;
  for (auto field : fields.getAsRange<StaticConfigFieldAttr>())
    if (!names.insert(field.getName()).second)
      return emitError() << "static config field names must be unique";
  return success();
}

LogicalResult StaticConfigFieldValuesAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, ArrayAttr fields) {
  if (failed(verifyAttributeArray<StaticConfigFieldValueAttr>(
          emitError, fields, "static config field values")))
    return failure();
  llvm::StringSet<> names;
  for (auto field : fields.getAsRange<StaticConfigFieldValueAttr>())
    if (!names.insert(field.getName()).second)
      return emitError() << "static config field value names must be unique";
  return success();
}

LogicalResult OneOfConstraintAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, ArrayAttr values) {
  if (!values || values.empty())
    return emitError() << "one_of requires a non-empty ordered value list";
  llvm::SmallDenseSet<Attribute> unique;
  for (Attribute value : values)
    if (!unique.insert(value).second)
      return emitError() << "one_of values must be unique";
  return success();
}

LogicalResult IntegerRangeConstraintAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError,
    StaticIntValueAttr minimum, StaticIntValueAttr maximum) {
  if (!minimum || !maximum || minimum.getType() != maximum.getType())
    return emitError() << "integer_range bounds must have one exact type";
  const bool isSigned = minimum.getType().getIsSigned();
  const llvm::APInt lower = minimum.getValue().getValue();
  const llvm::APInt upper = maximum.getValue().getValue();
  if (isSigned ? lower.sgt(upper) : lower.ugt(upper))
    return emitError() << "integer_range bounds are inverted";
  return success();
}

static bool isStaticParameterType(Attribute value) {
  return isa<StaticBoolTypeAttr, StaticIntTypeAttr, StaticEnumTypeAttr,
             StaticConfigTypeAttr>(value);
}

static bool isStaticParameterValue(Attribute value) {
  return isa<StaticBoolValueAttr, StaticIntValueAttr, StaticEnumValueAttr,
             StaticConfigValueAttr>(value);
}

LogicalResult StaticParameterAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, StringAttr name,
    StaticTypeAttr type, bool required, StaticValueAttr defaultValue,
    ArrayAttr constraints, SourceProvenanceAttr provenance) {
  if (!name || name.empty() || !type ||
      !isStaticParameterType(type.getValue()) || !constraints ||
      !provenance)
    return emitError() << "static parameter record is incomplete";
  if (required && defaultValue)
    return emitError() << "required static parameter cannot have a default";
  if (!required && !defaultValue)
    return emitError() << "optional static parameter requires a typed default";
  if (defaultValue &&
      !staticValueMatchesType(type.getValue(), defaultValue.getValue()))
    return emitError() << "static parameter default must match its exact type";
  for (Attribute rawConstraint : constraints) {
    auto wrapped = dyn_cast<StaticConstraintAttr>(rawConstraint);
    if (!wrapped)
      return emitError() << "static parameter has an unsupported constraint";
    Attribute constraint = wrapped.getValue();
    if (auto oneOf = dyn_cast<OneOfConstraintAttr>(constraint)) {
      for (Attribute value : oneOf.getValues())
        if (!staticValueMatchesType(type.getValue(),
                                    cast<StaticValueAttr>(value).getValue()))
          return emitError() << "one_of value must match the parameter type";
    } else if (!isa<StaticIntTypeAttr>(type.getValue())) {
      return emitError() << "integer_range requires a static integer parameter";
    }
    if (defaultValue &&
        !staticValueSatisfiesConstraint(defaultValue.getValue(), constraint))
      return emitError() << "static parameter default violates a constraint";
  }
  return success();
}

LogicalResult StaticParametersAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError,
    ArrayAttr parameters) {
  if (failed(verifyAttributeArray<StaticParameterAttr>(
          emitError, parameters, "static parameters")))
    return failure();
  llvm::StringSet<> names;
  for (auto parameter : parameters.getAsRange<StaticParameterAttr>())
    if (!names.insert(parameter.getName()).second)
      return emitError() << "static parameter names must be unique";
  return success();
}

LogicalResult StaticArgumentsAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, ArrayAttr arguments) {
  if (failed(verifyAttributeArray<StaticArgumentAttr>(
          emitError, arguments, "static arguments")))
    return failure();
  llvm::StringSet<> names;
  for (auto argument : arguments.getAsRange<StaticArgumentAttr>()) {
    if (!names.insert(argument.getName()).second)
      return emitError() << "static argument names must be unique";
  }
  return success();
}

LogicalResult StaticArgumentAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, StringAttr name,
    StaticValueAttr value) {
  if (!name || name.empty() || !value)
    return emitError() << "static argument requires a name and exact typed value";
  return success();
}

LogicalResult StaticCasesAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, ArrayAttr cases) {
  if (failed(verifyAttributeArray<StaticArgumentsAttr>(
          emitError, cases, "static cases", true)))
    return failure();
  llvm::SmallDenseSet<Attribute> unique;
  for (Attribute item : cases)
    if (!unique.insert(item).second)
      return emitError() << "static cases must be unique";
  return success();
}

LogicalResult SourceOwnerAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError,
    StringAttr implementation, StringAttr declaration) {
  auto valid = [](StringRef value) {
    return !value.empty() && !value.starts_with('/') && !value.contains('\\') &&
           value.ends_with(".py") && !value.contains("/../") &&
           !value.starts_with("../");
  };
  if (!implementation || !declaration || !valid(implementation) ||
      !valid(declaration))
    return emitError() << "source owner paths must be normalized relative .py paths";
  return success();
}

LogicalResult SourceProvenanceAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, StringAttr path,
    uint64_t line, uint64_t column, uint64_t endLine, uint64_t endColumn) {
  if (!path || path.empty() || path.getValue().starts_with('/') ||
      path.getValue().contains('\\') || path.getValue().contains("/../") ||
      path.getValue().starts_with("../") || line == 0 || column == 0 ||
      endLine == 0 || endColumn == 0 || endLine < line ||
      (endLine == line && endColumn < column))
    return emitError() << "source provenance requires a normalized path and ordered positive span";
  return success();
}

static bool isDependentValueRecord(Attribute value) {
  return isa<DependentIntegerLiteralAttr, DependentStaticLiteralAttr,
             DependentParameterAttr, DependentFieldAttr, DependentAddAttr,
             DependentSubAttr, DependentMulAttr, DependentIndexWidthAttr,
             DependentCountWidthAttr>(value);
}

LogicalResult DependentValueAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, Attribute value) {
  return isDependentValueRecord(value)
             ? success()
             : emitError() << "dependent value wrapper contains an unsupported record";
}

LogicalResult DependentParameterAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, StringAttr name) {
  if (!name || name.empty())
    return emitError() << "dependent parameter requires a source identifier";
  return success();
}

LogicalResult DependentFieldAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError,
    DependentParameterAttr root, ArrayAttr path) {
  if (!root || !path || path.empty() ||
      llvm::any_of(path, [](Attribute item) {
        auto name = dyn_cast<StringAttr>(item);
        return !name || name.empty();
      }))
    return emitError() << "dependent field requires a root parameter and non-empty ordered identifier path";
  return success();
}

LogicalResult DependentArgumentAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, StringAttr name,
    DependentValueAttr value) {
  if (!name || name.empty() || !value)
    return emitError() << "dependent argument requires a name and typed value";
  return success();
}

LogicalResult DependentArgumentsAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, ArrayAttr arguments) {
  if (failed(verifyAttributeArray<DependentArgumentAttr>(
          emitError, arguments, "dependent arguments")))
    return failure();
  llvm::StringSet<> names;
  for (auto argument : arguments.getAsRange<DependentArgumentAttr>())
    if (!names.insert(argument.getName()).second)
      return emitError() << "dependent argument names must be unique";
  return success();
}

static bool isTypeExprRecord(Attribute value) {
  return isa<TypeExprConcreteAttr, TypeExprBitsAttr, TypeExprRangeAttr,
             TypeExprValueArrayAttr, TypeExprTupleAttr, TypeExprNominalAttr,
             TypeExprQueueAttr>(value);
}

LogicalResult TypeExprAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, Attribute value) {
  return isTypeExprRecord(value)
             ? success()
             : emitError() << "type expression wrapper contains an unsupported record";
}

LogicalResult TypeExprTupleAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, ArrayAttr elements) {
  return verifyAttributeArray<TypeExprAttr>(emitError, elements,
                                             "tuple type expressions", true);
}

LogicalResult TypeExprNominalAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError,
    SymbolRefAttr declaration, DependentArgumentsAttr arguments) {
  if (!declaration || declaration.getRootReference().empty() || !arguments)
    return emitError() << "nominal type expression requires a declaration and complete arguments";
  return success();
}

LogicalResult InterfacePortAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, StringAttr name,
    StringAttr direction, TypeExprAttr logicalType,
    SourceProvenanceAttr provenance) {
  if (!name || name.empty() || !direction ||
      (direction.getValue() != "input" && direction.getValue() != "output") ||
      !logicalType || !provenance)
    return emitError() << "module interface port record is malformed";
  return success();
}

LogicalResult ModuleInterfaceAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError, ArrayAttr ports) {
  if (failed(verifyAttributeArray<InterfacePortAttr>(
          emitError, ports, "module interface ports")))
    return failure();
  llvm::StringSet<> names;
  for (InterfacePortAttr port : ports.getAsRange<InterfacePortAttr>())
    if (!names.insert(port.getName()).second)
      return emitError() << "module interface port names must be unique";
  return success();
}

LogicalResult ModuleFamilySchemaAttr::verify(
    llvm::function_ref<InFlightDiagnostic()> emitError,
    StaticParametersAttr parameters, StaticCasesAttr cases,
    ModuleInterfaceAttr interface, SourceOwnerAttr source,
    ArrayAttr nominalDeclarations) {
  if (!parameters || !cases || !interface || !source || !nominalDeclarations)
    return emitError() << "module family schema is incomplete";
  for (Attribute nominal : nominalDeclarations)
    if (!isa<FlatSymbolRefAttr>(nominal))
      return emitError() << "nominal declaration inventory must contain symbols";
  llvm::SmallDenseSet<Attribute> uniqueNominals;
  for (Attribute nominal : nominalDeclarations)
    if (!uniqueNominals.insert(nominal).second)
      return emitError() << "nominal declaration inventory must be unique";
  ArrayAttr declarations = parameters.getParameters();
  for (auto arguments : cases.getCases().getAsRange<StaticArgumentsAttr>()) {
    ArrayAttr values = arguments.getArguments();
    if (values.size() != declarations.size())
      return emitError() << "each finite case must bind every static parameter";
    for (auto [parameter, argument] : llvm::zip_equal(
             declarations.getAsRange<StaticParameterAttr>(),
             values.getAsRange<StaticArgumentAttr>())) {
      if (parameter.getName() != argument.getName())
        return emitError() << "finite case bindings must preserve declaration order";
      if (!staticValueMatchesType(parameter.getType().getValue(),
                                  argument.getValue().getValue()))
        return emitError() << "finite case binding must match its exact parameter type";
      for (auto constraint :
           parameter.getConstraints().getAsRange<StaticConstraintAttr>())
        if (!staticValueSatisfiesConstraint(argument.getValue().getValue(),
                                            constraint.getValue()))
          return emitError() << "finite case binding violates a parameter constraint";
    }
  }
  return success();
}

static DictionaryAttr activationQueueResource(MLIRContext *context,
                                              ActivationResourceKind kind,
                                              size_t ordinal) {
  Builder builder(context);
  NamedAttrList fields;
  fields.set("kind", ActivationResourceKindAttr::get(context, kind));
  fields.set("ordinal", builder.getI64IntegerAttr(ordinal));
  return builder.getDictionaryAttr(fields);
}

static DictionaryAttr activationStateResource(MLIRContext *context,
                                              StringRef resource) {
  Builder builder(context);
  NamedAttrList fields;
  fields.set("kind", ActivationResourceKindAttr::get(
                         context, ActivationResourceKind::State));
  fields.set("resource", FlatSymbolRefAttr::get(context, resource));
  return builder.getDictionaryAttr(fields);
}

static DictionaryAttr activationSlotResource(MLIRContext *context,
                                             StringRef resource) {
  Builder builder(context);
  NamedAttrList fields;
  fields.set("kind", ActivationResourceKindAttr::get(
                         context, ActivationResourceKind::Slot));
  fields.set("resource", FlatSymbolRefAttr::get(context, resource));
  return builder.getDictionaryAttr(fields);
}

static std::optional<bool> constantVarBool(Value value) {
  auto constant = value.getDefiningOp<VarConstantOp>();
  auto integer =
      constant ? dyn_cast<IntegerAttr>(constant.getValue()) : IntegerAttr();
  if (!integer)
    return std::nullopt;
  return !integer.getValue().isZero();
}

static RuleGuardKind guardKindFor(Value value) {
  return constantVarBool(value) == true ? RuleGuardKind::Always
                                        : RuleGuardKind::Predicate;
}

static StringRef ruleEndpointStableId(Operation *operation) {
  if (auto rule = dyn_cast<RuleOp>(operation))
    return rule.getStableId();
  if (auto firing = dyn_cast<FiringOp>(operation))
    return firing.getStableId();
  if (auto transform = dyn_cast<TransformOp>(operation)) {
    auto stable = transform->getAttrOfType<StringAttr>("ac.rule_stable_id");
    return stable ? stable.getValue() : StringRef();
  }
  return {};
}

static void addWriterArbitrationFields(NamedAttrList &fields, Operation *scope,
                                       StringRef owner,
                                       WriterPriorityAttr request) {
  Builder builder(scope->getContext());
  fields.set("owner", FlatSymbolRefAttr::get(scope->getContext(), owner));
  fields.set("endpoint_stable_id",
             builder.getStringAttr(ruleEndpointStableId(scope)));
  fields.set("policy",
             WriterArbitrationPolicyAttr::get(
                 scope->getContext(), WriterArbitrationPolicy::Priority));
  fields.set("declared_rank", builder.getI64IntegerAttr(request.getRank()));
  fields.set("resolution",
             WriterArbitrationResolutionAttr::get(
                 scope->getContext(),
                 WriterArbitrationResolution::WinnerTakesTransaction));
}

static bool presenceImpliesCandidate(Value present, Value candidate) {
  return present == candidate || constantVarBool(candidate) == true;
}

static LogicalResult verifyActivationEvidence(Operation *operation,
                                              ValueRange inputs,
                                              ValueRange outputs, Region &body,
                                              bool required) {
  auto sources = operation->getAttrOfType<ArrayAttr>("ac.activation_sources");
  auto transaction =
      operation->getAttrOfType<ArrayAttr>("ac.transaction_resources");
  auto initiallyActive =
      operation->getAttrOfType<BoolAttr>("ac.initially_active");
  if (!sources && !transaction && !initiallyActive)
    return required ? operation->emitOpError(
                          "requires typed activation/transaction evidence")
                    : success();
  if (!sources || !transaction || !initiallyActive)
    return operation->emitOpError(
        "activation/transaction evidence must be complete");

  Builder builder(operation->getContext());
  SmallVector<Attribute> expectedSources;
  SmallVector<Attribute> expectedTransaction;
  for (size_t index = 0; index < inputs.size(); ++index) {
    DictionaryAttr resource = activationQueueResource(
        operation->getContext(), ActivationResourceKind::InputQueue, index);
    expectedSources.push_back(resource);
    expectedTransaction.push_back(resource);
  }
  for (size_t index = 0; index < outputs.size(); ++index) {
    DictionaryAttr resource = activationQueueResource(
        operation->getContext(), ActivationResourceKind::OutputQueue, index);
    expectedSources.push_back(resource);
    expectedTransaction.push_back(resource);
  }
  llvm::StringSet<> sourceState;
  llvm::StringSet<> transactionState;
  llvm::StringSet<> sourceSlots;
  llvm::StringSet<> transactionSlots;
  body.walk([&](Operation *nested) {
    FlatSymbolRefAttr resource;
    if (auto read = dyn_cast<TableGetOp>(nested))
      resource = read.getTableAttr();
    else if (auto match = dyn_cast<TableMatchOp>(nested))
      resource = match.getTableAttr();
    else if (auto choose = dyn_cast<TableChooseOp>(nested))
      resource = choose.getTableAttr();
    else if (auto proposal = dyn_cast<TableProposeOp>(nested)) {
      resource = proposal.getTableAttr();
      if (transactionState.insert(resource.getValue()).second)
        expectedTransaction.push_back(activationStateResource(
            operation->getContext(), resource.getValue()));
    }
    if (resource && sourceState.insert(resource.getValue()).second)
      expectedSources.push_back(activationStateResource(operation->getContext(),
                                                        resource.getValue()));
  });
  body.walk([&](SlotGetOp get) {
    if (sourceSlots.insert(get.getSlot()).second)
      expectedSources.push_back(
          activationSlotResource(operation->getContext(), get.getSlot()));
  });
  body.walk([&](SlotProposeReleaseOp release) {
    if (transactionSlots.insert(release.getSlot()).second)
      expectedTransaction.push_back(
          activationSlotResource(operation->getContext(), release.getSlot()));
  });
  if (sources != builder.getArrayAttr(expectedSources) ||
      transaction != builder.getArrayAttr(expectedTransaction) ||
      initiallyActive.getValue() != inputs.empty())
    return operation->emitOpError(
        "activation/transaction evidence must exactly match typed resources");
  return success();
}

static LogicalResult
verifyTypedRuleSummary(Operation *operation, ValueRange inputs,
                       ValueRange outputs, Region &body, ArrayAttr footprints,
                       IntegerAttr priority, StringRef prefix) {
  auto name = [&](StringRef suffix) { return prefix.str() + suffix.str(); };
  auto guard = operation->getAttrOfType<RuleGuardKindAttr>(name("guard_kind"));
  auto schedule =
      operation->getAttrOfType<RuleScheduleKindAttr>(name("schedule_kind"));
  auto checks = operation->getAttrOfType<ArrayAttr>(name("checks_typed"));
  auto effects = operation->getAttrOfType<ArrayAttr>(name("effects_typed"));
  auto presence = operation->getAttrOfType<ArrayAttr>(name("output_presence"));
  auto stateAccesses =
      operation->getAttrOfType<ArrayAttr>(name("state_accesses"));
  auto arbitration =
      operation->getAttrOfType<ArrayAttr>(name("arbitration_membership"));
  if (!guard || !schedule || !checks || !effects || !presence ||
      !stateAccesses || !arbitration || !footprints || !priority)
    return operation->emitOpError(
        "requires complete typed rule summary evidence");

  bool predicate = false;
  body.walk([&](FiringConditionOp condition) {
    auto constant = condition.getCondition().getDefiningOp<VarConstantOp>();
    auto value =
        constant ? dyn_cast<IntegerAttr>(constant.getValue()) : IntegerAttr();
    predicate |= !value || value.getValue().isZero();
  });
  const RuleGuardKind expectedGuard =
      predicate ? RuleGuardKind::Predicate : RuleGuardKind::Always;
  SmallVector<TableProposeOp> proposals;
  body.walk([&](TableProposeOp proposal) { proposals.push_back(proposal); });
  const RuleScheduleKind expectedSchedule =
      proposals.empty() ? RuleScheduleKind::Independent
                        : RuleScheduleKind::LexicalPriority;
  if (guard.getValue() != expectedGuard ||
      schedule.getValue() != expectedSchedule)
    return operation->emitOpError(
        "typed guard/schedule evidence does not match the body");

  Builder builder(operation->getContext());
  auto queueRecord = [&](StringRef kindName, Attribute kind, size_t ordinal,
                         RuleGuardKind path) {
    NamedAttrList fields;
    fields.set(kindName, kind);
    fields.set("ordinal", builder.getI64IntegerAttr(ordinal));
    fields.set("guard_kind",
               RuleGuardKindAttr::get(operation->getContext(), path));
    return builder.getDictionaryAttr(fields);
  };
  SmallVector<Attribute> expectedChecks;
  SmallVector<Attribute> expectedEffects;
  SmallVector<Attribute> expectedPresence;
  SmallVector<Value> outputPresenceValues(outputs.size());
  body.walk([&](FiringOutputOp output) {
    if (output.getOrdinal() >= 0 &&
        static_cast<size_t>(output.getOrdinal()) < outputPresenceValues.size())
      outputPresenceValues[output.getOrdinal()] = output.getWhen();
  });
  for (size_t index = 0; index < inputs.size(); ++index) {
    expectedChecks.push_back(
        queueRecord("kind",
                    RuleCheckKindAttr::get(operation->getContext(),
                                           RuleCheckKind::InputAvailable),
                    index, RuleGuardKind::Always));
    expectedEffects.push_back(
        queueRecord("kind",
                    RuleEffectKindAttr::get(operation->getContext(),
                                            RuleEffectKind::InputConsume),
                    index, expectedGuard));
  }
  for (size_t index = 0; index < outputs.size(); ++index) {
    const RuleGuardKind outputGuard =
        outputPresenceValues[index] ? guardKindFor(outputPresenceValues[index])
                                    : expectedGuard;
    expectedChecks.push_back(
        queueRecord("kind",
                    RuleCheckKindAttr::get(operation->getContext(),
                                           RuleCheckKind::OutputCapacity),
                    index, outputGuard));
    expectedEffects.push_back(
        queueRecord("kind",
                    RuleEffectKindAttr::get(operation->getContext(),
                                            RuleEffectKind::OutputProduce),
                    index, outputGuard));
    NamedAttrList output;
    output.set("ordinal", builder.getI64IntegerAttr(index));
    output.set("presence_kind", RuleOutputPresenceKindAttr::get(
                                    operation->getContext(),
                                    outputGuard == RuleGuardKind::Always
                                        ? RuleOutputPresenceKind::Always
                                        : RuleOutputPresenceKind::Predicate));
    expectedPresence.push_back(builder.getDictionaryAttr(output));
  }

  llvm::StringSet<> readSlots;
  body.walk([&](SlotGetOp get) {
    if (!readSlots.insert(get.getSlot()).second)
      return;
    NamedAttrList effect;
    effect.set("kind", RuleEffectKindAttr::get(operation->getContext(),
                                               RuleEffectKind::StateRead));
    effect.set("resource", get.getSlotAttr());
    effect.set("guard_kind", RuleGuardKindAttr::get(operation->getContext(),
                                                    RuleGuardKind::Always));
    expectedEffects.push_back(builder.getDictionaryAttr(effect));
  });
  body.walk([&](SlotProposeReleaseOp release) {
    NamedAttrList effect;
    effect.set("kind", RuleEffectKindAttr::get(operation->getContext(),
                                               RuleEffectKind::StateWrite));
    effect.set("resource", release.getSlotAttr());
    effect.set("guard_kind",
               RuleGuardKindAttr::get(operation->getContext(),
                                      guardKindFor(release.getWhen())));
    expectedEffects.push_back(builder.getDictionaryAttr(effect));
  });

  SmallVector<Attribute> expectedConflicts;
  SmallVector<Operation *> summaryStateOperations;
  body.walk([&](Operation *nested) {
    if (isa<TableGetOp, TableMatchOp, TableChooseOp, TableProposeOp>(nested))
      summaryStateOperations.push_back(nested);
  });
  if (summaryStateOperations.size() != footprints.size())
    return operation->emitOpError(
        "typed rule summary state operation count mismatch");
  for (auto [attribute, stateOperation] :
       llvm::zip_equal(footprints, summaryStateOperations)) {
    auto footprint = dyn_cast<DictionaryAttr>(attribute);
    auto access =
        footprint ? footprint.getAs<StringAttr>("access") : StringAttr();
    auto resource = footprint ? footprint.getAs<FlatSymbolRefAttr>("resource")
                              : FlatSymbolRefAttr();
    auto indexKind =
        footprint ? footprint.getAs<StringAttr>("index_kind") : StringAttr();
    auto footprintGuard = footprint
                              ? footprint.getAs<RuleGuardKindAttr>("guard_kind")
                              : RuleGuardKindAttr();
    if (!access || !resource || !indexKind || !footprintGuard)
      return operation->emitOpError(
          "typed rule summary requires valid state footprints");
    const bool read = access.getValue() == "read";
    const RuleGuardKind stateGuard =
        read ? RuleGuardKind::Always : footprintGuard.getValue();
    NamedAttrList effect;
    effect.set("kind",
               RuleEffectKindAttr::get(operation->getContext(),
                                       read ? RuleEffectKind::StateRead
                                            : RuleEffectKind::StateWrite));
    effect.set("resource", resource);
    effect.set("guard_kind",
               RuleGuardKindAttr::get(operation->getContext(), stateGuard));
    if (!read) {
      auto proposal = dyn_cast<TableProposeOp>(stateOperation);
      auto request =
          proposal
              ? proposal->getAttrOfType<WriterPriorityAttr>("ac.arbitration")
              : WriterPriorityAttr();
      if (request)
        addWriterArbitrationFields(effect, operation, resource.getValue(),
                                   request);
    }
    expectedEffects.push_back(builder.getDictionaryAttr(effect));

    NamedAttrList conflict;
    conflict.set("kind", RuleStateAccessKindAttr::get(
                             operation->getContext(),
                             read ? RuleStateAccessKind::Read
                                  : (access.getValue() == "replace"
                                         ? RuleStateAccessKind::Replace
                                         : RuleStateAccessKind::FieldWrite)));
    conflict.set("resource", resource);
    RuleIndexKind typedIndex =
        indexKind.getValue() == "static"
            ? RuleIndexKind::Static
            : (indexKind.getValue() == "dynamic" ? RuleIndexKind::Dynamic
                                                 : RuleIndexKind::All);
    conflict.set("index_kind",
                 RuleIndexKindAttr::get(operation->getContext(), typedIndex));
    conflict.set("guard_kind",
                 RuleGuardKindAttr::get(operation->getContext(), stateGuard));
    if (auto fields = footprint.getAs<ArrayAttr>("fields"))
      conflict.set("fields", fields);
    expectedConflicts.push_back(builder.getDictionaryAttr(conflict));
  }

  SmallVector<Attribute> expectedArbitration;
  llvm::StringSet<> seenResources;
  for (TableProposeOp proposal : proposals) {
    auto request =
        proposal->getAttrOfType<WriterPriorityAttr>("ac.arbitration");
    if (!request)
      continue;
    if (!seenResources.insert(proposal.getTable()).second)
      continue;
    NamedAttrList record;
    addWriterArbitrationFields(record, operation, proposal.getTable(), request);
    expectedArbitration.push_back(builder.getDictionaryAttr(record));
  }
  if (checks != builder.getArrayAttr(expectedChecks) ||
      effects != builder.getArrayAttr(expectedEffects) ||
      presence != builder.getArrayAttr(expectedPresence) ||
      stateAccesses != builder.getArrayAttr(expectedConflicts) ||
      arbitration != builder.getArrayAttr(expectedArbitration))
    return operation->emitOpError(
        "typed checks/effects/presence/state-access/arbitration summary must "
        "exactly match the body");
  return success();
}

LogicalResult verifyLoweredRuleTransformContract(TransformOp transform) {
  constexpr llvm::StringLiteral kPrefix = "ac.rule_";
  llvm::StringSet<> allowed = {
      "ac.rule_definition",     "ac.rule_stable_id",
      "ac.rule_time_domain",    "ac.rule_priority",
      "ac.rule_footprints",     "ac.rule_effects_typed",
      "ac.rule_checks_typed",   "ac.rule_output_presence",
      "ac.rule_state_accesses", "ac.rule_guard_kind",
      "ac.rule_schedule_kind",  "ac.rule_arbitration_membership",
      "ac.rule_expression_dag", "ac.rule_footprints_exact",
  };
  bool hasRuleProof = false;
  for (NamedAttribute attribute : transform->getAttrs()) {
    StringRef name = attribute.getName().getValue();
    if (!name.starts_with(kPrefix))
      continue;
    hasRuleProof = true;
    if (!allowed.contains(name))
      return transform.emitOpError()
             << "has unknown lowered-rule proof '" << name << "'";
  }
  if (!hasRuleProof)
    return success();

  auto requireString = [&](StringRef name) -> FailureOr<StringAttr> {
    auto value = transform->getAttrOfType<StringAttr>(name);
    if (!value || value.getValue().empty()) {
      transform.emitOpError()
          << "requires non-empty lowered-rule proof '" << name << "'";
      return failure();
    }
    return value;
  };
  FailureOr<StringAttr> definition = requireString("ac.rule_definition");
  FailureOr<StringAttr> stableId = requireString("ac.rule_stable_id");
  FailureOr<StringAttr> domain = requireString("ac.rule_time_domain");
  auto priority = transform->getAttrOfType<IntegerAttr>("ac.rule_priority");
  auto footprints = transform->getAttrOfType<ArrayAttr>("ac.rule_footprints");
  auto exactDAG = transform->getAttrOfType<ArrayAttr>("ac.rule_expression_dag");
  auto exactFootprints =
      transform->getAttrOfType<ArrayAttr>("ac.rule_footprints_exact");
  if (failed(definition) || failed(stableId) || failed(domain) || !priority ||
      priority.getInt() < 0 || !footprints || !exactDAG || !exactFootprints)
    return failure();
  if (transform.getInputs().empty() || transform.getOutputs().size() != 1)
    return transform.emitOpError("lowered rule requires at least one input and "
                                 "exactly one output Queue");
  if ((*domain).getValue() != "cycle" || !footprints.empty() ||
      !exactDAG.empty() || !exactFootprints.empty())
    return transform.emitOpError(
        "has invalid lowered-rule domain/footprint proof");
  auto model = transform->getParentOfType<mlir::ModuleOp>();
  auto graphDomain =
      model ? model->getAttrOfType<StringAttr>("ac.queue_graph_domain")
            : StringAttr();
  if (!graphDomain || graphDomain.getValue() != (*domain).getValue())
    return transform.emitOpError(
        "lowered-rule domain must match the exact QueueGraph domain");
  if (failed(verifyTypedRuleSummary(transform.getOperation(),
                                    transform.getInputs(),
                                    transform.getOutputs(), transform.getBody(),
                                    footprints, priority, "ac.rule_")))
    return failure();
  return verifyActivationEvidence(transform.getOperation(),
                                  transform.getInputs(), transform.getOutputs(),
                                  transform.getBody(), true);
}

static LogicalResult verifyQueueRatesAgainstDepths(Operation *operation,
                                                   ValueRange outputs,
                                                   ArrayRef<int64_t> depths) {
  if (outputs.size() != depths.size())
    return operation->emitOpError(
        "Queue rate/depth verification requires aligned outputs");
  for (auto [output, depth] : llvm::zip_equal(outputs, depths)) {
    auto queue = cast<QueueType>(output.getType());
    if (queue.getRate() > depth)
      return operation->emitOpError(
          "Queue rate must not exceed its independently declared depth");
  }
  return success();
}

LogicalResult TransformOp::verify() {
  if (getInputs().empty())
    return emitOpError("requires at least one input queue");
  if (getOutputs().empty())
    return emitOpError("requires at least one output queue");

  ArrayRef<int64_t> depths = getOutputDepthsAttr().asArrayRef();
  ArrayRef<int64_t> latencies = getOutputLatenciesAttr().asArrayRef();
  if (depths.size() != getOutputs().size())
    return emitOpError("output depth count must match result count");
  if (latencies.size() != getOutputs().size())
    return emitOpError("output latency count must match result count");
  if (llvm::any_of(depths, [](int64_t value) { return value <= 0; }))
    return emitOpError("output depths must be positive");
  if (llvm::any_of(latencies, [](int64_t value) { return value <= 0; }))
    return emitOpError("output latencies must be positive");
  if (failed(verifyQueueRatesAgainstDepths(*this, getOutputs(), depths)))
    return failure();

  Block &block = getBody().front();
  if (block.getNumArguments() != getInputs().size())
    return emitOpError("body argument count must match input queue count");
  for (size_t index = 0; index < getInputs().size(); ++index) {
    Value input = getInputs()[index];
    BlockArgument argument = block.getArgument(index);
    auto queue = cast<QueueType>(input.getType());
    Type expected = VarType::get(getContext(), queue.getElementType());
    if (argument.getType() != expected)
      return emitOpError() << "body argument " << index << " must be "
                           << expected;
  }

  for (Operation &operation : block.without_terminator()) {
    if (!isPureExpressionOperation(&operation))
      return emitOpError() << "body operation '" << operation.getName()
                           << "' must be pure";
  }

  auto yield = dyn_cast<TransformYieldOp>(block.getTerminator());
  if (!yield)
    return emitOpError("body must terminate with ac.transform.yield");
  if (yield.getValues().size() != getOutputs().size())
    return emitOpError("yielded value count must match output queue count");
  for (size_t index = 0; index < getOutputs().size(); ++index) {
    Value output = getOutputs()[index];
    Value value = yield.getValues()[index];
    auto queue = cast<QueueType>(output.getType());
    Type expected = VarType::get(getContext(), queue.getElementType());
    if (value.getType() != expected)
      return emitOpError() << "yielded value " << index << " must be "
                           << expected;
  }
  return verifyLoweredRuleTransformContract(*this);
}

static TableOp resolveTable(Operation *operation, FlatSymbolRefAttr reference);
static SlotOp resolveSlot(Operation *operation, FlatSymbolRefAttr reference);
static bool tableVisibleFrom(Operation *operation, TableOp table);
static LogicalResult verifyStaticallySafeRuleTableIndex(Operation *operation,
                                                        TableOp table,
                                                        Value index);
static LogicalResult verifyTableFields(Operation *endpoint, TableOp table,
                                       ArrayAttr fields, StringRef kind);
static FailureOr<uint64_t> tableEntryFieldCount(Operation *endpoint,
                                                TableOp table);
static bool tableWriteFieldsAreComplete(Operation *endpoint, TableOp table,
                                        ArrayAttr writeFields);

namespace {

ArrayAttr declarationFields(Operation *op);
Operation *recordDecl(Operation *from, Type type);

struct TypedExpressionNormalizer {
  Operation *scope;
  Builder builder;
  DenseMap<Value, int64_t> handles;
  DenseSet<Value> active;
  DenseMap<Operation *, int64_t> producerHandles;
  SmallVector<Attribute> nodes;

  explicit TypedExpressionNormalizer(Operation *scope)
      : scope(scope), builder(scope->getContext()) {}

  FailureOr<int64_t> addSyntheticTrue() {
    Type valueType = VarType::get(scope->getContext(), builder.getI1Type());
    for (auto [ordinal, raw] : llvm::enumerate(nodes)) {
      auto node = dyn_cast<DictionaryAttr>(raw);
      auto opcode = node ? node.getAs<RuleExpressionOpcodeAttr>("opcode")
                         : RuleExpressionOpcodeAttr();
      auto type = node ? node.getAs<TypeAttr>("result_type") : TypeAttr();
      auto attributes =
          node ? node.getAs<DictionaryAttr>("attributes") : DictionaryAttr();
      auto value =
          attributes ? attributes.getAs<IntegerAttr>("value") : IntegerAttr();
      if (opcode && opcode.getValue() == RuleExpressionOpcode::Constant &&
          type && type.getValue() == valueType && value &&
          value.getType().isInteger(1) && value.getValue().isOne())
        return static_cast<int64_t>(ordinal);
    }
    NamedAttrList attributes;
    attributes.set("value", builder.getIntegerAttr(builder.getI1Type(), 1));
    return append(RuleExpressionOpcode::Constant, valueType, {},
                  builder.getDictionaryAttr(attributes));
  }

  int64_t append(RuleExpressionOpcode opcode, Type resultType,
                 ArrayRef<int64_t> operands, DictionaryAttr attributes) {
    NamedAttrList node;
    node.set("opcode",
             RuleExpressionOpcodeAttr::get(scope->getContext(), opcode));
    node.set("result_type", TypeAttr::get(resultType));
    node.set("operands", builder.getDenseI64ArrayAttr(operands));
    node.set("attributes", attributes);
    nodes.push_back(builder.getDictionaryAttr(node));
    return static_cast<int64_t>(nodes.size() - 1);
  }

  struct OwnerIdentity {
    Attribute owner;
    Attribute stableId;
    Type valueType;
  };

  FailureOr<OwnerIdentity> ownerIdentity(Operation *endpoint,
                                         FlatSymbolRefAttr resource) {
    OwnerIdentity identity;
    const bool slotEndpoint = isa<SlotGetOp, SlotProposeReleaseOp>(endpoint);
    if (slotEndpoint) {
      if (SlotOp slot = resolveSlot(endpoint, resource))
        identity = {
            slot.getOwnerAttr(), slot.getStableIdAttr(),
            VarType::get(
                scope->getContext(),
                cast<QueueType>(slot.getInput().getType()).getElementType())};
    } else if (TableOp table = resolveTable(endpoint, resource)) {
      identity = {table.getOwnerAttr(), table.getStableIdAttr(),
                  VarType::get(scope->getContext(), table.getEntryType())};
    }
    if (!identity.owner || !identity.stableId || !identity.valueType) {
      endpoint->emitOpError("cannot resolve exact committed owner identity");
      return failure();
    }
    return identity;
  }

  FailureOr<int64_t> ensureProducerAnchor(Operation *producer,
                                          FlatSymbolRefAttr resource, Type,
                                          StringRef endpointKind) {
    if (auto found = producerHandles.find(producer);
        found != producerHandles.end())
      return found->second;
    FailureOr<OwnerIdentity> identity = ownerIdentity(producer, resource);
    if (failed(identity))
      return failure();
    NamedAttrList attributes;
    attributes.set("resource", resource);
    attributes.set("owner", identity->owner);
    attributes.set("stable_id", identity->stableId);
    attributes.set("endpoint", builder.getStringAttr(endpointKind));
    attributes.set("producer", builder.getBoolAttr(true));
    int64_t handle =
        append(RuleExpressionOpcode::CommittedState, identity->valueType, {},
               builder.getDictionaryAttr(attributes));
    producerHandles.try_emplace(producer, handle);
    return handle;
  }

  static bool isClosedOperationName(StringRef name) {
    return llvm::StringSwitch<bool>(name)
        .Cases({"ac.var.enum", "ac.var.enum_match", "ac.var.tuple"}, true)
        .Cases({"ac.var.array", "ac.var.record", "ac.var.element"}, true)
        .Cases({"ac.var.dynamic_element", "ac.var.with_element"}, true)
        .Cases({"ac.var.add", "ac.var.sub", "ac.var.mul"}, true)
        .Cases({"ac.var.udiv", "ac.var.urem", "ac.var.and"}, true)
        .Cases({"ac.var.or", "ac.var.xor", "ac.var.shl"}, true)
        .Cases({"ac.var.shr", "ac.var.matches", "ac.var.not"}, true)
        .Cases({"ac.var.popcount", "ac.var.count_zeros"}, true)
        .Cases({"ac.var.priority_encode", "ac.var.cmp"}, true)
        .Cases({"ac.var.select", "ac.var.extract", "ac.var.concat"}, true)
        .Cases({"ac.var.range_wrap", "ac.var.range_saturate"}, true)
        .Cases({"ac.var.range_checked", "ac.var.range_refine"}, true)
        .Cases({"ac.var.range_bits", "ac.var.range_add"}, true)
        .Cases({"ac.var.range_sub", "ac.var.range_cmp"}, true)
        .Cases({"ac.var.insert", "ac.var.get", "ac.var.with"}, true)
        .Case("ac.table.index", true)
        .Default(false);
  }

  static bool isClosedOperationAttribute(StringRef operation,
                                         StringRef attribute) {
    if (attribute == "ac.source_provenance")
      return true;
    if (operation == "ac.var.enum" || operation == "ac.var.enum_match")
      return attribute == "declaration" || attribute == "enumerant" ||
             attribute == "enumerants";
    if (operation == "ac.var.element")
      return attribute == "index";
    if (operation == "ac.var.matches")
      return attribute == "mask" || attribute == "value";
    if (operation == "ac.var.count_zeros")
      return attribute == "direction";
    if (operation == "ac.var.priority_encode")
      return attribute == "order";
    if (operation == "ac.var.cmp" || operation == "ac.var.range_cmp")
      return attribute == "predicate";
    if (operation == "ac.var.extract")
      return attribute == "lsb" || attribute == "width";
    if (operation == "ac.var.insert")
      return attribute == "lsb";
    if (operation == "ac.var.get" || operation == "ac.var.with")
      return attribute == "field";
    if (operation == "ac.table.index")
      return attribute == "table";
    return false;
  }

  FailureOr<SmallVector<Value>> dependencies(Value value) {
    if (auto argument = dyn_cast<BlockArgument>(value)) {
      if (argument.getOwner() != &scope->getRegion(0).front()) {
        Operation *owner = argument.getOwner()->getParentOp();
        if (auto match = dyn_cast_or_null<TableMatchOp>(owner)) {
          if (failed(ensureProducerAnchor(owner, match.getTableAttr(),
                                          value.getType(), "table_match")))
            return failure();
        } else if (auto choose = dyn_cast_or_null<TableChooseOp>(owner)) {
          if (failed(ensureProducerAnchor(owner, choose.getTableAttr(),
                                          value.getType(), "table_choose")))
            return failure();
        }
      }
      return SmallVector<Value>{};
    }
    if (value.getDefiningOp<VarConstantOp>() ||
        value.getDefiningOp<SlotGetOp>())
      return SmallVector<Value>{};
    Operation *definition = value.getDefiningOp();
    if (!definition || !scope->isAncestor(definition)) {
      scope->emitOpError(
          "rule expression leaf is mutable, external, or unrepresentable");
      return failure();
    }
    if (auto read = dyn_cast<TableGetOp>(definition))
      return SmallVector<Value>{read.getIndex()};
    if (auto match = dyn_cast<TableMatchOp>(definition)) {
      Block &predicate = match.getPredicate().front();
      auto yielded = dyn_cast<TableMatchYieldOp>(predicate.getTerminator());
      if (!yielded)
        return match.emitOpError("requires exact predicate yield"), failure();
      if (failed(ensureProducerAnchor(definition, match.getTableAttr(),
                                      predicate.getArgument(0).getType(),
                                      "table_match")))
        return failure();
      SmallVector<Value> result;
      if (match.getDomainBase())
        result.push_back(match.getDomainBase());
      result.push_back(yielded.getValue());
      return result;
    }
    if (auto choose = dyn_cast<TableChooseOp>(definition)) {
      if (failed(ensureProducerAnchor(definition, choose.getTableAttr(),
                                      value.getType(), "table_choose")))
        return failure();
      SmallVector<Value> result{choose.getMask()};
      if (!choose.getKey().empty()) {
        auto yielded = dyn_cast<TableChooseYieldOp>(
            choose.getKey().front().getTerminator());
        if (!yielded)
          return choose.emitOpError("requires exact key yield"), failure();
        result.push_back(yielded.getValue());
      }
      return result;
    }
    if (!isClosedOperationName(definition->getName().getStringRef())) {
      definition->emitOpError(
          "is not admitted in persisted rule-expression DAGs");
      return failure();
    }
    return SmallVector<Value>(definition->operand_begin(),
                              definition->operand_end());
  }

  FailureOr<int64_t> materialize(Value value) {
    auto finish = [&](int64_t handle) -> FailureOr<int64_t> {
      active.erase(value);
      handles.try_emplace(value, handle);
      return handle;
    };
    if (auto argument = dyn_cast<BlockArgument>(value)) {
      NamedAttrList attributes;
      attributes.set("ordinal",
                     builder.getI64IntegerAttr(argument.getArgNumber()));
      if (argument.getOwner() == &scope->getRegion(0).front())
        return finish(append(RuleExpressionOpcode::RuleInput, value.getType(),
                             {}, builder.getDictionaryAttr(attributes)));
      Operation *owner = argument.getOwner()->getParentOp();
      FlatSymbolRefAttr resource;
      StringRef endpoint;
      if (auto match = dyn_cast_or_null<TableMatchOp>(owner)) {
        resource = match.getTableAttr();
        endpoint = "table_match";
      } else if (auto choose = dyn_cast_or_null<TableChooseOp>(owner)) {
        resource = choose.getTableAttr();
        endpoint = "table_choose";
      } else {
        scope->emitOpError(
            "rule expression block argument is outside an admitted lane scope");
        return failure();
      }
      auto producer = producerHandles.find(owner);
      if (producer == producerHandles.end())
        return failure();
      attributes.set("resource", resource);
      attributes.set("endpoint", builder.getStringAttr(endpoint));
      return finish(append(RuleExpressionOpcode::Lane, value.getType(),
                           {producer->second},
                           builder.getDictionaryAttr(attributes)));
    }
    Operation *definition = value.getDefiningOp();
    if (auto constant = dyn_cast<VarConstantOp>(definition)) {
      NamedAttrList attributes;
      attributes.set("value", constant.getValue());
      return finish(append(RuleExpressionOpcode::Constant, value.getType(), {},
                           builder.getDictionaryAttr(attributes)));
    }
    auto appendCommitted = [&](FlatSymbolRefAttr resource,
                               ArrayRef<Value> identityOperands,
                               StringRef endpoint) -> FailureOr<int64_t> {
      auto identity = ownerIdentity(definition, resource);
      if (failed(identity))
        return failure();
      SmallVector<int64_t> operands;
      for (Value operand : identityOperands)
        operands.push_back(handles.lookup(operand));
      NamedAttrList attributes;
      attributes.set("resource", resource);
      attributes.set("owner", identity->owner);
      attributes.set("stable_id", identity->stableId);
      attributes.set("endpoint", builder.getStringAttr(endpoint));
      attributes.set(
          "result_ordinal",
          builder.getI64IntegerAttr(cast<OpResult>(value).getResultNumber()));
      return finish(append(RuleExpressionOpcode::CommittedState,
                           value.getType(), operands,
                           builder.getDictionaryAttr(attributes)));
    };
    if (auto read = dyn_cast<TableGetOp>(definition))
      return appendCommitted(read.getTableAttr(), {read.getIndex()},
                             "table_get");
    if (auto get = dyn_cast<SlotGetOp>(definition))
      return appendCommitted(get.getSlotAttr(), {}, "slot_get");
    if (auto match = dyn_cast<TableMatchOp>(definition)) {
      auto identity = ownerIdentity(definition, match.getTableAttr());
      auto yielded =
          cast<TableMatchYieldOp>(match.getPredicate().front().getTerminator());
      if (failed(identity))
        return failure();
      SmallVector<int64_t> operands{producerHandles.lookup(definition)};
      if (match.getDomainBase())
        operands.push_back(handles.lookup(match.getDomainBase()));
      operands.push_back(handles.lookup(yielded.getValue()));
      NamedAttrList attributes;
      attributes.set("resource", match.getTableAttr());
      attributes.set("owner", identity->owner);
      attributes.set("stable_id", identity->stableId);
      attributes.set("endpoint", builder.getStringAttr("table_match"));
      for (StringRef name :
           {"domain_axes", "domain_shape", "domain_strides", "domain_offset"})
        if (Attribute attribute = match->getAttr(name))
          attributes.set(name, attribute);
      attributes.set("result_ordinal", builder.getI64IntegerAttr(0));
      return finish(append(RuleExpressionOpcode::Operation, value.getType(),
                           operands, builder.getDictionaryAttr(attributes)));
    }
    if (auto choose = dyn_cast<TableChooseOp>(definition)) {
      auto identity = ownerIdentity(definition, choose.getTableAttr());
      if (failed(identity))
        return failure();
      SmallVector<int64_t> operands{producerHandles.lookup(definition),
                                    handles.lookup(choose.getMask())};
      if (!choose.getKey().empty())
        operands.push_back(handles.lookup(
            cast<TableChooseYieldOp>(choose.getKey().front().getTerminator())
                .getValue()));
      NamedAttrList attributes;
      attributes.set("resource", choose.getTableAttr());
      attributes.set("owner", identity->owner);
      attributes.set("stable_id", identity->stableId);
      attributes.set("selection_stable_id", choose.getStableIdAttr());
      attributes.set("count", choose.getCountAttr());
      attributes.set("policy", choose.getPolicyAttr());
      if (choose.getKeyOrderingAttr())
        attributes.set("key_ordering", choose.getKeyOrderingAttr());
      attributes.set("initial_cursor", choose.getInitialCursorAttr());
      attributes.set("endpoint", builder.getStringAttr("table_choose"));
      attributes.set(
          "result_ordinal",
          builder.getI64IntegerAttr(cast<OpResult>(value).getResultNumber()));
      return finish(append(RuleExpressionOpcode::Operation, value.getType(),
                           operands, builder.getDictionaryAttr(attributes)));
    }
    StringRef name = definition->getName().getStringRef();
    SmallVector<int64_t> operands;
    for (Value operand : definition->getOperands())
      operands.push_back(handles.lookup(operand));
    NamedAttrList attributes;
    attributes.set("operation", builder.getStringAttr(name));
    attributes.set(
        "result_ordinal",
        builder.getI64IntegerAttr(cast<OpResult>(value).getResultNumber()));
    for (NamedAttribute attribute : definition->getAttrs()) {
      StringRef attrName = attribute.getName().getValue();
      if (!isClosedOperationAttribute(name, attrName)) {
        definition->emitOpError()
            << "has unrepresentable rule-expression attribute '" << attrName
            << "'";
        return failure();
      }
      if (attrName != "ac.source_provenance")
        attributes.set(attrName, attribute.getValue());
    }
    return finish(append(RuleExpressionOpcode::Operation, value.getType(),
                         operands, builder.getDictionaryAttr(attributes)));
  }

  FailureOr<int64_t> normalize(Value root) {
    if (!root) {
      scope->emitOpError("cannot persist a null rule expression");
      return failure();
    }
    struct WorkItem {
      Value value;
      bool expanded;
    };
    SmallVector<WorkItem> work{{root, false}};
    while (!work.empty()) {
      WorkItem item = work.pop_back_val();
      if (handles.contains(item.value))
        continue;
      if (item.expanded) {
        if (failed(materialize(item.value)))
          return failure();
        continue;
      }
      if (!active.insert(item.value).second) {
        scope->emitOpError("live rule expression contains a cycle");
        return failure();
      }
      FailureOr<SmallVector<Value>> deps = dependencies(item.value);
      if (failed(deps))
        return failure();
      work.push_back({item.value, true});
      for (Value dependency : llvm::reverse(*deps)) {
        if (handles.contains(dependency))
          continue;
        if (active.contains(dependency)) {
          scope->emitOpError("live rule expression contains a cycle");
          return failure();
        }
        work.push_back({dependency, false});
      }
    }
    return handles.lookup(root);
  }
};

static FailureOr<Attribute> exactFootprintProvenance(Operation *endpoint) {
  Builder builder(endpoint->getContext());
  if (Attribute raw = endpoint->getAttr("ac.source_provenance")) {
    auto origins = dyn_cast<ArrayAttr>(raw);
    if (!origins || origins.empty()) {
      endpoint->emitOpError(
          "exact footprint source provenance must be a non-empty origin array");
      return failure();
    }
    for (Attribute rawOrigin : origins) {
      auto origin = dyn_cast<DictionaryAttr>(rawOrigin);
      auto frames = origin ? origin.getAs<ArrayAttr>("frames") : ArrayAttr();
      if (!origin || origin.size() != 1 || !frames || frames.empty()) {
        endpoint->emitOpError("exact footprint source provenance is malformed");
        return failure();
      }
      for (Attribute rawFrame : frames) {
        auto frame = dyn_cast<DictionaryAttr>(rawFrame);
        auto file = frame ? frame.getAs<StringAttr>("file") : StringAttr();
        auto kind = frame ? frame.getAs<StringAttr>("kind") : StringAttr();
        auto line = frame ? frame.getAs<IntegerAttr>("line") : IntegerAttr();
        auto column =
            frame ? frame.getAs<IntegerAttr>("column") : IntegerAttr();
        if (!frame || !file || file.getValue().empty() || !kind ||
            (kind.getValue() != "statement" &&
             kind.getValue() != "definition" &&
             kind.getValue() != "inline_callsite") ||
            !line || line.getInt() <= 0 || !column || column.getInt() <= 0) {
          endpoint->emitOpError(
              "exact footprint source provenance frame is malformed");
          return failure();
        }
      }
    }
    if (auto location = dyn_cast<FileLineColLoc>(endpoint->getLoc());
        location && location.getFilename().getValue().ends_with(".py")) {
      auto origin = cast<DictionaryAttr>(origins[0]);
      auto frame = cast<DictionaryAttr>(origin.getAs<ArrayAttr>("frames")[0]);
      if (frame.getAs<StringAttr>("file").getValue() !=
              location.getFilename().getValue() ||
          frame.getAs<IntegerAttr>("line").getInt() != location.getLine() ||
          frame.getAs<IntegerAttr>("column").getInt() != location.getColumn()) {
        endpoint->emitOpError(
            "endpoint source provenance disagrees with source location");
        return failure();
      }
    }
    return raw;
  }
  if (auto file = dyn_cast<FileLineColLoc>(endpoint->getLoc())) {
    StringRef filename = file.getFilename().getValue();
    if (filename.empty() || file.getLine() == 0 || file.getColumn() == 0) {
      endpoint->emitOpError("exact footprint file location is malformed");
      return failure();
    }
    // Parser-owned .mlir/.ac locations are transport locations and are not
    // stable across print/parse. Source-language FileLineColLocs are semantic
    // provenance and are preferred over rule-level fallback metadata.
    if (filename.ends_with(".py")) {
      NamedAttrList source;
      source.set("file", file.getFilename());
      source.set("line", builder.getI64IntegerAttr(file.getLine()));
      source.set("column", builder.getI64IntegerAttr(file.getColumn()));
      return builder.getDictionaryAttr(source);
    }
  }
  Operation *scope = endpoint->getParentOfType<RuleOp>();
  if (!scope)
    scope = endpoint->getParentOfType<FiringOp>();
  auto sourceFile =
      scope ? scope->getAttrOfType<StringAttr>("ac.source_file") : StringAttr();
  auto sourceLine = scope ? scope->getAttrOfType<IntegerAttr>("ac.source_line")
                          : IntegerAttr();
  auto sourceColumn =
      scope ? scope->getAttrOfType<IntegerAttr>("ac.source_column")
            : IntegerAttr();
  if (sourceFile || sourceLine || sourceColumn) {
    if (!sourceFile || sourceFile.getValue().empty() || !sourceLine ||
        sourceLine.getInt() <= 0 || !sourceColumn ||
        sourceColumn.getInt() <= 0) {
      scope->emitOpError("rule source metadata is malformed");
      return failure();
    }
    NamedAttrList source;
    source.set("file", sourceFile);
    source.set("line", sourceLine);
    source.set("column", sourceColumn);
    return builder.getDictionaryAttr(source);
  }
  return builder.getDictionaryAttr({});
}

static std::vector<std::string> projectedLaneFields(Operation *endpoint) {
  Region *region = nullptr;
  if (auto match = dyn_cast<TableMatchOp>(endpoint))
    region = &match.getPredicate();
  else if (auto choose = dyn_cast<TableChooseOp>(endpoint))
    region = &choose.getKey();
  if (!region || region->empty())
    return {};
  BlockArgument lane = region->front().getArgument(0);
  auto descendsFromLane = [&](Value root) {
    SmallVector<Value> work{root};
    DenseSet<Value> seen;
    while (!work.empty()) {
      Value current = work.pop_back_val();
      if (current == lane)
        return true;
      if (!seen.insert(current).second || isa<BlockArgument>(current))
        continue;
      Operation *definition = current.getDefiningOp();
      if (!definition ||
          isa<TableGetOp, TableMatchOp, TableChooseOp, SlotGetOp>(definition))
        continue;
      llvm::append_range(work, definition->getOperands());
    }
    return false;
  };
  llvm::StringSet<> names;
  region->walk([&](VarGetOp get) {
    if (descendsFromLane(get.getRecord()))
      names.insert(get.getField());
  });
  std::vector<std::string> fields;
  FlatSymbolRefAttr resource;
  if (auto match = dyn_cast<TableMatchOp>(endpoint))
    resource = match.getTableAttr();
  else
    resource = cast<TableChooseOp>(endpoint).getTableAttr();
  TableOp table = resolveTable(endpoint, resource);
  Operation *declaration =
      table ? recordDecl(endpoint, table.getEntryType()) : nullptr;
  if (declaration)
    for (Attribute rawField : declarationFields(declaration)) {
      auto field = cast<DictionaryAttr>(rawField).getAs<StringAttr>("name");
      if (field && names.contains(field.getValue()))
        fields.push_back(field.getValue().str());
    }
  else {
    for (const auto &name : names)
      fields.push_back(name.getKey().str());
    llvm::sort(fields);
  }
  return fields;
}

static SmallVector<ExactRuleFootprintInput>
collectLiveExactFootprints(Operation *scope) {
  Value candidate;
  scope->getRegion(0).walk([&](Operation *nested) {
    if (auto condition = dyn_cast<RuleConditionOp>(nested))
      candidate = condition.getCondition();
    else if (auto condition = dyn_cast<FiringConditionOp>(nested))
      candidate = condition.getCondition();
  });
  SmallVector<ExactRuleFootprintInput> footprints;
  scope->getRegion(0).walk([&](Operation *nested) {
    if (auto read = dyn_cast<TableGetOp>(nested)) {
      ExactRuleFootprintInput input;
      input.endpoint = nested;
      input.resource = read.getTable().str();
      input.access = "read";
      input.index = read.getIndex();
      input.wholeEntry = true;
      footprints.push_back(std::move(input));
    } else if (auto match = dyn_cast<TableMatchOp>(nested)) {
      ExactRuleFootprintInput input;
      input.endpoint = nested;
      input.resource = match.getTable().str();
      input.access = "read";
      input.fields = projectedLaneFields(nested);
      input.wholeEntry = input.fields.empty();
      footprints.push_back(std::move(input));
    } else if (auto choose = dyn_cast<TableChooseOp>(nested)) {
      ExactRuleFootprintInput input;
      input.endpoint = nested;
      input.resource = choose.getTable().str();
      input.access = "read";
      input.fields = projectedLaneFields(nested);
      input.wholeEntry = input.fields.empty();
      footprints.push_back(std::move(input));
    } else if (auto proposal = dyn_cast<TableProposeOp>(nested)) {
      SmallVector<std::string> fields;
      for (Attribute raw : proposal.getWriteFields())
        fields.push_back(cast<StringAttr>(raw).getValue().str());
      ExactRuleFootprintInput input;
      input.endpoint = nested;
      input.resource = proposal.getTable();
      input.access = proposal.getMode();
      input.index = proposal.getIndex();
      input.fields.assign(std::make_move_iterator(fields.begin()),
                          std::make_move_iterator(fields.end()));
      input.wholeEntry = llvm::is_contained(input.fields, "$entry");
      if (input.wholeEntry)
        input.fields.clear();
      input.predicate = proposal.getWhen() ? proposal.getWhen() : candidate;
      footprints.push_back(std::move(input));
    } else if (auto get = dyn_cast<SlotGetOp>(nested)) {
      ExactRuleFootprintInput input;
      input.endpoint = nested;
      input.resource = get.getSlot().str();
      input.access = "read";
      input.wholeEntry = true;
      footprints.push_back(std::move(input));
    } else if (auto release = dyn_cast<SlotProposeReleaseOp>(nested)) {
      ExactRuleFootprintInput input;
      input.endpoint = nested;
      input.resource = release.getSlot().str();
      input.access = "release";
      input.wholeEntry = true;
      input.predicate = release.getWhen();
      footprints.push_back(std::move(input));
    }
  });
  return footprints;
}

static LogicalResult validatePersistedExactSummary(Operation *scope,
                                                   ArrayAttr dag,
                                                   ArrayAttr footprints) {
  for (auto [ordinal, raw] : llvm::enumerate(dag)) {
    auto node = dyn_cast<DictionaryAttr>(raw);
    auto opcode = node ? node.getAs<RuleExpressionOpcodeAttr>("opcode")
                       : RuleExpressionOpcodeAttr();
    auto type = node ? node.getAs<TypeAttr>("result_type") : TypeAttr();
    auto operands =
        node ? node.getAs<DenseI64ArrayAttr>("operands") : DenseI64ArrayAttr();
    auto attributes =
        node ? node.getAs<DictionaryAttr>("attributes") : DictionaryAttr();
    if (!opcode || !type || !operands || !attributes)
      return scope->emitOpError("has malformed exact expression-DAG node");
    for (int64_t operand : operands.asArrayRef())
      if (operand < 0 || operand >= static_cast<int64_t>(ordinal))
        return scope->emitOpError(
            "exact expression-DAG operand is dangling or cyclic");
  }
  for (Attribute raw : footprints) {
    auto footprint = dyn_cast<DictionaryAttr>(raw);
    auto index =
        footprint ? footprint.getAs<IntegerAttr>("index") : IntegerAttr();
    auto allEntries =
        footprint ? footprint.getAs<BoolAttr>("all_entries") : BoolAttr();
    auto predicate =
        footprint ? footprint.getAs<IntegerAttr>("predicate") : IntegerAttr();
    auto wholeEntry =
        footprint ? footprint.getAs<BoolAttr>("whole_entry") : BoolAttr();
    auto fields =
        footprint ? footprint.getAs<ArrayAttr>("fields") : ArrayAttr();
    if (!footprint || !allEntries || !predicate ||
        (allEntries.getValue() == static_cast<bool>(index)))
      return scope->emitOpError("has malformed exact state footprint roots");
    if (!wholeEntry || !fields || (wholeEntry.getValue() == !fields.empty()))
      return scope->emitOpError(
          "exact state footprint must choose whole entry or explicit fields");
    if ((index && (index.getInt() < 0 ||
                   index.getInt() >= static_cast<int64_t>(dag.size()))) ||
        predicate.getInt() < 0 ||
        predicate.getInt() >= static_cast<int64_t>(dag.size()))
      return scope->emitOpError("exact state footprint has a dangling root");
  }
  return success();
}

} // namespace

FailureOr<ExactRuleEffectSummary>
buildExactRuleEffectSummary(Operation *scope,
                            ArrayRef<ExactRuleFootprintInput> footprints) {
  if (!isa<RuleOp, FiringOp>(scope) || scope->getNumRegions() != 1 ||
      !scope->getRegion(0).hasOneBlock()) {
    scope->emitOpError("exact rule summary requires one rule/firing body");
    return failure();
  }
  TypedExpressionNormalizer normalizer(scope);
  Builder builder(scope->getContext());
  SmallVector<Attribute> persisted;
  for (const ExactRuleFootprintInput &input : footprints) {
    if (!input.endpoint || !scope->isAncestor(input.endpoint)) {
      scope->emitOpError("exact footprint endpoint is outside the rule scope");
      return failure();
    }
    auto resource = FlatSymbolRefAttr::get(scope->getContext(), input.resource);
    FailureOr<TypedExpressionNormalizer::OwnerIdentity> identity =
        normalizer.ownerIdentity(input.endpoint, resource);
    if (failed(identity))
      return failure();
    FailureOr<int64_t> predicate = input.predicate
                                       ? normalizer.normalize(input.predicate)
                                       : normalizer.addSyntheticTrue();
    if (failed(predicate))
      return failure();
    FailureOr<int64_t> index = failure();
    if (input.index) {
      index = normalizer.normalize(input.index);
      if (failed(index))
        return failure();
    }
    SmallVector<StringRef> fieldNames;
    for (const std::string &field : input.fields)
      fieldNames.push_back(field);
    ArrayAttr fields = builder.getStrArrayAttr(fieldNames);
    NamedAttrList footprint;
    footprint.set("access", builder.getStringAttr(input.access));
    footprint.set("resource", resource);
    footprint.set("owner", identity->owner);
    footprint.set("owner_stable_id", identity->stableId);
    footprint.set("fields", fields);
    footprint.set("whole_entry", builder.getBoolAttr(input.wholeEntry));
    footprint.set("all_entries", builder.getBoolAttr(!input.index));
    if (input.index)
      footprint.set("index", builder.getI64IntegerAttr(*index));
    footprint.set("predicate", builder.getI64IntegerAttr(*predicate));
    footprint.set("index_kind",
                  RuleIndexKindAttr::get(
                      scope->getContext(),
                      !input.index ? RuleIndexKind::All
                                   : (input.index.getDefiningOp<VarConstantOp>()
                                          ? RuleIndexKind::Static
                                          : RuleIndexKind::Dynamic)));
    footprint.set(
        "guard_kind",
        RuleGuardKindAttr::get(scope->getContext(),
                               !input.predicate ||
                                       constantVarBool(input.predicate) == true
                                   ? RuleGuardKind::Always
                                   : RuleGuardKind::Predicate));
    footprint.set("endpoint", builder.getStringAttr(
                                  input.endpoint->getName().getStringRef()));
    FailureOr<Attribute> provenance = exactFootprintProvenance(input.endpoint);
    if (failed(provenance))
      return failure();
    footprint.set("source_provenance", *provenance);
    persisted.push_back(builder.getDictionaryAttr(footprint));
  }
  return ExactRuleEffectSummary{builder.getArrayAttr(normalizer.nodes),
                                builder.getArrayAttr(persisted)};
}

FailureOr<NormalizedRuleExpressions>
normalizeRuleExpressions(Operation *scope, ArrayRef<Value> roots) {
  if (!isa<RuleOp, FiringOp>(scope) || scope->getNumRegions() != 1 ||
      !scope->getRegion(0).hasOneBlock()) {
    scope->emitOpError(
        "typed expression normalization requires one rule/firing body");
    return failure();
  }
  TypedExpressionNormalizer normalizer(scope);
  SmallVector<int64_t> normalizedRoots;
  normalizedRoots.reserve(roots.size());
  for (Value root : roots) {
    FailureOr<int64_t> handle = normalizer.normalize(root);
    if (failed(handle))
      return failure();
    normalizedRoots.push_back(*handle);
  }
  return NormalizedRuleExpressions{
      Builder(scope->getContext()).getArrayAttr(normalizer.nodes),
      std::move(normalizedRoots)};
}

FailureOr<ExactRuleEffectSummary>
buildExactRuleEffectSummary(Operation *scope) {
  SmallVector<ExactRuleFootprintInput> live = collectLiveExactFootprints(scope);
  return buildExactRuleEffectSummary(scope, live);
}

SmallVector<ExactRuleFootprintInput>
collectExactRuleFootprintInputs(Operation *scope) {
  return collectLiveExactFootprints(scope);
}

LogicalResult verifyExactRuleEffectSummary(Operation *scope,
                                           ArrayAttr persistedDAG,
                                           ArrayAttr persistedFootprints) {
  if (!persistedDAG || !persistedFootprints)
    return scope->emitOpError(
        "requires exact rule-expression DAG and footprints");
  if (failed(validatePersistedExactSummary(scope, persistedDAG,
                                           persistedFootprints)))
    return failure();
  FailureOr<ExactRuleEffectSummary> expected =
      buildExactRuleEffectSummary(scope);
  if (failed(expected))
    return failure();
  if (persistedDAG != expected->expressionDAG ||
      persistedFootprints != expected->footprints)
    return scope->emitOpError(
        "exact rule effect summary does not match the live body");
  return success();
}

LogicalResult RuleOp::verify() {
  // Zero-output rules are consume-only state transitions; zero-input rules
  // must still produce or update state.  Variadic output values are qualified
  // independently by compiler-owned RuleOutputOp presence records.
  if (getName().empty() || getStableId().empty())
    return emitOpError(
        "requires non-empty definition and stable instance names");
  if (getTimeDomain() != "cycle")
    return emitOpError("rule requires exact time domain 'cycle'");
  auto model = (*this)->getParentOfType<mlir::ModuleOp>();
  auto modelKind =
      model ? model->getAttrOfType<StringAttr>("ac.model_kind") : StringAttr();
  if (modelKind && modelKind.getValue() == "queue_graph") {
    auto graphDomain =
        model->getAttrOfType<StringAttr>("ac.queue_graph_domain");
    if (!graphDomain || graphDomain.getValue() != getTimeDomain())
      return emitOpError("rule domain must match the exact QueueGraph domain");
  }
  if (getTypeState() != TypeConstraintState::Exact)
    return emitOpError("frontend rule requires an exact Queue type");
  for (StringRef name :
       {"ac.rule.effects", "ac.rule.checks", "ac.rule.handshake",
        "ac.rule.guard", "ac.rule.schedule"})
    if ((*this)->hasAttr(name))
      return emitOpError() << "removed rule summary attribute '" << name
                           << "' is not part of canonical ACIR";
  ArrayRef<int64_t> depths = getOutputDepthsAttr().asArrayRef();
  ArrayRef<int64_t> latencies = getOutputLatenciesAttr().asArrayRef();
  if (depths.size() != getOutputs().size() ||
      llvm::any_of(depths, [](int64_t value) { return value <= 0; }))
    return emitOpError("output depths must match results and be positive");
  if (latencies.size() != getOutputs().size() ||
      llvm::any_of(latencies, [](int64_t value) { return value <= 0; }))
    return emitOpError("output latencies must match results and be positive");
  if (failed(verifyQueueRatesAgainstDepths(*this, getOutputs(), depths)))
    return failure();

  Block &block = getBody().front();
  if (block.getNumArguments() != getInputs().size())
    return emitOpError("body argument count must match input Queue count");
  for (auto [input, argument] :
       llvm::zip_equal(getInputs(), block.getArguments())) {
    auto queue = cast<QueueType>(input.getType());
    Type expected = VarType::get(getContext(), queue.getElementType());
    if (argument.getType() != expected)
      return emitOpError("body arguments must match input Queue payloads");
  }
  SmallVector<TableProposeOp> proposals;
  SmallVector<SlotProposeReleaseOp> slotReleases;
  SmallVector<TableGetOp> tableReads;
  bool hasVariableWrite = false;
  unsigned conditions = 0;
  Value conditionValue;
  for (Operation &operation : block.without_terminator())
    if (auto proposal = dyn_cast<TableProposeOp>(operation)) {
      proposals.push_back(proposal);
    } else if (auto release = dyn_cast<SlotProposeReleaseOp>(operation)) {
      slotReleases.push_back(release);
    } else if (auto get = dyn_cast<TableGetOp>(operation)) {
      tableReads.push_back(get);
    } else if (isa<VarAssignOp, VarAssignElementOp>(operation)) {
      hasVariableWrite = true;
    } else if (auto condition = dyn_cast<RuleConditionOp>(operation)) {
      ++conditions;
      conditionValue = condition.getCondition();
    } else if (!isPureExpressionOperation(&operation) &&
               !isa<TypeConstraintMarkerOp, ValueFactMarkerOp,
                    PendingObligationMarkerOp, VarAssignOp, VarAssignElementOp>(
                   operation) &&
               !isa<VarMatchOp, VarChooseOp, TableMatchOp, TableChooseOp>(
                   operation) &&
               !isa<RuleOutputOp, StateSnapshotOp, StateSnapshotSetOp>(
                   operation) &&
               !isa<SlotGetOp, SlotProposeReleaseOp>(operation))
      return emitOpError() << "body operation '" << operation.getName()
                           << "' must be pure in the supported rule subset";
  if (conditions > 1)
    return emitOpError("permits at most one functional condition");
  SmallVector<RuleOutputOp> outputPaths;
  getBody().walk([&](RuleOutputOp output) { outputPaths.push_back(output); });
  const bool hasPathEvidence =
      getOutputs().size() > 1 || !outputPaths.empty() ||
      llvm::any_of(
          proposals,
          [](TableProposeOp op) { return static_cast<bool>(op.getWhen()); }) ||
      !slotReleases.empty();
  if (hasPathEvidence) {
    if (conditions != 1)
      return emitOpError("SSA path evidence requires one rule condition");
    if (outputPaths.size() != getOutputs().size())
      return emitOpError("requires one SSA presence record per output");
    llvm::SmallDenseSet<int64_t> ordinals;
    for (RuleOutputOp output : outputPaths) {
      if (!ordinals.insert(output.getOrdinal()).second)
        return output.emitOpError(
            "output presence must uniquely name one rule result");
      if (!presenceImpliesCandidate(output.getWhen(), conditionValue) ||
          (output.getWhen() != conditionValue &&
           constantVarBool(conditionValue) != true))
        return output.emitOpError(
            "optional output presence requires a true candidate");
    }
    for (TableProposeOp proposal : proposals) {
      if (!proposal.getWhen() ||
          !presenceImpliesCandidate(proposal.getWhen(), conditionValue))
        return proposal.emitOpError(
            "state proposal presence must imply the rule condition");
      if (proposal.getWhen() != conditionValue) {
        if (constantVarBool(conditionValue) != true)
          return proposal.emitOpError(
              "conditional-effect presence requires a true candidate");
      }
    }
    for (SlotProposeReleaseOp release : slotReleases)
      if (!presenceImpliesCandidate(release.getWhen(), conditionValue))
        return release.emitOpError(
            "slot release presence must imply the rule condition");
  }
  for (TableGetOp read : tableReads)
    if (TableOp table = resolveTable(read, read.getTableAttr());
        !table || failed(verifyStaticallySafeRuleTableIndex(read, table,
                                                            read.getIndex()))) {
      return failure();
    }
  if (getInputs().empty() && getOutputs().empty() && proposals.empty() &&
      slotReleases.empty() && !hasVariableWrite)
    return emitOpError("rule without Queue endpoints must update state");
  if (getOutputs().empty() && proposals.empty() && slotReleases.empty() &&
      !hasVariableWrite)
    return emitOpError("outputless rule must update state");
  auto yield = dyn_cast<RuleReturnOp>(block.getTerminator());
  if (!yield || yield.getValues().size() != getOutputs().size())
    return emitOpError("body return count must match output Queue count");
  for (auto [value, output] :
       llvm::zip_equal(yield.getValues(), getOutputs())) {
    auto outputQueue = cast<QueueType>(output.getType());
    Type expectedOutput =
        VarType::get(getContext(), outputQueue.getElementType());
    if (value.getType() != expectedOutput)
      return emitOpError("returned payload must match output Queue type");
  }
  auto exactDAG = (*this)->getAttrOfType<ArrayAttr>("ac.rule.expression_dag");
  auto exactFootprints =
      (*this)->getAttrOfType<ArrayAttr>("ac.rule.footprints_exact");
  if (static_cast<bool>(exactDAG) != static_cast<bool>(exactFootprints))
    return emitOpError(
        "exact rule-expression DAG and footprints must appear together");
  if (exactDAG && failed(verifyExactRuleEffectSummary(getOperation(), exactDAG,
                                                      exactFootprints)))
    return failure();
  return success();
}

static LogicalResult verifyI1VarCondition(Operation *operation,
                                          Value condition) {
  auto variable = dyn_cast<VarType>(condition.getType());
  if (!variable || !variable.getElementType().isInteger(1))
    return operation->emitOpError("condition must be !ac.var<i1>");
  return success();
}

LogicalResult RuleConditionOp::verify() {
  return verifyI1VarCondition(*this, getCondition());
}

LogicalResult FiringConditionOp::verify() {
  return verifyI1VarCondition(*this, getCondition());
}

LogicalResult RuleOutputOp::verify() {
  if (failed(verifyI1VarCondition(*this, getWhen())))
    return failure();
  RuleOp rule = (*this)->getParentOfType<RuleOp>();
  if (!rule || getOrdinal() < 0 ||
      static_cast<size_t>(getOrdinal()) >= rule.getOutputs().size())
    return emitOpError("ordinal must name one rule output");
  auto queue = cast<QueueType>(rule.getOutputs()[getOrdinal()].getType());
  if (getValue().getType() !=
      VarType::get(getContext(), queue.getElementType()))
    return emitOpError("value must match the selected rule output payload");
  auto returned =
      dyn_cast<RuleReturnOp>(rule.getBody().front().getTerminator());
  if (!returned ||
      static_cast<size_t>(getOrdinal()) >= returned.getValues().size())
    return emitOpError("requires a matching ac.rule.return operand");
  Value returnedValue = returned.getValues()[getOrdinal()];
  if (returnedValue != getValue()) {
    auto obligation = returnedValue.getDefiningOp<PendingObligationMarkerOp>();
    if (!obligation || obligation.getInput() != getValue())
      return emitOpError("value must be the matching ac.rule.return payload");
  }
  return success();
}

LogicalResult FiringOutputOp::verify() {
  if (failed(verifyI1VarCondition(*this, getWhen())))
    return failure();
  FiringOp firing = (*this)->getParentOfType<FiringOp>();
  if (!firing || getOrdinal() < 0 ||
      static_cast<size_t>(getOrdinal()) >= firing.getOutputs().size())
    return emitOpError("ordinal must name one firing output");
  auto queue = cast<QueueType>(firing.getOutputs()[getOrdinal()].getType());
  if (getValue().getType() !=
      VarType::get(getContext(), queue.getElementType()))
    return emitOpError("value must match the selected firing output payload");
  auto yielded =
      dyn_cast<FiringYieldOp>(firing.getBody().front().getTerminator());
  if (!yielded ||
      static_cast<size_t>(getOrdinal()) >= yielded.getValues().size() ||
      yielded.getValues()[getOrdinal()] != getValue())
    return emitOpError("value must be the matching ac.firing.yield operand");
  return success();
}

LogicalResult StateSnapshotOp::verify() {
  if (!isa_and_nonnull<RuleOp, FiringOp>((*this)->getParentOp()))
    return emitOpError("must be nested directly in ac.rule or ac.firing");
  if (failed(verifyI1VarCondition(*this, getPredicate())))
    return failure();
  TableOp table = resolveTable(*this, getTableAttr());
  if (!table)
    return emitOpError() << "unresolved table " << getTable();
  if (!tableVisibleFrom(*this, table))
    return emitOpError("table is outside the snapshot scope ancestry");
  if (failed(verifyTableFields(*this, table, getReadFields(), "read")))
    return failure();
  if (getIndexKind() == RuleIndexKind::All) {
    if (getIndex())
      return emitOpError("all-entry snapshot must not carry an index");
    return success();
  }
  if (!getIndex())
    return emitOpError("indexed snapshot requires an index");
  if (failed(verifyStaticallySafeRuleTableIndex(*this, table, getIndex())))
    return failure();
  const bool isStatic =
      static_cast<bool>(getIndex().getDefiningOp<VarConstantOp>());
  if (isStatic != (getIndexKind() == RuleIndexKind::Static))
    return emitOpError("index_kind must match the snapshot index definition");
  return success();
}

LogicalResult StateSnapshotSetOp::verify() {
  if (!isa_and_nonnull<RuleOp, FiringOp>((*this)->getParentOp()))
    return emitOpError("must be nested directly in ac.rule or ac.firing");
  if (failed(verifyI1VarCondition(*this, getPredicate())))
    return failure();
  TableOp table = resolveTable(*this, getTableAttr());
  if (!table)
    return emitOpError() << "unresolved table " << getTable();
  if (!tableVisibleFrom(*this, table))
    return emitOpError("table is outside the snapshot-set scope ancestry");
  if (failed(verifyTableFields(*this, table, getReadFields(), "read")))
    return failure();
  if (!tableWriteFieldsAreComplete(*this, table, getReadFields())) {
    FailureOr<uint64_t> fieldCount = tableEntryFieldCount(*this, table);
    if (failed(fieldCount) || *fieldCount > 64)
      return emitOpError("field-qualified snapshot-set has too many fields");
  }
  Region *sourceRegion = nullptr;
  if (auto match = getSource().getDefiningOp<TableMatchOp>()) {
    if (match->getParentOp() != (*this)->getParentOp())
      return emitOpError(
          "source table.match must belong to the owning rule/firing");
    sourceRegion = &match.getPredicate();
  } else if (auto choose = getSource().getDefiningOp<TableChooseOp>()) {
    const int64_t count = choose.getCountAttr().getInt();
    if (count <= 0 || choose.getResults().size() != 2 * count ||
        !llvm::is_contained(choose.getResults().take_front(count),
                            getSource()) ||
        choose->getParentOp() != (*this)->getParentOp())
      return emitOpError(
          "source table.choose must use the owning rule/firing's index result");
    sourceRegion = &choose.getKey();
  } else {
    return emitOpError(
        "source must be an owning rule/firing table.match mask or "
        "table.choose index");
  }
  bool foundTarget = false;
  sourceRegion->walk([&](TableGetOp read) {
    foundTarget |= resolveTable(read, read.getTableAttr()) == table;
  });
  if (!foundTarget)
    return emitOpError(
        "source evaluation must contain a region-local read of the target "
        "table");
  return success();
}

LogicalResult TypeConstraintMarkerOp::verify() {
  if (getState() == TypeConstraintState::Exact)
    return emitOpError("exact facts must not remain marker-wrapped");
  return success();
}

LogicalResult ValueFactMarkerOp::verify() {
  if (getIdentity().empty() || getPathPredicate().empty())
    return emitOpError("requires non-empty identity and path predicate");
  return success();
}

LogicalResult PendingObligationMarkerOp::verify() {
  if (getOrigin().empty() || getPathPredicate().empty())
    return emitOpError("requires non-empty origin and path predicate");
  return success();
}

namespace {

LogicalResult verifyArchitectureExpressionRef(Operation *owner,
                                              DictionaryAttr reference,
                                              StringRef label) {
  auto table = reference.getAs<StringAttr>("table");
  auto rule = reference.getAs<StringAttr>("rule");
  auto node = reference.getAs<IntegerAttr>("node");
  if (reference.size() != 3 || !table ||
      table.getValue() != "ac.arch_expression_table" || !rule ||
      rule.getValue().empty() || !node || node.getInt() < 0)
    return owner->emitOpError() << label
                                << " must be an exact (table, rule, node) "
                                   "architecture-expression reference";
  auto module = owner->getParentOfType<ModuleOp>();
  auto expressionTable =
      module ? module->getAttrOfType<ArrayAttr>("ac.arch_expression_table")
             : ArrayAttr();
  if (!expressionTable)
    return owner->emitOpError("requires module-owned ac.arch_expression_table");
  llvm::StringSet<> expressionScopes;
  for (Attribute rawScope : expressionTable) {
    auto scope = dyn_cast<DictionaryAttr>(rawScope);
    auto scopeRule = scope ? scope.getAs<StringAttr>("rule") : StringAttr();
    if (!scopeRule || scopeRule.getValue().empty() ||
        !expressionScopes.insert(scopeRule.getValue()).second)
      return owner->emitOpError(
          "module architecture-expression rule scopes must be non-empty and "
          "unique");
  }
  for (Attribute rawScope : expressionTable) {
    auto scope = dyn_cast<DictionaryAttr>(rawScope);
    auto scopeRule = scope ? scope.getAs<StringAttr>("rule") : StringAttr();
    auto nodes = scope ? scope.getAs<ArrayAttr>("nodes") : ArrayAttr();
    auto ownerRule = scope ? scope.getAs<StringAttr>("owner_rule") : StringAttr();
    if (!scope || (scope.size() != 2 && scope.size() != 3) || !scopeRule ||
        !nodes || (scope.size() == 3 &&
                   (!ownerRule || ownerRule.getValue().empty())))
      return owner->emitOpError(
          "module architecture-expression table is malformed");
    if (scopeRule != rule)
      continue;
    for (auto [ordinal, rawNode] : llvm::enumerate(nodes)) {
      auto candidate = dyn_cast<DictionaryAttr>(rawNode);
      auto opcode = candidate
                        ? candidate.getAs<RuleExpressionOpcodeAttr>("opcode")
                        : RuleExpressionOpcodeAttr();
      auto candidateType =
          candidate ? candidate.getAs<TypeAttr>("result_type") : TypeAttr();
      auto operands = candidate ? candidate.getAs<DenseI64ArrayAttr>("operands")
                                : DenseI64ArrayAttr();
      auto attributes = candidate
                            ? candidate.getAs<DictionaryAttr>("attributes")
                            : DictionaryAttr();
      if (!candidate || candidate.size() != 4 || !opcode || !candidateType ||
          !operands || !attributes)
        return owner->emitOpError(
            "module architecture-expression table has a malformed node");
      for (int64_t operand : operands.asArrayRef())
        if (operand < 0 || operand >= static_cast<int64_t>(ordinal))
          return owner->emitOpError(
              "module architecture-expression table has a dangling or "
              "cyclic operand");
    }
    if (node.getInt() >= static_cast<int64_t>(nodes.size()))
      return owner->emitOpError() << label << " has a dangling expression node";
    auto expression = dyn_cast<DictionaryAttr>(nodes[node.getInt()]);
    auto type =
        expression ? expression.getAs<TypeAttr>("result_type") : TypeAttr();
    if (!type)
      return owner->emitOpError()
             << label << " references an untyped expression node";
    auto variable = dyn_cast<VarType>(type.getValue());
    if (!variable || !variable.getElementType().isInteger(1))
      return owner->emitOpError()
             << label << " must reference an exact !ac.var<i1> node";
    return success();
  }
  return owner->emitOpError() << label << " references unknown rule scope '"
                              << rule.getValue() << "'";
}

} // namespace

LogicalResult ArchitectureObligationOp::verify() {
  if (!isa_and_nonnull<ModuleCaseOp>((*this)->getParentOp()))
    return emitOpError("must be owned directly by one ac.module.case");
  if (getId().empty() || getId() != getSymName())
    return emitOpError(
        "requires one explicit non-empty stable ID equal to its symbol name");
  for (char character : getId())
    if (!(llvm::isAlnum(character) || character == '_' || character == '.' ||
          character == '-' || character == ':' || character == '/'))
      return emitOpError(
          "stable ID must use declared structural-name characters only");
  for (ArchitectureObligationOp sibling :
       (*this)->getParentOfType<ModuleOp>().getOps<ArchitectureObligationOp>())
    if (sibling != *this && sibling.getId() == getId())
      return emitOpError("duplicates an architecture-obligation stable ID");

  if (failed(
          verifyArchitectureExpressionRef(*this, getCondition(), "condition")))
    return failure();
  if (getSourceRules().empty())
    return emitOpError("requires at least one typed source-rule reference");
  llvm::StringSet<> sourceRules;
  for (Attribute raw : getSourceRules()) {
    auto rule = dyn_cast<StringAttr>(raw);
    if (!rule || rule.getValue().empty() ||
        !sourceRules.insert(rule.getValue()).second)
      return emitOpError("source-rule references must be non-empty and unique");
  }
  if (getStateOwners().empty())
    return emitOpError("requires at least one typed state-owner reference");
  llvm::StringSet<> owners;
  for (Attribute raw : getStateOwners()) {
    auto owner = dyn_cast<DictionaryAttr>(raw);
    auto resource = owner ? owner.getAs<SymbolRefAttr>("resource")
                          : SymbolRefAttr();
    auto path = owner ? owner.getAs<StringAttr>("owner_path") : StringAttr();
    auto stable = owner ? owner.getAs<StringAttr>("owner_stable_id")
                        : StringAttr();
    auto module = (*this)->getParentOfType<ModuleOp>();
    Operation *resolved = resource
                              ? SymbolTable::lookupNearestSymbolFrom(
                                    getOperation(), resource)
                              : nullptr;
    bool ownerMatches = false;
    if (module && resolved && path && stable) {
      auto symbol = SymbolTable::getSymbolName(resolved);
      auto candidatePath = resolved->getAttrOfType<StringAttr>("owner");
      auto candidateStable = resolved->getAttrOfType<StringAttr>("stable_id");
      if (candidatePath && candidateStable)
        ownerMatches = candidatePath == path && candidateStable == stable;
      else if (symbol) {
        const std::string structuralPath =
            ("/" + module.getSymName() + "/" + symbol.getValue()).str();
        ownerMatches = path.getValue() == structuralPath &&
                       stable.getValue() == symbol.getValue();
      }
    }
    if (!owner || owner.size() != 3 || !resource || !path ||
        path.getValue().empty() || !stable || stable.getValue().empty() ||
        !ownerMatches ||
        !owners.insert((path.getValue() + "\x1f" + stable.getValue()).str())
             .second)
      return emitOpError(
          "state-owner records must carry unique typed resource, canonical "
          "owner path, and stable ID");
  }

  llvm::SmallSet<ArchitectureRuntimeTarget, 3> targets;
  for (Attribute raw : getRuntimeTargets()) {
    auto target = dyn_cast<ArchitectureRuntimeTargetAttr>(raw);
    if (!target || !targets.insert(target.getValue()).second)
      return emitOpError("runtime targets must be typed and unique");
  }
  if (targets.contains(ArchitectureRuntimeTarget::Cpp) ||
      targets.contains(ArchitectureRuntimeTarget::Sva))
    return emitOpError(
        "cpp and sva architecture-obligation targets are not implemented in "
        "the F3 gfsim slice");

  DictionaryAttr sampling = getSampling();
  auto samplingKind = sampling.getAs<ArchitectureSamplingKindAttr>("kind");
  auto edge = sampling.getAs<ArchitectureSamplingEdgeAttr>("edge");
  auto anchor = sampling.getAs<StringAttr>("sample_anchor");
  auto active = sampling.getAs<DictionaryAttr>("active_predicate");
  auto disable = sampling.getAs<DictionaryAttr>("reset_recovery_disable");
  auto latency = sampling.getAs<IntegerAttr>("capture_latency");
  auto monitorOnly = sampling.getAs<BoolAttr>("monitor_only");
  for (NamedAttribute field : sampling)
    if (!llvm::is_contained(
            {StringRef("kind"), StringRef("edge"), StringRef("sample_anchor"),
             StringRef("active_predicate"), StringRef("reset_recovery_disable"),
             StringRef("capture_latency"), StringRef("monitor_only")},
            field.getName().strref()))
      return emitOpError("sampling contract contains an unknown union field");
  if (!samplingKind || !edge || !monitorOnly)
    return emitOpError("sampling contract is missing its typed union fields");
  const bool producer =
      samplingKind.getValue() == ArchitectureSamplingKind::ProducerEvent;
  const bool prePublish =
      samplingKind.getValue() == ArchitectureSamplingKind::PrePublish;
  const bool observation =
      samplingKind.getValue() == ArchitectureSamplingKind::TickObservation ||
      samplingKind.getValue() == ArchitectureSamplingKind::XferObservation;
  if ((observation && anchor) ||
      ((producer || prePublish) && (!anchor || anchor.getValue().empty())))
    return emitOpError("sampling anchor does not match the union arm");
  if ((!producer && edge.getValue() != ArchitectureSamplingEdge::None) ||
      (producer && edge.getValue() == ArchitectureSamplingEdge::None))
    return emitOpError("sampling edge does not match the union arm");
  if (latency && (latency.getInt() < 0 || !producer || !monitorOnly.getValue()))
    return emitOpError(
        "capture latency is legal only for monitor-only producer events");
  if (active && failed(verifyArchitectureExpressionRef(*this, active,
                                                       "active predicate")))
    return failure();
  if (disable && failed(verifyArchitectureExpressionRef(
                     *this, disable, "reset/recovery disable")))
    return failure();
  if (prePublish && anchor && !sourceRules.contains(anchor.getValue()))
    return emitOpError(
        "pre_publish sample anchor must name a source rule/firing");

  if (getMessage().empty() || getSourceProvenance().empty())
    return emitOpError("requires diagnostic text and source provenance");
  if (getStatus() == ArchitectureObligationStatus::Pending ||
      getStatus() == ArchitectureObligationStatus::Rejected)
    return success(); // Legal intermediate IR; closure rejects both.
  if (getStatus() == ArchitectureObligationStatus::Proved) {
    if (!getProofCertificate() || !getRuntimeTargets().empty() ||
        !getMaterializations().empty())
      return emitOpError(
          "proved obligation requires one certificate and no runtime state");
    return success();
  }
  if (getStatus() != ArchitectureObligationStatus::RuntimeChecked)
    return emitOpError("has an unknown architecture-obligation status");
  if (getKind() != ArchitectureObligationKind::Range || !prePublish ||
      !targets.contains(ArchitectureRuntimeTarget::Gfsim) ||
      targets.size() != 1 || getMaterializations().size() != 1)
    return emitOpError(
        "F3 runtime checks admit only one gfsim pre_publish range monitor");
  auto materialization = dyn_cast<DictionaryAttr>(getMaterializations()[0]);
  auto target =
      materialization
          ? materialization.getAs<ArchitectureRuntimeTargetAttr>("target")
          : ArchitectureRuntimeTargetAttr();
  auto firing = materialization ? materialization.getAs<StringAttr>("firing")
                                : StringAttr();
  auto maximum = materialization ? materialization.getAs<IntegerAttr>("maximum")
                                 : IntegerAttr();
  auto inputOrdinal = materialization
                          ? materialization.getAs<IntegerAttr>("input_ordinal")
                          : IntegerAttr();
  if (!materialization || materialization.size() < 4 || !target ||
      target.getValue() != ArchitectureRuntimeTarget::Gfsim || !firing ||
      firing.getValue() != anchor.getValue() || !maximum ||
      maximum.getInt() < 0 || !inputOrdinal || inputOrdinal.getInt() < 0)
    return emitOpError(
        "runtime materialization must be one exact gfsim firing/range record");
  return success();
}

LogicalResult SourceOp::verify() {
  if (getDepth() <= 0)
    return emitOpError("depth must be positive");
  if (getLatency() <= 0)
    return emitOpError("latency must be positive");
  if (cast<QueueType>(getOutput().getType()).getRate() > getDepth())
    return emitOpError(
        "Queue rate must not exceed its independently declared depth");
  Type payload = cast<QueueType>(getOutput().getType()).getElementType();
  llvm::SmallPtrSet<Operation *, 8> active;
  std::function<bool(Type)> containsDeclaredRange = [&](Type type) -> bool {
    if (isa<RangeType>(type))
      return true;
    if (auto array = dyn_cast<ValueArrayType>(type))
      return containsDeclaredRange(array.getElementType());
    if (auto tuple = dyn_cast<TupleType>(type))
      return llvm::any_of(tuple.getTypes(), containsDeclaredRange);
    SymbolRefAttr reference;
    if (auto structure = dyn_cast<StructType>(type))
      reference = structure.getName();
    else if (auto packet = dyn_cast<PacketType>(type))
      reference = packet.getName();
    else if (auto transaction = dyn_cast<TransactionType>(type))
      reference = transaction.getName();
    if (!reference)
      return false;
    Operation *declaration =
        SymbolTable::lookupNearestSymbolFrom(getOperation(), reference);
    if (!declaration || !active.insert(declaration).second)
      return false;
    auto fields = declaration->getAttrOfType<ArrayAttr>("fields");
    const bool found =
        fields && llvm::any_of(fields, [&](Attribute rawField) {
          auto field = dyn_cast<DictionaryAttr>(rawField);
          auto fieldType = field ? field.getAs<TypeAttr>("type") : TypeAttr();
          return fieldType && containsDeclaredRange(fieldType.getValue());
        });
    active.erase(declaration);
    return found;
  };
  if (containsDeclaredRange(payload))
    return emitOpError(
        "external source cannot carry an undecoded bounded range");
  return success();
}

LogicalResult ObserveOp::verify() {
  if (getName().empty())
    return emitOpError("name must be non-empty");
  return success();
}

LogicalResult ExpectOp::verify() {
  if (getMessage().empty())
    return emitOpError("message must be non-empty");
  Block &block = getPredicate().front();
  Type payload = cast<QueueType>(getInput().getType()).getElementType();
  Type expected = VarType::get(getContext(), payload);
  if (block.getNumArguments() != 1 ||
      block.getArgument(0).getType() != expected)
    return emitOpError("predicate argument must match queue payload Var");
  for (Operation &operation : block.without_terminator())
    if (!isPureExpressionOperation(&operation))
      return emitOpError() << "predicate operation '" << operation.getName()
                           << "' must be pure";
  auto yield = dyn_cast<ExpectYieldOp>(block.getTerminator());
  if (!yield || !cast<VarType>(yield.getCondition().getType())
                     .getElementType()
                     .isInteger(1))
    return emitOpError("predicate must terminate with an i1 ac.expect.yield");
  return success();
}

LogicalResult BroadcastOp::verify() {
  if (getOutputs().size() < 2)
    return emitOpError("requires at least two output queues");
  for (auto [index, output] : llvm::enumerate(getOutputs()))
    if (output.getType() != getInput().getType())
      return emitOpError() << "output queue " << index
                           << " must match input queue type";
  ArrayRef<int64_t> depths = getOutputDepthsAttr().asArrayRef();
  ArrayRef<int64_t> latencies = getOutputLatenciesAttr().asArrayRef();
  if (depths.size() != getOutputs().size())
    return emitOpError("output depth count must match result count");
  if (latencies.size() != getOutputs().size())
    return emitOpError("output latency count must match result count");
  if (llvm::any_of(depths, [](int64_t value) { return value <= 0; }))
    return emitOpError("output depths must be positive");
  if (llvm::any_of(latencies, [](int64_t value) { return value <= 0; }))
    return emitOpError("output latencies must be positive");
  return success();
}

LogicalResult ForkOp::verify() {
  if (getOutputs().size() < 2)
    return emitOpError("requires at least two output queues");
  for (auto [index, output] : llvm::enumerate(getOutputs()))
    if (output.getType() != getInput().getType())
      return emitOpError() << "output queue " << index
                           << " must match input queue type";
  ArrayRef<int64_t> depths = getOutputDepthsAttr().asArrayRef();
  ArrayRef<int64_t> latencies = getOutputLatenciesAttr().asArrayRef();
  if (depths.size() != getOutputs().size() ||
      llvm::any_of(depths, [](int64_t value) { return value <= 0; }))
    return emitOpError("output depths must match results and be positive");
  if (latencies.size() != getOutputs().size() ||
      llvm::any_of(latencies, [](int64_t value) { return value <= 0; }))
    return emitOpError("output latencies must match results and be positive");
  return success();
}

LogicalResult RouteOp::verify() {
  if (getOutputs().size() < 2)
    return emitOpError("requires at least two output queues");
  for (auto [index, output] : llvm::enumerate(getOutputs()))
    if (output.getType() != getInput().getType())
      return emitOpError() << "output queue " << index
                           << " must match input queue type";
  ArrayRef<int64_t> depths = getOutputDepthsAttr().asArrayRef();
  ArrayRef<int64_t> latencies = getOutputLatenciesAttr().asArrayRef();
  if (depths.size() != getOutputs().size() ||
      llvm::any_of(depths, [](int64_t value) { return value <= 0; }))
    return emitOpError("output depths must match results and be positive");
  if (latencies.size() != getOutputs().size() ||
      llvm::any_of(latencies, [](int64_t value) { return value <= 0; }))
    return emitOpError("output latencies must match results and be positive");
  Block &block = getSelector().front();
  if (block.getNumArguments() != 1 ||
      block.getArgument(0).getType() !=
          VarType::get(getContext(),
                       cast<QueueType>(getInput().getType()).getElementType()))
    return emitOpError("selector argument must match input payload Var");
  auto yield = dyn_cast<RouteYieldOp>(block.getTerminator());
  if (!yield)
    return emitOpError("selector must terminate with ac.route.yield");
  Type selector = cast<VarType>(yield.getSelector().getType()).getElementType();
  if (!isa<IntegerType, EnumType>(selector))
    return emitOpError("selector must be an integer or enum Var");
  return success();
}

LogicalResult SelectOp::verify() {
  if (getInputs().size() < 3)
    return emitOpError("requires one control and at least two data queues");
  llvm::DenseSet<Value> uniqueInputs;
  for (Value input : getInputs())
    if (!uniqueInputs.insert(input).second)
      return emitOpError("input queue operands must be unique");
  for (auto [index, input] : llvm::enumerate(getInputs().drop_front()))
    if (input.getType() != getOutput().getType())
      return emitOpError() << "data input queue " << index
                           << " must match output queue type";
  if (getDepth() <= 0 || getLatency() <= 0)
    return emitOpError("depth and latency must be positive");

  Block &block = getKey().front();
  Type controlPayload =
      cast<QueueType>(getInputs().front().getType()).getElementType();
  Type expected = VarType::get(getContext(), controlPayload);
  if (block.getNumArguments() != 1 ||
      block.getArgument(0).getType() != expected)
    return emitOpError("key argument must match control queue payload Var");
  for (Operation &operation : block.without_terminator())
    if (!isPureExpressionOperation(&operation))
      return emitOpError() << "key operation '" << operation.getName()
                           << "' must be pure";
  auto yield = dyn_cast<SelectYieldOp>(block.getTerminator());
  if (!yield)
    return emitOpError("key must terminate with ac.select.yield");
  Type selector = cast<VarType>(yield.getSelector().getType()).getElementType();
  auto integer = dyn_cast<IntegerType>(selector);
  if (!integer || integer.getWidth() > 64)
    return emitOpError("key must yield an integer Var with width at most 64");
  const uint64_t candidates = getInputs().size() - 1;
  if (integer.getWidth() < 64 &&
      candidates > (uint64_t{1} << integer.getWidth()))
    return emitOpError("data input count must fit selector width");
  return success();
}

LogicalResult MergeOp::verify() {
  if (getInputs().size() < 2)
    return emitOpError("requires at least two input queues");
  for (auto [index, input] : llvm::enumerate(getInputs()))
    if (input.getType() != getOutput().getType())
      return emitOpError() << "input queue " << index
                           << " must match output queue type";
  if (getPolicy() != "round_robin" && getPolicy() != "priority")
    return emitOpError("policy must be 'round_robin' or 'priority'");
  if (getDepth() <= 0 || getLatency() <= 0)
    return emitOpError("depth and latency must be positive");
  return success();
}

LogicalResult BarrierOp::verify() {
  if (getInputs().size() < 2)
    return emitOpError("requires at least two input queues");
  if (getOutputs().size() != getInputs().size())
    return emitOpError("output count must match input count");
  llvm::DenseSet<Value> uniqueInputs;
  for (auto [index, input] : llvm::enumerate(getInputs())) {
    if (!uniqueInputs.insert(input).second)
      return emitOpError("input queue operands must be unique");
    if (getOutputs()[index].getType() != input.getType())
      return emitOpError() << "output queue " << index
                           << " must match its input queue type";
  }
  ArrayRef<int64_t> depths = getOutputDepthsAttr().asArrayRef();
  ArrayRef<int64_t> latencies = getOutputLatenciesAttr().asArrayRef();
  if (depths.size() != getOutputs().size() ||
      llvm::any_of(depths, [](int64_t value) { return value <= 0; }))
    return emitOpError("output depths must match results and be positive");
  if (latencies.size() != getOutputs().size() ||
      llvm::any_of(latencies, [](int64_t value) { return value <= 0; }))
    return emitOpError("output latencies must match results and be positive");
  return success();
}

LogicalResult ReorderOp::verify() {
  if (getInput().getType() != getOutput().getType())
    return emitOpError("output queue must match input queue type");
  if (getCapacity() <= 0 || getDepth() <= 0 || getLatency() <= 0)
    return emitOpError("capacity, depth, and latency must be positive");
  if (getStart() < 0)
    return emitOpError("start must be non-negative");

  Block &block = getKey().front();
  Type payload = cast<QueueType>(getInput().getType()).getElementType();
  Type expected = VarType::get(getContext(), payload);
  if (block.getNumArguments() != 1 ||
      block.getArgument(0).getType() != expected)
    return emitOpError("key argument must match queue payload Var");
  for (Operation &operation : block.without_terminator())
    if (!isPureExpressionOperation(&operation))
      return emitOpError() << "key operation '" << operation.getName()
                           << "' must be pure";
  auto yield = dyn_cast<ReorderYieldOp>(block.getTerminator());
  if (!yield)
    return emitOpError("key must terminate with ac.reorder.yield");
  auto key = cast<VarType>(yield.getKey().getType()).getElementType();
  auto integer = dyn_cast<IntegerType>(key);
  if (!integer || integer.getWidth() > 64)
    return emitOpError("key must be an integer Var with width at most 64");
  if (integer.getWidth() < 64 &&
      static_cast<uint64_t>(getStart()) >= (uint64_t{1} << integer.getWidth()))
    return emitOpError("start must fit key width");
  return success();
}

LogicalResult DependencyOp::verify() {
  if (getInput().getType() != getOutput().getType())
    return emitOpError("output queue must match input queue type");
  if (getCapacity() <= 0 || getResources() <= 0 || getDepth() <= 0 ||
      getLatency() <= 0)
    return emitOpError(
        "capacity, resources, depth, and latency must be positive");
  if (getNoDependency() < 0)
    return emitOpError("no_dependency must be non-negative");
  if ((*this)->getAttr("schedule_provider"))
    return emitOpError(
        "schedule provider must use the canonical 'ac.schedule_provider' "
        "attribute");
  Attribute providerValue = (*this)->getAttr("ac.schedule_provider");
  StringAttr provider;
  if (providerValue) {
    provider = dyn_cast<StringAttr>(providerValue);
    if (!provider)
      return emitOpError("ac.schedule_provider must be a StringAttr");
  }
  if (provider && provider.getValue() != "v2")
    return emitOpError("schedule provider must be 'v2'");

  Type payload = cast<QueueType>(getInput().getType()).getElementType();
  Type argumentType = VarType::get(getContext(), payload);
  auto verifyPolicy = [&](Region &region,
                          StringRef name) -> FailureOr<IntegerType> {
    Block &block = region.front();
    if (block.getNumArguments() != 1 ||
        block.getArgument(0).getType() != argumentType) {
      emitOpError() << name << " argument must match queue payload Var";
      return failure();
    }
    for (Operation &operation : block.without_terminator())
      if (!isPureExpressionOperation(&operation)) {
        emitOpError() << name << " operation '" << operation.getName()
                      << "' must be pure";
        return failure();
      }
    auto yield = dyn_cast<DependencyYieldOp>(block.getTerminator());
    if (!yield) {
      emitOpError() << name << " must terminate with ac.dependency.yield";
      return failure();
    }
    auto value = cast<VarType>(yield.getValue().getType()).getElementType();
    auto integer = dyn_cast<IntegerType>(value);
    if (!integer || integer.getWidth() > 64) {
      emitOpError() << name
                    << " must yield an integer Var with width at most 64";
      return failure();
    }
    return integer;
  };

  FailureOr<IntegerType> key = verifyPolicy(getKey(), "key");
  FailureOr<IntegerType> dependency = verifyPolicy(getWaitsFor(), "waits_for");
  FailureOr<IntegerType> resource = verifyPolicy(getResource(), "resource");
  FailureOr<IntegerType> cost = verifyPolicy(getCost(), "cost");
  if (failed(key) || failed(dependency) || failed(resource) || failed(cost))
    return failure();
  if (*key != *dependency)
    return emitOpError("key and waits_for must use the same integer Var type");
  if (provider) {
    if (key->getWidth() > 16)
      return emitOpError(
          "schedule v2 key and waits_for width must be at most 16");
    const uint64_t allOnes = (uint64_t{1} << key->getWidth()) - 1;
    if (static_cast<uint64_t>(getNoDependency()) != allOnes)
      return emitOpError(
          "schedule v2 no_dependency must be the key type's all-ones value");
  }
  if (dependency->getWidth() < 64 &&
      static_cast<uint64_t>(getNoDependency()) >=
          (uint64_t{1} << dependency->getWidth()))
    return emitOpError("no_dependency must fit dependency width");
  if (resource->getWidth() < 64 && static_cast<uint64_t>(getResources()) >
                                       (uint64_t{1} << resource->getWidth()))
    return emitOpError("resources must fit resource width");
  return success();
}

LogicalResult CreditOp::verify() {
  if (getInput().getType() != getOutput().getType())
    return emitOpError("output queue must match input queue type");
  if (getCredits() <= 0 || getDepth() <= 0 || getLatency() <= 0)
    return emitOpError("credits, depth, and latency must be positive");

  Block &block = getCost().front();
  Type payload = cast<QueueType>(getInput().getType()).getElementType();
  Type expected = VarType::get(getContext(), payload);
  if (block.getNumArguments() != 1 ||
      block.getArgument(0).getType() != expected)
    return emitOpError("cost argument must match queue payload Var");
  for (Operation &operation : block.without_terminator())
    if (!isPureExpressionOperation(&operation))
      return emitOpError() << "cost operation '" << operation.getName()
                           << "' must be pure";
  auto yield = dyn_cast<CreditYieldOp>(block.getTerminator());
  if (!yield)
    return emitOpError("cost must terminate with ac.credit.yield");
  Type cost = cast<VarType>(yield.getCost().getType()).getElementType();
  auto integer = dyn_cast<IntegerType>(cost);
  if (!integer || integer.getWidth() > 64)
    return emitOpError("cost must yield an integer Var with width at most 64");
  return success();
}

LogicalResult FeedbackOp::verify() {
  if (getInput().getType() != getOutput().getType())
    return emitOpError("output queue must match input queue type");
  if (getDepth() <= 0 || getLatency() <= 0 || getMaxIterations() <= 0)
    return emitOpError("depth, latency, and max_iterations must be positive");

  Block &block = getBody().front();
  Type payload = cast<QueueType>(getInput().getType()).getElementType();
  Type expectedValue = VarType::get(getContext(), payload);
  if (block.getNumArguments() != 1 ||
      block.getArgument(0).getType() != expectedValue)
    return emitOpError("body argument must match queue payload Var");
  for (Operation &operation : block.without_terminator())
    if (!isPureExpressionOperation(&operation))
      return emitOpError() << "body operation '" << operation.getName()
                           << "' must be pure";
  auto yield = dyn_cast<FeedbackYieldOp>(block.getTerminator());
  if (!yield)
    return emitOpError("body must terminate with ac.feedback.yield");
  if (yield.getValue().getType() != expectedValue)
    return emitOpError("yielded value must match queue payload Var");
  if (yield.getContinueValue().getType() !=
      VarType::get(getContext(), IntegerType::get(getContext(), 1)))
    return emitOpError("continue value must be !ac.var<i1>");
  return success();
}

LogicalResult ScopeOp::verify() {
  Block &block = getBody().front();
  if (block.getNumArguments() != getInputs().size())
    return emitOpError("body argument count must match input queue count");
  for (size_t index = 0; index < getInputs().size(); ++index)
    if (block.getArgument(index).getType() != getInputs()[index].getType())
      return emitOpError() << "body argument " << index
                           << " must match input queue type";
  auto yield = dyn_cast<ScopeYieldOp>(block.getTerminator());
  if (!yield)
    return emitOpError("body must terminate with ac.scope.yield");
  if (yield.getQueues().size() != getOutputs().size())
    return emitOpError("yielded queue count must match result count");
  for (size_t index = 0; index < getOutputs().size(); ++index)
    if (yield.getQueues()[index].getType() != getOutputs()[index].getType())
      return emitOpError() << "yielded queue " << index
                           << " must match result type";
  return success();
}

LogicalResult FiringOp::verify() {
  if (getOutputDepthsAttr().size() != getOutputs().size() ||
      getOutputLatenciesAttr().size() != getOutputs().size())
    return emitOpError("output depth/latency counts must match results");
  if (llvm::any_of(getOutputDepthsAttr().asArrayRef(),
                   [](int64_t value) { return value <= 0; }) ||
      llvm::any_of(getOutputLatenciesAttr().asArrayRef(),
                   [](int64_t value) { return value <= 0; }))
    return emitOpError("output depths and latencies must be positive");
  if (failed(verifyQueueRatesAgainstDepths(*this, getOutputs(),
                                           getOutputDepthsAttr().asArrayRef())))
    return failure();
  if (getStableId().empty())
    return emitOpError("requires explicit stable identity");
  for (StringRef name :
       {"functional_guard", "checks", "handshake", "schedule", "effects"})
    if ((*this)->hasAttr(name))
      return emitOpError() << "removed firing summary attribute '" << name
                           << "' is not part of canonical ACIR";
  if (getTimeDomain() != "cycle")
    return emitOpError("firing requires exact time domain 'cycle'");
  auto model = (*this)->getParentOfType<mlir::ModuleOp>();
  auto modelKind =
      model ? model->getAttrOfType<StringAttr>("ac.model_kind") : StringAttr();
  if (modelKind && modelKind.getValue() == "queue_graph") {
    auto graphDomain =
        model->getAttrOfType<StringAttr>("ac.queue_graph_domain");
    if (!graphDomain || graphDomain.getValue() != getTimeDomain())
      return emitOpError(
          "firing domain must match the exact QueueGraph domain");
  }
  SmallVector<TableProposeOp> proposals;
  SmallVector<SlotProposeReleaseOp> slotReleases;
  SmallVector<TableGetOp> tableReads;
  SmallVector<FiringConditionOp> conditions;
  getBody().walk(
      [&](TableProposeOp proposal) { proposals.push_back(proposal); });
  getBody().walk(
      [&](SlotProposeReleaseOp release) { slotReleases.push_back(release); });
  getBody().walk([&](TableGetOp read) { tableReads.push_back(read); });
  getBody().walk(
      [&](FiringConditionOp condition) { conditions.push_back(condition); });
  for (TableGetOp read : tableReads)
    if (TableOp table = resolveTable(read, read.getTableAttr());
        !table || failed(verifyStaticallySafeRuleTableIndex(read, table,
                                                            read.getIndex()))) {
      return failure();
    }
  if (getInputs().empty() && getOutputs().empty() && proposals.empty() &&
      slotReleases.empty())
    return emitOpError("firing without Queue endpoints must update state");
  if (getOutputs().empty() && proposals.empty() && slotReleases.empty())
    return emitOpError("outputless firing must update state");
  if (conditions.size() > 1)
    return emitOpError("permits at most one functional condition");
  SmallVector<FiringOutputOp> outputPaths;
  getBody().walk([&](FiringOutputOp output) { outputPaths.push_back(output); });
  for (FiringOutputOp output : outputPaths)
    if (output.getOrdinal() < 0 ||
        static_cast<size_t>(output.getOrdinal()) >= getOutputs().size())
      return output.emitOpError("ordinal must name one firing output");
  const bool hasPathEvidence =
      getOutputs().size() > 1 || !outputPaths.empty() ||
      llvm::any_of(
          proposals,
          [](TableProposeOp op) { return static_cast<bool>(op.getWhen()); }) ||
      !slotReleases.empty();
  if (hasPathEvidence) {
    if (conditions.size() != 1)
      return emitOpError("SSA path evidence requires one firing condition");
    if (outputPaths.size() != getOutputs().size())
      return emitOpError("requires one SSA presence record per output");
    Value condition = conditions.front().getCondition();
    llvm::SmallDenseSet<int64_t> ordinals;
    for (FiringOutputOp output : outputPaths) {
      if (!ordinals.insert(output.getOrdinal()).second)
        return output.emitOpError(
            "output presence must uniquely name one firing result");
      if (!presenceImpliesCandidate(output.getWhen(), condition) ||
          (output.getWhen() != condition && constantVarBool(condition) != true))
        return output.emitOpError(
            "optional output presence requires a true candidate");
    }
    for (TableProposeOp proposal : proposals) {
      if (!proposal.getWhen() ||
          !presenceImpliesCandidate(proposal.getWhen(), condition))
        return proposal.emitOpError(
            "state proposal presence must imply the firing condition");
      if (proposal.getWhen() != condition) {
        if (constantVarBool(condition) != true)
          return proposal.emitOpError(
              "conditional-effect presence requires a true candidate");
      }
    }
    for (SlotProposeReleaseOp release : slotReleases)
      if (!presenceImpliesCandidate(release.getWhen(), condition))
        return release.emitOpError(
            "slot release presence must imply the firing condition");
  }
  auto priority = (*this)->getAttrOfType<IntegerAttr>("ac.rule_priority");
  auto footprints = (*this)->getAttrOfType<ArrayAttr>("ac.rule_footprints");
  auto exactDAG = (*this)->getAttrOfType<ArrayAttr>("ac.expression_dag");
  auto exactFootprints =
      (*this)->getAttrOfType<ArrayAttr>("ac.footprints_exact");
  const bool requiresInferredSchedule =
      modelKind && modelKind.getValue() == "queue_graph";
  if ((requiresInferredSchedule && (!priority || !footprints)) ||
      static_cast<bool>(priority) != static_cast<bool>(footprints) ||
      static_cast<bool>(exactDAG) != static_cast<bool>(exactFootprints) ||
      (priority && priority.getInt() < 0))
    return emitOpError(
        "requires inferred non-negative priority and exact typed footprints");
  if (footprints) {
    SmallVector<Operation *> stateOperations;
    getBody().walk([&](Operation *operation) {
      if (isa<TableGetOp, TableMatchOp, TableChooseOp, TableProposeOp>(
              operation))
        stateOperations.push_back(operation);
    });
    if (footprints.size() != stateOperations.size())
      return emitOpError()
             << "inferred footprint count must match state operations "
             << "(footprints=" << footprints.size()
             << ", operations=" << stateOperations.size() << ")";
    for (auto [rawFootprint, operation] :
         llvm::zip_equal(footprints, stateOperations)) {
      auto footprint = dyn_cast<DictionaryAttr>(rawFootprint);
      auto access =
          footprint ? footprint.getAs<StringAttr>("access") : StringAttr();
      auto resource = footprint ? footprint.getAs<FlatSymbolRefAttr>("resource")
                                : FlatSymbolRefAttr();
      auto indexKind =
          footprint ? footprint.getAs<StringAttr>("index_kind") : StringAttr();
      auto footprintGuard =
          footprint ? footprint.getAs<RuleGuardKindAttr>("guard_kind")
                    : RuleGuardKindAttr();
      if (!access || !resource || !indexKind || !footprintGuard ||
          (indexKind.getValue() != "static" &&
           indexKind.getValue() != "dynamic" && indexKind.getValue() != "all"))
        return emitOpError("has malformed inferred state footprint");
      Value index;
      StringRef expectedAccess;
      FlatSymbolRefAttr expectedResource;
      ArrayAttr expectedFields;
      RuleGuardKind expectedFootprintGuard = RuleGuardKind::Always;
      if (auto read = dyn_cast<TableGetOp>(operation)) {
        index = read.getIndex();
        expectedAccess = "read";
        expectedResource = read.getTableAttr();
      } else if (auto match = dyn_cast<TableMatchOp>(operation)) {
        expectedAccess = "read";
        expectedResource = match.getTableAttr();
      } else if (auto choose = dyn_cast<TableChooseOp>(operation)) {
        expectedAccess = "read";
        expectedResource = choose.getTableAttr();
      } else {
        auto proposal = cast<TableProposeOp>(operation);
        index = proposal.getIndex();
        expectedAccess = proposal.getMode();
        expectedResource = proposal.getTableAttr();
        expectedFields = proposal.getWriteFieldsAttr();
        expectedFootprintGuard =
            proposal.getWhen()
                ? guardKindFor(proposal.getWhen())
                : (conditions.empty()
                       ? RuleGuardKind::Always
                       : guardKindFor(conditions.front().getCondition()));
      }
      StringRef expectedIndexKind =
          !index
              ? "all"
              : (index.getDefiningOp<VarConstantOp>() ? "static" : "dynamic");
      auto fields = footprint.getAs<ArrayAttr>("fields");
      if (access.getValue() != expectedAccess || resource != expectedResource ||
          indexKind.getValue() != expectedIndexKind ||
          fields != expectedFields ||
          footprintGuard.getValue() != expectedFootprintGuard)
        return emitOpError(
            "inferred footprint must exactly match its state operation");
    }
  }
  const bool validArity = !getInputs().empty() || !getOutputs().empty() ||
                          !proposals.empty() || !slotReleases.empty();
  if (conditions.empty() && requiresInferredSchedule) {
    return emitOpError("requires one typed functional condition");
  }
  if (!validArity)
    return emitOpError("has invalid Queue/state arity");

  Block &block = getBody().front();
  if (block.getNumArguments() != getInputs().size())
    return emitOpError("body argument count must match input Queue count");
  for (auto [input, argument] :
       llvm::zip_equal(getInputs(), block.getArguments())) {
    auto queue = cast<QueueType>(input.getType());
    Type expected = VarType::get(getContext(), queue.getElementType());
    if (argument.getType() != expected)
      return emitOpError("body arguments must match input Queue payloads");
  }
  for (Operation &operation : block.without_terminator())
    if (!isPureExpressionOperation(&operation) &&
        !isa<TableGetOp, TableProposeOp, TableMatchOp, TableChooseOp,
             VarAssignOp, FiringConditionOp, FiringOutputOp, StateSnapshotOp,
             StateSnapshotSetOp, SlotGetOp, SlotProposeReleaseOp>(operation))
      return emitOpError() << "body operation '" << operation.getName()
                           << "' must be pure after marker elimination";
  auto yield = dyn_cast<FiringYieldOp>(block.getTerminator());
  if (!yield || yield.getValues().size() != getOutputs().size())
    return emitOpError("body must yield one payload per output Queue");
  for (auto [output, value] :
       llvm::zip_equal(getOutputs(), yield.getValues())) {
    auto queue = cast<QueueType>(output.getType());
    Type expected = VarType::get(getContext(), queue.getElementType());
    if (value.getType() != expected)
      return emitOpError("yielded values must match output Queue payloads");
  }
  if (failed(verifyTypedRuleSummary(getOperation(), getInputs(), getOutputs(),
                                    getBody(), footprints, priority, "ac.")))
    return failure();
  if (exactDAG && failed(verifyExactRuleEffectSummary(getOperation(), exactDAG,
                                                      exactFootprints)))
    return failure();
  return verifyActivationEvidence(getOperation(), getInputs(), getOutputs(),
                                  getBody(), true);
}

namespace detail {

ScopedProcessLivenessWorkCollector::ScopedProcessLivenessWorkCollector(
    ProcessLivenessWork &work)
    : previous(processLivenessWorkCollector) {
  work = {};
  processLivenessWorkCollector = &work;
}

ScopedProcessLivenessWorkCollector::~ScopedProcessLivenessWorkCollector() {
  processLivenessWorkCollector = previous;
}

} // namespace detail

namespace {

struct NamedRef {
  SymbolRefAttr name;
  StringRef opName;
};

std::optional<NamedRef> namedRef(Type type) {
  return TypeSwitch<Type, std::optional<NamedRef>>(type)
      .Case<StructType>([](auto type) {
        return NamedRef{type.getName(), StructOp::getOperationName()};
      })
      .Case<PacketType>([](auto type) {
        return NamedRef{type.getName(), PacketOp::getOperationName()};
      })
      .Case<TransactionType>([](auto type) {
        return NamedRef{type.getName(), TransactionOp::getOperationName()};
      })
      .Case<EnumType>([](auto type) {
        return NamedRef{type.getName(), EnumOp::getOperationName()};
      })
      .Default([](Type) { return std::nullopt; });
}

Operation *lookup(Operation *from, SymbolRefAttr name) {
  if (name.getNestedReferences().size() != 1)
    return SymbolTable::lookupNearestSymbolFrom(from, name);
  Operation *scope = nullptr;
  if (auto enclosing = from->getParentOfType<TypeScopeOp>();
      enclosing && enclosing.getSymNameAttr() == name.getRootReference())
    scope = enclosing;
  if (!scope) {
    auto root = FlatSymbolRefAttr::get(name.getRootReference());
    scope = SymbolTable::lookupNearestSymbolFrom(from, root);
    if (!scope)
      if (auto module = from->getParentOfType<mlir::ModuleOp>())
        scope = SymbolTable::lookupSymbolIn(module, root);
  }
  if (!isa_and_nonnull<TypeScopeOp>(scope))
    return nullptr;
  return SymbolTable::lookupSymbolIn(scope, name.getLeafReference());
}

LogicalResult requireQualified(Operation *from, SymbolRefAttr name) {
  if (name.getNestedReferences().size() == 1)
    return success();
  return from->emitOpError(
      "named data references require a qualified symbol such as "
      "'@types::@S'");
}

LogicalResult verifyNamedTypes(Operation *from, Type type) {
  LogicalResult result = success();
  type.walk([&](Type nested) {
    auto ref = namedRef(nested);
    if (!ref)
      return WalkResult::advance();
    if (failed(requireQualified(from, ref->name))) {
      result = failure();
      return WalkResult::interrupt();
    }
    Operation *decl = lookup(from, ref->name);
    if (!decl) {
      from->emitOpError() << "unresolved named data type '" << ref->name << "'";
      result = failure();
      return WalkResult::interrupt();
    }
    if (decl->getName().getStringRef() != ref->opName) {
      from->emitOpError() << "named type '" << ref->name << "' requires "
                          << ref->opName << " but resolves to "
                          << decl->getName();
      result = failure();
      return WalkResult::interrupt();
    }
    return WalkResult::advance();
  });
  return result;
}

LogicalResult verifyPlacement(Operation *op) {
  if (isa_and_nonnull<TypeScopeOp>(op->getParentOp()))
    return success();
  return op->emitOpError(
      "named data declarations must be direct children of ac.type_scope");
}

FailureOr<DictionaryAttr> fieldDictionary(Operation *op, Attribute field) {
  auto dictionary = dyn_cast<DictionaryAttr>(field);
  if (!dictionary || !dictionary.getAs<StringAttr>("name") ||
      !dictionary.getAs<TypeAttr>("type")) {
    op->emitOpError("field metadata requires string 'name' and type 'type'");
    return failure();
  }
  return dictionary;
}

StringRef fieldName(DictionaryAttr field) {
  return field.getAs<StringAttr>("name").getValue();
}

Type fieldType(DictionaryAttr field) {
  return field.getAs<TypeAttr>("type").getValue();
}

bool isNormativeValueType(Type type) {
  if (isa<IntegerType, FloatType, IndexType, RangeType, StructType, PacketType,
          TransactionType, EnumType>(type))
    return true;
  if (auto vector = dyn_cast<mlir::VectorType>(type))
    return isNormativeValueType(vector.getElementType());
  if (auto array = dyn_cast<ValueArrayType>(type))
    return isNormativeValueType(array.getElementType());
  if (auto tuple = dyn_cast<mlir::TupleType>(type))
    return llvm::all_of(tuple.getTypes(), isNormativeValueType);
  return false;
}

bool isProtocolPayloadType(Type type) {
  return isNormativeValueType(type) && !containsChannelType(type);
}

bool isTopologyLeaf(Type type) {
  return isa<FlowType, EndpointType, ResourceRefType, ChannelType,
             ResourceTokenType>(type);
}

Type findNestedTopologyLeaf(Type type) {
  if (isTopologyLeaf(type))
    return {};
  Type found;
  type.walk([&](Type nested) {
    if (!isTopologyLeaf(nested))
      return WalkResult::advance();
    found = nested;
    return WalkResult::interrupt();
  });
  return found;
}

template <typename OpTy>
OpTy lookupChild(Operation *container, FlatSymbolRefAttr name) {
  return dyn_cast_or_null<OpTy>(SymbolTable::lookupSymbolIn(container, name));
}

ProtocolOp lookupProtocol(Operation *from, FlatSymbolRefAttr name) {
  auto module = from->getParentOfType<mlir::ModuleOp>();
  return module ? dyn_cast_or_null<ProtocolOp>(
                      SymbolTable::lookupSymbolIn(module, name))
                : ProtocolOp();
}

bool isCarrierAction(StringRef action) {
  return action == "offer" || action == "response" || action == "notify";
}

bool matchesCarrierEvent(ProtocolOp protocol, Type payload,
                         FlatSymbolRefAttr from = {},
                         FlatSymbolRefAttr to = {}) {
  return llvm::any_of(protocol.getBody().getOps<EventOp>(), [&](EventOp event) {
    return isCarrierAction(event.getAction()) &&
           event.getPayload() == payload &&
           (!from || event.getFromAttr() == from) &&
           (!to || event.getToAttr() == to);
  });
}

LogicalResult verifyRoleReference(Operation *from, Operation *container,
                                  FlatSymbolRefAttr name, StringRef subject) {
  if (lookupChild<RoleOp>(container, name))
    return success();
  return from->emitOpError()
         << "unresolved " << subject << " role '@" << name.getValue() << "'";
}

LogicalResult verifyRoleContainer(Operation *container) {
  llvm::SmallDenseSet<StringRef> names;
  for (RoleOp role : container->getRegion(0).getOps<RoleOp>())
    if (!names.insert(role.getSymName()).second)
      return role.emitOpError()
             << "redefinition of symbol named '" << role.getSymName() << "'";
  for (RoleOp role : container->getRegion(0).getOps<RoleOp>()) {
    if (role.getCardinality() != "exclusive" &&
        role.getCardinality() != "shared")
      return role.emitOpError() << "unsupported role cardinality '"
                                << role.getCardinality() << "'";
    RoleOp dual = lookupChild<RoleOp>(container, role.getDualAttr());
    if (!dual)
      return role.emitOpError() << "unresolved dual role '@"
                                << role.getDualAttr().getValue() << "'";
    if (dual == role)
      return role.emitOpError("role cannot be its own dual");
    if (dual.getDualAttr() !=
        FlatSymbolRefAttr::get(role.getContext(), role.getSymName()))
      return role.emitOpError("role duality must be symmetric");
    if (dual.getCardinality() != role.getCardinality())
      return role.emitOpError("dual roles must have matching cardinality");
  }
  return success();
}

bool hasStringValue(StringRef value, ArrayRef<StringRef> accepted) {
  return llvm::is_contained(accepted, value);
}

GuaranteeOp findGuarantee(ProtocolOp protocol, StringRef kind) {
  for (GuaranteeOp guarantee : protocol.getBody().getOps<GuaranteeOp>())
    if (guarantee.getKind() == kind)
      return guarantee;
  return {};
}

LogicalResult verifyStringGuarantee(GuaranteeOp op,
                                    ArrayRef<StringRef> accepted) {
  auto value = dyn_cast<StringAttr>(op.getValue());
  if (!value || !hasStringValue(value.getValue(), accepted))
    return op.emitOpError()
           << "unsupported " << op.getKind() << " value '"
           << (value ? value.getValue() : StringRef("<non-string>")) << "'";
  return success();
}

bool isAllowedGuardExpression(Operation *operation) {
  return llvm::StringSwitch<bool>(operation->getName().getStringRef())
      .Cases({"arith.constant", "arith.cmpi",   "arith.cmpf",
              "arith.addi",     "arith.subi",   "arith.muli",
              "arith.divui",    "arith.divsi",  "arith.remui",
              "arith.remsi",    "arith.andi",   "arith.ori",
              "arith.xori",     "arith.shli",   "arith.shrui",
              "arith.shrsi",    "arith.select", "arith.index_cast",
              "arith.extui",    "arith.extsi",  "arith.trunci",
              "arith.addf",     "arith.subf",   "arith.mulf",
              "arith.divf",     "arith.negf",   "index.constant",
              "index.add",      "index.sub",    "index.mul",
              "index.divs",     "index.divu",   "index.rems",
              "index.remu",     "index.cmp",    "index.casts",
              "index.castu"},
             true)
      .Default(false);
}

LogicalResult verifyFields(Operation *op, ArrayAttr fields) {
  llvm::SmallDenseSet<StringRef> seen;
  for (Attribute attribute : fields) {
    FailureOr<DictionaryAttr> field = fieldDictionary(op, attribute);
    if (failed(field))
      return failure();
    StringRef name = fieldName(*field);
    Type type = fieldType(*field);
    if (!seen.insert(name).second)
      return op->emitOpError() << "duplicate field '" << name << "'";
    if (!isNormativeValueType(type))
      return op->emitOpError()
             << "field '" << name << "' has non-value type " << type;
    if (failed(verifyNamedTypes(op, type)))
      return failure();
    if (field->get("max_length")) {
      return op->emitOpError()
             << "field '" << name << "' cannot declare removed max_length";
    }
  }
  return success();
}

ArrayAttr declarationFields(Operation *op) {
  return op->getAttrOfType<ArrayAttr>("fields");
}

Operation *recordDecl(Operation *from, Type type) {
  auto ref = namedRef(type);
  if (!ref || (ref->opName != StructOp::getOperationName() &&
               ref->opName != PacketOp::getOperationName() &&
               ref->opName != TransactionOp::getOperationName()))
    return nullptr;
  if (failed(requireQualified(from, ref->name)))
    return nullptr;
  Operation *decl = lookup(from, ref->name);
  return decl && decl->getName().getStringRef() == ref->opName ? decl : nullptr;
}

std::optional<unsigned> findField(Operation *decl, StringRef name) {
  for (auto [index, attribute] : llvm::enumerate(declarationFields(decl))) {
    auto field = cast<DictionaryAttr>(attribute);
    if (fieldName(field) == name)
      return index;
  }
  return std::nullopt;
}

Type fieldType(Operation *decl, unsigned index) {
  return fieldType(cast<DictionaryAttr>(declarationFields(decl)[index]));
}

SmallVector<NamedRef> directValueReferences(Type type) {
  if (auto ref = namedRef(type))
    return {*ref};
  if (auto vector = dyn_cast<mlir::VectorType>(type))
    return directValueReferences(vector.getElementType());
  if (auto array = dyn_cast<ValueArrayType>(type))
    return directValueReferences(array.getElementType());
  if (auto tuple = dyn_cast<mlir::TupleType>(type)) {
    SmallVector<NamedRef> result;
    for (Type element : tuple.getTypes())
      llvm::append_range(result, directValueReferences(element));
    return result;
  }
  return {};
}

LogicalResult verifyNoRecursion(Operation *root) {
  auto rootName =
      root->getAttrOfType<StringAttr>(SymbolTable::getSymbolAttrName());
  llvm::SmallDenseSet<Operation *> active;
  std::function<LogicalResult(Operation *)> visit =
      [&](Operation *current) -> LogicalResult {
    if (!active.insert(current).second) {
      root->emitOpError() << "unbounded value recursion through '@"
                          << rootName.getValue() << "'";
      return failure();
    }
    if (ArrayAttr fields = declarationFields(current)) {
      for (Attribute attribute : fields) {
        Type type = fieldType(cast<DictionaryAttr>(attribute));
        for (NamedRef ref : directValueReferences(type)) {
          Operation *next = lookup(root, ref.name);
          if (next && declarationFields(next) && failed(visit(next)))
            return failure();
        }
      }
    }
    active.erase(current);
    return success();
  };
  return visit(root);
}

LogicalResult verifyRecordDeclaration(Operation *op, ArrayAttr fields) {
  if (failed(verifyPlacement(op)) || failed(verifyFields(op, fields)))
    return failure();
  return verifyNoRecursion(op);
}

SymbolRefAttr declarationReference(Operation *declaration) {
  auto scope = cast<TypeScopeOp>(declaration->getParentOp());
  auto leaf = FlatSymbolRefAttr::get(
      declaration->getAttrOfType<StringAttr>(SymbolTable::getSymbolAttrName()));
  return SymbolRefAttr::get(declaration->getContext(), scope.getSymName(),
                            ArrayRef<FlatSymbolRefAttr>{leaf});
}

Type declarationType(Operation *declaration) {
  SymbolRefAttr reference = declarationReference(declaration);
  return TypeSwitch<Operation *, Type>(declaration)
      .Case<StructOp>([&](auto) {
        return StructType::get(declaration->getContext(), reference);
      })
      .Case<PacketOp>([&](auto) {
        return PacketType::get(declaration->getContext(), reference);
      })
      .Case<EnumOp>([&](auto) {
        return EnumType::get(declaration->getContext(), reference);
      })
      .Default([](Operation *) { return Type(); });
}

FailureOr<DictionaryAttr> queryLayout(TypeScopeOp scope, Type type) {
  DataLayoutSpecInterface spec = scope.getDataLayoutSpec();
  if (!spec)
    return failure();
  FailureOr<Attribute> value = spec.query(DataLayoutEntryKey(type));
  if (failed(value))
    return failure();
  auto dictionary = dyn_cast<DictionaryAttr>(*value);
  if (!dictionary)
    return failure();
  return dictionary;
}

LogicalResult verifyDeclarationLayout(Operation *declaration) {
  Type type = declarationType(declaration);
  auto scope = cast<TypeScopeOp>(declaration->getParentOp());
  if (succeeded(queryLayout(scope, type)))
    return success();
  return declaration->emitOpError() << "missing DLTI layout entry for " << type;
}

LogicalResult verifyUniqueEnumerants(EnumOp op) {
  llvm::SmallDenseSet<StringRef> seen;
  for (Attribute value : op.getEnumerants()) {
    StringRef text = cast<StringAttr>(value).getValue();
    if (!seen.insert(text).second)
      return op.emitOpError() << "duplicate enumerant '" << text << "'";
  }
  return success();
}

} // namespace

DataLayoutSpecInterface TypeScopeOp::getDataLayoutSpec() {
  return getOperation()->getAttrOfType<DataLayoutSpecInterface>(
      DLTIDialect::kDataLayoutAttrName);
}

TargetSystemSpecInterface TypeScopeOp::getTargetSystemSpec() {
  return getOperation()->getAttrOfType<TargetSystemSpecInterface>(
      DLTIDialect::kTargetSystemDescAttrName);
}

LogicalResult TypeAliasOp::verify() {
  if (failed(verifyPlacement(*this)))
    return failure();
  return verifyNamedTypes(*this, getTarget());
}

LogicalResult StructOp::verify() {
  if (failed(verifyRecordDeclaration(*this, getFields())))
    return failure();
  return verifyDeclarationLayout(*this);
}

LogicalResult BitfieldOp::verify() {
  if (failed(verifyPlacement(*this)))
    return failure();
  if (getWidth() <= 0 || getWidth() > 64)
    return emitOpError("width must be in [1, 64]");
  if (getFields().empty())
    return emitOpError("requires at least one field");

  llvm::SmallDenseSet<StringRef> names;
  StringRef previous;
  for (Attribute attribute : getFields()) {
    auto field = dyn_cast<DictionaryAttr>(attribute);
    if (!field || field.size() != 3)
      return emitOpError("fields must contain exact {name, msb, lsb} records");
    auto name = field.getAs<StringAttr>("name");
    auto msb = field.getAs<IntegerAttr>("msb");
    auto lsb = field.getAs<IntegerAttr>("lsb");
    if (!name || name.getValue().empty() || !msb || !lsb)
      return emitOpError(
          "fields must contain non-empty name and integer msb/lsb");
    if (!names.insert(name.getValue()).second)
      return emitOpError() << "duplicate field '" << name.getValue() << "'";
    if (!previous.empty() && previous >= name.getValue())
      return emitOpError("fields must be sorted by UTF-8 name");
    previous = name.getValue();
    if (lsb.getInt() < 0 || msb.getInt() < lsb.getInt() ||
        msb.getInt() >= getWidth())
      return emitOpError() << "field '" << name.getValue()
                           << "' range must satisfy 0 <= lsb <= msb < width";
  }
  return success();
}

LogicalResult TransactionOp::verify() {
  return verifyRecordDeclaration(*this, getFields());
}

LogicalResult PacketOp::verify() {
  if (failed(verifyRecordDeclaration(*this, getFields())))
    return failure();
  return verifyDeclarationLayout(*this);
}

LogicalResult EnumOp::verify() {
  if (failed(verifyPlacement(*this)) || failed(verifyUniqueEnumerants(*this)))
    return failure();
  const bool hasValues = static_cast<bool>(getValuesAttr());
  const bool hasWidth = static_cast<bool>(getEncodingWidthAttr());
  if (hasValues != hasWidth)
    return emitOpError(
        "explicit enum values and encoding width must be provided together");
  if (hasValues) {
    ArrayAttr values = getValuesAttr();
    uint64_t width = *getEncodingWidth();
    if (values.size() != getEnumerants().size())
      return emitOpError("explicit enum value count must match enumerants");
    if (width <= 0 || width > 64)
      return emitOpError("encoding width must be in [1, 64]");
    std::set<uint64_t> seen;
    for (Attribute rawValue : values) {
      auto value = dyn_cast<IntegerAttr>(rawValue);
      if (!value || !value.getType().isSignlessInteger(64) ||
          (width < 64 &&
           value.getValue().getZExtValue() >= (uint64_t{1} << width)))
        return emitOpError(
            "explicit enum values must be nonnegative and fit encoding width");
      if (!seen.insert(value.getValue().getZExtValue()).second)
        return emitOpError("explicit enum values must be unique");
    }
  }
  return verifyDeclarationLayout(*this);
}

LogicalResult VarConstantOp::verify() {
  auto result = cast<VarType>(getResult().getType());
  auto value = dyn_cast<TypedAttr>(getValue());
  if (auto range = dyn_cast<RangeType>(result.getElementType())) {
    auto integer = dyn_cast_or_null<IntegerAttr>(value);
    const uint64_t upper = range.getUpper();
    const unsigned width = upper == std::numeric_limits<uint64_t>::max()
                               ? 64
                               : std::max(1u, llvm::Log2_64_Ceil(upper + 1));
    if (!integer || !integer.getType().isSignlessInteger(width) ||
        integer.getValue().getZExtValue() < range.getLower() ||
        integer.getValue().getZExtValue() > upper)
      return emitOpError(
          "range constant must use its storage width and lie within bounds");
    return success();
  }
  if (!value || value.getType() != result.getElementType())
    return emitOpError("attribute type must match Var element type");
  return success();
}

LogicalResult VarEnumOp::verify() {
  auto declaration = dyn_cast_or_null<EnumOp>(lookup(*this, getDeclaration()));
  if (!declaration)
    return emitOpError("declaration must resolve to ac.enum");
  auto result =
      dyn_cast<EnumType>(cast<VarType>(getResult().getType()).getElementType());
  if (!result || result.getName() != getDeclaration())
    return emitOpError("result must carry the referenced nominal enum type");
  if (!llvm::any_of(declaration.getEnumerants(), [&](Attribute value) {
        return cast<StringAttr>(value).getValue() == getEnumerant();
      }))
    return emitOpError() << "unknown enumerant '" << getEnumerant() << "'";
  return success();
}

LogicalResult VarEnumMatchOp::verify() {
  ValueRange values = getValues();
  if (values.size() < 3)
    return emitOpError("requires one selector, at least one case value, and "
                       "one invalid value");
  auto selector = dyn_cast<EnumType>(
      cast<VarType>(values.front().getType()).getElementType());
  if (!selector)
    return emitOpError("selector must carry a nominal enum type");
  auto declaration =
      dyn_cast_or_null<EnumOp>(lookup(*this, selector.getName()));
  if (!declaration)
    return emitOpError("selector enum declaration must resolve to ac.enum");
  if (getEnumerants().size() + 2 != values.size())
    return emitOpError(
        "enumerant list must match the positional case value count");

  llvm::StringSet<> declared;
  for (Attribute attribute : declaration.getEnumerants())
    declared.insert(cast<StringAttr>(attribute).getValue());
  llvm::StringSet<> covered;
  for (Attribute attribute : getEnumerants()) {
    StringRef enumerant = cast<StringAttr>(attribute).getValue();
    if (!declared.contains(enumerant))
      return emitOpError() << "unknown or unreachable enum case '" << enumerant
                           << "'";
    if (!covered.insert(enumerant).second)
      return emitOpError() << "duplicate enum case '" << enumerant << "'";
  }
  for (Attribute attribute : declaration.getEnumerants()) {
    StringRef enumerant = cast<StringAttr>(attribute).getValue();
    if (!covered.contains(enumerant))
      return emitOpError() << "non-exhaustive enum cases; missing '"
                           << enumerant << "'";
  }

  Type resultType = getResult().getType();
  for (Value value : values.drop_front())
    if (value.getType() != resultType)
      return emitOpError(
          "case and invalid operand types must exactly match the result type");
  return success();
}

LogicalResult VarTupleOp::verify() {
  auto tuple = dyn_cast<mlir::TupleType>(
      cast<VarType>(getResult().getType()).getElementType());
  if (!tuple || tuple.size() == 0 || tuple.size() != getValues().size())
    return emitOpError("result tuple must match the non-empty operand list");
  for (auto [value, type] : llvm::zip_equal(getValues(), tuple.getTypes()))
    if (value.getType() != VarType::get(getContext(), type))
      return emitOpError("tuple operand types must match result elements");
  return success();
}

LogicalResult VarArrayOp::verify() {
  auto array = dyn_cast<ValueArrayType>(
      cast<VarType>(getResult().getType()).getElementType());
  if (!array || array.getLength() <= 0 ||
      static_cast<size_t>(array.getLength()) != getValues().size())
    return emitOpError("result value_array length must match operands");
  Type expected = VarType::get(getContext(), array.getElementType());
  if (llvm::any_of(getValues(),
                   [&](Value value) { return value.getType() != expected; }))
    return emitOpError("value_array operands must match its element type");
  return success();
}

LogicalResult VarRecordOp::verify() {
  auto result = cast<VarType>(getResult().getType());
  Operation *declaration = recordDecl(*this, result.getElementType());
  if (!declaration)
    return emitOpError("result must be a record-like Var type");
  ArrayAttr fields = declarationFields(declaration);
  if (!fields || fields.empty() || fields.size() != getValues().size())
    return emitOpError("record fields must match the non-empty operand list");
  for (auto [value, index] :
       llvm::zip_equal(getValues(), llvm::seq<unsigned>(0, fields.size())))
    if (value.getType() !=
        VarType::get(getContext(), fieldType(declaration, index)))
      return emitOpError("record operand types must match declaration order");
  return success();
}

LogicalResult VarElementOp::verify() {
  Type aggregate = cast<VarType>(getAggregate().getType()).getElementType();
  Type expected;
  int64_t length = 0;
  if (auto tuple = dyn_cast<mlir::TupleType>(aggregate)) {
    length = tuple.size();
    if (getIndex() >= 0 && getIndex() < length)
      expected = tuple.getType(getIndex());
  } else if (auto array = dyn_cast<ValueArrayType>(aggregate)) {
    length = array.getLength();
    if (getIndex() >= 0 && getIndex() < length)
      expected = array.getElementType();
  } else {
    return emitOpError("aggregate must be tuple or value_array");
  }
  if (getIndex() < 0 || getIndex() >= length)
    return emitOpError("aggregate index is out of range");
  if (getResult().getType() != VarType::get(getContext(), expected))
    return emitOpError("result must match the selected aggregate element");
  return success();
}

LogicalResult VarDynamicElementOp::verify() {
  auto array = dyn_cast<ValueArrayType>(
      cast<VarType>(getAggregate().getType()).getElementType());
  if (!array)
    return emitOpError("dynamic aggregate must be a value_array");
  Type indexElement = cast<VarType>(getIndex().getType()).getElementType();
  auto index = dyn_cast<IntegerType>(indexElement);
  auto range = dyn_cast<RangeType>(indexElement);
  if ((!index || !index.isSignless() || index.getWidth() == 0 ||
       index.getWidth() > 64) &&
      !range)
    return emitOpError("dynamic array index must be an unsigned scalar");
  if (range && range.getUpper() >= static_cast<uint64_t>(array.getLength()))
    return emitOpError("bounded index exceeds the value_array length");
  if (getResult().getType() !=
      VarType::get(getContext(), array.getElementType()))
    return emitOpError("result must match the value_array element type");
  return success();
}

LogicalResult VarWithElementOp::verify() {
  auto aggregateType = cast<VarType>(getAggregate().getType());
  auto array = dyn_cast<ValueArrayType>(aggregateType.getElementType());
  if (!array)
    return emitOpError("aggregate must be a value_array");
  Type indexElement = cast<VarType>(getIndex().getType()).getElementType();
  auto index = dyn_cast<IntegerType>(indexElement);
  auto range = dyn_cast<RangeType>(indexElement);
  if ((!index || !index.isSignless() || index.getWidth() == 0 ||
       index.getWidth() > 64) &&
      !range)
    return emitOpError("index must be an unsigned scalar");
  if (range && range.getUpper() >= static_cast<uint64_t>(array.getLength()))
    return emitOpError("bounded index exceeds the value_array length");
  if (getValue().getType() !=
      VarType::get(getContext(), array.getElementType()))
    return emitOpError("replacement must match the value_array element type");
  if (getResult().getType() != getAggregate().getType())
    return emitOpError("result must preserve the value_array type");
  return success();
}

static bool supportsZeroImage(Operation *operation, Type type,
                              llvm::SmallPtrSetImpl<Operation *> &seen) {
  if (isa<IntegerType, EnumType>(type))
    return true;
  if (auto range = dyn_cast<RangeType>(type))
    return range.getLower() == 0;
  if (auto tuple = dyn_cast<TupleType>(type))
    return llvm::all_of(tuple.getTypes(), [&](Type element) {
      return supportsZeroImage(operation, element, seen);
    });
  if (auto array = dyn_cast<ValueArrayType>(type))
    return supportsZeroImage(operation, array.getElementType(), seen);
  Operation *declaration = recordDecl(operation, type);
  if (!declaration || !seen.insert(declaration).second)
    return false;
  const bool supported =
      llvm::all_of(declarationFields(declaration), [&](Attribute rawField) {
        return supportsZeroImage(
            operation, fieldType(cast<DictionaryAttr>(rawField)), seen);
      });
  seen.erase(declaration);
  return supported;
}

static VarDeclOp resolveVarDecl(Operation *operation,
                                FlatSymbolRefAttr reference) {
  for (Operation *ancestor = operation->getParentOp(); ancestor;
       ancestor = ancestor->getParentOp()) {
    if (ancestor->getNumRegions() != 1 || !ancestor->getRegion(0).hasOneBlock())
      continue;
    for (VarDeclOp variable :
         ancestor->getRegion(0).front().getOps<VarDeclOp>())
      if (variable.getSymName() == reference.getValue())
        return variable;
  }
  return {};
}

LogicalResult VarDeclOp::verify() {
  if (!isImmutablePayloadType(getValueType()))
    return emitOpError("value type must be an immutable ACIR payload type");
  if (auto shape = getShapeAttr()) {
    ArrayRef<int64_t> dimensions = shape.asArrayRef();
    if (dimensions.empty() ||
        llvm::any_of(dimensions, [](int64_t extent) { return extent <= 0; }))
      return emitOpError(
          "persistent ac.var shape must contain positive dimensions");
  }
  auto init = dyn_cast<TypedAttr>(getInit());
  const auto zero = dyn_cast<IntegerAttr>(getInit());
  const bool zeroImage = zero && zero.getValue().isZero();
  if (auto range = dyn_cast<RangeType>(getValueType())) {
    const uint64_t upper = range.getUpper();
    const unsigned width = upper == std::numeric_limits<uint64_t>::max()
                               ? 64
                               : std::max(1u, llvm::Log2_64_Ceil(upper + 1));
    if (!zero || !zero.getType().isSignlessInteger(width) ||
        zero.getValue().getZExtValue() < range.getLower() ||
        zero.getValue().getZExtValue() > upper)
      return emitOpError(
          "range init must use its storage width and lie within bounds");
  } else {
    llvm::SmallPtrSet<Operation *, 8> seen;
    if ((!init || init.getType() != getValueType()) &&
        !(zeroImage && isa<StructType, EnumType>(getValueType()) &&
          supportsZeroImage(*this, getValueType(), seen)))
      return emitOpError("init must match value type or be the zero image for "
                         "a struct or enum");
  }
  if (getOwner().empty() || !getOwner().starts_with('/') ||
      (getOwner().size() > 1 && getOwner().ends_with('/')))
    return emitOpError("owner must be a canonical absolute scope path");
  std::string expected = "var/";
  if (getOwner() != "/") {
    expected.append(getOwner().drop_front());
    expected.push_back('/');
  }
  expected.append(getSymName());
  if (getStableId() != expected)
    return emitOpError("stable_id must match canonical owner/symbol identity");
  return success();
}

LogicalResult VarReadOp::verify() {
  VarDeclOp variable = resolveVarDecl(*this, getVariableAttr());
  if (!variable)
    return emitOpError() << "unresolved ac.var " << getVariable();
  if (variable.getShapeAttr())
    return emitOpError("shaped ac.var requires ac.var.read_element");
  Type expected = VarType::get(getContext(), variable.getValueType());
  if (getResult().getType() != expected)
    return emitOpError("result must match declared ac.var value type");
  return success();
}

static LogicalResult verifyVarElementAccess(Operation *operation,
                                            FlatSymbolRefAttr variableRef,
                                            Value index, Type valueType) {
  VarDeclOp variable = resolveVarDecl(operation, variableRef);
  if (!variable)
    return operation->emitOpError()
           << "unresolved ac.var " << variableRef.getValue();
  auto shape = variable.getShapeAttr();
  if (!shape || shape.asArrayRef().empty())
    return operation->emitOpError("requires a shaped ac.var");
  auto indexVar = dyn_cast<VarType>(index.getType());
  Type indexElement = indexVar ? indexVar.getElementType() : Type();
  auto indexType = dyn_cast_or_null<IntegerType>(indexElement);
  auto rangeType = dyn_cast_or_null<RangeType>(indexElement);
  if (!indexType && !rangeType)
    return operation->emitOpError("index must be an unsigned scalar ac.var");
  uint64_t entries = 1;
  for (int64_t extent : shape.asArrayRef()) {
    if (extent <= 0 ||
        entries > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) /
                      static_cast<uint64_t>(extent))
      return operation->emitOpError("ac.var shape product overflows");
    entries *= static_cast<uint64_t>(extent);
  }
  if (rangeType && rangeType.getUpper() >= entries)
    return operation->emitOpError(
        "bounded index exceeds the shaped ac.var domain");
  if (!rangeType && shape.asArrayRef().size() > 1 &&
      indexType.getWidth() !=
          std::max<unsigned>(1, llvm::Log2_64_Ceil(entries)))
    return operation->emitOpError(
        "index must use the canonical flattened ac.var width");
  if (auto constant = index.getDefiningOp<VarConstantOp>()) {
    auto value = dyn_cast<IntegerAttr>(constant.getValue());
    if (!value || value.getValue().getZExtValue() >= entries)
      return operation->emitOpError("constant element index is out of range");
    Type expected =
        VarType::get(operation->getContext(), variable.getValueType());
    if (valueType != expected)
      return operation->emitOpError(
          "element value must match declared ac.var element type");
    return success();
  }
  Type expected =
      VarType::get(operation->getContext(), variable.getValueType());
  if (valueType != expected)
    return operation->emitOpError(
        "element value must match declared ac.var element type");
  return success();
}

LogicalResult VarReadElementOp::verify() {
  return verifyVarElementAccess(*this, getVariableAttr(), getIndex(),
                                getResult().getType());
}

LogicalResult VarAssignOp::verify() {
  if (!isa_and_nonnull<RuleOp, FiringOp>((*this)->getParentOp()))
    return emitOpError("must be nested directly in ac.rule or ac.firing");
  VarDeclOp variable = resolveVarDecl(*this, getVariableAttr());
  if (!variable)
    return emitOpError() << "unresolved ac.var " << getVariable();
  if (variable.getShapeAttr())
    return emitOpError("shaped ac.var requires ac.var.assign_element");
  Type expected = VarType::get(getContext(), variable.getValueType());
  if (getValue().getType() != expected)
    return emitOpError("assigned value must match declared ac.var value type");
  if (getWhen() && failed(verifyI1VarCondition(*this, getWhen())))
    return failure();
  return success();
}

LogicalResult VarAssignElementOp::verify() {
  if (!isa_and_nonnull<RuleOp, FiringOp>((*this)->getParentOp()))
    return emitOpError("must be nested directly in ac.rule or ac.firing");
  if (failed(verifyVarElementAccess(*this, getVariableAttr(), getIndex(),
                                    getValue().getType())))
    return failure();
  if (getWhen() && failed(verifyI1VarCondition(*this, getWhen())))
    return failure();
  return success();
}

static FailureOr<std::pair<VarDeclOp, int64_t>>
resolveVarCollection(Operation *operation, FlatSymbolRefAttr variableRef) {
  VarDeclOp variable = resolveVarDecl(operation, variableRef);
  if (!variable) {
    operation->emitOpError() << "unresolved ac.var " << variableRef.getValue();
    return failure();
  }
  auto shape = variable.getShapeAttr();
  if (!shape || shape.asArrayRef().empty()) {
    operation->emitOpError("requires a shaped ac.var");
    return failure();
  }
  uint64_t entries = 1;
  for (int64_t extent : shape.asArrayRef()) {
    if (extent <= 0 ||
        entries > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) /
                      static_cast<uint64_t>(extent)) {
      operation->emitOpError("ac.var shape product overflows");
      return failure();
    }
    entries *= static_cast<uint64_t>(extent);
  }
  return std::make_pair(variable, static_cast<int64_t>(entries));
}

static bool isCandidateMaskType(Type type, int64_t entries) {
  auto variable = dyn_cast<VarType>(type);
  if (!variable || entries <= 0)
    return false;
  Type element = variable.getElementType();
  if (entries <= 64) {
    auto integer = dyn_cast<IntegerType>(element);
    return integer && integer.getWidth() == static_cast<unsigned>(entries);
  }
  auto words = dyn_cast<ValueArrayType>(element);
  auto word =
      words ? dyn_cast<IntegerType>(words.getElementType()) : IntegerType();
  return words && word && word.getWidth() == 64 &&
         words.getLength() == (entries + 63) / 64;
}

LogicalResult VarMatchOp::verify() {
  auto collection = resolveVarCollection(*this, getVariableAttr());
  if (failed(collection))
    return failure();
  VarDeclOp variable = collection->first;
  const int64_t entries = collection->second;
  int64_t domainEntries = entries;
  if (getRow()) {
    ArrayRef<int64_t> shape = variable.getShapeAttr().asArrayRef();
    if (shape.size() != 2)
      return emitOpError("row projection requires a rank-two shaped ac.var");
    auto rowType = dyn_cast<IntegerType>(
        cast<VarType>(getRow().getType()).getElementType());
    if (!rowType || !rowType.isSignless() ||
        rowType.getWidth() !=
            std::max<unsigned>(1, llvm::Log2_64_Ceil(shape.front())))
      return emitOpError(
          "row projection requires the canonical first-axis width");
    domainEntries = shape.back();
  }
  if (!isCandidateMaskType(getMask().getType(), domainEntries))
    return emitOpError(
        getRow() ? "mask must exactly cover the projected ac.var row"
                 : "mask must exactly cover the ac.var domain in 64-bit words");
  if (!getPredicate().hasOneBlock())
    return emitOpError("predicate must contain exactly one block");
  Block &block = getPredicate().front();
  Type element = VarType::get(getContext(), variable.getValueType());
  if (block.getNumArguments() != 1 || block.getArgument(0).getType() != element)
    return emitOpError("predicate argument must match the ac.var element");
  for (Operation &operation : block.without_terminator())
    if (!isa<SlotGetOp, VarReadOp, VarReadElementOp>(operation) &&
        !isPureExpressionOperation(&operation))
      return emitOpError() << "predicate operation '" << operation.getName()
                           << "' is not permitted";
  auto yield = dyn_cast<VarMatchYieldOp>(block.getTerminator());
  if (!yield ||
      !cast<VarType>(yield.getValue().getType()).getElementType().isInteger(1))
    return emitOpError("predicate must yield !ac.var<i1>");
  return success();
}

LogicalResult VarChooseOp::verify() {
  auto collection = resolveVarCollection(*this, getVariableAttr());
  if (failed(collection))
    return failure();
  VarDeclOp variable = collection->first;
  const int64_t entries = collection->second;
  auto match = getMask().getDefiningOp<VarMatchOp>();
  if (!match)
    return emitOpError(
        "candidate mask must be produced directly by ac.var.match");
  if (resolveVarDecl(match, match.getVariableAttr()) != variable)
    return emitOpError("candidate mask must come from the same ac.var");
  const int64_t domainEntries =
      match.getRow() ? variable.getShapeAttr().asArrayRef().back() : entries;
  if (!isCandidateMaskType(getMask().getType(), domainEntries))
    return emitOpError(match.getRow()
                           ? "candidate mask must exactly cover the projected "
                             "ac.var row"
                           : "candidate mask must exactly cover the ac.var "
                             "domain in 64-bit words");
  if (getCount() != 1)
    return emitOpError("choose supports count=1 only");
  if (getPolicy() != "first" && getPolicy() != "min" && getPolicy() != "max")
    return emitOpError("policy must be first, min, or max");
  unsigned indexWidth =
      std::max<unsigned>(1, llvm::Log2_64_Ceil(static_cast<uint64_t>(entries)));
  if (getIndex().getType() !=
      VarType::get(getContext(), IntegerType::get(getContext(), indexWidth)))
    return emitOpError("index result width must address the ac.var domain");
  if (getValid().getType() !=
      VarType::get(getContext(), IntegerType::get(getContext(), 1)))
    return emitOpError("valid result must be !ac.var<i1>");
  if (getPolicy() == "first") {
    if (!getKey().empty() &&
        !(getKey().hasOneBlock() && getKey().front().empty()))
      return emitOpError("first policy does not accept a key region");
    return success();
  }
  if (!getKey().hasOneBlock())
    return emitOpError("min/max policy requires one key region");
  Block &block = getKey().front();
  Type element = VarType::get(getContext(), variable.getValueType());
  if (block.getNumArguments() != 1 || block.getArgument(0).getType() != element)
    return emitOpError("key argument must match the ac.var element");
  for (Operation &operation : block.without_terminator()) {
    if (isa<VarReadOp, VarReadElementOp>(operation))
      return emitOpError() << "key operation '" << operation.getName()
                           << "' is not permitted";
    if (!isPureExpressionOperation(&operation))
      return emitOpError() << "key operation '" << operation.getName()
                           << "' is not permitted";
  }
  auto yield = dyn_cast<VarChooseYieldOp>(block.getTerminator());
  auto key =
      yield ? dyn_cast<IntegerType>(
                  cast<VarType>(yield.getValue().getType()).getElementType())
            : IntegerType();
  if (!key || key.getWidth() == 0 || key.getWidth() > 64)
    return emitOpError(
        "min/max key must yield an unsigned fixed-width integer");
  return success();
}

static LogicalResult verifyVarBinary(Operation *operation, Value lhs, Value rhs,
                                     Value result) {
  if (lhs.getType() != rhs.getType() || lhs.getType() != result.getType())
    return operation->emitOpError(
        "operands and result must have one identical Var type");
  Type element = cast<VarType>(result.getType()).getElementType();
  if (!isa<IntegerType, FloatType>(element))
    return operation->emitOpError(
        "arithmetic Var element must be an integer or float");
  return success();
}

LogicalResult VarAddOp::verify() {
  return verifyVarBinary(*this, getLhs(), getRhs(), getResult());
}

LogicalResult VarSubOp::verify() {
  return verifyVarBinary(*this, getLhs(), getRhs(), getResult());
}

LogicalResult VarMulOp::verify() {
  return verifyVarBinary(*this, getLhs(), getRhs(), getResult());
}

static LogicalResult verifyVarUnsignedBinary(Operation *operation, Value lhs,
                                             Value rhs, Value result) {
  if (lhs.getType() != rhs.getType() || lhs.getType() != result.getType())
    return operation->emitOpError(
        "operands and result must have one identical Var type");
  auto element =
      dyn_cast<IntegerType>(cast<VarType>(result.getType()).getElementType());
  if (!element || !element.isSignless() || element.getWidth() == 0 ||
      element.getWidth() > 64)
    return operation->emitOpError(
        "unsigned arithmetic Var element must be a signless integer with width "
        "in [1, 64]");
  return success();
}

LogicalResult VarUDivOp::verify() {
  return verifyVarUnsignedBinary(*this, getLhs(), getRhs(), getResult());
}

LogicalResult VarURemOp::verify() {
  return verifyVarUnsignedBinary(*this, getLhs(), getRhs(), getResult());
}

namespace {

template <typename SourceOp, typename TargetOp, bool IsRemainder>
struct CanonicalizeUnsignedPowerOfTwo final : OpRewritePattern<SourceOp> {
  using OpRewritePattern<SourceOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(SourceOp operation,
                                PatternRewriter &rewriter) const override {
    auto divisor = operation.getRhs().template getDefiningOp<VarConstantOp>();
    auto value =
        divisor ? dyn_cast<IntegerAttr>(divisor.getValue()) : IntegerAttr();
    if (!value || !value.getValue().isPowerOf2())
      return failure();
    auto resultType = cast<VarType>(operation.getResult().getType());
    auto integerType = cast<IntegerType>(resultType.getElementType());
    const uint64_t divisorValue = value.getValue().getZExtValue();
    const uint64_t replacementValue =
        IsRemainder ? divisorValue - 1 : llvm::Log2_64(divisorValue);
    auto replacementConstant = VarConstantOp::create(
        rewriter, operation.getLoc(), resultType,
        rewriter.getIntegerAttr(integerType, replacementValue));
    auto replacement =
        TargetOp::create(rewriter, operation.getLoc(), resultType,
                         operation.getLhs(), replacementConstant);
    replacement->setDiscardableAttrs(operation->getDiscardableAttrDictionary());
    rewriter.replaceOp(operation, replacement.getResult());
    return success();
  }
};

} // namespace

void VarUDivOp::getCanonicalizationPatterns(RewritePatternSet &patterns,
                                            MLIRContext *context) {
  patterns.add<CanonicalizeUnsignedPowerOfTwo<VarUDivOp, VarShrOp, false>>(
      context);
}

void VarURemOp::getCanonicalizationPatterns(RewritePatternSet &patterns,
                                            MLIRContext *context) {
  patterns.add<CanonicalizeUnsignedPowerOfTwo<VarURemOp, VarAndOp, true>>(
      context);
}

static LogicalResult verifyVarBitBinary(Operation *operation, Value lhs,
                                        Value rhs, Value result) {
  if (lhs.getType() != rhs.getType() || lhs.getType() != result.getType())
    return operation->emitOpError(
        "operands and result must have one identical Var type");
  auto element =
      dyn_cast<IntegerType>(cast<VarType>(result.getType()).getElementType());
  if (!element)
    return operation->emitOpError(
        "bit operation Var element must be an integer");
  if (!element.isSignless() || element.getWidth() == 0 ||
      element.getWidth() > 64)
    return operation->emitOpError(
        "bit operation Var element must be a signless integer with width in "
        "[1, 64]");
  return success();
}

LogicalResult VarAndOp::verify() {
  return verifyVarBitBinary(*this, getLhs(), getRhs(), getResult());
}

LogicalResult VarOrOp::verify() {
  return verifyVarBitBinary(*this, getLhs(), getRhs(), getResult());
}

LogicalResult VarXorOp::verify() {
  return verifyVarBitBinary(*this, getLhs(), getRhs(), getResult());
}

LogicalResult VarShlOp::verify() {
  return verifyVarBitBinary(*this, getLhs(), getRhs(), getResult());
}

LogicalResult VarShrOp::verify() {
  return verifyVarBitBinary(*this, getLhs(), getRhs(), getResult());
}

LogicalResult VarMatchesOp::verify() {
  auto input = dyn_cast<IntegerType>(
      cast<VarType>(getInput().getType()).getElementType());
  if (!input || !input.isSignless() || input.getWidth() == 0 ||
      input.getWidth() > 64)
    return emitOpError(
        "input must be an ac.var carrying a signless i1..i64 integer");
  if (getResult().getType() !=
      VarType::get(getContext(), IntegerType::get(getContext(), 1)))
    return emitOpError("result must be !ac.var<i1>");
  const uint64_t widthMask = input.getWidth() == 64
                                 ? std::numeric_limits<uint64_t>::max()
                                 : (uint64_t{1} << input.getWidth()) - 1;
  if ((getMask() & ~widthMask) != 0 || (getValue() & ~widthMask) != 0)
    return emitOpError("mask and value must fit the input width");
  if ((getValue() & ~getMask()) != 0)
    return emitOpError("value may set only bits selected by mask");
  return success();
}

LogicalResult VarNotOp::verify() {
  if (getIn().getType() != getResult().getType())
    return emitOpError("operand and result must have one identical Var type");
  auto element = dyn_cast<IntegerType>(
      cast<VarType>(getResult().getType()).getElementType());
  if (!element)
    return emitOpError("bit operation Var element must be an integer");
  if (!element.isSignless() || element.getWidth() == 0 ||
      element.getWidth() > 64)
    return emitOpError(
        "bit operation Var element must be a signless integer with width in "
        "[1, 64]");
  return success();
}

LogicalResult VarPopcountOp::verify() {
  auto input = cast<VarType>(getIn().getType());
  auto result = cast<VarType>(getResult().getType());
  auto inputInt = dyn_cast<IntegerType>(input.getElementType());
  auto resultInt = dyn_cast<IntegerType>(result.getElementType());
  if (!inputInt || !resultInt)
    return emitOpError("input and result payloads must be integer types");
  if (!acir::isPrimitiveInputWidth(inputInt.getWidth()))
    return emitOpError("input width must be in [1, 64]");
  const unsigned required = acir::primitiveCountWidth(inputInt.getWidth());
  if (resultInt.getWidth() != required)
    return emitOpError()
           << "result width must be ceil(log2(input_width + 1)) = " << required;
  return success();
}

LogicalResult VarCountZerosOp::verify() {
  auto input = cast<VarType>(getIn().getType());
  auto result = cast<VarType>(getResult().getType());
  auto inputInt = dyn_cast<IntegerType>(input.getElementType());
  auto resultInt = dyn_cast<IntegerType>(result.getElementType());
  if (!inputInt || !resultInt)
    return emitOpError("input and result payloads must be integer types");
  if (!acir::isPrimitiveInputWidth(inputInt.getWidth()))
    return emitOpError("input width must be in [1, 64]");
  const unsigned required = acir::primitiveCountWidth(inputInt.getWidth());
  if (resultInt.getWidth() != required)
    return emitOpError()
           << "result width must be ceil(log2(input_width + 1)) = " << required;
  if (getDirection() != "leading" && getDirection() != "trailing")
    return emitOpError("direction must be leading or trailing");
  return success();
}

LogicalResult VarPriorityEncodeOp::verify() {
  auto input = cast<VarType>(getIn().getType());
  auto index = cast<VarType>(getIndex().getType());
  auto valid = cast<VarType>(getValid().getType());
  auto inputInteger = dyn_cast<IntegerType>(input.getElementType());
  auto indexInteger = dyn_cast<IntegerType>(index.getElementType());
  if (!inputInteger || !inputInteger.isSignless() ||
      !acir::isPrimitiveInputWidth(inputInteger.getWidth()))
    return emitOpError("input must carry a signless integer width in [1, 64]");
  if (!indexInteger || !indexInteger.isSignless())
    return emitOpError("index must carry a signless integer");
  const unsigned required =
      acir::primitivePriorityIndexWidth(inputInteger.getWidth());
  if (indexInteger.getWidth() != required)
    return emitOpError()
           << "index width must be max(1, ceil(log2(input_width))) = "
           << required;
  if (valid.getElementType() != IntegerType::get(getContext(), 1))
    return emitOpError("valid must be !ac.var<i1>");
  if (getOrder() != "low" && getOrder() != "high")
    return emitOpError("order must be low or high");
  return success();
}

static bool
supportsRecursiveEquality(Operation *operation, Type type,
                          llvm::SmallPtrSetImpl<Operation *> &seen) {
  if (isa<IntegerType, RangeType, EnumType>(type))
    return true;
  if (auto tuple = dyn_cast<TupleType>(type))
    return llvm::all_of(tuple.getTypes(), [&](Type element) {
      return supportsRecursiveEquality(operation, element, seen);
    });
  if (auto array = dyn_cast<ValueArrayType>(type))
    return supportsRecursiveEquality(operation, array.getElementType(), seen);
  if (!isa<StructType>(type))
    return false;
  Operation *declaration = recordDecl(operation, type);
  if (!declaration || !seen.insert(declaration).second)
    return false;
  bool supported =
      llvm::all_of(declarationFields(declaration), [&](Attribute attribute) {
        return supportsRecursiveEquality(
            operation, fieldType(cast<DictionaryAttr>(attribute)), seen);
      });
  seen.erase(declaration);
  return supported;
}

LogicalResult VarCmpOp::verify() {
  if (getLhs().getType() != getRhs().getType())
    return emitOpError("operands must have the same Var type");
  Type payload = cast<VarType>(getLhs().getType()).getElementType();
  llvm::SmallPtrSet<Operation *, 8> seen;
  const bool aggregate = isa<StructType, TupleType, ValueArrayType>(payload);
  if (!supportsRecursiveEquality(*this, payload, seen))
    return emitOpError(
        "operands must carry recursively comparable integer, enum, struct, "
        "tuple, or value_array payloads");
  if (isa<EnumType>(payload) && getPredicate() != "eq" &&
      getPredicate() != "ne")
    return emitOpError("enum comparison supports only eq or ne");
  if (aggregate && getPredicate() != "eq" && getPredicate() != "ne")
    return emitOpError("aggregate comparison supports only eq or ne");
  if (getResult().getType() !=
      VarType::get(getContext(), IntegerType::get(getContext(), 1)))
    return emitOpError("result must be !ac.var<i1>");
  if (!llvm::is_contained(ArrayRef<StringRef>{"eq", "ne", "slt", "sle", "sgt",
                                              "sge", "ult", "ule", "ugt",
                                              "uge"},
                          getPredicate()))
    return emitOpError(
        "predicate must be eq, ne, slt, sle, sgt, sge, ult, ule, ugt, or uge");
  return success();
}

static bool isInvariantIdentifier(StringRef value) {
  if (value.empty() || (!llvm::isAlpha(value.front()) && value.front() != '_'))
    return false;
  return llvm::all_of(value.drop_front(), [](char character) {
    return llvm::isAlnum(character) || character == '_';
  });
}

LogicalResult VarInvariantOp::verify() {
  auto input = cast<VarType>(getInput().getType());
  auto fail = [&](Twine message) {
    return emitOpError() << "invariant '" << getName() << "' for "
                         << input.getElementType() << ": " << message;
  };
  if (getName().empty())
    return fail("name must be non-empty");
  if (!isa<StructType>(input.getElementType()) ||
      !recordDecl(*this, input.getElementType()))
    return fail("input must carry a resolved nominal ac.struct payload");
  auto structure = cast<StructType>(input.getElementType());
  StringRef payloadName =
      cast<SymbolRefAttr>(structure.getName()).getLeafReference().getValue();
  auto [namePayload, nameFunction] = getName().split('.');
  if (namePayload != payloadName || !isInvariantIdentifier(nameFunction))
    return fail("name must have exact '<Payload>.<function>' form");
  for (auto ancestor = (*this)->getParentOfType<VarInvariantOp>(); ancestor;
       ancestor = ancestor->getParentOfType<VarInvariantOp>())
    if (ancestor.getName() == getName())
      return fail("recursive invariant call repeats an ancestor name");
  if (getResult().getType() !=
      VarType::get(getContext(), IntegerType::get(getContext(), 1)))
    return fail("result must be !ac.var<i1>");
  Block &block = getPredicate().front();
  if (block.getNumArguments() != 1 ||
      block.getArgument(0).getType() != getInput().getType())
    return fail("predicate must take exactly one argument matching the input");
  for (Operation &nested : block) {
    if (isa<VarInvariantYieldOp>(nested))
      continue;
    const bool nestedInvariant = isa<VarInvariantOp>(nested);
    if (nested.getDialect() != getOperation()->getDialect() ||
        (nested.getNumRegions() != 0 && !nestedInvariant))
      return fail(Twine("unsupported predicate operation '") +
                  nested.getName().getStringRef() + "'");
    if (!nestedInvariant && !isMemoryEffectFree(&nested))
      return fail(Twine("unsupported effectful predicate operation '") +
                  nested.getName().getStringRef() + "'");
    for (Value operand : nested.getOperands()) {
      if (operand == block.getArgument(0))
        continue;
      Operation *definition = operand.getDefiningOp();
      if (!definition || definition->getParentRegion() != &getPredicate())
        return fail("predicate captures a value outside its input region");
    }
  }
  auto yielded = dyn_cast<VarInvariantYieldOp>(block.getTerminator());
  if (!yielded)
    return fail("predicate must terminate with ac.var.invariant.yield");
  if (yielded.getValue().getType() !=
      VarType::get(getContext(), IntegerType::get(getContext(), 1)))
    return fail("predicate must yield !ac.var<i1>");
  Value yieldedValue = yielded.getValue();
  if (auto argument = dyn_cast<BlockArgument>(yieldedValue)) {
    if (argument.getOwner() != &block)
      return fail("predicate yield captures a value outside its input region");
  } else {
    Operation *definition = yieldedValue.getDefiningOp();
    if (!definition || definition->getParentRegion() != &getPredicate())
      return fail("predicate yield captures a value outside its input region");
  }
  return success();
}

LogicalResult VarInvariantYieldOp::verify() {
  if (getValue().getType() !=
      VarType::get(getContext(), IntegerType::get(getContext(), 1)))
    return emitOpError("value must be !ac.var<i1>");
  return success();
}

LogicalResult VarSelectOp::verify() {
  if (failed(verifyI1VarCondition(*this, getCondition())))
    return failure();
  if (getTrueValue().getType() != getFalseValue().getType() ||
      getResult().getType() != getTrueValue().getType())
    return emitOpError("selected values and result must have one Var type");
  return success();
}

static FailureOr<std::pair<int64_t, int64_t>> bitfieldRange(BitfieldOp schema,
                                                            StringRef name) {
  for (Attribute attribute : schema.getFields()) {
    DictionaryAttr field = cast<DictionaryAttr>(attribute);
    if (cast<StringAttr>(field.get("name")).getValue() == name)
      return std::make_pair(cast<IntegerAttr>(field.get("lsb")).getInt(),
                            cast<IntegerAttr>(field.get("msb")).getInt());
  }
  return failure();
}

static FailureOr<BitfieldOp> bitfieldProvenance(Operation *operation,
                                                StringRef selectionAttribute) {
  Attribute schemaAttribute = operation->getAttr("ac.bitfield_schema");
  Attribute selection = operation->getAttr(selectionAttribute);
  if (!schemaAttribute && !selection)
    return BitfieldOp();
  auto reference = dyn_cast_or_null<SymbolRefAttr>(schemaAttribute);
  if (!reference || !selection) {
    operation->emitOpError("bitfield provenance requires schema and selection");
    return failure();
  }
  auto schema = dyn_cast_or_null<BitfieldOp>(lookup(operation, reference));
  if (!schema) {
    operation->emitOpError("bitfield schema reference does not resolve");
    return failure();
  }
  return schema;
}

static LogicalResult verifyNamedBitfieldProvenance(Operation *operation,
                                                   unsigned baseWidth,
                                                   int64_t lsb, int64_t width) {
  FailureOr<BitfieldOp> resolved =
      bitfieldProvenance(operation, "ac.bitfield_field");
  if (failed(resolved))
    return failure();
  if (!*resolved)
    return success();
  auto field = operation->getAttrOfType<StringAttr>("ac.bitfield_field");
  if (!field)
    return operation->emitOpError("bitfield field must be a string");
  FailureOr<std::pair<int64_t, int64_t>> range =
      bitfieldRange(*resolved, field.getValue());
  if (failed(range))
    return operation->emitOpError("bitfield field is absent from its schema");
  if ((*resolved).getWidth() != baseWidth || range->first != lsb ||
      range->second - range->first + 1 != width)
    return operation->emitOpError(
        "bitfield field range does not match its schema");
  return success();
}

static LogicalResult verifyConcatBitfieldProvenance(VarConcatOp operation) {
  FailureOr<BitfieldOp> resolved =
      bitfieldProvenance(operation.getOperation(), "ac.bitfield_fields");
  if (failed(resolved))
    return failure();
  if (!*resolved)
    return success();
  auto fields = operation->getAttrOfType<ArrayAttr>("ac.bitfield_fields");
  if (!fields || fields.size() != operation.getInputs().size())
    return operation.emitOpError(
        "bitfield field list must match concat input arity");
  for (auto [attribute, input] :
       llvm::zip_equal(fields, operation.getInputs())) {
    auto field = dyn_cast<StringAttr>(attribute);
    if (!field)
      return operation.emitOpError("bitfield field names must be strings");
    FailureOr<std::pair<int64_t, int64_t>> range =
        bitfieldRange(*resolved, field.getValue());
    if (failed(range))
      return operation.emitOpError(
          "bitfield concat field is absent from its schema");
    unsigned inputWidth =
        cast<IntegerType>(cast<VarType>(input.getType()).getElementType())
            .getWidth();
    if (range->second - range->first + 1 != inputWidth)
      return operation.emitOpError(
          "bitfield concat input width does not match its field");
  }
  return success();
}

LogicalResult VarExtractOp::verify() {
  auto input = dyn_cast<IntegerType>(
      cast<VarType>(getInput().getType()).getElementType());
  auto result = dyn_cast<IntegerType>(
      cast<VarType>(getResult().getType()).getElementType());
  if (!input || !result || input.getWidth() == 0 || input.getWidth() > 64)
    return emitOpError("input and result must be 1..64-bit integer Vars");
  if (getLsb() < 0 || getWidth() <= 0 ||
      static_cast<uint64_t>(getLsb()) + static_cast<uint64_t>(getWidth()) >
          input.getWidth())
    return emitOpError("slice must be non-empty and within the input width");
  if (result.getWidth() != static_cast<unsigned>(getWidth()))
    return emitOpError("result width must equal the extracted width");
  return verifyNamedBitfieldProvenance(getOperation(), input.getWidth(),
                                       getLsb(), getWidth());
}

LogicalResult VarConcatOp::verify() {
  if (getInputs().empty())
    return emitOpError("requires at least one input");
  uint64_t totalWidth = 0;
  for (Value input : getInputs()) {
    auto integer =
        dyn_cast<IntegerType>(cast<VarType>(input.getType()).getElementType());
    if (!integer || integer.getWidth() == 0 || integer.getWidth() > 64)
      return emitOpError("inputs must be 1..64-bit integer Vars");
    totalWidth += integer.getWidth();
  }
  auto result = dyn_cast<IntegerType>(
      cast<VarType>(getResult().getType()).getElementType());
  if (!result || totalWidth == 0 || totalWidth > 64)
    return emitOpError("concatenated width must be in [1, 64]");
  if (result.getWidth() != totalWidth)
    return emitOpError("result width must equal the sum of input widths");
  return verifyConcatBitfieldProvenance(*this);
}

static unsigned rangeStorageWidth(RangeType range) {
  const uint64_t upper = range.getUpper();
  return upper == std::numeric_limits<uint64_t>::max()
             ? 64
             : std::max(1u, llvm::Log2_64_Ceil(upper + 1));
}

static bool isUnsignedScalar(Type type) {
  if (auto integer = dyn_cast<IntegerType>(type))
    return integer.isSignless() && integer.getWidth() > 0 &&
           integer.getWidth() <= 64;
  return isa<RangeType>(type);
}

static LogicalResult verifyRangeConversion(Operation *operation, Value input,
                                           Value result) {
  Type inputElement = cast<VarType>(input.getType()).getElementType();
  auto resultRange =
      dyn_cast<RangeType>(cast<VarType>(result.getType()).getElementType());
  if (!isUnsignedScalar(inputElement) || !resultRange)
    return operation->emitOpError(
        "range conversion requires unsigned scalar input and range result");
  return success();
}

LogicalResult VarRangeWrapOp::verify() {
  return verifyRangeConversion(*this, getInput(), getResult());
}

LogicalResult VarRangeSaturateOp::verify() {
  return verifyRangeConversion(*this, getInput(), getResult());
}

LogicalResult VarRangeCheckedOp::verify() {
  if (failed(verifyRangeConversion(*this, getInput(), getValue())))
    return failure();
  if (getValid().getType() !=
      VarType::get(getContext(), IntegerType::get(getContext(), 1)))
    return emitOpError("checked range validity must be !ac.var<i1>");
  return success();
}

LogicalResult VarRangeRefineOp::verify() {
  return verifyRangeConversion(*this, getInput(), getResult());
}

LogicalResult VarRangeBitsOp::verify() {
  auto inputRange =
      dyn_cast<RangeType>(cast<VarType>(getInput().getType()).getElementType());
  auto resultInteger = dyn_cast<IntegerType>(
      cast<VarType>(getResult().getType()).getElementType());
  if (!inputRange || !resultInteger || !resultInteger.isSignless() ||
      resultInteger.getWidth() != rangeStorageWidth(inputRange))
    return emitOpError(
        "range_bits result must be the exact unsigned storage width");
  return success();
}

static LogicalResult verifyRangeArithmetic(Operation *operation, Value lhs,
                                           Value rhs, Value result,
                                           bool subtract) {
  auto left =
      dyn_cast<RangeType>(cast<VarType>(lhs.getType()).getElementType());
  auto right =
      dyn_cast<RangeType>(cast<VarType>(rhs.getType()).getElementType());
  auto actual =
      dyn_cast<RangeType>(cast<VarType>(result.getType()).getElementType());
  if (!left || !right || !actual)
    return operation->emitOpError(
        "bounded arithmetic requires range operands and result");
  uint64_t lower = 0;
  uint64_t upper = 0;
  if (subtract) {
    if (left.getLower() < right.getUpper())
      return operation->emitOpError(
          "bounded subtraction may produce a negative result");
    lower = left.getLower() - right.getUpper();
    upper = left.getUpper() - right.getLower();
  } else if (right.getLower() >
                 std::numeric_limits<uint64_t>::max() - left.getLower() ||
             right.getUpper() >
                 std::numeric_limits<uint64_t>::max() - left.getUpper()) {
    return operation->emitOpError("bounded addition exceeds the u64 domain");
  } else {
    lower = left.getLower() + right.getLower();
    upper = left.getUpper() + right.getUpper();
  }
  if (actual.getLower() != lower || actual.getUpper() != upper)
    return operation->emitOpError(
        "bounded arithmetic result range is inconsistent");
  return success();
}

LogicalResult VarRangeAddOp::verify() {
  return verifyRangeArithmetic(*this, getLhs(), getRhs(), getResult(), false);
}

LogicalResult VarRangeSubOp::verify() {
  return verifyRangeArithmetic(*this, getLhs(), getRhs(), getResult(), true);
}

LogicalResult VarRangeCmpOp::verify() {
  if (!isa<RangeType>(cast<VarType>(getLhs().getType()).getElementType()) ||
      !isa<RangeType>(cast<VarType>(getRhs().getType()).getElementType()))
    return emitOpError("bounded comparison requires range operands");
  if (!llvm::is_contained(
          ArrayRef<StringRef>{"eq", "ne", "ult", "ule", "ugt", "uge"},
          getPredicate()))
    return emitOpError("bounded comparison predicate is unsupported");
  if (getResult().getType() !=
      VarType::get(getContext(), IntegerType::get(getContext(), 1)))
    return emitOpError("bounded comparison result must be !ac.var<i1>");
  return success();
}

LogicalResult VarInsertOp::verify() {
  auto base = dyn_cast<IntegerType>(
      cast<VarType>(getBase().getType()).getElementType());
  auto value = dyn_cast<IntegerType>(
      cast<VarType>(getValue().getType()).getElementType());
  if (!base || !value || base.getWidth() == 0 || base.getWidth() > 64 ||
      value.getWidth() == 0 || value.getWidth() > 64)
    return emitOpError("base and value must be 1..64-bit integer Vars");
  if (getResult().getType() != getBase().getType())
    return emitOpError("result must preserve the base Var type");
  if (getLsb() < 0 ||
      static_cast<uint64_t>(getLsb()) + value.getWidth() > base.getWidth())
    return emitOpError("inserted range must be within the base width");
  return verifyNamedBitfieldProvenance(getOperation(), base.getWidth(),
                                       getLsb(), value.getWidth());
}

namespace {

Location projectionSourceLocation(Location location) {
  if (auto file = dyn_cast<FileLineColLoc>(location))
    return file.getFilename().getValue().ends_with(".py")
               ? location
               : UnknownLoc::get(location.getContext());
  if (auto named = dyn_cast<NameLoc>(location)) {
    Location child = projectionSourceLocation(named.getChildLoc());
    return isa<UnknownLoc>(child)
               ? child
               : Location(NameLoc::get(named.getName(), child));
  }
  if (auto call = dyn_cast<CallSiteLoc>(location)) {
    Location callee = projectionSourceLocation(call.getCallee());
    Location caller = projectionSourceLocation(call.getCaller());
    if (isa<UnknownLoc>(callee))
      return caller;
    if (isa<UnknownLoc>(caller))
      return callee;
    return CallSiteLoc::get(callee, caller);
  }
  if (auto fused = dyn_cast<FusedLoc>(location)) {
    SmallVector<Location> children;
    for (Location child : fused.getLocations()) {
      child = projectionSourceLocation(child);
      if (!isa<UnknownLoc>(child) && !llvm::is_contained(children, child))
        children.push_back(child);
    }
    if (children.empty())
      return UnknownLoc::get(location.getContext());
    if (children.size() == 1)
      return children.front();
    return FusedLoc::get(location.getContext(), children);
  }
  return UnknownLoc::get(location.getContext());
}

Location mergeProjectionLocations(Location retained, Location removed) {
  Location retainedSource = projectionSourceLocation(retained);
  Location removedSource = projectionSourceLocation(removed);
  if (isa<UnknownLoc>(removedSource))
    return retained;
  if (isa<UnknownLoc>(retainedSource))
    return removedSource;
  if (retainedSource == removedSource)
    return retainedSource;
  return FusedLoc::get(retained.getContext(), {retainedSource, removedSource});
}

void preserveProjectionLocation(Value replacement, Location removed) {
  if (Operation *producer = replacement.getDefiningOp()) {
    producer->setLoc(mergeProjectionLocations(producer->getLoc(), removed));
    return;
  }
  if (auto argument = dyn_cast<BlockArgument>(replacement))
    argument.setLoc(mergeProjectionLocations(argument.getLoc(), removed));
}

struct FoldGetFromRecord final : OpRewritePattern<VarGetOp> {
  using OpRewritePattern::OpRewritePattern;

  LogicalResult matchAndRewrite(VarGetOp operation,
                                PatternRewriter &rewriter) const override {
    auto record = operation.getRecord().getDefiningOp<VarRecordOp>();
    if (!record)
      return failure();
    auto recordType = cast<VarType>(record.getResult().getType());
    Operation *declaration = recordDecl(operation, recordType.getElementType());
    auto index = declaration ? findField(declaration, operation.getField())
                             : std::nullopt;
    if (!index || *index >= record.getValues().size())
      return failure();
    Value replacement = record.getValues()[*index];
    preserveProjectionLocation(
        replacement,
        mergeProjectionLocations(record.getLoc(), operation.getLoc()));
    rewriter.replaceOp(operation, replacement);
    return success();
  }
};

struct FoldGetThroughWith final : OpRewritePattern<VarGetOp> {
  using OpRewritePattern::OpRewritePattern;

  LogicalResult matchAndRewrite(VarGetOp operation,
                                PatternRewriter &rewriter) const override {
    auto update = operation.getRecord().getDefiningOp<VarWithOp>();
    if (!update)
      return failure();
    if (operation.getField() == update.getField()) {
      Value replacement = update.getValue();
      preserveProjectionLocation(
          replacement,
          mergeProjectionLocations(update.getLoc(), operation.getLoc()));
      rewriter.replaceOp(operation, replacement);
      return success();
    }
    rewriter.modifyOpInPlace(operation, [&] {
      operation->setLoc(
          mergeProjectionLocations(operation.getLoc(), update.getLoc()));
      operation.getRecordMutable().assign(update.getRecord());
    });
    return success();
  }
};

Value createProjectedGet(PatternRewriter &rewriter, VarGetOp operation,
                         Value record) {
  OperationState state(operation.getLoc(), VarGetOp::getOperationName());
  state.addOperands(record);
  state.addTypes(operation.getResult().getType());
  state.addAttribute("field", operation.getFieldAttr());
  return rewriter.create(state)->getResult(0);
}

struct PushGetThroughSelect final : OpRewritePattern<VarGetOp> {
  using OpRewritePattern::OpRewritePattern;

  LogicalResult matchAndRewrite(VarGetOp operation,
                                PatternRewriter &rewriter) const override {
    auto select = operation.getRecord().getDefiningOp<VarSelectOp>();
    if (!select || !select.getResult().hasOneUse())
      return failure();
    operation->setLoc(
        mergeProjectionLocations(operation.getLoc(), select.getLoc()));
    Value trueValue =
        createProjectedGet(rewriter, operation, select.getTrueValue());
    Value falseValue =
        createProjectedGet(rewriter, operation, select.getFalseValue());
    OperationState state(operation.getLoc(), VarSelectOp::getOperationName());
    state.addOperands({select.getCondition(), trueValue, falseValue});
    state.addTypes(operation.getResult().getType());
    rewriter.replaceOp(operation, rewriter.create(state)->getResults());
    return success();
  }
};

} // namespace

LogicalResult VarGetOp::verify() {
  auto record = cast<VarType>(getRecord().getType());
  Operation *decl = recordDecl(*this, record.getElementType());
  if (!decl)
    return emitOpError("requires a record-like Var operand");
  auto index = findField(decl, getField());
  if (!index)
    return emitOpError() << "unknown field '" << getField() << "'";
  Type expected = VarType::get(getContext(), fieldType(decl, *index));
  if (getResult().getType() != expected)
    return emitOpError() << "field '" << getField() << "' result must be "
                         << expected;
  return success();
}

void VarGetOp::getCanonicalizationPatterns(RewritePatternSet &patterns,
                                           MLIRContext *context) {
  patterns.add<FoldGetFromRecord, FoldGetThroughWith, PushGetThroughSelect>(
      context);
}

LogicalResult VarWithOp::verify() {
  if (getRecord().getType() != getResult().getType())
    return emitOpError("must preserve record Var identity");
  auto record = cast<VarType>(getRecord().getType());
  Operation *decl = recordDecl(*this, record.getElementType());
  if (!decl)
    return emitOpError("requires a record-like Var operand");
  auto index = findField(decl, getField());
  if (!index)
    return emitOpError() << "unknown field '" << getField() << "'";
  Type expected = VarType::get(getContext(), fieldType(decl, *index));
  if (getValue().getType() != expected)
    return emitOpError() << "field '" << getField() << "' expects " << expected;
  return success();
}

static std::string queueScopePath(Operation *operation) {
  SmallVector<StringRef> parts;
  for (Operation *parent = operation->getParentOp(); parent;
       parent = parent->getParentOp())
    if (auto scope = dyn_cast<ScopeOp>(parent))
      parts.push_back(scope.getSymName());
  std::string path;
  for (StringRef part : llvm::reverse(parts)) {
    path.push_back('/');
    path.append(part);
  }
  return path.empty() ? "/" : path;
}

static MemoryInstanceOp resolveMemoryInstance(Operation *root,
                                              FlatSymbolRefAttr reference) {
  MemoryInstanceOp resolved;
  root->walk([&](MemoryInstanceOp candidate) {
    if (candidate.getSymName() == reference.getValue())
      resolved = candidate;
  });
  return resolved;
}

LogicalResult MemoryInstanceOp::verify() {
  auto data = dyn_cast<IntegerType>(getDataType());
  if (!data || data.getWidth() == 0 || data.getWidth() > 64)
    return emitOpError("data type must be an integer no wider than 64 bits");
  if (getEntries() <= 0 || getLatency() <= 0)
    return emitOpError("entries and latency must be positive");
  if (getInit() != 0)
    return emitOpError("memory init must be zero");
  if (getOwner().empty() || !getOwner().starts_with('/') ||
      (getOwner().size() > 1 && getOwner().ends_with('/')))
    return emitOpError("owner must be a canonical absolute scope path");
  if (getStableId().empty())
    return emitOpError("stable_id must be non-empty");

  Operation *root = getOperation();
  while (root->getParentOp())
    root = root->getParentOp();
  std::string expectedStableId = "memory/";
  if (getOwner() != "/") {
    expectedStableId.append(getOwner().drop_front());
    expectedStableId.push_back('/');
  }
  expectedStableId.append(getSymName());
  if (getStableId() != expectedStableId)
    return emitOpError("stable_id must match canonical owner/symbol identity");
  bool ownerExists = getOwner() == "/";
  bool duplicateStableId = false;
  root->walk([&](Operation *operation) {
    if (auto scope = dyn_cast<ScopeOp>(operation)) {
      std::string path = queueScopePath(scope);
      if (path != "/")
        path.push_back('/');
      path.append(scope.getSymName());
      ownerExists |= path == getOwner();
    }
    if (auto other = dyn_cast<MemoryInstanceOp>(operation))
      duplicateStableId |=
          other != *this && other.getStableId() == getStableId();
  });
  if (!ownerExists)
    return emitOpError("owner does not name a declared scope path");
  if (duplicateStableId)
    return emitOpError("stable_id must be unique");
  unsigned requests = 0;
  root->walk([&](MemoryRequestOp request) {
    // Resolve in the graph file rather than through the nearest symbol table:
    // an enclosing ac.scope is itself a symbol table, so the nearest-table
    // lookup cannot see an instance declared beside the request's scope.
    auto resolved = dyn_cast_or_null<MemoryInstanceOp>(
        lookupGraphSymbol(request, request.getInstanceAttr()));
    if (resolved == *this)
      ++requests;
  });
  if (requests == 0)
    return emitOpError("must have at least one memory.request endpoint");
  return success();
}

LogicalResult MemoryRequestOp::verify() {
  if (getInput().getType() != getOutput().getType())
    return emitOpError("output queue must match input queue type");
  if (getOrdinal() < 0 || getDepth() <= 0)
    return emitOpError("ordinal must be non-negative and depth positive");

  // Resolve in the graph file: an enclosing ac.scope is itself a symbol
  // table, so the nearest-table lookup cannot see an instance declared beside
  // the request's scope.
  auto instance = dyn_cast_or_null<MemoryInstanceOp>(
      lookupGraphSymbol(*this, getInstanceAttr()));
  if (!instance)
    return emitOpError() << "unresolved memory instance " << getInstance();
  const std::string requestScope = queueScopePath(*this);
  StringRef owner = instance.getOwner();
  StringRef requestPath(requestScope);
  const bool visible =
      owner == "/" || requestPath == owner ||
      (requestPath.size() > owner.size() && requestPath.starts_with(owner) &&
       requestPath[owner.size()] == '/');
  if (!visible)
    return emitOpError("memory instance is outside the request scope ancestry");
  auto endpointPath = (*this)->getAttrOfType<StringAttr>("ac.endpoint_path");
  auto endpointName = (*this)->getAttrOfType<StringAttr>("ac.name");
  if (!endpointPath || endpointPath.getValue().empty() || !endpointName ||
      endpointName.getValue().empty())
    return emitOpError("requires stable ac.endpoint_path and ac.name");
  std::string expectedEndpointPath = requestScope;
  if (expectedEndpointPath != "/")
    expectedEndpointPath.push_back('/');
  expectedEndpointPath.append(endpointName.getValue());
  if (endpointPath.getValue() != expectedEndpointPath)
    return emitOpError("ac.endpoint_path must match canonical scope/name path");

  Type payload = cast<QueueType>(getInput().getType()).getElementType();
  Operation *declaration = recordDecl(*this, payload);
  if (!declaration)
    return emitOpError("requires a record-like Queue payload");
  auto fieldIndex = findField(declaration, getResultField());
  if (!fieldIndex)
    return emitOpError() << "unknown result_field '" << getResultField() << "'";
  Type dataType = fieldType(declaration, *fieldIndex);
  if (dataType != instance.getDataType())
    return emitOpError(
        "result_field type must match memory instance data type");
  auto dataInteger = dyn_cast<IntegerType>(dataType);
  if (!dataInteger || dataInteger.getWidth() > 64)
    return emitOpError(
        "result_field must carry an integer no wider than 64 bits");

  Type argumentType = VarType::get(getContext(), payload);
  auto verifyPolicy = [&](Region &region, StringRef name) -> FailureOr<Type> {
    Block &block = region.front();
    if (block.getNumArguments() != 1 ||
        block.getArgument(0).getType() != argumentType) {
      emitOpError() << name << " argument must match queue payload Var";
      return failure();
    }
    for (Operation &operation : block.without_terminator())
      if (!isPureExpressionOperation(&operation)) {
        emitOpError() << name << " operation '" << operation.getName()
                      << "' must be pure";
        return failure();
      }
    auto yield = dyn_cast<MemoryYieldOp>(block.getTerminator());
    if (!yield) {
      emitOpError() << name << " must terminate with ac.memory.yield";
      return failure();
    }
    return cast<VarType>(yield.getValue().getType()).getElementType();
  };

  FailureOr<Type> address = verifyPolicy(getAddress(), "address");
  FailureOr<Type> write = verifyPolicy(getWrite(), "write");
  FailureOr<Type> data = verifyPolicy(getData(), "data");
  if (failed(address) || failed(write) || failed(data))
    return failure();
  auto addressInteger = dyn_cast<IntegerType>(*address);
  if (!addressInteger || addressInteger.getWidth() > 64)
    return emitOpError(
        "address must yield an integer Var no wider than 64 bits");
  if (addressInteger.getWidth() < 64 &&
      static_cast<uint64_t>(instance.getEntries()) >
          (uint64_t{1} << addressInteger.getWidth()))
    return emitOpError("entries must fit address width");
  if (!write->isInteger(1))
    return emitOpError("write must yield !ac.var<i1>");
  if (*data != dataType)
    return emitOpError("data must match result_field type");

  Operation *root = getOperation();
  while (root->getParentOp())
    root = root->getParentOp();
  DenseSet<int64_t> ordinals;
  StringSet<> endpointPaths;
  Type payloadType;
  uint64_t maximumOrdinal = 0;
  unsigned endpointCount = 0;
  WalkResult endpointResult = root->walk([&](MemoryRequestOp request) {
    auto resolved = dyn_cast_or_null<MemoryInstanceOp>(
        lookupGraphSymbol(request, request.getInstanceAttr()));
    if (resolved != instance)
      return WalkResult::advance();
    ++endpointCount;
    maximumOrdinal = std::max(maximumOrdinal, request.getOrdinal());
    if (!ordinals.insert(request.getOrdinal()).second) {
      request.emitOpError("duplicate endpoint ordinal for memory instance");
      return WalkResult::interrupt();
    }
    auto path = request->getAttrOfType<StringAttr>("ac.endpoint_path");
    if (!path || !endpointPaths.insert(path.getValue()).second) {
      request.emitOpError(
          "duplicate or missing endpoint path for memory instance");
      return WalkResult::interrupt();
    }
    Type candidate =
        cast<QueueType>(request.getInput().getType()).getElementType();
    if (!payloadType)
      payloadType = candidate;
    else if (payloadType != candidate) {
      request.emitOpError(
          "all endpoints of one memory must use one payload type");
      return WalkResult::interrupt();
    }
    return WalkResult::advance();
  });
  if (endpointResult.wasInterrupted())
    return failure();
  if (maximumOrdinal + 1 != endpointCount)
    return emitOpError("memory endpoint ordinals must be contiguous from zero");
  return success();
}

static bool isTableEntryType(Operation *anchor, Type type) {
  (void)anchor;
  if (auto integer = dyn_cast<IntegerType>(type))
    return integer.getWidth() > 0 && integer.getWidth() <= 64;
  return isa<RangeType, EnumType>(type) ||
         (isa<StructType>(type) && isImmutablePayloadType(type));
}

static FailureOr<uint64_t> tableEntryFieldCount(Operation *endpoint,
                                                TableOp table) {
  auto structure = dyn_cast<StructType>(table.getEntryType());
  if (!structure)
    return uint64_t{1};
  Operation *declaration = recordDecl(endpoint, structure);
  if (!declaration)
    return failure();
  ArrayAttr fields = declarationFields(declaration);
  if (!fields || fields.empty())
    return failure();
  return static_cast<uint64_t>(fields.size());
}

static LogicalResult verifyTableFields(Operation *endpoint, TableOp table,
                                       ArrayAttr fields, StringRef kind) {
  const std::string listName = (kind + "_fields").str();
  if (fields.empty())
    return endpoint->emitOpError() << listName << " must be non-empty";
  StringSet<> allowed;
  llvm::StringMap<unsigned> ordinals;
  if (auto structure = dyn_cast<StructType>(table.getEntryType())) {
    Operation *declaration = recordDecl(endpoint, structure);
    if (!declaration)
      return endpoint->emitOpError("table Entry struct declaration is missing");
    for (auto [ordinal, rawField] :
         llvm::enumerate(declarationFields(declaration))) {
      StringRef name = fieldName(cast<DictionaryAttr>(rawField));
      allowed.insert(name);
      ordinals[name] = ordinal;
    }
  } else {
    allowed.insert("$entry");
    ordinals["$entry"] = 0;
  }
  StringSet<> seen;
  std::optional<unsigned> previousOrdinal;
  for (Attribute rawField : fields) {
    auto field = dyn_cast<StringAttr>(rawField);
    if (!field || field.getValue().empty())
      return endpoint->emitOpError()
             << listName << " must contain non-empty field names";
    if (!seen.insert(field.getValue()).second)
      return endpoint->emitOpError()
             << "duplicate " << kind << " field '" << field.getValue() << "'";
    if (!allowed.contains(field.getValue()))
      return endpoint->emitOpError()
             << "unknown " << kind << " field '" << field.getValue() << "'";
    unsigned ordinal = ordinals.lookup(field.getValue());
    if (previousOrdinal && ordinal <= *previousOrdinal)
      return endpoint->emitOpError()
             << listName << " must follow Table Entry declaration order";
    previousOrdinal = ordinal;
  }
  return success();
}

static LogicalResult verifyTableWriteFields(Operation *endpoint, TableOp table,
                                            ArrayAttr writeFields) {
  return verifyTableFields(endpoint, table, writeFields, "write");
}

static bool tableWriteFieldsAreComplete(Operation *endpoint, TableOp table,
                                        ArrayAttr writeFields) {
  if (auto structure = dyn_cast<StructType>(table.getEntryType())) {
    Operation *declaration = recordDecl(endpoint, structure);
    if (!declaration)
      return false;
    ArrayAttr fields = declarationFields(declaration);
    if (writeFields.size() != fields.size())
      return false;
    for (auto [written, declared] : llvm::zip(writeFields, fields))
      if (cast<StringAttr>(written).getValue() !=
          fieldName(cast<DictionaryAttr>(declared)))
        return false;
    return true;
  }
  return writeFields.size() == 1 &&
         cast<StringAttr>(writeFields[0]).getValue() == "$entry";
}

static LogicalResult verifyTableWriteMode(Operation *endpoint, TableOp table,
                                          StringRef mode,
                                          ArrayAttr writeFields) {
  if (mode != "field" && mode != "replace")
    return endpoint->emitOpError("mode must be 'field' or 'replace'");
  if (mode == "replace" &&
      !tableWriteFieldsAreComplete(endpoint, table, writeFields))
    return endpoint->emitOpError(
        "replace mode must declare every Table Entry field");
  return success();
}

static TableOp resolveTable(Operation *operation, FlatSymbolRefAttr reference) {
  for (Operation *ancestor = operation->getParentOp(); ancestor;
       ancestor = ancestor->getParentOp()) {
    if (ancestor->getNumRegions() != 1 || !ancestor->getRegion(0).hasOneBlock())
      continue;
    for (TableOp table : ancestor->getRegion(0).front().getOps<TableOp>())
      if (table.getSymName() == reference.getValue())
        return table;
  }
  return {};
}

static bool tableVisibleFrom(Operation *operation, TableOp table) {
  std::string requestScope = queueScopePath(operation);
  StringRef owner = table.getOwner();
  StringRef requestPath(requestScope);
  return owner == "/" || requestPath == owner ||
         (requestPath.size() > owner.size() && requestPath.starts_with(owner) &&
          requestPath[owner.size()] == '/');
}

static unsigned canonicalTableIndexWidth(uint64_t extent);
static FailureOr<SmallVector<int64_t>> canonicalTableShape(TableOp table);
static FailureOr<uint64_t> flattenedTableEntries(ArrayRef<int64_t> shape);

static LogicalResult verifyCanonicalFlattenedTableIndex(Operation *operation,
                                                        TableOp table,
                                                        Value index) {
  if (auto flattened = index.getDefiningOp<TableIndexOp>()) {
    if (resolveTable(flattened, flattened.getTableAttr()) == table)
      return success();
    return operation->emitOpError(
        "flattened Table index belongs to another Table");
  }
  if (auto selection = index.getDefiningOp<TableChooseOp>()) {
    const int64_t count = selection.getCountAttr().getInt();
    if (count > 0 && selection.getResults().size() == 2 * count &&
        llvm::is_contained(selection.getResults().take_front(count), index) &&
        resolveTable(selection, selection.getTableAttr()) == table)
      return success();
    return operation->emitOpError("TableChoice index belongs to another Table");
  }
  return operation->emitOpError(
      "multidimensional access requires same-Table ac.table.index or "
      "ac.table.choose index provenance");
}

static LogicalResult verifyTableIndex(Operation *operation, TableOp table,
                                      Value index) {
  auto indexType = cast<VarType>(index.getType()).getElementType();
  auto range = dyn_cast<RangeType>(indexType);
  auto integer = dyn_cast<IntegerType>(indexType);
  if (!range &&
      (!integer || integer.getWidth() == 0 || integer.getWidth() > 64))
    return operation->emitOpError(
        "table index must be an integer Var no wider than 64 bits");
  if (range && range.getUpper() >= static_cast<uint64_t>(table.getEntries()))
    return operation->emitOpError(
        "bounded table index exceeds the Table domain");
  if (table.getShapeAttr()) {
    auto shape = canonicalTableShape(table);
    auto entries = succeeded(shape) ? flattenedTableEntries(*shape)
                                    : FailureOr<uint64_t>(failure());
    if (failed(entries))
      return operation->emitOpError("Table shape is malformed");
    if (shape->size() == 1 && range) {
      return success();
    }
    if (!integer || integer.getWidth() != canonicalTableIndexWidth(*entries))
      return operation->emitOpError(
          "table index must use the canonical flattened domain width");
    if (shape->size() > 1 &&
        failed(verifyCanonicalFlattenedTableIndex(operation, table, index)))
      return failure();
  }
  auto constant = index.getDefiningOp<VarConstantOp>();
  auto value =
      constant ? dyn_cast<IntegerAttr>(constant.getValueAttr()) : IntegerAttr();
  if (value && value.getValue().getZExtValue() >=
                   static_cast<uint64_t>(table.getEntries()))
    return operation->emitOpError("static table index is out of range");
  return success();
}

static LogicalResult verifyStaticallySafeRuleTableIndex(Operation *operation,
                                                        TableOp table,
                                                        Value index) {
  return verifyTableIndex(operation, table, index);
}

static unsigned canonicalTableIndexWidth(uint64_t extent) {
  return std::max<unsigned>(1, llvm::Log2_64_Ceil(extent));
}

static FailureOr<SmallVector<int64_t>> canonicalTableShape(TableOp table) {
  if (!table.getShapeAttr())
    return SmallVector<int64_t>{static_cast<int64_t>(table.getEntries())};
  ArrayRef<int64_t> rawShape = *table.getShape();
  SmallVector<int64_t> shape(rawShape.begin(), rawShape.end());
  if (shape.empty() ||
      llvm::any_of(shape, [](int64_t extent) { return extent <= 0; }))
    return failure();
  return shape;
}

static FailureOr<uint64_t> flattenedTableEntries(ArrayRef<int64_t> shape) {
  uint64_t entries = 1;
  for (int64_t extent : shape) {
    if (extent <= 0 ||
        entries > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) /
                      static_cast<uint64_t>(extent))
      return failure();
    entries *= static_cast<uint64_t>(extent);
  }
  return entries;
}

static SmallVector<int64_t> canonicalTableStrides(ArrayRef<int64_t> shape) {
  SmallVector<int64_t> strides(shape.size(), 1);
  for (size_t axis = shape.size(); axis > 1; --axis)
    strides[axis - 2] = strides[axis - 1] * shape[axis - 1];
  return strides;
}

static LogicalResult verifyTableInitValue(Operation *anchor, Type type,
                                          Attribute value) {
  if (auto integer = dyn_cast<IntegerType>(type)) {
    auto typed = dyn_cast<IntegerAttr>(value);
    return success(typed && typed.getType() == integer);
  }
  if (auto enumeration = dyn_cast<EnumType>(type)) {
    auto enumerant = dyn_cast<StringAttr>(value);
    auto declaration =
        dyn_cast_or_null<EnumOp>(lookup(anchor, enumeration.getName()));
    return success(
        enumerant && declaration &&
        llvm::any_of(declaration.getEnumerants(), [&](Attribute raw) {
          return cast<StringAttr>(raw).getValue() == enumerant.getValue();
        }));
  }
  if (auto structure = dyn_cast<StructType>(type)) {
    auto record = dyn_cast<DictionaryAttr>(value);
    Operation *declaration = recordDecl(anchor, structure);
    ArrayAttr fields =
        declaration ? declarationFields(declaration) : ArrayAttr();
    if (!record || !fields || record.size() != fields.size())
      return failure();
    for (Attribute rawField : fields) {
      auto field = cast<DictionaryAttr>(rawField);
      Attribute member = record.get(fieldName(field));
      if (!member ||
          failed(verifyTableInitValue(anchor, fieldType(field), member)))
        return failure();
    }
    return success();
  }
  if (auto tuple = dyn_cast<TupleType>(type)) {
    auto values = dyn_cast<ArrayAttr>(value);
    if (!values || values.size() != tuple.size())
      return failure();
    for (auto [elementType, element] :
         llvm::zip_equal(tuple.getTypes(), values))
      if (failed(verifyTableInitValue(anchor, elementType, element)))
        return failure();
    return success();
  }
  if (auto array = dyn_cast<ValueArrayType>(type)) {
    auto values = dyn_cast<ArrayAttr>(value);
    if (!values || static_cast<int64_t>(values.size()) != array.getLength())
      return failure();
    for (Attribute element : values)
      if (failed(verifyTableInitValue(anchor, array.getElementType(), element)))
        return failure();
    return success();
  }
  return failure();
}

LogicalResult TableOp::verify() {
  if (!isTableEntryType(*this, getEntryType()))
    return emitOpError(
        "entry type must be a <=64-bit integer, nominal enum, or immutable "
        "recursive struct");
  if (getEntries() <= 0)
    return emitOpError("entries must be positive");
  const bool hasTypedSchema = getShapeAttr() || getAxisWidthsAttr() ||
                              getLayoutAttr() || getLayoutVersionAttr() ||
                              getInitVersionAttr() || getInitImageAttr();
  if (hasTypedSchema && (!getShapeAttr() || !getAxisWidthsAttr() ||
                         !getLayoutAttr() || !getLayoutVersionAttr()))
    return emitOpError(
        "typed Table schema requires shape, axis_widths, layout, "
        "and layout_version");
  auto shape = canonicalTableShape(*this);
  if (failed(shape))
    return emitOpError("shape must be a non-empty tuple of positive extents");
  auto flattenedEntries = flattenedTableEntries(*shape);
  if (failed(flattenedEntries))
    return emitOpError("shape product overflows the canonical Table domain");
  if (*flattenedEntries != static_cast<uint64_t>(getEntries()))
    return emitOpError("entries must equal the flattened shape product");
  if (hasTypedSchema) {
    if (getLayout() != "row_major" || getLayoutVersion() != 1)
      return emitOpError("Table layout must be row_major version 1");
    ArrayRef<int64_t> axisWidths = *getAxisWidths();
    if (axisWidths.size() != shape->size())
      return emitOpError("axis_widths rank must match shape rank");
    for (auto [extent, width] : llvm::zip_equal(*shape, axisWidths))
      if (width != canonicalTableIndexWidth(extent))
        return emitOpError(
            "axis_widths must be the canonical unsigned widths for shape");
  }
  const uint64_t scalarInit = static_cast<uint64_t>(getInit());
  if (scalarInit != 0) {
    if (getInitImageAttr())
      return emitOpError(
          "scalar init and typed init_image are mutually exclusive");
    if (auto integer = dyn_cast<IntegerType>(getEntryType())) {
      if (integer.getWidth() < 64 &&
          scalarInit >= (uint64_t{1} << integer.getWidth()))
        return emitOpError("scalar init does not fit the Table entry type");
    } else if (auto range = dyn_cast<RangeType>(getEntryType())) {
      if (scalarInit < range.getLower() || scalarInit > range.getUpper())
        return emitOpError("scalar init lies outside the Table entry range");
    } else {
      return emitOpError(
          "non-zero scalar init requires an integer or range Table entry");
    }
  }
  if (!getInitImageAttr() && scalarInit == 0) {
    llvm::SmallPtrSet<Operation *, 8> seen;
    if (!supportsZeroImage(*this, getEntryType(), seen))
      return emitOpError(
          "zero-initialized Table entry type does not admit a zero image");
  }
  if (getInitVersionAttr() && !getInitImageAttr())
    return emitOpError("init_version requires a typed init_image");
  if (getInitImageAttr()) {
    if (!getInitVersionAttr() || getInitVersion() != 1)
      return emitOpError("typed init_image requires init_version 1");
    ArrayAttr initImage = *getInitImage();
    if (initImage.size() != *flattenedEntries)
      return emitOpError(
          "typed init_image count must equal the flattened entry count");
    for (Attribute value : initImage)
      if (failed(verifyTableInitValue(*this, getEntryType(), value)))
        return emitOpError(
            "typed init_image element does not match the Table Entry type");
  }
  if (getOwner().empty() || !getOwner().starts_with('/') ||
      (getOwner().size() > 1 && getOwner().ends_with('/')))
    return emitOpError("owner must be a canonical absolute scope path");
  std::string expectedStableId = "table/";
  if (getOwner() != "/") {
    expectedStableId.append(getOwner().drop_front());
    expectedStableId.push_back('/');
  }
  expectedStableId.append(getSymName());
  if (getStableId() != expectedStableId)
    return emitOpError("stable_id must match canonical owner/symbol identity");

  Operation *root = getOperation();
  while (root->getParentOp())
    root = root->getParentOp();
  bool ownerExists = getOwner() == "/";
  bool duplicateStableId = false;
  bool duplicateSelectionStableId = false;
  llvm::StringSet<> selectionStableIds;
  ModuleOp owningModule = (*this)->getParentOfType<ModuleOp>();
  unsigned endpoints = 0;
  llvm::StringMap<Operation *> fieldWriters;
  std::string overlappingField;
  unsigned replaceWriters = 0;
  unsigned proposalReplaceWriters = 0;
  root->walk([&](Operation *operation) {
    if (auto scope = dyn_cast<ScopeOp>(operation)) {
      std::string path = queueScopePath(scope);
      if (path != "/")
        path.push_back('/');
      path.append(scope.getSymName());
      ownerExists |= path == getOwner();
    }
    if (auto other = dyn_cast<TableOp>(operation))
      duplicateStableId |= other != *this &&
                           other->getParentOfType<ModuleOp>() == owningModule &&
                           other.getStableId() == getStableId();
    if (auto read = dyn_cast<TableReadOp>(operation)) {
      if (resolveTable(read, read.getTableAttr()) == *this)
        ++endpoints;
    }
    if (auto read = dyn_cast<TableGetOp>(operation)) {
      if (resolveTable(read, read.getTableAttr()) == *this)
        ++endpoints;
    }
    if (auto match = dyn_cast<TableMatchOp>(operation)) {
      if (resolveTable(match, match.getTableAttr()) == *this)
        ++endpoints;
    }
    if (auto choose = dyn_cast<TableChooseOp>(operation)) {
      if (choose->getParentOfType<ModuleOp>() == owningModule &&
          !choose.getStableId().empty() &&
          !selectionStableIds.insert(choose.getStableId()).second)
        duplicateSelectionStableId = true;
      if (resolveTable(choose, choose.getTableAttr()) == *this)
        ++endpoints;
    }
    if (auto write = dyn_cast<TableWriteOp>(operation)) {
      if (resolveTable(write, write.getTableAttr()) == *this) {
        ++endpoints;
        if (write.getMode() == "replace") {
          ++replaceWriters;
          return;
        }
        StringSet<> localFields;
        for (Attribute rawField : write.getWriteFields()) {
          auto field = cast<StringAttr>(rawField).getValue();
          if (localFields.insert(field).second &&
              !fieldWriters.try_emplace(field, operation).second)
            overlappingField = field.str();
        }
      }
    }
    if (auto write = dyn_cast<TableMaskedWriteOp>(operation)) {
      if (resolveTable(write, write.getTableAttr()) == *this) {
        ++endpoints;
        StringSet<> localFields;
        for (Attribute rawField : write.getWriteFields()) {
          auto field = cast<StringAttr>(rawField).getValue();
          if (localFields.insert(field).second &&
              !fieldWriters.try_emplace(field, operation).second)
            overlappingField = field.str();
        }
      }
    }
    if (auto proposal = dyn_cast<TableProposeOp>(operation)) {
      if (resolveTable(proposal, proposal.getTableAttr()) == *this) {
        ++endpoints;
        if (proposal.getMode() == "replace") {
          ++proposalReplaceWriters;
          return;
        }
        // Firing-local proposals are checked as normalized whole-model
        // footprints by ac-verify-value-constraints.  A declaration-local
        // source-order check cannot prove dynamic disjointness or arbitration.
      }
    }
    if (auto match = dyn_cast<TableMatchOp>(operation))
      if (resolveTable(match, match.getTableAttr()) == *this)
        ++endpoints;
  });
  if (!ownerExists)
    return emitOpError("owner does not name a declared scope path");
  if (duplicateStableId)
    return emitOpError("stable_id must be unique");
  if (duplicateSelectionStableId)
    return emitOpError(
        "Table selection stable_id must be unique within the module");
  if (endpoints == 0)
    return emitOpError("must have at least one table read/write endpoint");
  // Cross-endpoint overlap is a whole-model proof obligation.  Declaration
  // traversal cannot prove dynamic index/guard disjointness and must not
  // manufacture source-order writer priority.
  return success();
}

LogicalResult TableIndexOp::verify() {
  TableOp table = resolveTable(*this, getTableAttr());
  if (!table)
    return emitOpError() << "unresolved table " << getTable();
  if (!tableVisibleFrom(*this, table))
    return emitOpError("table is outside the index scope ancestry");
  auto shape = canonicalTableShape(table);
  if (failed(shape))
    return emitOpError("referenced Table has invalid shape metadata");
  if (getCoordinates().size() != shape->size())
    return emitOpError("coordinate rank must match the Table shape rank");
  for (auto [coordinate, extent] : llvm::zip_equal(getCoordinates(), *shape)) {
    Type element = cast<VarType>(coordinate.getType()).getElementType();
    auto type = dyn_cast<IntegerType>(element);
    auto range = dyn_cast<RangeType>(element);
    if ((!range && (!type || !type.isSignless() ||
                    type.getWidth() != canonicalTableIndexWidth(extent))) ||
        (range && range.getUpper() >= static_cast<uint64_t>(extent)))
      return emitOpError(
          "coordinate type must use the canonical unsigned axis width or a "
          "contained bounded range");
    auto constant = coordinate.getDefiningOp<VarConstantOp>();
    auto value = constant ? dyn_cast<IntegerAttr>(constant.getValueAttr())
                          : IntegerAttr();
    if (value &&
        value.getValue().getZExtValue() >= static_cast<uint64_t>(extent))
      return emitOpError("static Table coordinate is out of range");
  }
  auto flattenedEntries = flattenedTableEntries(*shape);
  if (failed(flattenedEntries))
    return emitOpError("Table shape product overflows during flattening");
  Type expected = VarType::get(
      getContext(), IntegerType::get(getContext(), canonicalTableIndexWidth(
                                                       *flattenedEntries)));
  if (getIndex().getType() != expected)
    return emitOpError(
        "flattened index must use the canonical Table-domain width");
  return success();
}

LogicalResult TableGetOp::verify() {
  TableOp table = resolveTable(*this, getTableAttr());
  if (!table)
    return emitOpError() << "unresolved table " << getTable();
  if (!tableVisibleFrom(*this, table))
    return emitOpError("table is outside the access scope ancestry");
  if (getResult().getType() != VarType::get(getContext(), table.getEntryType()))
    return emitOpError("result must match table entry Var type");
  return verifyTableIndex(*this, table, getIndex());
}

static LogicalResult verifyTableWriterArbitration(Operation *operation) {
  for (NamedAttribute attribute : operation->getAttrs()) {
    StringRef name = attribute.getName().getValue();
    if (name.starts_with("ac.writer_"))
      return operation->emitOpError()
             << "has unknown writer proof '" << name << "'";
    if (name == "safe" || name == "safety" || name == "ac.safe")
      return operation->emitOpError(
          "user safety assertions cannot bypass writer proof");
  }
  if (Attribute arbitration = operation->getAttr("ac.arbitration")) {
    if (!isa<WriterPriorityAttr>(arbitration))
      return operation->emitOpError(
          "ac.arbitration requires typed #ac.writer_priority<rank>");
    if (!isa<TableProposeOp>(operation)) {
      auto endpoint = operation->getAttrOfType<StringAttr>("ac.endpoint_id");
      if (!endpoint || endpoint.getValue().empty())
        return operation->emitOpError(
            "arbitrated writer requires stable ac.endpoint_id");
    }
  }
  return success();
}

LogicalResult TableProposeOp::verify() {
  Operation *parent = (*this)->getParentOp();
  if (!isa_and_nonnull<RuleOp, FiringOp>(parent))
    return emitOpError("must be nested directly in ac.rule or ac.firing");
  TableOp table = resolveTable(*this, getTableAttr());
  if (!table)
    return emitOpError() << "unresolved table " << getTable();
  if (!tableVisibleFrom(*this, table))
    return emitOpError("table is outside the proposal scope ancestry");
  if (failed(verifyTableWriteFields(*this, table, getWriteFields())) ||
      failed(verifyTableWriteMode(*this, table, getMode(), getWriteFields())))
    return failure();
  if (getValue().getType() != VarType::get(getContext(), table.getEntryType()))
    return emitOpError("proposal value must match the Table Entry Var type");
  if (getWhen() && failed(verifyI1VarCondition(*this, getWhen())))
    return failure();
  if (failed(verifyStaticallySafeRuleTableIndex(*this, table, getIndex())))
    return failure();
  return verifyTableWriterArbitration(*this);
}

static FailureOr<Type> verifyTablePolicy(Operation *endpoint, Region &region,
                                         StringRef name, Type argumentType,
                                         bool allowGet) {
  if (!region.hasOneBlock()) {
    endpoint->emitOpError() << name << " must contain exactly one block";
    return failure();
  }
  Block &block = region.front();
  const unsigned expectedArguments = argumentType ? 1 : 0;
  if (block.getNumArguments() != expectedArguments ||
      (argumentType && block.getArgument(0).getType() != argumentType)) {
    endpoint->emitOpError() << name << " argument must match endpoint input";
    return failure();
  }
  FlatSymbolRefAttr endpointTable;
  if (auto read = dyn_cast<TableReadOp>(endpoint))
    endpointTable = read.getTableAttr();
  else if (auto write = dyn_cast<TableWriteOp>(endpoint))
    endpointTable = write.getTableAttr();
  else if (auto write = dyn_cast<TableMaskedWriteOp>(endpoint))
    endpointTable = write.getTableAttr();
  auto verifyCapture = [&](Value operand) -> LogicalResult {
    if (auto argument = dyn_cast<BlockArgument>(operand)) {
      if (argument.getOwner() == &block)
        return success();
      return endpoint->emitOpError()
             << name << " captures an external block argument";
    }
    Operation *definition = operand.getDefiningOp();
    if (!definition || definition->getBlock() == &block)
      return success();
    if (auto match = dyn_cast<TableMatchOp>(definition)) {
      if (match.getTableAttr() == endpointTable)
        return success();
    } else if (auto choose = dyn_cast<TableChooseOp>(definition)) {
      if (choose.getTableAttr() == endpointTable)
        return success();
    }
    return endpoint->emitOpError()
           << name
           << " may only capture shared match/choose results for its Table";
  };
  for (Operation &operation : block)
    for (Value operand : operation.getOperands())
      if (failed(verifyCapture(operand)))
        return failure();
  for (Operation &operation : block.without_terminator()) {
    if (isa<SlotGetOp>(operation))
      continue;
    if (auto match = dyn_cast<TableMatchOp>(operation)) {
      FlatSymbolRefAttr endpointTable;
      if (auto read = dyn_cast<TableReadOp>(endpoint))
        endpointTable = read.getTableAttr();
      else if (auto write = dyn_cast<TableWriteOp>(endpoint))
        endpointTable = write.getTableAttr();
      else if (auto write = dyn_cast<TableMaskedWriteOp>(endpoint))
        endpointTable = write.getTableAttr();
      if (match.getTableAttr() != endpointTable) {
        endpoint->emitOpError() << name << " match belongs to another Table";
        return failure();
      }
      continue;
    }
    if (auto choose = dyn_cast<TableChooseOp>(operation)) {
      FlatSymbolRefAttr endpointTable;
      if (auto read = dyn_cast<TableReadOp>(endpoint))
        endpointTable = read.getTableAttr();
      else if (auto write = dyn_cast<TableWriteOp>(endpoint))
        endpointTable = write.getTableAttr();
      else if (auto write = dyn_cast<TableMaskedWriteOp>(endpoint))
        endpointTable = write.getTableAttr();
      if (choose.getTableAttr() != endpointTable) {
        endpoint->emitOpError()
            << name << " selection belongs to another Table";
        return failure();
      }
      continue;
    }
    if (allowGet)
      if (auto get = dyn_cast<TableGetOp>(operation)) {
        FlatSymbolRefAttr endpointTable;
        if (auto read = dyn_cast<TableReadOp>(endpoint))
          endpointTable = read.getTableAttr();
        else if (auto write = dyn_cast<TableWriteOp>(endpoint))
          endpointTable = write.getTableAttr();
        else if (auto write = dyn_cast<TableMaskedWriteOp>(endpoint))
          endpointTable = write.getTableAttr();
        if (get.getTableAttr() != endpointTable) {
          endpoint->emitOpError()
              << name << " may only observe its endpoint table";
          return failure();
        }
        continue;
      }
    if (!isPureExpressionOperation(&operation)) {
      endpoint->emitOpError() << name << " operation '" << operation.getName()
                              << "' is not permitted";
      return failure();
    }
  }
  auto yield = dyn_cast<TableYieldOp>(block.getTerminator());
  if (!yield) {
    endpoint->emitOpError() << name << " must terminate with ac.table.yield";
    return failure();
  }
  return cast<VarType>(yield.getValue().getType()).getElementType();
}

LogicalResult TableReadOp::verify() {
  TableOp table = resolveTable(*this, getTableAttr());
  if (!table)
    return emitOpError() << "unresolved table " << getTable();
  if (!tableVisibleFrom(*this, table))
    return emitOpError("table is outside the read scope ancestry");
  if (getDepth() <= 0 || getLatency() <= 0)
    return emitOpError("depth and latency must be positive");
  if (cast<QueueType>(getOutput().getType()).getElementType() !=
      table.getEntryType())
    return emitOpError("output Queue payload must match table entry type");
  Type argumentType;
  if (getInput())
    argumentType = VarType::get(
        getContext(), cast<QueueType>(getInput().getType()).getElementType());
  auto address =
      verifyTablePolicy(*this, getAddress(), "address", argumentType, false);
  auto when = verifyTablePolicy(*this, getWhen(), "when", argumentType, true);
  if (failed(address) || failed(when))
    return failure();
  auto index = dyn_cast<IntegerType>(*address);
  if (!index || index.getWidth() == 0 || index.getWidth() > 64)
    return emitOpError("address must yield an integer Var");
  if (failed(verifyTableIndex(
          *this, table,
          cast<TableYieldOp>(getAddress().front().getTerminator()).getValue())))
    return failure();
  if (!when->isInteger(1))
    return emitOpError("when must yield !ac.var<i1>");
  return success();
}

LogicalResult TableWriteOp::verify() {
  TableOp table = resolveTable(*this, getTableAttr());
  if (!table)
    return emitOpError() << "unresolved table " << getTable();
  if (!tableVisibleFrom(*this, table))
    return emitOpError("table is outside the write scope ancestry");
  if (failed(verifyTableWriteFields(*this, table, getWriteFields())))
    return failure();
  if (failed(verifyTableWriteMode(*this, table, getMode(), getWriteFields())))
    return failure();
  Type argumentType;
  if (getInput())
    argumentType = VarType::get(
        getContext(), cast<QueueType>(getInput().getType()).getElementType());
  auto address =
      verifyTablePolicy(*this, getAddress(), "address", argumentType, false);
  auto enable =
      verifyTablePolicy(*this, getEnable(), "enable", argumentType, false);
  auto value =
      verifyTablePolicy(*this, getValue(), "value", argumentType, true);
  if (failed(address) || failed(enable) || failed(value))
    return failure();
  auto index = dyn_cast<IntegerType>(*address);
  if (!index || index.getWidth() == 0 || index.getWidth() > 64)
    return emitOpError("address must yield an integer Var");
  if (failed(verifyTableIndex(
          *this, table,
          cast<TableYieldOp>(getAddress().front().getTerminator()).getValue())))
    return failure();
  if (!enable->isInteger(1))
    return emitOpError("enable must yield !ac.var<i1>");
  if (*value != table.getEntryType())
    return emitOpError("value must yield the table entry type");
  return verifyTableWriterArbitration(*this);
}

static FailureOr<uint64_t> tableMatchDomainEntries(TableMatchOp match,
                                                   TableOp table) {
  const bool hasProjection =
      match.getDomainAxesAttr() || match.getDomainShapeAttr() ||
      match.getDomainStridesAttr() || match.getDomainOffsetAttr() ||
      match.getDomainBase();
  if (!hasProjection) {
    if (auto shape = table.getShape(); shape && shape->size() > 1) {
      match.emitOpError(
          "multidimensional match requires an explicit projected mask domain");
      return failure();
    } else {
      return static_cast<uint64_t>(table.getEntries());
    }
  }
  if (!match.getDomainAxesAttr() || !match.getDomainShapeAttr() ||
      !match.getDomainStridesAttr() || !match.getDomainOffsetAttr()) {
    match.emitOpError(
        "projected mask domain requires axes, shape, strides, and offset");
    return failure();
  }
  auto tableShape = canonicalTableShape(table);
  if (failed(tableShape)) {
    match.emitOpError("referenced Table has invalid shape metadata");
    return failure();
  }
  ArrayRef<int64_t> axes = *match.getDomainAxes();
  ArrayRef<int64_t> shape = *match.getDomainShape();
  ArrayRef<int64_t> strides = *match.getDomainStrides();
  if (axes.size() != shape.size() || axes.size() != strides.size()) {
    match.emitOpError("projected mask domain arrays must have one shared rank");
    return failure();
  }
  SmallVector<int64_t> tableStrides = canonicalTableStrides(*tableShape);
  std::optional<int64_t> previousAxis;
  uint64_t domainEntries = 1;
  const int64_t signedDomainOffset = match.getDomainOffsetAttr().getInt();
  if (signedDomainOffset < 0) {
    match.emitOpError("projected mask domain offset is out of range");
    return failure();
  }
  const uint64_t domainOffset = static_cast<uint64_t>(signedDomainOffset);
  if (match.getDomainBase() && domainOffset != 0) {
    match.emitOpError(
        "dynamic projected mask domain requires zero static offset");
    return failure();
  }
  uint64_t maximumIndex = domainOffset;
  if (domainOffset >= static_cast<uint64_t>(table.getEntries())) {
    match.emitOpError("projected mask domain offset is out of range");
    return failure();
  }
  for (auto [axis, extent, stride] : llvm::zip_equal(axes, shape, strides)) {
    if (axis < 0 || static_cast<size_t>(axis) >= tableShape->size() ||
        (previousAxis && axis <= *previousAxis)) {
      match.emitOpError(
          "projected mask domain axes must be unique and increasing");
      return failure();
    }
    previousAxis = axis;
    if (extent != (*tableShape)[axis] || stride != tableStrides[axis]) {
      match.emitOpError(
          "projected mask domain must use canonical Table extents and strides");
      return failure();
    }
    if ((domainOffset / static_cast<uint64_t>(stride)) %
            static_cast<uint64_t>(extent) !=
        0) {
      match.emitOpError(
          "projected mask domain offset must fix only omitted axes");
      return failure();
    }
    const uint64_t unsignedExtent = static_cast<uint64_t>(extent);
    const uint64_t unsignedStride = static_cast<uint64_t>(stride);
    if (unsignedExtent - 1 >
            std::numeric_limits<uint64_t>::max() / unsignedStride ||
        domainEntries > std::numeric_limits<uint64_t>::max() / unsignedExtent) {
      match.emitOpError("projected mask domain arithmetic overflows");
      return failure();
    }
    const uint64_t contribution = (unsignedExtent - 1) * unsignedStride;
    if (maximumIndex > std::numeric_limits<uint64_t>::max() - contribution) {
      match.emitOpError("projected mask domain arithmetic overflows");
      return failure();
    }
    maximumIndex += contribution;
    domainEntries *= unsignedExtent;
  }
  if (maximumIndex >= static_cast<uint64_t>(table.getEntries())) {
    match.emitOpError("projected mask domain exceeds the Table shape");
    return failure();
  }
  if (Value base = match.getDomainBase()) {
    auto flattened = base.getDefiningOp<TableIndexOp>();
    if (!flattened ||
        resolveTable(flattened, flattened.getTableAttr()) != table) {
      match.emitOpError("dynamic projected mask base must come from same-Table "
                        "ac.table.index");
      return failure();
    }
    auto tableShape = canonicalTableShape(table);
    if (failed(tableShape) || tableShape->size() != 2 || axes.size() != 1 ||
        axes.front() != 1 || flattened.getCoordinates().size() != 2) {
      match.emitOpError(
          "dynamic projected mask currently requires one rank-two Table row");
      return failure();
    }
    auto zero =
        flattened.getCoordinates().back().getDefiningOp<VarConstantOp>();
    auto zeroValue =
        zero ? dyn_cast<IntegerAttr>(zero.getValueAttr()) : IntegerAttr();
    if (!zeroValue || !zeroValue.getValue().isZero()) {
      match.emitOpError(
          "dynamic projected mask base must fix the suffix coordinate to zero");
      return failure();
    }
    if (base.getType() !=
        VarType::get(
            match.getContext(),
            IntegerType::get(match.getContext(),
                             canonicalTableIndexWidth(table.getEntries())))) {
      match.emitOpError("dynamic projected mask base must use the canonical "
                        "Table index type");
      return failure();
    }
  }
  return domainEntries;
}

LogicalResult TableMaskedWriteOp::verify() {
  TableOp table = resolveTable(*this, getTableAttr());
  if (!table)
    return emitOpError() << "unresolved table " << getTable();
  if (!tableVisibleFrom(*this, table))
    return emitOpError("table is outside the write scope ancestry");
  if (failed(verifyTableWriteFields(*this, table, getWriteFields())))
    return failure();
  if (getMode() != "field")
    return emitOpError("masked write mode must be 'field'");
  auto match = getMask().getDefiningOp<TableMatchOp>();
  if (!match || resolveTable(match, match.getTableAttr()) != table)
    return emitOpError("mask must be produced by match on the same Table");
  auto domainEntries = tableMatchDomainEntries(match, table);
  if (failed(domainEntries) || *domainEntries == 0 || *domainEntries > 64)
    return emitOpError("masked write domain must contain 1..64 entries");
  auto maskType = dyn_cast<IntegerType>(
      cast<VarType>(getMask().getType()).getElementType());
  if (!maskType || maskType.getWidth() != *domainEntries)
    return emitOpError(match.getDomainAxesAttr()
                           ? "mask width must equal the projected Table domain"
                           : "mask width must equal the Table entry count");
  auto enable = verifyTablePolicy(*this, getEnable(), "enable", Type(), false);
  Type entryArgument = VarType::get(getContext(), table.getEntryType());
  auto value =
      verifyTablePolicy(*this, getValue(), "value", entryArgument, true);
  if (failed(enable) || failed(value))
    return failure();
  if (!enable->isInteger(1))
    return emitOpError("enable must yield !ac.var<i1>");
  if (*value != table.getEntryType())
    return emitOpError("value must yield the table entry type");
  return verifyTableWriterArbitration(*this);
}

LogicalResult TableMatchOp::verify() {
  TableOp table = resolveTable(*this, getTableAttr());
  if (!table)
    return emitOpError() << "unresolved table " << getTable();
  if (!tableVisibleFrom(*this, table))
    return emitOpError("table is outside the match scope ancestry");
  auto domainEntries = tableMatchDomainEntries(*this, table);
  if (failed(domainEntries))
    return failure();
  if (!isCandidateMaskType(getMask().getType(), *domainEntries))
    return emitOpError(
        getDomainAxesAttr()
            ? "mask must exactly cover the projected Table domain in 64-bit "
              "words"
            : "mask must exactly cover the Table domain in 64-bit words");
  if (!getPredicate().hasOneBlock())
    return emitOpError("predicate must contain exactly one block");
  Block &block = getPredicate().front();
  Type entry = VarType::get(getContext(), table.getEntryType());
  if (block.getNumArguments() != 1 || block.getArgument(0).getType() != entry)
    return emitOpError("predicate argument must match the Table Entry");
  for (Operation &operation : block.without_terminator())
    if (!isa<SlotGetOp, TableGetOp>(operation) &&
        !isPureExpressionOperation(&operation))
      return emitOpError() << "predicate operation '" << operation.getName()
                           << "' is not permitted";
  auto yield = dyn_cast<TableMatchYieldOp>(block.getTerminator());
  if (!yield ||
      !cast<VarType>(yield.getValue().getType()).getElementType().isInteger(1))
    return emitOpError("predicate must yield !ac.var<i1>");
  return success();
}

LogicalResult TableChooseOp::verify() {
  TableOp table = resolveTable(*this, getTableAttr());
  if (!table)
    return emitOpError() << "unresolved table " << getTable();
  if (!tableVisibleFrom(*this, table))
    return emitOpError("table is outside the choose scope ancestry");
  auto match = getMask().getDefiningOp<TableMatchOp>();
  if (!match)
    return emitOpError("candidate mask must be produced directly by "
                       "ac.table.match");
  if (resolveTable(match, match.getTableAttr()) != table)
    return emitOpError("candidate mask must come from the same Table");
  auto domainEntries = tableMatchDomainEntries(match, table);
  if (failed(domainEntries) ||
      !isCandidateMaskType(getMask().getType(), *domainEntries))
    return emitOpError(match.getDomainAxesAttr()
                           ? "candidate mask must exactly cover the projected "
                             "Table domain in 64-bit words"
                           : "candidate mask must exactly cover the Table "
                             "domain in 64-bit words");
  const int64_t count = getCountAttr().getInt();
  if (count <= 0 || static_cast<uint64_t>(count) > *domainEntries)
    return emitOpError(
        "count must be positive and not exceed the projected Table domain");
  if (match.getDomainBase() &&
      (count != 1 || getPolicy() != TableSelectionPolicy::First))
    return emitOpError(
        "dynamic row projection currently requires first/count=1");
  if (getResults().size() != static_cast<size_t>(2 * count))
    return emitOpError(
        "result count must be exactly 2*count with indices before valids");
  if (getStableId().empty())
    return emitOpError("stable_id must be non-empty");
  bool duplicateStableId = false;
  if (ModuleOp module = (*this)->getParentOfType<ModuleOp>())
    module.walk([&](TableChooseOp other) {
      duplicateStableId |=
          other != *this && other.getStableId() == getStableId();
    });
  if (duplicateStableId)
    return emitOpError("stable_id must be unique within the module");
  unsigned indexWidth = canonicalTableIndexWidth(table.getEntries());
  Type expectedIndex =
      VarType::get(getContext(), IntegerType::get(getContext(), indexWidth));
  Type expectedValid =
      VarType::get(getContext(), IntegerType::get(getContext(), 1));
  for (Value index : getResults().take_front(count))
    if (index.getType() != expectedIndex)
      return emitOpError(
          "index result segment must use the complete Table-domain width");
  for (Value valid : getResults().drop_front(count))
    if (valid.getType() != expectedValid)
      return emitOpError("valid result segment must contain !ac.var<i1>");

  const TableSelectionPolicy policy = getPolicy();
  const int64_t initialCursor = getInitialCursorAttr().getInt();
  if (policy == TableSelectionPolicy::First ||
      policy == TableSelectionPolicy::RoundRobin) {
    if (!getKey().empty() &&
        !(getKey().hasOneBlock() && getKey().front().empty()))
      return emitOpError(
          "first/round_robin policy does not accept a key region");
    if (getKeyOrderingAttr())
      return emitOpError(
          "first/round_robin policy does not accept key ordering");
    if (policy == TableSelectionPolicy::First && initialCursor != 0)
      return emitOpError("first policy requires initial_cursor=0");
    if (policy == TableSelectionPolicy::RoundRobin &&
        (initialCursor < 0 ||
         static_cast<uint64_t>(initialCursor) >= *domainEntries))
      return emitOpError(
          "round_robin initial_cursor must be within the projected domain");
    return success();
  }
  if (initialCursor != 0)
    return emitOpError("min/max policy requires initial_cursor=0");
  if (!getKeyOrderingAttr())
    return emitOpError("min/max policy requires typed key ordering");
  if (!getKey().hasOneBlock())
    return emitOpError("min/max policy requires one key region");
  Block &block = getKey().front();
  Type entry = VarType::get(getContext(), table.getEntryType());
  if (block.getNumArguments() != 1 || block.getArgument(0).getType() != entry)
    return emitOpError("key argument must match the Table Entry");
  for (Operation &operation : block.without_terminator()) {
    if (isa<TableGetOp>(operation)) {
      if (!isa_and_nonnull<RuleOp, FiringOp>((*this)->getParentOp()))
        return emitOpError(
            "key Table reads require transactional rule/firing ownership");
      continue;
    }
    if (!isa<SlotGetOp>(operation) && !isPureExpressionOperation(&operation))
      return emitOpError() << "key operation '" << operation.getName()
                           << "' is not permitted";
  }
  auto yield = dyn_cast<TableChooseYieldOp>(block.getTerminator());
  auto key =
      yield ? dyn_cast<IntegerType>(
                  cast<VarType>(yield.getValue().getType()).getElementType())
            : IntegerType();
  if (!key || key.getWidth() == 0 || key.getWidth() > 64)
    return emitOpError("min/max key must yield a fixed-width integer");
  return success();
}

static SlotOp resolveSlot(Operation *operation, FlatSymbolRefAttr reference) {
  return dyn_cast_or_null<SlotOp>(
      SymbolTable::lookupNearestSymbolFrom(operation, reference));
}

static bool slotVisibleFrom(Operation *operation, SlotOp slot) {
  std::string requestScope = queueScopePath(operation);
  StringRef owner = slot.getOwner();
  StringRef requestPath(requestScope);
  return owner == "/" || requestPath == owner ||
         (requestPath.size() > owner.size() && requestPath.starts_with(owner) &&
          requestPath[owner.size()] == '/');
}

LogicalResult SlotOp::verify() {
  Type payload = cast<QueueType>(getInput().getType()).getElementType();
  if (!isTableEntryType(*this, payload))
    return emitOpError("input Queue payload must be bool, a <=64-bit integer, "
                       "nominal enum, or a flat integer struct");
  if (getOwner().empty() || !getOwner().starts_with('/') ||
      (getOwner().size() > 1 && getOwner().ends_with('/')))
    return emitOpError("owner must be a canonical absolute scope path");
  std::string expectedStableId = "slot/";
  if (getOwner() != "/") {
    expectedStableId.append(getOwner().drop_front());
    expectedStableId.push_back('/');
  }
  expectedStableId.append(getSymName());
  if (getStableId() != expectedStableId)
    return emitOpError("stable_id must match canonical owner/symbol identity");
  Operation *root = getOperation();
  while (root->getParentOp())
    root = root->getParentOp();
  unsigned releases = 0;
  root->walk([&](SlotReleaseOp release) {
    if (resolveSlot(release, release.getSlotAttr()) == *this)
      ++releases;
  });
  root->walk([&](SlotProposeReleaseOp release) {
    if (resolveSlot(release, release.getSlotAttr()) == *this)
      ++releases;
  });
  if (releases != 1)
    return emitOpError("slot requires exactly one release endpoint");
  return success();
}

LogicalResult SlotGetOp::verify() {
  SlotOp slot = resolveSlot(*this, getSlotAttr());
  if (!slot)
    return emitOpError() << "unresolved slot " << getSlot();
  if (!slotVisibleFrom(*this, slot))
    return emitOpError("slot is outside the access scope ancestry");
  Type payload = cast<QueueType>(slot.getInput().getType()).getElementType();
  if (getValid().getType() !=
      VarType::get(getContext(), IntegerType::get(getContext(), 1)))
    return emitOpError("valid result must be !ac.var<i1>");
  if (getValue().getType() != VarType::get(getContext(), payload))
    return emitOpError("value result must match slot Queue payload");
  return success();
}

LogicalResult SlotReleaseOp::verify() {
  SlotOp slot = resolveSlot(*this, getSlotAttr());
  if (!slot)
    return emitOpError() << "unresolved slot " << getSlot();
  if (!slotVisibleFrom(*this, slot))
    return emitOpError("slot is outside the release scope ancestry");
  if (!getWhen().hasOneBlock() || getWhen().front().getNumArguments() != 0)
    return emitOpError("when must contain one zero-argument block");
  Block &block = getWhen().front();
  for (Operation &operation : block.without_terminator()) {
    if (auto get = dyn_cast<SlotGetOp>(operation)) {
      if (get.getSlotAttr() != getSlotAttr())
        return emitOpError("when may only observe its endpoint slot");
      continue;
    }
    if (auto get = dyn_cast<TableGetOp>(operation)) {
      (void)get;
      continue;
    }
    if (isa<TableMatchOp, TableChooseOp>(operation))
      continue;
    if (!isPureExpressionOperation(&operation))
      return emitOpError() << "when operation '" << operation.getName()
                           << "' is not permitted";
  }
  auto yield = dyn_cast<SlotYieldOp>(block.getTerminator());
  if (!yield ||
      !cast<VarType>(yield.getValue().getType()).getElementType().isInteger(1))
    return emitOpError("when must terminate with ac.slot.yield !ac.var<i1>");
  return success();
}

LogicalResult SlotProposeReleaseOp::verify() {
  SlotOp slot = resolveSlot(*this, getSlotAttr());
  if (!slot)
    return emitOpError() << "unresolved slot " << getSlot();
  if (!slotVisibleFrom(*this, slot))
    return emitOpError("slot is outside the release scope ancestry");
  if (!isa_and_nonnull<RuleOp, FiringOp>((*this)->getParentOp()))
    return emitOpError("requires direct ac.rule or ac.firing ownership");
  auto when = dyn_cast<VarType>(getWhen().getType());
  if (!when || !when.getElementType().isInteger(1))
    return emitOpError("when must be !ac.var<i1>");
  return success();
}

LogicalResult InterfaceOp::verify() {
  if (getBody().empty())
    return emitOpError("interface declaration requires a body block");
  for (Operation &child : getBody().front())
    if (!isa<RoleOp, PortOp>(child))
      return emitOpError()
             << "interface body only permits ac.role and ac.port, "
             << "found " << child.getName();
  return verifyRoleContainer(*this);
}

LogicalResult ProtocolOp::verify() {
  if (getBody().empty())
    return emitOpError("protocol declaration requires a body block");
  for (Operation &child : getBody().front())
    if (!isa<RoleOp, StateOp, EventOp, TransitionOp, GuaranteeOp>(child))
      return emitOpError() << "protocol body contains unsupported operation "
                           << child.getName();

  if (failed(verifyRoleContainer(*this)))
    return failure();

  unsigned initialStates = 0;
  for (StateOp state : getBody().getOps<StateOp>())
    initialStates += state.getInitial() ? 1 : 0;
  if (initialStates != 1)
    return emitOpError()
           << "protocol requires exactly one initial state, found "
           << initialStates;

  llvm::SmallDenseSet<StringRef> guaranteeKinds;
  for (GuaranteeOp guarantee : getBody().getOps<GuaranteeOp>())
    if (!guaranteeKinds.insert(guarantee.getKind()).second)
      return guarantee.emitOpError()
             << "duplicate protocol guarantee '" << guarantee.getKind() << "'";

  SmallVector<TransitionOp> transitions;
  for (TransitionOp transition : getBody().getOps<TransitionOp>()) {
    if (!lookupChild<StateOp>(*this, transition.getSourceAttr()))
      return transition.emitOpError()
             << "unresolved transition source state '@"
             << transition.getSourceAttr().getValue() << "'";
    if (!lookupChild<StateOp>(*this, transition.getTargetAttr()))
      return transition.emitOpError()
             << "unresolved transition target state '@"
             << transition.getTargetAttr().getValue() << "'";
    if (!lookupChild<EventOp>(*this, transition.getEventAttr()))
      return transition.emitOpError()
             << "unresolved transition event '@"
             << transition.getEventAttr().getValue() << "'";
    transitions.push_back(transition);
  }

  for (unsigned i = 0; i < transitions.size(); ++i) {
    SmallVector<TransitionOp> overlapping{transitions[i]};
    for (unsigned j = i + 1; j < transitions.size(); ++j)
      if (transitions[i].getSourceAttr() == transitions[j].getSourceAttr() &&
          transitions[i].getEventAttr() == transitions[j].getEventAttr())
        overlapping.push_back(transitions[j]);
    if (overlapping.size() < 2)
      continue;
    llvm::SmallSet<int64_t, 4> priorities;
    for (TransitionOp transition : overlapping) {
      if (!transition.getPriority())
        return transition.emitOpError(
            "overlapping transitions require explicit priority");
      if (!priorities.insert(static_cast<int64_t>(*transition.getPriority()))
               .second)
        return transition.emitOpError(
            "overlapping transitions require unique priority");
    }
  }

  GuaranteeOp stablePending = findGuarantee(*this, "stable_pending");
  bool stable = stablePending && dyn_cast<BoolAttr>(stablePending.getValue()) &&
                cast<BoolAttr>(stablePending.getValue()).getValue();
  for (TransitionOp transition : transitions) {
    EventOp event = lookupChild<EventOp>(*this, transition.getEventAttr());
    if (transition.getTransfer() && transition.getRetain())
      return transition.emitOpError(
          "transition cannot both transfer and retain ownership");
    if (event.getAction() == "offer" && !transition.getTransfer() &&
        !transition.getRetain())
      return transition.emitOpError(
          "offer transition must transfer or retain ownership");
    if (event.getAction() == "offer" && transition.getRetain() && !stable)
      return transition.emitOpError(
          "retained pending offer requires stable_pending = true");
    if (event.getAction() == "retry" && !transition.getRetain())
      return transition.emitOpError(
          "retry transition must retain the pending offer");
    if (event.getAction() == "retry" && transition.getTransfer())
      return transition.emitOpError(
          "retry transition cannot transfer the pending offer");
    if (transition.getRetain() && event.getAction() != "offer" &&
        event.getAction() != "retry")
      return transition.emitOpError(
          "retain is only valid for offer and retry transitions");
  }

  enum class Ownership : uint8_t { NoPending = 1, Pending = 2 };
  SmallVector<StateOp> states(getBody().getOps<StateOp>());
  llvm::StringMap<unsigned> stateIndices;
  for (auto [index, state] : llvm::enumerate(states))
    stateIndices.try_emplace(state.getSymName(), index);
  auto stateIndex = [&](FlatSymbolRefAttr name) -> unsigned {
    auto found = stateIndices.find(name.getValue());
    assert(found != stateIndices.end() &&
           "transition state references were verified");
    return found->second;
  };

  SmallVector<SmallVector<unsigned>> outgoing(states.size());
  SmallVector<unsigned> transitionTargets;
  SmallVector<EventOp> transitionEvents;
  transitionTargets.reserve(transitions.size());
  transitionEvents.reserve(transitions.size());
  for (auto [index, transition] : llvm::enumerate(transitions)) {
    outgoing[stateIndex(transition.getSourceAttr())].push_back(index);
    transitionTargets.push_back(stateIndex(transition.getTargetAttr()));
    transitionEvents.push_back(
        lookupChild<EventOp>(*this, transition.getEventAttr()));
  }

  SmallVector<uint8_t> ownership(states.size(), 0);
  SmallVector<std::pair<unsigned, Ownership>> worklist;
  auto ownershipBit = [](Ownership value) {
    return static_cast<uint8_t>(value);
  };
  auto hasOwnership = [&](unsigned state, Ownership value) {
    return (ownership[state] & ownershipBit(value)) != 0;
  };
  auto addOwnership = [&](unsigned state, Ownership value) {
    uint8_t bit = ownershipBit(value);
    if (ownership[state] & bit)
      return;
    ownership[state] |= bit;
    worklist.emplace_back(state, value);
  };
  for (auto [index, state] : llvm::enumerate(states))
    if (state.getInitial())
      addOwnership(index, Ownership::NoPending);

  auto transferOwnership = [&](unsigned transitionIndex,
                               Ownership input) -> std::optional<Ownership> {
    TransitionOp transition = transitions[transitionIndex];
    StringRef action = transitionEvents[transitionIndex].getAction();
    if (action == "offer") {
      if (input == Ownership::Pending)
        return std::nullopt;
      return transition.getTransfer() ? Ownership::NoPending
                                      : Ownership::Pending;
    }
    if (action == "retry")
      return input == Ownership::Pending
                 ? std::optional<Ownership>(Ownership::Pending)
                 : std::nullopt;
    if (action == "cancel" || action == "reject")
      return input == Ownership::Pending
                 ? std::optional<Ownership>(Ownership::NoPending)
                 : std::nullopt;
    if (transition.getTransfer())
      return input == Ownership::Pending
                 ? std::optional<Ownership>(Ownership::NoPending)
                 : std::nullopt;
    return input;
  };

  for (unsigned cursor = 0; cursor < worklist.size(); ++cursor) {
    auto [source, input] = worklist[cursor];
    for (unsigned transitionIndex : outgoing[source])
      if (std::optional<Ownership> output =
              transferOwnership(transitionIndex, input))
        addOwnership(transitionTargets[transitionIndex], *output);
  }

  uint8_t conflicting =
      ownershipBit(Ownership::NoPending) | ownershipBit(Ownership::Pending);
  for (auto [index, state] : llvm::enumerate(states))
    if (ownership[index] == conflicting)
      return state.emitOpError() << "ownership state conflict at join state '@"
                                 << state.getSymName() << "'";

  auto firstTransition = [&](auto predicate) -> TransitionOp {
    for (auto [index, transition] : llvm::enumerate(transitions))
      if (predicate(index, transition))
        return transition;
    return {};
  };
  if (TransitionOp transition = firstTransition([&](unsigned index, auto op) {
        return transitionEvents[index].getAction() == "offer" &&
               hasOwnership(stateIndex(op.getSourceAttr()), Ownership::Pending);
      }))
    return transition.emitOpError(
        "offer cannot begin while another offer is pending");
  if (TransitionOp transition = firstTransition([&](unsigned index, auto op) {
        return transitionEvents[index].getAction() == "retry" &&
               hasOwnership(stateIndex(op.getSourceAttr()),
                            Ownership::NoPending);
      }))
    return transition.emitOpError("retry requires a pending offer");
  if (TransitionOp transition = firstTransition([&](unsigned index, auto op) {
        StringRef action = transitionEvents[index].getAction();
        return (action == "cancel" || action == "reject") &&
               hasOwnership(stateIndex(op.getSourceAttr()),
                            Ownership::NoPending);
      }))
    return transition.emitOpError(
        "ownership resolution requires a pending offer");
  if (TransitionOp transition = firstTransition([&](unsigned index, auto op) {
        StringRef action = transitionEvents[index].getAction();
        return op.getTransfer() && action != "offer" && action != "retry" &&
               action != "cancel" && action != "reject" &&
               hasOwnership(stateIndex(op.getSourceAttr()),
                            Ownership::NoPending);
      }))
    return transition.emitOpError(
        "ownership transfer requires a pending offer");
  if (TransitionOp transition = firstTransition([&](unsigned index, auto op) {
        if (ownership[stateIndex(op.getSourceAttr())] != 0)
          return false;
        StringRef action = transitionEvents[index].getAction();
        return op.getTransfer() || action == "retry" || action == "cancel" ||
               action == "reject";
      }))
    return transition.emitOpError(
        "ownership resolution is unreachable from the initial state");

  for (auto [index, state] : llvm::enumerate(states)) {
    if (!hasOwnership(index, Ownership::Pending))
      continue;
    if (state.getTerminal())
      return state.emitOpError() << "terminal state '@" << state.getSymName()
                                 << "' is reachable with pending ownership";
    if (outgoing[index].empty())
      return state.emitOpError()
             << "pending ownership reaches state '@" << state.getSymName()
             << "' with no outgoing transition";
  }

  GuaranteeOp maxInflight = findGuarantee(*this, "max_inflight");
  if (maxInflight) {
    auto value = dyn_cast<IntegerAttr>(maxInflight.getValue());
    if (!value || !value.getType().isSignlessInteger(64) || value.getInt() <= 0)
      return maxInflight.emitOpError(
          "max_inflight requires a positive i64 value");
    if (value.getInt() > 1 && !findGuarantee(*this, "correlation"))
      return maxInflight.emitOpError(
          "max_inflight greater than one requires correlation");
  }
  GuaranteeOp backpressure = findGuarantee(*this, "backpressure");
  if (backpressure)
    if (auto value = dyn_cast<StringAttr>(backpressure.getValue());
        value && value.getValue() == "custom" &&
        !findGuarantee(*this, "custom_backpressure"))
      return backpressure.emitOpError(
          "custom backpressure requires a custom_backpressure declaration");
  GuaranteeOp ordering = findGuarantee(*this, "ordering");
  if (ordering)
    if (auto value = dyn_cast<StringAttr>(ordering.getValue());
        value && value.getValue() == "per_key" &&
        !findGuarantee(*this, "correlation"))
      return ordering.emitOpError("per_key ordering requires correlation");
  GuaranteeOp completion = findGuarantee(*this, "completion");
  if (completion) {
    if (auto value = dyn_cast<StringAttr>(completion.getValue())) {
      if (value.getValue() == "on_response" &&
          !findGuarantee(*this, "correlation"))
        return completion.emitOpError(
            "on_response completion requires correlation");
      auto hasReachableAction = [&](StringRef action) {
        return llvm::any_of(transitions, [&](TransitionOp transition) {
          return ownership[stateIndex(transition.getSourceAttr())] &&
                 lookupChild<EventOp>(*this, transition.getEventAttr())
                         .getAction() == action;
        });
      };
      if (value.getValue() == "on_response" && !hasReachableAction("response"))
        return completion.emitOpError(
            "on_response completion requires a reachable response event");
      if (value.getValue() == "on_accept" && !hasReachableAction("accept"))
        return completion.emitOpError(
            "on_accept completion requires a reachable accept event");
      if (value.getValue() == "on_terminal_phase" &&
          llvm::none_of(llvm::enumerate(states), [&](auto indexedState) {
            return indexedState.value().getTerminal() &&
                   ownership[indexedState.index()] != 0;
          }))
        return completion.emitOpError(
            "on_terminal_phase completion requires a reachable terminal state");
    }
  }
  GuaranteeOp correlation = findGuarantee(*this, "correlation");
  if (correlation) {
    auto field = dyn_cast<StringAttr>(correlation.getValue());
    if (field && !field.getValue().empty()) {
      Type correlationType;
      for (TransitionOp transition : transitions) {
        if (!ownership[stateIndex(transition.getSourceAttr())])
          continue;
        EventOp event = lookupChild<EventOp>(*this, transition.getEventAttr());
        if (event.getAction() != "offer")
          continue;
        Operation *declaration = recordDecl(event, event.getPayload());
        std::optional<unsigned> index =
            declaration ? findField(declaration, field.getValue())
                        : std::nullopt;
        if (!index)
          return correlation.emitOpError()
                 << "correlation field '" << field.getValue()
                 << "' is missing from reachable offer/response payload";
        Type type = fieldType(declaration, *index);
        if (!correlationType)
          correlationType = type;
        else if (type != correlationType)
          return correlation.emitOpError()
                 << "correlation field '" << field.getValue() << "' has type "
                 << type << " but expected " << correlationType;
      }
      if (!correlationType)
        return correlation.emitOpError()
               << "correlation field '" << field.getValue()
               << "' requires a reachable offer event";
      for (TransitionOp transition : transitions) {
        if (!ownership[stateIndex(transition.getSourceAttr())])
          continue;
        EventOp event = lookupChild<EventOp>(*this, transition.getEventAttr());
        if (event.getAction() != "offer" && event.getAction() != "response")
          continue;
        Operation *declaration = recordDecl(event, event.getPayload());
        std::optional<unsigned> index =
            declaration ? findField(declaration, field.getValue())
                        : std::nullopt;
        if (!index)
          return correlation.emitOpError()
                 << "correlation field '" << field.getValue()
                 << "' is missing from reachable offer/response payload";
        Type type = fieldType(declaration, *index);
        if (type != correlationType)
          return correlation.emitOpError()
                 << "correlation field '" << field.getValue() << "' has type "
                 << type << " but expected " << correlationType;
      }
    }
  }
  return success();
}

LogicalResult RoleOp::verify() {
  if (!isa_and_nonnull<InterfaceOp, ProtocolOp>(getOperation()->getParentOp()))
    return emitOpError(
        "role must be a direct child of ac.interface or ac.protocol");
  if (getCardinality() != "exclusive" && getCardinality() != "shared")
    return emitOpError() << "unsupported role cardinality '" << getCardinality()
                         << "'";
  return success();
}

LogicalResult StateOp::verify() {
  if (!isa_and_nonnull<ProtocolOp>(getOperation()->getParentOp()))
    return emitOpError("state must be a direct child of ac.protocol");
  return success();
}

LogicalResult EventOp::verify() {
  auto protocol = dyn_cast_or_null<ProtocolOp>(getOperation()->getParentOp());
  if (!protocol)
    return emitOpError("event must be a direct child of ac.protocol");
  if (failed(verifyRoleReference(*this, protocol, getFromAttr(),
                                 "event source")) ||
      failed(verifyRoleReference(*this, protocol, getToAttr(), "event target")))
    return failure();
  if (getFromAttr() == getToAttr())
    return emitOpError("event source and target roles must differ");
  if (!isProtocolPayloadType(getPayload()))
    return emitOpError(
        "event payload type must be a normative ACIR value type");
  if (failed(verifyNamedTypes(*this, getPayload())))
    return failure();
  static constexpr StringRef actions[] = {
      "offer", "accept", "cancel", "reject", "retry", "response", "notify"};
  if (!hasStringValue(getAction(), actions))
    return emitOpError() << "unsupported event action '" << getAction() << "'";
  return success();
}

LogicalResult TransitionOp::verify() {
  if (!isa_and_nonnull<ProtocolOp>(getOperation()->getParentOp()))
    return emitOpError("transition must be a direct child of ac.protocol");
  if (auto priority = getPriority(); priority && *priority > INT64_MAX)
    return emitOpError("transition priority must be a non-negative i64 value");
  WalkResult result = getGuard().walk([&](Operation *operation) {
    if (!isAllowedGuardExpression(operation)) {
      emitOpError() << "guard operation '" << operation->getName()
                    << "' is not in the pure expression allowlist";
      return WalkResult::interrupt();
    }
    auto effects = dyn_cast<MemoryEffectOpInterface>(operation);
    if (!effects) {
      emitOpError() << "allowed guard operation '" << operation->getName()
                    << "' must implement MemoryEffectOpInterface";
      return WalkResult::interrupt();
    }
    SmallVector<MemoryEffects::EffectInstance> instances;
    effects.getEffects(instances);
    if (!instances.empty()) {
      emitOpError() << "allowed guard operation '" << operation->getName()
                    << "' must have no memory effects";
      return WalkResult::interrupt();
    }
    return WalkResult::advance();
  });
  if (result.wasInterrupted())
    return failure();
  return success();
}

LogicalResult GuaranteeOp::verify() {
  if (!isa_and_nonnull<ProtocolOp>(getOperation()->getParentOp()))
    return emitOpError("guarantee must be a direct child of ac.protocol");
  StringRef kind = getKind();
  if (kind == "backpressure")
    return verifyStringGuarantee(
        *this, {"none", "accept", "credit", "capacity", "custom"});
  if (kind == "ordering")
    return verifyStringGuarantee(*this, {"fifo", "per_key", "unordered"});
  if (kind == "delivery")
    return verifyStringGuarantee(
        *this, {"exactly_once", "at_most_once", "best_effort"});
  if (kind == "completion")
    return verifyStringGuarantee(
        *this, {"on_accept", "on_response", "on_terminal_phase"});
  if (kind == "stable_pending") {
    if (!isa<BoolAttr>(getValue()))
      return emitOpError("stable_pending requires a boolean value");
    return success();
  }
  if (kind == "max_inflight")
    return success();
  if (kind == "correlation") {
    auto value = dyn_cast<StringAttr>(getValue());
    if (!value || value.getValue().empty())
      return emitOpError("correlation requires a non-empty field name");
    return success();
  }
  if (kind == "custom_backpressure") {
    auto value = dyn_cast<StringAttr>(getValue());
    if (!value || value.getValue().empty())
      return emitOpError(
          "custom_backpressure requires a non-empty declarative contract");
    return success();
  }
  return emitOpError() << "unknown mandatory protocol guarantee '" << kind
                       << "'";
}

LogicalResult PortOp::verify() {
  auto interface = dyn_cast_or_null<InterfaceOp>(getOperation()->getParentOp());
  if (!interface)
    return emitOpError("port must be a direct child of ac.interface");
  auto channel = dyn_cast<ChannelType>(getType());
  if (!channel)
    return emitOpError("port type must be !ac.channel<T, Protocol>");
  if (failed(verifyRoleReference(*this, interface, getFromAttr(),
                                 "port source")) ||
      failed(verifyRoleReference(*this, interface, getToAttr(), "port target")))
    return failure();
  if (getFromAttr() == getToAttr())
    return emitOpError("port source and target roles must differ");
  RoleOp fromRole = lookupChild<RoleOp>(interface, getFromAttr());
  if (fromRole.getDualAttr() != getToAttr())
    return emitOpError("port source and target roles must be dual");
  if (!isProtocolPayloadType(channel.getElementType()))
    return emitOpError(
        "channel payload type must be a normative ACIR value type");
  if (failed(verifyNamedTypes(*this, channel.getElementType())))
    return failure();
  ProtocolOp protocol = lookupProtocol(*this, channel.getProtocol());
  if (!protocol)
    return emitOpError() << "unresolved channel protocol '@"
                         << channel.getProtocol().getValue() << "'";
  RoleOp protocolFrom = lookupChild<RoleOp>(protocol, getProtocolFromAttr());
  if (!protocolFrom)
    return emitOpError() << "unresolved mapped protocol source role '@"
                         << getProtocolFromAttr().getValue() << "'";
  RoleOp protocolTo = lookupChild<RoleOp>(protocol, getProtocolToAttr());
  if (!protocolTo)
    return emitOpError() << "unresolved mapped protocol target role '@"
                         << getProtocolToAttr().getValue() << "'";
  if (protocolFrom.getDualAttr() != getProtocolToAttr() ||
      protocolTo.getDualAttr() != getProtocolFromAttr())
    return emitOpError("mapped protocol roles must be dual");
  RoleOp toRole = lookupChild<RoleOp>(interface, getToAttr());
  if (fromRole.getCardinality() != protocolFrom.getCardinality() ||
      toRole.getCardinality() != protocolTo.getCardinality())
    return emitOpError(
        "interface and mapped protocol roles must have matching cardinality");
  if (!matchesCarrierEvent(protocol, channel.getElementType(),
                           getProtocolFromAttr(), getProtocolToAttr()))
    return emitOpError() << "channel payload " << channel.getElementType()
                         << " from mapped protocol role '@"
                         << getProtocolFromAttr().getValue() << "' to '@"
                         << getProtocolToAttr().getValue()
                         << "' does not match any carrier event in protocol '@"
                         << channel.getProtocol().getValue() << "'";
  return success();
}

namespace {

ModuleFamilySchemaAttr graphFamilySchema(Operation *op) {
  return op ? op->getAttrOfType<ModuleFamilySchemaAttr>("schema")
            : ModuleFamilySchemaAttr();
}

FunctionType graphCaseType(Operation *op, StaticArgumentsAttr arguments) {
  auto module = dyn_cast_or_null<ModuleOp>(op);
  if (!module || !arguments)
    return {};
  for (ModuleCaseOp moduleCase :
       module.getBody().front().getOps<ModuleCaseOp>())
    if (moduleCase.getArguments() == arguments)
      return moduleCase.getFunctionType();
  return {};
}

LogicalResult verifyConcreteDictionary(Operation *op, DictionaryAttr values,
                                       StringRef subject) {
  for (NamedAttribute value : values)
    if (!isConcreteStaticValue(value.getValue()))
      return op->emitOpError() << subject
                               << " must contain only concrete builtin static "
                                  "values";
  LogicalResult result = success();
  values.walk([&](SymbolRefAttr reference) {
    if (lookupGraphSymbolReference(op, reference))
      return WalkResult::advance();
    op->emitOpError() << "unresolved static symbol reference '" << reference
                      << "'";
    result = failure();
    return WalkResult::interrupt();
  });
  if (failed(result))
    return failure();
  return success();
}

LogicalResult verifyOuterPlacement(Operation *op) {
  auto outer = dyn_cast_or_null<mlir::ModuleOp>(op->getParentOp());
  if (outer &&
      (!outer->getParentOp() || (isa<mlir::ModuleOp>(outer->getParentOp()) &&
                                 !outer->getParentOp()->getParentOp())))
    return success();
  return op->emitOpError("must be a direct child of the outer builtin.module");
}

LogicalResult verifyStructuralPlacement(Operation *op) {
  auto moduleCase = dyn_cast_or_null<ModuleCaseOp>(op->getParentOp());
  if (moduleCase && !moduleCase.getBody().empty() &&
      op->getBlock() == &moduleCase.getBody().front())
    return success();
  return op->emitOpError(
      "must be a direct child of one ac.module.case Graph block");
}

LogicalResult verifyExactBinding(Operation *op, DictionaryAttr binding,
                                 StringRef subject,
                                 StringRef requiredRegistry) {
  auto registry = binding.getAs<StringAttr>("registry");
  auto name = binding.getAs<StringAttr>("name");
  if (binding.size() != 2 || !registry || registry.getValue().empty() ||
      !name || name.getValue().empty() || registry.getValue() == "generic")
    return op->emitOpError()
           << subject << " requires exact registered {registry, name} metadata";
  if (registry.getValue() != requiredRegistry)
    return op->emitOpError() << subject << " requires registered registry '"
                             << requiredRegistry << "'";
  return success();
}

LogicalResult verifyCallShape(Operation *op, FunctionType signature,
                              TypeRange inputs, TypeRange outputs) {
  if (!signature)
    return op->emitOpError("definition has no canonical module signature");
  if (!llvm::equal(inputs, signature.getInputs()))
    return op->emitOpError("operand types do not match module signature");
  if (!llvm::equal(outputs, signature.getResults()))
    return op->emitOpError("result types do not match module signature");
  return success();
}

LogicalResult verifyStaticArgumentSet(Operation *op,
                                      StaticArgumentsAttr arguments,
                                      Operation *definition = nullptr) {
  if (!arguments)
    return op->emitOpError("instance requires complete dependent arguments");
  if (!definition)
    return success();
  ModuleFamilySchemaAttr schema = graphFamilySchema(definition);
  if (!schema)
    return op->emitOpError("definition has no typed family schema");
  auto parameters = schema.getParameters().getParameters();
  auto values = arguments.getArguments();
  if (parameters.size() != values.size())
    return op->emitOpError(
        "static argument names must exactly match definition parameters");
  for (auto [parameter, argument] :
       llvm::zip_equal(parameters.getAsRange<StaticParameterAttr>(),
                       values.getAsRange<DependentArgumentAttr>())) {
    if (parameter.getName() != argument.getName())
      return op->emitOpError(
          "static argument names must exactly match definition parameters");
    if (!staticValueMatchesType(parameter.getType().getValue(),
                                argument.getValue().getValue()))
      return op->emitOpError()
             << "static argument '" << parameter.getName().getValue()
             << "' must match its exact declared type";
  }
  if (isa<ModuleOp>(definition) && !graphCaseType(definition, arguments))
    return op->emitOpError("static arguments do not select one declared family case");
  return success();
}

bool isStructuralGraphChild(Operation &child) {
  auto file = child.getParentOp()->getParentOfType<mlir::ModuleOp>();
  auto kind =
      file ? file->getAttrOfType<StringAttr>("ac.model_kind") : StringAttr();
  const bool queueGraphChild =
      kind && kind.getValue() == "queue_graph" && isa<ScopeOp>(child);
  return isa<InstanceOp, ArrayOp, InstancesOp, ViewOp, QueueOp, EventQueueOp,
             ResourceOp, AddressSpaceOp, AddressMapOp, TimeDomainOp, ProcessOp,
             RequireOp, EnsureOp, StatOp, ArchitectureObligationOp, ReturnOp>(
             child) ||
         queueGraphChild || child.getName().getStringRef() == "arith.constant";
}

bool isStableHierarchySegment(StringRef segment) {
  return !segment.empty() && llvm::all_of(segment, [](char c) {
    return llvm::isAlnum(c) || c == '_' || c == '-';
  });
}

LogicalResult
verifyRuntimeReferences(ModuleOp module,
                        const llvm::StringMap<Operation *> &producerIndex) {
  LogicalResult result = success();
  auto lookupExpected = [&](Operation *operation, StringRef reference,
                            StringRef expectedName) -> Operation * {
    Operation *target = producerIndex.lookup(reference);
    if (!target) {
      operation->emitOpError()
          << "unresolved runtime target '@" << reference << "'";
      result = failure();
      return nullptr;
    }
    if (target->getName().getStringRef() != expectedName) {
      operation->emitOpError() << "runtime target '@" << reference
                               << "' must resolve to " << expectedName;
      result = failure();
      return nullptr;
    }
    return target;
  };
  auto lookupQueueRef = [&](Operation *operation,
                            SymbolRefAttr reference) -> QueueOp {
    Operation *target = lookupRuntimeSymbol(operation, reference);
    if (!target) {
      operation->emitOpError()
          << "unresolved runtime target '" << reference << "'";
      result = failure();
      return {};
    }
    auto queue = dyn_cast<QueueOp>(target);
    if (!queue) {
      operation->emitOpError()
          << "runtime target '" << reference << "' must resolve to ac.queue";
      result = failure();
      return {};
    }
    return queue;
  };
  auto moduleCase = cast<ModuleCaseOp>(module.getBody().front().front());
  for (ProcessOp process : moduleCase.getBody().front().getOps<ProcessOp>()) {
    WalkResult walk = process.getBody().walk([&](Operation *operation) {
      if (auto send = dyn_cast<TrySendOp>(operation)) {
        auto queue = lookupQueueRef(send, send.getQueue());
        if (queue && queue.getPayload() != send.getValue().getType()) {
          send.emitOpError()
              << "value type " << send.getValue().getType()
              << " does not match queue payload type " << queue.getPayload();
          result = failure();
        }
      } else if (auto recv = dyn_cast<TryRecvOp>(operation)) {
        auto queue = lookupQueueRef(recv, recv.getQueue());
        if (queue && queue.getPayload() != recv.getValue().getType()) {
          recv.emitOpError()
              << "result type " << recv.getValue().getType()
              << " does not match queue payload type " << queue.getPayload();
          result = failure();
        }
      } else if (auto schedule = dyn_cast<ScheduleOp>(operation)) {
        auto target = dyn_cast_or_null<ProcessOp>(lookupExpected(
            schedule, schedule.getTarget(), ProcessOp::getOperationName()));
        if (target && (target.getCaptures().size() != 1 ||
                       target.getCaptures().front().getType() !=
                           schedule.getValue().getType())) {
          schedule.emitOpError()
              << "scheduled value type " << schedule.getValue().getType()
              << " must match the target process's single capture type";
          result = failure();
        }
      } else if (auto wait = dyn_cast<WaitForOp>(operation)) {
        (void)lookupExpected(wait, wait.getResource(),
                             ResourceOp::getOperationName());
      } else if (auto await = dyn_cast<AwaitEventOp>(operation)) {
        (void)lookupExpected(await, await.getEventQueue(),
                             EventQueueOp::getOperationName());
      } else if (auto probe = dyn_cast<ProbeOp>(operation)) {
        StringRef expected =
            llvm::StringSwitch<StringRef>(probe.getKind())
                .Case("queue", QueueOp::getOperationName())
                .Case("resource", ResourceOp::getOperationName())
                .Case("module", ProcessOp::getOperationName())
                .Case("storage", AddressSpaceOp::getOperationName())
                .Case("protocol", QueueOp::getOperationName())
                .Case("event_queue", EventQueueOp::getOperationName())
                .Case("external_io", ProcessOp::getOperationName())
                .Case("statistics", StatOp::getOperationName())
                .Default(StringRef());
        Operation *target = lookupExpected(probe, probe.getTarget(), expected);
        if (auto queue = dyn_cast_or_null<QueueOp>(target);
            queue && probe.getKind() == "queue" &&
            queue.getPayload() != probe.getValue().getType()) {
          probe.emitOpError()
              << "result type " << probe.getValue().getType()
              << " does not match queue payload type " << queue.getPayload();
          result = failure();
        }
        if (auto eventQueue = dyn_cast_or_null<EventQueueOp>(target);
            eventQueue &&
            eventQueue.getPayload() != probe.getValue().getType()) {
          probe.emitOpError() << "result type " << probe.getValue().getType()
                              << " does not match event queue payload type "
                              << eventQueue.getPayload();
          result = failure();
        }
      } else if (auto stat = dyn_cast<StatAddOp>(operation)) {
        (void)lookupExpected(stat, stat.getStat(), StatOp::getOperationName());
      }
      return failed(result) ? WalkResult::interrupt() : WalkResult::advance();
    });
    if (walk.wasInterrupted())
      return failure();
  }
  return result;
}

LogicalResult verifyProcessOperations(ModuleOp module) {
  auto moduleCase = cast<ModuleCaseOp>(module.getBody().front().front());
  for (ProcessOp process : moduleCase.getBody().front().getOps<ProcessOp>()) {
    if (failed(process.verify()))
      return failure();
    LogicalResult result = success();
    process.getBody().walk([&](Operation *operation) {
      result =
          TypeSwitch<Operation *, LogicalResult>(operation)
              .Case<TrySendOp, TryRecvOp, ScheduleOp, WaitUntilOp, WaitForOp,
                    AwaitEventOp, YieldSimOp, RequireOp, EnsureOp, AssertOp,
                    ProbeOp, StatAddOp, InstrumentationOp>(
                  [](auto op) { return op.verify(); })
              .Default([](Operation *) { return success(); });
      return failed(result) ? WalkResult::interrupt() : WalkResult::advance();
    });
    if (failed(result))
      return failure();
  }
  return success();
}

Operation *resolveSystemMember(SystemOp system, SymbolRefAttr reference,
                               bool instrumentation) {
  if (reference.getRootReference() != system.getRootAttr().getValue())
    return nullptr;
  ArrayRef<FlatSymbolRefAttr> nested = reference.getNestedReferences();
  if (nested.size() != (instrumentation ? 2u : 1u))
    return nullptr;
  auto file = system->getParentOfType<mlir::ModuleOp>();
  if (!file)
    return nullptr;
  auto module = dyn_cast_or_null<ModuleOp>(
      SymbolTable::lookupSymbolIn(file, system.getRootAttr()));
  if (!module || module.getBody().empty())
    return nullptr;
  auto moduleCase = dyn_cast<ModuleCaseOp>(module.getBody().front().front());
  if (!moduleCase)
    return nullptr;
  ProcessOp process;
  for (ProcessOp candidate : moduleCase.getBody().front().getOps<ProcessOp>())
    if (candidate.getSymName() == nested.front().getValue()) {
      process = candidate;
      break;
    }
  if (!process)
    return nullptr;
  if (!instrumentation)
    return process;
  Operation *found = nullptr;
  process.getBody().walk([&](InstrumentationOp candidate) {
    if (candidate.getSymName() == nested.back().getValue()) {
      found = candidate;
      return WalkResult::interrupt();
    }
    return WalkResult::advance();
  });
  return found;
}

} // namespace

llvm::Expected<ModuleInterfaceAttr> materializeModuleInterface(
    ModuleInterfaceAttr interface, StaticArgumentsAttr arguments,
    FunctionType signature, mlir::ModuleOp file) {
  auto error = [](llvm::Twine message) -> llvm::Error {
    return llvm::createStringError(llvm::inconvertibleErrorCode(), message);
  };
  auto canonicalSigned = [](llvm::APInt value) {
    return value.sextOrTrunc(std::max(1u, value.getSignificantBits()));
  };
  auto staticInteger = [&](StaticValueAttr wrapped)
      -> llvm::Expected<llvm::APInt> {
    auto value = wrapped ? dyn_cast<StaticIntValueAttr>(wrapped.getValue())
                         : StaticIntValueAttr();
    if (!value)
      return error("dependent integer resolved to a non-integer static value");
    llvm::APInt bits = value.getValue().getValue();
    return value.getType().getIsSigned()
               ? canonicalSigned(bits)
               : canonicalSigned(bits.zext(bits.getBitWidth() + 1));
  };
  auto lookupArgument = [&](StringRef name)
      -> llvm::Expected<StaticValueAttr> {
    for (auto argument :
         arguments.getArguments().getAsRange<StaticArgumentAttr>())
      if (argument.getName().getValue() == name)
        return argument.getValue();
    return error("dependent parameter is absent from concrete arguments");
  };
  auto lookupField = [&](StaticValueAttr root, ArrayAttr path)
      -> llvm::Expected<StaticValueAttr> {
    StaticValueAttr current = root;
    for (Attribute rawName : path) {
      auto name = cast<StringAttr>(rawName);
      auto config = dyn_cast<StaticConfigValueAttr>(current.getValue());
      if (!config)
        return error("dependent field traverses a non-config static value");
      StaticValueAttr next;
      for (auto field : config.getFields().getFields().getAsRange<
               StaticConfigFieldValueAttr>())
        if (field.getName() == name) {
          next = StaticValueAttr::get(field.getContext(), field.getValue());
          break;
        }
      if (!next)
        return error("dependent field path is absent from static config");
      current = next;
    }
    return current;
  };
  std::function<llvm::Expected<llvm::APInt>(DependentValueAttr)> evaluate =
      [&](DependentValueAttr expression) -> llvm::Expected<llvm::APInt> {
    Attribute value = expression.getValue();
    if (auto literal = dyn_cast<DependentIntegerLiteralAttr>(value))
      return canonicalSigned(literal.getValue());
    if (auto literal = dyn_cast<DependentStaticLiteralAttr>(value))
      return staticInteger(literal.getValue());
    if (auto parameter = dyn_cast<DependentParameterAttr>(value)) {
      auto resolved = lookupArgument(parameter.getName());
      return resolved ? staticInteger(*resolved) : resolved.takeError();
    }
    if (auto field = dyn_cast<DependentFieldAttr>(value)) {
      auto root = lookupArgument(field.getRoot().getName());
      if (!root)
        return root.takeError();
      auto resolved = lookupField(*root, field.getPath());
      return resolved ? staticInteger(*resolved) : resolved.takeError();
    }
    auto binary = [&](DependentValueAttr lhs, DependentValueAttr rhs,
                      char operation) -> llvm::Expected<llvm::APInt> {
      auto left = evaluate(lhs);
      if (!left)
        return left.takeError();
      auto right = evaluate(rhs);
      if (!right)
        return right.takeError();
      unsigned width = operation == '*'
                           ? left->getBitWidth() + right->getBitWidth()
                           : std::max(left->getBitWidth(), right->getBitWidth()) +
                                 1;
      llvm::APInt l = left->sext(width);
      llvm::APInt r = right->sext(width);
      return canonicalSigned(operation == '+' ? l + r
                             : operation == '-' ? l - r
                                                : l * r);
    };
    if (auto add = dyn_cast<DependentAddAttr>(value))
      return binary(add.getLhs(), add.getRhs(), '+');
    if (auto sub = dyn_cast<DependentSubAttr>(value))
      return binary(sub.getLhs(), sub.getRhs(), '-');
    if (auto mul = dyn_cast<DependentMulAttr>(value))
      return binary(mul.getLhs(), mul.getRhs(), '*');
    auto width = [&](DependentValueAttr capacity,
                     bool count) -> llvm::Expected<llvm::APInt> {
      auto evaluated = evaluate(capacity);
      if (!evaluated)
        return evaluated.takeError();
      if (evaluated->isNegative() || (!count && evaluated->isZero()))
        return error("dependent width capacity is outside its admitted domain");
      llvm::APInt positive = evaluated->zext(evaluated->getBitWidth() + 1);
      if (!count)
        positive -= 1;
      return llvm::APInt(64,
                         std::max<uint64_t>(1, positive.getActiveBits()), true);
    };
    if (auto index = dyn_cast<DependentIndexWidthAttr>(value))
      return width(index.getCapacity(), false);
    if (auto count = dyn_cast<DependentCountWidthAttr>(value))
      return width(count.getCapacity(), true);
    return error("dependent integer expression contains an unsupported record");
  };
  auto positive = [&](DependentValueAttr expression)
      -> llvm::Expected<uint64_t> {
    auto value = evaluate(expression);
    if (!value)
      return value.takeError();
    if (value->isNegative() || value->isZero() || value->getActiveBits() > 64)
      return error("dependent cardinality must be a positive uint64 value");
    return value->getZExtValue();
  };
  auto lookupNominalDeclaration = [&](SymbolRefAttr reference) -> Operation * {
    if (!file)
      return nullptr;
    for (TypeScopeOp scope : file.getOps<TypeScopeOp>())
      for (Operation &candidate : scope.getBody().front())
        if (SymbolTable::getSymbolName(&candidate) &&
            SymbolTable::getSymbolName(&candidate).getValue() ==
                reference.getLeafReference().getValue())
          return &candidate;
    return nullptr;
  };
  auto resolveStaticValue = [&](DependentValueAttr expression,
                                Attribute expectedType)
      -> llvm::Expected<StaticValueAttr> {
    Attribute value = expression.getValue();
    StaticValueAttr resolved;
    if (auto literal = dyn_cast<DependentStaticLiteralAttr>(value))
      resolved = literal.getValue();
    else if (auto parameter = dyn_cast<DependentParameterAttr>(value)) {
      auto found = lookupArgument(parameter.getName());
      if (!found)
        return found.takeError();
      resolved = *found;
    } else if (auto field = dyn_cast<DependentFieldAttr>(value)) {
      auto root = lookupArgument(field.getRoot().getName());
      if (!root)
        return root.takeError();
      auto found = lookupField(*root, field.getPath());
      if (!found)
        return found.takeError();
      resolved = *found;
    } else if (auto integerType = dyn_cast<StaticIntTypeAttr>(expectedType)) {
      auto integer = evaluate(expression);
      if (!integer)
        return integer.takeError();
      unsigned width = integerType.getWidth();
      bool fits = integerType.getIsSigned()
                      ? integer->isSignedIntN(width)
                      : !integer->isNegative() && integer->isIntN(width);
      if (!fits)
        return error("dependent nominal integer argument is out of range");
      auto storageType = IntegerType::get(expression.getContext(), width);
      auto storage = integerType.getIsSigned() ? integer->sextOrTrunc(width)
                                               : integer->zextOrTrunc(width);
      auto typed = StaticIntValueAttr::get(
          expression.getContext(), integerType,
          IntegerAttr::get(storageType, storage));
      resolved = StaticValueAttr::get(expression.getContext(), typed);
    } else {
      return error("dependent nominal argument requires a typed static value");
    }
    if (!staticValueMatchesType(expectedType, resolved.getValue()))
      return error("dependent nominal argument has the wrong static type");
    return resolved;
  };
  auto resolveNominalArguments = [&](TypeExprNominalAttr nominal,
                                     Operation *declaration)
      -> llvm::Expected<DependentArgumentsAttr> {
    auto parameters = declaration->getAttrOfType<StaticParametersAttr>(
        "parameters");
    Builder builder(nominal.getContext());
    ArrayAttr declared = parameters ? parameters.getParameters()
                                    : builder.getArrayAttr({});
    ArrayAttr supplied = nominal.getArguments().getArguments();
    if (declared.size() != supplied.size())
      return error("nominal arguments must exactly match declaration parameters");
    SmallVector<Attribute> resolved;
    for (auto [rawParameter, rawArgument] : llvm::zip_equal(declared, supplied)) {
      auto parameter = cast<StaticParameterAttr>(rawParameter);
      auto argument = cast<DependentArgumentAttr>(rawArgument);
      if (parameter.getName() != argument.getName())
        return error(
            "nominal argument order/names must match declaration parameters");
      auto value = resolveStaticValue(argument.getValue(),
                                      parameter.getType().getValue());
      if (!value)
        return value.takeError();
      auto literal = DependentStaticLiteralAttr::get(nominal.getContext(), *value);
      auto dependent = DependentValueAttr::get(nominal.getContext(), literal);
      resolved.push_back(DependentArgumentAttr::get(
          nominal.getContext(), argument.getName(), dependent));
    }
    return DependentArgumentsAttr::get(nominal.getContext(),
                                       builder.getArrayAttr(resolved));
  };
  std::function<llvm::Expected<Type>(TypeExprAttr)> materialize =
      [&](TypeExprAttr expression) -> llvm::Expected<Type> {
    Attribute value = expression.getValue();
    MLIRContext *context = expression.getContext();
    if (auto concrete = dyn_cast<TypeExprConcreteAttr>(value))
      return concrete.getType().getValue();
    if (auto bits = dyn_cast<TypeExprBitsAttr>(value)) {
      auto width = positive(bits.getWidth());
      if (!width || *width > (1u << 16))
        return width ? error("dependent bit width exceeds backend bound")
                     : width.takeError();
      return IntegerType::get(
          context, *width,
          bits.getIsSigned()
              ? IntegerType::SignednessSemantics::Signed
              : IntegerType::SignednessSemantics::Signless);
    }
    if (auto tuple = dyn_cast<TypeExprTupleAttr>(value)) {
      SmallVector<Type> elements;
      for (auto element : tuple.getElements().getAsRange<TypeExprAttr>()) {
        auto type = materialize(element);
        if (!type)
          return type.takeError();
        elements.push_back(*type);
      }
      return TupleType::get(context, elements);
    }
    if (auto array = dyn_cast<TypeExprValueArrayAttr>(value)) {
      auto length = positive(array.getLength());
      auto element = materialize(array.getElement());
      if (!length || !element || *length > static_cast<uint64_t>(INT64_MAX))
        return !length ? length.takeError()
               : !element ? element.takeError()
                          : error("dependent array length exceeds int64");
      return ValueArrayType::get(context, static_cast<int64_t>(*length),
                                 *element);
    }
    if (auto range = dyn_cast<TypeExprRangeAttr>(value)) {
      auto lower = evaluate(range.getLowerInclusive());
      auto upper = evaluate(range.getUpperExclusive());
      if (!lower || !upper)
        return lower ? upper.takeError() : lower.takeError();
      llvm::APInt limit(65, 1);
      limit <<= 64;
      llvm::APInt lowerWide = lower->sextOrTrunc(65);
      llvm::APInt upperWide = upper->sextOrTrunc(65);
      if (lower->isNegative() || upper->isNegative() || lowerWide.uge(upperWide) ||
          lowerWide.uge(limit) || upperWide.ugt(limit))
        return error("dependent range bounds are invalid");
      return RangeType::get(context, lowerWide.getZExtValue(),
                            (upperWide - 1).trunc(64).getZExtValue());
    }
    if (auto queue = dyn_cast<TypeExprQueueAttr>(value)) {
      auto payload = materialize(queue.getPayload());
      auto lanes = positive(queue.getLanes());
      auto rate = positive(queue.getRate());
      if (!payload || !lanes || !rate)
        return !payload ? payload.takeError()
               : !lanes ? lanes.takeError()
                        : rate.takeError();
      if (*lanes > static_cast<uint64_t>(INT64_MAX) || *rate > *lanes)
        return error("dependent Queue lanes/rate are invalid");
      return QueueType::get(context, *payload, static_cast<int64_t>(*lanes),
                            static_cast<int64_t>(*rate));
    }
    if (auto nominal = dyn_cast<TypeExprNominalAttr>(value)) {
      Operation *declaration = lookupNominalDeclaration(nominal.getDeclaration());
      if (!declaration)
        return error("nominal type expression declaration is unresolved");
      auto resolved = resolveNominalArguments(nominal, declaration);
      if (!resolved)
        return resolved.takeError();
      if (isa_and_nonnull<StructOp>(declaration))
        return StructType::get(context, nominal.getDeclaration());
      if (isa_and_nonnull<PacketOp>(declaration))
        return PacketType::get(context, nominal.getDeclaration());
      if (isa_and_nonnull<TransactionOp>(declaration))
        return TransactionType::get(context, nominal.getDeclaration());
      if (isa_and_nonnull<EnumOp>(declaration))
        return EnumType::get(context, nominal.getDeclaration());
      return error("nominal type expression declaration is unresolved");
    }
    return error("logical type expression is unresolved or unsupported");
  };
  std::function<llvm::Expected<TypeExprAttr>(TypeExprAttr)> resolveLogical =
      [&](TypeExprAttr expression) -> llvm::Expected<TypeExprAttr> {
    Attribute value = expression.getValue();
    MLIRContext *context = expression.getContext();
    if (auto nominal = dyn_cast<TypeExprNominalAttr>(value)) {
      Operation *declaration = lookupNominalDeclaration(nominal.getDeclaration());
      if (!declaration)
        return error("nominal type expression declaration is unresolved");
      auto resolved = resolveNominalArguments(nominal, declaration);
      if (!resolved)
        return resolved.takeError();
      return TypeExprAttr::get(
          context, TypeExprNominalAttr::get(context, nominal.getDeclaration(),
                                            *resolved));
    }
    if (auto queue = dyn_cast<TypeExprQueueAttr>(value)) {
      auto payload = resolveLogical(queue.getPayload());
      auto lanes = evaluate(queue.getLanes());
      auto rate = evaluate(queue.getRate());
      if (!payload || !lanes || !rate)
        return !payload ? payload.takeError()
               : !lanes ? lanes.takeError()
                        : rate.takeError();
      auto lanesValue = DependentValueAttr::get(
          context, DependentIntegerLiteralAttr::get(context, *lanes));
      auto rateValue = DependentValueAttr::get(
          context, DependentIntegerLiteralAttr::get(context, *rate));
      return TypeExprAttr::get(
          context,
          TypeExprQueueAttr::get(context, *payload, lanesValue, rateValue));
    }
    if (auto array = dyn_cast<TypeExprValueArrayAttr>(value)) {
      auto length = evaluate(array.getLength());
      auto element = resolveLogical(array.getElement());
      if (!length || !element)
        return length ? element.takeError() : length.takeError();
      auto lengthValue = DependentValueAttr::get(
          context, DependentIntegerLiteralAttr::get(context, *length));
      return TypeExprAttr::get(
          context,
          TypeExprValueArrayAttr::get(context, lengthValue, *element));
    }
    if (auto tuple = dyn_cast<TypeExprTupleAttr>(value)) {
      SmallVector<Attribute> elements;
      for (TypeExprAttr element : tuple.getElements().getAsRange<TypeExprAttr>()) {
        auto resolved = resolveLogical(element);
        if (!resolved)
          return resolved.takeError();
        elements.push_back(*resolved);
      }
      Builder builder(context);
      return TypeExprAttr::get(
          context,
          TypeExprTupleAttr::get(context, builder.getArrayAttr(elements)));
    }
    auto concrete = materialize(expression);
    if (!concrete)
      return concrete.takeError();
    return TypeExprAttr::get(
        context,
        TypeExprConcreteAttr::get(context, TypeAttr::get(*concrete)));
  };
  Builder builder(interface.getContext());
  SmallVector<Attribute> ports;
  std::function<bool(TypeExprAttr)> containsNominal =
      [&](TypeExprAttr expression) -> bool {
    Attribute value = expression.getValue();
    if (isa<TypeExprNominalAttr>(value))
      return true;
    if (auto queue = dyn_cast<TypeExprQueueAttr>(value))
      return containsNominal(queue.getPayload());
    if (auto array = dyn_cast<TypeExprValueArrayAttr>(value))
      return containsNominal(array.getElement());
    if (auto tuple = dyn_cast<TypeExprTupleAttr>(value))
      return llvm::any_of(tuple.getElements().getAsRange<TypeExprAttr>(),
                          containsNominal);
    return false;
  };
  size_t input = 0;
  size_t output = 0;
  for (InterfacePortAttr port :
       interface.getPorts().getAsRange<InterfacePortAttr>()) {
    auto concrete = materialize(port.getLogicalType());
    if (!concrete)
      return concrete.takeError();
    Type expected;
    if (port.getDirection().getValue() == "input") {
      if (signature && input >= signature.getNumInputs())
        return error("materialized interface has excess inputs");
      expected = signature ? signature.getInput(input) : *concrete;
      ++input;
    } else {
      if (signature && output >= signature.getNumResults())
        return error("materialized interface has excess outputs");
      expected = signature ? signature.getResult(output) : *concrete;
      ++output;
    }
    if (*concrete != expected)
      return error(
          "materialized logical interface disagrees with concrete case signature");
    llvm::Expected<TypeExprAttr> expression =
        containsNominal(port.getLogicalType())
            ? resolveLogical(port.getLogicalType())
            : llvm::Expected<TypeExprAttr>(TypeExprAttr::get(
                  interface.getContext(),
                  TypeExprConcreteAttr::get(interface.getContext(),
                                            TypeAttr::get(expected))));
    if (!expression)
      return expression.takeError();
    ports.push_back(InterfacePortAttr::get(
        interface.getContext(), port.getName(), port.getDirection(), *expression,
        port.getProvenance()));
  }
  if (signature &&
      (input != signature.getNumInputs() ||
       output != signature.getNumResults()))
    return error("materialized interface arity is incomplete");
  return ModuleInterfaceAttr::get(interface.getContext(),
                                  builder.getArrayAttr(ports));
}

LogicalResult SystemOp::verify() {
  if (failed(verifyOuterPlacement(*this)))
    return failure();
  if (!isStableHierarchySegment(getRootName()))
    return emitOpError(
        "root instance name must be one stable hierarchy segment");
  if (getTickEpoch() != 0)
    return emitOpError("global tick epoch must be exactly 0");
  if (!hasStringValue(getTickUnit(), {"cycle", "ps", "ns", "us", "ms", "s"}))
    return emitOpError() << "unsupported exact global tick unit '"
                         << getTickUnit() << "'";
  auto seedKind = getSeedPolicy().getAs<StringAttr>("kind");
  auto seedValue = getSeedPolicy().getAs<IntegerAttr>("value");
  if (getSeedPolicy().size() != 2 || !seedKind ||
      seedKind.getValue() != "fixed" || !seedValue ||
      !seedValue.getType().isSignlessInteger(64))
    return emitOpError(
        "seed policy requires exact {kind = \"fixed\", value = signless i64} "
        "schema");
  if (seedValue.getInt() < 0)
    return emitOpError("fixed seed value must be a non-negative signless i64");
  auto resultId = getResultSchema().getAs<StringAttr>("id");
  auto resultFormat = getResultSchema().getAs<StringAttr>("format");
  if (getResultSchema().size() != 2 || !resultId ||
      resultId.getValue().empty() || !resultFormat ||
      resultFormat.getValue() != "json")
    return emitOpError("result schema requires exact {id = non-empty string, "
                       "format = \"json\"}");
  if (SymbolRefAttr workload = getPrimaryWorkloadAttr()) {
    auto process = dyn_cast_or_null<ProcessOp>(
        resolveSystemMember(*this, workload, false));
    if (!process)
      return emitOpError() << "primary workload '" << workload
                           << "' is unresolved";
    if (process.getKind() != "workload")
      return emitOpError() << "primary workload '" << workload
                           << "' must reference a workload process";
  }
  for (Attribute value : getInstrumentation()) {
    auto reference = dyn_cast<SymbolRefAttr>(value);
    if (!reference)
      return emitOpError("instrumentation entries must be symbol references");
    Operation *target = resolveSystemMember(*this, reference, true);
    if (!target || target->getName().getStringRef() != "ac.instrumentation")
      return emitOpError() << "instrumentation reference '" << reference
                           << "' does not resolve to ac.instrumentation";
  }
  return success();
}

LogicalResult ModuleCaseOp::verify() {
  auto family = dyn_cast_or_null<ModuleOp>(getOperation()->getParentOp());
  if (!family)
    return emitOpError("must be a direct child of one ac.module family");
  if (getBody().empty())
    return emitOpError("requires one concrete Graph body block");
  auto file = getOperation()->getParentOfType<mlir::ModuleOp>();
  auto materialized = materializeModuleInterface(
      family.getSchema().getInterface(), getArguments(), getFunctionType(), file);
  if (!materialized)
    return emitOpError()
           << "dependent interface does not materialize to the case signature: "
           << llvm::toString(materialized.takeError());
  Block &entry = getBody().front();
  if (!llvm::equal(entry.getArgumentTypes(), getFunctionType().getInputs()))
    return emitOpError("block arguments must match the concrete function type");
  if (entry.empty() || !isa<ReturnOp>(entry.back()))
    return emitOpError("concrete Graph body must end with ac.return");
  for (Operation &operation : entry.without_terminator())
    if (!isStructuralGraphChild(operation))
      return operation.emitOpError(
          "operation is not legal in an ac.module.case Graph region");
  llvm::StringMap<Operation *> producerIndex;
  llvm::StringSet<> localNames;
  llvm::StringSet<> stableIds;
  llvm::StringSet<> paths;
  for (Operation &operation : entry.without_terminator()) {
    if (auto name = mlir::SymbolTable::getSymbolName(&operation)) {
      if (!localNames.insert(name.getValue()).second)
        return operation.emitOpError("duplicate local structural name");
      producerIndex[name.getValue()] = &operation;
    }
    if (auto stableId = operation.getAttrOfType<StringAttr>("stable_id");
        stableId && !stableIds.insert(stableId.getValue()).second)
      return operation.emitOpError("duplicate local structural stable id");
    if (auto path = operation.getAttrOfType<StringAttr>("path");
        path && !paths.insert(path.getValue()).second)
      return operation.emitOpError("duplicate local structural path");
  }
  for (Operation &operation : entry.without_terminator()) {
    LogicalResult local = TypeSwitch<Operation *, LogicalResult>(&operation)
                              .Case<QueueOp, EventQueueOp, ResourceOp,
                                    AddressSpaceOp, AddressMapOp, TimeDomainOp>(
                                  [](auto op) { return op.verify(); })
                              .Default([](Operation *) { return success(); });
    if (failed(local))
      return failure();
  }
  if (failed(verifyModuleResourceReferences(family, producerIndex)) ||
      failed(verifyProcessOperations(family)) ||
      failed(verifyRuntimeReferences(family, producerIndex)))
    return failure();
  for (ViewOp view : entry.getOps<ViewOp>())
    if (failed(view.verify()) ||
        failed(view.verifyWithProducerIndex(producerIndex)))
      return failure();
  return success();
}

LogicalResult ModuleOp::verify() {
  if (failed(verifyOuterPlacement(*this)))
    return failure();
  if (!getSource() || !getSchema() || getSource() != getSchema().getSource())
    return emitOpError("module source owner must exactly match its family schema");
  if (getBody().empty() || getBody().front().empty())
    return emitOpError("module family requires one or more ordered cases");
  Block &entry = getBody().front();
  auto declared = getSchema().getCases().getCases();
  if (entry.getOperations().size() != declared.size())
    return emitOpError("module family body count must exactly match declared cases");
  for (auto [operation, arguments] : llvm::zip_equal(entry, declared)) {
    auto moduleCase = dyn_cast<ModuleCaseOp>(operation);
    if (!moduleCase || moduleCase.getArguments() != arguments)
      return operation.emitOpError(
          "module body must contain only non-symbol cases in schema order");
  }
  return success();
}

LogicalResult ModuleExternOp::verify() {
  if (failed(verifyOuterPlacement(*this)))
    return failure();
  if (!getSource() || !getSchema() || getSource() != getSchema().getSource())
    return emitOpError("external module source owner must match its schema");
  if (failed(verifyExactBinding(*this, getImplementation(),
                                "external module implementation", "cpp")))
    return failure();
  StringRef name = getImplementation().getAs<StringAttr>("name").getValue();
  if (!getStructuralProviderRegistry(getContext()).hasExternal(name))
    return emitOpError() << "structural provider 'cpp:" << name
                         << "' is not registered";
  return success();
}

LogicalResult ModuleImportOp::verify() {
  if (failed(verifyOuterPlacement(*this)))
    return failure();
  if (!getSource() || !getSchema() || getSource() != getSchema().getSource())
    return emitOpError("module import source owner must match its complete schema");
  auto file = getOperation()->getParentOfType<mlir::ModuleOp>();
  for (Attribute rawCase : getSchema().getCases().getCases()) {
    auto arguments = cast<StaticArgumentsAttr>(rawCase);
    auto materialized = materializeModuleInterface(
        getSchema().getInterface(), arguments, {}, file);
    if (!materialized)
      return emitOpError()
             << "import case interface is not concretely materializable: "
             << llvm::toString(materialized.takeError());
  }
  return success();
}

LogicalResult InstanceOp::verify() {
  if (failed(verifyStructuralPlacement(*this)))
    return failure();
  Operation *definition = lookupGraphSymbol(*this, getDefinitionAttr());
  if (!isa_and_nonnull<ModuleOp, ModuleExternOp, ModuleImportOp>(definition))
    return emitOpError() << "unresolved module definition '"
                         << getDefinitionAttr() << "'";
  if (!isStableHierarchySegment(getSymName()) ||
      !isStableHierarchySegment(getStableId()) ||
      !isStableHierarchySegment(getPath()))
    return emitOpError(
        "instance name, stable id, and path must be stable local segments");
  if (failed(verifyStaticArgumentSet(*this, getStaticArgs(), definition)))
    return failure();
  FunctionType signature = graphCaseType(definition, getStaticArgs());
  if (!signature && isa<ModuleExternOp, ModuleImportOp>(definition))
    signature = FunctionType::get(getContext(), getInputs().getTypes(),
                                  getOutputs().getTypes());
  return verifyCallShape(*this, signature,
                         getInputs().getTypes(), getOutputs().getTypes());
}

LogicalResult ArrayOp::verify() {
  if (failed(verifyStructuralPlacement(*this)))
    return failure();
  Operation *definition = lookupGraphSymbol(*this, getDefinitionAttr());
  if (!isa_and_nonnull<ModuleOp, ModuleExternOp, ModuleImportOp>(definition))
    return emitOpError() << "unresolved array element definition '"
                         << getDefinitionAttr() << "'";
  if (!isStableHierarchySegment(getSymName()) ||
      !isStableHierarchySegment(getStableId()) ||
      !isStableHierarchySegment(getPath()))
    return emitOpError(
        "array name, stable id, and path must be stable local segments");
  if (getShape().empty())
    return emitOpError("array shape must have at least one dimension");
  uint64_t count = 1;
  for (int64_t extent : getShape()) {
    if (extent < 0)
      return emitOpError("array shape dimensions must be non-negative");
    if (extent != 0 && count > std::numeric_limits<uint64_t>::max() /
                                   static_cast<uint64_t>(extent))
      return emitOpError("array cardinality overflows 64 bits");
    count *= static_cast<uint64_t>(extent);
  }
  constexpr uint64_t maxStaticElements = 1U << 20;
  if (count > maxStaticElements)
    return emitOpError(
        "array cardinality exceeds static elaboration bound 1048576");
  if (getStaticArgs().size() != count)
    return emitOpError("array requires one concrete static argument set per "
                       "lexicographically ordered element");
  FunctionType signature;
  for (Attribute value : getStaticArgs()) {
    auto arguments = dyn_cast<StaticArgumentsAttr>(value);
    if (!arguments ||
        failed(verifyStaticArgumentSet(*this, arguments, definition)))
      return emitOpError(
          "array static arguments must be complete typed argument tuples");
    auto physical = graphCaseType(definition, arguments);
    if (!physical && isa<ModuleExternOp, ModuleImportOp>(definition) &&
        getInputs().empty() && getOutputs().empty())
      physical = FunctionType::get(getContext(), {}, {});
    if (!physical || (signature && signature != physical))
      return emitOpError("array cases must share one exact physical signature");
    signature = physical;
  }
  auto matchesRepeatedSignature = [count](TypeRange actual,
                                          TypeRange elementTypes) {
    if (elementTypes.empty())
      return actual.empty();
    if (count > std::numeric_limits<uint64_t>::max() / elementTypes.size() ||
        actual.size() != count * elementTypes.size())
      return false;
    for (auto [index, type] : llvm::enumerate(actual))
      if (type != elementTypes[index % elementTypes.size()])
        return false;
    return true;
  };
  if (!matchesRepeatedSignature(getInputs().getTypes(),
                                signature.getInputs()) ||
      !matchesRepeatedSignature(getOutputs().getTypes(),
                                signature.getResults()))
    return emitOpError("array flattened interface shape does not match element "
                       "signature and static cardinality");
  return success();
}

LogicalResult InstancesOp::verify() {
  if (failed(verifyStructuralPlacement(*this)))
    return failure();
  if (!isStableHierarchySegment(getSymName()) ||
      !isStableHierarchySegment(getStableId()) ||
      !isStableHierarchySegment(getPath()))
    return emitOpError("collection name, stable id, and path must be stable "
                       "local segments");
  size_t count = getDefinitions().size();
  if (count == 0 || getNames().size() != count ||
      getStableIds().size() != count || getPaths().size() != count ||
      getStaticArgs().size() != count)
    return emitOpError("ordered instance metadata arrays must have identical "
                       "non-zero cardinality");
  llvm::SmallDenseSet<StringRef> names;
  llvm::SmallDenseSet<StringRef> ids;
  llvm::SmallDenseSet<StringRef> paths;
  for (size_t index = 0; index < count; ++index) {
    auto definition = dyn_cast<FlatSymbolRefAttr>(getDefinitions()[index]);
    if (!definition)
      return emitOpError("definitions must contain flat module symbols");
    Operation *target = lookupGraphSymbol(*this, definition);
    if (!isa_and_nonnull<ModuleOp, ModuleExternOp, ModuleImportOp>(target))
      return emitOpError() << "unresolved collection definition '" << definition
                           << "'";
    auto arguments = dyn_cast<StaticArgumentsAttr>(getStaticArgs()[index]);
    if (!arguments || failed(verifyStaticArgumentSet(*this, arguments, target)))
      return emitOpError("collection static arguments must be complete typed tuples");
    auto signature = graphCaseType(target, arguments);
    if (!signature && isa<ModuleExternOp, ModuleImportOp>(target))
      signature = getInterface();
    if (!signature || signature != getInterface())
      return emitOpError("collection element case does not implement the exact common interface");
    StringRef name = cast<StringAttr>(getNames()[index]).getValue();
    StringRef id = cast<StringAttr>(getStableIds()[index]).getValue();
    StringRef path = cast<StringAttr>(getPaths()[index]).getValue();
    if (!isStableHierarchySegment(name) || !names.insert(name).second ||
        !isStableHierarchySegment(id) || !ids.insert(id).second)
      return emitOpError("collection names and stable ids must be non-empty "
                         "and unique in declared order");
    if (!isStableHierarchySegment(path) || !paths.insert(path).second)
      return emitOpError("collection paths must be stable, unique "
                         "parent-relative segments");
  }
  auto matchesRepeatedInterface = [count](TypeRange actual,
                                          TypeRange elementTypes) {
    if (elementTypes.empty())
      return actual.empty();
    if (count > std::numeric_limits<size_t>::max() / elementTypes.size() ||
        actual.size() != count * elementTypes.size())
      return false;
    for (auto [index, type] : llvm::enumerate(actual))
      if (type != elementTypes[index % elementTypes.size()])
        return false;
    return true;
  };
  if (!matchesRepeatedInterface(getInputs().getTypes(),
                                getInterface().getInputs()) ||
      !matchesRepeatedInterface(getOutputs().getTypes(),
                                getInterface().getResults()))
    return emitOpError("ordered collection IO does not match its common "
                       "interface shape");
  return success();
}

LogicalResult ViewOp::verify() {
  if (failed(verifyStructuralPlacement(*this)))
    return failure();
  if (!isStableHierarchySegment(getSymName()))
    return emitOpError("view name must be a stable local segment");
  return success();
}

LogicalResult ViewOp::verifyWithProducerIndex(
    const llvm::StringMap<Operation *> &producerIndex) {
  ArrayRef<int64_t> indices = getIndices();
  ArrayRef<int64_t> shape = getShape();
  if (llvm::any_of(shape, [](int64_t value) { return value < 0; }))
    return emitOpError("view shape must be fully static and non-negative");
  auto checkedProduct = [&](ArrayRef<int64_t> dimensions,
                            uint64_t &product) -> LogicalResult {
    product = 1;
    for (int64_t extent : dimensions) {
      if (extent < 0)
        return emitOpError(
            "source shapes must be fully static and non-negative");
      if (extent != 0 && product > std::numeric_limits<uint64_t>::max() /
                                       static_cast<uint64_t>(extent))
        return emitOpError("view cardinality overflows 64 bits");
      product *= static_cast<uint64_t>(extent);
    }
    return success();
  };

  SmallVector<SmallVector<int64_t>> sourceShapes;
  SmallVector<SmallVector<Value>> sources;
  llvm::SmallDenseSet<Operation *> sourceProducers;
  auto getProducerShape = [&](Operation *producer) {
    SmallVector<int64_t> producerShape;
    if (auto instance = dyn_cast<InstanceOp>(producer))
      producerShape.push_back(instance.getNumResults());
    else if (auto array = dyn_cast<ArrayOp>(producer)) {
      producerShape.append(array.getShape().begin(), array.getShape().end());
      auto arguments = array.getStaticArgs().empty()
                           ? StaticArgumentsAttr()
                           : dyn_cast<StaticArgumentsAttr>(
                                 array.getStaticArgs()[0]);
      auto signature = graphCaseType(
          lookupGraphSymbol(array, array.getDefinitionAttr()), arguments);
      producerShape.push_back(signature ? signature.getNumResults() : 0);
    } else if (auto instances = dyn_cast<InstancesOp>(producer)) {
      producerShape.push_back(instances.getDefinitions().size());
      producerShape.push_back(instances.getInterface().getNumResults());
    } else if (auto view = dyn_cast<ViewOp>(producer))
      producerShape.append(view.getShape().begin(), view.getShape().end());
    return producerShape;
  };
  if (getSourceProducers().size() != getSourceShapes().size())
    return emitOpError(
        "source_producers and source_shapes must have identical cardinality");
  size_t operandOffset = 0;
  for (auto [producerReference, attribute] :
       llvm::zip(getSourceProducers(), getSourceShapes())) {
    auto sourceShape = dyn_cast<DenseI64ArrayAttr>(attribute);
    if (!sourceShape)
      return emitOpError("source_shapes entries must be dense i64 arrays");
    uint64_t cardinality = 0;
    if (failed(checkedProduct(sourceShape.asArrayRef(), cardinality)))
      return failure();
    if (cardinality > getInputs().size() - operandOffset)
      return emitOpError("source_shapes do not partition the view operands");
    SmallVector<Value> source;
    source.append(getInputs().begin() + operandOffset,
                  getInputs().begin() + operandOffset + cardinality);
    operandOffset += cardinality;
    auto producerSymbol = cast<FlatSymbolRefAttr>(producerReference);
    if (!isStableHierarchySegment(producerSymbol.getValue()))
      return emitOpError("source producer IDs must be stable local segments");
    Operation *producer = producerIndex.lookup(producerSymbol.getValue());
    if (!producer)
      return emitOpError() << "source producer '" << producerReference
                           << "' is unresolved";
    if (producer == getOperation())
      return emitOpError("view cannot name itself as a source producer");
    if (!isa<InstanceOp, ArrayOp, InstancesOp, ViewOp>(producer) ||
        producer->getBlock() != getOperation()->getBlock())
      return emitOpError("source producer must resolve to a direct structural "
                         "producer in the same ac.module");
    if (!sourceProducers.insert(producer).second)
      return emitOpError("source producers must not repeat");
    if (!source.empty()) {
      if (source.front().getDefiningOp() != producer ||
          producer->getNumResults() != source.size())
        return emitOpError(
            "each source must be the complete result group of its declared "
            "structural producer");
      for (auto [index, value] : llvm::enumerate(source))
        if (value != producer->getResult(index))
          return emitOpError(
              "source operands must preserve producer result order");
    }
    SmallVector<int64_t> producerShape = getProducerShape(producer);
    if (producerShape != sourceShape.asArrayRef())
      return emitOpError("source_shapes must exactly match producer shapes");
    sourceShapes.emplace_back(sourceShape.asArrayRef().begin(),
                              sourceShape.asArrayRef().end());
    sources.push_back(std::move(source));
  }
  if (operandOffset != getInputs().size())
    return emitOpError("source_shapes do not partition the view operands");

  SmallVector<Value> expected;
  StringRef kind = getKind();
  if (kind == "select") {
    if (sources.size() != 1 || indices.size() != sourceShapes[0].size() ||
        !shape.empty() || getAxisAttr())
      return emitOpError("select requires one source, one coordinate per "
                         "dimension, scalar shape, and no axis");
    uint64_t ordinal = 0;
    for (auto [coordinate, extent] : llvm::zip(indices, sourceShapes[0])) {
      if (coordinate < 0 || coordinate >= extent)
        return emitOpError("select coordinate is out of bounds");
      ordinal = ordinal * static_cast<uint64_t>(extent) + coordinate;
    }
    expected.push_back(sources[0][ordinal]);
  } else if (kind == "slice") {
    if (sources.size() != 1 || getAxisAttr() ||
        indices.size() != 2 * sourceShapes[0].size())
      return emitOpError("slice requires one source and [lower, upper] bounds "
                         "for every dimension");
    SmallVector<int64_t> derivedShape;
    for (size_t dimension = 0; dimension < sourceShapes[0].size();
         ++dimension) {
      int64_t lower = indices[2 * dimension];
      int64_t upper = indices[2 * dimension + 1];
      if (lower < 0 || upper < lower || upper > sourceShapes[0][dimension])
        return emitOpError("slice bounds are invalid");
      derivedShape.push_back(upper - lower);
    }
    if (derivedShape != shape)
      return emitOpError("slice result shape must equal its bound extents");
    for (size_t ordinal = 0; ordinal < sources[0].size(); ++ordinal) {
      size_t remainder = ordinal;
      bool included = true;
      for (size_t reverse = sourceShapes[0].size(); reverse-- > 0;) {
        int64_t coordinate = remainder % sourceShapes[0][reverse];
        remainder /= sourceShapes[0][reverse];
        included &= coordinate >= indices[2 * reverse] &&
                    coordinate < indices[2 * reverse + 1];
      }
      if (included)
        expected.push_back(sources[0][ordinal]);
    }
  } else if (kind == "concat") {
    if (sources.size() < 2 || !indices.empty() || !getAxisAttr())
      return emitOpError("concat requires at least two sources, an axis, and "
                         "no index metadata");
    int64_t axis = getAxisAttr().getInt();
    size_t rank = sourceShapes.front().size();
    if (axis < 0 || static_cast<size_t>(axis) >= rank)
      return emitOpError("concat axis is out of bounds");
    SmallVector<int64_t> derivedShape = sourceShapes.front();
    derivedShape[axis] = 0;
    for (ArrayRef<int64_t> sourceShape : sourceShapes) {
      if (sourceShape.size() != rank)
        return emitOpError("concat source ranks must match");
      for (size_t dimension = 0; dimension < rank; ++dimension)
        if (dimension != static_cast<size_t>(axis) &&
            sourceShape[dimension] != derivedShape[dimension])
          return emitOpError("concat non-axis dimensions must match");
      if (sourceShape[axis] >
          std::numeric_limits<int64_t>::max() - derivedShape[axis])
        return emitOpError("concat axis extent overflows signed i64");
      derivedShape[axis] += sourceShape[axis];
    }
    if (derivedShape != shape)
      return emitOpError("concat result shape is not derived from its sources");
    uint64_t outer = 1, inner = 1;
    for (int64_t extent : ArrayRef<int64_t>(shape).take_front(axis))
      outer *= extent;
    for (int64_t extent : ArrayRef<int64_t>(shape).drop_front(axis + 1))
      inner *= extent;
    for (uint64_t outerIndex = 0; outerIndex < outer; ++outerIndex)
      for (auto [sourceIndex, source] : llvm::enumerate(sources)) {
        uint64_t chunk = sourceShapes[sourceIndex][axis] * inner;
        expected.append(source.begin() + outerIndex * chunk,
                        source.begin() + (outerIndex + 1) * chunk);
      }
  } else if (kind == "zip") {
    if (sources.size() != 2 || !indices.empty() || getAxisAttr() ||
        sourceShapes[0] != sourceShapes[1])
      return emitOpError("zip requires two equal-shaped sources and no axis or "
                         "index metadata");
    SmallVector<int64_t> derivedShape = sourceShapes[0];
    derivedShape.push_back(2);
    if (derivedShape != shape)
      return emitOpError("zip result shape must append source count");
    for (size_t index = 0; index < sources[0].size(); ++index) {
      expected.push_back(sources[0][index]);
      expected.push_back(sources[1][index]);
    }
  } else if (kind == "permutation") {
    if (sources.size() != 1 || getAxisAttr() ||
        !llvm::equal(shape, sourceShapes[0]) ||
        indices.size() != sources[0].size())
      return emitOpError("permutation requires one source, unchanged shape, "
                         "and one index per element");
    llvm::SmallDenseSet<int64_t> seen;
    for (int64_t index : indices) {
      if (index < 0 || static_cast<size_t>(index) >= sources[0].size() ||
          !seen.insert(index).second)
        return emitOpError(
            "permutation indices must be an in-bounds bijection");
      expected.push_back(sources[0][index]);
    }
  } else if (kind == "elementwise") {
    if (sources.size() < 2 || !indices.empty() || getAxisAttr())
      return emitOpError("elementwise requires at least two sources and no "
                         "axis or index metadata");
    if (llvm::any_of(sourceShapes, [&](const auto &sourceShape) {
          return !llvm::equal(sourceShape, sourceShapes.front());
        }))
      return emitOpError("elementwise source shapes must match");
    SmallVector<int64_t> derivedShape = sourceShapes.front();
    derivedShape.push_back(sources.size());
    if (derivedShape != shape)
      return emitOpError("elementwise result shape must append source count");
    for (size_t index = 0; index < sources.front().size(); ++index)
      for (ArrayRef<Value> source : sources)
        expected.push_back(source[index]);
  } else {
    return emitOpError() << "unsupported static view kind '" << kind << "'";
  }
  uint64_t cardinality = 0;
  if (failed(checkedProduct(shape, cardinality)))
    return failure();
  if (cardinality != expected.size() ||
      !llvm::equal(getOutputs().getTypes(),
                   llvm::map_range(
                       expected, [](Value value) { return value.getType(); })))
    return emitOpError("resolved view shape/order/types do not match outputs");
  return success();
}

LogicalResult ReturnOp::verify() {
  auto moduleCase = dyn_cast_or_null<ModuleCaseOp>(getOperation()->getParentOp());
  if (!moduleCase)
    return emitOpError("must terminate an ac.module.case Graph region");
  if (!llvm::equal(getOperandTypes(), moduleCase.getFunctionType().getResults()))
    return emitOpError(
        "operand types and count must exactly match module case results");
  if (llvm::any_of(getOperandTypes(),
                   [](Type type) { return isa<ResourceTokenType>(type); }))
    return emitOpError(
        "private ownership handle cannot be exported from ac.module");
  return success();
}

LogicalResult verifyTopologyTypeUses(Operation *operation) {
  if (failed(verifyGraphStructure(operation)))
    return failure();
  std::function<LogicalResult(Type, Value)> verifyType =
      [&](Type type, Value value) -> LogicalResult {
    if (auto function = dyn_cast<FunctionType>(type)) {
      for (Type input : function.getInputs())
        if (failed(verifyType(input, {})))
          return failure();
      for (Type result : function.getResults())
        if (failed(verifyType(result, {})))
          return failure();
      return success();
    }
    if (Type nested = findNestedTopologyLeaf(type))
      return operation->emitOpError() << "topology type " << nested
                                      << " cannot be nested inside " << type;
    if (auto flow = dyn_cast<FlowType>(type)) {
      if (!isProtocolPayloadType(flow.getElementType()))
        return operation->emitOpError(
            "flow payload type must be a normative ACIR value type");
      if (failed(verifyNamedTypes(operation, flow.getElementType())))
        return failure();
      ProtocolOp protocol = lookupProtocol(operation, flow.getProtocol());
      if (!protocol)
        return operation->emitOpError() << "unresolved flow protocol '@"
                                        << flow.getProtocol().getValue() << "'";
      if (!matchesCarrierEvent(protocol, flow.getElementType()))
        return operation->emitOpError()
               << "flow payload " << flow.getElementType()
               << " does not match any carrier event in protocol '@"
               << flow.getProtocol().getValue() << "'";
      if (value && !value.hasOneUse() && !value.use_empty())
        return operation->emitOpError(
            "flow value has more than one functional use");
    }
    if (auto endpoint = dyn_cast<EndpointType>(type)) {
      auto module = operation->getParentOfType<mlir::ModuleOp>();
      InterfaceOp interface =
          module ? dyn_cast_or_null<InterfaceOp>(SymbolTable::lookupSymbolIn(
                       module, endpoint.getInterface()))
                 : InterfaceOp();
      if (!interface)
        return operation->emitOpError()
               << "unresolved endpoint interface '@"
               << endpoint.getInterface().getValue() << "'";
      RoleOp role = lookupChild<RoleOp>(interface, endpoint.getRole());
      if (!role)
        return operation->emitOpError()
               << "endpoint role '@" << endpoint.getRole().getValue()
               << "' is not a member of interface '@"
               << endpoint.getInterface().getValue() << "'";
      if (role.getCardinality() == "exclusive" && value && !value.hasOneUse() &&
          !value.use_empty())
        return operation->emitOpError(
            "exclusive endpoint value has more than one structural use");
    }
    return success();
  };

  auto verifyAttribute = [&](Attribute attribute) -> LogicalResult {
    if (!attribute)
      return success();
    LogicalResult result = success();
    attribute.walk([&](TypeAttr type) {
      if (failed(verifyType(type.getValue(), {}))) {
        result = failure();
        return WalkResult::interrupt();
      }
      return WalkResult::advance();
    });
    if (failed(result))
      return failure();
    attribute.walk([&](Type type) {
      if (failed(verifyType(type, {}))) {
        result = failure();
        return WalkResult::interrupt();
      }
      return WalkResult::advance();
    });
    return result;
  };

  for (Value result : operation->getResults())
    if (failed(verifyType(result.getType(), result)))
      return failure();
  for (OpOperand &operand : operation->getOpOperands())
    if (failed(verifyType(operand.get().getType(), operand.get())))
      return failure();
  for (Region &region : operation->getRegions())
    for (Block &block : region)
      for (BlockArgument argument : block.getArguments())
        if (failed(verifyType(argument.getType(), argument)))
          return failure();
  for (NamedAttribute attribute : operation->getAttrs())
    if (failed(verifyAttribute(attribute.getValue())))
      return failure();
  if (failed(verifyAttribute(operation->getPropertiesAsAttribute())) ||
      failed(verifyAttribute(LocationAttr(operation->getLoc()))))
    return failure();
  return success();
}

namespace {

SymbolRefAttr qualifiedRuntimeOwner(Operation *operation, StringRef local) {
  if (auto definition = operation->getParentOfType<ModuleOp>())
    return SymbolRefAttr::get(
        operation->getContext(), definition.getSymName(),
        {FlatSymbolRefAttr::get(operation->getContext(), local)});
  return SymbolRefAttr::get(operation->getContext(), local);
}

Operation *resolvedRuntimeTarget(Operation *operation, StringRef local) {
  ModuleCaseOp moduleCase = operation->getParentOfType<ModuleCaseOp>();
  if (!moduleCase || moduleCase.getBody().empty())
    return nullptr;
  for (Operation &candidate : moduleCase.getBody().front()) {
    auto name =
        candidate.getAttrOfType<StringAttr>(SymbolTable::getSymbolAttrName());
    if (name && name.getValue() == local)
      return &candidate;
  }
  return nullptr;
}

DictionaryAttr runtimeEffectParameters(Operation *operation, StringRef kind,
                                       StringRef identity) {
  Builder builder(operation->getContext());
  Operation *owner = resolvedRuntimeTarget(operation, identity);
  if (!owner)
    if (auto process = operation->getParentOfType<ProcessOp>())
      owner = process;
  if (owner)
    if (auto owners = owner->getAttrOfType<ArrayAttr>("ac.frozen_owners"))
      return builder.getDictionaryAttr({
          builder.getNamedAttr("identity_phase",
                               builder.getStringAttr("elaborated_absolute")),
          builder.getNamedAttr("owner_kind", builder.getStringAttr(kind)),
          builder.getNamedAttr("owners", owners),
      });
  return builder.getDictionaryAttr({
      builder.getNamedAttr("identity_phase",
                           builder.getStringAttr("definition_pre_freeze")),
      builder.getNamedAttr("owner_kind", builder.getStringAttr(kind)),
      builder.getNamedAttr("identity", builder.getStringAttr(identity)),
  });
}

void addEffect(SmallVectorImpl<MemoryEffects::EffectInstance> &effects,
               Operation *operation, MemoryEffects::Effect *effect,
               StringRef identity, StringRef kind,
               SideEffects::Resource *resource) {
  effects.emplace_back(effect, qualifiedRuntimeOwner(operation, identity),
                       runtimeEffectParameters(operation, kind, identity),
                       resource);
}

DictionaryAttr contractEffectParameters(Operation *operation, StringRef phase,
                                        StringRef identity) {
  Builder builder(operation->getContext());
  if (auto process = operation->getParentOfType<ProcessOp>())
    if (auto owners = process->getAttrOfType<ArrayAttr>("ac.frozen_owners"))
      return builder.getDictionaryAttr({
          builder.getNamedAttr("identity_phase",
                               builder.getStringAttr("elaborated_absolute")),
          builder.getNamedAttr("owner_kind", builder.getStringAttr("contract")),
          builder.getNamedAttr("owners", owners),
          builder.getNamedAttr("contract_phase", builder.getStringAttr(phase)),
      });
  if (operation->getAttrOfType<BoolAttr>("ac.freeze_proven"))
    return builder.getDictionaryAttr({
        builder.getNamedAttr("identity_phase",
                             builder.getStringAttr("elaborated_absolute")),
        builder.getNamedAttr("owner_kind", builder.getStringAttr("contract")),
        builder.getNamedAttr("identity", builder.getStringAttr(identity)),
        builder.getNamedAttr("contract_phase", builder.getStringAttr(phase)),
        builder.getNamedAttr("freeze_proven", builder.getBoolAttr(true)),
    });
  return builder.getDictionaryAttr({
      builder.getNamedAttr("identity_phase",
                           builder.getStringAttr("definition_pre_freeze")),
      builder.getNamedAttr("owner_kind", builder.getStringAttr("contract")),
      builder.getNamedAttr("identity", builder.getStringAttr(identity)),
      builder.getNamedAttr("contract_phase", builder.getStringAttr(phase)),
  });
}

ProcessOp enclosingProcess(Operation *operation) {
  return operation->getParentOfType<ProcessOp>();
}

LogicalResult requireProcess(Operation *operation) {
  if (enclosingProcess(operation))
    return success();
  return operation->emitOpError("must be nested in ac.process");
}

StringRef processIdentity(Operation *operation) {
  ProcessOp process = enclosingProcess(operation);
  return process ? process.getSymName() : StringRef("invalid_process");
}

void addContractEffect(SmallVectorImpl<MemoryEffects::EffectInstance> &effects,
                       Operation *operation) {
  if (!isa<AssertOp>(operation) &&
      isa_and_nonnull<ModuleCaseOp>(operation->getParentOp())) {
    constexpr StringLiteral identity = "contracts";
    effects.emplace_back(
        MemoryEffects::Read::get(), qualifiedRuntimeOwner(operation, identity),
        contractEffectParameters(operation, "topology_freeze", identity),
        ModuleStateResource::get());
    return;
  }
  StringRef identity = processIdentity(operation);
  effects.emplace_back(MemoryEffects::Write::get(),
                       qualifiedRuntimeOwner(operation, identity),
                       contractEffectParameters(operation, "runtime", identity),
                       ExternalIOResource::get());
}

bool isSuspension(Operation *operation) {
  return isa<WaitUntilOp, WaitForOp, AwaitEventOp, YieldSimOp>(operation);
}

bool isLinearAcrossSuspension(Type type) {
  return isa<FlowType, ResourceTokenType>(type);
}

bool isAllowedProcessOperation(Operation *operation) {
  StringRef name = operation->getName().getStringRef();
  if (name.starts_with("arith.") || name.starts_with("index.") ||
      name == "func.call" ||
      isa<scf::IfOp, scf::ForOp, scf::WhileOp, scf::ConditionOp, scf::YieldOp>(
          operation))
    return true;
  return isa<TrySendOp, TryRecvOp, ScheduleOp, WaitUntilOp, WaitForOp,
             AwaitEventOp, YieldSimOp, RequireOp, EnsureOp, AssertOp, ProbeOp,
             StatAddOp, InstrumentationOp>(operation);
}

std::optional<bool> constantBool(Value value) {
  Operation *definition = value.getDefiningOp();
  if (!definition)
    return std::nullopt;
  Attribute attribute = definition->getAttr("value");
  if (auto boolean = dyn_cast_or_null<BoolAttr>(attribute))
    return boolean.getValue();
  if (auto integer = dyn_cast_or_null<IntegerAttr>(attribute);
      integer && integer.getType().isInteger(1))
    return integer.getValue().getBoolValue();
  return std::nullopt;
}

LogicalResult verifySupportedSCFShape(ProcessOp process) {
  LogicalResult result = success();
  process.getBody().walk([&](Operation *operation) {
    auto requireTerminator = [&](Region &region, StringRef owner,
                                 StringRef terminator) -> Operation * {
      if (region.empty() || !llvm::hasSingleElement(region) ||
          region.front().empty() ||
          region.front().back().getName().getStringRef() != terminator) {
        operation->emitOpError()
            << "malformed " << owner << " region must terminate with "
            << terminator;
        result = failure();
        return nullptr;
      }
      return &region.front().back();
    };
    auto sameTypes = [](auto left, auto right) {
      if (left.size() != right.size())
        return false;
      return llvm::all_of(llvm::zip(left, right), [](auto pair) {
        return std::get<0>(pair).getType() == std::get<1>(pair).getType();
      });
    };
    auto emitArityError = [&](StringRef owner) {
      operation->emitOpError()
          << "malformed " << owner
          << " operand/result/block argument/yield arity or type mismatch";
      result = failure();
      return WalkResult::interrupt();
    };
    if (isa<scf::IfOp>(operation)) {
      if (operation->getNumRegions() != 2) {
        operation->emitOpError(
            "malformed scf.if region must terminate with scf.yield");
        result = failure();
        return WalkResult::interrupt();
      }
      Region &thenRegion = operation->getRegion(0);
      Region &elseRegion = operation->getRegion(1);
      Operation *thenYield =
          requireTerminator(thenRegion, "scf.if", "scf.yield");
      Operation *elseYield =
          elseRegion.empty()
              ? nullptr
              : requireTerminator(elseRegion, "scf.if", "scf.yield");
      if (!thenYield || (!elseRegion.empty() && !elseYield))
        return WalkResult::interrupt();
      if (operation->getNumOperands() != 1 ||
          !operation->getOperand(0).getType().isInteger(1) ||
          !thenRegion.front().getArguments().empty() ||
          (!elseRegion.empty() && !elseRegion.front().getArguments().empty()) ||
          thenYield->getNumResults() != 0 ||
          (elseYield && elseYield->getNumResults() != 0) ||
          !sameTypes(thenYield->getOperands(), operation->getResults()) ||
          (elseRegion.empty()
               ? operation->getNumResults() != 0
               : !sameTypes(elseYield->getOperands(), operation->getResults())))
        return emitArityError("scf.if");
    } else if (isa<scf::ForOp>(operation)) {
      if (operation->getNumRegions() != 1)
        return emitArityError("scf.for");
      Region &body = operation->getRegion(0);
      Operation *yield = requireTerminator(body, "scf.for", "scf.yield");
      if (!yield)
        return WalkResult::interrupt();
      unsigned resultCount = operation->getNumResults();
      if (operation->getNumOperands() < 3 ||
          operation->getNumOperands() != resultCount + 3 ||
          body.front().getNumArguments() != resultCount + 1)
        return emitArityError("scf.for");
      Type inductionType = operation->getOperand(0).getType();
      if (yield->getNumResults() != 0 ||
          (!inductionType.isIndex() && !isa<IntegerType>(inductionType)) ||
          operation->getOperand(1).getType() != inductionType ||
          operation->getOperand(2).getType() != inductionType ||
          body.front().getArgument(0).getType() != inductionType ||
          !sameTypes(operation->getOperands().drop_front(3),
                     operation->getResults()) ||
          !sameTypes(body.front().getArguments().drop_front(),
                     operation->getResults()) ||
          !sameTypes(yield->getOperands(), operation->getResults()))
        return emitArityError("scf.for");
    } else if (isa<scf::WhileOp>(operation)) {
      if (operation->getNumRegions() != 2)
        return emitArityError("scf.while");
      Region &before = operation->getRegion(0);
      Region &after = operation->getRegion(1);
      Operation *condition =
          requireTerminator(before, "scf.while before", "scf.condition");
      Operation *yield =
          requireTerminator(after, "scf.while after", "scf.yield");
      if (!condition || !yield)
        return WalkResult::interrupt();
      if (condition->getNumResults() != 0 || yield->getNumResults() != 0 ||
          condition->getNumOperands() < 1 ||
          !condition->getOperand(0).getType().isInteger(1) ||
          !sameTypes(operation->getOperands(), before.front().getArguments()) ||
          !sameTypes(condition->getOperands().drop_front(),
                     operation->getResults()) ||
          !sameTypes(after.front().getArguments(), operation->getResults()) ||
          !sameTypes(yield->getOperands(), before.front().getArguments()))
        return emitArityError("scf.while");
    }
    return WalkResult::advance();
  });
  return result;
}

class StructuredSuspensionAnalysis {
public:
  explicit StructuredSuspensionAnalysis(ProcessOp process)
      : work(processLivenessWorkCollector) {
    buildSummaries(process.getBody());
    buildEpochs(process.getBody());
  }

  bool guaranteesSuspend(Region &region) const {
    return regionGuarantees.lookup(&region);
  }

  LogicalResult verifyLinearLiveness(ProcessOp process) const {
    LogicalResult result = success();
    process.getBody().walk([&](Block *block) {
      if (failed(result) || !reachableBlocks.contains(block))
        return failed(result) ? WalkResult::interrupt() : WalkResult::advance();
      auto verifyValue = [&](Value value,
                             uint64_t definitionEpoch) -> LogicalResult {
        if (work)
          ++work->valueVisits;
        if (!isLinearAcrossSuspension(value.getType()))
          return success();
        for (OpOperand &use : value.getUses()) {
          if (work)
            ++work->useVisits;
          if (!reachableBlocks.contains(use.getOwner()->getBlock()))
            continue;
          if (beforeEpoch.lookup(use.getOwner()) > definitionEpoch)
            return process.emitOpError()
                   << "value of type " << value.getType()
                   << " cannot remain live across suspension";
        }
        return success();
      };
      uint64_t entryEpoch = blockEntryEpoch.lookup(block);
      for (BlockArgument argument : block->getArguments())
        if (failed(verifyValue(argument, entryEpoch))) {
          result = failure();
          return WalkResult::interrupt();
        }
      for (Operation &operation : *block) {
        if (work)
          ++work->livenessOperationVisits;
        for (Value value : operation.getResults())
          if (failed(verifyValue(value, afterEpoch.lookup(&operation)))) {
            result = failure();
            return WalkResult::interrupt();
          }
      }
      return WalkResult::advance();
    });
    return result;
  }

private:
  void buildSummaries(Region &root) {
    SmallVector<std::pair<Operation *, bool>> worklist;
    for (Block &block : llvm::reverse(root))
      for (Operation &operation : llvm::reverse(block))
        worklist.emplace_back(&operation, false);
    while (!worklist.empty()) {
      auto [operation, visited] = worklist.pop_back_val();
      if (!visited) {
        worklist.emplace_back(operation, true);
        for (Region &region : llvm::reverse(operation->getRegions()))
          for (Block &block : llvm::reverse(region))
            for (Operation &nested : llvm::reverse(block))
              worklist.emplace_back(&nested, false);
        continue;
      }
      if (work)
        ++work->summaryOperationVisits;
      for (Region &region : operation->getRegions()) {
        bool may = false;
        bool guarantees = false;
        for (Block &block : region)
          for (Operation &nested : block) {
            may |= operationMaySuspend.lookup(&nested);
            guarantees |= operationGuaranteesSuspend.lookup(&nested);
          }
        regionMaySuspend.try_emplace(&region, may);
        regionGuarantees.try_emplace(&region, guarantees);
      }
      bool may = isSuspension(operation);
      bool guarantees = isSuspension(operation);
      if (auto ifOp = dyn_cast<scf::IfOp>(operation)) {
        if (std::optional<bool> condition = constantBool(ifOp.getCondition())) {
          Region &taken = operation->getRegion(*condition ? 0 : 1);
          may |= regionMaySuspend.lookup(&taken);
          guarantees |= regionGuarantees.lookup(&taken);
        } else {
          Region &thenRegion = operation->getRegion(0);
          Region &elseRegion = operation->getRegion(1);
          may |= regionMaySuspend.lookup(&thenRegion) ||
                 regionMaySuspend.lookup(&elseRegion);
          guarantees |= !elseRegion.empty() &&
                        regionGuarantees.lookup(&thenRegion) &&
                        regionGuarantees.lookup(&elseRegion);
        }
      } else {
        for (Region &region : operation->getRegions())
          may |= regionMaySuspend.lookup(&region);
      }
      operationMaySuspend.try_emplace(operation, may);
      operationGuaranteesSuspend.try_emplace(operation, guarantees);
    }
    bool may = false;
    bool guarantees = false;
    for (Block &block : root)
      for (Operation &operation : block) {
        may |= operationMaySuspend.lookup(&operation);
        guarantees |= operationGuaranteesSuspend.lookup(&operation);
      }
    regionMaySuspend.try_emplace(&root, may);
    regionGuarantees.try_emplace(&root, guarantees);
  }

  void buildEpochs(Region &root) {
    SmallVector<std::pair<Block *, uint64_t>> worklist;
    for (Block &block : root)
      worklist.emplace_back(&block, 0);
    while (!worklist.empty()) {
      auto [block, entryEpoch] = worklist.pop_back_val();
      if (!reachableBlocks.insert(block).second)
        continue;
      blockEntryEpoch.try_emplace(block, entryEpoch);
      uint64_t epoch = entryEpoch;
      for (Operation &operation : *block) {
        if (work)
          ++work->epochOperationVisits;
        beforeEpoch.try_emplace(&operation, epoch);
        SmallVector<unsigned> reachableRegions;
        if (auto ifOp = dyn_cast<scf::IfOp>(operation)) {
          if (std::optional<bool> condition = constantBool(ifOp.getCondition()))
            reachableRegions.push_back(*condition ? 0 : 1);
          else
            reachableRegions.append({0, 1});
        } else {
          for (unsigned index = 0; index < operation.getNumRegions(); ++index)
            reachableRegions.push_back(index);
        }
        for (unsigned index : llvm::reverse(reachableRegions))
          for (Block &nested : llvm::reverse(operation.getRegion(index)))
            worklist.emplace_back(&nested, epoch);
        epoch += operationMaySuspend.lookup(&operation) ? 1 : 0;
        afterEpoch.try_emplace(&operation, epoch);
      }
    }
  }

  llvm::DenseMap<Operation *, bool> operationMaySuspend;
  llvm::DenseMap<Operation *, bool> operationGuaranteesSuspend;
  llvm::DenseMap<Region *, bool> regionMaySuspend;
  llvm::DenseMap<Region *, bool> regionGuarantees;
  llvm::DenseMap<Block *, uint64_t> blockEntryEpoch;
  llvm::DenseMap<Operation *, uint64_t> beforeEpoch;
  llvm::DenseMap<Operation *, uint64_t> afterEpoch;
  llvm::DenseSet<Block *> reachableBlocks;
  detail::ProcessLivenessWork *work;
};

template <typename Callback>
WalkResult walkOperationsIterative(Region &region, Callback callback) {
  SmallVector<Operation *> worklist;
  for (Block &block : llvm::reverse(region))
    for (Operation &operation : llvm::reverse(block))
      worklist.push_back(&operation);
  while (!worklist.empty()) {
    Operation *operation = worklist.pop_back_val();
    if (callback(operation).wasInterrupted())
      return WalkResult::interrupt();
    for (Region &nested : llvm::reverse(operation->getRegions()))
      for (Block &block : llvm::reverse(nested))
        for (Operation &child : llvm::reverse(block))
          worklist.push_back(&child);
  }
  return WalkResult::advance();
}

bool isObservationConsumer(Operation *operation) {
  return isa<ObservationOpInterface>(operation) ||
         operation->getParentOfType<InstrumentationOp>();
}

SideEffects::Resource *probeResource(StringRef kind) {
  return llvm::StringSwitch<SideEffects::Resource *>(kind)
      .Case("queue", QueueStateResource::get())
      .Case("resource", ReservationStateResource::get())
      .Case("module", ModuleStateResource::get())
      .Case("storage", StorageStateResource::get())
      .Case("protocol", ProtocolStateResource::get())
      .Case("event_queue", EventQueueStateResource::get())
      .Case("external_io", ExternalIOResource::get())
      .Case("statistics", StatisticsResource::get())
      .Default(ExternalIOResource::get());
}

} // namespace

LogicalResult ProcessOp::verify() {
  if (!isa_and_nonnull<ModuleCaseOp>((*this)->getParentOp()))
    return emitOpError("must be a direct child of ac.module.case");
  if (!isStableHierarchySegment(getSymName()))
    return emitOpError(
        "symbol name must be one stable hierarchy owner segment");
  if (getKind() != "control" && getKind() != "workload" &&
      getKind() != "monitor")
    return emitOpError("kind must be 'control', 'workload', or 'monitor'");
  if (getBody().empty())
    return emitOpError("requires one non-empty body block");
  if (!llvm::equal(getBody().front().getArgumentTypes(),
                   getCaptures().getTypes()))
    return emitOpError("body arguments must exactly match capture types");
  if (!isa<YieldSimOp>(getBody().front().back()))
    return emitOpError("body must terminate with ac.yield_sim");
  if (failed(verifyProcessLowerability(getOperation())))
    return failure();

  StructuredSuspensionAnalysis suspensionAnalysis(*this);

  LogicalResult result = success();
  walkOperationsIterative(getBody(), [&](Operation *operation) {
    if (getKind() == "monitor" &&
        isa<TrySendOp, TryRecvOp, ScheduleOp, WaitForOp>(operation)) {
      operation->emitOpError(
          "monitor process cannot perform functional state effects");
      result = failure();
      return WalkResult::interrupt();
    }
    if (auto whileOp = dyn_cast<scf::WhileOp>(operation)) {
      std::optional<bool> condition =
          constantBool(whileOp.getConditionOp().getCondition());
      if (condition != false &&
          !suspensionAnalysis.guaranteesSuspend(whileOp.getBefore()) &&
          !suspensionAnalysis.guaranteesSuspend(whileOp.getAfter())) {
        operation->emitOpError(
            "every scf.while backedge must suspend or prove bounded progress");
        result = failure();
        return WalkResult::interrupt();
      }
    }
    return WalkResult::advance();
  });
  if (failed(result))
    return failure();

  if (failed(suspensionAnalysis.verifyLinearLiveness(*this)))
    return failure();
  llvm::StringSet<> instrumentationNames;
  WalkResult instrumentationResult =
      getBody().walk([&](InstrumentationOp instrumentation) {
        if (!instrumentationNames.insert(instrumentation.getSymName()).second) {
          instrumentation.emitOpError()
              << "duplicate process-local instrumentation name '"
              << instrumentation.getSymName() << "'";
          return WalkResult::interrupt();
        }
        return WalkResult::advance();
      });
  if (instrumentationResult.wasInterrupted())
    return failure();
  return success();
}

LogicalResult TrySendOp::verify() { return requireProcess(*this); }
LogicalResult TryRecvOp::verify() { return requireProcess(*this); }

LogicalResult ScheduleOp::verify() {
  if (Operation *definition = getDelay().getDefiningOp();
      definition && definition->getName().getStringRef() == "arith.constant") {
    auto value = definition->getAttrOfType<IntegerAttr>("value");
    if (value && value.getInt() < 0)
      return emitOpError("schedule delay must be non-negative");
  }
  return requireProcess(*this);
}

LogicalResult WaitUntilOp::verify() { return requireProcess(*this); }
LogicalResult WaitForOp::verify() {
  if (failed(requireProcess(*this)))
    return failure();
  if (!isa_and_nonnull<ResourceOp>(
          lookupRuntimeSymbol(*this, getResourceAttr())))
    return emitOpError() << "unresolved runtime target '" << getResource()
                         << "'";
  return success();
}
LogicalResult AwaitEventOp::verify() {
  if (failed(requireProcess(*this)))
    return failure();
  if (!isa_and_nonnull<EventQueueOp>(
          lookupRuntimeSymbol(*this, getEventQueueAttr())))
    return emitOpError() << "unresolved runtime target '" << getEventQueue()
                         << "'";
  return success();
}

LogicalResult YieldSimOp::verify() {
  ProcessOp process = enclosingProcess(*this);
  if (!process || (*this)->getParentOp() != process)
    return emitOpError("must directly terminate an ac.process body");
  if (&(*this)->getBlock()->back() != getOperation())
    return emitOpError("must be the final operation in ac.process");
  return success();
}

LogicalResult RequireOp::verify() {
  if (isa_and_nonnull<ModuleCaseOp>((*this)->getParentOp()))
    return success();
  return requireProcess(*this);
}

LogicalResult EnsureOp::verify() {
  if (isa_and_nonnull<ModuleCaseOp>((*this)->getParentOp()))
    return success();
  return requireProcess(*this);
}

LogicalResult AssertOp::verify() { return requireProcess(*this); }

LogicalResult ProbeOp::verify() {
  if (!probeResource(getKind()) ||
      !hasStringValue(getKind(),
                      {"queue", "resource", "module", "storage", "protocol",
                       "event_queue", "external_io", "statistics"}))
    return emitOpError("unsupported probe resource kind '") << getKind() << "'";
  if (failed(requireProcess(*this)))
    return failure();
  for (Operation *user : getValue().getUsers())
    if (!isObservationConsumer(user))
      return emitOpError("probe result may only feed observation operations");
  return success();
}

LogicalResult StatOp::verify() {
  if (!isa_and_nonnull<ModuleCaseOp>((*this)->getParentOp()))
    return emitOpError("must be a direct child of ac.module.case");
  if (!isStableHierarchySegment(getSymName()))
    return emitOpError(
        "symbol name must be one stable hierarchy owner segment");
  if (!hasStringValue(getKind(),
                      {"counter", "gauge", "histogram", "event_log"}))
    return emitOpError(
        "kind must be 'counter', 'gauge', 'histogram', or 'event_log'");
  return success();
}

LogicalResult StatAddOp::verify() { return requireProcess(*this); }

LogicalResult InstrumentationOp::verify() {
  if (!enclosingProcess(*this))
    return emitOpError("must be nested in ac.process");
  if (!isStableHierarchySegment(getSymName()))
    return emitOpError(
        "symbol name must be one stable hierarchy owner segment");
  LogicalResult result = success();
  walkOperationsIterative(getBody(), [&](Operation *operation) {
    if (isa<ObservationOpInterface>(operation) || isMemoryEffectFree(operation))
      if (!isa<TrySendOp, TryRecvOp, ScheduleOp, WaitUntilOp, WaitForOp,
               AwaitEventOp, YieldSimOp>(operation))
        return WalkResult::advance();
    operation->emitOpError(
        "instrumentation may contain only removable observation operations");
    result = failure();
    return WalkResult::interrupt();
  });
  return result;
}

void TrySendOp::getEffects(
    SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  if (!isa_and_nonnull<QueueOp>(lookupRuntimeSymbol(*this, getQueue())))
    return;
  StringRef leaf = runtimeSymbolLeaf(getQueue());
  addEffect(effects, *this, MemoryEffects::Read::get(), leaf, "queue",
            QueueStateResource::get());
  addEffect(effects, *this, MemoryEffects::Write::get(), leaf, "queue",
            QueueStateResource::get());
  addEffect(effects, *this, MemoryEffects::Read::get(), leaf, "protocol",
            ProtocolStateResource::get());
  addEffect(effects, *this, MemoryEffects::Write::get(), leaf, "protocol",
            ProtocolStateResource::get());
}

void TryRecvOp::getEffects(
    SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  if (!isa_and_nonnull<QueueOp>(lookupRuntimeSymbol(*this, getQueue())))
    return;
  StringRef leaf = runtimeSymbolLeaf(getQueue());
  addEffect(effects, *this, MemoryEffects::Read::get(), leaf, "queue",
            QueueStateResource::get());
  addEffect(effects, *this, MemoryEffects::Write::get(), leaf, "queue",
            QueueStateResource::get());
  addEffect(effects, *this, MemoryEffects::Read::get(), leaf, "protocol",
            ProtocolStateResource::get());
  addEffect(effects, *this, MemoryEffects::Write::get(), leaf, "protocol",
            ProtocolStateResource::get());
}

void ScheduleOp::getEffects(
    SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  if (!isa_and_nonnull<ProcessOp>(resolvedRuntimeTarget(*this, getTarget())))
    return;
  addEffect(effects, *this, MemoryEffects::Write::get(), getTarget(), "module",
            ModuleStateResource::get());
  addEffect(effects, *this, MemoryEffects::Write::get(), getTarget(),
            "event_queue", EventQueueStateResource::get());
}

void WaitUntilOp::getEffects(
    SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  addEffect(effects, *this, MemoryEffects::Read::get(), processIdentity(*this),
            "event_queue", EventQueueStateResource::get());
  addEffect(effects, *this, MemoryEffects::Write::get(), processIdentity(*this),
            "module", ModuleStateResource::get());
}

void WaitForOp::getEffects(
    SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  if (!isa_and_nonnull<ResourceOp>(resolvedRuntimeTarget(*this, getResource())))
    return;
  addEffect(effects, *this, MemoryEffects::Read::get(), getResource(),
            "resource", ReservationStateResource::get());
  addEffect(effects, *this, MemoryEffects::Write::get(), processIdentity(*this),
            "module", ModuleStateResource::get());
}

void AwaitEventOp::getEffects(
    SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  if (!isa_and_nonnull<EventQueueOp>(
          resolvedRuntimeTarget(*this, getEventQueue())))
    return;
  addEffect(effects, *this, MemoryEffects::Read::get(), getEventQueue(),
            "event_queue", EventQueueStateResource::get());
  addEffect(effects, *this, MemoryEffects::Write::get(), processIdentity(*this),
            "module", ModuleStateResource::get());
}

void YieldSimOp::getEffects(
    SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  addEffect(effects, *this, MemoryEffects::Write::get(), processIdentity(*this),
            "module", ModuleStateResource::get());
}

void RequireOp::getEffects(
    SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  addContractEffect(effects, *this);
}

void EnsureOp::getEffects(
    SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  addContractEffect(effects, *this);
}

void AssertOp::getEffects(
    SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  addContractEffect(effects, *this);
}

void ProbeOp::getEffects(
    SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  Operation *target = resolvedRuntimeTarget(*this, getTarget());
  bool matches = llvm::StringSwitch<bool>(getKind())
                     .Case("queue", isa_and_nonnull<QueueOp>(target))
                     .Case("resource", isa_and_nonnull<ResourceOp>(target))
                     .Case("module", isa_and_nonnull<ProcessOp>(target))
                     .Case("storage", isa_and_nonnull<AddressSpaceOp>(target))
                     .Case("protocol", isa_and_nonnull<QueueOp>(target))
                     .Case("event_queue", isa_and_nonnull<EventQueueOp>(target))
                     .Case("external_io", isa_and_nonnull<ProcessOp>(target))
                     .Case("statistics", isa_and_nonnull<StatOp>(target))
                     .Default(false);
  if (!matches)
    return;
  addEffect(effects, *this, MemoryEffects::Read::get(), getTarget(), getKind(),
            probeResource(getKind()));
}

void StatOp::getEffects(
    SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  addEffect(effects, *this, MemoryEffects::Write::get(), getSymName(),
            "statistics", StatisticsResource::get());
}

void StatAddOp::getEffects(
    SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  if (!isa_and_nonnull<StatOp>(resolvedRuntimeTarget(*this, getStat())))
    return;
  addEffect(effects, *this, MemoryEffects::Read::get(), getStat(), "statistics",
            StatisticsResource::get());
  addEffect(effects, *this, MemoryEffects::Write::get(), getStat(),
            "statistics", StatisticsResource::get());
}

Operation *lookupRuntimeSymbol(Operation *from, SymbolRefAttr ref) {
  if (!from || !ref)
    return nullptr;
  if (!ref.getNestedReferences().empty()) {
    if (ref.getNestedReferences().size() != 1)
      return nullptr;
    auto file = from->getParentOfType<mlir::ModuleOp>();
    if (!file)
      return nullptr;
    ModuleOp targetModule;
    for (ModuleOp candidate : file.getOps<ModuleOp>()) {
      if (candidate.getSymName() == ref.getRootReference()) {
        targetModule = candidate;
        break;
      }
    }
    if (!targetModule)
      return nullptr;
    StringRef leaf = ref.getNestedReferences().front().getValue();
    auto targetCase = dyn_cast<ModuleCaseOp>(targetModule.getBody().front().front());
    if (!targetCase)
      return nullptr;
    for (Operation &candidate : targetCase.getBody().front()) {
      if (auto name = SymbolTable::getSymbolName(&candidate);
          name && name.getValue() == leaf)
        return &candidate;
    }
    return nullptr;
  }
  if (auto owner = from->getParentOfType<ModuleOp>()) {
    StringRef name = ref.getRootReference();
    auto ownerCase = from->getParentOfType<ModuleCaseOp>();
    if (!ownerCase)
      return nullptr;
    for (Operation &candidate : ownerCase.getBody().front()) {
      if (auto symbol = SymbolTable::getSymbolName(&candidate);
          symbol && symbol.getValue() == name)
        return &candidate;
    }
  }
  return SymbolTable::lookupNearestSymbolFrom(from, ref);
}

StringRef runtimeSymbolLeaf(SymbolRefAttr ref) {
  if (!ref)
    return {};
  ArrayRef<FlatSymbolRefAttr> nested = ref.getNestedReferences();
  if (nested.empty())
    return ref.getRootReference();
  return nested.back().getValue();
}

} // namespace acir::ac

#define GET_OP_CLASSES
#include "acir/Dialect/ACIR/ACIROps.cpp.inc"
