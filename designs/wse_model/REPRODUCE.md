# 复现编译问题

## 选择要复现的工具链

历史大函数与 ARM64 链接失败使用 LLVM/MLIR 22.1.8、pyCircuit 6.1.0 的本地提交 `505edbc7dc2429640a41a31385ce3ff951c02aad`。它是在 `3ee29b077725638081b69dcb0b267979ade07f1f` 上增加静态参数循环支持及分析预算后的版本。这些本地提交未作为本 PR 的编译器修改提交。

本 PR 的基点为上游 main `8b6eb3c46f78a44babb33945072f744f64c2f01f`。该基点的整条原生链路未在本次发布中复测；直接使用 main 工具链可能在静态循环或分析预算处更早失败。复现历史大 `Work()` 问题时，建议先重建下面的历史源树。若正在调查新编译器，也可以直接向复现脚本传入新安装目录，它会记录实际工具链版本与失败阶段。

## 重建历史编译器源树

以下命令从 pyCircuit 仓库根目录执行，输出目录必须不存在：

```bash
python3 designs/wse_model/reproduction/prepare_compiler.py \
  --repository "$PWD" \
  --output "$PWD/.pycircuit_out/wse-historical-source"
```

脚本在独立 worktree 上应用 `compiler-prerequisites.mbox.gz` 中的 5 个源提交，并验证最终 Git tree 等于 `1faf5a74a9a26b1e6f1aa9f07ca15d3e0c14dcc8`。提交人时间不同会产生新的 commit SHA，但被编译的源树与历史版本相同。补丁串是可移植的源输入，不是已构建工具链，也不会改当前检出的编译器文件。

准备 LLVM/MLIR 22.1.8、CMake ≥3.25、Ninja、Python ≥3.11、支持 C++20 的宿主编译器。按仓库[安装说明](../../docs/getting-started/installation.md)取得依赖；下面的 `LLVM_PREFIX` 必须指向你的 LLVM/MLIR 22.1.8 安装：

```bash
export LLVM_PREFIX=/absolute/path/to/LLVM-22.1.8
export WSE_COMPILER_SOURCE="$PWD/.pycircuit_out/wse-historical-source"
export WSE_COMPILER_BUILD="$PWD/.pycircuit_out/wse-historical-build"
export WSE_COMPILER_INSTALL="$PWD/.pycircuit_out/wse-historical-install"

cmake -S "$WSE_COMPILER_SOURCE" -B "$WSE_COMPILER_BUILD" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_DIR="$LLVM_PREFIX/lib/cmake/llvm" \
  -DMLIR_DIR="$LLVM_PREFIX/lib/cmake/mlir" \
  -DCMAKE_INSTALL_PREFIX="$WSE_COMPILER_INSTALL"
cmake --build "$WSE_COMPILER_BUILD" --parallel 4
cmake --install "$WSE_COMPILER_BUILD"
```

macOS 历史环境使用 ARM64、Clang/LLD 22.1.8、macOS SDK 26.5 和 `LDFLAGS=-fuse-ld=lld`。若要复现相同链接器诊断，使用这组宿主条件，并确保 `CC`/`CXX` 指向该 LLVM 的 clang/clang++，`SDKROOT` 指向本机 SDK。其他平台仍可检查生成函数规模，但不保证出现相同链接错误。

在 macOS 上复现历史宿主条件时，在配置、构建工具链和运行复现脚本之前设置：

```bash
export CC="$LLVM_PREFIX/bin/clang"
export CXX="$LLVM_PREFIX/bin/clang++"
export SDKROOT=/absolute/path/to/MacOSX26.5.sdk
export MACOSX_DEPLOYMENT_TARGET=26.0
export LDFLAGS=-fuse-ld=lld
```

## 从 Python 生成 ACIR、C++ 并使用 O0 编译

在同一个 shell 中执行。Linux 可按本机允许的栈限制调整 `ulimit`；历史 macOS 使用 65520 KiB。使用新输出目录，以保留每次失败：

```bash
ulimit -s 65520
export CMAKE_BUILD_PARALLEL_LEVEL=1
export WSE_OUTPUT="$PWD/.pycircuit_out/wse-repro-$(date +%Y%m%d-%H%M%S)"

python3 designs/wse_model/reproduction/reproduce.py \
  --toolchain "$WSE_COMPILER_INSTALL" \
  --output "$WSE_OUTPUT" \
  --suite allgather-control \
  --stop-after executable \
  --timeout 21600
```

脚本通过公开 SDK 命令依次执行：

1. CMake 配置 `models/calendar`，选择带完整日志的 `AllGatherControlSystem`。
2. 源单元编译、显式链接成 `source/bench_allgather_control.ac`（ACIR，电路中间表示）。
3. `pycircuit emit … --target cpp -o …` 生成 C++，统计完整顶层 `Work()` 函数与子任务分派数。
4. 配置 `-O0 -DNDEBUG`，单任务编译并链接仿真可执行文件。

`--stop-after acir` 可停在最终 IR，`--stop-after cpp` 可停在 C++ 生成后。延迟场景改用 `--suite allgather-delayed` 并选择另一新目录。每阶段默认上限为 21600 秒；超时会终止该阶段进程组并保留日志。历史源链接和生成需要数小时，单个 clang 命令执行中 Ninja 日志可能长期没有新完成记录。

## 从已有 ACIR 或 C++ 单独重试

先按上一节生成 ACIR，再在新目录发射 C++。不要覆盖正在使用的目录：

```bash
"$WSE_COMPILER_INSTALL/bin/pycircuit" emit \
  "$WSE_OUTPUT/source/bench_allgather_control.ac" \
  --target cpp -o "$WSE_OUTPUT/cpp-retry" \
  > "$WSE_OUTPUT/emit-retry.log" 2>&1

cmake -S "$WSE_OUTPUT/cpp-retry" -B "$WSE_OUTPUT/native-retry" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -Dpycircuit_DIR="$WSE_COMPILER_INSTALL/share/pycircuit/cmake" \
  '-DCMAKE_CXX_FLAGS_RELEASE=-O0 -DNDEBUG'
cmake --build "$WSE_OUTPUT/native-retry" \
  --target pycircuit_sim --parallel 1 --verbose \
  > "$WSE_OUTPUT/build-retry.log" 2>&1
```

如果链接成功，可运行基线 523 周期、1 个工作线程：

```bash
"$WSE_OUTPUT/native/bin/pycircuit_sim" --cycles 523 --workers 1 \
  > "$WSE_OUTPUT/run.log" 2>&1
```

`native-retry` 构建对应运行路径应改为 `native-retry/bin/pycircuit_sim`。直接执行会运行生成的源级检查；完整逐成员参考比对仍由 `tools/verify_calendar.py` 负责。后者保留历史精确版本锁，重建历史树的新 commit SHA 与该锁不同，因此上面的结构/编译复现脚本不依赖这个旧版本锁校验器。

## 观察结果和日志

| 文件 | 内容 |
| --- | --- |
| `reproduction.json` | 实际工具链版本、输入摘要、命令、耗时、退出码、错误与 Work 统计 |
| `01-configure.log` | 源项目配置 |
| `02-acir.log` | 源编译与最终 ACIR 链接/发布验证 |
| `03-emit-cpp.log` | C++ 生成与输入验证 |
| `04-configure-cpp.log` | 宿主 O0 构建配置 |
| `05-build-cpp.log` | 真实 clang 和链接命令及错误 |
| `source/bench_allgather_control.ac` | 最终 ACIR |
| `cpp/sources/wse_calendar/bench_allgather_control.hpp` | 含顶层 Work 的生成头文件 |
| `native/bin/pycircuit_sim` | 最终链接成功后生成的可执行文件 |

历史上可观察到：子模块的 `Work()` 定义存在，但顶层没有独立分派它们，而是展开大段计算；O0 对象编译成功，ARM64 LLD 最终链接报 `thunk range overrun`。具体数值和关闭日志的历史对照见 [VALIDATION.md](VALIDATION.md)。这是待调查的编译器问题，本文不把它定义为设计必须保持的行为。
