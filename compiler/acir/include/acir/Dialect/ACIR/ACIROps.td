#ifndef ACIR_DIALECT_ACIR_ACIROPS_TD
#define ACIR_DIALECT_ACIR_ACIROPS_TD

include "acir/Dialect/ACIR/ACIRTypes.td"
include "mlir/IR/CommonAttrConstraints.td"
include "mlir/IR/OpBase.td"
include "mlir/IR/SymbolInterfaces.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

class ACIR_Op<string mnemonic, list<Trait> traits = []>
    : Op<ACIR_Dialect, mnemonic, traits>;

def ACIR_TypeAliasOp : ACIR_Op<"type_alias", [Symbol]> {
  let summary = "Declare one source-owned transparent logical type alias";
  let arguments = (ins StrAttr:$sym_name, DictionaryAttr:$target);
  let hasVerifier = 1;
  let assemblyFormat = "$sym_name `target` $target attr-dict";
}

def ACIR_StructOp : ACIR_Op<"struct", [Symbol]> {
  let summary = "Declare one source-owned ordered nominal record";
  let arguments = (ins StrAttr:$sym_name, ArrayAttr:$fields,
                       FlatSymbolRefAttr:$constructor);
  let hasVerifier = 1;
  let assemblyFormat = "$sym_name `fields` $fields `constructor` $constructor attr-dict";
}

def ACIR_StructCreateOp : ACIR_Op<"struct.create", [Pure]> {
  let summary = "Construct a complete immutable nominal record";
  let arguments = (ins Variadic<AnyType>:$values);
  let results = (outs ACIR_StructType:$result);
  let hasVerifier = 1;
  let assemblyFormat = "`(` $values `)` attr-dict `:` functional-type($values, results)";
}

def ACIR_StructGetOp : ACIR_Op<"struct.get", [Pure]> {
  let summary = "Read one field from an immutable nominal record";
  let arguments = (ins ACIR_StructType:$value, StrAttr:$field);
  let results = (outs AnyType:$result);
  let hasVerifier = 1;
  let assemblyFormat = "$value `[` $field `]` attr-dict `:` functional-type(operands, results)";
}

#endif // ACIR_DIALECT_ACIR_ACIROPS_TD
