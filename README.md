# ACPy → MLIR → GFSim

Python 前端解析 ACPy 并输出类型明确的 MLIR ACIR；C++ pass 展开执行语义并经 EmitC 生成 C++，运行在静态调度的 GFSim 上。

| 入口 | 内容 |
| --- | --- |
| [编译器构建与使用](pycircuit/README.md) | `compile` / `emit`、支持的语言能力、验收命令 |
| [Ripes5](pycircuit/examples/ripes5/README.md) | 五级流水端到端模型、原生逐拍对照、固定周期 benchmark |
| [skyzh OoO](pycircuit/examples/skyzh_ooo/README.md) | 模块化 Queue 乱序 CPU、原生逐拍对照、ISA 检查和 benchmark |
| [GFSim C++20](gfsim/cpp/README.md) | runtime、独立安装及边界测试 |
| [ACPy 语言](acpy/spec.md) | 静态构造、资源与运行时语义 |
| [ACIR 编译契约](acir/rule.md) / [保存格式](pycircuit/acir.md) | 资源、路径、效果及 MLIR 表示 |
| [GFSim spec](gfsim/spec.md) / [待决问题](gfsim/open-questions.md) | 当前运行时契约及未支持能力 |

`pycircuit/examples/` 只保留 `ripes5` 和 `skyzh_ooo`。小电路与表达回归位于 `pycircuit/tests/` 和 `gfsim/cpp/tests/`；手写 C++ Ripes5 及 [Python 参考模型](gfsim/experiment/README.md) 供端到端验收使用。

## 本地参考、构建与结果

外部参考源码、构建、benchmark 统一放在根目录 `reference/`（本机 `/home/lc/tmp/reference`），不纳入 Git。

| 目录 | 内容 |
| --- | --- |
| `reference/ripes-reference/` | 固定原生 Ripes 源码和子模块 |
| `reference/skyzh-riscv-reference/` | 固定原生 skyzh 源码 |
| `reference/builds/` | 编译器、模型、原生参考构建及 CTest 结果 |
| `reference/benchmarks/` | 程序输入、轨迹、性能原始样本及本机实验 |

版本锁、观察适配器和可重复的验收脚本随源码保存。结果通过对应示例的命令重新生成；过期报告和源码目录内的结果快照已删除，历史由 Git 保留。已有本机 benchmark 基线不随源码清理删除。
