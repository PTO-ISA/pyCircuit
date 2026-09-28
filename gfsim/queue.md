# GFSim Queue 状态、proposal 与提交设计

本文记录拟采用的 Queue 资源契约，不表示当前框架已实现。Rule 如何产生和整体仲裁 proposal 见 [rule.md](rule.md)，调度与 A/B 容量策略见 [schedule.md](schedule.md)，字段路径见 [struct.md](struct.md)。

Queue 是电路持久状态的基础对象。一个 Queue 对应一个 SimQueue，连线两端引用同一对象；current 用 `std::vector<Element>` 保存。元素可以是单个 AC 类型，也可以是按值嵌套的 struct。FIFO 保存多个元素；寄存器使用容量为 1、初始化并始终保持一个元素的受限 Queue，只允许 revise，不允许 pop/push。寄存器阵列由多个这样的 Queue 组成。容量为 1 但允许空满切换的流水级仍属于 FIFO。

Current 是本 tick 的状态快照，Work 只读，统一 Xfer 才修改它。Proposal、临时预约和 accepted 记录是 Queue 的运行时管理信息。

## 操作定义

| 操作 | 读取或修改目标 | 发生阶段 |
| --- | --- | --- |
| `peek()` / `tryPeek()` | 读取 current 队首，空时不得读取数据 | Work 等只读阶段 |
| `empty()` / `full()` / `size()` | 查询 current | 只读阶段 |
| `proposePop(source)` | 请求删除旧队首 | Work 提出，Xfer 执行 |
| `proposePush(source, value)` | 请求将新元素追加到队尾 | Work 提出，Xfer 执行 |
| `proposeRevise<Path...>(source, value)` | 请求修改旧队尾的字段或整值 | Work 提出，Xfer 执行 |

Peek 始终是纯读，不消费、不推进位置。反复 peek 得到相同的旧队首；pop proposal 之后的 peek 也不跳到下一元素。Rule 读取消息输入 payload 时，由前端另行生成 pop proposal，不能把消费行为归给 `peek`。寄存器读取与 Queue 空满查询不生成 pop。Revise 的目标是旧队尾，不是本拍 push 的新元素。空 Queue 不允许 revise。

每个 Queue 每 tick 最多批准一个 pop 和一个 push。不同来源的 pop 不合并；重复调用 push/pop 不能通过覆盖候选自动成为合法操作。可以有多个不重叠字段的 revise。

## 来源注册

```cpp
using SourceId = uint32_t;
constexpr SourceId DefaultSourceId = 0;

input.registerSource(ruleId);
internal.registerSource(ruleId);
output.registerSource(ruleId);
```

Queue 只注册可能操作自己的来源，同一来源重复注册不增加槽位。初始化结束后，来源索引和槽位地址在执行期间保持稳定，不按全局 rule 总数分配。

每个 Queue 保留来源 0，供外部驱动或隔离测试使用；无 ID 的便利 propose 接口若保留，也只能用于该边界。生成的 Module/Rule Work 的所有状态修改都必须使用 RuleId（从 1 开始），不能借来源 0 绕过完整尝试和原子仲裁。不同 Queue 的来源 0 是独立来源，不自动成为跨 Queue 原子事务。来源 0 仍需仲裁和确认，不能隐式参加 Xfer；独立驱动入口及与 Rule 来源竞争时的优先级另行确定。并行来源不能无保护共用 0。

## Proposal 与槽位

```cpp
enum class ProposalStatus { Empty, Pending, Reserved, Accepted };

template<class Element>
struct QueueProposal {
    bool pop = false;
    std::optional<Element> push;
    std::vector<std::function<void(Element&)>> revises;
};

template<class Element>
struct ProposalSlot {
    ProposalStatus status = ProposalStatus::Empty;
    QueueProposal<Element> proposal;
};

template<class Element>
class SimQueue {
    std::vector<Element> current_;
    size_t capacity_;
    SourceIndex sourceIndex_; // SourceId → SlotIndex，示意类型。
    StableSlots<ProposalSlot<Element>> slots_;
    std::vector<SlotIndex> acceptedSlots_;
    std::optional<SourceId> acceptedPopOwner_;
    std::optional<SourceId> acceptedPushOwner_;
    // 临时预约归属另外保存，不等于 accepted 摘要。
};
```

槽位保存某来源对当前 Queue 的整组 pop/push/revise，不保存该来源对其他 Queue 的操作。**Proposal 是增量**，不复制整个 current。

Accepted 留在原槽位，不额外复制。AcceptedSlots 保存获准槽位索引，acceptedPopOwner/acceptedPushOwner 提供常数时间查询。使用 optional，不能把合法来源 0 当作“没有来源”。

Queue 每 tick 最多批准一个 pop 和一个 push，不合并多个 pop。重复 proposePush/proposePop 不得靠覆盖或反复设置 bool 静默接受；前端消费归一化与非法重复调用需区分，见 [rule.md](rule.md)。

## SimQueue 接口

| 方法 | 行为 |
| --- | --- |
| `registerSource(id)` | 登记来源与固定槽位 |
| `peek()` / `tryPeek()` | 只读 current 队首 |
| `empty()` / `full()` / `size()` / `capacity()` | 只读 current 信息 |
| `proposePop(id)` | 提出删除旧队首 |
| `proposePush(id, value)` | 按值保存新元素 |
| `proposeRevise<Path...>(id, value)` | 保存旧队尾延迟修改动作 |
| `hasProposal(id)` | 查询候选操作是否非空 |
| `arbitrate(id)` | 检查该来源整组操作；成功时取得临时预约，返回 bool |
| `accept(id)` | 确认预约，记录 accepted 槽位和摘要 |
| `cancel(id)` | 清理本来源本次候选并释放临时预约 |
| `Xfer()` | 执行 accepted 操作，清理本轮记录 |

Propose 不检查整条 firing 是否能成功，不预约、不更新 current。生成的 Rule Work 必须用自身 RuleId 调用 propose；Module Work 不直接提出状态修改。寄存器模式拒绝 pop/push，只接受合法 revise。基础方案 cancel 不保留失败候选；槽位注册信息与可复用缓冲容量保留。

空槽位的 arbitrate 可以作为无操作返回 true，accept/cancel 可以无操作；这些不替代 [rule 的 complete 判定](rule.md)。非法状态转移必须与空操作区分。Accepted 不属于可取消候选，本 tick 不撤回。

## 通用 revise 动作

字段路径使用编译期 C++ 数据成员指针，新值在 Work 计算并按值保存。Struct 保持普通 public 字段，不需要逐字段 setter 或运行时字段名字典。

```cpp
template<auto... Path, class Value>
void proposeRevise(SourceId source, Value value) {
    auto& proposal = pendingProposal(source);
    proposal.revises.emplace_back(
        [saved = std::move(value)](Element& target) {
            if constexpr (sizeof...(Path) == 0)
                target = saved;
            else
                fieldAt<Path...>(target) = saved;
        });
}
```

`pendingProposal` 示意定位已注册来源、将槽位标为 Pending，并通知调用方登记本次参与的 Queue；实际 API 不在此规定，见 [rule.md](rule.md)。模板代码位于 SimQueue 中，Element 为其元素类型。

```cpp
queue.proposeRevise<&Entry::meta, &Meta::epoch>(ruleId, nextEpoch);
queue.proposeRevise<&Entry::value>(ruleId, nextValue);
registerQueue.proposeRevise<>(ruleId, nextRegisterValue);
```

队列保存的是延迟赋值函数：Work 捕获结果值，Xfer 才传入旧队尾，函数通过成员路径直接赋值。嵌套路径递归访问的实现及可复制、可赋值等类型限制见 struct.md。

该函数不可在提交时再次读取其他 Queue、重新计算业务条件或使用失效引用。新值必须已经确定。Opaque lambda 不自动携带可检查的字段重叠信息；初版不依靠它解决写冲突。

## 资源仲裁与容量摘要

`Queue::arbitrate(source)` 只检查指定来源在这个 Queue 的整组操作、旧状态、已获准操作与临时预约；成功仅表示取得该 Queue 的临时预约，不表示整条 rule 已成功。调用方负责跨 Queue 的整体 accept/cancel，见 [rule.md](rule.md)。

`Queue::accept(source)` 确认预约，在原槽位标记 Accepted、将槽位索引加入 acceptedSlots，并更新 acceptedPopOwner/acceptedPushOwner。`Queue::cancel(source)` 释放该来源的临时预约，清空本次未获准操作；不能撤销其他来源或已获准操作。确认阶段必须保证不再失败。

```cpp
bool hasSpaceA = !full();
bool hasSpaceB = !full() || acceptedPopOwner_.has_value();
```

这只是容量提示，还须检查端口占用和同一来源的整组操作。方案 A 只使用 current 的原有空位；方案 B 还可使用其他整条 firing 已获准 pop 的空间。临时 pop 预约不是获准 pop。自身 pop/push 是否复用空间仍待定。A/B 的仲裁顺序与下一 delta 唤醒见 [schedule.md](schedule.md)。

## Xfer 顺序

```text
全部获准 revise：修改旧队尾
    ↓
获准 pop：删除旧队首
    ↓
获准 push：追加新元素
```

顺序固定为 **revise → pop → push**。必须按操作类别处理全部获准槽位，不能逐来源执行完整三步。

```cpp
void Xfer() {
    for (auto index : acceptedSlots_)
        for (auto& revise : slots_[index].proposal.revises)
            revise(current_.back());

    if (acceptedPopOwner_)
        current_.erase(current_.begin());

    if (acceptedPushOwner_) {
        auto& p = slotFor(*acceptedPushOwner_).proposal;
        current_.push_back(std::move(*p.push));
    }

    clearCycleRecords();
}
```

代码为结构示意。仲裁事先保证所有获准操作合法，Xfer 不重新选择赢家。确认与提交阶段必须保证不再失败，必要容量和构造准备在不可撤销阶段之前完成。

Current 为空时不能消费本拍新 push 的数据。所有 Work 都已经读取旧快照；Xfer 的顺序不产生同拍数据转发。Revise/pop 指向同一个旧元素的情况初版不保证结果。

## 生命周期与支持边界

失败 proposal 在其来源的仲裁中清理；accepted 内容跨多个 delta 保留到统一 Xfer。Queue 可在来源槽位首次被本 tick 触及时，把槽位索引加入 `touchedSlots_`；同一 tick 后续 delta 重试复用槽位，不重复加入。Xfer 处理 accepted 操作后，仅遍历本 tick 触及的槽位，释放捕获值并清空状态与临时预约，再清空 acceptedSlots、获准端口摘要和 touchedSlots；来源注册与可复用缓冲保留。即使所有尝试都失败、没有 accepted 操作，触及的 Queue 仍需执行收尾。

`touchedSlots_` 可用槽位的 `lastTouchedTick` 去重，比较当前 tick 而不用每 tick 扫描全部注册来源重置标志。若采用惰性失效标签，也必须确保过期 proposal 不被仲裁或 Xfer 读取，并处理捕获值长期占用内存；仅比较时间戳不能代替这些清理。Xfer 期间不执行 Work；调度器可遍历全部 Queue，或按本 tick 的 touchedQueues 调用收尾。

初版支持单 pop、单 push、嵌套 struct 与不重叠字段 revise。同字段多写、父子字段重叠、整值与字段修改重叠、revise/pop 指向同一旧元素，初版不保证结果。Opaque lambda 不自动提供字段冲突信息。

来源 0 的外部驱动入口及与 Rule 来源竞争时的优先级仍待定；生成的电路 Work 不使用它。后续测量槽位占用、元素复制、lambda 分配、队首删除和 Xfer 成本；容器优化不得改变快照与提交语义。

## 验收

- [ ] Peek 始终纯读；编译器为 Rule 消息输入 payload 读取另行生成 pop proposal，proposal 和 accepted 操作不改变 Work 可见数据。
- [ ] 每 tick 最多一个 pop、一个 push；重复请求不合并、不静默覆盖。
- [ ] 来源 0 是合法来源，不充当 accepted 摘要的空标记。
- [ ] 生成的 Module/Rule Work 不使用来源 0；寄存器始终占用，只能 revise，容量 1 FIFO 仍可 pop/push。
- [ ] 当前 Queue 的预约失败或取消不影响其他来源及 Accepted 内容。
- [ ] 标量、普通 struct、嵌套字段和不重叠 revise 保持 AC 值语义。
- [ ] Xfer 仅执行获准操作，依次 revise → pop → push，再清理本轮记录。
- [ ] 空、满、容量 1 和同拍 pop/push 在 A/B 策略下分别验证。
