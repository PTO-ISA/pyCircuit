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
| I27 M4 私有 design 文件桥 | done（复审 FAIL 已修补；候选 `81fb596b` 已推送） | m4_program_bridge 实现、m4_program_tests 独立测试、独立 reviewer、PM CMake 整合/证据/验收 | 新增 `acir-design-harness`：把逐源显式 body/header 读盘、link 并物化为一支 verified final design 文件，另一进程重解析后交同一 final verifier 与两个 emitter；三种模式都拒绝既有输出（file/dir/symlink/dangling），且全部验证与 emit 先于建文件。`ProposalGraph` 按 `ac.stage` 选 `ac.logical_type`（final）/`ac.logical_element`（source）。产物按来源 Python 文件名命名（`test_increment.py` → `test_increment.ac`），不设保留标签。[证据](../gates/logs/20260929-m4-design-bridge/README.md)：355 native（20 binaries）+52 Python 通过，2 V44 deselected。独立复审对 `ede5aec7` 判 FAIL：生产代码无缺陷，但两个反例测试空转（既有输出使 no-clobber 先拒绝；`"logical"` 子串只匹配 tmp_path）。已修：改断言真实诊断与"未创建输出"、stage 校验提到每 view 一次、补 `!logical` 诊断、去弱断言；负向对照证明该 stage 修复是 load-bearing（回退后正向双后端 emit 失败）。**独立 design/testbench 交付尚未成立** |
| I28 M4-D1 生成物角色拆分 | done（复审 PASS；修正 `0d2e3ab6`） | PM 实现 emitter 角色拆分与私有 `--glue-output`；独立测试与证据 | 新增 `FinalVerilogEmission{rtl,runtimeGlue}` 与 `emitFinalVerilogParts`；`emitFinalVerilog` 保持签名并返回 `rtl+runtimeGlue`，既有调用点与合并字节完全不变。emitter 在 `FinalModel`/`FinalModelSim` 边界分流；私有 harness `--glue-output` 把 `runtime-glue` 写到第二个文件，非 emit 模式 / `--target cpp` / 与 `--output` 相同均 rc=2；双目标先校验后创建、第二个失败回滚第一个。C++ 不拆分（模型消费 gfsim runtime，无 `FinalModelSim` 对应物）。[证据](../gates/logs/20260929-m4-role-split/README.md)：355 native（20 binaries）+60 Python 通过，2 V44 deselected。**尚无 `generated.json` bundle writer**，公开 emit 入口与 role 清单仍属后续。独立复审对 `fcdb75e6` 判 **PASS**（无 fail trigger 成立，合并输出逐字节不变、无接口/语义变更），但提出 1 项 major 证据溯源 + 3 项 minor，均已修复：`overlay-sha256.txt` 原先记录的是 clang-format **之前**的哈希、与任何已提交 blob 都不匹配，导致证据没有钉住被审修订——现已从干净重建重跑全部 lane，并在提交后按提交字节重算、用 `git show HEAD:<path>` 逐条核对通过；`FinalEmit.cpp` 抽出统一 `EmissionGuard`；回滚 `remove` 失败改为报错；C++ 不拆分表述收紧为「无可独立拆分的 simulation observation wrapper」；`rtl` 增加 `$strobe`/`AC_OBS` 缺席断言，并补 alias 路径用例以走回滚而非字符串相等守卫 |
| I29 M4-D2 design/testbench 产物边界 | implemented（候选 `3af302e0` 已推送；独立复审待做） | PM 设计边界测试与证据；不新增任何接口 | DUT-only closure 单独 link 成设计产物（名随源文件），system testbench 为**独立**产物；断言设计产物含 DUT 但无 `ac.observe`、无 testbench 符号、无 stimulus 字符串，且其 `rtl` 角色文件无 `FinalModelSim` 与 `AC_OBS`；带 typed 端口的 DUT root 仍 fail-closed（`unbound data formal`）并把 D5 依赖写成显式反例。[证据](../gates/logs/20260929-m4-artifact-boundary/README.md)：35 设计桥用例、62 Python selectors 通过，2 V44 deselected。仍是**无端口**设计产物，非独立 DUT 交付 |
| I30 N0-U1 masked next assignment | done（候选 `3b888d15`） | PM 侦察＋独立测试与证据；**无产品改动** | 先侦察确认 lowering 与专用 `U1` verifier 已实现要求，故只补测试。源 body 与最终产物**同时**断言：mask witness（`arith.andi`）、boundary `ac.numeric.proof` 带 check binding、`kind="range"` check、next-role UseID 与 `next_scalar` target、`ac.yield_bindings` 的 data/enable→target 绑定；RTL 断言 `initial0 = 8'd254`、full mask `8'd255`、d0/q0_e；用 Icarus + **层次探针**（无端口 root 无法从端口观察）跑 reset=254 → 两相保持 → 256 次 commit 回到 254，覆盖全部可达状态；7 个重定向/缺失突变（data/enable/role/target kind/check kind 重定向，proof/check 删除）各以具体 U1 诊断 fail-closed 且不产出文件。[证据](../gates/logs/20260929-n0-u1-masked-next/README.md)：新增 10 项通过；共享 lane 73 Python 通过、2 V44 deselected。**运行时 oracle 依赖仿真层次探针**；typed 端口 DUT 仍属 D5，本包不新增 accessor/端口/CLI/runtime API。补测后将 numeric next 形状覆盖实测成图（[覆盖图](../gates/logs/20260929-n0-u1-masked-next/numeric-shape-coverage.md)）：支持 4 种（无条件/使能守卫/条件 masked add、只观察）；9 种在编译期以具体诊断 fail-closed（无 mask、减法、乘法、operand 反序、非满 mask、双 reg、临时变量、双写）；并发现**链接成功但 emit 失败**的不一致——`state = other`、`state = 7` 能 link，reparse 后 emit 报 `source check analysis supports only verified U1 numeric next-state rules`。已在私有桥加 fail-closed 守卫（提交前用 emit 路径对 materialized 硬件模块的 clone 重建），二者改为在 **link** 拒绝且不产出文件 |
| 用户接口批准 | partial | 用户 | C1-C、C2-C、C3-C、C2-N1-C 与 R1-B/M1-C 联合实施包已批准；参数化/动态 collection、circular-buffer 库、外部 typed DUT、多时钟/CDC、四态 source 值及开放 RTL fault continuation 仍须后续合同 |

所有 writer 使用互斥文件归属。U01 的 ODS/原生 importer/非安装 harness 与产品 CMake 由 governance_impl 负责，测试及测试 CMake 由 baseline_verification 负责；private transport 单独派发。实现期间 native build 由 governance_impl 操作，稳定后移交测试 owner，其他 lane 不用同一输出目录构建。PM 维护主 checkout 文档，不改 candidate 产品源码。

U01 的临时文件规模例外已由 I06 关闭：共享 context/signature 抽取后 PythonImportRecords.cpp 为 498 行；N1 冻结候选为 518 行。保持新手写原生文件小于 600 行的要求，registry、Unicode 与前端职责分开，CMake 注册由 PM 串行整合。

## design / testbench 边界纠偏（2026-09-29）

用户指出框架把 pyCircuit 编译当成 program、生成物应是 design 与 testbench，
并要求 `.ac` 与来源 Python 文件名一致（“ac应该是和python的文件名一致”）。审计结论见
[边界与 primitive 授权审计](../reviews/20260929-design-testbench-ir-authority.md)：
M2 的封闭系统验证仍有效，但独立 design/testbench 交付不成立——V41/V42 选中的
root 是带 stimulus/phase/check/report 的 `@system`，这些内容一并进入 `FinalModel`。

本轮已做的收紧（不新增 IR/CLI/runtime 接口）：

- `.ac` 产物按来源 Python 文件名命名（`<stem>.ac`），不设保留标签；`design_top.ac`
  仅指 `design_top.py` 的产物。封闭自测 system 的产物即 `test_increment.ac`，
  不靠改名把测试刺激伪装成独立 DUT。
- 私有工具更名 `acir-design-harness`、参数 `--design`，不保留旧 alias。
- `FinalProgram` 仅是编译器内部容器名，不做机械重命名，也不引入
  `ac.program`/`ac.design`/`ac.testbench`。

已核对的实现事实（决定下一包的最小改动面）：

- `ModuleGraph` 已允许非 system root：无 `ac.root_kind` 时取唯一 zero-incoming
  定义（`ModuleGraph.cpp:125-139`），与 C3-C“基础 root 是普通 portless
  `@module`”一致。
- 但最终产物仍强制 system 形态：`FinalProgram.cpp:211` 要求 selected root，
  两个 emitter 直接产出 `FinalModel`+`FinalModelSim`+runner 的自驱动组合
  （`FinalEmitVerilog.cpp:336`/`:396`），即 design 与 testbench 混在同一产物。
- C3-C 已批准 generated bundle 的 `role` 集合
  `header/source/cmake/rtl/runtime-glue/source-map`，因此把硬件 RTL 与
  simulation/runtime glue 拆成不同 role 文件属于执行已批准合同，不是新接口。

### 边界探针（产品仓 `81fb596b`，只读可复现）

用已提交的 `acir-design-harness` 对三种 root 做实测，脚本与原始输出见
`docs/gates/logs/20260929-m4-design-bridge/design-testbench-probe.md`：

| root | link | 产物 |
| --- | --- | --- |
| `@module` **带端口**（`Increment(enabled,incoming,outgoing)`） | **rc=1** `selected root has an unbound data formal or synthetic root StateID` | 无 |
| `@module` **无端口**（两个 reg + 一条 rule） | rc=0，36 640 B | 有 `ac.system`（entry=DUT）与 DUT 自身 range check，无 `ac.observe`、无 stimulus |
| `@system`（当前 M2 fixture，带 stimulus/phase/check/report） | rc=0，193 173 B | `ac.system`/`ac.expect`/`ac.observe`/`TestIncrement` 全在同一产物 |

三条结论：

1. **无端口 module root 的 design 产物今天已经成立**（`blinker.py` → `blinker.ac`），
   正是 C3-C 已批准的
   base root 形态；其 `ac.expect` 是设计自身的 range check，审计明确允许留在设计内。
2. **带外部 typed 端口的真实 DUT 被 D05 外部 typed DUT/端口合同阻塞**：module 成为
   root 后其 formals 没有 parent 可绑定，直接失败。这才是"生成物应是 design 与
   testbench"这条纠偏的真正卡点——不是改名，也不是 emitter 工作量。
3. **角色混放仍在**：三种场景的 Verilog 输出都在同一份文本里同时含硬件
   `FinalModel` 与 simulation observation wrapper `FinalModelSim`；新 route 尚无
   bundle writer 产出 C3-C 已批准的 `generated.json` 角色清单，因此角色拆分虽属
   已批准合同，实现仍缺。

下一包（按依赖顺序，均不新增 op 或 role 属性）：

1. ~~M4-D1 生成物角色拆分~~：已完成（I28，`fcdb75e6`）；`rtl` 与 `runtime-glue`
   已可分文件产出且合并字节不变。仍缺的是把它接进公开 emit 入口与
   `generated.json` 角色清单。
2. ~~M4-D2 design/testbench 产物边界~~：已完成（I29，`3af302e0`），无端口 DUT 与
   system testbench 已作为两个独立产物被测试锁定。带 typed 端口/外部 testbench 的
   部分仍依赖 **D5 修订 B 的用户批准**（用户已回复“不用”），未批准前 fail closed。
3. `ac.expect` 完整字段 schema 的逐字段审批映射（当前
   `condition:i1,path:i1,kind:StringAttr,location:DictionaryAttr` 只找到语义与
   部分字段依据）；任何扩展先补映射并取得用户批准，不由子代理自批。
4. ~~C3-C 命名条款修订~~：已成文为[用户指示修订](../rfcs/migration/c3-artifact-naming-amendment.md)，
   冻结正文不改、只在下次 C3 修订时并入；公开 driver 实现时必须采用
   `<root-source-stem>.ac`，不得硬编码 `program.ac` 或保留标签。

## W10 状态更正与剩余缺口（2026-09-29 复核）

I25 行「仍缺 … 完整 Result/events」需要收窄，避免下一轮重复实现。

**已交付并通过**（与 M4-D1/N0-U1 同一冻结 lane，证据
[python.xml](../gates/logs/20260929-m4-role-split/python.xml)，73 passed / 0 skipped）：

- `test_v43_two_systems_terminate_once_and_reset_reruns`：两个 system 在 cpp 与
  verilog 上各跑 first_run/rerun，完整公开记录逐字段相等，terminal Result、
  report 身份与取值、commit 上限语义均被断言。
- `test_v43_zero_rule_system_is_quiescent_at_epoch_zero[cpp|verilog]`：零 rule
  系统第一次 Step 即 QUIESCENT，epoch=0、step_calls=1、模型保持 READY。
- `test_v43_oracle_rejects_empty_or_truncated_summaries`：runner 摘要 oracle 反例。

**依约 DEFERRED 到 M6**（`-k 'not v44'`，未计 PASS）：
`test_v44_schedule_permutations_preserve_values_errors_and_events`、
`test_v44_source_reorder_compares_explicit_semantic_identity`。

因此 W10 真正剩余的**工具出口**只有 M1 固定的 `--events`：

- 运行侧数据已具备：`ComposedObservations.cpp` 的 `composedObservation` 已产出
  完整 M1 Event 记录形状（kind/instance/registration/site/evaluation_epoch/
  commit_epoch/spec/values），composed run JSON 里也已有 `observations` 与
  `terminal` Result。
- 缺的是 sink 本身：`--events <path|->` 的运行前校验（既有文件、symlink、
  非普通文件、重复选项、打开失败一律非零退出且不覆盖）、JSONL 逐行输出，
  以及只交付成功 Step 的事件。
- **需先澄清的合同点**：M1-C 的 `Event.kind` 枚举含 `"report"`，但现实现把
  `report` 绑定只投递到 gauge 通道（`ComposedObservations.cpp` 要求
  `(tag == "G") == (kind == "report")`）。按字面实现 `--events`（report 也进事件流）
  会改变观察投递分工，属于 runtime/协议行为；因此在拿到合同裁决前不实现该项，
  也不擅自把 report 从 gauge 通道移走。

## 普通赋值的 final 重建缺口与 hasNumericInventory（2026-09-29 勘误）

N0-U1 的形状探测发现 `state = other` / `state = 7` 能 link、reparse 后 emit 失败。
**勘误（同日）**：这不是"是否要支持普通赋值"的新语言决定——普通赋值语义早已在
已批准合同内，问题是 serialized-final reconstruction 的分类不一致：

- 已有 direct-current/literal assignment authority（`FinalUses.cpp:16–65`），
  `ACIRFinalContracts.cpp:437–450` 也区分 numeric 与 generic final uses；
- 但 `hasNumericInventory`（`CheckGraph.cpp:40`、`Passes/InferRuleEffects.cpp:177`、
  `ProposalGraph.cpp:93` 三处逐字重复）把通用 `ac.value.binding`/`ac.value.use`
  当作"数值声明"。这两个 op 在 final materialization 中为**每个**赋值合成，
  于是谓词对非数值 rule 也为真，reparse 路径就只接受 U1/composition。
- 结论：这是**已批准赋值语义的 serialized-final reconstruction 缺口**；当前
  私有桥的 link 拒绝是保守的能力限制，不是最终修复，**不退役普通 copy/constant
  赋值，也不再要求用户重新决定是否允许普通赋值。**

### 已修复（2026-09-29，候选 `d877ff93`）

分类不变量已冻结并写入证据包，共享分析已收敛：

- 不变量：通用 provenance（`ac.value.binding`/`ac.value.use`）不等于数值义务；
  numeric 义务只能由显式载体声明（非空 `ac.required_numeric`、`ac.numeric.proof`、
  `ac.math.*`、带 `ac.check_template` 的 op）；不得用 dialect 名（`arith.*`）粗判；
  numeric 义务不能靠删字段降级；generic 仍走既有严格校验。
- 实现：新增内部 header-only 分类器
  `compiler/acir/lib/Compiler/RuleInventory.h::ruleHasNumericObligation`，四处副本
  全部收敛（`CheckGraph`、`ProposalGraph`、`Passes/InferRuleEffects`，以及经集成者
  明确批准扩入本包的 `ObservationGraph`——它原来内联了第四份同名逻辑，且在
  `:210` 对每个 rule 分类）。各调用点保留自己的 numeric/generic 校验责任。
- 校验：`copy` 与 `constant` 走完逐源 capture → link → 保存 `.ac` → 新进程 emit →
  clang/Icarus 实编实跑，观测轨迹 `[254,7,7]`、`[254,3,3]`，reset/rerun 相同，
  父子 alias 下物理 `ac.reg` 恰 3 个（无 relay/double reg）；4 项 generic 载体
  反例与 2 项 anti-downgrade 反例（删 `required_numeric`、再删 proof）全部拒绝。
  负向对照：回退五个编译器文件后 roundtrip 6 项全部失败。
- 证据：[generic-assignment-reconstruction](../gates/logs/20260929-generic-assignment-reconstruction/README.md)。
  lane：62 focused、89 system（2 项 V44 依约 deselected）、355 native（20 二进制），
  0 failures/errors/skips/disabled；V41/V42 轨迹与 4/5 物理寄存器不变。

历史勘误（保留原时点）：`CheckGraph.cpp`、`Passes/InferRuleEffects.cpp`、
`ProposalGraph.cpp` 三处逐字重复的 `hasNumericInventory` 把通用 value ops 当作
numeric 声明；`FinalProgram.cpp` 的 residual 检查虽提到这两个 op，但用
`hasGenericFinalUses` 正确豁免，未改。

`hasNumericInventory` 三处重复本身仍是待收敛的实现债；独立的 U1 closure
verifier（`ACIRNumericNextUse.cpp`）在任何选择下都继续强制闭合。

## design/testbench role 契约（工具层强制，2026-09-29）

审计「没有任何东西阻止把带 stimulus 的 `@system` 当成 design 产物」这条发现已修复：

- 私有桥新增 `--role design|testbench`，link 与 emit 两个模式都校验；
- root 带 `ac.root_kind = "system"`（外部驱动的 harness，实例化 design）时，
  必须显式标 `testbench`，否则 link/emit 都 fail-closed 并说明"system root 不是 design"；
- `@module` root 不得标 `testbench`；
- emit 从**产物自身**重新推导 root kind 再校验，不信任 link 时的声明。

用的是既有 `ac.root_kind`，**不新增 IR、role 属性、端口、runtime 或公开 CLI**。
IR 级的 design/testbench role 仍是需批准的开放提案（见审计 §1.3）。
审计 §1.5 的"私有夹具不得进入安装/导出面、不得成为标准入口必须知道的模型"也已核验
（三个私有 harness 无 `install(`、compiler lib 不引用 fixture 名）并加了守卫测试。

证据：[修复记录](../reviews/20260929-design-testbench-ir-authority.md) 末节、
`docs/gates/logs/20260929-m4-artifact-boundary/`（role 契约）、
`docs/gates/logs/20260929-m4-role-split/`（角色拆分）。lane：bridge 39 项、
masked-next 13 项、5 文件 80 通过 / 2 V44 deselected、0 failed/0 skipped；
native 未受影响（无编译器库源码变更）。

## 一等 system 提案状态与准备（2026-09-29）

用户已选择方案 2 方向：`ac.system` 成为带 symbol 与 body 的一等系统定义。
精确字段见 [C2-SYSTEM 修订 B](../rfcs/migration/c2-system-definition-role.md)
与 [C2-EXPECT 修订 B](../rfcs/migration/c2-expect-schema.md)。

**两份都没有用户精确批准记录**。2026-09-30 已补齐
[实际独立设计审阅归档](../reviews/20260930-revision-b-independent-review-archive.md)，
核对 reviewer 实例、结论及两份 B 的内容摘要，当前状态均为 **approval-ready，
待用户批准**。提案正文保留被审字节，当前状态由审阅记录维护；历史页眉不再
被误当作没有审阅。批准前仍不实施新 op/IR/CLI/manifest。

准备材料：[stage 分析](system-root-kind-stage-analysis.md)（source/header/linked/
final/reparse 谁承载 system、谁选入口，以及 final 的 `ac.entry` 与
`ac.system.entry` 已被 canonical 检查交叉核对、残留 `ac.root_kind` 已不起独立
作用）；[实施包三切片准备](system-first-class-slice-plan.md)（文件归属、独立测试
作者、拒绝用例、验收命令）。

已知冲突（批准后必须一起处理）：当前私有桥 `@system ⟹ testbench` 规则正是
修订 B §5/§7 要求删除的私有规则；它属 public `link --role`/IR role 变更，
本轮未改。

## M4 逐源 compile 编排入口（2026-09-29，候选 `64cb6106`）

私有可调用入口 `python/pycircuit/src/pycircuit/_source_compile.py::_compile_source_unit`
把已有组件接成一条可用流程：一次稳定的 Python source 快照 → 显式提供的 managed
interface units → 现有 native source compiler → 已验证 body/header → 原子发布
四文件 source unit（`<stem>.ac`、`<stem>.interface.ac`、`<stem>.d`、`unit.json`）。
stem 随来源文件名，不固定为 design_top。

关键约束与实现：

- **一套完整锁集合**：所有 interface unit 作共享输入、输出目录作独占输出，由同一个
  `_publication_lock_set` 覆盖输入快照、native 编译与发布；header 只从锁内快照写入
  scratch，native compiler 不会重新打开 provider 文件，因此"逐个读 header→释放→
  编译时重读→另取输出锁"在结构上不可能。
- **owner 发现不是权威**：discovery 只为命名预期输入；锁集合在锁内重新核对 owner 与
  artifact。
- **正常读取与恢复校验分开**：输入用新的 header-only stable validator，恢复用既有
  full validator；parent 因此只消费 header，恢复也不会为 header-only 让路。
- **depfile 来源可验证**：native helper 新增最小私有 `--deps-out`，报告**实际消费**的
  interface 闭包（来自 body 的 `ac.interfaces`，实测排除"提供了但未消费"的头）；depfile
  的 target 是已发布产物，依赖为原 source、被消费 provider 的 interface 与 receipt、
  以及 native 工具本身，并做 Make 转义，不含任何 scratch 路径。
- 另加 owner 后置校验：编译产物必须带请求的 `ac.source_owner`。

lane：unit 174 项 0 failures/errors（3 项既有平台 skip）；focused system 105 项
0 failures/errors/skips；LLVM transport 4 项 0 failures/errors/skips。三条 lane 退出码
均为 0，导出环境记录在证据目录的 `lane-env.txt`。证据
[source-compile-orchestration](../gates/logs/20260929-source-compile-orchestration/README.md)。

**如实报告**：`tests/system/test_source_module_units.py` 收集 **22** 项，配好 harness 后
**17 failed / 5 passed**，全部失败都带前端按设计拒绝 class/self 写法的诊断
（`@module and @system require function definitions; class/self authoring has been
retired`）。对**基线树**（`06680be2` detached worktree，同一 harness 二进制与同一环境，
只有 checkout 不同）复跑两个文件合计得到同样的 17 failed / 9 passed，失败集合逐项一致
→ 属既有陈旧 fixture，不是本包回归，本包不修（另立有界任务）。拆开看：本文件自身是
17 failed / 5 passed，transport 文件是 4 passed（合计 9 passed 即两者之和）。

`tests/system/test_source_transport_mlir.py` **不是**陈旧 fixture，也**不**排除：它的 **4**
项测试只受 `mlir-opt` 是否配置约束，而该工具在本机是 keg-only、不在 `PATH` 上。设好
`MLIR_OPT` 后基线与候选同为 **4 passed**；未设置时 4 项都在
`tests/system/test_source_transport_mlir.py:27` 以 `LLVM 22 mlir-opt is required; set
MLIR_OPT or PYC_TOOLCHAIN_ROOT` 失败（该未配置运行作为工具门负向对照留档）。该文件只
导入本包未改动的纯 Python capture/transport 模块，因此单列一条 lane，不计入"陈旧"。

本段与 lane 表是 `64cb6106` 的独立审阅判 FAIL 后的更正。审阅只否证证据完整性：原稿把
两个文件的收集数写成 15/6，并把 4 项 transport 失败误归因为已退役的 class/self fixture。
功能结论未被否证。代码侧的低危项（在持有的锁内重复读取 interface receipt，形成第二个
权威）已改为复用加锁前已验证的 owner（`51ec9ab5`）；证据更正落在 `d6233d1e`，其中
overlay 哈希由提交后的字节重算。

**复查（round 2）结论：PASS**（对 `51ec9ab5` + `d6233d1e`）。reviewer 独立重测三条 lane
（raw XML 174/0/0/3、105/0/0/0、4/0/0/0，重跑 171 passed/3 skipped、105 passed、4 passed，
退出码均 0），确认 XML 时间戳晚于 `51ec9ab5` 提交时间、且 `51ec9ab5`→`d6233d1e` 的
`python/ compiler/ tests/` 差异为空（证据提交未动代码）。基线对比在全新 detached worktree
重做，6461/6461 个跟踪文件与 `06680be2` 逐字节一致，两树同为 17 failed / 9 passed 且失败
集合相同。五项 overlay 哈希与提交字节及工作树均一致。功能结论 1–9/11 在 `51ec9ab5` 上仍
成立（对抗脚本 18/18、20/20）。一处非阻塞措辞已按 reviewer 建议澄清（上方 per-file 数字）。
结论原文与 reviewer 自述的未验证范围已归档在
[证据包](../gates/logs/20260929-source-compile-orchestration/README.md)。

本包仍**不代表 M4 完成**，也未取得用户对 packet 的验收；该 PASS 是独立审查结论，不是用户批准。

不新增公开 CLI/SDK/IR/runtime/manifest，也不实施一等 system。本包只是 M4 的逐源
compile 环节，不代表 M4 完成。

## C3 public driver：`compile` 与 `link`（2026-09-30，候选 `df3f2283`）

[批准记录](../rfcs/migration/approvals/c3-compile-link-driver.md)。批准对象是**实施已批准的
C3-C 接口**，不是新接口：`c3-driver-runtime.md` 正文 SHA-256
`0c476ced27519cf93427a89f77b9348d388e96db57fb183790144d24118b1170` 在动手前复核未变。

交付两个公开子命令，命令行形状逐选项取自 C3-C §compile / §link，无额外选项：

- `pycircuit compile -c <one-source.py> --source-root <root> [--package-prefix <p>]
  [-I <unit-dir>]... -o <unit-dir> [--replace]`：薄封装已验收的私有
  `_compile_source_unit`，发布闭合四文件集。
- `pycircuit link <unit-dir>... --top <qualified-module> [--parameters <bindings.json>]
  -o <program.ac> [--replace]`：一套 `_publication_lock_set` 覆盖输入快照、native link 与
  发布；在共享锁下读**完整 body 闭包**（与 compile 只需 managed interface 相对）；helper
  只拿到锁内快照的 scratch 副本，不重开 provider 文件。
- program 的 publication owner **不由命名推断**：私有 `--entry-owner-out` 报告 linked root
  声明的 `ac.source_owner` 与 canonical definition。注意 `ModuleSnapshot.owner` 是
  instance view owner、不是 source owner，权威属性是 root module 的 `ac.source_owner`。

lane：driver 65 项（43 unit + 22 system，独立测试作者编写，8/8 变异被检出）
0 failures/errors；既有 lane 复跑无回归：unit 174/0/0/3、focused system 105、transport 4、
CLI 回归 60、masked-next 15，六条 lane 退出码均为 0。证据
[driver-compile-link](../gates/logs/20260930-driver-compile-link/README.md)。实现期间自查发现并
修掉两个缺陷：符号链接输出路径会抛未捕获的 `_PublicationFileSystemError` 而打印 traceback；
私有 receipt reader 把"文件不存在"误报成编码错误，public 命令改为前置命名真实原因。

**如实声明的缺口**（本包未实现，也不以任何方式假装）：

- **C3 `emit` 未交付**：该子命令名被旧路线占用（旧 `emit` = 设计文件 → `.pyc`），C3-C 要求
  唯一 driver 且不保留 alias，而批准的计划规定旧路线在 M5 一次性 hard break 才退役，因此
  既不提前替换也不改名或加别名。
- **static parameter 特化未实现**：native linker 没有任何 binding 入口，`--parameters` 传
  非空数组即 fail closed；省略与显式空数组等价于"无绑定"。
- **`--replace` 对 program 不校验 owner**：owner 只写进 journal，提交即删除，`program.ac`
  没有 Python 可读的 owner 记录，也没有独立 native verify-only 入口。实测把不同 root
  （`demo.other.Other`）链到已存在的 program 路径会被接受；`compile` 因 receipt 内嵌 owner
  而更严格。这是本包对 C3-C §181 最尖锐的偏离，需 native verify/owner-read 入口才能关闭。
- **per-implementation-source `.hpp/.cpp` 分组与 `generated.json` 未产出**：cpp 后端仍返回
  单体产物；不用拆字符串的方式伪造逐源 codegen。
- **`@system` 根暂不可 link**：私有 `@system ⟹ testbench` 规则仍在生效，删除它属尚未批准的
  C2-SYSTEM 修订 B。

未新增 ODS/IR 或公开 CLI 之外的接口，未改 emitter/runtime/SDK/打包/manifest，旧
`build`/`emit`/`inspect`/`sidecar` 行为未动。本包不代表 M4 完成，也不代表 M1–M7 验收。

### 独立审阅（第一轮）与处置

第一轮独立审阅对 `df3f2283` + `57b1ed43` 判 **FAIL**，性质是**声明与治理，不是实现**：
审阅者独立复现了全部功能断言（Python 不解析 MLIR、helper 只拿锁内快照副本、native 步骤
期间输出锁确实被持有——用 3 秒 sleep 的 wrapper 证明第二个并发 link 阻塞 5.78s、root owner
报告在 unit 目录改名后仍报 `counter.py`、overlay 哈希与 raw-XML lane 计数、C++ diff 仅
102 增 1 改），但认定证据包**少声明了一条真实的 C3-C §143 违规**。

| # | 发现 | 处置 |
| --- | --- | --- |
| D1 | **材料性**：`link --replace` 会覆盖输出路径上已存在的任何非空普通文件，包括驱动从未发布的用户文件，而 §143 明文禁止；原稿只声明了 §181 的 owner 比对缺口。 | **关闭而非仅声明**：目标存在且带 `--replace` 时，要求发布控制目录已存在，否则拒绝且不创建任何东西、文件逐字节保留（`replace-guard.log`）。残留收窄为"已发布路径上的 owner 仍不能比对"，继续声明。 |
| D2 | 低：`compile` 还接受 `--source`/`--interface-unit`/`--output` 三个批准文本之外的拼写，与"no extra option"矛盾。 | 删除三个长别名，公开面现在与 C3-C 逐选项一致，`cli-surface.log` 重新生成。 |
| D3 | 低/文档：批准记录链接指向本分支不存在的路径。 | 改为按路径引用并写明规划分支 commit。 |
| D4 | 细节：`_PublicationError` 是死导入。 | 现由 replace guard 使用。 |
| D5 | 细节：文档串声称发布产物"读取时重新验证"，实际无此入口。 | 已改正。 |

修复落在 `0b35b2a2`（代码+测试）与 `8d56bbb0`（证据）；独立测试作者更新了三处过期断言并
新增一条 replace guard 端到端测试，driver lane 66 项（43 unit + 23 system）全绿，另
3/3 变异被检出。第二轮复验已请求。

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
