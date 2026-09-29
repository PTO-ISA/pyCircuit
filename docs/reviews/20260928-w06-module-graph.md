# W06 ModuleGraph / StateID link 候选验收

日期：2026-09-28。结论：PASS（有界 scalar/whole-record 子片）。

候选为 `/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`，
HEAD `82f161ea95336fdd15c238e103f13b9f823e8f28` 加迁移 overlay；最终摘要在
候选 `.pycircuit_out/w04-acceptance/candidate.json`。

新增 compiler-private ModuleGraph：完整 source link unit closure 生成唯一 root、
OwnerRef、parent/ordered children、postorder、owned/boundary StateID 与 formal-to-actual
aliases。root 优先使用唯一 system，否则唯一 zero-incoming definition。child owner path
追加 instance occurrence；owned StateID 由 owner/declaration/element 组成。

构图拒绝递归、不可达 definition、重复 owner/double-parent guard、错误 control
forwarding、private/foreign state handle、actual arity、R+W identity分裂、重复
write-capable actual 和不一致 LogicalType。SourceLink 在 authority、snapshot 和 native
verification 后强制构图，失败阻断 admission。

独立最终验收：SourceLinkAdmission 5/5、ModuleGraph 5/5、RuleEffects 7/7、
SourceFacts 14/14、lexical 20/20；零 failure/error/skip/disabled。clang-format、Ruff、
`git diff --check`通过。PM fresh rerun SourceLinkAdmission 5/5、ModuleGraph 5/5。

关键绑定：

- `ModuleGraph.h`：`10cc8f742a72f15086687a950a1db257f45c52681b2305a60301ace34efe5d68`；
- `ModuleGraph.cpp`：`2b174ac92eb2067b54e6dc92bcbe46c8ae4838b21bbde54cc81e61e81cdcf004`；
- `SourceLink.cpp`：`63def5c4051c167f18b67216ee258ec853c591380a813e3619f0a4147643fa31`；
- `ModuleGraphTest.cpp`：`67d763e6c269813884f95564cc1b0195b84450f665357ae52fdfb099bfd90066`。

本结论不关闭 V26 runtime Build/Work/Xfer 生命周期、non-scalar/multi-element child
connections、公开或 serialized graph ABI、W07 proposal/commit 或双后端责任。
