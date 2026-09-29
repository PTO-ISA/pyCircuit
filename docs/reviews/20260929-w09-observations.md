# W09 observations 候选验收

日期：2026-09-29。结论：PASS（unconditional direct-read 子片）。

候选为 `/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`，HEAD
`82f161ea95336fdd15c238e103f13b9f823e8f28` 加迁移 overlay。

本子片新增 source `ac.observe(path, values...)`、ordered
`ac.required_observations` 和 compiler-private ObservationGraph。verifier绑定 owning
instance、rule registration、site、spec、ValueID/constraint、actual SourceRead、path和
values；删除、复制、重定向、同型SSA交换和required list漂移均拒绝。Python frontend
只识别canonical unshadowed `print/log/report`，并对静态spec、有限direct-read values和
nonnegative u64 report做fail-closed校验。

runtime新增build-time配置的固定ObservationSlots。Work只stage冻结值；Check/Drive失败
整轮Discard；成功module Xfer和cycle++后按descriptor结构序发布epoch t+1 event或更新
gauge。Reset清pending/history并将gauge归零；Stage/Check/Xfer无分配，slot层不包含host
sink、stdout、SimQueue或model-vtable接口。

独立最终验收与PM fresh rerun：ObservationContracts 6/6、Observation runtime 9/9、
Python source observations 9/9、SourceLink 5/5；完整回归另含ModuleGraph/ProposalGraph/
RuleEffects和W08 runtime门槛。零failure/error/skip/disabled，格式与diff检查通过。

关键绑定：

- `ACIROps.td`：`424cb67157d510056589537bd03851c4371e21cb63a4657fafe7913b800f1436`；
- `ACIRModuleOps.cpp`：`26108709670df7e2f557af0019133dcfa29a21bbb833e45948530ec20314847d`；
- `ObservationGraph.cpp`：`ba0c88bfdaf1375ed60567b79276080f493a21153a753c1b3ec1c0d0bd2dce70`；
- `PythonImportObservations.cpp`：`1421193403b5e6fbf99adfef2511b4a486683a5c3b6271d73974a1bf659b65e6`；
- `ObservationSlot.h`：`dae06f3389c04d4148472d530b48205c40b4e4b53e8f757bfc9974eb997ff26d`。

本结论不关闭conditional observation、assert/check/numeric-proof integration、两个instance
same-site排序oracle、runner `--events`、ReportStat/statistics JSON、synthesis sink投影或
W10 emitter责任。
