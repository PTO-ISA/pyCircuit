# Module、Rule 与 Queue 的编译设计

本文规定编译输入与中间表示所需保留的信息；下文 IR 为契约示意，最小实现和实际 JSON 格式见 [独立 ACPy 编译器](../pycircuit/README.md)。GFSim 运行时契约统一见 [框架 spec](../gfsim/spec.md)，尚未确定的能力见 [待决问题](../gfsim/open-questions.md)。

## 基本模型

编译器保留两层控制流：Module 选择调用哪些 Rule；Rule 的实际分支决定读取和修改哪些资源。每条 Rule 的实际修改构成原子 firing。调度与快照语义见 [执行模型](../gfsim/spec.md#model) 和 [Work 语义](../gfsim/spec.md#work)。

当前状态资源是 Queue，包括 FIFO 和用受限 Queue 表示的寄存器。资源身份与读取出的 SSA 值分开，连线两端共享同一资源；独立 Cell 尚未定义。

GFSim 本版约束每个 Queue 的 pop 来源至多一个 Rule，push 来源至多一个 Rule；二者可以相同或不同，覆盖所有分支和 tick。当前不检测，由用户代码遵守，违反时不保证运行结果。多个处理路径由相应 Rule 的控制流表达。

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
| Rule 读取绑定输出 Queue 或只读状态引用的 payload | 只读 current 并登记依赖，不生成 pop；可用于前递 |
| Rule 读取 Module 持有的寄存器 Queue | 只读当前值，不生成 pop |
| Module 读取寄存器 Queue 并向 Rule 传 `var` | 传递组合值，不生成 pop；Module 不预读 Rule 的消息输入 payload |
| Module 查询 Queue 的当前空、满等状态 | 只读 current，不读取 payload，也不生成 pop；不读取 accepted pop 等可变仲裁资格 |
| `return value` | 对绑定输出 Queue 的 push proposal |
| `return None` | 当前路径不产生该位置输出，正常完成 |
| `if/elif/else` | 选择调用、计算和实际 proposal |

Rule 输出数量和类型预先固定，多输出的每个正常出口保持同样返回位置，缺失值用 None。没有输出值不等于执行失败。未使用的输入和未产生的输出不参与本次资源许可检查。

本例始终读取并消费 control；左路径只使用 left 和左侧状态，右路径只使用 right 和右侧状态。左路径 emit=false 时仍消费 control/left 并更新左侧状态，不使用输出容量。

消息输入 Queue 的角色由 Rule 绑定确定，内部 FIFO 作为消息输入时同样适用。一个消息输入在同一路径反复读取，不代表多次消费；前端归一化为该 Queue 的单个消费需求。不同参数绑定同一 Queue 时需按资源身份处理，不能仅按参数名重复生成操作。寄存器读取和 Queue 状态查询不产生消费需求；底层 `peek` 不消费元素，在 GFSim Work 中自动登记读取依赖。

消息 input 的 payload 不支持只观察而不消费：实际读取即生成受路径条件保护的 pop，即使正常返回没有输出；必要读取失败或 firing 未获准时不消费。输出或只读状态观察不生成 pop，但必须记录依赖。前端按当前 Rule 的绑定角色区分这些访问；例如 EX 观察自己的 EX_MEM 输出进行前递，消费 EX_MEM 的 pop 由 MEM Rule 的 input 读取生成。

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

Module 对 Rule 的选择必须表达或绑定明确的条件。未被选中的 Rule 不形成候选，不能只因内部条件满足就自动 fire。GFSim 可以保留最近一次 Module Work 选中的完整 pending 候选，跨 tick 直接仲裁；不要求本 tick 再次调用 Rule Work，见 [spec 候选生命周期](../gfsim/spec.md#commit)。

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

Frozen 明确资源操作契约：pop 需要旧队首；push 需要容量；revise 需要合法旧目标及已支持的修改范围。未活动操作没有需求。

Frozen 不必预先生成一个全局资源 can_fire 表达式；必须保留足以完整推导它的信息。它不能只留下业务条件，要求 RTL 后端凭经验补全漏失的资源需求。

## GFSim 映射

Frozen ACIR 保留 Module 选择、受 guard 保护的读取、proposal、正常完成条件与原子边界。生成成员函数、读取登记和静态记录所需的信息见 [spec 第 2 节](../gfsim/spec.md#construction)。运行时候选准备、仲裁和清理统一遵循 spec，不在编译文档另设一套生命周期。

Proposal 表达增量，不通过 ACIR 增加整个状态阵列的临时事务值。Rule 的未来唤醒请求保留目标 ModuleId、delay、路径 guard 和原子边界；GFSim 在整条 Rule 获准后计算到期 tick 并登记事件，允许纯事件 Rule。运行时语义见 [事件登记](../gfsim/spec.md#events)。

## RTL 的完整 fire

对每个实际操作 i，定义 guard_i 和对应资源 grant_i：

```text
operation_ok_i = !guard_i || grant_i
fire = module_selected && complete && AND(operation_ok_i)
operation_enable_i = fire && guard_i
```

Module_selected 表示当前 Module 控制条件选择该 Rule；GFSim 保留的选择在相关状态或事件激活 Module Work 时更新，不以本 tick 是否实际调用 Work 判断 firing 资格。Complete 表示必要观察和正常路径计算完整，RTL 中为组合条件。Grant 表示实际操作的资源许可，不是无条件的 ready。GFSim 本版不处理多 revise 等资源端口竞争，出现时不保证运行结果。

未产生的输出不要求容量，未选择的输入不要求 available。每项许可被共享路径条件门控，不需为每种提交组合分别构造一条 firing。复杂度随实际 guarded 操作和共享条件图增长，不主动枚举全部路径组合。

RTL 必须保持 [Queue 的 push 容量语义](../gfsim/spec.md#arbitration)：同 tick 有合法 pop 就可以复用其空间，包括同一 Rule 的 pop/push。来自其他 Rule 的空间必须以消费者整条 fire 已确定为前提，按容量依赖方向计算；同一 Rule 的 pop/push 共同受完整 fire 约束。不能把某个孤立 pop 意愿当作最终获准 pop。

## 依赖与身份信息

生成代码保留 Module 控制读取、Rule 实际读取及 pop/revise 目标；Python runtime 在相应 Queue 操作内自动登记，不要求生成代码另写 record_read。编译期保存 Queue 的 proposal 来源槽位及唯一 popRuleId/pushRuleId；动态读取关系与这些静态记录分开，定义见 [静态构造](../gfsim/spec.md#construction) 和 [运行记录](../gfsim/spec.md#records)。

编译器为每个 Module 显式声明全部可访问 Queue（包含输入、输出、私有状态和只读引用），为所属 Rule 分配局部位号，并构造 Queue→Module 的固定可能读者链接以及按 ModuleId 索引的局部槽位表（未声明为 -1）。别名合并为同一 QueueId；动态数组须声明全部可能表项。声明只限制可访问范围，不能把静态可能读取当成实际读取。

Module 控制读取按代号登记；Rule 的实际读取（含读空、pop/revise 目标）设置所属 Module 的资源读者位，并记录实际 readSlots。Queue 变化通过位图标记实际读者 Rule dirty；生成代码传入的普通 var 按值比较，变化时标记同一 dirty 位。Rule 既可依赖 input Queue，也可读取 Module 内部 Queue，两者使用同一动态登记机制。

生成 `work_<rule>(args)` 使用显式 begin/complete/abort 核心接口，runtime 据此维护 activeModule/activeRule。Queue 查询在 Work 内按该上下文登记，读空也登记；complete/abort 恢复 Module 上下文，Module 退出时清除，Work 外观察不订阅。生成代码直接写 peek/try_peek 等接口，消息 input 的 pop 仍显式生成，不由 peek 自动消费。显式 record_read 保留兼容，不应在新生成代码中重复插入。完整候选未 dirty 时直接复用，不扫描 Queue 版本、不重新登记读取。重算清旧候选和旧读者位；abort 与提交只清候选，保留已经尝试的读取；Module 不再选择 Rule 时才同时清候选与读者位。具体生命周期统一见 [spec 第 5 节](../gfsim/spec.md#prepare)。

原 GFSim 实验模拟生成代码；独立编译器现已复用该接口。普通 Module 类和成员 Rule 无需 Stage 公共基类、observe/take/put 包装层；上文 ACIR 示意操作不是新增 Python 引擎 API。Module→Module var 的因果传播和 delta Work 本轮不定义。

规则原子身份不绑定源码函数名，资源身份不绑定 SSA 打印名称。重复函数使用或未来独立实例能够分配独立身份，无需改变 complete、proposal 或原子边界的定义。静态展开设计本轮不讨论。

源码映射贯通 HIR、Raw/Frozen ACIR 和生成代码，便于解释某个 guard、资源需求和正常出口的来源。

## 支持边界与验收

运行时支持边界和未决问题统一见 GFSim spec 与问题记录。本文中的 IR 为语义示意，具体 Python 声明语法、op 名和类型语法尚未固化；静态构造与循环展开也未在本文定义。

- [ ] Python 的状态资源、输入与输出在 Raw/Frozen 中都有明确绑定。
- [ ] Module 运行时控制与 rule 内部嵌套分支均被保留为正确的活动条件。
- [ ] 未选路径及空 Queue payload 不被读取，必要读取失败导致 complete=false。
- [ ] 正常 None 出口与读取不足出口区分，部分 proposal 不会独立提交。
- [ ] 未使用输入、未产生输出不参与资源许可。
- [ ] Rule 的消息输入 payload 读取生成一次成功提交时的消费；寄存器读取、Queue 状态查询及 Module 传入的 `var` 均不消费。
- [ ] Module 可访问资源、局部 Rule 位号、实际动态读取、var 参数值比较和候选生命周期均可显式生成。
- [ ] Frozen 足以生成 GFSim 原子仲裁以及 RTL 完整 fire，无需重新分析 Python。
- [ ] 条件共享、不枚举分支组合，实例与资源身份不依赖显示命名。
