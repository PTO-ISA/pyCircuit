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

def ACIR_ConstantOp : ACIR_Op<"constant", [Symbol]> {
  let summary = "Declare one source-owned compile-time constant";
  let arguments = (ins StrAttr:$sym_name, DictionaryAttr:$type,
      DictionaryAttr:$value);
  let hasVerifier = 1;
}

def ACIR_MathConstantOp : ACIR_Op<"math.constant"> {
  let summary = "Materialize one source mathematical integer constant";
  let arguments = (ins ACIR_MathIntAttr:$value);
  let results = (outs ACIR_MathIntType:$result);
  let hasVerifier = 1;
}

def ACIR_ValueBindingOp : ACIR_Op<"value.binding"> {
  let summary = "Bind one proof-scoped ValueID to actual SSA and controls";
  let arguments = (ins AnySignlessInteger:$value, I1:$valid, I1:$path,
      DictionaryAttr:$id, DictionaryAttr:$domain);
  let hasVerifier = 1;
}

def ACIR_ValueUseOp : ACIR_Op<"value.use"> {
  let summary = "Bind one proof-scoped source use to verified finite SSA";
  let arguments = (ins AnySignlessInteger:$value, I1:$valid, I1:$path,
      DictionaryAttr:$id, DictionaryAttr:$source);
  let hasVerifier = 1;
}

def ACIR_SystemOp : ACIR_Op<"system"> {
  let summary = "Select one verified final hardware instance tree";
  let arguments = (ins DictionaryAttr:$entry, StrAttr:$domain);
  let hasVerifier = 1;
}

def ACIR_NumericProofOp : ACIR_Op<"numeric.proof"> {
  let summary = "Verify one constant-only exact numeric witness";
  let arguments = (ins Variadic<AnyType>:$values,
      DenseI32ArrayAttr:$operand_segment_sizes, StrAttr:$mode,
      DictionaryAttr:$result_domain, ArrayAttr:$input_ids,
      ArrayAttr:$input_domains, DictionaryAttr:$result_id,
      ArrayAttr:$obligations, ArrayAttr:$checks, DictionaryAttr:$origin);
  let hasVerifier = 1;
}

def ACIR_MathFromBitsOp : ACIR_Op<"math.from_bits"> {
  let summary = "Lift a bounded source integer into mathematical arithmetic";
  let arguments = (ins AnySignlessInteger:$value, DictionaryAttr:$domain);
  let results = (outs ACIR_MathIntType:$result);
  let hasVerifier = 1;
}

def ACIR_MathCompareOp : ACIR_Op<"math.compare"> {
  let summary = "Compare two lifted source mathematical integers";
  let arguments = (ins I1:$path, ACIR_MathIntType:$lhs, I1:$lhs_valid,
      ACIR_MathIntType:$rhs, I1:$rhs_valid, StrAttr:$predicate);
  let results = (outs AnySignlessInteger:$result, I1:$valid);
  let hasVerifier = 1;
}

def ACIR_MathBinaryOp : ACIR_Op<"math.binary"> {
  let summary = "Evaluate one guarded source mathematical binary operation";
  let arguments = (ins I1:$path, ACIR_MathIntType:$lhs, I1:$lhs_valid,
      ACIR_MathIntType:$rhs, I1:$rhs_valid);
  let results = (outs ACIR_MathIntType:$result, I1:$valid);
  let hasVerifier = 1;
}

def ACIR_MathToBitsOp : ACIR_Op<"math.to_bits"> {
  let summary = "Range-check and lower a mathematical integer to bounded bits";
  let arguments = (ins I1:$path, ACIR_MathIntType:$value, I1:$value_valid,
      DictionaryAttr:$domain);
  let results = (outs AnySignlessInteger:$result, I1:$valid);
  let hasVerifier = 1;
}

def ACIR_ModuleImportOp : ACIR_Op<"module.import", [Symbol]> {
  let summary = "Declare one source-owned Pythonic hardware module signature";
  let arguments = (ins StrAttr:$sym_name);
  let hasVerifier = 1;
  let assemblyFormat = "$sym_name attr-dict";
}

def ACIR_RegOp : ACIR_Op<"reg"> {
  let summary = "Declare one source-owned clocked register";
  let arguments = (ins I1:$clock, I1:$reset, StrAttr:$name);
  let results = (outs ACIR_RegType:$state);
  let hasVerifier = 1;
}

def ACIR_ModuleOp : ACIR_Op<"module", [IsolatedFromAbove, Symbol]> {
  let summary = "Define a source-owned hardware module";
  let arguments = (ins StrAttr:$name, StrAttr:$sym_name);
  let regions = (region AnyRegion:$body);
  let hasVerifier = 1;
  let hasRegionVerifier = 1;
}

def ACIR_RuleOp
    : ACIR_Op<"rule", [AttrSizedOperandSegments, IsolatedFromAbove]> {
  let summary = "Describe one registered source rule";
  let arguments = (ins StrAttr:$name, DictionaryAttr:$registration,
      Variadic<ACIR_RegType>:$inputs, Variadic<ACIR_RegType>:$targets);
  let results = (outs Variadic<AnyType>:$next_values);
  let regions = (region AnyRegion:$body);
  let hasVerifier = 1;
  let hasRegionVerifier = 1;
}

def ACIR_SourceReadOp : ACIR_Op<"source.read"> {
  let summary = "Record one read of a registered rule current payload";
  let arguments = (ins AnyType:$current);
  let results = (outs AnyType:$result);
  let hasVerifier = 1;
}

def ACIR_SourceUseOp : ACIR_Op<"source.use"> {
  let summary = "Record one source value contribution to a rule target";
  let arguments = (ins AnyType:$value, I1:$valid, I1:$path, DictionaryAttr:$id,
      DictionaryAttr:$source, DictionaryAttr:$target);
  let results = (outs AnyType:$data, I1:$enabled);
  let hasVerifier = 1;
}

def ACIR_SourceObserveOp : ACIR_Op<"observe"> {
  let summary = "Record one source observation without runtime side effects";
  let arguments = (ins I1:$path, Variadic<AnyType>:$values, StrAttr:$kind,
      DictionaryAttr:$spec);
  let hasVerifier = 1;
}

def ACIR_SourceExpectOp : ACIR_Op<"expect"> {
  let summary =
      "Record one source assertion check without runtime side effects";
  let arguments = (ins I1:$condition, I1:$path, StrAttr:$kind,
      DictionaryAttr:$location);
  let hasVerifier = 1;
}

def ACIR_InstanceOp : ACIR_Op<"instance", [AttrSizedOperandSegments]> {
  let summary = "Bind one child module instance to parent-owned handles";
  let arguments = (ins StrAttr:$name, AnyAttr:$callee, I1:$clock, I1:$reset,
      Variadic<ACIR_RegType>:$inputs, Variadic<ACIR_RegType>:$targets);
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
  let assemblyFormat =
      "$sym_name `fields` $fields `constructor` $constructor attr-dict";
}

def ACIR_StructCreateOp : ACIR_Op<"struct.create", [Pure]> {
  let summary = "Construct a complete immutable nominal record";
  let arguments = (ins Variadic<AnyType>:$values);
  let results = (outs ACIR_StructType:$result);
  let hasVerifier = 1;
  let assemblyFormat =
      "`(` $values `)` attr-dict `:` functional-type($values, results)";
}

def ACIR_StructGetOp : ACIR_Op<"struct.get", [Pure]> {
  let summary = "Read one field from an immutable nominal record";
  let arguments = (ins ACIR_StructType:$value, StrAttr:$field);
  let results = (outs AnyType:$result);
  let hasVerifier = 1;
  let assemblyFormat =
      "$value `[` $field `]` attr-dict `:` functional-type(operands, results)";
}

#endif // ACIR_DIALECT_ACIR_ACIROPS_TD
