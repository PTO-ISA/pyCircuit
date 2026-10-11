#ifndef ACPY_ACIR_TD
#define ACPY_ACIR_TD
include "mlir/IR/OpBase.td"
include "mlir/IR/AttrTypeBase.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

def ACIR_Dialect : Dialect {
  let name = "acir";
  let cppNamespace = "::acir";
  let summary = "Statically connected transactional circuits";
  let useDefaultTypePrinterParser = 1;
}

class ACIR_ElementType<string name, string typeMnemonic> : TypeDef<ACIR_Dialect, name> {
  let mnemonic = typeMnemonic;
  let parameters = (ins "::mlir::Type":$elementType);
  let assemblyFormat = "`<` qualified($elementType) `>`";
}
def ACIR_QueueType : ACIR_ElementType<"Queue", "queue">;
def ACIR_SignalType : ACIR_ElementType<"Signal", "signal">;
def ACIR_CellType : ACIR_ElementType<"Cell", "cell">;
def ACIR_QueueArrayType : ACIR_ElementType<"QueueArray", "qarray">;
def ACIR_VectorType : ACIR_ElementType<"Vector", "vector">;
def ACIR_ArrayType : TypeDef<ACIR_Dialect, "Array"> {
  let mnemonic = "array";
  let parameters = (ins "::mlir::Type":$elementType, "int64_t":$size);
  let assemblyFormat = "`<` qualified($elementType) `,` $size `>`";
  let genVerifyDecl = 1;
}
def ACIR_StructType : TypeDef<ACIR_Dialect, "Struct"> {
  let mnemonic = "struct";
  let parameters = (ins StringRefParameter<"nominal payload struct name">:$name);
  let assemblyFormat = "`<` $name `>`";
}
def ACIR_ResourceRef : AnyTypeOf<[ACIR_QueueType, ACIR_CellType, ACIR_SignalType]>;

class ACIR_Op<string mnemonic, list<Trait> traits = []> : Op<ACIR_Dialect, mnemonic, traits>;

// These declarative formats only change assembly. Keep semantic metadata in
// attr-dict; MLIR still accepts the generic quoted operation syntax.
// Resource declarations and function attributes form the static connection graph.
def ACIR_ResourceOp : ACIR_Op<"resource"> {
  let arguments = (ins StrAttr:$name, StrAttr:$kind, StrAttr:$type);
  let assemblyFormat = "$name $kind $type attr-dict";
  let hasVerifier = 1;
}
def ACIR_GetOp : ACIR_Op<"get", [Pure]> {
  let arguments = (ins StrAttr:$name);
  let results = (outs AnyTypeOf<[ACIR_QueueType, ACIR_CellType, ACIR_SignalType, ACIR_QueueArrayType]>:$reference);
  let assemblyFormat = "$name attr-dict `:` type($reference)";
}
def ACIR_ConfigOp : ACIR_Op<"config", [Pure]> {
  let arguments = (ins StrAttr:$name);
  let results = (outs AnyType:$value);
  let assemblyFormat = "$name attr-dict `:` type($value)";
}

// Reads/queries observe component views and require path-local readiness.
// Keep conservative effects so optimization cannot move them across branches.
// In particular these ops must never acquire Pure/AlwaysSpeculatable traits.
def ACIR_ReadOp : ACIR_Op<"read", [MemoryEffects<[MemRead, MemWrite]>]> {
  let arguments = (ins ACIR_ResourceRef:$resource);
  let results = (outs AnyType:$value);
  let assemblyFormat = "$resource attr-dict `:` type($resource) `->` type($value)";
  let hasVerifier = 1;
}
def ACIR_QueryOp : ACIR_Op<"query", [MemoryEffects<[MemRead, MemWrite]>]> {
  let arguments = (ins ACIR_QueueType:$resource, StrAttr:$kind);
  let results = (outs AnyInteger:$value);
  let assemblyFormat = "$resource $kind attr-dict `:` qualified(type($resource)) `->` type($value)";
  let hasVerifier = 1;
}
def ACIR_PopOp : ACIR_Op<"pop", [MemoryEffects<[MemRead, MemWrite]>]> {
  let arguments = (ins ACIR_QueueType:$resource);
  let assemblyFormat = "$resource attr-dict `:` qualified(type($resource))";
}
def ACIR_PushOp : ACIR_Op<"push", [MemoryEffects<[MemWrite]>]> {
  let arguments = (ins ACIR_QueueType:$resource, AnyType:$value);
  let assemblyFormat = "$resource `,` $value attr-dict `:` qualified(type($resource)) `,` type($value)";
  let hasVerifier = 1;
}
def ACIR_WriteOp : ACIR_Op<"write", [MemoryEffects<[MemWrite]>]> {
  let arguments = (ins ACIR_ResourceRef:$resource, AnyType:$value);
  let assemblyFormat = "$resource `,` $value attr-dict `:` type($resource) `,` type($value)";
  let hasVerifier = 1;
}
def ACIR_GuardOp : ACIR_Op<"guard", [MemoryEffects<[MemRead, MemWrite]>]> {
  let arguments = (ins I1:$condition, StrAttr:$name);
  let assemblyFormat = "$condition $name attr-dict";
}
// Stable shared SSA anchor. Keep it even when its input folds or is unused locally.
def ACIR_ExportOp : ACIR_Op<"export", [MemoryEffects<[MemRead, MemWrite]>]> {
  let arguments = (ins AnyType:$input);
  let results = (outs AnyType:$value);
  let assemblyFormat = "$input attr-dict `:` type($input) `->` type($value)";
  let hasVerifier = 1;
}
// References a shared preparation SSA definition in another instance.
def ACIR_ExternalOp : ACIR_Op<"external", [MemoryEffects<[MemRead, MemWrite]>]> {
  let arguments = (ins StrAttr:$source, StrAttr:$identity);
  let results = (outs AnyType:$value);
  let assemblyFormat = "$source $identity attr-dict `:` type($value)";
}
def ACIR_ReviseOp : ACIR_Op<"revise", [MemoryEffects<[MemRead, MemWrite]>]> {
  let arguments = (ins ACIR_QueueType:$resource, AnyType:$value, Variadic<AnyInteger>:$indices, ArrayAttr:$path);
  let assemblyFormat = "$resource `,` $value (`[` $indices^ `]`)? attr-dict `:` functional-type(operands, results)";
  let hasVerifier = 1;
}
def ACIR_InvokeOp : ACIR_Op<"invoke", [MemoryEffects<[MemRead, MemWrite]>]> {
  let arguments = (ins Variadic<AnyType>:$inputs, StrAttr:$name);
  let assemblyFormat = "$name `(` $inputs `)` attr-dict `:` functional-type(operands, results)";
}
// Aggregate indexing may report a bounds error; it is not speculatable.
def ACIR_AggregateOp : ACIR_Op<"aggregate", [NoMemoryEffect]> {
  let arguments = (ins Variadic<AnyType>:$elements);
  let results = (outs AnyType:$value);
  let assemblyFormat = "`(` $elements `)` attr-dict `:` functional-type(operands, results)";
}
def ACIR_ExtractOp : ACIR_Op<"extract", [NoMemoryEffect]> {
  let arguments = (ins AnyType:$aggregate, Variadic<AnyInteger>:$indices, OptionalAttr<StrAttr>:$field);
  let results = (outs AnyType:$value);
  let assemblyFormat = "$aggregate (`[` $indices^ `]`)? attr-dict `:` functional-type(operands, results)";
}
def ACIR_UpdateOp : ACIR_Op<"update", [NoMemoryEffect]> {
  let arguments = (ins AnyType:$aggregate, AnyType:$replacement, Variadic<AnyInteger>:$indices, ArrayAttr:$path);
  let results = (outs AnyType:$value);
  let assemblyFormat = "$aggregate `,` $replacement (`[` $indices^ `]`)? attr-dict `:` functional-type(operands, results)";
}
def ACIR_LengthOp : ACIR_Op<"length", [Pure]> {
  let arguments = (ins AnyType:$aggregate);
  let results = (outs I32:$length);
  let assemblyFormat = "$aggregate attr-dict `:` type($aggregate)";
}
def ACIR_CastOp : ACIR_Op<"cast", [Pure]> {
  let arguments = (ins AnyInteger:$input);
  let results = (outs AnyInteger:$value);
  let assemblyFormat = "$input attr-dict `:` type($input) `->` type($value)";
}
def ACIR_UnaryOp : ACIR_Op<"unary", [Pure]> {
  let arguments = (ins AnyInteger:$input, StrAttr:$operator_name);
  let results = (outs AnyInteger:$value);
  let assemblyFormat = "$input $operator_name attr-dict `:` type($input) `->` type($value)";
}
def ACIR_CompareOp : ACIR_Op<"compare", [Pure]> {
  let arguments = (ins AnyType:$lhs, AnyType:$rhs, StrAttr:$operator_name);
  let results = (outs I1:$value);
  let assemblyFormat = "$lhs `,` $rhs $operator_name attr-dict `:` type($lhs) `,` type($rhs)";
}
def ACIR_RangeOp : ACIR_Op<"range", [NoMemoryEffect]> {
  let arguments = (ins AnyInteger:$size);
  let results = (outs ACIR_VectorType:$value);
  let assemblyFormat = "$size attr-dict `:` type($size) `->` qualified(type($value))";
}
def ACIR_CheckOp : ACIR_Op<"check", [MemoryEffects<[MemWrite]>]> {
  let arguments = (ins I1:$condition);
  let assemblyFormat = "$condition attr-dict";
}
def ACIR_UnreachableOp : ACIR_Op<"unreachable", [MemoryEffects<[MemWrite]>]> {
  let assemblyFormat = "attr-dict";
}
#endif
