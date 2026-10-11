# Python → MLIR → 静态 rule 图

前端只解析 AST 和静态连接，不执行模型 Python。第一阶段输出可独立重载的事务 MLIR、条件访问摘要及 rule 关系图。编译器不生成仿真代码。旧 CPU 示例、benchmark 脚本和旧测试夹具已移除；当前编程范式示例见 [examples/README.md](examples/README.md)，包括混合端口、共享实例、五级 RV32I 子集流水线和跨 Module OoO 调度；`tests/models/` 保留最小验收模型，GFSim 保持独立。

本机使用已安装的 Clang 22、GCC 14 sysroot 和 LLVM/MLIR 22：

```bash
export LD_LIBRARY_PATH=/home/lc/opt/gcc14/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
cmake -S pycircuit -B pycircuit/builds/compiler -G Ninja \
  -DCMAKE_C_COMPILER=/home/lc/opt/pycircuit-dev/bin/cc \
  -DCMAKE_CXX_COMPILER=/home/lc/opt/pycircuit-dev/bin/c++ \
  -DMLIR_DIR=/home/lc/opt/llvm-22.1.8/lib/cmake/mlir \
  -DCMAKE_BUILD_TYPE=Release
cmake --build pycircuit/builds/compiler -j 6
export ACPY_MLIR_COMPILER="$PWD/pycircuit/builds/compiler/mlir/acir-compile"
python -m pycircuit compile pycircuit/tests/models/transfer.py --top Top \
  --output pycircuit/builds/benchmarks/rule-graph/transfer
python -m pycircuit analyze pycircuit/builds/benchmarks/rule-graph/transfer/model.acir.mlir \
  --output pycircuit/builds/benchmarks/rule-graph/reloaded
ctest --test-dir pycircuit/builds/compiler --output-on-failure
```

任意兼容 MLIR 22 的工具链也可使用。`ACPY_MLIR_COMPILER` 可省略，此时从 PATH 查找 `acir-compile`。`--no-opt` 关闭 canonicalize/CSE。两个入口均保存 `model.acir.mlir`、`model.readable.mlir`、`rule-graph.json`、`rule-graph.dot`；主 MLIR 保留真实源码位置并清理临时／无效位置；阅读版隐藏调试位置。独立重载和完整诊断应使用主文件。发现已确认依赖环或无法证明的顺序时，CLI 返回非零，保留带诊断的产物。

`@ac.module def` 实例的状态通过 `instance.state` 访问，公开 method 通过 `instance.method(...)` 调用，返回连接通过 `instance.outputs` 访问。固定 tuple 返回可用常量下标。支持本地 `.py` 文件的 import/from-import 和别名，完整定义名避免跨文件重名；导入同一已声明实例共享身份，分别构造两个实例则状态独立。

`@ac.rule` 表达独立原子事务；`@ac.method` 和普通 helper 的调用保存在 `func.call` 中，其效果及 guard 传递归入外层 rule。Module 可直接调用只读 method，包括其 helper、嵌套 method 和 guard；状态提交仍要求外层 Rule。`@ac.rule(guard=ready)`、`@ac.method(guard=ready)` 的函数 guard 接收同样的参数并返回 bool，只观察状态；`guard=enabled` 也可引用声明作用域的布尔 SSA 值，与外层选择路径分别保留。

Module 的计算保留真实 SSA/CFG。独立 JSON 中的 `preparations` 保存计算来源、输入输出、执行条件、输出有效性、状态及 method 依赖、使用者；Rule 通过 `preparations` 和 `preparation_bindings` 引用所需区域。`computations` 继续保存准备阶段状态访问，关联到 Rule 的访问保留 `source_access`、调用链和端口。共享定义只记录一份，独立调用保留独立身份，就绪要求随使用关系传播。[编译阶段的设计意图](docs/compiler-intent.md) 描述这些分析的目标；这些信息不表示已经实现运行时共享求值或缓存。

| 端口 | 输入 | 输出 |
| --- | --- | --- |
| `ac.queue[T]` | 实际读取 payload 的路径隐式 pop | 返回 payload 向固定 Queue push |
| `ac.cell[T]` | 读取周期开始的旧值 | 返回 payload 向显式 Cell write，下周期可见 |
| `ac.signal[T]` | 参数传递引用，值表达式读取当前值 | 返回 payload set 已初始化的 Signal |
| 普通类型 / `ac.var[T]` | 普通 SSA 值 | 仅 helper/method 返回值，Rule 不支持 |

混合输出用 `tuple[ac.queue[T], ac.cell[T], ac.signal[T]]` 逐位置绑定。Queue/Cell/Signal 返回 None 省略该效果。未注解的 rule 输出推导为 Queue；Rule 普通标量返回注解被拒绝。Cell 和 Signal 必须显式提供 `initial=` 声明。Queue 保持 `capacity=`、`initial=` 和固定一维资源数组。Queue `.value` 赋值保留旧队尾 revise，Cell `.value` 赋值表达 write。

`ac.signal[T](initial=...)` 通过赋值表达读写：已声明 Signal 在左侧时 set 并保持资源身份，普通值表达式中的 Signal 产生 payload SSA；Signal 参数和 Module 返回连接传递引用。写入属于 Rule 原子事务，跨 Rule get 只观察获准 set，无写或写者失败则读保留值；同 Rule 前序临时 set 可见。Signal 没有 pop、容量或 EHR 端口。完整组合示例见 [signal_link.py](examples/signal_link.py)，语言细节见 [ACPy spec](../acpy/spec.md)。Rule-var 输出、结果提取和 `var.validity` 已移除。

`ac.ehr[T](initial=...)` 复用 Cell 引用，通过 `.value` 读写。method 的 `After` 自动分配端口，无序相关访问共阶段；同端口写入不可见，晚端口看到前序有效写入，无写则旧值。helper 继承最内层 method，嵌套 method 保留自己的阶段，直接 Rule/准备访问使用端口 0。跨 Rule 旁路要求整个事务获准，同 Rule 前序 method 临时写入仍随整体提交或取消。冲突阶段报告资源、method 和来源。

Queue 内部 full 视图也使用 EHR：旧数量初始化，pop 早端口写 false，显式 full 和 push 隐式 guard 共用晚端口。容量视图不单独提交，Queue 数量由实际效果计算；empty、size 和 payload 保持旧状态。JSON `ehr` 与访问的 `port`、`bypass` 保存分配和有效写入来源，MLIR 重载忽略派生缓存并重新求解。验收见 `tests/test_ehr.py`。

消费性输入的角色沿 helper/method 参数传播，普通 method 的 Queue 参数默认观察。guard 的 Queue 读取始终观察。重复或别名读取按实际资源身份合并消费；不消费未执行分支。只查询 empty/full/size 不消费。`ac.ref[T]` 可声明 rule 的观察性 Queue 参数。

`@ac.method(After=("release",))` 声明同组件的 `release SB 当前 method`，也接受单个字符串或字符串 list，可与 guard= 共用。关系由组件契约驱动；Queue 默认 `read SB revise SB pop SB push` 并计算闭包，未声明且未推导出必要顺序的关系默认 CF。Queue read/pop 只观察或消费旧数据，新 push 数据下拍可读；push 可利用本拍成功 pop 释放的容量。Cell read/write 默认 CF。

MLIR 的 `acir.model` 保存原始组件契约、静态结构、类型和构造信息，具体资源、Module 和调用保存接口绑定。完整 RuleGraph 和推导契约导出到独立 JSON/DOT，不再写入顶层 `acir.analysis`/`acir.contracts`；EHR 端口标记和 `acir.ehr` 仍保留。原生 printer 为 ACIR 操作输出简洁语法，前端生成的通用语法和已有 v6 文件仍可解析；源码位置与稳定 ID 保留。JSON 顺序边统一为 `method.order`，保留接口、路径、位置和关系来源；Signal 使用 `set SB get`，不引入生产者成功的准入条件。用户声明与内部关系合并，同一 rule 内的 SB 保持原子事务且不生成自环。选择条件、guard、ready 和共享 SSA 读取分别保存。ACIR 格式升级为 v6，v3/v4/v5 产物必须重新编译。

条件证明限于常量、共享 SSA 条件的布尔互斥及 Queue empty/full 不共存的契约；复杂条件保留为 opaque SSA 条件。诊断区分 `confirmed-cycle` 和 `unproven-order`，列出 rule、资源、操作、位置及条件。互斥调用合并为同一实例节点，保留各调用的参数和选择条件。

定宽标量、结构体、payload 数组、算术、分支和 `for ... in range(...)` 复用已有 SSA/CFG。固定资源数组常量下标精确，动态下标覆盖可能元素。循环摘要保守处理循环携带值，保留完整循环 IR，不展开运行时路径。不支持递归调用、运行时重复选择同一 rule、运行时资源构造、while/break/continue、多维资源数组或复杂求解器。

验收覆盖完整 Module/rule/method 模型、共享实例、跨模块原子事务、分支消费、Pipeline FIFO、After 契约、混合输出、共享计算、条件环、ODS 验证、优化前后关系和删除 Python 源码后的重载，见 `tests/test_rule_graph.py`、`tests/test_method_contracts.py`、`tests/test_examples.py` 与 `tests/test_mlir.py`。这一阶段不实现运行时激活、仲裁、Xfer、仿真后端或竞争写入检查，也不宣称验证 CPU 时序。

本次两个完整 example 的 ACIR v6 静态编译产物保存在 `builds/riscv_pipeline/`、`builds/ooo_scheduler/`；外部参考源码使用根目录 `reference/`，新构建使用 `builds/`，验收日志使用 `builds/benchmarks/`。

## 绘制 Module / rule 顺序图

[`draw_rules.py`](draw_rules.py) 读取 ACIR v6 的 `rule-graph.json`，按实例层次绘制 Module 分组和 rule 节点。实线表示 SB 顺序，红色表示 confirmed-cycle，橙色表示 unproven-order。重复操作边合并展示，完整条件、位置和来源保留在 SVG 的悬停提示中。默认也显示只有 method、没有独立 rule 的组件；`--hide-empty-modules` 可以仅显示有 rule 的组件及其祖先。

脚本使用 Python 标准库和 Graphviz，不需要 MLIR 编译器或 Python graphviz 包。本机历史 Graphviz 安装在 `reference/builds/rule-graphviz/env/`（脚本保留该回退路径），脚本自动找到该环境的 `dot`，无需激活环境。其它机器可通过 [`graphviz-environment.yml`](graphviz-environment.yml) 配置：

```bash
CONDA_PKGS_DIRS="$PWD/pycircuit/builds/rule-graphviz/pkgs" \
  /home/lc/opt/miniforge3/bin/conda env create \
  --prefix "$PWD/pycircuit/builds/rule-graphviz/env" \
  --file pycircuit/graphviz-environment.yml
```

在仓库根目录绘图：

```bash
python pycircuit/draw_rules.py pycircuit/builds/riscv_pipeline/rule-graph.json
python pycircuit/draw_rules.py pycircuit/builds/ooo_scheduler/rule-graph.json
```

默认在输入 JSON 的同一目录保存 `module-rule-graph.svg` 和 `module-rule-graph.dot`，因此两个 example 的图分别位于对应 `builds/`。`python -m pycircuit.draw_rules` 同样可用。`-o PATH` 可指定 `.svg`、`.png`、`.pdf` 或 `.dot` 输出；DOT 输出无需安装 Graphviz。`--rankdir TB` 改为从上到下布局。`--dot PATH` 或 `ACPY_GRAPHVIZ_DOT` 可以指定其它 Graphviz，未指定时依次寻找仓库环境和 PATH。

图中的箭头表示同周期必要顺序，Queue 的 read/pop → push 可能与流水线的数据流方向相反。CF 对不画边，条件关系保留原始路径和 guard，不能将整个图直接当作无条件执行序列。
