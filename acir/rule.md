# Module、Rule 与 Queue 的编译设计

本文记录拟采用的编译契约，不表示当前框架已经实现。GFSim 对象结构见 [../gfsim/module.md](../gfsim/module.md)，rule 执行与原子提交见 [../gfsim/rule.md](../gfsim/rule.md)，Queue 接口见 [../gfsim/queue.md](../gfsim/queue.md)，A/B 调度备选见 [../gfsim/schedule.md](../gfsim/schedule.md)。

## 基本模型

Module 包含连接、持久状态和运行时控制逻辑，Work 根据条件调用 rule。Module 不是原子事务，多个独立 rule 可以分别成功或失败。

Rule 包含组合计算和控制流。由实际分支选中的输入消费、输出产生与内部状态修改构成一个原子 firing；所有实际操作一起成功，否则不提交。

Queue 是唯一的电路持久状态对象。FIFO 使用多元素 Queue，寄存器使用容量 1 且初始化为占用的 Queue，通过 revise 更新。Queue 的资源身份与其读出的 SSA 值分开，连线两端共享资源。

所有计算读取当前 tick 的快照。状态修改由增量 proposal 表达，获准后统一 Xfer，顺序为 revise → pop → push。Accepted 不让新值在本 tick 被其他 Work 读取。

## Python 作者语义

Python 作者使用函数、赋值、分支和返回；不要求显式书写事务或底层 push/pop 协议。

```python
@ac.rule
def select(left_entries, right_entries, control, left, right):
    if control.choose_left:
        left_entries[control.index].value = left.value
        if control.emit:
            return Result(value=left.value)
        return None

    right_entries[control.index].value = right.value
    return Result(value=right.value)
```

本例的 entries 语义绑定到寄存器 Queue 的阵列，不是被消费的消息输入。具体 Python 声明语法另行确定。

| 表达 | 语义 |
| --- | --- |
| 局部变量赋值 | 组合值计算 |
| 持久状态赋值 | 对相应 Queue 的 revise proposal |
| Rule 读取消息输入 Queue 的 payload，包括分支条件 | 安全读取旧队首；前端为实际读取路径生成一次 pop 需求，仅在 firing 成功时消费 |
| Rule 读取 Module 持有的寄存器 Queue | 只读当前值，不生成 pop |
| Module 读取寄存器 Queue 并向 Rule 传 `var` | 传递组合值，不生成 pop；Module 不预读 Rule 的消息输入 payload |
| Module 查询 Queue 的空、满等状态 | 只读资源状态，不读取 payload，也不生成 pop |
| `return value` | 对绑定输出 Queue 的 push proposal |
| `return None` | 当前路径不产生该位置输出，正常完成 |
| `if/elif/else` | 选择调用、计算和实际 proposal |

Rule 输出数量和类型预先固定，多输出的每个正常出口保持同样返回位置，缺失值用 None。没有输出值不等于执行失败。未使用的输入和未产生的输出不参与本次资源许可检查。

本例始终读取并消费 control；左路径只使用 left 和左侧状态，右路径只使用 right 和右侧状态。左路径 emit=false 时仍消费 control/left 并更新左侧状态，不使用输出容量。

消息输入 Queue 的角色由 Rule 绑定确定，内部 FIFO 作为消息输入时同样适用。一个消息输入在同一路径反复读取，不代表多次消费；前端归一化为该 Queue 的单个消费需求。不同参数绑定同一 Queue 时需按资源身份处理，不能仅按参数名重复生成操作。寄存器读取和 Queue 状态查询不产生消费需求；底层 `peek` 本身始终是纯读。

## 编译流程

```text
Python AST
    ↓ 名称、类型、资源与控制流分析
Typed HIR
    ↓ 共享谓词、SSA 和带 guard 的操作
Raw ACIR
    ↓ 资源绑定、安全读取、完成路径与操作契约
Frozen ACIR
    ├─ GFSim：module Work / rule Work、Arbitrate / Queue Xfer
    └─ RTL：完整 fire 与受 fire 限制的状态更新
```

HIR 保留 module 和 rule 各自的结构化块，包括嵌套 if、提前返回、局部赋值和状态修改。Module 的运行时控制流必须保留，不能把它全部当作连接声明处理。本版不展开静态构造与循环的设计。

HIR 区分组合值、持久资源、消息输入访问角色、正常返回和必要读取失败。保存函数定义与实例身份、源码名称和位置。

Raw ACIR 把 if 展开成共享条件及 guarded operations，不增加 ACIR 的结构化 if，不枚举所有提交组合或复制多份 rule。局部值通过 SSA/select 合并；Module 调用和 rule 内部操作分别受其路径 guard 控制。

Frozen ACIR 固化具体 Queue 绑定、安全观察、正常完成条件、全部实际操作及其 guard、原子边界和依赖信息。后端不再分析 Python 源码来猜测资源或分支。

## ACIR 的资源与操作表示

Queue 在 rule 之前声明。输入、输出及内部状态都直接引用对应资源，不增加 output endpoint。声明、连接与输出绑定可以由编译器生成。

以下 IR 为语义示意，op 名、类型和语法不是已实现接口。Queue array 的资源参数也明确保留，避免丢失 Python 输入对应关系。

```mlir
%result_q = ac.queue ... : !ac.queue<Result>

ac.rule @select
    inputs(%control_q, %left_q, %right_q)
    outputs(%result_q)
    resources(%left_entries, %right_entries) {
  %control = ac.queue.observe %control_q when true
  %choose_left = ac.var.get %control field "choose_left"
  %index = ac.var.get %control field "index"
  %emit = ac.var.get %control field "emit"
  %right_path = ac.var.not %choose_left

  %left = ac.queue.observe %left_q when %choose_left
  %right = ac.queue.observe %right_q when %right_path

  ac.queue.propose_pop %control_q when true
  ac.queue.propose_pop %left_q when %choose_left
  ac.queue.propose_pop %right_q when %right_path

  %left_reg = ac.resource.index %left_entries[%index]
      when %choose_left
  %right_reg = ac.resource.index %right_entries[%index]
      when %right_path
  ac.queue.propose_revise %left_reg
      field "value" value %left.value when %choose_left
  ac.queue.propose_revise %right_reg
      field "value" value %right.value when %right_path

  ac.queue.propose_push %result_q Result(%left.value)
      when %choose_left && %emit
  ac.queue.propose_push %result_q Result(%right.value)
      when %right_path
  ac.rule.normal_return
}
```

Observe 表达符号化的数据需求，不表示无条件取出空 Queue 的 payload。图中的 `propose_pop` 由前端按消息输入角色和实际读取路径生成，不要求 Python 作者显式调用 pop。Propose 明确区分消费、输出和 revise；每个操作保留资源、参数、路径条件和来源。动态下标的值是运行时计算，资源定位受对应分支限制。

Module 的实际调用也必须表达或绑定明确的选择条件。未调用的 rule 不形成 attempt，不能只因内部条件满足就自动 fire。

## Frozen ACIR：安全读取与 complete

Frozen 对上例补齐读取有效性及正常完成条件。以下展示核心信息，省略位宽、类型和源码属性：

```mlir
ac.rule @select ... {
  %have_control, %control = ac.queue.try_peek %control_q when true
  %choose_left = ac.var.get %control field "choose_left"
      when %have_control
  %index = ac.var.get %control field "index" when %have_control
  %emit = ac.var.get %control field "emit" when %have_control

  %left_active = %have_control && %choose_left
  %right_active = %have_control && !%choose_left

  %have_left, %left = ac.queue.try_peek %left_q when %left_active
  %have_right, %right = ac.queue.try_peek %right_q when %right_active
  %left_complete = %left_active && %have_left
  %right_complete = %right_active && %have_right
  %complete = %left_complete || %right_complete

  ac.queue.propose_pop %control_q when %have_control
  ac.queue.propose_pop %left_q when %left_complete
  ac.queue.propose_pop %right_q when %right_complete

  %left_reg = ac.resource.index %left_entries[%index]
      when %left_complete
  %right_reg = ac.resource.index %right_entries[%index]
      when %right_complete
  ac.queue.propose_revise %left_reg
      field "value" value %left.value when %left_complete
  ac.queue.propose_revise %right_reg
      field "value" value %right.value when %right_complete

  ac.queue.propose_push %result_q Result(%left.value)
      when %left_complete && %emit
  ac.queue.propose_push %result_q Result(%right.value)
      when %right_complete
  ac.rule.complete %complete
}
```

带 guard 的字段读取和数据使用仅在其有效域内执行；布尔表达式中的条件也受此约束。GFSim/RTL 都不能将未选中或无数据路径上的值无条件取出、访问或提交。

Control 有值但选中输入缺失时，示意 IR 可能已经提出 control pop，但 complete=false。这个 proposal 必须随整条尝试被拒绝并清理，不能独立消费 control。后端也可安全地延后纯 proposal 构造，不能改变上述语义。

Frozen 明确资源操作契约：pop 需要旧队首及单消费许可；push 需要容量和单写入许可；revise 需要合法旧目标及已支持的修改范围。未活动操作没有需求。

Frozen 不必预先生成一个全局资源 can_fire 表达式；必须保留足以完整推导它的信息。它不能只留下业务条件，要求 RTL 后端凭经验补全漏失的资源需求。

## GFSim 映射

Frozen ACIR 保留 module 选择、受 guard 保护的实际 Queue 读取、proposal、正常完成条件与原子边界。Rule 请求的延迟唤醒若进入该语言，HIR/ACIR 还须保留其目标时间、ModuleId 和路径 guard，不能在 Work 中直接入调度队列；GFSim 只在整条 Rule 获准后发布。具体调用、取消与接受契约见 [GFSim Rule 文档](../gfsim/rule.md)。Proposal 数据仍由 Queue 来源槽位保存，不通过 ACIR 增加整个状态阵列的临时事务值。

## RTL 的完整 fire

对每个实际操作 i，定义 guard_i 和对应资源 grant_i：

```text
operation_ok_i = !guard_i || grant_i
fire = module_selected && complete && AND(operation_ok_i)
operation_enable_i = fire && guard_i
```

Module_selected 表示该 rule 被 module 本拍控制路径调用。Complete 表示必要观察和正常路径计算完整，RTL 中为组合条件。Grant 表示真实容量、端口和已支持竞争策略下的许可，不是无条件的 ready。

未产生的输出不要求容量，未选择的输入不要求 available。每项许可被共享路径条件门控，不需为每种提交组合分别构造一条 firing。复杂度随实际 guarded 操作和共享条件图增长，不主动枚举全部路径组合。

A/B 必须使用对应容量语义：A 不能因 pop_enable 预计为真而给其他 rule 空间；B 可以使用消费者整条 fire 已确定后的 pop 许可，并按容量依赖方向计算。不能把某个孤立 pop 意愿当作最终获准 pop。

## 依赖与身份信息

保留 Queue 到观察者 module 的依赖，包括 module 控制读取、rule 只读状态和内部流水线。保留 Queue 到可能生产者 module 的依赖，支持 B 的获准 pop 下一 delta 唤醒。

规则原子身份不绑定源码函数名，资源身份不绑定 SSA 打印名称。重复函数使用或未来独立实例能够分配独立身份，无需改变 complete、proposal 或原子边界的定义。静态展开设计本轮不讨论。

源码映射贯通 HIR、Raw/Frozen ACIR 和生成代码，便于解释某个 guard、资源需求和正常出口的来源。

## 支持边界与验收

A/B 容量策略都保留，尚未选择。自身 pop/push、来源 0 外部驱动、字段竞争和依赖建图的边界沿用 GFSim 文档。初版不求解循环容量依赖，也不因本讨论增加大量竞争 verifier。

- [ ] Python 的状态资源、输入与输出在 Raw/Frozen 中都有明确绑定。
- [ ] Module 运行时控制与 rule 内部嵌套分支均被保留为正确的活动条件。
- [ ] 未选路径及空 Queue payload 不被读取，必要读取失败导致 complete=false。
- [ ] 正常 None 出口与读取不足出口区分，部分 proposal 不会独立提交。
- [ ] 未使用输入、未产生输出不参与资源许可。
- [ ] Rule 的消息输入 payload 读取生成一次成功提交时的消费；寄存器读取、Queue 状态查询及 Module 传入的 `var` 均不消费。
- [ ] Frozen 足以生成 GFSim 原子仲裁以及 RTL 完整 fire，无需重新分析 Python。
- [ ] 条件共享、不枚举分支组合，实例与资源身份不依赖显示命名。
