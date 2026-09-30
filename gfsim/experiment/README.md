# Python GFSim 调度实验

仅使用 Python 3.11 标准库。引擎约 400 行，测试和性能实验另放，全部 Python 代码约 900 行。

## 运行

从仓库根目录执行：

```bash
python3 -m unittest discover -s gfsim/experiment -v
python3 gfsim/experiment/bench.py
```

性能实验输出 JSON，包含计数、三次运行的耗时中位数、参考模式耗时 / 索引模式耗时。参数可调整：

```bash
python3 gfsim/experiment/bench.py --ticks 200 --size 32 --idle 1000 --work 2000 --repeat 3
```

`--size` 是流水级数，`--idle` 是休眠 Module 数，`--work` 是每次重计算的循环次数。建图和初次订阅不计入耗时；两种模式的获准序列、最终 Queue 状态必须一致。

## 最小模型

在该目录下运行以下代码，或将该目录加入 Python 导入路径：

```python
from engine import Simulator

sim = Simulator()
input_q = sim.queue(initial=(7,))
output_q = sim.queue()
module = sim.module()

def move():
    value = input_q.peek()  # 纯读；空时终止本次尝试。
    input_q.pop()          # 显式提出消费，实验不包含编译器。
    output_q.push(value + 1)

rule = sim.rule(module, move, pops=(input_q,), pushes=(output_q,))
module.work = rule
print(sim.step())          # ('r0',)
print(sim.snapshot())      # ((), (8,))
```

Module Work 用普通 if/else 调用 Rule；Rule Work 也用普通控制流。`sim.rule` 声明所有可能分支使用的 pop/push/revise Queue。读取依赖在运行时登记，不需要静态声明。Rule 可以接收普通值参数。

Queue 值和 Rule 参数只支持整数、布尔值及递归不可变 tuple，使用值语义。`current` 和 `snapshot()` 用于 Work 外观察；Work 必须通过读取接口访问状态。Work 不修改外部状态，计算结果只由 Queue 读取和显式参数决定。Rule 不直接读取 tick；时间值应由显式唤醒的 Module 作为参数传入。驱动 Module 的时间条件变化时，测试负责 `step(wake=(driver,))`。

`peek()` 必要输入不足会抛出 `NeedInput`，引擎负责收尾。`try_peek()` 返回 `None` 表示空，便于正常业务分支；它仍登记读取。Rule 正常返回且有操作表示完整候选，正常返回但无操作不产生 firing。Module 必要读取不足只停止其后续调用，此前选中的独立 Rule 仍可提交。

## 实验采用的语义

- 平级 Module，B 容量策略：已整体获准的 pop 可给同拍 push 腾空间；新 push 的数据下一 tick 才可读。
- 静态消费先行拓扑顺序，无依赖竞争按注册编号；排除同一 Rule 自身的边，其他静态容量环报错。
- Module 每 tick 至多 Work 一次，Rule 每 tick 至多获准一次。后续 delta 重试选中的候选，所有 delta 后统一 Xfer。
- Module 发布新 `read_gen`，Queue 只通知代号匹配的读者。旧条目懒失效，每对 Queue/Module 最多一条。
- Rule 缓存校验参数及实际读取、pop/revise 目标的 Queue 版本。缓存复用时重新登记到 Module 本轮读取集合。
- 未选中的旧 Rule 清理全部候选和等待关系；已获准候选提交后清除，未获准完整候选可跨 tick 保留。
- Queue 保管 proposal，Rule 只引用参与 Queue。预约失败释放全部临时预约，保留候选，并等待第一个失败 Queue。
- 获准 pop 通知静态生产者；跨 tick 的读取变化和资源端口重置通知分别处理。纯 push 不引入输出内容依赖。
- 同一 Rule 可以原子 pop/push 满 Queue。每 Queue 每 tick 单 pop、单 push、单整值 revise；revise 修改旧队尾。
- Xfer 为 revise → pop → push。无变化的 revise 不推进版本；pop/push 即使产生相同内容也推进版本，保守区分元素身份。
- 同一旧元素的 revise/pop、重复操作、未声明的修改、可变 payload 明确报错。模型错误终止运行，不支持异常后的恢复。

不包含字段修改、子 Module、定时事件、并行执行或编译器接入。Queue 使用 Python list，队首删除和 payload 计算成本由 Python 决定。

## 验证与计数

定向测试给出预期获准结果、状态和订阅，覆盖分支切换、读空、缓存覆盖、取消、原子失败、delta、同拍容量、提交顺序和端口重置。随机测试用 12 个固定种子，各运行 100 tick，逐 tick 对比获准序列、Queue 状态、版本和下一 tick 激活集合。

`Simulator(reference=True)` 关闭候选缓存，扫描 Module 正向读取集合、Rule 静态生产关系和等待关系寻找通知对象。它与索引模式共享仲裁和提交实现；这些共享部分由定向预期测试检查，随机对比主要检查缓存及反向索引的一致性。

| 计数 | 含义 |
| --- | --- |
| `module_work` / `rule_work` | 实际执行的 Work 次数 |
| `cache_hits` / `cache_invalidations` | 候选复用 / 依赖或参数失效 |
| `version_checks` | 实际 Queue 版本比较次数，失配时提前停止 |
| `reader_checks` / `producer_checks` / `waiter_checks` | 索引查询遍历的条目数 |
| `module_scans` / `rule_scans` | 参考模式通知查询遍历的对象数 |
| `arbitrations` / `reservation_checks` / `reservations` | Rule 仲裁次数 / Queue 预约检查次数 / 成功预约次数 |
| `accepted` / `deltas` / `ticks` | 获准次数 / 有任务的 delta 批次数 / 推进 tick 数 |
| `incomplete` / `module_incomplete` | Rule / Module 必要读取不足次数 |

背压实验让 Module 读取每 tick 改变的控制 Queue，保持选中同一条 Rule；Rule 的输入保持稳定、输出每 20 tick 被消费。这直接测量 Module 重算时仍可复用 Rule 计算的收益。稀疏实验在初次订阅后让 1000 个读空的 Module 休眠。满流水链每 tick 全部推进，主要测量索引查询成本。

耗时只描述这些 Python 负载，不设速度验收门槛，也不能据此预测 C++ 实现性能。

2026-09-30 本机 Python 3.11.16、默认参数的一次结果（三次运行取中位数）：

| 负载 | 索引模式 | 参考模式 | 参考 / 索引 |
| --- | --- | --- | --- |
| 32 级满流水链 | 120.51 ms | 153.84 ms | 1.28 |
| 长期背压，计算循环 2000 次 | 9.88 ms | 51.82 ms | 5.24 |
| 1000 个休眠 Module | 8.27 ms | 102.22 ms | 12.36 |

背压负载总 Rule Work 从 410 次降到 220 次，缓存命中 190 次。稀疏负载的读取通知从扫描 400800 个 Module 降到检查 400 个读者条目；两种模式的 Work 和获准次数一致。
