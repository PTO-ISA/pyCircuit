# pyCircuit：Python → MLIR → RuleGraph

前端静态展开 ACPy 模型，保存可独立重载的 MLIR，再分析条件化的 Rule 依赖图。当前实现提供编译和静态分析，不包含执行或仿真后端。

| 入口 | 内容 |
| --- | --- |
| [ACIR 基础设计](acir/basic-design.md) | 本轮确认的目标 primitive、组件契约和 guard 传播 |
| [编译器构建与使用](pycircuit/README.md) | 构建、compile/analyze、产物和可视化 |
| [ACPy 前端](acpy/spec.md) | 当前前端写法与支持范围 |
| [ACIR 当前格式](pycircuit/acir.md) | 当前 v6 操作、保存格式和分析字段 |
| [编译阶段的设计意图](pycircuit/docs/compiler-intent.md) | 各分析与转化需要保留的语义 |
| [编程范式示例](pycircuit/examples/README.md) | 组件组合、五级流水线和 OoO 调度例子 |

基础设计描述后续重构目标；当前格式文档描述已经实现的表示，两者尚未完全一致。

## 本地参考、构建与结果

| 目录 | 用途 |
| --- | --- |
| `reference/<项目名>/` | 外部参考源码，不纳入 Git |
| `pycircuit/builds/compiler/` | 编译器构建 |
| `pycircuit/builds/<例子名>/` | 可供 review 的 MLIR、JSON 与可视化 |
| `pycircuit/builds/benchmarks/` | benchmark 输入、实验、日志和结果 |

新构建和实验使用以上路径。已有本地参考目录与历史测量保留。
