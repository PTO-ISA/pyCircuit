#include "ACIRSourceContracts.h"
#include "acir/Dialect/ACIR/ACIRTypes.h"

#include "mlir/IR/DialectImplementation.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"

#include <algorithm>
#include <limits>
#include <utility>

using namespace mlir;

namespace acir::ac {
namespace {

FailureOr<llvm::APSInt>
parseCanonicalMathInt(StringRef spelling,
                      llvm::function_ref<InFlightDiagnostic()> emitError) {
  if (spelling.empty())
    return emitError() << "mathematical integer must not be empty";

  bool negative = spelling.consume_front("-");
  if (spelling.empty() || spelling.starts_with('+'))
    return emitError()
           << "mathematical integer must use canonical signed decimal";
  if (spelling.size() > 1 && spelling.starts_with('0'))
    return emitError() << "mathematical integer must not have leading zeroes";
  for (char digit : spelling)
    if (digit < '0' || digit > '9')
      return emitError()
             << "mathematical integer must use canonical signed decimal";
  if (negative && spelling == "0")
    return emitError() << "mathematical integer must not use negative zero";
  if (spelling.size() > (std::numeric_limits<unsigned>::max() - 1) / 4)
    return emitError() << "mathematical integer spelling is too large";

  unsigned width = static_cast<unsigned>(spelling.size() * 4 + 1);
  llvm::APInt bits(width, spelling, 10);
  if (negative)
    bits = -bits;
  return llvm::APSInt(std::move(bits), /*isUnsigned=*/!negative);
}

} // namespace

namespace detail {

FailureOr<uint64_t> decodeU64(IntegerAttr value, StringRef description,
                              EmitError emitError) {
  if (!value || isa<BoolAttr>(value))
    return emitError() << description << " must be a u64 IntegerAttr";
  auto integerType = dyn_cast<IntegerType>(value.getType());
  const llvm::APInt &bits = value.getValue();
  if (!integerType || (!integerType.isUnsigned() && bits.isNegative()))
    return emitError() << description << " must be non-negative";
  if (bits.getActiveBits() > 64)
    return emitError() << description << " is outside the u64 range";
  return bits.getZExtValue();
}

} // namespace detail

Attribute MathIntAttr::parse(AsmParser &parser, Type) {
  if (failed(parser.parseLess()))
    return {};
  bool negative = succeeded(parser.parseOptionalMinus());
  llvm::SMLoc digitsBegin = parser.getCurrentLocation();
  llvm::APInt magnitude;
  if (failed(parser.parseDecimalInteger(magnitude)))
    return {};
  llvm::SMLoc digitsEnd = parser.getCurrentLocation();
  const char *tokenEnd = digitsBegin.getPointer();
  while (tokenEnd != digitsEnd.getPointer() && *tokenEnd >= '0' &&
         *tokenEnd <= '9')
    ++tokenEnd;
  StringRef digits(digitsBegin.getPointer(),
                   tokenEnd - digitsBegin.getPointer());
  if (digits.size() > 1 && digits.starts_with('0')) {
    parser.emitError(digitsBegin,
                     "mathematical integer must not have leading zeroes");
    return {};
  }
  if (failed(parser.parseGreater()))
    return {};
  if (negative && magnitude.isZero()) {
    parser.emitError(digitsBegin,
                     "mathematical integer must not use negative zero");
    return {};
  }
  unsigned width = std::max(1u, magnitude.getActiveBits() + 1);
  llvm::APInt bits = magnitude.zextOrTrunc(width);
  if (negative)
    bits = -bits;
  return get(parser.getContext(),
             llvm::APSInt(std::move(bits), /*isUnsigned=*/!negative));
}

void MathIntAttr::print(AsmPrinter &printer) const {
  printer << '<' << getCanonicalValue() << '>';
}

Type MathIntType::parse(AsmParser &parser) {
  if (succeeded(parser.parseOptionalLess())) {
    parser.emitError(parser.getCurrentLocation(),
                     "math_int type does not accept parameters");
    return {};
  }
  return get(parser.getContext());
}

void MathIntType::print(AsmPrinter &) const {}

namespace detail {

FailureOr<uint64_t> readU64(DictionaryAttr record, StringRef name,
                            EmitError emitError) {
  return decodeU64(record.getAs<IntegerAttr>(name),
                   (Twine("field '") + name + "'").str(), emitError);
}

FailureOr<MathIntAttr> parseMathIntAttr(MLIRContext *context,
                                        StringRef spelling,
                                        EmitError emitError) {
  FailureOr<llvm::APSInt> value = parseCanonicalMathInt(spelling, emitError);
  if (failed(value))
    return failure();
  return MathIntAttr::get(context, *value);
}

LogicalResult verifySourceSpan(DictionaryAttr value, EmitError emitError) {
  if (!value || value.size() != 5)
    return emitError() << "SourceSpan must contain exactly five fields";
  if (!value.getAs<StringAttr>("path"))
    return emitError() << "SourceSpan field 'path' must be a StringAttr";

  FailureOr<uint64_t> line = readU64(value, "line", emitError);
  FailureOr<uint64_t> column = readU64(value, "column", emitError);
  FailureOr<uint64_t> endLine = readU64(value, "end_line", emitError);
  FailureOr<uint64_t> endColumn = readU64(value, "end_column", emitError);
  if (failed(line) || failed(column) || failed(endLine) || failed(endColumn))
    return failure();
  if (*line == 0 || *column == 0 || *endLine == 0 || *endColumn == 0)
    return emitError() << "SourceSpan coordinates must be one-based";
  if (*endLine < *line || (*endLine == *line && *endColumn < *column))
    return emitError() << "SourceSpan must not end before it starts";
  return success();
}

LogicalResult verifyPathComponent(DictionaryAttr value, EmitError emitError) {
  if (!value || value.size() != 2)
    return emitError() << "PathComponent must contain exactly two fields";
  auto kind = value.getAs<StringAttr>("kind");
  if (!kind)
    return emitError() << "PathComponent field 'kind' must be a StringAttr";
  if (kind.getValue() == "field") {
    auto name = value.getAs<StringAttr>("name");
    if (!name)
      return emitError() << "field PathComponent requires a StringAttr name";
    return success();
  }
  if (kind.getValue() == "index")
    return succeeded(readU64(value, "value", emitError)) ? success()
                                                         : failure();
  return emitError() << "PathComponent kind must be 'field' or 'index'";
}

LogicalResult verifySite(DictionaryAttr value, EmitError emitError) {
  if (!value || value.size() != 2)
    return emitError() << "Site must contain exactly two fields";
  if (!value.getAs<FlatSymbolRefAttr>("definition"))
    return emitError() << "Site field 'definition' must be a FlatSymbolRefAttr";
  auto path = value.getAs<ArrayAttr>("ast_path");
  if (!path)
    return emitError() << "Site field 'ast_path' must be an ArrayAttr";
  for (Attribute component : path) {
    auto record = dyn_cast<DictionaryAttr>(component);
    if (!record)
      return emitError()
             << "Site ast_path elements must be PathComponent DictionaryAttr";
    if (failed(verifyPathComponent(record, emitError)))
      return failure();
  }
  return success();
}

} // namespace detail
} // namespace acir::ac
