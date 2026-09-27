#ifndef ACIR_DIALECT_ACIR_ACIRATTRIBUTES_TD
#define ACIR_DIALECT_ACIR_ACIRATTRIBUTES_TD

include "acir/Dialect/ACIR/ACIRDialect.td"
include "mlir/IR/AttrTypeBase.td"

def ACIR_MathIntAttr : AttrDef<ACIR_Dialect, "MathInt"> {
  let mnemonic = "math_int";
  let summary = "Canonical arbitrary-precision mathematical integer";
  let parameters = (ins "::mlir::StringAttr":$value);
  let builders = [
    AttrBuilder<(ins "const ::llvm::APSInt &":$value), [{
      ::llvm::SmallString<64> spelling;
      value.toString(spelling, 10);
      return $_get($_ctxt, ::mlir::StringAttr::get($_ctxt, spelling));
    }]>
  ];
  let skipDefaultBuilders = 1;
  let hasCustomAssemblyFormat = 1;
  let extraClassDeclaration = [{
    ::llvm::StringRef getCanonicalValue() const { return getValue().getValue(); }
  }];
}

#endif // ACIR_DIALECT_ACIR_ACIRATTRIBUTES_TD
