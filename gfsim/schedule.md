# GFSim Module 激活与 Rule 仲裁调度

本文采用当前讨论的简化记录方案，不表示 C++ 框架已经实现。记录见 [scheduler-records.md](scheduler-records.md)，Module、Rule 和 Queue 的契约分别见 [module.md](module.md)、[rule.md](rule.md)、[queue.md](queue.md)。以下同 tick 传播按 B 容量策略描述；A 不使用其他 Rule 本拍获准 pop 的空间，仍作为容量政策备选。

## 两条推进路径

```text
跨 tick：
Queue 状态变化 → 按 Module 读取订阅激活
    → Module Work 选择 Rule
    → 参数和依赖版本匹配则复用候选，否则重新计算

同 tick（B）：
整条 Rule 的 pop 获准 → 查静态可能生产者
    → 首次激活的 Module 执行 Work
    → 已有完整候选直接重试仲裁
```

Module 是激活和控制流单位；Rule 是原子仲裁单位，仍为 Module 成员函数；Queue 是持久状态和 Xfer 单位。Module 没有整体成功或失败状态。

## 时间与任务

使用 (tick, delta)。Tick 是硬件周期，delta 是当前 tick 的任务批次，不是额外电路时钟。

Module 任务按 ModuleId 去重，Rule 仲裁任务按 RuleId 去重。运行前分配固定容量 ID 数组、有效长度和代号数组，当前与下一 delta 的缓冲区交替使用。下一 tick 的 Module 任务另有一组缓冲区。

初始化、明确的时钟／外部事件和 Queue 通知都是激活来源。时间相关控制必须由明确事件驱动，不能依赖无关 Queue 恰好变化。Rule 请求的未来事件先保留在候选中，只有整体获准后才发布；缓存计算中使用的可变时间须参与输入或参数匹配。

所有 Work 读取同一份 current。后续 delta 只改变预约和 accepted 资格，状态在 tick 末统一 Xfer。

## Module 首次 Work 和跨 tick 候选验证

Module 每 tick 至多 Work 一次，首次激活可以发生在任意 delta：

1. 标记 workedTick，准备 newReadGen。
2. 重新执行 Module 控制流，确定实际选中的 Rule 及调用参数。
3. 每条本 tick 首次调用的 Rule 检查完整候选、参数和实际 Queue 版本；有效则复用，否则清理旧候选并计算。
4. 实际读取直接登记 Queue 的 Module 读者代号；缓存命中也用 deps 重新登记。
5. 取消本轮没有选中的旧 Rule，发布 Module.readGen。

Module 控制流不观察预约、accepted pop 或 canPush 等 delta 内变化的资格。调用不代表获准；Module 提前返回只停止后续调用，不撤销此前独立 Rule。

必要输入缺失的部分 proposal 必须清理，但已登记的 Module 订阅保留。完整且无 proposal 的路径不算 firing。缓存验证见 [read-tracking-draft.md](read-tracking-draft.md)。

## 仲裁与同 tick 复用

Work 阶段结束后，按消费先行静态顺序仲裁完整候选。每次遍历全部实际 participants 预约：

- 任一失败：释放本 Rule 的全部临时预约，保留完整 proposal，记录第一个 waitingQueueId。
- 全部成功：整体 accept，记录 acceptedTick；发布实际获准 pop 的容量通知。

确认与通知发布之间不插入其他 Rule 仲裁，accepted 本 tick 不撤回。每个 Queue 每 tick 至多获准一个 pop 和一个 push；字段 revise 边界见 queue.md。

current、Module 选择和调用参数在本 tick 内不变，因此后续 delta 对已有完整候选直接调用仲裁入口，不重新进入 Rule Work，不再次比较参数或版本。每条 Rule 本 tick 至多获准一次。

## B 的容量传播

```text
数据流：A → Q1 → B → Q2 → C
消费先行仲裁顺序：C → B → A
```

编译器为每个 Queue 建立静态 producers：可能向它 push 的 Rule。某次只有实际获准 pop 的 Queue 传播容量，未走到的分支不传播。

```cpp
for (auto producer : Q.producers) {
    auto& module = modules[ownerModule(producer)];
    auto& rule = rules[producer];
    if (module.workedTick != tick)
        enqueueNextDeltaModule(ownerModule(producer));
    else if (rule.selectedTick == tick && rule.complete
             && !rule.participants.empty() && rule.acceptedTick != tick
             && !inCurrentBatch(producer))
        enqueueNextDeltaRule(producer);
}
```

若生产者已在当前批次中，消费先行顺序使其稍后检查容量，不重复安排下一 delta。若 Module 尚未 Work，静态通知不保证生产者一定被实际选中。

临时 pop 预约不提供空间，获准 pop 才提供。获准 push 的新数据在本 tick 不可读。自身 pop/push 的局部策略见 queue.md，Python experiment 允许同一候选复用自身 pop 空间。

初版支持无环的静态容量依赖；排除 Rule 自身边后仍有静态环则不支持。Module 之间跨 tick 的数据反馈不等于同 tick 容量依赖环。失败本身不重新入队，每次传播来自新获准 pop；有限实例和每 tick 一次获准限制保证不会因失败自行无限重试。

未写仲裁规则却出现端口竞争，属于用户模型错误，不报错、不保证哪个候选获胜。内部顺序不提供用户优先级契约，也不保证最大获准集合或公平性。

## tick 末 Xfer 与下一 tick 通知

同 tick 任务耗尽后，按 QueueId 去重处理实际使用过的 Queue。各 Queue 只提交 accepted 操作，按 revise → pop → push 更新 current，清理本拍端口和 accepted 摘要，保留其他 pending proposal。

状态或元素身份变化推进 stateVersion。Pop 后 push 相同 payload 仍是新元素，推进版本；无变化的 revise 和端口重置不推进版本。

所有 Queue 提交完成后：

```cpp
// 状态变化：读取订阅。
for (auto q : changedQueues)
    for (auto [moduleId, savedGen] : queues[q].readers)
        if (savedGen == modules[moduleId].readGen)
            enqueueNextTick(moduleId);

// 端口重置：静态来源筛选。
for (auto q : usedQueues)
    for (auto ruleId : queues[q].sources)
        if (rules[ruleId].waitingQueueId == q)
            enqueueNextTick(ownerModule(ruleId));
```

sources 包含可能 pop/push/revise 的 Rule，不能只查 producers。Rule 只保存 waitingQueueId，不维护 Queue.waiters、等待位置或双向等待集合。通知扫描按该 Queue 的静态可能来源数量计算。

已提交候选清空，未获准完整候选可跨 tick 保留。下一 tick 被调用前仍须验证参数和依赖，不能未经验证直接 accept。即使 current 未变，端口重置仍需通知对应阻塞者。

## A 容量政策

A 不使用其他 Rule 本拍获准 pop 的空间，满 Queue 的上游生产等待下一 tick。它可保留相同的读取订阅、跨 tick 版本检查和原子候选记录，只是不执行 B 的同 tick 容量传播。

A/B 不保证逐 tick 等价。本次简化不扩展来源 0、字段重叠、子 Module、定时接口或并行执行的支持范围。

## 验收

- [ ] 同 tick Module Work 一次，选择和参数不变，已有候选直接重试仲裁。
- [ ] Module 控制读取、Rule 读取、读空和缓存依赖都登记订阅。
- [ ] 跨 tick 参数或依赖变化重算，未选中候选取消。
- [ ] 失败只释放临时预约，完整候选保留，accepted 不重复提交。
- [ ] B 只传播实际整体获准 pop，delta 不提交、不转发新数据。
- [ ] Xfer 后才通知下一 tick，元素替换和无变化 revise 的端口通知不遗漏。
- [ ] waitingQueueId 清理正确，静态来源过滤不激活无关等待者。
