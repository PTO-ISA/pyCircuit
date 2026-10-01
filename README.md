# ACIR 与 GFSim

本仓库包含编译设计、GFSim 框架规格和 Python 调度实验。设计契约与实验实现进度分别说明。

| 文档 | 用途 |
| --- | --- |
| [GFSim 框架 spec](gfsim/spec.md) | 对象职责、生成代码、记录布局、跨 tick 激活与复用、同 tick 仲裁、Queue 提交及验收要求 |
| [GFSim 待决问题](gfsim/open-questions.md) | 尚未确定的语义、现有设计缺口和实现边界；不作为已支持能力 |
| [ACIR 编译契约](acir/rule.md) | Python/HIR/ACIR 的资源与路径表示，以及 GFSim、RTL 后端所需信息 |
| [Python 调度实验](gfsim/experiment/README.md) | 当前代码、运行方法、验证范围和性能计数 |

GFSim 的运行时契约统一以 spec 为入口。原 Module、Rule、Queue、Struct、调度、缓存和读取记录草稿已合并移除；历史版本保留在 Git 中。
