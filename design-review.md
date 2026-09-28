# ACIR / GFSim 方案审查记录（2026-09-28）

本文审查 [本目录](.) 下拟定的编译与执行设计，不代表当前 pyCircuit 的实现状态。标为“已解决”的条目已同步到对应设计文档，其余仍是待决问题。对象职责仍按 [Module](gfsim/module.md)、[Rule](gfsim/rule.md)、[Queue](gfsim/queue.md)、[Schedule](gfsim/schedule.md)、[ACIR](acir/rule.md) 划分；候选缓存是 [独立优化](gfsim/gfsim-cache.md)。本轮不讨论静态展开。

## 已经较清楚的主链路

Module 决定调用哪些 rule；Rule Work 从本 tick 的 current 安全读取、计算并向 Queue 提 proposal；所有实际调用都在 Work 屏障后仲裁收尾；失败清理本次 proposal，整体成功的操作由 Queue 在 tick 末 Xfer。每个 RuleId 的固定 RuleSlot 保存时间戳和本次动态信息，本批只登记实际尝试的 RuleId。这条主链路能表达分支下的部分输入消费、可选输出和“只丢弃过期消息”。

下列问题集中在**边界语义**：哪些动作属于一个原子 firing、谁有资格操作 Queue，以及 GFSim 与 RTL 能否执行同一种路径和资源判断。

## 边界语义审查

### 1. 来源 0 与“所有状态修改归属 rule”冲突（已解决）

[Module](gfsim/module.md) 禁止没有 rule 归属的状态修改，而 Queue 曾提供无 ID 的来源 0 propose 接口，[Struct](gfsim/struct.md) 也曾展示无 ID 调用。这样会允许 Module Work 绕过 RuleId、complete 和原子边界。

**已定契约**：生成的电路 Work 的所有状态修改必须归属于 RuleId。来源 0 仅留给外部驱动或隔离测试，并需独立仲裁和确认；Module Work 不能用它提出状态修改。外部来源与 Rule 来源的竞争优先级仍待定。

### 2. 延迟唤醒的原子归属（GFSim 方案已明确，RTL 待定）

直接在 Rule Work 中向调度队列插入 `tick+3` 事件，会在 Queue 仲裁失败时留下错误唤醒。现改为：

```cpp
slot.wakeRequests.push_back({tick + 3, moduleId}); // Rule Work，仅记录
// Rule Arbitrate：失败则清空；全部 Queue accept 后才 scheduler.enqueue(...)
```

事件仅保存调度元数据，payload/计时状态仍由内部 Queue 表达。只请求唤醒、没有 Queue proposal 的尝试不 firing。RTL 中的延迟需要什么显式状态表达仍待确定。

### 3. 有返回但无操作的 rule：文字与示例相矛盾（已解决）

[Rule](gfsim/rule.md) 曾将零操作路径是否 firing 留空，但示例会在 `complete=true`、participants 为空时返回成功并写入 `lastAcceptedTick`，两者矛盾。

**已定契约**：完整但没有 Queue proposal 的尝试正常结束，不构成 firing，不写 `lastAcceptedTick`，不发获准通知；单独的唤醒请求也不能使它 firing。只消费输入已有 pop proposal，仍可 firing。未来若允许独立非 Queue 原子效果，需另定其参与和接受契约。

### 4. 函数到 RuleId 绑定与重复调用

[Module](gfsim/module.md) 的函数式调用让接口变简单，但 [Rule](gfsim/rule.md) 仍规定同一 RuleId 每 delta 只尝试一次。若同一 Work 中写 `tryWorkRule<&work_add>(q0)`、再写 `tryWorkRule<&work_add>(q1)`，函数到 ID 的单一绑定会令第二次调用静默跳过，`q1` 完全未处理。

**建议**：初版规定一个 rule 实例在 module 控制图中只拥有一个调用身份。编译器对同一 delta 可能发生的重复调用给出明确诊断，不能静默忽略不同实参。复用代码时由生成包装/实例句柄提供独立 RuleId；不必现在展开前端静态构造语法。

### 5. Queue 的角色没有足够明确的操作权限（已解决）

[ACIR](acir/rule.md) 曾说 Rule 使用输入 payload 隐含消费，[Rule](gfsim/rule.md) 则说 `peek` 纯读。这里需要区分底层操作与编译器生成的资源效果：底层 `peek` 始终纯读；Rule 读取绑定为消息输入的 Queue payload 时，前端另行插入 pop proposal，成功 firing 才提交。重复读取同一实际输入只提出一次 pop。Rule 直接读 Module 持有的寄存器，或使用 Module 传来的普通 `var`，均不产生 pop；Module 不替 Rule 预读消息输入。

寄存器是容量 1 且始终占用的受限 Queue，只允许 revise；容量 1 但可空的流水级仍是 FIFO。资源角色由绑定确定，内部 FIFO 作为 Rule 消息输入时也采用上述消费语义。

**已定契约**：HIR/Frozen 保留资源访问角色和实际操作；消息输入 payload 读取由前端生成 pop，寄存器读取、Queue 状态查询和普通 `var` 均不生成 pop。寄存器只可 revise。

### 6. Guarded SSA 值的有效域没有正式定义

[ACIR](acir/rule.md) 的 Frozen 示例在 `%have_control=false` 时不应读取 `%control`，但仍会形成 `%choose_left`、`%right_active = %have_control && !%choose_left` 等 SSA 表达。若 `and` 或 `not` 的操作数被严格求值，GFSim 可能读取无效值；若后端各自假定短路，GFSim 与 RTL 的行为可能分歧。文档声明“仅在 guard 有效域内使用”，但缺少 IR 上可检查的判定规则。

**建议**：给每个受条件保护的值定义有效域，验证使用点的 guard 蕴含其有效域；无效值不得被普通 eager 运算读取。Frozen 应明确 `and`/select/资源索引如何保留短路或受保护求值，并用空 Queue、未选分支及嵌套条件做 GFSim/RTL 同语义测试。

## A/B 选择前需界定的资源与调度问题

### 7. 同一 rule 对同一满 Queue 的 pop+push 尚无容量规则

[Queue](gfsim/queue.md) 与 [Schedule](gfsim/schedule.md) 只定义方案 A 是否使用**其他** rule 的获准 pop，及 B 如何先处理消费者；同一 firing 同时 pop 和 push 同一满 Queue 的行为仍待定。它不需要跨 rule 拓扑排序，但会直接影响常见的容量 1 Queue 替换旧元素。

**建议**：先独立决定同来源的整组操作是否合法、是否允许自己的 pop 给自己的 push 腾位置，再检查 A/B。若允许，必须基于旧队首完成读取，Xfer 仍是 pop 旧值、push 新值，不产生本拍新数据旁路。

### 8. B 的空间复用可能被 Module 的 `full()` 分支屏蔽

B 依赖消费者获准 pop 后，下一 delta 唤醒生产者；但 Work 在各 delta 看到的 `current.full()` 始终相同。若 Module 写 `if not output.full(): call producer`，即使下游已获准 pop，上游也不会形成候选。文档提醒“不能仅因 full 排除生产候选”，但尚未说明作者该用什么信号表达**资源资格**，也未界定这是故意的业务限制还是设计误用。

**建议**：明确 `full()` 只表示 tick 快照，不等于本轮可 push。推荐普通生产 rule 无条件形成候选，由 Arbitrate 判断背压；若 Module 确需基于同拍资格分支，另定义 `canPush` 一类仲裁态查询，并在 ACIR/RTL 中处理它的组合依赖与无环约束。

### 9. B 的拓扑图在动态目标和条件分支下尚无构造规则

B 要求“消费者先、生产者后”，但目标 Queue 可能由本次 Work 的动态值确定，规则也可能只在某分支操作它。仅按“可能连接”静态建边，容易产生实际本拍不存在的边或假环；只看本次已激活的 rule，又必须保证获准 pop 能找到尚未激活的潜在生产者。当前文档两者都留作后续，因而还不能证明 B 的完整性。

**建议**：区分两类关系：保守的 Queue → 可能生产者 Module 唤醒索引，以及本批实际 proposal → 参与 Queue 的仲裁依赖。先以动态目标、条件选择、跨 module 的最小样例验证不漏唤醒、不引入错误环。B 的“无环”应针对会影响当拍容量许可的实际依赖，而不是所有跨 tick 数据连接。

## 可明确列为首版限制，但不能默默产生结果

### 10. 重叠 revise 与确认阶段失败

[Queue](gfsim/queue.md) 和 [Struct](gfsim/struct.md) 都把同字段、父子字段及整值/局部重叠写入列为“不保证结果”。由于 revise 保存的是 opaque lambda，Queue 自身看不到字段路径；如果两个来源都获准，Xfer 将按遍历顺序写出一个看似确定、但不对应受支持语义的结果。实现需要至少在 Frozen/编译期拒绝可确认的重叠；若要运行时检测动态重叠，proposal 必须另带可比较的路径/目标摘要，在 accept 前拒绝冲突，不能指望从 lambda 反推。否则应明确这类程序不属于正确性保证范围，不能拿它验证模块功能。

此外，跨 Queue 的整条 rule 接受要求所有 `accept()` 和 Xfer 都不失败。需在不可撤销阶段之前准备 acceptedSlots 容量、元素存储与赋值所需资源，并约束生成动作的异常/失败语义；只写“必须无失败”还不足以完成实现评审。

## 建议先验证的最小场景

1. 无 ID 的 module propose：应被禁止；来源 0 仅在外部驱动或隔离测试使用。
2. Rule 已提 input pop 后，后续 peek 失败：清理全部候选；下一 delta 可重试。
3. Rule 已请求未来唤醒但 Queue 仲裁失败：未来事件不能出现；成功时仅发布一次。
4. 同函数、不同参数连续调用两次：明确诊断或成为两个独立实例，不能静默丢第二次。
5. `complete=true` 但 participants 为空：正常结束但不 firing，不更新获准时间戳。
6. 容量 1 寄存器 pop 应被拒绝；容量 1 FIFO 同来源 pop+push 的容量规则仍待定。
7. 空输入上的条件分支：Frozen、GFSim、RTL 不读取无效 payload。
8. B 中满输出 Queue 的当前 `full()` 分支、获准 pop 通知、动态 Queue 目标：验证何时重新产生生产候选。
9. 两个 rule 对同一元素的同字段 revise：拒绝或明确不属于支持范围，不能随机覆盖。

## 收敛建议

输入访问角色、来源 0 边界、寄存器不变式、零效果 rule，以及 GFSim 延迟事件的原子归属已写入基础契约。仍需确定重复调用、RTL 延迟表达和 guarded SSA 值有效域，并验证 Frozen 同时足以驱动 GFSim 与 RTL。A/B 选择、同来源 pop+push 和 B 的容量依赖图继续保留为待决问题。候选缓存仍保持独立。
