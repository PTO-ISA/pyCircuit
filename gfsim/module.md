# GFSim Module 设计

本文记录拟采用的 Module 控制契约，不表示当前框架已实现。Rule 的执行与原子提交见 [rule.md](rule.md)，时间调度见 [schedule.md](schedule.md)，Queue 资源见 [queue.md](queue.md)。

## 职责与边界

一个 module 实例对应一个 SimObject，持有连接、内部 Queue 与控制逻辑，是激活和 Work 调度单位。Module 可组织多条流水线、共享组合计算，并根据控制流调用 rule 的 Work 函数。Module 本身没有整体原子成功或失败的含义；已调用的独立 rule 分别仲裁。

Module 的状态修改必须归属于明确的 rule。Module Work 不直接改 Queue 的 current，也不提出没有原子归属的 proposal。内部 Queue 与连线 Queue 同样遵守该约束；连接双方引用同一个 Queue。

## Module Work 的控制语义

Module 可使用 if/elif/else 决定调用哪些 rule。Rule 自身的分支语义见 [rule.md](rule.md)。

Module 可读取自己持有的寄存器 Queue，或查询 Queue 的空、满等资源状态，用于选择调用哪些 rule；这些访问不消费元素。Module 不代 Rule 预读消息输入 Queue 的 payload。Rule 自己 `peek` 消息输入，编译器另行插入该输入的 pop proposal；底层 `peek` 始终是纯读。Module 可把已读寄存器值作为普通 `var` 传给 Rule。

```cpp
void MyModule::Work() {
    // 只观察寄存器，不消费它。
    if (recovering.peek())
        tryWorkRule<&MyModule::work_recover>(recoverInput, recoverOutput);
    else
        tryWorkRule<&MyModule::work_completion>();

    tryWorkRule<&MyModule::work_other>();
}
```

`tryWorkRule` 是生成调用保护的示意名称，不规定实际 ABI。编译器把函数绑定到 RuleId；调用保护复用 `ruleSlots[RuleId]`，将 RuleId 记入本批仲裁列表，再执行 Rule Work。Python 作者不操作槽位。若同一函数代码对应多个实例，绑定还需包含实例身份；见 [rule.md](rule.md)。

Module 的提前 return 只停止后续调用，已经调用的 rule 仍进入仲裁收尾。若某个条件必须阻止全部相关 rule，应先判断再调用，或者将相关修改放在一个原子 rule 中。

调用顺序不表示提交顺序或成功。多个独立 rule 可以分别成功或失败；Work 返回不能作为本拍已提交的依据。若分支写成“flush 有数据就只调用 flush”，flush 后来提交失败也不会自动回退调用普通处理 rule。

所有 Work 只读本 tick 的 current。先调用 receive 再调用 execute，不允许 execute 读取本拍 receive 的 proposal。新数据在 Xfer 后可见；纯组合值可以通过参数传递。

## 资源与激活依赖

Rule 可以通过 module 成员访问其连接和内部状态。编译器需登记 module 控制条件所读的寄存器与 Queue 状态、内部流水线资源，以及 rule 的消息输入和只读状态资源，以便状态变化激活所属 module。资源可达不等于实际参与某次 firing。

不同 module 的 Work 可以并行，同一 module 每 delta 至多执行一次。Module 可在后续 delta 被再次激活，重新执行控制逻辑；规则尝试去重与失败重试由 [rule.md](rule.md) 定义。内部流水线 Queue 更新按 [schedule.md](schedule.md) 的通知语义安排激活。

延迟响应通过未来 tick 激活 module；待处理请求和结果存在内部 Queue。Rule Work 只向本次 RuleSlot 记录唤醒请求，不直接修改调度队列；整条 Rule 获准后才发布事件。

## 验收

- [ ] Module 的寄存器读取及 Queue 状态查询不消费 Queue；消息输入 payload 由 Rule 自己读取，内部状态变化可激活 module。
- [ ] Module 提前返回只停止后续 rule 调用，不取消此前独立尝试。
- [ ] 多条流水线可由同一 module 分支调用，module 无整体成功状态。
- [ ] Module 不直接写 current，也不提出匿名持久状态修改。
- [ ] 重复激活会重新运行控制逻辑，但不重置已获准 rule 的内容。
