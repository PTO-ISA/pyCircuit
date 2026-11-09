#ifndef PYCIRCUIT_STRUCTURAL_CPP_BACKEND_H
#define PYCIRCUIT_STRUCTURAL_CPP_BACKEND_H

#include "ConcreteDesignPlan.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/Support/raw_ostream.h"

#include <string>

namespace acir::compiler {

#ifdef PYCIRCUIT_ENABLE_FINAL_ACIR_CPP_TEST_ADAPTER
// Test-only migration oracle. Production tools and installed SDKs never see
// this final-ACIR entrypoint.
mlir::LogicalResult verifyStructuralCppInput(mlir::ModuleOp package,
                                             ac::HardwareAnalysis &analysis);
mlir::LogicalResult emitStructuralCpp(mlir::ModuleOp package,
                                      ac::HardwareAnalysis &analysis,
                                      llvm::raw_ostream &out);
#endif

/// Primary independent-HWIR admission and plan construction entrypoint.
mlir::FailureOr<ConcreteDesignPlan>
prepareStructuralCpp(mlir::ModuleOp hwirPackage);

// Structural C++ emission entrypoint. It is intentionally fail-closed for
// operations and primitives outside its approved concrete mappings.
mlir::LogicalResult emitStructuralCpp(mlir::ModuleOp hwirPackage,
                                      llvm::raw_ostream &out);

// Render one already materialized plan. Publication callers use this overload
// when the same plan also supplies manifest and source ownership metadata.
mlir::LogicalResult emitStructuralCpp(const ConcreteDesignPlan &plan,
                                      llvm::raw_ostream &out);

// Emit only shared declarations for a multi-file publication bundle.
mlir::LogicalResult emitStructuralCppHeader(const ConcreteDesignPlan &plan,
                                            llvm::raw_ostream &out);

// Emit the public typed host API into the same header as the structural
// design. Authored drivers include design.hpp; simulation_main.cpp only owns
// the default CLI entrypoint.
mlir::LogicalResult emitStructuralCppHostApi(const ConcreteDesignPlan &plan,
                                             llvm::raw_ostream &out);

// Emit the out-of-line definitions owned by one ConcreteSourceUnitPlan.
mlir::LogicalResult emitStructuralCppSourceUnit(const ConcreteDesignPlan &plan,
                                                unsigned sourceUnit,
                                                llvm::raw_ostream &out);

// One independently compilable group of noinline Work helpers.  Large source
// units are distributed over several shards so one generated module cannot
// recreate a monolithic ARM64 text atom or monopolize one optimizer process.
struct StructuralCppWorkShard {
  unsigned sourceUnit = 0;
  unsigned shard = 0;
  std::string path;
  std::string text;
};

mlir::FailureOr<llvm::SmallVector<StructuralCppWorkShard, 0>>
emitStructuralCppWorkShards(const ConcreteDesignPlan &plan);

} // namespace acir::compiler

#endif // PYCIRCUIT_STRUCTURAL_CPP_BACKEND_H
