# FA2-CORE-ARRAY3D-BUF-1004：三维阵列供数、布局与带宽 {#FA2-CORE-ARRAY3D-BUF-1004}

<!-- ndf: kind=mechanism modality=may refinement=L2 status=draft owner=flash_attention2 refines=FA2-CORE-BUF-0001,FA2-CORE-XFER-0001 -->

## 范围与所有权

本页给出 [16×16×16 阵列](fa2-core-array3d-1002.md) 的片上供数方案。
阵列源码接收已经准备好的 binary32 操作数；本页的 SRAM、装载器、转置器和双缓冲
控制器是待集成模块，不能计入已实现或已仿真的硬件。P0S 的 40.75 KiB 工作集和标量
总线仍按 [BUF-1001](fa2-core-buf-1001.md) 执行，不自动获得本页带宽。

阵列三个空间坐标为 `(m,n,k_lane)`。一组计算
`A[m,16*g+k_lane] * B[16*g+k_lane,n]`，其中 `g` 是归约组号。
`g` 与片外 KV token ID 是不同的索引；外部预生成的 index 仍由 gather 路径消费。
本设计不生成 index，不执行 indexer 或 top-k。

## 输入布局与广播

每侧输入是包含 16 个具名向量的 Table，每个向量含 `c0..c15` 共 16 个 u32 字段。
u32 承载 binary32 位模式。硬件接口具体类型名以 ARRAY3D-1002 和源码为准。

| 操作数 | 接口坐标 | 本地 bank | bank 内 512-bit 字的 lane | 广播方向 |
| --- | --- | --- | --- | --- |
| 左侧 A | `left[m].cK` | m，0..15 | k_lane，0..15 | 同一 m、k_lane 的 16 个 n |
| 右侧 B | `right[n].cK` | n，0..15 | k_lane，0..15 | 同一 n、k_lane 的 16 个 m |

两侧各读取 256 个独立 FP32 字，复用后产生 4096 个乘法输入对。
这是广播复用，不能把 4096 对操作数都算成独立 SRAM 读取，也不能只计算 32 个标量
的读取量。right 按输出列组织；QK 时相当于读取每个 key 的 16 个维度，PV 时需把
16 个 token 的同一输出维度整理为一个向量。

## 有界双缓冲候选

以下容量针对独立阵列 `1 <= length <= 64`，一块最多四组归约数据。
每个 bank 使用逻辑 1R1W 端口，读宽 512 bit；SRAM 宏的选择和真实读延迟尚待实现。

| 存储 | bank 数 | 每 bank 深度×位宽 | 单 slot 容量 | 双 slot 容量 |
| --- | ---: | --- | ---: | ---: |
| A operand store | 16 | 4×512 bit | 4096 B | 8192 B |
| B operand store | 16 | 4×512 bit | 4096 B | 8192 B |
| 合计 | 32 | 两侧各独立访问 | 8192 B | 16384 B |

bank 内地址为组号 g；bank/slot 字节地址可写为
`slot*4096 + bank*256 + g*64 + k_lane*4`。这里是本地布局，不能直接用作 KV cache
片外地址。每个 slot 独占 tag、长度、装载完成位和 mask；不计这些 metadata 的容量。
装载器写非消费 slot，消费端只读已完成并匹配 tag 的 slot；最后一次读取且不再存在
未返回的 SRAM 请求后才释放。取消使 metadata 失效，未返回读响应按 slot generation
丢弃，不能送入新 tile。该 generation 方案属于待实现 feeder，当前阵列无片外请求。

阵列内部另有乘积与四级归约寄存器、累加器，共 8192 个 32-bit 数据字，即 32 KiB；
加上上述供数双缓冲是 48 KiB **数据容量设计值**。该数不包括 token/mask/control、
Q/K/V 转换、score/softmax/行状态、Index store、输出队列或其他 attention 存储，
也不是 SRAM 面积、DFF 面积或综合结果。

## 带宽与停顿

满速一组需要 `2*16*16*4 = 2048 B`，两侧各 1024 B。
当每个 rising edge 都能接收一组时，输入 II=1，片上读带宽要求为 2048 B/cycle。
若实际时钟为 f GHz，形式上的片上带宽需求为 `2048*f GB/s`；f 尚无时序证据。
这不是外部 HBM/DDR 带宽，也不是测得的性能。

只有外部输入本身为 FP16/BF16 且两侧均无需其他转换时，独立数据量才是每组 1024 B；
PV 的概率权重可为 FP32，不能把所有阶段都按 16-bit 数据计算。上游重用 Q、K、V
会改变片外流量，完整 attention 的访存性能需在真实 gather/缓存/调度器上另测。

若某一组尚未准备好，调用方可以发送 BUBBLE，让已有乘积继续归约；整条流水需要
暂停时才不调用 `advance`，此时全部寄存状态保持。两侧数据、组号和 mask 必须作为
一个原子组交付，不能只更新 A 或只更新 B。同步 SRAM 的读请求与返回必须分别排程；
本页不能把未建模的同步读延迟当作零拍。

## QK、PV 和稀疏模式

- QK：m 是 query，n 是已 gather 的 key，k_lane 是 head dimension。
  cell mask 可以屏蔽 query/key 对，causal 判断使用原 token ID。
- PV：m 是 query，n 是输出维度，k_lane 是当前选中的 token。
  对所有有效 m 共用一份 k_mask；当前接口不能表达每个 query 不同的 K-lane 成员集合。
  因此不同 query 列表先按单 query 分开发送，或由未来明确验证的对齐/权重屏蔽模块处理。
- `length <= 64` 是当前独立计算块的一次归约上限。完整 2048-token sparse attention
  需要外层分块、online softmax/rescale 与部分结果提交，尚未接入此块。
  不能把多个 `begin` 简单串接称为同一完整归约，因为 begin 会清零累加器。

## 后续集成验收

接入 feeder 时须验证 A/B 坐标和右侧转置、32 个 bank 的同拍读取、尾组 mask、读响应
延迟、装载反压、取消后旧响应、双 slot 覆盖保护和 slot 重用。用地址/事务 trace 实测
每组供数间隔；结果不足 II=1 时按真实停顿计算周期，不修改阵列结果延迟来掩盖供数瓶颈。
