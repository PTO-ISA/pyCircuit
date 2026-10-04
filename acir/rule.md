# Module、Rule 与 Queue 的编译设计

本文规定编译输入与中间表示所需保留的信息；下文 IR 为契约示意，最小实现和实际 MLIR 格式见 [独立 ACPy 编译器](../pycircuit/README.md)。GFSim 运行时契约统一见 [框架 spec](../gfsim/spec.md)，尚未确定的能力见 [待决问题](../gfsim/open-questions.md)。

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
| Rule 读取绑定输出 Queue 或只读状态引用的 payload | 只读 current，不生成 pop；可用于前递 |
| Rule 读取 Module 持有的寄存器 Queue | 只读当前值，不生成 pop |
| Module 读取寄存器 Queue 并向 Rule 传 `var` | 传递组合值，不生成 pop；Module 不预读 Rule 的消息输入 payload |
| Module 查询 Queue 的当前空、满等状态 | 只读 current，不读取 payload，也不生成 pop；不读取 accepted pop 等可变仲裁资格 |
| `return value` | 对绑定输出 Queue 的 push proposal |
| `return None` | 当前路径不产生该位置输出，正常完成 |
| `if/elif/else` | 选择调用、计算和实际 proposal |

Rule 输出数量和类型预先固定，多输出的每个正常出口保持同样返回位置，缺失值用 None。没有输出值不等于执行失败。未使用的输入和未产生的输出不参与本次资源许可检查。

本例始终读取并消费 control；左路径只使用 left 和左侧状态，右路径只使用 right 和右侧状态。左路径 emit=false 时仍消费 control/left 并更新左侧状态，不使用输出容量。

消息输入 Queue 的角色由 Rule 绑定确定，内部 FIFO 作为消息输入时同样适用。一个消息输入在同一路径反复读取，不代表多次消费；前端归一化为该 Queue 的单个消费需求。不同参数绑定同一 Queue 时需按资源身份处理，不能仅按参数名重复生成操作。寄存器读取和 Queue 状态查询不产生消费需求；底层 `peek` 不消费元素，通过静态连接保证后续激活。

消息 input 的 payload 不支持只观察而不消费：实际读取即生成受路径条件保护的 pop，即使正常返回没有输出；必要读取失败或 firing 未获准时不消费。输出或只读状态观察不生成 pop，但必须声明静态依赖。前端按当前 Rule 的绑定角色区分这些访问；例如 EX 观察自己的 EX_MEM 输出进行前递，消费 EX_MEM 的 pop 由 MEM Rule 的 input 读取生成。

## 编译流程

```text
ACPy AST（静态构造、类型推导、输出预声明）
    ↓
ACIR MLIR（func / arith / cf + acir 资源与效果操作）
    ↓ 资源分析、分支内安全读取、Rule 生命周期
EmitC
    ↓ MLIR C++ exporter 与静态资源布局
GFSim Module / Rule / Queue / Signal
```

Module 与 Rule 保留真实控制流、SSA 块参数和运行时循环。没有中间 HIR、JSON ACIR
或 guard 展平路径。输出绑定和所有资源先于行为编译完成，源码位置保存在 MLIR 中。
实际保存格式、注册 ODS 类型与操作见 [ACIR MLIR v2](../pycircuit/acir.md)。

## ACIR 的资源与操作表示

Queue 在函数行为之外声明。输入、输出及内部状态直接引用资源，不增加 output endpoint。
`acir.get` 获取身份；`acir.read/query` 表达观察；`acir.pop/push/revise` 表达当前路径效果；
`acir.invoke` 表达 Module 的 Rule 选择，`acir.event` 表达事务内未来唤醒。

Module、Rule、Signal、helper 都使用带角色属性的 `func.func`。普通计算复用 arith/func，
分支使用 cf，固定资源数组的动态索引保留实际 SSA 下标。静态分析保守声明全部可能元素，
GFSim 的静态通知覆盖所有可能元素，proposal 仅包含实际操作的元素。

Queue 读取与查询保留保守效果，ODS 通过标准 MemoryEffectOpInterface 声明读写效果，
不能标记 Pure、删除未使用读取或将操作推测到原分支外。合法纯计算可使用 canonicalize/CSE。

## 安全读取与 complete

`acir-lower-gfsim` 在 Rule 入口生成 beginRule，在每个正常出口生成 completeRule。
必要 Queue 读取在其所在 CFG 位置展开 tryPeek 与非空分支；读空分支 abortRule 后返回。
Module 控制读取失败只结束当前 Work，保留此前已完成的独立 Rule。Signal 的无保护读空保持错误语义。

正常 None 返回与必要读取失败不同：前者不生成对应 push，但其他效果可以提交；
后者撤销整条候选的 pop、push、revise 和事件，静态连接始终有效以便重试。
别名或重复输入读取的 pop 按实际 Queue 身份去重。

pop 需要旧队首；push 需要容量；revise 需要合法旧目标。未执行操作没有资源需求。
动态字段 revise 捕获索引和值，通过通用回调在 Xfer 更新旧队尾，不复制整个资源阵列。

## GFSim 映射

C++ pass 根据相同静态信息生成类、成员函数、资源构造、访问声明及 proposal 来源绑定。
EmitC 生成行为函数，GFSim 负责候选准备、仲裁、提交、清理与事件登记，运行时契约仍以
[GFSim spec](../gfsim/spec.md) 为准。后端无需重新分析 Python，不识别 CPU 组件名称。

Module 未选择的 Rule 不形成候选；GFSim 可以保留最近一次选择产生的完整 pending 候选，
跨 tick 直接仲裁，不要求该 tick 再调用 Rule Work。

下面 RTL fire 公式仍是设计契约；本项目当前只实现 GFSim 后端，未实现 RTL 转换。

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

编译器按全部可能访问建立 Queue/Signal→Module 激活关系、Queue/Signal→Signal 输入关系，以及 Rule→Queue proposal 来源。Queue 的唯一 pop/push 来源构成静态容量边。运行时没有读取登记、局部资源槽位、Rule 读者位图或参数缓存。

`work_<rule>(args)` 使用 begin/complete/abort，读取与消费仍分离。Module 激活时先清全部旧候选，再执行选中 Rule；未激活 Module 保留完整 pending。容量释放仅将 pending 加入下一 delta 仲裁，不运行 Work。事件只在整体获准时发布，详见 [生命周期](../gfsim/spec.md#prepare)。

Signal 输入与下游均为固定连接；输入变化在全部 Queue Xfer 后按 Signal DAG 的拓扑序去重求值，输出变化才通知下游 Signal、激活关联 Module。初始化也使用同一拓扑序，组合环报错。Python 实验引擎保留历史动态语义，当前生成器只连接 C++ 静态调度接口。

同一 Module 内的同一 Rule 共享身份；不同 Module 实例独立。资源身份不绑定 SSA 打印名称，别名不创建新 Queue。

源码映射贯通 ACPy 与 MLIR，便于解释路径、资源需求和正常出口。

## 支持边界与验收

运行时支持边界见 GFSim spec。当前 ACPy/ACIR 接口见对应说明和 ODS；RTL fire 仍未实现。

- [x] Python 的状态资源、输入与输出在 MLIR 中都有明确绑定。
- [x] Module 运行时控制与 rule 内部嵌套分支均被保留为正确的活动条件。
- [x] 未选路径及空 Queue payload 不被读取，必要读取失败导致 complete=false。
- [x] 正常 None 出口与读取不足出口区分，部分 proposal 不会独立提交。
- [x] 未使用输入、未产生输出不参与资源许可。
- [x] Rule 的消息输入 payload 读取生成一次成功提交时的消费；寄存器读取、Queue 状态查询及 Module 传入的 `var` 均不消费。
- [x] Module 静态资源连接、普通 var 参数、proposal 来源和候选生命周期均可显式生成。
- [ ] RTL 完整 fire 转换（GFSim 已实现）。
- [x] 条件共享、不枚举分支组合，实例与资源身份不依赖显示命名。
