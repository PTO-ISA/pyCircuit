# GFSim Python 调度实验

本轮实现 [spec](../spec.md) 的动态 Rule 读者位图、dirty 机制和只读组合 Signal，模拟编译器生成后的普通成员函数与显式资源绑定。实现、验证和耗时见 [报告](report.md)；固定槽位与自动读取登记的分步计时见 [读取登记说明](read-tracking.md)；最新 begin_rule／wakeup 改动通过 39 项测试，实测见 [报告更新](report.md#begin-rule-wakeup)。新增 Signal 与完整流式电路见 [signals 示例与测速](examples/signals/README.md)。C++ runtime 暂未迁移，任意 Module 间 var 传播及 Signal 间依赖留待后续。

## 运行

仓库根目录，仅需 Python 3.11 标准库：

```bash
PYTHONPATH=gfsim/experiment python3 -m unittest test_engine examples.riscv.test_model review.test_review examples.signals.test_model -v
PYTHONPATH=gfsim/experiment python3 -m examples.signals.run
python3 gfsim/experiment/bench.py
PYTHONPATH=gfsim/experiment python3 -m examples.riscv.run gfsim/experiment/examples/riscv/programs/sum.s --latency 5 --html
```

基准运行 3 个完整程序 × 延迟 1/3/5 × 缓存开关 × Module 正反序共 36 组。`--output DIR` 指定逐拍轨迹和计时结果目录，`--compare traces.json` 与既有轨迹逐拍比较，`--repeat N` 控制计时次数。默认输出到被 Git 忽略的 `review-output/dirty/`。改造前后原始计时记录在 [results.json](results.json)，旧版重现方法在报告中。

上述验收不运行原生 Ripes。[ripes5](examples/ripes5/README.md) 原文件保留，本轮不迁移、不更新其对齐结论。旧七组 Python examples 和专用参考框架已移除；它们的历史可从 Git 获取。

## 阅读入口

| 文件 | 职责 |
| --- | --- |
| [construction.py](construction.py) | 固定 ID、资源声明、局部 Rule 位号、Queue 可能读者链接和 proposal 来源 |
| [engine.py](engine.py) | 动态读取、dirty、候选生命周期、容量 DFS、事件和 Xfer |
| [test_engine.py](test_engine.py) | 小电路验证精确失效及订阅生命周期 |
| [riscv/model.py](examples/riscv/model.py) | 五阶段实例化、Queue 连接及显式可访问资源 |
| [riscv README](examples/riscv/README.md) | 生成式阶段代码、ISA 子集和流水时序 |
| [signals](examples/signals/README.md) | 共享 Signal 的双输入流式电路、独立逐拍参考模型、测试与测速 |
| [bench.py](bench.py) | 完整程序、独立退休检查、逐拍对比与独立计时 |
| [review/recorder.py](review/recorder.py) | 可选通用观察器和离线 HTML 导出 |

## 构造与运行接口

`assemble(queues, modules, rules, cache=True, *, module_queues=None, signals=(), module_signals=None, signal_queues=None)` 接收显式静态表。`module_queues[mid]` 声明该 Module 可访问的全部 QueueId，包括动态数组所有可能元素；重复别名合并。省略时保守允许所有 Queue，兼容现存例子。声明仅决定可能访问范围，实际读取才建立通知关系。

每个 Module 使用普通 `Work()`，调用成员 `work_<rule>(args)`。生成式 Rule 显式执行：

1. `begin_rule(rid, args)`：同 tick 去重；未 dirty 时比较参数，变化则置 dirty；完整且未 dirty 的结果复用，包括无效果结果。已 dirty 时跳过参数比较，重算前保存新参数。
2. 直接调用 `peek/try_peek/empty/full/size/current`，接口根据当前 Module／Rule 自动登记；实际消息输入读取另行提出 pop。
3. 提出 push／pop／revise 或 `request_wakeup(rid, mid, delay)`。
4. 正常完成 `complete_rule`；缺输入或既有模型的就绪条件不满足时 `abort_rule` 并返回。

Queue 读取接口在 Work 内自动登记，读空也登记；在 Work 外的结果观察不订阅。`begin_rule` 真正执行时设置 active_rule，complete/abort 清除，Module 退出（含异常）清除全部上下文。Pop/revise 自动登记目标依赖，重复登记用位测试去重；纯 push 不订阅输出 current。Rule 不 accept，也不直接改变 current。所有持久修改归属 Rule，来源 0 没有外部提交协议。显式 `record_read` 保留兼容，新模型无需重复写它。

`step()` 首次执行先初始化全部 Signal，再完成全部激活的 Module Work，再用显式栈 DFS 仲裁，最后统一 Xfer。变化 Queue 将局部资源槽位交给 `_wakeup(mid, tick + 1, changed_slot=slot)`，入口立即通过实际读者位图置 dirty，有实际 Rule 读取或有效控制读取才登记激活。多资源的标脏都须处理，激活在到期时去重；普通定时事件不带槽位，只激活、不标脏。获准 pop 的容量通知只重试已有有效候选仲裁，不重跑 Work。全部 Queue Xfer 后，受变化输入影响的 Signal 去重求值；结果变化也经同一 wakeup 入口通知。执行异常后禁止继续。

## Signal

`Signal(helper)` 是组合值缓存，Module 或 Rule 读取 `.value` 时按执行上下文自动登记。helper 只能读取显式声明的 Queue current 与不可变配置，不能读取其他 Signal、依赖时间、提出 proposal 或安排事件。它的 Queue 读取登记到 Signal 自己；空输入用有效位和默认值显式表达。

构造时通过 `module_signals[mid]` 声明可能读取的 SignalId，通过 `signal_queues[sid]` 声明 helper 可能读取的 QueueId；使用 Signal 时这两张表必填。同一对象只注册一次，别名使用同一 ID。不实现 `@ac.signal` 前端装饰器。

Signal 初值在首次 Work 前计算；此前 `.value` 访问报错。此后全部 Queue Xfer 完成后才求值受影响 Signal，每阶段至多一次；输入依赖按本轮实际路径替换，结果相同不通知读者。下一 tick 的 Queue 与 Signal 是同一份新状态的直接值与派生值，不增加一拍延迟。

Queue 和 Signal 复用局部资源槽位、控制 readGen 与 Rule 位图，但 proposal 和容量仲裁仍只属于 Queue。Module 控制读取变化只唤醒；Rule 直接读取变化精确标脏。`begin_rule` 不增加 Signal 专用判断。

## 读取与候选分开管理

| 操作 | 候选 | 实际 Rule 读取关系 |
| --- | --- | --- |
| 缓存命中 | 保留 | 保留，不遍历、不续订 |
| 重算 | 清旧候选，执行新路径 | 清旧边，登记新读取 |
| abort／必要输入为空 | 清部分效果 | 保留已经尝试的读取 |
| 提交后 | 清已提交候选 | 保留，供后续变化唤醒 |
| 完整无效果 | 不 firing，完整且未 dirty 时可复用 | 保留 |
| Module 不再选择 Rule | 清候选 | 清读者位及 dirty |

Module 控制读取单独用 readGen 管理，每次 Work 替换控制读取集合。控制 Queue／Signal 变化只唤醒 Module，传给 Rule 的派生 var 值不变时仍可复用。普通 var 作为 args 按值比较，不建资源 slot。Rule 直接读取的 Queue／Signal 变化则精确标记该 Rule。共享 Signal 不是 Module 直接向其他 Module 写 var。

## 数据结构与成本

每条 Rule 在所属 Module 分配一个局部 bit，64 条一字。每个 Module 持有扁平 `rule_readers[resourceSlot * wordCount + word]`、`dirty_words`、`control_reads`。Queue 和 Signal 各自保存排序的可能读者 ModuleId 及局部槽位。两类资源另外保存固定 `module_slots[mid]` 数组直接定位槽位，未声明为 -1，随后位测试去重；Rule 的 `read_slots` 用于重算／取消时清旧边。

缓存命中只比较参数和检查候选标志；不扫描 Queue 版本。`state_version` 仍作观察计数。变化通知扫描可能读者的相关位图字，重算清理实际读取；相对旧版将部分成本从缓存检查移到登记／通知，不保证每类负载提速。

新增槽位映射有 (QueueCount + SignalCount)×ModuleCount 个整数项；通知仍使用可能读者列表。Python 位图使用 int 模拟每个 uint64 字；运行核心不以 set/dict 保存动态依赖。构造时可用集合去重。Python list.clear 不保证 C++ vector 的容量保留，Python 对象成本也不代表 C++。不可变 tuple／NamedTuple 对应按值 aggregate，未来 C++ 接口仍需单独实现与验证。

Signal 的 Queue 输入另用排序 ID 数组、读取代号和 Queue 上的固定反向链接；二分定位实际读取，通知匹配代号。空间随可能输入连接数增长，不增加全量 QueueCount×SignalCount 表。

## 可视化

任意本引擎模型可接入：

```python
from review import ReviewTrace
trace = ReviewTrace(cpu.sim, title='CPU')
try:
    cpu.run()
finally:
    trace.write_html('gfsim/experiment/examples/riscv/review-output/cpu.html')
```

页面显示实际读取、保留依赖、调用时 dirty、缓存原因、Xfer 变化通知、容量边及事件，以及 Signal 的实际输入、求值、值变化和读者。观察器默认关闭；它扫描并保存状态，不应计入引擎性能测量。失败现场可导出，但不能视为完整提交快照。
