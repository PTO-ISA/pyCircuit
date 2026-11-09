#ifndef PYCIRCUIT_VERILOG_SOURCE_PARTS_H
#define PYCIRCUIT_VERILOG_SOURCE_PARTS_H

#include "mlir/IR/BuiltinAttributes.h"
#include "llvm/ADT/SmallVector.h"

#include <cstdint>
#include <string>

namespace acir::compiler {

struct VerilogSourceGroup {
  mlir::DictionaryAttr sourceOwner;
  llvm::SmallVector<mlir::FlatSymbolRefAttr> definitions;
  std::string path, text;
};

// Backend-owned transport produced only from an admitted VerilogDesignPlan.
struct VerilogSourceParts {
  llvm::SmallVector<VerilogSourceGroup, 0> sourceGroups;
  llvm::SmallVector<std::string> standardSources;
  std::string core, runtimeGlue, rootRtlName;
  std::string simulationTop;
  uint64_t sourceCheckCount = 0;
};

} // namespace acir::compiler

#endif // PYCIRCUIT_VERILOG_SOURCE_PARTS_H
