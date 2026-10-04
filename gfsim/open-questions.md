# GFSim 待决问题

本文件只记录整理设计时发现的未决语义和缺口，不替它们作决定。已确认契约见 [框架 spec](spec.md)。实验行为用于说明现状，不自动成为框架契约。

<a id="q02"></a>

## Q02. 父子 Module 的构造、调用与激活

- 本轮范围：平级 Module 通过 Queue 交换持久效果；同一 Module 可以向成员 Rule 传普通 var，按值比较并标记 dirty。
- 已实现：Signal 从 Queue current 派生，全部 Xfer 后求值；Module／Rule 均可只读访问，输出变化使用静态 Module 关系与 Rule 掩码唤醒和标脏。不属于 Module 向 Module 直接写 var，也不引入 delta Work。
- 下一步：Signal 之间的依赖，以及 Module→Module var 的传播顺序、动态依赖、delta Work 与环检测尚未定义；本轮未实现，不能把它视为已有接口。
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
- 影响：外部更新须统一遵守 current/Xfer/通知边界。当前 Ripes5 实验通过程序 ROM 执行指令输入工作负载，未提供来源 0 外部驱动协议；旧 `step(wake=...)` 已移除。

<a id="q11"></a>

## Q11. 代号生命周期、重启与状态恢复

- 已确定：C++ 核心的 tick、readGen、stateVersion、任务代号和计数使用 uint64；递增及事件到期时间加法在溢出前抛出错误并终止该仿真实例，不回绕。执行异常或实际动态环也禁止继续使用该实例，不承诺回滚。
- 已实现：workedTick、selectedTick、acceptedTick 等时间戳使用 optional 区分未发生与 tick 0；任务以检查过的 tick+1 为非零代号；版本、读取代号和事件时间具有窄边界测试。
- 未决：重启／快照恢复时哪些订阅和候选恢复，哪些统一失效。溢出终止策略不定义恢复协议。
- 影响：旧条目不能因代号碰撞误判有效，已提交候选不能恢复成可再次提交候选。核心 spec 不据此新增恢复机制。

<a id="q12"></a>

## Q12. 并行执行与 C++ ABI 演进

- 现状：本文记录流程按单线程描述，Module Work 更新所属 Module 的控制读取代号与 Rule 读者位图。
- 未决：是否支持并行 Work；跨 Module 的 dirty 通知、候选准备与阶段屏障如何实现。
- 已实现：单线程 C++20 源码接口使用 QueueBase/Queue<T>、固定实例指针与 Module Work/Rule 仲裁入口、显式构造绑定和生成类内强类型参数缓存，见 [cpp/README](cpp/README.md)。可独立编译、安装并通过 `gfsim::gfsim` 链接。
- 未决：跨版本二进制 ABI 稳定性和编译器自动生成接口的演进；目前提供源码级集成，不承诺稳定二进制布局。
- 影响：当前单线程实现不定义并行写入、屏障或恢复语义。

<a id="q14"></a>

## Q14. 读者布局与成本（Python 与 C++ 已实现）

- 已确定：Queue 保存固定可能读者链接用于通知；新增按 ModuleId 索引的 module_slots 数组，直接定位局部资源槽位，未声明为 -1。所有映射在构造时确定。
- 已实现：Module 控制读取保留代号；Rule 实际读取用每资源位图、Rule readSlots 和 Module dirtyWords 管理。缓存命中不续订；重算／未选中才删除旧 Rule 读取关系；提交／abort 保留。
- 正确性条件：可访问资源声明覆盖所有分支和动态数组表项，别名归一；实际通知依据运行时读者位，不能静态标脏全部 Rule。省略 Python module_queues 时保守允许全部 Queue，用于兼容旧例子。
- 成本：每 Module A 个 Queue、R 条 Rule 时，需要 A×ceil(R/64) 个读者字，另有 A 个控制代号、ceil(R/64) 个 dirty 字及固定链接。本次选择直接数组映射，额外增加 (QueueCount + SignalCount)×ModuleCount 个槽位项，需计入空间，不能再宣称总元数据完全稀疏。大资源表／多 Rule 的稠密位图成本需要后续负载测量。
- 已实现：读取接口根据 activeModule/activeRule 自动登记依赖，读取与消费仍分离。
- 已实现：Signal 独立保存按 Module 分组的固定 Rule dirty 掩码；Signal 自己的 Queue 输入使用固定链接，不登记实际输入代号，额外空间随可能连接数增长，不增加 QueueCount×SignalCount 全量映射。
- 已实现：C++ 自动登记、Module 局部位图、跨 64 位读者、Signal 静态输入与输出过滤及无效果候选缓存。固定槽位映射、来源槽位和任务缓冲在 freeze 后地址稳定；缓存命中不扫描版本或重新登记读取。
- 已实现：C++、当前 Python、固定原生 Ripes 的三方逐拍验收与同 CPU 串行测速，入口见 [C++ Ripes5](cpp/examples/ripes5/README.md)。具体结果见 [报告](cpp/report.md)，不预设普遍加速比。
- 待办：更多编译器负载下的布局成本和性能测量，以及进一步的稀疏布局优化。

<a id="q15"></a>

## Q15. 前端 lowering 与 C++ 值类型的边界

- 本轮交付：可安装 runtime 和按未来输出形态手写的 Ripes5；实际 pyCircuit 前端／代码生成器接入另行完成。静态绑定表、普通 Module/Rule 成员函数、Signal helper 与宿主 testbench 已分开，见 [操作映射](cpp/README.md#编译操作对应)。
- 已支持：bool、标准定宽整数、std::array、嵌套值 struct；字段式比较，成员指针路径按值捕获并在 Xfer 赋值。Queue array 每项独立注册，构造后固定身份。
- 未决：AC 任意位宽整数、bit-field、动态 struct 数组字段路径和指针／可选对象路径如何 lowering；当前不能映射为已有成员指针接口，也没有新增动态路径 ABI。
- 生成要求：C++ 的有符号溢出和部分整数提升不能直接当成硬件位宽运算；当前模型使用 uint32 与显式符号位比较。前端须给出完整位宽转换规则。
- 未决：同一候选重叠字段 revise 的源码语义及诊断责任。当前样例使用不重叠字段或单次整值替换，不把延迟动作的插入顺序提升为 ACIR 冲突语义。
- 已实现检查：未注册／其他实例的资源读取、Signal 之间读取、helper 修改或请求事件均报错；Signal 声明完整性由构造保证，Debug 检查实际访问，Release 不逐次查找输入声明表。无法通过 runtime 证明 helper 没有读取普通可变宿主成员；纯性仍须由生成器保证。
- 生命周期约束：资源对象不移动，Queue/Signal 必须存活到 Simulator 析构完成；runtime 解除回调绑定，不提供重新 attach、快照恢复或模块析构通知协议。
