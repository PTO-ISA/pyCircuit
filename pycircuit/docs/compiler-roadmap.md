# MLIR 编译流程重构

目标：ACPy → 源语义 MLIR → 事务 MLIR → RTL 友好 MLIR → 逐拍执行验证。
语义以 `core-design.md` 为准；本轮不包含 SystemVerilog、激活调度、跨拍缓存或自动仲裁。

## 基线与保存

2026-10-13：开始时存在 83 个已修改文件及 3 个未跟踪 C++ 文件。
原始 HEAD、状态清单、完整二进制 patch、未跟踪文件归档保存于
`builds/benchmarks/compiler-refactor-baseline/`。现有改动作为独立基线提交保存。
开始时的链路为 v7 源 MLIR → C++ analysis lowering → program.json → Python 图分析；无执行器。
本 checkout 为 `/home/lc/tmp`，不适用仅允许在 `/home/lc/pyCircuit` 构建的 pyc6 skill。
继续使用 README 指定的 Clang 22、GCC 14 与 MLIR 22。

## 阶段记录

| 阶段 | 状态 | 验证与剩余工作 |
| --- | --- | --- |
| 1. class Module 与统一 Rule、源语义 IR | 已实现 | 仓库全部示例可生成 v8 源 MLIR；2 项源层回归通过，含原生验证、作用域、赋值和旧 API 拒绝 |
| 2. 事务提取与条件传播 | 已实现 | C++ pass 直接读取 MLIR；调用点独立、嵌套原子、共享 SSA、失败不进入 else、按资源合并消费均通过逐拍回归 |
| 3. MLIR 契约、EHR 与调度分析 | 已实现 | 声明沿调用栈传播，C／SBR 同路径诊断，EHR 固定视图、未声明可见性拒绝，分支／后继与合并 DAG 保存于 IR |
| 4. RTL 友好 lowering | 已实现 | Cell／Queue／EHR 化为 current、组合选择与 next；Signal 无 RTL 状态；独立 verifier 检查残留操作、状态类型与调度 |
| 5. MLIR 执行、CLI 与清理 | 已实现 | 当前 29 项回归、CTest 1/1、8 个模型各 32 拍；两层执行、优化与三阶段重载一致；旧编译路径已删除 |

## 验收

覆盖 Queue 反压／同拍腾空间／原子取消；共享读取、嵌套分支、读取失败和多个调用点；
Signal、EHR 可见性；OoO 分配／等待／发射／广播／退休；指令 CPU 与独立 ISA。
每阶段更新本文件并独立提交。语义缺口记录最小复现和影响，不通过改写行为绕过。

## 下一步

本轮五阶段完成。后续可独立开展 SystemVerilog 后端、动态循环的硬件表达、自动仲裁、
激活调度及跨拍缓存优化；这些工作不纳入本轮完成范围。

## 阶段 1 验证

class Module、独立静态 __init__、固定 __call__ 连接、显式接口、隐式实例作用域与统一 Rule 已迁移。
规则关系保存为模块内 `acir.contract`，分支仍保留 scf.if／CFG。源 MLIR 原生验证通过。
旧 Method／函数 Module 输入明确拒绝；内部 Work 命名也已移除。

迁移发现：旧 EHRSlots 的 release → available → allocate 把 available 的读取与 allocate 的写入
放在同一个 allocate 事务的不同端口，违反核心设计 §5。保留该例及诊断，新增合法共享读取模型验收。
最小复现为 A SB B、B SB C，C 的 guard 调用 B.read 且 C.write 同一个 EHR。
这属于旧模型与新核心契约不一致，不能通过任意顺序或临时旁路放行。

## 阶段 2 验证

事务 IR 由原生 C++ pass 建立，完整条件成为 SSA。模块共享定义保留单一来源；
分支合流恢复无关计算的控制作用域，合流值自身保留读取就绪条件。嵌套效果全部由外层 fire 控制。
消费请求按事务与真实 Queue 合并；普通效果性返回值跨独立事务时报错。

14 项新回归通过：包括两层一致、Queue 多级反压／腾空间／取消、读取失败与无关事务、
嵌套分支／重复消费、Signal 取消、EHR 固定视图、CPU 独立 ISA 和定向退休周期、OoO 顺序退休。
源／事务／RTL 均可独立重载；CPU 和 OoO 的优化开关结果一致。新日志位于
`builds/benchmarks/refactor/`。后续阶段继续扩展契约诊断、端口约束及结构检查。

设计参考：[BSC AAddScheduleDefs.hs](https://github.com/B-Lang-org/bsc/blob/main/src/comp/AAddScheduleDefs.hs)
明确／隐式条件合成 CAN_FIRE，效果使能与 WILL_FIRE 关联。本轮不仲裁，按核心设计以完整条件作为 fire。

## 阶段 3 验证

21 项回归通过。新增无共享资源的声明顺序、C／SBR 嵌套调用拒绝、
release → allocate 合法 EHR guard、无声明时读取旧值、共享读取对所有使用者的依赖传播、
拒绝端口排名引入的未声明可见性，以及 Queue 数组真实资源选择／重复消费合并。
事务 IR 的 `acir.path` 保存分支条件 SSA，边保存源分支和目标分支集合，fire 保存后继。
固定节点级拓扑顺序保存于 circuit；分支边合并形成环时明确拒绝。

同拍端口竞争保存为 conflicts 报告，不自动仲裁。对同事务禁用关系，仅在实际路径可能重合时拒绝；
布尔 SSA 证明无法处理过大的条件时采取保守诊断，不增加 EHR 可见性。

## 阶段 4 验证

RTL lowering 已独立为 RTLLowering.cpp，事务 pass 独立为 Transactions.cpp。
Queue 化为固定存储数组、count、消费移位和入队索引更新；Cell/EHR 为组合选择链与 next-state。
Signal 只有组合 data/valid，不留状态声明。共享读取使用同一转换映射，保持固定端口。
RTL verifier 检查无残留调用／控制流／状态效果、状态类型正确、调度包含全部 fire 且满足顺序边。
原生 pass 串联、默认构造配置、冻结配置重载、CLI 执行和损坏调度文件的拒绝均已回归。
25 项测试与 CTest（1/1）通过；完整日志与 JUnit 位于 builds/benchmarks/refactor/。

范围限制最小复现：`for i in range(counter): ...` 中 counter 来自运行时 Cell。
源层可以保存循环，当前有限组合 RTL lowering 对循环 CFG 报错；不能静态确定展开次数时不生成动态执行硬件。
字面量固定循环已支持。此限制不影响本轮验收模型，也不通过改变循环行为绕过。

## 阶段 5 验证与清理

2026-10-13：26 项回归全部通过，CTest 1/1 通过。事务与 RTL 执行器直接读取 MLIR，
每拍保存 fire 与提交状态；CLI 支持分阶段停止、独立重载、静态配置与指定拍数执行。
CPU 用独立指令位解码的 RV32I 参考核对寄存器和内存，退休拍为 4、5、9、13、14、18。
OoO 最终寄存器为 [0,42,6,7,48,13,61,0]，退休标签顺序为 1、2、3、4；
同时检查分配、操作数等待、ADD／MUL 发射和完成广播确实发生。

`tests/run_acceptance.py` 对 dual_channel、shared_state、mixed_ports、signal_link、
cell_feedback、ehr_shared、riscv_pipeline、ooo_scheduler 各执行 32 拍。
所有模型的事务／RTL 轨迹、优化开启／关闭、源／事务／RTL 独立重载均一致。
EHRSlots 的同事务跨端口操作、ExclusiveRing 与 QueueRing 的分支合并环均得到预期诊断。
完整轨迹、配置及汇总保存在 `builds/benchmarks/compiler-acceptance/`；
回归日志与 JUnit 保存在 `builds/benchmarks/refactor/`。

删除 program.json 导出、Python 图分析／契约／EHR 主链路及旧 Method／Work 测试，
以新接口的源层与逐拍验收替代。更新 ACPy／ACIR 规范、编译意图、CLI 使用文档和图查看工具。
仓库保留的示例输出已重新生成三个 MLIR 层与事务图；过时的 readable／Module 图输出已清理。
JSON 仅作报告，不作为编译或重载输入。原始未提交内容仍可由基线提交及归档恢复。

## 补充修正：只提供 Rule 的组件无需主体

2026-10-13：放宽前端对 `__call__` 的强制要求。只有状态和公开 Rule 的组件可省略主体，
实例化后直接调用其 Rule，不要求额外写 `rf()`，也不生成空 `acir.body` 或自动调度 Rule。
需要固定输入／输出或每拍选择逻辑的 Module 仍使用显式 `__call__` 连接。
未定义主体却调用实例或访问 `.outputs` 会报告具体原因；未连接输出也有明确诊断。

CPU 中 RegisterFile／DataMemory、DualChannel 的 Observer 已移除导出包装／空主体及无意义的连接调用。
内部 RF／内存状态直接由逐拍轨迹观察。同步更新核心设计、ACPy／ACIR、使用文档和保存的三层产物。
新增回归覆盖无主体实例的 guard／嵌套原子性、实例身份、未调用 Rule 不执行和无主体 top 的重载；
已有 EHR 固定共享读取回归也改为无主体组件，继续验证声明关系及端口依赖传播。

29 项回归和 CTest 1/1 通过，8 个示例的两层执行、优化开关及三阶段重载验收继续通过。
CPU 与 DualChannel 改写前后各 32 拍的 fire 和状态完全一致，CPU 的独立 ISA 与退休周期预期不变。
修正前轨迹、对比结果、回归日志和 JUnit 位于 `builds/benchmarks/rf-call-inspection/`。
