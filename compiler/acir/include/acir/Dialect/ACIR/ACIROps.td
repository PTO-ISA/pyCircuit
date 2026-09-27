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

def ACIR_ModuleImportOp : ACIR_Op<"module.import", [Symbol]> {
  let summary = "Declare one source-owned Pythonic hardware module signature";
  let arguments = (ins StrAttr:$sym_name);
  let hasVerifier = 1;
  let assemblyFormat = "$sym_name attr-dict";
}

def ACIR_DffeOp : ACIR_Op<"dffe"> {
  let summary = "Declare one source-owned enabled register";
  let arguments = (ins StrAttr:$name);
  let results = (outs ACIR_DffeType:$state);
  let hasVerifier = 1;
}

def ACIR_ModuleOp : ACIR_Op<"module", [IsolatedFromAbove, Symbol]> {
  let summary = "Define a source-owned hardware module";
  let arguments = (ins StrAttr:$name, StrAttr:$sym_name);
  let regions = (region AnyRegion:$body);
  let hasVerifier = 1;
  let hasRegionVerifier = 1;
}

def ACIR_RuleOp : ACIR_Op<"rule", [AttrSizedOperandSegments,
                                     IsolatedFromAbove]> {
  let summary = "Describe one registered source rule";
  let arguments = (ins StrAttr:$name, DictionaryAttr:$registration,
                       Variadic<ACIR_DffeType>:$inputs,
                       Variadic<ACIR_DffeType>:$outputs);
  let results = (outs Variadic<AnyType>:$next_values);
  let regions = (region AnyRegion:$readiness, AnyRegion:$conditions,
                        AnyRegion:$body, AnyRegion:$admission);
  let hasVerifier = 1;
  let hasRegionVerifier = 1;
}

def ACIR_InstanceOp : ACIR_Op<"instance", [AttrSizedOperandSegments]> {
  let summary = "Bind one child module instance to parent-owned handles";
  let arguments = (ins StrAttr:$name, AnyAttr:$callee,
                       Variadic<ACIR_DffeType>:$inputs,
                       Variadic<ACIR_DffeType>:$outputs);
  let results = (outs Variadic<AnyType>:$next_values);
  let hasVerifier = 1;
}

def ACIR_YieldOp : ACIR_Op<"yield", [Terminator]> {
  let summary = "Return source or final module and rule values";
  let arguments = (ins Variadic<AnyType>:$values);
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
