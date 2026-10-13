# ACPy 编程范式与基础组件

本文定义当前讨论确认的前端设计，与 [ACIR 基础设计](../acir/basic-design.md) 配套。它不是当前编译器已实现功能的清单；实现差异见文末，构建与使用见 [编译器 README](../pycircuit/README.md)。

ACPy 使用 Python 的函数、类型标注和控制流描述模型。编译器解析源码并静态展开实例、资源和连接，不将模型作为任意 Python 程序执行。

## 1. 定义了什么

| 定义 | 职责 |
| --- | --- |
| `@ac.module def` | 定义组件实例、内部资源、固定连接和 Work 逻辑 |
| `@ac.rule def` | 定义独立原子事务，包括参数、guard、计算、效果和返回值 |
| `@ac.method def` | 定义组件接口，可返回普通值，效果归入调用者的外层 Rule |
| 普通 `def` | 定义 helper，复用计算或调用逻辑，不建立独立事务 |
| `ac.queue[T]` | FIFO 消息状态 |
| `ac.cell[T]` | 下拍更新的寄存器状态 |
| `ac.signal[T]` | 本拍写入后有效的共享信号，读取带隐式有效性条件 |
| `ac.ehr[T]` | 按 Method 阶段提供写入可见性的状态 |
| `ac.var[T]`／普通值类型 | 普通组合计算值，不声明持久状态 |

`ac.signal` 是具有固定身份的基础组件，不是纯组合函数装饰器；它不保存跨周期值。普通局部变量、跨周期状态和本拍有效的 Signal 具有不同语义。

## 2. Module：静态结构与 Work

```python
@ac.module
def Stage(source: ac.queue[ac.u32], enabled: bool):
    out = ac.queue[ac.u32]()

    @ac.rule
    def transfer(message: ac.queue[ac.u32], amount: ac.u32) -> ac.queue[ac.u32]:
        return message.value + amount

    amount = ac.u32(1)
    if enabled:
        out = transfer(source, amount)
    return out
```

Module 函数体同时描述两类内容：

| 内容 | 含义 |
| --- | --- |
| 资源声明、子 Module 构造、函数定义、端口连接 | 构造期确定的静态结构 |
| 普通组合计算、状态观察、只读调用、运行时控制流、Rule 调用 | Module Work |

不需要额外的 Work 装饰器。构造期条件直接展开，运行时条件参与 Rule 选择。

### 实例和连接

- Module 参数可以描述组件引用、普通值和构造参数；资源与普通参数可显式标注，也可根据正确格式的调用推断。
- 资源数量、容量、初始化和子实例结构只依赖构造参数，不由运行时分支改变。
- 子 Module 调用表示静态实例化和连接，不表示父 Work 本拍执行一次子 Work。
- Module 返回固定连接或连接结构，不表示提交消息或更新状态。
- 构造调用取得实例句柄；`instance.state` 访问公开资源，`instance.method(...)` 调用接口，`instance.outputs` 取得返回连接。
- 本地 import、别名或传参可以共享已经声明的实例。重新构造得到独立实例，引用共享实例不复制状态。

### Work 的边界

Work 可以读取资源、计算普通值、调用只读 Method，并通过控制流选择 Rule。Work 不提交状态，不调用包含状态效果的 Method。

Module 可以同时拥有 Method、自主 Rule 和自己的 Work。内部或外部实例的资源可以被 Rule／Method 直接访问；事务归属由外层 Rule 确定，不由资源所属 Module 决定。

## 3. Rule：独立原子事务

### 参数、返回值与效果

| Rule 参数／输出类型 | 输入语义 | 返回语义 |
| --- | --- | --- |
| `ac.queue[T]` | 实际读取 payload 的路径提出 pop | 返回 payload，向固定输出 Queue 提出 push |
| `ac.cell[T]` | 引用 Cell，读取旧值 | 返回 payload，向固定 Cell 提出 write |
| `ac.signal[T]` | 引用 Signal，按其可见性读取 | 返回 payload，向固定 Signal 提出 set |
| `ac.var[T]`／普通类型 | 按值传入组合计算结果 | 不作为 Rule 间直接传递临时值的输出 |

Rule 可以有分支、循环、组合计算和 Method 调用，也可以直接读写可访问的状态对象。所有效果，包括跨 Module 调用产生的效果，属于同一个外层事务，一起提交或取消。Rule 被调用不等于事务已经获准。

### Queue 输入与观察

- 消费性 Queue 参数只有实际读取 payload 才产生 pop；仅查询 empty／full／size 不消费。
- 消费角色沿 helper／Method 参数传递，重复或别名读取按外层事务与实际 Queue 合并为一次消费。
- 未执行分支不消费；guard 中的读取始终只是观察。
- 直接观察非 I/O Queue 不自动 pop。普通 Method 的 Queue 参数默认观察；若由消费性参数传入，则保留该角色。
- `ac.ref[T]` 可以明确标注 Rule 的观察性 Queue 参数。

### 输出绑定

```python
out = transfer(source)
```

这里 `out` 表示固定输出资源，而不是一个只有 Rule 成功后才存在的临时返回值。Rule 返回 payload，编译器将它绑定为该事务对输出资源的效果。

- Queue 输出可以显式声明，也可以根据 Rule 输出推导生成；运行时分支不创建新的 Queue。
- Cell 输出必须绑定已经声明且有初值的资源；Signal 输出必须绑定已经声明的 Signal，不要求初值。
- 混合输出使用 `tuple[...]` 注解并逐位置绑定。
- `return None`，或混合输出某位置返回 None，表示省略该位置的输出效果，不代表整个事务失败。
- 未注解的 Rule 输出可按 Queue 推导。
- Rule 间不直接传递临时普通值；传递消息用 Queue，同拍传值用 Signal，跨拍保留值用 Cell。

同一 Module 实例内，同名 Rule 对应同一个 Rule 实例。多个互斥调用位置可以选择它；这些位置不创建新的 Rule 身份。

## 4. Method、guard 与普通计算

### Method

Method 是可组合接口，不自行发射，也不形成新的事务边界。

```python
@ac.module
def Counter():
    count = ac.cell[ac.u32](initial=0)

    @ac.method
    def add(value: ac.u32) -> ac.u32:
        count.value = count.value + value
        return value
```

`counter.add(value)` 必须位于 Rule 事务中，因为它修改状态。只读 Method 可以在 Module Work 中调用。Method 返回普通计算值，不要求声明输出 Queue。

普通 helper 同样没有独立事务；如果调用逻辑包含状态效果，该调用也必须属于外层 Rule。

### Guard 的写法

Rule 和 Method 支持 `guard=`，缺省为 `True`：

```python
@ac.rule(guard=enabled)
def send(message: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
    return message.value
```

guard 可以是布尔变量，也可以是接收对应函数参数、返回 bool 的函数。变量 guard 引用定义作用域当时的值，不被调用者的同名参数或后续局部赋值替换。guard 只观察状态，不消费或修改。

Rule 的完整执行条件包含：Module 的选择路径、Rule 自身 guard、实际路径上调用的 Method guard，以及基础组件操作的默认 guard。分支中的要求只约束对应路径；用户不必重复手写基础就绪条件。

### Module 调用 Method 后的条件传播

```python
value = component.read()  # Method guard 为 ready。
send(value + 1)           # send 继承 ready。
independent()            # 不依赖 value，不继承 ready。
```

Method 返回值的求值条件沿数据和控制依赖传播到相关 Rule。如果 `value` 用作 if 条件，两条分支都以 ready 成立为前提；调用失败不能当成条件为假而进入 else。

这些条件由编译器处理。普通计算值不增加持久 valid 状态，也不默认使用旧值。

### Method 顺序

```python
@ac.method(After="release")
def allocate(value: ac.u32):
    ...
```

表示同一组件的 release 必须先于 allocate。`After` 支持字符串或字符串 tuple／list，可与 guard 同时使用。顺序声明、内部调用与基础组件契约共同参与 Rule 顺序推导；同一 Rule 内组合多个有顺序的 Method，仍保持一个事务。

没有指定先后关系，不凭任意执行顺序额外增加状态可见性。本设计不提供资源端口竞争仲裁，也不自动修正用户模型。

## 5. 基础组件及操作

下表中的 Method 名称描述组件语义。前端使用读取表达式、赋值、Rule 输入和 return，不要求显式调用 pop／push／set／get。

### Queue

```python
q = ac.queue[ac.u32](capacity=2, initial=[1, 2])
```

省略 capacity 时为 1；省略 initial 时为空，标量初值表示一个元素，序列初值表示初始 FIFO 内容。

| 组件 Method | 前端表达 | 语义／默认条件 |
| --- | --- | --- |
| peek | `q.value` | 读取旧队头，要求非空 |
| pop | 消费性 Rule 参数的实际 payload 读取 | 提出消费，要求非空 |
| push | Rule 返回 Queue payload | 提出写入，要求有空间 |
| empty | `q.empty()` | 查询旧状态是否为空，不消费 |
| full | `q.full()` | 查询同拍容量视图，不消费 |
| size | `q.size()` | 查询旧状态元素数，不消费 |

顺序为 peek → pop → push。成功 pop 可以为本拍 push 腾空间；新 push 数据下拍才能读取，空 Queue 不提供同拍数据旁路。

full 的内部 EHR、阶段和 push guard 由 Queue 契约提供，用户不声明内部状态或端口号。

Queue 不提供 revise，也不通过 `q.value = ...` 修改已有消息。寄存器更新使用 Cell／EHR。

### Cell

```python
state = ac.cell[ac.u32](initial=0)
```

| 组件 Method | 前端表达 | 语义 |
| --- | --- | --- |
| read | `state.value` | 读取周期开始的旧值 |
| write | `state.value = value`，或 Rule 的 Cell 输出 | 提出写入，获准后下拍生效 |

Cell 必须有初值，没有 FIFO 容量或消费语义。即使本 Rule 已提出 write，普通 Cell 读取仍为旧值。字段／数组元素赋值表达所属 Cell 的状态更新。

### Signal

```python
signal = ac.signal[ac.u32]()
```

| 组件 Method | 前端表达 | 语义 |
| --- | --- | --- |
| get | `value = signal` 或在普通值表达式中使用 signal | 隐式 guard 为 valid，读取本拍可见值，不消费 |
| set | `signal = value`，或 Rule 的 Signal 输出 | 提出本拍写入，使信号有效，保持资源身份 |

Signal 采用 BSV Wire 语义，不声明初值，没有容量或 pop。每拍开始为 invalid，本拍有效 set 后才可 get，下一拍重新为 invalid；valid 为 False 时 payload 不可使用。已声明 Signal 在赋值左侧时表示 set；新局部变量接收其 payload。`target = source` 在 target 已是 Signal 时读取 source、写入 target，不合并两个对象。

传给 Signal 类型参数或作为 Module 输出连接时保留资源引用，不转成 payload。

set → get 是严格同拍先后关系；同一原子事务不能对同一 Signal 同时 set 和 get，包含嵌套调用。set 与 set 冲突，get 与 get 无冲突。get 只读取前序获准写入，生产者失败，其 set 无效；不提供同事务临时写入旁路。

消费者通过 get 的隐式有效性条件依赖本拍有效写入；没有有效写入时 get 失败，不回退到上一拍值。Module 读取 Signal 的有效性条件沿数据与控制依赖传播到相关 Rule，不把 invalid 当作 False。RTL 中 data 与 valid 对应组合信号，仿真对象存储不代表硬件寄存器。

### EHR

```python
occupied = ac.ehr[bool](initial=False)
```

| 组件 Method | 前端表达 | 语义 |
| --- | --- | --- |
| read | `occupied.value` | 读取当前 Method 阶段可见的值 |
| write | `occupied.value = value` | 提出该阶段的写入 |

EHR 的端口契约对齐 BSV CReg。EHR 必须有初值，用户不手填端口号。编译器根据 Method 先后关系分配阶段；后阶段观察前阶段获准写入，多个前序写入时选择端口编号最大的写入。同阶段写入不可见，没有前序获准写入时读取旧值。

同端口 read／read 为 CF，read → write 为 SB，write → write 为 SBR；低端口所有操作到高端口所有操作为 SBR。同一外层事务不能访问同一 EHR 的不同端口，包含嵌套调用；不提供同事务跨端口临时写入旁路。Queue 内部 EHR 的 pop／push 阶段也遵循该限制。

没有顺序关系时不增加可见性端口。获准写入中端口编号最大的写入决定下拍保存值；没有获准写入时保持旧值。

### 默认契约归属

各基础组件的 guard、效果、顺序和可见性由编译器的组件定义集中提供。用户组件通过 Method guard、顺序声明和实际组件调用表达契约，分析器使用统一接口处理。

关系含义对齐 BSV：CF 表示无冲突；C 表示不能同时使用；SB 表示普通先后，允许同事务组合；SBR 表示严格先后，禁止同一外层事务实际路径上的组合。同拍顺序不规定竞争时的仲裁优先级。

## 6. 普通值、数组与控制流

- bool、定宽整数、结构体和 payload 数组表达普通数据；`ac.var[T]` 是普通值类型标注，不是寄存器声明。
- 普通局部赋值和表达式是组合计算，跨周期保存值必须使用状态组件。
- if、短路布尔表达式和循环保留实际控制／计算路径，不把状态读取无条件提前。
- 多个 Rule 使用同一 Module 计算时保留共享来源；这不承诺运行时缓存或跨 tick 复用。
- 固定 Queue 数组可使用 `ac.array(ac.queue[T], shape=(N,), ...)` 声明。形状在构造期确定，每个元素具有独立资源身份；运行时下标选择已有元素。
- 当前数组讨论限于固定一维资源数组。其它组件数组及更一般的数组表达未在本轮扩展定义。

## 7. 设计与当前实现的边界

本文规定前端编程范式，不定义运行时调度器、端口竞争仲裁或执行后端。与 ACIR 的对应关系见 [基础设计](../acir/basic-design.md)，分析和转化的目标见 [编译阶段的设计意图](../pycircuit/docs/compiler-intent.md)。

当前编译器使用原生 ACIR v7：Module／Rule／Method、guard 和返回槽位由专用操作保存；Queue、Cell、Signal 和 EHR 使用独立引用与组件操作。单出口分支和循环保存为 scf，多出口 return 保留 CFG。通用 C++ lowering 在副本上展开消费、输出和 Queue 容量契约，Python 分析器重建 RuleGraph；主 MLIR 不携带派生端口和完整分析图。v6 及更早产物需要重新编译，详见 [ACIR v7](../pycircuit/acir.md)。

本文的 Signal 已改为 BSV Wire 目标语义；当前 v7 实现的 Signal 初始化及读取契约尚未迁移。

EHR 作为 Rule 返回目标、普通共享值的跨实例引用及辅助数据操作的完整语法，本轮未进一步确定，不在本文新增规则。
