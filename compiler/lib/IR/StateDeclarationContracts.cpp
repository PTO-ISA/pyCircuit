#include "IR/StateDeclarationContracts.h"
#include "pycircuit/Dialect/ACIR/RegisterContracts.h"
#include "pycircuit/Interfaces/LogicalStateInterfaces.h"

#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "pycircuit/Interfaces/ModuleChildInterfaces.h"
#include "pycircuit/Dialect/ACIR/ACIRTypes.h"
#include "pycircuit/Dialect/ACIR/SourceIdentifier.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir::ac::detail {
Attribute semanticAttr(Attribute attr) {
  if (auto expr = dyn_cast_or_null<StaticExprAttr>(attr))
    return semanticAttr(expr.getTree());
  if (auto dict = dyn_cast_or_null<DictionaryAttr>(attr)) {
    NamedAttrList fields;
    for (NamedAttribute field : dict)
      if (field.getName() != "origin" && field.getName() != "location")
        fields.append(field.getName(), semanticAttr(field.getValue()));
    return fields.getDictionary(attr.getContext());
  }
  if (auto array = dyn_cast_or_null<ArrayAttr>(attr)) {
    SmallVector<Attribute> fields;
    for (auto value : array)
      fields.push_back(semanticAttr(value));
    return ArrayAttr::get(attr.getContext(), fields);
  }
  return attr;
}
} // namespace acir::ac::detail

namespace acir::ac {
bool areEquivalentHardwareTypes(Type lhs, Type rhs) {
  if (lhs == rhs)
    return true;
  auto lt = dyn_cast<ac::TableType>(lhs), rt = dyn_cast<ac::TableType>(rhs);
  if (lt && rt)
    return detail::semanticAttr(lt.getShape()) ==
               detail::semanticAttr(rt.getShape()) &&
           areEquivalentHardwareTypes(lt.getElementType(), rt.getElementType());
  auto l = dyn_cast<ac::BitsType>(lhs), r = dyn_cast<ac::BitsType>(rhs);
  return l && r && detail::semanticAttr(l.getWidth()) ==
                      detail::semanticAttr(r.getWidth());
}
} // namespace acir::ac

namespace acir::ac::detail {
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

} // namespace acir::ac::detail

namespace acir::ac {

LogicalResult
verifyNamedLogicalDeclaration(Operation *declaration, StringAttr instanceName) {
  auto module =
      dyn_cast_or_null<OwningModuleOpInterface>(declaration->getParentOp());
  if (!module || !instanceName)
    return declaration->emitOpError()
           << "requires named direct module placement";
  if (failed(ac::detail::verifyPythonAstIdentifier(
          instanceName.getValue(), true,
          [&] { return declaration->emitOpError(); })))
    return failure();
  auto error = [&] { return declaration->emitOpError(); };
  auto identity = declaration->getAttrOfType<DictionaryAttr>("occurrence");
  if (failed(ac::detail::verifyOccurrence(identity, error)))
    return failure();
  auto symbol = module ? module.getOwnerSymName() : StringAttr();
  if (!symbol)
    return error() << "logical state declaration requires a named owning module";
  return ac::detail::verifyOriginDefinition(
      identity, FlatSymbolRefAttr::get(declaration->getContext(),
                                       symbol.getValue()),
      "logical state declaration", error);
}

LogicalResult verifyRoutedLogicalStatePlacement(Operation *operation) {
  auto owner = dyn_cast_or_null<OwningModuleOpInterface>(
      operation->getParentOp());
  auto package = owner ? dyn_cast_or_null<mlir::ModuleOp>(
                             owner->getParentOp())
                       : mlir::ModuleOp();
  auto stage = package ? package->getAttrOfType<StringAttr>("ac.stage")
                       : StringAttr();
  auto contract =
      package ? package->getAttrOfType<IntegerAttr>("ac.routed_contract")
              : IntegerAttr();
  if (!owner || !package || !stage || stage.getValue() != "routed" ||
      package->hasAttr("ac.unit_kind") || !contract ||
      !contract.getType().isSignlessInteger(64) ||
      (contract.getValue() != 4 && contract.getValue() != 5 && contract.getValue() != 7))
    return operation->emitOpError()
           << "requires direct module graph placement in a versioned routed "
              "package";
  return success();
}

LogicalResult verifyLiteralControlWidth(Operation *site, Value control,
                                        StringRef description) {
  auto bits = dyn_cast<ac::BitsType>(control.getType());
  if (!bits)
    return site->emitOpError() << description << " requires width-one Bits";
  auto tree = bits.getWidth().getTree();
  auto kind = tree.getAs<StringAttr>("kind");
  if (!kind || kind.getValue() != "literal")
    return success();
  auto literal = tree.getAs<DictionaryAttr>("value");
  auto literalKind = literal ? literal.getAs<StringAttr>("kind") : StringAttr();
  if (!literalKind || literalKind.getValue() != "integer")
    return success();
  auto width = literal.getAs<ac::MathIntAttr>("value");
  if (!width || llvm::APSInt(width.getCanonicalValue()) != 1)
    return site->emitOpError() << description << " requires width-one Bits";
  return success();
}

LogicalResult verifyLiteralWidthResolves(Operation *site, Type payload) {
  auto bits = dyn_cast<ac::BitsType>(payload);
  if (!bits)
    return success();
  auto tree = bits.getWidth().getTree();
  auto kind = tree.getAs<StringAttr>("kind");
  if (!kind || kind.getValue() != "literal")
    return success();
  auto literal = tree.getAs<DictionaryAttr>("value");
  auto literalKind = literal ? literal.getAs<StringAttr>("kind") : StringAttr();
  if (!literalKind || literalKind.getValue() != "integer")
    return success();
  auto width = literal.getAs<ac::MathIntAttr>("value");
  if (!width)
    return site->emitOpError()
           << "hardware static value must be a mathematical integer";
  llvm::APSInt value(width.getCanonicalValue());
  if (value.isNegative() || value.isZero() || value.getActiveBits() > 64)
    return site->emitOpError()
           << "hardware static value must be positive and fit u64";
  return success();
}

LogicalResult verifyPayloadConstraintShape(Operation *site, Attribute raw,
                                           Type payload) {
  auto value = dyn_cast_or_null<DictionaryAttr>(raw);
  auto kind = value ? value.getAs<StringAttr>("kind") : StringAttr();
  auto type = value ? value.getAs<TypeAttr>("type") : TypeAttr();
  auto sourceKind = value ? value.getAs<StringAttr>("source_kind")
                          : StringAttr();
  if (!kind || kind.getValue() != "hardware" || !type || !sourceKind ||
      !ac::areEquivalentHardwareTypes(type.getValue(), payload))
    return site->emitOpError()
           << "state payload hardware constraint disagrees with its "
              "physical type";
  StringRef source = sourceKind.getValue();
  bool logical = source == "boolean" || source == "integer";
  if (value.size() != (logical ? 4 : 3))
    return site->emitOpError()
           << "state payload hardware constraint fields are invalid";
  if (logical || source == "fixed_bits") {
    if (!isa<ac::BitsType>(payload))
      return site->emitOpError()
             << "state payload bit or logical constraint requires BitsType";
  } else if (source == "nominal") {
    if (!isa<ac::StructType, ac::EnumType>(payload))
      return site->emitOpError()
             << "state payload nominal constraint requires a struct or enum";
  } else if (source == "table") {
    auto table = dyn_cast<ac::TableType>(payload);
    if (!table || table.getShape().size() != 1 ||
        !isa<ac::BitsType, ac::StructType, ac::EnumType>(
            table.getElementType()))
      return site->emitOpError()
             << "state payload table constraint requires one dimension of "
                "bits or nominal elements";
  } else {
    return site->emitOpError()
           << "unsupported state payload constraint source kind";
  }
  return success();
}

} // namespace acir::ac

namespace acir::ac {
mlir::FailureOr<unsigned> getRegisterContract(Operation *declaration) {
  if (!declaration) return failure();
  auto *imported = declaration->getName().getStringRef() == "ac.module.import" ? declaration : nullptr;
  auto kind = imported ? imported->getAttrOfType<StringAttr>("primitive_kind") : StringAttr();
  if (!imported || !kind || (kind.getValue() != "dff" && kind.getValue() != "dffe"))
    return declaration->emitOpError("register contract requires a DFF/DFFE import");
  Attribute raw = declaration->getAttr("ac.register_contract");
  unsigned contract = 1;
  if (raw) {
    auto version = dyn_cast<IntegerAttr>(raw);
    if (!version || !version.getType().isSignlessInteger(64) || version.getInt() != 3)
      return declaration->emitOpError("register contract requires 3 : i64");
    contract = version.getInt();
  }
  SmallVector<StringRef> names{"clk", "rst"};
  if (kind.getValue() == "dffe") names.push_back("en");
  names.append({"d", "init"});
  if (contract == 3) names.push_back("reset_value");
  auto pins = imported->getAttrOfType<ArrayAttr>("input_names");
  auto outputs = imported->getAttrOfType<ArrayAttr>("output_names");
  auto rawSignature = imported->getAttrOfType<TypeAttr>("function_type");
  auto signature = rawSignature ? dyn_cast<FunctionType>(rawSignature.getValue()) : FunctionType();
  if (!pins || pins.size() != names.size() || !outputs || outputs.size() != 1 ||
      !signature || signature.getNumInputs() != names.size() || signature.getNumResults() != 1 ||
      dyn_cast<StringAttr>(outputs[0]) != StringAttr::get(declaration->getContext(), "q"))
    return declaration->emitOpError("register contract pin shape mismatch");
  for (auto [pin, name] : llvm::zip(pins, names))
    if (dyn_cast<StringAttr>(pin) != StringAttr::get(declaration->getContext(), name))
      return declaration->emitOpError("register contract pin schema mismatch");
  return contract;
}
mlir::FailureOr<RegisterPolicy> getRegisterPolicy(Operation *operation) {
  if (!operation) return failure();
  RegisterPolicy result;
  bool explicitPolicy = false;
  if (isa<LogicalStateOpInterface>(operation)) {
    auto *state = operation;
    auto package = state->getParentOfType<mlir::ModuleOp>();
    auto contract = package ? package->getAttrOfType<IntegerAttr>("ac.routed_contract") : IntegerAttr();
    explicitPolicy = contract && contract.getInt() == 7;
  } else if (isa<ModuleChildOpInterface>(operation)) {
    auto callee = operation->getAttrOfType<FlatSymbolRefAttr>("callee");
    auto package = operation->getParentOfType<mlir::ModuleOp>();
    auto *declaration = package && callee ? SymbolTable::lookupSymbolIn(package, callee) : nullptr;
    if (!declaration) return operation->emitOpError("register instance cannot resolve its package primitive declaration");
    auto version = getRegisterContract(declaration);
    if (failed(version)) return failure();
    explicitPolicy = *version == 3;
  }
  if (!explicitPolicy) {
    for (StringRef name : {"edge", "reset_kind", "reset_polarity", "init_kind"})
      if (operation->hasAttr(name))
        return operation->emitOpError("state policy requires the current register contract");
    return result;
  }
  auto string = [&](StringRef name) { return operation->getAttrOfType<StringAttr>(name); };
  auto edge = string("edge"), kind = string("reset_kind"), polarity = string("reset_polarity");
  if (!edge || (edge.getValue() != "posedge" && edge.getValue() != "negedge") ||
      !kind || (kind.getValue() != "sync" && kind.getValue() != "async" && kind.getValue() != "none"))
    return operation->emitOpError("register requires explicit edge and reset kind");
  result.edge = edge.getValue() == "posedge" ? RegisterEdge::Posedge : RegisterEdge::Negedge;
  result.resetKind = kind.getValue() == "none" ? RegisterResetKind::None :
      kind.getValue() == "async" ? RegisterResetKind::Async : RegisterResetKind::Sync;
  if (result.resetKind == RegisterResetKind::None) {
    if (operation->hasAttr("reset_polarity")) return operation->emitOpError("reset-free state forbids reset polarity");
  } else {
    if (!polarity || (polarity.getValue() != "high" && polarity.getValue() != "low"))
      return operation->emitOpError("register requires explicit reset polarity");
    result.resetPolarity = polarity.getValue() == "high" ? RegisterResetPolarity::High : RegisterResetPolarity::Low;
  }
  auto init = string("init_kind");
  if (init) {
    if (init.getValue() != "explicit" && init.getValue() != "unspecified")
      return operation->emitOpError("invalid register initialization kind");
    result.hasInitial = init.getValue() == "explicit";
  } else if (isa<ModuleChildOpInterface>(operation) || operation->hasAttr("operandSegmentSizes"))
    return operation->emitOpError("register requires explicit initialization kind");
  return result;
}
mlir::FailureOr<RegisterPrimitiveLayout> getRegisterPrimitiveLayout(Operation *declaration) {
  auto version = getRegisterContract(declaration);
  if (failed(version)) return failure();
  bool enabled = declaration->getAttrOfType<StringAttr>("primitive_kind").getValue() == "dffe";
  unsigned data = enabled ? 3 : 2;
  return RegisterPrimitiveLayout{*version, 0, 1, enabled ? std::optional<unsigned>(2) : std::nullopt,
                                 data, data + 1, *version == 3 ? data + 2 : data + 1};
}
}
