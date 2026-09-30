# pyCircuit 单一路线重构与 GFSIM 迁移计划

日期：2026-09-29。规划修订：8。状态：按用户要求改为有界交付、按需补齐；正式产品切换尚未完成。C1-C/C2-C/C3-C 是已批准的历史基础，R1/M1 联合增补的审阅/授权单独记录。最新进度见[执行账本](../work-items/single-route-migration.md)。

**执行入口：[逐包 checklist](migration-agent-checklist.md) → [M1 C 合同](../rfcs/migration/c2-m1-module-system.md) → [verification matrix](migration-verification-matrix.md)。**
后续 agent 只执行已绑定候选、文件与 Vxx 门槛的一个 Wxx 包。Python
不出现 Queue/FIFO 或手写 Interface/effect/调度 API；MLIR 从普通函数、
注解和真实读写推导 interface。Bluespec 仅提供效果/接口分析参考，
不引入隐式整 rule 阻塞、默认仲裁或另一个编译器。

2026-09-28 执行增补：用户已明确统一 `ac.reg`、无状态 rule proposal、
Xfer 内 commit/discard、reg-only 模块数据连接与 SimQueue 退役方向。
精确接口见 [C2-R1](../rfcs/migration/c2-r1-unified-register.md)，独立审阅与
接口批准状态以该包及执行账本为准。下面的规划时点分析保留来源事实；
后续普通状态和 Queue 任务按这一新方向分解，不继续扩大 SimQueue 路线。

同日 module/system 增补：用户进一步明确实例树/连接在 ACIR link
构建、system 驱动整树、Work 只算本地 rule、Xfer 执行 IR 提交，并已
选择“模块函数＋嵌套 rule”的无 self 源表面。联合候选见
[C2-M1](../rfcs/migration/c2-m1-module-system.md)。它替代目标设计中的
module class/self 与递归生命周期方向；冻结 C1/C2/C3 文本及已验收的
旧切片仍作为历史基线。M1 C 与执行交付包已获
[独立 approval-ready](../reviews/20260928-interface-handoff.md)，具体实现
派发仍按 W00 绑定精确授权和候选；设计审阅不作为实现证据。

**目标只有一条产品编译路线：Pythonic 模块函数/嵌套 rule → 语法捕获 → MLIR 语义分析与 lowering → 经过验证的硬件 IR → GFSIM C++ 或 Verilog。** 复用 GFSIM 成立的寄存器/对象结构并适配 system 驱动，以 hard break 退役旧编译路线。

用户最新要求取代上一版“保留 CAS、structural、Agentic 三种 authoring 路径”的前提。单一路线是已明确的架构方向，不再反复确认。具体 Python、IR、CLI、生成代码或 runtime 接口的改变，仍须先完成可审阅提案、独立技术审阅，再取得用户对精确修订的批准。**本规划不代替接口批准。**

配套：[治理与调度](project-governance.md)、[验收规范](pycircuit-modernization-tests.md)、[独立审阅记录](pycircuit-modernization-review.md)。下文来源分析记录规划时点；已批准合同的基础实施与测试另见执行账本，完整决定与产品切换按 M5 完成。

当前阶段：**M2 有界主干已验收**（132 项原生测试、27 项 Python/双后端测试通过）；
后续进入一条 M4 可用流程，M3 按需补缺口。并行调度、完整 SDK 等仍为后续责任。
验收范围与证据位置见 [M2 执行记录](../work-items/m2-closure-execution.md)。

## 最新纠偏：硬件 design 与 testbench 分离

用户明确要求设计产物与 testbench 分离，并追加命名规则：
「ac应该是和python的文件名一致」——`.ac` 产物按来源 Python 文件名命名
（`<stem>.ac`），不设保留标签；`design_top.ac` 只是 `design_top.py` 这一
root 源文件的产物，`test_increment.py` 产出 `test_increment.ac`。这与 C3-C
逐单元 `<stem>.ac` 的既有命名一致，C3-C 文本中的 `-o <program.ac>` 需按
此规则修订。M2 已验收的是封闭系统
回归范围，不是独立 DUT/testbench 的公开交付。M4 必须分别证明 design、
testbench 与通用框架/runtime 的边界，不能把含 stimulus/checker 的
测试 system 更名后当作 design。当前暂停进一步公开入口和 IR 扩展，
先按 [边界与 primitive 授权审计](../reviews/20260929-design-testbench-ir-authority.md)
收敛；已有精确批准继续适用，新增语义不得由代理自行批准。

## 2026-09-29：交付范围收缩

用户明确要求评估 M1–M7，不要求把所有能力实现完备，允许后续补充。
本节和下面修订后的阶段出口优先于旧 W/V 清单中的全量前置条件。
这是交付范围与顺序调整，不改变已支持功能的硬件语义，也不把未做的
验证标成通过。

当前问题是 M2 的“最小闭环”被细化为 V02–V48 全部适用门槛，提前
吸收了 M4 的工具交付和 M6 的工程加固；M3、M5、M7 又重复要求完整
能力表。因此任务持续扩面，阶段出口难以关闭。

新的交付主线是：**M2 验证主干 → M3 按一个实际用例补缺口，与 M4
可用入口并行 → M5 切换已声明的支持范围**。M6 持续加固；M7 可以先
形成有明确限制的预览候选，不等待全部 M3/M6 backlog 清空。

保持的底线：Pythonic 函数/嵌套 rule；MLIR 负责语义；统一 ac.reg；
Work 不发布 Q、Xfer 才提交并丢弃 disabled proposal；模块连接不复制
寄存器；同一 final IR 的双后端行为一致；不回旧编译器兜底。

可以延期的是能力与覆盖广度：一般化整数/record/collection/参数化、
FIFO 库、memory/CDC/多时钟/完整四态、实际并行调度、全平台 SDK、
全面性能优化与穷举故障注入。新增能力按实际需求选择，并记录所属
阶段、限制和测试；不因 backlog 存在而阻塞无依赖的当前阶段。
已暴露入口仍须拒绝不支持的输入，已知会产生错误结果、错误提交或
破坏既有输出的问题仍须修复。

## 分析结论与证据

### 需要收敛的现有路线

pyCircuit HEAD：`8887e6dec7b4cc530a9967c860dc6a224d79a4ab`。以下为代码直接证据，置信度高；不代表本轮已经运行编译验证。

| 路线或职责 | 代码依据 | 问题与目标 |
| --- | --- | --- |
| CAS/structural AST/JIT | `python/pycircuit/src/pycircuit/cli.py:238`、`v6.py:1955`、`jit.py:2310`、`design.py:443` | CAS 与 structural 部分共用 JIT，但仍是旧源接口和 lowering 入口；作为产品路线整体退役 |
| direct Python builder | `python/pycircuit/src/pycircuit/v6.py:2014` | 直接执行 Python elaboration；目标 capture 不运行用户模型，删除这一平行路线 |
| Agentic Python semantic compiler | `python/agentic-circuit/src/agentic_circuit/_capture_worker.py:332`、`_acc_py.py:105` | Python 已 lower ACIR，ACPy 同时生成；不是单一语法捕获 → MLIR 分析链 |
| Python 中的状态/效果证明 | `_queue_compiler/state_semantics.py:58`、`:254`、`:278` | 字段写入、writer arbitration、overlap 分析应迁入共享 MLIR 语义层 |
| MLIR 之外的 QueueGraph 语义 | `compiler/acir/lib/CodeGen/QueueGraphPlan.cpp:2300`、`:4563`、`:4679` | graph planning 重建仲裁和合法性；迁移职责与反例后删除独立语义引擎 |
| QueueGraph → PYC 文本 lowering | `compiler/acir/lib/CodeGen/QueueGraphPyc.cpp:519`、`:571`、`:772`、`:6877` | 在文本生成器中构建 Queue 控制/状态；转为 MLIR pass，退役该独立文本 lowering |
| 两套 C++ 生成 | `compiler/acir/tools/acc/acc.cpp:639`、`:653`；`compiler/mlir/tools/pycc.cpp:3330` | 最终只保留 GFSIM 设计的 C++ backend，删除旧 PYC C++ emitter 和旧 QueueGraph C++ 路线 |
| 可复用 RTL 与 MLIR 检查 | `compiler/mlir/lib/Emit/VerilogEmitter.cpp`、`SelectRtlPrimitivesPass.cpp:53`、`compiler/mlir/tools/pycc.cpp:2573` | 复用 emitter/算法/原语与检查，不因此保留旧 pycc frontend 或 C++ backend |

当前并不是三个完全互不共享的编译器。真正应消除的是多个源入口、Python 与 MLIR 分散的语义权威，以及非 MLIR backend planner 再推导行为。把 CLI 合成一个名字或拆分大文件，不能满足单一路线目标。

已有 `compiler/acir/lib/Transforms/LowerRules.cpp:1256` 集成了 MLIR 类型、效果、义务、握手与 firing lowering。这类适用算法和反例可迁入目标 pass；不因旧入口退役而无差别重写正确资产。当前前端实现位于 `python/` 源码树，迁移以实际文件与构建入口为准。

### GFSIM 是目标设计基线

GFSIM HEAD：`b852ed83fa0288d0be7406bba0ed47be4b2c0f63`。当前 dirty 内容仍为治理、skill 与 agent/DSH 配置。产品设计以精确批准记录与代码为准，不以旧文档开头的 pending 文案判定。

| 优先迁移项 | donor 依据 | 必须适配或补齐 |
| --- | --- | --- |
| donor 普通 class/method/record，加 `module/rule/system` | `frontend/ObjectFrontendProposal.md` 修订 C；`docs/IR/approvals/object-frontend.md:14` | 作为来源基线；本仓按最新用户选择改为模块函数＋lexical rule，不移植 self/module class 表面 |
| 仅 AST/literal/span capture | `frontend/python/gfsim_frontend/_capture.py:61`、`_emit.py:80` | 从 root capture 适配逐源 compilation/interface；不把语义分析搬回 Python |
| MLIR Python import 和对象语义 | `compiler/acir/lib/ImportPython.cpp:176`、`PythonLower.cpp:560`、`PythonObjectSemantics.cpp` | `@system`、部分 collection、integer-format/check path 尚有未完成能力 |
| 显式 current/next、owner、driver checks | `frontend/ObjectFrontendProposal.md:865`、`compiler/acir/lib/VerifyFinal.cpp:162` | 合入本仓需要的 clock/reset、四态、memory/CDC、事务义务；不足部分留在 MLIR 补齐 |
| 小型 pass 和 codegen-ready pipeline | `compiler/acir/lib/Passes.cpp:33` | 必须根据目标 source-unit/family 和双后端契约调整次序与验证 |
| C++ emitter 与 lifecycle | `compiler/acir/lib/CodeGenDriver.cpp:97`、`:249`；`CodeGenLifecyclePhases.cpp:107`、`:198`、`:224` | 保持逐源 source group，绑定统一硬件 IR；生命周期名字不是跨资源原子性证明 |
| runtime 与独立生成 DUT 测试 | `include/SimModule.h`、`include/SimQueue.h`、`compiler/acir/test/run_object_scalar.py` | ABI、payload、调度、整数语义和依赖标准逐项对齐；不同时保留两套模型执行系统 |
| PM/subagent 治理 | `docs/Project-Governance.md` 修订 7 | 迁移调度纪律，接口批准权归本仓用户，不继承 donor full-core 委托 |

已批准 donor 前端修订 C 的 SHA-256 为 `b0b5265f3b813b022bbcbf28091204478699c086af69c2f3fe94e385926747ae`；配套整数/C++ 修订 C 为 `b219db671bc54be35471a16d823bbbe4c019dbc1649a907cce25c4c65bab1229`。这些是来源证据，不自动成为本仓批准。

**成熟设计优先迁移；实现缺口单列补齐。** donor 的实际 `acir-codegen-ready` pipeline 已存在，但不能声称其所有源语义都已完成：`PythonLower.cpp:568` 仍拒绝 source `@system`；`VerifyFinal.cpp:244` 拒绝尚未支持的 integer-format/evaluation-path；历史完整测试记录仍为 220/235、15 个旧语法迁移失败。Verilog 尚未实现（`docs/architecture/ACIR-Pipeline.md:65`）。这些缺口是新路线的任务，不是永久保留旧路线的理由。

### 证据边界

两仓本轮均未运行产品构建。pyCircuit 已有 `.omx-state-locks*`、`examples/davo/` 等未跟踪内容保持原样；不会用 hard break 名义删除不属于本任务的工作。来源内容摘要、基线与旧 R4 文档保留在本地 `.omx/context/` 和 `.omx/plans/pycircuit-gfsim-migration-history/r4/`。

donor 源码迁入前登记文件来源、依赖和许可依据；当前检索未发现 LICENSE/COPYING/NOTICE，不能把未核清来源的代码直接当作可发布资产。设计与测试准备可继续。发布后的框架不得依赖 donor checkout、consumer 设计、ELF/ISA 或参考模型。

## 唯一目标架构

### 一条语义链，两个生成目标

```text
Pythonic source（模块函数、嵌套 rule、普通值/注解，无 Queue/Interface DSL）
  → capture：只保留语法、常量字面量、名称和源位置
  → MLIR import / name / type / effect / ownership analysis
  → 每源 semantic ACIR + source-owned interface
  → MLIR link / specialize / state & connection lowering
  → MLIR current-next / resource / driver / timing checks
  → verified final hardware ACIR（共同语义出口）
      ├→ GFSIM C++ emission → 一套模型 runtime
      └→ RTL legalization / emission → Verilog
```

“semantic ACIR”“final hardware ACIR”表示同一编译链的阶段，本计划不未经批准发明新 dialect 名称或 op。源码到硬件的名字、类型、效果、连接、状态、调度和合法性分析由 MLIR 负责。Python 可以做语法检查和损坏输入诊断，不能成为第二个 type/effect/scheduler compiler。

每个 implementation source 仍拥有一个 public module，普通 record/private helper 可以同源；每源由独立 CMake producer 发布自己的语义 ACIR/interface，selected-system composition 单独编译。不得先整系统编译再拆文件。

一个产品 driver 组织这条链，选择 `cpp` 或 `verilog` 只改变后端。MLIR 调试工具和 focused 手写 IR 测试可以保留，但不能形成第二个产品源码编译入口。源码 CLI 名称、Python import 路径、IR 文本/二进制格式及阶段 stop 选项由批准包决定。

### Pythonic 的具体要求

目标采用普通函数、record、局部变量、nonlocal、函数调用、静态
`if/for`、规则内条件和标准类型注解。module/system 函数声明结构，
嵌套 rule 计算；模块不写 self。module/rule/system 是少量领域标记，
不增加 Queue、Reg、Interface、Input/Output 或调度 DSL。

- module/system 的结构 scope 表达连接、实例、状态初值与 rule 注册；capture 不执行函数/import/decorator 或用户代码。
- MLIR 按读写效果推导连接方向，不能从 `input_`、`output_` 等名字猜方向。
- 普通局部变量是组合计算；实例成员的持久状态、current/next 和复位由静态分析明确。
- “Pythonic”不意味着动态对象图、任意 Python 运行时或无法确定的整数位宽。finite type、静态结构、错误诊断必须明确。
- Python 不公开队列协议；普通 list/head/tail/ready 名称不触发协议推断。未来缓冲库用普通 reg 算法表达，任何内部识别优化都需等价证明，不能添加隐藏背压或存储。
- interface 由 MLIR 从实际 read/use、类型、controls 与 child bindings 导出；parent 用 header，link 用 bodies 独立重算。不让用户手写 effects 或优先级。
- CAS occurrence/cycle cursor、direct builder 和旧 function-style Agentic 接口列为退役对象。需要保留的流水线/寄存器能力改用唯一 lexical module/rule 接口表达；不默认移植整个 CAS 自动平衡系统。

整数是必须呈现给用户的实质差异：donor 采用数学整数与显式范围检查，当前 pyCircuit 有定宽运算/显式扩宽合同。不能暗中把 wrap 变成 trap，或用目标位宽静默截断。donor 当前单 symbol/单参数 tuple 与 pyCircuit finite-family 的差异也必须具体裁决。

### MLIR 分析与 lowering 的职责

| 阶段 | 应产生的事实 | 必需 verifier / 失败条件 |
| --- | --- | --- |
| capture import | 无损 AST、span、模块引用；可追踪的 source unit | malformed carrier、重复/不安全路径、非法节点拒绝；不执行 host code |
| 源语义分析 | 符号、类型/range、静态值、连接 effects、state owner、rule registration | 未绑定名称、无限/不合法类型、动态拓扑、结构重绑定、非法跨 owner 写入 |
| source-unit publication/link | 每源 body、interface、实例 graph、参数绑定 | parent 不读取 child body；缺/重复定义、schema mismatch、非法实例递归拒绝 |
| specialize/physicalize | 固定形状、寄存器/Queue/memory、reset、latency、current/next/enable | 不新增源码未表达的 bank winner、buffer、优先级；layout/owner 明确 |
| resource/driver/timing checks | 准入、原子组、冲突/互斥证明、clock/CDC、组合环、check coverage | 所有可能 active drivers 被覆盖；无 source-order winner；非法跨域/组合环拒绝 |
| final hardware verify | 两个 backend 消费的闭合硬件语义 | 禁 unresolved source/参数/非法 residual op；重新核实义务，不能仅信 stage tag |
| backend legalization/emission | 类型布局、RTL 原语选择、C++ 表达式和生命周期映射 | 不重建缺失的语义、仲裁或状态；不支持构造在输出发布前失败 |

优先迁移 donor 的实际 importer 与 pass 组织：specialize → 局部 CSE/simplify → topology → queue expansion → static loop unroll → queue view → rule preparation → queue-next binding → next-driver checks → final verification。Python import 与 link 不在 donor 这个命名 pipeline 内；目标将其纳入统一 driver。donor 的七项源分析目前部分是 importer 内部分解，不伪称已有七个独立注册 pass。

需从本仓保留的算法，例如字段冲突证明、有限 family 或 clock-domain 检查，改为这一条 MLIR 链的 pass/analysis。可以用内部 C++ 数据结构作分析缓存，但不得序列化一个需要 backend 重建语义的 QueueGraph 第二合同；最终义务必须可从 MLIR 独立验证。

### C++ 与 Verilog 的边界

两个 backend 消费**同一份 verified final hardware IR**，包括类型/宽度/符号、实例与 state owner、current/next、enable、clock/reset、memory/Queue 延迟、准入和冲突规则。C++ 调用顺序、host 指针、runtime `CanTransfer` 调用不能替代 IR 语义。

C++ 优先迁移 GFSIM `CodeGen*.cpp`、Evaluate/Check/Drive 和 Work/Xfer 的已成立机制；所需原子性与错误阶段由 IR 明确。Verilog 优先迁入 pyCircuit 已验证的 RTL emitter、原语目录、组合环/时钟域/层级检查，不保留旧 `pycc --emit=cpp` 路线。

若复用 RTL emitter 需要低层 PYC carrier，只能作为 **RTL backend 私有的、由 final hardware IR 单向 lowering 得到的表示**：没有源 frontend、独立 scheduler、通用 C++ emitter或公共第二条编译路线。语义分析不能因此滞留在旧 `QueueGraphPyc.cpp` 文本生成器。C2 提案需明确是否保留这一内部 carrier；其存在本身不构成多路线，多份语义权威才构成。

默认产品能力要求同一声明可由两 backend 接纳。不支持的 memory/CDC/四态/数据类型必须在能力表中明确裁决，不能靠 C++ 自动 fallback 通过，也不能静默缩小为只支持最简单模型。

## 迁移、保留与删除清单

以下是职责级范围；派发前把每项落实到精确文件、引用者、测试、安装和删除 owner。删除是新候选满足合同后的切换动作，不在本轮直接删源码。

| 对象 | 处理 | 切换出口 |
| --- | --- | --- |
| `v6.py` CAS/JIT 适配与 direct builder；`jit.py`、旧 `design.py` lowering | 退役旧源编译路线；有用 diagnostics/provenance 单独抽取 | 旧 import/CLI/API 不再编译模型；新源码只进 MLIR importer |
| structural `Circuit/Wire/Reg` authoring 与旧 exports | 退役公开 authoring；需要的硬件能力以批准的新接口迁移 | wheel 不导出旧模型 DSL；旧例子已迁移或被批准移出 |
| `_queue_compiler/` Python semantics 与 `_acc_py.py` 旧 lowering | 由 donor capture + MLIR importer 替换；删除解析后直接生成低层语义 IR 的路径 | Python 不作 authoritative type/effect/arbitration/storage 分析 |
| ACPy 并行语义产物/schema 与旧 capture worker | 退役旧合同；必要语法 carrier 采用一套批准格式 | 无第二个可执行/可 lower 的 Python 语义模型 |
| `QueueGraphPlan.cpp` / graph schema / emitter planner | 迁移适用算法到 MLIR 后删除语义引擎 | 最终 IR 独立 verifier 覆盖原有需要保留的反例 |
| `QueueGraphPyc.cpp` | 将必要 queue-to-hardware 算法转成 MLIR lowering 后删除 | backend 不再从 C++ graph 打印文本并重新导入以定义硬件 |
| `QueueGraphGenerator.cpp` 旧 C++/bundle emitter | 由 donor 风格直接消费 final IR 的 emitter 取代；移植必要 source-unit/SDK publication | 不保留按旧模式可运行的 fallback/bundle generator |
| `compiler/mlir/lib/Emit/CppEmitter.cpp` 与旧 pycc C++ 分派 | 退役重复 C++ backend | 只构建/安装一个模型 C++ generator；旧分派不可达且代码删除 |
| `VerilogEmitter.cpp`、`library/verilog/`、原语 catalogs 与适用 MLIR checks | 迁入新 backend；明确新输入 invariant | 与 GFSIM C++ 对同一 IR 执行，并保留所需硬件检查 |
| runtime、安装包、bindings、CLI | 合并为一套执行/发布合同；库名与 ABI 在 C3 提案裁决 | 无两个模拟器用于弥补语义缺口；无旧 backend 安装产物 |
| 测试/样例/docs/gates | 区分语义 oracle、接口测试、过时资产，逐项迁移/替换/删除 | 旧正例不为兼容继续绿灯；关键语义断言不随文件删除而丢失 |

不接受“删除入口但旧实现仍随 wheel 安装”“新 driver 委托旧三个 compiler”“先生成全芯片再拆源文件”“两个 C++ backend 一个叫 reference 模式”等形式上的收敛。历史基线可在独立旧 revision 做迁移对照，但不进入最终源码、依赖或安装包，也不是唯一 oracle。

## 必须先提交用户批准的合同包

下表保留最初 C1/C2/C3 的责任划分；当前 source/storage/system/interface
的精确替换以 R1 和 M1 C 联合增补为准。R1 的历史 approval-ready 或
原 C1-C 的 class 例子不能授权执行者恢复旧作者形式。

当前 AGENTS/Decision 0148 仍要求 CAS；用户最新方向已要求改变它。本轮先把需 supersede 的范围写清，不偷偷改现行决定的状态，更不因旧规范而保留三路线。以下三个包可分别准备；有依赖的语义必须一起审阅，用户可一次批准组合修订。

| 批准包 | 具体内容 | 受影响决定/合同 |
| --- | --- | --- |
| C1：唯一 Pythonic source | donor 对象式语法、import/装饰器、constructor/rule/system、record/collection、静态与运行时边界、整数/range、替代 CAS/structural/Agentic 的前后示例 | 0136、0148、0150；0282 中现行 Python 表面的引用 |
| C2：统一 MLIR 与硬件语义 | capture/semantic/final 阶段、op/type/attribute、类型与数学/定宽选择、state/Queue、reset/四态/clock、原子/冲突、源单位/interface、参数化能力、backend capability | 0220、0236–0241、0267–0270、0273–0278及适用 0279–0282 条款 |
| C3：toolchain / runtime / SDK | 唯一 CLI/driver、bindings、源到 AC producer、生成头文件/API、runtime 生命周期/ABI/库名、包结构、诊断与 schema、旧入口删除、发布时点 | 0149、0150、0232、0233、0264及 source-unit/发布相关条款 |

C2 必须明确 MLIR 如何提取/发布 source interface 与读写 effects，以及 parent 只凭 interface 编译的顺序。不能因 donor 从 module body 推导方向，就让 parent 重新读取 child body 或先整系统 capture。

每个包必须附精确接口、旧/新示例、语义差异、所有受影响调用方、保留/替换/退役能力、实现与删除顺序、正反例及用户可理解的代价。独立 Astra 对精确内容给出 approval-ready 后再请用户 approve；批准前只做分析、提案、基线和不触及该接口的授权工作。新修订若改变接口含义，重新审阅和批准。

不能把所有旧行为都当作要锁定的兼容合同。能力表每行必须分成：**新接口保持硬件行为、采用 donor 语义替换、经用户批准退役/延期**。未分类项是切换阻塞；最后一种须列明功能影响，不能由执行 agent 自行选择。规格冻结后，不影响接口/行为的内部重排可自主继续。

## 执行顺序与阶段出口

### M0：固定目标、基线与批准包

- B01：锁定两仓源修订与相关 dirty 内容，记录 donor component 的直接迁移/适配/补齐及来源；设计以 GFSIM 为默认，不重做一轮“是否保留三路线”论证。
- B02：建立旧 public API、真实 lowering dispatch、CMake/安装依赖图和能力矩阵；记录源码、测试、文档、gate 的迁移 owner。
- B03：选定寄存器、整数、Queue 背压、层级、reset、四态、memory/CDC 等代表性旧基线，记录有效语义 oracle、已知失败、工具链及编译/仿真成本。
- D01–D03：完成 C1/C2/C3，尤其对数学整数、CAS timing 退役、Queue/state 区别、source-unit 和多参数化作明确建议；独立审阅后提交用户批准。

出口：用户批准的单一目标合同、没有未分类的切换能力、可重跑的基线与首个双后端切片。方向已明确，接口批准才是实现前的决策门槛。

### M1：最少治理与迁移边界

保留 G01–G03 编号用于追踪。只要求明确候选、模块归属、实现/测试/
审阅责任和必要 donor 来源；现有机制足够后就结束本阶段。可选 agent
adapter、完整资产普查、流程模板扩展均不阻塞开发。

出口：任务能被有界派发、验证和集成。M1 不成为持续增加流程工作的队列。

### M2：可复现的最小双后端主干

保留 U01–U05，限定到已经选定的单级/两级 closed-system fixture：

- 普通 Python module/system 函数和嵌套 rule 经逐源 body/header 编译、
  link 和必要 MLIR lowering，形成可验证的共同硬件 IR。
- 统一 ac.reg、真实实例/alias 连接、proposal 与 Xfer/discard；同拍只读
  current，reset 正确，ports 不添加物理寄存器。
- C++/Verilog 执行同一受支持程序，独立逐拍 oracle、物理 reg 数和
  关键语义反例通过；保留当前用例需要的 assert/观察能力。
- 从当前候选重建并用一个明确命令或脚本复现。可以使用现有私有
  工具；完整公共 CLI、C ABI、SDK 和发布矩阵不作为 M2 前置条件。

出口：上述有界程序的 source → IR → 两后端闭环和窄回归有当前候选
证据，并完成该范围的独立审阅。不得要求 W11 全部完成后才验收 M2。
新暴露的计算、提交、链接或双后端行为错误仍阻塞这个出口；未来能力
或尚未交付入口的完整性不足登记到后续阶段。

### M3：按实际用例补能力

H01–H05 保留为能力 backlog，不要求一轮全部实现。每轮选择一个实际
用例，补齐它必须的整数操作、类型、集合、参数、库或模块能力，并在
共同 MLIR 上验证；完成该用例就关闭本轮。消费者模型仍在消费者仓库。

FIFO/reg buffer 库、memory、CDC、多域、四态、复杂事务和外部 typed
DUT ABI 按各自合同与需求进入后续轮次。没有明确用例的能力不抢占主线。

出口：所选用例及其声明支持范围通过，未支持项明确拒绝并可追踪。
不是“全部能力表每行都已实现”。

### M4：让当前能力可用

X01–X04 首轮只交付一条文档化的使用流程、少量代表性例子，以及当前
平台可用的构建入口。标准 compile/link/emit、source-owned C++ groups、
独立 TU/CMake 和需要的 runner 在此收敛；不得以整系统生成后拆文件
冒充逐源编译。C ABI、额外包装和分发形态仅在本轮使用流程确实需要时
成为前置；实现时仍遵循既定 C3 同一执行器合同。

若交付安装入口，当前平台必须在源码树外完成干净安装/import/run smoke；
若交付 emit/发布入口，invalid final 必须正确拒绝并保留已有输出。
这些基本保护随入口交付，不能延期到 M6。

出口：用户能从源码重复构建和运行当前支持的模型。不等待全部旧例子、
全部消费者、wheel 形态或平台组合迁移完毕。

### M5：在明确支持范围内执行 hard break

保留 R01–R04。切换前固定本次支持清单、明确暂不支持的能力及迁移
说明；只要求当前交付范围的必要能力闭合，不要求 M3 backlog 全部完成。
旧入口、fallback、重复语义引擎及其构建/安装引用按既定单路线目标退出；
相应决策、活跃文档与 gate 同步更新。

缺失能力不能靠旧 backend 补齐，也不能删除 oracle 冒充通过。尚未补齐
的库/能力保留明确后续责任；历史基线可用于对照，不进入新产品路线。
本次范围修订不执行源码删除或产品切换，实际切换仍须绑定具体候选。

出口：声明范围内只有一条产品路线，构建与运行确实走新链。

### M6：按风险持续加固

实际并行调度/重排验证、规模与增量性能、复杂故障注入、扩展
relocation、平台扩展和完整 SDK 矩阵安排在这里，按交付需求分别关闭。
在入口首次暴露时必须提供的基本校验和输出保护，仍由该入口所属阶段
完成；不能把已知数据破坏或现有正确性回归推迟到这里。

出口：本轮明确的稳定性或性能目标有测量证据，不作为所有早期交付的
全量前置。未实现并行时，不宣称已有并行执行能力。

### M7：按声明范围验收候选

同步文档和当前范围 release/smoke gates，先形成可复现、限制清晰的
预览候选；正式稳定发布再满足其所承诺的平台、兼容和可靠性条件。
不把预览候选称为整个迁移路线图完成，也不要求全部未来能力才能交付。
外部发布仍按既定授权和 upstream 流程执行。

出口：该候选实际承诺的能力、文档、构建和测试一致。剩余工作在
backlog 中明确可见，不作为已完成能力。

## 并行、文件归属与关键路径

| lane | 主要归属 | 依赖/限制 |
| --- | --- | --- |
| 设计/批准 | C1/C2/C3 与能力表，PM + 独立 Astra | 当前主线；不能用 governance activation 替代用户批准 |
| frontend/import | donor capture、`Python*.cpp` 迁入与 source-unit import | 先冻结 capture/semantic interface；不修改 backend |
| MLIR core | dialect/verifier/passes/analysis | ODS/registry 由一个 owner 串行；后端只消费冻结合同 |
| C++ backend | donor `CodeGen*.cpp` 适配和一套 runtime | 与 RTL 可并行，但共同 IR 和 effect invariant 必须已冻结 |
| RTL backend | migrated RTL emitter、原语选择、必要目标 legalization | 不保留旧 source compiler，不发明 C++ 未见的硬件行为 |
| independent tests | 数学/状态机 oracle、非法 IR、两个 backend stimulus | 不从被测 emitter 反推唯一预期，不修改实现掩盖失败 |
| integration/retirement | 根 CMake、driver/export/wheel、旧文件删除、gates | 单 writer，依赖检查之后切换；review 覆盖其新增差异 |

并发上限读取当前宿主，不把历史 PM+3 当成永久配置。逻辑 lane 按
阶段轮换；W00–W12 的包、文件归属与门槛见 checklist。每个 task 在
ready 前列精确文件、候选、接口授权、实际测试 inventory 和退出条件。
不同 build 目录不隔离源码写入；候选冻结后从本 checkout 自行构建。

关键路径：**已批准合同 → 最小双后端闭环 → 当前用例/可用入口 → 有界 hard break**。能力补齐与工程加固按需求滚动推进，预览交付不等待整个 backlog。

开发可以分切片提交到隔离候选，但产品切换是一个完整 hard-break 变更序列/候选。旧发布在旧 revision；不向当前产品暴露两个 mode。回退撤回完整切换候选并从恢复源码重新构建，不在新 API 内嵌 旧路线 fallback，也不重置别人的 dirty 工作。

## 取舍、风险与完成标准

### 方案比较

| 方案 | 判断 |
| --- | --- |
| 保留三 frontend、统一 CLI、继续用 QueueGraph/PYC C++ | 维护成本和语义分散没有消失；与最新单路线目标冲突，排除 |
| 原样拷入 GFSIM 并删除其他全部代码 | 复用快，但无法覆盖 RTL、source-unit 和待补源语义；不满足交付条件 |
| GFSIM 主干 + 必需 MLIR/RTL/source-unit 适配 + 旧链 hard break | 采用；复用成熟设计，新增工作集中在明确缺口，有可审查删除出口 |

### 失败预演

| 失败 | 早期信号 | 缓解/停止范围 |
| --- | --- | --- |
| 新 frontend 成为第四条路线 | 新 driver 仍调用旧 JIT、Python semantic lowering 或 QueueGraph generator | M2 路由追踪，M5 源码/构建/安装三层 deletion gates；失败阻断切换 |
| donor C++ 设计变成 simulator-only IR | RTL 需要从 C++ 调用重建 Queue/优先级或 hidden state | M2 同 final IR 双后端；MLIR 补齐义务，禁止 backend patch |
| hard break 被误解为可随意丢功能 | 旧测试批量删除，能力表空行，数学整数/四态悄悄变义 | C1/C2 锁定迁移/替换/退役矩阵；独立 oracle；未经批准的缺失阻断 |
| donor 未完成能力导致长期双模式 | `@system`/collection/check 等缺口用旧入口兜底 | 在新 MLIR 主干补齐或提交用户明确缩减；不通过兼容层延后删除 |
| 并行导致错误通过 | review/hash 与实际构建不一致 | 单 writer、固定接口、候选冻结，变化后按影响重测/重审 |

### 完整路线图的最终目标（不作为每次交付前置）

1. 唯一 Pythonic source 合同以 GFSIM 为设计基线；语义分析/lowering 在 MLIR，Python 不执行模型或充当第二编译器。
2. C++ 和 Verilog 从同一 verified hardware IR 出发，只有一个模型 C++ generator；目标专用 legalization 不成为第二语义权威。
3. 旧 CAS/structural/direct-builder/function-style Agentic 路线、QueueGraph 语义引擎和 PYC C++ 产品入口已退役，源码/构建/安装/docs 均无活跃兼容分支。
4. 批准能力矩阵闭合；保留行为、采用 donor 的行为变化和明确退役项都可追踪到用户批准及独立测试。
5. 逐源独立编译、source provenance、实例/参数 identity、硬件时序、reset、适用四态与原子性完整验收。
6. 所有接口改变覆盖在用户批准的精确提案中；独立 Astra、独立测试、Sol review、集成与 PM 验收证据对应同一内容。
7. required PR/semantic/release gates 已切到新路线且覆盖成立；未跑的门槛与未交付范围明确，不把计划审阅称作产品完成。

## 2026-10-01 implementation status

Revision-8 M4 and M5 are accepted for the declared current-platform scalar,
portless-root, default-clock, empty-static-argument and serial-execution profile.
The M5 implementation is `d351079b` on `codex/gfsim-source-units`; the planning
checkout remains separate. See [the M5 acceptance index](../work-items/m5-cutover.md)
for exact independent reviews, byte binding, gates and scope limits. M3 capability
backlog and M6/M7 hardening/release work remain; no old-route fallback is retained.

## 2026-10-01 M6-01 acceptance

The first bounded M6 packet is accepted in `477beae8`: real-process C3
publication recovery and complete moved-prefix compile/link/dual-emission plus
Runtime execution on macOS arm64. [M6-01 evidence index](../work-items/m6-publication-relocation.md)
binds independent review and results. It changes no product semantics or ABI;
remaining M6/platform/performance/parallel and M7 release work remains open.

## 2026-10-01 M6-02 acceptance

The second bounded M6 packet is accepted in `25152f9c`: measured shared-definition
1/16/64 and distinct-source 1/8/32 source-unit builds, real Ninja no-op/invalidation,
independent CPP/RTL oracles and C3 build-integration repairs. The
[M6-02 evidence index](../work-items/m6-incremental-scale.md) binds independent
review and final-candidate results. Source invalidation is selective; full emit
still rebuilds all generated CPP targets. RSS/throughput, parallel simulation and
broader platform/fault/SDK work remain open. M7 may next form a scoped preview
candidate; no stable release or expanded profile is claimed.

## 2026-10-01 M7-01 local preview acceptance

The scoped macOS 26/arm64 preview is accepted in `4584ad0b`, with fresh native
build/install, current presets, source-owned semantic closure including M6,
Runtime/CompilerDev/model-ABI consumers and a disposable wheel. The
[M7-01 evidence index](../work-items/m7-local-preview.md) records independent
review, candidate binding and explicit skips/deselections. No public interface
or release policy was changed. Stable release, formal platform/minimum-OS
acceptance and the broader capability roadmap remain separate; existing
`v6.1.0` points to older source and no publication was dispatched.
