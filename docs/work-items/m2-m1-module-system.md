# M2-M1：模块实例、System 与 lexical Python

日期：2026-09-28。状态：approved，W00–W03 active；产品实现未验收。

最新用户授权吸收 Bluespec interface/effects 思想，要求 Python 纯净且
没有 Queue 概念，由 MLIR 自动推导，并准备有界 agent 的 checklist
和 verification。已更新 [M1 C](../rfcs/migration/c2-m1-module-system.md)、
[W00–W12](../development/migration-agent-checklist.md)和
[V00–V49](../development/migration-verification-matrix.md)。新派发受到
原生 thread limit 限制，现复用未参与精确提案写作的 `bluespec_research`
实例完成独立审阅，实际模型为 gpt-6-astra/high；三项发现修复后已
approval-ready，见[精确摘要与处置](../reviews/20260928-interface-handoff.md)。
下文 A 的失败记录保留历史，不冒充当前 C 的结论。

用户已批准[联合接口与执行包](../rfcs/migration/approvals/c2-r1-m1-interface.md)。
PM 现按 checklist 派发；approval-ready/approved 仍不代表任何 Vxx
产品门槛已通过。

最终交付的编号/关联、四个 Python 示例语法、pre-commit、API hygiene
和 strict MkDocs 均已验证，见[文档验证](../gates/logs/20260928-interface-handoff/documentation-validation.md)。
Wxx 产品复选项仍未完成；Vxx 是后续独立实现测试责任，不是当前通过记录。

## 请求与交付

用户要求 ACIR link 完成 module parent/children 实例树、任意有限
input/output reg、inner reg 与零存储连接，避免 double reg；module
具备 assert/print-log/report，system 可独测/组测并驱动 Work/Xfer。
用户通过结构化回复明确选择“模块函数＋嵌套 rule”，模块不再需要 self。

交付为 [C2-M1 联合提案](../rfcs/migration/c2-m1-module-system.md)，包括
新的源语法、InstanceView、ac.system、system-flat lifecycle、观察
carrier、标准 runner、完整 single/composite system 示例与五周期
oracle。它联合重基 R1 B，而不是另一条产品路线。

## 候选、角色与写入边界

主仓 `/Users/zhoubot/linx-isa/tools/pyCircuit` 的 HEAD 为
`7e5ffdc22416e8e63bf66cbee9dfa049a7614c41`。本轮开始时已有本会话
R1 文档/导航修改及 `.omx-state-locks*`、`examples/davo/`；全部保留。
隔离产品候选和 donor 均不修改。Donor 依据仍为 GFSIM b852ed83 的
SimModule/SimSystem/Logger/Reporter/SimDFF 及生成器实际行为。

| 责任 | 实际实例/配置 |
| --- | --- |
| Module/link/system 架构建议 | migration_architecture_review，gpt-6-astra / xhigh |
| Lexical source/观察/system fixture 设计建议 | lexical_frontend_design，gpt-6-astra / xhigh |
| Donor/API 事实调查与交叉核对 | migration_implementation_review，gpt-5.6-sol / high；不作为独立设计批准 |
| 文档转录、整合、验证 | 本会话 PM；仅本包、相关计划/能力/账本/导航及证据 |
| 独立 Astra 设计验证 | 未成功启动；原生 agent thread limit，见下文 |

作者、研究者和 PM 不能代替缺失的独立设计验证。没有为此创建新的
用户 chat、修改 OMX 状态、启动替代运行时或伪称 R1 旧审阅覆盖 M1。

## 修订 A 的历史审阅状态

对初稿进行了两种原生派发尝试：向旧独立 reviewer 发送 follow-up，
以及创建新的独立 architect；均返回 `agent thread limit reached`。
旧 reviewer 不在当前 live agent 列表。因此本包仍是 draft，不给出
approval-ready、不请求按未审阅接口实施，也不以作者自审替代。

可继续完成的部分是 donor 事实核对、Python 示例语法检查、独立
逐拍 oracle 核算、文档规范与链接验证。它们不是新 compiler/runtime
或双后端 gate 的通过证据。工具资源允许后，需要一个未参与本包
作者工作的 Astra 实例，对最终 SHA 进行独立技术审阅。

修订 A 最终设计候选 SHA-256 为
`7b7aec52144f2f44b0f6c410213faab4454b461c2efd7a6bb3c78bf282478f43`。
[设计/事实核对记录](../reviews/20260928-c2-m1-module-system.md)保留
派发失败及 donor API 两项修正；[文档与例子验证](../gates/logs/20260928-c2-m1-design/documentation-validation.md)
记录 4 个语法例子、4 组独立状态方程、pre-commit/API hygiene/strict
MkDocs 的通过结果。没有产品实现或双后端通过声明。

## 依赖与下一执行边界

- R1 B 的 ac.reg/commit/discard 保留；旧 Drive 阶段由 M1 改为 Xfer
  内 staging，system 独占遍历，递归 Work/Xfer 不与 flat 模式共存。
- C1-C class/self、rule-return binding 需按 M1 精确增补替代，旧冻结
  文本不改字节；MLIR owner/type/provenance 不因源形式变化而丢失。
- 待批 ac.dut 的 root invocation/direct-current 与新 system/reg 方向
  冲突，需要重基；closed-system fixture 不依赖它。
- source report 新增 gauges，但保持 C3 每行统计 schema；观测不反馈
  为 source state，失败普通 events 不发布。
- FIFO 库、动态/参数化集合、外部 ABI、多域/四态和开放 RTL fault
  continuation 均保留单独明确的验收义务。

建议实施顺序为 lexical source/逐源接口 → instance/connection link →
system lifecycle → observation lowering → 两个完整 system 双后端门槛。
精确接口完成独立审阅与批准后才派发 implementation、独立 tests 和
Sol code review；每个 writer 绑定互斥文件与候选 build 目录。

本轮不修改产品源码，不关闭 M2/M5，不创建 tag 或发布。回退以完整
隔离候选为单位，不增加 class/function 或 recursive/flat 兼容 mode。
