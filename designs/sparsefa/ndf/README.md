# SparseAttention 加速器 NDF

## NDF 范围契约（必须遵守）

本目录只规定 SparseAttention 加速器的**架构和微架构契约**。L0 保存算法与外部
Index 决策；L1 规定功能、配置、接口和可观察行为；L2 规定模块划分、状态所有权、
存储容量与布局、数据通路、资源宽度、吞吐能力、握手和原子提交；L3 规定这些
条款的结构与功能验收条件。已冻结的 L0 算法基线不因本范围收敛而改写；
其余页面按各自标记保持草案状态。

以下边界对新增或修改的 NDF 条款均为强制要求：

- SparseAttention 目标必须以调用方预生成的 Index 为输入，按最多 64 项批次
  直接定位 KV cache；K 与 V 使用同一份 KV，多个 head 可复用该 KV，
  但各 head 的 Q、softmax 行状态和输出独立。不能把 Index 计算或选择器
  性能写成加速器已有功能。
- 可以规定与实现无关的容量、并行度、接口顺序、流水寄存边界、最大并发数和
  **资源稳态吞吐规格**，例如 64 项 Index 批次、16×16×16 阵列、普通向量
  256 B/周期和 exp 32 个 FP32 元素/周期。吞吐是模块能力要求，不是算子完成时间。
- 不得给出 QK、softmax、PV、Index 校验、单 tile 或整次 attention 的固定
  周期数、时延公式、发射周期下界、tile cadence、加速比或某个示例负载的
  周期推算。不能从资源吞吐直接推导算子时延作为 NDF 合同。
- 必须保留决定正确性的时间关系：请求先于响应、**当前 64 项 Index 批次**
  校验通过后才能发该批 KV gather、下一批 Index/KV 可与当前批重叠、
  输出所属列表全体无错且数据到齐后才能发布该输出、取消时旧响应 drain、
  `m/l/acc` 原子提交。这些是协议与
  状态转移要求，不是算子时延承诺。
- 周期模型的假设、公式、实测 trace、带时间刻度的泳道图和性能对照放在
  [独立 C++ 性能模型](../tests/timing_model/README.md)及其测试/验证记录中；
  NDF 仅引用其存在，不把模型值转写成架构约束。功能实现、综合和 PPA 的
  实际状态以 [VALIDATION](../VALIDATION.md) 为准。

本范围契约优先于 L1–L3 中遗留的周期性描述；修订相关页面时必须清理越界内容。
架构规格、源码完成和性能测量分别验收，不能互相替代。

| 层级 | 入口 |
| --- | --- |
| L0 | [算法基线](l0/fa2-adr-001.md)、[外部索引补充](l0/fa2-adr-002.md) |
| L1 | [架构合同目录](l1/README.md)、[P0S具体profile](l1/fa2-core-profile-0001.md) |
| L2 | [模块实现目录](l2/README.md) |
| L3 | [验证目录](l3/README.md)、[当前实际状态](../VALIDATION.md) |

| 硬件职责 | L1合同 | L2实现细节 | 当前源码 |
| --- | --- | --- | --- |
| 完整调用 | [CORE](l1/fa2-core-0001.md) | [ENG](l2/fa2-core-eng-0001.md)、[数据流泳道图](l2/fa2-core-dataflow-swimlane-1001.md) | FA2TensorScheduler→FA2Engine |
| 存储 | [BUF](l1/fa2-core-buf-0001.md) | [BUF](l2/fa2-core-buf-1001.md) | Q/Index/双bank accumulator Memory |
| 外部传输 | [XFER](l1/fa2-core-xfer-0001.md) | [XFER](l2/fa2-core-xfer-1001.md) | MemoryRequest/Reply与REQ/WAIT/DRAIN |
| 数值计算 | [COMP](l1/fa2-core-comp-0001.md) | [COMP](l2/fa2-core-comp-1001.md) | fa2_numeric + engine共享ALU |
| 控制 | [CTRL](l1/fa2-core-ctrl-0001.md) | [CTRL](l2/fa2-core-ctrl-1001.md) | TensorLayoutValid、tensor/row FSM |
| 外部index | [IDX](l1/fa2-core-idx-0001.md)、[Shared-KV](l1/fa2-core-skv-0002.md) | [P0S IDX](l2/fa2-core-idx-1001.md)、[双缓冲 IDX](l2/fa2-core-idx-stream-1007.md) | 预生成 Index 输入、逐批校验与 GatherAGU |

L1各条款直接细化L0，同层依赖不用L1→L1 refines；L2细化L1；L3细化L2并verifies L1。
P0S 串行实现与 P0T 并行阵列是参考基线，不替代 Shared-KV SparseAttention
目标合同，也不混用容量、吞吐或证据。当前并行方向是
[16×16×16 三维阵列](l2/fa2-core-array3d-1002.md)，配套
[供数与 buffer 布局](l2/fa2-core-array3d-buf-1004.md)及
[独立结构与数值验证](l3/fa2-core-array3d-1003.md)。
进一步的 [Shared-KV 多Head合同](l1/fa2-core-skv-0002.md) 规定单份 KV 同时
作为 K/V、64 项 Index 批次和跨Head复用；对应
[Index 双缓冲](l2/fa2-core-idx-stream-1007.md)、[KV DMA/双缓冲](l2/fa2-core-skv-1005.md)、
[详细硬件模块结构图](assets/sparsefa-shared-kv-hardware-architecture.svg)、
[新泳道图](l2/fa2-core-skv-swimlane-1005.md)及
[共享 KV 验收](l3/fa2-core-skv-1006.md)和
[Index 双缓冲验收](l3/fa2-core-idx-stream-1008.md)均为目标草案，尚非当前硬件实现。
此前的 [二维阵列](l2/fa2-core-array-1002.md)及其
[验证合同](l3/fa2-core-array-1003.md)保留为独立基线；具体编译和运行结果以
[VALIDATION](../VALIDATION.md)为准。
原16-entry整数map原型已归档，当前线性KV路径无地址映射表。
