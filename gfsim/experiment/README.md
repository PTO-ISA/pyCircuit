# GFSim Python 调度实验

> 本文保留静态调度重构前的实现／测量记录。当前 C++ 调度契约见 [GFSim spec](../spec.md)；Python 实验引擎不随本次重构迁移。

实现 [GFSim spec](../spec.md) 的读取依赖、dirty、候选复用、容量 DFS、事件与 Signal 调度。唯一的完整模型示例是 [Ripes5 五级流水](examples/ripes5/README.md)，使用普通 Module 类、Rule 成员函数和显式资源绑定。

## 运行

在仓库根目录运行，Python 部分仅需 Python 3.11 标准库：

```bash
python3 -m unittest discover -s gfsim/experiment -v
# 必须执行原生 Ripes 对照；参考环境构建方法见 Ripes5 README
RIPES5_REQUIRE_NATIVE=1 python3 -m unittest discover -s gfsim/experiment -v
PYTHONPATH=gfsim/experiment python3 -m examples.ripes5.run --case array_sum --html
PYTHONPATH=gfsim/experiment python3 -m examples.ripes5.bench
```

未构建原生 Ripes 时，常规测试明确跳过原生对照；设置 `RIPES5_REQUIRE_NATIVE=1` 后将缺失视为失败。模型测试覆盖完整程序、缓存开关、Module 顺序、Signal 与原生逐拍比较；通用引擎测试保留读取与候选生命周期等边界检查。

## 目录与阅读入口

| 文件／目录 | 职责 |
| --- | --- |
| [engine.py](engine.py) | 通用调度引擎 |
| [construction.py](construction.py) | 固定 ID、资源声明、位图与 proposal 来源构造 |
| [test_engine.py](test_engine.py) | 通用引擎边界测试 |
| [review/](review/recorder.py) | 通用观察器、HTML 导出与测试 |
| [examples/ripes5/model.py](examples/ripes5/model.py) | 五阶段连线、Signal 与观测接口 |
| [examples/ripes5/stages.py](examples/ripes5/stages.py) | 五个阶段及 Rule 行为 |
| [examples/ripes5/isa.py](examples/ripes5/isa.py) | 汇编与译码；指令类型在 records.py |
| [examples/ripes5/README.md](examples/ripes5/README.md) | 程序、原生参考、运行与测速说明 |

生成轨迹和 HTML 统一放在 `examples/ripes5/review-output/`，不纳入 Git。Ripes5 最新一次记录的结果与测速数据保存在示例目录；旧示例、旧结果和历史实现可从 Git 获取。

## 构造与运行接口

`assemble(queues, modules, rules, cache=True, *, module_queues=None, signals=(), module_signals=None, signal_queues=None, rule_signals=None)` 接收显式静态表。`module_queues[mid]` 声明该 Module 可访问的全部 QueueId，包括动态数组所有可能元素；重复别名合并。省略时保守允许所有 Queue，兼容现存例子。声明仅决定可能访问范围，实际读取才建立通知关系。

每个 Module 使用普通 `Work()`，调用成员 `work_<rule>(args)`。生成式 Rule 显式执行：

1. `begin_rule(rid, args)`：同 tick 去重；未 dirty 时比较参数，变化则置 dirty；完整且未 dirty 的结果复用，包括无效果结果。已 dirty 时跳过参数比较，重算前保存新参数。
2. 直接调用 `peek/try_peek/empty/full/size/current`，接口根据当前 Module／Rule 自动登记；实际消息输入读取另行提出 pop。
3. 提出 push／pop／revise 或 `request_wakeup(rid, mid, delay)`。
4. 正常完成 `complete_rule`；缺输入或既有模型的就绪条件不满足时 `abort_rule` 并返回。

Queue 读取接口在 Work 内自动登记，读空也登记；在 Work 外的结果观察不订阅。`begin_rule` 真正执行时设置 active_rule，complete/abort 清除，Module 退出（含异常）清除全部上下文。Pop/revise 自动登记目标依赖，重复登记用位测试去重；纯 push 不订阅输出 current。Rule 不 accept，也不直接改变 current。所有持久修改归属 Rule，来源 0 没有外部提交协议。显式 `record_read` 保留兼容，新模型无需重复写它。

`step()` 首次执行先初始化全部 Signal，再完成全部激活的 Module Work，再用显式栈 DFS 仲裁，最后统一 Xfer。变化 Queue 将局部资源槽位交给 `_wakeup(mid, tick + 1, changed_slot=slot)`，入口立即通过实际读者位图置 dirty，有实际 Rule 读取或有效控制读取才登记激活。多资源的标脏都须处理，激活在到期时去重；普通定时事件不带槽位，只激活、不标脏。获准 pop 的容量通知只重试已有有效候选仲裁，不重跑 Work。全部 Queue Xfer 后，受变化输入影响的 Signal 去重求值；结果变化经 `changed_signal` 和固定 dirty 掩码通知。执行异常后禁止继续。

## Signal

`Signal(helper)` 是固定输入上的纯组合值缓存，Module 或 Rule 读取 `.value` 不修改动态依赖。helper 只能读取显式声明的 Queue current 与不可变配置，不能读取其他 Signal、依赖时间、提出 proposal 或安排事件。它的 Queue 读取只检查声明，不登记动态依赖；空输入用有效位和默认值显式表达。

构造时通过 `module_signals[mid]` 声明静态激活该 Module 的 SignalId，通过 `signal_queues[sid]` 声明 helper 的全部输入 QueueId（含未选分支和数组表项），通过 `rule_signals[rid]` 声明固定 Rule 依赖并隐含其所属 Module 激活；使用 Signal 时 `module_signals` 和 `signal_queues` 必填，`rule_signals` 默认全空（下标 0 保留）。仅 Module 声明不标脏其 Rule。同一对象只注册一次，别名使用同一 ID。不实现 `@ac.signal` 前端装饰器。

Signal 初值在首次 Work 前计算；此前 `.value` 访问报错。此后全部 Queue Xfer 完成后才求值受影响 Signal，每阶段至多一次；任一声明输入变化均触发求值，结果相同不通知读者。下一 tick 的 Queue 与 Signal 是同一份新状态的直接值与派生值，不增加一拍延迟。

Queue 使用动态局部资源槽位、控制 readGen 与 Rule 位图；Signal 独立保存固定的 Module→Rule 掩码。Signal 输出变化直接标脏已声明 Rule 并安排下一拍 Module Work，关系不因上次未读、提交或取消而消失。proposal 和容量仲裁只属于 Queue。`begin_rule` 不增加 Signal 专用判断。

## 读取与候选分开管理

| 操作 | 候选 | 实际 Rule Queue 读取关系 |
| --- | --- | --- |
| 缓存命中 | 保留 | 保留，不遍历、不续订 |
| 重算 | 清旧候选，执行新路径 | 清旧边，登记新读取 |
| abort／必要输入为空 | 清部分效果 | 保留已经尝试的读取 |
| 提交后 | 清已提交候选 | 保留，供后续变化唤醒 |
| 完整无效果 | 不 firing，完整且未 dirty 时可复用 | 保留 |
| Module 不再选择 Rule | 清候选 | 清读者位及 dirty |

Module 控制读取单独用 readGen 管理，每次 Work 替换控制读取集合。控制 Queue 变化或仅绑定 Module 的 Signal 输出变化只唤醒 Module，传给 Rule 的派生 var 值不变时仍可复用。普通 var 作为 args 按值比较，不建资源 slot。Rule 实际读取的 Queue 或静态绑定的 Signal 输出变化则精确标记该 Rule。共享 Signal 不是 Module 直接向其他 Module 写 var。

## 数据结构与成本

每条 Rule 在所属 Module 分配一个局部 bit，64 条一字。每个 Module 持有扁平 `rule_readers[resourceSlot * wordCount + word]`、`dirty_words`、`control_reads`。Queue 保存排序的可能读者 ModuleId 及局部槽位，并保存固定 `module_slots[mid]` 数组直接定位槽位，未声明为 -1，随后位测试去重；Rule 的 `read_slots` 用于重算／取消时清旧边。

缓存命中只比较参数和检查候选标志；不扫描 Queue 版本。`state_version` 仍作观察计数。变化通知扫描可能读者的相关位图字，重算清理实际读取；相对旧版将部分成本从缓存检查移到登记／通知，不保证每类负载提速。

Queue 槽位映射有 QueueCount×ModuleCount 个整数项，Python Signal 的静态 dependents 表仅保存实际声明的 Module 及掩码（C++ 使用直接 Module 索引）；通知仍使用可能读者列表。Python 位图使用 int 模拟每个 uint64 字；运行核心不以 set/dict 保存动态依赖。构造时可用集合去重。Python list.clear 不保证 C++ vector 的容量保留，Python 对象成本也不代表 C++。不可变 tuple／NamedTuple 对应按值 aggregate。独立 C++20 实现、生成式 Ripes5 及三方验收见 [C++ 后端](../cpp/README.md)。

Signal 的 Queue 输入使用排序 ID 数组和 Queue 上的固定反向链接；输入改变直接去重入队。Python 保留输入声明检查，但不登记读取代号。独立 evaluations 只供观察。空间随输入连接数增长，不增加全量 QueueCount×SignalCount 表。静态 Signal 关系可能改变旧版本的求值、激活及重算计数。

## 可视化

Ripes5 可视化入口：

```bash
PYTHONPATH=gfsim/experiment python3 -m examples.ripes5.run --case array_sum --html
```

结果位于 `examples/ripes5/review-output/array_sum/`。其他模型也可通过 `ReviewTrace(sim)` 和 `write_html(path)` 接入通用观察器。

页面显示实际读取、保留依赖、调用时 dirty、缓存原因、Xfer 变化通知、容量边及事件，以及 Signal 的声明输入、求值、值变化和静态下游。观察器默认关闭；它扫描并保存状态，不应计入引擎性能测量。失败现场可导出，但不能视为完整提交快照。
