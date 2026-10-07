#ifndef ACIR_DIALECT_ACIR_ACIRATTRIBUTES_TD
#define ACIR_DIALECT_ACIR_ACIRATTRIBUTES_TD

include "pycircuit/Dialect/ACIR/ACIRDialect.td"
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

def ACIR_StaticExprAttr : AttrDef<ACIR_Dialect, "StaticExpr"> {
  let mnemonic = "static_expr";
  let summary = "Source-linked compile-time expression tree";
  let parameters = (ins "::mlir::DictionaryAttr":$tree);
  let assemblyFormat = "`<` $tree `>`";
  let genVerifyDecl = 1;
}

def ACIR_SourceDomainAttr : AttrDef<ACIR_Dialect, "SourceDomain"> {
  let mnemonic = "source_domain";
  let summary = "Width-free resolved source boolean or integer domain";
  let parameters = (ins "::mlir::DictionaryAttr":$value);
  let assemblyFormat = "`<` $value `>`";
  let genVerifyDecl = 1;
}

def ACIR_SourceTypeExprAttr : AttrDef<ACIR_Dialect, "SourceTypeExpr"> {
  let mnemonic = "source_type_expr";
  let summary = "Import-time annotation with static bounds or alias syntax";
  let parameters = (ins "::mlir::DictionaryAttr":$value);
  let assemblyFormat = "`<` $value `>`";
  let genVerifyDecl = 1;
}

#endif // ACIR_DIALECT_ACIR_ACIRATTRIBUTES_TD
