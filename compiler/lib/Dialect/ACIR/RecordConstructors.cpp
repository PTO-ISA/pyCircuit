#include "pycircuit/Dialect/ACIR/RecordConstructors.h"
#include "ACIRSourceContracts.h"
#include "TransactionalRepresentation.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Verifier.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/StringSet.h"

using namespace mlir;
namespace acir::ac {
namespace {
StaticExprAttr literal(Builder &b, uint64_t number) {
  return detail::transactionalInteger(
      b, number, detail::transactionalOccurrence(b, "__record_constructor__"),
      detail::transactionalSpan(b));
}
FailureOr<llvm::APSInt> scalarInteger(Attribute value, Operation *site) {
  if (auto integer = dyn_cast<MathIntAttr>(value))
    return llvm::APSInt(integer.getCanonicalValue());
  if (auto flag = dyn_cast<BoolAttr>(value))
    return llvm::APSInt(llvm::APInt(1, flag.getValue()), true);
  return site->emitOpError("record default must be an Integer or Boolean");
}
class Constructors {
public:
  explicit Constructors(mlir::ModuleOp package) : hardware(package) {}
  LogicalResult validate(Type type, Attribute value, Operation *site,
                         unsigned depth = 0) {
    if (++work > 1024 * 1024 || depth > 128)
      return site->emitOpError("record constructor expansion budget exceeded");
    if (auto scalar = dyn_cast<StaticExprAttr>(value)) {
      auto bits = dyn_cast<BitsType>(type);
      if (!bits || failed(StaticExprAttr::verify(
                       [&] { return site->emitOpError(); }, scalar.getTree())))
        return site->emitOpError(
            "record default requires its declared logical type");
      if (hardware.isStaticEvaluable(scalar, {})) {
        auto number = hardware.evaluateStatic(scalar, {}, site);
        auto width = hardware.evaluateStatic(bits.getWidth(), {}, site);
        if (failed(number) || failed(width))
          return failure();
        auto integer = scalarInteger(*number, site);
        auto widthInt = dyn_cast<MathIntAttr>(*width);
        if (failed(integer) || !widthInt)
          return failure();
        const auto &n = *integer;
        llvm::APSInt w(widthInt.getCanonicalValue());
        if (w.isNegative() || w.getActiveBits() > 64 ||
            (n.isNegative() ? n.getSignificantBits() : n.getActiveBits()) >
                w.getZExtValue())
          return site->emitOpError("record default does not fit its field");
      }
      return success();
    }
    auto record = dyn_cast<DictionaryAttr>(value);
    auto kind = record ? record.getAs<StringAttr>("kind") : StringAttr();
    auto nominal = record ? record.getAs<FlatSymbolRefAttr>("nominal")
                          : FlatSymbolRefAttr();
    if (!kind || !nominal)
      return site->emitOpError("invalid record constructor default");
    if (auto enumeration = dyn_cast<EnumType>(type)) {
      if (kind.getValue() != "enum" ||
          nominal.getValue() != enumeration.getName().getValue() ||
          record.size() != 3)
        return site->emitOpError("Enum default has a different nominal type");
      auto declaration = hardware.lookupEnum(enumeration);
      auto member = record.getAs<StringAttr>("member");
      if (!declaration || !member ||
          !llvm::any_of(declaration.getMembers(), [&](Attribute raw) {
            return cast<DictionaryAttr>(raw).getAs<StringAttr>("name") ==
                   member;
          }))
        return site->emitOpError("unknown Enum default member");
      return success();
    }
    auto structure = dyn_cast<StructType>(type);
    if (!structure || kind.getValue() != "record" ||
        nominal.getValue() != structure.getName().getValue() ||
        record.size() != 3)
      return site->emitOpError("Struct default has a different nominal type");
    auto declaration = hardware.lookupStruct(structure);
    auto fields = record.getAs<DictionaryAttr>("fields");
    if (!declaration || !fields)
      return site->emitOpError("record default requires declared fields");
    return validateFields(declaration, fields, site, depth + 1);
  }
  LogicalResult validateFields(StructOp declaration, DictionaryAttr values,
                               Operation *site, unsigned depth = 0) {
    for (NamedAttribute value : values) {
      Type type;
      for (Attribute raw : declaration.getFields()) {
        auto field = cast<DictionaryAttr>(raw);
        if (field.getAs<StringAttr>("name") == value.getName())
          type = field.getAs<TypeAttr>("type").getValue();
      }
      if (!type)
        return site->emitOpError("unknown default field");
      if (failed(validate(type, value.getValue(), site, depth + 1)))
        return failure();
    }
    return success();
  }
  FailureOr<Value> materialize(OpBuilder &b, Operation *site, Type type,
                               Attribute value = {}, unsigned depth = 0) {
    if (++work > 1024 * 1024 || depth > 128)
      return site->emitOpError("record constructor expansion budget exceeded"),
             failure();
    auto make = [&](StringRef name, ValueRange inputs, TypeRange outputs,
                    ArrayRef<NamedAttribute> attrs) {
      OperationState state(site->getLoc(), name);
      state.addOperands(inputs);
      state.addTypes(outputs);
      state.addAttributes(attrs);
      if (auto origin = site->getAttr("ac.origin"))
        state.addAttribute("ac.origin", origin);
      return b.create(state);
    };
    if (isa<BitsType>(type)) {
      auto expression = value ? cast<StaticExprAttr>(value) : literal(b, 0);
      // Use the same contextual literal constraint as an explicit constructor
      // argument. Link owns representability and two's-complement lowering,
      // including when the field width still depends on static binding.
      Attribute number = expression;
      if (hardware.isStaticEvaluable(expression, {})) {
        auto evaluated = hardware.evaluateStatic(expression, {}, site);
        if (failed(evaluated))
          return failure();
        auto integer = scalarInteger(*evaluated, site);
        if (failed(integer))
          return failure();
        number = MathIntAttr::get(b.getContext(), *integer);
      }
      return make("ac.payload.pending", {}, type,
                  {b.getNamedAttr("kind", b.getStringAttr("literal")),
                   b.getNamedAttr("number", number)})
          ->getResult(0);
    }
    if (auto enumeration = dyn_cast<EnumType>(type)) {
      if (value)
        return make("ac.enum.create", {}, type,
                    {b.getNamedAttr("member",
                                    cast<DictionaryAttr>(value).get("member"))})
            ->getResult(0);
      auto definition = hardware.resolveEnum(enumeration, site);
      if (failed(definition))
        return failure();
      auto carrier =
          BitsType::get(b.getContext(), literal(b, definition->width));
      auto zero = materialize(b, site, carrier, {}, depth + 1);
      if (failed(zero))
        return failure();
      return make("ac.enum.from_bits", *zero,
                  TypeRange{type, BitsType::get(b.getContext(), literal(b, 1))},
                  {})
          ->getResult(0);
    }
    auto structure = dyn_cast<StructType>(type);
    if (!structure)
      return site->emitOpError("record zero construction requires Bits, Enum "
                               "or Struct fields"),
             failure();
    auto declaration = hardware.lookupStruct(structure);
    if (!declaration)
      return failure();
    DictionaryAttr supplied;
    DictionaryAttr defaults;
    if (value) {
      supplied = cast<DictionaryAttr>(value).getAs<DictionaryAttr>("fields");
      defaults = declaration->getAttrOfType<DictionaryAttr>("source_defaults");
    }
    SmallVector<Value> fields;
    for (Attribute raw : declaration.getFields()) {
      auto field = cast<DictionaryAttr>(raw);
      auto name = field.getAs<StringAttr>("name");
      Attribute initial =
          supplied ? supplied.get(name.getValue()) : Attribute();
      if (!initial && defaults)
        initial = defaults.get(name.getValue());
      auto result =
          materialize(b, site, field.getAs<TypeAttr>("type").getValue(),
                      initial, depth + 1);
      if (failed(result))
        return failure();
      fields.push_back(*result);
    }
    return make("ac.struct.create", fields, type, {})->getResult(0);
  }
  HardwareAnalysis hardware;
  uint64_t work = 0;
};
} // namespace
LogicalResult verifyRecordDefaults(StructOp declaration) {
  auto defaults = declaration->getAttr("source_defaults");
  if (!defaults)
    return success();
  auto values = dyn_cast<DictionaryAttr>(defaults);
  if (!values)
    return declaration.emitOpError("source_defaults must be a dictionary");
  // Declared defaults belong to the record's source scope. In particular a
  // missing name must not become a deferred reference to a caller's formals.
  // Explicit constructor arguments retain the caller's own static scope and
  // are closed by the existing instance binding/payload analysis at Link.
  if (failed(detail::verifyHardwareAttributeScope(values, declaration)))
    return failure();
  return Constructors(declaration->getParentOfType<mlir::ModuleOp>())
      .validateFields(declaration, values, declaration);
}
LogicalResult StructConstructOp::verify() {
  HardwareAnalysis hardware((*this)->getParentOfType<mlir::ModuleOp>());
  auto declaration = hardware.lookupStruct(getResult().getType());
  if (!declaration || getFieldNames().size() != getValues().size())
    return emitOpError(
        "constructor requires declared fields and matching names");
  llvm::StringSet<> names;
  for (auto [raw, value] : llvm::zip(getFieldNames(), getValues())) {
    auto name = dyn_cast<StringAttr>(raw);
    Type type;
    if (!name || !names.insert(name.getValue()).second)
      return emitOpError("constructor requires unique field names");
    for (Attribute rawField : declaration.getFields()) {
      auto field = cast<DictionaryAttr>(rawField);
      if (field.getAs<StringAttr>("name") == name)
        type = field.getAs<TypeAttr>("type").getValue();
    }
    if (!type || !areEquivalentHardwareTypes(type, value.getType()))
      return emitOpError("constructor field type mismatch");
  }
  return success();
}
LogicalResult normalizeRecordConstructors(mlir::ModuleOp package) {
  Constructors constructors(package);
  for (auto declaration : package.getOps<StructOp>())
    if (failed(verifyRecordDefaults(declaration)))
      return failure();
  SmallVector<StructConstructOp> calls;
  package.walk([&](StructConstructOp call) { calls.push_back(call); });
  for (auto call : calls) {
    if (failed(call.verify()))
      return failure();
    auto declaration =
        constructors.hardware.lookupStruct(call.getResult().getType());
    auto defaults =
        declaration->getAttrOfType<DictionaryAttr>("source_defaults");
    SmallVector<Value> fields;
    OpBuilder b(call);
    for (Attribute raw : declaration.getFields()) {
      auto field = cast<DictionaryAttr>(raw);
      auto name = field.getAs<StringAttr>("name");
      Value explicitValue;
      for (auto [supplied, value] :
           llvm::zip(call.getFieldNames(), call.getValues()))
        if (supplied == name)
          explicitValue = value;
      if (explicitValue) {
        fields.push_back(explicitValue);
        continue;
      }
      auto value = constructors.materialize(
          b, call, field.getAs<TypeAttr>("type").getValue(),
          defaults ? defaults.get(name.getValue()) : Attribute());
      if (failed(value))
        return failure();
      fields.push_back(*value);
    }
    OperationState state(call.getLoc(), "ac.struct.create");
    state.addOperands(fields);
    state.addTypes(call.getResult().getType());
    if (auto origin = call->getAttr("ac.origin"))
      state.addAttribute("ac.origin", origin);
    auto *created = b.create(state);
    call.getResult().replaceAllUsesWith(created->getResult(0));
    call.erase();
  }
  return success();
}
} // namespace acir::ac
