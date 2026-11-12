#include "pycircuit/Dialect/ACIR/TransactionalContracts.h"
#include "ACIRSourceContracts.h"
#include "pycircuit/Dialect/ACIR/SourceEffects.h"
#include "pycircuit/Dialect/ACIR/SourceIdentifier.h"
#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/DenseSet.h"
using namespace mlir;
namespace acir::ac {
namespace {
bool hardware(Type t) {
  return isa<BitsType, StructType, EnumType, TypeParamType, TableType>(t);
}
bool equivalentTypes(TypeRange lhs, TypeRange rhs) {
  if (lhs.size() != rhs.size())
    return false;
  for (auto [left, right] : llvm::zip(lhs, rhs))
    if (!areEquivalentHardwareTypes(left, right))
      return false;
  return true;
}
} // namespace
namespace detail {
bool isCombinationalHardwareOperation(Operation *operation) {
  return isa<BitsConstantOp, BitsConstant4Op, BitsUnaryOp, BitsBinaryOp,
             BitsCompareOp, BitsSelectOp, BitsConcatOp, BitsExtractOp,
             BitsResizeOp, StructCreateOp, StructGetOp, EnumCreateOp,
             EnumToBitsOp, EnumFromBitsOp, TableCreateOp, TableSplatOp,
             TableMapOp, TableViewOp, TableIndexOp, TableReadOp, TableGetOp, TableMatchOp,
             TableChooseOp, TableFoldOp, ValueMergeOp, ValueRequireOp>(
      operation);
}

LogicalResult verifyModuleSignature(Operation *op) {
  if (op->hasAttr("output_kinds"))
    return op->emitOpError("output_kinds requires the transactional version 2 contract");
  if (!isa<mlir::ModuleOp>(op->getParentOp()))
    return op->emitOpError() << "module declaration requires package placement";
  auto error = [&] { return op->emitOpError(); };
  if (failed(detail::verifySourceOwner(
          op->getAttrOfType<DictionaryAttr>("source_owner"), error)))
    return failure();
  auto type = op->getAttrOfType<TypeAttr>("function_type");
  auto sig = type ? dyn_cast<FunctionType>(type.getValue()) : FunctionType();
  auto inputs = op->getAttrOfType<ArrayAttr>("input_names"),
       outputs = op->getAttrOfType<ArrayAttr>("output_names");
  if (!sig || !inputs || !outputs || inputs.size() != sig.getNumInputs() ||
      outputs.size() != sig.getNumResults())
    return error() << "port name counts must match function signature";
  auto rootKind = op->getAttrOfType<StringAttr>("ac.root_kind");
  if (rootKind && rootKind.getValue() != "system")
    return error() << "ac.root_kind must be 'system' when present";
  if (failed(getSystemSamples(op)))
    return failure();
  llvm::StringSet<> names;
  for (ArrayAttr list : {inputs, outputs})
    for (Attribute raw : list) {
      auto name = dyn_cast<StringAttr>(raw);
      if (!name || name.getValue().empty() || name.getValue().contains('\0') ||
          !names.insert(name.getValue()).second)
        return error() << "port names must be nonempty and unique";
    }
  auto parameters = op->getAttrOfType<ArrayAttr>("parameters"),
       types = op->getAttrOfType<ArrayAttr>("type_parameters");
  if (!parameters || !types)
    return error() << "module requires parameter inventories";
  // The closed-zero-result system shape check moved to the staged module
  // declaration entry with the port layout (r3 ACIRModuleOps row).
  names.clear();
  bool defaultSeen = false;
  for (Attribute raw : parameters) {
    auto p = dyn_cast<DictionaryAttr>(raw);
    auto name = p ? p.getAs<StringAttr>("name") : StringAttr();
    auto type = p ? p.getAs<TypeAttr>("type") : TypeAttr();
    Attribute def = p ? p.get("default") : Attribute();
    if (!p || (p.size() != 2 && p.size() != 3) || !name ||
        name.getValue().empty() || !names.insert(name.getValue()).second ||
        !type || !isa<MathIntType>(type.getValue()) ||
        (def && !isa<StaticExprAttr>(def)) || (!def && defaultSeen))
      return error() << "integer parameters require unique name, math_int type "
                        "and trailing StaticExpr defaults";
    defaultSeen |= bool(def);
  }
  for (Attribute raw : types) {
    auto name = dyn_cast<StringAttr>(raw);
    if (!name || name.getValue().empty() ||
        !names.insert(name.getValue()).second)
      return error()
             << "type parameter names must be unique across all formals";
  }
  for (Type t : sig.getInputs())
    if (failed(detail::verifyHardwareTypeScope(t, op)))
      return failure();
  for (Type t : sig.getResults())
    if (failed(detail::verifyHardwareTypeScope(t, op)))
      return failure();
  for (auto raw : parameters)
    if (failed(detail::verifyHardwareAttributeScope(
            cast<DictionaryAttr>(raw).get("default"), op)))
      return failure();
  for (Type t : sig.getInputs())
    if (!hardware(t))
      return error() << "input must have hardware payload type";
  for (Type t : sig.getResults())
    if (!hardware(t))
      return error() << "output must have hardware payload type";
  if (failed(verifySourceReferenceParameters(op))) return failure();
  // The source call contract and the remaining bound declaration checks run
  // from the staged module declaration entry (r3 ACIRModuleOps row).
  return success();
}
} // namespace detail
namespace {
LogicalResult body(Operation *op, TypeRange inputs, TypeRange outputs) {
  auto &region = op->getRegion(0);
  if (!region.hasOneBlock())
    return op->emitOpError() << "requires one graph block";
  auto &block = region.front();
  if (!equivalentTypes(block.getArgumentTypes(), inputs) || block.empty() ||
      !isa<YieldOp>(block.back()))
    return op->emitOpError()
           << "block arguments or final ac.yield do not match signature";
  if (!equivalentTypes(block.back().getOperandTypes(), outputs))
    return op->emitOpError() << "yield types do not match signature";
  for (Operation &nested : block.without_terminator()) {
    if (isa<YieldOp>(nested))
      return op->emitOpError() << "early yield is invalid";
    if (isa<RuleOp>(op) && !detail::isCombinationalHardwareOperation(&nested))
      return nested.emitOpError()
             << "rule accepts only closed combinational hardware operations";
  }
  return success();
}
} // namespace
LogicalResult ModuleOp::verify() {
  if (isTransactionalPackage(*this)) return verifyTransactionalOperation(*this);
  if ((*this)->hasAttr("primitive_kind"))
    return emitOpError()
           << "primitive_kind belongs only to trusted leaf imports";
  return detail::verifyModuleSignature(*this);
}
LogicalResult ModuleOp::verifyRegions() {
  if (isTransactionalPackage(*this)) return verifyTransactionalRegions(*this);
  if (failed(body(*this, getFunctionType().getInputs(),
                  getFunctionType().getResults())))
    return failure();
  if (failed(detail::verifyModuleObservationIdentities(*this)))
    return failure();
  // Scope authority is this exact immutable lexical definition. Repeated
  // operands share interned types, so validate each type root once
  // here; standalone queries and every other definition retain their own proof.
  llvm::DenseSet<Type> checkedTypes;
  auto typeScope = [&](Type type) -> LogicalResult {
    if (checkedTypes.contains(type)) return success();
    if (failed(detail::verifyHardwareTypeScope(type, *this))) return failure();
    checkedTypes.insert(type);
    return success();
  };
  auto result = walk([&](Operation *op) {
    for (Type type : op->getOperandTypes())
      if (failed(typeScope(type)))
        return WalkResult::interrupt();
    for (Type type : op->getResultTypes())
      if (failed(typeScope(type)))
        return WalkResult::interrupt();
    for (auto attr : op->getAttrs())
      if (failed(detail::verifyHardwareAttributeScope(attr.getValue(), *this)))
        return WalkResult::interrupt();
    return WalkResult::advance();
  });
  if (result.wasInterrupted())
    return failure();
  // Per-definition source checks run through the staged module declaration
  // entry from the preflight catalog and the source walk (r9 §5); the
  // native region verifier keeps the local scope walk above.
  return success();
}
LogicalResult RuleOp::verify() {
  if (!isa<ModuleOp>((*this)->getParentOp()) || getName().empty())
    return emitOpError() << "rule requires named direct module placement";
  return detail::verifyOccurrence(getOccurrence(),
                                  [&] { return emitOpError(); });
}
LogicalResult RuleOp::verifyRegions() {
  return body(*this, getOperandTypes(), getResultTypes());
}
LogicalResult InstanceOp::verify() {
  if (isTransactionalPackage(*this)) return verifyTransactionalOperation(*this);
  if ((*this)->hasAttr("bound_count")) return emitOpError("bound_count requires transactional version 2");
  for (Type type : getOperandTypes())
    if (!hardware(type)) return emitOpError("legacy instances require hardware payloads");
  for (Type type : getResultTypes())
    if (!hardware(type)) return emitOpError("legacy instances require hardware payloads");
  auto parent = dyn_cast_or_null<ModuleOp>((*this)->getParentOp());
  if (!parent || getInstanceName().empty())
    return emitOpError() << "instance requires named direct module placement";
  for (Operation &other : parent.getBody().front())
    if (&other != getOperation() &&
        isa<InstanceOp, CollectionOp, QueueOp>(other) &&
        other.getAttrOfType<StringAttr>("instance_name") ==
            getInstanceNameAttr())
      return emitOpError() << "instance_name must be unique within parent";
  return detail::verifyOccurrence(getOccurrence(),
                                  [&] { return emitOpError(); });
}
LogicalResult InstanceOp::verifySymbolUses(SymbolTableCollection &) {
  if (isTransactionalPackage(*this)) return verifyTransactionalOperation(*this);
  // Callee resolution, occurrence binding and payload contracts run from
  // the staged module declaration entry (r3 ACIRModuleOps row).
  return success();
}
LogicalResult YieldOp::verify() {
  if (isTransactionalPackage(*this)) return verifyTransactionalOperation(*this);
  for (Type type : getOperandTypes())
    if (!hardware(type)) return emitOpError("legacy yield requires hardware payloads");
  if (!isa_and_nonnull<ModuleOp, RuleOp, TableMapOp, TableMatchOp,
                       SourceMethodOp, SourceTransactionOp, SourceBranchOp>(
          (*this)->getParentOp()))
    return emitOpError() << "must terminate a module, rule, table region, "
                            "Method, transaction or source branch";
  return success();
}
LogicalResult ModuleImportOp::verify() {
  if (isTransactionalPackage(*this)) return verifyTransactionalOperation(*this);
  if (failed(detail::verifyModuleSignature(*this)) ||
      failed(detail::verifyModuleImportSummarySelection(*this)))
    return failure();
  if ((*this)->hasAttr("cpp_binding") || (*this)->hasAttr("rtl_binding"))
    return emitOpError()
           << "backend bindings are outside hardware module semantics";
  auto primitive = getPrimitiveKindAttr();
  if (primitive &&
      !llvm::is_contained(ArrayRef<StringRef>{"dff", "dffe", "sync_mem",
                                              "sync_mem_dp", "byte_mem",
                                              "fifo_local", "fifo_downstream_pop"},
                          primitive.getValue()))
    return emitOpError() << "unknown trusted primitive kind";
  // The transaction-summary structure and import schema checks run from the
  // staged module declaration entry (r3 ACIRModuleOps row).
  return success();
}
LogicalResult SourceObserveOp::verify() {
  return detail::verifySourceObservationStructure(*this);
}

// Module reference formal grammar (declared in SourceMethodAnalysis.h; the
// native verifyModuleSignature walk and the staged method analysis share it;
// defined here so the operations archive stays self-sufficient).
LogicalResult verifySourceReferenceParameters(Operation *module) {
  Attribute raw = module->getAttr("ac.reference_parameters");
  if (!raw) return success();
  auto refs = dyn_cast<ArrayAttr>(raw);
  if (!refs || refs.empty())
    return module->emitOpError() << "ac.reference_parameters requires a nonempty formal array";
  if (module->hasAttr("ac.root_kind"))
    return module->emitOpError() << "closed System cannot declare Module reference formals";
  if (isa<ModuleImportOp>(module)) {
    auto semantic = module->getAttrOfType<StringAttr>("semantic_kind");
    if (!semantic || semantic.getValue() != "transactions")
      return module->emitOpError() << "Module reference imports require transaction schema authority";
  }
  auto data = module->getAttrOfType<ArrayAttr>("ac.parameters");
  if (!data) return module->emitOpError() << "Module reference formals require a complete source call contract";
  unsigned count = refs.size() + (data ? data.size() : 0);
  llvm::StringSet<> names;
  for (StringRef field : {"input_names", "output_names", "type_parameters"})
    if (auto values = module->getAttrOfType<ArrayAttr>(field))
      for (Attribute value : values) if (auto name = dyn_cast<StringAttr>(value)) names.insert(name.getValue());
  if (auto values = module->getAttrOfType<ArrayAttr>("parameters"))
    for (Attribute value : values) {
      auto row = dyn_cast<DictionaryAttr>(value);
      auto name = row ? row.getAs<StringAttr>("name") : StringAttr();
      if (!name) return module->emitOpError() << "invalid static parameter inventory";
      names.insert(name.getValue());
    }
  llvm::DenseSet<int64_t> positions;
  int64_t previous = -1;
  for (Attribute item : refs) {
    auto row = dyn_cast<DictionaryAttr>(item);
    auto name = row ? row.getAs<StringAttr>("name") : StringAttr();
    auto callee = row ? row.getAs<FlatSymbolRefAttr>("callee") : FlatSymbolRefAttr();
    auto position = row ? row.getAs<IntegerAttr>("position") : IntegerAttr();
    if (!row || row.size() != 3 || !name || !callee || callee.getValue().empty() ||
        !position || !position.getType().isSignlessInteger(64) || position.getInt() < 0 ||
        static_cast<uint64_t>(position.getInt()) >= count || position.getInt() <= previous ||
        !positions.insert(position.getInt()).second || !names.insert(name.getValue()).second ||
        failed(detail::verifyPythonAstIdentifier(name.getValue(), true, [&] { return module->emitOpError(); })))
      return module->emitOpError() << "invalid, duplicate or unordered Module reference formal";
    previous = position.getInt();
  }
  return success();
}

// ---------------------------------------------------------------------------
// Declaration source-owner resolution (declared in ACIRSourceContracts.h;
// the namespace grammar, the staged unit validation and the compiler layer
// share it; defined here because it resolves the module/import accessors).
namespace detail {
DictionaryAttr declarationSourceOwner(Operation *operation) {
  if (auto module = dyn_cast_or_null<ModuleOp>(operation))
    return module.getSourceOwner();
  if (auto imported = dyn_cast_or_null<ModuleImportOp>(operation))
    return imported.getSourceOwner();
  return operation ? operation->getAttrOfType<DictionaryAttr>("ac.source_owner")
                   : DictionaryAttr();
}
} // namespace detail

// Observation grammar (B2 step-2 design r7 row): the native verifiers keep
// structure/spec/identity; the source-placement path-width predicate runs
// from the staged dispatcher row and the check-plan tier after the ops
// archive stopped referencing the analysis target.
namespace detail {
// Only statement arms extend a callable body. Other region owners (including
// guards and pure helpers) never acquire source-effect placement by ancestry.
Operation *sourceEffectCallable(Operation *operation) {
  Region *region = operation->getParentRegion();
  Operation *parent = operation->getParentOp();
  while (auto branch = dyn_cast_or_null<SourceBranchOp>(parent)) {
    if (branch->getNumRegions() != 2)
      return nullptr;
    if (region != &branch.getThenRegion() && region != &branch.getElseRegion())
      return nullptr;
    region = branch->getParentRegion();
    parent = branch->getParentOp();
  }
  return isa_and_nonnull<SourceMethodOp, SourceTransactionOp>(parent) &&
                 parent->getNumRegions() == 2 && region == &parent->getRegion(1)
             ? parent
             : nullptr;
}
LogicalResult
verifySourceObservationStructure(SourceObserveOp observe) {
  auto error = [&] { return observe.emitOpError(); };
  auto kind = observe->getAttrOfType<StringAttr>("kind");
  auto spec = observe->getAttrOfType<DictionaryAttr>("spec");
  auto *parent = detail::sourceEffectCallable(observe);
  bool source = parent != nullptr;
  if (!source)
    parent = observe->getParentOp();
  if (source) {
    if (!kind ||
        !llvm::is_contained(ArrayRef<StringRef>{"log", "report"},
                            kind.getValue()) ||
        observe->hasAttr("ac.observation_id") ||
        observe->hasAttr("ac.check_id"))
      return error()
             << "source observation requires statement Method/transaction "
                "body placement, log/report kind and no final identity";
    if (failed(verifySourceEffectMetadata(
            observe->getAttrOfType<DictionaryAttr>("occurrence"),
            observe->getAttrOfType<DictionaryAttr>("ac.location"),
            parent->getParentOp(), error)))
      return failure();
  } else {
    if (!isa<ModuleOp>(parent) || !kind ||
        !llvm::is_contained(ArrayRef<StringRef>{"print", "log", "report"},
                            kind.getValue()))
      return error() << "observation requires module placement and known kind";
    if (failed(verifyObservationID(
            observe->getAttrOfType<DictionaryAttr>("ac.observation_id"),
            error)) ||
        failed(verifySourceSpan(
            observe->getAttrOfType<DictionaryAttr>("ac.location"), error)))
      return failure();
  }
  if (observe->getNumOperands() == 0 ||
      !isa<BitsType, TypeParamType>(observe->getOperand(0).getType()))
    return error() << "observation path requires scalar Bits";
  // The source-placement path width predicate runs from the staged
  // dispatcher row (verifySourceObservationWidths) under the caller's
  // actual bindings; module-placement observations keep no width check,
  // exactly as the native verifier before the split.
  for (auto operand : observe->getOperands().drop_front())
    if (!isa<BitsType, EnumType, TypeParamType>(operand.getType()))
      return error() << "observation value requires a scalar hardware payload";
  return verifySourceObservationSpec(kind, spec, observe->getNumOperands() - 1,
                                     error);
}

LogicalResult verifySourceObservationSpec(StringAttr kind,
                                          DictionaryAttr spec,
                                          uint64_t valueCount,
                                          EmitError error) {
  if (!kind || !spec)
    return error() << "observation requires a known kind and closed spec";
  auto text = [&](StringRef key, bool nonempty = false) {
    auto value = spec.getAs<StringAttr>(key);
    return value && (!nonempty || !value.getValue().empty());
  };
  if (kind.getValue() == "report") {
    if (spec.size() != 1 || !text("name", true) || valueCount != 1)
      return error() << "report requires a nonempty name and one scalar value";
    return success();
  }
  if (spec.size() != 3 ||
      (kind.getValue() == "print" && (!text("sep") || !text("end"))) ||
      (kind.getValue() == "log" &&
       (!text("event", true) || !text("level") ||
        !llvm::is_contained(
            ArrayRef<StringRef>{"debug", "info", "warning", "error"},
            spec.getAs<StringAttr>("level").getValue()))))
    return error() << "observation spec has invalid closed fields";
  auto items = spec.getAs<ArrayAttr>("items");
  if (!items)
    return error() << "observation items must be an array";
  uint64_t next = 0;
  for (auto raw : items) {
    auto item = dyn_cast<DictionaryAttr>(raw);
    auto itemKind = item ? item.getAs<StringAttr>("kind") : StringAttr();
    if (!item || item.size() != 2 || !itemKind)
      return error() << "observation item requires exact kind and payload";
    if (itemKind.getValue() == "literal") {
      if (!item.getAs<StringAttr>("text"))
        return error() << "literal observation item requires text";
    } else if (itemKind.getValue() == "value") {
      auto ordinal = decodeU32(item.getAs<IntegerAttr>("ordinal"),
                               "observation item ordinal", error);
      if (failed(ordinal) || *ordinal != next++)
        return error()
               << "observation items must cover values exactly in order";
    } else {
      return error() << "observation item kind must be literal or value";
    }
  }
  if (next != valueCount)
    return error() << "observation items must cover values exactly in order";
  return success();
}

LogicalResult verifyModuleObservationIdentities(Operation *rawDefinition) {
  auto definition = dyn_cast_or_null<ModuleOp>(rawDefinition);
  if (!definition)
    return failure();
  SmallVector<ObservationLocalKey, 0> keys;
  for (auto observe : definition.getBody().front().getOps<SourceObserveOp>()) {
    if (failed(verifySourceObservationStructure(observe)))
      return failure();
    auto identity = observe->getAttrOfType<DictionaryAttr>("ac.observation_id");
    ObservationLocalKey key{identity, identity.getAs<DictionaryAttr>("owner"),
                            observe.getKind() == "report"
                                ? observe.getSpec().getAs<StringAttr>("name")
                                : StringAttr()};
    for (const auto &prior : keys) {
      if (prior.identity == key.identity)
        return observe.emitOpError() << "duplicate full ObservationID";
      if (key.reportName && prior.reportName == key.reportName &&
          prior.owner == key.owner)
        return observe.emitOpError()
               << "report name must be unique within logical owner: "
               << key.reportName.getValue();
    }
    keys.push_back(key);
  }
  return success();
}
} // namespace detail
} // namespace acir::ac
