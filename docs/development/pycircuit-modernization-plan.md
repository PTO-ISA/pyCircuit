# pyCircuit 单一路线重构与 GFSIM 迁移计划

日期：2026-09-27。规划修订：6。状态：C1-C/C2-C/C3-C 已获用户批准，隔离候选实施中；正式产品切换尚未完成。最新进度见[执行账本](../work-items/single-route-migration.md)。

**目标只有一条产品编译路线：GFSIM 风格 Pythonic 源码 → 语法捕获 → MLIR 语义分析与 lowering → 经过验证的硬件 IR → GFSIM C++ 或 Verilog。** 优先迁移 GFSIM 已有设计与实现，以 hard break 退役 pyCircuit 的旧编译路线。

用户最新要求取代上一版“保留 CAS、structural、Agentic 三种 authoring 路径”的前提。单一路线是已明确的架构方向，不再反复确认。具体 Python、IR、CLI、生成代码或 runtime 接口的改变，仍须先完成可审阅提案、独立技术审阅，再取得用户对精确修订的批准。**本规划不代替接口批准。**

配套：[治理与调度](project-governance.md)、[验收规范](pycircuit-modernization-tests.md)、[独立审阅记录](pycircuit-modernization-review.md)。下文来源分析记录规划时点；已批准合同的基础实施与测试另见执行账本，完整决定与产品切换按 M5 完成。

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
| 普通 class/method/record，加 `module/rule/system` | `frontend/ObjectFrontendProposal.md` 修订 C；`docs/IR/approvals/object-frontend.md:14` | 本仓精确接口批准、现有例子的硬迁移；不再保留三套 DSL |
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
Pythonic source（GFSIM 对象式设计，精确接口待本仓批准）
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

目标优先采用 GFSIM 的普通 class、constructor、method、record、局部变量、函数调用、静态 `if/for`、规则内条件和标准类型注解。`module/rule/system` 是少量领域标记；不新增三种 authoring 风格的选择器。

- constructor 表达静态连接、实例、状态初值与 rule 注册；capture 不执行 constructor/import/decorator 或用户代码。
- MLIR 按读写效果推导连接方向，不能从 `input_`、`output_` 等名字猜方向。
- 普通局部变量是组合计算；实例成员的持久状态、current/next 和复位由静态分析明确。
- “Pythonic”不意味着动态对象图、任意 Python 运行时或无法确定的整数位宽。finite type、静态结构、错误诊断必须明确。
- FIFO/Queue 消费与普通多 reader state 不是同一种连接；精确源表达必须批准，不能用语法简化隐藏背压或额外存储。
- CAS occurrence/cycle cursor、direct builder 和旧 function-style Agentic 接口列为退役对象。需要保留的流水线/寄存器能力改用唯一对象式接口表达；不默认移植整个 CAS 自动平衡系统。

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

### M1：治理与 donor 准入，并行推进

治理不是等全部文档完美才开始技术设计的长前置。PM 沿用 [调度方案](project-governance.md)，先启用文件 owner、候选冻结、独立测试/审查和轻量任务包。

- G01：正式治理入口、project PM/design-review skills、任务/审阅模板；保持生成标记和现有未完成工作。
- G02：迁移 donor source provenance、文件和依赖清单、一个 pass 一个职责、源文件规模 debt/ratchet。
- G03：可选 DeepSeek adapter 独立包；新增 CLI/配置先批准，mock 失败路径先测；不阻塞核心 compiler 迁移。

出口：真实任务试运行、真实模型绑定和 independent review 记录；不把配置文件存在当成调度可用。不继承 GFSIM full-core 的 PM 代批权限。

### M2：先打通唯一主干和双后端最小闭环

依赖：对应 C1/C2/C3 已批准。实现放隔离候选；不向正式产品增加第四个可选择前端。

- U01：迁入 donor syntax capture 和 MLIR importer；按批准合同处理单 source 与 interface。
- U02：迁入 donor source analysis/ACIR/verifier/passes；补齐首切片的精确类型、state owner、rule current/next、reset 和 driver checks。
- U03：迁入 GFSIM C++ emission；同时为**同一 final IR**实现最小 RTL backend，可抽取现有 RTL emitter 所需部分，禁止调用旧 Python/QueueGraph semantic compiler。
- U04：首包先实现 C1/C3 批准的最小 top/root 选择与逐源 composition 入口，不依赖后续完整 `system`/testbench，也不能用旧 frontend 补齐。独立 fixture 用新 Pythonic source 描述带条件 enable/reset 的计数叶模块，父模块实例化两次；增加单 Queue 的阻塞/收发场景。验证 source-unit producer、实例独立性、同拍 current、逐拍结果和两个 backend。
- U05：检查标准 pipeline/driver 的完整 provenance；删除候选中该切片对旧 lowering 的依赖。首个闭环不过，不扩面。

出口：新源码 → 逐源 semantic ACIR → linked final hardware IR → C++/Verilog 的可执行证明。不能先做完全部 C++ 再发现 RTL 无法表达。

### M3：补齐批准能力，语义留在 MLIR

- H01：整数/record/collection/参数化，保留精确诊断和 source maps；donor 不支持项在新 MLIR 链补齐，不回旧 Python compiler。
- H02：Queue、持久 state、memory、slot/多输出/多 lane（仅批准范围）的合法性、冲突与 atomic effect，明确 hold、backpressure、reset/failure。
- H03：时钟复位、组合环、CDC、memory 时序、value/known/Z 和 RTL primitive selection；将适用现有算法作为新链 MLIR pass。
- H04：source unit、headers、linker、实例与参数 identity；一个源 body 独立编译、生成 source group 并独立编译链接。
- H05：补齐 source `system` 与通用 testbench、SDK 生命周期，保持 consumer-neutral；每项同时更新 C++/RTL coverage。

出口：批准能力表每行都有新路线正反例与独立语义证据；与旧基线相同的语义比较结果，批准改变的语义按新 oracle 验收。未知项不以只测 C++、缺失 RTL 或 silently unsupported 通过。

### M4：迁移使用面并准备一次切换

- X01：把 maintained examples、integration fixtures、testbench、文档示例改成唯一 Pythonic source；旧消费者迁移在其仓库对 pinned revision 进行。
- X02：逐项替换 Python/MLIR/runtime/SDK tests，保留所需语义反例；给 retired API 添加明确拒绝测试。
- X03：根 CMake、安装/export、CLI、Python bindings、wheel、gate/workflow 改用一个 driver、一套 pass registry 和唯一 C++ generator。
- X04：先建 deletion manifest 和依赖反向搜索清单，冻结“新输入/输出/语义证据 + 被删接口/实现/测试/文档”的完整候选。

出口：可以删除旧路线且没有活跃依赖；不允许“以后再迁某个 example”作为继续安装旧 frontend 的理由。

### M5：hard break 切换与结构清理

- R01：按第 3 节删除旧 frontend/JIT/direct builder、Python semantic lowerer、ACPy 并行合同、QueueGraph 语义路径、旧 C++ emitters 及相应注册。
- R02：移除旧 flags、模式开关、隐式 fallback、别名和安装资产；正式入口不可再选择旧路线。
- R03：收拢新源码目录和 CMake ownership。仅整理最终存活的模块，不花大量时间给将删除的 8k/9k 行文件做长期重构。
- R04：切换同一候选同步写入已批准决策的 supersession、AGENTS/skills、活跃语言/接口文档和 gate 改动；不能推迟到 M7。完成独立 code review、架构符合性、静态引用/动态执行路由/installed wheel 检查，以及能力矩阵全量验证。

出口：一个 frontend lowering、一个共同 MLIR semantic pipeline、两个 backend（一个 C++，一个 Verilog）。旧参考 revision 仅作历史证据；产品树内没有兼容桥或第二个可执行 lowering 引擎。

### M6：规模、性能和 SDK 收尾

在正确性稳定后，测逐源并行编译、重复实例、相同/不同静态参数、增量失效、源码定位、输出发布失败与 root relocation。比较 stage time、RSS、生成代码规模、编译/仿真成本；采样与阈值由 M0 基线确定，不用文件变短宣称性能提升。

出口：source-unit、SDK、构建图和平台证据对应同一 frozen candidate；必要性能退化有证据和明确处置，不能通过恢复旧 backend 解决。

### M7：规范切换与发布候选

复核 M5 已同步切换的决策、AGENTS/skills、语言手册、唯一 frontend guide、IR/pipeline、gate/workflow 和 README，完成发布层面的闭环；不在此阶段才第一次废止旧合同。完整 release gates 迁移到新路线后仍保持语义覆盖，不保留旧 gate 命令只是为制造通过，也不删掉关键断言消除失败。

出口：独立审阅问题关闭、所有批准能力验证、安装包仅含新路线，发布候选按当时正式 upstream 流程验收。发布版本号、包名/ABI 和外部发布动作遵循 C3 及用户授权，本规划不直接发布。

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

当前最多 PM + 3 个原生子 agent；逻辑 lane 按阶段轮换。每个 task 在 ready 前列精确文件、候选、接口批准、测试命令和退出条件。不同 build 目录不隔离源码写入；候选冻结或使用正确 source overlay 的 worktree，从该 checkout 自行构建。

关键路径：**能力与接口批准 → 共享 MLIR 合同 → 同 IR 双后端最小闭环 → 必需能力闭合 → 使用面迁移 → 一次 hard break → 规模/发布验收**。治理和静态调查可并行，不把治理文件数量作为里程碑。

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

### 项目完成标准

1. 唯一 Pythonic source 合同以 GFSIM 为设计基线；语义分析/lowering 在 MLIR，Python 不执行模型或充当第二编译器。
2. C++ 和 Verilog 从同一 verified hardware IR 出发，只有一个模型 C++ generator；目标专用 legalization 不成为第二语义权威。
3. 旧 CAS/structural/direct-builder/function-style Agentic 路线、QueueGraph 语义引擎和 PYC C++ 产品入口已退役，源码/构建/安装/docs 均无活跃兼容分支。
4. 批准能力矩阵闭合；保留行为、采用 donor 的行为变化和明确退役项都可追踪到用户批准及独立测试。
5. 逐源独立编译、source provenance、实例/参数 identity、硬件时序、reset、适用四态与原子性完整验收。
6. 所有接口改变覆盖在用户批准的精确提案中；独立 Astra、独立测试、Sol review、集成与 PM 验收证据对应同一内容。
7. required PR/semantic/release gates 已切到新路线且覆盖成立；未跑的门槛与未交付范围明确，不把计划审阅称作产品完成。
