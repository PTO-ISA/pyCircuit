# FA2-CORE-IDX-1003：精确 Patricia 索引校验器 {#FA2-CORE-IDX-1003}

<!-- ndf: kind=mechanism modality=must refinement=L2 status=draft owner=flash_attention2 refines=FA2-CORE-IDX-0001,FA2-CORE-PROFILE-0001 -->

本页描述P0S索引性能优化实现，执行状态见[VALIDATION](../../VALIDATION.md)。外部提供uint32 index，保留原顺序、
完整前缀验证、memory/range/duplicate错误优先级及取消/drain协议。替换原逐项扫描重复
检测，不生成index、不排序执行列表、不改变KV地址和数值运算顺序。L0保持冻结。

## 存储与状态

原indices 2048×32仍保存调用者顺序。新增4096×64节点Memory（32768 byte），
最多2048叶+2047分支=4095个可达节点，总主要数据存储由8.75KiB增至40.75KiB。
独立 `IndexTrie` Module拥有此Memory、root、next_free、cursor、node、split_bit、
parent、parent_node、parent_right、parent_valid。逻辑指针用u13，编码指针用低12位；
next_free最大4095，分配前断言next_free≤4093。节点地址为slot×8，15-bit byte address。

| 节点类型 | 64-bit布局 |
| --- | --- |
| leaf | bit63=1；bits0..31保存完整uint32 token；其余0 |
| branch | bit63=0；bits32..36分歧位b；bits37..48左child；bits49..60右child；其余0 |

branch按token[b]选择child；沿路径的b严格递减，最多32个branch。first index建leaf0，
root=0、next_free=1。后续每个独特token分配新leaf和branch，next_free加2。
phase0清root、next_free等生命周期状态，RAM不用全清；position=0表示空树，禁止读旧root。
重启后只有新建节点可达。Memory不承担无界key bitmap或hash冲突处理。

## 两次遍历及提交

`IndexTrie(incoming_index,position,phase,memory_valid,cancel)->TrieStatus`。
TrieStatus的is_leaf/matches/insert_here/has_parent字段一起交给PhaseController。
phase来自既有controller，一个step Rule以实际active phase和~cancel控制相关状态更新，
phase4的响应捕获另经memory_valid约束；无手写内部ready。
phase29/30/31/33的互斥节点写入共用step内一个write调用，组合mux选择提交前状态的
地址和数据。phase31仅parent_valid时写Memory；没有parent时更新root。Memory写入、
容量断言及指针更新属于同一原子事务，断言失败不得提交暂存的写请求。
INDEX_WAIT仍先按memory.error/range终止，之后才开始树访问。首项也经过25/5，保持
phase4成功到25的诊断边界。phase5不再是旧逐项比较语义，其余新增phase见下表。

| phase | 动作与下一阶段 |
| ---: | --- |
| 4 | 有响应时cursor=root，parent_valid=0；engine捕获incoming_index；控制器保留错误优先级 |
| 25 | position>0时读node[cursor]到node；空树不读；→5 |
| 5 | 空树→33；branch按bit选择child→25；leaf相等→code4；不等计算最高不同bit，cursor=root，parent_valid=0→27 |
| 27 | 读node[cursor]→28 |
| 28 | leaf或node.bit<split_bit时找到插入点→29；否则保存parent及方向并下降→27 |
| 29 | 写新leaf至next_free→30 |
| 30 | 写新branch至next_free+1，children为新leaf和当前cursor，按新key的split_bit排列→31 |
| 31 | 有parent时替换parent对应child；否则更新root；next_free+=2→32 |
| 33 | 首leaf写slot0，root=0、next_free=1→32 |
| 32 | engine写indices[position]并推进position；整前缀完成则position=0→Q，否则→INDEX_REQ |

分歧位为31-clz(incoming_index XOR leaf.key)，仅两个不同有限uint32值执行该计算。
两次遍历保持固定 key 宽度上界，每次至多沿 32 个 branch 检查节点；
节点写入和原序列表发布分属明确的状态阶段。实际路径深度由 key 集合决定，
不在 NDF 中推导每项或整批校验的完成时延。

parent链接公布前只写新节点；取消在任一阶段抑制新写并进入原终止/drain路径。
链接已公布而indices尚未发布时取消也允许留下内部无效数据；没有Q/KV访问，下一命令
重新建立空树。只有phase32提交才请求下一个index，因此后续错误不会覆盖较早duplicate。
phase31修改parent只写一个节点；每阶段最多一个Memory读或写，避免多端口假设。

## 验收与边界

实现前独立审查树不变量、指针边界、最高位分裂和发布/取消；实现后独立host set oracle
覆盖N128/512/2048、升序/降序/无序、高uint32、相同低位/长公共前缀、首尾duplicate、
range/error优先级、空表、cancel/restart、密集路径不访问树。相同C++适配器和输入运行
旧/新生成DUT，检查命令接受、首Q、首KV和最终完成的外部
请求顺序、结果和单outstanding。保持原完整System和controller四态回归。

同一verified final生成C++/Verilog，执行C++ workers1/2；按用户要求不做RTL仿真。
新树增加的容量、逻辑CLZ和节点读延迟需后续宏映射/时序验证。没有物理频率或端到端
带宽承诺，完整2048有效列表以本次实际执行结果为准。
