# pyCircuit 单一路线规划审阅记录

日期：2026-09-27。候选：规划修订 6。结论：**独立 Architect 与 Critic 均批准规划，无阻断性发现。**

本记录描述执行启动前的修订 6 字节，由 PM 根据独立 subagent 的实际返回落盘。执行阶段已将治理提案转为正式治理，并做标题、用语与活动链接校准；下表不声称这些当前文件仍等于历史摘要。当前治理集成另由独立代码审查验收。它只证明单一路线迁移计划经过审阅，不是用户对 C1/C2/C3 精确接口的批准，也不是源码删除、产品测试或发布通过证明。本轮仅修改规划文档。

## 已批准的修订 6 规划基线（历史内容）

| 文件 | SHA-256 |
| --- | --- |
| [已审阅主计划](../gates/logs/20260927-migration-intake/reviewed-plan-r6.txt) | `5327d2da1b27d847d7fd18bd83a2645747d4cd19fff840687ded39ca83be8e2c` |
| [已审阅治理提案](../gates/logs/20260927-migration-intake/reviewed-governance-r6.txt) | `4081fcbed834ac26fedeae9e1652f1a3e94bd6bcd54089fe1ee4288c3045a19d` |
| [已审阅验收规范](../gates/logs/20260927-migration-intake/reviewed-tests-r6.txt) | `5c5be2860cd2c7d42227be7deae52a776b6006987f39ea997620c33634355768` |

| 顺序 | 实际独立 reviewer | 模型/强度 | 判定 |
| --- | --- | --- | --- |
| 1 | `/root/architecture_review`，Architect | `gpt-6-astra` / `xhigh` | APPROVE |
| 2 | `/root/plan_critic`，Critic | `gpt-5.6-sol` / `high` | OKAY — APPROVE |

Architect 完成后才启动当前候选的 Critic。两者在规划当时均只读核对实际代码与修订 6 摘要。完整返回保存在本会话，本地记录为 `.omx/plans/pycircuit-gfsim-migration-architect-r5.md`、`-architect-r6.md`、`-critic-r6.md`。以下结论不依赖这些本地记录才能理解。

## 关键判断

1. 新方案准确响应用户最新方向：GFSIM 对象式 Pythonic 前端为主干，Python 只 capture，语义分析/lowering 集中到 MLIR，同一硬件 IR 生成一个 C++ backend 和一个 Verilog backend。
2. 清理对象有代码依据：Python `_queue_compiler` 的效果/仲裁分析、非 MLIR QueueGraph 计划和文本 PYC lowering、重复 PYC C++ emitter 都有明确迁移与删除出口。统一 CLI 后继续调用旧三路线不能通过验收。
3. RTL emitter、原语与已有 MLIR 算法可以迁入新链；必要的低层 RTL 私有表示不等于第二产品路线，但不能保留独立 frontend、scheduler 或 C++ generator。
4. donor 已批准设计是首选，`@system`、整数/check、RTL 和 source-unit 的未完成部分是新主干任务，不用旧路线 fallback 弥补。
5. hard break 不等于任意丢功能：能力必须分类为保持硬件行为、采用批准的 donor 语义替换、用户明确批准退役/延期；独立 oracle 与接口拒绝测试相应迁移。
6. 删除验收覆盖源码调用图、CMake/build/link graph、relocated wheel/SDK 和真实执行路径；source-unit、consumer-neutrality、独立审查和候选冻结仍成立。

Critic 分别模拟了 frontend/import trunk、同 IR 双后端最小闭环、完整 hard-break 切换三个任务，检查了依赖、文件 owner、现有代码与命令，未发现需要执行者自行猜测的规划阻断项。

## 已补清的执行边界

- C2 明确 MLIR interface/effect 推导与发布顺序，使 parent 只凭 interface 编译，不能为复用 donor importer 而整系统捕获。
- M2 首样板具有用户批准的最小 top/root 与逐源 composition 入口，不能依赖后续完整 `system` 或旧 frontend。
- M5 切换同一候选同步更新已批准决策、AGENTS/skills、活跃文档和 gates；M7 仅复核发布闭环，不推迟合同切换。

最强反论是同时退役旧路线和补齐 donor 缺口可能扩大切换风险。接受的方案是在隔离候选中尽早证明同 IR 双后端切片，用能力分类与用户接口批准控制范围；不以恢复旧路线减轻短期工作。

## 审阅与验证的范围

规划修订 6 当时的三份文件通过 changed-file pre-commit（Markdown lint、空白、API hygiene）、全范围 API hygiene、严格 MkDocs、相对链接与摘要检查；命令均返回 0。日志位于 `.pycircuit_out/gates/20260927-modernization-single-route/`。

规划审阅本身没有运行产品编译、语义回归或 release closure。执行阶段的治理已启用并纳入导航，当前验证结果见[执行账本](../work-items/single-route-migration.md)；不要用本历史记录代替新的基线或治理验收。

修订 4 的多 authoring 保留方案已被用户最新方向取代，其原文与审阅保存在本地 `.omx/plans/pycircuit-gfsim-migration-history/r4/`；该历史批准不适用于当前内容。
