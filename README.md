# ACIR 与 GFSim 设计说明

这些文档记录拟采用的重构契约，不表示当前 pyCircuit 已完成实现或通过验收。

## 核心执行模型

```text
Queue 状态或事件激活 module
    → module Work 执行控制流，调用 rule Work
    → Work 屏障
    → 所有被调用 rule 进入 Arbitrate 收尾
        complete=false：取消本次 proposal
        complete=true 且有 proposal：预约实际资源，整体 accept；失败释放预约并保留候选
        complete=true 且无 proposal：正常结束，不产生 firing
    → tick 结束，Queue 统一 Xfer
```

Module 和 rule 都可以表达控制流。Module 不承担整体原子提交，rule 是原子单位。所有 Work 读取同一份 current，Queue 按 `revise → pop → push` 提交。

当前采用的记录方案：Queue 状态变化按 Module.readGen 激活下一 tick；Rule 跨 tick 首次被选中时比较参数和实际依赖版本；同 tick 已有完整候选直接重试仲裁。Rule 只保存单个 waitingQueueId，端口重置时扫描 Queue 的静态来源，不维护反向等待列表。

## 文档分工

| 文档 | 内容 |
| --- | --- |
| [ACIR rule 契约](acir/rule.md) | Python/HIR/ACIR 边界、路径条件、读取合法性及 RTL firing 信息 |
| [Module](gfsim/module.md) | Module 连接、控制逻辑、激活依赖与 rule 调用 |
| [Rule](gfsim/rule.md) | 实例身份、Work、complete、实际资源与原子仲裁 |
| [调度](gfsim/schedule.md) | 激活、执行阶段、仲裁收尾、A/B 容量策略与 delta |
| [Queue](gfsim/queue.md) | 元素、来源槽位、资源接口、预约摘要与 Xfer |
| [Struct](gfsim/struct.md) | 嵌套数据、编译期成员路径和延迟字段赋值 |
| [完整记录方案](gfsim/scheduler-records.md) | 静态表、Module 订阅、Rule 依赖与候选生命周期 |
| [读取与版本匹配](gfsim/read-tracking-draft.md) | 读取代号、实际依赖和跨 tick 缓存判断 |
| [候选复用](gfsim/gfsim-cache.md) | 跨 tick 复用、同 tick 仲裁及单个等待标记 |

[设计审查记录](design-review.md)标明已解决的矛盾和尚未收敛的语义缺口，并列出验收场景。

## 已确定的范围

Module 负责连接和控制，Rule 负责本次原子状态转移，Queue 保存持久状态与 proposal；调度器按 module 激活、按 rule 仲裁、按 Queue Xfer。Rule Work 的 complete、失败清理、资源身份和去重统一见 [Rule 文档](gfsim/rule.md)。

每个 RuleId 对应一个可复用的 RuleSlot，记录 complete、实际读取及参与的 Queue、参数和阻塞 Queue；完整候选预约失败只释放预约，可以跨 tick 保留。未来唤醒请求如支持则保留在候选中，只在整体获准后进入调度队列，候选取消时清除。

底层 Queue `peek` 始终纯读。Rule 读取消息输入 payload 时，编译器另行生成在该 rule 成功时提交的 pop proposal；读取寄存器和 Queue 状态不消费。Module 可读自己的寄存器并传普通 `var`，不预读 Rule 的消息输入。生成的电路状态修改均归属 RuleId；来源 0 仅供外部驱动或隔离测试。

## 保留的备选与边界

A 不使用本拍 pop 释放的空间；B 使用已经整体获准的 pop，并要求消费先行的拓扑仲裁和 delta 唤醒。两者继续保留，没有选定默认方案，也不视为逐 tick 等价的性能替换。

Queue 重叠 revise、自身 pop/push 的局部空间政策、来源 0 外部驱动竞争优先级和公平性等边界见各自文档。固定全部 I/O、嵌套 rule 和联合原子组不属于当前已确定的契约。
