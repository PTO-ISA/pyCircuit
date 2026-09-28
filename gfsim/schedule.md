# GFSim Module 激活与 Rule 仲裁调度

本文记录拟采用的执行与调度契约，不表示当前框架已经实现。Module 控制结构见 [module.md](module.md)，Rule 执行契约见 [rule.md](rule.md)，Queue 资源协议见 [queue.md](queue.md)。A/B 容量策略仍是备选，没有默认方案。候选缓存见 [gfsim-cache.md](gfsim-cache.md)，不属于基础流程。

## 阶段与粒度

| 阶段 | 粒度 | 作用 |
| --- | --- | --- |
| 激活 | ModuleId | 收集需要重新计算的 module |
| Work | Module | 执行控制逻辑，调用 rule Work |
| 仲裁收尾 | RuleId | 检查 complete，统一取消或预约并 accept |
| Xfer | QueueId | 更新持久状态 |

Module 对应 SimObject，rule 是其函数和独立事务，不要求独立 SimObject。事件队列按时间排序，同一时刻的 ModuleId 去重。同一个 module 可以组织多条流水线，而不是只能无条件执行所有 rule。

## 全局执行流程

```text
取出本批次激活的 modules
    ↓
并行 module.Work
    ├─ 按控制流调用 rule
    ├─ 在 ruleSlots[RuleId] 写本次动态信息
    └─ 只把实际尝试的 RuleId 加入本批列表
    ↓ 全局屏障
所有尝试进入 rule Arbitrate
    ├─ 不完整：清理 proposal
    ├─ 完整且有 proposal：预约实际资源，整体 accept 或 cancel
    └─ 完整但无 proposal：正常结束，不产生 firing
    ↓
方案 A：本批次结束
方案 B：accepted pop 引起下一 delta 的 module 激活
    ↓
没有同 tick 任务后统一 Queue.Xfer
    ↓
下一 tick 的状态与端口资格通知
```

不能只仲裁 complete=true 的候选，否则 Work 中途失败留下的 proposal 无人清理。未被 module 调用的 rule 不进入本批次尝试集合。完整但无 proposal 的尝试不算成功 firing。成功 rule 的 proposal 保留到 tick 结束，失败操作在仲裁阶段全部清理。

## 对象、身份与入口

Module 是激活与 Work 单位，rule 是仲裁与原子单位，Queue 是持久状态和 Xfer 对象。Module 对应 SimObject，持有控制逻辑并调用 rule，不是单纯的函数容器。

```cpp
struct ModuleEntry {
    std::function<void()> work;
};

struct RuleEntry {
    ModuleId owner;
    std::function<bool(RuleId, Tick)> arbitrate;
};

moduleRegistry[moduleId].work = [&module] { module.Work(); };
ruleRegistry[ruleId] = {
    moduleId,
    [&module](RuleId id, Tick tick) { return module.arbitrate_update(id, tick); },
};
```

代码只说明调度入口绑定，不规定容器或 ABI。初始化后注册表及对象地址稳定。RuleId 的实例语义见 [rule.md](rule.md)。

ModuleId 不替代 proposal 的 RuleId；来源槽位规则见 [queue.md](queue.md)。

## 时间与队列

使用 (tick, delta) 表示时间。Tick 是硬件周期，delta 是同 tick 的计算与仲裁批次，不是额外的硬件时钟。

```cpp
std::map<std::pair<Tick, Delta>, std::set<ModuleId>> scheduledModules;
```

事件队列按时间处理，同一时刻的 ModuleId 去重。初始化和常规 tick 唤醒进入 delta 0。Rule Work 请求的未来 tick 唤醒先记在 `ruleSlots[RuleId].wakeRequests`，整体仲裁获准后才进入事件队列；失败请求直接清除。

所有 delta 的 Work 都读取同一份 current。Delta 之间只有预约与 accepted 资格变化，只有全部 delta 结束后的统一 Xfer 更新数据。

## Module Work 与 Rule 仲裁收尾

调度器执行 module.Work，由控制流选择实际调用的 rule。Module 每 delta 至多 Work 一次；同一 RuleId 在该 delta 至多尝试一次，本 tick 至多获准一次。未被调用的 rule 不进入本批仲裁集合；再激活时跳过已获准实例，保留其 accepted proposal。

Work 全局屏障后，按本批 RuleId 列表仲裁，包括未 complete 与无 proposal 的尝试。失败只清理该 Rule 的 proposal 和唤醒请求。`RuleSlot` 按 ID 固定存储，下次尝试才清空动态字段；本批 RuleId 列表在仲裁后清空。无需每 delta 或每 tick 扫描重置全部槽位。具体调用与复用见 [rule.md](rule.md)。

不同 module 的 Work 可并行；初版不默认并行展开同一 module 的内部控制流。本批 RuleId 先按 Module 或工作线程局部登记，屏障后合并与排序，避免并行追加同一个列表。每个 RuleId 在同一 delta 只由其所属 Module Work 写入槽位。

## 方案 A：不复用同拍 pop 的空间

目标是保持容量判断和调度简单。

```text
一批 module Work
    → 全局屏障
    → 所有 rule 尝试收尾，完整候选按固定优先级仲裁
    → 统一 Xfer
```

Push 容量只看 current，不使用本拍其他 rule 的获准 pop。Queue 在 tick 开始时已满，消费者可 pop，但生产者 push 等待后续 tick。原本非空且有空位的 Queue 仍可同拍接受一次 pop 和一次 push。

优点是没有容量仲裁依赖、拓扑排序或同拍上游重试。代价是满 Queue 的空间到下一 tick 才可使用，容量为 1 的 FIFO 可能产生气泡。预约、单端口竞争和整条 rule accept/cancel 仍然存在。

## 方案 B：复用已经整体获准的 pop

目标是让上游 push 使用下游获准 pop 释放的同拍空间，减少满 Queue 的气泡。

临时预约不提供空间。只有整个消费 rule 的全部资源获准并 accept 后，Queue 才公开 accepted pop，且本 tick 不再撤回。

### 仲裁依赖与拓扑顺序

```text
数据流：A → Q1 → B → Q2 → C
仲裁顺序：C → B → A
```

消费者先获准，生产者后判断。固定优先级只处理不违反依赖关系的竞争候选。若消费 rule 失败，不得预支它可能释放的空间。

拓扑顺序可以跨 module。例如 `A(M) → B(N) → C(M)` 仍按 `C → B → A` 仲裁，不能一次仲裁完 M 再处理 N。调度器保留 rule 级入口，不要求 module 总的 Arbitrate。

相关容量依赖无环是初版支持前提。Module 之间允许跨 tick 反馈，不应将其与同 tick 容量依赖环混淆。具体依赖建图和分支筛选实现另行确定；不要求提前激活整条可能上游链。

### Accepted pop 与下一 delta

整条 rule 成功后，将可能利用空间的直接生产者所属 ModuleId 加入下一 delta。重新执行其 module Work，尝试尚未获准的 rule，不保留失败 proposal 或详细阻塞原因表。

```text
取出本 delta 的 modules 并行 Work
    → 屏障
    → 所有尝试进入仲裁收尾
    → 完整候选按消费先行拓扑序预约并确认
    → accepted pop 通知下一 delta 的上游 modules
    → 同 tick 任务耗尽后统一 Xfer
```

不在仲裁中途递归执行新 module，不在 delta 边界 Xfer。再次执行读取的仍是本拍旧数据，不会形成零周期数据传递。

例如 Q1/Q2 开始时已满，C 所属 module 首先被唤醒：

```text
(t,0)：C 获准 pop Q2，通知 B 的 module。
(t,1)：B 的 module Work；B 利用 Q2 资格获准，pop Q1。
(t,2)：A 的 module Work；A 利用 Q1 资格获准。
tick 结束：统一提交。
```

如果 A/B/C 在同一个 module，通知仍可再次激活该 module；已获准者跳过，其他 rule 可以重新尝试。如果它们同一 delta 已有完整候选，则直接按 `C → B → A` 仲裁。

### 容量提示与终止

```cpp
canPush = !pushOccupied && (!currentFull || hasAcceptedPop);
```

这是资格提示，不替代整体预约。CurrentFull 本 tick 内不变，临时 pop 不属于 hasAcceptedPop。一个 pop 不提供第二个 push 或第二个消费机会。本拍新 push 的数据始终不可读。

为支持 B，不能仅因 current.full 就在 module Work 排除全部生产候选。允许按最终仲裁失败，不要求 Work 提前算出完整 fire。

只有整体 accepted pop 引起本 tick 后续通知，失败本身不重排。每条 rule 本 tick 至多获准一次；同一 delta 去重。有限规则和连接下，该事件推进不会因失败自行无限循环，但不求解无起点的循环空间依赖，也不保证最大获准集合或公平性。

## 两类 Queue 通知与依赖

| 触发事件 | 时间 | 对象 |
| --- | --- | --- |
| Xfer 状态提交及 tick 边界端口重置 | (tick+1,0) | 控制逻辑或 rule 依赖它的 modules |
| 整条 rule 成功后的 accepted pop，仅 B | (tick,delta+1) | 可能向它 push 的生产 rule 所属 modules |

初始化和定时/延迟事件也是任务来源。Rule 请求的延迟事件仅在整条 Rule 成功后发布；Queue 的 accepted pop 下一 delta 通知由仲裁器生成，不通过 `wakeRequests`。通知按 ModuleId 去重，不仅按 payload 位是否变化判断。

初始化登记：Queue 到相关 module、Queue 到可能生产者所属 module、RuleId 到所属 module 和仲裁入口。必须包含 module 控制条件的读取、内部流水线 Queue 和只读 Queue，不能只登记上次选中分支的实际依赖。来源注册不替代读取依赖登记。

时间相关控制需要明确的事件，不能等待无关 Queue 恰好变化。通知仅安排重新计算，不保证 rule 会 fire。

## 调度记录与生命周期

Rule 注册表绑定 RuleId、所属 ModuleId 与仲裁入口。`ruleSlots[RuleId]` 保留时间戳并复用动态容器，本批 RuleId 列表只负责找出需要收尾的槽位。整体获准后先记录成功，再发布延迟事件与 pop 通知；失败候选和未发布的延迟事件当批清理。Queue 的 accepted 内容跨 delta 保留到 Xfer。

## Xfer 与收尾

每个 Queue 每 tick 只 Xfer 一次，按 `revise → pop → push` 应用全部获准操作，再清理本 tick 触及的来源状态、预约、accepted 列表和摘要。可遍历全部 Queue，或维护覆盖本 tick 所有 delta 的去重 touchedQueues。即使某 Queue 的候选全都失败而没有 accepted 操作，若其槽位曾被触及，也必须完成清理。

逐个 Queue Xfer 期间不执行 Work 或仲裁。所有资源提交结束后再发布下一 tick 通知。没有任务时应区分真正完成、外部等待与未完成请求停滞。时间相关控制需要定时或时钟事件，不能依赖无关 Queue 偶然唤醒。

## A/B 比较与未限定范围

| 项目 | A | B |
| --- | --- | --- |
| 使用其他 rule 本拍获准 pop 的空间 | 不使用 | 使用 |
| Work 批次 | 一批 module Work | 多个 delta 的 module Work |
| 完整候选仲裁 | 固定竞争顺序 | 消费先行拓扑序，无依赖者按固定优先级 |
| 上游 module 再激活 | 下一 tick | accepted pop 后下一 delta |
| 实现代价 | 调度和容量判断简单 | 依赖排序、额外 Work、delta 与屏障 |
| 原子性 | 每条 rule，统一 Xfer | 相同 |

两者不是逐 tick 等价的性能替换。Module Work 不承担整体原子性；调用顺序和返回不表示提交成功。自身 pop/push 的局部空间政策、来源 0 外部驱动的竞争优先级、字段冲突、公平性和具体建图保持原支持边界，不在本轮扩展。

## 验收标准

- [ ] 多个 Queue 更新只触发同 delta 的一次 module Work。
- [ ] 内部流水线和 module 控制读取变化不漏唤醒。
- [ ] 同一 module 再激活不重复提交已获准 rule，剩余 rule 可重试。
- [ ] A 的满 Queue push 等待下一 tick；B 利用整条 rule 获准 pop 的空间。
- [ ] B 跨 module 消费先行仲裁，delta 无数据穿透，失败不无限自行重排。
- [ ] 相同 payload 交接与端口重置通知不遗漏。
- [ ] 延迟唤醒只在所属 Rule 整体获准后入调度队列；失败或未 complete 不留下事件。
- [ ] RuleSlot 按 RuleId 复用，本批列表只保存实际尝试的 RuleId；并行 Work 不争用全局追加点。
