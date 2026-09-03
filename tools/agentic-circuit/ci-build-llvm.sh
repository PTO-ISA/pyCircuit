#!/usr/bin/env bash
set -euo pipefail

: "${LLVM_RELEASE:?LLVM_RELEASE must be set}"

cmake -S ".cache/llvm-project-${LLVM_RELEASE}.src/llvm" \
  -B .cache/llvm-build \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_PROJECTS=mlir \
  -DLLVM_TARGETS_TO_BUILD=Native \
  -DLLVM_INCLUDE_TESTS=OFF \
  -DLLVM_INCLUDE_BENCHMARKS=OFF \
  -DLLVM_INCLUDE_EXAMPLES=OFF
# mlir-opt drives the MLIR package verification; FileCheck, not, split-file,
# and count are the lit substitutions used by
# tests/mlir/agentic-circuit/lit.cfg.py, so they must
# exist in the cached build outputs as well.
cmake --build .cache/llvm-build --target mlir-opt FileCheck not split-file count
