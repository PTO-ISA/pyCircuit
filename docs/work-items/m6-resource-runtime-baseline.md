# M6-04：资源采样与有限长运行基线

状态：done for selected macOS/POSIX developer measurements。日期：2026-10-01。输入产品 `9ff015a2`。
授权：M3/M6 扩展计划与用户继续下一步。测量工具/报告仅开发证据，非 installed
CLI/IR/runtime/ABI/schema；没有新性能阈值、cache 或 steady-state 保证。

## 有界目标

首轮复用 M6 shared 1/16/64、distinct 1/8/32 generator 中所选小/大尺寸，
通过现有 per-source compile/link/emit/model CMake 流程。采集每个实际阶段的
wall time 和 POSIX owned process-group RSS 采样；数字是 sampled sum/下界，
不是精确整机/独占内存峰值。短暂子进程可能漏峰，共享页重复计量必须披露。

长运行使用已批准 model ABI 的 create/configure/reset/step/statistics/last_error
回调或现有 runner，均使用同一执行器。首轮关注 C ABI consumer loop 的
amortized epoch/s，包括 callback cost；不冒称裸 kernel/steady-state throughput。
无公开计数器/参数/trace ABI 增补，无新 RootDriver 或硬件 source 实现。

## Ownership / invariant

PM 单一 integration owner：fresh build/install、work packet/evidence/status。
Luna implementation 仅 `flows/tools/m6_process_usage.py` 与
`flows/tools/measure_m6_resources.py`（尚不存在）及 ignored output helpers；
独立 Luna tests：新的 measurement unit/system tests，不能改实现。
Sol high 审阅测量方法/实际源码 DAG/循环 oracle；所有被测程序来自本 checkout。
M3 提案作者/审阅文件互不重叠。并发准备允许，正式比较阶段避免其他 native build
或记录竞争 workload，不用这些探索样本建立回归阈值。

- 不修改既有 C3 receipt/journal/managed publication 或 runtime callbacks。
- 使用 existing generator/public driver，不能 whole-design compile 后切 `.ac`/CPP。
- 不把 rusage 子进程累计 high-water 或一个 parent RSS 当完整进程树峰值。
- 不用 stdout/退出 0 猜 epoch：C ABI StepResult/status/stats验证实际有限终止与 reset。
- 构建 startup/warm-up/consumer callback/header依赖/输出采样分别记录。

## Checklist

- [x] collector 记录 root/group、样本数、最大 sampled RSS、单位/间隔/不可用原因。
- [x] process group 子进程、共享页/漏峰/平台限制明确；超短命令采不到即 unavailable。
- [x] stage真实argv/exit/wall和可审阅log，独立验证分类/byteinventory/元数据Gitpin。
- [x] selected shared/distinct sourceproducer/TU/CPP&RTLemit正确，不只重用旧二进制。
- [x] finite长运行几何epoch/重复样本，精确end epoch/TERMINATED/无error并Reset重跑。
- [x] 小模型三拍现有literal field report oracle仍通过，长期数据不代替correctness。
- [x] 独立 unit/system tests、Solreview、hooks/doc checks和当前candidate哈希完整。
- [x] 测量结果只关闭selectedmethod/sizes，完整矩阵/精确RSS/隔离steady-state仍单列。

## 验收与限制

独立SolAPPROVE，candidateaggregate
`4418364a20201086fd83aa9c085625025705951e88ad81576232b201ebc61cf7`；
[审阅](../reviews/20261001-m6-resource-runtime-review.md)与
[报告/日志](../gates/logs/20261001-m6-resource-runtime/README.md)留档。
selected shared1/16、distinct1/8四case，32phase及24finiteCABIrun通过；
独立与PMfocused39tests通过。未测RTL仿真/吞吐、精确exclusiveRSS、隔离steady-state、
最大原始size矩阵或perfthreshold，不能用本包数字扩大这些承诺。
shared1起始约12s有PMtest竞争，探索值保留而不建立回归阈值。
