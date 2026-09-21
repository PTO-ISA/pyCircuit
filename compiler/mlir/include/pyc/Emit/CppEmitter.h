#pragma once

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Support/LogicalResult.h"
#include "llvm/Support/raw_ostream.h"
#include "pyc/Dialect/PYC/PYCOps.h"

#include <string>

namespace pyc {

struct CppEmitterOptions {
  enum class SplitMode {
    None,
    Module,
  };

  SplitMode splitMode = SplitMode::None;
  unsigned shardThresholdLines = 120000;
  unsigned shardThresholdBytes = 4 * 1024 * 1024;
  // Chunk full-topology eval bodies to avoid mega-functions that are expensive
  // for downstream C++ compilers.
  unsigned evalTopoChunkNodes = 256;
  // Chunk fused comb helpers to avoid single mega-functions that dominate
  // downstream C++ TU cost even after file sharding.
  unsigned combChunkNodes = 256;
  std::string probePlanPath{};
};

::mlir::LogicalResult emitCpp(::mlir::ModuleOp module, ::llvm::raw_ostream &os,
                              const CppEmitterOptions &opts = {});

::mlir::LogicalResult emitCppFunc(::mlir::ModuleOp module, ::mlir::func::FuncOp f, ::llvm::raw_ostream &os,
                                  const CppEmitterOptions &opts = {});

// One finite-family module case as standalone C++: the split (--out-dir) build
// compiles every module family into its own translation unit.
::mlir::LogicalResult emitCppFamily(::mlir::ModuleOp module, ::pyc::FamilyOp family,
                                    ::llvm::raw_ostream &os,
                                    const CppEmitterOptions &opts = {});

} // namespace pyc
