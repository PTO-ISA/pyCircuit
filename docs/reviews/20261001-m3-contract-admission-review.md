# M3-P01 扩展准入独立核对

日期：2026-10-01。作者：`m3_contract_admission_author` / gpt-6-astra high。
独立 reviewer：`m3_m6_plan_validation` / 另一 gpt-6-astra high 实例。
产品基线 `5b0d610d`；planning 输入为 `6bfcdeed` 加保留的他人提案 overlay。

结论：**APPROVE，限准入事实准确性，不批准新产品接口或当前能力实现。**
最终报告 SHA-256：
`09b14608ce0bba511a10e4eade220b961c38a9bbce5d561743442379ad33a7c4`。
[完整报告与原字节输入](../gates/logs/20261001-m3-admission/README.md)已归档。

独立核对七份原始 proposal 在两个 checkout 的精确批准哈希，及归档的原
checklist/matrix/M1 bytes。当前 checklist/matrix 与原批准版本不相同，报告
没有以新字节替代原批准对象。原提案中的历史 pending 状态与独立批准记录分开。

初版报告 `729fa787...` 被判 REVISE：错误字段 `ac.results` 应为冻结 C2 和当前
importer 的 `ac.return_form`、`ac.result_constraints`。作者只修正该字段清单，
独立 reviewer 重新绑定最终摘要后通过；没有因此修改任何产品 schema。

确认的准入边界：

- record constructor/default/header/field projection 和 concrete literal-list importer/native
  有局部基础，但不能宣称 stateful final/CPP/RTL 已支持。
- C2-DECL 当前 final 投影精确限制为 scalar alias/constant，并保留 record/helper
  preflight。E01 只需对 final nominal-record declaration/constructor-reference 处置
  等缺失边界做增补，已有 C1/C2/R1 record/helper/state 语义不整体重新审批。
- concrete literal reg collection 的闭合 R1 形状可成为另一个有界 implementation-only
  repair；本轮未选中/派发。它不能代替 Bank 参数决定的 shape/carrier。
- C3 已冻结部分 ordered parameter/SpecKey/generated entry/source-group 形状，不是
  “所有静态参数字段都缺批准”；但 joint exclusions 与现有 empty-key guards仍约束 Bank。
- SYSTEM/EXPECT B 原字节与独立 approval-ready 审阅归档已齐，原“缺归档”状态被后续
  记录取代；仍无用户精确批准。归档 claim 不是对当前 HEAD 的产品审阅或实现授权。

下一包选定 [M3-E01 design/oracle](../work-items/m3-record-final-projection-design.md)，
不写产品实现，不引入 static/typed DUT/SYSTEM 依赖。该派发包另获独立 Astra
planning readiness APPROVE，SHA-256
`dc7c0d66735455e4ecb8e72549c279b2c1173752b527c712a1a779be637bb280`。
后续精确提案仍须独立审阅和用户批准；本次没有 executable compiler probe 或 capability PASS。
