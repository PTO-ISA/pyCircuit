# #307 / #319 端到端审查与执行清单

**当前验收（2026-10-18）：** W6 当前 17 例迁移完成，examples nightly **34/34 PASS**；54 个旧目录完整归档，不计通过。API gate **23/23 PASS**，unit **589 passed / 5 skipped / 82 deselected**。14 份 Module 收据和归档已独立核对。完整 API lit 汇总为 161/169，仍有 8 项失败，#307/#319 整体未完成。W0–W5 的历史记录不代表所有组合已闭合。

审查日期：2026-10-17。范围为 [#319](https://github.com/PTO-ISA/pyCircuit/issues/319) 和 [#307](https://github.com/PTO-ISA/pyCircuit/issues/307)，不含误写的 #309。本文件是当前交接清单，不是新的语义批准，也不表示两个 issue 已完成。

配套文件：[逐例清单](issue-307-319-examples.md)。作者规范以 [frontend guide](agent-frontend-guide.md)、[language](../reference/language.md)、[examples standard](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/STANDARD.md) 为入口；历史条目与用户最新要求冲突时，按设计归并表处理。

## 历史审查结论和候选

以下接手结论记录收敛前状态；现行 examples 结果以文首和 W6 为准。

**删减设计后的完成度复评（2026-10-18）：仍未完成。** Signal、作者关系表/public-private、额外跨scope排序/调度等已从验收范围移除。剩余阻塞是现有设计的真实编译失败、作者表达迁移和完整验证，不是等待上述新接口。

当前验收基线：API gate23/23、相关System回归13/13、TransactionLowering45/45通过；examples gate13/15，ROB/Table完整System仍失败；scheduling原完整测试仍在链接准备阶段失败。71例中69个登记System、68组原module driver/RTL、34个四态owner；不要求新增自动多时钟System，但各原owner必须完成验证。完整nightly和71行最终验收尚未通过。

本次核对复核证据中编译器、测试与安装工具哈希均未变化，复用对应结果；近期修改仅为设计收敛和文档，不以删任务代替修复失败。

**独立复核更新（2026-10-18）：仍未全部满足。** 接手发现并修复 W2/W3 的3项链接回归，追加 proof 内重复 DFS 消除；API gate23/23、相关System13/13、transaction lowering native45/45通过。examples gate仍13/15。不能将原执行者的W2a/W3完成标签当作全部组合已验收，也不能将工程缺陷统一标成等待产品决策。详细发现、修复与剩余覆盖见 [执行状态](issue-307-319-execution-status.md)。

**尚未全部满足。** 普通 Module 层次、有限类型、嵌套/跨 Module Rule、proposal/commit、隐式资源条件和输出有效性已有实质实现与分项验收；统一模型还存在跨层级写组合、存储选择和完整 examples 验收缺口。不能因 API gate 通过就宣称整项完成，也不应重造已工作的 compiler/runtime flow。

接手位置：`/Users/zhoubot/.codex/worktrees/typed-module-outputs/pyCircuit`，分支 `codex/typed-module-outputs`，HEAD `412a180928d3bdf1af3a99f14ed4a21a1c1b172a` **加现有未提交内容**。HEAD 本身不包含本轮实现。保留所有 tracked/untracked 修改；不要从 HEAD 新建空白候选而丢失它们。该环境的共享 Git 配置含 `core.worktree`，在此目录使用 `git --work-tree="$PWD" ...` 或 `GIT_WORK_TREE="$PWD"`，先核实实际根目录。

当前 compiler build/install 位于本工作树 `.pycircuit_out/toolchain/{build,install}`；本轮最后产品构建称为 build/install51。这只是本地证据编号，不是公共 API 版本。CompilerDev 用 LLVM/MLIR 22.1.8。执行者重新记录文件哈希及工具哈希，不能仅引用 HEAD 来证明脏工作树的结果。

### 本轮证据边界

| 项目 | 实际结果 | 范围限制 |
| --- | --- | --- |
| build/install51 + SourceInterface | PASS | 本树构建/安装；单组 CTest 通过 |
| 标准 API gate51 | **23/23 CTest 组 PASS** | 包括该 tier 的原生和 lit；不是完整 nightly |
| 完整 Table query family51 | **PASS，exit 0** | 原 `enum-source.py --semantics --case-family table-query`，包含 current candidate、saved SSA、lambda shadowing；不是所有类型矩阵 |
| Queue Source51 | FAIL，正在做测试台对齐 | ReorderPipeline RTL `.*` 缺生成有效性端口；最终结果见本文件验收附记 |
| Queue CodeGen50 + 两个 Queue IR 文件 | PASS | 早一候选的完整原矩阵；不能替代最终全量接受 |
| System48 | 25/25 PASS | 既有 C++ workers 1/2、Verilator，以及专项真实 Icarus；后续产品改动需重新验证 |
| examples gate50 | 12/14 PASS | `rob_system`、`table_rule_system` 失败；不是71例验收 |
| docs/catalog/CMake/gate unit50 | 58/58 PASS | 文档/注册一致性，非硬件执行 |
| 当前全部71例 + 完整 API/examples nightly | **NOT RUN** | 必须由执行者完成；不得继承历史通过状态 |

本地原始证据目录：`docs/gates/logs/20261017-issue307-319-review/`、`20261017-issue307-319-api51/`、`20261017-queue-native/`。它们被 Git 忽略，不是远端可访问的交付物。交付时附实际收据或归档路径。最终清单只在满足证据后勾选。

## 设计归并：保留、取代和待定

| 设计点 | 当前执行合同 | 对 #307 的处理 |
| --- | --- | --- |
| Module 层次 | 函数 `@module` 静态直接调用；普通多输入多输出 instance | class/init/call 与 allocate-then-bind 不恢复 |
| 接口 | 独立消费者用异构 tuple；同一接口共同字段用具名 Struct | 不强迫所有输出塞进一个大 Struct，也不拆散真实 payload |
| 状态 | Module 持久声明产生普通 DFF/DFFE 子实例；Table 为现有元素存储集合；Rule locals 是 SSA | `ac.cell` 不是要恢复的前端 |
| Rule | 简单内聚、串行 Pythonic；外层一次调用全成全败；嵌套调用共享边界 | 不能按函数长度拆碎一个原子动作 |
| 读值 | 独立 Rule 默认旧 committed Q；事务内 candidate 顺序可见，已保存 local 保持 SSA 快照 | #307 “同事务写后永远读旧值”被取代；不擅自引入 CReg port API |
| 输出 | 编译器产生/传播可用性；失败不补零、不加保持寄存器，不允许 invalid 触发 else | 旧“独立事务之间一律不能传 var”不能直接套用 |
| Queue | 普通 native 子 Module、显式 `push/pop/peek`、已有 FIFO kernel/RTL；新数据不擅自旁路 | 隐式赋值 push/pop 不恢复；capacity/initial/full/size/重复 pop 合并等旧提案不得直接移植 |
| SCF | 在现有支持的有限 if/for/while/execute_region 和既有类型上闭环 | 不扩大到所有 SCF op、任意循环、新浮点类型或自动每迭代一拍 |
| Signal | **设计已取消**；删除独立资源及声明/set/get接口要求 | 按2026-10-18用户指令，不列为延期功能或后续待办；编译器 availability 保留 |
| CF/C/SB/SBR | 不因历史 Signal 提案增加作者关系声明；保留内部依赖/冲突/顺序分析 | 旧 Signal 同事务 set/get 禁止规则不施加到普通值或状态候选 |
| public/private | 无现有examples需求，移除新增作者修饰符任务 | 既有接口导出/owner/私有内部端口检查仍保留 |
| 自动调度 | 复用现有依赖分析、work partition、System commit | 不增加可选自动仲裁、动态 activation worklist 或第二套调度器 |

`approved v1/v2` 等历史技术记录应按文件中的具体语义和适用范围读取，不是“版本更大就整体覆盖”的公共接口迁移指令。不要改写冻结 approvals；未明确的新 API 先形成最小差异说明，暂停相关任务，继续其他已明确工作。

## 端到端发现

| 优先级 / 编号 | 触发、影响和证据 | 处理 |
| --- | --- | --- |
| P1 G1 | 同一实际 state owner 的跨层级 field/index 写或 after/mixed writer groups 明确拒绝：`LowerTransactions.cpp:1673`、`HardwareSampledAdmission.cpp:3981`、`HardwareWriteDomainProof.inc:528` | 已有 whole-owner 互斥组合不能代表统一完成；W2 |
| P1 G2 | ROB/Table 完整 System 在事务物化/原始属性检查上限处失败，原物理模块通过不能替代 System | 记录首次失败阶段和重复工作根因；W5；不提高上限、不缩短刺激，不做成本/计费模型 |
| 取消 G3 | 独立 Signal 设计已撤销 | 无实现或验收任务，不再作为缺口或阻塞项 |
| P2 G4 | Module 状态仍保守生成 DFFE：`PythonImportBehavior.inc:2705`、`LowerTransactions.cpp:3139` | 缺基于实际 enable 的 DFF/DFFE 选择；W3 |
| P2 G5 | 71例尚未逐项完成最新候选执行；3例无 System，部分独立接口还在总 Result 中，DMA 有内部握手连接 | 按逐例清单重整、保留原 driver/RTL/四态；W6 |
| P2 G6 | 活跃文档仍有 blanket “one outer writer per actual owner”：`language.md:1188,1271`；与已支持互斥/有序组合混淆 | W7 用能力矩阵替换过时绝对表述；不能只删除限制而不说明边界 |
| 待核验 G7 | `PythonImportRecords.cpp:4480–4520` 的 alias 路径仅有源码疑点，尚无当前语言最小复现 | W0 先用函数直接调用/typed outputs 验证；不能恢复旧两阶段 API 来“修复” |

以上代码路径中无目录前缀者分别位于 `compiler/lib/Compiler/`、`compiler/lib/Transforms/`、`compiler/lib/Dialect/ACIR/`。行号是此候选定位，后续以符号/诊断为准。

已有且必须保留的通路：`SourceLink.cpp` 的 body/interface/closure，`SourceMethodAnalysis.cpp` 的 effects，`SourceTransactionStateAnalysis.cpp` 的状态与本地 after，`HardwareAnalysisDependencies.cpp` 的依赖环拒绝，`HardwareWorkPartition.cpp` 的执行分区，`LowerTransactions.cpp` 的物化及原始源码回放，独立 final proof，`SimSystem.h:151–178` / `FinalEmitVerilog.cpp:455–488` 的全 System 检查后统一提交。继续修这些 owner，不建立平行引擎。

## 可执行 checklist

每个 packet 完成须有：改动路径、行为前后差异、独立测试、命令/退出码/输入与产物哈希、剩余限制。执行和独立 review 分离。根集成者独占 build/install、共享注册/CMake/catalog 和最终证据。下列 owner 是文件责任，不是已派发的 agent。

### W0 — 固定基线和关闭当前回归

依赖：无。Owner：集成者 + 独立测试；只修简单、已定位错误。

- [x] 保存 dirty tree 的 tracked/untracked 清单和哈希、工具版本、issue 快照与环境；确认使用当前 worktree。证据：`.pycircuit_out/integration-319-307/prechange-snapshot-candidate/`。
- [x] 接收 Queue Source 完整原 oracle 重跑结果，核对生成有效性及 managed check/commit 端口；不删除原始检查。当前候选 `queue-source.py --oracle-checks` exit 0（25 native + Icarus + 23 Verilator + 57 source rejections）。
- [x] 接收 Table query51 和 API gate51 收据；发生后续产品改变则运行受影响回归。最终 API gate 23/23 PASS（`20261017-issue319-w4p`）。
- [x] 用当前合法 Module/Rule 语法验证 immutable instance alias、public output binding 与 indirect Method 拒绝，记录 G7 是否真实存在。alias 只产生一个 Child 实例；public 结果可读；`m = child.read; m()` 以 `unknown instance output` 拒绝。G7 的 alias 路径未复现。
- [x] 重跑当前 examples gate，区分 ROB/Table 已知失败和新回归。最终 13/15 PASS；`rob_system`、`table_rule_system` FAIL 均为固定 1M 分析/证明预算，非新回归（`20261017-issue319-egate3`）。

验收：当前候选身份可重现，已知失败有原始 reproducer；不得要求先掩盖所有复杂失败才开始其他独立任务。

### W1 — 冻结本轮需要实现的合同差异

依赖：W0 的快照，可与例子静态整理并行。Owner：设计/文档；不改 product。

- [x] 将 #307/#319 每项标记 retained、superseded、implemented-with-limits 或 decision-needed；使用设计归并表作为起点。见 [执行决定](issue-307-319-decisions.md)。
- [x] 明确跨层级 partial、有序独立 Rule、嵌套原子调用之间的边界；EHR 可见性不替代冲突仲裁。见 decisions 文档。
- [x] 撤销独立 Signal 及其公开关系接口要求；同时移除无用例依据的public/private、旧Queue扩展等新增任务；保留既有行为和测试。
- [x] 为真正新语义按仓库要求做独立架构与任务分解审查；缺少决定时只暂停受影响 packet，并向用户说明分歧及建议。W2a 经独立只读 review；W4 的额外接口需求已撤回。

验收：执行者无需猜测新公共 API；不以“复杂”为由停止已批准的完整实现。

### W2 — 统一跨层级状态与 EHR 组合

依赖：W0；新增组合语义依赖 W1。Owner：`SourceTransactionStateAnalysis*`、`LowerTransactions.cpp` / `Transaction*`、`HardwareSampledAdmission*` / `HardwareWriteDomain*`。测试由另一实例负责。

- [~] W2a：实现已落地、完整验收未闭合。复用现有 domain composer 支持实际 owner 的跨 scope field/index 部分写，保留未写字段、alias/同实例识别、selected-path contributor。实现：祖先 partial lane 在路由前用自己的 selectors/masks 与 captured Q 组合；接收方再组合本地 domains。新增ordered partial回归已修复；该文件9/9通过，跨层级四态专项仍见下项。
- W2b **移除无用例依据的新增设计任务**：不新增跨Module独立registration排序接口或通用mixed调度。保留现有本Module after、父Rule内嵌套原子调用、同一父scope的有序Rule调用子partial Method，以及对应EHR与冲突证明。它们仍需按已确认语义验收。
- [x] 原始 source replay 与最终 SSA proof 分别验证 owner、完整贡献者集合、途经端口、有效前缀和 E/D/Q；不能只删除拒绝条件。新增 `proveSingleCandidate` 要求 peel 回物理 old Q；source replay 独立重算 lane。
- [ ] 覆盖 Bits（含>64位）、bool、有界 Integer、Enum、嵌套 Struct、固定 Table 的合法组合……**部分**：Bits[65]、u1、Enum、嵌套 Struct、Table index 已覆盖；bool 在有界 Struct 用 u1；有界 Integer 跨层级 partial 未单独覆盖。
- [ ] 正例覆盖两层调用、同/不同实例、局部 partial + 外部 partial、当前候选影响后序 guard；负例覆盖重叠无序写、伪造/遗漏贡献者、循环、未绑定 owner，以及不同物理 clock/reset domain 的非法组合……**部分**：两层调用、重叠 field/Table index 拒绝、互补 enable 接受已覆盖；不同实例/未绑定 owner/X/Z 控制未覆盖。
- [~] 生成 System：C++ workers1/2、Verilog 已覆盖；新增四态行为需真实 Icarus 的 cross-scope partial 专项尚未补齐。reset/hold 与失败零提交由 W3/既有 system owners 覆盖。

验收：新接受情况运行正确，旧拒绝边界仍有效，完整候选有独立审查。

### W3 — 从实际更新条件选择 DFF/DFFE

依赖：W0，可与 W2 设计并行；共享 lowering 修改必须串行。Owner：已有状态规划/primitive 选择及其 source/final proof。

- [x] 从已经验证的本地更新条件证明无条件写；满足条件时才去除 DFFE enable，其余保留 DFFE。当前保守地只对单一local Rule直接候选的恒真enable选择DFF；复合候选保留DFFE。更激进的DFF优化不是额外迁移前提。
- [x] 初始化/reset/domain、X/Z、hold、原子性、不同使能的 aggregate leaves 和 pipeline boundaries 保持不变。`test_storage_selection.py` 覆盖 DFF/DFFE 选择、零提交、X payload、Struct partial hold。
- [~] 已有无条件/guard更新、失败零提交、不同字段使能和native X-payload测试；不能将这些结果等同于新增路径完整四态控制与Discard/retry矩阵验收。
- [x] 同步现有 storage verifier、body/interface 和双后端；不要为 DFF 引入前端开关或新寄存器 API。

验收：结构确实分别产生 DFF/DFFE，外部轨迹保持既有语义；若现有 DFF primitive 无法表达 commit 保护，先解释合同冲突而非强制替换。

### W4 — 已取消

2026-10-18 用户明确要求去除独立 Signal 设计。该任务取消，不延期、不等待接口决策、不作为完成条件。保留编号以便核对历史记录。

连接使用普通 Module/Rule typed 参数和返回值；有效性、依赖、冲突、因果环及 EHR/提交逻辑继续由现有编译器负责。

### W5 — 修复完整编译路径的根因

依赖：W0。Owner：实际首次失败的 existing pass；先诊断，再限定文件。

- [x] 对 ROB System 的事务物化限制、Table System 的原始 property dictionary 限制保存完整输入、首个失败阶段、调用栈/计数与最小复现。并发现 DMA/memory_banks 同类预算失败。
- [ ] 历史 61-module multi-rule 完整门禁未重跑（nightly 构建先被 DMA 阻断）。
- [x] 定位并修复 `eraseTargetMappings` 的 12 次全包扫描（约 639k 单元）为一次批量扫描；proof inventory 不再重复分配 clone-map 已有条目。
- [x] 相同完整 workload bytes 比较；未提高 cap、未降低证明。清理后 ROB 仍在完整证明过程中触及固定限制，DMA/memory_banks 也失败；最后一次请求不代表完成编译的剩余工作量。
- [x] 后续复杂失败保留 reproducer；ROB/Table/DMA/memory_banks 完整 System 在固定限制内仍不通过；继续定位真实重复工作/IR展开，不能削减必要证明，也不等待无关新接口决策。

验收：原 ROB/Table 完整 System 和原大规模门禁通过；原数值、四态、失败/发布保护 oracle 不变。

### W6 — 当前示例的适用性、精简与验证

2026-10-18 用户要求先检查过时定位、重复设计和验证负担。历史71目录不再是无条件保留/补齐目标；以最新[适用性清单](issue-307-319-examples.md)和 examples/GUIDE.md 为准。

- [x] 完成71项用途审查：推荐主线7项、设计/专项10项；21项合并教学入口、31项API候选、2项退出JIT定位。
- [x] 更新导航和写法标准：默认短System；手写host/RTL仅用于独有物理、四态或生命周期边界，不强制每例三重脚手架。
- [x] 现有必要前端改写已执行：nested Rule、typed独立输出、跨Module Queue调用、合法显式DFF/DFFE typed返回；全部177个Python文件Ruff检查/格式通过，格式前后AST一致。
- [x] 三个物理例的typed接口与原独立时钟测试通过；两个多时钟例不要求新增自动多时钟System。
- [x] 当前 17 例 nightly 34/34 PASS；保留 digital_clock 原完整周期和独立 Module oracle。DMA/Memory/Table 新短 System 与原长 bench 的证据分别记录。
- [x] 54 个旧目录完整搬至历史 fixtures；现有显式消费者重定向，25 条 API coverage 映射不变。其余归档不冒充已执行 API 回归。
- [x] 刷新全部 14 份 Module 收据；独立核对 catalog/CMake/cycles、原 oracle、归档哈希和最终运行证据，无阻塞发现。

验收以用途清晰、推荐写法简单和实际行为验证为准，不再按71/71目录计数驱动新增机制。

### W7 — 文档、完整接受和回交

依赖：对应实现完成。Owner：集成者、独立 reviewer/tester。

- [x] 更新 frontend guide、language、source-unit workflow、known limitations、decisions；去掉过时单 writer 绝对描述。
- [x] 当前 examples nightly 34/34 和 API gate 23/23 通过；unit 589 passed / 5 skipped / 82 deselected。
- [x] example build、Runtime-only install、CompilerDev consumer 定向检查通过；完整 wheel NOT RUN。
- [x] 测试与 gate 产物保留在 `.pycircuit_out/integration-319-307/` 和 `docs/gates/logs/20261018-example-consolidation/`；同一 final IR 双目标。
- [ ] 完整 API lit 汇总为 161/169，仍有 8 个失败；后续 package nightly NOT RUN。不能将 gate 或现行 examples 通过作为整项接受。
- [x] 当前示例实现独立审查、收据/归档独立核对完成；用户已授权本轮本地提交；不 push/merge。

## 失败处理与回审标准

简单且定位明确的诊断、测试接口或实现错误，补针对性回归后直接修。复杂错误先说明“失败命令、首次分歧阶段、最小复现、根因证据、受影响范围和建议”；不得继续堆叠特殊路径。未确认新语义暂停对应 packet，不影响无依赖的任务。

回审者重新检查实际 diff 和产物，优先核查跨 owner partial/EHR、值可用性依赖环、自动资源条件的路径性、未知控制与 zero commit，再检查当前目录的执行收据与历史归档边界。只读检查不是执行证据；作者自评不能替代独立审查。当前示例审查无阻塞发现；完整 API 验收仍未完成。
