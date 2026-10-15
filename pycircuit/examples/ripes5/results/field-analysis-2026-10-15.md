# 字段依赖分析与 C++ 膨胀修复

验收完成后补做的动态计时见[仿真速度复测](field-analysis-performance-2026-10-15.md)。下文保留修复验收时的测量范围。

本轮只修改通用编译器、回归与对照脚本。默认 CPU 模型和 ISA helper 保持原样。
`fetch(control.stall, control.ecall_stall, ...)` 现在按连接字段传播普通载荷就绪条件；
完整 `fetch(control)` 也继续支持，条件排序回退不再按分支路径递归复制计算。

## 原因和修改

原有事务提取先把整个 Control 的就绪条件合并，再从中取字段，导致 stall 的消费者
继承其他成员的来源。C++ 需求分析额外加入条件排序边之后，可能形成条件依赖环，
即使实际 SSA 数据计算没有环。旧回退代码沿条件分支反复展开共享计算，IMC 的 Fetch
被放大到 50,172 行。这是编译器分析和回退生成的问题，无需用户拆掉所有结构体。

修复分为三部分：

1. 事务提取为纯组合 struct/tuple 保留逐字段的就绪条件及生产者信息，经过别名、参数、
   导出、嵌套字段、结构化分支合流和静态字段更新传播。资源读取保持完整快照；未知动态路径保守处理。
2. 公共 `acir-project-fields` pass 在事务阶段解析字段的 SSA 来源，供 C++、RTL 生成与
   IR 解释共同使用；独立 Source/Transaction IR 重载也执行相同归一化。无 CPU 专用路径。
3. 条件排序有环或布尔需求分析超过预算时，C++ 改为局部按需计算 DAG；重复节点局部缓存。
   未选分支不执行，不跨状态发布复用；真正的数据环与 Rule 调度环仍按原有规则报错。

显式 assert、声明的 Rule guard、事务原子性和跨事务生产者限制仍保留。
自动生成的 return 覆盖检查与显式 assert 在内部区分，避免把无关字段的 Signal 就绪条件
错误提升成全局强制条件。详情见[编译器设计说明](../../../docs/field-projection.md)。

## 同一输入的接线对照

本实验使用相同的 96 字节、未压缩机器码循环，分别启用 IM 和 IMC，以隔离启用解压逻辑
造成的生成规模变化。四种前端接线都是实验副本，没有替换默认模型：

- packed：传完整 Control。
- scalar_all：移除 Control 参数，传全部七条标量信号。
- scalar_needed：移除 Control 参数，各模块只传用到的标量。
- field_arguments：保留顶层 Control，接口只接 `control.field` 中实际需要的成员。

旧编译器的 packed 为 IM 6,197 行、IMC 53,671 行，IMC Fetch 为 50,172 行。
原始证据见 `pycircuit/builds/benchmarks/ripes5-standard/control-split-20261015/summary.json`。
最终编译器的四种接线数据见本报告旁 `field-analysis/release-wiring/summary.json`。

| 接线方式 | IM C++ 行数 | IMC C++ 行数 | IMC Fetch 行数 | 条件环计数（IM/IMC） |
| --- | ---: | ---: | ---: | ---: |
| packed | 4,895 | 5,364 | 746 | 1 / 1 |
| scalar_all | 4,229 | 4,698 | 742 | 0 / 0 |
| scalar_needed | 4,195 | 4,664 | 742 | 0 / 0 |
| field_arguments | 4,673 | 5,142 | 759 | 1 / 1 |

在这个相同程序输入上，开启 C 的头文件增量从旧 packed 的 47,474 行降为 469 行；
最终四种接线的增量都是 469 行。这里衡量的是静态生成代码规模。

每种配置检查 64 拍完整状态和 fire，C++、事务 IR、RTL 友好 IR 逐拍一致，并与新采集的
原生 RV5S 严格对照。该输入是无限循环，原生运行达到周期上限是预期的窗口结束，
不把它算作程序正常退出。所有配置的输入、模型副本、差异和原生轨迹保留在实验目录。

保留完整 Control 仍可能出现条件需求排序环；本轮没有宣称消除所有这种环。
字段参数对照验证精确来源分析，有界回退保证保守情形也不会重复展开到原来的规模。
field_arguments 的剩余回退位于共享就绪计算中，Fetch 本体没有局部递归展开。

## 通用规模与语义回归

独立于 CPU 的嵌套条件 DAG，在同一源码输入上得到：

| 嵌套深度 | 旧 C++ 行数 | 修复后 C++ 行数 | 最终回退原因 |
| --- | ---: | ---: | --- |
| 4 | 834 | 306 | 条件排序环 |
| 8 | 10,922 | 398 | 条件排序环 |
| 12 | 未运行 | 490 | 需求分析预算 |

深度 4/8 的最终状态与旧编译器相同；所有最终用例 C++ 与两层 IR 一致。
ASan/UBSan 及 reset 验证未选中的除零分支保持惰性。
数据及可复现脚本分别为 `field-analysis/release-growth/summary.json`、
`field-analysis/release-growth.py`；不据此推断动态仿真速度。

新增通用回归覆盖普通字段独立性、分支式 Signal 返回、嵌套字段及静态更新、显式 guard/assert、
effectful 返回值不能绕过跨事务限制，以及独立事务 IR 中嵌入记录数据的 assertion。
含原始／优化模式、Source/Transaction/RTL 三阶段独立重载、生成 C++ 和 sanitizer。

## 完整验收与证据位置

最终验收使用冻结的 `field-analysis/release-tools/` 工具，避免测试期间覆盖正在运行的二进制。
只使用 `release-*` 的最终证据；目录内早期试验、线程并发失败和中途重建造成的失败日志
保留作过程记录，不作为最终验收结论。

最终编译器回归与程序验收结果记录在 `field-analysis/final-verification.json`。
全仓发现的 93 项测试全部通过，分成四个互不重复的模块组执行，并核对与 unittest discover 的
测试集合完全一致。完整程序结果如下：

| 模型接口 | 项数 | 严格通过 | 已知参考缺陷 | 参考不支持 |
| --- | ---: | ---: | ---: | ---: |
| 原有完整 Control | 79 | 74 | 3 | 2 |
| control.field 参数副本 | 10 | 9 | 1 | 0 |
| 合计 | 89 | 83 | 4 | 2 |

83 项严格通过共对照 6,419 拍；所有 89 项的独立程序预期和编译器多后端检查均通过。
24 项边界程序执行了 C++ ASan/UBSan。默认示例另外通过保存的原生 25 拍轨迹及三阶段重载。
没有新失败，也没有把已有参考差异计作严格通过。

标准程序验证复用现有 `run_case(check='all')`，包括独立预期、自检通过码、原生采集、
C++、两层 IR 和三阶段独立重载。边界程序另运行 C++ ASan/UBSan。
完整 Control 运行原有全部 79 个程序，field_arguments 运行 8 个 smoke、rvc 和 IMC 矩阵程序，共 89 项。
参考缺陷及不支持情况继续单独分类，未增加自动忽略规则。

这里的 RTL 友好层验证是 **IR 解释执行**，没有真正 RTL 仿真。
没有重测动态吞吐、编译耗时或峰值内存；生成规模下降不等于相同幅度的速度提升。

从仓库根目录复现（先按 pyCircuit README 配好 LLVM/MLIR/C++ 环境）：

```bash
export ACPY_MLIR_COMPILER="$PWD/pycircuit/builds/benchmarks/field-analysis-fix/release-tools/acir-compile"
export ACPY_MLIR_OPT="$PWD/pycircuit/builds/benchmarks/field-analysis-fix/release-tools/acir-opt"
export LD_LIBRARY_PATH=/home/lc/opt/gcc14/lib
export ASAN_OPTIONS=detect_leaks=0
python -m unittest discover -s pycircuit/tests -v
python -m pycircuit.examples.ripes5.tests.benchmarks.split_control --output pycircuit/builds/benchmarks/field-analysis-reproduce/wiring
python -m pycircuit.examples.ripes5.tests.benchmarks.verify_fields --workers 12 --output pycircuit/builds/benchmarks/field-analysis-reproduce/acceptance
python -m pycircuit.examples.ripes5.tests.verify --case load_branch_wrong_path --reload --output pycircuit/builds/benchmarks/field-analysis-reproduce/default
```

`release-acceptance/provenance.json` 保存源码校验和并在结束时检查未改变；现有 runner 的
`result.json` 保存输入、ELF 和原生二进制校验和，`final-verification.json` 汇总冻结编译器及产物校验和。
默认 `builds/ripes5/` 生成物也已用最终工具重建。
