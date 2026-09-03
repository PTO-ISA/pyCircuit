#include "acir/Dialect/ACSim/ACSimTypes.h"
#include "acir/Dialect/ACSim/ACSimOps.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/DialectImplementation.h"
#include "llvm/ADT/TypeSwitch.h"

using namespace mlir;

namespace acir::acsim {

Type ArrayType::parse(AsmParser &parser) {
  if (failed(parser.parseLess()) || failed(parser.parseLSquare()))
    return {};

  SmallVector<int64_t> shape;
  if (failed(parser.parseOptionalRSquare())) {
    do {
      int64_t extent;
      if (failed(parser.parseInteger(extent)))
        return {};
      shape.push_back(extent);
    } while (succeeded(parser.parseOptionalComma()));
    if (failed(parser.parseRSquare()))
      return {};
  }

  Type elementType;
  if (failed(parser.parseComma()) || failed(parser.parseType(elementType)) ||
      failed(parser.parseGreater()))
    return {};
  return ArrayType::getChecked(
      [&] { return parser.emitError(parser.getCurrentLocation()); },
      parser.getContext(), DenseI64ArrayAttr::get(parser.getContext(), shape),
      elementType);
}

void ArrayType::print(AsmPrinter &printer) const {
  printer << "<[";
  llvm::interleaveComma(getShape().asArrayRef(), printer,
                        [&](int64_t extent) { printer << extent; });
  printer << "], " << getElementType() << ">";
}

LogicalResult ArrayType::verify(function_ref<InFlightDiagnostic()> emitError,
                                DenseI64ArrayAttr shape, Type elementType) {
  if (!shape)
    return emitError() << "array shape must be concrete";
  if (shape.empty())
    return emitError() << "array shape must have at least one extent";
  uint64_t volume = 1;
  for (int64_t extent : shape.asArrayRef()) {
    if (extent < 0)
      return emitError() << "array extents must be non-negative";
    if (extent == 0) {
      volume = 0;
      continue;
    }
    uint64_t unsignedExtent = static_cast<uint64_t>(extent);
    if (volume > kMaxArrayVolume / unsignedExtent)
      return emitError() << "array volume exceeds ACSim capability "
                         << kMaxArrayVolume;
    volume *= unsignedExtent;
  }
  if (!elementType)
    return emitError() << "array element type must be concrete";
  return success();
}

} // namespace acir::acsim

#define GET_TYPEDEF_CLASSES
#include "acir/Dialect/ACSim/ACSimTypes.cpp.inc"

void acir::acsim::ACSimDialect::initialize() {
  addTypes<
#define GET_TYPEDEF_LIST
#include "acir/Dialect/ACSim/ACSimTypes.cpp.inc"
      >();
  addOperations<
#define GET_OP_LIST
#include "acir/Dialect/ACSim/ACSimOps.cpp.inc"
      >();
}
