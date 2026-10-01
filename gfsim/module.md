# GFSim Module 设计

本文记录拟采用的 Module 控制契约，不表示当前框架已实现。Rule 的执行与原子提交见 [rule.md](rule.md)，时间调度见 [schedule.md](schedule.md)，Queue 资源见 [queue.md](queue.md)。

## 职责与边界

一个 module 实例对应一个 SimObject，持有连接、内部 Queue 与控制逻辑，是激活和 Work 调度单位。Module 有自己的公共 `Work()`，用于组织流水线、执行组合控制逻辑，并选择本轮尝试哪些 rule。每条 rule 是 Module 的一对公共成员函数：`work_<rule>()` 提出本次状态转移，`arbitrate_<rule>()` 决定整条 rule 接受还是取消。

`Work()` 直接调用 `work_<rule>(...)`，不调用 `arbitrate_<rule>()`。调用只表示发起一次尝试，不表示提交成功。本批 Module Work 结束后，调度器仅对实际尝试过的 rule 调用相应的 `arbitrate_<rule>()`；Module 不提供总的 `Arbitrate()`。Module 本身没有整体原子成功或失败的含义，已调用的不同 rule 分别仲裁。

Module 的状态修改必须归属于明确的 rule。Module Work 不直接改 Queue 的 current，也不提出没有原子归属的 proposal。内部 Queue 与连线 Queue 同样遵守该约束；连接双方引用同一个 Queue。

## Module Work 的控制语义

Module 可使用 if/elif/else 决定调用哪些 `work_<rule>()`。Rule 函数自身也可包含控制流；其分支语义见 [rule.md](rule.md)。

Module 的控制流只读取本 tick 的 current 状态和由它计算出的组合值，包括寄存器值、Queue 当前空满与当前时间；不读取会在 delta 内变化的预约、accepted pop 或 `canPush` 等仲裁资格。Module Work 不消费元素，不代 Rule 预读消息输入 Queue 的 payload。Rule 自己 `peek` 消息输入，编译器另行插入该输入的 pop proposal；底层 `peek` 始终是纯读。Module 可把已读寄存器值作为普通 `var` 传给 Rule。

同一 tick 内 current 不变，Module Work 没有其他可变输入或外部副作用，因此其控制流和选中的 rule 调用保持不变。每个 Module 在本 tick 首次被激活时只执行一次 `Work()`，记录实际选中的 rule 实例、调用顺序及参数；后续 delta 的容量通知只重试记录中尚未获准且可能受影响的 `work_<rule>()`，不重新执行 Module Work。调用参数若来自 Work 局部计算，须按值保存；Queue 引用可保存稳定句柄，不能保留悬空的局部引用。

```cpp
// Module 运行时记录：std::optional<Tick> lastWorkTick;
//                 std::vector<RuleCall> selectedCalls;
void activateModule(ModuleId id, Tick tick) {
    auto& module = modules[id];
    if (module.lastWorkTick != tick) {
        module.selectedCalls.clear();
        module.Work(); // 调用 work_<rule>()，同时记录本 tick 的调用描述。
        module.lastWorkTick = tick;
    } else {
        for (const auto& call : module.selectedCalls)
            if (call.mayUseNewCapacity() && !call.ruleAlreadyAccepted())
                call.retryRuleWork(); // 使用保存的参数，不重算 Module 控制流。
    }
}
```

代码只示意生命周期，不规定容器或 ABI。同一 tick 首次激活可能发生在任意 delta。初次 Work 即使尚未获得输出空间，也要选择相应 rule；不能用 `current.full` 排除等待同拍 pop 腾出空间的生产 rule。必要输入缺失而未形成完整候选的 rule 无须仅因输出空间变化重试。

```cpp
void MyModule::Work() {
    // 只观察寄存器，不消费它。
    if (recovering.peek())
        work_recover(recoverInput, recoverOutput);
    else
        work_completion();

    work_other(); // 独立 rule；前一条 rule 是否获准，此时仍未知。
}

void MyModule::work_completion() {
    const auto* event = completionInput.tryPeek();
    if (!event)
        return; // 必要读取不足：本次尝试不完整。

    completionInput.proposePop(completionRuleId);
    resultOutput.proposePush(completionRuleId, makeResult(*event));
    markRuleComplete(completionRuleId); // 正常结束；仍需后续仲裁。
}

bool MyModule::arbitrate_completion() {
    return arbitrateRule(completionRuleId); // 整体 accept 或 cancel。
}
```

代码只示意生成后的成员函数关系，不规定 `markRuleComplete`、`arbitrateRule` 的实际 ABI。编译器将每对 `work_<rule>` / `arbitrate_<rule>` 绑定到同一个 RuleId；`work_<rule>` 的生成入口负责去重、初始化并登记本次尝试，正常出口记录 complete。必要读取失败时，已提出的 proposal 留待该 rule 的 `arbitrate_<rule>` 取消。Python 作者不操作 RuleId 或槽位。若同一函数代码对应多个实例，绑定还需包含实例身份；见 [rule.md](rule.md)。

Module 的提前 return 只停止后续调用，已经调用的 rule 仍进入仲裁收尾。若某个条件必须阻止全部相关 rule，应先判断再调用，或者将相关修改放在一个原子 rule 中。

调用顺序不表示提交顺序或成功。多个独立 rule 可以分别成功或失败；Work 返回不能作为本拍已提交的依据。若分支写成“flush 有数据就只调用 flush”，flush 后来提交失败也不会自动回退调用普通处理 rule。

所有 Work 只读本 tick 的 current。先调用 receive 再调用 execute，不允许 execute 读取本拍 receive 的 proposal。新数据在 Xfer 后可见；纯组合值可以通过参数传递。

## 资源与激活依赖

Rule 可以通过 module 成员访问其连接和内部状态。编译器需登记 module 控制条件所读的寄存器与 Queue 状态、内部流水线资源，以及 rule 的消息输入和只读状态资源，以便状态变化激活所属 module。资源可达不等于实际参与某次 firing。

不同 module 的首次 Work 可以并行，同一 module 每 tick 至多执行一次。Module 可在后续 delta 被再次通知；已保存的调用由受影响 rule 重试，不重新运行 Module 控制逻辑。规则尝试去重与失败重试由 [rule.md](rule.md) 定义。内部流水线 Queue 更新按 [schedule.md](schedule.md) 的通知语义安排激活。

延迟响应通过未来 tick 激活 module；待处理请求和结果存在内部 Queue。Rule Work 只向本次 RuleSlot 记录唤醒请求，不直接修改调度队列；整条 Rule 获准后才发布事件。

## 验收

- [ ] `Work()` 直接调用选中的 `work_<rule>()`；调度器只对实际尝试的 rule 调用相应 `arbitrate_<rule>()`，没有 Module 总仲裁入口。
- [ ] Module 的寄存器读取及 Queue 状态查询不消费 Queue；消息输入 payload 由 Rule 自己读取，内部状态变化可激活 module。
- [ ] Module 提前返回只停止后续 rule 调用，不取消此前独立尝试。
- [ ] 多条流水线可由同一 module 分支调用，module 无整体成功状态。
- [ ] Module 不直接写 current，也不提出匿名持久状态修改。
- [ ] Module 同一 tick 只运行一次 Work；后续 delta 使用记录的 rule 调用与稳定参数重试，已获准 rule 不重复提交。
- [ ] Module 控制流只观察 current；仲裁资格变化不改变本 tick 已记录的选择。
