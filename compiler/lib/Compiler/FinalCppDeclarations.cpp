#include "FinalCppDeclarations.h"

#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/Twine.h"

using namespace mlir;

namespace acir::compiler {

FailureOr<std::string>
emitCppValueDeclaration(Operation *operation, StringRef cppName,
                         StringRef ownerText, ac::detail::EmitError emitError) {
  if (operation->getName().getStringRef() == "ac.type_alias") {
    auto target = operation->getAttrOfType<DictionaryAttr>("target");
    auto kind = target ? target.getAs<StringAttr>("kind") : StringAttr();
    if (!kind)
      return emitError() << "C++ declaration has no logical type category";
    StringRef valueType;
    if (kind.getValue() == "bool") {
      valueType = "bool";
    } else if (kind.getValue() == "integer") {
      auto interpretation = target.getAs<StringAttr>("interpretation");
      if (!interpretation || (interpretation.getValue() != "signed" &&
                              interpretation.getValue() != "unsigned"))
        return emitError() << "C++ declaration has an unsupported integer "
                              "interpretation: "
                           << ownerText;
      valueType = interpretation.getValue() == "signed" ? "::std::int64_t"
                                                        : "::std::uint64_t";
    } else {
      return emitError()
             << "C++ declaration has an unsupported alias category: "
             << ownerText;
    }
    return (Twine("using ") + cppName + " = " + valueType + ";\n").str();
  }

  auto type = operation->getAttrOfType<DictionaryAttr>("type");
  auto value = operation->getAttrOfType<DictionaryAttr>("value");
  auto kind = type ? type.getAs<StringAttr>("kind") : StringAttr();
  auto valueKind = value ? value.getAs<StringAttr>("kind") : StringAttr();
  if (!kind || !valueKind || kind != valueKind)
    return emitError() << "C++ declaration has mismatched constant type/value: "
                       << ownerText;
  if (kind.getValue() == "bool") {
    auto boolean = value.getAs<BoolAttr>("value");
    if (!boolean)
      return emitError() << "C++ boolean declaration has no boolean value: "
                         << ownerText;
    return (Twine("inline constexpr bool ") + cppName + " = " +
            (boolean.getValue() ? "true;\n" : "false;\n"))
        .str();
  }
  if (kind.getValue() != "integer")
    return emitError() << "C++ declaration has an unsupported constant "
                          "category: "
                       << ownerText;
  auto integer = value.getAs<ac::MathIntAttr>("value");
  if (!integer)
    return emitError() << "C++ integer declaration has no exact value: "
                       << ownerText;
  llvm::APSInt parsed(integer.getCanonicalValue());
  SmallString<64> decimalBuffer;
  parsed.toString(decimalBuffer, 10);
  StringRef decimal(decimalBuffer);
  const bool negative = parsed.isNegative();
  StringRef magnitude = negative ? decimal.drop_front() : decimal;
  constexpr StringLiteral minMagnitude = "9223372036854775808";
  constexpr StringLiteral maxUnsigned = "18446744073709551615";
  auto greaterMagnitude = [](StringRef left, StringRef right) {
    return left.size() > right.size() ||
           (left.size() == right.size() && left.compare(right) > 0);
  };
  if ((negative && greaterMagnitude(magnitude, minMagnitude)) ||
      (!negative && greaterMagnitude(magnitude, maxUnsigned)))
    return emitError() << "C++ integer declaration capability rejection for "
                       << ownerText << " value " << decimal
                       << "; supported range is [-9223372036854775808, "
                          "18446744073709551615]";
  if (!negative)
    return (Twine("inline constexpr ::std::uint64_t ") + cppName +
            " = UINT64_C(" + decimal + ");\n")
        .str();
  if (magnitude == minMagnitude)
    return (Twine("inline constexpr ::std::int64_t ") + cppName +
            " = (-INT64_C(9223372036854775807) - INT64_C(1));\n")
        .str();
  return (Twine("inline constexpr ::std::int64_t ") + cppName + " = -INT64_C(" +
          magnitude + ");\n")
      .str();
}

} // namespace acir::compiler
