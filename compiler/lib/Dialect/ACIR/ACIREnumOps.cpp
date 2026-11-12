#include "ACIRSourceContracts.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysisContext.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"

using namespace mlir;

namespace acir::ac {

// Nominal declaration grammar shared with HardwareAnalysis::resolveEnum
// (B2 step-2 design r7 row: the shared helper keeps a single definition on
// the operations side, where the native EnumOp verifier needs it).
FailureOr<EnumDefinitionView> validateEnumDeclaration(EnumOp declaration,
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

LogicalResult EnumOp::verify() {
  return success(succeeded(validateEnumDeclaration(*this, *this)));
}

// The enum value-operation bound checks (member existence, carrier widths)
// run from the staged enum rows: the source-unit semantic walk in its
// symbolic-tolerant mode and HardwareAnalysis::verifyResolvedOperation at
// the binding tiers (B2 step-2 design r7 row). The native verifiers keep
// the declaration grammar above.
} // namespace acir::ac
