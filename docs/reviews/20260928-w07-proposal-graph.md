# W07 ProposalGraph / conflict closure 候选验收

日期：2026-09-28。结论：PASS（compiler-private analysis 子片）。

候选为 `/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`，HEAD
`82f161ea95336fdd15c238e103f13b9f823e8f28` 加迁移 overlay；文件摘要位于
候选 `.pycircuit_out/w04-acceptance/candidate.json`。

新增 private ProposalGraph：遍历 ModuleGraph postorder，将每个实际 SourceUse 的
UseID、ValueID、D/E、raw value/valid/path、owner/rule/child 与解析后的 StateID
绑定。每个 StateID 形成 Hold、Forward 或无优先级的 ExclusiveMerge recipe。
true/true overlap 静态拒绝；无 proof carrier 的 possible overlap fail closed；
constant-false contribution 不阻塞其他合法 commit。当前 global permit 只接纳无待执行
check 的恒真闭包；source markers 保留，不提前做 final lowering。

最终 verifier 不信任 mutable recipe：重新解析 target StateID，逐项重绑 actual
SourceUse SSA/identity，重算 active writers 与全部 unordered conflict pairs，并要求
commit contributors 恰等于 state-local active 全集。越界、cross-state、漏pair、伪造
MutuallyExclusive、同型target交换和global-permit篡改均拒绝。

独立最终验收及PM fresh rerun：Proposal mutation 9/9、SourceLink 5/5、
ModuleGraph 5/5；完整回归另含 RuleEffects 7/7、SourceFacts 14/14、lexical20/20，
零 failure/error/skip/disabled。格式与diff检查通过。

关键绑定：

- `ProposalGraph.h`：`87f7826bc03f185415954fb0ab62ce5fe2a52bc8e42dfa3430fe21c370a5db7b`；
- `ProposalGraph.cpp`：`faf96cc2b5a5d12009f124a68245cf99deb41e53df20f17b22548d43f74f0fd0`；
- `SourceLink.cpp`：`404d2112571803b20ec44db5b55b3d333097093555eb9415b1852e5462573439`；
- `ProposalContractsTest.cpp`：`723e360156e038418a7c47013a1c1f0a9dbe76cb224ec8a4025fc998c231e37c`。

本结论不关闭 numeric/check permit materialization、动态child proposal fixture、final
residual-source elimination、runtime Xfer或真实C++/Verilog emitter责任。
