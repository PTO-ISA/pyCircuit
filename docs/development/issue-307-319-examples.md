# #307 / #319：示例适用性与迁移清单

2026-10-18，依据用户最新要求重新审查时效性、独立教学价值和测试负担。
历史 71 个目录不再是必须逐个保留、增加脚手架或扩展编译器的目标。

## 本轮决定与实现边界

- 推荐教学主线 7 个，完整设计/专项参考 10 个；已落到 examples/GUIDE.md。
- 21 个合并教学入口、31 个主要属于 API 回归、2 个退出过时 JIT 定位。
- 合并讲解不等于合成一个巨大 DUT；退出推荐入口不等于行为测试已迁移或通过。
- 54 个旧目录已完整搬到 `tests/fixtures/historical_examples/`，保留源码、oracle、刺激和历史收据；当前 catalog/CMake 只注册 17 个保留例。
- 现有显式 API/package 引用已重定向；25 条 API coverage 映射未变。其余归档资产尚不等于 API 自动执行覆盖。
- DMA/memory_banks 保留并简化 System；Memory Bank 改为一个内聚的阶段 FSM Rule，独立 Module 时序 oracle 保持通过。

## 保留的推荐入口

| 例子 | 独立用途 |
| --- | --- |
| [hello_counter](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/hello_counter/README.md) | 最小 System 与持久状态 |
| [multi_outputs](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/multi_outputs/README.md) | Module 层次与独立 typed 输出 |
| [struct_transform](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/struct_transform/README.md) | Struct 接口与寄存状态 |
| [enum_payload_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/enum_payload_pipeline/README.md) | Enum/Struct 值与不可变快照 |
| [queue_rule_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/queue_rule_pipeline/README.md) | 跨 Module Rule、原子 Queue 传输、SCF 与本地 after |
| [table_rule](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/table_rule/README.md) | 固定 Table 状态与嵌套 Rule |
| [calculator](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/calculator/README.md) | 多个相关状态的顺序控制 |
| [fifo_loopback](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/fifo_loopback/README.md) | 外部 FIFO 协议边界 |
| [multiclock_domain_isolation](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/multiclock_domain_isolation/README.md) | 真实独立时钟域与子实例 |
| [rob](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/rob/README.md) | 有序 Rule、EHR 与提交 |
| [issue_queue_2picker](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/issue_queue_2picker/README.md) | 有依赖的双出队与状态转移 |
| [digital_filter](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/digital_filter/README.md) | 有历史状态的 FIR 数据通路 |
| [dma](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/dma/README.md) | 同步存储器与跨模块事务 |
| [memory_banks](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/memory_banks/README.md) | 多银行路由、存储时序与仲裁 |
| [digital_clock](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/digital_clock/README.md) | 计时状态与长时进位 |
| [dodgeball_game](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/dodgeball_game/README.md) | VGA 时序与游戏状态 |
| [bf16_fmac](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/bf16_fmac/README.md) | 四级定宽数据通路；不是完整 IEEE FMA |

## 合并教学入口，保留不同语义的回归

| 条目 | 教学归宿 | 处理依据 |
| --- | --- | --- |
| [counter](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/counter/README.md)、[obs_points](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/obs_points/README.md) | [hello_counter](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/hello_counter/README.md) | 状态/观察教程；使能、旧值、回绕、X/Z 回归仍由 state/observation API 保留。 |
| [hier_modules](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/hier_modules/README.md)、[module_loop](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/module_loop/README.md) | [multi_outputs](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/multi_outputs/README.md) | 层次与实例隔离教程；显式 DFFE 反馈保留在 standard-leaf API。 |
| [multiclock_regs](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/multiclock_regs/README.md) | [multiclock_domain_isolation](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/multiclock_domain_isolation/README.md) | 合并教学入口，去掉重复的完整 wrap 扫描前先保留独立域差异场景。 |
| [pipeline_builder](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/pipeline_builder/README.md) | [struct_transform](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/struct_transform/README.md) | 当前只是两级 Packet 寄存流水，不是 builder API。 |
| [queue_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/queue_pipeline/README.md)、[rule_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/rule_pipeline/README.md)、[inferred_boundary_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/inferred_boundary_pipeline/README.md)、[struct_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/struct_pipeline/README.md)、[recursive_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/recursive_pipeline/README.md)、[latency_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/latency_pipeline/README.md) | [queue_rule_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/queue_rule_pipeline/README.md) | 基础 Queue 讲解合并；容量、延迟、payload、wrap 等独有矩阵归 queue-source。 |
| [rule_pair_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/rule_pair_pipeline/README.md)、[multi_input_rule_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/multi_input_rule_pipeline/README.md)、[fork_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/fork_pipeline/README.md)、[barrier_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/barrier_pipeline/README.md)、[broadcast_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/broadcast_pipeline/README.md)、[conditional_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/conditional_pipeline/README.md)、[select_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/select_pipeline/README.md)、[feedback_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/feedback_pipeline/README.md)、[loop_control_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/loop_control_pipeline/README.md) | [queue_rule_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/queue_rule_pipeline/README.md) | 作为独立流、join/fork、选择/广播、反馈的少量配方；各自原子性与背压并不等价，不能丢覆盖。 |

## 已归档的独立包装与后续覆盖归属

以下资产已完整归档，表中 API 方向仍是覆盖归属建议，并非全部已接入执行。已有 API nightly 失败的 owner，不能用“已有同名测试”宣称承接完成。

| 条目 | 现有 API 承接方向 | 处理依据 |
| --- | --- | --- |
| [wire_ops](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/wire_ops/README.md)、[arith](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/arith/README.md)、[boundary_value_ports](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/boundary_value_ports/README.md)、[net_resolution_depth_smoke](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/net_resolution_depth_smoke/README.md)、[cache_params](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/cache_params/README.md)、[decode_rules](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/decode_rules/README.md)、[fastfwd](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/fastfwd/README.md) | fixed-width / frontend-branches / direct-call-graph / Module outputs | 主要是运算、接口或展开边界；cache_params 已无参数化，fastfwd 只是多路透传。 |
| [bit_widths](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/bit_widths/README.md)、[masked_decode_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/masked_decode_pipeline/README.md)、[bitfield_scalar_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/bitfield_scalar_pipeline/README.md)、[bitfield_decode_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/bitfield_decode_pipeline/README.md)、[popcount_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/popcount_pipeline/README.md)、[count_zeros_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/count_zeros_pipeline/README.md)、[bit_primitive_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/bit_primitive_pipeline/README.md)、[onehot_encode](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/onehot_encode/README.md) | fixed-slices / fixed-unsigned-shifts / bits-popcount / bits-count-zeros / bits-priority-encoding | 位运算及类型矩阵无需每项另建完整 Queue 示例包。 |
| [record_projection](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/record_projection/README.md)、[record_projection_update](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/record_projection_update/README.md)、[record_spread_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/record_spread_pipeline/README.md)、[nested_payload_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/nested_payload_pipeline/README.md)、[aggregate_payload_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/aggregate_payload_pipeline/README.md)、[recursive_aggregate_payload_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/recursive_aggregate_payload_pipeline/README.md)、[frontend_composition_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/frontend_composition_pipeline/README.md) | record-projection-simplification / defaults-composition / aggregate CodeGen | 保留字段、名义类型、快照和跨特性交互；不以合并单项测试替代完整交互 fixture。 |
| [encoded_enum_pipeline](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/encoded_enum_pipeline/README.md)、[enum_helpers](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/enum_helpers/README.md) | enum-source / enum carrier | 基础 Enum 教学保留 enum_payload_pipeline；编码与异常输入矩阵放 API。 |
| [array_reductions](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/array_reductions/README.md)、[array_combinators](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/array_combinators/README.md)、[bounded_integer_operations](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/bounded_integer_operations/README.md)、[recursive_array_updates](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/recursive_array_updates/README.md)、[array_scans](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/array_scans/README.md) | table-methods / bounded-for / exact integer / aggregate-table | 多数是操作符合集；顺序 prefix、65-lane 更新和23路独立背压不能在归并中遗失。 |
| [bounded_full_u64](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/bounded_full_u64/README.md) | numeric / queue-source | 三路透传加一路常量，主要价值是全 u64 域边界验证。 |
| [traffic_lights_ce_pyc](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/traffic_lights_ce_pyc/README.md) | source behavioral / encoder oracle | 历史显示编码包含三位截断，不能作为普通 BCD/交通灯教程；保留原 FSM 回归，不自动修正其行为。 |

## 过时命名与定位

| 条目 | 处理 |
| --- | --- |
| [jit_control_flow](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/jit_control_flow/README.md) | 退掉 JIT 定位；当前是普通 if/for，讲解归 SCF，回归归 frontend-branches / bounded-for。 |
| [jit_pipeline_vec](https://github.com/PTO-ISA/pyCircuit/blob/main/tests/fixtures/historical_examples/jit_pipeline_vec/README.md) | 退掉 JIT 定位；当前是三段普通寄存流水，讲解归状态流水，原周期契约归 state/hierarchy。 |

## 验证负担与当前结果

收敛前的历史 71 例按主入口 Python 文件统计约 5,241 行；bench.py、driver.cpp、rtl_tb.sv、four_state.py 合计 58,618 行。该口径不计辅助 DUT 源码，行数不是单独的删减依据。实际重复集中在双 Queue 包装、运算穷举和每例重复的生命周期矩阵。

默认示例使用短 System 场景与双后端；手写 C++/RTL 只承担生成 System 无法表达的独有边界。全域数值、IR 篡改、极限资源和穷举四态归 API/nightly。digital_clock 的完整进位场景保留为长时验证，不作为每次作者修改的默认任务。

当前 17 例的 nightly **34/34 PASS**，包括 14 个 Module、16 个 System、3 个四态测试和 clock encoder。DMA/Memory/Table 的新 System 分别为 42/26/16 周期；原长 bench 保存在 `system_benches/`，未以新短场景的通过宣称原长场景通过。digital_clock 保留并通过 100,000,001 周期。14 份 Module 生成收据已更新。

API gate **23/23 PASS**；unit **589 passed / 5 skipped / 82 deselected**；example build、Runtime-only install、CompilerDev consumer 定向检查通过。完整 wheel 未运行。本轮 API lit 汇总为 **161/169**，8 项失败，后续 package nightly 未运行；因此当前 examples 迁移完成，#307/#319 整体仍未完成。

最新本地证据：`docs/gates/logs/20261018-repo-closeout/`、`20261018-closeout-examples/`。归档核对的原始记录保留在 `20261018-example-consolidation/`。独立核对确认目录/CMake/cycle 注册一致、14 份收据可重现、54 个归档目录的 563 个文件哈希匹配，原已跟踪资产无缺失。退役、归档与覆盖建议均不计为 PASS。
