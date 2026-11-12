# FA2-CORE-IDX-STREAM-1007：64 项 Index 双缓冲与逐批 KV 放行 {#FA2-CORE-IDX-STREAM-1007}

<!-- ndf: kind=mechanism modality=must refinement=L2 status=draft owner=sparsefa refines=FA2-CORE-IDX-0001,FA2-CORE-SKV-0002 -->

## 模块边界

这是 Shared-KV SparseAttention 目标的 Index 路径。Index 由上游提前产生，
本模块只读取、校验并将原 token ID 交给 GatherAGU；不计算选择结果。
P0S 的整条前缀装载和单 outstanding 总线仍是独立旧实现，不能当作本模块。

```text
Count/Index DMA ─→ Index slot 0/1 ─→ 顺序 Validator ─→ GatherAGU ─→ KV slot 0/1
                       ↑                    │                 │
                  slot owner         跨批 Patricia 集     tag + 原位置
```

| 状态 owner | 必须保存的状态 | 边界 |
| --- | --- | --- |
| IndexSlotManager | 两个 64×uint32 slot、每 slot 的 operation/generation、`b,i`、list owner、batch、有效项数、位置/错误位图和状态 | 只有它发布批 VALID、释放 slot |
| IndexLoader | Count、下一批起始位置、DMA 请求/响应 tag 和每项装配状态 | 只写 FILLING slot；按原位置发布完整项 |
| IndexValidator | 当前待校验位置、至多 2048 项的精确跨批 Patricia 集、首个错误 | 批次按 `t` 递增、批内按 `p` 递增校验 |
| GatherAGU | 已获批批次游标、目标 KV slot、原 token ID 和 causal mask | 只对本批 VALID 项生成 KV 行请求 |

两个 Index slot 各存 64×32 bit，即各 **256 B**、合计 **512 B** 有序数据。
每 slot 另有 256-bit 已到达 byte 位图（32 B）和 64-bit 位置错误位（8 B）；
双 slot 的上述数据与装配位合计 **592 B**，不含 tag、控制和 ECC。
响应可乱序或拆分，但每个 byte 只能归属同一个已预留的
`(operation,generation,list_owner,batch,position,byte_offset)`；重叠、重复
或越界 fragment 是协议错误，不能用后到数据覆盖已到达 byte。
每 slot 至少提供一个填充写口和一个校验/AGU 读口；FILLING 只写，
LOADED/VALIDATING/BATCH_VALID/DISPATCHING 只读，不依赖同址读写结果。
尾批只标记有效项，padding 既不请求也不校验。Patricia 节点最多
2048 叶加 2047 分支，另按 [IDX-1003](fa2-core-idx-1003.md) 的 4096×64 bit
结构预留 32 KiB。该树保存跨批去重所需的 key，不是 2048 项有序
IndexStore；有序载荷只存在两个批 slot 和已复制到 KV slot 的原 token metadata。
Count 的范围 `0..min(2048,Sk)` 在装载第一批前确认。`count=0` 不分配
Index/KV slot，也不读取 padding。

## 每批装载、校验与放行

令 `N=ceil(count/64)`，批 `t` 包含原列表位置
`p=64*t .. min(count,64*(t+1))-1`。IndexLoader 可以在一个 slot 供
Validator/AGU 使用时向另一个 FREE slot 装载下一批；请求与响应始终携带
`(operation,generation,b,i,list_owner,batch,position)`，乱序 beat 按位置装配。
所有有效项的值或错误都可识别后，slot 从 FILLING 进入 LOADED。

Validator 只能按列表位置递增处理 LOADED 批：先检查该位置的 memory.error，
再检查 `token<Sk`，最后在跨批 Patricia 集中查重并插入。causal mask 不免除
范围/重复校验。批内全部有效项通过后发布 **BATCH_VALID(t)**；批 `t`
无需等待批 `t+1..N-1`。全局校验状态跨 slot 释放保持到该列表完成或终止。
即使后一批的总线响应先到，也不能越过前一批的校验位置决定首错。

GatherAGU 收到 BATCH_VALID 后，为本批保留匹配的 FREE KV slot，将每个
原 token ID、列表位置和 mask 复制到 KV slot metadata，并按原位置发起
未被 causal 排除的 KV 行请求。**第一批通过即可发 KV batch 0**；同时
IndexLoader 可继续装载 batch 1。batch 1 通过后也可发 KV batch 1，
与 batch 0 的 KV 返回或计算重叠，但必须有可用 KV slot 和 DMA credit。
Index slot 在其全部有效位置完成 KV 请求发射或 mask 跳过、且所需 metadata
已复制后才能释放；未发完时保留游标并反压上游。KV 响应只依赖 KV slot 的
tag/metadata，不得再读取已释放的 Index slot。两个 Index slot 与两个 KV slot
独立仲裁，不要求同号配对；没有空 slot 时只停顿，不覆盖有效批。
Index 校验允许领先于 KV 消费，但阵列必须按 batch 号递增消费 READY tile，
后批不能越过前批更新同一 head 的 online softmax 状态。

Index 与 KV 请求可以同时在途。若共用一个物理总线，仲裁器仍须给 Index
装载、KV 请求和已接受响应提供有界队列与公平进展；“重叠”不意味着同一
总线拍能传两份载荷。Index DMA 的响应空间和 KV DMA 的 8 行 credit
分别预留；最多两个 Index 批次占用两个 slot，不能因 KV 反压丢弃已经
接受的 Index beat 或向无 slot 的第三批发请求。

## 状态推进、故障与提交

Index slot 状态为 `FREE → FILLING → LOADED → VALIDATING → BATCH_VALID
→ DISPATCHING → FREE`。取消或已确定的 Index 错误使占用 slot 进入
DRAINING；旧 Index 响应按 operation/generation/tag 清空后才可复用。
KV 故障待决时，操作进入只校验模式：后续有效批可从 VALIDATING 直接
释放，不进入 DISPATCHING；已接受 KV 响应另由 KV DMA drain。
批次 token 和 KV slot 的
身份至少包含 `(operation,generation,b,i,list_owner,batch)`；共享列表的
`list_owner=shared`，head-specific 列表以 `head_id` 区分。head-specific
模式首版逐 head 处理；前一 head 完成或 drain 后才重置 Patricia 集并接纳
下一 head 的批次，不能把前一 head 的 token 判为后一 head 的重复。
只匹配 batch 编号或 KV base 不足以复用数据。

Index 错误按原列表最小位置报告。若较早批的 KV 已在途、后续批发现
Index 错误，立即停止新的 KV/计算/O 请求，失效该行候选状态并 drain
已接受的 Index/KV 响应。若 KV 故障先被观察到，停止新 KV 请求但继续
完成尚可读取的 Index 校验；此时后续有效批只更新 Patricia 集，
不再等待 KV slot 或发 gather，校验结果记入全局状态后丢弃该批载荷并
释放 Index slot，以免两个 slot 被等待 KV 的批次占满。最终若有 Index 错误则按原位置报告，
否则报告 KV 故障。这样早发 KV 不改变原完整前缀校验优先的错误边界。
取消优先于正常完成；复位需总线 fence/reset 或 generation 隔离。

内部各 batch 的 `m/l/acc` 可按 [SKV-1005](fa2-core-skv-1005.md) 的顺序
更新候选 HeadState，但在**当前 `(b,i,list_owner)` 的完整 Index 列表通过、
全部相关 KV/计算完成且无错**之前不得向外部 O 发写请求。晚到错误丢弃候选 HeadState，整次操作
输出无效；正常 completion 仍等待全部 O 写确认。此提交屏障不阻止
Index、KV 和计算的批间流水。
