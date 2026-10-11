# Agentic Circuit：Python → MLIR 静态 rule 图设计

日期：2026-10-11。

状态：第一阶段编译契约。具体实现和验收见 `pycircuit/README.md`。

### 已确定的 Python 接口

- `@ac.module def` 的调用绑定实例；`m.state` 访问声明的状态，`m.method(...)` 调用公开 method，`m.outputs` 访问 Module 的固定返回结构。本地 import/from-import/别名按完整定义名解析；导入同一已声明实例共享身份，重新构造实例获得独立身份，不执行任意 Python。
- `@ac.rule(guard=ready)` / `@ac.method(guard=ready)` 的 guard 接收同样的参数，返回 bool，可捕获静态状态；guard 只观察，不消费或修改。method 默认观察 Queue，来自 rule 消费参数的角色沿调用保留。
- `@ac.method(After=("release",))` 声明同一组件的 `release SB 当前 method`；支持字符串、tuple/list，与 `guard=` 可共用。引用必须是本组件的公开 method。未声明、未推导出必要顺序的关系默认为 CF；声明与内部推导合并。
- 状态输出拼写为 `ac.queue[T]`、`ac.cell[T]`、`ac.signal[T]`。混合输出使用 `tuple[...]` 注解，逐位置绑定；None 省略对应效果。未注解输出仍推导为 Queue；Cell/Signal 输出必须绑定显式声明且有 `initial=` 的对应资源。Rule 普通 SSA 返回通路已移除。
- `ac.cell[T](initial=...)` 声明旧值寄存器，赋值 `.value` 表达下周期 write；Queue `.value` 赋值保留旧队尾 revise。固定 Queue 数组保持现有语法。
- `compile SOURCE --top TOP --output DIR` 保存分析后的 MLIR、JSON 和 DOT；`analyze MODEL.acir.mlir --output DIR` 只解析 MLIR 重建访问、条件与关系。读操作保持保守效果；条件证明限于常量和同一 SSA 条件的布尔互斥，不能证明的环单独诊断。

## 1. 第一阶段目标

将 Python 前端编译为包含事务语义、状态访问和静态依赖关系的 MLIR，并生成 rule 的静态关系图。

```text
Python 源码
  → 静态实例化与类型检查
  → module / rule / method / 状态操作的 MLIR
  → 条件、参数和传递资源访问分析
  → 带条件和来源的 rule 静态关系图
```

第一阶段不生成 GFSim 执行代码，不实现激活引擎、运行时调度、冲突仲裁、更新端口或 Xfer。后续可以为同一份 IR 提供不同后端。

假定程序不存在非法资源争用，不生成竞争写入的优先级或仲裁器。这不取消顺序分析、事务原子性和依赖环诊断。

## 2. 基本抽象

| 抽象 | 职责 |
| --- | --- |
| module | 组织持久状态、子 module、固定连接、rule、公开 method 和选择 rule 的控制逻辑 |
| rule | 原子事务；执行控制流、组合计算和状态操作，可以跨 module 调用 method |
| method | 可组合的公开操作；可以读取、修改状态、返回值和调用其他 method，可定义显式 guard |
| Cell | 无同周期旁路的寄存器组件 |
| EHR | 按 method 阶段读取前序有效写入、无写则回退旧值的寄存器组件 |
| Queue | 使用旧队头视图、成功 pop 释放容量和 SB 契约的 Pipeline FIFO |
| Signal | 有初值、跨 tick 保持最新值，set SB get 的状态组件 |
| SSA / var | 普通计算值，不是持久状态组件 |

module 边界不构成原子边界。同一个 module 的不同 rule 可以独立成功或失败。

method 可以由本 module 或其他 module 的 rule 调用。method 不独立发射，其 guard 和状态效果沿调用链归属于最外层 rule。在 method 中不启动另一条独立 rule。

rule / method 不仅可以操作参数绑定的资源，也可以直接访问本 module 或外部 module 的状态对象。前端可以通过 Python import、构造时传参等方式使这些对象可见；编译器必须能够在静态构造阶段确定所引用的状态实例。外部状态访问不要求经过公开 method，其效果仍归属于最外层 rule。

module 的实例层次与接口访问关系分别保存。接口引用可以传递给其他 module；共享接口始终引用同一个状态实例。

## 3. rule 输入与输出

rule 允许普通 SSA 参数及 Queue、Cell、Signal 资源参数；输出只支持 Queue、Cell、Signal。

| 端口类型 | 输入行为 | 输出行为 |
| --- | --- | --- |
| Queue<T> | 当前路径实际读取 payload 时隐式 pop | 返回 payload 向绑定的输出 Queue 隐式 push |
| Cell<T> | 读取周期开始的值，不消费 | 返回 payload 向绑定的输出 Cell 隐式 write，下周期可见 |
| Signal<T> | 值表达式 get，不消费 | 返回 payload set 已初始化的 Signal |
| 普通 SSA / var<T> | 使用普通计算值 | Rule 不支持；helper/method 可返回 |

### 3.1 输出端口类型与返回表达式

Queue / Cell / Signal 输出类型描述固定的目标状态端口，返回表达式提供要写入的 payload。它不表示把某个状态对象引用作为动态返回值。

module 侧获得的 Queue / Cell / Signal 输出是固定资源引用，不产生额外 Module 写入。

签名形式示意如下，实际 Python 类型拼写及端口绑定语法另行落实：

```text
rule transform(
    message: Queue<u32>,
    bias: Cell<u32>,
    factor: var<u32>
) -> Queue<u32>

    return message.value * factor + bias.value
```

这条 rule 的事务包含输入 Queue 的 pop 和输出 Queue 的 push；bias 只是读取。

Queue / Cell / Signal 输出目标必须在静态构造阶段确定。不能在运行时分支里创建新的状态对象。Cell/Signal 输出必须绑定显式声明且有初值的资源。

### 3.2 Queue 的隐式消费

- 只查询 empty、full 等状态不构成 payload 消费。
- 只有本次实际执行路径读取了 Queue 输入的 payload，才产生隐式 pop。
- 同一事务内，同一 Queue 经重复读取或别名读取，只隐式 pop 一次。
- 没有使用的 Queue 输入不消费。
- 分支中没有执行的输入读取不产生 pop。
- 资源身份和输入角色需要沿 helper / method 调用传播，以便分析间接读取。

例如只消费选中的输入：

```python
if choose:
    value = input_a.value
else:
    value = input_b.value
```

非消费观察与消费性输入必须区分。通过捕获资源或公开观察 method 读取 Queue，不自动把观察变成消费；具体只读接口语法在前端实现时确定。

直接访问本 module 或外部 module 的 Queue，也不因访问发生在 rule / method 内而自动产生 pop。隐式消费取决于资源的消费性输入角色；该角色沿 helper / method 调用传播，普通观察不引入新的消费角色。编译器分析全部状态访问，而不只分析输入、输出端口。

返回 None 可以沿用省略对应 Queue push 的约定。Cell/Signal 的 None 同样省略写入；混合返回使用 tuple 注解逐位置绑定。

### 3.3 Signal 的赋值、保持和原子发布

`ac.signal[T](initial=...)` 声明资源，必须有初值，无容量。已声明或传入的 Signal 在赋值左侧时 set，身份不变；普通值表达式 get，新局部绑定接收 payload SSA。Signal 参数与 Module 返回连接传递引用；`target = source` 读取源并写入目标，保持两个对象身份。

写入只允许属于 Rule 事务；Module 外层允许读取与组合计算。底层 `set SB get` 要求读者等待写者判断，不要求写者成功；获准时读新值，失败时读保留值。前序临时 set 对同 Rule 后序 get 可见，全部效果一起提交或取消。Signal 不引入 pop、容量或 EHR 端口。读取的路径及 method guard 保留，Module 准备依赖只关联实际使用者。

`-> ac.signal[T]` 返回 payload 写入预先初始化的目标；调用结果绑定不产生 Module 写入。Rule-var 结果提取、生产者有效性和 `var.validity` 已移除，普通 SSA 参数、helper/method 返回和 Module 共享计算保留。

## 4. rule / method 的原子性与条件

rule 的所有必要状态修改、输入消费和输出产生属于一个事务。method 的效果与外层 rule 一起成功，不能先发布其中一部分效果。

概念上的成功条件为：

```text
fire = 选择条件
       && rule / method 的显式 guard
       && 当前路径所有必要基础操作的 ready
```

第一阶段不加入冲突仲裁条件，也不要求把以上条件全部降低成最终可执行的布尔电路；但保存的信息必须足以供后端生成正确条件。

runtime 分支只要求实际路径上的必要操作就绪。不能把未选中分支的状态读取或 guard 无条件移到执行路径之前。

显式 guard 描述操作允许发生的业务条件；基础组件的 guard 描述组件操作的可用性。它们都可以包含运行时状态读取，均参与依赖分析。

## 5. 基础组件契约

### 5.1 Cell

Cell 的 read 和 write 通常无额外 ready 限制。

- 本 tick 所有 read 读取周期开始的值。
- 本 tick write 的值从下一个 tick 开始可见。
- 不提供同周期 write → read 旁路。
- MLIR 保存读取视图和写入效果，不指定后端怎样缓冲或提交。

多个非法竞争写入暂由程序约束排除，不实现仲裁。

### 5.2 Queue

Queue 统一采用 Pipeline FIFO，读取与消费只面向周期开始的旧数据。

| 操作 | ready | 视图或效果 |
| --- | --- | --- |
| read | !empty | 观察旧队头；本拍 push 的数据下拍才可读 |
| revise | !empty | 更新旧队尾，先于 pop |
| pop | !empty | 消费旧数据 |
| push | !full 的晚端口容量视图 | 利用旧空位或成功 pop 释放的容量 |
| empty / size | true | 查询旧状态，与修改操作默认 CF |
| full | true | 读取内部 EHR 晚端口，看到获准 pop 释放的容量 |

payload 声明 `read SB revise SB pop`；内部 EHR 容量视图形成 `pop SB query.full`，push 的 guard 调用形成 `query.full SB push`，不重复声明 pop→push。闭包包含 `read SB pop`、`read SB push`、`revise SB push`，即使某个中间接口本拍没有被调用，也保留其必要顺序。push 的 ready 只由外层事务整体成功的 pop 释放容量；仅有 pop 意图不够。

关系表只包含接口对、SB 和来源，不包含 `when`、ready 或 guard。read/pop 都要求旧状态非空。同一 rule 可以原子组合 read、revise、pop、push，内部顺序不拆分事务，也不产生 rule 自环。

### 5.3 EHR 与自动阶段

`ac.ehr[T](initial=...)` 通过 `.value` 访问，复用 Cell 状态引用、资源初始化与 `acir.read/write`，声明 `visibility="ehr"`。用户仅声明 method 的 `After`，不指定端口。

按具体 EHR 实例收集 method 访问、调用绑定和关系闭包。相关读写有明确先后时要求严格先后端口，无顺序则共阶段；纯读取之间的顺序不改变可见性。合并同阶段，再对阶段图分配连续端口。分支与是否获准不改变编号。矛盾约束报资源、method 和来源，不自动引入用户未规定的可见性。

helper 访问继承所属 method 阶段；嵌套 method 保留自己的阶段，父调用只用于提升事务依赖。直接 Rule 或 Module 准备访问用端口 0。读返回前序端口最后一次有效写入，没有则旧值，同端口写入不可见。跨 Rule 只传播整个事务获准的写入；同 Rule 内允许前序 method 临时写入，最终整体提交或取消。

Queue 内部 full 视图由旧数量初始化，有效 pop 在早端口写 false，full 与 push guard 在晚端口读取。它不是独立 bool 寄存器，拍末数量仍由实际 pop/push 计算。empty、size 和 payload 始终旧状态，不提供空 Queue 新数据旁路。EHR 后端口读取不能用作旧状态 occupancy 证明，优化不得合并不同端口读取。

声明与派生结果分开保存，MLIR 重载重新分配；图展示资源、aspect、端口及依赖来源。验收只覆盖编译表示与静态依赖，不增加仿真运行后端。

## 6. module 控制流与计算的 MLIR 保存

module 的运行时控制流选择 rule，并计算传入的 SSA 值。它不要求整个 module 的运行逻辑连续执行。

```python
offset = config.value + 1

if enabled.value:
    run(input_q, offset)
```

编译器沿 SSA 的定义链追踪选择条件和参数计算，保存其状态读取、算术、分支及值依赖，再将这些计算关联到 run 节点。

module 外层 if 等效于给对应 rule 增加选择 guard。嵌套条件取 AND，else 使用相应条件的否定。同一 if 下的多个 rule 仍是独立事务。

### 6.1 共享计算

共享表达式和同一次分支判断保留为共享 SSA 值或计算区块：

```python
if enabled.value:
    offset = config.value + 1
    rule_a(input_a, offset)
    rule_b(input_b, offset)
```

这里 enabled 的判断和 offset 的计算可以被两个节点引用。config 的读取保留在选中分支内。

共享值的求值关系及读取版本必须保留。后端不能通过任意重读状态改变同一次源程序判断的含义。

### 6.2 计算与事务分别表示

MLIR 保存：

- 无状态修改的准备计算：条件、参数和组合值，使用 SSA / CFG。
- rule 的事务函数或 region。
- 节点到选择条件、参数值、事务逻辑的关联。
- 状态读取的路径条件、版本和必要就绪要求。

准备计算包含状态读取，不应整体标记为可任意推测执行的纯函数。读取可见性、缺输入条件和分支位置必须受到保护。

第一阶段不规定后端将准备计算内联、生成函数、缓存共享值或独立调度。

## 7. 静态实例与节点身份

rule 图的节点代表具体 module 实例中的 rule 实例，不是 Python 函数定义。

同一 module 类型实例化两次，产生两组独立状态和 rule 身份。状态别名和共享接口引用需要解析到同一个资源身份。

首版建议约束：一个 rule 实例在同一个 tick 最多被选择一次。互斥分支中的多处调用可以合并，但保留其路径条件和参数选择。动态循环中的重复 rule 调用暂不支持。

method 调用目标必须编译期可确定，调用图不递归。rule 内部的普通计算循环可以保留为 CFG，不因循环自动产生新的事务节点。

## 8. 静态分析与关系图

### 8.1 资源访问摘要

对每个 rule 沿 method / helper 调用传递分析，得到：

- 可能读取和修改的资源。
- 基础操作种类：read、pop、push、write 等。
- 操作发生的路径条件。
- 显式 guard 和基础 ready 依赖。
- 读取视图、Signal 保留值语义、稳定接口身份及关系来源。

动态索引可以先保守覆盖数组中所有可能资源；常量索引和静态绑定保留精确身份。

### 8.2 顺序关系

图边 A → B 表示同一个 tick 必须先处理 A 的相关结果，再处理 B。它不表示 A 必须成功，B 才允许执行。

分析依次收集 method 与基础操作访问、绑定具体组件实例、查契约表并计算必要顺序闭包、结合路径与 guard，最后提升为 rule 顺序。只有同一实例上的 SB 形成约束。Cell read/write 默认 CF，旧值反馈不形成同周期顺序边。Signal set SB get 不要求写者成功，共享计算 SSA 来源关联实际使用者。

用户组件的默认关系来自绑定后的基础访问摘要和嵌套 method 调用。无资源参数的已注解接口也保存摘要函数，未调用的接口可以参与无条件关系闭包。资源参数和动态资源索引保留具体调用绑定，不将某次绑定的顺序无条件推广到其它绑定。嵌套接口的必要关系提升至父组件，并与用户 `After` 合并。

能证明两个访问互斥时排除约束，其余路径保守保留。关系推导保存源码位置、接口、组件和 `default` / `derived` / `user` 来源。JSON 的组件顺序边统一为 `method.order`；删除 Queue 容量／数据专用边类型。

### 8.3 路径条件与环

静态关系图保留边的适用条件和来源。多个分支产生的关系可以保守分析，但不能把互斥条件抹去后直接宣称存在真实同周期环。

可静态确定的合法执行顺序形成 DAG。真实同周期依赖环、或首版分析无法证明可静态执行的约束，应给出诊断，列出涉及的 rule、资源、操作和条件。

第一阶段不实现运行时选序或复杂互斥证明。跨周期经过持久状态的反馈不属于同周期依赖环。

### 8.4 rule 图与完整计算依赖

rule 图是主要事务视图。完整 IR 还包含产生条件和参数的计算节点。

例如：

```text
旧 Queue 读取的共享计算
  → rule_b / rule_c 的访问摘要
  → 查组件契约得到与 Queue push 所属 rule 的必要顺序
```

可以将计算依赖挂在 rule 节点上展示；MLIR 中必须保留完整关系，避免丢失后端需要的求值顺序。

每条边记录来源位置、资源 / 值身份和产生约束的操作对。这样图可用于诊断和后续后端处理。

## 9. 编译产物

一个可重载的 MLIR 文件至少保存：

1. module 实例、固定接口连接、资源身份及初始化信息。
2. 顶层组件契约表、稳定接口身份、资源／实例／调用绑定及默认、推导、用户来源；ready 与 guard 独立保存。
3. rule / method 定义及事务归属。
4. 选择条件和参数计算的 SSA / CFG。
5. Queue / Cell / Signal 输出绑定、隐式 I/O 操作和 Signal 原子发布语义。
6. 带路径条件的传递资源访问摘要。
7. 静态 rule 关系及每条边的来源。
8. 源码位置和必要的诊断信息。

重载后的分析不能依赖 Python 源文件、原始 AST 或先前进程中的对象。

可另行导出图用于检查，但图文件不是唯一语义载体。格式为 ACIR v6；v3/v4/v5 必须从 Python 源码重新编译，避免继续使用旧格式语义。

## 10. 实施顺序与验收

### 实施顺序

1. 明确三类输入 / 输出端口、Cell / Queue 和隐式 I/O 契约。
2. 扩展前端及 MLIR，表达 method、跨 module 调用和输出绑定。
3. 提取选择条件与参数的 SSA 计算依赖。
4. 建立 method 的传递 guard / 状态访问摘要。
5. 根据组件契约和值依赖生成静态关系图。
6. 提供 MLIR 重载、图导出和环诊断。

### 最小验收案例

- Cell 写入不向同周期读取旁路；EHR 的无序、严格先后、条件写入与旧值回退正确。
- EHR guard/helper/嵌套 method、跨实例、同 Rule 原子组合与冲突阶段诊断正确。
- Queue full 与 push guard 使用一致视图，满 Queue 可用获准 pop 释放容量；没有有效 pop 则回退旧 full。
- Queue 空时，本拍 push 不提供 read 或 pop 资格；没有 pop 调用仍由闭包得到 read → push。
- Queue 满时，成功 pop 可以为 push 提供容量。
- 未选中分支不产生隐式消费，重复读取同一输入只消费一次。
- Queue / Cell / Signal 的混合输出绑定与 Signal 当前值、失败回退正确保存。
- module 条件与参数计算被 rule 引用，共享计算关系保留。
- 一条 rule 跨 module 调用多个 method，效果仍属于同一事务。
- After 跨 Module 和嵌套 method 提升；不同实例隔离，互斥调用不生成边；声明和内部关系合并，矛盾顺序可追溯。
- 同一 rule 内的 SB 不产生自环，Cell 旧值反馈保持 CF。
- arith.select 资源身份、helper 循环读取依赖、词法捕获和层次名称在优化前后与 MLIR 重载中保持一致。
- 静态图边能追溯到资源、接口、操作、源码位置和关系来源。
- MLIR 单独重载后可以重新验证和分析，不需要 Python 源码。

## 11. 首版支持边界

准确的前端写法已在本文开头与 ACPy spec 固定；operation 与 attribute 组织见 ACIR v6。固定 Queue 数组保留一维语法，动态下标保守覆盖可能元素；Cell 必须显式初始化。条件证明限于常量、共享 SSA 条件的简单布尔互斥及 Queue occupancy 契约，不引入复杂求解器。

递归、运行时资源构造和运行时重复选择同一 rule 不支持。循环携带值采用保守摘要，完整循环 CFG 保存在 MLIR。激活、运行时调度、冲突仲裁、Xfer 和提交仍留给后续设计。

## 12. 与现有资料的关系

- [ACPy 当前语言 spec](acpy/spec.md)：现有前端语义和限制。
- [ACIR 当前保存格式](pycircuit/acir.md)：已有 SSA / CFG、资源分析及 MLIR 基础。
- [早期公共 method 思考](note2.md)：前期思路，包含旧 proposal / GFSim 路径。
- [Bluespec 学习笔记](bluespec-notes.md)：原子规则、method、调度与 EHR 的参考。

本文定义本次讨论收敛出的新编译目标。实现时可以复用现有 MLIR 基础，旧 proposal 生命周期与 GFSim lowering 不作为新阶段的语义前提。
