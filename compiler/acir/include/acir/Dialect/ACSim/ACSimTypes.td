#ifndef ACIR_DIALECT_ACSIM_ACSIMTYPES_TD
#define ACIR_DIALECT_ACSIM_ACSIMTYPES_TD

include "acir/Dialect/ACSim/ACSimDialect.td"
include "mlir/IR/AttrTypeBase.td"

class ACSim_Type<string name, string typeMnemonic, list<Trait> traits = []>
    : TypeDef<ACSim_Dialect, name, traits> {
  let mnemonic = typeMnemonic;
}

class ACSim_SymbolType<string name, string mnemonic>
    : ACSim_Type<name, mnemonic> {
  let parameters = (ins "mlir::SymbolRefAttr":$symbol);
  let assemblyFormat = "`<` $symbol `>`";
}

def ACSim_ValueType : ACSim_SymbolType<"Value", "value">;
def ACSim_ExprType : ACSim_SymbolType<"Expr", "expr">;

def ACSim_OwnerType : ACSim_Type<"Owner", "owner"> {
  let parameters = (ins "mlir::SymbolRefAttr":$realization);
  let assemblyFormat = "`<` $realization `>`";
}

def ACSim_RefType : ACSim_Type<"Ref", "ref"> {
  let parameters = (ins "mlir::SymbolRefAttr":$realization);
  let assemblyFormat = "`<` $realization `>`";
}

def ACSim_PortType : ACSim_Type<"Port", "port"> {
  let parameters = (ins "mlir::SymbolRefAttr":$interface,
                        "mlir::SymbolRefAttr":$role,
                        "mlir::SymbolRefAttr":$payload,
                        "mlir::SymbolRefAttr":$protocol);
  let assemblyFormat =
      "`<` $interface `,` $role `,` $payload `,` $protocol `>`";
}

def ACSim_ResourceType : ACSim_Type<"Resource", "resource"> {
  let parameters = (ins "mlir::SymbolRefAttr":$resource,
                        "mlir::SymbolRefAttr":$role);
  let assemblyFormat = "`<` $resource `,` $role `>`";
}

def ACSim_ArrayType : ACSim_Type<"Array", "array"> {
  let parameters = (ins "mlir::DenseI64ArrayAttr":$shape,
                        "mlir::Type":$elementType);
  let hasCustomAssemblyFormat = 1;
  let genVerifyDecl = 1;
}

def ACSim_ObjectIdType : ACSim_Type<"ObjectId", "object_id">;
def ACSim_ActivationIdType : ACSim_Type<"ActivationId", "activation_id">;
def ACSim_PcType : ACSim_SymbolType<"Pc", "pc">;
def ACSim_WakeType : ACSim_SymbolType<"Wake", "wake">;

#endif // ACIR_DIALECT_ACSIM_ACSIMTYPES_TD
