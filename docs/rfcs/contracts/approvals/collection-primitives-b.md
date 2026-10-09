# Collection/Table 原语修订 B 批准记录

状态：approved。日期：2026-10-02。批准者：本项目用户。

精确对象：[Collection 原语提案 B](../../../work-items/collection-primitives-proposal.md)，
SHA-256：`db6b1004fe9058b88f63c663463a236518ecdc8e1d7fa1017f492907d8e67db9`。
[独立设计审阅](../../../reviews/20261002-collection-primitives-review.md)为 approval-ready；
architect / gpt-6-astra / xhigh 对同一内容完成架构符合性核对。

用户在收到该精确提案及原语/行为摘要后明确回复：

> 批准 开始实现吧

授权按修订 B 实现、独立测试、修复和集成：共同 `!ac.table` 值类型、
`ac.collection` 实例族、Table 构造/广播/map/view/index/get/match/choose/fold、
静态不重叠 `ac.value.merge`、既有字段/索引/选择/归约规则复用、bulk runtime 与
cpp/verilog 从同一 verified IR 生成模块源码和构建入口。类型、shape、四态、
索引哨兵、状态 identity、shared ownership、Work/Xfer 与失败清除以提案原文为准。

保持提案列出的首包范围；不借本批准新增 Python 作者语法、runner/DUT C ABI、
动态 shape、异构集合、约束布局、隐式流控或 field-enable DFFE，也不恢复旧路线。
历史批准文档不改写；本记录是此前 collection 排除范围的本次具体增补。

批准不证明实现或 gate 通过。提案原文及其审阅 hash 保持不变，实际实施和证据
另记到工作包与本轮 gate 目录。
