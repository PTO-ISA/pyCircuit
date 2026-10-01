# GFSim 候选复用与按需仲裁

本文描述当前采用的简化方案，不表示 C++ 框架已实现。完整记录见 [scheduler-records.md](scheduler-records.md)，Rule 原子性见 [rule.md](rule.md)，调度见 [schedule.md](schedule.md)。

## 两种推进路径，共用候选

```text
跨 tick：
Queue 状态变化 → 按 Module 读取代号激活
    → Module Work 重新选择 Rule
    → 参数和实际依赖版本匹配则复用，否则重算

同 tick（B）：
整条 Rule 的 pop 获准 → 查静态生产者
    → Module 尚未 Work：先执行一次 Work
    → 已有完整候选：直接重试仲裁
```

跨 tick 才判断计算是否仍有效；同 tick 只推进仲裁资格。完整候选及 payload 只保存一份，proposal 位于 Queue 来源槽位。

## 缓存的内容与有效性

Rule 保存：

- 实际读取的 QueueId 和 stateVersion，包括 pop/revise 目标。
- 实际提出 proposal 的 QueueId 列表。
- 候选参数、完整性及 waitingQueueId。
- 选择和获准时间戳。

只有完整且有 proposal 的未获准候选参加缓存复用。必要输入不足的部分计算清理；正常无 proposal 的路径不算 firing。

Module 本 tick 首次调用 Rule 时，检查候选参数和全部实际依赖版本。匹配则保留 proposal，跳过计算；任一不匹配则清理旧候选和等待标记，再重新 Work。

调用参数必须比较。例如 Module 读取 mode 并调用 R(mode)，R 自己只读 input；mode 变化不会改变 R 已登记的 input 版本，但会改变结果。

纯 push 的输出空间变化不使 payload 计算失效；容量和端口在仲裁阶段重新检查。Pop 后 push 相同值也改变元素身份，必须推进 Queue 版本；无变化的 revise 不推进版本。

Rule 不使用 readGen/dirty，也不建立 Queue → Rule 的动态读取订阅。缓存命中仍需遍历 deps，为 Module 的新 readGen 重新登记 Queue → Module 关系，因此主动标 dirty 并不能省掉整个依赖遍历。

## 同 tick 直接重试仲裁

Module 控制流、调用参数和 Queue current 在本 tick 内不变。首次选择已经完成计算或缓存验证后，后续 delta 不进入 Rule Work 的缓存保护入口，不再做版本或参数比较。

每次重试仍预约所有 participants：

```text
任一失败 → 释放全部临时预约，保留完整候选
全部成功 → 整体 accept，本 tick 不再撤回
```

不能只预约上次失败的 Queue，因为此前取得的临时预约已经释放。只有整体获准 pop 才公开空间，不传播临时预约或新 push 数据。

## 单个等待标记

Rule 只保存第一次预约失败的 waitingQueueId。不维护 Queue.waiters、等待位置或多 Queue 等待集合。

重新仲裁前清空旧标记；失败时保存新的阻塞 Queue；取消和成功时清空。tick 边界，处理实际使用过的 Queue 的端口重置：

```cpp
for (auto ruleId : Q.sources) {
    if (rules[ruleId].waitingQueueId == Q.id)
        enqueueNextTick(ownerModule(ruleId));
}
```

Q.sources 是编译期的可能修改来源。它可能含未走到的分支，但只有 waitingQueueId 匹配者被通知。该扫描覆盖端口重置导致的重试，即使 current 没变，也不能省略。它不代替正常的读取订阅。

## 取消、提交和跨 tick 保留

- 不完整候选：清理部分 proposal 和等待标记，保留 Module 已登记的读取订阅。
- Module 本轮未选中的旧 Rule：取消候选。
- 参数或依赖版本失效：清理旧候选，再重新计算。
- 完整候选仲裁失败：仅释放预约，候选跨 delta、跨 tick 保留。
- 获准候选：tick 末 Xfer，随后清空候选，不能再次提交。

所有 Queue Xfer 完成后才发布下一 tick 的读取及端口通知。Xfer 不清空其他 Rule 的 pending proposal。没有任务的 tick 不进行无关的全表重置。

## 成本与边界

版本比较和重新登记订阅的成本按实际依赖数量计算；端口通知按该 Queue 的静态可能来源数量计算。先采用这些直接扫描，若实际负载证明它们成为瓶颈，再增加更精细的索引。

Queue array 的依赖与订阅按实际表项稀疏建立，旧读取关系的懒失效仍需要后续清理。未写仲裁规则的端口竞争属于用户模型错误，不报错、不保证获胜者；端口限制和整条 Rule 原子性仍然保证。缓存不解决容量依赖环，也不保证最大获准集合或公平性。

当前 Python experiment 保留版本比较，但还使用 Module.reads、反向等待列表，并在后续 delta 进入 Rule Work 保护入口；本文的新布局尚未全部实现。本次仅更新设计文档。
