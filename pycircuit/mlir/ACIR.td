#ifndef ACPY_ACIR_TD
#define ACPY_ACIR_TD
include "mlir/IR/OpBase.td"
include "mlir/IR/AttrTypeBase.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

def ACIR_Dialect : Dialect {
  let name = "acir";
  let cppNamespace = "::acir";
  let summary = "Statically connected transactional circuits for GFSim";
  let useDefaultTypePrinterParser = 1;
}

class ACIR_ElementType<string name, string typeMnemonic> : TypeDef<ACIR_Dialect, name> {
  let mnemonic = typeMnemonic;
  let parameters = (ins "::mlir::Type":$elementType);
  let assemblyFormat = "`<` qualified($elementType) `>`";
}
def ACIR_QueueType : ACIR_ElementType<"Queue", "queue">;
def ACIR_SignalType : ACIR_ElementType<"Signal", "signal">;
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
def ACIR_ResourceRef : AnyTypeOf<[ACIR_QueueType, ACIR_SignalType]>;

class ACIR_Op<string mnemonic, list<Trait> traits = []> : Op<ACIR_Dialect, mnemonic, traits>;

// Resource declarations and function attributes form the static connection graph.
def ACIR_ResourceOp : ACIR_Op<"resource"> {
  let arguments = (ins StrAttr:$name, StrAttr:$kind, StrAttr:$type);
  let hasVerifier = 1;
}
def ACIR_GetOp : ACIR_Op<"get", [Pure]> {
  let arguments = (ins StrAttr:$name);
  let results = (outs AnyTypeOf<[ACIR_QueueType, ACIR_SignalType, ACIR_QueueArrayType]>:$reference);
}
def ACIR_ConfigOp : ACIR_Op<"config", [Pure]> {
  let arguments = (ins StrAttr:$name);
  let results = (outs AnyType:$value);
}

// Reads/queries observe current state and may abort on missing input.
// Keep conservative effects so optimization cannot move them across branches.
// In particular these ops must never acquire Pure/AlwaysSpeculatable traits.
def ACIR_ReadOp : ACIR_Op<"read", [MemoryEffects<[MemRead, MemWrite]>]> {
  let arguments = (ins ACIR_ResourceRef:$resource);
  let results = (outs AnyType:$value);
  let hasVerifier = 1;
}
def ACIR_QueryOp : ACIR_Op<"query", [MemoryEffects<[MemRead, MemWrite]>]> {
  let arguments = (ins ACIR_QueueType:$resource, StrAttr:$kind);
  let results = (outs AnyInteger:$value);
  let hasVerifier = 1;
}
def ACIR_PopOp : ACIR_Op<"pop", [MemoryEffects<[MemRead, MemWrite]>]> {
  let arguments = (ins ACIR_QueueType:$resource);
}
def ACIR_PushOp : ACIR_Op<"push", [MemoryEffects<[MemWrite]>]> {
  let arguments = (ins ACIR_QueueType:$resource, AnyType:$value);
  let hasVerifier = 1;
}
def ACIR_ReviseOp : ACIR_Op<"revise", [MemoryEffects<[MemRead, MemWrite]>]> {
  let arguments = (ins ACIR_QueueType:$resource, AnyType:$value, Variadic<AnyInteger>:$indices, ArrayAttr:$path);
  let hasVerifier = 1;
}
def ACIR_InvokeOp : ACIR_Op<"invoke", [MemoryEffects<[MemRead, MemWrite]>]> {
  let arguments = (ins Variadic<AnyType>:$inputs, StrAttr:$name);
}
def ACIR_EventOp : ACIR_Op<"event", [MemoryEffects<[MemWrite]>]> {
  let arguments = (ins AnyInteger:$delay);
}

// Aggregate indexing may report a bounds error; it is not speculatable.
def ACIR_AggregateOp : ACIR_Op<"aggregate", [NoMemoryEffect]> {
  let arguments = (ins Variadic<AnyType>:$elements);
  let results = (outs AnyType:$value);
}
def ACIR_ExtractOp : ACIR_Op<"extract", [NoMemoryEffect]> {
  let arguments = (ins AnyType:$aggregate, Variadic<AnyInteger>:$indices, OptionalAttr<StrAttr>:$field);
  let results = (outs AnyType:$value);
}
def ACIR_UpdateOp : ACIR_Op<"update", [NoMemoryEffect]> {
  let arguments = (ins AnyType:$aggregate, AnyType:$replacement, Variadic<AnyInteger>:$indices, ArrayAttr:$path);
  let results = (outs AnyType:$value);
}
def ACIR_LengthOp : ACIR_Op<"length", [Pure]> {
  let arguments = (ins AnyType:$aggregate);
  let results = (outs I32:$length);
}
def ACIR_CastOp : ACIR_Op<"cast", [Pure]> {
  let arguments = (ins AnyInteger:$input);
  let results = (outs AnyInteger:$value);
}
def ACIR_UnaryOp : ACIR_Op<"unary", [Pure]> {
  let arguments = (ins AnyInteger:$input, StrAttr:$operator_name);
  let results = (outs AnyInteger:$value);
}
def ACIR_CompareOp : ACIR_Op<"compare", [Pure]> {
  let arguments = (ins AnyType:$lhs, AnyType:$rhs, StrAttr:$operator_name);
  let results = (outs I1:$value);
}
def ACIR_RangeOp : ACIR_Op<"range", [NoMemoryEffect]> {
  let arguments = (ins AnyInteger:$size);
  let results = (outs ACIR_VectorType:$value);
}
def ACIR_CheckOp : ACIR_Op<"check", [MemoryEffects<[MemWrite]>]> {
  let arguments = (ins I1:$condition);
}
def ACIR_UnreachableOp : ACIR_Op<"unreachable", [MemoryEffects<[MemWrite]>]>;
#endif
