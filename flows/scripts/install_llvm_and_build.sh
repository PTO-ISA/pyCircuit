#!/usr/bin/env bash
# 安装 LLVM（含 MLIR）+ 构建 pyCircuit 的 pycc
# 在终端中执行: bash flows/scripts/install_llvm_and_build.sh

set -euo pipefail
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd -- "${SCRIPT_DIR}/../.." && pwd)"
source "${SCRIPT_DIR}/lib.sh"

echo "[1/3] 安装 LLVM 22、CMake、Ninja（Homebrew，LLVM 体积较大请耐心等待）..."
HOMEBREW_NO_AUTO_UPDATE=1 brew install llvm@22 cmake ninja

echo "[2/3] 设置 PATH 以便找到 llvm-config..."
# Apple Silicon 用 /opt/homebrew，Intel 用 /usr/local
if [[ -x /opt/homebrew/opt/llvm@22/bin/llvm-config ]]; then
  export PATH="/opt/homebrew/opt/llvm@22/bin:$PATH"
elif [[ -x /usr/local/opt/llvm@22/bin/llvm-config ]]; then
  export PATH="/usr/local/opt/llvm@22/bin:$PATH"
else
  pyc_die "未找到 LLVM 22 llvm-config，请确认 brew install llvm@22 已成功完成"
fi

echo "[3/3] 构建 pycc..."
cd "${ROOT_DIR}"
flows/scripts/pyc build

echo "完成。可运行: flows/scripts/pyc test"
