# 编译器构建与使用

编译流程为 ACPy AST → ACIR MLIR → 条件化 RuleGraph。当前支持静态编译、独立重载和依赖分析，不生成仿真代码。

核心抽象的修订草稿见 [pyCircuit 核心抽象设计](docs/core-design.md)。目标语义见 [ACIR 基础设计](../acir/basic-design.md)，当前支持的前端写法见 [ACPy 前端](../acpy/spec.md)，当前保存格式见 [ACIR v6](acir.md)。基础设计中的原生 Module／Rule／Method 等结构尚未全部实现。

## 构建与测试

本机使用 Clang 22、GCC 14 sysroot 和 LLVM/MLIR 22：

```bash
export LD_LIBRARY_PATH=/home/lc/opt/gcc14/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
cmake -S pycircuit -B pycircuit/builds/compiler -G Ninja \
  -DCMAKE_C_COMPILER=/home/lc/opt/pycircuit-dev/bin/cc \
  -DCMAKE_CXX_COMPILER=/home/lc/opt/pycircuit-dev/bin/c++ \
  -DMLIR_DIR=/home/lc/opt/llvm-22.1.8/lib/cmake/mlir \
  -DCMAKE_BUILD_TYPE=Release
cmake --build pycircuit/builds/compiler -j 6
ctest --test-dir pycircuit/builds/compiler --output-on-failure
```

也可以使用其他兼容 MLIR 22 的工具链。

## 编译与重载

```bash
export ACPY_MLIR_COMPILER="$PWD/pycircuit/builds/compiler/mlir/acir-compile"
python -m pycircuit compile pycircuit/tests/models/transfer.py --top Top \
  --output pycircuit/builds/benchmarks/rule-graph/transfer
python -m pycircuit analyze \
  pycircuit/builds/benchmarks/rule-graph/transfer/model.acir.mlir \
  --output pycircuit/builds/benchmarks/rule-graph/reloaded
```

`ACPY_MLIR_COMPILER` 可省略，此时从 PATH 查找 `acir-compile`。`--no-opt` 关闭 canonicalize/CSE。重载不依赖 Python 源码，也不使用旧分析图恢复计算。

| 产物 | 用途 |
| --- | --- |
| `model.acir.mlir` | 程序与源码位置，可独立重载 |
| `model.readable.mlir` | 隐藏调试位置的阅读版，重载后不保留原操作位置 |
| `rule-graph.json` | 完整分析图、条件和来源 |
| `rule-graph.dot` | Rule 顺序图 |

完整 RuleGraph 不嵌入 MLIR。发现已确认依赖环或无法证明的顺序时，CLI 返回非零，并保留带诊断的产物。当前不提供运行时调度、竞争写入检查或 CPU 时序验证。

## 示例与可视化

源码、模型意图及对应测试见 [示例说明](examples/README.md)。当前五级流水线和 OoO 调度的产物分别位于 `builds/riscv_pipeline/` 和 `builds/ooo_scheduler/`。

```bash
python pycircuit/draw_rules.py pycircuit/builds/riscv_pipeline/rule-graph.json
python pycircuit/draw_rules.py pycircuit/builds/ooo_scheduler/rule-graph.json
```

脚本使用 Python 标准库和 Graphviz。默认在输入 JSON 目录生成 `module-rule-graph.svg` 和 `module-rule-graph.dot`。箭头表示同周期必要顺序，不等同于 payload 流动方向；条件关系的完整信息保留在 JSON 和 SVG 悬停提示中。

`-o PATH` 支持 SVG、PNG、PDF 或 DOT；DOT 输出无需 Graphviz。`--rankdir TB` 改为纵向布局，`--hide-empty-modules` 隐藏没有 Rule 的组件。可通过 `--dot PATH` 或 `ACPY_GRAPHVIZ_DOT` 指定 Graphviz；脚本也查找仓库本地环境和 PATH。

新建本地 Graphviz 环境：

```bash
CONDA_PKGS_DIRS="$PWD/pycircuit/builds/rule-graphviz/pkgs" \
  /home/lc/opt/miniforge3/bin/conda env create \
  --prefix "$PWD/pycircuit/builds/rule-graphviz/env" \
  --file pycircuit/graphviz-environment.yml
```

外部参考源码位于根目录 `reference/`，构建位于 `pycircuit/builds/`，实验、日志和测量位于 `pycircuit/builds/benchmarks/`。
