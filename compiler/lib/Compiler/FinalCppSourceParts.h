#ifndef PYCIRCUIT_FINAL_CPP_SOURCE_PARTS_H
#define PYCIRCUIT_FINAL_CPP_SOURCE_PARTS_H
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include <string>
#include <vector>
namespace acir::compiler {
struct FinalCppSourceGroup {
  mlir::DictionaryAttr sourceOwner;
  std::string headerPath, sourcePath, header, source;
};
struct FinalCppSourceParts {
  std::string supportHeader, systemHeader, rootCppName, rootHeaderPath;
  std::vector<FinalCppSourceGroup> sourceGroups;
  std::string simulationMain, simulationConfig;
};
mlir::FailureOr<FinalCppSourceParts> emitCppSourceParts(
    mlir::ModuleOp package, ac::HardwareAnalysis &analysis);
mlir::LogicalResult emitCpp(mlir::ModuleOp package,
    ac::HardwareAnalysis &analysis, llvm::raw_ostream &out);
} // namespace acir::compiler
#endif
