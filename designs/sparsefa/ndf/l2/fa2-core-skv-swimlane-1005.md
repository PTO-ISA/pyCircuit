# FA2-CORE-SKV-SWIMLANE-1005：64-Index 与共享 KV 双缓冲泳道图

<!-- ndf: kind=mechanism modality=may refinement=L2 status=draft owner=sparsefa refines=FA2-CORE-SKV-0002,FA2-CORE-SKV-1005 -->

下图是**目标并行架构**的事务顺序，不是当前 P0S 的实测逐拍日志。
时间向下；Index 由 Host 提前产生，K 和 V 是同一份 `KV[b,token,d]`。
为了保持当前完整前缀校验的错误边界，所有最多 64 项的 Index 批次先验证完，
再开始本行的 KV 请求。双 slot 的重叠发生在后续 KV batch 装载与计算之间。

```mermaid
sequenceDiagram
    autonumber
    participant Host as Host/Index 输入
    participant Sch as 调度/HeadState
    participant Idx as Index DMA (64项)
    participant Val as IndexStore/校验
    participant DMA as KV Gather DMA
    participant S0 as KV Slot 0
    participant S1 as KV Slot 1
    participant MAC as QK/Softmax/PV
    participant O as O 写回

    Host->>Sch: SharedKVCommand(H,D,KV,Index,Q,O)
    Sch->>Idx: 读取 count
    loop t=0..ceil(count/64)-1
        Idx->>Host: 读取最多 64 个 uint32 Index
        Host-->>Idx: 按位置返回 Index 批次
        Idx->>Val: 逐项 range/duplicate 校验并保存原顺序
    end
    Val-->>Sch: 完整前缀 VALID（错误则终止）
    Sch->>DMA: batch 0 的至多 64 个原 token
    DMA->>Host: gather 至多 64 行，共用一份 KV
    Host-->>DMA: 带 tag 的 KV 响应，可乱序
    DMA->>S0: 按原位置填充，全部返回后 READY
    par 消费 batch 0
        S0->>MAC: 同一 KV tile 广播到全部 16-head wave
        MAC->>MAC: QK token×D 分块 → batch max/weight → PV output-dim×token 分块
        MAC->>Sch: 整批完成后原子发布每 head 的 m/l/acc
    and 预取 batch 1
        Sch->>DMA: batch 1 的至多 64 个原 token
        DMA->>Host: 下一组 scatter KV 行读取
        Host-->>DMA: 带 tag 的 KV 响应
        DMA->>S1: 填充后 READY
    end
    Note over S0,S1: slot 0 在全部 head wave 消费后才可复用；下一批交换角色
    loop 后续 batch：READY 后交替消费/填充
        Sch->>MAC: 从当前 READY slot 供 QK → online softmax → PV
        MAC->>Sch: 保留各 head 的跨 batch 状态
        Sch->>DMA: 只向已释放的 slot 预取后续批次
    end
    Sch->>O: 每 head 归一化并写 O
    O-->>Sch: 所有 O 写入 ack
    Sch-->>Host: 一次 completion
```

对于 head-specific Index，每个 head 的 Index 前缀须独立验证；KV 仍来自同一
共享 cache，但首版按 head 顺序一次处理一个列表，slot 绑定
`list_identity,head_id,batch`，阵列只启用该 head 行。消费其 batch `t` 时
仅预取同一 head 的 batch `t+1`；列表不同的 tile 不能仅按 `batch` 位置复用。
无论双缓冲还是三缓冲，
DMA 返回慢时 MAC 等待 READY；图中的并行分支不承诺固定 II 或零停顿。
详细 tag、bank、容量和失效规则见 [SKV-1005](fa2-core-skv-1005.md)。
