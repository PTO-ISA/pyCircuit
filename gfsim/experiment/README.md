# GFSim 调度实验

按照当前 [spec](../spec.md)实现单线程调度，模拟编译器生成后的成员函数、静态表和记录。Rule 是 Module 成员函数，运行前显式构造连接；运行期间不分析函数体。

验收全部使用完整电路，从输入请求运行到输出结果或预期错误。测试不直接构造候选、调用内部仲裁或修改 runtime 状态推进电路。实现和实验结论见 [实验报告](report.md)，未决语义仍见 [open-questions](../open-questions.md)。

## 从哪里开始读

| 你想检查什么 | 阅读入口 |
| --- | --- |
| 调度引擎 | [engine.py](engine.py)：从 `step` 开始，看 `_work`、`begin_rule`、`_visit`、Queue 的 `xfer`；静态绑定在 [construction.py](construction.py) |
| 一个完整硬件模型 | [examples/](examples/) 下每个目录是一个模型，行为、连接、独立参考和端到端测试放在一起 |
| Ripes 固定五级对照 | [ripes5 README](examples/ripes5/README.md)：原版 RV32_5S、统一 JSONL、四配置逐拍验收 |
| 五级 CPU | [CPU README](examples/riscv/README.md) → [model.py](examples/riscv/model.py) → 五个阶段；实现限制见 [findings.md](examples/riscv/findings.md) |
| 运行轨迹可视化 | [review/](review/)：通用观察器与页面，接入方法见下文；模型相关入口放在各模型目录 |
| 验证结论和性能数据 | [report.md](report.md)、[results.json](results.json) |

## 运行与阅读入口

仅依赖 Python 3.11 标准库，从仓库根目录执行：

```bash
python3 -m unittest discover -s gfsim/experiment -v
python3 gfsim/experiment/bench.py
```

完整测试包含 1100 级满流水从初始状态排空，运行约一分钟以上；其他电路测试通常一秒左右完成。基准输出 JSON，[results.json](results.json)保存本次默认参数运行结果。

## 文件组织

按完整电路例子组织。小例子的组件行为与连接写在同一个 `model.py`，参考行为和端到端测试放在相邻文件中；CPU 将五个阶段分别放在独立文件，`model.py` 负责连接。

```text
experiment/
  engine.py                 # 通用调度核心
  construction.py           # 通用静态表和记录初始化
  reference.py              # 通用参考调度器，行为函数由例子传入
  bench.py                  # 性能测试入口
  review/                   # 可选审阅工具，核心不依赖这个包
    __init__.py             # 导出 ReviewTrace
    recorder.py             # 通用观察器与 HTML 导出
    viewer.html             # 通用离线页面
    test_review.py          # 完整电路的观察一致性与失败现场测试
  examples/
    common.py               # 共用 Module、Source、Sink、Config、Merge 及 Netlist
    reference_common.py     # 共用组件的独立参考行为
    testing.py              # 共用逐拍比较和记录检查
    pipeline/
      model.py              # Compute 行为、pipeline() 电路连接
      reference.py          # 本例的独立参考行为
      test_model.py         # 本例输入、运行及输出断言
      review.py             # 本例可视化运行入口
    packets/                # 以下目录均采用相同的三文件结构
    pairs/
    memory/
    feedback/
    lookup/
    retry/
    ripes5/                 # 固定 Ripes 原版对照，独立的同步五级流水
    riscv/                  # 五个阶段各一文件；汇编器、顺序解释器和完整程序
```

| 例子 | 模型与连接 | 参考行为 | 端到端测试 |
| --- | --- | --- | --- |
| 弹性计算流水 | [model.py](examples/pipeline/model.py) | [reference.py](examples/pipeline/reference.py) | [test_model.py](examples/pipeline/test_model.py) |
| 可配置包处理网络 | [model.py](examples/packets/model.py) | [reference.py](examples/packets/reference.py) | [test_model.py](examples/packets/test_model.py) |
| 双输入原子处理 | [model.py](examples/pairs/model.py) | [reference.py](examples/pairs/reference.py) | [test_model.py](examples/pairs/test_model.py) |
| 分 bank 存储 | [model.py](examples/memory/model.py) | [reference.py](examples/memory/reference.py) | [test_model.py](examples/memory/test_model.py) |
| 反馈网络 | [model.py](examples/feedback/model.py) | [reference.py](examples/feedback/reference.py) | [test_model.py](examples/feedback/test_model.py) |
| 在线系数查表 | [model.py](examples/lookup/model.py) | [reference.py](examples/lookup/reference.py) | [test_model.py](examples/lookup/test_model.py) |
| 重试缓冲 | [model.py](examples/retry/model.py) | [reference.py](examples/retry/reference.py) | [test_model.py](examples/retry/test_model.py) |
| 五级 RISC-V CPU | [model.py](examples/riscv/model.py) | [reference.py](examples/riscv/reference.py) | [test_model.py](examples/riscv/test_model.py) |

阅读一个例子时，先看它的 `model.py` 中的组件和电路构造函数，再看同目录 `test_model.py` 的输入与断言；需要核对独立行为时看该目录的 `reference.py`。共用组件定义见 [common.py](examples/common.py)。新增例子时增加一个目录，并由该例的 `Netlist(reference, evaluate)` 显式传入参考行为；无需修改通用调度器或维护集中式例子分发表。

例如，只运行存储例子的测试：

```bash
python3 -m unittest discover -s gfsim/experiment -k examples.memory -v
```

核心 `engine.py` 为 459 行，静态构造为 27 行，包含空行和注释。其余文件负责组件、电路、参考模型、测试、可视化和基准。

## 人工审阅可视化

可生成单文件 HTML，直接用浏览器打开；无需服务器、网络或第三方依赖。

```bash
PYTHONPATH=gfsim/experiment python3 -m examples.pipeline.review
PYTHONPATH=gfsim/experiment python3 -m examples.riscv.run \
  gfsim/experiment/examples/riscv/programs/sum.s --latency 5 --html
```

默认结果分别生成到 `examples/pipeline/review-output/pipeline.html` 和 `examples/riscv/review-output/riscv.html`。生成目录自动创建并由 Git 忽略；也可以给流水线入口传入输出路径，或用 `--html 路径` 自定义 CPU 输出位置。

页面支持逐拍切换、Rule 时间线、实际 DFS 容量依赖与仲裁顺序、候选详情、Queue 拍初/拍末值、版本和有效读者、未来事件。`W✓` 表示重算后获准，`C✓` 表示缓存命中后获准，`P✓` 表示没有重新调用 Rule，保留候选直接获准。仲裁前记录与 Xfer 后状态分开展示；未完成的候选不推断业务停顿原因。容量图只展示 DFS 实际遇到的依赖，已经获准的 pop 不会再次产生该边。

通用记录器 [recorder.py](review/recorder.py) 与页面 [viewer.html](review/viewer.html) 不依赖 CPU。任意使用本实验 `Simulator` 的电路均可接入：

```python
from pathlib import Path
from review import ReviewTrace

example_dir = Path(__file__).parent  # 本段放在对应 example 的运行脚本中
trace = ReviewTrace(circuit.sim, queue_names={0: "input"})  # 名称可省略
try:
    for _ in range(100):
        circuit.sim.step()
finally:
    trace.write_html(example_dir / "review-output" / "circuit.html")  # 出错时也可保存现场
```

观察器不会驱动电路，默认关闭。它为审阅逐拍扫描 Queue、读者表并保存候选，不适合开启后测引擎性能；大模型应截取短窗口。Queue 值保存初始状态和逐拍变化，超出 JavaScript 精确整数范围的值显示为十进制字符串。动态读取只展示实际发生的记录，不声称获得完整的静态读取连接图。

## 接口与执行过程

`assemble(queues, modules, rules, cache=True)` 接收显式 ID 表，分配每个 Queue 的来源槽位和 Module 读者数组，绑定唯一 pop/push Rule。来源 0 保留但未提供外部驱动入口。

`step()` 执行一拍并返回获准 RuleId；第一次调用自动激活全部 Module。后续激活只来自状态通知和获准 Rule 的事件。`snapshot()` 供观察器读取结果。执行异常会将实例标为失败，后续 `step()` 拒绝继续。旧 `step(wake=...)` 已移除，测试驱动改成电路内的 Source/Config 组件。

```text
到期事件及 tick 0 初始化
  → 全部激活 Module Work
  → 验证、复用或重算候选，取消未再选中的候选
  → 显式栈 DFS：实际消费者先仲裁
  → 检查全部 Queue 后整体 accept，发布未来事件
  → 获准 pop 激活唯一生产者的现存候选
  → revise / pop / push 统一 Xfer
  → 变化 Queue 按读取代号登记下一 tick 事件
```

`Module.read(queue, rid)` 是显式 `record_read + peek` 的便捷写法；Module 控制读取省略 rid。它不推断依赖。Queue 读接口本身纯读，pop/revise 自动登记目标依赖。

Rule 使用 `begin_rule(rid, args)`，返回 false 时跳过计算。正常结束调用 `complete_rule`；必要读取失败调用 `abort_rule`。不完整路径清理 proposal 和未发布事件，Module 已登记订阅保留。

`request_wakeup(rid, moduleId, delay)` 只保存候选请求，整体获准时才按获准 tick 发布。Source 的等待分支和 Sink 的就绪时间控制使用纯事件 Rule；PairALU 的请求与多个 Queue 效果共同确认。

Source ROM 保存 `(最早发送tick, payload)`，Source 显式把当前 tick 作为 Rule 参数。存储服务的 due 值也是明确参数计算的持久状态；只有 busy 为空时创建事务，因此它的启动 Rule 不受输出容量阻塞，后续响应可独立等待。电路不把事件到期解释成隐式硬件状态变化。

## 与 C++ 的对应

| Python 表达 | C++ 对应与成本 |
| --- | --- |
| 固定 ID、读者和任务数组 | `std::vector<T>(N)` 或 `std::array<T, N>`，下标访问 |
| Queue data/head/count | 固定容量环形 FIFO，pop/push 为 O(1) |
| Rule deps/participants/events | 可复用 vector，仅记录实际访问；deps 线性去重，d 个不同读取最坏 O(d²) |
| 排序的来源 ID 和槽位 | 小型静态表，二分查询 O(log S)，不按全局 RuleCount 分配 |
| DFS `(rid, cursor)` 栈 | `vector<Frame>`，不依赖语言递归深度 |
| 事件最小堆 | `priority_queue`，登记和取出为 O(log E)；到期后按 ModuleId 去重 |
| 不可变 tuple / NamedTuple | 按值的固定 aggregate；数字字段路径对应生成的成员路径 |
| 成员函数绑定 | 实例句柄和生成函数入口 |

运行核心不使用 set/dict 存储动态依赖。构造阶段可以用集合归并静态来源；独立参考模型可以使用字典和集合。Python 的 list.clear 不保证像 C++ vector 一样保留底层容量，本实验只验证固定数组和槽位身份及记录生命周期。

结构体用固定形状 tuple 或字段递归满足值约束的 NamedTuple 表示，字段修改保留记录类型；字段路径是生成的常量。整数业务运算由组件显式截断，不实现完整 AC 类型库或 C++ ABI。时间戳使用 Python 整数。

## 完整电路与独立对比

- 弹性流水：数据源、多级计算和间歇接收端，验证逐拍延迟、吞吐、背压和计算复用。
- 包处理网络：两条独立通路、在线配置、显式汇流和正常丢包，验证 Module 与 Rule 的控制分支。
- 双输入处理：到达时间不同的两个源，原子输出到两个队列，由同一个接收 Rule 消费。
- 分 bank 存储：请求分发、动态寻址、字段写入、延迟服务、响应汇流和最终存储状态。
- 在线系数查表：配置和表项更新发生在输出阻塞期间，验证参数、实际依赖及旧订阅替换。
- 反馈与重试：静态环有空位、分支退出、自身 pop/push、相同 payload 替换、revise/pop，以及真实动态容量环。
- 五级 CPU：输入汇编代码，独立顺序解释器逐条核对退休结果；另外断言吞吐、前递、load-use、分支冲刷和长访存时序。

各例子目录下的 `reference.py` 独立描述该例的组件事务，共用组件的参考行为见 [reference_common.py](examples/reference_common.py)，均不调用被测 Module/Rule 函数。[reference.py](reference.py) 使用前向读取集合、每次激活重算、反复扫描的固定点许可和独立提交；用 Kahn 消除检查剩余动态环，不复用核心 DFS、缓存、版本通知或 Queue 存储代码。

逐拍比较获准集合、全部 Queue 数据和版本、未来事件、Module 激活次数及有效读取关系。额外 scoreboard 根据输入独立核对结果、顺序、最终存储和完成性。不规定无依赖 Rule 的仲裁顺序。长链用确定的输出序列和深度计数验收，避免再运行一个规模相同的参考引擎。

## 范围

每个 Queue 的 pop/push 分别只有一个 Rule 来源，配置选择由用户代码表达；不检测或保证竞争行为。执行异常终止本次仿真，不支持恢复。

已实现基础事件、纯事件 Rule、静态字段修改和 Queue array。父子 Module 激活、独立 Cell、已发布事件取消、来源 0 外部驱动、代号回绕、并行执行和最终 C++ ABI 未实现，也不由实验补充语义。
