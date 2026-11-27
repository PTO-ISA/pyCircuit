# FA2-CORE-BUF-1001：P0S Buffer 规格、布局与端口 {#FA2-CORE-BUF-1001}

<!-- ndf: kind=mechanism modality=must refinement=L2 status=draft owner=flash_attention2 refines=FA2-CORE-BUF-0001,FA2-CORE-PROFILE-0001 -->

## 存储清单

本页描述 `src/fa2_engine.py` 的 P0S 工作集。FA2Engine拥有Q、索引顺序和累积三块
Memory，子模块IndexTrie独占节点Memory；外层scheduler只持有描述符和b/h/query游标。

| 名称 | 深度×位宽 | 容量 | 逻辑地址 | 使用方式 |
| --- | --- | ---: | --- | --- |
| queries | 64×32 | 256 B | byte address `4*d` | Q 输入解码后保存 binary32 |
| indices | 2048×32 | 8192 B | `4*p` | 保存已验证的当前行索引前缀 |
| IndexTrie.nodes | 4096×64 | 32768 B | `8*slot` | 精确uint32重复检测，最多4095个可达节点 |
| accumulators | 128×32 | 512 B | `4*(bank_offset+d)` | bank_offset=0/64，active/shadow 各64项 |
| query_lane / accumulator_lane / value_lane | 各1×32 | 12 B | 标量状态 | 本次 MAC/normalize 的操作数 |
| dot / score / maximum / denominator | 各1×32 | 16 B | 标量状态 | 当前 token score 和已提交行状态 |
| next_maximum / next_denominator / alpha / beta | 各1×32 | 16 B | 标量状态 | 待提交 token 的候选行状态 |
| scaled_accumulation | 1×32 | 4 B | 标量状态 | PV 两次乘法之间的中间值 |
| incoming_index | 1×32 | 4 B | 标量状态 | 当前外部索引 |
| IndexTrie状态 | 见IDX-1003 | root/cursor/allocator等 | u13逻辑指针、u64节点缓存 | 两次遍历和节点发布 |
| 描述符/游标 | 见源类型 | FA2Engine状态 | saved/position等 | 地址和进度 |
| phase / code | 6+4 bit | PhaseController状态 | controller结果 | 阶段、终止和错误 |

四块主要数据存储合计 **41728 B = 40.75 KiB**，不含标量寄存器、编译器生成的控制
和数值算术中间线网。没有完整 K/V tile、score 矩阵或 probability 矩阵。K/V 从外部
逐元素读取，K 直接进入 dot，V 暂存一个标量后更新 accumulator。

## 数据布局

片外 Q/K/V/O 每元素2字节，片内 Q 和 accumulator 每元素4字节。FP16 与 BF16 在
片内均转换为相同 binary32 编码；Index 始终 uint32，不经过浮点解码。

```text
queries:       [q0 q1 ... q63]
indices:       [j0 j1 ... j2047]
trie.nodes:    [4096个64-bit节点；leaf/branch布局见IDX-1003]
accumulators:  [bank0:d0..d63][bank1:d0..d63]
active = bank_offset | d; shadow = (bank_offset XOR 64) | d
```

运行时 d 仅遍历 0..D-1。D 可为1或64，不用 D 的低6位表示长度；长度使用 u7，
游标使用 u6。count 使用 u32，索引物理地址使用低11位位置，但仅在 `p<count≤2048`
的阶段有效；计数2048不能截成0。完成后保留的 RAM 内容不赋予下一行有效性。

## 端口、延迟与冲突

源码采用 byte-addressed Memory 分配和 Rule 的 read/write，不手写 SRAM 物理端口。
读值在该 Rule 的 Work 中参与计算，写入在成功边沿提交；跨阶段保留的读值进入对应
lane 状态。实际宏映射由编译器/综合负责；这里没有声称得到同步 SRAM 宏面积或频率。

| 存储 | 互斥阶段的访问 | 所需逻辑端口 |
| --- | --- | --- |
| queries | Q response 写；K 运算前取当前 d | 1R、1W，装载/消费不重叠 |
| indices | phase32校验后写；phase8取p | 1R、1W，插入/消费分阶段 |
| IndexTrie.nodes | phase25/27读；29/30/31/33写 | 1R、1W，每阶段最多一次操作 |
| accumulators | Q load 清当前 bank；PV 读 active/写 shadow；O 读 active | 1R、1W；不同地址的活跃/影子 bank |

若目标同步 SRAM 的读延迟与当前 Memory 语义不同，必须插入并验证请求/结果阶段，
不能把“1R1W”文字当作无代价映射。综合前还需核实容量、enable、读写同址语义和映射
报告。P0T 三维阵列每侧 16-bank 宽读与双缓冲候选见
[ARRAY3D-BUF-1004](fa2-core-array3d-buf-1004.md)，不适用于这份 P0S 端口预算。

## 有效性与原子提交

1. 新 query 从 d=0 开始装载 Q，同时把 active bank 的0..D-1清零；旧 shadow 不必清零。
2. index[0..p-1]保存已验证前缀的原顺序；IndexTrie只访问当前命令建立的可达节点。
   首项不读旧root，phase0清控制状态，RAM无需清零；节点布局和取消边界见[IDX-1003](fa2-core-idx-1003.md)。
3. 每个 token 的所有 V 维写入 shadow 后，在最后一维更新边沿同时切换 bank、m、l
   和 token 进度。此前 active bank 保留上一 token 的完整结果。
4. V 错误或取消时不发布影子版本；整个 O 无效。下一 query 仍按第1步初始化，不依赖
   取消后 RAM 清零。复位清控制有效性，外部未完成事务另按传输合同处理。
5. 归一化只读 active；写请求反压时 accumulator_lane、l、d 不变，因此写数据稳定。

## 必验场景

D=1/63/64、count=0/1/2048、首 token=0、上一行残留与新行前缀隔离、最后 V 的错误、
部分 shadow 更新后取消、O 反压和连续行。容量测试必须实际执行对应边界，不能用
静态声明尺寸替代。RTL SRAM 推断和面积报告仍是独立验收项。
