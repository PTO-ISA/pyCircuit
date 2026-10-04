# ACIR / GFSim 设计审查入口

当前设计与验证记录见：

- [GFSim 框架 spec](gfsim/spec.md)：已确认的对象、记录、调度、事件和提交契约。
- [GFSim 待决问题](gfsim/open-questions.md)：尚未确定的语义与实现边界。
- [Ripes5 实验](gfsim/experiment/examples/ripes5/README.md)：逐拍对照、性能测量和限制。
- [ACIR 编译契约](acir/rule.md)：编译器需要保留的资源、路径和原子边界。

GFSim 的最新讨论基准是 [Issue #270](https://github.com/PTO-ISA/pyCircuit/issues/270)。与 #268、#269 的 GFSim 内容重合或冲突时，以 #270 为准。

2026-09-28 的审查记录保存在 [历史快照 b32639c](https://github.com/PTO-ISA/pyCircuit/blob/b32639cccee8af9e66dcfa663ea9050e957fd2b0/design-review.md)。其中的 A/B 备选、delta、静态容量图等判断对应当时草稿；当前契约与未决项分别以上面的 spec 和待决问题为准。
