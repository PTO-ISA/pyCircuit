# FA2-CORE-IDX-STREAM-1008：Index 双缓冲与逐批 Gather 验收 {#FA2-CORE-IDX-STREAM-1008}

<!-- ndf: kind=verification modality=must refinement=L3 status=draft owner=sparsefa refines=FA2-CORE-IDX-STREAM-1007 verifies=FA2-CORE-IDX-0001,FA2-CORE-SKV-0002 -->

验证 [IDX-STREAM-1007](../l2/fa2-core-idx-stream-1007.md) 的资源边界、
Index→KV 顺序、跨批校验和故障隔离。独立内存模型只提供预生成的 Index、
KV 和响应；期望 token 序列、地址、错误与 O 由测试 oracle 从原始输入计算，
不得从 DUT 的 Index slot 或 KV slot 读出再作为期望。

| 场景 | 必须观察到的行为 |
| --- | --- |
| count=0/1/63/64/65/127/128/2048 | 恰好读取 count 个 Index；尾批不读 padding；同时驻留的有序 Index 载荷不超过两个 64 项 slot |
| 批 0 有效、批 1 的 Index 响应挂起 | 批 0 的 BATCH_VALID 后可发对应 KV 请求，不等待批 1；批 1 可占另一个 Index slot |
| 批 0 的 KV 正在填充/计算、批 1 已校验 | 有可用 KV slot/credit 时批 1 KV 可启动；没有时反压并保留批 1 的 tag/token，不覆盖 |
| 批 2 到来、批 0 的 Index slot 已释放而 KV 响应未完成 | 批 2 可复用该 Index slot；迟到的 KV 0 响应只落到 KV 0 tag，不能误写批 2 |
| 批内与跨批重复，含第 65、128、2048 项 | Patricia 保留所有先前有效 key；按原列表最小错误位置报告，后批错误不使已发前批 KV 变成成功输出 |
| 后批越界或 Index bus.error，前批 KV 已在途 | 停止新 KV/计算/O 请求，drain 已接受 Index/KV 响应，候选 HeadState 失效，本行无 O 写或正常 completion |
| KV 错误先于后批 Index 错误，后面还有多批 Index | 停止新 KV，后续批以校验模式推进并释放 Index slot，避免等待 KV slot 死锁；Index 首错优先，若无 Index 错误才报告 KV 错误 |
| causal mask 与乱序 token | 被 mask 项仍查重/查范围，但不发 KV；未 mask 项的 KV 地址使用原 token ID，输出按原列表次序规约 |
| Index beat 乱序、拆分、重复或越界 fragment | 仅在有效 byte 全部装配后校验；重叠/越界报协议错误，不覆盖已有数据；两个 slot 的响应不串批 |
| 共享列表与 head-specific 列表 | 共享列表只校验/装载一次并供所有 head；不同 head 的 Patricia 状态、slot tag、候选行状态隔离 |
| Index/KV 同总线反压、乱序响应、取消与复位 | 已接受 beat 不丢失；slot 未满足释放条件不复用；旧 generation drain/fence 后新操作才可使用资源 |

测试必须分别观测 Index slot 的 FILLING、BATCH_VALID、DISPATCHING、FREE，
KV slot 的 FILLING、READY、IN_USE、FREE，及请求 tag、原位置和 head/list 身份。
验证的是因果顺序和有限容量，不规定 Index、KV 或完整算子的固定时延。
具体性能和重叠程度由独立 C++ 性能模型在同输入、同总线条件下测量。
