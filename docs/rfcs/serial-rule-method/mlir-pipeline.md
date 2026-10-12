# MLIR 分析与变换合同

本文件定义[串行 Rule/Method 扩展](README.md)的目标编译责任。
下列阶段是规范性职责，不代表同名 pass 已经存在，也不要求每项单独成为可执行工具。

## 当前边界与需要改变的地方

`compiler/lib/Compiler/SourceUnit.cpp` 当前组织写分析、绑定推导、source lowering、
record wire 简化及克隆后的接口提取。`SourceRuleWritesAnalysis` 保留包括原值回写在内
的源码写意图；`SourceMemoryBindingsAnalysis` 当前还持有准备好的 importer context。
`PythonImportRuleCall.inc` 和 `PythonImportComposition.inc` 构造并合并候选值/使能。

最终 `ac.rule` 是 Pure 的组合计算，显式 captures/results 本身不携带完整的
Method 效果、资源请求、guard 或事务身份。扩展必须在这些信息消失以前建立可验证边界。
不得对最终纯 SSA 猜测已经丢失的源码写意图；不得仅把私有 importer 函数包装为多个
pass，却仍让每个 pass 独立解释 Python AST 或依赖不可检查的隐含状态。

## 必须保留的语义信息

从绑定完成到事务降低之间，操作或其正式可验证属性必须表达以下信息。
下节固定首个实现增量的 ODS 名称，落到现有 ACIR 中；不得另建语义编译器或另一个前端。

| 信息 | 必须能够验证的内容 |
| --- | --- |
| 定义与 occurrence | Module 定义、实例路径、Rule 注册和 Method 调用位置分别有稳定身份 |
| owner 引用 | 资源的实际实例、声明、字段/索引路径；别名不复制状态 |
| 调用 | callee、typed 实参/结果、调用者 Rule、源位置和静态调用次序 |
| 状态访问 | 读/写、旧 Q 或事务候选版本、准确写域、原值回写意图 |
| 路径 | 执行条件、值有效条件、guard、错误检查路径分别保留 |
| 效果 | owner 更新、资源消费/生产、检查、观测和资源端口需求 |
| 资源契约 | 容量、延迟、可见性、reset、端口及已支持的策略 |
| 事务 | 唯一外层 Rule、请求集合、获准条件以及每项效果的归属 |
| 来源 | source owner、声明/调用/写入位置和发布接口身份 |

有状态效果或隐含资源要求的操作不得标为 Pure。普通 MLIR memory effects 接口
可以参与基础分类，但其通用读写集合不能替代本表中的 guard、事务和资源语义。
操作 verifier 检查局部结构；闭包 verifier 检查真实 owner、跨调用关系和完整效果集合。

## 类型化效果操作

首个原生 IR 增量使用以下六个操作，不增加新的引用、token 或 payload 类型。
这组操作的原生验证不授予 source-unit 发布或 emit 权限；源码 producer、摘要和最终
lowering 必须分别完成后续验收。当前发布与最终硬件边界拒绝残留的这些操作。

| 操作 | 输入 → 输出 | 身份和区域 |
| --- | --- | --- |
| `ac.state` | clock、reset、initial → old Q | 非 Symbol 分配操作，`instance_name` 和 declaration occurrence；复用现有分配命名空间 |
| `ac.state.read` | path、索引值 → payload | `state` 字符串定位实际本模块 state inventory；字段/索引 selectors |
| `ac.state.write` | path、payload、索引值 → 无 | 同样的 owner 定位和 selectors；保留原值回写 |
| `ac.method` | 普通 captures → 无 | 真正的 Symbol；function type、参数名、guard/body 两区 |
| `ac.method.call` | path、显式实参 → 普通结果 | qualified Method symbol 与已验证的静态 `instance_path` |
| `ac.transaction` | clock、reset、captures → 普通结果 | 非 Symbol，`name`、Rule declaration occurrence 和注册 occurrence；guard/body 两区 |

MLIR Symbol 不能携带 SSA 结果，因而 state/transaction 复用既有 Queue/Rule 的命名方式，
不得接纳旧的 `sym_name` 别名。Method 定义本身没有 SSA 结果，其 body yield 提供调用结果。
Method 两区的 block 参数为显式形参之后接 captures；transaction 两区的 block 参数为
captures。guard yield 一个一位四态条件，body yield 对应普通结果。

path、clock、reset 都用现有 width-one Bits，不用 builtin i1 丢掉四态信息。
`selectors` 为 `{field = "field_name"}` 或 `{index = N : i64}` 的序列；后者按顺序引用
索引 operands，不能将动态索引塞入静态 owner identity。所有投影须验证实际名义类型。
定位名字不是身份：最终 owner key 来自验证后的实例 occurrence 链和 state declaration
occurrence，必须核对所属模块和重复身份。

state old-Q 输出可以被显式捕获为普通快照；候选 owner 访问必须保留 `state.read/write`。
首个 initializer verifier 只接纳已验证的闭合字面量/构造/投影 DAG，不执行另一个 SSA
算术解释器。更广 source initializer 接入必须复用现有静态语义，不能忽略不支持的节点。

未完成证明的 read 仍有检查义务，除状态 Read 外保留 check Write effect，防止 MLIR
将未使用读取提前当作 trivially dead。call/transaction 保守携带状态及检查 effects。
原生正反例位于 `tests/compiler/IR/TransactionEffectsTest.cpp` 和现有 IR lit 入口；
它们不证明 Python Method、动态执行或跨源 Method 已完成。

## 阶段职责和不变量

### 绑定、类型与所有权

输入是单个源文件的 capture 与完整已发布依赖接口。扩展既有绑定和类型分析，输出
类型化的源语义表示。必须固定实例身份、Method 声明、owner 路径、标量 kind、名义类型、
静态形状及隐藏域。读取引用和按值复制不可混淆。

无法解析的 owner、动态实例创建、签名不一致、被遮蔽的内建名字和不支持的跨域调用必须
在此阶段或其闭包核对中拒绝。Python capture 只保存语法与位置，不执行设计函数。
实例生成仅复用现有已准入规则；新 Method 不扩大一般静态参数支持。

### 跨过程效果摘要

已接入的首个分析子集是公共 Dialect 层的 `SourceMethodSummariesAnalysis`
（`SourceMethodAnalysis.h/.cpp`），由 Transforms 层的 `AnalyzeSourceMethods.cpp`
接入 `ac-analyze-source-method-summaries`。它检查实际调用图、递归及有值事务的保守
totality；不发布通过标签，也不构成完整条件效果/端口/接口摘要。
源 compiler 和未来 interface extraction 必须复用这个 IR-only owner，不能反向形成
Transforms → SourceCompiler 的库依赖或复制证明算法。

当前恒真事实只来自已验证的一位常量和普通 capture/formal 传递；状态初始化不是证明。
通用 Method 摘要对显式形参取 Unknown；调用实例的常量事实单独证明，不污染通用缓存。
未调用的 guarded Method 不影响一个 total Rule；所有声明中的递归仍拒绝，包括死路径。

按调用图推导并实例化条件性读写、返回依赖、required guard、资源请求、顺序、端口和
时钟域摘要。声明摘要可先建立，涉及具体 SSA 的写域及条件事实在状态版本明确后精化。
递归调用拒绝；调用深度、摘要大小、访问数量和证明工作量必须有预算与明确诊断。

摘要中的 owner 路径是相对于定义或明确引用的路径；调用点绑定后必须指向真实 occurrence。
两次构造是两个 owner；两个别名指向同一次构造仍是一个 owner。不得为了内联 Method
复制实例或持久状态。对同一事实使用统一分析，不能在 source、link 和 backend 各推导一套。

### 事务局部状态 SSA

每个基础 Rule 从其 owner 的旧 Q 开始。按实际语句顺序生成局部版本，Method 调用接收
并返回相应候选版本，分支通过既有四态选择语义合并。保存过的局部值保持快照。
字段、索引更新必须保留完整宽度的索引事实和未触及的写域。

同 owner 多次调用共享事务局部候选；不同 Rule 不继承候选。写入同值仍保留源码写意图，
不能在冲突证明前被优化掉。Memory/Queue 访问依据其原语合同生成请求，不能用普通变量
SSA 隐式改变同步读、旧队头或端口策略。

输出应能独立检查：所有读都有唯一版本来源；分支 join 类型一致；每个更新都有 owner、
路径及外层 Rule；不存在未绑定的候选状态。

### 值有效性与事务准入

至少区分三类事实：组合值的求值有效条件、Rule 能否发射、普通返回数据中的状态值。
只有 required guard 加入外层获准条件。名为 `accepted`、`valid` 或 `retry` 的字段均无
隐含语言含义；不能按字段名字推导 guard。

对在已知有效路径 `p` 上调用且要求条件 `g` 的 Method，其逻辑义务是 `p` 成立时 `g`
必须成立。该关系不是把未知值强行转成二态的许可。完整四态和错误行为服从源语义合同。
guarded value 的条件分支必须分别携带“值有效且为真”“值有效且为假”；无效不进入 else。

普通状态返回仍是数据。PFL 风格 retry 的游标更新不得消失。可选操作、合法前缀选择和
真正必需操作的区别来自作者代码/Method 合同，不来自编译器对算法的猜测。

### 资源请求、冲突与因果分析

先构造条件性请求及依赖，再计算可实现的获准关系，最后物化有效传输。
Queue 的事务内容量抵扣见[资源合同](extensions.md#queue-request-admission)。不能直接产生
`fire → pop → ready → fire`，也不能通过 runtime 固定点寻找一个结果。

复用既有 owner/字段写集合、SSA grant 和地址分离证明，检查跨事务写域；同事务可合法
合并的候选写最终形成对应 D/E。检查实际 primitive 的读写端口和操作组合，不能把
“串行源码”理解为无限端口或自动多周期复用。

数据、guard、容量、候选可见性和显式顺序关系必须进入统一的因果检查。旧 Q 是真正的
时间边界，事务内候选和同拍前递不是。未使用结果也不能隐藏非法环或错误资源连接。
源码相邻但无依赖的纯计算不得被无条件串接。

请求、路径有效性、冲突和因果分析可以共享 provisional 表示，但必须在有界分析内完成；
失败或预算耗尽必须诊断，不能降格为“默认可执行”。不得添加隐式仲裁或任意 winner。

### 事务降低

输入必须已经完成 owner、guard、冲突、资源和因果义务。统一 Rule 获准条件门控其实际
路径上的全部效果，输出现有组合操作、Table 操作、`ac.value.merge`、Queue 输入及
存储 D/E。随后才允许移除高层事务/Method 操作并形成纯 `ac.rule`。

必须保留合法未知数据、未知有效控制的错误、reset 优先级、held/falling-clock 行为及
所有需要的观测/检查。事务不发射不等于系统失败；硬错误仍在任何 Xfer 前取消全系统
的数据及候选时钟历史。现有 Runtime 和 managed RTL lifecycle 继续拥有提交边界。

每个高层效果必须有唯一 lowering 归宿，或有经过证明的无效果解释。
“backend 不支持所以忽略”与“结果无人使用所以删除效果”均不合法。

### 效果安全优化

优化前必须检查原表示的语义合法性，避免 DCE 隐藏冲突、死环、检查或未实现操作。
扩展已有 record wire 简化及纯表达式优化，优先删除冗余传输、复用公共计算并保留紧凑
Struct/Table 表示。不得将每个数组元素、每次 Method 调用都提前完整展开。

| 优化 | 准入条件 | 不允许的变化 |
| --- | --- | --- |
| CSE/共享纯计算 | 相同类型、值及有效性上下文，且无隐含效果 | 合并资源调用、复制/合并持久 owner |
| 字段投影/稀疏更新 | 名义布局、类型和未写字段保留 | 用整记录覆盖其他 writer 的字段 |
| 常量/位宽传播 | kind、范围、溢出及四态合同已证明 | 将区间当常量、丢弃 X/Z 或静默截断 |
| 写合并 | 写意图、检查及资源义务已验证，候选顺序等价 | 去掉仍影响未知使能失败的操作 |
| mux/归约重组 | 精确四态运算下的等价性证明 | 仅凭二态代数或循环形状重排 |
| 资源共享 | 允许的操作组合及既定成本目标 | 增加周期、改仲裁、超端口 |

任何 mutating pass 都必须失效受影响的分析，重新验证变换后的事实。预算、错误诊断和
接口权威不能依赖已经失效的缓存。优化不会自动授予 retiming、pipeline 插入或 CDC 支持。

### 最终验证与双后端

最终包必须闭合，不能残留未降低的 Method、事务、资源占位符或未经核对的 import。
共同验证确认类型、源 owner、组合/采样环、primitive 合同及检查计划后，两个 emitter
消费同一保存的 final artifact。backend 不重新推导 guard、优先级、事务或前递语义。

## compile、link 与接口发布

局部事实在 compile 验证。依赖完整实例闭包的事务绑定、资源冲突和因果义务必须显式保留
到 link。源单元 body 可保留本扩展的类型化语义操作；这仍是同一逐源编译路线。

发布接口必须包含 Method 签名、可绑定 owner 路径、条件效果、返回有效性、域及顺序/端口
要求。只发布一个“有副作用”位或值端口列表不够。link 必须用 provider body 重新核对
摘要及其绑定；摘要不能作为未验证的用户声明直接获得权威。

消费方 compile 不得读取 provider Python 或 implementation body 代替接口；link 使用
显式提供的 body/interface closure。变换改变摘要事实时，必须在发布前重新提取/验证，
不能沿用旧哈希、旧依赖或旧 source owner。

本扩展改变 source-unit body/interface 的可表达内容，第一任务必须同时更新其 schema、
结构验证和注册表验证；遇到未知或不兼容内容必须拒绝。不得添加 legacy fallback。
仍保留原 Python basename、source owner、独立 unit 以及生成文件归属，不能整设计展开
后再切分文件伪装逐源编译。

## 检查点与保留产物

| 检查点 | 必须可检查的证据 |
| --- | --- |
| Capture | 原始 source snapshot、capture 与源位置 |
| 类型化效果 | 实例/owner、调用、状态访问、guard 和 Rule 归属 |
| 状态 SSA | 前后版本、分支合并、Method 组合和写域 |
| 准入/资源 | 请求、获准条件、冲突和因果证明结果 |
| 公共硬件 | 完成 lowering 的 IR、效果覆盖验证与优化前后结果 |
| Link/emit | unit interfaces、完整 closure、final IR 和同源 C++/Verilog |

现有 `--source-import-out` 捕获的是 source lowering 后的硬件 IR，不是新的类型化效果层。
新增阶段检查应扩展现有 pass instrumentation、测试和 manifest owner；不能把旧输出改名
冒充新证据，也不能引入另一个产品编译入口。精确检查输出形式由第一任务确定，默认 public
CLI 不增加参数。验收必须把所有产物绑定到相同候选源码、工具和命令。

## 效率与可扩展性

分析工作应复用调用摘要和 MLIR 缓存；展开和证明均有预算。基准必须分别记录编译时间、
峰值内存、IR 大小、存储位/端口、组合深度及模拟吞吐。可用时报告同约束下的综合面积/时序。
单独的 IR 数量、Python 行数或一个 native 运行结果不证明硬件更高效。

基础验收要求不增加隐式周期/状态，不复制 Method owner，且指定等价夹具中的冗余纯
计算/记录传输被消除。跨设计速度或面积改善必须有单独测量；没有测量就不宣称改善。
验收具体场景及交接文件见[验收标准](acceptance.md)和[实现分解](implementation-plan.md)。
