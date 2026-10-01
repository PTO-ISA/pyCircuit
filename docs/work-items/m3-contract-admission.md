# M3-P01：扩展能力实施准入核对

状态：done, read-only admission audit。日期：2026-10-01。
依据：[M3/M6 扩展计划](m3-m6-expansion-plan.md)。产品输入 `4584ad0b`；
实际派发时重新记录 HEAD、dirty overlay 和精确批准哈希。
本包不改 Python/IR/CLI/runtime/schema，不开放 profile，不执行产品接口提案。

## 目标与产物

交付当前合同→实现→测试→未闭合 carrier 的逐项对账，用于选择 E01 或 E02。
SYSTEM B、EXPECT B 各自列出归档缺口；不接管其他 agent 的未提交文稿。
实际设计修订由另一个获得明确文件 ownership 的任务起草。

| 候选能力 | 必须回答的问题 | 独立核对来源 |
| --- | --- | --- |
| record/helper | 哪些 construct/default/kwargs/field/projection 已进入 source→IR，哪些尚无 stateful CPP/RTL oracle？ | C1/C2 精确批准；test_source_unit_packet.py；namespace/helper 测试；SourceType/StaticMatching native cases |
| fixed owned/reference reg collection | 具体 list materialization 与参数决定 shape 的 carrier 是否同一批准形状？ | R1/M1 exclusions；RegContractsTest V16；SourceStaticMatching；alias/effect/final reader |
| Bank 非空 static 特化 | source/header/link SpecKey/final/package 的各字段是否已冻结？现有拒绝在哪一层？ | C1/C2 Bank oracle；test_driver_compile_link.py；test_source_module_units.py；source/header registry 与 final helpers |
| 一等 system/EXPECT | B 文本/哈希、独立 reviewer/结论、方向选择与精确批准分别有什么证据？ | 原 owner 的只读提案与归档；current ODS/verifier；历史批准记录；不改正文 |

产物只入 `docs/gates/logs/<run-id>/` 的可审阅表与最终 work-item/review，不新增
installed API/schema。每行至少列 source form、IR/header/receipt 字段、批准来源、
当前实现路径、真实通过/未执行证据、缺口、最小后续包和“能否写产品代码”。

## 文件与角色

- 契约分析：Astra high/xhigh；只读 `docs/rfcs/migration/`、current language/profile、
  compiler import/link/verifiers/emit readers 与相关测试。不得改 ODS 或 pending 提案。
- 独立覆盖核对：另一实例 Astra；检查批准字段与反例，不能由作者自证全部 covered。
- PM：唯一写本包、current status/index 与最终 evidence 表的 owner。
- 对应未来实现/测试：按 E01/E02 选定后分别派 Luna 与独立 Luna；Sol review。

## Checklist / verification

- [ ] 保存批准文件原字节及哈希；以批准记录为准，不按文稿状态行猜授权。
- [ ] current M5 profile 的 scalar/empty-static 与 native/capture 子集分栏。
- [ ] Bank `(0,5)→(9,13)` oracle 保留；旧 class source 只作历史数据，不恢复 API。
- [ ] record 的 general helper/stateful backend 缺口不被 header 成功掩盖。
- [ ] 列出 precise carrier 差异；原合同覆盖者可进入有界实现，缺字段者先提案。
- [ ] 非空 parameters 仍 fail closed、child static 不被擦为 empty key；如做 executable
  probe，工具链必须从本候选重建，不能借旧 prefix 的 PASS。
- [ ] 两份 B 各自审批，原始 owner/独立结论归档；未批准不改 system/EXPECT 产品代码。
- [ ] 独立 reviewer 核对，PM 给出可派发/待提案表，不能把 plan PASS 记为 capability PASS。

本包只读默认不要求 full native/backend closure。Changed-doc lint/API hygiene/strict
MkDocs 是计划检查；确有 executable probe 时另留 command/exit/output 与范围。
完成条件：每个候选能力有上述准入结论，随后只选一个最小实际用例。

## 本轮完成记录 — 2026-10-01

产品提交 `9ff015a2` 完成 M3-P01 准入核对与 M6-03 首次发布扩面。
M3 独立 Astra 核对原批准字节、局部证据和 current代码：record 的既有 C1/C2/R1
语义无需整体重批，但 scalar-only final declaration projection 需精确增补。
下一包 [M3-E01 design/oracle](m3-record-final-projection-design.md) 已独立 planning
readiness通过；不批准产品实现。SYSTEM/EXPECT B 的原审阅归档已齐、精确批准仍无；
Bank carrier不自动准入，他人文稿保持未提交且未修改。

M6：15个新 first-publish SIGKILL 场景和10次 bundle follow-up writer中断恢复
通过；原 replacement/source-first/reentrant/lock断言保留。PM系统5passed；
regression131passed、3Windows-only skips；独立 Sol 实跑5passed。没有产品
协议/接口改动。范围限macOS/POSIX，不证明Windows、networkFS或power-loss。

- [M3完整核对与证据](https://github.com/PTO-ISA/pyCircuit/blob/9ff015a2/docs/gates/logs/20261001-m3-admission/README.md)
- [M6完整测试与证据](https://github.com/PTO-ISA/pyCircuit/blob/9ff015a2/docs/gates/logs/20261001-m6-first-publication/README.md)
- [M3独立核对](https://github.com/PTO-ISA/pyCircuit/blob/9ff015a2/docs/reviews/20261001-m3-contract-admission-review.md)
- [M6独立审阅](https://github.com/PTO-ISA/pyCircuit/blob/9ff015a2/docs/reviews/20261001-m6-first-publication-review.md)

实施 checkout hooks/strictdocs通过。规划 checkout 的12条既有historical
missinglinks保持单独披露，不在此包修复或冒称planningstrictpass。
