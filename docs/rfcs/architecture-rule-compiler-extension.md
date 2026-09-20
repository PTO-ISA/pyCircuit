# RFC: Architecture Rule Compiler 扩展 —— 从 Rule 到 Obligation 的通用推导层

<!-- markdownlint-disable MD032 MD036 -->

**Status:** Proposed
**Baseline:** pyCircuit 6.1.0（决策登记截至 0276；release identity 位于 IR 外部）
**Target repository:** `PTO-ISA/pyCircuit`
**Primary validation workload:** large concurrent architecture 的 reduced generic fixture（完整设计留在 consumer repository）
**Scope:** Agentic Circuit / ACIR / 编译器分析 / codegen / 验证 / CBB refinement
**关联文档:**
- `docs/rfcs/pyc6-decisions.md`（决策 0148、0157、0158、0235、0241、0262、0263）
- `docs/acir/spec/agentic-circuit.md`（ACIR 规范，含 rule 原子性与提交语义）
- `docs/development/agent-frontend-guide.md`（agent 授权模型与硬规则）
- `docs/architecture/overview.md`、`docs/architecture/simulation.md`
- `docs/development/testing-and-gates.md`（gate 分层与证据约定）
- `docs/rfcs/ac-architecture-rule-rtl-verification-extension-checklist.md`
  （实现顺序、RTL/SRAM/X 约束、验证方法学与逐阶段退出条件）

---

## 摘要

本文提出 pyCircuit 的下一代扩展方向：把「架构意图」提升为编译器可推理的一等契约，
而不是继续往 framework 里堆特定处理器类别的专用 primitive。目标能力是从

```text
Structure + Rule + State Contract + Resource Contract + Recovery Contract
```

推导出 valid/ready、stall、credit、reservation、arbitration、state update、kill、recovery、
assertion、coverage obligation 与 implementation refinement。

本文同时给出**现状基线（gap analysis）**：逐条把需求映射到当前 ACIR/agentic_circuit 实现，
标明「已有 / 部分已有 / 完全缺失 / 命名冲突」。结论与直觉相反：

- 需求 A（Rule Effect Graph）已有本地分析事实与 Table writer 冲突证明，但**当前序列化摘要
  不完整**：它丢失规范化 index/predicate 表达式，F1 可以更改摘要表示并必须重新验证 live body；
- 需求 B（Obligation IR）没有可复用的架构生命周期；现有 `ac.marker.obligation` 仅是
  transient handshake lowering marker，架构 obligation 必须是独立的一等对象；
- 真正的缺口集中在**恢复语义（C/S/J）、多事务代数（D/E/F/G/H）、内存序（I）与
  可观测性（M/N/O）**，这些在当前源码中检索到的命中数为 0。

因此本文的实施建议不是重写，而是**在同一套 typed 基础设施上做受控扩展**，并优先解决命名冲突。
本文描述的全部是 design-neutral 的 framework 契约：任何具体处理器类别的结构、规模、
指令集与流水线组织都不进入本仓库。

## 动机

大规模并发架构的困难不在单个 module，而在同一拍内数百个持久状态对象、数十条并行 pipeline
与大量 transaction rule 之间的状态一致性。用「一个 Queue 接到另一个 Queue」的方式描述这类设计，
会把架构规则散落到各个状态 owner、等待队列、pipeline、assertion 和 testbench 里，
并且无法由编译器统一保证一致性。

现有体系已经覆盖 cycle-aware pipeline、hierarchy、register/memory/FIFO、Queue transaction、
Table persistent state、Slot、atomic rule firing、资源预留、credit、dependency scheduling、
writer arbitration、deterministic codegen 与 C++/Verilog backend parity。这些能力足以构造中等规模
流水线与部分架构状态，但不足以让编译器回答「哪些 rule 会写同一个 Entry」
「恢复事件之后旧响应能否污染新 Entry」这类跨 rule、跨 owner 的问题。

因此下一阶段的产出不应是若干新 HDL primitive，而应是一层 **Architecture Rule Compiler**：
让设计者只描述「什么条件下、哪个 architecture transaction、允许修改什么 architecture state」，
其余由编译器推导并保持各实现层一致。

## 目标

- 让 architecture transaction identity、state ownership、recovery 语义与同拍状态语义**显式化**。
- 建立 whole-design 的 Rule Effect Graph 与跨 rule 冲突判定。
- 把「可静态证明」与「必须运行期检查」分开，形成 Proved / RuntimeChecked / Rejected 三态。
- 支持每拍多事务、多资源、原子预留的事务代数。
- 为恢复与推测提供统一的 slot / generation / recovery-epoch / attempt 身份。
- 让 PPA 相关实现选择（banking、port、pipeline cut）成为可验证的 refinement，而非隐含行为。
- 让 agent 能直接查询「某条 rule 为什么没 fire」，而不是从 waveform 反推。

## 非目标

- **不在 framework 中加入特定处理器类别的 API**：任何以某个架构结构的名字直接暴露的构造
  （例如把某种窗口、队列或访存单元做成 `ac.<structure>()`）一律不做。
  这些结构应当由通用 building block 组合出来，并留在 consumer design。
- **不在 framework 契约与文档中出现 consumer 侧模型身份**。这不是本文的新主张：
  仓库已有回归测试在钉这条线，`test_document_freezes_installed_commands_and_opaque_abi`
  断言 `docs/development/sdk-release-contract.md` 中不得出现某个 consumer 模型名，
  与 `load_trace_json`、`agentic-model-trace`、`trace_position` 并列
  （`tests/python/agentic-circuit/contracts/test_sdk_release_contract.py:139-152`）。
  本文因此只用通用的架构能力词汇描述扩展，不绑定任何处理器类别、规模或指令集。
- **不把完整设计放进本仓库**。AGENTS.md 与决策 0158、0235 规定：完整 CPU/NPU/SoC/board
  设计、consumer testbench、ISA decoder、产品 payload 与 reference model 均留在 owning consumer
  repository，framework 语义保持 design-neutral。本文只授权 **reduced generic fixture**。
- **第一阶段不做 liveness/eventuality 性质**，只做 safety property。
- **不追求一次做完**。六个阶段必须顺序推进，宽配置不得先于窄配置。
- **不让 backend 修语义**。所有 semantic fix 必须落在 ACIR、analysis、verifier 或 lowering。
- **不让 Python 更像 Verilog**。信息只能被证明、推导、翻译或 refine，不能被 compiler 猜出来。

## 现状基线（gap analysis）

以下结论来自撰写和最近一次检查时的当前源码，并附 `file:line` 证据。仓库根相对路径省略
`compiler/acir/` 前缀时以 `lib/`、`include/` 标注。

### 已经存在的能力

| 能力 | 证据 |
| --- | --- |
| 单 rule 效果摘要骨架（consume / produce / state read / state write） | `include/acir/Dialect/ACIR/ACIRAttributes.td:204-222`（`RuleEffectKind`）、`lib/Transforms/LowerRules.cpp:165-314`（`inferRuleEffects`）；当前序列化表示不完整，不能作为 exact footprint |
| state 访问粒度到 field | `ACIRAttributes.td:239-255`（`RuleStateAccessKind` = read/replace/field_write） |
| state 访问索引粒度 | `ACIRAttributes.td:257-271`（`RuleIndexKind` = static/dynamic/all） |
| 当前摘要字段与函数体的验证 | `lib/Dialect/ACIR/ACIROps.cpp:203-411`（`verifyTypedRuleSummary`）；它不能验证未被序列化的 normalized index/predicate DAG，F1 必须补齐表示和交叉检查 |
| 跨 rule Table writer 冲突检测与仲裁 | `lib/Transforms/VerifyValueConstraints.cpp:192-354`（`verifyWriterArbitration`） |
| 仲裁优先级与跨 owner 环检测 | `VerifyValueConstraints.cpp:233-309`；`ACIRAttributes.td:127-152`（`WriterArbitrationPolicy/Resolution`） |
| rule 激活与事务资源分类 | `ACIRAttributes.td:80-98`（`ActivationResourceKind` = input_queue/output_queue/state/slot）、`LowerRules.cpp:345-409` |
| rule guard / schedule 分类 | `ACIRAttributes.td:100-125`（`RuleGuardKind`、`RuleScheduleKind`） |
| transient handshake marker 生命周期 | `ACIRAttributes.td:48-78`（`ObligationState` = pending/materialized/discharged；`ObligationResolver` = handshake/checks/schedule）；不得作为 Architecture Obligation IR |
| 动态索引值域证明 | `lib/Transforms/VerifyValueConstraints.cpp:369-501`；`:55-113` |
| Table 选择策略 | `ACIRAttributes.td:154-173`（`TableSelectionPolicy` = first/min/max/round_robin） |
| 多 lane Queue 类型 | `include/acir/Dialect/ACIR/ACIRTypes.td:60-68`；`lib/Dialect/ACIR/ACIRTypes.cpp:198-205` |
| assertion 落到两个 backend | `compiler/mlir/include/pyc/Dialect/PYC/PYCOps.td:384-389`；C++ `CppEmitter.cpp:1536-1542,1920-1926`；Verilog `VerilogEmitter.cpp:785-801` |
| 逐拍 trace 基础设施（comb / TICK-OBS / XFER-OBS） | `library/cpp/pyc_trace_bin.hpp:19,122-124,182-192`；解码 `tools/pycircuit/dump_pyctrace.py:20-35` |
| IR/op 覆盖门禁与 ledger | `tools/agentic-circuit/check-ir-coverage.py`；`docs/development/acir/verification/ir-coverage.md` |
| C++/Verilog backend closure | `tests/mlir/agentic-circuit/CodeGen/acc-driver.mlir` 与 `acc-python-driver.mlir` |

**关键结论：现有证明可复用，但需求 A 尚未实现。**
`verifyWriterArbitration` 对同一 owner 的每一对重叠 writer 依次检查：
field 不相交则放行（`fieldsAreDisjoint`，`VerifyValueConstraints.cpp:128-142`）、
索引可证不相交则放行（`:317-318`）、presence 可证互斥则放行（`:319-320`）、
同一 stable endpoint 内重叠则报错（`:341-343`）、
同一 scope 重叠则报错（`:344-346`）、
其余情况要求每个 writer 显式声明 `#ac.writer_priority`
（`"same-field overlap on owner @... requires explicit priority on every writer endpoint"`，`:347-350`）。

也就是说现有 analyzer 已能证明「A 写 `Table[i].field0`、B 写
`Table[i].field1` ⇒ 允许合并」并要求「A、C 同写 `Table[i].field0` ⇒ 证明或仲裁」，
且与代码出现顺序无关。不过当前 persisted summary 不携带 exact normalized
index/predicate DAG，不能据此声称 whole-design effect graph 或 portable exact footprint 已完成。

### 部分存在的能力

| 需求 | 已有部分 | 缺口 |
| --- | --- | --- |
| A Rule Effect Graph | 单 rule typed 摘要 + Table writer 冲突 | 没有 **rule→rule 边**；无 dump 接口（`--dump-rule-effect-graph`）；非 Table 效果（queue/slot）无冲突判定 |
| B Obligation IR | `ObligationState` 三态 + `ObligationResolver` 三值 | `checks` resolver 被显式拒绝（`LowerRules.cpp:427-436,546-555`）；`schedule` resolver 无实现（`:600-603`）；现有 obligation 语义是**rule 输出握手凭证**，不是架构性质 |
| F VersionedTable | `ac.table[...]` + field write + 仲裁 + `.match`/`.choose` | 无 generation/epoch 元数据；无 `invalidate_where`；无 slot 复用资格判定 |
| G AgeSelect-K | `TableSelectionPolicy`、`priority_encode`/`onehot_encode`、`.choose` round-robin cursor、`argmin` | 无「N 选 K 且满足资源约束」的语义原语；无执行资源类约束选择 |
| J Checkpoint | 无 | `ac.var`/`ac.table` 有快照读语义，但没有 capture/restore/release 生命周期 |
| N Rule Coverage | IR/op 覆盖门禁成熟；trace 有 Write/Reset/Invalidate chunk | trace schema 无 rule/guard/arbitration 事件；无 rule fired/blocked 覆盖 |
| Q Retained Result | Slot 提交语义保留 payload（`docs/acir/spec/agentic-circuit.md:1006`） | 没有独立 primitive；无 recovery-aware 资格 |
| T Atomic State Snapshot | old/new image 语义**已在运行期实现并被文档规定**（`simulator/gfsim/include/gfsim/queue_blocks.h:1657-1690,1912-1979`；spec `:1083-1084`） | 编译器不检查同拍 RAW；`same_cycle_forward` 无显式声明位 |

### 完全缺失的能力

以下符号在 `python/agentic-circuit/src`、`compiler/acir`、`schemas`、`simulator/gfsim`、`library` 中
检索命中数为 **0**：

| 能力 | 检索结论 |
| --- | --- |
| Recovery Domain / recovery epoch | `recovery` 命中 0；spec 中的 "recovery epoch" 只是示例里的普通 16-bit 标量（`docs/acir/spec/agentic-circuit.md:1591,1597`） |
| generation（作为事务身份） | 源码中 `generation` 只出现在 "code generation"；示例里的 `old.generation` 是用户自定义字段 |
| TransactionRef / versioned_id | 0 命中 |
| ExecutionAttempt | 0 命中；示例中的 `attempt` 语义是用户字段 |
| KillSet / invalidation | `kill`、`invalidat` 命中 0 |
| Checkpoint / rollback | `checkpoint`、`rollback` 命中 0 |
| replay / nuke / squash | 命中 0 |
| TransactionGroup / ValidPrefix | `prefix accept` 命中 0；`ac-resolve-rule-schedule` 要求每个输入 Queue 独占（`LowerRules.cpp:608-619`），因此每拍多事务的原子多资源 dispatch **当前不可表达** |
| ReservationSet | 预留是 compiler-owned；作者写 `reserve`/`reservation`/`commit` 会被拒绝（`python/agentic-circuit/src/agentic_circuit/_queue_compiler/parser.py:452-475`） |
| MultiAllocator | 只有单 `.allocate` endpoint 规则（`_queue_compiler/state_semantics.py:292-296`） |
| DependencySet（运行期 tracking） | `.depend(...)`/`DependencyBinding` 是**编译期** queue readiness 边（`_queue_compiler/graph_statements.py:190-284`） |
| MemoryOrderEdge / forwarding | 命中 0 |
| Why-not-fire | 命中 0；trace 无 rule/guard 事件 |
| 架构级 coverage manifest | 只有 IR/op 覆盖；`cover property` 全树命中 0 |
| Rule manifest | NDF 目前是**文档 profile**（`tools/agentic-circuit/check-ndf.py`），不是 rule manifest |
| SVA（DUT 侧） | DUT 只发 `ifndef SYNTHESIS` + `$fatal`；SVA 仅在 Python 生成的 SV **testbench** 中（`python/pycircuit/src/pycircuit/cli.py:1996-2035`） |
| C++ 断言宏 | 无 `PYC_ASSERT`/`PYC_CHECK`；`pyc.assert` 直接内联 `std::abort()` |
| gfsim 运行期 invariant | 只有 `static_assert` 与 `throw std::invalid_argument`，加上结构化 `fail(code,msg)` |

### 命名冲突与语义归属

这是本文最需要在落地前解决的问题。以下拼写**已经被占用**，新 API 不能直接复用其语义：

| 拼写 | 现有语义 | 证据 | 建议 |
| --- | --- | --- | --- |
| `obligation` | rule 输出握手凭证的 pending/materialized/discharged 生命周期 | `ACIRAttributes.td:48-78`；`LowerRules.cpp:479-561` | 保留为 transient lowering marker；架构级 obligation 使用独立的一等 operation/symbol，禁止把两种生命周期塞进同一 enum/resolver |
| `epoch` | `gfsim::Epoch` 仿真时钟；release 不编码进 IR | `simulator/gfsim/include/gfsim/core.h`；Decision 0268 | recovery epoch 必须显式命名，禁止裸 `epoch` |
| `generation` | compiler/code generation 通用术语 | compiler codegen | 事务 generation 用 `slot_generation` 或在 `TransactionRef` 内命名空间化 |
| `allocator` | `EntityAllocator`/`StableNameAllocator`，编译器内部命名与实体 ID 分配 | `_acpy.py:254`；`_naming.py` | 硬件分配器另立名字，或限定在 `ac.allocator(...)` 且文档明确区分 |
| `reservation` | compiler-owned 预留；作者使用被拒绝 | `parser.py:452-475` | `ReservationSet` 若公开，必须先修订该拒绝规则与 ACPY-RULE-014 |
| `invariant` | 纯 payload invariant（`@ac.invariant`，编译期） | `_definitions.py:193` | 运行期不变量不能叫 invariant |
| `transaction` | record 类型装饰器 `@ac.transaction` | `_definitions.py:171` | `TransactionGroup` 需避免与之混淆 |
| `dependency` | `ac.dependency` op 与 `.depend()`，编译期 readiness 边 | `graph_statements.py:190-284` | `DependencySet` 是运行期 tracking，命名需区分 |
| `refine` | capture-only marker `ac.refine` | `markers.py:132` | Requirement K 的 `@ac.refine(...)` **与现有 marker 同名，必须改名或复用** |
| `table` 容量上限 | spec 声明 `rank<=4, entries<=256, 65536 bits, ≤4 writers`，但 Python 侧无对应常量 | `docs/acir/spec/agentic-circuit.md:1636-1637` | VersionedTable 落地前需把该边界变成真实校验 |

## Requirement A：Rule Effect Graph

**需求。** 建立 whole-design 的 rule 交互分析：哪些 rule 读/写同一 state、哪些可能同拍、
哪些必须互斥、哪些可 merge、哪些存在 ordering relation；为每个 rule 产出标准化
`RuleEffectSummary`，并把 state footprint 精确到 owner / index / field / predicate。
提供 `BuildRuleEffectGraphPass` 与 `--dump-rule-effect-graph` 的 `.dot`/`.json` 输出。

**现状。** 单 rule 摘要骨架已经存在，当前字段会被验证，但序列化摘要仍不完整。
`RuleEffectSummary` 的字段中，`consumed_transactions`、`produced_transactions`、
`state_reads`、`state_writes`、`resources`、`arbitration_domains` 均有对应属性；
`activation_sources` 与 `transaction_resources` 已由 `ac-infer-rule-activation` 产出并精确校验
（`lib/Dialect/ACIR/ACIROps.cpp:131-201`）。

**差距。**

1. 缺 rule→rule 边。现有消费者只做验证、属性传播与 plan 提取
   （`lib/CodeGen/QueueGraphPlan.cpp:1935-1956`），没有构建跨 rule 图的 pass。
2. 缺 `recovery_domain` 与 `ordering_edges` 字段（依赖 Requirement C）。
3. 缺 `side_effect_class`。
4. 缺 dump 接口与 `.dot`/`.json` 呈现。
5. 冲突判定只覆盖 Table writer endpoint，queue/slot 效果不参与。

**阶段。** Phase 1。先允许调整摘要表示，持久化 exact typed index/predicate expression
DAG。每个节点使用 closed opcode、exact result type、ordered operands 与 typed attributes；
leaf 仅允许 rule input ordinal、committed owner/field/index、typed constant/static parameter
和 admitted lane identity。序列化 ordinal 只是 handle，不是 semantic ID。verifier 必须独立
normalize live rule body 后交叉检查；随后新增 `BuildRuleEffectGraphPass`，消费完整摘要并输出
rule→{state, resource, conflict, ordering, arbitration, obligation} 边。F2 必须复用现有
`verifyWriterArbitration` 与 value-constraint 证明，不得重新实现较弱的 overlap 判定。

**验收。** 构造三个 rule：A 写 `Table[i].field0`、B 写 `Table[i].field1`、C 写 `Table[i].field0`。
编译器必须允许 A+B 合并，并对 A+C 要求证明或仲裁，且结论与源码顺序无关。
对 Table 范围该判据已经成立，本阶段需要把它推广到 rule 图视角并有可导出的证据。

## Requirement B：Architecture Obligation IR

**需求。** 把静态检查扩展为三态：`PROVED` / `RUNTIME_CHECKED` / `REJECTED`，
并支持 mutual_exclusion、single_writer、resource_capacity、ready_valid_integrity、
transaction_atomicity、generation_match、epoch_match、ordering、range、onehot、
no_partial_commit、no_stale_update、credit_balance 等 phase-one safety obligation 类型，
按 backend 降低为 SVA / C++ assertion / gfsim invariant，三者共用 obligation ID。
compiler IR 统一命名为 Architecture Obligation。`progress` 与
`eventual_completion` 是明确拒绝的 future liveness kinds，不得伪装成 safety check。

**现状。** 骨架已存在但是**不同语义**：`ac.marker.obligation` 是 rule 输出握手凭证，
生命周期 pending → materialized → discharged，由 `ac-materialize-rule-handshake` 与
`ac-discharge-rule-obligations` 处理（`lib/Transforms/LowerRules.cpp:479-561`），
其检查内容是"每个 rule 输出都在显式 obligation 包裹下返回"。

**差距。**

1. `checks` resolver 被显式拒绝：`"dynamic checks are not executable in the supported pure rule
   subset"`（`LowerRules.cpp:427-436`）与 `"dynamic check obligations are not supported by typed
   summaries"`（`:546-555`）；测试 `tests/mlir/agentic-circuit/Transforms/rule-checks.mlir:1-18` 钉住了该拒绝。
   该 resolver 仍属于 transient marker，不能通过“打开”它来实现架构 obligation。
2. `schedule` resolver 无实现（`LowerRules.cpp:600-603`）。
3. 缺 obligation ID、kind、severity、condition、proof_status、runtime_policy、message 字段。
4. 缺按 backend 的运行期降低：C++ 目前对 `pyc.assert` 直接内联 `std::cerr` + `std::abort()`
   （`compiler/mlir/lib/Emit/CppEmitter.cpp:1536-1542,1920-1926`），没有统一宏、没有 ID、没有可关闭开关；
   Verilog DUT 只发 `$fatal`（`compiler/mlir/lib/Emit/VerilogEmitter.cpp:785-801`），无 SVA；
   gfsim 无 invariant 设施。
5. SVA 能力目前只存在于 Python 生成的 SV testbench（`python/pycircuit/src/pycircuit/cli.py:1996-2035`），
   DUT codegen 不产出 SVA。

**阶段。** Phase 2。`ac.marker.obligation` 保持 transient rule-output lowering
凭证，最终由 rule lowering 消除。架构级 obligation 必须使用 module-owned
`ac.arch_obligation` symbol operation，其 condition 引用 module-owned typed expression table，
并拥有自己的 stable ID、kind、proof status、typed sampling contract 与 runtime targets。
禁止扩展现有 marker enum/resolver 去承载第二套不相同的生命周期。
Stable ID 只能来自 declared structural name，不得来自 content、source line、traversal
counter 或 process-local counter。sampling contract 是 exact tagged union：
`tick_observation`→TICK-OBS、`xfer_observation`→XFER-OBS、rule/firing anchored
`pre_publish`、explicit event anchored `producer_event`。edge enum 为
`posedge|negedge|none`：前两者与 pre-publish 必须 `none`，只有 producer-event
可选物理 edge；tick/xfer 禁止 anchor，另两者要求 matching anchor。active/disable
predicate 在同一 event sampling，disable 是 synchronous sampled skip 而非 implicit async
`disable iff`。capture latency 以 cycle 计，仅允许 monitor-only producer-event capture，
不能参与 admission/mutation；union arms 互斥，active-only value 无 producer anchor 必须拒绝。
Mandatory release C++/gfsim checks 必须在 `NDEBUG` 下保留，并在受保护 mutation/publication
之前执行；任何 condition-changing transform 都必须 invalidate 并重新计算 proof 与 runtime
materialization。

## Requirement C：Recovery Domain

**需求。** 引入 generic 的 `ac.recovery_domain(name=, epoch_bits=)`；属于该 domain 的 transaction
自动携带 recovery epoch；引入 `TransactionRef { slot, generation, epoch }`、recovery event、
以及 kill/invalidation set，用统一 predicate 处理所有受影响资源。
recovery 不应是一根 Boolean wire，而应是一种事务。

**现状。** 完全缺失。`recovery`、`rollback`、`checkpoint`、`kill`、`invalidat`、`nuke`、`replay`
在源码中命中均为 0。`epoch` 的两个现有含义（工具链契约版本、仿真时钟）都与恢复无关。

**差距。** 这是最明显的 framework 缺口，也是本文的**最高优先级**之一。
需要新增：recovery domain 声明、transaction 身份中的 epoch 分量、
以及跨所有受影响 owner（索引状态、等待队列、映射表、pipeline、retained result、engine response）
的统一 kill predicate。

**阶段。** Phase 3（与 Requirement S 合并推进，因为两者都依赖身份模型）。

**验收。** T0 在 slot 3、gen0、epoch4 分配；recovery 事件推进到 epoch5；
T1 在 slot 3、gen1、epoch5 分配；旧 T0 的 completion 返回时，必须证明 T1 未受影响，
并生成对应的运行期 assertion。

## Requirement D：Multi-Lane Transaction Algebra

**需求。** 支持每拍多事务所需的 `TransactionGroup<N>`、valid mask、lane identity，
以及 AllOrNone / ValidPrefix / Independent 三种 commit 语义；
把「preview 资源可用性 → 原子多资源预留 → 提交」做成通用事务模式，
而不是硬编码为某条流水线的阶段。

**现状。** 缺失且**结构上不可表达**：`ac-resolve-rule-schedule` 要求每个输入 Queue 独占
（`"independent scheduling requires every input Queue to be exclusive"`，`lib/Transforms/LowerRules.cpp:608-619`），
rule 输入是每 Queue 一个 `!ac.var<elementType>` 而非每 lane 一个
（`lib/Dialect/ACIR/ACIROps.cpp:598-604`）。`lanes`/`rate` 只存在于 Queue 类型上，
lane 展开发生在 codegen planning 阶段（`lib/CodeGen/QueueGraphPlan.cpp:2770-2777`）。
`prefix accept` 全树命中 0。

**差距。** 需要 lane 级 rule 语义、prefix 接受判据、以及跨 owner 的原子预留集合语义。
这正是「某 owner 分到 4 个名额、另一 owner 只分到 3 个」这类分歧的根源。

**阶段。** Phase 4。必须按事务宽度递增推进，不得直接跳到最宽配置。

## Requirement E：MultiAllocator

**需求。** generic 的多分配器原语，支持 allocate K / free K / 同拍 free+allocate /
generation tracking / prefix allocate / priority allocate，
并把 `reuse_same_cycle` 变成显式 policy；自动生成 double allocate / double free /
allocated 与 free 集合不相交 / 占用上界 / 占用变化与分配计数一致等检查。

**现状。** 只有单 `.allocate` endpoint 规则（`python/agentic-circuit/src/agentic_circuit/_queue_compiler/state_semantics.py:292-296`）
与编译器内部的 `EntityAllocator`/`StableNameAllocator`（命名用途，非硬件分配器）。

**差距。** 缺多端口分配、缺 generation tracking、缺 `reuse_same_cycle` 语义声明。
注意 `allocator` 拼写已被内部工具占用，需在命名上区分（见命名冲突表）。

**阶段。** Phase 4。

## Requirement F：VersionedTable

**需求。** 在 `Table` 之上增加 slot 复用 + generation + selective invalidation：

```python
table = ac.versioned_table[256, SlotEntry](
    generation_bits=2,
    recovery=recovery,
)
```

entry 元数据为 `{valid, generation, epoch, payload}`，
`lookup(ref)` 自动包含 `valid && slot match && generation match && epoch match`，
避免 consumer 每次手工重复这四个条件。

**现状。** `ac.table[...]` 已具备 field write、仲裁、`.match`/`.choose` 选择、
以及分散的旧/新镜像语义；但**没有任何 generation/epoch 元数据**，
`ac.table` 的形态约束在 `_queue_compiler/state_semantics.py:318-357`，
`.match` 域上限 64 entries（`_queue_compiler/state_statements.py:650-658`）。

**差距。** 需要把 `TransactionRef` 的匹配条件下沉为 Table 的隐含查找语义。
同时需要把 spec 中的容量上界（`docs/acir/spec/agentic-circuit.md:1636-1637`）变成真实校验。

**阶段。** Phase 3。

## Requirement G：AgeSelect-K

**需求。** 从 N 个 ready candidate 中选出 K 个满足资源约束的 winner，
语义上只定义「哪些 candidate 合法、采用什么 ordering rule、选几个」，
不定义硬件实现；实现可以是 linear priority、balanced tree、bank-local + global、
age matrix 或 segmented selection，由 refinement 决定。
同时支持执行资源类约束（多种资源类，每类有固定槽位数），
第一版只支持静态资源类，之后再扩展 flexible matching。

**现状。** 已有 `TableSelectionPolicy`（first/min/max/round_robin）、
`priority_encode`/`onehot_encode`、`.choose` 的 round-robin cursor、
`min`/`max`/`first`/`argmin` 归约。没有「N 选 K + 资源约束」的语义原语。

**差距。** 「选 K 个」不是简单复制 K 次「选 1 个」，需要 prefix/独立接受语义（依赖 Requirement D）
与资源匹配（依赖 Requirement E 的资源类）。

**阶段。** Phase 4。

## Requirement H：Dependency Set

**需求。** 运行期依赖跟踪向量，支持 add dependency、resolve dependency、
`resolve_if(predicate)`、is_ready、kill dependency，
且 resolve **必须**以 `{slot, generation, epoch}` 限定，否则旧事务的 resolve 会污染新事务。

**现状。** `.depend(...)`/`no_dependency`/`DependencyBinding` 是**编译期** queue readiness 边
（`_queue_compiler/graph_statements.py:190-284`，`model.py:349`），不是运行期集合。

**差距。** 需要运行期、generation 限定的 resolve 语义，
并支持「某个 predecessor 可证不相交时提前解除依赖」这类 predicate resolve。

**阶段。** Phase 5。

## Requirement I：Memory Ordering Contract

**需求。** 不做完整内存队列 primitive，先做 Memory Ordering Graph：
表达 older_than / must_wait / may_bypass / must_forward / must_replay_if / visibility_before，
内部 IR 为 `MemoryOrderEdge { producer, consumer, kind, predicate }`。
第一阶段只做 Core-local ordering，不碰完整 ISA memory consistency model。

**现状。** 完全缺失（`MemoryOrderEdge`、resolve、forwarding、replay 命中均为 0）。
现有 `ac.memory` 只有单读单写、init 必须为 0、positive latency 的受限契约
（`_queue_compiler/memory_statements.py:200-260`）。

**差距。** 需要「地址未知则等待 → 证明不相交则解除 → 命中且数据就绪则可转发 →
命中但已执行则必须 replay」这一完整关系表达，并与 identity/recovery 语义对齐。

**阶段。** Phase 5。

## Requirement J：Checkpoint State

**需求。** `ac.checkpoint(state=[...], entries=16)`，支持 capture / restore / release / nested lifetime；
API 描述架构语义，实现可以是 full snapshot、delta log、mapping history 或 history buffer，
由 refinement 决定，而不是规定成整表拷贝。

**现状。** 缺失。`ac.var`/`ac.table` 有旧镜像读语义，但没有 capture/restore 生命周期。

**差距。** 需要与 Recovery Domain（C）一致的生命周期定义；checkpoint 的恢复边界必须与 epoch 语义对齐。

**阶段。** Phase 3。

## Requirement K：Rule Refinement Contract

**需求。** 把语义 IR 与物理实现解耦：Semantic IR → Refinement → Physical CBB，
refinement metadata 用独立 config（YAML）而非混入 functional Python；
建立 Semantic Primitive Registry 与 Implementation Catalog，
例如同一语义原语在不同参数下对应多个实现变体。

**现状。** 已有 `ac.resource` 的 capacity/issue_width/II/latency/lifecycle/arbiter/classes
（`include/acir/Dialect/ACIR/ACIROps.td:1400-1417`，验证 `lib/Dialect/ACIR/ACIRResources.cpp:478-535`），
有 topology-closure 后的确定性结构，但没有内容身份，也没有“同一语义对应多个实现变体”的目录结构。

**差距。** 缺 implementation catalog 与其选择规则。注意 `refine` 拼写已被 capture-only marker 占用
（`markers.py:132`），因此该配置不能直接叫 `@ac.refine(...)`。

**阶段。** Phase 6。

## Requirement L：PPA-aware CBB Selection

**需求。** 每个 implementation variant 记录 supported parameters / latency / II / area estimate /
logic depth / port count / target constraints；第一阶段只做 rule-based selection
（例如按宽度或并发度选择简单实现、balanced tree 或 banked 实现），
后续才引入 synthesis database。

**现状。** 缺 PPA metadata 与选择规则。IR/op 覆盖门禁与 ledger 已经成熟
（`tools/agentic-circuit/check-ir-coverage.py`、`docs/development/acir/verification/ir-coverage.md`），
可作为 implementation variant 覆盖记录的模板。

**阶段。** Phase 6。

## Requirement M：Why-Not-Fire Infrastructure

**需求。** 每条 rule 自动形成 FireCondition，由 input available / resource available / guard true /
recovery valid / output capacity / arbitration winner 组成；compiler 拆分未 fire 原因，
运行期支持 `--explain-rule <id>`，trace 产出 blocker JSON，使 agent 不必从 waveform 反推。

**现状。** 缺失。trace schema 只有 ProbeDeclare / CycleBegin / CycleEnd / ValueChange / Log /
Assert / Write / Reset / Invalidate（`library/cpp/pyc_trace_bin.hpp:182-192`），
**没有 rule / guard / arbitration 事件**；`did not fire` 相关检索命中 0。
另外 `ASSERT` chunk 已定义但**没有写入方**，且解码器显式忽略它
（`library/cpp/pyc_trace_bin.hpp:137`；`tools/pycircuit/dump_pyctrace.py:325-330`）。

**差距。** 需要 trace schema 扩展（rule id、guard、blocker 分类），
并把已有的 `ac.rule.checks_typed`（`RuleCheckKind` = input_available/output_capacity）
扩展为完整 blocker 分类。

**阶段。** Phase 2（与 B 共用 obligation/check 基础设施）。

## Requirement N：Rule Coverage

**需求。** 在 line/toggle/FSM 覆盖之外增加架构级覆盖：
rule fired、rule blocked、rule blocker、rule pair conflict、arbitration winner、
recovery、stale transaction rejection、atomic failure。

**现状。** 只有 IR/op 覆盖门禁（positive + negative lit 覆盖必须齐全）与 golden 测试；
无 rule/架构级覆盖，`cover property` 全树为 0。Python 侧 `pytest-cov` 只是声明依赖，未启用。

**差距。** 依赖 M 的 blocker 分类与 trace 扩展。

**阶段。** Phase 2 起步，Phase 6 收口。

## Requirement O：Rule Evidence Index

**需求。** 每个 rule 有 machine-readable evidence index（id / name / intent / owners / inputs /
effects / recovery_domain / requirements / obligations），compiler 自动生成人类可读文档。
该 index 只引用显式 semantic ID 与产物路径，不参与身份计算或 release 判定。
Markdown 不得成为 semantic authority，Python rule 与 verified IR 才是。

**现状。** 仓库已有 NDF 体系，但它是**文档 profile**：clause ID、metadata、typed edge，
由 `tools/agentic-circuit/check-ndf.py` 在 `docs/acir/spec` 与 `docs/rfcs/acir` 两个 root 上校验，
要求每个 clause 具备 `kind/level/layer/status`，且 `kind=req level=must layer=L1` 必须有 `kind=verif` 覆盖。

**差距。** NDF 目前是**规范来源**，而本需求需要的是**rule 级 evidence index**。
两者不能混用同一套名字，否则会破坏 `check-ndf.py` 的 fail-closed 语义。
建议：rule evidence index 使用独立命名空间与独立校验工具。

**阶段。** Phase 1 起骨架，随各阶段扩展。

## Requirement P：Terminal Transaction

**需求。** 一个 architecture transaction 可能分裂为多个 execution effect，
只有 terminal effect 成功后才允许置完成标志，以避免 partial completion，
尤其是在 recovery/replay 交叉时。

**现状。** 现有 rule 原子性保证"一次提交全有或全无"
（`docs/acir/spec/agentic-circuit.md:1002-1005`、`:1531-1534`），
但没有"多 effect 中哪个是 terminal"的表达。

**差距。** 需要 terminal effect 声明与 partial completion 禁止检查。

**阶段。** Phase 4。

## Requirement Q：Retained Result

**需求。** 可变延迟 engine 的 response 不能丢：accept once、hold until consumed、
preserve transaction identity、recovery-aware。
本质接近一深度 Queue，但带明确的 completion 语义与恢复资格。

**现状。** 语义最接近的是 Slot：成功后 payload 被保留
（`docs/acir/spec/agentic-circuit.md:1006`），且"epoch 开始时已满的 Slot 不能在同一 edge 释放并重填"
（`:1008-1009`）。但没有独立的 RetainedResult primitive。

**差距。** 需要独立 primitive，并具备 recovery 资格判定。Slot 的既有纪律已经预演了
generation/epoch 的语义，可作为设计起点。

**阶段。** Phase 3。

## Requirement R：Unified Identity Hierarchy

**需求。** 提供 generic 的 `TypedIdentity` 与 parent-child 关系，
使 compiler 能回答"这个 completion 属于哪个 architecture transaction"。
framework 不应知道具体业务身份名字，只提供层级与归属判定。

**现状。** 身份概念只有 stable naming（`_types.py:339,380`）、
ACPy 实体 ID（`_acpy.py:254`）、MLIR `stable_id`。没有 typed identity 父子层级。

**差距。** 需要统一的身份层级与归属判定，并与 `TransactionRef`（C）保持一致。

**阶段。** Phase 3。

## Requirement S：Speculative Execution Attempt

**需求。** 同一 architecture transaction 可能经历多次执行尝试，
因此身份还需要 attempt 分量，旧 attempt 的 response 必须失效。

**现状。** `attempt` 命中的只有诊断措辞与 `BuildAttempt`；没有执行尝试身份。

**差距。** 与 C、R 共用身份模型；这是"旧 response 不得污染新事务"的形式化基础。

**阶段。** Phase 3。

## Requirement T：Atomic State Snapshot Semantics

**需求。** 统一规定：所有 rule 求值读取 tick 起点的 committed state，
所有成功 proposal 在 tick 边界发布；若存在 forwarding，必须显式声明 `same_cycle_forward`，
不得由 Python 语句顺序决定。

**现状。** 该语义**已经实现且已被文档规定**：
"Same-tick reads observe old committed data, and a write becomes visible at tick commit."
（`docs/acir/spec/agentic-circuit.md:1083-1084`）；
运行期由 `SimTable` 的 `initial_/committed_/pending_/prepared_` 与 `commitWrite()` 保证
（`simulator/gfsim/include/gfsim/queue_blocks.h:1657-1690,1912-1979`）。

**差距。** 编译器侧不检查同拍 RAW：`stateFootprints` 分别记录读与写但不做配对
（`lib/Analysis/VariableAnalysis.cpp:908-953`），跨 writer 分析只收集写 endpoint
（`lib/Transforms/VerifyValueConstraints.cpp:115-126,196-216`）。
因此"同拍读自己或同拍写"目前是**未被检查**的，而 `same_cycle_forward` 没有显式声明位。

**阶段。** Phase 1（检查）+ Phase 2（运行期 obligation）。

## 编译器 Pass Pipeline 演进

建议在现有 pipeline 上**增量插入**，不改变既有顺序语义。
现有顺序为（`lib/Compiler/Driver.cpp:334-405`）：
`verify-ac-file` → `addRuleLoweringPipeline` → `normalize-ac-file` → topology closure → ACC QueueGraph/PYC lowering。

```text
Python
  |
  v
ACPy Capture -> ACIR
  |
  +-- [已有] verify-ac-file / ac-verify-model / ac-verify-value-constraints
  +-- [已有] ac-infer-rule-types / effects / activation
  +-- [新增] BuildRuleEffectGraph          (Requirement A)
  +-- [新增] Infer Transaction Identity     (Requirement R/S)
  +-- [新增] Infer Recovery Domains         (Requirement C)
  +-- [新增] Infer Ordering Edges           (Requirement A/I)
  +-- [新增] Infer Obligations              (Requirement B)
  +-- [新增] Prove Obligations              (Requirement B)
  +-- [新增] Materialize Architecture Obligations(Requirement B)
  +-- [新增] Resolve Multi-Lane Transactions(Requirement D)
  +-- [已有] ac-resolve-rule-schedule / ac-lower-rules-to-firing
  +-- [新增] Resource Refinement / CBB Selection (Requirement K/L)
  |
  v
Refined ACIR -> PYC -> C++ / Verilog
```

关键约束：F1 可以替换当前不完整的摘要表示；其 verifier 必须从 live body 独立
normalize 后验证 exact DAG。F2 的 graph pass 是只读分析，消费完成验证的 F1 摘要，
并复用现有 value-constraint/writer-arbitration proof。

## 实施阶段

### Phase 1：Rule Contract Foundation

- rule ID、Rule Effect Summary 复用、whole-design Rule Effect Graph。
- state ownership 与 access footprint 的跨 rule 视图（只读）。
- 显式冲突诊断与 dump 接口。
- rule manifest 骨架（独立命名空间，不复用 NDF）。
- 验收 workload：索引状态窗口、待选队列、映射表这三个 reduced generic fixture。
- 完成标志：compiler 可以打印所有 rule 的读写 state，以及 rule 之间的冲突。

### Phase 2：Obligation and Verification

- 新增 module-owned `ac.arch_obligation` symbol operation 和 module-owned typed
  expression table，引入 proved/runtime_checked/rejected 处置；
  不复用 `ac.marker.obligation` 的 resolver 或生命周期。
- onehot、single-writer、atomic transaction、ready/valid integrity、stale update check。
- C++/gfsim 与 Verilog SVA 使用同一 typed condition、obligation ID 和 sampling contract。
  若 admission 依赖 runtime check，该检查不可关闭；runtime assertion 不能授权 overlap、
  onehot optimization 或 synthesis/deployment legality。
- Why-not-fire 的 blocker 分类与 trace 扩展。
- 完成标志：一条 Python rule 的 invariant 能自动出现在 C++ model 与生成的 Verilog SVA 中，且共用同一 obligation ID。

### Phase 3：Recovery and Identity

- RecoveryDomain、TransactionRef、slot generation、recovery epoch。
- VersionedTable、Checkpoint、Kill Set、RetainedResult、TypedIdentity、ExecutionAttempt。
- 验收 workload：推测路径被取消时，索引状态窗口、映射表、待选队列与 completion 的全链恢复。
- 完成标志：旧 completion 不能污染新 state，且该性质有运行期 assertion。

### Phase 4：Wide Transaction Concurrency

- TransactionGroup、ValidPrefix、ReservationSet、MultiAllocator、AgeSelect-K、
  multi-writer Table、multi-completion、multi-retire、Terminal Transaction。
- 顺序：每拍 4 个并发事务 → 8 个 → 10 个。

### Phase 5：Dependency Tracking and Memory Ordering

- DependencySet、Tracking relation、MemoryOrderEdge、resolve、forwarding、replay identity。
- 验收 workload：load behind unresolved store、non-alias resolution、alias forwarding、
  late violation、recovery during outstanding load、old response after replay。

### Phase 6：Refinement and PPA

- Refinement Contract、CBB implementation catalog、PPA metadata、
  banking、port mapping、pipeline cut、implementation selection。
- 到这一阶段才真正回答"自动生成 RTL 的 PPA 到底行不行"。

## Framework Regression Suite

新增 `tests/integration/architecture/`，至少覆盖：
atomic multi-resource dispatch、versioned slot reuse、recovery epoch stale completion、
multi-lane prefix accept、age-select-k、independent completion、tracking dependency、
same-cycle allocate-retire、writer conflict、field-disjoint merge、checkpoint restore、
replay attempt identity。

注意：这些 fixture 必须是 **reduced generic** 的，不得包含完整处理器。
`tools/agentic-circuit/check-release-layout.py` 对 `examples/pycircuit/**/*.py` 有
system/soc/board/cluster 命名拒绝规则，新 fixture 需遵守同样的 design-neutral 边界。

## Acceptance Demo

整个扩展最终需要一个 framework 级 demo，但**不是完整设计**。
建议一个 reduced concurrent-architecture fixture：

```text
每拍 4 个并发事务 / 32 个索引状态槽位 / 16 个待选槽位 / 32 个可分配标签
多资源类执行槽位 / versioned completion / 恢复事件驱动的取消与重放 / 简单访存依赖
支持 dispatch, map, select, execute, completion, retire, recovery
```

验收方式：随机跑 1M+ transactions，验证 C++ / Verilator parity、zero-loss、precise retire、
recovery correctness、stale response rejection、allocator integrity、ordering 正确性、
no-double-issue。Python 模型建议控制在 2,000 meaningful LOC 以内——重点不是硬卡 LOC，
而是验证抽象是否真的有效。

## 建议新增的 Generic API 与命名

优先考虑 `ac.recovery_domain()`、`ac.versioned_id()`、`ac.versioned_table()`、
`ac.transaction_group()`、`ac.reservation_set()`、`ac.allocator()`、`ac.age_select()`、
`ac.dependency_set()`、`ac.checkpoint()`。

但**不建议一次全部 public**：先作为 internal compiler object，
待 semantic contract 稳定后再公开 Python API。公开前必须先解决
§「命名冲突与语义归属」中的每一项，尤其是 `refine`、`epoch`、`reservation`、`invariant`、
`dependency` 五个已占用的拼写。

## Definition of Done

以下清单是这一轮扩展的完成判据。注意"能 compile"不构成完成。

**Architecture**
- Architecture transaction identity explicit
- State ownership explicit
- Recovery semantics explicit
- Same-cycle state semantics explicit

**Rule compiler**
- Whole-design Rule Effect Graph
- Rule conflict detection
- Writer arbitration
- Resource reservation
- Multi-lane transaction inference

**Verification**
- Obligation IR
- Proved / RuntimeChecked / Rejected
- C++ assertion generation
- SVA generation
- Rule coverage
- Why-not-fire

**Generic architecture primitives**
- VersionedTable / TransactionRef / RecoveryDomain
- MultiAllocator / AgeSelect-K / DependencySet / Checkpoint

**Backend**
- PYC lowering / C++ model / Verilog
- C++ / RTL parity
- Deterministic codegen

**Recovery**
- cancellation / replay / stale completion / slot reuse / epoch transition

**Multi-transaction concurrency**
- multi-dispatch / multi-allocation / multi-select / multi-completion / multi-retire

**Memory ordering**
- dependency tracking / forwarding / replay / load-store generation / stale response

**Physical realization**
- CBB refinement / implementation variants / PPA regression

## 架构原则

整个扩展过程中需要一直守住一条线：

> pyCircuit 不应该努力让 Python 更像 Verilog，而应该让 Python 更接近 Architecture Intent。

```text
Architecture Intent -> Rule -> Effect -> Obligation -> Transaction
                    -> Resource -> Refinement -> PYC -> RTL
```

信息只能被**证明、推导、翻译或 refine**，不能被 compiler 猜出来。
这也是本轮扩展与普通 HDL feature 最本质的区别：价值不在于少写多少行 Verilog，
而在于原来散落在各个状态 owner、等待队列、pipeline、assertion 和 testbench 中的同一条架构规则，
只描述一次，然后由 compiler 保证它在所有实现层中保持一致。

## 落地流程与未决问题

**Status 为 Proposed implementation RFC。** Decisions 0271–0276 已接受本文中 F0
收敛的 contract；本文其余 recovery、transaction、memory-order 与 refinement 内容仍需按阶段决策。

1. Decisions 0273–0276 在 `docs/gates/decision_status_v6.md` 中保持 `gap-in-scope`；
   Decision 0272 保持 `implemented-unverified`，直到各自实现与验证证据完整。严格 release closure 因这些行而 fail-closed 是预期行为，
   不得用虚假的 `implemented-unverified` 绕过门禁。
2. 若要把通用契约写成规范性条款，放入 `docs/rfcs/acir/` 或 `docs/acir/spec/` 时必须满足 NDF 约束
   （clause 级 `kind/level/layer/status`、边引用必须可解析、
   `kind=req level=must layer=L1` 必须有 `kind=verif` 覆盖）。
3. 每阶段的实现 PR 必须附 ACIR lit、C++/gfsim 或 PYC parity 的窄证据，
   并按 `docs/development/testing-and-gates.md` 的映射选择 gate。

**未决问题。**

- recovery epoch 与 `gfsim::Epoch` 的命名空间如何划分？release identity 不进入该命名空间。
- `ReservationSet` 公开化后，`parser.py:452-475` 对作者写 `reserve`/`commit` 的拒绝规则如何修订？
- VersionedTable 的 `generation_bits` 与 spec 的 256-entry / 65536-bit 上界如何统一校验？
- 验证构建如何选择附加 runtime/SVA 诊断而不改变 admission 语义？依赖 runtime check 的
  obligation 不可关闭，synthesis/deployment profile 必须提供静态 proof。
- rule evidence index 与 NDF 的关系如何在文档中避免语义重叠？
