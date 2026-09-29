# W08 GFSIM DFFE / flat system 候选验收

日期：2026-09-28。结论：PASS（header-only runtime 子片）。

候选为 `/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`，HEAD
`82f161ea95336fdd15c238e103f13b9f823e8f28` 加迁移 overlay。

新增 `gfsim::SimDFFE<T>`：Write只暂存D/E，Q只由Xfer更新；disabled pair在
Xfer清除且不重放；重复Write拒绝并保留首pair；Reset为pending且优先于write；
DiscardNext清write/reset而不改变Q；Xfer为noexcept并要求no-throw assignment。

新增无children/递归的flat `SimModule`接口及`SimSystem`。Step固定执行全部Work、
全部Check、全部Drive和最后统一Xfer。任一Check或Drive失败会对全部module执行
DiscardNext，零Xfer、零cycle并进入Failed；Failed拒绝后续Step。Reset先对全部
module建立pending reset，再统一Xfer，恢复Ready/cycle0。空系统或全部HasWork=false
进入Quiescent且epoch保持0；有work但无write仍完成一个成功cycle。

独立验收与PM fresh rerun：RegRuntime 5/5、SystemLifecycle 8/8；零failure/error/
skip/disabled。完整正常Step分配计数为0；新headers无SimQueue、queue.h、object.h、
children或walk依赖。格式与diff检查通过。

关键绑定：

- `SimDFF.h`：`d00917c16960e84292f3c5ee5b830a75aa55c0672c12ffb56033c9438a27af7c`；
- `SimModule.h`：`d90272d8286c003848ddc1c555444891bdf78233ac07bba4d57c1b6fbfbf5733`；
- `SimSystem.h`：`411040f6972249cb4684bb43922fa516c241e83ec1e4d415b3cb8eb08956806e`。

本结论不关闭generated ModuleGraph/ProposalGraph到runtime的adapter、ReportStat内容、
灾难性host exception、runtime安装导出、W09 observations或W10双后端责任。
