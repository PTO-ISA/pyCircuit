# PyCircuit 五级 RV32I CPU demo

这个项目用一颗能够运行 C 程序的五级 RISC-V CPU，展示
**NDF → Spec → PyCircuit → C++/RTL → 验证与综合** 的完整开发流程。
需求、规格、实现和独立检查通过条款关联；同一份 PyCircuit 源码生成软件模型和
物理 RTL，并完成开源 SKY130 标准单元映射。

**从[完整中文实验报告](EXPERIMENT.md)开始**：报告介绍 CPU 与程序、设计流程、
环境安装、生成和运行方法，以及软件模型、RTL 和综合结果。

## 阅读与使用入口

| 内容 | 入口 |
| --- | --- |
| 设计意图与需求 | [NDF 设计输入](spec/design-input.md) |
| 流水线、接口、状态与效果 | [Core Spec](spec/core-spec.md) |
| 独立验收与源码关联 | [验收条款](spec/acceptance.md)、[NDF 工作流](spec/README.md) |
| PyCircuit 实现 | `core.py`、`types.py` 与各计算/流水阶段模块 |
| C 应用 | [programs/c-array.c](programs/c-array.c) |
| 生成 C++ 与物理 RTL | [产物指南](generated/README.md) |
| 映射网表与综合结果 | [综合指南](results/synthesis/README.md) |
| 运行结果与日志 | [实测数据](results/reproduction.json)、[日志](results/validation) |

CPU 实现 RV32I 基础整数指令，具有 32 个通用寄存器与实际重叠执行的五级流水线。
转发、加载依赖造成的停顿、分支冲刷、存储器等待、复位与精确异常由统一的状态和提交
规则处理。`Machine` 保存完整状态，`Core.step` 原子提交下一状态。

生成源码和运行 SDK 固定在 `3ee29b0`；具体依赖见
[生成说明](generated/artifacts.json)和[工具安装文档](verification/TOOLS.md)。
直接使用已提供的 C++/RTL 时，运行入口为：

```sh
python3 designs/rv32i_core/verification/run_generated.py \
  --prefix "$PYC_TOOLCHAIN_ROOT" --program "$OUT/programs/c-array" \
  --output "$OUT/compiled-artifacts"
```

准备 C 程序、检查停顿依赖、重新生成源码及运行映射网表的完整步骤，见
[环境与复现](EXPERIMENT.md#环境与复现)和[综合与面积](EXPERIMENT.md#综合与面积)。

## 实验结果

C 程序在功能/时序 × C++/RTL 四个组合中均通过独立参考检查，每个组合正常完成
434 条指令，以一次预期 trap 结束，共 567 个模拟时钟周期。三类停顿依赖检查共
12 个组合通过。SKY130 映射得到 13,860 个单元、109,284.8128 µm² 单元面积，
映射后 C 程序仿真也通过。结果定义和解释集中在[实验报告](EXPERIMENT.md)。
