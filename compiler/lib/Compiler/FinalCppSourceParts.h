#ifndef PYCIRCUIT_FINAL_CPP_SOURCE_PARTS_H
#define PYCIRCUIT_FINAL_CPP_SOURCE_PARTS_H
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include <cstdint>
#include <string>
#include <vector>
namespace acir::compiler {
class HardwareEmitContext;

struct FinalCppSourceGroup {
  mlir::DictionaryAttr sourceOwner;
  std::string headerPath, sourcePath, header, source;
};
struct FinalCppSourceParts {
  std::string supportHeader, systemHeader, rootCppName, rootHeaderPath;
  std::vector<FinalCppSourceGroup> sourceGroups;
  std::string simulationMain, simulationConfig, portMap;
  uint64_t sourceCheckCount = 0;
};
mlir::LogicalResult emitCppRecords(HardwareEmitContext &context,
                                   llvm::raw_ostream &out);
void appendCppDutHeader(FinalCppSourceParts &parts,
                        llvm::ArrayRef<std::string> inputs,
                        llvm::ArrayRef<std::string> outputs,
                        llvm::StringRef configure, bool hasChecks, bool observes);
void addCppSystemDriver(FinalCppSourceParts &parts, llvm::StringRef clock,
                        llvm::StringRef reset, mlir::ArrayAttr samples,
                        bool resetIfOne,
                        llvm::ArrayRef<std::string> sampledInputs = {});
mlir::FailureOr<FinalCppSourceParts> emitCppSourceParts(
    mlir::ModuleOp package, ac::HardwareAnalysis &analysis);
mlir::LogicalResult emitCpp(mlir::ModuleOp package,
    ac::HardwareAnalysis &analysis, llvm::raw_ostream &out);
} // namespace acir::compiler
#endif
