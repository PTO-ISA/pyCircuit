# FA2-CORE-VECTOR-1006：Vector 与 online softmax 资源契约 {#FA2-CORE-VECTOR-1006}

<!-- ndf: kind=mechanism modality=must refinement=L2 status=draft owner=sparsefa refines=FA2-CORE-COMP-0001,FA2-CORE-SKV-0002 -->

## 适用边界

本页规定 Shared-KV SparseAttention 路径的 Vector 计算资源规格。它不修改
P0S 标量 `fp_exp_neg` 的数值语义；数值结果仍遵守
[SKV-1005](fa2-core-skv-1005.md) 的 binary32 RTZ/FTZ、归约顺序与原子提交。
这是待实现的架构要求，不代表现有 RTL 或 C++ 模型已经达到该能力。

## 向量资源

| 资源 | 必须提供的稳态处理能力 | FP32 对应能力 |
| --- | --- | --- |
| 普通向量计算 | 每周期 256 B 逻辑元素载荷 | 每周期 64 个元素 |
| `exp` 计算 | 每周期 32 个 FP32 元素 | 每周期 32 个元素 |

普通向量资源承担 score 比较、减法、行状态加乘等 FP32 操作；exp 资源承担
非正输入的 `fp_exp_neg`。两类资源的吞吐不能混算。对被 mask 的位置不发起
有效运算；每个 head 的 `m/l/acc` 独立，跨 batch 保持状态。score/weight
scratch 的复用必须服从 [SKV-1005](fa2-core-skv-1005.md) 的阶段与提交边界。

这里的 256 B 是被处理元素的逻辑宽度。二元运算还需第二路输入和结果写口；
score scratch、行状态与 Vector 的端口和仲裁须能支撑上述能力，不能把该数字
误读为外部总线或某个单端口 SRAM 的带宽。实现可采用并联单元或等效流水，
但必须保留所声明的数值语义、背压和取消行为。

QK、online softmax、PV 或整批的完成时延不属于本 NDF；它们由独立性能模型
在给定资源连接、仲裁和存储响应条件后测量。
