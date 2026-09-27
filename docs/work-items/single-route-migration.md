# 单一路线迁移执行账本

状态：active。开始：2026-09-27。PM：本会话。完整目标是按[修订 6 计划](../development/pycircuit-modernization-plan.md)使 pyCircuit 收敛为成熟、可验证的 GFSIM Pythonic → MLIR → 统一硬件 IR → C++/Verilog 框架；本账本不把目标缩成治理或样板。

## 授权与边界

用户已明确授权按计划执行迁移，并指定本会话为 PM。单一路线与 hard break 方向已给定。用户此前要求任何接口变动先请其 approve，此要求继续有效；计划审阅不是 C1/C2/C3 的精确接口批准。

初始 source HEAD 为 `8887e6dec7b4cc530a9967c860dc6a224d79a4ab`；[来源清单](../gates/logs/20260927-migration-intake/source-inventory.json)记录 donor、dirty 状态和关键摘要。既有 `examples/davo/` 和 `.omx-state-locks*` 不属于本任务的清理范围。历史 OMX ultragoal 仍有其他 consumer 工作，本次不覆盖；原生 Goal 保持整体目标，任务状态在本账本维护。

## 当前工作包

| ID | 状态 | owner / 文件归属 | 验收与证据 |
| --- | --- | --- | --- |
| B01 来源与组件清单 | done | PM；`docs/gates/logs/20260927-migration-intake/source-inventory.json` | 两仓 fresh HEAD/status，7 组件路径、donor 批准内容摘要、来源/许可事实；独立 governance reviewer 已核对 donor HEAD 与九个内容绑定 |
| B02 能力/退役矩阵 | review | PM；`migration-capabilities.md` | 现行能力、target disposition、批准包、oracle/gate、退役资产完整关联 |
| B03 当前版本基线 | verified | `baseline_verification`，test-engineer，Sol medium；专属 baseline 输出与 gate evidence 目录 | 当前源码自行构建，Python G0 和最窄 native/双后端证据；不采用旧绿灯 |
| G01 治理与 skills 落地 | done | `governance_impl`，executor，Sol medium；其派发中列出的治理/skills/导航文件 | lint/docs/skill validation + 独立审查；不改产品合同 |
| D01 C1 设计 | done | `interface_design`，Architect，Astra xhigh，只读设计建议；PM 写精确提案 | [C1 修订 C](../rfcs/migration/c1-pythonic-source.md) C 版已独立 Astra approval-ready；[审阅证据](../gates/logs/20260927-c1-source-review/revision-c-review.md)，[用户已批准该精确修订](../rfcs/migration/approvals/c1-pythonic-source.md)；精确 IR/SDK 另见后续 D02/D03 |
| D02 C2 IR 精确提案 | done | interface_design（Astra xhigh）设计补齐，PM 整理 | [C2 修订 C](../rfcs/migration/c2-mlir-contract.md) 已关闭全部独立审阅问题，[Astra xhigh approval-ready](../gates/logs/20260927-c2-review/revision-c-review.md)；[用户已批准](../rfcs/migration/approvals/c2-c3-foundation.md) |
| D03 SDK/driver/runtime | done | PM 起草，interface_design（Astra xhigh）只读补齐设计，PM 转录 | [C3 修订 C](../rfcs/migration/c3-driver-runtime.md) 已关闭全部独立审阅问题，[Astra xhigh approval-ready](../gates/logs/20260927-c3-review/revision-c-review.md)；[用户已批准](../rfcs/migration/approvals/c2-c3-foundation.md) |
| I01 私有单文件源码捕获 | done | 隔离 checkout；governance_impl 实现（Sol medium），baseline_verification 独立测试（Sol medium） | 36 focused / 253 unit 通过、独立 Sol high code-review PASS，集成 `30e4f709`；[证据](../gates/logs/20260927-c1-capture/review.md)。不接 C2/C3/公开入口；Luna 派发受 thread limit 阻断，实际使用 Sol |
| 用户接口批准 | partial | 用户 | C1-C、C2-C、C3-C 均已批准；其范围外的硬件扩展仍须精确批准 |

所有 writer 共享 checkout 且有互斥文件归属；ODS/CMake/product source 此刻未派发写入。native build 由 baseline owner 统一操作，其他 lane 不用同一输出目录构建。

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

C2-F01 由 governance_impl（Sol medium）只读分解，C2-C 已获批准，当前为 ready。复用 `compiler/acir` 内唯一 `ac` dialect，先实现任意精度 MathInt 属性、临时 math_int 类型和 SourceSpan/PathComponent/Site 的闭合记录验证，独立正反例。DictionaryAttr 记录按 C2 原形式验证，不借 ODS 为其另造未批准的公开语法。Occurrence 依赖 StaticValue，后续按类型值→Occurrence→source-unit→header/schema cutover 顺序推进。

首批实现写入 ACIRAttributes.td、ACIRTypes.td、独立 ACIRSourceContracts.cpp 及其 target；测试由独立 owner 放在 `tests/mlir/agentic-circuit/ACIR/`。使用该候选自己配置的 LLVM22 build，构建 ACIRDialect/acir-opt-internal，再运行精确 lit filter。不得复制旧 build binaries。

当前 target SourceOwner 是 implementation/declaration schema，donor Python importer 仍 whole-project capture，且两仓 C++ namespace 不同。因此不能 bulk-copy importer，也不能用新基础类型已通过声称 source-unit/双后端闭环完成。公开 driver、捕获到 native 的通道、发布、生成/SDK/runtime wiring 已获 C3-C 批准，按各自工作包依赖顺序实施。
