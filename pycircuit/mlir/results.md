# MLIR 迁移验收与性能

> 本文保留静态调度重构前的实现／测量记录。当前 C++ 调度契约见 [GFSim spec](../../gfsim/spec.md)；Python 实验引擎不随本次重构迁移。

记录日期：2026-10-04。环境：aarch64、Clang/LLVM/MLIR 22.1.8、GCC 14 sysroot、Python 3.11。

## 实现与验证

- ODS 定义见 [ACIR.td](ACIR.td)，三个注册 pass 见 [Passes.td](Passes.td)。保存格式为 MLIR v2，旧 HIR、JSON ACIR 和 guard 展平路径已删除。
- Release CTest：14/14 通过；语言套件含 25 个测试方法。涵盖 ODS 验证、读取依赖不可删除、删源码后重载、固定输出、临时资源列表生命周期、静态 Signal 全输入、动态字段 revise 等。
- Ripes5 保持既有五方逐拍对照；既有 OoO、GFSim 原生语义与独立安装链接回归通过。
- 完整 Queue CPU：5 程序 × 2 个写回配置 × 3 个生成版本 × 缓存开关 × Module 正反序 = 120 次完整运行。逐条提交对照独立 RV32I 解释器，变体逐拍一致。实际覆盖 ROB=12 填满、乱序发射/完成、槽位复用及在途恢复。runner 还检查每个保留站的唯一 pop/push 来源。
- ASan/UBSan：14/14 通过，包括完整 CPU 的另外 120 次运行；语言套件 25 项通过。开启 detect_leaks 与 halt_on_error，未发现 sanitizer 错误。

详细验证摘要与源码指纹见 [verification.json](verification.json)。Sanitizer 日志：`/tmp/acpy-mlir-asan-ctest-accepted.log`。Release 日志：`/tmp/acpy-mlir-ctest-accepted.log`。大型 CPU 轨迹与二进制指纹位于 `/tmp/acpy-mlir-build/examples/skyzh_ooo/evidence/`。完整命令见 [构建说明](../README.md)。

## 同模型新旧编译链

旧编译器来自重构前快照 `/tmp/acpy-mlir-baseline`，仓库基线提交 `1f5edc0179a475c55f99f1d325170f5db79c9079`，旧构建 `/tmp/acpy-before-mlir` 的既有原生回归 12/12 通过。新旧 Ripes5 使用相同 Clang 与 `-O3 -DNDEBUG -std=gnu++20`。

五个长程序先逐拍核对并检查独立计算的最终架构值，再以 K=1024 预热、固定 N 拍计时；每版预热一个进程，CPU 191 上串行轮换七次。下表为中位数。全部样本、构建选项和二进制指纹保存在 [migration-benchmark.json](migration-benchmark.json)。

| 程序 | 测量 tick | 旧 ns/tick | MLIR ns/tick | MLIR 变化 |
| --- | ---: | ---: | ---: | ---: |
| independent_integer | 142979 | 2778.7 | 2560.5 | -7.9% |
| forwarding_chain | 142979 | 2816.0 | 2613.1 | -7.2% |
| branch_flush | 142979 | 2645.5 | 2383.0 | -9.9% |
| load_use | 190979 | 2661.0 | 2401.1 | -9.8% |
| consecutive_memory | 190979 | 2961.7 | 2735.6 | -7.6% |

| 编译指标（3 次中位数） | 旧 | MLIR |
| --- | ---: | ---: |
| 前端与发射 | 197.02 ms | 229.72 ms |
| 生成 C++ 编译 | 4.27 s | 5.18 s |
| 前端进程峰值 RSS | 20.28 MiB | 19.50 MiB |
| C++ 编译峰值 RSS | 144.66 MiB | 154.93 MiB |
| 生成文本总字节（cpp/hpp/support） | 106517 | 81354 |

本批运行时 ns/tick 降低约 7–10%，生成文本减少约 24%；前端与 C++ 编译均变慢。文本统计包含注释，不等同于机器码大小。Ripes5 两版整进程峰值 RSS 均约 14,100 KiB；RSS 包含进程启动，不代表模型本身的净分配。编译耗时包含启动与工具调用，不含首次构建 LLVM/ACIR 编译工具。

## Queue CPU 与原生 skyzh

两个模型都先通过 4096 次循环的完整程序架构检查，再测从初态开始的固定 N 次 step()/tick()（K=0）。构造、装载、结束判断、快照均在计时外；首次 Signal 初始化属于计时范围。统一 Clang、`-O3 -DNDEBUG -std=gnu++20`，CPU 191，预热一个进程后串行轮换七次。每次测速后再次检查全部寄存器与变化的内存字节。

原生参考固定于 `8989a09c357a69b68612f653380d60816f5176c2`。其 ROB 可用容量为 7、内存 4 MiB；Queue 模型分别为 12、256 KiB，流水和恢复策略也不同。该比较不能解释为调度器单独的性能比。

| 程序 | 模型 | 提交指令 | cycles | IPC | ns/tick | 架构指令/秒 | 峰值 RSS KiB |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| window | Queue/GFSim | 40967 | 61486 | 0.666 | 6906.5 | 96471 | 50436 |
| window | skyzh | 40967 | 40991 | 0.999 | 326.3 | 3062811 | 19056 |
| branches | Queue/GFSim | 12302 | 24662 | 0.499 | 5249.2 | 95029 | 50384 |
| branches | skyzh | 12302 | 12328 | 0.998 | 342.8 | 2911117 | 19120 |

Queue 模型仍显著慢于原生参考。本次修复了 Work 反复复制资源指针表的问题：初测 window 为约 56.5 μs/tick，最终约 6.91 μs/tick；仍有框架、候选管理及模型表达开销，尚未做逐项归因。构造约需数秒，虽然不计入 tick 耗时，仍计入整进程耗时与峰值 RSS。

全部样本及实际生成代码指纹见 [skyzh-benchmark.json](skyzh-benchmark.json)，重现命令见 [Queue CPU README](../examples/skyzh_ooo/README.md)。原生 AUIPC、SRAI、LB 和重叠访存差异仍独立记录于 [findings](../examples/skyzh_ooo/findings.md)，不作为新 CPU 的预期结果。

## 当前边界

资源数组首版一维、构造时定长；动态索引按绑定视图的可能集合声明。不同消费者使用固定子列表表达各自资源范围，编译器不推导任意整数路径条件。运行时循环支持非负 range 的单位步进，未实现 while/break/continue。保留少量通用配置、值转换和断言适配操作；未实现 RTL 后端。
