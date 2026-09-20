#include "pyc/Dialect/PYC/PYCDialect.h"
#include "pyc/Dialect/PYC/PYCAttributes.h"
#include "llvm/ADT/TypeSwitch.h"

#include "pyc/Dialect/PYC/PYCOps.h"
#include "pyc/Dialect/PYC/PYCTypes.h"

#include "mlir/IR/DialectImplementation.h"

using namespace mlir;

#define GET_ATTRDEF_CLASSES
#include "pyc/Dialect/PYC/PYCAttributes.cpp.inc"
using namespace pyc;

PYCDialect::PYCDialect(MLIRContext *ctx) : Dialect(getDialectNamespace(), ctx, TypeID::get<PYCDialect>()) {
  initialize();
}

Operation *PYCDialect::materializeConstant(OpBuilder &builder, Attribute value, Type type, Location loc) {
  auto intTy = dyn_cast<IntegerType>(type);
  auto intAttr = dyn_cast<IntegerAttr>(value);
  if (!intTy || !intAttr)
    return nullptr;

  llvm::APInt v = intAttr.getValue();
  if (v.getBitWidth() != intTy.getWidth())
    v = v.zextOrTrunc(intTy.getWidth());

  return builder.create<pyc::ConstantOp>(loc, intTy, IntegerAttr::get(intTy, v));
}

Type PYCDialect::parseType(DialectAsmParser &parser) const {
  StringRef mnemonic;
  if (failed(parser.parseKeyword(&mnemonic)))
    return Type();

  MLIRContext *ctx = parser.getContext();
  if (mnemonic == "clock")
    return pyc::ClockType::get(ctx);
  if (mnemonic == "reset")
    return pyc::ResetType::get(ctx);

  parser.emitError(parser.getNameLoc(), "unknown pyc type: ") << mnemonic;
  return Type();
}

void PYCDialect::printType(Type type, DialectAsmPrinter &printer) const {
  if (isa<pyc::ClockType>(type)) {
    printer << "clock";
    return;
  }
  if (isa<pyc::ResetType>(type)) {
    printer << "reset";
    return;
  }
  llvm_unreachable("unknown pyc type");
}

void PYCDialect::initialize() {
  addAttributes<
#define GET_ATTRDEF_LIST
#include "pyc/Dialect/PYC/PYCAttributes.cpp.inc"
      >();
  addTypes<
#define GET_TYPEDEF_LIST
#include "pyc/Dialect/PYC/PYCTypes.cpp.inc"
      >();
  addOperations<
#define GET_OP_LIST
#include "pyc/Dialect/PYC/PYCOps.cpp.inc"
      >();
}
