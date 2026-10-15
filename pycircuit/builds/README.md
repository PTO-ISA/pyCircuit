# 示例编译产物

使用当前编译器重新生成，采用默认优化。每个示例包含三个可独立重载的 MLIR 阶段。

| 示例 | 源语义 IR | 事务 IR | RTL 友好 IR |
| --- | --- | --- | --- |
| ehr_shared | [source（88 行）](ehr_shared/model.source.mlir) | [transaction（44 行）](ehr_shared/model.transaction.mlir) | [rtl（29 行）](ehr_shared/model.rtl.mlir) |
| array_arbiter | [source（800 行）](array_arbiter/model.source.mlir) | [transaction（663 行）](array_arbiter/model.transaction.mlir) | [rtl（582 行）](array_arbiter/model.rtl.mlir) |
| riscv_pipeline | [source（1852 行）](riscv_pipeline/model.source.mlir) | [transaction（1282 行）](riscv_pipeline/model.transaction.mlir) | [rtl（1278 行）](riscv_pipeline/model.rtl.mlir) |
| ooo_scheduler | [source（2789 行）](ooo_scheduler/model.source.mlir) | [transaction（1471 行）](ooo_scheduler/model.transaction.mlir) | [rtl（1472 行）](ooo_scheduler/model.rtl.mlir) |
| ripes5 | [source（5140 行）](ripes5/model.source.mlir) | [transaction（4134 行）](ripes5/model.transaction.mlir) | [rtl（32672 行）](ripes5/model.rtl.mlir) |

阅读顺序建议：ehr_shared → array_arbiter → riscv_pipeline → ooo_scheduler。

- source：静态组件、Rule 定义、Module 主体和控制流。
- transaction：完整 fire 条件、共享计算、条件化效果、端口和同拍顺序。
- rtl：组合逻辑、标量状态 current／next-state。
- rule-graph.json：事务关系报告。

`compiler/` 是重建后的工具及构建文件。

两个 CPU 的寄存器堆已改为独立 Cell 数组，C++ 生成结果可查看
[流水线模型](riscv_pipeline/cpp/model.hpp)及 [OoO 模型](ooo_scheduler/cpp/model.hpp)。
迁移前后 32 拍的 fire 和逻辑状态一致；证据保存在
`benchmarks/registerfile-cells/`，轨迹中的 Cell 表项以 `.0`、`.1` 等路径分别列出。

2026-10-15 的源层／事务条件优化与 BSC 生成代码对照见
[C++ lowering 审阅](../docs/cpp-lowering-review.md)。本轮基线、66 项测试、四模型三路逐拍对照、
阶段重载、sanitizer／reset 和代码／缓存／时间测量在 `benchmarks/bsc-inspired-compiler/`。
事务及 RTL 友好层是 **IR 解释执行**，另与编译的 C++ 及保存的旧 C++ 逐拍比较。

Ripes5 的入口是 `examples/ripes5/model/demo.py`；[生成的 C++](ripes5/cpp/model.hpp)保留五级 Module、
RF／存储器 Cell 数组和前递／停顿／冲刷逻辑。9 个原生 Ripes 参考程序的逐拍证据位于
`benchmarks/ripes5/`；本次使用保存的原生轨迹，没有重新运行 Ripes。

Ripes5 级间 Cell 已统一由顶层声明，并通过 Stage return 绑定输出。
迁移前后 9 个程序共 260 拍的全部状态／fire 一致（仅资源路径改变）；
证据在 `benchmarks/ripes5-ports/`，包含原生参考对照、迁移比较及 Demo 验证。

Ripes5 的 HazardUnit／ForwardingUnit 已从 Execute 拆出，分别负责 load-use 检测和前递。
9 个程序共 260 拍的全部状态／fire 与拆分前一致，仍为 6 个状态修改事务；
原生保存轨迹及 C++／两层 IR 对照结果在 `benchmarks/ripes5-units/`。

此前只读译码版本的验证在 `benchmarks/ripes5-decode/`。
当前 Decode 内部用 Signal 发布本拍译码，公开 `inspect()` 读取；顶层不再传回译码结果。
新增一个发布事务，共 7 个事务；9 个程序共 260 拍的 Cell 状态及原有事务的 fire 与之前一致。
Signal 顺序检查、原生保存轨迹及 C++／两层 IR 对照结果在 `benchmarks/ripes5-signal/`。


2026-10-15 RV32IMC 原用例验证记录：五级 Module、独立 HazardUnit／ForwardingUnit 和 Decode Signal 保留，
默认 Ripes5 增加环境事件事务，共 8 个事务。此处三层 IR、C++ 和图均已用当前模型重建；Demo 的 25 拍
与原有原生轨迹一致，并通过生成 C++ 的 ASan／UBSan。

原生 RV5S／Qt 已重新构建，原有 9 个程序重新采集。完整验收的 66 个程序中，61 个共 5798 拍严格一致；
3 个确认参考缺陷、2 个参考不支持单列。全部 75 个 RV32I／M／RV32C 整数指令名称有实际执行覆盖，
完整编译器回归 76 项通过。报告见 [RV32IMC 验收](../examples/ripes5/results/rv32imc-report.md)，
原始证据位于 `benchmarks/rv32imc/`。事务和 RTL 友好层仍为 IR 解释执行，没有真正 RTL 仿真。

后续审查确认仍有未修复的问题，当前尚未完成最终验收，见[审查报告](../examples/ripes5/results/review-2026-10-15.md)。

Ripes5 的取指错误已沿流水传到 EX，原生观察适配器的打印在拍末完成；默认产物已重建。当前 79 项对照与四项缺陷状态见[修复验证](../examples/ripes5/results/fixes-2026-10-15.md)。历史性能数据对应旧版本，本轮没有重测。

字段依赖分析和 C++ 条件回退修复后，默认 Ripes5 模型保持原样；默认 IMC C++ 为 5,364 行。
四种接线对照、通用回归及完整程序验收见[修复报告](../examples/ripes5/results/field-analysis-2026-10-15.md)，
最终证据位于 `benchmarks/field-analysis-fix/release-*`，仍未进行真正 RTL 仿真。
