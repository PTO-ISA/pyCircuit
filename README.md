# Python → MLIR → 静态 rule 图

Python 前端静态实例化 ACPy 模型，保存完整事务计算、状态访问和控制流为 MLIR，再生成带条件和来源的 rule 关系图。保存的 MLIR 可独立重载分析，不需要 Python 源码。

| 入口 | 内容 |
| --- | --- |
| [rule 图设计](rule-graph-spec.md) | 本阶段目标、组件契约、条件依赖与环诊断 |
| [编译器构建与使用](pycircuit/README.md) | compile/analyze、产物和验收命令 |
| [编译阶段的设计意图](pycircuit/docs/compiler-intent.md) | 各分析与转化的目标，编译表示与后续执行能力的边界 |
| [ACPy 语言](acpy/spec.md) | Module/rule/method、Queue/Cell/Signal、SSA 与静态连接 |
| [ACIR 基础设计](acir/basic-design.md) | 本轮确认的结构与组件 primitive、guard 传播及分析职责 |
| [ACIR 编译契约](acir/rule.md) / [保存格式](pycircuit/acir.md) | 事务、SSA/CFG、访问摘要与条件图 |
| [GFSim C++20](gfsim/cpp/README.md) | 独立 runtime 项目，保持现有接口 |
| [编程范式示例](pycircuit/examples/README.md) | 混合端口、共享状态、RISC-V 流水线、跨模块 OoO 调度和依赖环 |

第一阶段不实现运行时激活、仲裁、Xfer、仿真后端或竞争写入检查。旧 EmitC/GFSim 编译路线及其构建依赖已移除；旧 ACPy CPU 示例及测试工具已移除，不维护第二套编译语义。

## 本地参考、构建与结果

外部参考源码放在根目录 `reference/<项目名>/`（本机 `/home/lc/tmp/reference`），不纳入 Git。新构建使用 `pycircuit/builds/`，benchmark 输入、实验、日志和结果使用 `pycircuit/builds/benchmarks/`；历史测量中的旧路径保留。

| 目录 | 内容 |
| --- | --- |
| `reference/ripes-reference/` | 固定原生 Ripes 源码和子模块 |
| `reference/skyzh-riscv-reference/` | 固定原生 skyzh 源码 |
| `pycircuit/builds/` | 编译器、模型、原生参考构建及 CTest 结果 |
| `pycircuit/builds/benchmarks/` | 程序输入、轨迹、性能原始样本及本机实验 |

原生参考的版本锁、观察适配器和验收脚本仍随 GFSim 源码保存。旧 ACPy 示例和测量脚本的历史由 Git 保留；已有本机参考构建与 benchmark 基线不随源码清理删除。
