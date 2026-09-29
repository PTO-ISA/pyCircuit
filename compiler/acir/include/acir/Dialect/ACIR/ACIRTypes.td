#ifndef ACIR_DIALECT_ACIR_ACIRTYPES_TD
#define ACIR_DIALECT_ACIR_ACIRTYPES_TD

include "acir/Dialect/ACIR/ACIRAttributes.td"
include "mlir/IR/AttrTypeBase.td"

class ACIR_Type<string name, string typeMnemonic>
    : TypeDef<ACIR_Dialect, name> {
  let mnemonic = typeMnemonic;
}

def ACIR_MathIntType : ACIR_Type<"MathInt", "math_int"> {
  let summary = "Temporary arbitrary-precision source mathematical integer";
  let hasCustomAssemblyFormat = 1;
}

def ACIR_StructType : ACIR_Type<"Struct", "struct"> {
  let summary = "Qualified nominal immutable record value";
  let parameters = (ins "::mlir::StringAttr":$name);
  let assemblyFormat = "`<` $name `>`";
  let genVerifyDecl = 1;
}

def ACIR_RegType : ACIR_Type<"Reg", "reg"> {
  let summary = "Handle to a source-owned clocked register";
  let parameters = (ins "::mlir::Type":$elementType);
  let assemblyFormat = "`<` $elementType `>`";
  let genVerifyDecl = 1;
}

#endif // ACIR_DIALECT_ACIR_ACIRTYPES_TD
