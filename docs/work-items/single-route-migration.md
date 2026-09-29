# 单一路线迁移执行账本

状态：active。开始：2026-09-27。PM：本会话。完整目标是按[修订 6 计划](../development/pycircuit-modernization-plan.md)使 pyCircuit 收敛为成熟、可验证的 GFSIM Pythonic → MLIR → 统一硬件 IR → C++/Verilog 框架；本账本不把目标缩成治理或样板。

2026-09-28 最新设计方向：用户要求统一 ac.reg、无状态 rule proposal、
原语内 Xfer commit/discard、reg-only 模块数据连接与 SimQueue 退役。
执行增补见 [C2-R1](../rfcs/migration/c2-r1-unified-register.md)；已有
切片仍按其原精确候选保留验收，不自动升级为新 reg 合同通过。

后续同线程输入又确定了 [M1 模块/System/无 self 联合方向](../rfcs/migration/c2-m1-module-system.md)：
parent/children 属实例，link 建树和唯一 reg 连接；system 独占遍历；
Python 采用用户明确选择的模块函数与嵌套 rule。R1 B 的既有 review
不覆盖这些实质新增内容，当前设计需联合重基审阅。

用户最新授权把 Bluespec 的 interface/effects 设计吸收入 M1 C，并要求
纯 Pythonic、无 Queue 作者概念、MLIR 自动推导及可交给有界执行 agent
的清单。现在以[W00–W12 checklist](../development/migration-agent-checklist.md)
和[V00–V49 verification matrix](../development/migration-verification-matrix.md)
派发；所有产品复选项仍未完成，不能把准备文档计作迁移实现。

## 授权与边界

用户已明确授权按计划执行迁移，并指定本会话为 PM。单一路线与 hard break 方向已给定。用户此前要求任何接口变动先请其 approve，此要求继续有效；计划审阅不是 C1/C2/C3 的精确接口批准。

初始 source HEAD 为 `8887e6dec7b4cc530a9967c860dc6a224d79a4ab`；[来源清单](../gates/logs/20260927-migration-intake/source-inventory.json)记录 donor、dirty 状态和关键摘要。既有 `examples/davo/` 和 `.omx-state-locks*` 不属于本任务的清理范围。历史 OMX ultragoal 仍有其他 consumer 工作，本次不覆盖；原生 Goal 保持整体目标，任务状态在本账本维护。

## 当前工作包

| ID | 状态 | owner / 文件归属 | 验收与证据 |
| --- | --- | --- | --- |
| B01 来源与组件清单 | done | PM；`docs/gates/logs/20260927-migration-intake/source-inventory.json` | 两仓 fresh HEAD/status，7 组件路径、donor 批准内容摘要、来源/许可事实；独立 governance reviewer 已核对 donor HEAD 与九个内容绑定 |
| B02 能力/退役矩阵 | done | PM；`migration-capabilities.md` | 五组修补后独立 Astra coverage 审计 PASS；[证据](../gates/logs/20260927-capability-inventory/revision-b-review.md)。仅清单完成，所有能力实现/双后端/退役门槛仍须逐行关闭 |
| B03 当前版本基线 | verified | `baseline_verification`，test-engineer，Sol medium；专属 baseline 输出与 gate evidence 目录 | 当前源码自行构建，Python G0 和最窄 native/双后端证据；不采用旧绿灯 |
| G01 治理与 skills 落地 | done | `governance_impl`，executor，Sol medium；其派发中列出的治理/skills/导航文件 | lint/docs/skill validation + 独立审查；不改产品合同 |
| D01 C1 设计 | done | `interface_design`，Architect，Astra xhigh，只读设计建议；PM 写精确提案 | [C1 修订 C](../rfcs/migration/c1-pythonic-source.md) C 版已独立 Astra approval-ready；[审阅证据](../gates/logs/20260927-c1-source-review/revision-c-review.md)，[用户已批准该精确修订](../rfcs/migration/approvals/c1-pythonic-source.md)；精确 IR/SDK 另见后续 D02/D03 |
| D02 C2 IR 精确提案 | done | interface_design（Astra xhigh）设计补齐，PM 整理 | [C2 修订 C](../rfcs/migration/c2-mlir-contract.md) 已关闭全部独立审阅问题，[Astra xhigh approval-ready](../gates/logs/20260927-c2-review/revision-c-review.md)；[用户已批准](../rfcs/migration/approvals/c2-c3-foundation.md) |
| D03 SDK/driver/runtime | done | PM 起草，interface_design（Astra xhigh）只读补齐设计，PM 转录 | [C3 修订 C](../rfcs/migration/c3-driver-runtime.md) 已关闭全部独立审阅问题，[Astra xhigh approval-ready](../gates/logs/20260927-c3-review/revision-c-review.md)；[用户已批准](../rfcs/migration/approvals/c2-c3-foundation.md) |
| I01 私有单文件源码捕获 | done | 隔离 checkout；governance_impl 实现（Sol medium），baseline_verification 独立测试（Sol medium） | 36 focused / 253 unit 通过、独立 Sol high code-review PASS，集成 `30e4f709`；[证据](../gates/logs/20260927-c1-capture/review.md)。不接 C2/C3/公开入口；Luna 派发受 thread limit 阻断，实际使用 Sol |
| I02 C2-F01 MLIR 基础 | done | 隔离 checkout b3df12ad；governance_impl 实现、baseline_verification 独立测试，均 Sol medium | 已集成 d104dae0；独立 Sol high review PASS，17 GTest + 4 lit 通过，主 checkout 重建复验相同 21 项；[证据](../gates/logs/20260927-c2-f01/integration/results.md)。仅基础属性/类型与私有验证器，不代表 C2 pipeline 闭合 |
| I03 C2-F02 类型/静态值 | done | 隔离 checkout 9c1a3502；governance_impl 实现、baseline_verification 独立测试，均 Sol medium | 已集成 29b424bf；独立 Sol high review PASS；主 checkout 28+6 GTest 与4 lit通过，[证据](../gates/logs/20260927-c2-f02/integration/results.md)。只验证结构与注入 resolver 匹配，真实 header authority 待接入 |
| I04 C2-F03 identity | done | 隔离 checkout 8f7bd5bb；governance_impl 实现、baseline_verification 独立测试，均 Sol medium | 已集成 4b84de00；独立 Sol high PASS，主 checkout 36+6 GTest/4 lit通过，[证据](../gates/logs/20260927-c2-f03/integration/results.md)。结构验证完成，转入实际 Packet header；不声称 context/unit/link 完成 |
| I05 U01 Packet source/header | done（隔离候选） | codex/gfsim-source-units；governance_impl importer（Sol medium），u01_header_authority registry 与 source_transport（Luna high），baseline_verification 独立测试（Sol medium），PM 整合 | [验收](../gates/logs/20260927-u01-native/acceptance.md)：d6fb408e，36 foundation/14 header/51 system 共 101 项通过、0 skip，Sol high PASS。真实 Packet/body/header 与 header-only 消费成立；尚非公共 driver/link/双后端闭环 |
| D04 C2-N1 名称绑定增补 | done（合同） | interface_design Astra xhigh 设计，PM 整理；namespace_review 独立 Astra xhigh 审阅 | [修订 C](../rfcs/migration/c2-n1-namespaces.md) approval-ready，[用户已批准](../rfcs/migration/approvals/c2-n1-namespaces.md)。实施与完整 gate 尚待完成，不能以 U01 替代 |
| I06 U02-A0 共享前端服务 | done（隔离候选） | b797eaf77；u02_frontend Luna high 实现，baseline_verification Sol medium 独立测试，governance_review Sol high 独立审阅；PM CMake 整合 | [验收](../gates/logs/20260928-u02-a0/acceptance.md)：101 项、0 skip、CTest 2/2；六份原始产物逐字节等同 U01-E，14 项提交内容绑定核对；Records 621→498 行，不改变 schema |
| I07 N1 source/header 子集 | done（三类别隔离候选） | 隔离提交 03625a3d；u02_frontend、u01_header_authority、namespace_unicode 均 Luna high；baseline_verification Sol medium 独立测试；governance_review Sol high 审阅 | [最终验收](../gates/logs/20260928-u02-n1/acceptance.md)：126 项通过；普通 Python 3.12 CTest 3/3，显式 Unicode 工具通道 CTest 4/4；Sol high PASS，21 文件绑定核对。全 N1/link/emit 仍待后续 |
| D05 外部 DUT I/O 与资源事务 | active（设计） | dut_resource_design，architect，Astra xhigh 设计；独立 Astra 审阅随后进行 | [typed DUT I/O 修订 B](../rfcs/migration/c3-dut-io.md)已起草，[独立 Astra xhigh 审阅](../gates/logs/20260928-dut-io-review/revision-a/independent-design-review.md)要求补齐 ingress 检查、C linkage、失败状态和调用者/gate；修订 B 已获[独立 approval-ready](../gates/logs/20260928-dut-io-review/revision-b/independent-design-review.md)，已向用户提交精确审批，尚未批准；[资源事务备忘](../rfcs/migration/c2-resources-transactions.md)只列 donor 复用边界及设计缺口，不是完整合同。两者均未批准，禁止用 host 模拟核或后端私有补丁绕过 |
| S01 基础 ALU/BRU ELF 消费者接入 | active | 独立 SSM 工作区 codex/pyc-alu-bru-migration；框架树只记录外部验收边界 | SSM 3c1cbeef5 修复 ELF 可执行 section，独立审阅 PASS、8 项主机测试通过；新建 gfrun 对同八 ELF 的 Block 数与七例可见 R2 符合独立 oracle，gfsim 仅三例 ALU 可运行，五例分支因 SPE Unsupport 失败。生成 pyc DUT 与完整同 ELF 对照仍未执行 |
| D06 SSM basic-ELF scalar 配置 | approval-ready（待用户批准） | SSM 5897214f8 的修订 D；ssm_basic_architecture Astra xhigh 独立设计建议，PM 整理；ssm_profile_review 另一个 Astra xhigh 实例独立审阅通过 | 精确 SHA-256 `13ee3dfc564f1fa4e3ad033508ad8143a7a37452d6540670cda06aa98331c0e4`；八例 J/B.cond、IFU byte-valid、Core/SPE owner 和真实提交边界已写明；已单独请求用户批准，尚未获批。不改原 oracle、不将核逻辑移到 host |
| D07 C2-A2 源 use 载体 | approved（由 M1 C 联合重基） | u02_ods_design Astra xhigh 给出缺口与精确建议；PM 写[修订 B](../rfcs/migration/c2-a2-source-use.md)，source_use_design_review 独立 Astra xhigh 审阅 | 原修订 B 由 [M1 C](../rfcs/migration/c2-m1-module-system.md) 的 lexical/reg 规则精确重基，并在[联合批准记录](../rfcs/migration/approvals/c2-r1-m1-interface.md)中授权实施；旧 self/DFFE 解释不再适用 |
| I08 U02-A 逐源 module/state/rule | done（隔离候选） | 隔离提交 946cd022；u02_frontend 实现 importer，PM 整合 ODS/registry/CMake，baseline_verification Sol medium 独立测试，governance_review Sol high 独立审阅 | [U02-A 验收](../gates/logs/20260928-u02-a/acceptance.md)：38 文件绑定核对；真实 Packet→AccumulatorProbe→ProbeRoot 各自编译、parent 只读 header；159/159、零 skip，CTest 4/4，独立审阅 PASS。原样 Accumulator/Core、link/final、双后端仍未完成 |
| I09 N1 常量类别 | done（scalar literal source/header 子集） | n1_constant executor 实现 importer/registry/namespace；PM 实现 ac.constant ODS/verifier/CMake；n1_constant_review Sol high 独立审阅 | 隔离提交 `5923ce4e`、`856ff9cc`、`2cac172a`；[验收](../gates/logs/20260928-u02-n1-constants/acceptance.md)记录独立复审 PASS、15 native + 5 system。record/list/computed 常量、module facade、N1 link/final/emit 仍开放 |
| I10 U02-B 数学 IR 基础 | done（隔离候选有界子集） | u02b_math_design Astra xhigh 只读设计；u02b_math_ops executor 实现 ODS/verifier/测试；u02b_math_review Sol high 独立审阅；math_provenance_design Astra xhigh 制定修补边界；math_authority_repair executor 实现权威校验 | 隔离提交 `ca5bf204`、`05df7fe1`、`1018c501`、共享测试 `ac821900`；[验收](../gates/logs/20260928-u02b-math/acceptance.md)记录独立复审 PASS、PM CTest 5/5 与逐源系统 78/78。其余数学、SCF、link/final/双后端仍开放；A2 精确批准前不实现 source use |
| I11 U02-B rule 读写分析 | done（隔离候选有界子集） | u02b_rule_analysis executor 修补；u02b_rule_review Sol high 独立审阅；PM 集成共享测试 | 隔离提交 `cb1a5195`、`2af0649d` 与共享测试 `ac821900`；[验收](../gates/logs/20260928-u02b-rule-analysis/acceptance.md)记录独立复审 PASS、24 native + 22 system。完整 rule lowering/SCF/source use 仍开放 |
| D08 U02-C source link 设计 | active（只读设计） | u02c_link_design Astra xhigh；PM 集成建议 | [纵向切片摘要](../gates/logs/20260928-next-vertical-slices/design-summary.md)建议先完整 body/header 接纳，再 private linked-semantic 特化与 owner；ElementEffect read-origin 完整重算的已批准载体正在独立核对，缺口若需新增接口仍须审阅与用户批准 |
| D09 C2-L1 current-read 载体 | approved（由 M1 C 联合重基） | u02c_link_design Astra xhigh 找出来源丢失，PM 起草[修订 B](../rfcs/migration/c2-l1-source-read.md)，source_read_design_review 独立 Astra xhigh 复审 | 修订 B 的 current-read carrier 由 [M1 C](../rfcs/migration/c2-m1-module-system.md) 重基到 lexical reg，并在[联合批准记录](../rfcs/migration/approvals/c2-r1-m1-interface.md)中授权；完整 effects/link/final closure 仍由 W04–W10 分包验证 |
| I12 C3 单源 compile | done（P1 私有 macOS/static）/revise（P2） | c3_compile_design Astra xhigh 只读设计；c3_publication executor 事务修补、c3_publication_fs_repair executor 平台修补，独立 code-reviewer 复审；c3_source_files executor 私有 P2 | [P1 验收](../gates/logs/20260928-c3-publication/acceptance.md)：事务审阅 C 与 FS 审阅 B PASS，PM 111 pass/3 Windows skip。P2 `136cb84d` 的[独立审阅 A](../gates/logs/20260928-c3-source-files/review-a.md)发现源码祖先 symlink race 和命令级锁集合两项问题，修补中。Windows 真机、P3 helper、P4 CLI、P5 CMake/gate 均开放，公共 compile 不可用 |
| I13 N1 module facade 子集 | done（隔离候选 source/header） | PM 写真实 producer/facade/consumer 系统门槛；n1_constant_review Sol high 独立审阅 | 隔离测试提交 `c97959ff`；[验收](../gates/logs/20260928-u02-n1-module/acceptance.md)记录 6/6 namespace 系统用例、provider/facade Python/body 隐藏、两个独立 child 与过期绑定拒绝。全 N1 link/final/emit 仍开放 |
| D10 统一 reg/proposal/Xfer | approved | migration_architecture_review，Astra xhigh 架构建议；PM 转录；unified_reg_design_review，独立 Astra xhigh 审阅 | [工作包](m2-r1-unified-register.md)、[独立复审](../reviews/20260928-c2-r1-unified-register.md)和[联合批准记录](../rfcs/migration/approvals/c2-r1-m1-interface.md)绑定修订 B SHA-256 `76283661bffba96aad544e31d2b4996f019135f448c4c8b685ebb02eab8edfba` |
| D11 module/system/lexical source | done（设计合并 D12 复审） | migration_architecture_review 与 lexical_frontend_design，两个 Astra xhigh 作者建议；PM 转录；donor 调查 Sol high | [工作包](m2-m1-module-system.md)；初稿 A 的独立派发曾受 thread limit 阻断，现 M1 C 与交付包在 D12 完成独立复审；此 done 仅为设计，非产品实现 |
| D12 interface 与执行交付 | approval-ready（设计交付） | PM 编写 M1 C、checklist、matrix；bluespec_research 独立验证，实际 gpt-6-astra/high，未写本提案 | [独立审阅](../reviews/20260928-interface-handoff.md)关闭输出交付、源码重排比较和 QUIESCENT 三项问题；Python 无 Queue/Interface DSL；W00–W12/V00–V49 有文件、oracle、反例与候选绑定；未宣称产品 gate 通过 |
| I14 W00 候选预检 | done | PM；隔离候选与 Git metadata | 用户[批准联合实施包](../rfcs/migration/approvals/c2-r1-m1-interface.md)；修复该 managed worktree 的 per-worktree core.worktree 指向，普通 git status/top-level 现精确定位候选；冻结 HEAD `82f161ea` 与 7 个既有 SourceLink overlay 文件摘要，不覆盖其内容 |
| I15 W01 独立门槛 bootstrap | active（首批门槛已建立） | 独立 test lane；RegContracts、SourceFacts、lexical system 与旧 native schema 迁移；PM 串行整合测试 CMake | 当前候选 Reg 10/10、SourceFacts 14/14、Math 13/13、Module 25/25、lexical 18/18，均零 skip/xfail；V00/V01 完整 evidence launcher 与未来 V19+ selectors 仍未闭合，不能标 verified |
| I16 W02 reg/rule schema | implemented（待最终包验收） | implementation lane；ACIR ODS/type/reg/module verifier；独立 test/review lane | 单一 `ac.reg`、clock/reset、单 rule region、D/E results、concrete list/reset authority、rule owner/target/type/alias verifier 已实现；PM fresh gates 见 `.pycircuit_out/w04-acceptance/`。runtime DFF/DFFE discard/Xfer 的 V10/V11 动态责任仍属 W08，故本行不冒称整包 verified |
| I17 W03 lexical frontend | implemented（获批子集） | frontend implementation lane；独立 test/review lane | function module/system、nested rule、nonlocal、local SSA、单层条件 proposal、bare return、owned/connection alias、Queue API hard break 与 system-as-child 拒绝已实现；lexical 18/18。更复杂控制流/helper/动态 collection 按合同 fail closed，完整 V04/V06/V07 仍随后续包扩展 |
| I18 W04 source facts / next-scalar | done（有界子片） | dialect verifier、frontend producer、独立 SourceFacts/system tests、独立 Sol high code review | `ac.source.read/use`、UseID、真实 read occurrence、next-scalar target/ValueID、conditional path、yield ownership、duplicate ID/cross-target/type-domain拒绝已实现；SourceFacts 14/14、lexical 18/18，最终 reviewer 绑定 `ACIRModuleOps.cpp=c11a515f…`、`PythonImportRules.cpp=126d9ed0…`、`SourceFactsTest.cpp=666bcbc5…` 后 PASS。helper_result/helper_return、P3 RuleEffectView、无条件 bypass closure 与 header effects 重算仍开放，不能称完整 A2/P3 closure |
| I19 W04-P3 / W05 interface authority | done（有界 scalar/record 子片） | compiler-private RuleEffectView、header producer、snapshot consumer；独立 tests 与只读 acceptance | header effects 由实际 SourceRead/SourceUse、bindings/handles及 verified child contracts重算；SourceBodySnapshots复用同一分析比较type/R/W/precision/origins；old ModuleMember flags不再发布接口。shared closed comparator无printer fallback；单rule重复拒绝、跨registration origins canonical union。RuleEffects 7/7、lexical 20/20、SourceFacts 14/14、Reg 10/10、Module/snapshots 25/25，独立最终验收 PASS。helper-result/return、dynamic selection/collection、标准 `acir-infer-rule-effects` pass注册和W06 bottom-up StateID link仍开放 |
| I20 W06 ModuleGraph / StateID link | done（有界 scalar/record 子片） | compiler-private ModuleGraph、SourceLink integration；独立 ModuleGraph/SourceLink tests与只读 acceptance | 完整 source-unit closure构建唯一root、OwnerRef、parent/ordered children、postorder、owned/boundary StateID与formal actual aliases；拒绝递归、不可达、bad controls、private/foreign handle、arity/port和write-capable alias错误。SourceLink在authority/snapshot/native verify后强制构图。SourceLinkAdmission 5/5、ModuleGraph 5/5，独立验收 PASS。V26 runtime lifecycle、multi-element child connections、公开/serialized graph ABI仍开放 |
| I21 W07 ProposalGraph / conflict closure | done（private analysis 子片） | compiler-private ProposalGraph、SourceLink integration；independent proposal mutation tests与只读 acceptance | 实际 SourceUse 经ModuleGraph重解为StateID contributions，保留UseID/ValueID/D/E/value/valid/path；按StateID生成Hold/Forward/ExclusiveMerge recipe。true/true与无证明possible overlap拒绝，constant-false不阻其他commit；verifier重绑actual SSA/identity、重算完整conflict pair matrix并检查commit active全集/global permit。Proposal 9/9、SourceLink 5/5、ModuleGraph 5/5，独立验收PASS。numeric/check permit materialization、child动态fixture、final residual-source elimination与双emitter仍开放 |
| I22 W08 GFSIM DFFE / flat system | done（M1-C bounded runtime） | `gfsim::SimDFFE`、private `SimModule` lifecycle、generated-precommit `SimSystem`；independent runtime tests/review | Q-only-Xfer、disabled/discard/reset pending、不重放和无分配保留；已删除公开Check/Drive。System执行all Work(epoch)→one generated precommit→owner Xfer内Write(D,E)+primitive Xfer；失败零Xfer/Q/cycle并丢scratch，Reset只走reset transfer，零rule Quiescent保持Ready，partial Build失败不可Reset复活，event按成功epoch替换batch且失败保留上一batch。RegRuntime/SystemLifecycle/Observation三target绿，独立Sol/high复审PASS。真实multi-instance generated对象、Build/Freeze owning tree和W11 ABI仍开放 |
| I23 W09 observations | done（unconditional direct-read 子片） | source observe ODS/verifier、private ObservationGraph、Python print/log/report producer、bounded runtime slots；independent tests/review | canonical unshadowed print/log/report生成SourceRead/SourceObserve、ValueID/constraints和ordered required_observations；Graph重绑owner/registration/site/spec/value/path；runtime固定slots在Work冻结old-Q，失败Discard，成功Xfer后t+1稳定发布，gauge commit/reset闭合且hot path无分配。ObservationContracts 6/6、runtime 9/9、source observations 9/9、SourceLink 5/5，独立review PASS。conditional observe、assert/check/numeric proof、two-instance graph排序oracle、runner `--events`与ReportStat JSON仍开放，不能称完整V36–V40 |
| I24 W09 checks / conditional observation / link permit | done（direct-bool check子片） | source expect ODS/verifier、Python assert/conditional instrumentation、private CheckGraph及ProposalGraph permit integration；independent mutations/link tests | assert生成direct bool SourceRead、CheckID与ordered RequiredCheck；conditional assert/print/log/report使用真实branch path；two-instance same-site observation按OwnerRef稳定排序且unit输入置换不变。SourceLink顺序Module→Check→Proposal→Observation，有checks时global/per-commit permit非恒真，删图/重定向/required漂移拒绝。CheckContracts 9/9、source observations16/16、Observation6/6、SourceLink5/5，独立验收PASS。numeric proofs、runner `--events`/ReportStat JSON、sink I/O和synthesis projection仍开放 |
| I25 W10 FinalProgram / bounded dual emit | implemented（hierarchical dual emit＋Numeric N0-A/B0/B1a/B1b/C1/D1–D3 accepted；W10仍request changes） | move-only FinalProgram、frozen FinalSystem/realization、private C++/Verilog emitters、原子backend harness、独立可执行oracle | Hierarchy/commit/observation与双backend三层执行已接受。C1完成input-add事务 lowering；D1完成sub和六类compare；D2完成exact and_bits与显式full-mask low_bits；D3完成单checked to_bits、range CheckBinding和guarded scf.if。所有包闭合source ownership、I/F/C/S、APInt domain、multi-rule rollback、root identity与幂等，独立Sol复审PASS。75/75 source-math、PM 19/19 ACIR targets，[C1复审](../reviews/20260929-n0-c1-scalar-lowering.md)、[D1–D3复审](../reviews/20260929-n0-d1-d3-numeric-closure.md)、[阶段复审](../reviews/20260929-w10-final-system-closure.md)。backend harness已删除静态JSON假证据并真实运行minimal C++/RTL；严格V41–V44当前1绿7红。仍缺Python算术/控制流producer、graph/final/backend numeric执行、完整Result/events与调度/源码重排；禁止进入W11 |
| I26 Python numeric producer bridge | done（单输入unused-local子片） | 独立Sol实现与测试；Sol/high复审；PM整合private harness | Python add/sub与六类compare生成既有C1/D1 recipe、SourceRead/ValueID/proof_scope；private `--lower-numeric` 验证真实Python到finite SSA。32/32新系统测试、68/68相关frontend测试、19/19 ACIR通过，[验收](../reviews/20260929-python-numeric-bridge.md)。numeric next/use/yield绑定、条件流、Graph/Final/backend admission仍开放；下包N0-U1 masked next assignment |
| 用户接口批准 | partial | 用户 | C1-C、C2-C、C3-C、C2-N1-C 与 R1-B/M1-C 联合实施包已批准；参数化/动态 collection、circular-buffer 库、外部 typed DUT、多时钟/CDC、四态 source 值及开放 RTL fault continuation 仍须后续合同 |

所有 writer 使用互斥文件归属。U01 的 ODS/原生 importer/非安装 harness 与产品 CMake 由 governance_impl 负责，测试及测试 CMake 由 baseline_verification 负责；private transport 单独派发。实现期间 native build 由 governance_impl 操作，稳定后移交测试 owner，其他 lane 不用同一输出目录构建。PM 维护主 checkout 文档，不改 candidate 产品源码。

U01 的临时文件规模例外已由 I06 关闭：共享 context/signature 抽取后 PythonImportRecords.cpp 为 498 行；N1 冻结候选为 518 行。保持新手写原生文件小于 600 行的要求，registry、Unicode 与前端职责分开，CMake 注册由 PM 串行整合。

## 全项目里程碑

| 阶段 | 状态 | 完成证据要求 |
| --- | --- | --- |
| M0 合同、基线与准入 | active | B01–B03 及精确 C1/C2/C3 用户批准，能力无未分类行 |
| M1 治理和 donor 准入 | active | 可执行治理/skills、试运行、独立审查；DeepSeek 可选接口另批准 |
| M2 单主干双后端最小闭环 | active（出口未验收） | 新 capture/MLIR/source-unit/root/Cpp/RTL 同一 final IR 执行；R1 reg 首片与后续 FIFO 库各有出口，不能由旧链兜底 |
| M3 完整批准能力 | pending | 整数/record/Queue/state/memory/CDC/四态/原子性等矩阵逐行验证 |
| M4 使用面与删除准备 | pending | examples/tests/docs/driver/SDK 迁移、反向依赖/删除 manifest |
| M5 hard break | pending | 旧源码/构建/安装路径退役；同候选更新决定、AGENTS、docs、gates |
| M6 规模与 SDK | pending | 并行/增量编译、relocation、错误发布、性能基线与平台结果 |
| M7 完整发布候选验收 | pending | 独立最终审查、完整 gate/能力/删除/接口批准审计；如发布则遵守外部授权 |

任务 `verified` 需要对应当前内容的证据，`done` 由 PM 集成验收后设置。单个 agent 完成或一个小样板通过均不代表整体目标完成。

## 当前限制与下一步

C1-C/C2-C/C3-C 均已获用户精确批准，进入对应接口实施。继续不依赖尚未批准接口的准备工作，实施时逐项核对批准范围。实际测试结果、独立 review 与用户回复到达后更新本账本；不提前宣布产品通过，不把审批等待扩大成全仓停止。

## 本轮基线结果

当前源码重建后：unit 217/217、Agentic contracts 29/29、CLI 6/6、frontend 426 pass/1 skip、focused native 20/20、selected lit 7/7、额外执行 C++/Verilator pass/fail 的义务 gate 1/1。详见[结果](../gates/logs/20260927-migration-baseline/results.md)。有限 family/multilane parity 用例在逐源发射前失败 `ACIR-EMIT-002`，该能力仍未获得执行证据；不能称为全框架全绿。

## PM 本轮验收

G01 已经独立 Sol code-review PASS，项目 skills 验证、文档构建、changed-file checks 和仓库/文档目录精确门槛通过；治理激活不批准产品接口。B01 的 donor/target 定位与内容由独立 reviewer 核对，完成来源调查交付。B03 已完成定向基线采集并明确失败/未运行范围；完整产品质量目标继续 active。C1 修订 C 已由用户明确批准；批准记录与原文内容绑定，C2/C3 未因此获得批准。

## C1 实施准备边界

C1 已批准的私有语法捕获可先在隔离 checkout 实施：读取单文件、保留 AST/源位置、禁止执行模型，不发布 C2 IR/schema、C3 CLI 或新的 public Python compile API。该工作不切换产品入口、不开放第四条 lowering 路线，后续只接入获批的新主干。

## C2 批准后的首个实施包

C2-F01 由 governance_impl（Sol medium）只读分解，C2-C 已获批准，此包已独立验证并集成。复用 `compiler/acir` 内唯一 `ac` dialect，先实现任意精度 MathInt 属性、临时 math_int 类型和 SourceSpan/PathComponent/Site 的闭合记录验证，独立正反例。DictionaryAttr 记录按 C2 原形式验证，不借 ODS 为其另造未批准的公开语法。Occurrence 依赖 StaticValue，后续按类型值→Occurrence→source-unit→header/schema cutover 顺序推进。

首批实现写入 ACIRAttributes.td、ACIRTypes.td、独立 ACIRSourceContracts.cpp 及其 target；测试由独立 owner 放在 `tests/mlir/agentic-circuit/ACIR/`。使用该候选自己配置的 LLVM22 build，构建 ACIRDialect/acir-opt-internal，再运行精确 lit filter。不得复制旧 build binaries。

当前 target SourceOwner 是 implementation/declaration schema，donor Python importer 仍 whole-project capture，且两仓 C++ namespace 不同。因此不能 bulk-copy importer，也不能用新基础类型已通过声称 source-unit/双后端闭环完成。公开 driver、捕获到 native 的通道、发布、生成/SDK/runtime wiring 已获 C3-C 批准，按各自工作包依赖顺序实施。

## C2-F02 实施边界

按获批 C2 的四类 DictionaryAttr 实现结构与上下文验证，不新增 op/type/attr assembly 或 public API。LogicalType 的 finite integer 使用数学边界验证最小 signless iN、signed/unsigned interpretation，运行 profile 1..64；StaticType/StaticValue 整数不受运行位宽上限。列表形状、bool/int、nominal identity、字段顺序/arity、range/default 逐项匹配。

私有 resolver 提供所请求 record 的 ordered LogicalType fields；F02 检查未知/错误 symbol、字段类型、active/done 区分按值循环与共享 DAG。真实 header authority、SourceOwner/import snapshot 一致性由后续 source-unit 包接入，F02 的注入 resolver 测试不能替代它。

实现可新增独立 ACIRValueContracts.cpp，复用私有 SourceContracts 声明与 MathInt/u64 helper，避免把已完成的基础文件扩成大文件。实现者负责 lib CMake 注册，独立测试作者负责 SourceTypeContractsTest.cpp 及其测试 target 注册；两者互斥文件归属，build 串行移交。不得额外增加非空 symbol/name、固定递归层数、任意元素数量上限或 record 总宽度64限制。

## 下一整合目标：实际逐源闭环

architect 已完成 F03 和后续主干分解。F03 只补一次 SourceOwner/Occurrence/ExpansionFrame/SpecKey 与 proof/owner/state identity 的私有结构验证，随后转入实际 Packet 源码生成 header；不继续以孤立字典测试替代 source→IR→backend 进展。

| 工作包 | 依赖与可执行出口 | 主要复用/替换 |
| --- | --- | --- |
| F03 identity | F02；闭合记录正反例，context 义务留给 unit/link | 复用 Site、StaticValue、u64 helpers，不新建公开 AttrDef |
| U01 Packet header | F03；Packet.py 真正产生 body/interface，header 单独支持 Request 默认值/kwargs/字段读取 | donor ImportPython/PythonScope/PythonConstants/ObjectSemantics/RecordSemantics/LowerEmit；接真实 header resolver |
| U02 Accumulator/Core 单元和 link | U01；三个 source 各自 producer，parent 仅读 header，link 才读 bodies；重复 child 独立 state | donor rule analysis/lowering/initialization、link 装载图算法；补 C2 authority/snapshot/SpecKey |
| U03 共同 final IR | U02；原样 C1 arithmetic/current-next，经 source-math、check/use/target proof 到可重验 final IR | 补 donor 对象 BinOp、integer-format/evaluation-path 缺口；不能调用旧 QueueGraph 文本链 |
| B01 C++/runtime 与 B02 RTL | 同依赖 U03，可独立并行；同一 final IR 执行独立 oracle | donor CodeGen/SimSystem 固定树；target VerilogEmitter/pyc.reg 与必要分析；适配 SourceOwner/SpecKey/data-enable |
| C3 driver/CMake 整合 | 发布事务可先开发；集成依赖 U02/B01/B02 | 获批 compile/link/emit、三 producer、每源 TU、ABI/reset/error/目录恢复 |

U01 的同名 schema 冲突须在隔离 migration 候选中一次替换：一个 ac dialect、同名 op 仅获批 C2 形式，更新 ODS/builders/verifiers/调用者；候选 CMake 不再编译或链接依赖旧 schema 的 semantic pipelines。不得添加新旧选择开关、ac2 dialect 或旧 lowerer fallback。旧产品和候选处于不同 revision，候选完成 M5 前不正式安装/发布；暂不编译的旧文件不等于 M5 已删除。

首个普通状态闭环坚持 C1 oracle：Reset 为 (7,19)，Work 后仍 (7,19)，三次 Xfer 为 (1,2)/(1,4)/(2,6)，Reset 后重跑。它不能关闭 M2 尚需单独资源合同的 Queue 用例，更不能关闭全框架目标。

固定 reference-list 的本地显式元素给出具体长度；constructor 中与参数无关的 literal len 约束可按 donor 对象式设计在 MLIR 提取。依赖 static 参数的 formal-list 长度属于尚未批准的 dependent-interface 扩展；裸 list 参数若无可闭合长度，不从某个 caller actual、默认值或最大下标猜测。Packet/Accumulator/Core 使用 scalar/record 端口，不依赖这一扩展。

### HeaderView 的消费边界

内部 loadHeaderView 仅验证 receipt 的闭合字段/安全名字和 header 内容，不打开或要求存在 child Python、body、depfile，也不扫描其他 AC；它不证明完整单元有效。producer 发布前、replace、完整恢复与 link 另用 FullSourceUnit 验证。只有合法 controller 无 journal 的稳定状态可使用缺 body 的 HeaderView；prepared/rollback 必须先恢复，committed 清理未结束仍须完整新目标验证，损坏控制状态不得降级读取。

无 controller 的外部 header-only projection 尚无 C3 接纳合同，不擅自套用只针对 program 的 unmanaged 分支；当前 U01 原生内存 header registry 和后续正常发布完成的单元无需这一扩展。

## U02 逐源模块实施分解

interface_design（Astra xhigh）已根据冻结 C1/C2、原样 fixture、donor 与正在实施的 U01 提供只读架构建议。该建议不改变接口，也不是 U01 代码验收。U01 通过独立测试/审查后，沿同一个 compilePythonSourceUnit 与 SourceHeaderRegistry 推进以下三个包；不另建 compiler 或重复 record/type/default 解析器。

| 包 | 实施内容 | 可验证出口 |
| --- | --- | --- |
| U02-A | module import/header、module/rule/instance/DFFE/yield 的获批 schema；constructor 分类、registered rule effects、owned reset 与 ports | Accumulator request 只读、result 只写；total reset=0；所有 rule output 为 data,enable。只计算实际注册的方法 |
| U02-B | 原样 Accumulator/Core 源；建 source-math、bool/scf、record/helper 运算和 source use/target 关系 | Packet、Accumulator、Core 三次独立编译；Core 四个 owned state、两个 child。parent 仅输入显式 headers，无 child source/body 读取 |
| U02-C | 单一路线内的 header/body linker、authority/snapshot 比较与去重、实际 body effects 重算、SpecKey/OwnerRef/StateID 绑定 | 相同 Accumulator specialization 复用定义，left/right 的 total 保持两个实例状态；缺 body 只阻断 link，不阻断 header-only compile |

U02-B 先交付已批准 C2 的表达式/分析基础，再在 A2 获得精确批准后接入可序列化的实际 source use。当前 `RuleCompiler` 只接受名称、字段和 literal，并把输出数误当返回形状；原样 `Accumulator.accumulate` 有一个返回值与两个 next 目标，`Core.advance` 没有返回值却有一个 next 目标。rule scope/effects 须分别跟踪局部 SSA、拍初 current 与 next 写，且修正 input argument index 和 member index 混用；不能把 member Store 计作 current read。引入 `scf.if` 后，helper 允许集、调用 DAG 和非法 effect 检查必须递归走子区域。表达式仍先用任意精度数学整数，经显式范围证明才转换到有限 bits；测试应包括 255+1 在 range(512) 得 256，不能只用 `&255` 掩盖过早 i8 回绕。A2 获批前，不能用临时 assignment 字典或直接合并 yield 冒充所需的 source use/target 关系。

优先复用 donor PythonLower 的 constructor 分类、PythonRuleAnalysis 的 effects、PythonLowerRules 的注册/参数绑定、PythonLowerEmit 的 child endpoint 绑定和 PythonLowerInitial 的初始化递归。child 的源成员查询替换为已验证 header contract。record 初始化执行 header-owned helper，不把 constructor 参数顺序当成字段布局。donor acir-link 的符号/递归算法可用，其直接合并所有声明的接纳逻辑不符合目标 authority/snapshot 合同。

U02 必须为 fixture 中的数学加法和 mask 建立已批准 source-math IR；不能替换成旧 i8 回绕算术。它保存每个源写入/返回的 origin、value、path、target 关系，U03 才完成区间/位宽、range/error、ValueID/use/target witness 和 final verifier。不能从合并后的 yield 反猜源目标，也不能新增临时公开 assignment schema。

普通 DFFE 不引入 Queue admission。Accumulator 的 result 每个成功路径都返回；total 仅在旧 item.valid 为真时写入。Core advance 读旧 left_request、写完整新 Request；right_request 无 writer，保持旧值。两后端后续仍须执行 Reset=(7,19)、Work 后=(7,19)、三次 Xfer=(1,2)/(1,4)/(2,6) 及 reset/rerun oracle；U02 linked-semantic 产物不是这些执行证据。

独立正反例覆盖：隐藏 child source/body、仅 header 默认值与构造器调用、端口改名不改变 R/W、未注册方法无 active effect、序列化重读后绑定不变、缺 header、错误 nominal/range/actual、重复 authority、篡改 snapshot/signature/effects、结构 alias 重绑、跨 child 内部 state 访问，以及错误合并两个实例 total。仍含 math_int/helper/未闭合义务的产物不发布为 final program，不调用 backend。

该 scalar/record fixture 不需要新的用户接口批准；资源、memory/CDC/四态等扩展继续按能力矩阵补齐合同与审批。

### 已批准 N1 后的实施顺序

先执行 U02-A0：从 RecordCompiler 抽取 PythonImportContext 与 PythonImportSignature 私有服务，移除 Records/Helpers 重复的类型/静态值转换。签名服务返回 parameter AST、binding kind、optional default AST，不能固定输出 HelperParameter；module 的 static/connection Parameter 与 helper ValueConstraint 由各自调用者产生。隐式 receiver 由调用者明确选择，保留当前 capability diagnostics，不能把普通 helper 的第一个参数误删。清理前以 U01 的 101 项独立测试锁定行为，重构后复验并独立审阅；新文件按实际职责划分，解除 Records 的规模例外。

随后把 N1 两个必需属性同时接入 producer、registry 和真实 Packet→Facade→Consumer，替换按 canonical symbol 末段猜 export 的逻辑。词法名字到 canonical reference、reference 到 local/header declaration view 分层；独立保留所有 named-import 消费事件，不能从最终名称表反推。此时只记录已实现类别的 source/header 子集证据；ac.constant/ac.module.import、真实 link 和两个 emit 的门槛仍须逐项完成。

module/rule/DFFE 扩展沿同一 compilePythonSourceUnit 和共享 context 推进。模块构造器参数、owned state、注册调用与 rule body 的 Site.definition 使用 enclosing module class；相对路径包含 class body 内的方法与语句位置。record constructor 仍以真实 func helper 为锚。不要为模块静态构造器或 rule 方法制造假 func；registration occurrence 与 body occurrence 分开。保留 source-Module 绝对 AST path 供 N1 NamespaceSite 使用，从私有 anchor/root view 派生 definition-relative path，不破坏原路径。这是现有 C2 的 producer 约定，不是新增 header AST-path 白名单。

frontend lane 独占 context/signature/Records/Helpers/后续 Modules/Rules；registry lane 独占 header authority/N1 验证；测试 lane 独立写 oracle。SourceUnit.h、ODS、注册和 CMake 由单个 integration owner 串行修改。实际 Accumulator/Core 不能用 stub rule body 或旧 lowering 宣称完成。

## 外部基础 ALU/BRU 验收

用户追加的完成条件是 SuperScalarModel 的 pyc 实际运行现有基础 ALU/BRU ELF。
这是完整框架成熟目标的外部验收，不将目标缩小为消费者或 Packet。框架只承担
通用 Python/MLIR/统一硬件 IR/双后端/SDK；ELF、ISA decode、核层次、独立 oracle
与执行证据全部留在 SSM。SSM 主机前端不能计算操作数、ALU、分支结果或提交状态。

八个现有程序覆盖 ALU 独立/依赖/WAW、跳转、分支双向、reset/zero alias 和循环。
最终必须用新构建的逐源模块 DUT 无 skip 执行，保留原始期望、寄存器/提交与 block PC、
周期对账、真实在途重叠和负向用例；同 ELF reference 与独立 golden 比较留在消费者仓。
现阶段只有 fixture 和主机前端 8 项通过。SSM 原有 NDF 检查存在 627 项 NDF、48 项
style 诊断，修复前后完全一致，change-check 因既有 index 无效失败；不记为全仓通过。

typed DUT I/O 和资源/事务需要新合同，不能把 C3 portless root 或普通 DFFE 包装成
已经支持真实消费者。设计工作与获批 N1/U02 实施并行；新接口完成独立审阅后提交用户。
