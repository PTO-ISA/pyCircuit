# M3-P01：扩展能力实施准入核对

状态：done, bounded read-only admission audit。日期：2026-10-01。
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

- [x] 保存批准文件原字节及哈希；以批准记录为准，不按文稿状态行猜授权。
- [x] current M5 profile 的 scalar/empty-static 与 native/capture 子集分栏。
- [x] Bank `(0,5)→(9,13)` oracle 保留；旧 class source 只作历史数据，不恢复 API。
- [x] record 的 general helper/stateful backend 缺口不被 header 成功掩盖。
- [x] 列出 precise carrier 差异；原合同覆盖者可进入有界实现，缺字段者先提案。
- [x] 非空 parameters 仍 fail closed、child static 不被擦为 empty key；如做 executable
  probe，工具链必须从本候选重建，不能借旧 prefix 的 PASS。
- [x] 两份 B 各自审批，原始 owner/独立结论归档；未批准不改 system/EXPECT 产品代码。
- [x] 独立 reviewer 核对，PM 给出可派发/待提案表，不能把 plan PASS 记为 capability PASS。

本包只读默认不要求 full native/backend closure。Changed-doc lint/API hygiene/strict
MkDocs 是计划检查；确有 executable probe 时另留 command/exit/output 与范围。
完成条件：每个候选能力有上述准入结论，随后只选一个最小实际用例。

## 本轮派发 — 2026-10-01

产品基线 `5b0d610d`，派发前工作树干净；授权来自用户“好的，开始计划”
及本包已有精确范围。PM 唯一写 docs/build/integration。M6 独立测试为
`m5_semantic_oracle_migration` / Luna high，Sol high 实例
`m6_03_independent_review` 只读验证。M3 契约作者为
`m3_contract_admission_author` / Astra high；其产物完成后，另一个 Astra
实例独立核对，不能作者自证准入。

Fresh build/install：`.pycircuit_out/m6-03-root`/`m6-03-install`，本 checkout
自行构建，不借旧 M7 cache/其他 worktree。M3 只读输入与他人未提交提案字节
单独绑定并保护；不将方向选择或 draft/approval-ready 当产品批准。

## 准入结果

独立 Astra APPROVE，最终完整报告 SHA-256
`09b14608ce0bba511a10e4eade220b961c38a9bbce5d561743442379ad33a7c4`。
[审阅](../reviews/20261001-m3-contract-admission-review.md)与
[完整证据](../gates/logs/20261001-m3-admission/README.md)留档。
七份 proposal 原批准字节/哈希和归档 checklist/matrix/M1 已核对；当前文稿不
冒充原批准对象。他人五份提案/审阅输入未改。SYSTEM/EXPECT B 归档已齐，
精确批准仍无；record/helper 和 list 的 local/native evidence 不当 end-to-end PASS。

选择下一包：[E01 final-record 投影 design/oracle](m3-record-final-projection-design.md)。
原 C1/C2/R1 record/helper/state 算法不整体重批，只有 scalar-only C2-DECL 未定义的
final nominal-record/constructor-reference 等缺失边界先精确增补。Bank 非空 static
与参数 shape 不获实现准入；concrete literal collection可另作 R1 有界内部修复，
但本轮未派发。无新 product primitive/API/schema 或 executable probe。
