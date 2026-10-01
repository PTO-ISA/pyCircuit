# M3/M6 扩展计划独立审阅

日期：2026-10-01。作者：PM；事实调查：`m5_semantic_oracle_migration` 与
`m6_incremental_measurement`，两个 gpt-6-luna/high 独立只读实例。
独立审阅：`m3_m6_plan_validation`，gpt-6-astra/high；没有改写被审阅文档。
产品基线 `4584ad0b`，规划基线 `be68c6b3`。

结论：**APPROVE，限计划/准入调查/现有合同测试准备，不批准产品接口。**

| 文档 | 最终 SHA-256 |
| --- | --- |
| m3-m6-expansion-plan.md | f0a0639cf5f695d007fcd18a7b8c8bfcddb5426bdfdf11c2f0855dbf07dcccec |
| m3-contract-admission.md | 827d8e9db4f811334d065e62af82c02476653f485da48ddd2a5e5936d7b4593c |
| m6-first-publication-expansion.md | 70f52aff8ac3f414c486814d3c97b31a56e852bdd936dc2d56dd78e821ff5e25 |

初稿规划结论通过，最终内容多次按澄清/事实补齐重新绑定，未用旧摘要覆盖新字节。
最终三份文档没有待解决 finding。

审阅确认：

- 已验收 M5、M6-01/02、M7-01 不重排为未完成主干；当前 scalar/empty-static/
  serial profile 与完整能力 backlog 分开。
- C1 提及 static/collection 不代表未冻结 carrier 已获批准；R1/M1 的明确 exclusions
  保留，SYSTEM/EXPECT B 各自未批准。Pythonic/capture-only、MLIR 语义权威、
  common final 双后端、逐源 producer/TU、无公开 Queue/interface/scheduler DSL 不变。
- M3-P01 可派发只读调查；E01/E02 在精确 source/header/final carrier 核对后再决定准入。
  record header/native list 基础与 Bank capture-only/backend UNRUN 证据分列。
- Bank 原 `(0,5)→(9,13)` oracle 保留，非均匀 reset 是补充；`entries=257` 的
  invalid Word 值 256 不被误写为全局长度限制。
- M6-03 可派发现有 C3 tests-only：三 artifact × 五可达点为 15 场景；commit 前
  absence、commit 后合法新 bytes；after_previous_saved 对 first-publish 不可达。
  writer 可能重发布，必须在已有 test-only checkpoint先观察 rollback 后 absence。
- 测量准备可以并行，正式比较需要资源/竞争说明；无 RSS/性能阈值或并行能力的假声明。
  选择性发布/TU 与调度在实现前核对合同；fixed-source scheduling 和 source-reorder
  使用两个不同 oracle，不拿旧 AST identity 当相同源码。
- 公共表的 checklist 允许明确 N/A＋理由，避免文档/测量包重跑全闭环；四 slot、
  单 writer、独立 author/test/review、候选冻结与实机平台证据明确。

实际派发仍须记录最新 HEAD/dirty、批准哈希、run-id、具体允许文件和实际模型。
发现产品缺陷只能按独立 repair ownership 修补；本次 review 不直接授权产品代码。
没有执行新能力 gate、没有新增 IR primitive/CLI/schema/ABI 或发布版本。

[规划证据](../gates/logs/20261001-m3-m6-plan/README.md)留存事实笔记与最终文档绑定。
