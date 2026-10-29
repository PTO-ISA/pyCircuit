#ifndef ACIR_LIB_DIALECT_ACIR_TRANSACTIONALREPRESENTATION_H
#define ACIR_LIB_DIALECT_ACIR_TRANSACTIONALREPRESENTATION_H

#include "mlir/IR/Builders.h"
#include "pycircuit/Dialect/ACIR/ACIRTypes.h"

namespace acir::ac::detail {
inline mlir::DictionaryAttr transactionalOccurrence(mlir::Builder &b,
                                                    llvm::StringRef top,
                                                    unsigned ordinal = 0) {
  auto index = b.getDictionaryAttr(
      {b.getNamedAttr("kind", b.getStringAttr("index")),
       b.getNamedAttr("value", b.getI64IntegerAttr(ordinal))});
  auto site = b.getDictionaryAttr(
      {b.getNamedAttr("definition",
                      mlir::FlatSymbolRefAttr::get(b.getContext(), top)),
       b.getNamedAttr("ast_path", b.getArrayAttr({index}))});
  return b.getDictionaryAttr({b.getNamedAttr("site", site),
                              b.getNamedAttr("expansion", b.getArrayAttr({}))});
}
inline mlir::DictionaryAttr transactionalSpan(mlir::Builder &b) {
  return b.getDictionaryAttr(
      {b.getNamedAttr("path", b.getStringAttr("__transactional__.py")),
       b.getNamedAttr("line", b.getI64IntegerAttr(1)),
       b.getNamedAttr("column", b.getI64IntegerAttr(1)),
       b.getNamedAttr("end_line", b.getI64IntegerAttr(1)),
       b.getNamedAttr("end_column", b.getI64IntegerAttr(2))});
}
// Representation only: callers choose the value, type and provenance.
inline StaticExprAttr transactionalInteger(mlir::Builder &b, uint64_t value,
                                           mlir::DictionaryAttr origin,
                                           mlir::DictionaryAttr location) {
  auto literal = b.getDictionaryAttr(
      {b.getNamedAttr("kind", b.getStringAttr("integer")),
       b.getNamedAttr(
           "value",
           MathIntAttr::get(b.getContext(),
                            llvm::APSInt(llvm::APInt(64, value), true)))});
  return StaticExprAttr::get(
      b.getContext(),
      b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("literal")),
                           b.getNamedAttr("value", literal),
                           b.getNamedAttr("origin", origin),
                           b.getNamedAttr("location", location)}));
}
inline StaticExprAttr transactionalInteger(mlir::Builder &b, uint64_t value,
                                           StaticExprAttr provenance) {
  auto tree = provenance.getTree();
  return transactionalInteger(b, value,
                              tree.getAs<mlir::DictionaryAttr>("origin"),
                              tree.getAs<mlir::DictionaryAttr>("location"));
}
inline BitsType transactionalBits(mlir::Builder &b, unsigned width,
                                  llvm::StringRef top) {
  return BitsType::get(b.getContext(),
                       transactionalInteger(b, width,
                                            transactionalOccurrence(b, top),
                                            transactionalSpan(b)));
}
} // namespace acir::ac::detail
#endif
