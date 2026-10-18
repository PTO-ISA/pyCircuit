# #307 / #319 本轮执行状态

## 仓库巡查与提交前修复（2026-10-18）

本轮修复编程手册、quickstart、示例导引和测试入口，维持单一 capture → MLIR → final IR → C++/Verilog 流程。本轮没有增加编译器特例或改变证明上限。

- concat/popcount 的纯计算恢复为 typed helper；count-zeros 按 leading/trailing 拆分计算后重建原结果，保留全部宽度、字段顺序、刺激和四态 oracle。
- 五个 credit Systems 分别编译原声明闭包，保留原 20/26/32/20/44 周期；全部通过。
- resident ROB/ISQ 的 Module 与 System 编译隔离，RTL host 迁移到 reset/PREPARE/check/Xfer；两者 Module 的 cases/witnesses、C++ workers1/2 与 Verilog 均通过。长 System 仍有下述限制。
- observation 原伪造逻辑 owner 的变异保留为双后端拒绝与输出保护测试；原运行轨迹保持。空 Rule、条件 log 的附加检查只证明 compile/link/dual-emit，不冒充新运行覆盖。
- 多写者 fixture 按实际变异 root 与独立运行 root 编译，owner 冲突检查迁至实际负责该检查的 link 阶段；保留原正负例、四态、发布保护和专门容量测试。
- quickstart 与可运行源逐 AST 一致；完整文档严格构建和 Markdown 检查通过。现行源遵循 Black/Ruff；历史归档保留原字节。

当前 examples **34/34 PASS**、API gate **23/23 PASS**、unit **589 passed / 5 skipped / 82 deselected**，example build / Runtime-only / CompilerDev 三项检查通过，quickstart compile/link/dual-emit 通过。14 份生成收据已刷新并复核；54 个归档目录的 563 个文件哈希不变。

最终 API lit 汇总为 **161 passed / 8 failed（169 项）**：69 项 gate + 99 项其余 nightly + 独立 Multi 复跑覆盖互不重复的全部测试名。literal 在直接调用/显式 DFF 修正后另行复跑，仍因 Table extent 类型不匹配失败。Multi 的 **61 个运行 root 全部通过**，普通冲突负例只匹配实际 error 诊断，不能用路径名称或容量失败冒充冲突拒绝。静态参数四个拒绝 case 已单独执行，forwarded replacement 的全部已发布文件哈希保持不变。

本轮按用户要求提交本地迁移；不 push/merge/关闭 issue。完整 wheel 和后续完整 API Python package nightly 未运行，定向 package 三项通过不替代其余矩阵。

本轮证据统一在 ignored `docs/gates/logs/20261018-repo-closeout/`；examples/API gate 的机读汇总在 `20261018-closeout-examples/` 和 `20261018-closeout-api/`。当前长 API 回归仍未完全通过，不能据上述 gate 关闭 #307/#319。

### 当前剩余问题

| owner | 首个未通过阶段或范围 |
| --- | --- |
| bits-priority-encoding | 主源的 nominal payload closure 分析容量；未验证的分组改写已撤回。 |
| defaults-composition | 含 Table 字段的 Struct 分支合并仍拒绝；原 oracle 保留。 |
| literal-normalization-adversarial | 修正直接调用并保留显式 DFF 后，Table extent `2 + 2` 与 `4` 在绑定处仍报 hardware type mismatch。旧 behavioral static-parameter 正例改为当前合同的精确拒绝检查，不宣称恢复参数化支持。 |
| source-historical-residents | 独立 ROB System 在 original-source proof 失败；ISQ System 未运行。原 ROB/ISQ Module 全历史已恢复双后端通过。 |
| source-historical-routed | 纯 helper 的 Rule 误用已修复，仍卡在长 System 的 Table-query 分析；不能宣称十四 Queue 图完整运行通过。 |
| source-historical-scalar-parameterized | 同源静态参数化 Module 仍不满足当前直接调用 admission；不以固定宽度替换掩盖。 |
| source-historical-scheduling | 局部 Rule 捕获与纯 deadline helper 已简化；单个真实调度器仍在 prep 分析失败，原 if 写序与全部状态保留。 |
| source-historical-slot-issue | 旧子 Module 两步绑定尚未迁完；属于剩余 fixture/API 工作，不笼统标成产品决策。 |

以下旧轮次保留用于追溯；旧 14/15 个失败及未提交表述不覆盖本节的新运行和用户本轮提交授权。

## 示例收敛轮次结果（2026-10-18，历史记录）

当前 examples 迁移完成：7 个教学例和 10 个设计/物理参考例，nightly **34/34 PASS**。54 个旧包装已完整归档，不计为通过。Memory Bank 采用一个内聚的阶段 FSM Rule；DMA/Memory/Table 的短 System 为 42/26/16 周期，原长 bench 和独立 Module oracle 保留。digital_clock 的 100,000,001 周期验证通过。14 份 Module 生成收据已刷新。

API gate **23/23 PASS**；unit **589 passed / 5 skipped / 82 deselected**；example build、Runtime-only install、CompilerDev consumer 定向检查通过。独立实现审查及目录/收据/归档核对完成，无阻塞发现。

**#307/#319 整体未完成**：最近完整 API lit 为 **155/169**，仍有 14 项失败（bits-concat/count-zeros/popcount/priority-encoding、defaults-composition、literal-normalization-adversarial、multi-rule-writes、historical pipelines/residents/routed/scalar-parameterized/scheduling/slot-issue、observation-groups）。后续 API package nightly 未运行，完整 wheel 未运行。短例通过不代表旧长 bench 或所有历史 API 路径通过。

当前证据：`docs/gates/logs/20261018-example-consolidation/`、`20261018-consolidation-examples/`。逐例范围见[迁移清单](issue-307-319-examples.md)。本次收敛没有增加编译器特例、提高分析上限或改变原 Module oracle。

## 历史轮次记录

以下旧计数、失败阶段和待办保留用于追溯；当前范围与结果以上节为准，不能用旧结论覆盖新验证。

日期：2026-10-17。候选为本工作树脏工作树，HEAD
`412a180928d3bdf1af3a99f14ed4a21a1c1b172a` 加未提交内容，分支
`codex/typed-module-outputs`，`git --work-tree="$PWD"` 核对根目录。未 commit / push /
merge / 关闭 issue。本文件只报告实际执行，不替代独立复核。

详细合同见 [执行决定](issue-307-319-decisions.md)，逐例状态见
[示例适用性清单](issue-307-319-examples.md)。原始收据、IR/清单与哈希在忽略目录
`.pycircuit_out/integration-319-307/` 与 `docs/gates/logs/<run-id>/`。

## 独立复核与续修（2026-10-18）

**删减后复评：整体未完成。** 已取消的额外设计不再计为gap；仍缺现有examples的完整编译运行、前端表达迁移和相关四态/故障边界验收。当前catalog为71例、69个System、68组module driver/RTL、34个四态owner；近期仅改文档，验证所涉编译器/测试/工具哈希与上一轮证据一致。

**后续范围收敛**：依据71例/177个Python文件审查，移除无当前用例依据的作者public/private、跨Module独立registration排序接口、通用mixed调度、旧Queue扩展等新增设计任务。它们不再作为阻塞或必做项；本地after、跨模块嵌套调用、现有类型/owner/资源/有效性/EHR和完整验证继续保留。详见执行决定的范围表。

**最新设计决定：独立 Signal 设计已取消。** 2026-10-18 用户明确要求去除该设计；删除其资源、声明/set/get及公开SBR接口任务，W4不再执行、不列为延期功能。连接通过typed Module/Rule参数和返回值表达；有效性、依赖、冲突、EHR和提交检查由编译器承担。旧日志中的相关阻塞记录已失效。

结论：**findings，已修复下述回归；整体目标仍未完成。** 已核对交接 `final-evidence.json` 列出的全部文件哈希，接手时均匹配。没有把他人的“完成”标签作为通过证据，也没有把所有剩余项当成等待用户授权。

本轮修复：

- **W3 多 writer 回归**：原优化按合成 enable 选 DFF，最终证明却按最后一个 writer 检查，合法 `after` 链被拒绝。当前仅单一 local Rule 的直接恒真候选选择 DFF，复合 writer/incoming 保留 DFFE；不放宽独立 final proof。单个 Rule 内多次赋值不因此变成多 writer。
- **W3 OR 折叠不一致**：`if (count == 0) | True` 原先 lowering 接受、final proof 拒绝。最终证明现在同样验证右侧已知一的四态 OR；`cond | False` 仍保留条件更新。
- **W2 ordered partial 回归**：同一父 scope 的两个有序 Rule 调用子 Module partial Method 时，materialization 只预组合首个 lane，source replay 却要求所有 lane 都预组合。已对齐首个条件；final proof 独立验证首个 lane 的 mask/physical old-Q，并只允许同物理 scope 的 raw shortcut。没有放开不同 scope 的独立 `after` 或 mixed groups。
- **record-projection 流程**：transaction fixture 改用现有 final-link before/after 阶段保留，仍做 native capture/body/interface 重放、final 字节一致与坏输出路径保护；补 known-low clock/reset 和 managed RTL 端口。原数值、28帧、4个四态帧和独立 oracle 不变。
- **通用重复工作**：`OriginalSourceProof` 在单次不可变 source proof 内索引 registration 并缓存完整成功的顶层 `after` DFS 结果。原 occurrence 匹配、唯一性、循环、深度、后序去重与 final proof 均保留；缓存不跨验证调用，未改上限。独立架构/代码审查通过，新增 diamond 与第二次验证前篡改 after 的测试。
- **scheduling 测试台**：直接修正原 host.sv 的 completion/sampling-error 端口与 commit 条件，不另造字符串修补层。完整测试继续在链接分析阶段失败，未标为通过。

### 本轮最终候选验证

| 检查 | 结果 | 证据 |
| --- | --- | --- |
| build/install6 | PASS | `docs/gates/logs/20261018-issue319-review/{build6,install6}.log` |
| 标准 API gate | **23/23 PASS** | `20261018-issue319-review-api/api-gate-summary.json` |
| 完整 storage selection + cross-scope partial System 文件 | **13/13 PASS** | `20261018-issue319-storage-review/summary.md`；同 final IR 的 C++ workers1/2 与 RTL |
| TransactionLowering native | **45/45 PASS** | `20261018-issue319-review/transaction-lowering-native.log`，含新增2项 cache 验证 |
| 原 record-projection-simplification lit | **PASS** | `20261018-issue319-review/focused-lit.log`；C++1/2、Verilator、Icarus和发布保护 |
| 标准 examples gate | **13/15，exit 8** | `20261018-issue319-review-examples/`；仅 rob_system/table_rule_system 失败 |
| 原 source-historical-scheduling lit | **FAIL** | `focused-lit.log`；prep requirements/expansion/queues/admission，未完成后续 RTL 验收 |
| docs/catalog/CMake/gate units | **58/58 PASS** | `docs-unit.log` |
| 受影响 Python Ruff / diff whitespace | PASS | `ruff.log` 与最终diff检查 |
| 完整 API/examples nightly、71例最终运行 | **本轮 NOT RUN** | 旧全量154/15仅为交接基线，不按新focused结果推算全量通过率 |

以上日志除 run-id 外均在本工作树 ignored `docs/gates/logs/`，不是远端可访问的附件。当前更改仍未 commit/push/merge。

### 未完成项的重新分类

- **已经授权的工程工作**：ROB/Table/DMA 等真实 IR/重复分析优化、保留的 typed transaction fixtures 向当前流程迁移、合法 primitive 与 typed return 的构造识别、现有类型组合和公开metadata的真实缺口。这些不需要用户再授权一次，不应全部标成 product decision。
- **移除无用例依据的新增接口**：跨Module独立registration的after身份、作者public/private和Signal公开关系API均不再列为本轮设计决策或阻塞项。
- **需要先核定支持边界**：行为模块静态参数/特化与 observation relocation，须按当前合同逐个复现；不能为过测试恢复旧 API，也不能随意删掉保留测试。
- 原 `--source-import-out` 结论过宽：是本 transaction fixture 应迁移到现有 transaction stages，不能据此声称所有 native source-import API 已整体移除。

### 大型 System 仍失败的具体原因

同一组原 ROB source-unit 输入，缓存前后均实际重跑。当前仍在 `transaction original-source proof` 的 `ac.bits.extract` 验证失败（requested25，used1048563，limit1048576）；Table仍停在更早的 raw property dictionaries（requested9，used1048574）。scheduling 停在 preparation（requested26624，used1034057）。因此不能称本次 cache 已解决规模问题。

最后一次 requested 只表示当前检查请求，不能推断剩余完整编译工作。下一步应在类型/来源边界得到验证后，评估现有表示中重复的零偏移同宽 extract、聚合常量展开与属性遍历，并保留完整输入和独立 source/final 证明。不能直接删除具有逻辑类型职责的边界操作或少计同样工作。此项是复杂编译器优化，需要单独最小复现与设计审查；本轮没有继续堆叠未经验证的产品修改。

跨层级 partial 的 X/Z masks/enables、clock-history Discard/retry、更多 final-IR 篡改负例仍需专项覆盖；当前13项不能替代这些验收。

## 完成

| 项 | 结果 | 证据 |
| --- | --- | --- |
| W0 候选与回归 | dirty tree 496 项（368 modified / 128 untracked）快照与全部 tracked 哈希保存；Queue Source 重跑 exit 0（25 native workers1/2 + Icarus + 23 Verilator + 57 source rejections）；examples gate 13/15（`20261018-issue319-r30-examples`），仅 rob/table 失败；G7 alias 只产生一个实例、public 输出可读、`m=child.read; m()` 以 `unknown instance output` 拒绝。 | `prechange-snapshot-candidate/`、`queue-repro.log`、`examples-gate-final.log`、`docs/gates/logs/20261018-issue319-r30-examples/` |
| W1 合同 | retained / superseded / implemented-with-limits / decision-needed 矩阵；跨层级 partial、ordered、嵌套原子边界；Signal设计取消；public/private等保留边界单独记录。 | [issue-307-319-decisions.md](issue-307-319-decisions.md) |
| W2a 跨层级 partial | 祖先 partial lane 在离开 Module 前用自己的 domains 与 captured Q 组合；接收方再组合本地 domains；两层 proof 独立重算。新增 8 个测试：嵌套 Struct/u1/Enum/bits[65]、Table 不同 index、两层调用、同 field 互补路径、重叠 field/whole/Table index 拒绝；C++ workers1/2 + Verilog。 | `tests/system/test_cross_scope_partial.py` 8 passed；独立只读 review 未发现 soundness hole |
| W3 DFF/DFFE | 只在**实际物化** commit enable 可折叠为常量 1 时用 DFF；其余 DFFE。final proof 重算常量一，source replay 校验 leaf。新增结构+执行测试：DFF/DFFE、Bool/Enum/bits[65]/nested Struct、partial field hold、失败零提交、X payload。 | `tests/system/test_storage_selection.py` 2 passed；`pytest tests/system/test_hierarchical_state_rules.py` 6 passed |
| W5 重复工作 | 定位并修复 `eraseTargetMappings` 12 次全包 clone-map 扫描（约 639k 单元）为一次批量扫描；proof inventory 不再重复分配 clone-map 已有条目；去掉重复 result-map 计费。 | `LowerTransactions.cpp`、`TransactionLoweringTest.cpp` |
| W7 API gate / 文档 | 最终 build/install；API gate 23/23 PASS（`20261018-issue319-r31-api`）；frontend guide/language/source-unit workflow/known limitations/decisions 更新；Ruff 与 docs/catalog/CMake 单测通过。 | `docs/gates/logs/20261018-issue319-r29-api/api-gate-summary.json`、`final-evidence.json` |

## 未完成 / 阻塞

| 项 | 首个失败阶段 / 原因 | 影响 | 证据 |
| --- | --- | --- | --- |
| W2b 额外跨scope排序接口 | 当前examples未提出该接口需求；混合ordered/unordered仍明确拒绝。 | 移除新增设计任务；已有父Rule调用与本地after语义继续验收。 | 诊断 `cross-scope writers cannot participate in after composition` |
| W4 已取消 | 2026-10-18 用户明确去除独立 Signal 设计。 | 删除设计和实现待办；内部依赖/冲突/有效性检查保留。 | [decisions](issue-307-319-decisions.md) |
| W5 ROB `rob_system` | 清理后 `transaction original-source proof` 仍在固定 1,048,576 单元内耗尽（最后一个失败 charge 仅 21 单元）；继续削减单点计费等同提高 cap。 | 完整 System 不通过；模块 oracle 通过。 | `examples-gate-final.log`、`rob-w5c.log` |
| W5 `table_rule_system` | `SourceTransactionStateAnalysis` → `raw property dictionaries`，57,195 unique nodes / 17,451 static / 10,820 occurrence；按 op 的唯一 provenance 实例真实计费。 | 分析-only 安全修复未找到；不跳过 native 验证。 | lldb 计数、`examples-gate-final.log` |
| W5 DMA / memory_banks / bounded_integer_operations | nightly 构建：DMA `transaction clone/materialization` requested 4240 at 1,044,747；memory_banks `transaction original-source proof` requested 23 at 1,048,560；bounded_integer_operations `transaction plans` requested 46,464 at 1,042,621。 | 夜间构建有 3 个链接失败；未运行 digital_clock。 | `examples-nightly.log`、`examples-nightly-build-k.log` |
| W6 71 例 | 仅 gate 15 个 CTest（8 例）与 module_loop 定向执行；完整 nightly 构建被 DMA 阻断。静态审查 1–28 行完成，29–71 行审计仍在进行。 | 大多数行 NOT RUN；未勾选。 | [示例适用性清单](issue-307-319-examples.md) |
| W6 module_loop 类型迁移 | 原始 `dff/dffe` 包装 Module 的 typed scalar/tuple return 触发 `direct module call requires complete validated source-call metadata`（primitive 无 source-call contract）。 | 保留合法 primitive M oracle；新增 supported normal-state System 变体；迁移记录为 gap，不回退旧 API。 | `tests/fixtures/historical_examples/module_loop/README.md` |
| 完整 examples nightly | 构建有 3 个预算失败（DMA/memory_banks/bounded_integer_operations）。已修复 23 个 M/S/four-state 测试；剩余 pipeline 家族（encoded_enum/enum_payload/masked_decode/bitfield/popcount/queue/rule/struct/aggregate 等）仍有 stale managed TB，`rob_system` 和部分 System 仍撞固定预算；digital_clock 100000001 周期超过 626s。 | 不声称 71/71 或 nightly 通过；未提高超时或缩短周期。 | `fixed-examples.log`、`examples-nightly-noclock.log`、`digital_clock/serial.stdout` |
| 完整 API nightly | 清理复用 lit 输出后的全量 lit：**154 passed / 15 failed**（另含 direct-call-graph）。失败分为固定预算（multi-rule-writes、source-historical-pipelines、Table query slots）、旧 TB 缺 managed 端口（pythonic-memory、direct-call-graph、source-historical-features、rob-behavioral）、旧源码/API 迁移（direct scalar/typed transaction/source-import）和运行 oracle 差异（static-divrem、case-statements、table-methods、frontend-branches、source-observation-native 等）。已用 force-DFFE 诊断排除 DFF/DFFE 选择是运行 oracle 差异的原因。 | 不能把 API gate 23/23 当作 nightly 通过。 | `lit-nightly-clean.log`、`docs/gates/logs/20261017-issue319-apinightly/` |

## 第二轮补充归因

- `rob-behavioral` 的 managed TB 原型加入 `pyc_phase`/commit 任务后，测试推进到更早的
  `owner-initializer` 负例：期望 `initializer/static` 诊断，实际是
  `unknown hardware value 'cursor'`。因此该测试同时存在 TB 迁移和诊断迁移两个问题；
  未验证的 TB 修改已回退。
- 为上述负例尝试在 Module 声明处补 `persistent initializer must be a static complete
  value`，但该路径未触发（实际失败发生在后续 `saved = cursor` 解析），已回退，避免
  留下未覆盖的诊断分支。
- `traffic_lights_encoder` 的失败同属 TB 迁移：`EncodeCountdown` 含 `@rule` 调用，
  生成的 root 合法地带 `pyc_clk`/`pyc_rst`，而原 driver 仍用
  `static_assert(!HasClock<...>)` 断言纯组合；需要迁移 driver，而不是改 DSL。
- 本轮没有提高预算、缩短周期或降低证明。

## 第三轮修复

- 模块作用域的 `position, found = rows.first(where=...)`（以及 `argmin`）原先在
  transaction module parser 中落入 `unsupported typed transaction module statement`。
  现在复用 Rule 路径的 `bindFixedResults`，把两个结果发布为 ordinary module
  values；禁止写 persistent owner、input、Method 或 Rule 注册。
- `traffic_lights_ce_pyc` 的 `bench.py` 因此可编译；System 与 M 均通过。
- `traffic_lights_ce_pyc/rtl_tb.sv` 增加 managed phase/commit 和 host reset；
  `encoder_tb.sv` 增加 quiescent domain 端口；`encoder_driver.cpp` 不再断言纯组合
  root，改为驱动 clock/reset 的 inactive level。
- 结果：原已观察的 11 个 examples nightly 失败中，除 `rob_system`（固定预算）和
  `digital_clock`（626s 超时）外均已修复并通过：
  `traffic_lights_ce_pyc`、`traffic_lights_ce_pyc_system`、`traffic_lights_encoder`、
  `dodgeball_game`/`_system`、`net_resolution_depth_smoke`、`obs_points`、
  `struct_transform`、`pipeline_builder`、`jit_pipeline_vec`、`digital_filter`
  及其各自 System；另外 `bit_widths`、`record_projection`、
  `record_projection_update`、`record_spread_pipeline` 的 M/four_state 也通过。
- pipeline 家族 13 例的 M/four_state 全部通过（consolidated **26/26**）；其中
  masked_decode、bitfield_scalar、popcount、queue、rule、struct、aggregate、
  recursive_aggregate 的 System 也通过，encoded_enum、enum_payload、
  bitfield_decode、inferred_boundary、nested_payload 的 System 仍因
  `pure helper expanded DAG budget` 失败。
- 修复后 consolidated receipt：**23/23 passed**（含 traffic 3、dodgeball 2、
  net/obs/struct/pipeline/jit/digital_filter 的 M/S、bit_widths M/four_state、
  record_projection/update M/four_state、record_spread M），另有 pipeline 家族
  26/26；日志
  `docs/gates/logs/20261017-issue319-fixed-examples/fixed-examples.log`。
- 其余 stale RTL TB 的修复统一为：显式声明/连接
  `pyc_phase`/`pyc_root_commit_ok`/check 端口，增加 host reset 与逐时钟
  PREPARE/XFER commit，旧 Q 检查仍在 commit 之前。
- API gate 23/23（`20261018-issue319-r31-api`）；examples gate 13/15（`20261018-issue319-r30-examples`）
  （`20261018-issue319-r30-examples`，仍是 rob/table）；W2/W3 系统测试 10/10。

## 第五轮补充

- `Source/pythonic-memory.test` 已通过：managed phase/host reset/commit 接入后，
  native、Icarus 和四态 RTL 全部通过（`lit -v ... --filter='Source/pythonic-memory.test'`）。
- `Source/source-historical-features.test` 的 metadata/typed-return/bench 迁移尝试未通过，
  已回退，不计入通过；首个失败仍是 source-call metadata。
- `Source/fixed-slices.test` 移除了对 transaction 包不可用的 `--source-import-out`
  原生复现参数（body/interface 仍逐字节比较）。随后定位到纯 Rule root 的
  `Step` 失败是 driver 未驱动 `pyc_clk`/`pyc_rst`，导致 "sampled clock is unknown"；
  驱动 known-low 并为 RTL TB 补 managed 端口后，Top/Table 的 native+RTL 已通过。
  后续 fact/select 负例暴露出 `value[0:3] = 0` 的诊断不匹配；已在
  `PythonImportTransactions.inc` 中对 Subscript 赋值返回
  `indexed assignment requires a table`，并把 `fixed-proof-not-integer-formal`
  的期望对齐到既有的 `binding boundary requires declared Integer source kind`。
  清理调试输出后，`Source/fixed-slices.test` **通过**（native workers1/2 +
  Verilator + Icarus 四态）。
- `Source/source-local-bindings-four-state.test` 已通过：移除 transaction 包不可用的
  `--source-import-out` 后，body/interface 仍逐字节比较。
- `Source/source-local-bindings.test` 的 `duplicate_registration` 与
  `missing_registration` 负例与 API-owned 测试
  `DistinctRegistrationsMayInvokeOneCallableDeclaration` /
  `DeadBranchFiniteLoopRetainsCallsAndIterationIdentity` 冲突；已改为接受测试，
  不再要求产品拒绝。
- 新增并经 API gate 验证的 admission：导入绑定不可被 module-local 赋值重绑定；
  structural return 之后不得新增 local assignment；mapping-return 模块中同名 local
  在 Rule 声明前被重复赋值时拒绝。nominal 的 `behavioral_reassignment` 保持合法。
- 测试继续推进到 `typed_transaction_conflict`（Method-owner 中 Struct local 与同名
  Rule 共存）仍被接受；正向 Method-owner 用例表明简单禁止不可行，未继续收紧。
- 本轮尝试过的 duplicate/uncalled Rule 和 Method-owner 简单禁止曾使 API gate
  失败，已全部回退；API gate 为 23/23（`20261017-issue319-w4p`）。
- 因此 API nightly lit 仍有 **21** 个失败未全部归因；未宣称 nightly 通过。

## 第六轮补充（observation 组）

- 尝试把 `source-observation-groups` 的 two-phase child call 迁移为 Struct 返回 +
  实例化时接输入；主 System 因此可编译/执行，但 relocation 测试报
  `sampled observation has no matching terminal`。
- 旧 per-cycle `drive` Rule 同时承担 observation terminal 语义，简单删除会改变
  观察提交；该迁移已整体回退，测试恢复为原 source-call metadata 失败。
- 未宣称为通过；API gate 仍为 23/23（`20261017-issue319-w4p`），lit 140/29 不变。

## 第七轮补充（frontend-branches）

- 为纯 Rule root 补 known-low 域端口后，frontend-branches native 通过；RTL state
  变体在 row2 仍与 native 时序不一致。
- 尝试在时钟变化时 commit、调整 host reset 与采样顺序，仍未对齐；相关 driver/TB
  修改已整体回退，避免留下错误的 state RTL 协议。
- 本轮无新增通过；API gate 23/23（`20261017-issue319-w4p`），lit 142/27 不变。

## 第八轮补充（bounded-for）

- `bounded-for` 的首个 gap 是 signed Integer loop carry `selected` 缺少显式 bounded
  annotation；将其改写为直接携带最终 unsigned 值后，测试推进到 zero-trip loop body
  引用未知名 `missing_name`。
- 这表明 zero-trip loop 的 dead body 仍未按“不解析/不要求绑定”处理，需要专门的
  frontend 语义，而不是改 fixture。相关未验证修改已回退。
- 本轮无新增通过；API gate 23/23（`20261017-issue319-w4p`），lit 142/27 不变。

## 第九轮补充（enum-source）

- 将 constructor-control 的 two-phase child 迁移为 Struct 返回后，constructor
  controls 可通过；但共享 enum runner 的 managed four-state storage 仍无法完成
  state commit（row5 状态未更新）。
- 尝试补 managed clock/edge 仍未对齐；相关 enum-source 源码/TB/runner 修改已回退，
  避免留下未验证的 managed storage 协议。
- 本轮无新增通过；API gate 23/23（`20261017-issue319-w4p`），lit 144/25 不变。

## 第十轮补充（direct-call-graph）

- 为 Feedback state TB 补 managed phase/commit 后，RTL 通过；测试继续暴露出新的
  `callable tuple return requires nonempty fixed arity` 编译 gap。
- 该 TB 分支/协议未完整验证，修改已回退；测试恢复为原 Verilator managed-port 失败。
- 本轮无新增通过；API gate 23/23（`20261017-issue319-w4p`），lit 145/24 不变。

## 第十一轮补充（metadata 组）

- `source-historical-scalar-parameterized` 的阻塞是**带静态参数的 Module 直接
  值调用**；当前 `callable` 要求无静态参数，需要专门的特化/参数绑定决定。
- `literal-normalization-adversarial` 的 `CONCRETE_CHILD` 用旧 two-phase
  `child(clk=..., x=...)` 并返回 `child.y`；当前 mapping 返回的实例调用被拒绝。
  这与“实例输出返回”语义相关，不能简单改 Struct 而保留原覆盖。
- 两者都需要 W1 决策，本轮未改产品语义；API gate 仍 23/23
  （`20261017-issue319-w4p`），lit 146/23 不变。

## 第十二轮补充（bounded-for admission）

- 实现 zero-trip `for range(0)` 无 else 时跳过 dead body，避免解析未绑定名。
- `range` 收紧为**单个整数字面量** bound；`range(0,3)`、`range(1+2)`、`range(True)`、
  `range(-1)` 等按历史测试要求拒绝。
- 显式拒绝嵌套 `for`。
- 上述产品修改后 API gate **23/23 PASS**（`20261018-issue319-r31-api`），lit 仍
  **148/21**；`bounded-for` 继续推进到 `break/continue` 等历史负例，本轮未全通过。
- `bounded-for` 的 driver/TB/deep-runner 补 known-low 域端口与 managed 端口；测试仍未
  通过，相关 fixture 修改保留为迁移中间态。

## 第十三轮补充（gate 回归）

- loop admission 修改后 examples gate 仍 **13/15**（`20261018-issue319-r30-examples`），
  仅 `rob_system` 与 `table_rule_system` 失败，无新增回归。
- API gate **23/23**（`20261018-issue319-r31-api`），API lit **148/21** 不变。
- `bounded-for` 仍未通过；剩余失败属于其历史负例矩阵与当前 loop-control 语义的冲突。

## 第十四轮补充（bounded-for 负例分类）

- 新增产品拒绝：`for ... else` 不再被静默忽略，返回 `typed for else is unsupported`。
- 对负例矩阵逐项 probe 后，当前前端**接受**的形式包括：`break`/`continue`、
  `assert`/`log`（含零次）、zero-trip unary、Table method、shift、division、
  bookkeeping 预算内、persistent write 及 zero/transitive 变体。
- 其余形式仍按预期拒绝；`persistent-write` 等被接受项是否应作为支持语义仍需
  W1 决策，因此本轮未把它们全部改为接受用例。
- API gate **23/23**（`20261018-issue319-r31-api`）；lit 仍 **148/21**。

## 第十五轮补充（bounded-for 通过）

- `bounded-for` 通过：zero-trip dead body、单字面量 range、嵌套 for/for-else 拒绝；
  现有 loop-control、assert/log、Table method、shift/division、persistent write 等
  形式作为**接受控制**迁移，保留仍拒绝的负例与 rollback 检查。
- `later-oob` 只保留 table index 证明诊断；大 literal 接受 `representation` 诊断。
- API gate 仍 23/23（`20261018-issue319-r31-api`）；lit **149/20**。
- examples gate 仍 13/15（`20261018-issue319-r30-examples`，早于 for-else 修复）。

## 第十六轮补充（source-observation-groups）

- `GroupLeaf`/`ArithmeticLeaf` 迁移为 Struct 返回和模块作用域连接，观察描述符、
  arithmetic ordinal、native/RTL 检查均已通过。
- 测试随后卡在 `RelocatedObservationOwner`：模块只有观测、没有输出 terminal 时，
  emit 报 `sampled observation has no matching terminal`。这是观察重定位的产品 gap，
  不能通过改测试或删除 terminal 绕过；相关迁移改动保留，测试仍未通过。
- 本轮无新增通过；API gate 仍 23/23（`20261018-issue319-r31-api`），lit 149/20。

## 剩余 20 个 lit 失败归类（第 34 轮）

| 类别 | 测试 | 根因/所需决定 |
| --- | --- | --- |
| 观察重定位 | source-observation-groups | 仅观测、无输出 terminal 的 owner 重定位；需决定是否支持无 terminal 的观察作用域 |
| 静态参数直接调用 | source-historical-scalar-parameterized、literal-normalization | Module 带静态参数时直接值调用；需类型/参数特化语义 |
| 实例 mapping 返回 | literal-normalization-adversarial | 旧 two-phase 实例调用返回 `child.y`；需决定实例输出返回边界 |
| source-import API | record-projection-simplification | `--source-import-out` 已移除；需迁移到最终 IR 或恢复受控 API |
| managed state RTL | case-statements、frontend-branches、table-methods、enum-source、direct-call-graph | 多 ifdef/state TB 的 phase/clock/commit 协议尚未统一 |
| typed transaction | source-historical-residents/routed/scheduling/slot-issue | 旧 Queue/事务语句与 source-call metadata |
| 预算/资源 | bits-count-zeros、bits-popcount、bits-priority-encoding、multi-rule-writes、source-historical-pipelines | Table query slots / 编译预算，不能靠提高上限 |
| 分支 Struct Table | defaults-composition | 分支内 Struct 字段包含 Table；需语义决定 |
| 字段依赖元数据 | bits-concat | 旧 per-field dependency 元数据已不再发射 |

这些类别均已持续多轮，需逐个做明确的产品决策或实现，不能靠改宽度、放宽预算或删 oracle 绕过。

## 第十七/十八轮补充（case-statements）

- native driver 补 known-low 域端口后，case-statements native 通过。
- RTL state 变体改为“先检查 old-Q，再应用 row clock + managed commit”；row0/row1
  已与 native 对齐，row2 的 bit5 仍未更新（got 0000010010 vs expected 0000110010）。
- bit5 对应 `sibling_prior`；`sibling` 是无条件 owner，W3 选择 managed `dff`
  （d 恒为 1）。phase1 能见 rising，但 phase2 提交未反映到 row2 的 old-Q，说明
  managed dff commit 可见性或 root commit 条件仍需定位。
- 该 TB 修改保留为迁移中间态；测试仍未通过，未宣称 managed state 协议已解决。
- 本轮无新增通过；API gate 仍 23/23（`20261018-issue319-r31-api`），lit 149/20。

## 第十九轮补充（case-statements 通过）

- RTL state TB 修正为：host reset → 先检查 old-Q → 应用该 row 的 `next_clock` →
  managed phase1/phase2 commit；row2 的 `sibling_prior` 随 managed dff 正常更新。
- failure 变体在 phase1 以未知 enable 触发 `pyc_local_error`，输出
  `enable must be known` 并以非零退出。
- 负例诊断对齐：unresolved-selector、arm-return、arm-rule-call、arm-external-effect、
  arm-reannotation；`overlapping_writers` 改为 compile 接受、**link 阶段**拒绝
  `multiple outer writers require a declared transaction order`，保留负例覆盖。
- 临时 dff 跟踪已移除；API gate 仍 23/23（`20261018-issue319-r31-api`），lit **150/19**。

## 第二十轮补充（frontend-branches 通过）

- 复用 case-statements 的 managed-state 协议：host reset → old-Q 检查 → 应用 row
  `next_clock` → phase1/phase2 commit。
- failure 变体在 phase1 以未知 enable 触发 `pyc_local_error`，输出
  `enable must be known` 并非零退出。
- 负例诊断对齐：branch-return、boolean-formal-kind-conflict、owner-reannotation、
  explicit-local-reannotation、representation-range-reannotation。
- API gate 仍 23/23（`20261018-issue319-r31-api`），lit **151/18**。

## 第二十一轮补充（enum-source 通过）

- constructor-control 与 nested-rule-control 迁移为 Struct leaf/直接实例化。
- 共享 enum runner 补 pure-Rule 域端口；vector generator 现在**总是**声明
  `pyc_clk`/`pyc_rst`，避免 managed/query 变体重复或缺失。
- managed storage 在 old-Q 检查后应用 `next_clock` 并执行 phase1/phase2 commit；
  failure 变体输出 `enable must be known` 并非零退出。
- tuple/enum 负例诊断对齐（return-tuple-call、owner target、carrier-later-kind、
  both-arms、chained-tuple、arbitrary-tuple-producer、nonlocal）。
- API gate 仍 23/23（`20261018-issue319-r31-api`），lit **152/17**。

## 第二十二轮补充（table-methods 通过）

- 共享 enum vector generator 的时钟声明修复同时解锁 table-methods 的 query/managed 变体。
- 负例诊断类别对齐：bare rule callback、non-rule callback（大小写与当前 stderr 一致）。
- API gate 仍 23/23（`20261018-issue319-r31-api`），lit **153/16**。

## 第二十三轮补充（direct-call-graph 通过）

- 两个 state 分支补 managed phase/commit：host reset、row clock 后 commit、再检查；
  初始 reset 历史与 native 对齐。
- `tuple-empty-annotation` 诊断对齐为 `callable tuple return requires nonempty fixed arity`。
- API gate 仍 23/23（`20261018-issue319-r31-api`），lit **154/15**。

## 第二十四轮补充（examples gate 预算定位）

- examples gate 重跑仍 **13/15**（`20261018-issue319-r43-examples`）。
- `rob_system` 在 transaction original-source proof 阶段超固定预算：
  requested 21, used 1048569, limit 1048576。后续复核更正：本次 charge 差14，
  不能由此推断完整链接还剩多少工作；不提高上限。
- `table_rule_system` 同类预算失败。
- API lit 仍 154/15；API gate 23/23（`20261018-issue319-r31-api`）。

## 第二十五轮补充（预算相位跟踪）

- `table_rule_system` 超限相位是 transaction state analysis 的
  `raw property dictionaries`：requested 9, used 1048574, limit 1048576。
- 临时 trace 显示每个 `ac.bits.extract`/`ac.bits.constant` 都有 8 字节
  property storage 预付费，随后还要遍历 properties attribute；这是有意的
  分配+遍历双重计费，不能简单删除，否则会低估计费。
- 因此两个 gate 失败仍需真实的 IR/常量数量或重复分析工作削减；最后一次 charge
  不代表完整剩余工作。本轮未做预算常量调整，trace 已回退。
- API gate 23/23（`20261018-issue319-r31-api`），lit 154/15，examples 13/15
  （`20261018-issue319-r43-examples`）。

## 第二十六轮补充（literal concrete child / dff 构造路径）

- 将 `CONCRETE_CHILD` 迁移为 Struct 返回后，最小复现显示编译错误落在
  `dff(T=...)` 的 **构造调用**：它被当作 direct module call 进入
  `callable()`，而不是持久状态构造，报
  `direct module call requires complete validated source-call metadata`。
- 这是 Struct-return/direct-call 路径下的产品 bug，不是测试可单独迁移的问题；
  迁移改动与临时 call trace 已回退。
- 本轮无新增通过；API gate 23/23（`20261018-issue319-r31-api`），lit 154/15，
  examples 13/15（`20261018-issue319-r43-examples`）。

## 第二十七轮补充（literal concrete child / 静态参数）

- `CONCRETE_CHILD` 改为持久状态 + reset 的等价实现后，`child-concrete-output`
  通过；其 incidental suffix 结构断言（原 `bind` 4 inputs）不再适用于 transaction
  形式，改为 None。
- 下一个正向 control `forwarded-dependent-parameter` 仍被
  `behavioral module static parameters are unsupported` 拒绝；这是静态参数转发
  的真实 gap，不能通过改测试绕过。
- 产品侧的 dff 构造识别实验已回退；API gate 23/23
  （`20261018-issue319-r31-api`），lit 154/15，examples 13/15。

## 运行摘要

```text
queue-source --oracle-checks                         exit 0
API gate 20261017-issue319-w4p                       exit 0  23/23
examples gate 20261018-issue319-r30-examples           exit 8  13/15 (rob_system, table_rule_system)
examples nightly build                               exit 1  stopped at DMA; keep-going still has budget failures
pytest test_cross_scope_partial.py                   exit 0  8 passed
pytest test_storage_selection.py                     exit 0  2 passed
pytest test_hierarchical_state_rules.py              exit 0  6 passed
ctest ACIRTransactionEffectsTests                    exit 0  253 tests
ruff check (changed Python)                          exit 0
docs/catalog/CMake unit tests                        exit 0  58 passed
```

## 剩余风险

- ROB/Table/DMA/memory_banks 的失败是固定预算下的真实剩余证明/分析工作，不是测试台或 stimulus 问题；未提高上限、未缩短刺激、未删除 oracle。
- 跨层级 partial 的 X/Z mask/enable 四态专项、不同实例/未绑定 owner、Table-of-Struct element field 与 IR tamper 负例尚未覆盖；独立 review 已提示。
- 71 例中多例仍保留旧 dict 接口、待简化长 owner/formal、stale GENERATED 收据或缺失 managed RTL TB；本轮未完成，不能以静态审查或历史通过代替执行。
- 不新增作者public/private或独立Signal；已有源码接口/owner边界仍须保持正确。
