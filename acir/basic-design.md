# ACIR 基础设计

编译流程为 ACPy → 源语义 MLIR → 事务 MLIR → RTL 友好 MLIR → 逐拍验证。
完整语义以 [核心设计](../pycircuit/docs/core-design.md) 为准；实际操作与保存格式见
[ACIR v8](../pycircuit/acir.md)。

源层保存静态实例、连接、统一 Rule、guard／body、SSA／region 和声明契约。
普通计算复用标准 MLIR 方言，组件身份与访问用 ACIR 类型／操作表示。
事务层保存调用点身份、完整条件、条件化效果、固定 EHR 端口、分支后继和固定拓扑顺序。
RTL 层保存组合数据流、选择器、fire／写使能、旧状态与 next-state。
JSON 只用于报告；所有后续编译信息必须在 MLIR 中自足。

| 组件 | 读取与可见性 | 获准效果 |
| --- | --- | --- |
| Cell | 始终读取本拍开始旧值，首版 CF | 下拍统一提交 |
| Queue | 旧队头，peek → pop → push；容量视图允许前序成功 pop 腾空间 | pop／push 原子提交，新数据下拍读取 |
| Signal | 本拍前序成功 set 后有效；set → get 为 SBR | 组合 data／valid，无跨拍状态 |
| EHR | 固定端口读取前序获准写入中端口最大的值 | 最高端口获准写入决定 next-state |

Rule 完整条件沿数据／控制依赖传播，实际分支的所有效果共享外层原子 fire。
读取失败不进入 else；没有 persistent var.valid 或默认旧值。独立事务通过组件传值。
声明关系沿嵌套调用传播，分支边合并到节点级图并要求无环。
EHR 不从任意拓扑排序增加可见性，跨端口同事务访问明确拒绝。

本轮不使用 CIRCT，不生成 SystemVerilog，不做自动仲裁、激活调度或跨拍缓存。
未处理的端口竞争结果不保证。具体限制与最小复现见 [路线图](../pycircuit/docs/compiler-roadmap.md)。
