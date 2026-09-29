# C2-R1 统一寄存器独立设计审阅

日期：2026-09-28。主仓基线：`7e5ffdc22416e8e63bf66cbee9dfa049a7614c41`。
对象：[统一寄存器提案](../rfcs/migration/c2-r1-unified-register.md)。

架构建议作者为 `migration_architecture_review`（gpt-6-astra，xhigh），
PM 转录并整合精确合同。独立 reviewer 为 `unified_reg_design_review`
（gpt-6-astra，xhigh）；该实例未撰写提案或产品实现，未修改文件。
本记录由 PM 根据实际审阅返回转录。

## 修订 A

候选 SHA-256：
`1bbb05f5382649446328f1e224721884cefec779309834dbd9575913b777f5c2`。
[冻结内容](../gates/logs/20260928-c2-r1-review/revision-a.txt)。
结论：**revise**。

| 严重度 | 问题 | 修订 B 的对应修改 |
| --- | --- | --- |
| P1 | source 提前展开 collection 后，属性/element/完整 initializer 的权威及求值次数不精确 | 增加 source/linked/final 必需/禁止属性表，按 declaration 分组核验完整元素，initializer 每特化恰求值一次；补 [3,7,11] 与篡改反例 |
| P1 | clk/reset 前缀及 reg-only 数据连接未说明全树本轮 fatal 如何贯穿逐源 RTL hierarchy | 明确无状态 subtree_error 上行/root_commit_ok 下行的 RTL-private 投影、reset 覆盖、未门控 E 计算检查、完整端点核验及孙模块故障 oracle |
| P2 | R1 有界门槛和 M5/FIFO 删除、全量决定验收混列 | 增加路径/注册入口、替代证据/删除条件表，区分合法 SimDFFE/pyc.reg，分离 R1 与 M5/release gates |

reviewer 认可无输出反压、原语内 discard、单 owner、独立 proposal
buffers、reset transfer、有限故障验收及后续 FIFO/动态集合边界。
不因后续能力未实现而扩大本包，也不将有界验收称为完整迁移通过。

## 修订 B

最终候选 SHA-256：
`76283661bffba96aad544e31d2b4996f019135f448c4c8b685ebb02eab8edfba`。
[冻结内容](../gates/logs/20260928-c2-r1-review/revision-b.txt)。
结论：**approval-ready**，无剩余设计阻断。

初次 B 送审摘要为
`f52ec6584b1c85d6b0cca843445f93af10ee346493e5b2339d55a4548af6089e`。
reviewer 确认三项实质修补关闭后，要求将不存在的“C3 保留控制命名
空间”引用改为实际 C3 名称 legalization/collision 规则，且明确角色
由投影绑定识别。最终候选仅作此表述修正，reviewer 已重新读取并
绑定上面的最终摘要。

最终复审确认：逐阶段 source/linked/final 属性、完整元素集合、
initializer 单次求值与非均匀初值反例闭合；无状态的层级 fatal 投影
没有 error/commit 组合自依赖或额外周期；R1 与 M5 删除/验收范围
清楚，合法 SimDFFE 与 RTL-private pyc.reg 未被误列为删除对象。
计划、账本和能力表仍准确区分方向、精确批准、实现及验收。

此结论允许提交该精确修订的接口批准请求，不是接口批准、代码验收
或产品 gate 通过。FIFO 库、参数化/动态集合及完整 fault continuation
依赖保持开放；修改本提案实质内容需再次独立审阅。

## 证据范围

审阅依据包含冻结 C1/C2/C3、A2/L1 来源义务、donor GFSIM
`b852ed83fa0288d0be7406bba0ed47be4b2c0f63` 的实际 DFF/DFFE，
以及现有 RTL-private pyc.reg。审阅是精确设计验证，不是产品构建或
新 R1 测试通过。实质改变候选内容会使对应摘要的结论失效。

用户给定的 ac.reg/无状态 proposal/Xfer/SimQueue 退役方向已纳入；
独立 approval-ready 不代替新增精确接口的用户批准。
