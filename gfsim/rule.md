# GFSim Rule 执行与原子提交设计

本文记录拟采用的 rule 执行契约，不表示当前框架已实现。Module 的控制与调用见 [module.md](module.md)，Queue 的资源接口见 [queue.md](queue.md)，激活和仲裁顺序见 [schedule.md](schedule.md)，Python/ACIR 表达见 [../acir/rule.md](../acir/rule.md)。

## 职责与边界

一条 rule 是一次原子状态转移。Rule Work 可以有嵌套控制流，根据实际业务路径读取当前 Queue、计算结果，并向输入、输出或内部 Queue 提出 pop、push、revise proposal。只有实际提出的操作参与本次资源仲裁；整条 rule 的这些操作一起获准或一起失败。

Module Work 决定调用哪些 rule；Module 本身不承担整体原子性。被调用的 rule 才产生一次 attempt。能访问某个 Queue 不表示它在本次 firing 中被消费或修改。输出接口位置和类型固定，但当前路径可不产生某个输出；正常的 `return None` 表示完整且无该输出的路径。Rule 不要求每条路径使用全部声明的 I/O。

## 实例身份与实际资源

RuleId 标识独立的 rule 执行实例，同时作为操作 Queue 的 SourceId，从 1 开始。RuleId 与函数代码和一次执行尝试分离：重试复用实例 ID，不同实例不共享记录。来源 0 仅保留给外部驱动或隔离测试，不是生成的 Module/Rule Work 的状态修改旁路，也不是某条跨 Queue 原子 rule。保留多实例和函数复用扩展空间，本版不展开静态实例化。

当一个生成的 rule 函数在当前 module 中只对应一个实例时，编译器可建立“函数 → RuleId”的静态绑定。Module 调用只指明函数和业务参数，内部取得 RuleId 对应的槽位；无需把 ID 和函数重复传入。若同一函数代码被多个独立实例复用，仍需独立实例包装或绑定句柄，为每个实例分配 RuleId。

Rule 可以显式接收 Queue 参数，也可以访问已连接的 module 成员。生成的 C++ 方法没有 Queue 参数，不代表语义上没有输入、输出或依赖。编译器记录实际读取和修改的资源、路径条件及源码来源。底层 `peek` 始终是纯读；Rule 读取绑定为消息输入的 Queue payload 时，编译器另行生成对应 pop proposal。直接读取 Module 的寄存器 Queue，或读取 Module 传入的普通 `var`，不生成 pop。内部 FIFO 若绑定为消息输入，也按消息输入处理。只读访问登记激活依赖，但本身不成为修改参与者。

每个 RuleId 对应一个固定槽位。下标就是身份，不在槽位中重复存 ID：

```cpp
struct WakeRequest {
    Tick tick; // 必须晚于当前 tick
    ModuleId module;
};

struct RuleSlot {
    std::optional<Epoch> lastAttemptEpoch; // 跨尝试保留，用于同 delta 去重
    std::optional<Tick> lastAcceptedTick;  // 跨尝试保留，用于同 tick 去重
    bool complete = false;                 // 只对 lastAttemptEpoch 有效
    std::vector<QueueHandle> participants; // 本次实际提出 proposal 的 Queue
    std::vector<WakeRequest> wakeRequests; // 本次请求的未来 tick 唤醒
};

std::vector<RuleSlot> ruleSlots; // ruleSlots[ruleId]；0 留空，初始化后固定大小
```

槽位和唤醒请求是仿真管理信息，不是电路状态。QueueHandle 只示意仲裁、取消、确认接口。Proposal 内容仍在 Queue 的来源槽位；RuleSlot 只记录实际参与的 Queue，按资源身份去重。只读 peek 不加入 participants。延迟唤醒只保存时间和目标 ModuleId；请求 payload 与计时状态仍由内部 Queue 表达。

## 调用与去重

生成的调用保护负责复用槽位，调度器当前批次只记录实际尝试过的 RuleId：

```cpp
void tryWorkRule(RuleId id, Epoch now, auto&& work) {
    auto& slot = ruleSlots[id];
    if (slot.lastAcceptedTick == now.tick || slot.lastAttemptEpoch == now)
        return;

    // 前一尝试必须已经仲裁收尾；不能覆盖未处理的 proposal。
    slot.lastAttemptEpoch = now;
    slot.complete = false;
    slot.participants.clear();
    slot.wakeRequests.clear();
    localAttemptedRuleIds.push_back(id); // 本 Module/线程的本批列表
    work(id, slot);                  // 早退仍保留本次尝试
}
```

同一 rule 实例每 delta 至多尝试一次、每 tick 至多获准一次。失败尝试在方案 B 的后续 delta 可以重算 Work；成功实例不会同 tick 重复提交。未被 module 调用的 rule 不形成 attempt，调用入口不能把全部 I/O ready 作为统一必要条件。

## Work、分支与 complete

Work 只做安全读取、计算、propose、登记未来唤醒请求和正常完成标记，不预约、accept、cancel 或直接向调度队列入队。

```cpp
void MyModule::work_completion(RuleId id, RuleSlot& slot) {
    const auto* event = completionInput.tryPeek();
    if (!event)
        return;

    completionInput.proposePop(id); // 编译器按消息输入读取另行生成。

    if (event->epoch == currentEpoch.peek()) { // 寄存器只读，不产生 pop。
        const auto* config = configInput.tryPeek();
        if (!config)
            return; // 保留此前 proposal，由 Arbitrate 收尾。

        configInput.proposePop(id); // 编译器按消息输入读取另行生成。
        entries[event->index].proposeRevise<&Entry::done>(
            id, true);
        ackOutput.proposePush(id, makeAck(*event, *config));
        // 需要延迟响应时：slot.wakeRequests.push_back({currentTick() + 3, moduleId});
    }

    // 旧 epoch 路径只消费 completion，也是正常完成。
    slot.complete = true;
}
```

代码为结构示意，省略资源注册、下标合法性和调用记录实现。普通返回、return None 和提前的正常业务返回均标记 complete=true；必要读取不足的内部退出保持 false。前端必须区分这两种出口，不能只在 C++ 函数末尾机械追加标记。

Work 中必要读取失败仍需仲裁收尾；此前 proposal 保持 Pending，唤醒请求留在槽位，二者都不能提前生效。Arbitrate 负责取消，不需要 Work 回滚。

## Rule Arbitrate 与原子性

屏障后合并各局部列表。本批 `attemptedRuleIds` 中的每条 Rule 都必须仲裁收尾，包括 `complete=false` 和无 proposal 的尝试：

```cpp
bool MyModule::arbitrate_completion(RuleId id, Tick tick) {
    auto& slot = ruleSlots[id];
    if (!slot.complete || slot.participants.empty()) {
        cancelAttempt(id, slot);
        return false;
    }

    for (auto queue : slot.participants) {
        if (!queue.arbitrate(id)) {
            cancelAttempt(id, slot);
            return false;
        }
    }

    for (auto queue : slot.participants)
        queue.accept(id);

    slot.lastAcceptedTick = tick;
    for (const auto& wake : slot.wakeRequests)
        scheduler.enqueue(wake.tick, wake.module); // 整条 Rule 获准后才入队
    slot.wakeRequests.clear();
    publishAcceptedPopNotifications(id);          // 方案 B 的下一 delta 通知
    return true;
}

void cancelAttempt(RuleId id, RuleSlot& slot) {
    for (auto queue : slot.participants)
        queue.cancel(id);
    slot.participants.clear();
    slot.wakeRequests.clear(); // 未获准的延迟唤醒从未进入调度队列
    slot.complete = false;
}
```

未 complete 时只清理候选；资源失败时同时释放临时预约。全部预约成功后，`accept` 和调度事件发布必须无失败；中间不插入其他 Rule 仲裁。取消只作用于本次来源。

仲裁后槽位保留，但本次动态字段仅在 `lastAttemptEpoch` 对应的尝试中有效；下次尝试开始时清空并复用其容器容量。Queue 自己保留 accepted proposal 到统一 Xfer。不能在某个 Queue 局部 accept 后提前发布唤醒或获准 pop 通知。Work 和 Arbitrate 都不直接改变 current。

正常完成但没有 Queue proposal 的尝试不构成 firing；即使记录了唤醒请求，也不会发布。只消费输入的路径已有 pop proposal，可以正常 firing。独立的“只唤醒”Rule 若需要支持，须另定其原子效果契约。嵌套原子域、固定全 I/O 和缓存不属于基础机制。

## 生命周期与避免重置

| 记录 | 生效区间 | 边界处理 |
| --- | --- | --- |
| `lastAttemptEpoch` | RuleId 的历史时间戳 | 不按 delta 或 tick 清空；与当前 (tick, delta) 比较 |
| `lastAcceptedTick` | RuleId 的历史获准 tick | 不按 tick 清空；与当前 tick 比较 |
| RuleSlot 的 `complete`、`participants`、`wakeRequests` | `lastAttemptEpoch` 所标记的尝试 | 本次仲裁收尾后不再读取；下次尝试开始时清空复用 |
| `attemptedRuleIds` | 本个 (tick, delta) 批次 | Work 登记，仲裁后清空 |
| Queue 的失败 proposal / 临时预约 | 本次尝试至仲裁失败 | 当场 cancel，只清理自身来源 |
| Queue 的 accepted proposal / 端口摘要 | 整体获准至本 tick 的 Xfer | 跨 delta 保留；Xfer 后清理 |
| Queue 的 current | 持久电路状态 | Xfer 更新，下一 tick 才对 Work 可见 |

下一 delta 的失败 Rule 可用相同槽位重试；已获准的 Rule 因 `lastAcceptedTick` 相同而跳过。下一 tick 时间戳不再匹配。只清空本批 `attemptedRuleIds`，不逐 tick 扫描重置全部 RuleSlot；Queue 的 accepted proposal 仍保留到 Xfer。

时间戳使用可表示 tick 0 的 `std::optional`（或等价有效位），不把数字 0 当作无效值。仿真重新开始、恢复到较早时间或加载快照时，必须同步恢复这些时间戳，或更换运行代号使旧时间戳失效；“无需逐 tick 重置”不等于跨独立运行复用旧记录。

Queue 来源槽位仍需在 cancel/Xfer 后清理实际捕获的值和预约，不能只依靠时间戳假装这些对象不存在。按本 tick 实际触及的槽位登记清理列表，可以避免每 tick 扫描所有注册来源；详见 [queue.md](queue.md)。

## 验收

- [ ] 正常业务出口（包括 `return None`）标记 complete，必要读取失败不标记。
- [ ] 先 propose 再 peek 失败时，Arbitrate 清理全部本 rule 部分候选，没有端口或预约残留。
- [ ] 实际路径未使用的输入、输出不参加本次预约；过期消息可以只被消费。
- [ ] `peek` 始终纯读；消息输入 payload 读取由编译器另行插入一次 pop；寄存器读取及普通 `var` 不产生 pop。
- [ ] `complete=true` 且没有任何 Queue proposal 的尝试不计为 firing，不更新获准时间戳。
- [ ] Queue 别名与多个操作按真实资源身份去重，同一 Queue 的本 rule 操作一起检查。
- [ ] 一项资源预约失败只取消本 rule 的候选，不影响同 module 的独立 rule 或已获准操作。
- [ ] 同一 delta 不重复尝试、本 tick 不重复获准；失败后下一 delta 可以重算 Work。
- [ ] Tick 0 正常执行；跨 delta、跨 tick 无需批量重置 RuleSlot 时间戳。
- [ ] 同一 RuleId 的旧尝试仲裁收尾后才复用槽位；已获准 proposal 跨 delta 保留至 Xfer。
- [ ] 失败 Rule 的延迟唤醒不入调度队列；成功后仅发布本次记录的请求。
- [ ] 重启或快照恢复不会误用另一次运行的时间戳。
- [ ] Rule 身份与函数定义、单次尝试分离，多实例不共享记录。
