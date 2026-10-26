#include "ACIRSourceContracts.h"
#include "HardwareSourceChecks.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceEffects.h"
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

LogicalResult verifyHardwareTypeScope(Type type, Operation *owner);
LogicalResult verifyHardwareAttributeScope(Attribute attr, Operation *owner) {
  // A closed Occurrence contains only sites and static values, never hardware
  // parameter/type references. Its intrinsic proof supplies no owner authority.
  if (auto occurrence = dyn_cast_or_null<DictionaryAttr>(attr);
      occurrence && ((occurrence.size() == 2 &&
                      isVerifiedOccurrenceGrammar(occurrence)) ||
                     (occurrence.size() == 5 &&
                      isVerifiedSourceSpanGrammar(occurrence))))
    return success();
  if (auto expr = dyn_cast_or_null<StaticExprAttr>(attr)) {
    auto tree = expr.getTree();
    // A verified literal contains only closed StaticValue grammar: it cannot
    // carry a hardware formal or a type_width reference.
    if (auto kind = tree.getAs<StringAttr>("kind");
        kind && kind.getValue() == "literal" &&
        isVerifiedStaticExpressionGrammar(tree))
      return success();
    auto tag = tree.getAs<StringAttr>("kind");
    if (tag && tag.getValue() == "reference") {
      auto ref = tree.getAs<DictionaryAttr>("ref");
      auto kind = ref ? ref.getAs<StringAttr>("kind") : StringAttr();
      auto symbol =
          ref ? ref.getAs<FlatSymbolRefAttr>("owner") : FlatSymbolRefAttr();
      auto name = ref ? ref.getAs<StringAttr>("name") : StringAttr();
      bool declared = false;
      if (auto parameters = owner->getAttrOfType<ArrayAttr>("parameters"))
        for (auto raw : parameters)
          if (auto parameter = dyn_cast<DictionaryAttr>(raw))
            if (parameter.getAs<StringAttr>("name") == name)
              declared = true;
      if (!kind || kind.getValue() != "parameter" || !symbol ||
          symbol.getValue() != SymbolTable::getSymbolName(owner).getValue() ||
          !declared)
        return owner->emitOpError() << "hardware static reference must name an "
                                       "integer formal of its owning module";
      return success();
    }
    return verifyHardwareAttributeScope(tree, owner);
  }
  if (auto dict = dyn_cast_or_null<DictionaryAttr>(attr)) {
    for (auto field : dict)
      if (field.getName() != "origin" && field.getName() != "location" &&
          failed(verifyHardwareAttributeScope(field.getValue(), owner)))
        return failure();
  }
  if (auto array = dyn_cast_or_null<ArrayAttr>(attr))
    for (auto element : array)
      if (failed(verifyHardwareAttributeScope(element, owner)))
        return failure();
  if (auto type = dyn_cast_or_null<TypeAttr>(attr))
    return verifyHardwareTypeScope(type.getValue(), owner);
  return success();
}
LogicalResult verifyHardwareTypeScope(Type type, Operation *owner) {
  if (auto table = dyn_cast<TableType>(type)) {
    if (failed(verifyHardwareAttributeScope(table.getShape(), owner)))
      return failure();
    return verifyHardwareTypeScope(table.getElementType(), owner);
  }
  if (auto formal = dyn_cast<TypeParamType>(type)) {
    auto parameters = owner->getAttrOfType<ArrayAttr>("type_parameters");
    auto symbol = SymbolTable::getSymbolName(owner);
    if (!parameters || !symbol ||
        formal.getOwner().getValue() != symbol.getValue() ||
        !llvm::is_contained(parameters, Attribute(formal.getName())))
      return owner->emitOpError()
             << "wire type parameter must belong to its declaring module";
  }
  if (auto bits = dyn_cast<BitsType>(type))
    return verifyHardwareAttributeScope(bits.getWidth(), owner);
  return success();
}
} // namespace detail
namespace detail {
LogicalResult verifyModuleSignature(Operation *op) {
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
  auto layout = getModulePortLayout(op);
  if (failed(layout))
    return failure();
  Attribute rawRootKind = op->getAttr("ac.root_kind");
  auto rootKind = dyn_cast_or_null<StringAttr>(rawRootKind);
  if (rawRootKind && (!rootKind || rootKind.getValue() != "system"))
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
  if (rootKind &&
      (!parameters.empty() || !types.empty() || layout->sourceInputCount != 2 ||
       layout->sourceOutputCount != 0 ||
       !op->getAttrOfType<DictionaryAttr>("ac.domain_inputs")))
    return error() << "source system must be a closed zero-result definition "
                      "with one inferred clock/reset domain";
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
  return detail::verifyModuleSourceCallContract(op);
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
  if ((*this)->hasAttr("primitive_kind"))
    return emitOpError()
           << "primitive_kind belongs only to trusted leaf imports";
  return detail::verifyModuleSignature(*this);
}
LogicalResult ModuleOp::verifyRegions() {
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
  return HardwareAnalysis((*this)->getParentOfType<mlir::ModuleOp>())
      .verifyDefinitionSourceChecks(*this);
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
  HardwareAnalysis a((*this)->getParentOfType<mlir::ModuleOp>());
  HardwareBindings b;
  b.owner = (*this)->getParentOp();
  return a.verifyInstance(*this, b);
}
LogicalResult YieldOp::verify() {
  if (!isa_and_nonnull<ModuleOp, RuleOp, TableMapOp, TableMatchOp,
                       SourceMethodOp, SourceTransactionOp, SourceBranchOp>(
          (*this)->getParentOp()))
    return emitOpError() << "must terminate a module, rule, table region, "
                            "Method, transaction or source branch";
  return success();
}
LogicalResult ModuleImportOp::verify() {
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
  if (getSemanticKind() == "transactions")
    return verifySourceTransactionSummary(*this, getTransactionSummaryAttr());
  return HardwareAnalysis((*this)->getParentOfType<mlir::ModuleOp>())
      .verifyImport(*this);
}
LogicalResult SourceObserveOp::verify() {
  return detail::verifySourceObservationStructure(*this);
}
LogicalResult SourceExpectOp::verify() {
  HardwareBindings bindings;
  auto *parent = (*this)->getParentOp();
  bindings.owner = isa_and_nonnull<SourceMethodOp, SourceTransactionOp>(parent)
                       ? parent->getParentOp()
                       : parent;
  return detail::verifySourceExpectOperation(
      HardwareAnalysis((*this)->getParentOfType<mlir::ModuleOp>()), *this,
      bindings);
}
} // namespace acir::ac
