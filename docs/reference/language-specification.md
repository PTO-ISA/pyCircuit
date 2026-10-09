# pyCircuit 编程语言与编译执行规范

日期：2026-10-06。本规范描述当前单一源码流程及其实现边界。
“必须”表示可验收的行为合同；“当前未支持”表示实现缺口，不是硬件语言的
永久限制。语法、IR 表达能力、公开 codegen 和运行证据必须分别判断。
本次多 rule 修复的候选和实测证据见
验收记录 (historical local record)。

本文是从编程模型到编译、IR、双后端和验收的规范入口。表达式细则见
[源码语言参考](language.md)，[Enum](spec-enums.md)、[Queue](spec-queues.md)
及 [Table/collection](spec-collections.md) 是对应子规范。历史 RFC、旧工具名、
未执行的设计稿和测试文件的存在，都不能扩大当前支持范围。

## 目标与编译责任

pyCircuit 用 Python 语法描述有限硬件结构、组合值和状态事务。源码应使用
普通变量、命名 struct、rule 和 module 组合；作者不管理 current/next/proposal
对象，也不手动调用 Work/Xfer。框架必须按类型、SSA、声明身份、效果和
真实依赖处理设计，不得识别 example 名字、固定宽度或语句配方决定语义。

Python capture 只解析语法和位置，不执行设计函数。MLIR 负责名字/类型绑定、
写集合、连接、约束、实例关系和硬件降低。禁止在 Python、C++ emitter 或
Verilog emitter 中另建一套设计语义编译器。

公开流程是：每个源文件独立 `pycircuit compile`，使用完整显式单元集合
`pycircuit link`，再从同一个已验证 final artifact 执行
`pycircuit emit --target cpp` 或 `--target verilog`。接口提取不是先编译整个
系统再拆文件。消费者编译依赖已发布接口，不能偷偷读取 provider Python。

## 源文件、名字与声明

推荐 `import pycircuit as ac`。受支持的 from-import、别名和 facade reexport
必须按真实声明身份解析；同名、同字段布局或同物理宽度不能伪造类型/模块身份。
局部名字遵守词法绑定和已有遮蔽诊断，不得把被输入参数遮蔽的名字当全局 rule。

- `@ac.struct` 声明名义记录类型。
- `@ac.encoding(...)` 配合 Python Enum 声明名义编码值。
- `@ac.module` 声明硬件定义；静态调用 occurrence 创建实例。
- `@ac.rule` 声明模块作用域中注册的计算/事务；定义本身不执行。
- 普通局部表达式产生组合 SSA 值，不能因赋值自动增加寄存器。
- 模块作用域的受支持注解变量声明持久状态；rule 内同类注解描述局部值。

当前普通 typed-result module 调用要求完整运行时实参，不接纳一般静态/类型
实参。既有标准叶的类型参数和结构式 keyword-only 参数不证明任意依赖类型
elaboration 已实现。一般 Python 循环、递归硬件创建、任意 Python 执行、
设计内 host I/O 均不是当前可执行源码合同。

## 标量类型与表达式

| 类别 | 语义 |
| --- | --- |
| `ac.bits[W]`、`ac.u1`…`ac.u64` | 静态正宽度、无符号固定位向量；别名不改变类型语义 |
| Boolean | 逻辑值；条件允许 Boolean 或具有 fixed-bit 身份的一位值 |
| 有限数学 Integer | 精确整数及可证明范围；不因物理宽度相同就变成 bits |
| Enum | 名义类型与明确编码；成员与原始 bits 之间须经受支持转换 |
| struct | 按字段声明顺序组成的名义值；可作为完整数据或状态 |

bits 加减乘在操作数宽度内模运算。扩宽目标不会追溯改变已经发生的运算：
需要更宽中间结果时，先显式扩宽操作数；缩窄必须显式切片。Integer 的精确
计算与 bits 的模运算不是同一合同，不能以“统一位宽”为由静默混淆。

当前受支持的 bits 操作包括位运算、比较、选择、静态切片、concat、静态移位、
popcount、首尾零计数、优先/one-hot 编码，以及满足条件的静态除数除余和同宽固定 bits 运行时除余。
静态移位不按宿主机器字长取模；移位量大于等于宽度产生零。
固定 bits 运行时除余保持组合时序；除数为零或任一操作数含X/Z时，商和余数均为全X，
与通用四态RTL算术一致。ISA/consumer特例在对应wrapper中表达。数学Integer运行时除法、一般动态移位
与一般 signed 算术仍有实现缺口。多拍ready/valid除法器与普通算术表达式分开。

切片 `[low:high]` 以 bit0 为最低位，须证明 `0 <= low < high <= W`。
`concat(high, low)` 的自然宽度是各操作数宽度之和。切片/拼接保留被传输的
value/known/Z 位；算术与选择按各操作的四态合同处理。不能把两态优化恒等式
直接套在可能为 X/Z 的位运算上。

条件表达式、`if` 和 `match` 使用一致的条件身份检查。分支继承相同入口值；
未赋值路径保留入口值及已有写使能。新局部值在使用前必须确定已绑定。
`match` 按首次匹配选择，逻辑覆盖全部编码不能删除物理 X/Z/非法编码的回退。

## Struct、默认值与不可变值

Struct 字段有序、名义化，当前字段可为 bits、Enum 或嵌套 struct。首字段占
打包值最高位，字段间无填充；不能依赖宿主 C++ 对象对齐决定硬件布局。

同源构造使用字段关键字。显式值覆盖声明默认值；遗漏且没有默认值的字段
递归置零。嵌套字段未声明默认值时取递归零，而不是自动应用内层类型的非零
默认值；若需要内层默认值，应显式写 `inner: Inner = Inner()`。
所有声明默认值都必须静态合法，包括后来被显式覆盖或未使用的默认值。

局部 `value.field = replacement` 重建该局部的 SSA 值，之前保存的值保持快照。
它不分配存储，也不修改另一个普通局部副本。持久变量作为 rule 的直接实参时，
对相应 formal 的重绑定/字段赋值形成该声明的状态更新意图。
普通别名副本、字段投影和表达式实参不会自动变为可写状态引用。

导入 struct 的注解、值传输和名义类型核对已有支持。普通表达式和带返回注解的
rule 可按已发布的名义字段显式构造导入 struct，所有字段必须完整提供；
provider 默认值不作为导入权威。导入类型的静态初始化和 Table 查询构造回调
仍采用原有的较窄准入边界。
Struct 内一般数组/Table、任意递归布局仍未实现。此处必须诊断，不能复制类型、
改成不透明 packed bits 或读取 provider 源码来冒充完整支持。

## Table 与 collection 的不同含义

`ac.table[N, Entry](init=0)` 是当前 Python 的一维固定状态表分配形式。
`init=0` 对每个叶字段递归置零，忽略 struct 的非零默认值；`init=Entry()`
使用该构造值初始化各个独立元素。索引须有0≤lower<upper≤depth的半开区间
证明，或已有完整物理位宽证明。字面量及固定bits对正静态Integer取余可提供范围；
alias/rule capture保留事实，后续wrapping算术不能沿用旧范围。范围不是常量/known-bit
权威，比较保留完整index的高X/Z位，运行时除数不在此推导范围。

公共 IR 的 `!ac.table` 是聚合值类型，本身不代表存储；`ac.collection` 是
同一模块定义的静态实例集合，元素拥有独立状态。当前 Python 状态表可降低为
标准存储叶的 collection。不能把二者等同于 Python 模块对象数组。

IR 已有 create/splat/map/view/index/get/match/choose/fold 等操作，采用声明的
静态形状和布局；不因此宣称 Python 已支持一般数组 map/scan、模块 collection
作者接口或 Table-in-Struct。新能力须贯通源码、类型验证、公共 IR 和双后端。

同一一维Table现在支持不同直接Integer字面量元素、或不相交Struct字段的多个writer。
元素可以是Bits、Enum或递归Struct；动态同字段写入需证明显式grant下地址分离，
或whole-owner互斥。完整候选通过
已有TableMap与ValueMerge合并，parent字段先展开到唯一scalar leaf；owner仅绑定一次，
保留原rule-owner enable及未知poison。所有规则读old-Q，不按注册顺序覆盖。

索引范围与写域分离分别证明。早期capture只识别直接Integer字面量；lowering可以
从实际SSA证明不同的闭合常量，包括0+2和别名2。动态索引必须使用真实赋值时SSA，
检查全部相交路径：两个owner同时known1时，ordinal与各索引的完整宽度比较不为
known0的可能地址集合必须不相交。源码可以显式写
`gb = req_b & (~ga | (i != j))` 或等价select，框架不自动选winner。
不同动态宽度的额外等价推导、算术/范围独立性、隐式查询provenance、内部字段条件
排斥仍不作证明。整Table替换需owner互斥；带索引的整元素替换可以使用地址证明。
Table-of-Table、Table-in-Struct和第二层索引仍未实现。

Table 查询使用 `index, valid = entries.first(where=lambda entry: ...)` 或
`entries.argmin(where=lambda entry: ..., key=lambda entry: ...)`。receiver 必须是
正的闭合一维 Table，回调是一个参数的纯表达式 lambda；结果原子绑定到两个不同的
普通名字。index 为 `bits[max(1, ceil(log2(N)))]`，valid 为 `bits[1]`，无匹配时为0/0。
known-value 范围不代表索引没有 X/Z，也不代表命中。

first 选择最低 ordinal。argmin 的 key 必须是同一权威 unsigned bits 类型；相同已知
key 取较低 ordinal。四态语义固定为相邻两两合并、奇数尾项原样传递的树：
`take_right = right.valid & (~left.valid | (right.key < left.key))`，valid 用 OR，
key/index 用该条件选择。单个已知可选项的 key 即使未知，也保留该项的 ordinal。
叶 valid 规范化 Z→X；未知竞争可能产生 valid=1、index 含X的结果。

回调在调用点读取 receiver 与 capture 的 SSA 快照，允许受普通索引证明约束的跨 Table
读取。禁止副作用、嵌套查询、分配、module/rule 注册和 instrumentation，即使出现在
死分支。普通命名 def 回调仍不支持。前端复用既有 scalar lowering，将读取边界分成
紧凑 map/view/splat/get 阶段；不逐行复制回调。跨 N 行读取 M 项仍有 N×M 逻辑成本，
须在构造前计入源码级预算。查询不增加存储、时序、消费、仲裁或读 reservation；
谓词互斥不能自动证明含X的编码索引地址集合互斥。

## Rule 注册、局部计算与状态写入

同owner重叠写入需证明owner互斥，或同时known1时实际索引地址域分离。采用真实SSA/capture/
yield身份与有界必要事实，支持常量、NOT/AND/OR、四态select和typed常量比较；
未证明及资源耗尽拒绝。源码显式grant决定协议策略，框架不自动选winner/消费/取消。
条件字段加无条件sibling仍是enabled owner，不能拿内部字段条件假装owner互斥。

捕获分析可以产生pending pairs并报告等待SSA证明；plan成功不等于完整compile成功。
所有module pending必须在proposal绑定前完成证明，并在源整体发布前核对ledger。
缓存plan不可变，CompilerDev接口改为isPlanValid且不保留旧isValid别名。
候选选择使用domain AND original-enable，防止disabled候选用old-Q覆盖active更新。
p&~p的二态不可能性仅用于admission，不能优化掉X/Z下的未知enable与失败行为。
地址mask由原有TableMap生成，私有Rule结果放在owner pairs之后、检查后缀之前。
原owner enable及poison不从mask重新归约。select的未知selector与两个相同已知分支
同样参与证明；不改写实际运算。missing witness和分析失败不能降格为保守成功。

同一模块可以注册多个纯计算 rule 和多个状态写入 rule。状态规则可以自然地
以语句调用，无需为了注册而构造一个无意义结果：

```python
import pycircuit as ac

@ac.struct
class Counters:
    left: ac.u8
    right: ac.u8

@ac.rule
def step_left(state, enable) -> None:
    if enable:
        state.left = state.left + 1

@ac.rule
def copy_left(state, enable) -> None:
    if enable:
        state.right = state.left

@ac.module
def TwoCounters(left_enable: ac.u1, right_enable: ac.u1) -> Counters:
    state: Counters = Counters()
    step_left(state, left_enable)
    copy_left(state, right_enable)
    return state
```

此例两个规则写不同字段。两者都读取该 epoch 的旧 state，因此 right 得到旧
left；交换两条注册语句不会把更新后的 left 隐式前递给第二个规则。

在一个 rule 内，赋值按源顺序更新局部值，后续表达式可使用其计算结果；这些
局部值不是已经提交的状态。显式 rule 返回值可以连接后续计算，因而建立真实
组合依赖；不得把这种显式连接与隐式状态前递混淆。

当前 rule 注册使用普通必需位置参数；一般关键字/default/可变参数尚未完整
接纳，不应据此把它们永久排除在语言设计外。

无返回值、末尾裸 `return`、受支持的 `return None`/`-> None` 规则具有零个
源码输出。带类型结果的语句调用可以忽略其返回值，但不能丢弃其状态更新或
检查。void rule 不得用于需要值的表达式；`-> None` 不得返回非 None 值。
无返回注解但实际返回值的既有规则保持其原合同。一般早退/条件 return 和
rule 内注册其他 rule/module 不在本次扩展范围。

## 写集合、冲突与合并

写目标身份为持久声明及静态字段路径，不是 formal 名字，也不是结果值是否
变化。不同规则可以写不同声明；同一 struct 的兄弟字段、嵌套兄弟字段可合并。
未写字段必须保留旧值，其他规则的字段更新不能被整记录重建覆盖。
`entries[index].field` 也只更新目标字段或子树：即使 index 为 X/Z，其他字段
仍保留本 rule 当前候选中的值，包括此前赋值。显式 RHS 的未知索引读取保持原有
全 X 读值语义；整元素赋值仍替换整个元素。索引比较只控制数据选择，不能取代
原 owner enable 或改变未知控制失败的行为。

| 两个注册的写目标 | 当前结果 |
| --- | --- |
| 不同状态声明 | 允许 |
| `state.left` 与 `state.right` | 允许 |
| `state.header.tag` 与 `state.header.valid` | 允许 |
| 同一字段，包括 `x = x` 的原值回写 | 必须证明 whole-owner grants 互斥 |
| `state.header` 与 `state.header.tag` | 必须证明 whole-owner grants 互斥 |
| 整个 `state` 与任意子字段 | 必须证明 whole-owner grants 互斥 |
| 同一 Table 的不同直接字面量元素，或不相交 Struct 字段 | 允许 |
| 同一 Table 的其他重叠写路径 | 必须证明 whole-owner grants 互斥，或显式grant下可能地址集合分离 |

同一 rule 内的重复赋值仍按顺序计算，不是跨 rule 冲突。跨 formal 的歧义
可写别名仍拒绝。重叠 writer 必须经通用 SSA 分析证明owner互斥或对应索引域分离；
无法证明或分析预算耗尽时拒绝。不自动推导调用顺序优先级。
诊断保留写语句及对应注册位置；当前公共 fatal 打印器不显示附加注册位置 notes，
直接 MLIR diagnostic 接口仍保留这些位置。

每个 rule 对每个 owner 保留既有的有效写使能。合并不同 rule 时，已知激活的
writer 不能遮蔽另一个 writer 的 X/Z 使能。其控制失败必须经现有存储检查导致
整个 epoch 丢弃；数据本身为 X/Z、使能已知时仍允许存储。

必须保留单 rule 原有语义：同一 rule 中，未知条件字段更新后又无条件更新兄弟
字段，可以形成已知 owner 使能和带 X 的字段值，不能因多 rule 修复而改成失败。
未知条件是该 rule 唯一更新时，其未知 owner 使能仍按现有存储合同处理。

## Module、组合连接与独立实例

普通 typed-result 调用创建静态实例，返回值表示连接值。两次调用即两个实例；
多次使用一次调用的结果只是 fanout。嵌套表达式中的调用与定义体中的子模块
层次须按现有 occurrence 规则解释，不能因包装层或源代码顺序增加寄存器。

已支持的跨文件调用使用完整已发布声明，包括源参数身份、返回形式和隐藏域
映射。链接必须核对声明与实际 provider body。前向连接只对唯一、未被修改的
模块/队列结果等已明确接纳的形式有效，不是任意 Python 前向读取。

同一前端也接纳语言参考列出的部分显式标准叶结构式源码；它不是另一个
编译流程，也不能代替普通变量和隐藏域作者体验的统一。新例子应优先使用
普通 typed module/rule 形式，不能把底层接线方式包装成已完成的 Pythonic 接口。

组合依赖必须经过公共分析。状态 Q 构成时序边界；真实组合环必须报错，包括
未使用但仍构成非法硬件图的连接。编译器不能插入隐式延迟来隐藏组合环。

## Queue 与内存

`ac.queue[T](valid, data, take, depth=N)` 分配完整 token FIFO，
返回 ready、valid、data。默认 ready 仅依赖本地容量；显式
`ready_policy="downstream_pop"` 才允许满队列同边沿取出并替换。没有空队列直通。
latency 是静态正整数；等待中的 token 也占声明容量，不能偷偷增加槽位。

队列的时钟、复位和内部提案由框架管理。源作者连接数据与握手；非法环、未知
有效控制、复位和丢弃采用当前 Queue 子规范，不得从 FIFO 名称猜测策略。

标准 memory 叶已经存在，但统一的 Pythonic memory 组合尚未完成。同步读采样
写入前的数据，字节使能选择写入通道；reset 与清空 RAM 不是同一行为。
不能用会清空内容的寄存器表替代保留 RAM 的 memory 来通过迁移。
标准叶 IR/runtime 能力不等于对应 Python 便利接口已经交付。

## MLIR 分析与公共硬件 IR

源码单元编译必须执行以下职责：

1. 在既有 `ac.python_capture` 上执行注册的 `ac-analyze-rule-writes`。
   保留实际声明/注册 occurrence、写路径和原值回写意图，诊断重叠。
2. lowering 消费 MLIR AnalysisManager 缓存的同一分析结果，完成类型边界、
   条件 SSA 和 rule 提案。分析不能是与实际生成无关的装饰性 pass。
3. 多 writer Struct 复用既有 `ac.value.merge`，按声明字段顺序合并候选。
   其既有 enable-OR 合同不改变；存储连接额外保留跨 rule 未知使能传播。
4. 执行已验证的 record wire 简化、接口提取、接口/body 一致性核对。
5. 显式链接完整 closure，验证实例、类型、依赖、时序边界及最终硬件包。

这个写分析 pass 的输入是 capture，不是任意已经简化的硬件 IR。最终纯 SSA
可能已经消除了 `s.a=s.a`，不能声称能从它恢复所有源码写意图。写集分析也
不能代替字段类型、静态范围或完整源语法验证。

`ac.module`/`ac.instance`/`ac.rule` 表达定义、实例和显式输入输出的纯计算；
存储由标准叶拥有。`ac.struct.create/get`、Table 操作和 `ac.value.merge`
表达聚合数据流。IR 中 `!` 引导类型，`%` 引导 SSA 值，`@` 引导符号引用；
这些是 MLIR 表示法，不是 Python 作者接口。

`pycircuit-opt` 注册 canonicalize/CSE 供明确实验；注册不等于已加入默认硬件
编译流程。特别是 DCE、四态代数化简和检查操作的可达性，必须经过硬件语义
验证，不能以普通 MLIR verify 成功替代。

## Codegen、运行与原子提交

C++ 和 Verilog 必须消费同一 verified final artifact，保留源文件所有权与
对应产物名称。backend 实现公共操作语义，不重新猜测源码的写集合、优先级、
实例关系或数值范围。不得增加 design 专用 emitter 分支。

生成 C++ 使用现有 gfsim Runtime；生成 RTL 使用相同语义的标准叶。Runtime
包不依赖 LLVM；CompilerDev 使用当前精确 LLVM/MLIR 工具链要求。

一个 runner Step 是一次 Work/Xfer 采样 epoch，不自动等于完整时钟周期。
驱动器提供输入和物理时钟/复位电平，并给出有限运行上限。Work 读取已提交
状态并准备候选；全系统检查成功后才 Xfer。失败必须丢弃所有候选及候选时钟
历史，不能只回滚触发失败的 rule 或只回滚数据。

rule 内构造的输出可能是组合计算结果；返回持久变量表示旧状态值。Xfer 不会
重新计算本次已经采样的输出。typed DUT 的 sample 在第一次成功 epoch 前和
失败后必须拒绝。Host Reset、时钟边沿的 reset 信号和重新构造 DUT 必须区分。

## 检查、观测与当前缺口

源码 assert/observe 的 capture 和部分内部分析/执行能力存在，但完整公开
双后端 instrumentation 尚未闭环；当前公开 emission 对未实现能力必须诊断。
不能引用内部测试 emitter 的通过来宣称公开产品已经支持完整 @system/EXPECT。

| 尚未完整实现 | 不得采用的替代验收 |
| --- | --- |
| 导入 struct 构造/默认值、聚合内数组/Table | 在每个文件复制类型，或手工 packed bits 替代完整类型支持 |
| 一般静态参数与可复用生成 | 仅硬编码一个尺寸、手工展开一个固定设计 |
| 更一般规则/索引冲突证明 | 依靠调用顺序覆盖、丢弃任一 writer 或将所有事务强制合并 |
| Pythonic memory 及资源调度组合 | 换成不同 reset/容量/延迟的硬件模型 |
| 完整 source instrumentation、自动域调度/CDC | 只检查 capture，或只跑没有相关功能的例子 |
| 任意后端宽度/规模下的性能 | 仅引用类型可表达性、测试数量或单一小样例运行时间 |

## 简洁性、效率与验收

重写必须同时满足行为正确、源码清晰、通用复用和可解释的效率。复用现有纯
rule/module、struct 默认值和聚合状态；不能把前端缺口转嫁为长参数列表和
重复展开。具有相同时钟/reset/enable 的状态可聚合；不同 enable 的状态必须
保留区别。旧源码的寄存器实例数量不自动成为不可改变的语义要求。

性能报告必须区分源码重复、IR/RTL/C++ 体积、编译时间和模拟时间，说明输入、
优化级别、工具链及采样方式。局部改善不能扩大为所有工作负载的保证。

独立验收使用 [Current limitations](../development/known-limitations.md)；
审查者任务见 [Current limitations](../development/known-limitations.md)。目录、
测试名、设计审阅或“已注册 pass”都不是执行成功证据。验收必须绑定候选内容，
保存具体命令、退出码、独立 oracle、实际双后端结果及未覆盖项目。
