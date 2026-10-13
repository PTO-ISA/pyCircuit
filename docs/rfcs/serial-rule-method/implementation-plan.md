# 串行 Rule 与 Method 扩展实现计划

本计划把[目标扩展](README.md)分解为七个有界切片，输入语义来自[source-semantics](source-semantics.md)、[MLIR 流程](mlir-pipeline.md)与[扩展边界](extensions.md)，出口采用[验收规范](acceptance.md)。这是后续派发依据，完整七切片尚未完成；局部 IR/producer 检查不构成完整源语言或执行能力，不能据此扩大当前支持声明。

## 派发与共同约束

先由真实 `architect` 实例确认表示与语义，再由独立 decomposition 实例核对本切片边界。实现、独立测试与代码评审使用分离实例；按[治理](../../development/project-governance.md)记录实际 role/model/effort。实现与评审请求默认 `gpt-6.1-sol`，架构 preset 为 `gpt-6-astra`/`xhigh`；请求值不能替代实际值。

PM 独占公共接线：`compiler/lib/Compiler/SourceUnit.cpp`、`compiler/include/pycircuit/Transforms/Passes.h`、`compiler/include/pycircuit/Dialect/ACIR/ACIROps.td`、`compiler/include/pycircuit/Dialect/ACIR/ACIROps.h`、`compiler/lib/Dialect/ACIR/ACIRDialect.cpp`、公共 pass 注册、所有 CMake 与测试登记。Python 公共导出和必要 capture/transport 语法接线也由 PM 统一整合；capture 只记录语法、身份和位置，不承担效果或事务推断。实现者提交精确接线请求，不能同时编辑共享注册表。

下文区分已接入入口和后续预留文件；已接入的局部增量不代表整个切片完成。PM 必须在每个派发包里再次冻结实际使用的集合、函数接口、op schema、证据输出入口与测试名；省去未必需要的文件也须在派发前明确。执行中不得以通配目录扩张所有权，也不得另起语义编译器、runner、协议或兼容层。拆成七个责任不要求七个独立可执行程序。

共享工作树中的其他编辑必须保留。每个切片只改自己的文件；依赖变更或边界调整报 PM，重新确定独占权后再继续。新 pass/analysis 的头文件直接声明既有 MLIR pipeline 使用的职责，不解析一份额外 Python AST。所有变换都必须有可检查的前后表示和验证不变量。

## 七个实现切片

### 类型化效果与资源身份

**输入／依赖：** 架构确认的 typed effect schema、Module/Rule/Method 身份、同域限制和源位置映射；无实现前置切片。

**独占现有文件：** `compiler/lib/Compiler/SourceMemoryBindings.{h,cpp}`、`compiler/lib/Compiler/PythonImportContext.{h,cpp}`。明确的现有所有者是 binding/type/ownership 与 importer context，不能把 type authority 留给后端。

**已接入入口：** `compiler/lib/Dialect/ACIR/ACIRTransactionOps.cpp` 和现有 `SourceEffects.h/.cpp` 负责原生效果验证；`compiler/lib/Compiler/PythonImportTransactions.inc` 通过同一个 Importer 生成类型化效果。不得再创建 Compiler 层的重复 effect verifier。

**产出与不变量：** 类型化操作保留事务入口、call、owner/resource、access path、guard、普通返回、观测和状态 effect。实例 occurrence 与 alias 可比较；effectful 操作不得 Pure。数学 Integer 与 fixed bits 权威不混用；动态 owner、无法解析的类型／域失败关闭。PM 将必要的最小 op 扩展接入既有 ACIR。

源接入必须采用 source-semantics 已冻结的同域首批边界：Method-owning module 零 authored runtime 参数、无普通返回、静态句柄；嵌套公开 Method 捕获 owner，禁用 `nonlocal` 和 owner shadow。包括只读 Method 的调用都在 Rule 内。每实例 Rule 恰好静态注册一次；runtime／循环／重复注册及嵌套自主 Rule 调用拒绝。普通 typed-result module 继续原有接口。

**移除范围：** 替换已由 typed facts 接管的重复 importer 私有身份／效果推断；未接管职责不提前删除。不得保留并行权威。

**最小出口：** `METHOD-INSTANCE-IDENTITY`、`METHOD-SOURCE-OWNER`、`METHOD-REGISTRATION`、`METHOD-DIRECT-MODULE-PRESERVATION`、`METHOD-FOUR-STATE`、`METHOD-EFFECT-RETENTION` 的 typed IR 与公共源正负例；独立 reviewer 检查新操作 effect traits 和身份。

### Method 摘要与独立发布

**输入／依赖：** 前一切片的 verified typed effects；确定的递归拒绝／有限调用展开策略，发布 body/interface 扩展契约。

**独占现有文件：** `compiler/lib/Compiler/SourceRuleWrites.{h,cpp}`、`compiler/lib/Transforms/ExtractSourceInterface.cpp`、`compiler/lib/Compiler/SourceHeaderRegistry.cpp`、`compiler/lib/Compiler/SourceBodySnapshots.cpp`、`compiler/lib/Dialect/ACIR/SourceUnitValidation.cpp`、`compiler/lib/Dialect/ACIR/SourceEffects.cpp`。

**共同分析入口：** `compiler/include/pycircuit/Dialect/ACIR/SourceMethodAnalysis.h`、`compiler/lib/Dialect/ACIR/SourceMethodAnalysis.cpp`；pass 位于 `compiler/lib/Transforms/AnalyzeSourceMethods.cpp`。真实定义和导入 row 共用调用图/totality、实例闭包和 `SourceMethodResolution.cpp` 中的 receiver 解析；独立 source-unit 发布保留 typed effects，final link 仍拒绝未降低事务。Compiler 层不保留另一份算法或 forwarding header。

原生发布摘要的有界实现使用同一公共 analysis API，并将较大的闭合 schema 验证／派生
放在 Dialect 层 `SourceTransactionSummary.cpp`；内部 `SourceTransactionSummary.h`
仅共享派生入口与既有 literal fact helper，不暴露另一套语义或编译路线。精确字段见
[事务接口摘要合同](publication-summary.md)。独立测试使用下表已预留的
`MethodSummaryAuthorityTest.cpp`，接入既有 transaction native target。这个出口只证明
native derivation／结构验证／body 重验。Import 标签硬切覆盖全部 producer、getter 和 fixture，
并删除三个不完整的临时 import carrier。真实 provider/consumer 发布使用同一 native
closure 与 body 重验入口；其公共证据和最终 lowering／执行证据分别验收。

**产出与不变量：** 条件读写 footprint、guard、普通返回、端口／域／顺序需求在每个调用处绑定实际 owner，不克隆状态。发布保留完整闭包才能解决的 obligations；consumer 不重新读取 provider Python。相关变换后摘要重算。link 的 body 重验不能只检查哈希或信任 caller 提供的摘要。

独立发布前先完成 Method 逻辑类型权威前置切片：producer 用既有 `sourceConstraint`
编码显式参数与返回，Dialect 从 module contract 提取共用验证，不建立另一套类型系统。
每个 Method 的三个必填 `ac.*` 属性及其 exact shape 见 [IR 契约](mlir-pipeline.md#typed-effects)。
独立验收必须覆盖同一物理一位表示下 Boolean／Integer／fixed bits 的打印重读区分，
参数来源与 owner 路径、返回形状、类型／域和字段篡改拒绝，以及调用方先于 callee 子节点
验证时的安全诊断。这个前置切片不修改 source grammar、source-unit 发布或 final admission；
缺少 metadata 的旧手写 IR 一次性更新，不保留兼容默认值。

**移除范围：** 仅移除已经有 body 派生摘要替代的私有 call metadata；保留包括身份写在内的源写意图。

保留的纯 `@rule` helper 调用者由 PM 列出精确清单、逐文件冻结改写权，显式迁移为纯 `def`，不扩大 examples/catalog 的支持范围。纯函数不得隐含 owner 读取或 effect。最终切换不得并存旧 helper fallback，也不得把 nested autonomous Rule 调用静默当成 Method。

helper grammar 限于现有已准入 typed 表达式／语句子集及 source-semantics 明列约束，不从“有限纯函数”推断一般 Python、递归或通用静态参数能力。

**最小出口：** `METHOD-SUMMARY-AUTHORITY`、`METHOD-PURE-HELPER-CUTOVER`、`METHOD-ALIAS-CONFLICT`、`METHOD-OUTPUT-PROTECTION`；独立编译 provider/consumer，逐项伪造摘要并绕过外层哈希验证 native authority 拒绝。

### 事务局部 state SSA

**输入／依赖：** 类型化 owner、调用摘要与源串行语义；原语可见性限制。

**先导分析的独占文件：** 新增 `compiler/include/pycircuit/Dialect/ACIR/SourceTransactionStateAnalysis.h`、`compiler/lib/Dialect/ACIR/SourceTransactionStateAnalysis.cpp` 和 `compiler/lib/Transforms/AnalyzeTransactionStateSSA.cpp`。仅为传递既有 parent bindings，允许在 `SourceMethodAnalysis.h`／`SourceMethodResolution.cpp` 给实际 call resolver 增加重载；不得构造假的 publication summary row。

**真正变换的既有 owner：** `compiler/lib/Transforms/LowerTransactions.cpp`，同 owner 的有界执行已接入。后续跨实例继续扩展这一 pass，精确合同见[跨实例接线](cross-instance-lowering.md)；不再创建先前预留的 `LowerTransactionStateSSA.cpp` 平行物化路径。effectful branch／selector 的源码扩展由 `PythonImportTransactions.inc` 及其 native verifier/summary 所有者承担；不能用 legacy importer 的私有候选环境替代通用 MLIR 变换。

**产出与不变量：** 每个独立 Rule 从旧 Q 建立候选；同一事务通过语句、分支、Method 调用线程化候选版本；局部值保持不可变快照。字段／下标更新保留精确路径和身份写。source order 只产生真实数据、状态或 effect 依赖，不能让所有纯计算串行。memory/Queue 不自动获得寄存器候选转发语义。

**移除范围：** state SSA 接管后的私有候选版本和重复字段合并算法；移除前核对 reset/hold/discard、来源与类型。

先导分析本身不移除执行路径、不改变 Method 公共签名、不生成硬件。初始独立分析检查点
未接入默认流程；当前 link 在含 source transaction operations 时复用它，再交由唯一
LowerTransactions 物化。consumer compile 仍只读接口，不为该分析读取 provider body。
它输出事务相对实例的不可变 typed plan，保持输入 IR 的对象身份和内容不变；对所有事务
完整成功后才公开查询结果。独立测试先验证 Q0/Q1/Q2、跨调用快照、实例身份、guard 入口
版本、有序事件、whole-owner nominal payload、同域绑定、预算和失败原子性。这个阶段
通过不能勾选下述完整出口；分支 join、部分更新、物理 owner 接线和准入检查仍需后续包。

**最小出口：** `METHOD-STATE-ORDER`、`METHOD-SNAPSHOT-OLD-Q`、`METHOD-RETRY-CURSOR`；设计中立槽队列等价 oracle、快照与独立 Rule 注册顺序变体。

### 路径有效性与事务 admission

**输入／依赖：** typed guards、返回状态、state SSA、条件摘要；资源可用性先保持符号义务，等待下一切片解决。

**首批直线采样切片：** 精确 schema、共享证明、Runtime/RTL 生命周期、依赖与文件责任采用
[事务采样准入](sampled-admission.md)。复用既有 SourceTransactionStateAnalysis、
HardwareSourceChecks 和 LowerTransactions；不创建此前预留的
`AnalyzeTransactionAdmission.cpp` 平行 pass。guard span/case_eq 前置完成不构成采样执行。

**后续分支切片的现有入口：** `compiler/lib/Compiler/PythonImportBehavior.inc`、
`compiler/lib/Compiler/PythonImportCaseSyntax.{h,cpp}`；事务源接入仍由
`PythonImportTransactions.inc` 及 native verifier/summary 拥有，派发前单独冻结分支表示与
join proof，不以 legacy 私有环境替代 MLIR 分析。

**产出与不变量：** 分开表达值有效性、外层事务 admission 和普通 accepted/success/retry。只有实际执行路径上的 required 条件参与 admission。已知 false 只 hold 所属 Rule；不能升格为全局硬错误。无效 payload 不当 false 使用；未知控制保留四态义务。optional progress、required all-or-none 与明确 prefix 策略不互相推断。

guard 首批只接纳具有完全匹配必需位置参数的 lambda；owner 在调用入口读候选，普通自由值保持定义位置词法 SSA。guard 不产生 effects，body 的后续写入不改变该次选择。active edge 的未知有效 guard/call control 产生全局硬错误；已知 false 支配路径和 reset 不激活这些新错误。potentially guarded Rule 首批必须 `-> None`；total Rule 的普通结果保持现有语义，不制造隐藏 valid state。

**移除范围：** 已由路径分析替代的 importer guard/status 混用；不得删全局检查或把硬错误弱化为 backpressure。

**最小出口：** `METHOD-GUARD-ENTRY`、`METHOD-RESULT`、`METHOD-PATH-VALIDITY`、`METHOD-REQUIRED-ATOMICITY`、`METHOD-OPTIONAL-PROGRESS`、`METHOD-GUARD-LOCAL-HOLD`、`METHOD-RETRY-CURSOR`；库存多 owner 发布和探测 cursor 的独立 oracle，包括 status 字段改名变体。

### 资源、冲突与因果闭包

**输入／依赖：** 条件摘要、state SSA、符号 admission；基线原语端口／延迟契约。可选 Queue 必须先冻结 extensions 中的事务式契约，不能阻塞无 Queue 基线出口。

**独占现有文件：** `compiler/lib/Dialect/ACIR/HardwareAnalysisDependencies.cpp`、`compiler/lib/Dialect/ACIR/ACIRQueueOps.cpp`、`compiler/lib/Compiler/PythonImportQueues.inc`。

**冻结新增文件：** `compiler/lib/Transforms/VerifyTransactionResources.cpp`。

**产出与不变量：** 归一化 alias，证明不同 Rule 的互斥或不相交写；检查实际资源和端口数。联合验证 data、validity、ready、forwarding 与显式顺序；保留无用输出内的非法环。可选 Queue 分离 request/accept 与 prospective capacity，随后才形成物理 handshake；不产生 fire/pop/ready 自循环。有限分析预算失败须诊断，不能用运行时固定点、自动仲裁、无限端口或隐含寄存器修复。

**移除范围：** 被 op 级证明取代的私有 overlap/owner-enable 特例与重复图遍历；保持 write-intent、unknown-enable 及 capacity 义务。

**最小出口：** `METHOD-ALIAS-CONFLICT`、`METHOD-CAUSALITY` 和真实端口拒绝；可选范围另验三个 `METHOD-QUEUE-*` 标识，继承 no-empty-bypass 与延迟 oracle。

### 最终 lowering 与公共双目标闭环

**输入／依赖：** 前五个切片完成的 typed obligations；完整 source-unit closure。PM 冻结 compile 留存／link 解消边界和各阶段证据接线。

**独占现有文件：** `compiler/lib/Compiler/PythonImportRecords.{h,cpp}`、`compiler/lib/Compiler/PythonImportComposition.inc`、`compiler/lib/Compiler/SourceLink.{h,cpp}`、`compiler/lib/Transforms/VerifyHardware.cpp`、`compiler/lib/Dialect/ACIR/FinalSourceUnitValidation.cpp`、`compiler/lib/Dialect/ACIR/HardwareAnalysisValidation.cpp`。

**既有实现文件：** `compiler/lib/Transforms/LowerTransactions.cpp`。

首个可执行增量直接复用该 owner，按 [MLIR 事务降低合同](mlir-pipeline.md#transaction-lowering)
完成同 owner、恒真 guard、whole-owner Bits、单外层 writer 的真实 DFFE/Rule 物化；
不另建临时 lowerer。独立 native 用例由 `tests/compiler/IR/TransactionLoweringTest.cpp`
及 `tests/compiler/lit/IR/transaction-lowering.mlir` 承担，公共源码由下述
`tests/compiler/lit/Source/Inputs/serial-method-public.py` harness 生成；表中后续 DUT
路径为预留，不得将尚不存在的独立源码文件当作现有执行入口。
PM 另负责 `SourceLink.h/.cpp` 的阶段捕获与权威重验，以及 `compiler/tools/link.cpp`
的成对诊断输出；共享六类残留事务查询由既有 `ACIRSourceContracts.h`／
`ACIRTransactionOps.cpp` 提供。原生结构验证与公共 canonical 权威分别验收。无事务的
普通硬件不得新增事务预算限制。这个增量通过也不勾选本切片的完整跨实例和 system 出口。

**产出与不变量：** 先检查全闭包，再将已证明事务变为 fire-gated proposals、字段合并、既有 Table/Queue 操作与 D/E 连接。到这一边界才能擦除 Method/transaction effect 成为纯硬件图。两目标消费同一最终 IR；whole-system 检查先于物理 Xfer，失败不提交 Q 或 clock history。`@system` 和 compiler-generated simulation closure 必须保留。

硬错误使 SimSystem 进入 sticky Failed。测试须先在恢复前观测全部 Q／clock history 不变，再使用既有 Reset／重建协议恢复；仅修正输入不能作为重试成功的承诺。

**移除范围：** 已分配到 typed analyses 的 importer owner-enable、overlap、state/proposal 私有实现；不保留旧 lowering fallback。不把删除可用源支持算作完成。后端若发现遗漏只报精确缺陷给 PM，由原所有者修复，不允许目标侧另建 Method 语义。

**最小出口：** `METHOD-GLOBAL-ZERO-COMMIT`、`METHOD-DUAL-FINAL-IR`、`METHOD-SYSTEM-CLOSURE`、`METHOD-OUTPUT-PROTECTION`；workers 1/2、RTL、真实四态、全局失败后的原状态与时钟探针。

### 保效果的优化与成本证据

**输入／依赖：** 已验证的 effect/资源义务和既有 record/dependency owners；默认在第六切片闭环建立后派发。若优化位置在事务擦除前，需由 architect 明确其 verified typed 输入与完整 effect 保留证明。

**独占现有文件：** `compiler/lib/Transforms/SimplifyRecordWires.cpp`。

**冻结新增文件：** `compiler/lib/Transforms/OptimizePureTransactionValues.cpp`。

**产出与不变量：** 复用 record projection；只对类型、guards、支配和四态语义一致的纯值实施 CSE／共享、常量与宽度推理、稀疏更新和 mux/reduction 整形。先验证再消除，checks、资源义务、身份写和 unknown enable 在证明前不得删除。结合律／前缀重排须由实际运算及四态契约证明；真实候选状态依赖不得消失。

未使用字段结构义务只针对指定合法 fixture 的临时聚合字段计算；公开 Struct/layout、状态 owner 及可观察字段的布局保留其接口契约，不能以改变 schema 满足优化验收。

**移除范围：** 重复且已由现有 utility 覆盖的 projection/纯表达式处理，不引入新依赖或广义两态优化解释器。

**最小出口：** `METHOD-EFFECT-RETENTION`、`METHOD-NO-ADDED-LATENCY`、`METHOD-UNUSED-FIELD-ELIMINATION`、`METHOD-PURE-SHARING`；冻结合法 fixture 的完整前后 IR、双目标与 oracle，分别测量 compile 成本和硬件／仿真成本。

## 独立测试的文件与职责

测试所有者独占以下已接入或预留文件，不编辑实现者文件；已有测试登记与共享 helper 的接线由 PM 完成。每个路径在对应派发前核对实际存在状态和所有权冲突，并冻结用例及 CTest/lit 选择。typed effects、summary、state SSA 和公共 harness 已有局部入口；表中的分支、资源、优化义务仍需逐项核对实现。文件存在不表示整行需求已覆盖，任何局部结果都不能替代该切片的完整验收。

| 对应切片 | 独立测试文件（精确路径） | 测试内容与移除边界 |
| --- | --- | --- |
| typed effects | `tests/compiler/IR/TransactionEffectsTest.cpp` 与 `tests/compiler/IR/SourceTransactionCaptureTest.cpp` | owner、域、type authority、effect traits、源位置；不冻结 incidental op 排版 |
| summaries | `tests/compiler/IR/MethodSummaryAuthorityTest.cpp` | 条件 footprint、资源／顺序摘要、body/interface 伪造、变换后失效重算 |
| state SSA | `tests/compiler/IR/TransactionStateSSATest.cpp` | 候选串行、快照、字段、旧 Q、注册顺序；保留既有 ROB 独立 oracle |
| admission | `tests/compiler/IR/TransactionAdmissionTest.cpp` | path validity、required/optional、false guard 局部 hold、retry progress |
| resources | `tests/compiler/IR/TransactionResourcesTest.cpp` | alias、端口、写冲突、联合因果环；Queue 可选项独立报告 |
| dual flow | `tests/compiler/lit/Source/serial-method-public.test`；`tests/compiler/lit/Source/Inputs/serial-method-public.py`；`tests/compiler/lit/Source/Inputs/serial-method-public.cpp`；`tests/compiler/lit/Source/Inputs/serial-method-public.sv` | 复用既有公共编译／link／emit 测试模式，workers 1/2、RTL、失败发布保护与真实 X/Z |
| effect-safe optimization | `tests/compiler/IR/TransactionOptimizationTest.cpp` | effect 不丢失、非法图不被优化隐藏、三个结构效率义务 |

设计中立 DUT 精确预留为 `tests/compiler/lit/Source/Inputs/serial-method/ordered_slots.py`、`owner_bundle.py`、`retry_probe.py`、`provider.py`、`consumer.py`、`closed_system.py`，均位于该同一目录。外置 oracle 精确预留为 `tests/compiler/oracles/serial_method_oracle.py`。oracle 不进入 DUT 或 product lowering。是否拆出更多源由 PM 在派发前冻结，执行者不能临时扩展文件集合。

已有 `tests/compiler/lit/Source/multi-rule-writes.test`、`source-interface-extraction-invalid.test`、`record-projection-simplification.test`、`queue-source.test` 与 `tests/system/test_source_system_execution.py` 是应保持的责任入口；若确需修改这些文件，先由 PM 单独分配给独立测试所有者。不要创建第二 runner，不要求测试依赖本 RFC、local 日志或验收计数。长期 oracle/mutation 与变体矩阵放入现有 nightly 选择；短 gate 不偷偷运行全矩阵。

## 依赖、整合与证据出口

后续派发先核对[扩展实施与验收矩阵](extension-roadmap.md)中的检查点与待实现边界。
跨实例的新增共享接口按专项规格审查；动态 guard 在采样域、history、检查身份与顺序的
精确共同 IR schema 冻结前不得派发实现，不能由 executor 在测试失败时自行补定。

主链为 typed effects → summaries → state SSA → admission → resource/causality → final lowering → optimization。state SSA、admission 和 resource analysis 交换已声明符号事实；按有界分析接口闭合，不以顺序列表掩盖循环求解。允许独立测试准备与各切片实现并行；不可并行编辑同一 owner 文件。

每个最小出口须逐层列状态。早期 typed IR/unit 正负例可以先通过；若公共源流程尚未接通，compile/link、双目标与执行 oracle 必须仍为 pending/NOT-RUN，待最终 lowering 切片闭环再补验。不得用 mock、脚本结果或内部 IR 直跑替代公共证据，也不得据早期层通过宣布全部需求完成。

PM 每次整合须检查所有权清单、接入共享 schema/registries/CMake、更新 stage 验证与分析失效策略、绑定候选、执行最窄登记检查，然后才进入适用 gate。完整命令与双 emit 模板见[验收规范](acceptance.md#commands-and-gates)；新测试名／pass flags 实现登记前不能虚构命令。

typed effects、summary、state SSA、admission/resource 和 final lowering 的 inspectable input/output 是新增义务。当前 native `--source-import-out` 仅产生 source lowering 后、record simplification 前的硬件 IR，不能作为以上早期阶段证据。公共 compile 留存的 unit、接口、最终 link artifact 与两个 emit inventories 均须候选绑定；native 复现须和公共发布物核对 body/interface。

验收前每项切片分别完成：

- [ ] architect 表示及责任审查、独立 decomposition、派发包已记录，所有新路径和共享接线请求已冻结。
- [ ] 输入／输出不变量及 invalidation 策略落实，旧重复权威在明确范围内移除。
- [ ] 独立正负例已实现并执行，诊断类别和输出保护由原始结果证明。
- [ ] 独立 review 核对 NO HARDCODE、NO SHIM、四态、旧 Q、Xfer/hold/discard/reset 与全局零提交。
- [ ] 命令、退出状态、完整阶段 artifacts、manifest、dirty 内容与工具绑定已保存。
- [ ] 局部结果、适用 gate 与 NOT-RUN nightly 分别列出；未执行项目没有勾选为完成。

同域 Method 基线与可选 Queue 使用各自验收表。Signal/EHR 源接口、一般静态参数、自动 CDC 和其他未分配接口不属于本派发；需要独立提案和 review，不能因一个 fixture 需要而临时实现。计划结束条件是所交付范围有独立语义与公共双目标证据，剩余范围明确列出；不是七个文件已经创建。
