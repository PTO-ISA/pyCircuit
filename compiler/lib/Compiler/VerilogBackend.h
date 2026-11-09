#ifndef PYCIRCUIT_VERILOG_BACKEND_H
#define PYCIRCUIT_VERILOG_BACKEND_H

#include "ConcreteDesignPlan.h"
#include "VerilogSourceParts.h"

namespace acir::compiler {

inline constexpr uint64_t kVerilogSourceShardTargetBytes = 1024 * 1024;

enum class VerilogExportMode { Managed, Physical };

struct VerilogDesignPlan {
  ConcreteDesignPlan design;
  VerilogExportMode mode = VerilogExportMode::Managed;
  struct PhysicalInitializer {
    unsigned moduleIndex = 0;
    unsigned childIndex = 0;
    llvm::SmallVector<uint64_t, 2> words;
    llvm::SmallVector<uint64_t, 2> knownWords;
    llvm::SmallVector<uint64_t, 2> zWords;
  };
  // Physical startup parameters are closed constants materialized during
  // admission. The renderer must never wire a live SSA net to pyc_initial.
  llvm::SmallVector<PhysicalInitializer, 0> physicalInitializers;
};

/// Admit one verified independent-HWIR package and materialize every fact the
/// RTL renderer consumes. No final-ACIR analysis is available after this call.
mlir::FailureOr<VerilogDesignPlan>
prepareVerilog(mlir::ModuleOp hwirPackage,
               VerilogExportMode mode = VerilogExportMode::Managed);

mlir::FailureOr<VerilogSourceParts>
emitVerilog(const VerilogDesignPlan &plan,
            uint64_t sourceShardTargetBytes = kVerilogSourceShardTargetBytes);

mlir::LogicalResult emitVerilog(const VerilogDesignPlan &plan,
                                llvm::raw_ostream &out);

} // namespace acir::compiler

#endif // PYCIRCUIT_VERILOG_BACKEND_H
