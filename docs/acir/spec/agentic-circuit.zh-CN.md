# Agentic Circuit 团队 Specification 手册

| 字段 | 内容 |
| --- | --- |
| 目标版本 | Explicit Memory contract epoch `0.4` |
| 状态 | 已在 `main` 实现；本文是团队阅读入口 |
| 适用读者 | Python 前端、ACIR、gfsim、PYC/Verilog 和模型验证开发者 |
| 规范主文档 | [Agentic Circuit Specification Manual](agentic-circuit.md) |
| 机器可读清单 | `opcodes.json` |
| 可执行示例 | `examples/pipelines` |

## 文档定位

本文帮助团队成员快速理解和使用 Agentic Circuit。它解释编程模型、
常用积木、后端差异、验证方法和常见错误，并给出与仓内测试一致的示例。

本文不复制所有 ODS 签名和 verifier 条件。发生差异时，按以下顺序判断：

1. JSON Schema、opcode catalog 和 MLIR ODS；
2. verifier 与 conformance test；
3. [英文规范](agentic-circuit.md)；
4. 本文和设计提案。

规范中的 **MUST**、**MUST NOT**、**SHOULD**、**SHOULD NOT** 和 **MAY**
具有 RFC 风格的约束含义。本文使用“必须”“禁止”“应该”和“可以”表达同一
含义。

## 一句话理解

用户编写串行风格的 Python。编译器读取 AST，把 Python 变量解释成静态连接的
`ac.queue<T>`，把 lambda 内的值解释成零延迟 `ac.var<T>`，再从同一份冻结 ACIR
生成两种内部结构不同的后端：

```text
串行 Python
    |
    v
AST capture / ACPy
    |
    v
Queue/Var ACIR
    |
    +------------------------------+
    |                              |
    v                              v
typed gfsim C++              canonical PYC IR
SimQueue<T> 模型                  |
                                  v
                         pinned pycc
                           |       |
                           v       v
                        PYC C++  Verilog
```

Python 看起来按顺序书写，但运行时不是逐行解释 Python。系统体只在编译期完成
拓扑 elaboration；生成的 Queue、积木和作用域在运行期保持静态。

## 核心对象

### Queue 是带时序的状态

`ac.queue<T>` 是有限深度、带类型、带反压的 FIFO 通道。它具有：

- 编译期确定的 payload 类型 `T`；
- 正数 `depth` 和正数 `latency`；
- 稳定的逻辑身份和层级路径；
- `peek`、`pop`、`push` 和 commit-time 状态变化；
- gfsim 中的 `SimQueue<T>` 实例；
- PYC/RTL 中的 valid/data/ready 与固定存储。

Queue 的 latency 禁止为零。需要零延迟组合逻辑时使用 Var 表达式。

### Var 是不可变组合值

`ac.var<T>` 没有容量、占用率和运行时对象身份。以下对象都属于 Var：

- lambda 参数；
- 常量和算术结果；
- 结构体字段投影；
- 比较结果；
- `with_fields(...)` 产生的新 token。

Queue 可以改变占用状态，Queue 中的 token 不可原地修改。

```python
# 正确：创建一个新 token。
next_item = item.with_fields(remaining=item.remaining - 1)

# 错误：原地修改输入 token。
item.remaining -= 1
```

### Opcode 是公共硬件积木

公开积木统一使用 `ac.*` 命名空间。不存在 `ac.std.*`，也不允许用户定义私有
opcode、C++ provider、PYC provider 或直接插入 Verilog。

`decode`、`dispatch`、`rename`、`retire` 等名称属于具体架构的 scope 或积木组合，
不是通用 opcode。公共积木描述的是 transform、route、merge、memory、barrier、
credit、dependency 和 reorder 等可复用硬件行为。

查看当前闭集：

```bash
build/dev-llvm22/bin/acir-opcode-catalog
agentic-circuit schema opcode ac.transform
```

## 最小示例

下面的系统没有显式输入输出声明。`source` 和 `sink` 定义可执行边界，变量的定义和
使用关系定义 Queue 连接。

```python
import agentic_circuit as ac


@ac.system
def pipeline() -> None:
    incoming = ac.source(int, depth=4, latency=1)
    outgoing = incoming.apply(
        lambda item: item + 1,
        depth=2,
        latency=1,
    )
    ac.sink(outgoing)
```

它 elaboration 成：

```text
incoming Queue -> ac.transform(item + 1) -> outgoing Queue -> ac.sink
```

系统函数必须无参数。`source`、`apply` 和 `sink` 是 AST marker，不能通过普通
Python 调用系统体来模拟电路。

## 定义结构化 payload

使用 `@ac.struct` 冻结字段顺序、位宽和结构身份。

```python
import agentic_circuit as ac


@ac.struct
class WorkItem:
    sequence: ac.u64
    value: ac.u32
    route: ac.u2
    remaining: ac.u16
    valid: bool


@ac.system
def pipeline() -> None:
    incoming = ac.source(WorkItem)
    updated = incoming.apply(
        lambda item: item.with_fields(
            value=item.value + 1,
            remaining=item.remaining - 1,
        )
    )
    ac.sink(updated)
```

当前常用字段类型包括 `bool`、`int`、`ac.u1/u2/u4/u8/u16/u32/u64` 和
`ac.s8/s16/s32/s64`。当前 ACIR 契约已冻结整数宽度，但尚未把有符号性作为完全独立的
类型语义；不要假设所有比较都自动采用 Python 的有符号规则。

可执行示例：
`pyc_struct_pipeline.py`。

## 使用 scope 表达层级

`with ac.scope("name"):` 表达所有权和层级，不声明端口。编译器根据跨 scope 的
def-use 自动推导输入、输出和 interconnect 所属的最低公共祖先。

```python
@ac.system
def pipeline() -> None:
    incoming = ac.source(int)

    with ac.scope("frontend"):
        prepared = incoming.apply(lambda item: item + 1)

    with ac.scope("backend"):
        completed = prepared.apply(lambda item: item * 2)

    ac.sink(completed)
```

scope 名必须非空，同一路径不能重复声明。跨 scope 的 Queue 不会被两个子模块重复
拥有；生成系统拥有 interconnect，子模块只借用类型化 Queue 引用。

## 数据通路积木

### Transform

`apply` 生成一个 `ac.transform`。一次 firing 原子地 pop 输入并 push 输出。

```python
updated = incoming.apply(
    lambda item: item.with_fields(value=(item.value + 1) * 2),
    depth=4,
    latency=2,
)
```

lambda 必须是纯 Var 表达式，且返回类型与输出 Queue payload 一致。当前支持字段读取、
常量、`+`、`-`、`*`、比较和不可变 `with_fields(...)` 更新。

### Broadcast 与 Fork

同一 Queue 被多个消费点使用时，前端自动插入严格原子的 `ac.broadcast`：所有输出
必须在同一 firing 接收 token。

需要各输出在不同 cycle 接收时，显式使用 `fork`：

```python
left, right = incoming.fork(outputs=2, depth=2, latency=1)
```

`fork` 为当前 token 保存 delivered mask，每个输出恰好收到一次，全部完成后才 pop
输入。两者不能互换，因为反压和内部状态不同。

可执行示例：
`pyc_broadcast_pipeline.py` 和
`pyc_fork_pipeline.py`。

### Route 与 Merge

`route` 根据一个 Var selector 把 token 发送到一个静态输出，`merge` 把同类型 Queue
合并回来。

```python
scalar, vector = prepared.route(
    outputs=2,
    key=lambda item: item.route,
    depth=2,
    latency=1,
)

scalar_done = scalar.apply(lambda item: item.with_fields(value=item.value + 1))
vector_done = vector.apply(lambda item: item.with_fields(value=item.value + 2))

completed = scalar_done.merge(
    vector_done,
    policy="round_robin",
    depth=4,
    latency=1,
)
```

selector 越界产生确定性 `route_selector_out_of_range` 失败，不会回绕。merge policy
仅支持 `priority` 和 `round_robin`。

可执行示例：
`pyc_route_merge_pipeline.py`。

## 静态集合与运行时选择

Queue 和 Var 可以放入编译期确定形状的 array、map 和 set。

```python
lanes = ac.array(4, lambda index: ac.source(int, depth=index + 1))
named = ac.map({"scalar": lanes[0], "vector": lanes[1]})
active = ac.set({named["scalar"], named["vector"]})
```

静态索引和编译期遍历会被展开。运行时从 flat Queue collection 选择一个成员时，必须
提供显式 control Queue；编译器生成 `ac.select`，而不是动态 Queue 指针。

```python
@ac.struct
class Control:
    route: ac.u2


@ac.system
def selected_pipeline() -> None:
    control = ac.source(Control)
    lanes = ac.array(4, lambda index: ac.source(int))
    selected = lanes.select(
        control,
        key=lambda item: item.route,
        depth=2,
        latency=1,
    )
    ac.sink(selected)
```

control token 与被选中的 data token 原子传输。越界产生
`select_selector_out_of_range`。嵌套集合必须先静态展开，不能在运行时保存或返回
Queue handle。

可执行示例：
`pyc_select_pipeline.py`。

## 状态和调度积木

### Typed Memory

`memory` 是单读单写的类型化状态积木。一次请求总会读取；写入在 Xfer commit；同一
请求对同地址读写时返回旧值，新值对后续请求可见。

```python
@ac.struct
class Request:
    address: ac.u8
    write: bool
    data: ac.u16


@ac.system
def memory_pipeline() -> None:
    sram = ac.memory(ac.u16, entries=16, init=0, latency=3)
    requests = ac.source(Request)
    responses = sram.request(
        requests,
        address=lambda item: item.address,
        write=lambda item: item.write,
        data=lambda item: item.data,
        result_field="data",
        depth=4,
    )
    ac.sink(responses)
```

只允许 `init=0`，实例 `latency` 必须为正数。周期 `T` 接受的请求最早在
`T + latency` 提交响应；request 的 response Queue latency 固定为 1。一个实例可以
连接多个逻辑 endpoint，但只有一个物理端口和一个 outstanding request。endpoint 按
冻结 ordinal 固定优先级仲裁；访问延迟期间及 response Queue 阻塞时全部反压，直到
选中 response Queue 接纳响应。PYC 每个实例只生成一个 `pyc.sync_mem`。

Python 前端允许用同构 `ac.array` 静态声明 memory banks，并以
`banks.select(requests, key=...).request(...)` 明确选择 bank。该写法只在前端展开：
冻结 ACIR 包含一个 `ac.route`、每个 bank 各一个普通 memory instance/request，以及
一个 response `ac.merge`，不会引入新的 primitive。各 bank 的 outstanding 状态独立，
因此跨 bank response 可能乱序；需要保序时应在 payload 中保留 tag 并显式接
`reorder`。epoch 0.4 仅支持一维、data type、entries、init 和 latency 完全相同的 memory
array。

可执行示例：
`pyc_memory_pipeline.py`。

### Stateful Table 原型

epoch `0.4` 新增一维、全零初始化的状态 Table：

```python
Table16 = ac.table[16, Entry]
table = Table16(init=0)
entry = table.view(0)
snapshots = entry.read(when=entry.valid, depth=1, latency=1)

table.view(lambda update: update.index).patch(
    updates,
    enable=lambda update: update.enable,
    done=True,
    result=lambda update: update.result,
)

pending = table.match(lambda entry: not entry.valid)
table.view(pending).patch(
    enable=request_slot.valid,
    valid=True,
    age=lambda entry: entry.age + 1,
)

table.view(tail).allocate(
    enable=allocation.valid,
    value=allocation.value,
)
```

Entry 只能是 bool、定宽整数或仅包含这些字段的扁平 struct。`read()` 总是返回
`Queue<Entry>`；Queue-driven read 的 `when=false` 不消费输入，disabled write 消费
输入但不提出写 proposal。同 tick 读写返回 old committed Entry，写入在 tick commit
后可见，动态越界报告 `table_index_out_of_range`。

`Table.view(candidates)` 接受同一张 Table 的 `match` 所产生的 `CandidateSet`，用于
state-driven masked update。masked `write` 给所有命中 Entry 写入同一个完整值；
masked `patch` 的字段可以是统一表达式，也可以是从各命中 old Entry 求值的纯
`lambda entry`。`enable=false` 不求值 mask/value，空 mask 是 no-op，所有命中项在
同一个 tick edge 原子提交。scalar 与 masked endpoint 可以共存，但各 endpoint 静态声明的
顶层写字段集合必须两两不相交。第一版不分析 address、mask、enable 或 predicate 的动态
互斥性；只要两个 endpoint 声明同一字段就静态拒绝。

每张 Table 可以额外声明一个 state-driven scalar `allocate` endpoint，与上述普通字段
writer 共存。它在调用方提供的 index 安装完整 Entry，不搜索空位、不检查占用状态，也不
隐式修改 `valid`。Queue-driven 和 CandidateSet masked allocation 均拒绝。所有 policy
读取 old committed image；commit 先合并普通字段 proposal，再应用 allocation，因此同一
Entry 上 allocation 覆盖普通更新，不同 Entry 仍独立更新。

每次作者写出的 `match` 和 `choose` 都是共享值，不是 endpoint-local 语法糖。
Frozen ACIR 只生成一个支配所有使用点的 `ac.table.match` 或 `ac.table.choose`，read/write
policy region 捕获其 SSA result。QueueGraph 与 typed gfsim 保留这种共享关系：结果按完整
Epoch 惰性求值并缓存，同一 Epoch 的多个 consumer 只触发一次 Table scan；Epoch 前进或
模型 reset 后重新计算。choose 的 mask 必须来自同 Table 的 match。`policy="first"`
使用空 key region，min/max 仍要求一个有类型的 key region。

`EntryView` 只存在于 elaboration。`patch` 在 Frozen ACIR 前展开成
`ac.table.get -> ac.var.with -> ac.table.write` 或 `ac.table.masked_write`，不存在
`ac.table.patch`。两种 Frozen write op 都必须携带规范化、非空、无重复的
`write_fields` 与必需的 `mode`：普通 write 使用 `mode "field"`，scalar allocation
使用 `mode "replace"`，masked write 只允许 `field`。完整 struct write 展开为所有实际
字段，bool/int Entry 使用 `$entry`。
value region 仍返回完整 Entry，但 commit 只复制声明字段；所有 writer 从同一 old
committed image 求值，并在 tick edge 合并后一次发布。
当前 Table 只支持 typed gfsim C++；PYC/RTL 返回稳定的
`unsupported provisional Table` 诊断。旧 `ac.table(...)` 已删除，请求响应存储继续
使用 `ac.memory`。纵向示例见
唯一公开的 Python Table 示例是
`issue.py`。它组合两个字段不相交的操作数唤醒、下一
tick 的最小 age 选择、grant 驱动的移除，以及显式 match、choose old-state 空位后的
完整 Entry allocation。Table 满时 allocation 请求会保留到空位可选。其余聚焦源码仅
作为内部 E2E fixture，不再作为公开示例。

### Credit

`credit` 表达固定数量的并行 in-flight slot。每个 slot 独立倒计时，完成顺序可以和
输入顺序不同。

```python
completed = issued.credit(
    cost=lambda item: item.cycles,
    credits=4,
    depth=4,
    latency=1,
)
```

`credits` 必须为正，运行时 cost 必须为正。多个 slot 同时完成时，选择最低 canonical
slot index，并且每个 epoch 最多输出一个 token。

可执行示例：
`pyc_credit_pipeline.py`。

### Barrier

`barrier` 等待所有输入可 pop 且所有输出可 push，然后一次性提交所有效果。各输入
payload 类型可以不同，但输出必须按位置匹配。

```python
left_ready, right_ready = left.barrier(
    right,
    depth=2,
    latency=1,
)
```

输入 Queue 必须互不相同。barrier firing 前禁止发布部分结果。

可执行示例：
`pyc_barrier_pipeline.py`。

### Dependency 与 Reorder

`depend` 表达有界依赖窗口、资源占用和执行 cost；`reorder` 按连续 key 恢复顺序。
它们可以组合成 issue/execute/retire 风格架构，但这些应用阶段仍是 scope，而不是新
opcode。

```python
completed = issued.depend(
    key=lambda item: item.sequence,
    waits_for=lambda item: item.waits_for,
    resource=lambda item: item.route,
    cost=lambda item: item.cycles,
    capacity=16,
    resources=4,
    no_dependency=255,
    depth=8,
    latency=1,
)

retired = completed.reorder(
    key=lambda item: item.sequence,
    capacity=16,
    start=0,
    depth=4,
    latency=1,
)
```

完整参考：
`davincioo_queue_model.py`。该模型用公共
积木构造 DavinciOO-like 拓扑，并验证 15 条记录、out-of-order completion、in-order
retirement、Queue occupancy 和 453-cycle 投影。

## 串行控制流

### 编译期控制

`if True/False`、`range(constant)`、静态集合遍历和结构递减的有限递归在编译期展开。

```python
def add_stages(queue, count):
    if count == 0:
        return queue
    return add_stages(
        queue.apply(lambda item: item + 1),
        count - 1,
    )


@ac.system
def recursive_pipeline() -> None:
    incoming = ac.source(int)
    outgoing = add_stages(incoming, 3)
    ac.sink(outgoing)
```

递归深度必须是 `[0, 1024]` 范围内的编译期整数。后端中不会留下递归或动态模块创建。

可执行示例：
`pyc_recursive_pipeline.py`。

### 运行时 if

当前高层语法支持对同一 Queue 的对称二分支更新。它会 lowering 成 route、两个
transform 和互斥 priority merge。

```python
if incoming.route == 0:
    selected = incoming.apply(
        lambda item: item.with_fields(value=item.value + 10)
    )
else:
    selected = incoming.apply(
        lambda item: item.with_fields(value=item.value + 20)
    )
```

两个分支必须消费同一 Queue、各执行一次 apply，并赋值给同一新变量。更复杂的控制
使用显式 route/merge 组合。

可执行示例：
`pyc_conditional_pipeline.py`。

### 有界 while

Queue rebinding 的串行 `while` lowering 成带状态 feedback Queue 的 `ac.feedback`。

```python
while current.remaining > 0:
    if current.stop:
        break
    current = current.apply(
        lambda item: item.with_fields(remaining=item.remaining - 1)
    )
    if current.skip:
        continue
```

当前允许 update 前一个 break guard 和尾部一个 continue guard。最大迭代次数为
1024，超出时产生 `feedback_iteration_limit`。

可执行示例：
`pyc_feedback_pipeline.py` 和
`pyc_loop_control_pipeline.py`。

## 显式 Queue effect

> **当前实现说明：** `D-RULE-LOWERING-001` 已取代这些 Python 拼写作为目标公共
> rule 前端。`.firing()` 仅在当前 prototype 存续期间记录于此；后续使用简单的
> `@ac.rule`，由 MLIR pass 生成检查、握手、调度与内部 `ac.firing` transaction。

需要强调 pop 的副作用和 peek 的非消费读取时，使用 `firing`：

```python
outgoing = incoming.firing(
    lambda queue: queue.push(
        queue.pop().with_fields(
            value=queue.peek().value + 1,
        )
    )
)
```

一次 Python firing 必须恰好包含一次 `pop` 和一次最外层 `push`，可以重复 `peek`。
`peek` 和 `pop` 返回不可变 Var。当前单输入单输出形式会规范化为标准
`ac.transform`，不会在生成的 hot path 中解释 Python effect 对象。

可执行示例：
`pyc_firing_pipeline.py`。

## Observation 与 Verification

`ac.observe(queue)` 非消费地观察 committed head，不参与反压，可以进入设计 lowering。

`ac.expect(...)` 也是非消费 leaf，但角色是 verification：

```python
ac.expect(
    completed,
    predicate=lambda item: item.value > 0,
    message="value must be positive",
)
```

gfsim 对每个新 committed head 检查 predicate，失败时报告 `expectation_failed`。
PYC design hierarchy 明确拒绝 `ac.expect`；对应检查必须放到 PYC testbench boundary。
这不是后端缺失，而是 design role 与 verification role 的边界。

可执行示例：
`gfsim_expect_pipeline.py`。

## gfsim 与 PYC/Verilog 的差异

两种后端共享冻结 ACIR 语义，但内部 IR 和状态结构不要求相同。

| 方面 | typed gfsim | PYC/Verilog |
| --- | --- | --- |
| Queue | `SimQueue<T>` 对象 | valid/data/ready + FIFO/register |
| Var | C++ 局部值或纯表达式 | wire、packed value、组合 op |
| 层级 | `gfsim::Module` 对象层级 | 静态 module hierarchy |
| 调度 | snapshot/proposal/Xfer/commit | 时钟边沿和 ready/valid handshake |
| Memory | 类型化 QueueMemory 状态 | `pyc.sync_mem` + 对齐寄存器 |
| Verification | `ac.expect` 可执行 | design 中拒绝，testbench 中实现 |
| 输出 | 类型化 C++ simulator | PYC C++ 与 Verilog |

跨后端 refinement 比较声明过的语义投影：输入/输出 transaction、接受与完成身份、
架构状态、memory-visible effect 和明确的 assertion。它不比较 gfsim delta、内部 Queue
布局、PYC 寄存器名称或每个未声明的内部 cycle。

同一 PYC IR 生成的 PYC C++ 和 Verilog 必须 cycle equivalent。gfsim 与 PYC 可以有
不同内部 latency，但必须满足选定的 observation/refinement contract。

## 编译和验证示例

先配置 LLVM 22.1.8 开发环境：

```bash
scripts/bootstrap-dev.sh
source .venv/bin/activate
cmake --preset dev-llvm22
cmake --build --preset dev-llvm22
```

生成 frozen ACIR、QueueGraph plan 和 typed gfsim C++：

```bash
PYTHONPATH=src .venv/bin/python tools/ac-queue-cxxgen.py \
  examples/pipelines/davincioo_queue_model.py \
  --system davincioo_queue_model \
  --acir-output build/davincioo_queue_model.ac.mlir \
  --plan-output build/davincioo_queue_model.queue-plan.json \
  --acir-opt build/dev-llvm22/bin/acir-opt \
  --queue-plan-tool build/dev-llvm22/bin/acir-queue-plan \
  --queue-cxxgen-tool build/dev-llvm22/bin/acir-queue-cxxgen \
  --output build/davincioo_queue_model.cpp
```

验证生成 C++：

```bash
c++ -std=c++20 -I include -fsyntax-only build/davincioo_queue_model.cpp
```

使用锁定的 pyCircuit toolchain 生成 PYC C++ 与 Verilog：

```bash
PYC_TOOLCHAIN_ROOT=/path/to/pycircuit/toolchain/install

.venv/bin/python tools/ac-queue-pyc-build.py \
  build/davincioo_queue_model.ac.mlir \
  --pycgen-tool build/dev-llvm22/bin/acir-queue-pycgen \
  --pycc "$PYC_TOOLCHAIN_ROOT/bin/pycc" \
  --toolchain-lock toolchains/pyc.lock.json \
  --toolchain-metadata \
    "$PYC_TOOLCHAIN_ROOT/share/pycircuit/toolchain-metadata.json" \
  --cxx "$(command -v c++)" \
  --verilator "$(command -v verilator)" \
  --pyc-output build/davincioo_queue_model.pyc \
  --cpp-output-dir build/davincioo_queue_model-pyc-cpp \
  --verilog-output-dir build/davincioo_queue_model-verilog \
  --manifest build/davincioo_queue_model-pyc-manifest.json
```

命令会检查 `pyc.lock.json`，执行 PYC
verification、C++ syntax check、Verilator lint，并记录确定性 artifact hash。目标输出
路径必须不存在，防止覆盖旧证据。

## 明确禁止的写法

### 显式系统端口

```python
# 禁止：系统边界由 source/sink 和 def-use 推导。
@ac.system
def pipeline(input_queue, output_queue):
    ...
```

### 零延迟 Queue

```python
# 禁止：Queue latency 必须为正。
incoming = ac.source(int, latency=0)
```

### 运行时拓扑和动态 Queue handle

```python
# 禁止：运行时不能构造、保存或返回 Queue 指针。
selected_queue = lanes[token.route]
```

应改用 `lanes.select(control, key=...)`。

### 私有 opcode 或后端代码

```python
# 禁止：用户不能从 Python 注册私有实现。
ac.register_opcode("my.dispatch", cpp_provider=..., verilog=...)
```

需要新能力时，先把它定义成通用硬件积木，并同步更新 Python、ACIR、verifier、
QueueGraph、gfsim、PYC、测试和 opcode catalog。

## 常见问题

| 现象 | 原因 | 处理方法 |
| --- | --- | --- |
| Queue 被两个消费者使用 | pop 是消费 effect | 需要原子复制时依赖自动 broadcast；需要解耦时显式 fork |
| `latency=0` 被拒绝 | Queue 是状态，不是 wire | 把逻辑写入 lambda，让它 lowering 成 Var |
| selector 越界 | route/select 拓扑是静态闭集 | 修正 selector 位宽或在上游保证合法范围 |
| loop 被拒绝 | 不是受支持的单 Queue 有界 feedback 形状 | 简化为一次 Queue update，或显式组合 route/merge/feedback |
| PYC 拒绝 `ac.expect` | verification leaf 不能进入 design | 把 assertion 放入 PYC testbench boundary |
| 后端结果内部 cycle 不同 | gfsim 与 RTL IR 不同 | 比较声明的 transaction/state/refinement projection |
| artifact epoch 不匹配 | serialized epoch 是 hard break | 重新生成 exact epoch `0.4` artifact，不使用兼容 shim |

## 修改公共契约的完成条件

新增或修改一个公共积木时，必须同步完成：

- Python 正向和拒绝语法；
- ACIR ODS 类型或 operation；
- verifier、错误码和诊断文本；
- QueueGraph canonical plan；
- gfsim runtime 语义与类型化 C++ 生成；
- PYC lowering，或明确的 backend-role 拒绝；
- 正向、负向、determinism、round-trip 和 cross-backend 测试；
- `opcodes.json`；
- [英文规范](agentic-circuit.md) 和本文对应入口。

不要把某个后端的实现细节提升成共享 ACIR 语义，也不要为已移除的公共表面增加兼容
别名。旧契约由 Git 历史、release tag 和
[`REF-HISTORY-001`](refs/history.md) 保存。

## 推荐阅读顺序

新同学可以按以下顺序阅读和动手：

1. 本文的“核心对象”和“最小示例”；
2. `examples/pipelines/README.md`；
3. `davincioo_queue_model.py`；
4. [英文规范](agentic-circuit.md)；
5. `opcodes.json` 和
   `test/ACIR`；
6. [NDF 仓库布局验证](../../development/acir/verification/repository-layout.md)。
