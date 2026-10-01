# C2-DECL-R record final 设计审阅与批准

日期：2026-10-01。设计作者 `m3_contract_admission_author` / Astra high；
独立设计验证 `m3_m6_plan_validation` / 另一 Astra high；独立 oracle 作者
`m3_e01_oracle` / Luna high。产品基线 `9ff015a2`。

最终精确 proposal 修订 B SHA-256：
`ea242da0d85de4f51c439051c80c2e7ce12c17dca5ef9a63b8743f0f280a0043`。
结论 **approval-ready，随后用户明确批准该修订 B**，见
[精确批准记录](../rfcs/migration/approvals/c2-decl-record-final.md)。

初版 A 摘要 `28f6d168...` 判 revise：helper return 不能把使用路径等同于旧值
binding 的 path；guarded record selector 与 E=false synthetic zero 未完整冻结。
B 绑定返回 value/valid 与独立 threaded exit/caller continuation；采用共同 final
中两个 get、两个同一 E 的 scalar select、一个 aggregate create，明确 synthetic
值的类型/出处/用途闭合。没有 residual scf/index，多个 record contribution/writer
仍 capability-reject；没有 default priority 或 backend 自造选择行为。

其余闭合已独立核对：canonical constructor 仅 provenance、source owner/origin/
nominal/field order/reset、value-operand 限定、新 required-record 来源与 actual SSA
关系、whole-record唯一 StateID/D/E/no-fail Xfer、统一derived scalar-leaf view、CPP
value aggregate 与 RTL packed layout/模型 ABI 分离，以及逐源生成/输出保护。
没有新增 op/type/runtime方法/PythonDSL/CLI/receipt/source-map字段。

独立 oracle 已按 B 对齐并获 reviewer 闭合确认：

- MD `ce0acc1f0a8a6be9d8b76a877fa0e607f8d031efb95837742683a29393ae0d97`。
- JSON `b9b6b4069312fb317aed2b9cca4c81d087150bb668bdef91f3be816b14c412c6`。

A06限定 local value aliases、record端口负例；A14断言失败/return原值路径；
A19/A20 guarded零与selector/provenance篡改。使用当前compile/link/emit，明确
producer/frozen-snapshot versus standalone-final内在自洽边界，不认证历史源码。

[完整设计/审阅/oracle证据](../gates/logs/20261001-m3-record-design/README.md)留档。
本页不宣称record代码/测试/backend已完成；批准后按有界实施包、独立测试和
Sol review收敛。SYSTEM/EXPECT B、Bank、record ports、multi-writer等未获此批准。
批准提案原字节不为Markdown样式修改；exact-path style例外不豁免实际IR验证。
