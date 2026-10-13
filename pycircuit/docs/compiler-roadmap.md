# MLIR 编译流程重构

目标：ACPy → 源语义 MLIR → 事务 MLIR → RTL 友好 MLIR → 逐拍执行验证。
语义以 `core-design.md` 为准；本轮不包含 SystemVerilog、激活调度、跨拍缓存或自动仲裁。

## 基线与保存

2026-10-13：开始时存在 83 个已修改文件及 3 个未跟踪 C++ 文件。
原始 HEAD、状态清单、完整二进制 patch、未跟踪文件归档保存于
`builds/benchmarks/compiler-refactor-baseline/`。现有改动作为独立基线提交保存。
当前链路为 v7 源 MLIR → C++ analysis lowering → program.json → Python 图分析；无执行器。
本 checkout 为 `/home/lc/tmp`，不适用仅允许在 `/home/lc/pyCircuit` 构建的 pyc6 skill。
继续使用 README 指定的 Clang 22、GCC 14 与 MLIR 22。

## 阶段记录

| 阶段 | 状态 | 验证与剩余工作 |
| --- | --- | --- |
| 1. class Module 与统一 Rule、源语义 IR | 已实现 | 仓库全部示例可生成 v8 源 MLIR；2 项源层回归通过，含原生验证、作用域、赋值和旧 API 拒绝 |
| 2. 事务提取与条件传播 | 待实施 | 调用点身份、嵌套原子性、只读依赖、分支要求、共享 SSA、消费合并 |
| 3. MLIR 契约、EHR 与调度分析 | 待实施 | 声明传播、固定读取端口、分支合并 DAG、IR 保存顺序 |
| 4. RTL 友好 lowering | 待实施 | 完整 fire、选择器、组件状态与 next-state、消除决策 |
| 5. MLIR 执行、CLI 与清理 | 待实施 | 两层逐拍一致、独立预期、OoO、ISA CPU、优化与重载 |

## 验收

覆盖 Queue 反压／同拍腾空间／原子取消；共享读取、嵌套分支、读取失败和多个调用点；
Signal、EHR 可见性；OoO 分配／等待／发射／广播／退休；指令 CPU 与独立 ISA。
每阶段更新本文件并独立提交。语义缺口记录最小复现和影响，不通过改写行为绕过。

## 下一步

实现源接口并在现有工具链上构建验证；随后将完整事务条件和调度结果写入 MLIR。

## 阶段 1 验证

class Module、独立静态 __init__、固定 __call__ 连接、显式接口、隐式实例作用域与统一 Rule 已迁移。
规则关系保存为模块内 `acir.contract`，分支仍保留 scf.if／CFG。源 MLIR 原生验证通过。
旧 Method／函数 Module 输入明确拒绝；内部 Work 命名也已移除。

迁移发现：旧 EHRSlots 的 release → available → allocate 把 available 的读取与 allocate 的写入
放在同一个 allocate 事务的不同端口，违反核心设计 §5。保留该例及诊断，新增合法共享读取模型验收。
最小复现为 A SB B、B SB C，C 的 guard 调用 B.read 且 C.write 同一个 EHR。
这属于旧模型与新核心契约不一致，不能通过任意顺序或临时旁路放行。
