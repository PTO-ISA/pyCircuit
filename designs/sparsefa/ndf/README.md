# FlashAttention2 NDF

L0保存算法与外部index范围决策；L1定义硬件职责/可观察合同；L2给出模块、端口、状态、
存储和周期；L3定义独立验证。L0基线保持冻结原文；L1/L2仍draft，不把文档或代码
完成等同于RTL、全域数值或PPA验收。

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
| 外部index | [IDX](l1/fa2-core-idx-0001.md) | [IDX](l2/fa2-core-idx-1001.md) | count/index读、前缀校验、DirectGatherAGU |

L1各条款直接细化L0，同层依赖不用L1→L1 refines；L2细化L1；L3细化L2并verifies L1。
P0S串行实现与 P0T 并行阵列分开，不混用容量/吞吐/证据。当前并行方向是
[16×16×16 三维阵列](l2/fa2-core-array3d-1002.md)，配套
[供数与 buffer 布局](l2/fa2-core-array3d-buf-1004.md)及
[独立逐位/周期验证](l3/fa2-core-array3d-1003.md)。
进一步的 [Shared-KV 多Head合同](l1/fa2-core-skv-0002.md) 规定单份 KV 同时
作为 K/V、64 项 Index 批次和跨Head复用；对应
[DMA/双缓冲微架构](l2/fa2-core-skv-1005.md)、
[新泳道图](l2/fa2-core-skv-swimlane-1005.md)及
[验收矩阵](l3/fa2-core-skv-1006.md) 均为目标草案，尚非当前硬件实现。
此前的 [二维阵列](l2/fa2-core-array-1002.md)及其
[验证合同](l3/fa2-core-array-1003.md)保留为独立基线；具体编译和运行结果以
[VALIDATION](../VALIDATION.md)为准。
原16-entry整数map原型已归档，当前线性KV路径无地址映射表。
