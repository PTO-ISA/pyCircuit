# GFSim Rule 执行与原子提交设计

本文记录当前采用的简化契约，不表示 C++ 框架已实现。Module 控制与调用见 [module.md](module.md)，Queue 资源见 [queue.md](queue.md)，调度见 [schedule.md](schedule.md)，完整记录见 [scheduler-records.md](scheduler-records.md)。

## 职责与身份

Rule 是 Module 的成员函数，是独立的原子状态转移单位，不要求独立 SimObject。Module Work 的控制流决定调用哪些 Rule；Rule 自身的控制流决定本次实际读取和提出哪些 proposal。Module 没有整体成功或失败的含义。

RuleId 标识独立实例，同时作为 Queue 的 SourceId，从 1 开始；来源 0 只用于外部驱动或隔离测试。不同实例可以复用同一段成员函数代码，但不能共享记录。编译器将 work_<rule> 和 arbitrate_<rule> 绑定到同一 RuleId；Python 作者不操作 ID 或槽位。

能访问或静态声明某 Queue 不表示本次使用了它。只读 Queue 不进入修改参与列表；每次实际提出的操作作为一个整体获准或失败，不要求所有声明 I/O 都 ready。

## 固定 Rule 记录

```cpp
struct ReadDep {
    QueueId queue;
    uint64_t stateVersion;
};

struct RuleSlot {
    std::optional<Tick> selectedTick;
    std::optional<Tick> acceptedTick;
    bool complete = false;
    CandidateArgs candidateArgs;           // 编译器生成的按值参数类型
    std::vector<ReadDep> deps;
    std::vector<QueueId> participants;
    std::optional<QueueId> waitingQueueId;
    std::vector<WakeRequest> wakeRequests;  // 如支持未来事件
};
```

记录数组在运行前按 RuleId 分配固定大小，0 留空。Proposal 值只存在 Queue 的来源槽位，Rule 不复制 payload。deps 和 participants 按 QueueId 去重；参数、捕获值使用值语义，不保留悬空的 Work 局部引用。

本批任务入队标记位于调度任务缓冲区，避免同一 delta 重复仲裁；selectedTick 避免 Module 重复调用同一 Rule 时重新准备。同一 RuleId 本 tick 的多次调用必须参数相同，否则不能视为同一次候选。

不增加 Rule.ruleGen、dirty、Queue → Rule 读取订阅、Queue 等待者列表或等待位置。

## Module 本 tick 首次选择 Rule

```text
记录本轮选择及调用参数
    → 完整且有 proposal 的旧候选存在？
    → 参数相同且全部 deps 版本相同？
        是：复用候选，重新登记 Module 订阅
        否：清理旧候选，再执行业务 Work
```

参数比较不能省略：Module 可能从自己的控制 Queue 计算参数，而该 Queue 不属于 Rule 的读取集合。版本比较只覆盖实际读取及 pop/revise 目标；纯 push 不引入输出旧状态依赖。

缓存命中时遍历 deps，把每个 Queue 的 Module 读者条目登记为 Module 本轮 newReadGen。Module Work 完成后发布 readGen；这次未选中的旧 Rule 候选取消。

必要输入不足的旧候选不能继续使用。重新计算前必须清理旧 participants 上的全部 proposal，清空 deps、waitingQueueId 和未发布事件。

## 业务 Work 和完成性

业务 Work 只读取 current、计算、提出 proposal 和记录完成性，不预约或 accept，不直接修改状态或发布事件。底层 peek 始终纯读；编译器为消息输入 payload 的实际读取路径另行生成 pop proposal。寄存器读取和普通 var 参数不消费 Queue。

每次读取，包括读空，都登记到所属 Module 的本轮订阅；Rule 的实际读取及 pop/revise 目标还记录到 deps。必要读取失败使候选不完整，其部分 proposal 必须在进入仲裁前或收尾中清理；Module 已登记的订阅保留，后续数据变化仍能唤醒它。

正常业务提前返回和 return None 都可以表示完整路径。完整但没有 Queue proposal 的路径不算 firing，不更新 acceptedTick，也不发布未来事件。只消费输入的路径有 pop proposal，可以正常获准。

生成代码可以显式使用 begin/complete/abort 入口管理候选；业务函数自身的控制流不承担资源回滚。Rule 计算只能依赖登记的 Queue、调用参数及固定配置。时间或其他可变外部值须显式表达为输入或参数。

## 原子仲裁

以下为伪代码，省略注册和错误模型检查：

```cpp
bool arbitrateRule(RuleId id, Tick tick) {
    auto& slot = ruleSlots[id];
    if (slot.selectedTick != tick || slot.acceptedTick == tick)
        return false;
    if (!slot.complete) {
        discardCandidate(id);
        return false;
    }
    slot.waitingQueueId.reset();
    if (slot.participants.empty())
        return false;

    for (auto q : slot.participants) {
        if (!queues[q].arbitrate(id)) {
            for (auto participant : slot.participants)
                queues[participant].release(id);
            slot.waitingQueueId = q;
            return false; // 释放预约，保留完整 proposal、deps 和参数
        }
    }
    for (auto q : slot.participants)
        queues[q].accept(id);
    slot.acceptedTick = tick;
    publishAcceptedPopNotifications(id);
    publishAcceptedWakeRequests(id);
    return true;
}
```

每次重试都预约全部 participants，不能只预约上次失败的 Queue。任意失败释放本 Rule 的全部临时预约，完整候选回到 pending；不影响其他 Rule。全部成功后整体 accept，确认和通知发布之间不插入其他 Rule 仲裁，也不得部分失败。

只有整体获准 pop 才提供同 tick 容量；获准候选本 tick 不撤回。未写仲裁规则却出现端口竞争，属于用户模型错误，不报错、不保证获胜者；端口限制和原子性仍然保证。

## 同 tick 复用

Module 每 tick 至多 Work 一次，current、选择和参数不变。后续 delta 对完整且已选中、尚未获准的候选直接调用仲裁入口，不重跑 Rule Work，不再比较参数或依赖版本。

必要输入缺失的尝试不因容量变化重算；新 push 数据下一 tick 才可读。生产者所属 Module 尚未 Work 时，先首次激活 Module，确定实际选择。

## 取消、提交和保留

| 情况 | 处理 |
| --- | --- |
| 必要读取失败 | 清理部分 proposal、deps 和 waitingQueueId；保留 Module 订阅 |
| Module 本轮未选中旧 Rule | 取消旧候选及未发布事件 |
| 参数或依赖版本变化 | 取消旧候选，再重新计算 |
| 完整候选预约失败 | 释放全部临时预约，保留候选，记录第一个阻塞 Queue |
| 整体获准 | 保留 accepted 到 tick 末，同 tick 不重复提交 |
| Xfer 完成 | 清空已提交候选；其他 pending 候选保留 |

waitingQueueId 是单个标记，不是多资源等待集合。端口重置时扫描该 Queue 的静态 sources，筛选标记匹配者并激活其 Module；取消和成功时清空标记。

如支持未来 tick 唤醒，请求保留在候选中，只有整体获准后才发布；部分尝试、失效或未选中的候选丢弃未发布请求。请求不替代 Queue 中的电路持久状态，Python experiment 暂不包含该能力。

时间戳要能表示 tick 0，使用 optional 或有效位；重启和快照恢复需同步恢复或失效记录。无需逐 tick 扫描重置所有 RuleSlot，但取消和提交仍要释放实际保存的捕获值。

## 验收

- [ ] Module 控制读取与 Rule 读取都不漏订阅，读空也登记。
- [ ] 参数或实际依赖变化使旧候选重新计算，缓存命中重新登记 Module 订阅。
- [ ] pop/revise 目标变化或相同 payload 的元素替换推进版本。
- [ ] 不完整 proposal 清理；完整预约失败只释放预约，保留候选。
- [ ] 不同分支只预约实际 participants，别名按资源身份去重。
- [ ] 同 tick 已有候选直接仲裁，不重跑 Work、不重复获准。
- [ ] 未选中候选和未发布事件取消，已提交候选不能跨 tick 再提交。
- [ ] 无变化的 revise 仍释放端口并通知 waitingQueueId 匹配的 Module。
