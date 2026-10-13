# 编译器构建与使用

编译链路：ACPy → 源语义 MLIR → 事务 MLIR → RTL 友好 MLIR → 逐拍轨迹。
语义见 [核心设计](docs/core-design.md)，阶段进度与问题见 [路线图](docs/compiler-roadmap.md)。

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
export ACPY_MLIR_COMPILER="$PWD/pycircuit/builds/compiler/mlir/acir-compile"
python -m pycircuit.tests.run_acceptance
```

Python 仅解析 ACPy 和调用编译器。事务提取、条件传播、契约／端口分析、调度、RTL lowering
以及两个通用执行器直接读取 MLIR，在 C++ 中完成。JSON 仅为分析／执行报告。
验收脚本复现 8 个示例的 32 拍执行、优化开关和三阶段重载，并更新示例输出。
轨迹与汇总位于 `builds/benchmarks/compiler-acceptance/`；回归包含独立 ISA 和定向周期预期。

## 编译、阶段输出与执行

```bash
python -m pycircuit compile pycircuit/examples/riscv_pipeline/model.py --top Pipeline \
  --output pycircuit/builds/benchmarks/cpu --cycles 32
python -m pycircuit run pycircuit/builds/benchmarks/cpu/model.transaction.mlir \
  --output pycircuit/builds/benchmarks/cpu-reload --cycles 32
python -m pycircuit run pycircuit/builds/benchmarks/cpu/model.rtl.mlir \
  --output pycircuit/builds/benchmarks/rtl-reload --cycles 32
```

`compile` 接受 ACPy；`analyze` 和 `run` 接受独立 MLIR 文件。`--stop-after source|transaction|rtl`
控制最后阶段，默认 rtl。`--cycles N` 执行 N 拍；从源文件开始时会保存事务和 RTL 两种轨迹。
`--config NAME=VALUE` 为 top 的构造参数提供静态配置，可重复；布尔值使用 0／1。
`--no-opt` 关闭 canonicalize／CSE。重载不需要 Python 源码、图 JSON 或其他阶段文件。

| 文件 | 内容 |
| --- | --- |
| `model.source.mlir` | v8 静态实例、组件、统一 Rule 的 guard／body、SSA、SCF／CFG、契约 |
| `model.transaction.mlir` | 每调用点独立 fire、路径条件、条件化效果、共享 SSA、固定端口、后继与无环顺序 |
| `model.rtl.mlir` | 完整组合 fire／选择器、固定 Queue 存储和占用、current／next-state；Signal 为组合 data／valid |
| `rule-graph.json` | 事务节点、分支边、端口竞争和固定调度的查看报告 |
| `transaction-trace.json`、`rtl-trace.json` | 每拍 fire 与提交后的状态；Signal 不列为跨拍状态 |

`model.acir.mlir` 是源层文件的同内容别名。v7 及更早文件需从 ACPy 重新编译。
也可直接使用原生 pass：

```bash
pycircuit/builds/compiler/mlir/acir-opt model.source.mlir --acir-extract-transactions
pycircuit/builds/compiler/mlir/acir-opt model.transaction.mlir --acir-lower-rtl
```

构造配置保存于源 MLIR 的 `acir.config`；直接运行 pass 时必须已有完整配置。
错误返回非零，说明资源、事务或契约原因。所有分支顺序边的节点合并图必须无环。

## 语义与边界

统一 `@ac.rule` 由实际效果推导只读／修改分类。Module body 中每个修改性调用点建立事务，
嵌套调用共享外层事务。失败的读取／guard 阻止相关分支执行；分支内要求只约束所选路径，
合流后的无关事务保持独立。同事务重复消费按真实 Queue 合并。

Cell 读取旧值；Queue 满时允许前序成功 pop 腾空间，新 push 数据下拍可读；
Signal 只在本拍成功 set 后有效；EHR 读取固定端口之前的成功写入。
EHR 端口不能因任意拓扑排序而增加未声明可见性。竞争报告不进行自动仲裁，
未处理的端口竞争结果不保证。RTL 层已无 Rule 调用、隐式就绪条件或待定调度。

固定 Queue 数组支持运行时下标选择，Cell 支持字段／数组更新，字面量固定循环静态展开。
依赖运行时数据、无法静态展开的循环保留在源层，但事务／RTL lowering 明确诊断循环 CFG；
当前执行目标是有限组合逻辑。无 SystemVerilog 后端、激活调度或跨拍缓存优化。

## 示例与可视化

[示例说明](examples/README.md) 包含 CPU、OoO、共享计算与诊断模型。

```bash
python pycircuit/draw_rules.py pycircuit/builds/riscv_pipeline/rule-graph.json
```

绘图使用 Graphviz；`-o PATH.dot` 无需安装 Graphviz。JSON 和图均不参与后续编译。
外部源码位于 `reference/<项目名>/`，构建位于 `builds/`，新实验输入、日志与结果位于
`builds/benchmarks/`。
