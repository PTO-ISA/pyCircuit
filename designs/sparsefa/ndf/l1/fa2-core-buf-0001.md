# FA2-CORE-BUF-0001：片上存储架构合同 {#FA2-CORE-BUF-0001}

<!-- ndf: kind=requirement modality=must refinement=L1 status=draft owner=flash_attention2 refines=FA2-ADR-001,FA2-ADR-002 -->

## 目的

定义计算引擎必须看到的片上存储类别、逻辑布局、有效范围和所有权边界。这里的
“架构”指模块之间可观察的存储接口和容量语义；具体 SRAM 宏、bank 数和时序属于
`FA2-CORE-BUF-1001` 的 L2 机制。

## 存储类别

一次调用的工作集由以下类别组成：

| 类别 | 内容 | 生命周期 | 对外可见性 |
| --- | --- | --- | --- |
| Q tile store | 当前 query 行块 | 当前 query tile | 仅计算路径 |
| Index store | P0S 的完整前缀，或 Shared-KV 的两个 64 项 slot、count 与跨批校验状态 | 当前 query | gather 边界可见 |
| Original-token metadata | 当前原token或packed KV row对应的token ID | 当前 K/V tile | mask/传输边界可见 |
| K tile store | 当前 key 行块或 gathered key 行块 | 当前 K/V tile | 仅 score 路径 |
| V tile store | 当前 value 行块 | 当前 K/V tile | 仅 value 路径 |
| Row state | 每个 query 行的 `m`、`l`、有效标志 | 当前调用 | 计算/控制边界可见 |
| Accumulator | 每个 query 行的 FP32 向量累加 | 当前调用 | normalize/writeback 可见 |
| Score scratch | 当前 tile 的有限 score 或权重临时值 | 一个 tile step | 不作为调用接口 |
| Output staging | 已归一化、待写回的输出 tile | 一个写回事务 | writeback 边界可见 |
| Metadata | operation、generation、坐标、有效范围、mask 配置 | 当前事务 | 控制/传输边界可见 |

实现不得把完整 `S` 或 `P` 矩阵作为调用的持久化片外工作集。允许在片上保留当前 tile
的有限 scratch，但 scratch 释放后不能要求调用者重新提供其中间值。

这些是逻辑存储职责，允许融合或串行化。P0S的Q为一行，K/V和score只保留当前标量，
output由active accumulator生成；具体40.75KiB memory（含32KiB索引树）和lane寄存器见L2。
P0T双缓冲阵列为另一候选，不能把每类完整tile SRAM作为P0S已实现硬件。

## 逻辑布局

所有 tile 以逻辑行优先描述：

```text
Q_tile[q_row, d]
K_tile[k_row, d]
V_tile[k_row, d_v]
Accumulator[q_row, d_v]
```

并行indexed模式的 `k_row` 是packed row，须保存其原token ID；P0S直接持有原token，
不设置token到物理地址的映射表。
不同 query 的列表不能共享一个未标记归属的 KV tile。

`q_row`、`k_row` 的有效范围分别由 `valid_q_rows`、`valid_k_rows` 给出；`d` 和 `d_v`
由 `valid_d` 给出。无效尾部元素不能进入 dot、max、sum 或 value 累加。物理布局可以
做 lane striping、bank interleave 或 padding，但必须能从 tile 描述符恢复上述逻辑索引，
并且 padding 不得影响结果。

## 架构容量语义

首个 profile 必须公布每类存储的最大逻辑容量、元素类型、是否双缓冲以及是否允许多个
operation 同时驻留。一个被接受的调用必须在接受时锁定所需的 Q/K/V tile、Index store、原token metadata、Row state、
Accumulator、metadata 和 output staging 资源；资源不足时拒绝，不得边执行边隐式扩容。

同一 buffer slot 在释放前不能被另一个 operation 或另一个 generation 覆盖。读者只能
观察已经完成装载并带匹配 identity 的 tile；写者必须通过 owner 的提交边界生效。

## 复位与故障

复位和取消失效所有未提交片上状态的 metadata，RAM 数据可保留但不可访问。片上单元发生错误时，错误 operation 的 tile 和
行状态不得部分提交到下一阶段；其他 operation 的 slot、identity 和写回不受影响。

## L2 细化

具体规格、候选容量、bank/layout、双缓冲和端口数量见
[FA2-CORE-BUF-1001](../l2/fa2-core-buf-1001.md)。
三维阵列的独立供数容量、32 个逻辑 bank 和 2 KiB/group 接口见
[ARRAY3D-BUF-1004](../l2/fa2-core-array3d-buf-1004.md)；该 feeder 尚待实现和集成。
Shared-KV 目标将一份 KV tile 在 QK/PV 和多个 head 间复用；其 64-position
双/多 slot 容量、bank 与生命周期见 [SKV-1005](../l2/fa2-core-skv-1005.md)。
