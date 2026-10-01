#include "ACIRFinalRecordUses.h"

#include "ACIRFinalContracts.h"
#include "ACIRFinalDeclarations.h"
#include "ACIRRecordSelector.h"
#include "ACIRSourceContracts.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"

#include <algorithm>

using namespace mlir;

namespace acir::ac {
namespace {

enum class RecordKind { Read, Get, Create };

struct RequiredRecord {
  RecordKind kind;
  DictionaryAttr id;
  FlatSymbolRefAttr record;
  DictionaryAttr state;
  DictionaryAttr base;
  uint32_t field = 0;
  SmallVector<DictionaryAttr> fields;
};

struct Field {
  StringAttr name;
  DictionaryAttr logical;
  Type physical;
};

bool exactAttrs(Operation *operation,
                std::initializer_list<StringRef> expected) {
  auto attrs = operation->getDiscardableAttrDictionary();
  if (attrs.size() != expected.size())
    return false;
  return llvm::all_of(expected,
                      [&](StringRef name) { return operation->hasAttr(name); });
}

LogicalResult localOccurrence(DictionaryAttr occurrence, RuleOp rule,
                              Operation *owner);

LogicalResult proofScope(RuleOp rule) {
  auto emit = [&] { return rule.emitOpError(); };
  auto scope = rule->getAttrOfType<DictionaryAttr>("ac.proof_scope");
  auto module = rule->getParentOfType<ModuleOp>();
  if (!scope || failed(detail::verifyProofScope(scope, emit)) ||
      scope.getAs<DictionaryAttr>("registration") !=
          rule.getRegistrationAttr() ||
      !module)
    return emit() << "record value proof scope is incomplete";
  auto specialization = scope.getAs<DictionaryAttr>("specialization");
  auto definition = specialization
                        ? specialization.getAs<FlatSymbolRefAttr>("definition")
                        : FlatSymbolRefAttr();
  auto arguments = specialization ? specialization.getAs<ArrayAttr>("arguments")
                                  : ArrayAttr();
  if (!definition || definition.getValue() != module.getSymName() ||
      !arguments || !arguments.empty())
    return emit() << "record value proof scope must name its empty SpecKey";
  return success();
}

LogicalResult ruleMetadata(RuleOp rule) {
  auto emit = [&] { return rule.emitOpError(); };
  static constexpr StringLiteral allowed[] = {"name",
                                              "registration",
                                              "operandSegmentSizes",
                                              "ac.origin",
                                              "ac.source_owner",
                                              "ac.input_bindings",
                                              "ac.output_bindings",
                                              "ac.input_types",
                                              "ac.output_types",
                                              "ac.required_checks",
                                              "ac.required_observations",
                                              "ac.required_records",
                                              "ac.required_uses",
                                              "ac.yield_bindings",
                                              "ac.proof_scope",
                                              "ac.rule_kind",
                                              "ac.domain"};
  auto ruleKind = rule->getAttrOfType<StringAttr>("ac.rule_kind");
  auto domain = rule->getAttrOfType<StringAttr>("ac.domain");
  if (rule.getName().empty() ||
      failed(detail::verifyOccurrence(rule.getRegistrationAttr(), emit)) ||
      failed(localOccurrence(rule.getRegistrationAttr(), rule, rule)) ||
      llvm::any_of(rule->getAttrs(),
                   [&](NamedAttribute attribute) {
                     return !llvm::is_contained(allowed,
                                                attribute.getName().strref());
                   }) ||
      (rule->hasAttr("ac.rule_kind") &&
       (!ruleKind || ruleKind.getValue() != "function")) ||
      (rule->hasAttr("ac.domain") &&
       (!domain || domain.getValue() != "default")))
    return emit() << "S2A rule metadata contains unsupported semantics";
  auto module = rule->getParentOfType<ModuleOp>();
  auto unit = rule->getParentOfType<mlir::ModuleOp>();
  auto stage =
      unit ? unit->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
  auto unitKind =
      unit ? unit->getAttrOfType<StringAttr>("ac.unit_kind") : StringAttr();
  auto sourceOwner = rule->getAttrOfType<DictionaryAttr>("ac.source_owner");
  if (!module || !unit || !stage || stage.getValue() != "final" || !unitKind ||
      unitKind.getValue() != "implementation" ||
      sourceOwner != module->getAttrOfType<DictionaryAttr>("ac.source_owner") ||
      sourceOwner != unit->getAttrOfType<DictionaryAttr>("ac.source_owner") ||
      failed(detail::verifySourceOwner(sourceOwner, emit)) ||
      failed(localOccurrence(rule->getAttrOfType<DictionaryAttr>("ac.origin"),
                             rule, rule)))
    return emit() << "S2A rule provenance is not source-module local";
  return success();
}

LogicalResult localOccurrence(DictionaryAttr occurrence, RuleOp rule,
                              Operation *owner) {
  auto emit = [&] { return owner->emitOpError(); };
  auto module = rule->getParentOfType<ModuleOp>();
  auto sourceOwner =
      module ? module->getAttrOfType<DictionaryAttr>("ac.source_owner")
             : DictionaryAttr();
  auto path =
      sourceOwner ? sourceOwner.getAs<StringAttr>("path") : StringAttr();
  auto site =
      occurrence ? occurrence.getAs<DictionaryAttr>("site") : DictionaryAttr();
  auto definition =
      site ? site.getAs<FlatSymbolRefAttr>("definition") : FlatSymbolRefAttr();
  auto expansion =
      occurrence ? occurrence.getAs<ArrayAttr>("expansion") : ArrayAttr();
  auto location = dyn_cast<FileLineColLoc>(owner->getLoc());
  if (!module || !path || !site || !definition ||
      definition.getValue() != module.getSymName() || !expansion ||
      !expansion.empty() ||
      failed(detail::verifyOccurrence(occurrence, emit)) || !location ||
      location.getFilename() != path.getValue())
    return emit()
           << "record value provenance must be local to its source module";
  return success();
}

FailureOr<SmallVector<RequiredRecord>> requirements(RuleOp rule) {
  auto emit = [&] { return rule.emitOpError(); };
  auto raw = rule->getAttrOfType<ArrayAttr>("ac.required_records");
  if (!raw || raw.empty())
    return emit() << "record value requirement array must be nonempty";

  SmallVector<RequiredRecord> result;
  result.reserve(4);
  DenseSet<Attribute> ids;
  const RequiredRecord *read = nullptr, *create = nullptr;
  const RequiredRecord *gets[2] = {};
  Attribute previous;
  for (Attribute itemAttr : raw) {
    auto item = dyn_cast<DictionaryAttr>(itemAttr);
    auto kind = item ? item.getAs<StringAttr>("kind") : StringAttr();
    auto id = item ? item.getAs<DictionaryAttr>("id") : DictionaryAttr();
    auto record =
        item ? item.getAs<FlatSymbolRefAttr>("record") : FlatSymbolRefAttr();
    if (!item || !kind || !id || !record ||
        failed(detail::verifyValueID(id, emit)) || !ids.insert(id).second ||
        (previous && detail::compareClosedSourceStructure(previous, id) >= 0))
      return emit() << "record value requirements are malformed or unsorted";
    previous = id;

    if (kind.getValue() == "read") {
      auto state = item.getAs<DictionaryAttr>("state");
      if (read || item.size() != 4 || !state ||
          failed(detail::verifyStateRef(state, emit)))
        return emit() << "record read requirement is malformed";
      result.push_back({RecordKind::Read, id, record, state, {}, 0, {}});
      read = &result.back();
      continue;
    }
    if (kind.getValue() == "get") {
      auto base = item.getAs<DictionaryAttr>("base");
      auto field = detail::decodeU32(item.getAs<IntegerAttr>("field"),
                                     "record get field ordinal", emit);
      if (item.size() != 5 || !base || failed(field) || *field > 1 ||
          gets[*field] || failed(detail::verifyValueID(base, emit)))
        return emit() << "record get requirement is malformed";
      result.push_back({RecordKind::Get, id, record, {}, base, *field, {}});
      gets[*field] = &result.back();
      continue;
    }
    if (kind.getValue() == "create") {
      auto fields = item.getAs<ArrayAttr>("fields");
      if (create || item.size() != 4 || !fields || fields.size() != 2)
        return emit() << "record create requirement is malformed";
      SmallVector<DictionaryAttr> fieldIDs;
      for (Attribute field : fields) {
        auto fieldID = dyn_cast<DictionaryAttr>(field);
        if (!fieldID || failed(detail::verifyValueID(fieldID, emit)))
          return emit() << "record create field identity is malformed";
        fieldIDs.push_back(fieldID);
      }
      result.push_back(
          {RecordKind::Create, id, record, {}, {}, 0, std::move(fieldIDs)});
      create = &result.back();
      continue;
    }
    return emit() << "record value requirement has an unknown variant";
  }

  if (result.size() != 4 || !read || !gets[0] || !gets[1] || !create)
    return emit() << "S2A requires one record read, two gets and one create";
  if (gets[0]->base != read->id || gets[1]->base != read->id ||
      gets[0]->record != read->record || gets[1]->record != read->record ||
      create->record != read->record || create->fields[0] != gets[0]->id ||
      create->fields[1] != gets[1]->id)
    return emit() << "record requirements are not a closed read/get/create DAG";
  return result;
}

ValueBindingOp bindingFor(Block &body, DictionaryAttr id) {
  ValueBindingOp found;
  for (ValueBindingOp binding : body.getOps<ValueBindingOp>())
    if (binding.getIdAttr() == id) {
      if (found)
        return {};
      found = binding;
    }
  return found;
}

LogicalResult unsupportedEvidence(RuleOp rule, Block &body) {
  auto emit = [&] { return rule.emitOpError(); };
  auto checks = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
  auto observations =
      rule->getAttrOfType<ArrayAttr>("ac.required_observations");
  if (rule->hasAttr("ac.required_numeric") || !checks || !checks.empty() ||
      !observations || !observations.empty() ||
      !body.getOps<NumericProofOp>().empty() ||
      !body.getOps<SourceExpectOp>().empty() ||
      !body.getOps<SourceObserveOp>().empty() ||
      !body.getOps<SourceReadOp>().empty() ||
      !body.getOps<SourceUseOp>().empty())
    return emit() << "S2A record value packet rejects numeric, check, source, "
                     "helper and observation evidence";
  return success();
}

} // namespace

bool hasFinalRecordUses(RuleOp rule) {
  return rule && rule->hasAttr("ac.required_records");
}

LogicalResult verifyFinalRecordUses(RuleOp rule) {
  if (!rule)
    return failure();
  auto emit = [&] { return rule.emitOpError(); };
  if (!hasFinalRecordUses(rule) || failed(proofScope(rule)) ||
      !isa<ArrayAttr>(rule->getAttr("ac.required_records")) ||
      rule.getBody().getBlocks().size() != 1 || rule.getInputs().size() != 1 ||
      rule.getTargets().size() != 1)
    return emit() << "S2A requires one final record current input and target";
  if (failed(ruleMetadata(rule)))
    return failure();
  Block &body = rule.getBody().front();
  if (body.getNumArguments() != 1 || failed(unsupportedEvidence(rule, body)))
    return emit() << "S2A record rule body or evidence is unsupported";

  auto inputBindings = rule->getAttrOfType<ArrayAttr>("ac.input_bindings");
  auto outputBindings = rule->getAttrOfType<ArrayAttr>("ac.output_bindings");
  auto inputTypes = rule->getAttrOfType<ArrayAttr>("ac.input_types");
  auto outputTypes = rule->getAttrOfType<ArrayAttr>("ac.output_types");
  auto state = inputBindings && inputBindings.size() == 1
                   ? dyn_cast<DictionaryAttr>(inputBindings[0])
                   : DictionaryAttr();
  auto outputState = outputBindings && outputBindings.size() == 1
                         ? dyn_cast<DictionaryAttr>(outputBindings[0])
                         : DictionaryAttr();
  auto logical = inputTypes && inputTypes.size() == 1
                     ? dyn_cast<DictionaryAttr>(inputTypes[0])
                     : DictionaryAttr();
  auto outputLogical = outputTypes && outputTypes.size() == 1
                           ? dyn_cast<DictionaryAttr>(outputTypes[0])
                           : DictionaryAttr();
  auto symbol = logical ? logical.getAs<FlatSymbolRefAttr>("symbol")
                        : FlatSymbolRefAttr();
  auto record = dyn_cast<StructType>(body.getArgument(0).getType());
  if (!state || !outputState ||
      failed(
          detail::verifyStateRef(state, [&] { return rule.emitOpError(); })) ||
      failed(detail::verifyStateRef(outputState,
                                    [&] { return rule.emitOpError(); })) ||
      state != outputState ||
      state.getAs<StringAttr>("kind").getValue() != "owned" || !logical ||
      !state.getAs<ArrayAttr>("element") ||
      !state.getAs<ArrayAttr>("element").empty() ||
      failed(detail::verifyLogicalTypeStructure(
          logical, [&] { return rule.emitOpError(); })) ||
      logical != outputLogical ||
      logical.getAs<StringAttr>("kind").getValue() != "record" || !symbol ||
      !record || record.getName().getValue() != symbol.getValue() ||
      body.getArgument(0).getType() !=
          cast<RegType>(rule.getInputs()[0].getType()).getElementType() ||
      failed(verifyFinalRuleHandle(rule, rule.getInputs()[0], state, logical,
                                   "current")) ||
      failed(verifyFinalRuleHandle(rule, rule.getTargets()[0], state, logical,
                                   "next")) ||
      rule.getInputs()[0] != rule.getTargets()[0])
    return emit() << "S2A record input and target must be the same owned state";

  auto required = requirements(rule);
  if (failed(required))
    return failure();
  auto module = rule->getParentOfType<ModuleOp>();
  auto resolved = final_detail::resolveFinalRecordDeclaration(
      rule, symbol, [&] { return rule.emitOpError(); });
  if (failed(resolved))
    return failure();
  auto declaration = *resolved;
  if (!declaration || declaration.getSymName() != symbol.getValue() ||
      declaration.getFields().size() != 2)
    return emit() << "S2A requires the provider-owned two-field record";
  auto unit = declaration->getParentOfType<mlir::ModuleOp>();
  auto declarationUnitKind =
      unit ? unit->getAttrOfType<StringAttr>("ac.unit_kind") : StringAttr();
  auto stage =
      unit ? unit->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
  auto owner = unit ? unit->getAttrOfType<DictionaryAttr>("ac.source_owner")
                    : DictionaryAttr();
  auto currentUnit = rule->getParentOfType<mlir::ModuleOp>();
  auto moduleOwner =
      currentUnit
          ? currentUnit->getAttrOfType<DictionaryAttr>("ac.source_owner")
          : DictionaryAttr();
  if (!unit || !currentUnit || unit == currentUnit || !declarationUnitKind ||
      (declarationUnitKind.getValue() != "implementation" &&
       declarationUnitKind.getValue() != "declarations") ||
      !stage || stage.getValue() != "final" || !owner || owner == moduleOwner)
    return emit() << "record declaration must be owned by its provider unit";

  SmallVector<Field> fields;
  for (Attribute raw : declaration.getFields()) {
    auto field = dyn_cast<DictionaryAttr>(raw);
    auto name = field ? field.getAs<StringAttr>("name") : StringAttr();
    auto fieldType =
        field ? field.getAs<DictionaryAttr>("type") : DictionaryAttr();
    auto kind = fieldType ? fieldType.getAs<StringAttr>("kind") : StringAttr();
    Type physical;
    if (kind && kind.getValue() == "bool")
      physical = IntegerType::get(rule.getContext(), 1);
    else if (kind && kind.getValue() == "integer") {
      auto storage = fieldType.getAs<TypeAttr>("storage");
      if (storage)
        physical = storage.getValue();
    }
    if (!field || field.size() != 4 || !name || !fieldType || !physical ||
        failed(record_detail::verifyRecordLeafType(fieldType, physical,
                                                   declaration)))
      return emit() << "S2A record field schema is incomplete";
    fields.push_back({name, fieldType, physical});
  }
  if (fields.size() != 2)
    return emit() << "S2A record requires exactly two fields";

  DenseMap<Attribute, ValueBindingOp> bindings;
  for (ValueBindingOp binding : body.getOps<ValueBindingOp>())
    if (binding->getAttrs().size() != 2 ||
        !binding->getDiscardableAttrDictionary().empty() ||
        failed(detail::verifyValueID(binding.getIdAttr(),
                                     [&] { return binding.emitOpError(); })) ||
        failed(
            localOccurrence(binding.getIdAttr().getAs<DictionaryAttr>("origin"),
                            rule, binding)) ||
        !bindings.try_emplace(binding.getIdAttr(), binding).second)
      return binding.emitOpError()
             << "record value binding identity or attributes are invalid";
  if (bindings.size() != required->size())
    return emit() << "S2A record value bindings must close required records";

  const RequiredRecord *read = nullptr, *create = nullptr;
  SmallVector<const RequiredRecord *> gets(2, nullptr);
  for (const RequiredRecord &item : *required) {
    if (item.record != symbol)
      return emit() << "required record does not name the target nominal type";
    if (item.kind == RecordKind::Read)
      read = &item;
    else if (item.kind == RecordKind::Create)
      create = &item;
    else
      gets[item.field] = &item;
  }
  if (!read || !create || !gets[0] || !gets[1] || read->state != state ||
      create->fields[0] != gets[0]->id || create->fields[1] != gets[1]->id)
    return emit() << "S2A RequiredRecord state or field order is invalid";

  auto recordBinding = bindingFor(body, read->id);
  auto loBinding = bindingFor(body, gets[0]->id);
  auto hiBinding = bindingFor(body, gets[1]->id);
  auto createBinding = bindingFor(body, create->id);
  if (!recordBinding || !loBinding || !hiBinding || !createBinding ||
      recordBinding.getDomainAttr() != logical ||
      loBinding.getDomainAttr() != fields[0].logical ||
      hiBinding.getDomainAttr() != fields[1].logical ||
      createBinding.getDomainAttr() != logical ||
      recordBinding.getValue() != body.getArgument(0) ||
      !record_detail::isTrueI1(recordBinding.getValid()) ||
      !record_detail::isTrueI1(recordBinding.getPath()) ||
      loBinding.getValid() != recordBinding.getValid() ||
      hiBinding.getValid() != recordBinding.getValid() ||
      loBinding.getPath() != recordBinding.getPath() ||
      hiBinding.getPath() != recordBinding.getPath() ||
      !record_detail::isTrueI1(loBinding.getValid()) ||
      !record_detail::isTrueI1(hiBinding.getValid()) ||
      createBinding.getPath() != recordBinding.getPath() ||
      failed(record_detail::verifyRecordLeafType(
          fields[0].logical, loBinding.getValue().getType(), loBinding)) ||
      failed(record_detail::verifyRecordLeafType(
          fields[1].logical, hiBinding.getValue().getType(), hiBinding)))
    return emit() << "record bindings have invalid type, validity or path";

  auto get0Origin = gets[0]->id.getAs<DictionaryAttr>("origin");
  auto get1Origin = gets[1]->id.getAs<DictionaryAttr>("origin");
  auto createOrigin = create->id.getAs<DictionaryAttr>("origin");
  if (!get0Origin || !get1Origin || !createOrigin)
    return emit() << "record value ID origin is missing";

  StructGetOp sourceGets[2];
  StructCreateOp sourceCreate;
  for (StructGetOp op : body.getOps<StructGetOp>()) {
    auto origin = op->getAttrOfType<DictionaryAttr>("ac.origin");
    if (origin == get0Origin)
      sourceGets[0] = op;
    else if (origin == get1Origin)
      sourceGets[1] = op;
  }
  for (StructCreateOp op : body.getOps<StructCreateOp>())
    if (op->getAttrOfType<DictionaryAttr>("ac.origin") == createOrigin)
      sourceCreate = op;
  if (!sourceGets[0] || !sourceGets[1] || !sourceCreate)
    return emit() << "source and selector record operations are incomplete";

  for (unsigned index = 0; index != 2; ++index) {
    auto op = sourceGets[index];
    auto binding = index == 0 ? loBinding : hiBinding;
    if (!exactAttrs(op, {"ac.origin"}) || op->getNumRegions() != 0 ||
        op.getValue() != recordBinding.getValue() || !op.getFieldAttr() ||
        op.getFieldAttr().getValue() != fields[index].name.getValue() ||
        op.getResult().getType() != fields[index].physical ||
        op.getResult() != binding.getValue() ||
        op->getAttrOfType<DictionaryAttr>("ac.origin") !=
            (index == 0 ? get0Origin : get1Origin) ||
        failed(localOccurrence(op->getAttrOfType<DictionaryAttr>("ac.origin"),
                               rule, op)))
      return op.emitOpError()
             << "source record get differs from its obligation";
  }
  if (!exactAttrs(sourceCreate, {"ac.origin"}) ||
      sourceCreate->getNumRegions() != 0 ||
      sourceCreate.getValues().size() != 2 ||
      sourceCreate.getValues()[0] != loBinding.getValue() ||
      sourceCreate.getValues()[1] != hiBinding.getValue() ||
      sourceCreate.getResult().getType() !=
          recordBinding.getValue().getType() ||
      sourceCreate.getResult() != createBinding.getValue() ||
      sourceCreate->getAttrOfType<DictionaryAttr>("ac.origin") !=
          createOrigin ||
      failed(localOccurrence(createOrigin, rule, sourceCreate)))
    return sourceCreate.emitOpError()
           << "source record create differs from its ordered obligation";

  Value aggregateValid;
  auto fieldValidAnd = createBinding.getValid().getDefiningOp<arith::AndIOp>();
  if (fieldValidAnd && fieldValidAnd->getAttrs().empty() &&
      ((fieldValidAnd.getLhs() == loBinding.getValid() &&
        fieldValidAnd.getRhs() == hiBinding.getValid()) ||
       (fieldValidAnd.getLhs() == hiBinding.getValid() &&
        fieldValidAnd.getRhs() == loBinding.getValid())))
    aggregateValid = createBinding.getValid();
  else if (record_detail::isTrueI1(createBinding.getValid()) &&
           record_detail::isTrueI1(loBinding.getValid()) &&
           record_detail::isTrueI1(hiBinding.getValid()))
    aggregateValid = createBinding.getValid();
  if (!aggregateValid)
    return emit() << "record create validity is not the field-validity AND";

  auto rawUses = rule->getAttrOfType<ArrayAttr>("ac.required_uses");
  auto yieldBindings = rule->getAttrOfType<ArrayAttr>("ac.yield_bindings");
  if (!rawUses || rawUses.size() != 1 || !yieldBindings ||
      yieldBindings.size() != 1)
    return emit() << "S2A requires one RequiredUse and one YieldBinding";
  auto requiredUse = dyn_cast<DictionaryAttr>(rawUses[0]);
  auto useID =
      requiredUse ? requiredUse.getAs<DictionaryAttr>("id") : DictionaryAttr();
  auto useValue = requiredUse ? requiredUse.getAs<DictionaryAttr>("value")
                              : DictionaryAttr();
  auto target = requiredUse ? requiredUse.getAs<DictionaryAttr>("target")
                            : DictionaryAttr();
  auto useRole = useID ? useID.getAs<StringAttr>("role") : StringAttr();
  auto useSlot = detail::decodeU32(useID ? useID.getAs<IntegerAttr>("slot")
                                         : IntegerAttr(),
                                   "record next UseID slot", emit);
  auto targetKind = target ? target.getAs<StringAttr>("kind") : StringAttr();
  auto targetState =
      target ? target.getAs<DictionaryAttr>("state") : DictionaryAttr();
  if (!requiredUse || requiredUse.size() != 3 || !useID || !useValue ||
      failed(detail::verifyUseID(useID, emit)) ||
      failed(
          localOccurrence(useID.getAs<DictionaryAttr>("origin"), rule, rule)) ||
      useRole.getValue() != "next" || failed(useSlot) || *useSlot != 0 ||
      useValue != create->id || !target || target.size() != 2 || !targetKind ||
      targetKind.getValue() != "next_scalar" || targetState != state ||
      failed(detail::verifyStateRef(targetState, emit)))
    return emit() << "S2A RequiredUse does not name the record target";

  ValueUseOp use;
  for (ValueUseOp candidate : body.getOps<ValueUseOp>()) {
    if (use)
      return emit() << "S2A has more than one ValueUse";
    use = candidate;
  }
  if (!use || use.getIdAttr() != useID || use.getSourceAttr() != create->id ||
      use.getValue() != createBinding.getValue() ||
      use.getValid() != createBinding.getValid() ||
      !record_detail::isSafeUsePath(use.getPath()) ||
      use->getAttrs().size() != 2 ||
      !use->getDiscardableAttrDictionary().empty() ||
      failed(localOccurrence(useID.getAs<DictionaryAttr>("origin"), rule, use)))
    return emit() << "S2A ValueUse does not consume the source create";

  auto yieldBinding = dyn_cast<DictionaryAttr>(yieldBindings[0]);
  auto contributions = yieldBinding
                           ? yieldBinding.getAs<ArrayAttr>("contributions")
                           : ArrayAttr();
  auto contribution = contributions && contributions.size() == 1
                          ? dyn_cast<DictionaryAttr>(contributions[0])
                          : DictionaryAttr();
  auto dataOperand = detail::decodeU32(
      yieldBinding ? yieldBinding.getAs<IntegerAttr>("data_operand")
                   : IntegerAttr(),
      "record yield data operand", emit);
  auto enableOperand = detail::decodeU32(
      yieldBinding ? yieldBinding.getAs<IntegerAttr>("enable_operand")
                   : IntegerAttr(),
      "record yield enable operand", emit);
  if (!yieldBinding || yieldBinding.size() != 4 || failed(dataOperand) ||
      *dataOperand != 0 || failed(enableOperand) || *enableOperand != 1 ||
      yieldBinding.getAs<DictionaryAttr>("target") != state || !contribution ||
      contribution.size() != 2 ||
      contribution.getAs<DictionaryAttr>("use") != useID ||
      !contribution.getAs<UnitAttr>("selection_ordinal"))
    return emit() << "S2A YieldBinding is stale or redirected";
  auto yield = dyn_cast_or_null<YieldOp>(body.getTerminator());
  if (!yield || yield.getValues().size() != 2 ||
      rule.getNextValues().size() != 2)
    return emit() << "S2A rule yield arity is invalid";

  SmallVector<record_detail::SelectorField> selectorFields;
  for (const Field &field : fields)
    selectorFields.push_back({field.name, field.physical});
  SmallVector<StructGetOp> sourceGetList{sourceGets[0], sourceGets[1]};
  auto selector = record_detail::verifyFinalRecordSelector(
      rule, createBinding.getValue(), use.getPath(), use.getValid(),
      recordBinding.getValue().getType(), selectorFields, sourceGetList,
      sourceCreate, fieldValidAnd);
  if (failed(selector))
    return failure();
  if (selector->value.getResult().getType() !=
          cast<RegType>(rule.getTargets()[0].getType()).getElementType() ||
      !selector->enable.getResult().getType().isInteger(1) ||
      rule.getNextValues().size() != 2 ||
      rule.getNextValues()[0].getType() !=
          selector->value.getResult().getType() ||
      rule.getNextValues()[1].getType() !=
          selector->enable.getResult().getType())
    return emit() << "S2A yield or field validity is outside the exact closure";
  return success();
}

} // namespace acir::ac
