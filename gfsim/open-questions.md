# GFSim 待决问题

本文件只记录整理设计时发现的未决语义和缺口，不替它们作决定。已确认契约见 [框架 spec](spec.md)。实验行为用于说明现状，不自动成为框架契约。

<a id="q02"></a>

## Q02. 父子 Module 的构造、调用与激活

- 已确定：子 Module 只有输入、输出 Queue 效果；子 Module 的读取订阅归属自己。
- 未决：父 Module 调用子 Module 是构造连接、直接执行 Work，还是提交激活任务？父控制分支是否限制子 Module 本 tick 的独立 Queue 唤醒？
- 影响：父子 Module 的 workedTick、Work 屏障和控制选择。现有平级调度实验没有覆盖这些语义。

<a id="q03"></a>

## Q03. 独立 Cell 状态对象

- 现状：寄存器可用受限容量 1 Queue 表示，但用户保留增加 Cell 的可能。
- 未决：是否增加 Cell；若增加，它的读取、版本、原子写入和订阅接口如何与 Queue 统一？
- 影响：目前以 QueueId 表达的依赖与参与者记录。不能将“目前用 Queue”写成永久排除 Cell。

<a id="q06"></a>

## Q06. 已登记事件的取消／覆盖

- 已确定：调度器持有 eventQueue，统一由 wakeup 登记；到期事件在 tick 开始激活 Module Work，同 Module 同 tick 去重。
- 已确定：除调度器自身 wakeup 外，只有 Rule 可以主动请求未来事件。请求保存在候选，整体获准后发布；delay ≥ 1，按获准 tick 计算到期时间。纯事件 Rule 可以获准。
- 已确定：不完整、失效或未选中候选丢弃未发布请求；已发布事件独立于候选和 Module.readGen，不因候选清理失效。
- 未决：已登记事件是否支持取消／覆盖，以及相应身份和接口。此前提出过“先不支持”的建议，尚未确认；不自行加入机制。
- 影响：基础事件登记和到期语义已明确，实验已实现并由完整电路验证。若需要取消／覆盖，须在 `(wakeTick, ModuleId)` 之外明确如何定位事件。

<a id="q07"></a>

## Q07. 外部驱动协议

- 已确定：启动时在 tick 0 激活全部已构造 Module，各 Work 一次建立实际读取订阅；全部 Work 完成后才开始仲裁。
- 已确定：不依赖 Queue 变化的源 Module 可以由获准 Rule 的未来事件驱动持续激活。
- 未决：来源 0 如何完成 proposal、仲裁、确认和提交，外部状态输入如何进入仿真。
- 已确定：获准 pop 只通知已有完整候选的唯一生产者仲裁。候选提交后已清空时，这类通知不为源 Module 运行 Work。
- 已确定：来源 0 不绕过原子规则，不自动组成跨 Queue 事务；每个 Queue 的 pop/push 来源分别至多一个 Rule，当前不加检测，违反时不保证运行结果。多 revise 等端口竞争不在本版范围。
- 影响：外部更新须统一遵守 current/Xfer/通知边界。当前实验通过预装请求 ROM 的 Source Module 输入请求，未提供来源 0 外部驱动协议；旧 `step(wake=...)` 已移除。

<a id="q11"></a>

## Q11. 代号生命周期、重启与状态恢复

- 已确定：C++ 核心的 tick、readGen、stateVersion、任务代号和计数使用 uint64；递增及事件到期时间加法在溢出前抛出错误并终止该仿真实例，不回绕。执行异常或实际动态环也禁止继续使用该实例，不承诺回滚。
- 已实现：workedTick、selectedTick、acceptedTick 等时间戳使用 optional 区分未发生与 tick 0；任务以检查过的 tick+1 为非零代号；版本、读取代号和事件时间具有窄边界测试。
- 未决：重启／快照恢复时哪些订阅和候选恢复，哪些统一失效。溢出终止策略不定义恢复协议。
- 影响：旧条目不能因代号碰撞误判有效，已提交候选不能恢复成可再次提交候选。核心 spec 不据此新增恢复机制。

<a id="q12"></a>

## Q12. 并行执行与 C++ ABI 演进

- 现状：本文记录流程按单线程描述，Module Work 直接写入 Queue 的固定读者数组。
- 未决：是否支持并行 Work；共享 Queue 读者数组的写入、候选准备与阶段屏障如何实现。
- 已实现：单线程 C++20 源码接口使用 QueueBase/Queue<T>、固定实例指针与 Module Work/Rule 仲裁入口、显式构造绑定和生成类内强类型参数缓存，见 [cpp/README](cpp/README.md)。可独立编译、安装并通过 `gfsim::gfsim` 链接。
- 未决：跨版本二进制 ABI 稳定性和编译器自动生成接口的演进；目前提供源码级集成，不承诺稳定二进制布局。
- 影响：当前单线程实现不定义并行写入、屏障或恢复语义。

<a id="q14"></a>

## Q14. Queue 读者表的空间成本与静态缩减

- 现状：当前 spec 和 Python experiment 为每个 Queue 分配长度为 ModuleCount 的 readGen 数组，按 ModuleId 直接索引。永远不会读取该 Queue 的 Module 也占槽位，总空间为 QueueCount × ModuleCount × sizeof(ReadGen)。
- 可缩减方向：编译器为每个 Queue 收集所有可能读取它的 Module，包括这些 Module 内 Rule 的读取；运行前仅为这些 Module 构造固定的 `(ModuleId, readGen)` 表。Rule proposal 来源槽位已经按相关 Rule 分配，无需按全部 Rule 展开。
- 正确性条件：静态读者集合必须覆盖所有可能执行路径。Queue array 的动态下标须覆盖所有可能访问的表项；每个表项仍按实际读取登记 readGen，通知时只唤醒代号匹配的 Module。缓存命中也须向对应局部槽位重登订阅。
- 未决：是否采用该布局，以及读取时如何定位局部槽位；可考虑编译器生成槽位编号或小表查找。当前仅记录问题，尚未修改 spec 或实验的存储布局。
- 影响：代号数据空间可缩减为所有 Queue 的静态可能读者数量之和 × sizeof(ReadGen)，另计 ModuleId 和表描述信息；状态通知只扫描该 Queue 的静态可能读者。订阅失效和跨 tick 唤醒语义沿用当前方案。
