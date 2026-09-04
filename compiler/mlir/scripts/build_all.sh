#!/usr/bin/env bash
set -euo pipefail

PYC_REPO_ROOT="${PYC_REPO_ROOT:-$(cd "$(dirname "$0")/../../.." && pwd)}"
PYC_BUILD_DIR="${PYC_BUILD_DIR:-$PYC_REPO_ROOT/compiler/mlir/build}"
SUPERPROJECT_ROOT="$(cd "$PYC_REPO_ROOT/../.." && pwd)"
if [[ -z "${LLVM_PROJECT_DIR:-}" ]]; then
  if [[ -d "$SUPERPROJECT_ROOT/compiler/llvm/llvm" ]]; then
    LLVM_PROJECT_DIR="$SUPERPROJECT_ROOT/compiler/llvm"
  else
    echo "error: standalone builds require explicit LLVM_PROJECT_DIR" >&2
    exit 1
  fi
fi
LLVM_BUILD_DIR="${LLVM_BUILD_DIR:-${TMPDIR:-/tmp}/pyc-llvm-build}"

if [[ ! -d "$LLVM_PROJECT_DIR/llvm" ]]; then
  echo "error: LLVM_PROJECT_DIR does not look like llvm-project: $LLVM_PROJECT_DIR" >&2
  exit 1
fi

cmake -G Ninja -S "$LLVM_PROJECT_DIR/llvm" -B "$LLVM_BUILD_DIR" \
  -DLLVM_ENABLE_PROJECTS=mlir \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  -DCMAKE_BUILD_TYPE=Release

ninja -C "$LLVM_BUILD_DIR" mlir-opt

cmake -G Ninja -S "$PYC_REPO_ROOT/compiler/mlir" -B "$PYC_BUILD_DIR" \
  -DMLIR_DIR="$LLVM_BUILD_DIR/lib/cmake/mlir" \
  -DLLVM_DIR="$LLVM_BUILD_DIR/lib/cmake/llvm"

ninja -C "$PYC_BUILD_DIR" pyc-opt pycc

echo "Built:"
echo "  mlir-opt:    $LLVM_BUILD_DIR/bin/mlir-opt"
echo "  pyc-opt:     $PYC_BUILD_DIR/bin/pyc-opt"
echo "  pycc: $PYC_BUILD_DIR/bin/pycc"
