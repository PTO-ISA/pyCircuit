# FA2-CORE-SKV-0002：多 Head 共享 KV 与 64-Index 批处理合同 {#FA2-CORE-SKV-0002}

<!-- ndf: kind=requirement modality=must refinement=L1 status=draft owner=sparsefa refines=FA2-ADR-001,FA2-ADR-002 -->

## 范围与数学语义

本页定义新的 **Shared-KV 并行目标 profile**，不改变已存在的 P0S
`TensorCommand` 或其独立 K/V、标量访存行为。此 profile 中 K 和 V 是同一份只读
`KV` 数据，所有 query head 读取同一个 KV cache；`Q` 和 `O` 仍各有 head 维。
这不是仅把两个传输端口接到同一地址而继续假定 K/V 内容可以不同。

```text
Q:  [B,H,Sq,D]       KV: [B,Sk,D]       O: [B,H,Sq,D]
score[b,h,i,j] = scale * dot(Q[b,h,i,:], KV[b,j,:])
O[b,h,i,:] = sum(j in A[b,h,i]) softmax(score[b,h,i,j]) * KV[b,j,:]
```

`A` 只包含外部 index 指定、通过 causal mask 的原 token ID。Index 由上游提前产生；
设备不计算 indexer、top-k 或 token→物理地址映射。共享 KV 不自动意味着各 head
的 index 相同。共享列表可表示为 `Index[b,i,p]`、`Count[b,i]`，或者沿用现有
`[B,H,Sq,...]` 视图并令 index/count 的 head stride 为零；若列表随 head 变化，
仍分别校验和执行，不得合并不同 head 的选择集合。

## 配置与可观察边界

首个可实现候选在命令接受时快照 `H` 和 `D`，运行中不得改变。`H` 和 `D`
都是运行时可配置的正整数；本候选声明 `1 <= H <= 64`、`1 <= D <= 64`，
并用 16 个 head lane、16 个维度 lane 及尾 mask 覆盖非整倍数。超过上限应在访存前
拒绝，不得截断。扩展上限必须同步增加片上状态容量并重新验证；“可配置”不表示
无限容量。`B`、`Sq`、`Sk`、dtype、scale 和 causal 仍按现有调用合同。

新命令只需一个 `kv_base`、`kv_batch_stride`、`kv_row_stride`，KV 元素维连续，
没有 KV head stride；`D_k=D_v=D`。一个元素在 QK 中作为 key、在 PV 中作为
value 使用。旧 P0S 六对象描述符不隐式转换为本 profile；若适配器提供转换，
必须先证明 K/V 是同一只读视图且内容在操作结束前稳定。

每次 index 装载最多 64 个连续 uint32 项，最后一批只包含剩余有效项；不读取 count
以外的 padding，也不越过声明的可读区间。完整有效前缀仍须先完成范围、重复和总线
错误检查，**通过以前不发起该行的 KV 读取**。64 项是搬运/调度粒度，不是新的
注意力选择语义；count 仍为 `0..min(2048,Sk)`，有效前缀允许无序而不允许重复。
后续 KV 装载按相同的最多 64 个列表位置分块，causal 被排除的位置不发 KV 读，
但保留原位置与 token ID 以解释 mask 和输出顺序。

## Head 间重用与完成

当 index/count 在 head 维共享时，同一 `(b,i,batch)` 的 64 个 KV token 只需装载
一次，已装载的 tile 在所有有效 head wave 消费完前不得覆盖。Q、score、online
softmax 的 `m/l`、PV 累加和输出均按 head 独立。若不同 head 的列表不同，可以
复用带精确 token/布局/generation tag 的命中 tile；未命中必须重新装载，不能只因
KV cache 的 base 相同就错误地复用某个 head 的 token 选择。

双缓冲至少提供一个正在消费的 KV slot 和一个正在填充的 slot；多缓冲只扩大有界
预取窗口，不改变输出、错误或取消语义。消费者只能读已完整装载、无错误且身份匹配
的 slot。加载慢于计算时允许停顿；不得以假定零延迟的 SRAM 或无限 outstanding
声称固定 II。取消或存储错误使本 operation 的 slot 和输出无效，已接受的外部
请求必须按 tag/generation drain；正常完成仍等待所有 O 写响应。

具体 bank、DMA、16×16×16 阵列映射、调度和边界条件见
[SKV-1005](../l2/fa2-core-skv-1005.md)，独立验收见
[SKV-1006](../l3/fa2-core-skv-1006.md)。
