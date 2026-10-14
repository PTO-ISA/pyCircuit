# pyCircuit：ACPy → MLIR → 逐拍执行验证

静态展开 class Module，保存源语义 MLIR，在原生 MLIR pass 中提取事务、传播条件与契约、
分配 EHR 端口和固定调度，降低为 RTL 友好的组合逻辑与 next-state，并对照两层逐拍执行。

| 入口 | 内容 |
| --- | --- |
| [核心抽象设计](pycircuit/docs/core-design.md) | Cell、Queue、Signal、EHR、统一 Rule、Module 与同拍顺序 |
| [array 设计](pycircuit/docs/array-design.md) | 多维构造、match、选取域、仲裁及保守 lowering 的正式目标设计 |
| [编译路线图](pycircuit/docs/compiler-roadmap.md) | 阶段进度、基线保存、验证证据、问题与下一步 |
| [构建与使用](pycircuit/README.md) | 编译、分层重载、逐拍执行和 CLI |
| [ACPy 语法](acpy/spec.md) | class Module、显式类型、固定连接与 Rule 关系 |
| [ACIR v8](pycircuit/acir.md) | 源语义、事务和 RTL 三层 MLIR |
| [编译意图](pycircuit/docs/compiler-intent.md) | 各阶段必须保留的语义 |
| [示例与验收](pycircuit/examples/README.md) | Queue、共享计算、OoO 和指令 CPU |

## 本地参考、构建与结果

| 目录 | 用途 |
| --- | --- |
| `reference/<项目名>/` | 外部参考源码，不纳入 Git |
| `pycircuit/builds/compiler/` | 原生编译器构建 |
| `pycircuit/builds/<例子名>/` | 可 review 的三层 MLIR 和图报告 |
| `pycircuit/builds/benchmarks/` | 新 benchmark 输入、实验、日志和结果 |

历史测量保留原路径；新运行统一使用以上位置。
