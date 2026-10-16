# 串行 Rule 与 Method 源语言合同

本文件是[目标扩展规格](README.md)的规范性部分。所有“目标源码”都需要新实现与
独立验收，不表示 main 已接纳。基础范围只覆盖一个共同物理 clock/reset 域。

## 概念与调用关系

| 概念 | 唯一职责 | 允许的行为 |
| --- | --- | --- |
| Module | 静态结构、状态所有权、组件边界 | 声明状态/子实例、连接、定义并注册 Rule、声明 Method |
| Rule | 自主原子事务入口 | 纯计算、owner 更新、Method 调用、既有检查/观测 |
| Method | 现有组件实例上的操作 | 纯计算、修改本 owner、调用本实例或子实例的 Method、返回普通值 |
| 普通 `def` | 有限纯计算复用 | 参数到结果的计算，不隐式读取 owner，不调用 Rule/Method，不分配硬件 |

`@module`、`@rule`、`@method` 及其合规 `pycircuit` import alias 都属于一个前端。
不添加新的前缀、版本化装饰器或运行 Python 设计的解释器。
`@system` 保留既有生成模拟闭包责任；本扩展不把时钟、复位或 testbench 脚手架转给作者。

Rule → Method → Method 共享唯一外层事务。普通 `def` 可被这些函数调用。
Rule/Method 内不得调用自主 Rule，也不得实例化 Module。需要共享操作时定义 Method
或纯 helper；需要独立的跨拍执行时通过明确的硬件状态/连接表达。

## Module 声明和实例

**METHOD-INSTANCE：**保留现有 `Child(inputs) -> Result` 的普通 typed-result 调用含义。
声明中直接包含 `@method` 的模块是 Method owner；首批要求它没有 authored runtime
参数、没有普通 module return，也不引入新的静态/类型参数能力。`owner = Owner()` 在
模块静态体中构造一次实例并绑定句柄。声明的已发布形式决定调用结果种类，不能只看调用
是否带参数猜测 instance/value。

Method owner 内可声明普通持久变量、现有固定 Table、纯局部常量、静态子实例和嵌套
Rule/Method。首批不允许一个 Method owner 同时带普通数据端口/返回值；这是明确的
有界 admission 限制，不是另一个编译路线。parent 仍可通过 total Method Rule 返回
typed 数据，其普通接口不改变。

嵌套 `@method` 的名字是实例的公开操作。普通嵌套 `def` 不自动公开。
句柄不是硬件 payload，不能放入 Struct/Table、作为 module 输出、跨文件全局共享实例、
传入任意值参数或在运行时选择。实例局部不可变别名保留同一 owner，不能复制状态。
无 Method 的既有模块、标准存储叶和直接组合规则继续服从原合同。

## 基础目标源码

以下代码定义目标语法和行为，可作为后续正例，不是当前运行示例：

```python
import pycircuit as ac


@ac.struct
class CounterView:
    before: ac.u8
    after: ac.u8


@ac.module
def Counter():
    count: ac.u8 = 0

    @ac.method
    def read() -> ac.u8:
        return count

    @ac.method
    def add(delta: ac.u8) -> ac.u8:
        count = count + delta
        return count

    @ac.method(guard=lambda amount: count >= amount)
    def take(amount: ac.u8) -> ac.u8:
        count = count - amount
        return count


@ac.module
def Parent(delta: ac.u8) -> CounterView:
    counter = Counter()

    @ac.rule
    def advance() -> CounterView:
        before = counter.read()
        after = counter.add(delta)
        return CounterView(before=before, after=after)

    return advance()
```

初始化完成、旧 count=5、delta=2 时，Work 的结果是 before=5、after=7。
一次合法有效上升沿且全系统检查成功后，count 提交为 7。held/falling clock 的 Work
可以计算候选结果，但不会因此提交或增加一个周期。不同物理时钟的事务不在基础范围。

这个例子不意味着 `take` 可以直接成为普通 module 输出：guarded 返回值边界见下文。

## Rule 身份、注册与运行选择

**RULE-REGISTRATION：**基础 Rule 身份为 Module 实例与 Rule 声明身份的组合。
每个 Rule 声明在其模块内必须恰好注册一次；未注册声明和同声明重复注册均诊断。
注册可以是模块体语句、结果赋值或直接 module return 中的一次调用。
注册的实参绑定为连接，并不在 capture 时运行函数。

首批不接纳循环注册、运行时条件注册、多个互斥位置合并注册、Rule 递归或嵌套 Rule 调用。
运行选择放在 Rule guard 或 body 中。不同实例中同一 Rule 定义形成独立事务身份。
交换独立 Rule 的注册顺序不得改变可见状态、仲裁或周期行为。

普通必需位置参数及其现有 owner/formal 绑定继续适用；新增 kwargs/default/可变参数
不由本扩展自动开放。Method 的参数和返回值必须显式类型化，`-> None` 表示无值操作。
一条 Rule 中可多次调用同一 Method，但必须通过资源和候选状态分析。

## owner 绑定、顺序与快照

**METHOD-STATE-ORDER：**Method owner 模块作用域的持久声明是唯一状态 owner。
其嵌套 Rule/Method 对该名称或字段/元素的赋值表示候选更新，不需要也不允许 `nonlocal`。
参数、局部声明和其他绑定不得遮蔽捕获的 owner 名字；其余新局部按普通 SSA 值处理。

同一外层 Rule 内，Method 按逻辑调用顺序接收该 owner 的最新候选版本。读取在此前的
更新之后发生，就读取该候选版本；保存过的普通局部不随 owner 后续更新改变。
Method 对本模块其他 Method 的直接声明名调用，与 `instance.method(...)` 一样归入
外层 Rule。它只能直接修改自己声明模块的 owner；访问其他模块必须经过其公开 Method。

不同 Rule 从相同旧 Q 开始。普通变量不提供跨 Rule 同拍前递、源码优先级或自动仲裁。
跨 Rule 写重叠必须经既有的通用互斥/地址分离证明，否则拒绝。

首批不增加 `cell`、`signal`、`ehr` 或 `var` 类型。普通变量服从上述候选语义；不能同时
承诺“同一名字写后读候选”和“写后始终读旧值”。显式旧值用提前保存的普通局部表达。
原有 memory/Queue 叶仍按自身的同步读、旧队头、容量及延迟规则工作。

**METHOD-ATOMICITY：**Method 返回不表示物理提交。所有 owner 候选、队列传输和观测
都属于同一个外层事务。某个必需调用不获准时，该事务无提交效果；其他合法独立 Rule
仍可发射。所有 Rule 的实际候选最终仍受全系统 Precheck/Precommit/Xfer 约束。

## guard 的唯一首批拼写

**METHOD-GUARD-ENTRY：**只接纳裸 `@rule`/`@method` 或
`@rule(guard=lambda ...: expression)` / `@method(guard=lambda ...: expression)`。
裸形式的 guard 是 True。lambda 的形式参数必须与被装饰函数的必需位置参数同名同序；
无参数函数使用 `lambda: ...`。不接纳其他 decorator 参数、直接布尔表达式、字符串或
任意函数对象作为 guard。

guard 自由名字在定义位置解析身份；普通捕获值固定为当时的词法 SSA 绑定。
owner 读取在每次调用入口使用该事务当时的候选版本。guard 先于本次 body 求值，body
后续写入不能反向改变此次 guard。Rule 自身 guard 使用该 Rule 的旧 Q 入口版本。

guard 只允许已准入的纯标量表达式和有限纯 helper；不得调用 Method/Rule、分配资源、
提交效果、记录观测或消费 Queue。返回类型必须是 Boolean 或 authoritative fixed u1。
普通 Integer 0/1 不因物理宽度成为 predicate。

**METHOD-GUARD-PATH：**实际被选择的调用路径才提出 guard 要求。已知不执行的分支
不阻塞 Rule。某个必需 guard 已知为 false 时，整个外层 Rule 不发射；这不是 runtime
failure，也不是进入其 else、执行另一分支或保留先前 Method 部分更新的许可。

暂不可执行与普通状态返回不同：`success`、`accepted`、`retry`、`valid` 等字段都只是
普通值，名称不赋予任何编译语义。Method 可以返回 retry 并修改游标，调用者可以继续
执行其他操作。不能自动把成功条件提升成 guard。

### 可选操作和前缀

ROB 的完成、退休、分配可各有显式选择条件。退休未发生不必取消完成；不能把现有
可选操作机械改成三个无条件的 guarded 调用。应通过普通结果或显式条件选择表达。

批次全部成功与成功前缀是不同协议。需要前缀时，作者必须明确计算/选择该前缀的效果，
再在事务中提交；不得把第 N 次 required guard 失败解释为前 N−1 次已经提交。
不支持的带效果循环应诊断，本规格不因这一算法需求自动开放新的循环语法。

## 四态、检查和物理域

**METHOD-FOUR-STATE：**普通计算、赋值和分支沿用现有四态值/known/Z 语义。
`and/or` 不新增 Python 短路效果。当前 bounded statement `if` 支持 whole-Bits state、
scalar locals 及同 nominal Enum/有限 Bits/Enum/Struct 局部值合并；含 Table 的改变 join
仍拒绝。statement arms 可调用 Method 并包含直接 body effects，按
[条件合同](conditional-effects.md)增加路径与 selector-known 准入义务；纯数据分支保留
四态合并。局部字段重建不开放 partial/aggregate persistent state。
纯函数参数按源码顺序绑定一次，但这不人为串行化无关组合硬件。

基础 Method 事务的外层 guard、实际调用路径及 required guard 必须能在需要作出选择时
得到确定结果。有效上升沿、非 reset、仍可能发射的事务遇到未知有效 Method 调用控制
或 required guard 时，必须经 managed check 导致全系统零提交，不得把 X/Z 当 false。
先前已知为 false 的支配路径/guard 不激活其下属调用检查。纯局部数据为 X/Z 不因此失败。
对于有效控制已知的普通字段更新，继续保留基线允许的 X 数据和既有 owner-enable 合并行为。

是否到达一次调用边界只由已建立的支配路径和先前 guard 决定，不能用最终整体 fire
反向屏蔽此前必需的未知控制检查。先遇到 X guard、后遇到 false guard，结果仍是硬错误；
先遇到 false guard、其下后续调用的 guard 为 X，则本 Rule hold 且不产生该下属新错误。
外层 Rule guard 的 X 同样需要检查，不能被 body 中稍后的 false guard 遮蔽。

reset 保留原有优先级，并禁止被 reset 取代的 Method 状态操作触发新的准入控制故障。
held/falling clock 不提交状态；新增状态操作的控制检查以其实际采样域为准。
首批事务涉及的所有效果必须属于同一 clock/reset 域；未知域或跨域组合拒绝，不插入 CDC。
外层 guard 和新增调用控制检查本身也构成采样域义务，即使没有状态写入也不得省略。
通过既有隐藏域推导/明确结构连接绑定唯一域；无法确定其实际采样域时必须拒绝。

编译期类型、结构、非法索引等诊断不能因运行时 guard 不发射而省略。
新增事务内运行时断言/观测按最终发射和有效路径门控；为计算准入本身必需的控制检查
必须先完成，不能用依赖该检查结果的 fire 把错误遮蔽。既有非事务源码检查行为不被改写。
任一激活的硬错误仍取消整个 epoch，包括其他 owner 和所有候选时钟历史。

## 返回值与有效性边界

**METHOD-RESULT：**Method 返回普通 typed 值，不创建 Queue、寄存器或新事务。
首批所有 Method 调用，包括只读 Method，都必须位于 Rule 的调用链内。
不接纳 module Work 中直接调用 Method、保存 Method 对象或间接调用。

**RULE-RESULT-VALIDITY：**保证每次求值都有结果的 total Rule 保留现有普通返回值。
只要 Rule 有不能证明恒真的显式 guard，或可能调用不能证明恒真的 guarded Method，
首批必须声明 `-> None`，不得把结果逃逸为 module 普通输出。Method 返回值可以继续
在本事务内参与计算，但不绕过这一边界。

恒真证明只使用已接纳的静态常量事实及经验证的调用摘要，不能以初始化值、某次测试
输入或 runtime interval 假装恒真。不能自动补零、使用上拍结果或增加持久 valid 状态。
在明确有效性接口开放前，失败 Rule 的 payload 不得进入不受保护的普通组合输出。

`None` 在基础扩展中只表示无值返回；不表示省略某个 Queue 输出、撤销整个事务或
隐式创建资源效果。资源输出和混合 tuple 返回不由本规格增加新含义。

<a id="pure-helper"></a>

## 纯函数与现有接口迁移

**PURE-HELPER：**普通 `def` 复用现有类型、表达式和有限控制流，不建立新的 Python
解释能力。首批使用显式类型化的必需位置参数和返回类型，body 接纳普通局部绑定、
不可变 Struct 字段重建、已准入的表达式、`if/else`、`pass` 和一个末尾 return。
两支局部绑定服从现有 definite-assignment 规则；无值 helper 使用 `-> None`。
标量运算、构造、投影、Table 表达式及其范围限制逐项继承当前语言合同，不因进入 helper
而获得更广的 admission。helper 内有限字面量循环与不可变局部 Table 更新复用现有
bounded-source-loops 合同，不增加周期或效果；不接纳直接嵌套循环、分支早退、嵌套定义、可变参数、
异常处理、host I/O 或新的初始化求值器。需要这些形式时单独扩展语法与证据。

调用图必须无环、无硬件分配、无状态效果、无隐式 owner 读取。owner 值通过参数显式
传入；不可变普通词法 SSA 可以捕获。helper 只能调用已验证的纯 helper/既有纯 intrinsic。
跨源 helper 通过发布的 typed 定义/接口和明确闭包解析，不能回读 provider Python。
签名/调用解析、跨源摘要和展开预算与 Method 共用现有 owner，不另建 helper 编译器。

main 的一些 `@rule` 被用于纯 scalar helper。目标语义不把它们静默变成事务。
最终切换必须把保留的纯 helper 定义及所有相关调用者迁移到普通 `def`，保留数值和
时序行为；真实 owner 更新保留 Rule 或提取 Method。不得仅凭是否有 return 分类。
部分实现期间当前语言继续按当前合同工作；完整切换候选不得遗留两个隐藏解释或兼容别名。

普通 typed module 返回值、标准 Queue 握手接口、Struct/Table、现有系统执行、源文件
所有权和 Runtime API 均保留。新增接口不能要求旧的合规设计悄悄换周期、状态或 reset
语义；确需源码迁移时必须同时列出定义、调用者、删除范围及独立等价证据。

## 拒绝边界与诊断内容

以下均须在替换已发布输出前失败，并携带相关声明/调用/owner 位置：

- Method owner 的混合普通端口/结果、动态构造、句柄值化或不支持的参数化。
- 捕获 owner 被遮蔽、直接写另一个实例私有状态或无法判定的别名。
- 自主 Rule 嵌套调用、未注册/重复注册、动态注册或递归。
- Method 在事务外调用、guard 有效果、签名不匹配或不能证明 total 的结果逃逸。
- 不可证明的跨 Rule 冲突、超出资源端口、组合/准入依赖环和跨域事务。
- 不支持的源形式、预算耗尽或无法完成接口权威核对。

诊断类别是验收合同；具体稳定诊断码由实现任务按现有诊断 owner 分配。
不能用“unexpected operation”或 emitter 崩溃代替源语义错误。
