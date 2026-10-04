# ACPy 前端范式

本文记录 ACPy 前端语义；当前 MLIR 实现及构建入口见 [编译器说明](../pycircuit/README.md)。
ACPy 使用 Python 的函数、控制流和赋值语法，由编译器区分静态资源构造、组合计算和状态效果。
底层调度契约见 [GFSim spec](../gfsim/spec.md)。

## 1. 核心对象

| 对象 | 职责 |
| --- | --- |
| `@ac.module def ...` | 定义 Module 的资源、固定连接、控制流、组合计算及 Rule 调用 |
| `@ac.rule def ...` | 定义具有原子状态效果的计算 |
| `@ac.signal def ...` | 从 Queue 计算全局共享的只读组合值 |
| `ac.queue` | 持有持久状态，可作为 FIFO 或寄存器使用 |
| `ac.var` | 普通组合值，包括 Signal 输出、Module 局部值和 Rule 参数 |

## 2. Module

### 2.1 输入与输出

- 用 `@ac.module` 修饰的 `def` 定义一个 Module。
- 运行时输入可以是 Queue 或 var。Queue 参数保留资源身份，不在调用处预先转换成 payload。
- Module 的运行时 var 输入来自第三方 Signal；Module 之间不直接传递普通动态 var。
- 输出为固定的 Queue 连接，由 Module 的 `return` 导出；Module 不输出 var。
- Module 的 var 输入保留与 Signal 的静态绑定，表示其当前值，不能在构造时取一次值后当作常量。

### 2.2 函数体

Module 函数体可以直接包含：

- Queue 等资源的声明。
- 控制流与组合逻辑计算。
- 对 Queue 的观察、对 Signal 值的使用。
- Rule 调用及输出连接。

已移除 `@ac.work`，Module 函数体直接表达 Work。编译器从 Module 函数体提取运行逻辑，生成 Module Work。
Module Work 选择本次调用哪些 Rule，并计算传给 Rule 的普通参数。Module 本身不是原子事务。

资源声明与运行逻辑可以写在同一个函数体中，但编译后职责分开：

- 资源及固定连接在构造时创建一次，声明和初始化不随 Work 重复执行。
- 控制流、组合计算和 Rule 调用在 Work 中执行。
- 资源形状、容量和初值在构造时确定，不由运行时 Signal var 改变。

Module 的观察不消费 Queue。构造初始化之后，Module 不直接修改持久状态；所有运行时状态效果由 Rule 提出。

## 3. Rule

### 3.1 输入与输出

- 用 `@ac.rule` 修饰的 `def` 定义 Rule；可以定义在 Module 内或外。
- 输入可以是 Queue 或 var。普通 var 可来自 Module 的组合计算或 Signal。
- Queue 输入实际读取 payload 时，提出一次 pop；仅传递 Queue 引用或查询其空满不消费。
- Rule 的输出是固定 Queue 端口。函数体 `return` 返回的 payload 转换成对应输出 Queue 的 push proposal。
- Module 侧获得的 Rule 调用结果是输出 Queue 的引用，不是本拍刚计算出的 payload。
- 多输出按固定返回位置绑定 Queue。某个位置返回 `None` 时，本次不向该 Queue 提出 push。

未执行分支中的输入读取不产生 pop。同一路径对同一输入重复读取，只产生一次消费需求。
返回 `None` 不表示失败；已经提出的其他状态效果仍属于本次候选。

### 3.2 控制流与状态效果

Rule 中可以写控制流、组合计算，并观察非 I/O Queue：

| 行为 | 语义 |
| --- | --- |
| 普通局部变量赋值 | 组合计算 |
| 实际读取输入 Queue 的 payload | 读取旧状态，并提出 pop |
| 观察非 I/O Queue，包括内部状态或绑定输出 | 只读，不自动 pop |
| 对已有 Queue 的数据或字段赋值 | 提出 revise |
| 返回输出 payload | 提出 push |

前端代码不主动调用 Queue 的 pop/push。编译器根据 I/O 角色、实际读取路径和返回值生成这些效果。
消费语义由资源的访问角色决定，不因为某个 Queue 被当作寄存器使用而自动改变。

一条 Rule 本次提出的 pop、push、revise 等效果整体仲裁，获准后统一在 Xfer 提交。
必要输入读取失败时，本次候选不能部分提交。

Rule 的调用与赋值都不立即修改 Queue current。同一轮 Work 的后续读取仍看到旧状态；
需要继续使用刚计算出的值时，将它保存在局部 var 中。

同一 Module 实例内，同一个 Rule 实例的不同调用位置共享固定输出连接与调度记录。
普通参数及捕获的 Module 局部组合值按 var 参数处理；Queue 参数保留资源身份。

## 4. Signal

### 4.1 定义与连接

- 用 `@ac.signal` 修饰的 `def` 定义纯组合函数。
- 输入全部是 Queue。读取 payload、查询空满均不消费 Queue。
- 函数可以有控制流和组合计算，不修改状态、不调用 Rule，不以其他 Signal 为输入。
- `return` 产生一个 var。每个静态绑定的 Signal 实例持有一个全局共享、只读的当前值。
- 同一个 Signal 函数可绑定不同 Queue，形成独立实例；函数定义本身不是一个唯一的全局值。

Signal 的输入 Queue 以及使用其值的 Module、Rule，均按照静态连接建立依赖。
某个 Queue 即使在本轮分支中没有被读取，作为静态输入发生变化时，仍触发该 Signal 重算。
Signal 不维护本轮实际读取集合。

```python
@ac.signal
def ready(left: ac.queue[ac.u32],
          right: ac.queue[ac.u32]) -> ac.var[bool]:
    return not left.empty() and not right.empty()


# 静态连接示意：can_issue 是 Signal 实例输出的 var。
can_issue = ready(left_queue, right_queue)
output = Stage(input_queue, can_issue)
```

### 4.2 更新与通知

首次 Module Work 前，先初始化全部 Signal。之后每轮更新顺序为：

1. 全部 Queue 完成 Xfer。
2. 静态输入中有 Queue 状态变化的 Signal 各重新计算一次。
3. Signal 返回值与旧值不同，才通知静态下游；返回值相同则不通知。
4. 激活相关 Module，标脏直接依赖该 Signal 的 Rule，并激活这些 Rule 所属的 Module。

被激活的 Module 在下一 tick 执行 Work。Signal 值在本轮 Work 和仲裁期间保持不变，
不引入 Module 之间的同拍 var 传播或 delta Work。

## 5. Queue

### 5.1 状态、声明和访问

Queue 是当前统一的持久状态对象：

- FIFO 用法允许空满变化，通过 pop/push 消费和产生元素。
- 寄存器用法采用容量 1、构造时有一个初值、后续通过 revise 更新的 Queue。
- Queue 可在 Module 中显式声明，也可由编译器根据 Rule 输出推导生成。
- Queue 可以组成数组；每个元素仍是一个独立 Queue。首版支持 `ac.array(ac.queue[T], shape=(N,), ...)`，长度构造时固定、运行时可动态索引。

连接两端引用同一个 Queue 对象。“某 Module 的输出 Queue”表示其连接角色，
不表示其他 Module 不能访问；相关 Module 和 Rule 根据固定连接引用该对象。

revise 修改已有元素，不改变元素数量，不能向空 Queue 创建元素。
按当前 GFSim 语义，revise 修改旧队尾；容量 1 时即当前寄存器值。
保留站等容量 1 的 FIFO 可以用 revise 更新条目内的操作数，用 push/pop 管理槽位占用与释放。

### 5.2 显式声明与隐式生成

Module 中的 `out = produce(...)` 表达 Rule 调用及静态输出绑定：

- 已经显式声明 `out` 时，将对应 Rule 输出绑定到该 Queue，使用其声明的类型、容量等配置。
- 没有提前声明 `out` 时，编译器根据 Rule 返回 payload 推导元素类型，补建 Queue 声明，默认容量 1、初始为空。
- 多输出的每个 Queue 分别处理，允许通过显式声明指定不同容量。
- Queue 始终是构造阶段创建的固定对象，不随本轮控制分支创建或消失。

显式声明：

```python
out = ac.queue[ac.u32](capacity=4)
if enabled:
    out = produce(input_q)
return out
```

隐式声明：

```python
if enabled:
    out = produce(input_q)
return out
```

上述片段位于 Module 函数体中。`enabled` 为假时，本轮不选择 `produce`，但 `out` Queue 仍然存在，
Module 的 `return out` 仍导出同一个固定连接。

**Module 中的 Rule 输出绑定不表示 revise；Rule 内对 Queue 数据赋值才表示 revise。**

编译器需要先识别显式和隐式资源、Rule 输出及其静态绑定，再编译引用这些资源的运行逻辑。
因此，Module 或 Rule 可以观察自己的绑定输出 Queue；不要求源代码中的引用一定写在 Rule 调用之后。

目标选择属于模型控制逻辑。选定目标后，push 的容量许可交给调度器，允许利用本拍合法 pop 释放的空间。
这沿用 GFSim 的唯一来源约束：每个 Queue 的 pop、push 分别至多有一个 Rule 来源。

## 6. Module 与 Rule 的完整写法示例

```python
@ac.module
def Stage(source: ac.queue[ac.u32],
          enable: ac.var[bool]):
    count = ac.queue[ac.u32](initial=0)
    out = ac.queue[ac.u32](capacity=4)

    @ac.rule
    def transfer(message: ac.queue[ac.u32],
                 offset: ac.var[ac.u32]):
        value = message.value          # 输入读取：pop
        count.value = count.value + 1  # 内部状态赋值：revise
        return value + offset          # 输出 payload：push

    if enable:
        offset = count.value & 1       # Module 组合计算，不消费 count
        out = transfer(source, offset)

    return out                         # 导出固定 Queue 连接
```

`count` 与 `out` 创建一次。`enable` 来自外部 Signal，`offset` 是本次 Work 计算的普通 var。
Rule 捕获的 `count` 是非 I/O 状态资源，读取不自动 pop。
`transfer` 的输入消费、内部状态更新和输出产生属于同一次原子提交。

## 7. 数组与循环

`ac.array(ac.queue[T], shape=(N,), capacity=1, initial=...)` 创建 N 个独立 Queue。
首版只支持一维，N 在构造阶段固定。省略 initial 时所有元素为空；initial 可以是相同初值，
或接收元素下标的纯 helper。容量与初值遵循普通 Queue 的规则。

固定 tuple/list 返回结构逐个叶子绑定输出 Queue，叶子为 None 时只省略该位置的 push。
结构体与 `ac.array[T, N]` 是普通 payload 值，支持字段、下标读取及更新；
`q.value.field[index] = new_value` 捕获 index 与 new_value，Xfer 时修改旧队尾。

构造资源的固定循环在静态构造阶段处理；运行时 `for i in range(stop)` 或
`range(start, stop)` 保留为循环，ROB/RS 扫描不强制展开。首版不支持 break/continue、
while、运行时资源声明、多维资源数组。类型推导出现无信息的连接循环时需补充注解。

是否新增 Cell、更多数组维度和循环形式仍可继续讨论。
