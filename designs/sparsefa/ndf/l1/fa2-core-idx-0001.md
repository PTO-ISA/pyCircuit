# FA2-CORE-IDX-0001：索引输入与稀疏 KV 访问合同 {#FA2-CORE-IDX-0001}

<!-- ndf: kind=requirement modality=must refinement=L1 status=draft owner=flash_attention2 refines=FA2-ADR-002 -->

## 接口与职责

本合同与 [调用合同](fa2-core-0001.md) 同层协作，定义硬件的 `IndexInput`、
`IndexStore`、`GatherAddress` 与 `KVTransfer` 边界。索引由上游提供；设备不运行
indexer/top-k，不隐式追加 local window 或其他 token。

| 输入 | 候选类型/逻辑形状 | 含义 |
| --- | --- | --- |
| `selection_mode` | `dense` / `indexed` | 与 `mask_mode` 独立 |
| `Index` | uint32 `[B,H,S_q,K_max]` | 每个 query 的逻辑 KV token 编号 |
| `IndexCount` | uint32 `[B,H,S_q]` | 本行有效前缀长度 |
| `K_max` | P0S固定2048 | 每行分配容量，不是 `S_k`；只读取count项 |
| index/count 描述符 | base、各维 byte stride、可读区间 | 独立于 K/V 的布局 |
| K/V cache 描述符 | 两套 base、各维 byte stride、可读区间 | `K/V[B,H,S_k,D]` 的只读视图 |

dense 模式不访问 Index/IndexCount。indexed 模式要求上述索引输入成对出现；不允许
缺失 index 后静默回退 dense。P0S 的 Index 最内维条目连续，固定为 `index_stride_p=4`；
该值属于ABI，不是硬件描述符字段。其他轴可有 padding 或显式只读广播。调用方或适配层
必须在构造描述符前拒绝非连续uint32列表；硬件无法从base推断上游原本的条目stride。
P0S 的 S_k 上限为
`2^32-1`，K_max 上限为 2048；均为待冻结参数。`IndexCount` 范围为 `0..min(K_max,S_k)`，有效 token
编号满足 `0 <= j < S_k`，类型是整数。允许无序列表；有效前缀中的重复编号报
`INDEX_DUPLICATE`，不能重复计分或静默去重。越界报 `INDEX_RANGE`，count 非法报
`INDEX_COUNT`。先验证整个有效前缀，再发出该 query 的 KV 读请求。

`Index[..., count:K_max]` 是 padding，内容不参与合法性判断和地址生成；没有特殊 sentinel
语义。count=0 合法，不发出该行 KV 请求，输出全零。向量化 index 读取可以覆盖分配
范围内的 padding 字节，但不能越过声明的可读区间。

## 选择与 mask

令 `J(b,h,i)` 为有效索引集合，`M(i)` 为原 L1 的 none/causal 允许集合：

```text
dense:   A(b,h,i) = {0,...,S_k-1} ∩ M(i)
indexed: A(b,h,i) = J(b,h,i) ∩ M(i)
causal:  j <= i + causal_offset
```

softmax 的最大值、分母和 PV 累加都仅覆盖 A。`i` 是原 query 坐标，`j` 是原 KV token
编号；列表位置 `p` 或 gather 后的 slot row 不能代替 `j`。causal_offset 使用有符号
数学加法，不得以 uint32 回绕；decode 的绝对位置由调用者显式提供 offset。

例如 `Index=[7,2,5]`、query 位置 `i+offset=4`，只有 token 2 有效，不是列表的前五项。
不同 query/head 的集合互不合并。完整唯一索引集应在数值容差内等价于 dense；置换顺序
允许改变浮点归约误差，仍须满足同一 L1 数值容差。

## KV cache 与共享

P0 候选是已经展开为 K/V 向量、以逻辑 token 编号寻址的 cache。K 与 V 用同一个 `j`
配对，但各自使用自己的 base 和 stride。不得把 token 编号解释为字节地址或 page ID。
本 profile 的 Q/K/V head 数一致；只读输入的 head stride 可为零，以表达
上游在多个 head 共享同一列表。不同 query 的共享也必须由明确的 stride 描述。

新的 [Shared-KV 目标 profile](fa2-core-skv-0002.md) 进一步要求 K=V 且所有
head 共享单份 `KV[B,Sk,D]`；index 是否跨 head 共享仍由其布局明确决定，
不能从 KV 共享自行推断。它的 64 项批处理、双缓冲与带 tag gather 不属于
当前 P0S 的单 outstanding 实现。

Paged KV 的 block table、cache 环形窗口位置变换、GQA/MQA head 映射、MLA latent
重建和量化解码属于后续 profile。当前描述符没有这些格式的识别字段；调用方或适配层
必须拒绝未经展开的布局，或先提供符合本ABI的线性K/V视图，不能依赖硬件自动识别。
不能声称已覆盖DeepSeek的完整cache ABI。支持这些扩展时，必须保留逻辑token位置用于mask。

调用者在接受至完成/终止 drain 期间保持 Index、IndexCount、Q 和所引用 K/V 的内容与
映射稳定，并提供可读区间；它们与 O 写区间不重叠。设备不实现与 cache 写入者的隐式
一致性。描述符在接受时快照，索引按行装载；这不是全 cache 的硬件快照。

## 硬件边界与失败

IndexStore 保存有限容量的索引、count、query identity 和验证状态；GatherAddress
输出携带原 token ID 的 K/V 请求。并行实现的KVTileStore按局部packed row存放数据；P0S仅保存当前token及K/V标量，
不需要packed row映射。计算只消费当前已接受请求的响应，并在V向量完整后提交行状态。
资源不足只能等待或拒绝，不得扩大无限索引缓存。

非法 count/索引、地址溢出、不可读范围及存储故障均可区分。描述符错误在任何访存前拒绝执行（P0S通过已捕获命令的错误completion报告）；
索引内容错误可能在接受后才发现，终止当前 operation，并遵守调用合同的 drain 与
部分输出失效规则。被 causal mask 排除的有效前缀条目也必须先通过范围/重复验证。

具体 buffer、端口、校验器和 gather 调度见
[FA2-CORE-IDX-1001](../l2/fa2-core-idx-1001.md)；独立验收见
[FA2-CORE-IDX-1002](../l3/fa2-core-idx-1002.md)。
