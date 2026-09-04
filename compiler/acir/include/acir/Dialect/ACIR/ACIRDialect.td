include "mlir/IR/DialectBase.td"

def ACIR_Dialect : Dialect {
  let name = "ac";
  let cppNamespace = "::acir::ac";
  let summary = "Agentic Circuit architecture dialect";
  let useDefaultAttributePrinterParser = 1;
  let useDefaultTypePrinterParser = 1;
}
