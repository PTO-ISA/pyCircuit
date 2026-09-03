#ifndef ACIR_DIALECT_ACIR_ACIRINTERFACES_TD
#define ACIR_DIALECT_ACIR_ACIRINTERFACES_TD

include "mlir/IR/OpBase.td"

def ACIR_RecordLike : OpInterface<"RecordLikeOpInterface"> {
  let cppNamespace = "::acir::ac";
  let methods = [
    InterfaceMethod<"Returns ordered field dictionaries.", "::mlir::ArrayAttr",
                    "getRecordFields", (ins), [{
      return $_op.getFields();
    }]>
  ];
}

def ACIR_InterfaceContainer : OpInterface<"InterfaceContainerOpInterface"> {
  let cppNamespace = "::acir::ac";
  let methods = [
    InterfaceMethod<"Returns the declaration region containing roles and ports.",
                    "::mlir::Region &", "getInterfaceBody", (ins), [{
      return $_op.getBody();
    }]>
  ];
}

def ACIR_ProtocolContainer : OpInterface<"ProtocolContainerOpInterface"> {
  let cppNamespace = "::acir::ac";
  let methods = [
    InterfaceMethod<"Returns the finite-state protocol declaration region.",
                    "::mlir::Region &", "getProtocolBody", (ins), [{
      return $_op.getBody();
    }]>
  ];
}

def ACIR_ProcessRegion : OpInterface<"ProcessRegionOpInterface"> {
  let cppNamespace = "::acir::ac";
  let methods = [
    InterfaceMethod<"Returns the resumable SSACFG process region.",
                    "::mlir::Region &", "getProcessBody", (ins), [{
      return $_op.getBody();
    }]>,
    InterfaceMethod<"Returns the process execution kind.",
                    "::llvm::StringRef", "getProcessKind", (ins), [{
      return $_op.getKind();
    }]>
  ];
}

def ACIR_Observation : OpInterface<"ObservationOpInterface"> {
  let cppNamespace = "::acir::ac";
  let methods = [
    InterfaceMethod<"Returns true for operations barred from functional dataflow.",
                    "bool", "isObservationOnly", (ins), [{ return true; }]>
  ];
}

#endif
