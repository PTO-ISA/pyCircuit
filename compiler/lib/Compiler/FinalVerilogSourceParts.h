#ifndef PYCIRCUIT_FINAL_VERILOG_SOURCE_PARTS_H
#define PYCIRCUIT_FINAL_VERILOG_SOURCE_PARTS_H
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include <string>
#include <vector>
namespace acir::compiler {
struct FinalVerilogSourceGroup {
  mlir::DictionaryAttr sourceOwner;
  llvm::SmallVector<mlir::FlatSymbolRefAttr> definitions;
  std::string path, text;
};
struct FinalVerilogSourceParts {
  llvm::SmallVector<FinalVerilogSourceGroup, 0> sourceGroups;
  llvm::SmallVector<std::string> standardSources;
  std::string core, runtimeGlue, rootRtlName;
  std::string simulationTop;
};
mlir::FailureOr<FinalVerilogSourceParts> emitVerilogSourceParts(
    mlir::ModuleOp package, ac::HardwareAnalysis &analysis);
mlir::LogicalResult emitVerilog(mlir::ModuleOp package,
    ac::HardwareAnalysis &analysis, llvm::raw_ostream &out);
} // namespace acir::compiler
#endif
