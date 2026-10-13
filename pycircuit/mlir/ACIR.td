#ifndef ACPY_ACIR_TD
#define ACPY_ACIR_TD
include "mlir/IR/OpBase.td"
include "mlir/IR/AttrTypeBase.td"
include "mlir/Interfaces/SideEffectInterfaces.td"
include "mlir/IR/SymbolInterfaces.td"

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
def ACIR_EHRType : ACIR_ElementType<"EHR", "ehr">;
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
def ACIR_ResourceRef : AnyTypeOf<[ACIR_QueueType, ACIR_CellType, ACIR_SignalType, ACIR_EHRType]>;

class ACIR_Op<string mnemonic, list<Trait> traits = []> : Op<ACIR_Dialect, mnemonic, traits>;

// Statically elaborated instance scope.
def ACIR_ModuleOp : ACIR_Op<"module", [Symbol, SymbolTable, NoTerminator]> {
  let arguments = (ins SymbolNameAttr:$sym_name);
  let regions = (region AnyRegion:$declarations);
  let assemblyFormat = "$sym_name attr-dict-with-keyword $declarations";
}
def ACIR_InstanceOp : ACIR_Op<"instance", [Symbol]> {
  let arguments = (ins SymbolNameAttr:$sym_name, SymbolRefAttr:$target);
  let assemblyFormat = "$sym_name `=` $target attr-dict";
  let hasVerifier = 1;
}
def ACIR_ContractOp : ACIR_Op<"contract"> {
  let arguments = (ins ArrayAttr:$relations, ArrayAttr:$rules);
  let assemblyFormat = "attr-dict";
}
class ACIR_Definition<string name> : ACIR_Op<name, [Symbol, IsolatedFromAbove]> {
  let arguments = (ins SymbolNameAttr:$sym_name, TypeAttrOf<FunctionType>:$function_type);
  let regions = (region AnyRegion:$guard, AnyRegion:$body);
  let assemblyFormat = "$sym_name `:` $function_type attr-dict (`guard` $guard^)? `body` $body";
  let hasVerifier = 1;
}
def ACIR_RuleOp : ACIR_Definition<"rule">;
def ACIR_BodyOp : ACIR_Op<"body", [IsolatedFromAbove]> {
  let regions = (region AnyRegion:$body);
  let assemblyFormat = "attr-dict-with-keyword $body";
  let hasVerifier = 1;
}
class ACIR_Declaration<string name> : ACIR_Op<name, [Symbol]> {
  let arguments = (ins SymbolNameAttr:$sym_name, StrAttr:$name, StrAttr:$type, TypeAttr:$payload_type);
  let assemblyFormat = "$sym_name `:` $payload_type attr-dict";
  let hasVerifier = 1;
}
def ACIR_QueueOp : ACIR_Declaration<"queue">;
def ACIR_CellOp : ACIR_Declaration<"cell">;
def ACIR_SignalOp : ACIR_Declaration<"signal">;
def ACIR_EHROp : ACIR_Declaration<"ehr">;
def ACIR_QueueArrayOp : ACIR_Declaration<"queue.array">;
def ACIR_GetOp : ACIR_Op<"ref", [Pure]> {
  let arguments = (ins SymbolRefAttr:$target);
  let results = (outs AnyType:$reference);
  let assemblyFormat = "$target attr-dict `:` type($reference)";
  let hasVerifier = 1;
}
def ACIR_RuleCallOp : ACIR_Op<"rule.call", [MemoryEffects<[MemRead, MemWrite]>]> {
  let arguments = (ins Variadic<AnyType>:$inputs, SymbolRefAttr:$target);
  let results = (outs Variadic<AnyType>:$outputs);
  let assemblyFormat = "$target `(` $inputs `)` attr-dict `:` functional-type(operands, results)";
  let hasVerifier = 1;
}
class ACIR_End<string name> : ACIR_Op<name, [Terminator]> {
  let arguments = (ins Variadic<AnyType>:$values);
  let assemblyFormat = "($values^ `:` type($values))? attr-dict";
}
def ACIR_GuardYieldOp : ACIR_Op<"guard.yield", [Terminator]> {
  let arguments = (ins I1:$condition);
  let assemblyFormat = "$condition attr-dict";
  let hasVerifier = 1;
}
def ACIR_ReturnOp : ACIR_End<"return"> { let hasVerifier = 1; }
def ACIR_BodyEndOp : ACIR_Op<"body.end", [Terminator]> {
  let assemblyFormat = "attr-dict";
  let hasVerifier = 1;
}
class ACIR_Read<string name, Type ref, Type result = AnyType> : ACIR_Op<name, [MemoryEffects<[MemRead, MemWrite]>]> {
  let arguments = (ins ref:$resource);
  let results = (outs result:$value);
  let assemblyFormat = "$resource attr-dict `:` qualified(type($resource)) `->` type($value)";
  let hasVerifier = 1;
}
class ACIR_Write<string name, Type ref> : ACIR_Op<name, [MemoryEffects<[MemWrite]>]> {
  let arguments = (ins ref:$resource, AnyType:$value);
  let assemblyFormat = "$resource `,` $value attr-dict `:` qualified(type($resource)) `,` type($value)";
  let hasVerifier = 1;
}
def ACIR_QueuePeekOp : ACIR_Read<"queue.peek", ACIR_QueueType>;
def ACIR_CellReadOp : ACIR_Read<"cell.read", ACIR_CellType>;
def ACIR_SignalGetOp : ACIR_Read<"signal.get", ACIR_SignalType>;
def ACIR_EHRReadOp : ACIR_Read<"ehr.read", ACIR_EHRType>;
def ACIR_QueueEmptyOp : ACIR_Read<"queue.empty", ACIR_QueueType, I1>;
def ACIR_QueueFullOp : ACIR_Read<"queue.full", ACIR_QueueType, I1>;
def ACIR_QueueSizeOp : ACIR_Read<"queue.size", ACIR_QueueType, I32>;
def ACIR_PopOp : ACIR_Op<"queue.pop", [MemoryEffects<[MemRead, MemWrite]>]> {
  let arguments = (ins ACIR_QueueType:$resource);
  let assemblyFormat = "$resource attr-dict `:` qualified(type($resource))";
}
def ACIR_PushOp : ACIR_Write<"queue.push", ACIR_QueueType>;
def ACIR_CellWriteOp : ACIR_Write<"cell.write", ACIR_CellType>;
def ACIR_SignalSetOp : ACIR_Write<"signal.set", ACIR_SignalType>;
def ACIR_EHRWriteOp : ACIR_Write<"ehr.write", ACIR_EHRType>;
def ACIR_ConfigOp : ACIR_Op<"config", [Pure]> {
  let arguments = (ins StrAttr:$name);
  let results = (outs AnyType:$value);
  let assemblyFormat = "$name attr-dict `:` type($value)";
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

def ACIR_AssertOp : ACIR_Op<"assert", [MemoryEffects<[MemWrite]>]> {
  let arguments = (ins I1:$condition);
  let results = (outs I1:$checked);
  let assemblyFormat = "$condition attr-dict";
}
// Executable transaction and RTL graph: explicit validity, path and fire operands.
def ACIR_CircuitOp : ACIR_Op<"circuit", [SymbolTable, NoTerminator]> {
  let regions = (region AnyRegion:$body);
  let assemblyFormat = "attr-dict-with-keyword $body";
}
def ACIR_StateOp : ACIR_Op<"state", [Symbol]> {
  let arguments = (ins SymbolNameAttr:$sym_name, TypeAttr:$payload_type);
  let assemblyFormat = "$sym_name `:` $payload_type attr-dict";
}
def ACIR_ReadOp : ACIR_Op<"read", [MemoryEffects<[MemRead]>]> {
  let arguments = (ins Variadic<AnyType>:$inputs);
  let results = (outs AnyType:$value, I1:$ready);
  let assemblyFormat = "`(` $inputs `)` attr-dict `:` functional-type(operands, results)";
}
def ACIR_FireOp : ACIR_Op<"fire", [Pure]> {
  let arguments = (ins I1:$condition, StrAttr:$name);
  let results = (outs I1:$fire);
  let assemblyFormat = "$name $condition attr-dict";
}
def ACIR_EffectOp : ACIR_Op<"effect", [MemoryEffects<[MemWrite]>]> {
  let arguments = (ins I1:$enable, Variadic<AnyType>:$inputs);
  let assemblyFormat = "$enable `(` $inputs `)` attr-dict `:` functional-type(operands, results)";
}
def ACIR_PathOp : ACIR_Op<"path"> {
  let arguments = (ins I1:$condition);
  let assemblyFormat = "$condition attr-dict";
}
def ACIR_CurrentOp : ACIR_Op<"current", [MemoryEffects<[MemRead]>]> {
  let results = (outs AnyType:$value);
  let assemblyFormat = "attr-dict `:` type($value)";
}
def ACIR_NextOp : ACIR_Op<"next", [MemoryEffects<[MemWrite]>]> {
  let arguments = (ins AnyType:$value);
  let assemblyFormat = "$value attr-dict `:` type($value)";
}
#endif
