#include "acir/Dialect/ACIR/ACIROps.h"

#include "ACIRSourceContracts.h"
#include "ACIRStaticEvaluation.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/StringSwitch.h"

using namespace mlir;

namespace acir::ac {
namespace {

struct CalculationOwner {
  RuleOp rule;
  func::FuncOp helper;
};

FailureOr<Type> helperPhysicalType(DictionaryAttr constraint,
                                   Operation *operation) {
  auto error = [&] { return operation->emitOpError(); };
  auto kind = constraint ? constraint.getAs<StringAttr>("kind") : StringAttr();
  if (!kind)
    return error() << "helper ValueConstraint requires a kind";
  if (kind.getValue() == "mathematical_integer") {
    if (constraint.size() != 1)
      return error() << "mathematical_integer constraint has extra fields";
    return Type(MathIntType::get(operation->getContext()));
  }
  auto logical = constraint.getAs<DictionaryAttr>("type");
  if (kind.getValue() != "logical" || constraint.size() != 2 || !logical ||
      failed(detail::verifyLogicalTypeStructure(logical, error)))
    return error() << "source math helper requires a closed value constraint";
  auto logicalKind = logical.getAs<StringAttr>("kind");
  if (logicalKind.getValue() == "bool")
    return Type(IntegerType::get(operation->getContext(), 1));
  if (logicalKind.getValue() == "integer")
    return logical.getAs<TypeAttr>("storage").getValue();
  if (logicalKind.getValue() == "record")
    return Type(StructType::get(
        operation->getContext(),
        StringAttr::get(
            operation->getContext(),
            logical.getAs<FlatSymbolRefAttr>("symbol").getValue())));
  return error() << "source math helper does not support list constraints yet";
}

LogicalResult verifyUnitPhase(Operation *operation, mlir::ModuleOp &file) {
  auto enclosing = operation->getParentOfType<mlir::ModuleOp>();
  auto stage = enclosing ? enclosing->getAttrOfType<StringAttr>("ac.stage")
                         : StringAttr();
  if (!enclosing || !stage ||
      (stage.getValue() != "source" && stage.getValue() != "linked"))
    return operation->emitOpError()
           << "source math op requires the source or linked semantic phase";
  file = enclosing;
  return success();
}

LogicalResult verifyRuleOwner(Operation *operation, RuleOp rule,
                              mlir::ModuleOp file) {
  Operation *nested = operation;
  while (nested && nested->getParentOp() != rule.getOperation())
    nested = nested->getParentOp();
  if (!nested || nested->getParentRegion() != &rule.getBody())
    return operation->emitOpError()
           << "source math op must be inside the ac.rule computation body";
  auto error = [&] { return operation->emitOpError(); };
  auto owner = rule->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto unitOwner = file->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto inputs = rule->getAttrOfType<ArrayAttr>("ac.input_bindings");
  auto inputTypes = rule->getAttrOfType<ArrayAttr>("ac.input_types");
  if (failed(detail::verifySourceOwner(owner, error)) || owner != unitOwner ||
      failed(detail::verifyOccurrence(
          rule->getAttrOfType<DictionaryAttr>("ac.origin"), error)) ||
      failed(detail::verifyOccurrence(rule.getRegistrationAttr(), error)) ||
      !inputs || !inputTypes || inputs.size() != rule.getInputs().size() ||
      inputTypes.size() != rule.getInputs().size() ||
      rule.getBody().getBlocks().size() != 1)
    return error() << "source math rule owner contract is incomplete";
  return success();
}

LogicalResult verifyHelperOwner(Operation *operation, func::FuncOp helper,
                                mlir::ModuleOp file) {
  Operation *nested = operation;
  while (nested && nested->getParentOp() != helper.getOperation())
    nested = nested->getParentOp();
  if (!nested || nested->getParentRegion() != &helper.getBody())
    return operation->emitOpError()
           << "source math op must be inside an executable helper body";
  auto error = [&] { return operation->emitOpError(); };
  auto owner = helper->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto unitOwner = file->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto interfaces = file->getAttrOfType<ArrayAttr>("ac.interfaces");
  auto role = helper->getAttrOfType<StringAttr>("ac.declaration_role");
  auto kind = helper->getAttrOfType<StringAttr>("ac.helper_kind");
  auto parameters = helper->getAttrOfType<ArrayAttr>("ac.parameters");
  auto returnForm = helper->getAttrOfType<StringAttr>("ac.return_form");
  auto constraints = helper->getAttrOfType<ArrayAttr>("ac.result_constraints");
  auto checks = helper->getAttrOfType<ArrayAttr>("ac.check_templates");
  bool ownerAuthorized =
      role && ((role.getValue() == "definition" && owner == unitOwner) ||
               (role.getValue() == "import_snapshot" && interfaces &&
                llvm::is_contained(interfaces, owner)));
  if (helper.isExternal() || helper.getBody().getBlocks().size() != 1 ||
      failed(detail::verifySourceOwner(owner, error)) || !ownerAuthorized ||
      failed(detail::verifyOccurrence(
          helper->getAttrOfType<DictionaryAttr>("ac.origin"), error)) ||
      !role ||
      (role.getValue() != "definition" &&
       role.getValue() != "import_snapshot") ||
      !kind ||
      (kind.getValue() != "value" && kind.getValue() != "record_constructor") ||
      !parameters || !returnForm || !constraints || !checks ||
      helper.getFunctionType().getNumInputs() != parameters.size() + 1 ||
      !helper.getFunctionType().getInput(parameters.size()).isInteger(1) ||
      (kind.getValue() == "record_constructor" &&
       !helper->getAttrOfType<FlatSymbolRefAttr>("ac.record")))
    return error() << "source math helper owner contract is incomplete";

  llvm::StringSet<> names;
  unsigned previousBinding = 0;
  bool hasPreviousBinding = false;
  SmallVector<Type> dataInputs;
  for (auto [index, raw] : llvm::enumerate(parameters)) {
    auto parameter = dyn_cast<DictionaryAttr>(raw);
    auto name = parameter ? parameter.getAs<StringAttr>("name") : StringAttr();
    auto binding =
        parameter ? parameter.getAs<StringAttr>("binding") : StringAttr();
    auto constraint = parameter ? parameter.getAs<DictionaryAttr>("constraint")
                                : DictionaryAttr();
    auto defaultValue = parameter ? parameter.getAs<DictionaryAttr>("default")
                                  : DictionaryAttr();
    auto parameterOrigin = parameter ? parameter.getAs<DictionaryAttr>("origin")
                                     : DictionaryAttr();
    auto parameterLocation = parameter
                                 ? parameter.getAs<DictionaryAttr>("location")
                                 : DictionaryAttr();
    unsigned rank = !binding                                        ? 3
                    : binding.getValue() == "positional_only"       ? 0
                    : binding.getValue() == "positional_or_keyword" ? 1
                    : binding.getValue() == "keyword_only"          ? 2
                                                                    : 3;
    auto physical = helperPhysicalType(constraint, operation);
    if (!parameter || parameter.size() != 6 || !name ||
        !names.insert(name.getValue()).second || rank == 3 ||
        (hasPreviousBinding && rank < previousBinding) || failed(physical) ||
        failed(detail::verifyDefaultStructure(defaultValue, error)) ||
        failed(detail::verifyOccurrence(parameterOrigin, error)) ||
        failed(detail::verifySourceSpan(parameterLocation, error)) ||
        parameterLocation.getAs<StringAttr>("path") !=
            owner.getAs<StringAttr>("path"))
      return error() << "source math helper parameter[" << index
                     << "] contract is invalid";
    auto constraintKind = constraint.getAs<StringAttr>("kind");
    if (constraintKind && constraintKind.getValue() == "logical") {
      auto resolver = [&](FlatSymbolRefAttr symbol)
          -> FailureOr<detail::ResolvedRecordView> {
        return detail::resolveSourceRecord(symbol, operation, error);
      };
      if (failed(detail::verifyDefaultMatchesType(
              defaultValue, constraint.getAs<DictionaryAttr>("type"),
              detail::ExpectedTypeKind::Logical, resolver, error)))
        return error() << "source math helper parameter[" << index
                       << "] default does not match its logical type";
    }
    dataInputs.push_back(*physical);
    previousBinding = rank;
    hasPreviousBinding = true;
  }
  if (!llvm::equal(helper.getFunctionType().getInputs().drop_back(),
                   dataInputs))
    return error() << "source math helper data signature is not authoritative";

  SmallVector<Type> dataResults;
  for (auto [index, raw] : llvm::enumerate(constraints)) {
    auto physical =
        helperPhysicalType(dyn_cast<DictionaryAttr>(raw), operation);
    if (failed(physical))
      return error() << "source math helper result constraint[" << index
                     << "] is invalid";
    dataResults.push_back(*physical);
  }
  StringRef form = returnForm.getValue();
  if ((form == "none" && !dataResults.empty()) ||
      (form == "single" && dataResults.size() != 1) ||
      (form == "tuple" && dataResults.empty()) ||
      (form != "none" && form != "single" && form != "tuple") ||
      helper.getFunctionType().getNumResults() != dataResults.size() + 1 ||
      !llvm::equal(helper.getFunctionType().getResults().drop_back(),
                   dataResults) ||
      !helper.getFunctionType().getResult(dataResults.size()).isInteger(1))
    return error()
           << "source math helper result signature is not authoritative";
  if (kind.getValue() == "record_constructor") {
    auto record = helper->getAttrOfType<FlatSymbolRefAttr>("ac.record");
    auto result = dataResults.size() == 1 ? dyn_cast<StructType>(dataResults[0])
                                          : StructType();
    if (!record || !result || result.getName().getValue() != record.getValue())
      return error() << "record constructor result does not match ac.record";
  } else if (helper->hasAttr("ac.record")) {
    return error() << "value helper must not carry ac.record";
  }
  return success();
}

FailureOr<CalculationOwner> calculationOwner(Operation *operation) {
  mlir::ModuleOp file;
  if (failed(verifyUnitPhase(operation, file)))
    return failure();
  if (auto rule = operation->getParentOfType<RuleOp>()) {
    if (failed(verifyRuleOwner(operation, rule, file)))
      return failure();
    return CalculationOwner{rule, {}};
  }
  if (auto helper = operation->getParentOfType<func::FuncOp>()) {
    if (failed(verifyHelperOwner(operation, helper, file)))
      return failure();
    return CalculationOwner{{}, helper};
  }
  operation->emitOpError()
      << "source math op requires an ac.rule or source-owned helper owner";
  return failure();
}

LogicalResult verifyMathMetadata(Operation *operation) {
  if (isa<UnknownLoc>(operation->getLoc()))
    return operation->emitOpError()
           << "source math op requires a source location";
  auto origin = operation->getAttrOfType<DictionaryAttr>("ac.origin");
  if (failed(detail::verifyOccurrence(
          origin, [&] { return operation->emitOpError(); })))
    return failure();
  return success();
}

LogicalResult requireSourceMathOp(Operation *operation) {
  if (failed(calculationOwner(operation)))
    return failure();
  return verifyMathMetadata(operation);
}

LogicalResult verifyFormalCurrentProvenance(MathFromBitsOp operation,
                                            RuleOp rule, unsigned index,
                                            DictionaryAttr binding,
                                            DictionaryAttr domain) {
  auto error = [&] { return operation.emitOpError(); };
  Value handle = rule.getInputs()[index];
  auto argument = dyn_cast<BlockArgument>(handle);
  auto module = rule->getParentOfType<ModuleOp>();
  if (!argument || !module || argument.getOwner() != &module.getBody().front())
    return error() << "formal current input must use its real module port";
  auto ports = module->getAttrOfType<ArrayAttr>("ac.ports");
  if (!ports || argument.getArgNumber() >= ports.size())
    return error() << "formal current input has no matching PortSlot";
  auto port = dyn_cast<DictionaryAttr>(ports[argument.getArgNumber()]);
  auto role = port ? port.getAs<StringAttr>("role") : StringAttr();
  if (!port || port.size() != 6 || !role || role.getValue() != "current" ||
      port.getAs<StringAttr>("parameter") !=
          binding.getAs<StringAttr>("parameter") ||
      port.get("ordinal") != binding.get("ordinal") ||
      port.getAs<DictionaryAttr>("type") != domain ||
      failed(detail::verifyOccurrence(port.getAs<DictionaryAttr>("origin"),
                                      error)) ||
      failed(detail::verifySourceSpan(port.getAs<DictionaryAttr>("location"),
                                      error)))
    return error() << "formal current StateRef and PortSlot domain disagree";
  return success();
}

LogicalResult verifyOwnedStateProvenance(MathFromBitsOp operation, RuleOp rule,
                                         unsigned index, DictionaryAttr binding,
                                         DictionaryAttr domain) {
  auto error = [&] { return operation.emitOpError(); };
  DffeOp state = rule.getInputs()[index].getDefiningOp<DffeOp>();
  auto element = binding.getAs<ArrayAttr>("element");
  auto module = rule->getParentOfType<ModuleOp>();
  if (!state || state->getParentOfType<ModuleOp>() != module ||
      state->getAttrOfType<DictionaryAttr>("ac.declaration") !=
          binding.getAs<DictionaryAttr>("declaration") ||
      state->getAttrOfType<DictionaryAttr>("ac.logical_element") != domain ||
      state->getAttrOfType<DictionaryAttr>("ac.source_owner") !=
          module->getAttrOfType<DictionaryAttr>("ac.source_owner") ||
      !state->getAttrOfType<DictionaryAttr>("ac.initial_value") ||
      !state->getAttrOfType<StringAttr>("ac.domain") ||
      state->getAttrOfType<StringAttr>("ac.domain").getValue() != "default" ||
      !element || !element.empty() ||
      !state->getAttrOfType<ArrayAttr>("ac.shape") ||
      !state->getAttrOfType<ArrayAttr>("ac.shape").empty())
    return error() << "owned current input must use its real scalar DFFE";
  return success();
}

LogicalResult verifyRuleFromBitsProvenance(MathFromBitsOp operation,
                                           RuleOp rule) {
  auto argument = dyn_cast<BlockArgument>(operation.getValue());
  if (!argument || argument.getOwner() != &rule.getBody().front() ||
      argument.getArgNumber() >= rule.getInputs().size())
    return operation.emitOpError()
           << "from_bits requires the actual ac.rule current argument";
  unsigned index = argument.getArgNumber();
  auto inputTypes = rule->getAttrOfType<ArrayAttr>("ac.input_types");
  auto inputBindings = rule->getAttrOfType<ArrayAttr>("ac.input_bindings");
  auto domain = operation.getDomain();
  auto declared = dyn_cast<DictionaryAttr>(inputTypes[index]);
  auto binding = dyn_cast<DictionaryAttr>(inputBindings[index]);
  auto error = [&] { return operation.emitOpError(); };
  if (!declared || declared != domain || !binding ||
      failed(detail::verifyStateRef(binding, error)))
    return operation.emitOpError()
           << "from_bits domain must equal its rule current declaration";
  auto kind = binding.getAs<StringAttr>("kind");
  if (!kind)
    return operation.emitOpError() << "rule current StateRef has no kind";
  if (kind.getValue() == "owned")
    return verifyOwnedStateProvenance(operation, rule, index, binding, domain);
  if (kind.getValue() == "formal")
    return verifyFormalCurrentProvenance(operation, rule, index, binding,
                                         domain);
  return operation.emitOpError()
         << "from_bits rule current StateRef must be owned or formal";
}

LogicalResult verifyHelperFromBitsProvenance(MathFromBitsOp operation,
                                             func::FuncOp helper) {
  auto argument = dyn_cast<BlockArgument>(operation.getValue());
  auto parameters = helper->getAttrOfType<ArrayAttr>("ac.parameters");
  if (!argument || argument.getOwner() != &helper.getBody().front() ||
      argument.getArgNumber() >= parameters.size())
    return operation.emitOpError()
           << "from_bits requires an actual helper data parameter";
  auto parameter =
      dyn_cast<DictionaryAttr>(parameters[argument.getArgNumber()]);
  auto constraint = parameter ? parameter.getAs<DictionaryAttr>("constraint")
                              : DictionaryAttr();
  auto kind = constraint ? constraint.getAs<StringAttr>("kind") : StringAttr();
  auto type =
      constraint ? constraint.getAs<DictionaryAttr>("type") : DictionaryAttr();
  auto error = [&] { return operation.emitOpError(); };
  if (!parameter || parameter.size() != 6 || !constraint ||
      constraint.size() != 2 || !kind || kind.getValue() != "logical" ||
      !type || type != operation.getDomain() ||
      failed(detail::verifyLogicalTypeStructure(type, error)) ||
      failed(detail::verifyOccurrence(parameter.getAs<DictionaryAttr>("origin"),
                                      error)) ||
      failed(detail::verifySourceSpan(
          parameter.getAs<DictionaryAttr>("location"), error)))
    return error() << "from_bits helper parameter must carry the exact logical "
                      "integer domain";
  return success();
}

LogicalResult verifyIntegerDomain(DictionaryAttr domain, Type physical,
                                  Operation *operation) {
  auto error = [&] { return operation->emitOpError(); };
  if (!domain || failed(detail::verifyLogicalTypeStructure(domain, error)))
    return error() << "domain must be a closed LogicalType.Integer";
  auto kind = domain.getAs<StringAttr>("kind");
  auto storage = domain.getAs<TypeAttr>("storage");
  if (!kind || kind.getValue() != "integer" || !storage)
    return error() << "domain must be LogicalType.Integer";
  if (storage.getValue() != physical)
    return error() << "integer domain storage must exactly match bounded bits";
  auto integer = dyn_cast<IntegerType>(physical);
  if (!integer || !integer.isSignless() || integer.getWidth() == 0 ||
      integer.getWidth() > 64)
    return error() << "bounded source integer storage must be signless i1..i64";
  return success();
}

LogicalResult verifyCheckTemplate(DictionaryAttr check, StringRef expectedKind,
                                  Operation *operation) {
  auto error = [&] { return operation->emitOpError(); };
  if (!check || check.size() != 4)
    return error() << "check_template must contain exactly four fields";
  auto leaf = check.getAs<DictionaryAttr>("leaf");
  auto kind = check.getAs<StringAttr>("kind");
  auto obligation = check.getAs<IntegerAttr>("obligation");
  auto location = check.getAs<DictionaryAttr>("location");
  if (!leaf || failed(detail::verifySite(leaf, error)) || !kind ||
      kind.getValue() != expectedKind ||
      failed(
          detail::decodeU64(obligation, "check_template obligation", error)) ||
      !location || failed(detail::verifySourceSpan(location, error)))
    return error() << "check_template does not match the source obligation";
  return success();
}

} // namespace

LogicalResult MathConstantOp::verify() { return requireSourceMathOp(*this); }

LogicalResult MathFromBitsOp::verify() {
  auto owner = calculationOwner(*this);
  if (failed(owner) || failed(verifyMathMetadata(*this)))
    return failure();
  if (failed(verifyIntegerDomain(getDomain(), getValue().getType(), *this)))
    return failure();
  return owner->rule ? verifyRuleFromBitsProvenance(*this, owner->rule)
                     : verifyHelperFromBitsProvenance(*this, owner->helper);
}

LogicalResult MathBinaryOp::verify() {
  if (failed(requireSourceMathOp(*this)))
    return failure();
  auto operation = (*this)->getAttrOfType<StringAttr>("operator");
  if (!operation)
    return emitOpError() << "requires StringAttr 'operator'";
  if (!llvm::StringSwitch<bool>(operation.getValue())
           .Cases({"add", "and_bits"}, true)
           .Default(false))
    return emitOpError()
           << "operator '" << operation.getValue()
           << "' is not implemented by the source-math foundation";
  if ((*this)->getAttr("ac.check_template"))
    return emitOpError() << "add and and_bits must not carry a check_template";
  return success();
}

LogicalResult MathToBitsOp::verify() {
  if (failed(requireSourceMathOp(*this)))
    return failure();
  if (failed(verifyIntegerDomain(getDomain(), getResult().getType(), *this)))
    return failure();
  return verifyCheckTemplate(
      (*this)->getAttrOfType<DictionaryAttr>("ac.check_template"), "range",
      *this);
}

} // namespace acir::ac
