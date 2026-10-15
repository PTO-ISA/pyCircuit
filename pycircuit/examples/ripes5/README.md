# Ripes5：RV32IMC 五级 CPU 与验证报告

本例用 ACPy 描述与原生 Ripes RV5S 对照的五级 CPU，保留独立 HazardUnit、ForwardingUnit 和 Decode 内部 Signal。
I、IM、IMC 共用同一模型，通过配置选择扩展；运行环境支持退出及整数、字符、字符串输出。
以下为 **2026-10-15** 的最终验收与后续速度复测结果。

## 正确性验证

验证从汇编／freestanding C 生成 ELF，将相同机器码、初始寄存器和内存送入完整 CPU 与原生 RV5S。
逐拍比较取指 PC、各级有效 PC、停顿／冲刷、前递选择、寄存器、RAM、访存宽度和事件、退休及 ECALL 事件；
另用独立顺序预期、自检通过码和算法预设答案核对退休序列、最终状态、退出状态及输出。

| 程序验证配置 | 项数 | 与原生严格一致 | 已知参考缺陷 | 参考不支持 |
| --- | ---: | ---: | ---: | ---: |
| 默认模型，传完整 Control | 79 | 74 | 3 | 2 |
| 只传 `control.field` 的接口副本 | 10 | 9 | 1 | 0 |
| 合计 | 89 | 83 | 4 | 2 |

10 项为已有程序在接口副本上的复测，合计仍是 **79 个不同程序**。83 项严格通过共对照 **6,419 拍**。
所有 89 项均通过独立程序预期、生成 C++、事务 IR／RTL 友好 IR 解释及 Source／Transaction／RTL 三阶段独立重载检查；
其中 24 项边界程序另通过 C++ ASan／UBSan。全仓 **93 项编译器回归全部通过**，没有新增失败。

参考差异单列，不计为严格通过：JALR 未清最低位、EBREAK 被当作 ECALL、`rvc` 零填充区域的取指长度差异；
其中 `rvc` 在两种接口下重复出现，合计 4 条缺陷记录。参考不支持的两项为 C.EBREAK 和非法指令陷阱。
本例保留 ISA 语义，没有为迁就参考而改写 CPU 行为。之前发现的 ELF 漏段、store 宽度漏检、错误路径越界取指、
store 后字符串 ECALL 采样四项问题均已修复并加入回归。

完整 Control 与三种标量／字段接线共八种配置另完成每种 64 拍对照；各配置 C++、两层 IR 和原生结果一致。
这里的 RTL 友好层是 **IR 解释执行，尚无真正 RTL 仿真**。
原生参考固定于 Ripes `5b8a616edcb6f0a2ddb07e78951348b72497f1e1`、VSRTL `8497dd14fe80e57efcff4c424a9a3b6363d93eb7`。

证据：[最终验收汇总](results/field-analysis/final-verification.json)、[89 项逐项结果](results/field-analysis/release-acceptance/summary.json)、
[编译器修复报告](results/field-analysis-2026-10-15.md)、[既有审查问题与修复](results/fixes-2026-10-15.md)。

## 运行速度

使用同一个混合循环汇编源，分别正常生成 IM（96 字节）和 IMC（82 字节）程序。
预热 1,000 周期后计时 100,000 周期，重复五次取中位数；生成 C++ 使用 `-O3`。
只计 CPU 推进周期的时间，编译、初始化、逐拍 trace 和最终状态输出均不计入。

| 配置 | 本例生成 C++（周期/秒） | 原生 Ripes（周期/秒） | 相对原生速度 | 本例十万周期耗时 |
| --- | ---: | ---: | ---: | ---: |
| IM | **220,580** | 144,387 | **1.53 倍** | 0.453 秒 |
| IMC | **191,108** | 141,287 | **1.35 倍** | 0.523 秒 |

每次运行的最终寄存器、RAM、退休数量及 PC 均与原生一致；窗口内退休 75,000 条指令，表中单位是仿真周期/秒。
这些结果代表该混合循环。历史 `-O3` 记录为 IM 373,004、IMC 36,100 周期/秒：当前 IMC 改善，IM 下降；
历史模型早于后续审查修复，尚未隔离造成变化的原因，不能把差值全部归因于编译器补丁。
本次前端至产物生成另耗时 IM 41.15 秒、IMC 44.90 秒，生成 C++ 的 `-O3` 编译分别约 5.16 秒、5.06 秒；生成过程仍有待优化。

证据：[速度复测说明](results/field-analysis-performance-2026-10-15.md)、[原始计时与汇总](results/field-analysis/performance/summary.json)、
[程序、模型及工具校验和](results/field-analysis/performance/provenance.json)。

## 手写前端代码量对比

本例统计 [model.py](model/model.py)（324 行）与 [isa.py](model/isa.py)（393 行），包含五级模型、载荷类型、
译码、乘除法、解压、访存及环境事件。原生统计从 `rv5s/rv5s.h` 出发，在 `src/processors/RISC-V/` 内递归引用到的
**21 个 CPU 头文件**，每个文件只计一次；包括 RV5S 顶层、流水寄存器及其基类、ALU、译码、控制、解压、访存、寄存器堆、前递和冒险单元。

| 统计口径 | 本例 ACPy 前端 | 原生 RV5S C++ 头文件 | 本例减少 |
| --- | ---: | ---: | ---: |
| 全部物理行，含空行及注释 | **717** | **2,939** | **2,222 行，75.6%** |
| 非空物理行，含注释 | **615** | **2,576** | **1,961 行，76.1%** |

按全部物理行计，本例手写前端约为原生 CPU 头文件的 **24.4%**。双方都排除通用框架（ACPy 编译器／VSRTL／Qt）、
测试与原生运行器、Demo、GUI 布局和生成产物。原生头文件仍包含 RV64 分支及 Ripes 界面观察接口，
本例仅实现约定的 RV32IMC 与运行环境；这是明确文件范围内的文本规模比较，不是完全同功能的最小实现比较。

逐文件行数、文件清单、SHA256 和统计口径见[代码量原始数据](results/field-analysis/source-size/summary.json)。复现：

```bash
python -m pycircuit.examples.ripes5.tests.benchmarks.source_size
```

生成代码另计：同一程序字节的开关对照中，完整 Control 的 IMC C++ 从修复前 53,671 行降到 5,364 行；
开启 C 的代码增量从 47,474 行降到 469 行。该数值不计入上表的手写前端代码量，详见[四种接线对照](results/field-analysis-2026-10-15.md)。

## 文件与复现入口

统一程序测试流程仍在本示例内探索，尚未迁移到 `@ac.system`。

| 目录 | 内容 |
| --- | --- |
| [model/](model/) | ACPy 五级 CPU、ISA helper 和可读 Demo |
| [tests/](tests/README.md) | 统一程序测试入口、比较器与顺序预期 |
| [tests/programs/](tests/programs/) | 独立汇编／C 程序；配置见 [cases.json](tests/cases.json) |
| [tests/native/](tests/native/) | 原生 RV5S runner 与构建脚本 |
| [tests/benchmarks/](tests/benchmarks/) | 性能 benchmark 脚本 |
| [tests/audit/](tests/audit/) | 审查与复测脚本 |
| [tests/fixtures/](tests/fixtures/) | 历史原生轨迹与已记录差异 |
| [tests/licenses/](tests/licenses/) | 上游许可证与来源 |
| [results/](results/) | 报告、覆盖表、程序清单与运行结果入口 |

I、IM、IMC 共用 [model/model.py](model/model.py) 和 [model/isa.py](model/isa.py)，通过配置启用扩展，没有两套独立模型，因此不再拆 IM／IMC 子目录。

结果集中入口：

- [字段依赖分析与 C++ 膨胀修复](results/field-analysis-2026-10-15.md)：通用编译器修改、四种接线对照与最终验收。
- [修复后的仿真速度](results/field-analysis-performance-2026-10-15.md)：无 trace 的 IM／IMC 计时及原生 Ripes 对照。
- [当前修复验证](results/fixes-2026-10-15.md)／[全部 79 项结果](results/standard/fixes-20261015/full-summary.json)。
- [标准流程](tests/README.md)、[本轮标准化记录](results/standardization.md)、[缺陷状态](tests/known-issues.json)。
- [审查报告](results/review-2026-10-15.md)、[原用例报告](results/rv32imc-report.md)、[实现和复现说明](results/implementation.md)。
- [指令覆盖](results/instruction-coverage.md)、[程序清单](results/program-manifest.json)、[上游纳入／排除清单](results/upstream-inventory.json)。
- [历史端到端原始数据](results/acceptance/)、[历史性能测量](results/performance/)、[审查反例与复测](results/audit/)。
- [默认生成产物](results/generated/)、[原生构建来源](results/native-build/provenance.json)。

原始数据仍位于 `pycircuit/builds/benchmarks/`，生成产物仍位于 `pycircuit/builds/`；`results/` 使用相对符号链接集中访问，保留历史证据中的原始路径。外部参考源码仍在仓库 `reference/`。

从仓库根目录运行（编译器环境设置见 [pyCircuit 说明](../../README.md)）：

```bash
python -m pycircuit.examples.ripes5.tests.native.build
python -m pycircuit.examples.ripes5.tests list
python -m pycircuit.examples.ripes5.tests run
python -m pycircuit.examples.ripes5.tests run --case compressed_calls --check all
python -m unittest pycircuit.tests.test_ripes5 pycircuit.tests.test_rv32imc
```

全仓测试发现入口仍保留在 `pycircuit/tests/`，实际 Ripes5 测试代码集中在本例 `tests/`。当前四项审查发现及修复状态在 `tests/known-issues.json`；历史报告不覆盖。搬迁验证见[验证记录](results/reorganization-verification.md)；搬迁清单与模型校验和见 [reorganization.json](results/reorganization.json)。
