# FA2-CORE-SKV-SWIMLANE-1005：Index/KV 双缓冲数据流

<!-- ndf: kind=mechanism modality=may refinement=L2 status=draft owner=sparsefa refines=FA2-CORE-SKV-0002,FA2-CORE-SKV-1005,FA2-CORE-IDX-STREAM-1007 -->

下图只表达目标架构的事务依赖和可重叠关系，不表达周期数或固定算子时延。
Index 由 Host 提前产生，K 和 V 是同一份 `KV[b,token,d]`。每个 64 项
Index 批次单独校验并放行对应 KV；跨批查重状态持续保留。Index slot 与
KV slot 各有两个，编号不要求相同。

```mermaid
sequenceDiagram
    autonumber
    participant Host as Host / 预生成 Index + KV
    participant Sch as 调度 / HeadState
    participant Idx as Index DMA + 双 slot
    participant Val as 跨批校验器
    participant DMA as KV Gather DMA
    participant S0 as KV Slot 0
    participant S1 as KV Slot 1
    participant MAC as QK / Softmax / PV
    participant O as O 写回

    Host->>Sch: SharedKVCommand(H,D,KV,Index,Q,O)
    Sch->>Idx: 读取 Count、装载 Index batch 0
    Host-->>Idx: 最多 64 个 uint32，按原位置装配
    Idx->>Val: 顺序校验 batch 0，保留跨批去重状态
    Val-->>Sch: BATCH_VALID(0)
    par 当前批 KV 与计算
        Sch->>DMA: batch 0 原 token 与位置
        DMA->>Host: 按有效 token scatter 读取 KV
        Host-->>DMA: 带 tag 的 KV 响应
        DMA->>S0: 全部有效数据到齐后 READY
        S0->>MAC: 同一 KV tile 供所有共享列表 head wave
        MAC->>Sch: 更新候选 m/l/acc
    and 下一批 Index 与 KV 预取
        Sch->>Idx: 在另一 Index slot 装载 batch 1
        Host-->>Idx: batch 1 的有效项
        Idx->>Val: 按原列表位置做跨批 range/duplicate 校验
        Val-->>Sch: BATCH_VALID(1)
        Sch->>DMA: 有可用 KV slot/credit 后发 batch 1
        DMA->>Host: batch 1 的 KV scatter 请求
        Host-->>DMA: 带 tag 的响应
        DMA->>S1: batch 1 READY
    end
    loop 其余 batch：Index slot 和 KV slot 分别释放、复用
        Sch->>Idx: 装载并校验后续最多 64 项
        Val-->>Sch: 本批 BATCH_VALID 或首个 Index 错误
        Sch->>DMA: 只放行已校验批的 KV
        Sch->>MAC: 按 batch 顺序消费 READY KV tile
    end
    alt 全部 Index/KV/计算成功
        Sch->>O: 归一化后写每 head 的 O
        O-->>Sch: 所有 O 写入 ack
        Sch-->>Host: 一次正常 completion
    else 后批 Index 错误、KV 错误或取消
        Sch->>Idx: 停止新请求，drain 已接受 Index 响应
        Sch->>DMA: 停止新 KV 请求，drain 已接受 KV 响应
        Sch->>Sch: 丢弃候选行状态，整次 O 无效
        Sch-->>Host: 错误或取消完成
    end
```

Index slot 在本批所有位置已发 KV 请求或因 mask 跳过，且 metadata 已复制
到 KV slot 后释放；KV slot 在所有需要的 head wave 消费完后释放。较晚批
发现重复、越界或总线错误时，先前批可以已经发过 KV 请求，但 O 尚未写回。
head-specific Index 逐 head 使用同一规则；首版一次处理一个 head 的列表，
不把不同 head 的同号批次当作同一 tile。总线和 slot 反压使重叠成为条件能力，
详细状态与错误优先级见 [IDX-STREAM-1007](fa2-core-idx-stream-1007.md)
和 [SKV-1005](fa2-core-skv-1005.md)。
