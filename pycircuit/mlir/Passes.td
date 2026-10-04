include "mlir/Pass/PassBase.td"
def AnalyzeResources : Pass<"acir-analyze-resources", "::mlir::ModuleOp"> {
  let summary = "Resolve possible Queue/Signal identities through calls, aliases and CFG edges";
  let constructor = "::acir::createAnalyzeResourcesPass()";
}
def LowerGFSim : Pass<"acir-lower-gfsim", "::mlir::ModuleOp"> {
  let dependentDialects = ["::mlir::emitc::EmitCDialect", "::mlir::cf::ControlFlowDialect", "::mlir::func::FuncDialect"];
  let summary = "Expand Rule lifecycle and branch-local missing-input exits";
  let constructor = "::acir::createLowerGFSimPass()";
}
def ConvertToEmitC : Pass<"acir-convert-to-emitc", "::mlir::ModuleOp"> {
  let dependentDialects = ["::mlir::emitc::EmitCDialect", "::mlir::cf::ControlFlowDialect", "::mlir::func::FuncDialect"];
  let summary = "Convert typed circuit operations and integer semantics to EmitC";
  let constructor = "::acir::createConvertToEmitCPass()";
}
