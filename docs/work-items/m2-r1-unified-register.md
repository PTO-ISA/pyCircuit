# M2-R1：统一 reg 与无状态 proposal

日期：2026-09-28。状态：approved，implementation active。

后续设计输入已形成 [M1 联合增补](m2-m1-module-system.md)。上面的
approval-ready 仅指冻结 R1 B 字节；M1 的 lexical source、system-flat
遍历和 Xfer 内 staging 需要联合重基审阅，不能直接沿旧阶段分工实施。

## 目标与用户方向

按用户明确要求，把 GFSIM DFF/DFFE 与 RTL 寄存器统一为 `ac.reg`，
以 reg/collection current 为 rule 输入，以 next proposal 为输出；
rule 无 persistent state，只有 Xfer 修改 Q，enable=false 的候选在
原语内 discard。模块数据只通过 reg 连接，SimQueue 退役，未来
ac.queue 降到 reg circular-buffer 模块库。

[精确提案](../rfcs/migration/c2-r1-unified-register.md)说明 signature、
controls、reset、owner、冲突、并行与验收。用户方向无需重复确认；
新增精确接口通过独立审阅后才提交接口批准，不把设计交付记为实现。

## 基线与归属

| 项目 | 内容 |
| --- | --- |
| 主仓 | `/Users/zhoubot/linx-isa/tools/pyCircuit`，HEAD `7e5ffdc22416e8e63bf66cbee9dfa049a7614c41` |
| 隔离候选 | `/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`，HEAD `82f161ea95336fdd15c238e103f13b9f823e8f28` |
| 既有 dirty | 主仓 `.omx-state-locks*`、`examples/davo/`；候选 4 个修改文件与 3 个新增 source-link 文件，均不改动 |
| Donor | GFSIM `b852ed83fa0288d0be7406bba0ed47be4b2c0f63`；仅只读检查并刷新远端 refs，未修改 checkout 内容 |
| 设计写入 owner | PM，仅本提案/工作包、计划/能力/账本和导航；独立 review 结果由 PM 转录 |
| 禁止写入 | 所有产品源码、donor、隔离实现、冻结 C1/C2/C3/N1 批准文本与 OMX runtime 状态 |

候选的 common Git `core.worktree` 当前误指主仓；本次只读核验显式
使用 `git --work-tree=<candidate>`。修复工作树定位是后续实现派发前的
候选卫生任务，本设计不修改共用 Git 配置。

## 独立角色

| 责任 | 实际实例与模型 |
| --- | --- |
| 架构作者建议 | migration_architecture_review；gpt-6-astra，xhigh；只读建议 |
| Donor/API/原语落点调查 | migration_implementation_review；gpt-5.6-sol，high；只读调查 |
| 转录与集成 | 当前 PM；本会话模型；不把角色名当模型证据 |
| 独立设计验证与测试策略 | unified_reg_design_review；gpt-6-astra，xhigh；不撰写提案或实现 |
| 实现／独立测试／代码审阅 | 未派发；按治理分别使用不同实例，精确合同批准后再绑定文件 |

## 合同与依赖

- C1/C2/C3/N1 已批准文本保持冻结；R1 以显式增补替代具体 storage/rule
  与 runtime 条款，产品切换时更新决定和规范。
- A2/L1 待批提案需基于 reg/targets/results 重基，不能借 rename 删除
  原始 read/use、math proof 或 owner 义务。
- 首片支持 scalar 与具体固定集合/静态索引；参数化 collection 和
  动态索引保留为已批准、未闭合的能力，不假称实现。
- FIFO 库、完整 RTL source-fault continuation、typed external DUT、
  多域/四态等不是本设计的已验证产物；各自保持明确依赖。
- Decisions 0236/0237、0238–0241、0262–0264、0270、0274–0278、0280
  的受影响条款按提案表逐项处置；只在实际 M5 候选写入 supersession。

## 实施分解与退出条件

R1-IR → R1-RUNTIME/R1-RULE → R1-BACKENDS → R1-CUTOVER，具体正反例
和拟新增 target/命令见提案末节。独立测试必须从逐拍数学 oracle 推导，
实现者不能用自己的 primitive 计算预期。每个实现包先冻结 checkout、
source SHA/overlay、互斥文件和 build 输出，再派发。

本轮设计退出条件为：精确提案、独立候选摘要绑定的审阅、问题修订与
文档 gate；不要求尚未批准接口的代码实现。产品退出仍要求同 final IR
C++/RTL 执行、source→AC→TU 图、安装资产删除、完整语义责任与原子
hard break，不以文档通过关闭 M2。

## 当前证据与风险

独立修订 A 审阅为 revise；三项问题关闭后的修订 B 已获独立
approval-ready，SHA-256 为
`76283661bffba96aad544e31d2b4996f019135f448c4c8b685ebb02eab8edfba`；见
[精确审阅记录](../reviews/20260928-c2-r1-unified-register.md)。用户已批准
[R1/M1 联合实施包](../rfcs/migration/approvals/c2-r1-m1-interface.md)。
changed-file pre-commit、API hygiene 和严格 MkDocs 已通过，见
[设计文档验证](../gates/logs/20260928-c2-r1-review/documentation-validation.md)。
产品 gate 未运行；提案中的 R1 测试文件/targets 是
待实现门槛。donor 的 DFFE commit/discard 是可复用代码证据，不证明
新 control/schema、模块连接、并行和 backend 合同已经实现。

风险重点：source/final rule arity 与 controls 的一次性迁移、多个
proposal 合并后的唯一 Write、reset 通过 Xfer、false proposal 不重放、
collection 边界、以及 SimQueue 退役时保留 FIFO 行为验收。

回退以完整隔离候选为单位，不建立 ac.dffe/SimQueue 兼容开关。
