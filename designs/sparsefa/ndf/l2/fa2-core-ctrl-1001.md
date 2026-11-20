# FA2-CORE-CTRL-1001：命令 ABI、张量调度与生命周期 {#FA2-CORE-CTRL-1001}

<!-- ndf: kind=mechanism modality=must refinement=L2 status=draft owner=flash_attention2 refines=FA2-CORE-CTRL-0001,FA2-CORE-PROFILE-0001 -->

## 顶层端口和描述符

`FA2TensorScheduler(command:TensorCommand,memory:MemoryReply,completion_ready:u1,
 cancel:u1) -> tuple[MemoryRequest,EngineStatus]`。MemoryRequest直接交给总线，
EngineStatus交给控制面，两个独立接口不包成无关字段的大Struct。

| TensorCommand字段组 | 类型 | 解释 |
| --- | --- | --- |
| valid / tag | u1 / u32 | 请求与调用者提供的身份；无需全局单调递增 |
| batches / heads | u16 / u16 | B/H，非零 |
| queries / keys | u32 / u32 | S_q非零，S_k可为零 |
| dimensions / scale | u7 / u32 | D1..64；binary32 scale |
| indexed / causal / bf16 | 各u1 | 选择集合、mask和输入输出格式 |
| causal_origin / causal_origin_negative | u32 / u1 | causal_offset的绝对值和符号 |
| q/k/v/o/count/index_base | 各u64 | 六个逻辑对象的byte base |
| 各对象batch/head/row_stride | 各u64 | 独立byte stride；Index最内维固定4，tensor最内维2 |

负origin用sign/magnitude避免前端有符号算术歧义：query<origin时causal_empty=1；
否则position=query-origin。正origin先用u64检查S_q-1+origin≤2^32-1，再形成u32
position。负零等价零。原token的位置不随列表排列变化。

描述符按成功valid&ready边沿整体快照。运行中外部command变动不能影响active operation。
单行child命令由快照和b/h/query计算，child复用唯一实例，禁止每行创建新状态身份。

### 行引擎内部 ABI

`FA2Engine(command:EngineCommand,memory:MemoryReply,completion_ready:u1,
cancel:u1) -> tuple[MemoryRequest,EngineStatus]`。外层到行引擎直接传EngineCommand，
不把字段拆成平行端口再重建。

| EngineCommand字段 | 类型 | scheduler产生方式 |
| --- | --- | --- |
| valid / tag | u1 / u32 | ROW_OFFER与原tag |
| q_base / o_base | u64 | 原base+b*batch_stride+h*head_stride+query*row_stride |
| k_base / v_base | u64 | 原base+b*batch_stride+h*head_stride |
| count_base / index_base | u64 | 当前b/h/query独立地址 |
| k_stride / v_stride | u64 | K/V各自row_stride |
| key_length / query_position | u32 | S_k、计算后的原query位置 |
| dimensions / scale | u7 / u32 | 原D、binary32 scale |
| indexed / causal / causal_empty / bf16 | 各u1 | 模式及负origin导致的空集合 |

EngineStatus包含command_ready:u1、completion_valid:u1、tag:u32、code:u4、phase:u6。
parent与child各持有自己的phase，外层完成仅表示整个tensor终止。这里没有隐式doorbell、
MMIO寄存器映射或命令队列；SoC适配器需将其接到这组明确的握手接口。

## 地址与布局验证

对任一对象，令row_width为2D（tensor）、4（count）或8192（index）：

```text
row_extent  = (rows-1)*row_stride + row_width
head_extent = (H-1)*head_stride + row_extent
last_end    = base + (B-1)*batch_stride + head_extent
```

所有中间使用128位，zero rows单独处理，禁止无符号减1回绕。tensor base/stride按2
对齐，metadata按4对齐。只读输入显式允许zero-stride广播；非零stride依次容纳前一级
extent。O禁止广播：多行row_stride≥row_width，多head head_stride≥row_extent，
多batch batch_stride≥head_extent。合法layout带padding，不要求紧密连续。

独立对象之间的内存不别名、allocation权限和内容稳定由调用方保证；本接口不带区间
权限表。总线错误只能在访问时报告。错误描述符捕获后返回code1且无任何访存。

## 外层 FSM

| phase | 状态 | 推进条件与原子效果 |
| ---: | --- | --- |
| 0 | IDLE | valid且非cancel：快照、清游标/错误；非法→3，合法→1 |
| 1 | ROW_OFFER | child.command_ready且非cancel：同拍行命令接受，→2 |
| 2 | ROW_RUN | 等child completion；错误/取消→3；成功则query进位到head/batch |
| 3 | TERMINAL | completion_valid保持；completion_ready→0 |

row completion同时被child和parent接受；下一行最早在child回IDLE后提交。全局cancel
在ROW_RUN锁存直到child完成drain；cancel短脉冲不能让排空中的操作重新恢复。ROW_OFFER
取消不发child命令，直接终止。TERMINAL不重复完成，done反压不丢tag/code。

## 行引擎阶段

编码是诊断ABI，phase字段u6，外层status.phase报告外层状态，不把两个FSM编码混用。

| phase | 作用 | 完成后 |
| ---: | --- | --- |
| 0 | IDLE | 接受→COUNT或Q |
| 1/2 | COUNT_REQ/WAIT | count校验→INDEX或Q |
| 3/4 | INDEX_REQ/WAIT | 收index→25 |
| 25 | TRIE_READ | 非首项读node[cursor]；→5 |
| 5 | TRIE_SEARCH | 首项→33；branch下降→25；leaf重复报错，否则计算split→27 |
| 27/28 | TRIE_RETRACE | 读/检查插入点；继续下降→27，找到→29 |
| 29/30 | TRIE_ALLOCATE | 依次写新leaf、新branch；→31 |
| 31 | TRIE_LINK | 替换parent child或root，推进allocator；→32 |
| 33 | TRIE_FIRST | 写首leaf，初始化root/allocator；→32 |
| 32 | INDEX_PUBLISH | 写indices[p]并推进；下一index或Q |
| 6/7 | Q_REQ/WAIT | 下一d，或→8 |
| 8 | LOAD_TOKEN | 取index[p]或dense位置到token→26；列表耗尽→24 |
| 26 | MASK_TOKEN | 按原token筛选：skip→8；保留→9 |
| 9/10 | K_REQ/WAIT | 取query_lane、dot更新；最后d→23 |
| 23 | SCALE_SCORE | dot×scale→11 |
| 11 | MAX | candidate最大值→18 |
| 18 | ALPHA | exp(old_m-new_m)→19 |
| 19 | BETA | exp(score-new_m)→20 |
| 20 | DENOM | l×alpha+beta→12 |
| 12/13 | V_REQ/WAIT | 捕获V/active acc→21 |
| 21 | RESCALE_VECTOR | active_acc×alpha→22 |
| 22 | ACCUM_VECTOR | 加beta×V，写shadow；最后d切换bank/m/l→8 |
| 24 | LOAD_OUTPUT | 读取active accumulator→14 |
| 14/15 | O_REQ/WAIT | 等写ack；下一d→24，最后d→16 |
| 16 | DONE | completion握手→0 |
| 17 | DRAIN | 等唯一旧响应→16 |

`PhaseController`独占phase/code，`ControllerTransition`计算下一阶段；FA2Engine
独占描述符、数值状态、游标与三块Memory，`IndexTrie`独占重复检测节点和遍历状态，
以结构化TrieStatus交给控制器；`Datapath`负责组合计算。控制和数据Rule
均观察提交后的phase；不能在一次边沿穿越多个访存阶段。各数据Rule由互斥phase选择，
该阶段的Memory端口无竞争，读取地址已初始化并在范围内；因此控制推进与对应数据
操作在同一成功边沿提交。编译器负责资源可用性和全系统提交检查，没有手写内部ready
转发。若以后加入端口仲裁或多周期资源，必须重新建立此同步条件，不能只改FSM。

最后V维的shadow写入、bank切换、m/l和position更新属于同一数据Rule。其他跨阶段
临时值保存于所属Module状态。Rule内部赋值按顺序可见，不能按非阻塞RTL赋值理解。
精确周期和可运行性以当前源码及生成System为准，阶段表不能替代编译执行证据。

## 错误码与优先级

| code | 触发 |
| ---: | --- |
| 0 | 所有输出写均成功ack |
| 1 | 维度/scale/layout/alignment/address overflow/profile错误 |
| 2 | count>min(2048,S_k) |
| 3 | index≥S_k |
| 4 | 有效前缀重复 |
| 5 | memory.error |
| 6 | cancel |
| 7 | 已读取Q/K/V元素超数值域 |

cancel优先于当前有效响应的数值/总线错误；取消响应只drain，不提交数据。单个错误
响应同时含非法载荷时code5优先。进入终止后保持原code，不被下一命令覆盖。无正常
completion之前，O整体无效；无写回rollback。外部复位需配合bus fence。

非取消的错误响应同拍仍可更新count/index暂存、Q buffer、dot或V暂存；phase转入
终止后不再消费这些值。该内部更新不发布有效行状态，也不要求RAM回滚。下一命令的
初始化与有效前缀校验负责隔离旧值；具体响应行为见[XFER](fa2-core-xfer-1001.md)。
