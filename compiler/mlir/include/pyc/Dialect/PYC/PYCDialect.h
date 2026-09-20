#pragma once

#include "mlir/IR/Dialect.h"
#include "mlir/IR/DialectImplementation.h"
#include "mlir/IR/Builders.h"
#include "llvm/ADT/StringRef.h"

namespace pyc {

class PYCDialect : public ::mlir::Dialect {
public:
  explicit PYCDialect(::mlir::MLIRContext *ctx);
  static ::llvm::StringRef getDialectNamespace() { return "pyc"; }

  void initialize();

  ::mlir::Attribute parseAttribute(::mlir::DialectAsmParser &parser,
                                   ::mlir::Type type) const override;
  void printAttribute(::mlir::Attribute attribute,
                      ::mlir::DialectAsmPrinter &printer) const override;

  ::mlir::Type parseType(::mlir::DialectAsmParser &parser) const override;
  void printType(::mlir::Type type, ::mlir::DialectAsmPrinter &printer) const override;

  ::mlir::Operation *materializeConstant(::mlir::OpBuilder &builder, ::mlir::Attribute value, ::mlir::Type type,
                                        ::mlir::Location loc) override;
};

} // namespace pyc
