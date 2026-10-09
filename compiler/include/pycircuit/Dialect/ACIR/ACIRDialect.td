include "mlir/IR/DialectBase.td"

def ACIR_Dialect : Dialect {
  let name = "ac";
  let cppNamespace = "::acir::ac";
  let summary = "pycircuit common hardware architecture dialect";
  let description = [{
    Hardware modules form a hierarchy of reusable definitions and fixed
    instances. Typed SSA values are wires: module inputs, instance results,
    internal connections and module outputs carry the same hardware types.
    Port direction belongs to the module signature, not to the payload type.

    Bits and packed structs describe signal data. Combinational rules compute
    signals; storage belongs to leaf module instances. A module instance is
    structural hardware, not a software call or an implicit copy of its callee.
    Source mathematical values and compile-time parameters are resolved before
    they can become hardware signals.
  }];
  let useDefaultAttributePrinterParser = 1;
  let useDefaultTypePrinterParser = 1;
  let hasOperationAttrVerify = 1;
}
