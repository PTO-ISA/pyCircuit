#ifndef ACIR_DIALECT_ACIR_ACIRTYPES_TD
#define ACIR_DIALECT_ACIR_ACIRTYPES_TD

include "acir/Dialect/ACIR/ACIRAttributes.td"
include "mlir/IR/AttrTypeBase.td"
include "mlir/Interfaces/DataLayoutInterfaces.td"

class ACIR_Type<string name, string typeMnemonic, list<Trait> traits = []>
    : TypeDef<ACIR_Dialect, name, traits> {
  let mnemonic = typeMnemonic;
}

class ACIR_NamedType<string name, string mnemonic, list<Trait> traits = []>
    : ACIR_Type<name, mnemonic, traits> {
  let parameters = (ins "mlir::SymbolRefAttr":$name);
  let assemblyFormat = "`<` $name `>`";
}

class ACIR_DataNamedType<string name, string mnemonic, list<Trait> traits = []>
    : ACIR_NamedType<name, mnemonic, traits> {
  let genVerifyDecl = 1;
}

class ACIR_LayoutNamedType<string name, string mnemonic>
    : ACIR_DataNamedType<name, mnemonic, [
        DeclareTypeInterfaceMethods<DataLayoutTypeInterface,
                                    ["verifyEntries", "getPreferredAlignment"]>
      ]>;

def ACIR_StructType : ACIR_LayoutNamedType<"Struct", "struct">;
def ACIR_PacketType : ACIR_LayoutNamedType<"Packet", "packet">;
def ACIR_TransactionType : ACIR_DataNamedType<"Transaction", "transaction">;
def ACIR_EnumType : ACIR_LayoutNamedType<"Enum", "enum">;
def ACIR_ResourceTokenType
    : ACIR_NamedType<"ResourceToken", "resource_token">;

// Compiler-owned speculative identity.  These are deliberately nominally
// distinct from ordinary packed integers even though lowering may choose a
// packed physical representation.
def ACIR_TransactionRefType : ACIR_Type<"TransactionRef", "transaction_ref"> {
  let parameters = (ins "uint64_t":$slotWidth,
                        "uint64_t":$generationWidth,
                        "uint64_t":$epochWidth);
  let assemblyFormat = "`<` $slotWidth `,` $generationWidth `,` $epochWidth `>`";
  let genVerifyDecl = 1;
}

def ACIR_ExecutionAttemptType
    : ACIR_Type<"ExecutionAttempt", "execution_attempt"> {
  let parameters = (ins "::mlir::Type":$transaction,
                        "uint64_t":$attemptWidth);
  let assemblyFormat = "`<` qualified($transaction) `,` $attemptWidth `>`";
  let genVerifyDecl = 1;
}

// A proof-carrying unsigned closed range.  The Python surface is half-open;
// `ac.range[lo, hi]` lowers to `!ac.range<lo, hi - 1>` so the full u64 domain
// remains representable without a 2**64 type parameter.
def ACIR_RangeType : ACIR_Type<"Range", "range"> {
  let parameters = (ins "uint64_t":$lower, "uint64_t":$upper);
  let assemblyFormat = "`<` $lower `,` $upper `>`";
  let genVerifyDecl = 1;
}

// An immutable fixed-length payload value. This is deliberately distinct from
// ac.array, which is a static collection of Queue/Var topology objects.
def ACIR_ValueArrayType : ACIR_Type<"ValueArray", "value_array"> {
  let parameters = (ins "int64_t":$length, "mlir::Type":$elementType);
  let assemblyFormat = "`<` $length `x` $elementType `>`";
  let genVerifyDecl = 1;
}

def ACIR_VarType : ACIR_Type<"Var", "var"> {
  let parameters = (ins "mlir::Type":$elementType);
  let assemblyFormat = "`<` $elementType `>`";
  let genVerifyDecl = 1;
}

def ACIR_QueueType : ACIR_Type<"Queue", "queue"> {
  let parameters = (ins "mlir::Type":$elementType,
                        DefaultValuedParameter<"int64_t", "1">:$lanes,
                        DefaultValuedParameter<"int64_t", "1">:$rate);
  let assemblyFormat = [{
    `<` $elementType (`,` `lanes` `=` $lanes^)? (`,` `rate` `=` $rate^)? `>`
  }];
  let genVerifyDecl = 1;
}

def ACIR_ArrayType : ACIR_Type<"Array", "array"> {
  let parameters = (ins "int64_t":$length, "mlir::Type":$elementType);
  let assemblyFormat = "`<` $length `x` $elementType `>`";
  let genVerifyDecl = 1;
}

def ACIR_FlowType : ACIR_Type<"Flow", "flow"> {
  let parameters = (ins "mlir::Type":$elementType,
                        "mlir::FlatSymbolRefAttr":$protocol);
  let assemblyFormat = "`<` $elementType `,` $protocol `>`";
  let genVerifyDecl = 1;
}

def ACIR_EndpointType : ACIR_Type<"Endpoint", "endpoint"> {
  let parameters = (ins "mlir::FlatSymbolRefAttr":$interface,
                        "mlir::FlatSymbolRefAttr":$role);
  let assemblyFormat = "`<` $interface `,` $role `>`";
}

def ACIR_ResourceRefType : ACIR_Type<"ResourceRef", "resource_ref"> {
  let parameters = (ins "mlir::FlatSymbolRefAttr":$resourceType,
                        "mlir::FlatSymbolRefAttr":$role);
  let assemblyFormat = "`<` $resourceType `,` $role `>`";
}

def ACIR_ChannelType : ACIR_Type<"Channel", "channel"> {
  let parameters = (ins "mlir::Type":$elementType,
                        "mlir::FlatSymbolRefAttr":$protocol);
  let assemblyFormat = "`<` $elementType `,` $protocol `>`";
  let genVerifyDecl = 1;
}

def ACIR_EventType : ACIR_Type<"Event", "event"> {
  let parameters = (ins "mlir::Type":$elementType);
  let assemblyFormat = "`<` $elementType `>`";
  let genVerifyDecl = 1;
}

#endif // ACIR_DIALECT_ACIR_ACIRTYPES_TD
