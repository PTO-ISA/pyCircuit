# FA2-ADR-002：外部索引驱动的稀疏注意力 {#FA2-ADR-002}

<!-- ndf: kind=decision modality=must refinement=L0 status=draft owner=flash_attention2 -->

## 变更来源与边界

用户在 2026-11-03 明确要求：支持 sparse attention；读取 KV cache 前先获取 index，
当前 index 是调用输入的一部分。这项范围变更补充已冻结的
[FA2-ADR-001](fa2-adr-001.md)，其原文保持不变；原“首个闭环暂不要求稀疏模式和
KV cache”的建议不再用于排除本功能。

新增要求是消费上游选出的索引，而非在加速器中训练或执行 DeepSeek 的 indexer、
相关性打分、top-k 或选块算法。这里定义外部索引驱动的 attention 接口，并不宣称
已经兼容某个 DeepSeek 模型版本的全部 DSA、MLA、量化或 cache 存储格式。

## 架构意图

调用可选择 dense 或 indexed 模式。indexed 模式先读取 index，再据此 gather 对应的
K/V，只对所选且通过 mask 的 token 做 softmax 和加权求和。未选择的 key 不进入分母，
不能在 dense softmax 之后才把它的输出置零。该算法对选择集合精确；任意稀疏集合的
结果不要求等于全量 dense attention。

原基线的 online softmax、有限片上工作集、禁止完整 S/P 片外物化以及调用隔离继续适用。
索引生产者负责选择质量，加速器负责选择集合的执行正确性。

## 待冻结的接口选择

当前 L1/L2 草案按单个 KV token 的逻辑编号设计，列表可以逐 query 不同，允许无序，
不允许有效前缀内重复。token/block 粒度、索引共享方式和物理 cache 格式尚未获得参数
冻结确认；当前候选在 [L1 索引合同](../l1/fa2-core-idx-0001.md) 和
[L2 gather 结构](../l2/fa2-core-idx-1001.md) 中具体化。

本补充记录用户已确定的功能范围；新增接口和实现参数保持 draft，不改变 ADR-001 的
冻结记录，也不是 RTL、性能或整机兼容性证据。
