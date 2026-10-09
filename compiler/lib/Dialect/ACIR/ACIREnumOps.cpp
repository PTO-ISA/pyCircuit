#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"

using namespace mlir;

namespace acir::ac {
namespace {

FailureOr<EnumDefinitionView> validateEnum(EnumOp declaration,
                                           Operation *site) {
  auto error = [&] { return site->emitOpError(); };
  if (!isa_and_nonnull<mlir::ModuleOp>(declaration->getParentOp()))
    return error() << "enum requires package placement";
  auto name = declaration.getSymNameAttr();
  if (!name || name.getValue().empty() || name.getValue().contains('\0'))
    return error() << "enum declaration requires a nonempty nominal name";
  auto widthAttr = declaration.getWidthAttr();
  if (!widthAttr)
    return error() << "enum width must be a mathematical integer";
  llvm::APSInt width(widthAttr.getCanonicalValue());
  if (width.isNegative() || width.isZero() || width.getActiveBits() > 64)
    return error() << "enum width must be positive and fit u64";
  auto encoding = declaration.getEncodingAttr();
  if (!encoding || !llvm::is_contained(
                       ArrayRef<StringRef>{"explicit", "binary_sequential",
                                           "binary_one_hot", "gray_sequential"},
                       encoding.getValue()))
    return error() << "unknown enum encoding";
  auto members = declaration.getMembers();
  if (!members || members.empty())
    return error() << "enum requires at least one member";

  EnumDefinitionView view{width.getZExtValue(), encoding, {}};
  llvm::StringSet<> names, codes;
  for (auto [ordinal, raw] : llvm::enumerate(members)) {
    auto member = dyn_cast<DictionaryAttr>(raw);
    auto name = member ? member.getAs<StringAttr>("name") : StringAttr();
    auto code = member ? member.getAs<MathIntAttr>("code") : MathIntAttr();
    if (!member || member.size() != 2 || !name || name.getValue().empty() ||
        name.getValue().contains('\0') || !code)
      return error() << "enum members require exactly a nonempty name and "
                        "mathematical integer code";
    if (!names.insert(name.getValue()).second)
      return error() << "enum member names must be unique";
    llvm::APSInt value(code.getCanonicalValue());
    if (value.isNegative())
      return error() << "enum member code must be nonnegative";
    if (value.getActiveBits() > view.width)
      return error() << "enum member code does not fit declared width";
    if (!codes.insert(code.getCanonicalValue()).second)
      return error() << "enum member codes must be unique";

    StringRef kind = encoding.getValue();
    if (kind != "explicit") {
      // Sequential/Gray ordinals derive from the finite ordered member array.
      // Keep a sign bit beyond u64; member codes never pass through host u64.
      llvm::APInt expected(65, ordinal);
      if (kind == "gray_sequential")
        expected ^= expected.lshr(1);
      if (kind == "binary_one_hot") {
        // Use the actual code's precision so no allocation scales with the
        // declared hardware width. An absent ordinal bit cannot match.
        if (ordinal >= value.getBitWidth())
          return error() << "enum member code does not match derived encoding";
        expected = llvm::APInt(value.getBitWidth(), 1).shl(ordinal);
      }
      if (!llvm::APSInt::isSameValue(value, llvm::APSInt(expected, true)))
        return error() << "enum member code does not match derived encoding";
    }
    view.members.push_back({name, code});
  }
  return view;
}

HardwareBindings scope(Operation *operation) {
  HardwareBindings bindings;
  bindings.owner = operation->getParentOfType<ModuleOp>();
  return bindings;
}

LogicalResult verifyValue(Operation *operation) {
  auto package = operation->getParentOfType<mlir::ModuleOp>();
  if (!package)
    return operation->emitOpError() << "enum value requires package placement";
  return HardwareAnalysis(package).verifyEnumValueOperation(operation,
                                                            scope(operation));
}

} // namespace

LogicalResult EnumType::verify(llvm::function_ref<InFlightDiagnostic()> error,
                               StringAttr name) {
  if (!name || name.getValue().empty() || name.getValue().contains('\0'))
    return error() << "enum type requires a nonempty nominal name";
  return success();
}

LogicalResult EnumOp::verify() {
  return success(succeeded(validateEnum(*this, *this)));
}

LogicalResult EnumCreateOp::verify() { return verifyValue(*this); }
LogicalResult EnumToBitsOp::verify() { return verifyValue(*this); }
LogicalResult EnumFromBitsOp::verify() { return verifyValue(*this); }

EnumOp HardwareAnalysis::lookupEnum(EnumType type) const {
  return dyn_cast_or_null<EnumOp>(lookupDefinition(type.getName().getValue()));
}

FailureOr<EnumDefinitionView>
HardwareAnalysis::resolveEnum(EnumType type, Operation *site) const {
  auto declaration = lookupEnum(type);
  if (!declaration)
    return site->emitOpError() << "cannot resolve nominal enum declaration '"
                               << type.getName().getValue() << "'";
  return validateEnum(declaration, site);
}

LogicalResult
HardwareAnalysis::verifyEnumValueOperation(Operation *operation,
                                           const HardwareBindings &bindings,
                                           bool requireResolved) const {
  auto width = [&](Type type, uint64_t expected,
                   StringRef description) -> LogicalResult {
    auto bits = dyn_cast<BitsType>(type);
    if (!bits)
      return operation->emitOpError() << description << " requires bits type";
    auto resolved = resolveType(bits, bindings, operation);
    if (failed(resolved))
      return failure();
    bits = cast<BitsType>(*resolved);
    if (!isStaticEvaluable(bits.getWidth(), bindings)) {
      if (requireResolved || !operation->getParentOfType<ModuleOp>())
        return operation->emitOpError()
               << description << " requires a resolved finite width";
      // The owning module's existing scope verifier checks every reference.
      // Each closed occurrence must call this verifier with requireResolved.
      return success();
    }
    auto actual = getPackedWidth(bits, bindings, operation);
    if (failed(actual))
      return failure();
    if (*actual != expected)
      return operation->emitOpError()
             << description << " width must equal " << expected;
    return success();
  };
  auto definition = [&](Type type) -> FailureOr<EnumDefinitionView> {
    auto nominal = dyn_cast<EnumType>(type);
    if (!nominal)
      return operation->emitOpError()
             << "enum operation requires an explicit nominal enum type";
    return resolveEnum(nominal, operation);
  };

  if (auto create = dyn_cast<EnumCreateOp>(operation)) {
    auto view = definition(create.getResult().getType());
    if (failed(view))
      return failure();
    for (const EnumMember &member : view->members)
      if (member.name == create.getMemberAttr())
        return success();
    return operation->emitOpError()
           << "unknown enum member '" << create.getMember() << "'";
  }
  if (auto toBits = dyn_cast<EnumToBitsOp>(operation)) {
    auto view = definition(toBits.getInput().getType());
    if (failed(view))
      return failure();
    return width(toBits.getResult().getType(), view->width,
                 "enum.to_bits result");
  }
  if (auto fromBits = dyn_cast<EnumFromBitsOp>(operation)) {
    auto view = definition(fromBits.getValue().getType());
    if (failed(view) || failed(width(fromBits.getInput().getType(), view->width,
                                     "enum.from_bits input")))
      return failure();
    return width(fromBits.getMember().getType(), 1,
                 "enum.from_bits membership result");
  }
  return success();
}

} // namespace acir::ac
