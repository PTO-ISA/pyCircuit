# Agentic Circuit Stateful Table Abstraction Design

> 状态：epoch `0.4` 历史原型由 Decisions 0151–0156 定义；epoch `0.5` 已采用
> `@ac.rule` 与 staged MLIR lowering，其余 Table/Reg rule 内容为后续设计
> 当前范围：一维、全零初始化、字段不相交的多 writer、单一 scalar allocation、state-driven scalar/masked update、match/choose、committed slot、typed gfsim C++
> 规范依据：Decisions 0151–0156 与 `docs/acir/spec/agentic-circuit*.md`

## 0. epoch 0.4 原型边界

当前实现承诺 `ac.table[entries, Entry]` 的 `init=0` 形式、`EntryView.read()`、
Queue-driven 与 state-driven `write/patch`、`table.match/choose`、`ac.slot(queue)`，
同 Table CandidateSet 驱动的 masked `write/patch`，以及 Frozen ACIR 的
`ac.table/get/read/write/masked_write/match/choose` 与 `ac.slot/get/release`
以及 QueueGraph 到 typed gfsim C++ 的纵向链路。`patch` 在 Frozen ACIR 前展开为
`table.get -> var.with -> table.write/masked_write`。同 tick 读写返回 old committed Entry，write
在 tick commit 后可见；动态越界报告 `table_index_out_of_range`。多个 scalar/masked
writer 仅在规范化顶层写字段集合两两不相交时合法。每个 endpoint 从同一 old committed
image 求值，commit 只合并其声明字段并一次发布；同字段 writer 无条件静态拒绝。
每张 Table 还可有一个 state-driven scalar allocation endpoint；它以
`ac.table.write mode "replace"` 安装完整 Entry。普通字段 proposal 先合并，replace 后应用，因此同 Entry
复用时 allocation wins。allocation 不搜索空位、不检查占用，也不隐式修改 `valid`。
每个作者声明的 match/choose 在 Frozen ACIR 与 QueueGraph 中只保留一个共享定义；各
endpoint 捕获共享 SSA/ref。typed gfsim 按完整 Epoch 惰性缓存，因此同一 Epoch 无论多少
read/write consumer 都只扫描一次，Epoch 前进或 reset 后失效。`first` choose 使用空 key
region，min/max 使用单一 typed key region。

本文件下文关于 `ac.firing`、跨对象原子性、PYC/RTL、多维、多选、同字段多
writer 的动态互斥或优先级、非零 image、仲裁及 SRAM inference 的设计均为明确 deferred，不是 epoch
`0.4` 的可用产品契约。旧 `ac.table(value, address=..., ...)` 已删除；请求响应 memory
继续使用 `ac.memory`。

## 1. 目标

`ac.table<Entry>` 用于表达固定大小、同构 Entry 组成的状态结构，作为以下硬件组件的通用基础抽象：

```text
ROB
Issue Table / Reservation Station
Scoreboard
Rename Map
Free List
MSHR
Cache Tag Table
Replacement Metadata
Directory
TLB / CAM
Outstanding Transaction Table
```

核心定义：

> Table 是由单个组件拥有的、固定 shape、同构 Entry、跨 tick 保存状态的硬件容器，支持组合索引读取、并行匹配、候选选择和在 commit edge 提交的并行写入。

Frozen Table primitive 不承担：

```text
ready-valid
request-response
访问 latency
bank arbitration
enqueue/dequeue
insert/delete
ROB commit
cache hit/miss
LRU
动态 pointer/reference
```

这些行为由 Table 的 owner、现有 Queue/firing primitive 或独立 Module 表达。简单前端
`EntryView.read/write/patch` 可以隐式建立零/单输入、零/单输出 firing；多个 Queue 输入或
多个 Queue 输出使用 `@ac.rule` 显式框定事务。这些前端写法不会给 Frozen Table primitive
增加 ready-valid、latency 或隐藏状态。

## 2. Table 与 Module 的边界

### 2.1 Table

Table 解决的问题是：

> 在固定数量的同构状态 Entry 中，读取、匹配、选择和更新哪些 Entry？

Table 具有：

```text
静态 shape
不可变 Entry value
组合读取
并行匹配
显式写 proposal
时钟沿提交
```

### 2.2 Module

Module 解决的问题是：

> 向独立硬件单元发送 transaction，并在 ready-valid、latency 或内部调度后获得 response。

具有以下性质的结构应建模为 Module，而不是 Table：

```text
多周期 SRAM
带 bank conflict 的 Data Array
异步 request-response 单元
DMA Engine
DRAM Channel
NoC Port
执行单元
```

多个同构 Module 可以由更高层 structural abstraction 展开成：

```text
instances + route + merge
```

这不属于第一版 Table 设计范围。

## 3. 前端声明

统一使用小写 `ac.table`。

一维 Table：

```python
rob = ac.table[128, RobEntry](
    init=RobEntry(
        valid=False,
        done=False,
        result=0,
    )
)
```

多维 Table：

```python
tags = ac.table[[SETS, WAYS], TagEntry](
    init=TagEntry(
        valid=False,
        tag=0,
    )
)
```

基于静态索引初始化：

```python
rename_map = ac.table[ARCH_REGS, ac.u8](
    init=lambda index: index
)
```

约束：

```text
shape 必须在 elaboration 时确定
每个维度必须大于 0
Entry 类型必须在 Frozen ACIR 前确定
init 必须生成确定的静态初始 image
```

### 3.1 配套 Scalar State

Table index、计数器和指针等单个定宽状态不伪装成单 Entry Table。本设计假设存在通用
`ac.reg`：

```python
head = ac.reg[ac.u7](init=0)
tail = ac.reg[ac.u7](init=0)
```

`head`/`tail` 在组合表达式中读取当前 committed scalar value，并可直接作为
`table.view(index)` 的 index。在已经建立的隐式 firing 或 `@ac.rule` 中，增强赋值形成 next-state
proposal：

```python
tail += 1
```

概念 Frozen ACIR：

```mlir
ac.reg @tail type i7 init 0

%tail = ac.reg.get @tail : !ac.var<i7>
%next_tail = ac.var.add %tail, %one : !ac.var<i7>
ac.reg.write @tail, %next_tail enable %true
```

`!ac.var<i7>` 只是当前 tick 的 immutable、zero-latency 值；跨 tick 保存状态的是
`ac.reg`，不是 `ac.var`。`ac.reg` 是配套的通用 scalar-state primitive，不属于 Table
primitive family。

## 4. Entry 类型

Entry 必须是固定宽度、不可变、可综合的值。

允许：

```text
bool
定宽 signed/unsigned integer
固定宽度 bit vector
record/struct
由上述类型组成的固定 shape 嵌套值
```

禁止递归包含：

```text
Queue
Table
View
Mask
Selection
Module handle
动态 list/map/set
pointer/reference
function
动态长度对象
```

完整 Entry 写入必须属于 firing。简单写入可以直接由 Queue-driven anchor 建立：

```python
rob.view(lambda request: request.index).write(
    requests,
    value=lambda request: new_entry(request),
)
```

字段 Patch 仅适用于 record/struct：

```python
rob.view(lambda completion: completion.index).patch(
    completions,
    done=True,
    result=lambda completion: completion.result,
)
```

字段名必须静态确定。

## 5. 单 Owner

每个 Table 有且只有一个静态 owner。

```python
@ac.component
class Rob:
    entries = ac.table[128, RobEntry](init=...)
```

这里 `Rob` 是 `entries` 的 owner。

Owner 负责：

```text
保存 committed state
提供组合读取
收集 write proposal
检查动态写冲突
在 commit edge 更新 next state
执行 reset
提供 snapshot
```

同一个 owner 内可以有多个独立 firing。

其他组件不能通过 SymbolRef 直接读写该 Table。外部影响必须通过 owner 的显式接口进入。

Frozen ACIR 中 Table 位于 owner 的词法范围：

```mlir
ac.component @rob {
  ac.table @entries
      shape [128]
      element !ac.struct<@types::@RobEntry>
      init #ac.table_init<zero>
}
```

如果具体 IR 阶段尚不能词法嵌套，可以暂时使用显式 owner 引用，但语义仍是唯一 owner。

## 6. Firing 前端模型

> **后续公共前端方向：** `D-RULE-LOWERING-001` 已决定由 `@ac.rule` 作为唯一显式
> 调度边界。Python 用户不再书写 `atomic`、`firing`、显式 Queue effect 或检查；类型、
> effect、guard、检查、握手和冲突由 MLIR pass 分阶段推导。下文的 implicit anchor、
> `with ac.atomic()` 和 `.firing()` 描述 epoch `0.4` prototype 与形成该决策时的设计背景，不是
> 目标公共 API。
>
> 下列小节的状态为：6.1 implicit anchor、6.2 基本块附着、6.3 显式 `when=` 和
> 6.4 decorator `when` callback 的 Python 拼写均被 supersede；它们表达的 firing guard
> 与 local effect predicate 区别继续保留，但改由 `@ac.rule` 的普通控制流和 MLIR path
> predicate 推导。6.5 单 token/tick 限制与 committed-snapshot 语义继续适用，直至后续
> decision 明确修改。

Firing 表示组件在一个 tick 中成功完成的一次完整状态转换：

> 一个 firing 中的 Queue 输入消费、Queue 输出产生、Table write 和 Reg write 共享同一个
> `fire`，一起提交或一起取消。

Frozen ACIR 始终显式使用 `ac.firing`。Python 前端根据 Queue 拓扑提供两种写法：

```text
简单的零/单输入、零/单输出 Table 操作
    → Table 方法隐式建立 firing

多个输入 Queue、多个输出 Queue 或无法唯一归组的事务
    → @ac.rule 显式定义 firing scope
```

因此从前端看，table 可以作为 Queue 的 sink（`write/patch(queue, ...)`），也可以产生
Queue 输出（`read(...)`）。但在 Frozen ACIR 中，Queue handshake 属于 `ac.firing`，
`ac.table.get/write` 只负责 committed state 观察和 next-state proposal；table 本身不是
Queue endpoint。

纯 Queue 到 Queue 的组合操作继续使用 `ac.transform`、`ac.broadcast` 等已有 opcode。
这些 opcode 自己定义输入消费和输出产生，不需要再包一层 `ac.firing`；只有需要把
Queue transaction 与 Table/Reg proposal 绑定，或无法由更窄 opcode 表达时，才使用
`ac.firing`。

### 6.1 简单 Firing Anchor（已被目标前端取代）

能够直接消费或产生 Queue 的 Table 方法是 firing anchor：

```python
# one input, no output
rob.view(lambda completion: completion.rob_index).patch(
    completions,
    done=True,
    result=lambda completion: completion.result,
)

# one input, one output
responses = tags.view(
    lambda request: (request.set_index, request.way)
).read(requests)

# zero input, one output
retired = rob.view(head).read(when=can_retire)
```

这些方法只是前端 sugar，分别 lower 为 `1→0`、`1→1` 和 `0→1` 的 `ac.firing`；Frozen
Table primitive 本身仍然没有 Queue operand/result。

第一版方法形态固定为：

```text
entry.read([input_queue], *, result=..., when=...)
entry.write(input_queue, *, value=..., when=..., enable=...)
entry.patch(input_queue, *, when=..., enable=..., fields...)

entry.write(*, value=..., enable=...)     # 只附着到已有 firing
entry.patch(*, enable=..., fields...)     # 只附着到已有 firing
```

`when=` 只允许出现在 anchor 上；无 Queue 的附着式 `write/patch` 只能使用 local
`enable=`。Queue operand 与 Entry value 不复用同一个位置参数，避免编译器根据动态类型
猜测调用含义。

`match(queue, predicate)` 也可以成为单输入 anchor，用于让 predicate 和后续 masked state
effect 共享同一个 token：

```python
waiting = scoreboard.match(
    writebacks,
    lambda entry, writeback:
        entry.allocated & (entry.producer == writeback.producer),
)
scoreboard.view(waiting).patch(ready=True)
```

若 Queue-dependent `match` 所在 group 最终既没有状态 effect 也没有 Queue 输出，前端必须
报错，不能无意义地消费输入 token。

### 6.2 Anchor 与基本块边界（已被目标前端取代）

一个 anchor 建立当前 Python 基本块中的隐式 firing。紧随其后的无 Queue `write/patch`
和 `ac.reg` 更新附着到该 firing：

```python
rob.view(tail).write(
    requests,
    value=lambda request:
        RobEntry(
            valid=True,
            done=False,
            instruction=request,
            result=0,
        ),
)
tail += 1
```

该 firing 只有在请求 token 可消费时才一起提交 ROB write 和 `tail` increment。

以下任一情况结束当前隐式 firing：

```text
离开当前 Python 基本块
遇到下一个 firing anchor
遇到 return
进入不属于当前控制路径的新分支
函数结束
```

第二个 anchor 关闭前一个 firing 并建立新的 firing：

```python
first = table.view(i).read()
table.view(i).patch(valid=False)

second = table.view(j).read()
table.view(j).patch(valid=False)
```

无参数 `write()`/`patch()` 不能独立创建 autonomous firing。若当前基本块没有唯一 anchor，
前端必须报告 `state effect has no firing anchor`。如果一个 effect 可能归属于多个 anchor，
也必须报错并要求使用 `@ac.rule`，不能依赖任意语句顺序猜测。

### 6.3 Guard 与 Local Enable（拼写已取代，语义保留）

第一版不要求 Python 前端编译 symbolic `if`。Firing guard 由 anchor 的 `when=` 显式提供：

```python
head_entry = rob.view(head)
can_retire = head_entry.valid & head_entry.done

retired = head_entry.read(when=can_retire)
head_entry.patch(valid=False)
head += 1
```

这里 `when=can_retire` lower 为 `ac.firing.yield guard %can_retire`。Guard 为假时不消费
输入、不产生输出，也不提交任何状态 proposal。

单个 write/patch 的 `enable=` 是 local enable：

```python
entry.patch(valid=False, enable=should_clear)
```

其实际写使能为：

```text
firing_fire && should_clear
```

local enable 为假只取消该 write，不取消 firing。需要阻止整个事务时必须使用 anchor 的
`when=`，不能使用 `enable=` 代替。

### 6.4 显式 `@ac.rule`（decorator guard 草案已取代）

多个 Queue 输入或多个 Queue 输出需要共同提交时，使用 `@ac.rule`：

```python
@ac.rule
def rename(inst, new_physical):
    src0 = rename_map.view(inst.src0_arch)
    src1 = rename_map.view(inst.src1_arch)
    old = rename_map.view(inst.destination_arch)

    rename_map.view(inst.destination_arch).write(value=new_physical)

    return (
        RenamedInstruction(
            instruction=inst.instruction,
            src0_physical=src0,
            src1_physical=src1,
            destination_physical=new_physical,
        ),
        RenameRecord(
            destination_arch=inst.destination_arch,
            old_physical=old,
            new_physical=new_physical,
        ),
    )

renamed, record = rename(decoded_with_dst, free_phys)
```

规则：

```text
调用实参是输入 Queue
函数参数是 firing-local immutable payload Var
return value 自动形成输出 Queue
函数内无 Queue 的 Table/Reg effect 属于该 rule
rule 内禁止 read() 和带 Queue 参数的 Table 方法，避免嵌套 firing
普通 def/lambda 仍是纯组合 helper，不形成 firing
```

`@ac.rule` 默认 guard 为 true。需要非平凡 guard 时，第一版在 decorator 上提供纯组合
`when` callback；callback 接收与 rule body 相同的 payload Var：

```python
def request_enabled(request, credit):
    return request.valid & credit.valid

@ac.rule(when=request_enabled)
def admit(request, credit):
    ...
```

`when` callback 可以组合观察 committed Table/Reg state，但不允许 Queue effect、Table/Reg
write 或 Python side effect。

### 6.5 第一版吞吐限制

第一版只处理单 token/tick：

```text
每个 firing 每 tick 最多 firing 一次
每个输入 Queue 每次 firing 消费一个 token
任何读写 Table 或 ac.reg 的 Queue-driven firing 要求 trigger Queue rate = 1
零输入 state-driven firing 每 tick 最多产生一个输出 token
```

`choose(count=K)` 仍可在一次 firing 中选择并更新 K 个不同 Entry；它不表示消费 K 个
trigger token。若 Table state effect 绑定到 `rate > 1` 的 Queue，第一版前端必须诊断，
不得让多个 lane 读取同一个 committed scalar index 后产生冲突 proposal。多 token/tick
需要未来单独定义 batch、lane index、accepted count 和 intra-batch state forwarding。

同一个 owner 每个 tick 可以有多个 firing：

```text
allocation firing
completion firing
retirement firing
```

它们：

- 读取相同的旧 committed snapshot；
- 可以独立成功或阻塞；
- 在同一个 commit edge 提交；
- 不得对同一 Entry 产生冲突写入。

## 7. Tick 与可见性

Table 使用 committed-snapshot get / next-state write 语义。

```text
tick T combinational phase:
    get/match/choose 观察 committed state T
    write/patch 生成 next-state proposal

commit edge:
    提交所有已接受、有效且无冲突的 write proposal

tick T+1:
    get/match/choose 观察新 committed state
```

同一 firing 中：

```python
entry = table.view(i)
old_field = entry.some_field
entry.write(requests, value=new_value)
```

`old_field` 仍来自旧 committed value，而不是 `new_value`。

## 8. View

完整索引产生 `EntryView`：

```python
entry = rob.view(index)
can_commit = entry.valid & entry.done
```

部分索引产生 `TableView`：

```python
set_view = tags.view(set_index)
line = set_view.view(way)
```

View 是前端临时 AccessPath：

```text
Table + bound selection path
```

View 不是 runtime pointer/reference。

完整索引的 `EntryView` 允许：

```python
entry = table.view(lambda request: request.index)
responses = entry.read(
    requests,
    result=lambda value, request: make_response(value, request),
)
entry.patch(
    field=lambda value, request: update(value, request),
)
```

这里第一个 `read(requests)` 建立 firing，后续无参数 `patch()` 自动附着到同一基本块的
当前 group，并继续允许其 value lambda 使用同一个 firing-local `request`。

完整替换 Entry 时，使用 `entry.write(value=value)` 代替 patch；同一 firing 不得同时对同一
Entry 产生两个有效 write proposal。

`entry.field` 产生临时组合 `Var`。`read(queue)` 不是本地解引用，而是建立一个单输入、
单输出 firing；无参数 `read()` 建立一个零输入、单输出 firing。两者默认 payload 都是
完整 Entry，并且都可以通过 `when=` 设置 firing guard。

当完整 `EntryView` 出现在需要 Entry value 的组合表达式中时，前端会自动 materialize
对应的旧 committed Entry。例如将 `entry` 放入另一个 struct，等价于使用一次
`ac.table.get` 的结果，但不会创建 Queue。

部分索引或 Mask 的 `TableView` 允许：

```python
set_view = tags.view(set_index)
hits = set_view.match(...)
selected = set_view.choose(hits, ...)
set_view.view(way).patch(...)
issue.view(mask).patch(...)
```

单操作的 Queue-facing View 可以用 token-to-index lambda 表达：

```python
responses = rob.view(
    lambda request: request.rob_index
).read(requests)
```

这种 token-dependent View 只能由该 Queue anchor 及其随后附着的 state effect 使用。在
anchor 建立 firing-local token 之前访问其字段，或把它交给另一个 anchor，都是编译错误。

普通前端代码中禁止：

```python
state.write(view)
table.view(i).write(view)
component_output = view  # 试图跨 owner 导出 View
```

View 不能跨 tick、跨 owner 或跨组件接口存活，并在 Frozen ACIR 前完全消失。
在 `@ac.rule` 内将完整 `EntryView` 作为 rule return payload 是允许的；编译器先将它
materialize 为 immutable Entry Var，再生成输出 Queue。

## 9. EntryView 字段观察与 Firing Read

本地组合观察不调用 `read()`：

```python
entry = rob.view(index)
done = entry.done
```

`EntryView` 第一次被观察时 materialize 一次 Table get；同一 snapshot 内对同一 View
的多个字段访问共享该读取，不能为每个字段创建独立端口。

Queue-facing read：

```python
entries = rob.view(
    lambda request: request.rob_index
).read(requests)
```

默认类型为 `Queue[RobEntry]`。可在同一 firing 中映射成其他固定类型：

```python
responses = rob.view(
    lambda request: request.rob_index
).read(
    requests,
    result=lambda entry, request:
        RobResponse(
            tag=request.tag,
            valid=entry.valid,
            value=entry.result,
        ),
)
```

无输入 Queue 的 state-driven read：

```python
head_entry = rob.view(head)
can_retire = head_entry.valid & head_entry.done
retired = head_entry.read(when=can_retire)
head_entry.patch(valid=False)
head += 1
```

`read()` 建立零输入 firing 并返回 `Queue[RobEntry]`，`when` 成为整个 firing 的 guard。
它不是无副作用的本地观察；本地观察始终使用 `EntryView` 字段或 Entry value coercion。
后续无参数 state effect 只在同一基本块已经存在唯一当前 firing 时合法。

一个 Queue-driven anchor 之后可以省略后续 state effect 的 Queue 参数；省略不创建或
消费第二个 token。多个显式 Queue-facing 方法即使使用同一个 Queue，也不会自动合并成
同一 firing；需要共享一次消费时应改写成一个自包含方法或使用 `@ac.rule`。

在 `@ac.rule` 内禁止 `read()`。Rule 中的本地读取仍使用 View，输出 Queue 只由 rule 的
`return` 创建：

```python
@ac.rule
def lookup(request, credit):
    entry = table.view(request.index)
    return Response(value=entry, credit=credit)
```

`read(queue)` 的输出 Queue 不能接受 token 时，整个 firing 阻塞，输入不消费，Table/owner
状态不更新。无参数 `read()` 的输出 Queue 不能接受 token 时，其零输入 firing 同样阻塞，
Table/owner 状态不更新。

对应 Frozen Table 观察：

```mlir
%entry = ac.table.get @entries[%index]
    : (!ac.var<i7>)
      -> !ac.var<!ac.struct<@types::@RobEntry>>
```

多维：

```mlir
%line = ac.table.get @tags[%set, %way]
    : (!ac.var<i6>, !ac.var<i3>)
      -> !ac.var<!ac.struct<@types::@TagEntry>>
```

语义：

```text
读取当前 committed state
组合结果
无状态修改
无 latency
```

每个未被 snapshot CSE 合并的 `ac.table.get` 表示一个独立逻辑组合读端口。

RTL：

```verilog
assign entry = entries[index];
```

## 10. Write

Queue-facing 前端：

```python
rob.view(lambda request: request.index).write(
    requests,
    value=lambda request: new_entry(request),
)
```

`write` 不产生输出。若协议需要完成确认，应使用 `@ac.rule` 返回显式 ack payload，不能把
Table write 的局部成功状态伪装成独立结果。

已经由前序操作建立唯一 firing 时，可以省略 Queue 参数：

```python
responses = rob.view(index).read(requests)
rob.view(index).write(value=new_entry)
```

无参数 `write()` 不独立创建 autonomous firing。

Frozen ACIR 中每个 write 都具有显式 enable：

```mlir
ac.table.write @entries[%index], %new_entry
    enable %write_enable
    : !ac.var<i7>,
      !ac.var<!ac.struct<@types::@RobEntry>>,
      !ac.var<i1>
```

最终有效 write enable 为：

```text
firing_fire && local_write_enable
```

RTL：

```verilog
always_ff @(posedge clk) begin
    if (fire && write_enable)
        entries[index] <= new_entry;
end
```

每个 Frozen `ac.table.write` 都表示一个同周期逻辑写 proposal。后端不得将多个 write 隐式串行化到多个 tick。

## 11. Patch

Queue-facing 前端：

```python
rob.view(lambda completion: completion.rob_index).patch(
    completions,
    done=True,
    result=lambda completion: completion.result,
)
```

字段值可以是常量、输入 token 的函数，或同时依赖旧 Entry 与输入 token 的函数：

```python
counters.view(lambda request: request.index).patch(
    requests,
    count=lambda entry, request:
        entry.count + request.increment,
)
```

`patch` 不产生输出。若协议需要 ack，应使用 `@ac.rule` 明确构造输出 payload。索引越界、
写冲突、字段或类型错误是编译诊断或运行时 assertion，不是业务返回值。

已存在唯一 firing anchor 时，可以省略 Queue 参数：

```python
retired = head_entry.read()
head_entry.patch(valid=False)
```

无参数 `patch()` 继承当前 firing 的 fire/guard，不独立消费 Queue，也不独立建立 firing。

Patch 不进入 Frozen ACIR。

Canonicalization：

```mlir
%old = ac.table.get @entries[%index]

%v0 = ac.var.with %old, %true field "done"
%new = ac.var.with %v0, %result field "result"

ac.table.write @entries[%index], %new
    enable %local_enable
```

对应 RTL：

```text
Entry read mux
→ immutable field replacement logic
→ full Entry next value
→ write decoder / write enable
```

### 11.1 前端返回类型汇总

| 前端表达式 | 结果 |
| --- | --- |
| `table.view(full_index)` | `EntryView<Entry>` |
| `table.view(prefix_or_mask)` | `TableView<Entry, domain>` |
| `entry.field` 或 Entry value coercion | firing-local `Var` |
| `entry.read(queue)` | `Queue[Entry]` |
| `entry.read(queue, result=...)` | `Queue[Result]` |
| `entry.read(queue, result=..., when=...)` | 带 guard 的 `Queue[Result]` |
| `entry.read(when=...)` | 建立零输入 firing，返回 `Queue[Entry]` |
| `entry.read(result=..., when=...)` | 建立零输入 firing，返回 `Queue[Result]` |
| `entry.write/patch(queue, ...)` | 无返回 |
| 当前隐式 firing 或 `@ac.rule` 中的 `entry.write/patch(...)` | 无返回的 state effect |
| `table_view.match(...)` | `Mask<domain>` |
| `table_view.choose(mask, ...)` | 固定数量的 `Selection[index, valid]` |
| `@ac.rule` return value | 一个或多个输出 Queue |

这里的 Queue 返回均表示前端 endpoint，不会成为新的 Frozen Table primitive。

## 12. Match 与前端 Mask

前端：

```python
ready = issue.match(
    lambda entry:
        entry.valid
        & entry.src0_ready
        & entry.src1_ready
)
```

前端逻辑类型：

```text
Mask<issue-domain>
```

`Mask<domain>`：

- 只存在于前端；
- 不是 runtime handle；
- 记录宽度和来源 domain；
- 不允许与不同 domain 的 Mask 混用；
- 在 Frozen ACIR 中降为 `!ac.var<iN>`。

对于大小为 N 的一维 domain：

```text
mask bit i ↔ local Entry index i
bit 0 为最低有效位
```

## 13. Table Match

Frozen ACIR：

```mlir
%ready = ac.table.match @issue[*] {
  ^bb0(%entry : !ac.var<!ac.struct<@types::@IssueEntry>>):
    %valid = ac.var.get %entry field "valid"
    %r0 = ac.var.get %entry field "src0_ready"
    %r1 = ac.var.get %entry field "src1_ready"
    %x = ac.var.and %valid, %r0
    %result = ac.var.and %x, %r1
    ac.table.match.yield %result
} : !ac.var<i64>
```

本质：

```text
N 个 committed Entry
→ N 份 predicate logic
→ N-bit candidate mask
```

RTL：

```verilog
for (genvar i = 0; i < 64; ++i) begin
    assign ready[i] =
        issue[i].valid
        && issue[i].src0_ready
        && issue[i].src1_ready;
end
```

## 14. Match Captures

Match predicate 可以捕获 owner 当前 firing 中的 immutable `!ac.var<T>`。

前端：

```python
hits = set_view.match(
    lambda entry:
        entry.valid
        & (entry.tag == request_tag)
)
```

Frozen：

```mlir
%hits = ac.table.match @tags[%set_index, *]
    captures(%request_tag) {
  ^bb0(
      %entry : !ac.var<!ac.struct<@types::@TagEntry>>,
      %tag   : !ac.var<i32>
  ):
    ...
    ac.table.match.yield %hit
} : !ac.var<i8>
```

Region 内只允许 immutable field extraction、比较、布尔运算、位运算、简单算术和显式 captures。

禁止嵌套 Table 操作、Queue effect、Module request、状态修改或 Python side effect。

## 15. 多维 Match

第一版规定：

> `match/choose` 只能作用于恰好剩余一维的 Table/View domain。

合法：

```python
issue.match(...)
tags.view(set_idx).match(...)
```

不合法：

```python
tags.match(...)
three_d.view(x).match(...)
```

例如：

```mlir
%hits = ac.table.match @tags[%set_idx, *] ...
    : !ac.var<i8>
```

映射：

```text
hits[0] ↔ tags[set_idx, 0]
...
hits[7] ↔ tags[set_idx, 7]
```

## 16. Table Choose

`choose` 是 Table/View 专属操作。

前端：

```python
grant = issue.choose(
    ready,
    count=2,
    policy="min",
    key=lambda entry: entry.seq,
)
```

Frozen：

```mlir
%idx0, %valid0,
%idx1, %valid1 =
    ac.table.choose @issue[*], %ready
        count 2
        policy "min" {
      ^bb0(%entry : !ac.var<!ac.struct<@types::@IssueEntry>>):
        %seq = ac.var.get %entry field "seq"
        ac.table.choose.yield %seq
    }
```

`first` 不需要 key region：

```mlir
%way, %valid =
    ac.table.choose @tags[%set_idx, *], %hits
        count 1
        policy "first"
```

第一版 policy：

```text
first
min
max
```

确定性规则：

- `first`：最低 local index 优先；
- `min`：最小 key 优先，平局时最低 index 优先；
- `max`：最大 key 优先，平局时最低 index 优先；
- `count=K`：返回排序后的前 K 个不同候选；
- 候选不足时，剩余结果为 `index=0, valid=false`。

`count` 必须是正的静态常量，并且不超过 domain 大小。

## 17. Selection

前端 Selection 显式包含 `index` 和 `valid`。

```python
selected = grant[0]
entry = issue.view(selected.index)

issued = entry.read(when=selected.valid)
entry.patch(valid=False)
```

这里 Selection 来自 committed Table state。无参数 `read()` 建立零输入 state-driven
firing，`when=selected.valid` 成为 guard；patch 自动与输出一起提交。

Selection 不能隐式转换成 index，避免将 `index=0, valid=false` 误认为有效的 Entry 0。

对于大小 N 的 domain：

```text
mask width  = N
index width = max(1, ceil(log2(N)))
```

例如：

```text
ac.table[1]   → mask i1,   index i1
ac.table[8]   → mask i8,   index i3
ac.table[48]  → mask i48,  index i6
ac.table[128] → mask i128, index i7
```

## 18. Masked Patch

前端统一使用 `view`：

```python
hits = issue.match(
    writebacks,
    lambda entry, writeback:
        entry.valid
        & (entry.src0 == writeback.physical),
)

issue.view(hits).patch(
    src0_ready=True,
)
```

带 Queue 参数的 `match` 建立单输入 firing anchor，并产生与该 token 绑定的 firing-local
Mask。Queue 只在整个 firing 提交时消费一次；后续无参数 `patch` 或 `write` 附着到该
firing。若需要同时产生输出 Queue，复杂组合应改用 `@ac.rule`，不能再追加第二个 `read`
anchor。

因为 `hits` 的前端类型是 `Mask<issue-domain>`：

```text
view(integer) → 单 Entry View
view(Mask)    → masked View
```

Canonicalization：

```text
for every static local index i:
    old_i = table.get(i)
    new_i = old_i.with_fields(src0_ready=True)
    table.write(i, new_i, enable=hits[i])
```

RTL：

```verilog
for (int i = 0; i < 64; ++i) begin
    if (fire && hits[i])
        issue[i].src0_ready <= 1'b1;
end
```

这些更新保持同周期并行，不能转换成多周期循环。

## 19. Write Conflict

同一个 commit edge 对同一个 Entry 最多允许一个有效写 proposal。

非法情况：

```text
write0.enable
&& write1.enable
&& write0.index == write1.index
```

第一版不提供自动仲裁、last-write-wins、隐式优先级或自动字段合并。

设计者负责保证合法代码不发生动态冲突。

实现要求：

- 明显静态冲突可以编译时报错；
- GFSim 应动态检测并报告 `table_write_conflict`；
- PYC/RTL 应生成对应 assertion；
- assertion 触发后的 Table next state 不属于合法设计语义。

```verilog
assert property (
    !(we0 && we1 && addr0 == addr1)
);
```

## 20. Index 越界

规则：

- 静态常量越界：编译错误；
- 动态 get/write/view 越界：`table_index_out_of_range`；
- 多维 Table 逐维检查；
- 不允许取模、截断或饱和。

```verilog
assert property (!read_enable || read_index < ENTRY_COUNT);
assert property (!write_enable || write_index < ENTRY_COUNT);
```

`choose` 无候选时返回：

```text
index = 0
valid = false
```

## 21. Initialization 与 Reset

每个 Table 必须有确定的静态初始 image。

前端允许常量初始化或静态索引初始化函数。初始化 lambda 仅在 elaboration 执行，不生成运行时初始化逻辑。默认 `init` 可以是全零。

Table 使用同步 reset：

```text
reset asserted:
    禁止 owner 的普通 firing
    丢弃未提交 write proposal

commit edge:
    committed state 恢复到初始 image

next tick:
    get/match/choose 观察初始 image
```

```verilog
always_ff @(posedge clk) begin
    if (reset)
        table <= INITIAL_IMAGE;
    else
        commit_writes();
end
```

Provider 可以优化无可观察影响的 data bit reset，但行为必须与完整初始 image 等价。

## 22. ACIR Effect 模型

Table 观察不是全局 Pure operation。

```text
ac.table.get
    = Read effect on referenced Table

ac.table.match
    = Read effect on referenced Table

ac.table.choose
    = Read effect when key region observes Entry

ac.table.write
    = Write proposal effect on referenced Table
```

同一个 firing snapshot 内，相同地址的 get 可以合并。禁止跨 firing 或 commit barrier
CSE，也禁止将 get 移出状态循环。

## 23. Frozen ACIR Primitive 集

第一版新增：

```text
ac.table
ac.table.get
ac.table.write
ac.table.match
ac.table.match.yield
ac.table.choose
ac.table.choose.yield
```

继续复用或作为配套通用 primitive 假设：

```text
!ac.var<T>
ac.var.constant
ac.var.get
ac.var.with
ac.var.*
ac.reg
ac.reg.get
ac.reg.write
ac.firing
ac.firing.yield
scf.*
```

不增加：

```text
ac.view
!ac.mask<N>
ac.table.patch
ac.table.delete
ac.table.allocate
ac.table.read_mask
ac.table.write_mask
```

View、Mask、Patch 以及 Queue-facing Table method wrapper 均在 Frozen ACIR 前消失。
这里不增加 `ac.table.allocate` Frozen primitive；前端 `.allocate(...)` 降为
`ac.table.write mode "replace"`。
`ac.firing` 的 Queue operands/results、region token 参数和 `ac.firing.yield` 显式表达
输入消费、输出产生、guard 和共享 commit 边界，不需要在 Python 或 Frozen region 中
逐条书写 `peek/pop/push`。

## 24. Primitive 到 RTL 的映射

| Primitive | RTL footprint |
| --- | --- |
| `ac.table` | Register/state array |
| `ac.table.get` | Address bounds check + mux |
| `ac.table.write` | Write enable + decoder + next-state input |
| `ac.reg` / `get` / `write` | Scalar register + committed output + next-state input |
| `ac.table.match` | N-way predicate/comparator bank |
| `ac.table.choose first` | Priority encoder |
| `ac.table.choose min/max` | Comparator/tournament network |
| `count=K choose` | K-stage winner selection 或等价并行网络 |
| masked Patch lowering | Per-entry write enable network |
| synchronous reset | Initial-image restore logic |

每个 get 都是独立组合读端口。每个 write 都是同周期逻辑写 proposal。

Provider 不得给 Frozen Table access 增加访问 latency 或独立 ready-valid，不得将端口
时分复用或将并行 masked Patch 串行执行。前端 Queue wrapper 的 ready-valid 只控制
`ac.firing` 是否提交，不改变 `ac.table.get/write` 的组合/时序语义。

以下组件示例中的 ACIR 是目标 Frozen 形态，用于冻结数据流和 effect 语义；具体 assembly
format 可以在 ODS 实现时做等价调整。示例使用以下通用约定：

```text
ac.firing inputs(...) -> output Queues
    region 参数 = firing-local input token Var
    ac.firing.yield guard G outputs(...)

ac.firing () -> output Queues
    = 无输入 Queue 的 state-driven firing，每 tick 最多 firing 一次

fire = all_input_valid && all_output_ready && G

ac.table.get
    = committed snapshot 的组合观察

ac.table.write enable E
    = fire && E 时提交的 write proposal

ac.reg.get / ac.reg.write
    = committed scalar observation / 与 firing 一起提交的 next-state proposal
```

`ac.var.create/select/and/or/not/extract` 表示需要由通用 Var 层提供的固定宽度组合逻辑，
不是新的 Table primitive。

## 25. ROB 示例

### 25.1 前端

```python
@ac.struct
class RobEntry:
    valid: bool
    done: bool
    sequence: ac.u16
    destination: ac.u8
    result: ac.u64

rob = ac.table[128, RobEntry](
    init=RobEntry(
        valid=False,
        done=False,
        sequence=0,
        destination=0,
        result=0,
    )
)

head = ac.reg[ac.u7](init=0)
tail = ac.reg[ac.u7](init=0)

rob.view(tail).write(
    dispatch,
    value=lambda request:
        RobEntry(
            valid=True,
            done=False,
            sequence=request.sequence,
            destination=request.destination,
            result=0,
        ),
)
tail += 1

rob.view(
    lambda completion: completion.rob_index
).patch(
    completions,
    done=True,
    result=lambda completion: completion.result,
)

head_entry = rob.view(head)
can_retire = head_entry.valid & head_entry.done

retired = head_entry.read(when=can_retire)
head_entry.patch(valid=False)
head += 1
```

前端没有显式 Queue push/pop。`rob.view(tail).write(dispatch, ...)` 建立 dispatch firing，紧随其后的
`tail += 1` 自动附着到同组，所以只有 ROB allocation 成功时 tail 才增加。

`head_entry.read(when=can_retire)` 建立零输入、单输出的 retirement firing；ROB patch 和
`head += 1` 自动附着到同组，`when` 成为 guard。若 `retired` 输出 Queue 反压，三个效果
全部不提交。这里直接输出完整 `RobEntry`；如接口只需要部分字段，可以在下游转换成
`CommitResult`，或使用 `read(result=...)` 显式构造。head/tail 使用通用 `ac.reg`，不再
伪装成单 Entry Table。

### 25.2 预计完整 Frozen ACIR

```mlir
ac.table @rob
    shape [128]
    element !ac.struct<@types::@RobEntry>
    init #ac.table_init<zero>

ac.reg @head type i7 init 0
ac.reg @tail type i7 init 0

// dispatch: one input, no output, two state proposals.
ac.firing inputs(%dispatch) {
^body(%request : !ac.var<!ac.struct<@types::@DispatchRequest>>):
  %zero_i1 = ac.var.constant false as !ac.var<i1>
  %true_i1 = ac.var.constant true as !ac.var<i1>
  %zero_i64 = ac.var.constant 0 : i64 as !ac.var<i64>
  %one_i7 = ac.var.constant 1 : i7 as !ac.var<i7>

  %tail_index = ac.reg.get @tail : !ac.var<i7>
  %sequence = ac.var.get %request field "sequence"
  %destination = ac.var.get %request field "destination"
  %new_entry = ac.var.create !ac.struct<@types::@RobEntry> {
      valid = %true_i1,
      done = %zero_i1,
      sequence = %sequence,
      destination = %destination,
      result = %zero_i64
  }
  ac.table.write @rob[%tail_index], %new_entry enable %true_i1

  %next_tail = ac.var.add %tail_index, %one_i7 : !ac.var<i7>
  ac.reg.write @tail, %next_tail enable %true_i1
  ac.firing.yield guard %true_i1
} : (!ac.queue<!ac.struct<@types::@DispatchRequest>>) -> ()

// completion: one input, no output, one read-modify-write proposal.
ac.firing inputs(%completions) {
^body(%completion : !ac.var<!ac.struct<@types::@Completion>>):
  %true_i1 = ac.var.constant true as !ac.var<i1>
  %rob_index = ac.var.get %completion field "rob_index"
  %result = ac.var.get %completion field "result"
  %old = ac.table.get @rob[%rob_index]
  %done = ac.var.with %old, %true_i1 field "done"
  %updated = ac.var.with %done, %result field "result"
  ac.table.write @rob[%rob_index], %updated enable %true_i1
  ac.firing.yield guard %true_i1
} : (!ac.queue<!ac.struct<@types::@Completion>>) -> ()

// retirement: zero input; output backpressure, ROB clear and head increment
// share one state-driven fire.
%retired = ac.firing inputs() depths [1] latencies [1] {
^body:
  %true_i1 = ac.var.constant true as !ac.var<i1>
  %false_i1 = ac.var.constant false as !ac.var<i1>
  %one_i7 = ac.var.constant 1 : i7 as !ac.var<i7>

  %head_index = ac.reg.get @head : !ac.var<i7>
  %entry = ac.table.get @rob[%head_index]
  %valid = ac.var.get %entry field "valid"
  %done = ac.var.get %entry field "done"
  %can_retire = ac.var.and %valid, %done

  %cleared = ac.var.with %entry, %false_i1 field "valid"
  ac.table.write @rob[%head_index], %cleared enable %true_i1
  %next_head = ac.var.add %head_index, %one_i7 : !ac.var<i7>
  ac.reg.write @head, %next_head enable %true_i1

  ac.firing.yield guard %can_retire outputs(%entry)
} : () -> !ac.queue<!ac.struct<@types::@RobEntry>>
```

retirement 没有输入 Queue token。它在 `%can_retire` 为真且 `%retired` 输出可接受时每
tick 最多 firing 一次。三个 firing 都读取同一个 tick 的 committed snapshot；它们的
有效 Table/Reg write proposal 在同一个 edge 提交。普通 Table field writer 若静态写字段
集合相交则属于非法设计；字段不相交时，即使动态命中同一 Entry 也按字段合并。每张 Table
允许一个完整 replace allocation writer 与它们重叠：commit 先合并 field，再应用 replace，
所以同槽 remove+allocate 得到新 Entry。第一版不根据地址、mask、enable 或 predicate 证明
同字段 writer 互斥。

### 25.3 RTL

```text
128-entry state array
+ tail allocation write path
+ completion write path
+ head retirement write path
+ head combinational read mux
+ head/tail scalar registers
+ dispatch/completion input Queue firing control
+ state-driven retired output valid/ready control
+ write collision assertions
```

## 26. Issue Table 示例

### 26.1 前端

```python
@ac.struct
class IssueEntry:
    valid: bool
    src0_ready: bool
    src1_ready: bool
    src0: ac.u8
    src1: ac.u8
    sequence: ac.u16
    opcode: ac.u8

issue = ac.table[64, IssueEntry](init=IssueEntry(...))

ready = issue.match(
    lambda entry:
        entry.valid
        & entry.src0_ready
        & entry.src1_ready
)

grant = issue.choose(
    ready,
    count=2,
    policy="min",
    key=lambda entry: entry.sequence,
)

slot0 = grant[0]
slot1 = grant[1]
entry0 = issue.view(slot0.index)
entry1 = issue.view(slot1.index)

issued = entry0.read(
    when=slot0.valid,
    result=lambda first:
        IssueBundle(
            valid0=slot0.valid,
            entry0=first,
            valid1=slot1.valid,
            entry1=entry1,
        ),
)
entry0.patch(valid=False)
entry1.patch(
    valid=False,
    enable=slot1.valid,
)
```

`entry1` 在构造 `IssueBundle` 时按 Entry value materialize。`issued` 是唯一业务输出
Queue；`read(when=slot0.valid)` 建立零输入的 state-driven issue firing，两个无参数 patch
自动附着到同组。输出反压时不会清除任何 Issue Entry。

### 26.2 预计完整 Frozen ACIR

```mlir
ac.table @issue
    shape [64]
    element !ac.struct<@types::@IssueEntry>
    init #ac.table_init<zero>

%ready = ac.table.match @issue[*] {
^predicate(%entry : !ac.var<!ac.struct<@types::@IssueEntry>>):
  %valid = ac.var.get %entry field "valid"
  %src0_ready = ac.var.get %entry field "src0_ready"
  %src1_ready = ac.var.get %entry field "src1_ready"
  %both0 = ac.var.and %valid, %src0_ready
  %both1 = ac.var.and %both0, %src1_ready
  ac.table.match.yield %both1
} : !ac.var<i64>

%idx0, %valid0, %idx1, %valid1 =
    ac.table.choose @issue[*], %ready
        count 2
        policy "min"
        order "unsigned" {
^key(%entry : !ac.var<!ac.struct<@types::@IssueEntry>>):
  %sequence = ac.var.get %entry field "sequence"
  ac.table.choose.yield %sequence
    }

%issued = ac.firing inputs() depths [1] latencies [1] {
^body:
  %true_i1 = ac.var.constant true as !ac.var<i1>
  %false_i1 = ac.var.constant false as !ac.var<i1>

  // choose 在无候选时提供安全 index=0；%valid0 是整个 firing guard。
  %inst0 = ac.table.get @issue[%idx0]
  %inst1 = ac.table.get @issue[%idx1]
  %bundle = ac.var.create !ac.struct<@types::@IssueBundle> {
      valid0 = %valid0,
      entry0 = %inst0,
      valid1 = %valid1,
      entry1 = %inst1
  }

  %cleared0 = ac.var.with %inst0, %false_i1 field "valid"
  %cleared1 = ac.var.with %inst1, %false_i1 field "valid"
  ac.table.write @issue[%idx0], %cleared0 enable %true_i1
  ac.table.write @issue[%idx1], %cleared1 enable %valid1

  ac.firing.yield guard %valid0 outputs(%bundle)
} : () -> !ac.queue<!ac.struct<@types::@IssueBundle>>
```

### 26.3 RTL

```text
64 Entry registers
→ 64 readiness predicates
→ 64-bit candidate mask
→ 2-wide min-key selector
→ idx0/valid0 + idx1/valid1
→ 2 个 Entry read mux
→ 2 个清除 write path
→ one state-driven output firing handshake
```

## 27. Scoreboard / Wakeup 示例

### 27.1 前端

```python
@ac.struct
class ScoreEntry:
    allocated: bool
    ready: bool
    producer: ac.u8

scoreboard = ac.table[128, ScoreEntry](
    init=ScoreEntry(
        allocated=False,
        ready=False,
        producer=0,
    )
)

waiting = scoreboard.match(
    writebacks,
    lambda entry, writeback:
        entry.allocated
        & (entry.producer == writeback.producer),
)

scoreboard.view(waiting).patch(
    ready=True,
)
```

带 `writebacks` 参数的 `match` 绑定 Queue-driven firing-local token；后续 masked patch
省略 Queue 参数并附着到该 firing。每个 writeback token 仍只消费一次。

### 27.2 预计完整 Frozen ACIR

```mlir
ac.table @scoreboard
    shape [128]
    element !ac.struct<@types::@ScoreEntry>
    init #ac.table_init<zero>

ac.firing inputs(%writebacks) {
^body(%writeback : !ac.var<!ac.struct<@types::@Writeback>>):
  %true_i1 = ac.var.constant true as !ac.var<i1>
  %completed_producer = ac.var.get %writeback field "producer"

  %waiting = ac.table.match @scoreboard[*]
      captures(%completed_producer) {
  ^predicate(
      %entry : !ac.var<!ac.struct<@types::@ScoreEntry>>,
      %producer : !ac.var<i8>
  ):
    %allocated = ac.var.get %entry field "allocated"
    %entry_producer = ac.var.get %entry field "producer"
    %same = ac.var.cmp "eq" %entry_producer, %producer
        : !ac.var<i8> -> !ac.var<i1>
    %hit = ac.var.and %allocated, %same
    ac.table.match.yield %hit
  } : !ac.var<i128>

  // Static trip count; canonicalization unrolls this before structural RTL.
  scf.for %i = 0 to 128 step 1 {
    %index = ac.var.from_index %i : index -> !ac.var<i7>
    %enabled = ac.var.extract %waiting[%i]
        : !ac.var<i128> -> !ac.var<i1>
    %old = ac.table.get @scoreboard[%index]
    %updated = ac.var.with %old, %true_i1 field "ready"
    ac.table.write @scoreboard[%index], %updated enable %enabled
  }

  ac.firing.yield guard %true_i1
} : (!ac.queue<!ac.struct<@types::@Writeback>>) -> ()
```

### 27.3 RTL

```text
writeback Queue head
→ 128 producer comparators
→ 128-bit wakeup mask
→ 128 个 ready-bit write enable
```

## 28. Rename Map 示例

### 28.1 前端

```python
rename_map = ac.table[64, ac.u8](
    init=lambda architectural: architectural
)

@ac.rule
def rename(inst, new_physical):
    src0_physical = rename_map.view(inst.src0_arch)
    src1_physical = rename_map.view(inst.src1_arch)
    old_physical = rename_map.view(inst.destination_arch)

    rename_map.view(inst.destination_arch).write(value=new_physical)

    return (
        RenamedInstruction(
            sequence=inst.sequence,
            instruction=inst.instruction,
            src0_physical=src0_physical,
            src1_physical=src1_physical,
            destination_arch=inst.destination_arch,
            destination_physical=new_physical,
        ),
        RenameRecord(
            sequence=inst.sequence,
            destination_arch=inst.destination_arch,
            old_physical=old_physical,
            new_physical=new_physical,
        ),
    )

renamed, record = rename(decoded_with_dst, free_phys)
```

该 rule 同时消费 decoded instruction 与一个 free physical register，产生 issue 与 ROB
两个输出，并更新 rename map。完整 `EntryView` 在需要 Entry value 的位置自动 materialize
为 committed mapping Var，不创建 Queue。`decoded_with_dst` 只允许携带具有 destination 的
指令；无 destination 指令必须在上游 route 到不消费 `free_phys` 的路径。

### 28.2 预计完整 Frozen ACIR

```mlir
ac.table @rename_map
    shape [64]
    element i8
    init #ac.table_init<identity>

%renamed, %record = ac.firing
    inputs(%decoded_with_dst, %free_phys)
    depths [1, 1]
    latencies [1, 1] {
^body(
    %inst : !ac.var<!ac.struct<@types::@DecodedInstruction>>,
    %new_physical : !ac.var<i8>
):
  %true_i1 = ac.var.constant true as !ac.var<i1>
  %sequence = ac.var.get %inst field "sequence"
  %instruction = ac.var.get %inst field "instruction"
  %src0_arch = ac.var.get %inst field "src0_arch"
  %src1_arch = ac.var.get %inst field "src1_arch"
  %destination_arch = ac.var.get %inst field "destination_arch"

  %src0_physical = ac.table.get @rename_map[%src0_arch]
  %src1_physical = ac.table.get @rename_map[%src1_arch]
  %old_physical = ac.table.get @rename_map[%destination_arch]

  %renamed_instruction = ac.var.create
      !ac.struct<@types::@RenamedInstruction> {
    sequence = %sequence,
    instruction = %instruction,
    src0_physical = %src0_physical,
    src1_physical = %src1_physical,
    destination_arch = %destination_arch,
    destination_physical = %new_physical
  }

  %rename_record = ac.var.create
      !ac.struct<@types::@RenameRecord> {
    sequence = %sequence,
    destination_arch = %destination_arch,
    old_physical = %old_physical,
    new_physical = %new_physical
  }

  ac.table.write @rename_map[%destination_arch], %new_physical
      enable %true_i1
  ac.firing.yield guard %true_i1
      outputs(%renamed_instruction, %rename_record)
} : (!ac.queue<!ac.struct<@types::@DecodedInstruction>>, !ac.queue<i8>)
    -> (!ac.queue<!ac.struct<@types::@RenamedInstruction>>,
        !ac.queue<!ac.struct<@types::@RenameRecord>>)
```

### 28.3 RTL

```text
64 × 8-bit register array
+ 3 个组合 read mux
+ 1 个 write decoder
+ two-input/two-output rename firing control
+ identity reset image
```

## 29. Free List 示例

### 29.1 前端

```python
@ac.struct
class FreeEntry:
    free: bool

free_list = ac.table[128, FreeEntry](
    init=lambda index:
        FreeEntry(free=index >= RESERVED_REGS)
)

available = free_list.match(lambda entry: entry.free)
allocation = free_list.choose(
    available,
    count=1,
    policy="first",
)[0]

selected = free_list.view(allocation.index)
allocated = selected.read(
    when=allocation.valid,
    result=lambda entry:
        FreeAllocation(physical=allocation.index),
)
selected.patch(free=False)

free_list.view(
    lambda release: release.physical
).patch(
    releases,
    free=True,
)
```

allocation 是零输入的 state-driven firing：只要存在 free Entry 且 `allocated` 输出可接受，
每 tick 最多分配一个 physical register。多发射版本需要未来的 batch 输出语义；第一版
单 token/tick 不通过简单增加 `count` 暗示一次产生多个 Queue token。

### 29.2 预计完整 Frozen ACIR

```mlir
ac.table @free_list
    shape [128]
    element !ac.struct<@types::@FreeEntry>
    init #ac.table_init<index_function>

%available = ac.table.match @free_list[*] {
^predicate(%entry : !ac.var<!ac.struct<@types::@FreeEntry>>):
  %free = ac.var.get %entry field "free"
  ac.table.match.yield %free
} : !ac.var<i128>

%physical, %allocation_valid =
    ac.table.choose @free_list[*], %available
        count 1 policy "first"

%allocated = ac.firing inputs() depths [1] latencies [1] {
^body:
  %true_i1 = ac.var.constant true as !ac.var<i1>
  %false_i1 = ac.var.constant false as !ac.var<i1>
  %old = ac.table.get @free_list[%physical]
  %cleared = ac.var.with %old, %false_i1 field "free"
  %response = ac.var.create !ac.struct<@types::@FreeAllocation> {
      physical = %physical
  }
  ac.table.write @free_list[%physical], %cleared enable %true_i1
  ac.firing.yield guard %allocation_valid outputs(%response)
} : () -> !ac.queue<!ac.struct<@types::@FreeAllocation>>

ac.firing inputs(%releases) {
^body(%release : !ac.var<!ac.struct<@types::@Release>>):
  %true_i1 = ac.var.constant true as !ac.var<i1>
  %physical = ac.var.get %release field "physical"
  %old = ac.table.get @free_list[%physical]
  %released = ac.var.with %old, %true_i1 field "free"
  ac.table.write @free_list[%physical], %released enable %true_i1
  ac.firing.yield guard %true_i1
} : (!ac.queue<!ac.struct<@types::@Release>>) -> ()
```

### 29.3 RTL

```text
free bitmap/register entries
→ priority encoder
→ 1 个 physical index
→ 1 个 state-driven allocation clear enable
+ release set enable
+ allocation output 与 release input Queue firing control
+ collision assertion
```

## 30. MSHR 示例

### 30.1 前端

```python
@ac.struct
class MshrEntry:
    valid: bool
    line_address: ac.u48
    transaction: ac.u16
    pending_mask: ac.u8

mshr = ac.table[16, MshrEntry](init=MshrEntry(...))

@ac.rule
def handle_miss(request):
    matches = mshr.match(
        lambda entry:
            entry.valid
            & (entry.line_address == request.line_address)
    )
    hit = mshr.choose(matches, count=1, policy="first")[0]

    free_mask = mshr.match(lambda entry: ~entry.valid)
    free_slot = mshr.choose(
        free_mask,
        count=1,
        policy="first",
    )[0]

    selected_index = ac.mux(
        hit.valid,
        hit.index,
        free_slot.index,
    )

    mshr.view(hit.index).patch(
        pending_mask=lambda entry:
            entry.pending_mask | request.source_mask,
        enable=hit.valid,
    )

    mshr.view(free_slot.index).write(
        value=MshrEntry(
            valid=True,
            line_address=request.line_address,
            transaction=request.transaction,
            pending_mask=request.source_mask,
        ),
        enable=(~hit.valid) & free_slot.valid,
    )

    return MshrResponse(
        accepted=hit.valid | free_slot.valid,
        merged=hit.valid,
        index=selected_index,
    )

response = handle_miss(miss_requests)
```

这里虽然只有一个输入和一个输出，但一次事务中包含两个 match、两个 choose、两个互斥
state effect 和一个 response，因此使用 `@ac.rule` 明确 scope，比依赖多个隐式 anchor
归组更清晰。Rule 参数 `request` 是 firing-local payload Var，返回值形成 response Queue。

如果既未命中又无空闲 Entry，本例仍消费请求并返回 `accepted=false`，且不产生 Table
write。若组件需要保持反压，应给 `@ac.rule` 提供纯组合 `when` callback，使
`hit.valid | free_slot.valid` 成为 firing guard；不能用 write local enable 代替 guard。

### 30.2 预计完整 Frozen ACIR

```mlir
ac.table @mshr
    shape [16]
    element !ac.struct<@types::@MshrEntry>
    init #ac.table_init<zero>

%response = ac.firing inputs(%miss_requests) depths [1] latencies [1] {
^body(%request : !ac.var<!ac.struct<@types::@MissRequest>>):
  %true_i1 = ac.var.constant true as !ac.var<i1>
  %false_i1 = ac.var.constant false as !ac.var<i1>
  %line_address = ac.var.get %request field "line_address"
  %transaction = ac.var.get %request field "transaction"
  %source_mask = ac.var.get %request field "source_mask"

  %matches = ac.table.match @mshr[*] captures(%line_address) {
  ^predicate(
      %entry : !ac.var<!ac.struct<@types::@MshrEntry>>,
      %line : !ac.var<i48>
  ):
    %valid = ac.var.get %entry field "valid"
    %entry_line = ac.var.get %entry field "line_address"
    %same_line = ac.var.cmp "eq" %entry_line, %line
        : !ac.var<i48> -> !ac.var<i1>
    %match = ac.var.and %valid, %same_line
    ac.table.match.yield %match
  } : !ac.var<i16>
  %hit_index, %hit_valid =
      ac.table.choose @mshr[*], %matches count 1 policy "first"

  %free_mask = ac.table.match @mshr[*] {
  ^predicate(%entry : !ac.var<!ac.struct<@types::@MshrEntry>>):
    %valid = ac.var.get %entry field "valid"
    %free = ac.var.not %valid
    ac.table.match.yield %free
  } : !ac.var<i16>
  %free_index, %free_valid =
      ac.table.choose @mshr[*], %free_mask count 1 policy "first"

  %not_hit = ac.var.not %hit_valid
  %allocate = ac.var.and %not_hit, %free_valid
  %accepted = ac.var.or %hit_valid, %free_valid
  %selected_index = ac.var.select %hit_valid, %hit_index, %free_index
      : !ac.var<i4>

  %response_value = ac.var.create !ac.struct<@types::@MshrResponse> {
      accepted = %accepted,
      merged = %hit_valid,
      index = %selected_index
  }

  // Safe index=0 is observed when valid=false; enables prevent an invalid write.
  %hit_entry = ac.table.get @mshr[%hit_index]
  %old_pending = ac.var.get %hit_entry field "pending_mask"
  %merged_pending = ac.var.or %old_pending, %source_mask
  %merged_entry = ac.var.with %hit_entry, %merged_pending field "pending_mask"
  ac.table.write @mshr[%hit_index], %merged_entry enable %hit_valid

  %new_entry = ac.var.create !ac.struct<@types::@MshrEntry> {
      valid = %true_i1,
      line_address = %line_address,
      transaction = %transaction,
      pending_mask = %source_mask
  }
  ac.table.write @mshr[%free_index], %new_entry enable %allocate

  // 本例即使 accepted=false 也消费请求并产生一个 response。
  ac.firing.yield guard %true_i1 outputs(%response_value)
} : (!ac.queue<!ac.struct<@types::@MissRequest>>)
    -> !ac.queue<!ac.struct<@types::@MshrResponse>>
```

### 30.3 RTL

```text
16-way line-address CAM comparison
→ hit mask + priority encoder
→ 16-way free comparison
→ free mask + priority encoder
→ selected Entry mux / response Queue
→ merge-update and allocation write paths
```

## 31. Cache Tag / TLB 示例

### 31.1 前端

```python
@ac.struct
class TagEntry:
    valid: bool
    tag: ac.u40
    permissions: ac.u4

tags = ac.table[[64, 8], TagEntry](
    init=TagEntry(
        valid=False,
        tag=0,
        permissions=0,
    )
)

@ac.rule
def lookup(request):
    set_view = tags.view(request.set_index)
    hits = set_view.match(
        lambda entry:
            entry.valid
            & (entry.tag == request.tag)
    )
    way = set_view.choose(
        hits,
        count=1,
        policy="first",
    )[0]

    tag_entry = set_view.view(way.index)
    tag_entry.patch(
        valid=False,
        enable=way.valid & request.invalidate,
    )

    return TagLookupResult(
        request_id=request.request_id,
        hit=way.valid,
        way=way.index,
        permissions=tag_entry.permissions,
    )

lookup_results = lookup(tag_lookups)

tags.view(
    lambda fill: (fill.set_index, fill.way)
).write(
    fills,
    value=lambda fill:
        TagEntry(
            valid=True,
            tag=fill.tag,
            permissions=fill.permissions,
        ),
)
```

同样的形式可用于 TLB：predicate 捕获 VPN/ASID，rule return 生成 translation response，
masked patch 用于 invalidate。Lookup 使用 `@ac.rule` 明确组合 match、choose、response 和
可选 invalidate state effect；fill 的独立 `write(fills, ...)` 仍是简单单输入隐式 firing。

### 31.2 预计完整 Frozen ACIR

```mlir
ac.table @tags
    shape [64, 8]
    element !ac.struct<@types::@TagEntry>
    init #ac.table_init<zero>

%lookup_results = ac.firing inputs(%tag_lookups) depths [1] latencies [1] {
^body(%request : !ac.var<!ac.struct<@types::@TagLookup>>):
  %true_i1 = ac.var.constant true as !ac.var<i1>
  %false_i1 = ac.var.constant false as !ac.var<i1>
  %set_index = ac.var.get %request field "set_index"
  %request_tag = ac.var.get %request field "tag"
  %request_id = ac.var.get %request field "request_id"
  %invalidate_request = ac.var.get %request field "invalidate"
  %hits = ac.table.match @tags[%set_index, *]
      captures(%request_tag) {
  ^predicate(
      %entry : !ac.var<!ac.struct<@types::@TagEntry>>,
      %tag : !ac.var<i40>
  ):
    %valid = ac.var.get %entry field "valid"
    %entry_tag = ac.var.get %entry field "tag"
    %same_tag = ac.var.cmp "eq" %entry_tag, %tag
        : !ac.var<i40> -> !ac.var<i1>
    %hit = ac.var.and %valid, %same_tag
    ac.table.match.yield %hit
  } : !ac.var<i8>

  %way, %hit_valid =
      ac.table.choose @tags[%set_index, *], %hits
          count 1
          policy "first"

  %entry = ac.table.get @tags[%set_index, %way]
  %permissions = ac.var.get %entry field "permissions"
  %lookup_result = ac.var.create !ac.struct<@types::@TagLookupResult> {
      request_id = %request_id,
      hit = %hit_valid,
      way = %way,
      permissions = %permissions
  }

  %invalidate = ac.var.and %hit_valid, %invalidate_request
  %invalidated = ac.var.with %entry, %false_i1 field "valid"
  ac.table.write @tags[%set_index, %way], %invalidated enable %invalidate

  ac.firing.yield guard %true_i1 outputs(%lookup_result)
} : (!ac.queue<!ac.struct<@types::@TagLookup>>)
    -> !ac.queue<!ac.struct<@types::@TagLookupResult>>

ac.firing inputs(%fills) {
^body(%fill : !ac.var<!ac.struct<@types::@TagFill>>):
  %true_i1 = ac.var.constant true as !ac.var<i1>
  %set_index = ac.var.get %fill field "set_index"
  %way = ac.var.get %fill field "way"
  %tag = ac.var.get %fill field "tag"
  %permissions = ac.var.get %fill field "permissions"
  %entry = ac.var.create !ac.struct<@types::@TagEntry> {
      valid = %true_i1,
      tag = %tag,
      permissions = %permissions
  }
  ac.table.write @tags[%set_index, %way], %entry enable %true_i1
  ac.firing.yield guard %true_i1
} : (!ac.queue<!ac.struct<@types::@TagFill>>) -> ()
```

### 31.3 RTL

```text
lookup Queue head / set index
→ 选中一个 8-way Tag set
→ 8 个 tag comparator
→ 8-bit hit vector
→ priority encoder
→ way index + valid
→ selected Tag Entry mux
→ response Queue and optional invalidate write
```

带 latency、bank conflict 或独立 ready-valid 行为的 Data Array 应建模为 Module，而
不是加入 Frozen Table primitive。

## 32. GFSim 实现模型

概念运行时：

```cpp
template <typename Entry, size_t N>
class SimTable {
  std::array<Entry, N> committed;
  std::array<Entry, N> initialImage;
  std::vector<FiringWriteProposal<Entry>> proposals;
};
```

组合读取从 `committed` 返回 Entry。`ac.table.write` 只向当前 firing transaction 登记
`{table, index, value, local_enable}` proposal；它不能自行修改 `committed`，也不能在
firing 尚未决定时提前接受写入。

每个 tick：

```text
1. 所有 firing 从 Queue head 与 committed Table/Reg snapshot 计算候选 transaction
2. 根据 input valid、output ready 和 guard 计算每个 firing 的 fire
3. 只保留 fire=true 且 local_enable=true 的 state proposal
4. 检查 index 越界和跨 firing/同 firing 的同地址 write conflict
5. Queue pop、Queue push、Table write 与 Reg write 作为同一 tick commit 统一应用
6. 清空本 tick 的 transaction/proposal
```

如果任一步验证失败，本 tick 对应非法设计的提交不得部分发生。所有 match/choose 都从
`committed` snapshot 计算；GFSim 的组件 Work 顺序不得影响最终结果。实现可以复用现有
Queue atomic transform 的 transaction 骨架，但 Table/Reg proposal 必须归属于同一个
firing，而不是成为独立 Work。

## 33. Provider 资源限制

Table 在语言层面不设置固定总大小上限。

Provider 可以为以下项目设置可配置安全阈值：

```text
总 Entry 数
总状态 bit 数
match domain 大小
choose count
展开 write 数
生成 IR/RTL 大小
```

超过限制时必须产生明确诊断，可以允许用户显式提高限制，但不得静默引入 latency、仲裁或时分复用。

## 34. Lowering Pipeline

```text
Python Frontend
│
│  ac.table[...]()
│  ac.reg[...]()
│  @ac.rule(...)
│  view/read/write/patch
│  match/choose
│  Mask<domain>/Selection
│
▼
High-Level Representation
│
├─ 解析 Table shape / Entry / init
├─ 绑定唯一 owner
├─ ac.reg → typed scalar state
├─ EntryView / TableView → typed AccessPath
├─ 简单 Queue-facing method → 零/单输入、零/单输出 firing anchor
├─ Queue-dependent match → 单输入 firing anchor
├─ 无参数 read() → 零输入、单输出 firing anchor
├─ anchor 的 when= → firing guard
├─ 后续无参数 Table effect / Reg update → 附着到同一基本块的唯一当前 firing
├─ 新 anchor / 基本块结束 / return → 关闭当前隐式 firing
├─ @ac.rule 参数/return → 多输入/多输出 firing
├─ @ac.rule(when=callback) → 显式 rule guard
├─ rule 内 read() 或带 Queue 参数 Table method → 拒绝嵌套 firing
├─ trigger Queue rate 检查；Table/Reg effect 第一版只接受 rate=1
├─ firing input（若存在）→ firing-local token Var
├─ Match → typed Mask<domain>
├─ Choose → typed Selection[index, valid]
└─ read result / rule return → typed output Queue
│
▼
Canonicalization
│
├─ View 消失
├─ Reg expression → reg.get
├─ Reg augmented assignment → reg.write proposal
├─ EntryView field/value use → snapshot table.get
├─ Queue/state-driven read → table.get + response construction + firing output
├─ Patch → get + var.with + write
├─ masked View → 常量索引并行 write
├─ Mask<domain> → !ac.var<iN>
├─ Selection → 显式 index + valid
├─ firing guard/local condition → firing/write enable
├─ trigger Queue（若存在）→ firing operand/token region argument
├─ 零输入 anchor → 无 operand 的 firing
├─ read/rule return Queue → firing result/yield output
└─ lambda captures → 显式 region operands
│
▼
Frozen ACIR
│
├─ ac.table
├─ ac.table.get
├─ ac.table.write
├─ ac.table.match
├─ ac.table.choose
├─ ac.reg
├─ ac.reg.get
├─ ac.reg.write
├─ !ac.var<T>
├─ ac.firing
└─ ac.firing.yield
│
▼
Structural Lowering
│
├─ state arrays
├─ scalar registers
├─ read mux
├─ write decoder / enable
├─ comparator bank
├─ priority encoder
├─ comparator tournament
├─ reset image
└─ assertions
│
▼
GFSim / PYC / RTL
```

## 35. Contract 迁移

新的 `ac.table` 直接替换当前 memory-service Table 语义。新的 Queue-facing
`EntryView.read/write/patch` 只是在前端包装 `ac.firing + ac.table.*`，不恢复旧的
隐式 memory latency、端口仲裁或 request opcode 模型。

这是 contract epoch breaking change：

- 不保留旧 `ac.table(...)` 前端兼容入口；
- 旧 request/response memory 使用 `ac.memory.instance/request`；
- 可另行提供 `ac.memory(...)` 前端封装；
- 旧 `gfsim::Table` wrapper 删除或重命名；
- block catalog 中 `ac.table` 改为新的 state Table；
- verifier 拒绝旧 Table 参数形式；
- 所有后端必须实现新的 Table primitive family。

现有 Queue mux `ac.select` 保持不变。候选选择使用：

```text
table.choose(...)
ac.table.choose
```

## 36. 第一版明确不支持

```text
多 owner
隐藏 write priority
write collision arbitration
多周期 Table get
Frozen Table primitive 自带 ready-valid
SRAM 自动推断语义
bank conflict
隐藏 latency 的异步 request-response
跨 tick View
runtime pointer/reference
动态 shape
多维全域 match
剩余维度大于 1 的 choose
嵌套 Table predicate
stateful round-robin choose
Table/Reg state firing 的 rate > 1
symbolic Python `if` 直接充当 firing guard
在 `@ac.rule` 中嵌套 firing anchor
多个 firing anchor 的隐式合并
无唯一 anchor 的无参数 write/patch/Reg write
运行时可选的 Queue 输入或输出
```

Round-robin 若未来支持，必须显式提供和返回 arbitration state，不允许 `table.choose` 隐藏内部状态。

## 37. 待冻结细节

以下不阻塞核心设计，但在正式实现前仍需确定：

- `min/max` key 的 signed/unsigned 编码方式；
- sequence number 回绕的推荐 age-key 模式；
- provider 默认资源阈值；
- Table initializer attribute 的最终 MLIR 语法；
- assertion 在综合 RTL 与验证 RTL 中的保留策略；
- `Array<Module>` 的独立设计。

总体上，这套方案形成如下闭环：

```text
前端 Table/EntryView/TableView/Mask/Selection
→ 简单方法从 Queue-driven 或 state-driven anchor 隐式生成 firing
→ 将同一基本块中后续无参数 Table effect 与 Reg update 附着到该 firing
→ 复杂多输入/多输出事务由 @ac.rule 显式框定
→ Frozen firing + state primitives
→ committed snapshot + write proposal
→ register array / mux / comparator / selector / write-enable RTL
```

它能够统一表达 ROB、Issue Table、Scoreboard、Rename Map、Free List、MSHR、Cache Tag 和 TLB，同时保持 Table 本身不承担任何专用组件策略。
