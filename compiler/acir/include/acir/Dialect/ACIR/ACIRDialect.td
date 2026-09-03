include "mlir/IR/DialectBase.td"

def ACIR_Dialect : Dialect {
  let name = "ac";
  let cppNamespace = "::acir::ac";
  let summary = "Agentic Circuit architecture dialect";
  let useDefaultTypePrinterParser = 1;
}
