# L2 硬件模块与微架构

当前实现为P0S：真实外部索引、线性AGU、共享标量浮点、有限Memory及整机循环。
各页包含唯一状态owner、类型/位宽、访问方式、推进/终止条件和必要测试。

- [整机层级和数据连接](fa2-core-eng-0001.md)
- [P0S 数据流泳道图](fa2-core-dataflow-swimlane-1001.md)
- [Buffer容量、物理布局候选、端口和有效性](fa2-core-buf-1001.md)
- [索引装载、完整前缀验证和直接AGU](fa2-core-idx-1001.md)
- [Shared-KV 的 64 项 Index 双缓冲、跨批校验与逐批 KV 放行](fa2-core-idx-stream-1007.md)
- [Patricia索引校验器、节点布局与状态推进](fa2-core-idx-1003.md)
- [外部标量总线、请求/响应、cancel drain](fa2-core-xfer-1001.md)
- [共享运算单元、位级FP和原子PV](fa2-core-comp-1001.md)
- [P0T Vector/exp 资源吞吐契约](fa2-core-vector-1006.md)
- [TensorCommand、地址验证、tensor/row FSM](fa2-core-ctrl-1001.md)
- [P0T 16×16×16 三维阵列、归约流水与生命周期](fa2-core-array3d-1002.md)
- [三维阵列供数、bank 布局和双缓冲候选](fa2-core-array3d-buf-1004.md)
- [Shared-KV的64项Gather、bank、DMA与多缓冲目标](fa2-core-skv-1005.md)
- [Shared-KV 的 Index/KV 双缓冲数据流泳道图](fa2-core-skv-swimlane-1005.md)
- [此前的 16×16 二维阵列基线](fa2-core-array-1002.md)

P0S源码的可运行状态见 [VALIDATION.md](../../VALIDATION.md)。L2 只约束架构；
算子时延、频率和 PPA 另由性能模型及实现证据评估。
