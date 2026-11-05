# FA2-CORE-ARRAY-1003：并行阵列验证与周期统计 {#FA2-CORE-ARRAY-1003}

<!-- ndf: kind=verification modality=must refinement=L3 status=draft owner=flash_attention2 refines=FA2-CORE-ARRAY-1002 verifies=FA2-CORE-COMP-0001 -->

本页落实 [16×16 外积阵列合同](../l2/fa2-core-array-1002.md)的检查方法。
当前执行结果见 [VALIDATION.md](../../VALIDATION.md)。测试存在、独立 oracle 通过、
源码 compile、final link、双 emit 和生成 C++ 执行是不同证据，分别记录。

## 源码和独立期望

| 文件 | 责任 |
| --- | --- |
| `src/fa2_array_interfaces.py` | begin/step/result、RowStep 和 Row16 的 nominal 类型 |
| `src/fa2_array.py` | Array16x16 → 16 个 ArrayRow → 每行 16 个 ArrayPE，状态与方法 |
| `src/fa2_numeric.py` | PE 使用的 pure add/mul 和既有 Module API，只有一份数值实现 |
| `tests/array/generate.py` | Fraction 精确计算、逐次舍入编码、独立生命周期模型 |
| `tests/array/cases.json` | 输入日程、期望接受动作和完整 256 元素结果 |
| `tests/array/emit_stimulus.py` | 只将输入整理为14组独立向量和235条指令的常量表，不导出期望结果 |
| `tests/array/bench.py` | 仅输入字面量和方法调用、独立 tick、观测；不把期望送入 DUT |
| `tests/array/run.py` | 独立 compile/link、同 final 双 emit、C++ workers 1/2、全量结果比较 |
| `tests/array/test_oracle.py` | 手算锚点及日志破坏负例，检查 oracle 和校验器本身 |

bench 的 `tick` 与 `drive` 是两条独立 Rule。错误 tag/index/状态令 drive 不提交时，
tick 仍推进到下一条指令，避免让测试本身停在一个错误调用上。take、结果日志和
completed 计数位于同一事务，非法 take 不能增加完成数。测试模型的 `accepted` 和
预期 rows 只供 host 校验，不进入设计 source unit。

## 首批矩阵

235 条指令预期产生 25 个成功 tile，每个 worker 应比较全部 6,400 个 FP32 编码：

| 情景 | 必须观测的性质 |
| --- | --- |
| Dense K=1/4/16/64 | 每个 row/column 使用不同输入；检查全部 lane、steps 和 useful_macs |
| 16 个单行 sparse 位置 | 每次仅一行有效，其余 240 个元素保持 +0；每 tile 32 useful MAC |
| 非矩形 cell_mask、变化 col_mask | 三种 mask 共同决定更新；未选元素保持原编码 |
| 全空 mask | 消费全部 index、useful_macs=0、所有结果 +0 |
| ACTIVE 气泡、延迟 take | 不多算 reduction 项，RESULT 保持，周期统计反映等待 |
| length=0/65、错误 tag、重复/乱序 index、early end、busy begin | 没有接受事件，没有额外更新或结果 |
| ACTIVE/RESULT abort 后重启 | 被取消 tile 没有结果；新 begin 清零，不泄漏旧累加值 |
| 非融合消去和 FTZ | 乘法先舍入再加法，逐位区别于 fused FMA |

独立数值编码器按输入 FTZ、乘法 RTZ/FTZ/有限饱和、加法 RTZ/FTZ/有限饱和计算；
同幅消去的零符号沿用旧 accumulator。它不调用 DUT 或使用 host float 代替精确分数。
手算锚点覆盖正负零、下溢、饱和以及 nonfused/FMA 差异。首批 generated 矩阵不是
整个有限 FP32 域的穷举，也不能借此宣称 NaN/Inf 支持。

## 周期和吞吐的统计口径

生成 System 每周期有低/高两个 Work sample。仅偶数 `commit_epoch` 对应本测试
驱动的 rising edge；校验 `commit_epoch == 2*(cycle+1)`，不能把两次观察计为两次
接受。日志须与独立模型完全一致，包括顺序、次数和被拒动作的缺失：

- `array_action(cycle, op, tag, index)`：实际接受的生命周期动作。
- `array_row(tag, row, c0, ..., c15)`：take 当拍的完整行；每 tile 恰好 16 条。
- `array_result(tag, steps, useful_macs)`：take 当拍元数据。
- `array_pass(total_results)`：日程结束后仅一次完成检查。

记录 begin、各 step、end、take 的真实接受周期，并计算：

```text
begin_to_take = take_cycle - begin_cycle
occupied_cycles = take_cycle - begin_cycle + 1
active_stall_cycles = end_cycle - begin_cycle - accepted_steps - 1
result_wait_cycles = take_cycle - end_cycle - 1
step_intervals = 相邻两次实际 step 的周期差
```

无阻塞期望为 step II=1、begin-to-take=K+2、tile 占用 K+3。
满阵列每个接受 step 是 256 MAC；有 mask 时用 useful_macs 统计，不把空槽计入
有效吞吐。仿真停止的 epoch 仅表示日程完成界限；host 执行秒数不是硬件周期。
完整 attention 性能还需要包含供数、KV gather、softmax、rescale、写回及各层等待。

## 后续验收边界

Queue 的 `take()+push()` 原子背压、ACTIVE/RESULT 中途 reset 和 X/Z 到达控制的
全局零提交需要专门用例；首批日程中的延迟 take 不能替代它们。初始 System Reset
也不能代替中途 reset。未执行项保留在 runner 的 manifest 中，不能从 P0S 或其他
Queue 测试外推本阵列已经通过。

默认预算下的完整 16×16 final 必须保留 256 个独立 accumulator owner 及其运算链；
源代码实例计数本身不证明最终硬件结构。若链接/生成失败，保留具体阶段和诊断，
不缩小阵列、不提高检查预算、不替换为 host 计算结果。
Verilog 与 C++ 由同一 final 生成；按用户要求不执行 Verilog 仿真。
综合面积、时序、MHz、功耗和端到端加速比不属于这组周期模型的通过结论。
