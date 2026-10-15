# 字段分析修复后的仿真速度

使用现有 `tests/benchmarks/measure.py` 和 `runner.cpp` 重测，只改变输出目录，未改生产模型或编译器。
原始证据位于 [performance/](field-analysis/performance/)，包含输入、生成代码、每次计时、
原生最终状态和源码／工具校验和。执行包装脚本为 `field-analysis/run-performance.py`。

每个配置先预热 1,000 周期，再计时 100,000 周期，重复五次取中位数。
只计 `Model::tick()` 循环；编译、初始化、trace 输出和最终状态序列化不在运行计时内。
原生 Ripes 使用相同预热及计时窗口。IM 文本 96 字节，IMC 文本 82 字节，
各自输入校验和与历史性能输入相同；两者使用同一汇编源、各自 ISA 的正常汇编结果。
这与隔离 C 开关的“相同机器码字节”代码规模实验不同。

| 配置 | 当前 C++ -O1 周期/秒 | 当前 C++ -O3 周期/秒 | 当前原生 Ripes 周期/秒 | -O3 / Ripes |
| --- | ---: | ---: | ---: | ---: |
| IM | 218,416 | 220,580 | 144,387 | 1.53 倍 |
| IMC | 193,276 | 191,108 | 141,287 | 1.35 倍 |

`-O3` 推进十万周期分别用 0.453 秒和 0.523 秒。计时窗口内均退休 75,000 条指令，
因此表中单位是仿真周期/秒，不能直接称为指令/秒。
每次 C++ 运行的最终寄存器、RAM、退休数量及 PC 都与对应原生运行一致。
这是单个混合循环的动态速度，没有运行两层 IR 解释器的性能 benchmark，也没有真正 RTL 仿真。

历史 `rv32imc-performance/summary.json` 的 `-O3` 为 IM 373,004 周期/秒、IMC 36,100 周期/秒。
当前 IMC 明显快于历史记录，IM 则低于历史记录。历史性能产物早于后续 CPU 审查修复，
没有本次完整的源码冻结证据；这里不能把差值归因于单独的编译器补丁，也不能宣称所有配置全面提速。

生成耗时另计：本次 Python 前端至产物的 analyze 用时 IM 41.15 秒、IMC 44.90 秒；
生成 C++ 的 `-O3` 编译约为 5.16 秒、5.06 秒。历史生成耗时为 5.90 秒、6.58 秒。
当前生成过程较慢，需要进一步定位；它不计入上表的仿真速度，也说明代码规模缩小并不保证生成过程更快。

复现命令（仓库根目录，环境按 pyCircuit README 配置）：

```bash
ACPY_MLIR_COMPILER="$PWD/pycircuit/builds/benchmarks/field-analysis-fix/release-tools/acir-compile" \
ACPY_MLIR_OPT="$PWD/pycircuit/builds/benchmarks/field-analysis-fix/release-tools/acir-opt" \
LD_LIBRARY_PATH=/home/lc/opt/gcc14/lib \
python -m pycircuit.builds.benchmarks.field-analysis-fix.run-performance
```

脚本保存原始测量代码，仅改 `OUT`，并检查测量前后相关源码和冻结工具的校验和一致。
