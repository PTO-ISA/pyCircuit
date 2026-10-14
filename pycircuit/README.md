# 编译器构建与使用

编译链路：ACPy → 源语义 MLIR → 事务 MLIR → RTL 友好 MLIR → 逐拍轨迹。
语义见 [核心设计](docs/core-design.md)，阶段进度与问题见 [路线图](docs/compiler-roadmap.md)。
多维 array、match、选取和仲裁接口见 [array 设计](docs/array-design.md)。

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
验收脚本复现 9 个示例的 32 拍执行、优化开关和三阶段重载，并更新示例输出。
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
| `model.source.mlir` | v9 静态实例、组件、统一 Rule 的 guard／body、SSA、scf.if、module.call、array／选取和契约 |
| `model.transaction.mlir` | 每调用点独立 fire、路径条件、条件化效果、共享 SSA、固定端口、后继与无环顺序 |
| `model.rtl.mlir` | 完整组合 fire／选择器、固定 Queue 存储和占用、current／next-state；Signal 为组合 data／valid |
| `rule-graph.json` | 事务节点、分支边、端口竞争和固定调度的查看报告 |
| `transaction-trace.json`、`rtl-trace.json` | 每拍 fire 与提交后的状态；Signal 不列为跨拍状态 |

阶段文件仅有 source、transaction、rtl，不再输出同内容别名。旧格式需从 ACPy 重新编译；不提供 v8 或旧前端兼容。
也可直接使用原生 pass：

```bash
pycircuit/builds/compiler/mlir/acir-opt model.source.mlir --acir-extract-transactions
pycircuit/builds/compiler/mlir/acir-opt model.transaction.mlir --acir-lower-rtl
```

构造配置保存于源 MLIR 的 `acir.config`；直接运行 pass 时必须已有完整配置。
错误返回非零，说明资源、事务或契约原因。所有分支顺序边的节点合并图必须无环。

## 语义与边界

只提供状态和公开 Rule 的组件可省略 `__call__`，实例化后直接调用其 Rule，无需额外的 `rf()`。
有输入连接、固定输出或每拍主体逻辑的模块仍通过 `__call__` 连接。
逐拍轨迹包含内部状态，RF／内存无需为了观测而导出状态组件；无主体的实例不会自动执行 Rule。
流水线 Queue 由顶层 `__init__` 明确声明，使用 `if_id = fetch()`、
`id_ex = decode_stage(if_id, rf, mem_wb)` 等赋值绑定输出目的地。
`.outputs`、隐式生成输出 Queue 和返回内部组件引用的包装均已删除。
赋值保持目的地身份，输出效果和容量要求纳入产生值的 Rule 事务。

统一 `@ac.rule` 由实际效果推导只读／修改分类。Module body 中每个修改性调用点建立事务，
嵌套调用共享外层事务。失败的读取／guard 阻止相关分支执行；分支内要求只约束所选路径，
合流后的无关事务保持独立。同事务重复消费按真实 Queue 合并。

Cell 读取旧值；Queue 满时允许前序成功 pop 腾空间，新 push 数据下拍可读；
Signal 只在本拍成功 set 后有效；EHR 读取固定端口之前的成功写入。
EHR 端口不能因任意拓扑排序而增加未声明可见性。竞争报告不进行自动仲裁，
未处理的端口竞争结果不保证。RTL 层已无 Rule 调用、隐式就绪条件或待定调度。

统一使用 `ac.array(T, shape=(...))`，元素可以是 payload、组件或固定 Module 实例。
`array[mask]` 保留 shape 和身份，`match` 产生临时 mask；普通整数坐标单选，仲裁返回 `(found, position)`。
动态越界或访问未选元素向实际使用者传播未就绪。部分选取不能跨拍保存；没有公开 index 类型。
源／事务层保留结构，RTL 层物理展开为标量状态、比较、mux 和使能，轨迹恢复逻辑坐标。

条件严格要求 bool，helper 只处理 payload，组件构造只允许在 __init__，循环必须静态展开。
不支持 var/ref/vector/qarray、通用类型转换、链式赋值或嵌套声明。
无 SystemVerilog 后端、自动仲裁、激活调度或跨拍缓存优化。

## 示例与可视化

[示例说明](examples/README.md) 包含 CPU、OoO、共享计算与诊断模型。

```bash
python pycircuit/draw_rules.py pycircuit/builds/riscv_pipeline/rule-graph.json
```

绘图使用 Graphviz；`-o PATH.dot` 无需安装 Graphviz。JSON 和图均不参与后续编译。
外部源码位于 `reference/<项目名>/`，构建位于 `builds/`，新实验输入、日志与结果位于
`builds/benchmarks/`。
