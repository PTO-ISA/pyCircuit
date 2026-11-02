# SKY130 综合与映射结果

本目录提供五级 RV32I core 的实际综合输入、标准单元映射网表和运行记录。
实验得到 **13,860 个单元、109,284.8128 µm² Liberty 单元面积**。
生成来源与工艺依赖集中在[生成说明](../../generated/artifacts.json)，结果解释与
复现步骤见[中文实验报告](../../EXPERIMENT.md#综合与面积)。

| 文件 | 内容 |
| --- | --- |
| `mapped.v` | 完整映射 Verilog 网表 |
| `synthesis.v` | sv2v 转换后送入 Yosys 的输入 |
| `synth.ys.in` | 综合命令序列，输入和 Liberty 路径使用参数占位符 |
| `yosys.txt` | 完整 Yosys/ABC 日志，本机路径以变量表示 |
| `stat.json`、`area.json`、`cell-counts.csv` | 综合统计、面积和各类单元数量 |
| `normalization.json` | 归一化前后的单元与连接一致性结果 |
| `ports.json` | 映射端口信息，用于标量端口仿真适配 |

`area.json` 记录综合阶段，[实测结果](../reproduction.json)和
[映射仿真日志](../validation/mapped-simulation.txt)记录后续 C 程序运行。
通过 `verification/synthesize.py` 可以重新映射，通过
`verification/check_mapped_artifact.py` 可以重算现有网表的单元数与面积，并进行
映射仿真；完整命令位于实验报告。
