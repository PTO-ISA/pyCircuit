# 乱序 CPU 验收报告

记录时间：2026-10-03T05:40:09.131010+00:00。详细指纹和配置见 [results.json](results.json)。

20 场景 × 4 个调度配置 × 2 条生成路径 = 160 次 Release 运行，另有 160 次 ASan/UBSan 运行。每次均逐条对照独立顺序解释器，同一构建的八份完整轨迹逐拍一致；两个编译器构建的完整轨迹另行逐拍比对。

现有工程完整 CTest：11/11 通过。Sanitizer 下新示例 CTest：3/3 通过。

## 程序结果

| 场景 | 周期 | 提交数 | IPC |
| --- | ---: | ---: | ---: |
| congestion | 232 | 55 | 0.237 |
| dual_issue | 17 | 5 | 0.294 |
| fetch_bounds | 15 | 2 | 0.133 |
| fetch_fault | 15 | 2 | 0.133 |
| inflight | 205 | 38 | 0.185 |
| invalid | 8 | 2 | 0.250 |
| isa | 49 | 14 | 0.286 |
| latency | 18 | 7 | 0.389 |
| memory | 67 | 14 | 0.209 |
| mixed_2026 | 530 | 225 | 0.425 |
| mixed_65537 | 389 | 222 | 0.571 |
| mixed_7 | 361 | 214 | 0.593 |
| precise_load_fault | 10 | 2 | 0.200 |
| precise_store_fault | 13 | 2 | 0.154 |
| recovery | 457 | 158 | 0.346 |
| rename | 286 | 101 | 0.353 |
| memory_blocked | 168 | 14 | 0.083 |
| mixed_2026_blocked | 1082 | 225 | 0.208 |
| recovery_blocked | 931 | 158 | 0.170 |
| rename_blocked | 719 | 101 | 0.140 |

提交数包含 halt 或导致停止的精确错误事件。

## 实际触发的行为

以下计数只汇总每个场景的一份基准轨迹，未将八种配置重复累加。

| 轨迹证据 | 次数／周期数 |
| --- | ---: |
| out_of_order_issue | 828 |
| out_of_order_complete | 869 |
| dual_issue | 141 |
| full_window | 2539 |
| memory_busy | 1168 |
| result_backpressure | 2552 |
| request_backpressure | 1861 |
| wakeup_under_backpressure | 501 |
| flush_inflight | 139 |
| slot_reuse | 1366 |
| epoch_slot_reuse | 1192 |
| squashed_memory_fault | 48 |
| squashed_store | 60 |
| operand_survives_reuse | 104 |
| load_waited_for_store | 1400 |
| three_tick_memory | 377 |
| one_tick_integer | 1786 |
| max_occupancy | 8 |

## 宿主计时

机器 aarch64，固定 CPU 0，一次预热、七次串行轮换采样。只计 tick 循环（含结束检查和首次 Signal 初始化），排除构造、输入、快照和 JSON。

下表选取直接生成、Module 正序的中位数；完整 24 组及全部样本见 [timing.json](timing.json)。

| 程序 | 缓存 | ns / cycle |
| --- | --- | ---: |
| congestion | 开 | 6261.2 |
| congestion | 关 | 6201.9 |
| latency | 开 | 17064.0 |
| latency | 关 | 17011.8 |
| mixed_2026 | 开 | 11275.5 |
| mixed_2026 | 关 | 11177.3 |

短程序的每周期开销包含首次初始化，不能将这组数值视为硬件周期时间。

## 代码量与限制

| 项目 | 行／字节 |
| --- | ---: |
| hardware_acpy | 450 |
| host_tools_tests | 835 |
| runner_cpp | 177 |
| repro_sources | 48 |
| assembly | 358 |
| generated_cpp | 18278 |
| generated_hpp | 347 |
| acir_bytes | 2642297 |

行数包含注释和空行。硬件行数不重复计算复用的 Ripes5 译码文件；生成行数不包括 GFSim 和 ac_support.hpp。

编译器和 GFSim 保持原样。F1 构造常量在条件区域错误复用的缺陷仍然存在，本模型通过循环前显式转换表达相同组合逻辑，最小复现及影响见 [findings.md](findings.md)。

没有未完成的计划内功能；不包含 trap/CSR/中断、缓存、访存推测和 Store 转发。

复现命令见 [README.md](README.md)。测试日志及首个差异上下文保存在 `output/`，该目录不纳入版本管理。
