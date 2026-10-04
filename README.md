# ACIR 与 GFSim

本仓库包含编译设计、GFSim 框架规格、C++20 单线程核心和 Python 调度实验。设计契约与实验实现进度分别说明。

| 文档 | 用途 |
| --- | --- |
| [GFSim 框架 spec](gfsim/spec.md) | 对象职责、生成代码、记录布局、跨 tick 激活与复用、同 tick 仲裁、Queue 提交 |
| [GFSim 待决问题](gfsim/open-questions.md) | 尚未确定的语义、现有设计缺口和实现边界；不作为已支持能力 |
| [ACIR 编译契约](acir/rule.md) | ACPy/MLIR ACIR 的资源与路径表示，以及 GFSim、RTL 后端所需信息 |
| [独立 ACPy 编译器](pycircuit/README.md) | AST → MLIR ACIR → EmitC → GFSim C++；完整 Ripes5 与 Queue 版乱序 CPU |
| [C++20 GFSim 核心](gfsim/cpp/README.md) | 独立构建安装、生成式接口、完整电路参考对照、原生验收及性能基准 |
| [Python 调度实验](gfsim/experiment/README.md) | 通用引擎与 Ripes5 示例、运行方法和性能计数 |

GFSim 的运行时契约统一以 spec 为入口。原 Module、Rule、Queue、Struct、调度、缓存和读取记录草稿已合并移除；历史版本保留在 Git 中。

GFSim 的最新讨论基准见 [Issue #270](https://github.com/PTO-ISA/pyCircuit/issues/270)。与 #268、#269 的 GFSim 内容重合或冲突时，以 #270 为准；旧设计审查的历史入口见 [design-review.md](design-review.md)。
