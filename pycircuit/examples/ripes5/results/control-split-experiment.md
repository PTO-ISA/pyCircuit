# Control 拆分对照实验

后续通用编译器修复及四种接线复测见[字段分析修复](field-analysis-2026-10-15.md)。以下为修复前对照记录。

结果：在当前 Ripes5 模型中，仅把 Control 接口拆成独立标量参数，就消除了此前测到的
两个条件排序环，开启 C 后不再发生同量级的 C++ 膨胀。
实验未修改生产模型或编译器。

## 输入与改动

使用固定循环程序的同一份 96 字节机器码、相同入口、初始寄存器和 RAM，分别关闭/开启 C。
共比较六组：

- packed：原始 Control 参数。
- scalar_all：Control 的七个字段全部改成独立参数，每个消费者仍接收全部七个字段。
- scalar_needed：独立参数，只传消费者实际使用的字段。

两个拆分版本都保留原来的字段计算、Rule 边界、状态与算法。ForwardedOperands、ExecutionResult
等其他 payload 保留。所有 Rule/Module 的 Control 输入都已拆开；只在 observe 内重建
last_control 记录，供相同格式的逐拍观测，不再把该记录传入其他 Rule/Module。
因此 scalar_all 是直接检验去掉 Control 聚合的对照，scalar_needed 另外检验减少无关输入。

实验模型、ISA 快照及差异文件保存在构建目录的 models/ 下。AST 变换可由测试脚本重建，
不复制修改生产源码。

## 测量结果

| 版本 | C 关闭：C++ 行数 | C 开启：C++ 行数 | C 开启：Fetch 行数 | C 开启：条件排序环数 |
| --- | ---: | ---: | ---: | ---: |
| packed | 6,197 | 53,671 | 50,172 | 2 |
| scalar_all | 2,940 | 3,513 | 865 | 0 |
| scalar_needed | 3,066 | 3,639 | 865 | 0 |

scalar_all 中，开启 C 的整体 C++ 行数仅增加约 19.5%，原模型约为 8.66 倍。
与 packed 的 C 开启版本相比，其整体 C++ 减少约 93.5%，Fetch 减少约 98.3%。
scalar_needed 的整体文件略大于 scalar_all，不能把参数数量减少直接等同于更小代码；
本次主要结论是消除巨大递归展开，而非两个拆分接口之间的性能优劣。

| 版本/配置 | 事务 IR 操作数 | RTL 友好 IR 操作数 |
| --- | ---: | ---: |
| packed / C 关闭 | 2,332 | 30,710 |
| packed / C 开启 | 3,031 | 31,397 |
| scalar_all / C 关闭 | 2,374 | 30,744 |
| scalar_all / C 开启 | 3,073 | 31,431 |
| scalar_needed / C 关闭 | 2,378 | 30,757 |
| scalar_needed / C 开启 | 3,077 | 31,444 |

拆分后 IR 没有大幅减少，C++ 却显著缩小；这与此前定位到的后端条件排序/递归回退路径相符。
已有诊断编译器独立重载六份事务 IR：packed 各测到两个环，拆分版本均未测到环，
诊断版输出与各组正常编译器的 C++ 头文件逐字节一致。

## 行为验证与边界

六组各运行 64 拍，完整状态和 Rule fire 轨迹逐拍相同；每组生成的 C++、事务 IR
解释执行、RTL 友好 IR 解释执行一致。全部六组还与当前原生 Ripes RV5S 逐拍对上，
使用已有严格比较器核对 PC、流水状态、前递/冲刷/使能、寄存器、内存及事件。

输入是无结束标记的循环程序。原生运行器采样初态及 64 拍后按周期上限返回其已知的
`end marker did not retire within max_cycles`；脚本严格要求该返回原因及 65 条完整样本，
不将此记为程序正常退出。raw stdout/stderr 和结束原因保存在 reference/ 与 reference-summary.json。

这是同字节、固定 64 拍的结构与行为对照，不是完整程序清单验收；这份程序的机器码均为
32 位，因此开启 C 的配置没有实际执行压缩编码。本轮未做真实 RTL 仿真、动态速度测量或
完整编译器回归，也没有将实验模型替换为默认模型。

结论限于当前模型与编译器：Control 聚合是本例触发巨大回退展开的重要因素，
甚至无需去掉多余输入，标量化就足以避开它。该实验不证明任意标量接口都不会产生条件环，
也不代替通用后端修复。

## 复现

在仓库根目录运行：

```sh
env ACPY_MLIR_COMPILER="$PWD/pycircuit/builds/compiler/mlir/acir-compile" \
    ACPY_MLIR_OPT="$PWD/pycircuit/builds/compiler/mlir/acir-opt" \
    LD_LIBRARY_PATH=/home/lc/opt/gcc14/lib \
    python -m pycircuit.examples.ripes5.tests.benchmarks.split_control
```

脚本：`pycircuit/examples/ripes5/tests/benchmarks/split_control.py`。
结果：`pycircuit/builds/benchmarks/ripes5-standard/control-split-20261015/summary.json`。
所有输入、六组模型/分阶段 IR/C++/轨迹/诊断日志和原生参考窗口保存在同一结果目录。
summary 保存生产源码 SHA256、输入校验和、脚本 SHA256、计数和逐拍对比状态；
production_sources_unchanged 核对生产模型/ISA/C++ lowerer/emitter 未被本次实验修改。
