# C2-M1 设计记录与待审状态

本页保留修订 A 的历史状态。当前 M1 C/checklist/matrix 已另获
[独立 approval-ready](20260928-interface-handoff.md)；旧线程资源限制
不再作为当前候选的审阅状态。

日期：2026-09-28。
候选：[C2-M1 修订 A 冻结文本](../gates/logs/20260928-c2-m1-design/revision-a.txt)。
最终候选 SHA-256：
`7b7aec52144f2f44b0f6c410213faab4454b461c2efd7a6bb3c78bf282478f43`。

## 作者与来源

module/link/system 建议由 `migration_architecture_review`
（gpt-6-astra / xhigh）提供；lexical Python、观察和测试例子由
`lexical_frontend_design`（另一个 gpt-6-astra / xhigh）提供。
PM 转录为精确联合候选。用户已明确选择模块函数＋嵌套 rule。

donor 事实调查及 API cross-check 由 `migration_implementation_review`
（gpt-5.6-sol / high）完成。该角色此前参与 donor 研究，不能被记作
本联合架构的独立设计批准。

## 独立设计审阅不可用

初稿摘要为
`644e0fa9ba4ef5e895618782c5cf01f8f4adf3737adc239ffa1a6d505ca45eef`。
向旧独立 reviewer 的 follow-up 和新 architect 派发均返回
`agent thread limit reached`；旧 reviewer 不在 live agent 列表。

因此本记录**不给出 approval-ready 或设计通过结论**。R1 B 的旧
独立审阅不覆盖 M1 的实例/system、source、observation 和新 lifecycle。
作者/PM 校对、事实核对和文档验证均不能代替独立 Astra 审阅。

## Donor/API 事实核对及修正

Sol cross-check 读取的候选摘要为
`9622cd845c5bdea4250cef4fca8a9293b6cb5c28521142d39a3b9f7c6426b101`。
它发现两项事实/验收遗漏，PM 作如下修正：

| 问题 | 修正 |
| --- | --- |
| 把 donor SimSystem 的 ReportStat 写成 Report | 使用 Build/Step/Reset/ReportStat；明确 Work/Xfer 属 module，Print 是生成 module 的本地方法 |
| flat traversal 的旧递归删除/验证不完整 | 列全 Build/Work/Xfer/Reset/ReportChildren、child Evaluate/Check/Drive，新增 NoCallChildren 静态门槛；Freeze 仅在 flat Build 后从 root 递归一次 |

Sol 对后续摘要 `58c4d66fe1511ffac34a8bae74c96ed31d54547c573aa67b5d80acb52d7b3626`
复核，确认生命周期/Freeze 删除责任关闭，并找到测试清单的一处
Report 残留。PM 在最终摘要中将该处同样改为 ReportStat；没有改动
源例或结构合同，不把这次文字修正记作独立架构审阅。

同时固定 TestIncrement/TestPipeline 的物理 reg 数为 4/5。fact lane
核对五周期期望分别为 0,2,5,5,5 与 0,0,3,6,6；source gauges 被明确
描述为新增 system observation adapter，没有伪称 donor Reporter
已经支持 JSON gauge。

## 已完成的有限验证

四个 Python 示例通过 AST parse 与语法/词法 compile 检查，未执行
模型或 imports。独立普通 Python 状态方程核对单模块/组合两例、
正反 rule 次序，结果与提案五周期表和完成 epoch=5 相同。该脚本
不调用 pyCircuit compiler、donor runtime 或 emitter。

[例子验证结果](../gates/logs/20260928-c2-m1-design/example-validation.json)
保存候选摘要、四种 oracle 结果及未运行产品 gate 的标记。

下一步独立 reviewer 必须读取最终候选，检查新 observation carrier
的来源/路径、source gauges、system 入口、reg identity、flat lifecycle
和 runner 精确接口。未通过独立审阅前不提交精确实现批准请求，
不修改产品语义。
