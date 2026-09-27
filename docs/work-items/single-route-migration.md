# 单一路线迁移执行账本

状态：active。开始：2026-09-27。PM：本会话。完整目标是按[修订 6 计划](../development/pycircuit-modernization-plan.md)使 pyCircuit 收敛为成熟、可验证的 GFSIM Pythonic → MLIR → 统一硬件 IR → C++/Verilog 框架；本账本不把目标缩成治理或样板。

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
| I05 U01 Packet source/header | active | 隔离 checkout 85cb2a06，分支 codex/gfsim-source-units；governance_impl 原生实现（Sol medium），source_transport 私有 transport（Luna high），baseline_verification 独立测试（Sol medium） | transport 已独立 Sol high PASS 并集成 8b39f70a，[主 checkout 266 unit/4 parser 通过](../gates/logs/20260927-u01-transport/integration/results.md)。原生 body/interface 与 header-only constructor/default/kwargs/get 仍在实施测试；不代表公共 driver、link 或双后端完成 |
| 用户接口批准 | partial | 用户 | C1-C、C2-C、C3-C 均已批准；其范围外的硬件扩展仍须精确批准 |

所有 writer 使用互斥文件归属。U01 的 ODS/原生 importer/非安装 harness 与产品 CMake 由 governance_impl 负责，测试及测试 CMake 由 baseline_verification 负责；private transport 单独派发。实现期间 native build 由 governance_impl 操作，稳定后移交测试 owner，其他 lane 不用同一输出目录构建。PM 维护主 checkout 文档，不改 candidate 产品源码。

## 全项目里程碑

| 阶段 | 状态 | 完成证据要求 |
| --- | --- | --- |
| M0 合同、基线与准入 | active | B01–B03 及精确 C1/C2/C3 用户批准，能力无未分类行 |
| M1 治理和 donor 准入 | active | 可执行治理/skills、试运行、独立审查；DeepSeek 可选接口另批准 |
| M2 单主干双后端最小闭环 | pending | 新 capture/MLIR/source-unit/root/Cpp/RTL 同一 final IR 执行；不能由旧链兜底 |
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

优先复用 donor PythonLower 的 constructor 分类、PythonRuleAnalysis 的 effects、PythonLowerRules 的注册/参数绑定、PythonLowerEmit 的 child endpoint 绑定和 PythonLowerInitial 的初始化递归。child 的源成员查询替换为已验证 header contract。record 初始化执行 header-owned helper，不把 constructor 参数顺序当成字段布局。donor acir-link 的符号/递归算法可用，其直接合并所有声明的接纳逻辑不符合目标 authority/snapshot 合同。

U02 必须为 fixture 中的数学加法和 mask 建立已批准 source-math IR；不能替换成旧 i8 回绕算术。它保存每个源写入/返回的 origin、value、path、target 关系，U03 才完成区间/位宽、range/error、ValueID/use/target witness 和 final verifier。不能从合并后的 yield 反猜源目标，也不能新增临时公开 assignment schema。

普通 DFFE 不引入 Queue admission。Accumulator 的 result 每个成功路径都返回；total 仅在旧 item.valid 为真时写入。Core advance 读旧 left_request、写完整新 Request；right_request 无 writer，保持旧值。两后端后续仍须执行 Reset=(7,19)、Work 后=(7,19)、三次 Xfer=(1,2)/(1,4)/(2,6) 及 reset/rerun oracle；U02 linked-semantic 产物不是这些执行证据。

独立正反例覆盖：隐藏 child source/body、仅 header 默认值与构造器调用、端口改名不改变 R/W、未注册方法无 active effect、序列化重读后绑定不变、缺 header、错误 nominal/range/actual、重复 authority、篡改 snapshot/signature/effects、结构 alias 重绑、跨 child 内部 state 访问，以及错误合并两个实例 total。仍含 math_int/helper/未闭合义务的产物不发布为 final program，不调用 backend。

该 scalar/record fixture 不需要新的用户接口批准；资源、memory/CDC/四态等扩展继续按能力矩阵补齐合同与审批。
