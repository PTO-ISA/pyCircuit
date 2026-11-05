# WSE Calendar 方案

> **一句话**：路由信息压成 **40 × 2 bit = 80 bit 的 `routeBits`** 位图，作为**编译期静态表项落在 `.rodata`**，
> 发包时由 SU 一条普通标量 LD 取出（miss 走标准 D-Cache 通路，**经 Batcher.mem 从 DDR 回填——不经 local DRAM、不经 L1**）；
> 时隙信息的**值**由编译期的 NoC 算法生成并**常驻在 NoC 节点的 Calendar 时隙寄存器 `CalReg`**，核内只给一个
> **编译期立即数 `opcode`**（值由集合语义决定）——从核的角度看，这就是对 NoC 节点寄存器的一次索引 `CalReg[opcode]`。
> 二者都在**发包指令**处进入数据面，因此不存在任何"每轮配置一次的硬件状态"，也不存在与之配套的配置可见性栅栏。
> **控制面新增机器指令 0 条，数据面新增 2 条。**

**本文的适用范围**：WSE die 上 **40 个 NoC 节点**、SPMD 执行模型、Calendar 平面上的集合通信
（`AllGather` / `Reduce` / `AllReduce` / `ReduceScatter` / `Scatter` / `Gather`，首期实现 `AllGather` 与 `Reduce`）。
全文的示例统一采用一个 FFN 场景：40 节点排成 **5 × 8 mesh**（`node = r*8 + c`），FFN 用前 4 行共 **32 个 cell**，
phase B 沿 ROW 组做 8 路 `AllGather`、phase C 沿 COL 组做 4 路 `AllGather`。

**全文六章的关系**：

```text
第 1 章  整体流程          —— 一轮集合在时间轴上长什么样（Step0–Step6、不变式、同步层次）
第 2 章  编码表达          —— routeBits 与 opcode 这两个值本身怎么编码、由谁算出来
第 3 章  下发链路          —— 这两个值从"被制造"到"被指令读到"经过哪些存储层级
第 4 章  硬件指令          —— 指令流里出现哪些机器指令、各自的操作数与硬件契约
第 5 章  CCE / PTO 接口     —— 这些机器指令在 CCE facade 与 PTO 包装层上长什么样
第 6 章  依赖项与待决项     —— 落地前必须由硬件/NoC/运行时兑现的项，以及尚未冻结的选择
```

---

## 1. 整体流程

### 1.1 两阶段总体流程

Calendar 的数据流分成**编译与安装**、**设备执行**两个阶段。两类拓扑信息（路径、时隙）在编译期由同一次 NoC
算法调用联合产出，然后走两条完全不同的下发路线：路径进核侧 `.rodata`，时隙进 NoC 节点寄存器。

```text
阶段一：编译与安装

PyPTO 图/算子
    ├─ 从 IR 识别 GROUP、集合通信语义与集合间并发关系；读入目标芯片的拓扑信息与 PG 信息（不属于 IR）
    ├─ 调用 NoC 提供的 C 算法，一次同时产出两类东西：
    │     ① 逐 source 的 40 × {P,L} routeBits（"走哪里"）
    │     ② 全域 NoC 节点时隙寄存器内容 CalReg[opcode]（"何时放行"；内部格式由 NoC 定义）
    ├─ ① 发射成 .rodata 静态表：kCalendarRoute[keyId][node]
    │     每种集合语义（keyId）存满 40 张 40×2bit=80bit 位图；40 核镜像逐字节相同
    │     本核发包时用运行期 blockId 索引出属于自己的那一张
    ├─ ② 交付给装载期，一次性写入各 NoC 节点的时隙寄存器
    └─ 调用点只固化三个编译期立即数：keyId / opcode / expVal
       （expVal = 落核源数 × tile 几何，由 PyPTO 推出）；再独立规划 selfOff 与事件依赖
                         │
                         ▼
Host / Runtime
    ├─ kernel binary（.text + .rodata 同一批）一次 H2D → DDR 代码段/数据段
    │     取指/取数 miss 时由 Batcher.mem 供给各核 I$/D$（不经 local DRAM、不经 L1）
    ├─ 把 CalReg 全域镜像原子写入各 NoC 节点时隙寄存器（停机窗口内，kernel 期只读）
    ├─ 三版本联合校验（topologyVersion / calendarVersion / routeVersion）；提供 blockId
    ├─ 不分配 notify / 信号量，不预建 collectionEpoch
    │  （每轮 epoch 由核内按 opcode 域递增的计数器给出，见 §1.5）
    └─ 按完整 group 调度：分 wave 时不得拆开正在同步的组

    ★ 不做 per-core 展开、不下发任何运行期描述符、不经 L1 中转，
      每次 launch 不下发任何 Calendar 数据

阶段二：设备执行（一轮集合 = Step0…Step6）

AICORE（按 AICORE 发出的指令列步骤）
    ├─ Step0   MatMul：本核前序运算，产出本轮要广播的 sendTile
    ├─ Step1   Barrier：本核前序指令完成 + 通信数据就绪
    ├─ Step2   Align：复用既有跨核报到/汇合指令做全组 rendezvous（信号量 id = opcode）；
    │                BSP 以该汇合点为时间原点对齐全组（程序不感知 cycle）——不新增 ISA
    ├─ Step3   RECV：向 MTE4 下发带 expVal 的接收指令，按本轮期望接收字节数阻塞完成
    ├─ Step4   SEND：MTE3 发包，每包携带 {routeBits80, opcode}；NoC 按 {P,L} 被动转发/落核，
    │                NoC 节点按 CalReg[opcode] 放行
    ├─ Step5   Join：排空 MTE3，并等待 Step3 的 MTE4 感知指令因达到 expVal 而完成
    └─ Step6   返回 RecordEvent，回到 Step0 或接续 TSTORE、Vector、下一段通信

    注：SEND 所需的 routeBits 由编译器生成的普通标量 LD 从 .rodata 读入寄存器对；D-Cache miss 由硬件
        经 Batcher.mem 从 DDR 回填（不经 local DRAM、不经 L1），对用户透明，不是 Calendar 专用指令，
        因此不单列步骤；opcode 是立即数，随取指进入 I-Cache，无需访存
```

### 1.2 端到端执行流程（Step0–Step6）

这是本方案的**权威口径**，后文的时空图（§1.3）、指令流伪码（§1.7）与下沉伪码（§5.9）一律按此对齐。
一轮集合通信 = Step1…Step5；Step0 是它的生产者，Step6 是它的消费者。表中只列 AICORE 需要发出的指令。

| Step | 名称 | 做什么 | 机器形态 | 流水线 |
|---|---|---|---|---|
| **Step0** | `MatMul` | 矩阵乘等本核前序运算，产出本轮要广播的 `sendTile` | 既有 `TMATMUL` / Vector 指令 | `PIPE_M` / `PIPE_V` |
| **Step1** | `Barrier` | 本核前序指令全部完成 **且** 通信数据就绪 | `TSYNC(events...)` + `pipe_barrier(PIPE_ALL)` + `dsb(DSB_DDR)` | 全流水 |
| **Step2** | `Align(opcode)` | 向 BSP 报到"本核已就绪"，阻塞到全组到齐（时间原点由硬件内部对齐，程序不感知 cycle）；硬件预留的 arm lead time 用于下发 Step3/4 | **复用既有 ISA，无新增**：`SET_CROSS_CORE(calGroup, opcode)` 报到 + `WAIT_FLAG_DEV(opcode)` 等齐；`opcode` 直接充当信号量 id（§4.2） | 控制 / BSP |
| **Step3** | `MTE4 expVal 感知` | 为本核 `recvTile` 建立本轮接收范围与期望字节数。**MTE4 不搬运数据**——载荷由 NoC 直接写进本核竞技场，MTE4 只对匹配 `{opcode, epoch, dstRange}` 的有效载荷做感知与计量，累计达到 `expVal` 才完成 | ★`MTE4_NOC_RECV_WAIT(dst, capacity, expVal, opcode, epoch)`（§4.5）；阻塞的是 MTE4 命令完成，不得阻塞 AICORE 继续向 MTE3 发指令 | `PIPE_MTE4` |
| **Step4** | `MTE3 send` | 在 arm lead time 内入队，**每条发包指令携带 `routeBits80` 与 `opcode`**；由 NoC 按 `CalReg[opcode]` 放行。**核间通信的发包一律由 MTE3 承担**——它就是既有的核间写通路，本方案只给它加操作数 | ★`MTE3_NOC_SEND(dstSym, src, sid, nBurst, lenBurst, srcGap, dstGap, am0, am1, rbLo, rbHi, opcode, redOp)`（§4.4） | `PIPE_MTE3`（既有流水，非新增） |
| **Step4s** | `Self copy` | fabric 不 self-delivery 时逐行补写本核段；本地字节**不计入** `expVal` | 复用 `copy_ubuf_to_ubuf` | `PIPE_MTE3` |
| **Step5** | `Join` | 先排空本核 MTE3 发包，再等待 Step3 的 MTE4 感知命令完成 | `pipe_barrier(PIPE_MTE3)` + `pipe_barrier(PIPE_MTE4)`；无 `Notify`、无 `WAIT_SPR` | `PIPE_MTE3` + `PIPE_MTE4` |
| **Step6** | `next` | 返回 `RecordEvent`；回到 Step0 或接续 `TSTORE` / Vector / 下一段通信 | 调用方 `Event<Op::TCALENDAR, Op::Y>` | — |

> **为什么不单列"取路由"步骤**：`routeBits` 是发包指令的寄存器操作数，编译器按 `kCalendarRoute[keyId][blockId]`
> 自动生成两条普通 64 bit 标量 LD 把它读进寄存器对（可调度到 Step1 之前，让可能的 miss 与"等数据"重叠）；
> 命中即返回，miss 由 D-Cache 硬件经 Batcher.mem 从 DDR 回填。它对用户透明、不是 Calendar 专用指令，
> 因此上表只列 AICORE 需要发出的 Calendar 相关步骤。`opcode` 是立即数，随取指进入 I-Cache，根本不需要访存。

### 1.3 一次集合的时空图

横轴是时间，每一行是一个成员；`====` 表示**阻塞**。全组在 [A] 处对齐（Step2 吸收各核到达时间差），
在 [D] 处各自等齐（Step5）。

```text
  time ------------------------------------------------------------------------------------------------->

             Step0       Step1        Step2            Step3          Step4         Step5      Step6
             MatMul     Barrier   Align(SET_CROSS_CORE)  MTE4 RECV     MTE3 SEND       Join      next
          +----------+-----------+--------------+---------------+--------------+------------+-------+
 member 0 |  MatMul  |  barrier  | ====>        | post expVal   | row0..row7   | ====> OK   | next  |
 member 1 |  MatMul  |  barrier  | ==========>  | post expVal   | row0..row7   | =====> OK  | next  |
 member 2 |  MatMul  |  barrier  | =>           | post expVal   | row0..row7   | ==> OK     | next  |
 member 3 |  MatMul  |  barrier  | =======>     | post expVal   | row0..row7   | =======> OK| next  |
          +----------+-----------+------^-------+-------|-------+------|-------+-----^------+-------+
                                        |               |              |             |
                                       [A]             [B]            [C]           [D]

  Step0 MatMul    = 本核前序计算，产出 sendTile
  Step1 Barrier   = 前序指令完成 + 通信数据就绪（吸收了 TSYNC 与 release 两件事）
  Step2 Align     = SET_CROSS_CORE(calGroup, opcode) 报到 + WAIT_FLAG_DEV(opcode) 等齐（均为既有 ISA）；
                    BSP 以该汇合点对齐全组时间原点并留出 arm lead time
  Step3 MTE4 RECV = 下发 MTE4_NOC_RECV_WAIT(dst, capacity, expVal, opcode, epoch)；按匹配数据字节量完成，
                    只阻塞 MTE4 命令，不阻塞 AICORE 向独立 MTE3 流水继续派发
  Step4 MTE3 SEND = 入队发包，每条携带同一份 {routeBits80, opcode}；由 NoC 按 CalReg[opcode] 放行。
                    落点 = 对称地址 + selfOff + r × 行跨距；路径由 routeBits 决定
  Step5 Join      = 先 pipe_barrier(PIPE_MTE3) 排空本核发送，再等待 Step3 的 MTE4 命令退休
  Step6 next      = 返回 RecordEvent，回到 Step0 或接续 TSTORE / Vector / 下一段通信
  [A] BSP 对齐点  = 全组对齐（时间原点由硬件内部对齐）；其后的 arm lead time 用于 MTE4/MTE3 arm
  [B] MTE4        = 建立本轮接收范围与期望字节数；早到数据也必须按 {opcode, epoch} 正确记账
  [C] NoC         = MTE3 发出，拆 flit / 按 {P,L} 被动转发 / 按 CalReg[opcode] 放行 / 目的端 merge
  [D] 收侧完成    = 匹配有效载荷累计达到 expVal 后 MTE4 命令退休；`====` 段是阻塞
  本轮 expVal（AllGather，不含本地自拷贝）= rowCount × rowBytes × (memberCount − 1)
  注：SEND 的 routeBits 操作数由编译器生成的普通 LD 取入寄存器（可提前到 Barrier 之前），
      D-Cache miss 由硬件回填，不单列步骤
```

> **完成机制**：集合流程不设置独立 `Notify` 列、`notify=ON` 字段或 `WAIT_SPR`；完成条件是 MTE4 对实际接收
> 有效载荷的字节量判断。末尾 `setflag` 由调用方的 `Event<Op::TCALENDAR, Op::Y>` 产生，不属于集合本身；
> 发送 drain 并入 Step5 的第一动作。

### 1.4 排序不变式

四条排序不变式分别约束路由/时隙一致性、避冲突、完成活性与操作数就绪；第五条是 SPMD 下的分支纪律。

| # | 不变式 | 为什么 | 破坏后果 |
|---|---|---|---|
| **O1** | **路由与时隙同源**：同一次发包携带的 `routeBits` 与 `opcode` 必须来自**同一个逻辑身份**；程序不得把 A 语义的 `opcode` 配 B 语义的 `routeBits` | 本方案**没有"一条指令原子选中路由/时隙"的硬件锚点**——`routeBits` 走 `.rodata`/D-Cache，`opcode` 走 `.text`/I-Cache，两条链物理分离 | 路径与时隙错配：包沿正确路径走、但按错误的时隙寄存器项放行 ⇒ 计划外链路冲突、拥塞，且**编译期查不出来** |
| **O2** | **对齐先于实际发送**：Step2 全组对齐返回后，Step4 才入队 MTE3；对齐由硬件完成，程序不感知 cycle | 全组共用同一项 `CalReg[opcode]`；成员若未经对齐就发包，各自在硬件时间轴上的起点不同，原本错峰的时隙会错位 | 原本错峰的 flits 冲突、拥塞；路径仍由 `routeBits` 决定，因此**不是静默错转发** |
| **O3** | **感知可记账先于发送，发送排空先于最终等待**：先发出 Step3 的 MTE4 `expVal` 感知命令，再发 Step4 的 MTE3；Step5 先排空 MTE3，再等待 MTE4 完成 | MTE4 与 MTE3 是独立流水；MTE4 的阻塞不能阻止 MTE3 前进。Step3 还负责建立本轮 `{opcode, epoch}` 上下文，Step4 的 flit `epochTag` 从中取值 | 丢失早到数据的计量、错误满足 `expVal`，或全组死锁 |
| **O4** | **`routeBits` 就绪先于首包注入**：读取 `routeBits` 的普通 LD 必须在 Step4 第一条 SEND 发射前写回 GPR | 由标量流水的 RAW 依赖天然保证；**但如果改用 §4.4 方案 B（一次锁存进 SPR），则必须显式加一道 SPR 写→MTE3 读的栅栏** | 用到未定义的路由位图，包被丢弃或错转发 |
| **E1** | **分支参与一致**：同一 `opcode` 域内的全部成员核，对集合调用的**次数与顺序必须逐次一致**；非成员核不调用是安全的，但成员核不得有条件地跳过某一轮 | 本轮 `collectionEpoch` 由每核私有的计数器给出，靠"SPMD 同构"保证全组取值一致 | `epochCtr` 在成员间错位 ⇒ 接收侧按错 epoch 丢包并 fault |

> **O1 是本方案的主要新增风险**，也是"删掉原子选表指令"的必然代价。缓解手段是把一致性上移到编译期与类型系统：
> §2.5 要求 `keyId` 与 `opcode` 由同一个逻辑身份一次解析出来并封装成不可拆分的编译期常量对（`CalendarKeyRef`），
> 调用点不得分别传入；§2.7 的编译校验再做一次交叉验证。

> **跨轮无需"重配置安全"这类约束**：本方案没有"当前活动路由/时隙配置"这个硬件状态，`routeBits` 只活在寄存器与 flit 头里，
> `opcode` 是立即数，因此不存在"旧窗口未排空不得覆盖"这一整类约束。Calendar 也**不占用任何 L1**，
> 与 GEMM 的 L1 分配器无互斥关系。

### 1.5 时隙对齐与本轮 `epoch`

对齐的语义位置固定在集合下沉内部：**Step1 数据就绪之后、Step3/4 数据面 arm 之前，每轮执行一次**
（此时 `routeBits` 已由普通 LD 读入寄存器）。

| 形态 | 含义 | 机器指令增量 | 结论 |
|---|---|---|---|
| **复用既有跨核报到 + 设备级等待** | 以信号量 id = `opcode` 向 BSP 报到并阻塞到全组到齐，由硬件内部对齐时间原点 | **+0（复用）** | **采用**（Step2） |
| 新增专用 `CALENDAR_ALIGN_REQ(opcode)` | 专门新增一条控制面指令做同一件事 | +1 | **不选**：报到/汇合的语义既有指令已经具备，`opcode` 又恰好能放进现成的信号量 id 槽（0–15），没有新增的理由 |
| 折进 Step4 首包 | 第一条 MTE3 发送隐式完成对齐 | 0 | **不选**：无法在首包被 `CalReg[opcode]` 放行前为 MTE4/MTE3 留出统一 arm lead time，且对齐/异常语义隐含 |
| 由装载期一次性对齐、运行期不再对齐 | 全片共用一个自由跑的硬件时间基准 | 0 | **不选**：各成员进入集合的时刻不同，仍会在同一项 `CalReg[opcode]` 上错峰失效（不变式 **O2**） |

**报到指令是纯 side-effect 指令、没有返回值**，因此本轮 `collectionEpoch` 不由对齐指令带回，
改由**核内按 `opcode` 域递增的轮次计数器**给出：

```text
epoch = CalendarNextEpoch(opcode)     // 纯标量自增，零 ISA、零访存、零 D-Cache 行；从 1 起算
```

| 侧面 | 内容 |
|---|---|
| **为什么全组一致** | SPMD 下同一 `opcode` 域的成员跑同一份 `.text`、执行同一条指令序列，且刚在 Step2 汇合过；每个 `opcode` 域的第 n 次集合在所有成员上必然拿到同一个 n（这正是不变式 **E1** 存在的理由） |
| **谁来用** | Step3 的 `MTE4_NOC_RECV_WAIT(..., epoch)` 用它建立本轮 `{opcode, epoch}` 上下文，Step4 的 flit `epochTag` 从该上下文取值，**不占 SEND 的编码槽** |
| **代价** | 唯一性不再由 BSP 集中保证，而由"SPMD 同构 + 同 `opcode` 不并发 + 回绕前旧 epoch 已排空 + 分支参与一致（**E1**）"四条共同保证 |
| **跨 launch** | 计数器落在 kernel 内的局部对象上 ⇒ **每次 launch 从 0 重置**，本 launch 第一轮 epoch = 1。跨 launch 的隔离**完全依赖"launch 边界前全片 Calendar 流量已排空"**（host 在相间插 barrier），不依赖 epoch 数值单调。若要求 epoch 跨 launch 继续单调，需要一块按 `blockId` 索引的每核私有可写存储，代价是每轮一次读改写（§6.3 待决项 C-5） |
| **arm lead time** | 对齐返回后到首个发包时隙之间的准备窗口由 **BSP / NoC 在该 `opcode` 域内部预留**，须覆盖 Step3 的接收命令与全部发包描述符入队的最坏时延。它**不是**程序可见操作数，程序不感知 cycle |

> ★ **对齐域按 `opcode` 划分，不按 group 划分**。一次 launch 内所有使用同一 `opcode` 的核都汇合在同一个
> 信号量域上：FFN phase B 是 4 个 ROW 组共 **32 核**同时报到，不是每组 8 核各自汇合。这对**时隙对齐是好事**
> （比按组对齐更强），但把到齐计数的要求抬到 32——对信号量位宽提出硬要求，见 §4.2 的 X-3 与 §6.1。

### 1.6 同步层次

| 层次 | 解决的问题 | 机制 | 对应 Step |
|---|---|---|---|
| **S1** 本核前置依赖 | `sendTile` 的生产者是否完成 | 包装层 `TSYNC(events...)` | Step1 |
| **S2** 跨流水线完成 | 哪个后继 pipe 可读 `recvTile` | `RecordEvent` + `Event<Op::TCALENDAR, Op::Y>`；源 pipe 为 `PIPE_MTE4` | Step6 |
| **S3** 跨核集合完成 | 远端是否已把本轮数据写入本核竞技场 | MTE4 对匹配 `{opcode, epoch, dstRange}` 的有效载荷按字节累计，达到 `expVal` 后完成 | Step3 / Step5 |
| **S4** 组起点对齐 | 成员是否处于同一轮 | 复用既有 `SET_CROSS_CORE(calGroup, opcode)` + `WAIT_FLAG_DEV(opcode)`，BSP 以该汇合点对齐时间原点 | Step2 |
| **S5** 路由信息就绪 | 本核这一发走哪些节点、落在哪些节点 | 编译器生成的普通标量 LD 从 `.rodata` 取 `routeBits`；**RAW 依赖即栅栏**，没有第二个可见性面 | （取数，不单列步骤） |
| **S6** 时隙选定 | 本核这一发按哪项时隙寄存器放行 | `opcode` 立即数随 flit 走，NoC 节点一次索引 `CalReg[opcode]` | Step4 |
| **S7** 轮次身份 | 本轮的接收记账用哪个 epoch | 核内 `CalendarNextEpoch(opcode)` 标量自增 | Step2 |

### 1.7 一次 AllGather 的完整指令流伪码

前置条件只是：kernel binary（含 `.rodata` 静态表）已下发到 **DDR**（各核 I$/D$ 经 **Batcher.mem** 回填），
且 `CalReg` 已写入各 NoC 节点时隙寄存器。

```text
Step0   ... TMATMUL / Vector ...                     // 产出 sendTile（调用方代码，不在指令体内）

(取数)  // ★ 不是 Calendar 步骤、也不是新增 ISA —— 编译器为 SEND 操作数生成的两条普通 64 bit 标量 LD
        GR_rb0 = LD [ &kCalendarRoute[keyId][0] + blockId*16 + 0 ]   // 立即数基址 + 运行期 blockId 索引
        GR_rb1 = LD [ &kCalendarRoute[keyId][0] + blockId*16 + 8 ]   // 必与上一条同 64 B 行 ⇒ 必命中
        // 命中：直接回 GR。miss：由 D-Cache 硬件经 MSHR → BIU → Batcher.mem 取回
        //       （Batcher.mem 未命中再回 DDR）→ 64 B 整行回填 → 之后本 key 不再 miss；全过程对用户透明
        // routeBits80 = GR_rb0[63:0] ++ GR_rb1[15:0]
        // expVal      = GR_rb1[63:32]      landCount = GR_rb1[23:16]      flags = GR_rb1[31:24]
        // ★ 可调度到 Step1 之前：与 sendTile 无数据依赖，miss 延迟可与"等数据"重叠

Step1   WAIT_FLAG(<srcPipe>, PIPE_MTE4, token)       // TSYNC(events...)：本核前置依赖
        PIPE_BARRIER(PIPE_ALL); DSB(DSB_DDR)         // 前序完成 + 发布 sendTile
                                                     //   ★ 这道栅栏【不】承担"配置对 router 可见"

Step2   SET_CROSS_CORE(calGroup, opcode)             // ☆复用既有 ISA，机器指令增量 +0
        WAIT_FLAG_DEV(opcode)                        //   opcode 是 3 bit 立即数，直接充当信号量 id
                                                     //   向 BSP 报到；阻塞到全组到齐（时间原点由硬件内部对齐）
                                                     //   程序不感知 cycle；硬件预留的 arm lead time
                                                     //   保证 Step3/4 来得及入队
        epoch = CalendarNextEpoch(opcode)            // 本轮 collectionEpoch：按 opcode 域递增的核内计数器
                                                     //   SPMD 下 40 核执行同一序列 ⇒ 计数器天然一致

Step3   expVal = GR_rb1[63:32]                       // 或用户显式传入的常量，二者必须相等
        MTE4_NOC_RECV_WAIT(
            dst      = recvSymOff,
            capacity = recvRowCount * recvRowStride,
            expVal   = expVal,                       // 单位：有效 payload bytes
            opcode   = opcode,                       // ★3 bit 立即数：判别键的一半
            epoch    = epoch)                        // ★来自 Step2 的核内计数器；本指令同时建立
                                                     //   本轮 {opcode, epoch} 上下文，Step4 的 flit
                                                     //   epochTag 从中取值，不占 SEND 编码槽
                                                     //   MTE4 只感知不搬运；AICORE 可继续派发 MTE3

Step4   // 形态一（整 tile 一条）：rowCount 行共用同一份 routeBits/opcode，几何用 srcGap/dstGap 表达
        MTE3_NOC_SEND(
            dstSym  = recvSymOff + selfOff, src = sendUb,
            sid     = 0,
            nBurst  = rowCount,  lenBurst = rowBytes,
            srcGap  = sendRowStride - rowBytes,  dstGap = recvRowStride - rowBytes,
            am0 = 0, am1 = 0,
            rbLo    = GR_rb0,                        // ★路由信息进发包指令：80 bit routeBits
            rbHi    = GR_rb1,                        //   硬件只取 rbHi[15:0]，高 48 bit 忽略
            opcode  = opcode,                        // ★时隙信息进发包指令：3 bit 立即数索引 CalReg[opcode]
            redOp   = redOp)                         //   3 bit 立即数；AllGather 时 dormant
        // 形态二（逐行）：nBurst = 1，循环 rowCount 次；ReduceScatter/Scatter 可在循环内换 rbLo/rbHi

Step4s  fabric 不 self-delivery（flags.selfLand == 0）时逐行补本地 UB → UB copy；本地字节不计入 expVal

Step5   PIPE_BARRIER(PIPE_MTE3)                      // 先排空本核对外发送（O3）
        PIPE_BARRIER(PIPE_MTE4)                      // 再等 Step3；收到的有效字节 >= expVal 才返回

Step6   return RecordEvent                           // 调用方据此发 SET_FLAG(PIPE_MTE4, <dstPipe>)
                                                     //   指令体内【不发】set_flag
```

Calendar 的完成机制不使用 `notify` / write-with-notify / `NOTIFY_GROUP` / `WAIT_SPR`。MTE3 发侧只提交 payload，
MTE4 收侧直接按已提交的有效 payload bytes 计量；不存在额外信号量写、门铃包或阈值影子。

**为什么取 `routeBits` 的 LD 可以放在 `TSYNC` 之前**（三条理由）：

1. **无数据依赖**：`keyId` 和 `blockId` 与 `sendTile` 的生产者无关，提前发出可让 D-Cache miss 延迟与"等数据"的空窗重叠；
2. **不需要可见性栅栏**：`routeBits` 只活在 GPR 与 flit 头里，**没有第二个可见性面**，普通标量 RAW 依赖就是全部；
3. **对齐语义干净**：Step2 报到时 `opcode` 已是立即数，因而可以直接充当报到消息的信号量 id，不需要任何表句柄或路由信息——
   这也是"复用既有指令、不新增控制面 ISA"能成立的前提。

### 1.8 NoC 数据形态与 flit 头开销

- 软件每行发送一段连续字节（或用 `nBurst` 一条指令描述整个二维 tile），不逐 flit 编程；尾 flit、仲裁、
  重传和目的 merge 由 NoC 处理。
- **每个 flit 携带同一份 `{opcode(3b), redOp(3b), epochTag, routeBits(80b)}` 头部**。NoC 每跳按 §2.2 的两条规则
  被动转发/落核，**不查任何路由表**；发出时机由 NoC 节点按 `CalReg[opcode]` 放行。
- 路径与时隙都不改变落点：`dstSymOff = recvSymBase + selfOff + r * recvRowStride`。
- 只有已经提交到目标接收范围的有效 payload bytes 才能增加 MTE4 的接收量；包头、填充、重传和本地自拷贝
  不计入 `expVal`。
- **头部复制对软件透明**：一段连续字节拆成多个 flit 后，每个 flit 复制同一份头部由硬件完成，软件不感知。

**头部开销是本方案的主要代价，必须量化**：

| 项 | 大小 | 说明 |
|---|---|---|
| `routeBits` | 80 bit = 10 B | 每 flit 一份 |
| `opcode` + `redOp` | 6 bit | 每 flit 一份 |
| `epochTag` | 待定（建议 8~16 bit） | MTE4 判别键的一半 |
| **合计** | **≈ 12 B** | 若链路位宽 64 B ⇒ 头部占 **≈ 19%** |

**这 12 B 就是"NoC 不查表、每跳无状态"换来的代价。** 两条缓解路线见 §6.3 待决项 C-2：
（a）逐 flit 复制（本方案基线，NoC 完全无状态）；（b）逐 **packet** 头部一次 + 沿途保留每 packet 上下文
（省带宽，但把状态还给了 NoC）。

---

## 2. 路由与时隙信息的编码表达

### 2.1 两条物理载体的分工

Calendar 需要表达两类拓扑信息："走哪里"（路径）与"何时放行"（时隙）。本方案把它们放在**两条完全不同的
物理载体**上——路径随包携带、时隙常驻寄存器：

| | **路由信息** | **时隙信息** |
|---|---|---|
| 表达形式 | 40 × 2 bit = 80 bit `routeBits` 位图 | NoC 节点常驻的时隙寄存器 `CalReg`，核内只给一个 `opcode` 索引它 |
| 值在哪里生成 | 编译期，NoC C 算法 | 编译期，**同一次** NoC C 算法调用 |
| 值最终住在哪里 | **核侧 `.rodata` 静态表** → 发包时进 GPR → **随每个 flit 走** | **NoC 节点时隙寄存器 `CalReg`**（装载期一次性写入，kernel 期只读） |
| 核内怎么拿到 | `keyId`（立即数）+ `blockId`（运行期）索引 `.rodata`，**SU 两条普通 LD** | 不需要"拿"——只给 3 bit `opcode` 立即数 |
| 走哪条 cache | **D-Cache（64 B 行）** | `opcode` 随 `.text` 走 **I-Cache（2 KB 块）** |
| 每轮是否更换 | 是（不同逻辑身份取不同行） | **否**（寄存器常驻不换，只换 `opcode` 索引哪一项） |
| NoC 侧是否查表 | **否**——每跳只看 flit 自带的 2 bit | 是——按 `opcode` 对本节点时隙寄存器做一次索引 `CalReg[opcode]` |

**两类信息都在发包指令处进入数据面**，因此不存在"配置指令 + 当前活动配置"这个硬件状态，也不存在
"配置对 router / send scheduler / MTE4 ingress 同时可见"那一整类栅栏。代价是不变式 **O1** 失去硬件锚点，
必须由编译期与类型系统兜底（§2.5）。

### 2.2 路由位图 `routeBits`

WSE die 上 40 个 NoC 节点，每个节点分到一个 **`{P, L}` 位对**，共 80 bit = 10 B。
位序固定为 **`routeBits[2i+1] = P_i`、`routeBits[2i] = L_i`**（bit `b` 落在字节 `b/8` 的第 `b%8` 位，LSB 优先）。

| 位对 | 语义 | NoC 每跳的动作 |
|---|---|---|
| `00` | 与本 flit 无关 | 不落核、不转发 |
| `10` | 经过、不落核 | 仅继续转发 |
| `11` | 经过且落核（**可续转**） | 落核提交 payload **并**继续向下游转发 |
| `01` | **非法**（`L=1` 而 `P=0`） | 必须 fault，编译期即拒绝 |

**NoC 每跳只做两件事，不查任何路由表**：

```text
① if L_self == 1:  本地落核，把 payload 提交到接收范围（并计入目的核 MTE4 的有效字节）
② out = { 邻居 j | P_j == 1 } − ingress ;  对 out 中每个端口复制一份 flit
```

这条规则要成立，编译期必须保证 **`{P_i = 1}` 在物理拓扑上导出的子图是一棵含 source 的树**：
连通保证可达，无环保证被动转发会终止。

> ⚠️ **注意这里是"诱导"子图**：每跳都向**所有** `P=1` 的物理邻居复制（规则 ②），因此 `{P_i = 1}` 中任意两个
> 物理相邻节点之间的链路都会被用上，而不只是算法"想走"的那些边。例如 2 × 2 方块的 4 个节点全置 `P=1`，
> 诱导子图必然成环——flit 会沿环反复复制、目的核重复落核。所以算法选路时必须避开"计划外的相邻 `P` 节点"。

**`{P, L}` 的语义由集合语义参数化**——这是 2 bit 能覆盖 6 种集合语义的关键：

| 集合语义 | `L=1` 的含义 | `11` 且有下游 `P` 邻居的节点 = |
|---|---|---|
| `AllGather` / `Gather` / `Scatter` | 提交 payload 到本核接收范围 | 复制点（多播分叉） |
| `Reduce` / `AllReduce` / `ReduceScatter` | 本节点参与归约：与本地贡献按 `redOp` merge | **归约点**（merge 后继续转发） |

因此**不需要在 `routeBits` 里另加"归约点"位**：归约点就是"位对为 `11` 且存在下游 `P` 邻居"的节点。

#### 2.2.1 编码一致性锚点（golden vector）

编译器、打包器、RTL、仿真器与运行时 dump 必须对该向量得到**完全一致**的结果：

```text
source = N00
pass   = {N00, N01, N02, N03, N04, N10, N18, N19}
land   = {N04, N19}
routeBits[2i+1] = P_i , routeBits[2i] = L_i
  →  10 B = AA 03 20 00 E0 00 00 00 00 00
```

**FFN 示例的编码**（40 节点排成 5 × 8 mesh，`node = r*8 + c`；FFN 用前 4 行共 32 个 cell，
node 32..39 在这两个逻辑身份中位对恒为 `00`）：

```text
keyId 0 = {AllGather, ROW(my_row)}，source = cell(0,0) = N00
  ROW 组 = {N00..N07}；P = 全 8 个；L = 除自己外的 7 个（本核段由 UB→UB 本地 copy 补，见 Step4s）
  →  FE FF 00 00 00 00 00 00 00 00        landCount = 7   expectedRxBytes = 8×192×7  = 10752

keyId 1 = {AllGather, COL(my_col)}，source = cell(0,0) = N00
  COL 组 = {N00, N08, N16, N24}（竖直路径）；P = 全 4 个；L = 除自己外的 3 个
  →  02 00 03 00 03 00 03 00 00 00        landCount = 3   expectedRxBytes = 8×1536×3 = 36864
```

### 2.3 时隙索引 `opcode`

时隙信息的**值**由编译期的 NoC 算法算出，装载期一次性写入各 NoC 节点的 **Calendar 时隙寄存器 `CalReg`**，
**kernel 执行期只读、不随轮次更换**。核内唯一要做的事是在发包时给出一个 `opcode`。
**从核的角度看，这就是对 NoC 节点寄存器的一次索引**：

```text
SEND 携带 opcode  →  NoC 节点读 CalReg[opcode]  →  决定本包何时放行
CalReg 的内部格式与排时隙方式由 NoC 硬件定义；程序不感知 cycle，也不感知寄存器内部结构
```

`opcode` 宽度取 **3 bit**，值由集合语义唯一决定（0 号码位留给非法值，使"全零 flit 头"必然 fault）：

| `opcode` | 集合语义 | 索引的时隙寄存器项 | 首期 |
|---|---|---|---|
| `0` | 保留 / 非法 | 不允许进入 Calendar 数据面，必须 fault | — |
| `1` | `AllGather` | `CalReg[1]` | ✅ 实现 |
| `2` | `Reduce` | `CalReg[2]` | ✅ 实现 |
| `3` | `AllReduce` | `CalReg[3]` | 后续 |
| `4` | `ReduceScatter` | `CalReg[4]` | 后续 |
| `5` | `Scatter` | `CalReg[5]` | 后续 |
| `6` | `Gather` | `CalReg[6]` | 后续 |
| `7` | 保留 / 扩展 | 必须 fault 或丢弃并上报 | — |

> **为什么必须是 3 bit 而不是 2 bit**：集合语义共有 6 个值，2 bit 只能编 4 个（还要扣掉非法码位）。
> 首期若只上 `AllGather` 与 `Reduce`，硬件可以只实现 `CalReg[1]` 和 `CalReg[2]`、对 3..6 直接 fault——
> 但**编码位宽必须一次留够 3 bit**，否则后续加语义要改 flit 头格式。

**`opcode` 的两个职责**（这是它在本方案里比"一个集合语义编号"更重的地方）：

1. **索引 NoC 节点时隙寄存器** `CalReg[opcode]`——决定本包何时放行；
2. **兼作对齐信号量 id**（`opcode ∈ 1..6 ⊂ id ∈ 0..15`）——这是"对齐不新增指令"能成立的前提（§4.2）。

`opcode` **不承担**归约算子语义：`redOp` 是 flit 头的独立字段（§2.4）。

#### 2.3.1 "寄存器不随轮次换"带来的核心约束

程序里**所有使用同一 `opcode` 的集合共享同一项 `CalReg[opcode]`**。
FFN 示例的 phase B（ROW AllGather，8 成员）与 phase C（COL AllGather，4 成员）都用 `opcode = 1`，
因此它们按同一项 `CalReg[1]` 放行。这在两相**分开 launch、任一时刻只有一种流量在飞**时是安全的；
一旦两个同 `opcode` 的集合可能并发在飞，就必须**重新分相**（`opcode` 由集合语义唯一确定，
不能靠换用其他 `opcode` 规避）。

> ★ 由于 `opcode` 同时是对齐信号量 id，"同 `opcode` 不并发"这条约束从**数据面不并发**扩展到
> **对齐也不并发**：两个使用同一 `opcode` 的组不得同时汇合，否则它们会汇合到同一个信号量域上，
> 互相把对方的计数算进自己的到齐判定。

这条"同 `opcode` 不并发"与"同 `opcode` 无冲突"一起，**由 NoC 算法在生成寄存器内容时保证**（§2.7）；
PyPTO 负责把集合间的并发关系交给算法。

### 2.4 归约算子 `redOp` 与归约元素类型

`redOp`（`None/Sum/Max/Min/Prod`）作为 flit 头的**独立 3 bit 字段**，同样由编译期立即数给出；
`AllGather` / `Gather` / `Scatter` 时 dormant，`Reduce` / `AllReduce` / `ReduceScatter` 时给出 merge 算子。

> ★ **一处必须在 flit 头格式冻结前补上的缺口**：`redOp` 单独**不足以**唯一确定机器运算——`f32` 的 SUM 与
> `s32` 的 SUM 是两条不同的机器运算，`f16` 与 `b16` 的 MAX 亦然（前者按浮点比较、后者按位模式比较）。
> 归约类语义落地时，flit 头与发包接口需追加 **3 bit 元素类型**（取既有 `vtype_t` 除 `fmix` 外的 7 个取值，
> 正好落在 3 bit 内）；纯复制语义下 `redOp = None`、`dtype` 被忽略。**首期只上 `AllGather` 时不阻塞**，
> 但必须在头格式冻结前决定——见 §6.3 待决项 C-3。

### 2.5 逻辑身份与不可拆分的编译期常量对

**逻辑身份（`RouteKey`）** 是编译期给**一个逻辑 Calendar 路由/时隙组合**分配的**稳定符号身份**：

```text
RouteKey = { programId, kernelId, phaseId, collective, groupRole }
```

| 字段 | 含义 | 为什么必须在键里 |
|---|---|---|
| `programId` / `kernelId` / `phaseId` | 所属的程序 / 内核 / 执行相 | 同一个 group 在不同相可以有不同路径 ⇒ 是不同的 `routeBits` 集合 |
| `collective` | 服务的集合语义 | **直接决定 `opcode`**，从而决定索引哪一项时隙寄存器 `CalReg[opcode]` |
| `groupRole` | **组角色**（`ROW(my_row)`、`COL(my_col)`、`EP(my_expert_group)`…），**不是具体组** | SPMD 下 40 核跑同一份 kernel，代码里只能写"我所在的 ROW 组" |

**`groupRole` 在编译期就被代入本核坐标**——PyPTO 为**全部 40 个 node 各生成一行** `routeBits`，一次写死；
运行期只剩**一次 `blockId` 索引**，从 40 行里挑出属于自己的那行。这是本方案化解 SPMD 分裂的方式：

```text
"哪几张"和"每核内容"PyPTO 都知道，唯独"我是哪个核"它不知道
  ⇒ 把 40 个 cell 的行【全部】写死 ⇒ 表在 40 核上同构 ⇒ 运行期只需一次 blockId 索引
  代价：表按 40 行发射（640 B / key）——但它在 DDR 里【只有一份】、由 Batcher.mem 供给全片（§3.4）
  收益：真正的 .rodata，随 binary 一次 H2D；且【每核 D-Cache 占用不放大】（§3.7）
```

逻辑身份的四条作用：

1. **编译期的连接键**：IR 上每个集合调用点先解析出逻辑身份，NoC C 算法据此生成 40 行 `routeBits`
   与对应的 `opcode`，再落成 `{keyId, opcode, expVal}` 三个编译期立即数。
2. **编译器 ↔ 运行时的契约**：交付的是 `.rodata` 静态表 + 全域 `CalReg` 镜像 + `conflictProof`；
   运行时只负责"把 binary 下发"和"把时隙寄存器内容写入 NoC 节点"，**不做 per-core 展开**。
3. **接收计量的绑定键**：MTE4 累计匹配 `{opcode, collectionEpoch, dstRange}` 的有效载荷字节。
   ⚠️ `opcode` 的判别力弱——程序里所有 `AllGather` 共用 `opcode = 1`，因此 epoch 必须承担全部区分职责（§4.5）。
4. **一致性校验的锚点**：同一集合的全部成员必须解析到同一逻辑身份；安装期以它校验
   "40 行覆盖完整""落核集合与 `expectedRxBytes` 守恒""`opcode` 与时隙寄存器版本匹配"。

**`keyId` 与 `opcode` 必须打包为不可拆分的编译期常量对**（这是不变式 **O1** 的落地手段）：

```cpp
// 为每个逻辑身份发射一个不可拆分的常量对；调用点只能整体传入，不得分别传 keyId / opcode，也不得取址。
struct CalendarKeyRef {
    uint16_t           keyId;        // .rodata 静态表的 key 维下标
    CalendarCollective opcode;       // 索引 CalReg[opcode]，并兼作对齐信号量 id
    CalendarReduceOp   redOp;        // AllGather 时为 None
    uint32_t           routeVersion; // 与时隙寄存器的 calendarVersion / 芯片 topologyVersion 联合校验
};
inline constexpr CalendarKeyRef kRowAllGather{ 0, CalendarCollective::AllGather,
                                               CalendarReduceOp::None, ROUTE_VERSION };
inline constexpr CalendarKeyRef kColAllGather{ 1, CalendarCollective::AllGather,
                                               CalendarReduceOp::None, ROUTE_VERSION };
```

典型解析链如下：

```text
调用点 -> 逻辑身份 -> CalendarKeyRef{keyId, opcode, redOp}   +  expVal
       -> TCalendar<kRowAllGather>(chan, expVal, selfOff, ...)
       -> 取数:   e = kCalendarRoute[keyId][blockId]                  // 普通 LD：.rodata → D-Cache → GR 对
       -> Step2:  SET_CROSS_CORE(calGroup, opcode) + WAIT_FLAG_DEV(opcode)  // 既有 ISA；opcode 立即数 → I-Cache
       -> Step4:  MTE3_NOC_SEND(..., rbLo, rbHi, opcode, redOp)
       -> NoC:    每跳按 {P,L} 被动转发/落核；节点按 CalReg[opcode] 放行
```

### 2.6 静态表项 `CalendarRouteEntry` 与寄存器对布局

**`.rodata` 静态表项固定为 16 B**，正好一次两条 64 bit LD 装进一个寄存器对：

```cpp
struct alignas(16) CalendarRouteEntry {   // 16 B；.rodata 静态表项，40 核共享同一份
    uint8_t  routeBits[10];   // 本核作为 source 时的 40 × {P,L}；bit(2i+1)=P_i, bit(2i)=L_i
    uint8_t  landCount;       // popcount(L)；本核这一发的落核数，供交叉校验
    uint8_t  flags;           // bit0 isMember  bit1 isRoot  bit2 selfLand(= FabricSelfDelivery)
    uint32_t expectedRxBytes; // 本核作为 destination 时本轮应收的远端有效字节数（= expVal）
};
static_assert(sizeof(CalendarRouteEntry) == 16, "entry must fit one GPR pair");

inline constexpr uint32_t kNocNodeCount        = 40;  // WSE die 上的 NoC 节点数；routeBits = 2 × 40 bit
inline constexpr uint32_t kCalendarKeyMax      = 16;  // .rodata 表 key 维上限（§3.7 预算）
inline constexpr uint32_t kCalendarOpcodeCount = 8;   // opcode 码位数（3 bit；0 与 7 保留）
```

寄存器对布局（这是把表项定死为 16 B 的理由）：

```text
rbLo = 表项字节 0..7   →  node 00..31 的 32 个位对
rbHi = 表项字节 8..15  →  [15:0]  = node 32..39 的 8 个位对
                          [23:16] = landCount   [31:24] = flags   [63:32] = expectedRxBytes

发包指令只取 rbHi[15:0] 作路由位，高 48 bit 被硬件忽略（且不得做任何检查）
  ⇒ landCount / flags / expectedRxBytes 免费搭车，零额外 LD、零额外 cache 行
```

**同一个 source 的一行既表达"我作为发送方走哪里"，也表达"我作为接收方该收多少"**——因为在 SPMD 下
本核既是 source 也是 destination，两者都是 `blockId` 的函数，所以能共用同一行、同一次 LD。

**表的形状**：

```text
kCalendarRoute[keyId][node]         // 第一维 = 逻辑身份（keyCount 种），第二维 = 源核编号（40）
    每项 16 B  = routeBits[10] + landCount(1) + flags(1) + expectedRxBytes(4)
    每个 keyId = 40 × 16 B = 640 B      ← 一种集合语义的"全部 40 张位图"
    整张表     = keyCount × 640 B       ← FFN 示例：2 × 640 = 1280 B

本核取自己那张：  kCalendarRoute[keyId][blockId]
                   └─编译期立即数─┘ └─运行期唯一变量─┘
```

> **为什么 `routeBits` 只能是数据、不能编进指令立即数**：同一次集合通信里，40 个核用到的 `routeBits`
> **内容各不相同**——每个核作为 source 的路径树都不一样。SPMD 下 40 核共享同一份 `.text`，立即数无法区分核。
> 这是"把 `routeBits` 折成立即数"这条路线唯一的、也是致命的反对理由。

### 2.7 编译期如何联合生成两类信息

两类信息在**同一个编译阶段、同一次 NoC 算法调用**中联合生成。NoC 侧提供可静态链接或通过稳定 FFI 调用的
C 实现；PyPTO 负责构造输入（GROUP / 集合语义 / 并发关系取自 IR，拓扑信息与 PG 信息取自目标芯片）、
调用算法、校验输出，并把 `routeBits` 发射进 `.rodata`、把时隙寄存器内容交给装载期。

> 算法**不是**运行时在 launch 时临时运行的：拓扑、GROUP 与集合语义均已在编译阶段确定，
> 提前生成才能完成 cycle-accurate 冲突检查。

```text
编译期输入（只有这些会影响 routeBits 与时隙寄存器内容）
  ├─ topology（拓扑信息；来自目标芯片描述，不属于 IR）：
  │     40 个 NoC 节点、链路、端口、带宽、时延、VC、packet/flit 约束
  ├─ PG 信息（Partial Good；来自目标芯片实例）：坏核与坏链，即哪些节点 / 链路不可用
  ├─ GROUP（来自 IR）：成员集合、groupRole、rank → 物理 node 映射、root（若语义需要）
  ├─ collective（来自 IR）：AllGather/Reduce/...、reduce op
  ├─ concurrency（来自 IR 的 phase / launch 结构）：哪些逻辑身份可能同时在飞
  │     （只用于判定"同 opcode 不并发"，不改变 routeBits / 寄存器内容）
  └─ compile constraints：opcode 空间、NoC 时隙寄存器格式约束、端口 VC 容量

  ✗ payload 几何（rowCount / rowBytes / 行跨距）不是算法输入：
    · routeBits 只描述"经过谁、落在谁"，与包长无关；
    · 时隙寄存器内容按周期复用，同一 opcode 的不同集合（FFN 的 192 B 行与 1536 B 行）共用同一项，
      包长只决定占用多少个周期，不影响是否冲突；
    · 它只在 PyPTO 侧用于推导 expVal / capacity / selfOff 与"arm 可达"校验
                         │
                         ▼
             noc_calendar_compile(...)          // NoC 提供的 C 代码
                         │
          ┌──────────────┴───────────────┐
          ▼                              ▼
  routeBitsSet[keyId][node]       calRegImage[opcode]
  40 × 10 B 逐 source 位图         全域 CalReg 镜像（格式由 NoC 定义）
  → 发射进 .rodata                 → 交装载期写入各 NoC 节点
          └──────────────┬───────────────┘
                         ▼
             keyMeta[keyId] = { RouteKey, opcode, redOp,
                                rxSourceCount[40], armLeadCycles[opcode], version }
             PyPTO 再按 tile 几何算：expectedRxBytes[d] = rxSourceCount[d] × rowCount × rowBytes
```

建议的 C ABI 轮廓如下；具体字段宽度由 NoC 接口评审冻结，**软件不得自行重写路由算法**：

```c
typedef struct {
    uint64_t route_key_hash;
    uint32_t key_id;
    uint8_t  opcode;                     // 索引 NoC 节点时隙寄存器 CalReg[opcode]（3 bit 有效）
    uint8_t  red_op;                     // 归约算子（3 bit 有效）；AllGather 时为 0
    const uint8_t  *route_bits;          // node_count × 10 B，逐 source
    const uint8_t  *rx_source_count;     // node_count；逐 destination：在该节点落核的 source 数（与包长无关）
    const uint8_t  *land_count;          // node_count；popcount(L)，供交叉校验
    uint32_t node_count;                 // = 40
} NocCalendarRouteKeyOut;

typedef struct {
    uint8_t  opcode;
    uint32_t arm_lead_cycles;            // 对齐返回后硬件预留的 arm 窗口（NoC/BSP 内部使用，程序不读）
    const void *reg_image;               // CalReg[opcode] 的寄存器镜像，格式由 NoC 定义
    uint64_t size;                       // reg_image 字节数
} NocCalRegImage;

int noc_calendar_compile(const NocCalendarCompileInput *input,
                         NocCalendarRouteKeyOut **keys,  uint32_t *key_count,
                         NocCalRegImage        **regs,  uint32_t *reg_count,
                         NocCalendarDiagnostic *diag);
```

#### 2.7.1 由 NoC 算法承担的两项时序正确性保证

这两项只有掌握 NoC 时序模型（端口、VC、quota、流水时延）的算法才能判定，PyPTO 不做 cycle 级复算——
否则等于在软件侧重写一遍排程算法：

| 由 NoC 算法保证 | 判定条件 | PyPTO 的职责 |
|---|---|---|
| **同 `opcode` 无冲突** | 所有使用同一 `opcode` 的集合共享同一项 `CalReg[opcode]`；它们各自按 `routeBits` 展开的流量，在该寄存器放行下不在任何端口 / VC / 时隙上碰撞或超出配额 | 把共用同一 `opcode` 的全部逻辑身份放进同一次算法调用；算法失败时报告冲突资源与时隙位置，编译失败 |
| **同 `opcode` 不并发** | 同一 `opcode` 的两个不同逻辑身份不得同时在飞（一个 `opcode` 域只有一个由硬件内部维护的时间原点，两个集合无法各自对齐，时隙会错位）；由于 `opcode` 兼作对齐信号量 id，这条同时约束**对齐面** | 从 IR 的 phase / launch 结构导出集合间并发关系作为算法输入；算法判定不满足时由 PyPTO 重新分相 |

#### 2.7.2 PyPTO 侧的结构性硬校验

它们与算法实现无关、只需按节点数线性检查，用于交叉验证算法输出与 ABI：

| 校验 | 判定条件 | 失败后果 |
|---|---|---|
| **位对合法** | 全部 40 个位对中不存在 `01`（`L=1` 而 `P=0`） | 编译失败并指出 node |
| **诱导子图是树** | 取 `{P_i = 1}` 的节点集合，在物理拓扑上取其**诱导子图**（集合内任意两个物理相邻节点之间的链路都算在内），该子图必须连通、含 source、且无环 | 编译失败并输出环；否则被动转发不终止、目的核重复落核 |
| **`P_self = 1`** | source 自身必须在路径上 | 编译失败；硬件侧亦须 fault |
| **落核集合完备** | `{L_i = 1}` 恰等于该集合语义要求的目的核集合；`landCount` 与之一致 | 编译失败 |
| **发送—接收守恒** | 对每个目的核 `d`：`Σ_{s: L_d(s)=1} rowCount × rowBytes = expectedRxBytes[d]` | 编译失败；该值正是设备侧 MTE4 的 `expVal` |
| **`selfLand` 一致** | source 自身的 `L` 位取值必须与运行期 `CalendarPlatformTraits::FabricSelfDelivery` 一致 | 不一致则本核段**漏写或双写**；编译失败 |
| **arm 可达** | `armLeadCycles[opcode]` 覆盖 MTE4 感知命令与全部 MTE3 描述符入队的最坏时延/队列容量 | 编译失败或改用整 tile `nBurst` 形态 |
| **静态表可发射** | `keyCount × 40 × 16 B` 满足 64 B 对齐，且**全片一份的 DDR 占用**与**每核 D-Cache 驻留行数**均在预算内（§3.7） | 编译告警 / 失败 |
| **拓扑同构** | 目标芯片实例的物理拓扑与 PG 信息与编译期假设一致（无坏核/坏链导致的每实例重映射） | 否则 `.rodata` 静态表不成立，必须退回装载期只读 buffer（§3.10 退化路径） |

#### 2.7.3 编译期五步

**没有 window 装箱、没有活跃区间分析、没有 L1 预留区扣除**——本方案的编译期只有五步：

| 步 | 做什么 | 依据（全部编译期可判定） | 产物 |
|---|---|---|---|
| ① 枚举并调算法 | 遍历 IR 调用点解析逻辑身份，调 NoC C 算法，同时得到 40 行 `routeBits` 与全域时隙寄存器内容 | 拓扑、PG、GROUP 与集合语义均为编译期已知 | `NocCalendarRouteKeyOut[]` + `NocCalRegImage[]`，FFN 示例为 2 个 key、1 项 `CalReg[1]` |
| ② 硬校验 | §2.7.2 全部项；"同 `opcode` 无冲突/不并发"由算法保证并随结果给出 `conflictProof` | §2.7.1 / §2.7.2 | 编译诊断 + `conflictProof` |
| ③ 发射静态数据 | `kCalendarRoute[keyId][node]` 发射进 `.rodata`，`alignas(64)`；**不做 per-core 裁剪** | 每项 16 B，`keyCount × 40 × 16 B` | `.rodata` 段字节 + 段内符号 |
| ④ 固化调用点立即数 | 每个调用点拿到 `CalendarKeyRef{keyId, opcode, redOp}` 与经守恒校验的 `expVal`、`capacity`、`selfOff` | 算法给出的落核源数 × tile 几何——payload 几何只在这一步进入 | `.text` 里的立即数；无任何 DMA / 激活伪操作 |
| ⑤ 交付装载期 | 全域 `CalReg` 镜像 + `armLeadCycles[opcode]` + 三版本号（topology / calendar / route） | 时隙寄存器排程结果 | 装载期安装清单与版本校验记录 |

FFN 示例的完整推导：

```text
IR:  phase B: TCalendar(...)  →  {AllGather, ROW(my_row)}  →  keyId 0, opcode 1
     phase C: TCalendar(...)  →  {AllGather, COL(my_col)}  →  keyId 1, opcode 1

.rodata 发射：kCalendarRoute[2][40]，2 × 40 × 16 B = 1280 B，64 B 对齐 ⇒ 20 条 cache 行
     每核每 key 只触碰 1 条行（cell c 的行落在第 c/4 条）⇒ 每核实际驻留 2 条行 = 128 B

时隙寄存器：两相都是 AllGather ⇒ 共用 CalReg[1]
     ★ 两相分开 launch、任一时刻只有一种流量在飞 ⇒ PyPTO 向算法声明两相不并发，
       同时满足"同 opcode 数据面不并发"与"对齐不并发"
     ★ 若改成同一 launch 内并发，算法将判定不满足，PyPTO 必须重新分相

调用点：constexpr CalendarKeyRef kRowAllGather{0, AllGather, None, V};  expVal = 10752
        constexpr CalendarKeyRef kColAllGather{1, AllGather, None, V};  expVal = 36864
        ★ epochCtr 随 launch 重置 ⇒ 两相各自的第一轮 epoch 都是 1；
          跨相隔离靠"launch 边界前全片 Calendar 流量已排空"，不靠 epoch 数值区分
```

### 2.8 落点的编码：对称地址、`selfOff` 与载荷几何

Calendar 方向的拓扑对软件不透明，**软件算不出对端的物理地址**。因此目的操作数一律使用
**SPMD 对称地址 + 本核段偏移**，由硬件解析成各 peer 的物理落点：

```text
dst(s, r) = recvSymBase + r * recvRowStride + selfOff(s)
selfOff(s) = rankInGroup(s) * rowBytes                 // packed AllGather
```

| 不变式 | 内容 | 谁保证 |
|---|---|---|
| **A1 对称同址** | 组内每个成员的 `recvTile` 必须落在**相同的本地 UB 偏移**。SPMD 下一个常量 `TASSIGN` 即满足 | 调用方 |
| **A2 转发不改落点** | `routeBits` 只描述"经过谁、落在谁"，落点始终是 `recvSymBase + selfOff + r × recvRowStride` | 硬件 / NoC |
| **A3 铺满性** | 组内全部成员的 `selfOff` 段必须**不重叠地铺满**竞技场的每一行 | 编译器 + 装载期校验 |

`selfOff` 是 SPMD kernel 中**唯一逐核不同**的发包操作数。典型写法：

```cpp
const uint32_t rowSelfOff = static_cast<uint32_t>(col) * kIShard   * sizeof(half);
const uint32_t colSelfOff = static_cast<uint32_t>(row) * kRowBlock * sizeof(half);
```

**载荷几何全部从 tile 类型派生**，不进接口参数：

| 量 | 来源 | 用途 | 下沉到哪个操作数 |
|---|---|---|---|
| `rowCount` | `sendTile` 的有效行数 | 发包行数 | `nBurst`（整 tile 形态）或循环次数（逐行形态） |
| `rowBytes` | `sendTile` 的有效行字节 | 每行有效字节数 | `lenBurst`（单位为**字节**） |
| `sendRowStride` | `sendTile` 的行跨距 | 源 UB 地址步进 | `srcGap = sendRowStride - rowBytes` |
| `recvRowCount` | `recvTile` 的有效行数 | 与 `rowCount` 一致性校验 | — |
| `recvRowStride` | `recvTile` 的行跨距 | 竞技场行步进 | `dstGap = recvRowStride - rowBytes` |
| `capacity` | `recvRowCount * recvRowStride` | 接收范围上界；越界写入必须 fault | 接收指令的 `capacity` |

**`expVal` 的推导与三方一致性**：

```text
AllGather:  expVal = rowCount * rowBytes * landCount        // landCount = 远端落核源数
用户传入的 expVal  ==  载荷几何推导值  ==  表项 expectedRxBytes
```

三者必须相等，debug 构建下由包装层断言。`landCount` 与 `expectedRxBytes` 都随 `routeBits` **同一次 LD
免费带回**，因此这项校验零额外访存。

> 选择"字节"而非"flit 数"作为 `expVal` 单位，可以避免链路位宽、尾 flit 填充或未来 packetization 改变
> 软件语义。硬件只累计已提交到目标接收范围、且匹配 `{opcode, epoch}` 的有效 payload bytes。

**两条继承自复用指令的几何约束**（详见 §4.4）：

- **R-1（正确性）**：UB 源行起点必须 **32 B 对齐** ⇒ `sendRowStride` 应为 32 B 整数倍；
- **R-2（效率）**：`lenBurst` 以字节计，align 模式的用途正是支持非整块尾部 ⇒ `rowBytes` **不必**是 32 B
  整数倍；"链路位宽整数倍"仍是效率建议，且**链路位宽不进软件接口**。

---

## 3. 路由信息和时隙信息的下发链路

> 本章回答一个问题：`routeBits` 与 `opcode` 这两类值，从"被制造出来"到"被指令读到"之间经过哪些环节、
> 各自落在哪层存储上。展开顺序为：总纲与户籍（§3.1、§3.2）→ 立即数通道（§3.3）→ `.rodata` 通道（§3.4、§3.5）
> → 六阶段时间轴（§3.6）→ 一次 SEND 的三链模型（§3.7）→ D-Cache 预算（§3.8）→ `CalReg` 的下发（§3.9）
> → 禁令与自检（§3.10）→ 退化路径（§3.11）。

### 3.1 总纲：cache 从不"生产"值，它只是 DDR 的一扇镜像窗

这是回答一切"之前放在哪"的唯一入口：

> **I-Cache / D-Cache 都是物理地址空间（DDR）的缓存副本，不是独立的存储对象。**
> 任何一个出现在 D-Cache 行里的字节，它的"上一站"永远是 **DDR 的某个地址**（中间经 Batcher.mem）。

所以"D-Cache 的值加载前放在哪"这个问题，字面答案是平凡的（DDR），有信息量的答案是再往上一层：
**那个 DDR 地址上的内容，是谁写进去的？什么时候写的？走的哪条路？**

★ **WSE 上的回填通路固定为**：

```text
D-Cache miss  →  MSHR  →  BIU  →  Batcher.mem  →（未命中再回）DDR  →  64 B 整行回填
I-Cache miss  →  取指  →  Batcher.mem  →（未命中再回）DDR  →  2 KB 块回填
★ 全程【不经 local DRAM、不经 L1】——L1 只服务 GEMM 等显式搬运，不在 I$/D$ 的回填通路上
```

**所有会进 cache 的值只有三个来源（三种户籍）**：

| 户籍 | 内容在何时被"制造" | 由谁搬进 DDR | 典型代表 |
|---|---|---|---|
| **A. 编译期静态镜像** | 编译/链接时就固化在二进制文件里 | Host runtime **一次性** H2D DMA | `.text` 指令、`.rodata` 常量表 |
| **B. 运行期 Host 下发** | 每次 launch 前在 host 侧算出来 | runtime **每次 launch** 拷贝 | kernel 入参、tiling 结构体、输入张量 |
| **C. 运行期 Device 自产** | kernel 跑起来之后在片上产生 | 没人搬——**就地写出来的** | 栈、MTE 写回的结果、别核写的标志 |

**关键推论**：I-Cache 的值 100% 是 A 类（它和指令走同一次 DMA、同一份二进制），这是它免一致性维护的根本原因；
D-Cache 的值 A/B/C 三类都有，这也正是 D-Cache 一致性问题的来源。

### 3.2 户籍判定：Calendar 的四个值各属哪一类

| 值 | 户籍 | 内容何时被"制造" | 由谁搬进 DDR | 进哪个 cache |
|---|---|---|---|---|
| `opcode` / `redOp` / `expVal` / `capacity` / `selfOff` | **A 类** | 编译/链接时固化 | **随 `.text` 一次 H2D**（其实是"物理上就是 `.text` 的一部分"） | **I-Cache（2 KB 块）** |
| `routeBits` 静态表 `kCalendarRoute[key][node]` | **A 类** | 编译/链接时固化 | 随 `.rodata` 与 `.text` **同一次 H2D** | **D-Cache（64 B 行）** |
| `blockId` | **B 类** | 每次 launch 前由 host / TS 确定 | runtime 每次 launch 下发（或由 SPR 直接给出） | D-Cache（或 0 次访存，见 §3.7） |
| `CalReg` 时隙寄存器内容 | **A 类（但不进核）** | 编译期由 NoC 算法算出 | 装载期一次性写入 **NoC 节点时隙寄存器** | **两个 cache 都不进** |

★ **关键观察**：本方案把 Calendar 的控制信息**全部压进 A 类**。取消 per-core 展开之后，表是真正的 A 类。
唯一的 B 类残留是 `blockId`——而它只是一个整数下标。

### 3.3 立即数通道：`opcode` / `expVal` → `.text` → I-Cache

按"这个值该住在哪个段"的四问判定：

```text
Q1: 值在编译期已知吗？
 ├─ 否 ──────────────────────────────────► 它根本不是"静态数据"，是运行期值（栈 / GM buffer）
 └─ 是 ↓
Q2: 它需要有一个"地址"吗？（被数组下标索引、被取址 &x、太大编不进立即数）
 ├─ 否 ──► ★ 不占任何段，折叠成指令立即数 ──► 随 .text 走 I-Cache
 └─ 是 ↓
Q3: 它可写吗？
 ├─ 否（const/constexpr）──► .rodata  ──► 随二进制 H2D，LD 经 D-Cache，永不 dirty
 └─ 是 ↓
Q4: 初值全是零吗？
 ├─ 是（或未初始化）──► .bss   ──► 二进制里只有 size，运行期清零，LD/ST 经 D-Cache，会 dirty
 └─ 否 ─────────────► .data  ──► 随二进制 H2D，LD/ST 经 D-Cache，会 dirty
```

**Q2 是"进 I-Cache 还是进 D-Cache"的分水岭**；Q3/Q4 决定"进 D-Cache 之后要不要维护一致性"。
Calendar 的立即数类值在 Q2 处答"否"：

```text
Q1 值在编译期已知吗？  → 是（opcode 由集合语义决定；expVal 由收发守恒推出）
Q2 它需要有一个"地址"吗？→ 否 ──► ★ 不占任何段，折叠成指令立即数 ──► 随 .text 走 I-Cache
```

具体形态：

```cpp
inline constexpr CalendarKeyRef kRowAllGather{ 0, CalendarCollective::AllGather,
                                               CalendarReduceOp::None, ROUTE_VERSION };
inline constexpr uint64_t kRowExpVal      = uint64_t(kT) * kIShard * sizeof(half) * (kGridCols - 1); // = 10752
inline constexpr uint32_t kRowSelfOffUnit = kIShard * sizeof(half);                                  // = 192
```

★ **这些量在最终二进制里，`.data` / `.bss` / `.rodata` 三个段各占 0 字节。** 它们在编译期被完全求值，
结果直接烧进指令的立即数字段：

```asm
; SET_CROSS_CORE(calGroup, opcode)   opcode = AllGather = 1（既有 ISA，充当信号量 id）
SET_CROSS_CORE  GR_grp, #1        ← 3 bit 立即数就在指令里，零访存
WAIT_FLAG_DEV   #1

; MTE4_NOC_RECV_WAIT(..., expVal, ...)   expVal 编译期算出 = 10752
MOVI  GR7, #10752                 ← 值就在指令里，零访存
MTE4_NOC_RECV_WAIT  GR_dst, GR_cap, GR7, #1, GR_epoch
```

**完整下发链路**：编译器求值 → 编入指令 → `.text` → H2D → DDR 代码段 → **Batcher.mem** →
**I-Cache（2 KB 粒度）** → 解码时直接得到操作数。
**零 D-Cache 流量、零 DDR 数据段占用、零一致性负担；40 核共享同一份 `.text`，所以这些值对 40 核必然相同。**

> ⚠️ **唯一的、也是很脆的一道墙**：`constexpr` 量一旦被**取地址**或**绑引用**，编译器就被迫物化它，
> 它会掉进 `.rodata`，从此走 D-Cache：
> ```cpp
> TCalendar<kRowAllGather>(chan, ...);          // 非类型模板参 → 立即数，不占段 ✅
> TCalendar(chan, &kRowAllGather, ...);         // 取址 → 物化到 .rodata ❌
> const CalendarKeyRef& k = kRowAllGather;      // 绑引用 → 同上 ❌
> ```
> 因此 §5.9 的接口把 `CalendarKeyRef` 定义为**非类型模板参**（或按值传递的小结构体），从源头上杜绝取址。
> 自检方式见 §3.10。

> **一个共享层的直接收益**：40 个核跑同一份 SPMD kernel 时，代码段只需要从 DDR 真正读上来**一次**——
> 第一个核的 I-Cache miss 把 2 KB 块拉进 Batcher.mem，其余核的同地址 miss 全部在 Batcher.mem 命中。
> 所以冷启动的 I-Cache miss 代价**不是** `N × DDR 延迟`，而是 `1 × DDR + (N−1) × Batcher.mem`。

### 3.4 `.rodata` 通道：`routeBits` 静态表 → D-Cache

#### 3.4.1 四问判定：Q2 的答案相反

```text
Q1 值在编译期已知吗？  → 是（40 行位图由 NoC 算法在编译期一次算出）
Q2 它需要有一个"地址"吗？
   → 是：被【运行期变量 blockId】索引 ──► 必须物化
Q3 它可写吗？
   → 否（constexpr）──► ★ .rodata ──► 随二进制 H2D，LD 经 D-Cache，永不 dirty
```

决定性的那一条规则是**是否被运行期变量索引**：

```cpp
// 情形 A：编译期常量下标 → 折成立即数，表可能被整个消除，走 I-Cache
constexpr auto e0 = kCalendarRoute[0][7];        // 不适用：SPMD 下不能写死"我是 7 号核"

// 情形 B：运行期变量下标 → 编译器必须把整个 40 行表物化到 .rodata，生成 LD
const CalendarRouteEntry e = kCalendarRoute[kRowAllGather.keyId][blockId];   // ★本方案走这条
   → 生成 LD [kCalendarRoute_base + keyId*40*16 + blockId*16]
   → keyId 是立即数 ⇒ 折进地址偏移；blockId 是【唯一的变量部分】
```

#### 3.4.2 完整下发链路

```text
编译期：PyPTO 调 NoC 算法 → 40 行 × 16 B 的初值字节写进 .o 的 .rodata 段
  └─ 链接器：分配段地址，alignas(64) 落位到 64 B 边界
     └─ 打包进 kernel binary
        └─【和 .text 同一次 H2D DMA】→ DDR 数据段（与代码段相邻）
           └─ 运行期首次 LD kCalendarRoute[keyId][blockId]（为 SEND 取操作数）
              → D-Cache miss
              → MSHR → BIU → Batcher.mem（Batcher.mem 未命中再回 DDR）
              ★ 全程【不经 local DRAM、不经 L1】——L1 只服务 GEMM 等显式搬运，不在 I$/D$ 回填通路上
              → 64 B 整行回填（一次拉进 4 个相邻 cell 的表项）
              → 第 2 条 LD（表项后 8 B）落在同一行，必命中
              → 之后本 key 在本核不再 miss
```

★ **这就是"SU 进行查找、若 miss 则按流程加载"的完整落地**：取数的两条普通 LD 就是"SU 查找"，
miss 路径就是 §3.6 阶段 ⑤ 的**标准 D-Cache 回填通路**（由硬件完成、对用户透明），
本方案**没有为它开任何特殊通道**——没有专门的 DMA 引擎、没有 wait、没有 visibility fence、没有激活状态机。

**`.rodata` 的三个性质在本方案里的兑现**（两好一坏）：

| 性质 | 本方案 |
|---|---|
| ✅ **永不 dirty** | 表是 `constexpr`，程序从不写它 ⇒ 算子尾部只需 invalidate，**不需要 `DataCacheCleanAndInvalid` 的 clean 半边** |
| ✅ **40 核共享同一份镜像正好是收益** | 表在 **DDR 里只有一份**，由 **Batcher.mem** 统一供给全片各核的 D-Cache——"共享"既是"内容逐字节相同"，也是"物理上就一份"。收益是：**无需 per-core 展开、无需每 launch 下发、无需基址入参**，各核只按需回填自己用到的那一条行 |
| ❌ **免维护不免容量** | 但本方案的访问模式**不是随机 LUT**——`blockId` 在整个 kernel 内是常量，每核每 key 只触碰 1 条行。这是 §3.8 的核心论证：**表按 40 行发射，每核 D-Cache 占用不变** |

### 3.5 为什么它能是**真正的** `.rodata`

先把术语说死：真正的链接期 `.rodata` 是 **A 类**、地址在链接期固定、40 核必然解析到同一个符号；
而按 `blockId` 展开的 per-core 镜像是 **B 类**、地址由 runtime 分配、每核内容各不相同。
**"如果它真的是 `.rodata`，它就一定不可能 per-core。"**

本方案**正面接受这条约束**，用"表里存满 40 行"换回 A 类户籍：

| | 装载期 per-core route image（**本方案不采用**） | **本方案的 `.rodata` 静态表** |
|---|---|---|
| 户籍 | B 类：装载期按 `blockId` 展开，每次 launch 下发 | **A 类：编译期固化，随二进制一次 H2D** |
| 地址怎么定 | runtime 分配，基址随 tiling 入参传进来 | **链接期固定，符号直接可寻址** |
| SPMD 下 | 每核一份，内容各不相同 | **40 核同一个链接期符号地址、镜像内容逐字节相同** |
| 表里有什么 | 只有"我这一行" | **全部 40 行；"我是谁"退化成一个运行期下标** |
| **DDR 占用（全片一份）** | 40 核各一行 ⇒ `40 × 16 B = 640 B / key` | **`40 × 16 B = 640 B / key`（同一张表，全片共用）** |
| 供给各核 D-Cache 的是谁 | Batcher.mem | **Batcher.mem（同一条通路，无差别）** |
| 每次 launch 要下发吗 | **要** | **不要** |
| 需要运行期描述符 / 基址入参吗 | 要 | **不要** |

★ **这里必须把账算准：那个"40 倍"并不存在。**
`.rodata` 表随 kernel binary 一次 H2D 落在 **DDR**，各核取指/取数 miss 时由 **Batcher.mem** 统一供给——
**表在物理上只有一份，不逐 cell 复制**。而 per-core 路线虽然"每核只存自己那一行"，40 个核的 40 行同样要
全部存在于同一片 DDR 里，合计也是 `40 × 16 B = 640 B/key`。**两条路线的 DDR 总占用完全相同**，
本方案多出来的只是"把这 40 行按链接期符号排在一起"这一件事。

> ✅ **这正是"共享层"的直觉起作用的场景**：I$/D$ 的回填通路是 `DDR → Batcher.mem → I$/D$`，
> 全片各核共享同一份 DDR 镜像与同一级 Batcher.mem。因此"把 40 行全写死"不会被乘以核数——
> 被乘以核数的只有**各核 D-Cache 里实际驻留的行**，而那恰恰是每核 `keyCount` 条、与 40 无关（§3.8.1）。

**于是这笔交易几乎是净赚——容量侧持平，时间侧全是收益**：

| 资源 | 稀缺度 | 本方案 vs per-core 路线 | 绝对量 |
|---|---|---|---|
| **DDR（全片一份）** | GB 级，**最不稀缺** | **× 1（持平）** | 640 B/key；`keyCount ≤ 16` 上限 10 KB ⇒ 相对 kernel binary 是噪声 |
| **Batcher.mem** | 供给全片 I$/D$ 的共享级 | **× 1（持平）** | 同一条回填通路，两条路线读的行数相同 |
| **D-Cache（16 KB 量级）** | **最稀缺** | **× 1**（改用 §3.8.3 的 node-major 布局后 **× 0.25**） | 每核 `keyCount` 条 64 B 行；FFN 示例 128 B |
| **每次 launch 的 H2D 带宽** | 每轮都付 | **× 0** | 从"每 launch 下发一份 per-core 镜像"降为"随 binary 一次" |
| **取数的指令数 / 访存次数** | 每轮都付 | **少一次** | 本方案 2 条 LD；per-core 路线要先读基址入参 ⇒ 多一条 D-Cache 行、多一次可能 miss |

**因此本方案的正确理由是**："40 行全写死在容量上一分钱没多花（DDR 里本来就要放 40 行），
换回来的是 A 类户籍——不用 per-core 展开、不用每 launch 下发、不用基址入参。"

### 3.6 六阶段时间轴：谁在什么时候搬什么

| 阶段 | 谁执行 | 搬 / 做什么 | 从 → 到 | 频率 | 涉及 cache |
|---|---|---|---|---|---|
| **① 编译期** | PyPTO + NoC C 算法 + 编译器 + 链接器 | 调算法产 40 行 `routeBits` 与全域时隙寄存器内容；`opcode/expVal/capacity/selfOff` 折成立即数编进 `.text`；**`routeBits` 表发射进 `.rodata` 并 64 B 对齐** | IR / 源码 → host 二进制文件 | 离线，一次 | 决定后续所有 cache 行为 |
| **② 注册/加载期** | runtime + driver | kernel binary（`.text` + **`.rodata` 同批**）；**另外：`CalReg` 全域镜像写入各 NoC 节点时隙寄存器（停机窗口内原子写入）** | host mem →(PCIe DMA)→ **DDR 代码段/数据段**；host → **NoC 节点时隙寄存器** | 每 kernel 一次；**时隙寄存器一次性** | 尚未进任何 cache |
| **③ 下发期** | runtime + TS | task descriptor、args buffer（含 `blockId` 来源）、输入张量 | host mem →(PCIe)→ DDR 参数区/张量区 + 任务队列 | **每次 launch** | 尚未进任何 cache |
| **④ kickstart** | TS + AI Core | 置 PC = 入口；栈指针；I$ invalidate(+preload)；D$ invalidate | **不搬数据**，只动控制状态 | 每个 block | 清空两个 cache |
| **⑤ 运行期（硬件自动）** | cache 控制逻辑 | 取指 2 KB 块把 `opcode/expVal` 立即数带进来；**取数 LD 的 miss 把 `routeBits` 表项按 64 B 行回填** | **DDR → Batcher.mem → I$/D$**（★ 不经 local DRAM、不经 L1） | 每次 miss | ← 本方案唯一的 cache 主体 |
| **⑤′ 运行期（软件显式）** | Scalar 发射 MTE3/MTE4 | payload 走 NoC，**旁路 D$** | UB ↔ NoC ↔ 对端 UB | 每次 SEND/RECV | 不经 D$ |
| **⑥ 收尾期** | 编译器插桩 / 手写 DCCI | **`routeBits` 行永不 dirty ⇒ 只需 invalidate，不需要 clean** | D$ → — | 算子尾部 | 为下一算子留干净状态 |

**读这张表的要点**：**④ 是分界线——kickstart 之后 host 就再也插不上手了**，剩下的全是片上自治。
本方案在 ②、③ 两阶段的形态是关键差异点：② 多一步 NoC 节点时隙寄存器写入，③ **不下发任何 Calendar 数据**。

### 3.7 一次 SEND 的三链模型

一条最普通的 LD 背后，是三条来源完全不同的链在同一拍汇合。Calendar 的发包指令是**同样的三链结构**：

```text
链 ①：SEND 指令本身 + 它携带的 opcode / redOp / bytes 立即数（A 类，走 I-Cache）
   PyPTO 求值 → 编进指令 → .text → host binary → PCIe → DDR 代码段 → Batcher.mem → I-Cache(2 KB 块)
   → 取指队列 → 解码 → 立即数字段直接成为操作数

链 ②：指令里用到的 routeBits（A 类，走 D-Cache）
   NoC C 算法编译期算出 → .rodata → host binary →【与 .text 同一次 PCIe DMA】→ DDR 数据段
   → Batcher.mem → D-Cache(64 B 行) → rbLo/rbHi（两条普通 LD 已经把它读进寄存器对）

链 ③：索引用的 blockId（B 类，走 D-Cache 或 SPR）
   host/TS 决定 block ↔ cell 映射 → args buffer → PCIe → DDR 参数区 → Batcher.mem
   → D-Cache(64 B 行) → GR
   （若平台提供 block_idx SPR，则这条链在 kickstart 阶段就已写入 SPR，运行期 0 次访存）
                                     ↓
                        三链在发包指令发射的这一拍汇合
```

- 链①的时间尺度是 **模型加载一次**；
- 链②的时间尺度**也是模型加载一次**（★ 本方案把 `routeBits` 从"每 launch 下发"压到了这个尺度上）；
- 链③的时间尺度是 **每次 launch 一次**；
- 三条链在 **DDR 汇聚**，经 **Batcher.mem** 在 **I$/D$ 分流**，在 **GR 会合**，最后一起被塞进 flit 头发出去。
  ★ 这条通路上**没有 local DRAM、没有 L1**。

### 3.8 D-Cache 容量预算与 miss 代价

`.rodata` 的第三条性质是"**免维护，不免容量**"——**随机索引的大 LUT 是 D-Cache 的头号污染源**。
§2.6 把表放大到 `keyCount × 40 × 16 B`，正面撞上这条警告，本节必须逐项回答。

#### 3.8.1 为什么 40 倍**不会**传导到 D-Cache

结论先行：**表按 40 行发射，每核的 D-Cache 占用一点都没变。**
这不是估算，是可以机械推出来的——依据是 D-Cache 的一条基本性质与 `blockId` 的一条使用性质。

**性质一：D-Cache 不是 `.rodata` 的镜像，是它的按需窗口。**
D-Cache 只在**某条 LD/ST 实际命名过的地址**上分配行，且分配粒度是 64 B 行、时机是 miss 的那一刻。
`.rodata` 段变大，只是让更多字节静静躺在 DDR 里；**没有任何机制会把没被访问过的字节搬进 cache**。

**性质二：`blockId` 在整个 kernel 内是常量。** 它由 `block_idx` SPR 或 kernel 入参给出，一次确定，此后不再变化。

两条合起来，本核对这张表发出的**全部** LD 地址集合是**可以完整枚举**的：

```text
本核在整个 kernel 内触碰这张表的地址集合
  = { kCalendarRoute_base + k*640 + blockId*16 + {0, 8}  :  k ∈ 本 kernel 用到的 key }
                            └编译期常量┘  └每核常量┘
    · 每个 key 恰好 2 条 LD（表项 16 B = 两条 64 bit LD）
    · 两条 LD 必落同一条 64 B 行（表项 16 B 且表 alignas(64) ⇒ 表项绝不跨行）⇒ 第 2 条必命中
    · 不同 key 相距 40*16 = 640 B = 10 条行 ⇒ 必落不同行

⇒ 每核 D-Cache 占用 = keyCount 条 64 B 行     ← 与 40 无关
⇒ 另外 39 行的字节：本核【没有任何一条指令会命名它们】⇒ 永不被 fill ⇒ 占用恒为 0
```

★ **"40" 只出现在地址算术的 stride 里，不出现在被触碰的行数里。** 这就是整个论证的全部内容。

**与 per-core 路线的对照（D-Cache 侧本方案反而更省）**：

| | per-core route image（B 类） | **本方案 40 行共享表（A 类）** |
|---|---|---|
| 装本核表项的行 | 1 条 | 1 条 |
| 装 runtime 下发基址入参的行 | **+1 条**（基址随 tiling 入参进 D-Cache） | **0**（链接期符号，折进指令立即数） |
| 每 key D-Cache 行数 | **2** | **1** |
| 取数访存 | 先读基址（可能 miss）→ 再读表项 | 直接读表项 |

#### 3.8.2 与"随机索引大 LUT"的本质区别

被警告的那类结构和本方案**形状相似、行为完全相反**，差别只在**下标的时间性质**：

| | **随机索引大 LUT**（被警告的病理） | **本方案的 `kCalendarRoute`** |
|---|---|---|
| 下标来自哪里 | **运行期输入数据**（每个元素、每次迭代都在变） | **`blockId`**：每核一次确定，kernel 期恒为常量 |
| 一次 kernel 内触碰多少条不同的行 | **不可预测，趋近整张表** | **恰好 `keyCount` 条，编译期就能算出来** |
| 工作集 | = 整张表（因为下一次访问可能落在任何一行） | = `keyCount × 64 B`，**与表的总大小解耦** |
| 命中率 | 随表增大而崩塌 | **首次 miss 之后 100% 命中**（只读、永不 dirty、不会被自己踢出） |
| 表按 40 行发射的后果 | 工作集放大 40 倍 ⇒ 持续踢行、污染 GEMM 的行 | **工作集不变**；那 40 行只躺在全片一份的 DDR 里 |

**一句话**：LUT 的病理**不在表大，而在下标随机**——大表只是把"下标随机"的代价放大。
本方案的下标是**编译期就固定、运行期恒定**的 `blockId`，访问模式退化成"每 key 一个固定地址"，
本质上更接近**一个常量的取址读**，而不是一次查表。另外 39 行是**冷数据**，和本核的热数据在 D-Cache 里从不相遇。

#### 3.8.3 真实存在的放大是 **4×**，与表大小无关，且可消除

诚实起见：确实存在一个放大，但它是 **4×、上界由 `64 B 行 ÷ 16 B 表项` 定死、与 40 无关**——
一条 64 B 行装 4 个表项（cell `4k..4k+3`），本核只用其中 1 个，**行内另外 48 B 是邻居 cell 的行，白占**。
这是"16 B 对象放进 64 B 行 cache"的固有损耗，任何布局都有。

**它可以被一个免费的布局改动消掉：把表转置成 node-major。**

| | `kCalendarRoute[keyId][node]`（key-major，本文基线） | **`kCalendarRoute[node][keyId]`（node-major）** |
|---|---|---|
| 本核 `keyCount` 个表项的地址 | 相距 640 B ⇒ **必然分散在 `keyCount` 条行上** | **连续 `keyCount × 16 B`** ⇒ 占 `⌈keyCount/4⌉` 条行 |
| 每核 D-Cache 行数 | `keyCount` | **`⌈keyCount/4⌉`** |
| FFN 示例（`keyCount = 2`） | 2 条行 = 128 B，2 次冷 miss | **1 条行 = 64 B，1 次冷 miss** |
| `keyCount = 16`（预算上限） | 16 条行 = 1 KB | **4 条行 = 256 B** |
| 地址算术 | `base + keyId*640 + blockId*16` | `base + blockId*(keyCount*16) + keyId*16` |
| 指令数 | 一次"运行期变量 × 编译期常量 + 编译期常量" | **完全相同**（`keyCount`、`keyId` 均为编译期常量） |
| DDR 占用（全片一份） | `keyCount × 640 B` | **完全相同** |

★ **代价为零、收益 4×**：指令数、寻址形态、表总字节数三者都不变，只是把"同一个 key 的 40 个核"聚在一起
改成"同一个核的 `keyCount` 个 key"聚在一起——而后者才是**实际的访问局部性**。

> **状态：已识别的优化项，本文**未**采纳为权威布局**（基线仍是 key-major）。切换只影响 §2.6 的寄存器对布局
> 说明、§1.7 取数伪码的两条 LD 地址、§3.8.1 的地址集合推导与编译期形状断言，**不影响任何硬件依赖与调用点**。
> **建议在 `keyCount > 4` 之前完成切换**——那之后 key-major 的每核驻留会线性增长到 1 KB，而 node-major 稳定在 256 B。
> 见 §6.3 待决项 C-9。

#### 3.8.4 容量与 miss 代价汇总

**DDR 占用（表的总字节数，全片一份）**：

```text
keyCount × 40 × 16 B                          ← 全片共用同一张表，不逐 cell 复制
  FFN 示例：2 × 40 × 16 = 1280 B（20 条 64 B 行）
  上限估算：即使 16 个逻辑身份，也只有 16 × 640 = 10 KB —— 相对 kernel binary 可忽略
  ★ per-core 路线的 40 行同样要全部放在这片 DDR 里，总量一致
```

**每核 D-Cache 驻留**（真正影响命中率的量）：

```text
key-major（基线）  ：keyCount 条行        FFN 示例 = 2 条 = 128 B
node-major（可选） ：⌈keyCount/4⌉ 条行    FFN 示例 = 1 条 =  64 B
  在 16 KB / 64 B = 256 行的 D-Cache 里占 2/256 = 0.78%（转置后 0.39%）
```

**miss 代价**（回填通路 = `DDR → Batcher.mem → D$`；**不经 local DRAM、不经 L1**）：

| 事件 | 次数 | 代价 |
|---|---|---|
| 每核每 key 的冷 miss | 1 次（第 2 条 LD 必命中同行） | 一次 64 B 行回填 |
| 全片 40 核对同一条行的 miss | 该行被 4 个核共享（cell `4k..4k+3`） | 各核各自发起 ⇒ **4 次 Batcher.mem 回填**（转置后 2 次）；**其中只有首次真正回到 DDR，其余由 Batcher.mem 直接供给** |
| 全片对整张表 | 20 条行，每条被 4 个核各读一次 | **80 次 64 B 回填请求**（= 40 核 × 2 key），但**回到 DDR 的只有 20 次**（每条行一次），一个 kernel 内一次性付清 |
| 循环内重复调用同一 key | 0 次 miss | 行已驻留，且永不 dirty、不会被自己踢出 |

> ★ **共享 Batcher.mem 的直接收益**：40 核读的是**同一份** DDR 镜像，所以"全片冷 miss"里绝大多数在
> Batcher.mem 就被吸收，真正穿透到 DDR 的只有整张表的 20 条行。

> ⚠️ **算子尾部 DCCI 的影响**：编译框架默认在算子尾部插入 `DataCacheCleanAndInvalid`，会把 Calendar 的行一并失效
> ⇒ **每个算子的第一轮都要重新 miss 一次**，账应按"每算子一次冷 miss"记，而不是"每 kernel 一次"。
> 好消息是表**永不 dirty**（禁令 F3），invalidate 不需要 clean 半边，代价只是一次重新回填。

**预算结论**：把 `.rodata` Calendar 表的 DDR 占用上限（全片一份）定为 **`keyCount ≤ 16`**（10 KB），
每核 D-Cache 驻留上限 **`keyCount` 条行（≤ 1 KB）**，并作为编译期硬校验项。
超过时的处理是**按相拆 kernel**（不同 launch 用不同 key 子集）或改用 node-major 布局，而不是压缩表项格式。

### 3.9 `CalReg` 的下发：装载期一次性写入 NoC 节点

时隙信息走的是**第三条路**——它既不进 I-Cache 也不进 D-Cache，而是直接落在 NoC 节点的寄存器里：

| 子项 | 内容 | 要求 |
|---|---|---|
| **存放位置** | 各 NoC 节点的 Calendar 时隙寄存器 `CalReg[opcode]` | 项数 = `opcode` 码位数；每项内部格式与容量由 NoC 硬件定义 |
| **写入时机** | **装载期一次性**，在停机窗口内**原子写入**；kernel 执行期只读 | 不允许在有流量在飞时改写；改写必须先排空全片 Calendar 流量 |
| **写入者** | Host / Runtime（配置通路，非数据通路） | 与 kernel binary 下发相互独立；寄存器内容的生命周期可跨多个 kernel |
| **索引方式** | 每个到达的 flit 用自己头部的 `opcode` 做一次寄存器索引 `CalReg[opcode]`，由其决定何时放行 | 节点侧无需知道逻辑身份、group 或 epoch；`opcode` 是**唯一**的索引；程序不感知寄存器内部格式与 cycle |
| **每 `opcode` 独立排队** | 节点为每个 `opcode` 维持独立的等待队列 | 避免跨 `opcode` 队头阻塞 |
| **非法 `opcode`** | `opcode ∈ {0, 7}` 或未写入内容的 `opcode` | 必须 fault 或丢弃并上报，不得按默认项放行 |
| **共享约束** | 同一 `opcode` 的所有集合共享同一项 `CalReg[opcode]` | 由 NoC 算法在生成寄存器内容时保证"同 `opcode` 无冲突"与"同 `opcode` 不并发"（§2.7.1） |
| **版本一致性** | `CalReg` 的 `calendarVersion` 必须与静态表的 `routeVersion`、芯片 `topologyVersion` 匹配 | 三版本联合校验放在装载期；任一不匹配必须 fault |

> 这条通路换来的是：**时隙信息完全不占核内存储、不占带宽、不随轮次搬运**——核内只出一个 3 bit 立即数。
> 与之对称的代价是 flit 头的 12 B 开销（§1.8）：**路由信息走"随包携带"，时隙信息走"常驻寄存器、按 `opcode`
> 索引"，两类信息各选了一条相反的路线。**

### 3.10 三条硬禁令与编译产物自检

| # | 禁令 | 为什么 | 违反后果 |
|---|---|---|---|
| **F1** | Calendar 的任何静态量**不得落进 `.data` / `.bss`**；`CalendarChannel`（含 `epochCtr`）必须是 kernel 内的局部对象 | SPMD 下可写全局变量是**全片共享**的——40 个核同时写同一个地址 | 数据竞争，且症状随核数与调度非确定；`routeBits` 被踩会静默错转发；`epochCtr` 被 40 核互踩 |
| **F2** | `CalendarKeyRef` / `expVal` 等编译期常量**不得被取址或绑引用** | 一次取址就把它从"立即数 → I-Cache"打落到"`.rodata` → D-Cache" | 每轮多一次可能 miss 的访存；`opcode` 不再是立即数，报到指令的信号量 id 与发包指令的时隙索引都退化成寄存器形式 |
| **F3** | `kCalendarRoute` 表**必须 `alignas(64)`，且程序内不得有任何 ST 指向它** | 未对齐会让一个表项跨两条行，miss 翻倍；一次 ST 会让整条行变 dirty，尾部 DCCI 必须走 clean 半边 | 冷启动代价翻倍；失去"永不 dirty"这条性质 |

★ **由 F2/F3 派生的一条具体纪律**：**不得**按 `/*rbLo=*/e.routeBits, /*rbHi=*/e.routeBits + 8` 传参——
那是**取址**，会把表项变成内存操作数。发包指令的 `rbLo` / `rbHi` 是 **`uint64_t` 值**，
由取数 helper 从已 LD 回来的表项中按值取出（§5.8 的 `CalendarRouteRegs`），**取值不取址**。

**编译产物自检脚本**（建议进 CI）：

```bash
# 1) .data / .bss 必须接近 0 —— 出现自己的符号就是 F1 违规
llvm-readelf -S calendar_kernel.o | grep -E '\.text|\.rodata|\.data|\.bss'
llvm-nm --print-size --size-sort calendar_kernel.o | grep -E ' [bBdD] '   # ← 有输出就要警惕

# 2) Calendar 表必须在 .rodata（符号类型 R/r），且大小 == keyCount*40*16
llvm-nm --print-size --size-sort calendar_kernel.o | grep kCalendarRoute
#    期望：xxxxxxxx 00000500 r kCalendarRoute        （FFN 示例：0x500 = 1280 B）

# 3) 64 B 对齐自检
llvm-readelf -S calendar_kernel.o | grep -A1 rodata      # 看 Address 与 AddrAlign

# 4) F2 自检：kCalendarRoute 之外不应出现 kRowAllGather / kColAllGather 等符号
#    它们若被物化，说明某处取了地址
llvm-nm calendar_kernel.o | grep -E 'kRowAllGather|kColAllGather'   # ← 期望无输出

# 5) 反汇编确认 opcode 确实是立即数（报到指令的信号量 id 与发包指令的 opcode 字段）
llvm-objdump -d calendar_kernel.o | grep -A2 SET_CROSS_CORE         # ← 期望看到 #1 这样的立即数

# 6) 反汇编中【不应】出现任何 Calendar 专用控制面指令（时隙对齐必须走既有跨核报到指令）
llvm-objdump -d calendar_kernel.o | grep -i CALENDAR_ALIGN          # ← 期望无输出
```

### 3.11 退化路径：拓扑非同构时表降级为 B 类

§3.5 的结论有一个前提——**目标芯片实例的物理拓扑与编译期假设一致**。
若坏核 / 坏链导致每片实例的可用节点集或链路不同，那么 40 行的内容就随实例而变，表被迫降级回 **B 类**，
只能改用"装载期分配的只读 buffer + 基址随入参下发"。届时链路变成：

```text
编译期只出模板 → 装载期按实例拓扑重算 40 行 → runtime 分配只读 buffer → H2D → DDR
   → 基址随 tiling 入参进 D-Cache（B 类）→ 取数变成【两次 LD】：先读基址、再读表项
   （回填通路不变，仍是 DDR → Batcher.mem → D-Cache）
```

代价是取数多一次可能 miss 的访存，以及每次 launch 多一次下发；**其余各章不受影响**，
因为它们只依赖"SEND 前能拿到 `routeBits`"，不依赖它来自哪个段。是否允许编译期假设"拓扑同构"，
还是从一开始就按 B 类设计，见 §6.3 待决项 C-11。

---

## 4. Calendar 相关硬件指令

### 4.1 指令增量总表：控制面 0 条新增 + 数据面 2 条

**指令流需要三种 Calendar 语义，其中控制面那一种完全复用既有指令，只有数据面两种是新增。**
路由取值使用普通标量 LD（不单列步骤），同样不占新增名额。

| 侧 | 指令 | 增量 | 每轮条数 | 前提 |
|---|---|---|---|---|
| **取路由位图** | `LD` × 2（普通标量指令） | **+0** | 2（同一 64 B 行，第 2 条必命中） | `.rodata` 表 64 B 对齐；`blockId` 可得 |
| **取本核节点号** | `MOV_SPR2X`（BLOCKID → Xd）或 kernel 入参 | **+0（复用）** | 1（或 0） | §4.3 |
| **对齐·报到** | `SET_CROSS_CORE(calGroup, opcode)` | **+0（复用既有）** | 1 | §4.2；`opcode` 作为 3 bit 立即数直接充当信号量 id；需扩展作用域至全组并让 BSP 可观测该汇合点 |
| **对齐·等齐** | `WAIT_FLAG_DEV(opcode)` | **+0（复用既有）** | 1 | 同上 |
| **本轮 epoch** | `CalendarNextEpoch(opcode)`（普通标量自增） | **+0** | 1 | §1.5；报到指令无返回值后 epoch 的来源 |
| **感知侧（收）** | `MTE4_NOC_RECV_WAIT(dst, capacity, expVal, opcode, epoch)` | **★+1（新增）** | 1 | **只感知不搬运**；必须按实际有效字节完成，不能由通用等待指令替代 |
| **发送侧** | `MTE3_NOC_SEND(dstSym, src, sid, nBurst, lenBurst, srcGap, dstGap, am0, am1, rbLo, rbHi, opcode, redOp)` | **★+1（扩展既有 MTE3 核间写）** | **1**（整 tile）或 `rowCount`（逐行） | 需要 2 个额外 GPR 源槽 + 6 bit 立即数；既有编码没有 80 bit 路由槽，基本不可原样复用 |
| （备选 B） | `CALENDAR_SEND_CTX_SET(rbLo, rbHi, opcode, redOp)` | +1（仅方案 B） | 1 | 仅当 SEND 编码槽放不下时启用（§4.4.3） |
| **流水线** | `PIPE_MTE3` + `PIPE_MTE4` | **两者都是既有流水（+0）** | — | 必须独立前进；§4.7 |

> **机器码增量口径**：**方案 A 为 2 种新增；方案 B 为 3 种**（控制面 0 种）。
> 无论哪种编码选择，程序中都必须出现**一条 MTE4 `expVal` 感知语义**和**一组 MTE3 发送语义**，
> 且不得退回 `notify` / 单独信号量写作为正确性路径。

### 4.2 控制面：时隙对齐复用既有跨核报到 / 汇合指令（+0）

**本方案不为时隙对齐新增任何机器指令。** 报到 + 汇合这件事，既有的
`SET_CROSS_CORE(core_id, event_id)` / `WAIT_FLAG_DEV(event_id)` 语义已经完全具备
（信号量计数器：`SET_CROSS_CORE` 加一、`WAIT_FLAG_DEV` 减一并在归零时放行），
而 Calendar 需要的唯一额外信息 `opcode` 又恰好能塞进现成的 `event_id` 槽。

```text
Step2   SET_CROSS_CORE(calGroup, opcode)     // 本核向 opcode 对齐域报到（既有 ISA）
        WAIT_FLAG_DEV(opcode)                // 阻塞到全组到齐（既有 ISA）
```

- **是什么**：两条既有指令组成的全组 rendezvous，语义为"通告 BSP：本核数据与路由位图已就绪，本轮走 `opcode` 域"。
  全组到齐后，BSP / NoC 硬件在内部为全组对齐本轮时间原点，然后放行。
- **操作数**：`core_id` 给出 Calendar 对齐域（全组），`event_id` = **3 bit `opcode` 立即数**；
  没有任何表句柄、没有 `routeBits`。
  - `opcode ∈ 1..6` 落在 `event_id ∈ 0..15` 的合法区间内，**不需要扩宽既有编码**。
  - 为什么需要 `opcode`：BSP 要用它把本核归入正确的 align 域，并确定该域在硬件内部预留的 arm lead time。
  - 为什么不需要 `routeBits`：对齐只决定"本轮从何时起按 `CalReg[opcode]` 放行"，与路径无关。
- **程序无需感知 cycle**：时间原点由硬件内部维护、不进入程序可见状态。程序侧不保存、不传递、
  不计算任何 cycle 值或时隙编号。
- **为什么必须有**：NoC 节点按时隙寄存器错峰共享链路。若各成员进入集合的时刻不对齐，同一项
  `CalReg[opcode]` 在不同成员上的时隙会错位，原本错峰的 flits 可能同时争用同一传输资源，引起
  **总线传输冲突/拥塞与性能大幅下降**（不变式 **O2**）。转发路径由 `routeBits` 决定、不因时隙错位而改变，
  因此这**不是静默错转发**。

#### 4.2.1 复用的真实代价：三处对既有指令的扩展请求

复用既有指令不等于零成本。下面三项必须由硬件兑现，否则复用不成立：

| # | 扩展请求 | 既有现状 | WSE 要求 | 归属 |
|---|---|---|---|---|
| **X-1** | **作用域**：从 cluster 内扩展到 Calendar 的 `opcode` 域 | `SET_CROSS_CORE` 是 cluster 内（1 Cube : 2 Vector）的 broadcast/reduce | 必须覆盖该 `opcode` 域的**全部报到核**（FFN phase B 为 32 核，全片集合为 40 核） | 硬件 |
| **X-2** | **观测方**：BSP 必须能观测该汇合点 | 既有形态没有这个观测方，纯粹是核间同步 | BSP 必须能观测信号量归零那一刻，并以它为全组时间原点（不返回给程序） | 硬件 + BSP |
| **X-3** | ★**信号量位宽**：必须容得下该 `opcode` 域的报到核数 | 每个 `event_id` 一个 **4 bit** 计数器（0..15） | 32 核与 40 核都需位宽 **≥ 6 bit**（即 `≥ ⌈log2(n+1)⌉`），或改用"按成员位图判定到齐"的 reduce 语义（不用计数器） | 硬件 ISA |

> **X-3 是最尖锐的一条。** 它不影响"+0 条指令"这个结论（不是新指令，是既有指令的编码位宽），
> 但它是一条**必须在 ISA 评审上确认的正确性依赖**：4 bit 计数器在 16 个报到核以上会回绕，
> 回绕后 `WAIT_FLAG_DEV` 会在全组未到齐时提前放行，直接破坏不变式 **O2**，且**软件侧无法检测**。
> 同时需确认 `event_id` 的可用编号域在 WSE 上仍 ⊇ `0..6`（`opcode` 占 1..6）。

另需一个新的跨核同步消息 **mode 码位**（`CAL_GROUP_SYNC`）以区别于既有的核间同步 mode：
这是对**消息编码**的扩展，不是对指令集的扩展。

#### 4.2.2 被否决的三个替代形态

| 形态 | 增量 | 为什么不选 |
|---|---|---|
| 新增 `CALENDAR_ALIGN_REQ(opcode) -> epoch` | +1 | 语义与既有报到/汇合重合；唯一的额外收益是靠返回值带回 epoch，而 epoch 可由核内计数器零成本给出（§1.5） |
| 折进 Step4 首包（首条 SEND 隐式对齐） | 0 | 无法在首包被 `CalReg[opcode]` 放行前为 MTE4/MTE3 留出统一 arm lead time，且对齐/异常语义隐含 |
| 装载期一次性对齐、运行期不再对齐 | 0 | 各成员进入集合的时刻不同，仍会在同一项 `CalReg[opcode]` 上错峰失效（不变式 **O2**） |

### 4.3 取路由位图：两条普通标量 LD（+0）

这是本方案在指令面上最"省"的一处：**取路由信息不需要任何新增指令。**

| 项 | 说明 |
|---|---|
| **指令形态** | 两条普通 64 bit 标量 `LD`，由编译器为发包指令的寄存器操作数自动生成。对用户透明、不是 Calendar 专用指令，因此**不占新增指令名额、也不单列执行步骤** |
| **地址形态** | `base + keyId*640 + blockId*16 + {0, 8}`：`keyId` 是编译期立即数（折进地址偏移），`blockId` 是唯一的运行期变量部分 |
| **访存通路** | `.rodata` → D-Cache（64 B 行）；miss 由硬件经 **MSHR → BIU → Batcher.mem**（未命中再回 **DDR**）→ **64 B 整行回填**。**全程不经 local DRAM、不经 L1** |
| **命中率** | 表项 16 B、表 `alignas(64)`，一个表项不跨行 ⇒ 同一表项的第 2 条 LD 必命中；每核每 key 只有 1 次冷 miss |
| **可调度性** | 与 `sendTile` 的生产者无数据依赖，可提前到 `TSYNC` 之前发射，让可能的 miss 延迟与"等数据"的空窗重叠 |
| **不需要栅栏** | `routeBits` 只活在 GPR 与 flit 头里，**没有第二个可见性面**——普通标量 RAW 依赖就是全部（不变式 **O4**） |

**`blockId` 的取值**：

| 优先级 | 形态 | 机器形态 | 访存次数 |
|---|---|---|---|
| 1 | 读 `BLOCKID` SPR | `MOV_SPR2X`（BLOCKID → Xd） | **0** |
| 2 | 随 kernel args 下发 | 普通标量 load | 1（与表项 LD 形成两级依赖，miss 可能串行叠加） |

硬件仍须对 `blockId ≥ 40` 自行 fault；软件侧另有一道预检断言。

### 4.4 数据面（发）：`MTE3_NOC_SEND`

**这是本方案与"配置式路由"路线的分水岭。** 没有"每轮配置一次的硬件状态"，**两类信息必须在发包处进入数据面**。

#### 4.4.1 方案 A（采用）：直接编码进发包指令

发包指令的形态是**对既有 MTE3 核间写出通路的扩展**：前 9 个操作数逐位沿用既有的 align 模式字节粒度 DMA
（`UB → 核外`，`lenBurst` 以字节计、支持非整块尾部），末尾追加 4 个 Calendar 操作数。

```text
MTE3_NOC_SEND(dstSym, src, sid, nBurst, lenBurst, srcGap, dstGap, am0, am1,
              rbLo, rbHi, opcode, redOp)

  ---- 以下 9 个逐位沿用既有操作数表 ----
  dstSym  : 既有槽①  目的地址  → ★扩展为「对称 NoC 地址」（= recvSymBase + selfOff + r0 × recvRowStride）
  src     : 既有槽②  本核 UB 行起点（32 B 对齐，见 R-1）
  sid     : 既有槽③  service / QoS 标识；Calendar 方向无意义，恒 0
  nBurst  : 既有槽④  行数（= rowCount，或 1）
  lenBurst: 既有槽⑤  每行有效字节数（= rowBytes），**单位为字节**
  srcGap  : 既有槽⑥  源相邻行间隙，由 sendRowStride − rowBytes 推出
  dstGap  : 既有槽⑦  目的相邻行间隙，由 recvRowStride − rowBytes 推出
  am0/am1 : 既有槽⑧⑨ align 模式头/尾处理域；Calendar 恒 0

  ---- 以下 4 个是 Calendar 新增的架构操作数 ----
  rbLo    : GPR，routeBits[63:0]        ← node 00..31 的位对
  rbHi    : GPR，硬件只取 [15:0]        ← node 32..39 的位对；高 48 bit 必须忽略
  opcode  : 3 bit 立即数                ← 索引 NoC 节点时隙寄存器 CalReg[opcode]
  redOp   : 3 bit 立即数                ← 归约算子；AllGather 时 dormant
```

| # | 采用理由 | 说明 |
|---|---|---|
| 1 | **`routeBits` 本来就要逐包送到 flit 生成器** | 本方案的前提就是"NoC 不查表 ⇒ 每个 flit 自带路由位图"。放在发包指令里是它**唯一的自然入口**，不是冗余 |
| 2 | **支持逐行不同路由** | `ReduceScatter` / `Scatter` 的不同 shard 去往不同目的，本方案靠**循环内换 `rbLo/rbHi` 寄存器**表达 |
| 3 | **AllGather 场景零额外开销** | `rowCount` 行共用同一份 `routeBits` ⇒ 循环外一次 LD、循环内只是复用两个寄存器，**零额外访存、零额外 D-Cache 流量**；代价只在**每条指令的编码宽度**上 |
| 4 | **不引入额外的控制面指令** | 相比方案 B 少一条 ISA，也少一个需要上下文隔离的硬件状态 |

#### 4.4.2 「地址做扩展」到底扩展了什么

既有指令的槽① 是一个**已解析的线性核外地址**：硬件按它直接寻址一个落点。Calendar 变体对该槽做两项扩展，
**其余 8 个槽一字不改**：

| # | 扩展 | 内容 |
|---|---|---|
| **A-1** | **地址语义**：已解析地址 → **对称地址** | 槽① 里的值是**本核**竞技场地址；各成员的物理落点由 NoC 按 `routeBits` 解析（`L=1` 的节点在**同一核内 offset** 上落核）。软件不做任何跨核地址算术，也不持有对端指针 |
| **A-2** | **地址域**：线性全局空间 → **Calendar 对称空间** | 需要一个"这是对称地址、不是普通核外地址"的编码位。**请裁决：这在机器编码上是一个寻址模式位，还是另一个操作数槽？**（§6.3 待决项 S-3） |

除此之外发生变化的只是**操作数个数**（末尾 +4）。这正是"只是地址做扩展 + 加路由/时隙参数"这句话在 ISA
层面的全部含义——**没有新的寻址算术、没有新的长度单位、没有新的完成语义**。

#### 4.4.3 方案 B（备选，仅当方案 A 的编码槽放不下时启用）

```text
CALENDAR_SEND_CTX_SET(rbLo, rbHi, opcode, redOp)   // ★额外 +1 条 ISA，每轮 1 条
MTE3_NOC_SEND(dstSym, src, sid, nBurst, lenBurst, srcGap, dstGap, am0, am1)  // 隐式用当前 send context
```

| 侧面 | 方案 A（采用） | 方案 B（备选） |
|---|---|---|
| 新增机器指令总数 | **2** | 3 |
| SEND 指令编码需求 | 2 个额外 GPR 源槽（或 1 个偶奇寄存器对槽）+ 6 bit 立即数 | 无额外槽 |
| 每轮 SEND 条数 | 1（整 tile）或 `rowCount`（逐行） | 同左 |
| 逐行换路由 | ✅ 直接支持 | ❌ 需在循环内插 `CALENDAR_SEND_CTX_SET`，退化成 `2 × rowCount` 条 |
| 新增硬件状态 | 无 | 一个需按 AICORE / kernel / stream 上下文隔离的 SPR |
| 额外栅栏 | 无（标量 RAW 即可，**O4**） | **需要 SPR 写 → MTE3 读的显式可见性栅栏**（把本方案删掉的那类栅栏又请了回来） |

> **决策标准**：若 MTE3 编码能容纳"2 个额外 GPR 源操作数槽（或 1 个偶奇寄存器对槽）+ 6 bit 立即数"，采用 **方案 A**；
> 否则采用方案 B，并同步接受它的 SPR 上下文隔离与栅栏成本。见 §6.3 待决项 C-1。

#### 4.4.4 `routeBits` 的硬件契约（五条，无论 A/B）

1. 硬件按 §2.2 的位序解释 `routeBits`：`bit(2i+1) = P_i`、`bit(2i) = L_i`，bit `b` 位于字节 `b/8` 的第 `b%8` 位；
2. 出现 `01` 位对（`L=1` 而 `P=0`）必须 **fault**，不得静默丢弃；
3. `P_self == 0` 必须 fault——发送核自己必须在路径上；
4. 全零 `routeBits` 或 `opcode == 0` 必须 fault（保证"未初始化的头部"不会被当成合法包）；
5. `rbHi` 的高 48 bit **必须被忽略**，硬件不得对其做任何检查——软件把 `landCount` / `flags` /
   `expectedRxBytes` 打包在那里（§2.6 的免费搭车依赖这一条）。

> 位图的**合法性**（`{P_i=1}` 的诱导子图连通、含 source、无环）由编译期硬校验保证（§2.7.2），
> 硬件只负责按上面五条执行与 fault。

#### 4.4.5 两种发包形态

复用既有 align DMA 的 `(nBurst, lenBurst, srcGap, dstGap)` 参数骨架，与 Calendar 竞技场的二维几何
（`rowCount` 行 × `rowBytes` 字节 × 源/目的行跨距）**逐项对应**：

| 用法 | `nBurst` | 每轮 SEND 条数 | 适用 |
|---|---|---|---|
| **整 tile 一条** | `rowCount` | **1** | `AllGather` / `Reduce`：`rowCount` 行共用同一份 `routeBits` 与 `opcode`，几何是等距二维 ⇒ 用 `srcGap`/`dstGap` 表达即可 |
| **逐行** | `1` | `rowCount` | `ReduceScatter` / `Scatter`：不同 shard 去往不同目的 ⇒ 循环内换 `rbLo/rbHi` 寄存器对 |

整 tile 形态要求硬件把**同一份 flit 头**施加到该描述符展开出的全部 burst 上（`{opcode, redOp, epochTag,
routeBits}` 逐 flit 复制，与逐行形态逐字等价），MTE4 的字节计量不受影响（总字节数相同）。
**首期若硬件只支持 `nBurst = 1`，退化为逐行形态即可，软件接口不变。**

#### 4.4.6 继承自被复用指令的两条约束

| # | 约束 | 对 Calendar 的含义 |
|---|---|---|
| **R-1** | **UB 源地址必须 32 B 对齐**，且源侧按整数个 32 B burst chunk 读取 | `sendTile` 的行起点必须 32 B 对齐 ⇒ `sendRowStride` 应为 32 B 整数倍。FFN 示例（`rowBytes` = 192 B / 1536 B）天然满足。**这是正确性约束，不是效率建议** |
| **R-2** | `lenBurst` 以**字节**计，align 模式的用途正是**支持非整块尾部** | 行字节数**不必**是 32 B 整数倍；"链路位宽整数倍"仍是**效率**建议（不是正确性约束），且**具体链路位宽不进软件接口** |

`srcGap` / `dstGap` 的**单位**（32 B block 还是 byte）以及槽⑧⑨ 的确切含义继承自既有指令，
需以目标 ISA 手册为准——见 §6.3 待决项 S-1。

### 4.5 数据面（收）：`MTE4_NOC_RECV_WAIT`

```text
MTE4_NOC_RECV_WAIT(dst, capacity, expVal, opcode, epoch)
    dst      : 本核接收竞技场基址（对称地址，全组同址）
    capacity : 接收范围字节容量 = recvRowCount × recvRowStride
    expVal   : 本轮期望的远端有效 payload 字节数
    opcode   : 3 bit 立即数 —— 判别键的一半，并建立本轮 {opcode, epoch} 上下文
    epoch    : 本轮 collectionEpoch，来自核内 CalendarNextEpoch(opcode)
```

在 `PIPE_MTE4` 上建立本轮接收范围与期望字节数：**只有匹配当前 `{opcode, epoch, dstRange}` 且已提交可读的
有效 payload bytes 累计达到 `expVal`，该 MTE4 命令才退休**。它**不搬运任何字节**——载荷由 NoC 直接写进
本核竞技场；其"未完成"纯粹是计量未达阈值，不占搬运带宽。

阻塞的是**该流水命令不退休**，不是 AICORE 标量发射阻塞——发出本指令后 AICORE 必须能继续向 `PIPE_MTE3`
派发（不变式 **O3**）。

#### 4.5.1 ★ 为什么 `opcode` 必须是显式操作数

既然对齐复用的是**通用**跨核同步指令，它**不在核内建立任何 Calendar 侧上下文**——没有"当前 align 域"
这个可被 MTE4 读取的硬件状态；`event_id` 恰好取值为 `opcode` 是**软件约定**，不是硬件把它记进了某个
Calendar 寄存器。因此 `opcode` 必须由接收指令**显式携带**。这带来三个好处，没有坏处：

1. **判别键自洽**：`{opcode, epoch, dstRange}` 三个分量全部来自本条指令的操作数，不依赖任何隐含状态；
2. **不引入需要上下文隔离的硬件状态**：与"控制面 0 条新增"的目标一致；
3. **代价为零**：`opcode` 是 3 bit 立即数，不增加寄存器压力。

**本条指令同时建立本轮 `{opcode, epoch}` Calendar 上下文**，供 Step4 的 flit `epochTag` 取值
（这样 `epoch` 不必占 SEND 的编码槽）。其硬件含义是一个**按核的、每轮一次写入的隐含上下文**，要求：

- 由 **O3** 保证 Step3 先于 Step4（软件侧纪律）；
- 该上下文必须按 **AICORE / kernel / stream 上下文隔离**；
- 备选做法是让 SEND 也显式带一个 `epoch` 操作数（彻底无状态，代价是多占编码槽）——见 §6.3 待决项 C-6。

#### 4.5.2 `expVal` 的 P0 契约（七条）

1. 单位固定为有效 payload **字节**：不计 flit 头（含那约 12 B 的 `routeBits` / `opcode` / `epochTag` 头）、
   CRC、对齐填充、重传副本与本核来源数据；即使 `routeBits` 把 `L_self` 置 1，`srcRank == selfRank` 的字节
   也必须标记为 non-counting；
2. 只统计落在 `[dst, dst + capacity)` 且匹配当前 `{opcode, epoch}` 的写入；越界、错 `opcode` 或错 epoch
   必须丢弃并 fault；
3. 计量发生在 payload 已提交并对本核后继可读之后——**MTE4 完成天然具备 acquire 语义**；
4. 早于本指令执行、但属于同一已建立 epoch 的合法到达**不得漏计**（实现可用 ingress 记账或先登记接收描述符）；
5. 重复包不得重复计数，需要 packet / segment identity 去重。**多播分叉复制出的副本必须按落核点去重**——
   位对为 `11` 的节点既落核又续转，硬件必须保证同一 segment 在同一目的核只计一次；
6. `expVal == 0` 立即完成；`expVal > capacity`、与编译结果不一致或计数溢出必须 fault，**不能永久静默阻塞**；
7. **★ 判别键弱化的补偿**：`opcode` 的判别力**很弱**——程序里所有 `AllGather` 共用 `opcode = 1`。
   因此 **`collectionEpoch` 必须承担全部区分职责**：本轮 epoch 必须在该 `opcode` 域内唯一
   （由核内 `CalendarNextEpoch(opcode)` 单调递增给出，SPMD 下全组一致；回绕前旧 epoch 的包必须已排空），
   且 NoC 算法的"同 `opcode` 不并发"保证（§2.7.1）必须成立。两者缺一，上一轮（或另一集合）滞留的重传 /
   复制副本就可能被本轮同 `opcode` 的 MTE4 误记入。

### 4.6 为何收与发不能合并为一条指令

| 拟议指令 | 执行侧/流水 | 关键操作数 | 完成语义 |
|---|---|---|---|
| `MTE4_NOC_RECV_WAIT` | 每个目的核，`PIPE_MTE4` | 对称接收范围、容量、期望有效字节数、`opcode`、collection epoch | **纯感知，不搬运字节**：仅匹配 `{opcode, epoch, dstRange}` 且已提交可读的 payload bytes 累计达到 `expVal` 后退休 |
| `MTE3_NOC_SEND` | 每个源核，`PIPE_MTE3`（**既有核间写流水**） | 源 UB、对称目的地址、二维几何、**路由位图**、**时隙寄存器索引**、归约算子 | **MTE3 把包发到目的地**：数据由 NoC 节点按 `CalReg[opcode]` 放行注入，按 `routeBits` 被动转发；本指令不写信号量、不携带 notify |

- MTE3 send 的操作对象是本核源数据和对端地址，MTE4 感知的操作对象是本核目标范围与本轮期望量，**操作数不对称**；
- 一发多收（多播）时，**发送条数与某个目的核的接收总量不一一对应**；
- 感知等待必须允许发送流水继续前进，否则所有核可能互等；
- 二者语义正交（一个搬运、一个纯计量），合并会把"发多少"和"收多少"绑死；
- 独立指令还能支持 receive-only、send-only、流水化分块和未来非对称集合语义。

**相关既有机制的适用性**：

| 指令或机制 | 能做到什么 | 本方案要求 |
|---|---|---|
| 收方向的既有块 DMA（外部 → UB） | **主动搬运**：由本核发起读取 | ✗ Calendar 的载荷是**远端推来的**，本核既不知道源地址、也不发起读 |
| `WAIT_SPR(slot, threshold)` / 记分板等待 | 等一个 SPR/SCB 数值 | ✗ 不绑定接收目标范围、`opcode`、epoch 或 payload 提交，不能证明"数据已落地且够数" |
| `WAIT_FLAG_DEV(id)` | 设备级事件等待（信号量归零放行） | ✗ 度量的是**事件/通知次数**而非实际数据量；且它已被 §4.2 用于对齐，语义复用会冲突 |
| `pipe_barrier(PIPE_MTE4)` | 等本核 MTE4 队列里的命令完成 | ✗ 只有当队列里那条命令**自身带 `expVal`** 时它才等价于"收齐"——这正好说明必须先有 `MTE4_NOC_RECV_WAIT` |
| `notify` / write-with-notify / `NOTIFY_GROUP` | 额外写信号量或随 payload 产生 credit | ✗ 度量通知次数而非实际数据量，并引入计数器分配/清零/溢出与 payload→notify 有序性合同 |
| 原子累加 + 普通 DMA | 让写入侧原子累加一个计数器 | ✗ 需要发送侧额外写与清零协议，且仍不绑定 `dstRange` 与 epoch |
| **既有 MTE3 细粒度核间写** | 点到点或核间写，UB → 对端 | **流水线与方向完全复用**（`PIPE_MTE3`，+0 条流水线）；但**指令编码不可原样复用**——既有形态没有 80 bit 路由操作数槽，也没有 `opcode` 时隙索引 |

> **没有一条既有指令同时具备"绑定接收地址范围 + 按有效 payload 字节计量 + 在 MTE4 流水上阻塞完成"这三件事**，
> 因此接收侧是本方案唯一一条**全新**的指令。

### 4.7 流水线要求：`PIPE_MTE3` / `PIPE_MTE4` 独立前进

- `PIPE_MTE3` 是**发方向**，也是**核间通信的既有通路**：本方案把 Calendar 的发包挂在同一条流水上，
  由 MTE3 把包发到目的地；发出时机由 NoC 节点按 `CalReg[opcode]` 放行，路径由本包携带的 `routeBits` 决定。
  **`PIPE_MTE3` 是既有流水线级，本方案不新增流水线。**
- `PIPE_MTE4` 是**感知方向**：它**不搬运数据**——载荷由 NoC 直接写进本核竞技场；MTE4 只按
  `{opcode, epoch, dstRange}` 统计已提交的有效载荷，并使接收指令在达到期望字节数前保持未完成。
- 两条流水必须能**独立派发、独立反压、独立 barrier**。若 MTE4 未满足 `expVal` 时会冻结 AICORE 或阻止 MTE3
  继续派发，则所有成员可能都在等感知完成、无人发包，形成**全组死锁**。

同步/事件编码至少满足：

1. `set_flag` / `wait_flag` 可在 `PIPE_MTE4`、`PIPE_MTE3` 两个方向表达本核前后继依赖；
2. `pipe_barrier(PIPE_MTE4)` 与 `pipe_barrier(PIPE_MTE3)` 均有效，`PIPE_ALL` 覆盖两者；
3. MTE4 命令的"阻塞"定义为**该流水命令不退休**，不是 AICORE 标量发射阻塞；MTE3 必须仍能前进；
4. `pipe_barrier(PIPE_MTE3)` 必须等待所有已入队命令**实际发出/排空**，不能因"尚未被 `CalReg[opcode]` 放行"
   而提前返回；
5. Step5 的 `PIPE_MTE3` 排空先于最终等待 MTE4 完成；
6. 标量流水到 `PIPE_MTE3` 的 RAW 依赖必须覆盖 `rbLo` / `rbHi`：取 `routeBits` 的 LD 未写回前，
   Step4 的 SEND 不得读到旧值（不变式 **O4**）。若采用方案 B，则需显式 SPR 写 → 读栅栏。

> ⚠️ **复用 `PIPE_MTE3` 的一个副作用**：`pipe_barrier(PIPE_MTE3)`（Step5 的第一动作）会**连同本核其他
> store-out 命令一起排空**——例如同一相里的 `TSTORE`。这在正确性上是安全的（更强的栅栏），但会把
> 不相关的搬运也拖进关键路径。若实测代价明显，可用**按 event token 的细粒度 `wait_flag`** 代替整条
> `pipe_barrier`，只等本轮 Calendar 发包的那批命令退休。

### 4.8 拆包 / 转发 / merge 对软件透明

- 软件每次只描述一个二维 tile（或一行），**不逐 flit 编程**；按链路位宽拆 flit、按 `{P,L}` 转发、
  目的核 merge、尾 flit 填充、仲裁与重传全部由硬件完成。软件唯一的切分来自**竞技场二维**，与 flit 无关。
- **头部复制对软件透明**：一段连续字节拆成多个 flit 后，每个 flit 复制同一份约 12 B 头部由硬件完成。
- 建议每行字节数是链路位宽的整数倍（**效率**约束，不是正确性约束）：FFN 示例 phase B `rowBytes = 192 B`、
  phase C `rowBytes = 1536 B`，均为 64 B 的整数倍。**链路位宽只是示例量，不进软件接口。**
- MTE4 以**有效字节提交**为计量点。跨行 flit 可以乱序，只要去重后每个匹配 segment 的有效字节仅计一次。

---

## 5. CCE 与 PTO 指令接口设计

本章把第 4 章的机器指令面落到两层软件接口上：**CCE facade**（`__builtin` 的薄封装，暴露给下沉层）与
**PTO 包装层**（`TCalendar<K>`，暴露给算子作者）。

### 5.1 三名对应表与命名依据

| 类别 | 机器助记符 | CCE facade | builtin | 命名层 | 增量 |
|---|---|---|---|---|---|
| 控制面·报到 | `SET_CROSS_CORE` | `ffts_cross_core_sync` | `__builtin_cce_ffts_cross_core_sync` | 裸名层 | **+0** |
| 控制面·等齐 | `WAIT_FLAG_DEV` | `wait_flag_dev` | `__builtin_cce_wait_flag_dev` | 裸名层 | **+0** |
| 控制面·epoch | — | `CalendarNextEpoch`（纯标量自增，**不是** intrinsic） | — | PTO 层 | **+0** |
| 取路由·节点号 | `MOV_SPR2X`（BLOCKID → Xd） | `get_block_idx()` | `__builtin_cce_get_blockid` | 裸名层 | **+0** |
| 取路由·表项 | `LD` × 2（普通标量） | 编译器生成，无 facade | — | — | **+0** |
| 数据面·收 | `MTE4_NOC_RECV_WAIT` | `wait_noc_to_ubuf` | `__builtin_cce_wait_noc_to_ubuf` | 裸名层 | **★+1** |
| 数据面·发 | `MTE3_NOC_SEND` | `copy_ubuf_to_noc_ubuf_align_b8` | `__builtin_cce_copy_ubuf_to_noc_ubuf_align_b8` | 裸名层 | **★+1** |
| 数据面·发（备选 B） | `CALENDAR_SEND_CTX_SET` | `set_calendar_send_ctx` | `__builtin_cce_set_calendar_send_ctx` | 裸名层 | +1 |

**命名依据**（逐条对照既有命名惯例）：

| 名 | 依据 |
|---|---|
| `ffts_cross_core_sync` | 既有裸名层同步族成员，语义为"多核调度上下文中的跨核同步"。报到这件事既有指令已完全具备，Calendar 需要的唯一额外信息 `opcode` 又恰好落在消息现成的 `flagId` 槽（`opcode ∈ 1..6 ⊂ flagId ∈ 0..15`） |
| `wait_flag_dev` | 既有裸名层 wait 族成员（"设备级 / 扩展事件等待变体"），与 `ffts_cross_core_sync` 成对使用是既有用法 |
| `copy_ubuf_to_noc_ubuf_align_b8` | 搬运类一律 `<VERB>_<SRC>_to_<DST>[_<suffix>]`；"对端修饰词 + 存储层级"沿用既有 `copy_ubuf_to_neighbor_ubuf` 与 `copy_l1_to_peer_l1` 的同一构词法，只把对端修饰词换成 `noc`；`_align_b8` 后缀**原样保留**，用以宣告"复用的是既有 align 模式 DMA 的参数骨架与字节粒度" |
| `wait_noc_to_ubuf` | 动词打头（`wait`，与既有 `wait_flag` / `wait_flag_dev` 同族）+ 地址空间入名 + `_to_` 方向。`_to_` 在此描述**被感知的数据到达方向**，本指令自身不搬运字节——这一点由机器助记符与 §4.5 语义说死 |
| 不带 dtype 后缀（收） | 既有 `_b8/_b16/_b32` 后缀表示**元素位宽分派**；接收侧是纯字节计量，不按元素宽度分派指令，故不加 |
| 带 `_b8` 后缀（发） | `_b8` 不是"Calendar 按元素宽度分派"，而是**被复用的既有指令身份的一部分**。Calendar 只需要字节粒度那一条，因此**只扩展 `_b8`，不扩展 `_b16/_b32`** |
| `set_calendar_send_ctx` | `set_*` 是既有最大的一族，子规律 `set_<配置目标>_<参数名>`；写本核架构状态 ⇒ 裸名层 |

> **既有指令选型：为什么发包侧选 `copy_ubuf_to_gm_align_b8` 作为扩展基底**——Calendar 的发包动作是
> "把本核 UB 的一段（或二维一组）连续字节写到『对称地址 + 各成员落点』"。逐条筛选既有 `copy_*` 族后：
> ① 方向与流水都是既有的（MTE3 上"UB → 核外"的既有写出通路，不需要新增流水线级）；
> ② **地址域够宽**——它的目的操作数是**片外/全局地址槽**，是既有族里唯一天然放得下"对称 NoC 地址"的槽位
> （UB 内偏移槽放不下），这正是"只是地址做扩展"这句话的落点；
> ③ **参数骨架正好够用**——`align` 模式的 `(nBurst, lenBurst, srcGap, dstGap)` 与 Calendar 竞技场的二维几何
> 逐项对应，一条指令即可表达一整个 tile 的发送。
> 被排除的候选：块粒度 DMA（行字节数不是块整数倍时无法表达尾部）、`_b16/_b32`（不按元素宽度分派）、
> `_pad_*`（不需要 padding）、`scatter_*`（落点不是索引表）、`copy_ubuf_to_ubuf`（目的槽放不下对端寻址）、
> `copy_ubuf_to_neighbor_ubuf`（一跳邻居、单成员、无时隙概念）。

> **与点到点 / 矩形组核间通信接口的关系**：平台上另有 `copy_l1_to_peer_l1`（对端由单个 block id 给出）与
> `mov_l1_to_group`（对端由 64-bit 矩形描述符给出）。三者共用**同一套寻址范式**——"对称地址给 offset、
> 对端集合另给载体"，差别只在**对端集合怎么表达**：单元素 / 矩形 / **任意转发树 + 排程式时隙（`routeBits`）**。
> Calendar 的成员集合是 NoC 算法排出来的转发树，不能被矩形描述符表达，因此 Calendar 数据面**不是**
> 矩形组搬运的特例。

### 5.2 CCE 公共类型与枚举

#### 5.2.1 `calendar_op_t`：时隙寄存器索引（新增枚举）

```cpp
// 架构守卫：仅在提供 Calendar 平面的目标上可见
#if defined(__DAV_WSE__)
typedef enum {
    CAL_ILLEGAL       = 0,   // 保留：必须 fault。保证"全零头部"不会被当成合法包
    CAL_ALLGATHER     = 1,   // 首期实现：CalReg[1]
    CAL_REDUCE        = 2,   // 首期实现：CalReg[2]
    CAL_ALLREDUCE     = 3,
    CAL_REDUCESCATTER = 4,
    CAL_SCATTER       = 5,
    CAL_GATHER        = 6,
    CAL_RESERVED      = 7,   // 保留：必须 fault 或丢弃并上报
} calendar_op_t;
#endif
```

| 约束 | 内容 |
|---|---|
| **位宽** | **3 bit，必须一次留够**。首期即使只实现 `CAL_ALLGATHER` 与 `CAL_REDUCE`，编码宽度也不得按 2 bit 冻结 |
| **取值形态** | **编译期立即数**。由集合语义唯一决定，不是运行期变量 |
| **非法值** | `CAL_ILLEGAL` / `CAL_RESERVED` / 未安装内容的码位必须 fault，不得按默认项放行 |
| **兼作 `flagId`** | 同一个值直接充当对齐指令的 `flagId`（`1..6 ⊂ 0..15`），这是"对齐不新增指令"能成立的前提 |
| **与 PTO 层的对应** | 与 `pto::CalendarCollective` **逐值一一对应、数值相同** |

> **命名**：值前缀 `CAL_` 遵循既有枚举的"族前缀 + ALL_CAPS"惯例（`PIPE_*` / `DSB_*` / `ATOMIC_*`）；
> 类型名取 `<族>_op_t` 短式（对照既有 `atomic_op_t` 与矩形组通信的 `group_op_t`）。
> **本文一律把该操作数称为 `opcode`**，不引入 NoC 内部的时隙编号、授权、bank 或分组术语——
> 从核的角度它就是一次寄存器索引 `CalReg[opcode]`。

#### 5.2.2 `calendar_red_t`：归约算子（新增枚举）

```cpp
#if defined(__DAV_WSE__)
typedef enum {
    CAL_RED_NONE = 0,   // 非归约语义（AllGather / Gather / Scatter）时取此值，字段 dormant
    CAL_RED_SUM  = 1,
    CAL_RED_MAX  = 2,
    CAL_RED_MIN  = 3,
    CAL_RED_PROD = 4,
} calendar_red_t;
#endif
```

**归约元素类型**由既有 `vtype_t` 承载（不新增取值）：

```cpp
typedef enum { b8, b16, b32, s8, s32, f16, fmix, f32 } vtype_t;   // 复用既有枚举
```

归约类 `opcode` 落地时，flit 头在 `redOp` 之外增加 **3 bit 元素类型**（取 `vtype_t` 除 `fmix` 外的 7 个取值），
发包 facade 追加一个 `vtype_t dtype` 立即数操作数；`CAL_ALLGATHER` 等纯复制语义下 `redOp = CAL_RED_NONE`、
`dtype` 被忽略（传 `b8` 即可）。见 §6.3 待决项 C-3。

#### 5.2.3 `pipe_t`：**无需扩展**（对既有枚举零改动）

两条数据面指令要求"收"与"发"**独立派发、独立反压、独立 barrier**。现有 `pipe_t` 的取值已包含
`PIPE_MTE3`（发）与 `PIPE_MTE4`（收），因此：

- **不新增** `pipe_t` 取值，也不需要同步扩展 `PIPE_ALL` 的覆盖范围；
- `set_flag(src, PIPE_MTE3, id)` / `wait_flag(PIPE_MTE3, dst, id)` / `pipe_barrier(PIPE_MTE3)` 直接可用，
  `event_t` 仍用既有 `EVENT_ID0..7`。

> **`wait_mte4_completion()` 不是新增指令**：在 CCE 层映射为既有的 `pipe_barrier(PIPE_MTE4)`
> （或由调用方用 `set_flag(PIPE_MTE4, dst, id)` / `wait_flag` 对表达）。它能证明"远端写入本核的数据量满足
> 要求"的**前提**是 MTE4 命令自身带 `expVal`——这正是 `wait_noc_to_ubuf` 必须是一条独立机器指令的理由。

#### 5.2.4 计量操作数与地址空间约定

| 操作数 | 类型 | 语义 | 单位 |
|---|---|---|---|
| `expVal` | `uint32_t` | 本轮必须从**远端**收到的有效 payload 字节数；未达到前接收命令保持未完成 | **字节** |
| `capacity` | `uint32_t` | 本核接收竞技场 `[dst, dst + capacity)` 的字节容量 | **字节** |
| `epoch` | `uint32_t` | 本轮 collection epoch；接收记账的隔离键 | 计数 |

- **位宽收窄说明**：接收竞技场在 UB 内，容量与单轮期望字节数都远小于 4 GiB（FFN 示例为 10752 B / 36864 B），
  因此 CCE 侧按 `uint32_t` 给出。最终位宽由 ISA 评审冻结（§6.3 待决项 S-1）。
- **地址空间**：`sendTile` / `recvTile` 都是 UB tile（`__ubuf__ uint8_t*`）。所有指针一律带 `__<space>__`
  限定词，**禁用裸 `void*`**。若 Calendar 竞技场最终落在 L1，facade 名与签名里的 `ubuf` 词根需整体替换为
  `cbuf`（`copy_cbuf_to_noc_cbuf_align_b8` / `wait_noc_to_cbuf`）——见 §6.3 待决项 S-2；本文不混用 `ub`/`l1` 等同义异拼。
- **参数顺序**：遵循搬运类惯例——**dst 在前、src 在后**，随后是尺寸/步长，再是模式操作数，
  表达对端集合的操作数（`rbLo`/`rbHi`）与身份键（`epoch`）排在模式操作数之后。
  > ★ **与机器层写法的一处差异**：机器侧 `MTE3_NOC_SEND` 惯常按 `(src, dstSym, bytes, …)`（src 在前）描述。
  > 下沉到 CCE facade 时必须**交换前两个实参**，与被复用的既有搬运指令保持一致。
- **非架构操作数**：GM-mock 需要对端窗口基址才能模拟对称地址解析。本方案下 mock **可以直接从 `rbLo/rbHi`
  解出 `{L_i = 1}` 的落核集合**，窗口基址与行跨距由 facade 内部从已初始化的 mock 通道状态取得，
  **不进 intrinsic 签名**——这是位图随包携带的附带收益。

#### 5.2.5 编译期 vs 运行期操作数切分

| 操作数 | 形态 | 理由 |
|---|---|---|
| `opcode` / `redOp` / `keyId` / `routeVersion` | **编译期立即数**（非类型模板参） | 由集合语义与逻辑身份唯一决定；必须折成指令的立即数字段（禁令 **F2**） |
| `sid` / `alignMode0` / `alignMode1` | **编译期立即数**（恒 0） | Calendar 方向无意义 |
| `nBurst` / `lenBurst` / `srcGap` / `dstGap` / `capacity` / `expVal` / `selfOff` | 由 tile 类型与集合语义**编译期推导**，以运行期值形式传入 | 载荷几何来自 tile 类型；`selfOff` 是唯一逐核不同的量 |
| `rbLo` / `rbHi` | **运行期寄存器值**（按值传，不取址） | 来自两条普通 LD；40 核内容各不相同，只能是数据 |
| `blockId` | **运行期值** | 唯一的 B 类量 |
| `epoch` | **运行期值**（核内标量自增） | 每轮递增 |

### 5.3 CCE 控制面：对齐（+0 条新增）

```cpp
// 两条都是既有 CCE intrinsic，本文只规定其在 Calendar 协议中的用法。
AICORE inline void ffts_cross_core_sync(pipe_t pipe, uint64_t fftsMsg);   // 既有
AICORE inline void wait_flag_dev(uint64_t flagId);                        // 既有

// Calendar 用法（PTO 层 helper，不是新增 intrinsic）：
//   mode   = Calendar 全组对齐域（★需新增一个 mode 码位 CAL_GROUP_SYNC）
//   flagId = opcode（3 bit 编译期立即数）
ffts_cross_core_sync(PIPE_MTE3, calendar_ffts_msg(CAL_GROUP_SYNC, opcode));
wait_flag_dev(static_cast<uint64_t>(opcode));
```

| 操作数 | 取值 | 说明 |
|---|---|---|
| `pipe` | **`PIPE_MTE3`**（推荐）或 `PIPE_V` | 报到消息与本轮发包在同一条流水上，天然保证"对齐先于发送"（**O2**），不需要额外栅栏。副作用：报到会等本核既有 MTE3 命令（如同相 `TSTORE`）排空；若实测代价明显改用 `PIPE_V`（与 `sendTile` 生产者同流水）。Step1 的 `pipe_barrier(PIPE_ALL)` 已使 `sendTile` 对外可见，因此 pipe 的选择只影响顺序，不影响数据可见性 |
| `fftsMsg` | `calendar_ffts_msg(CAL_GROUP_SYNC, opcode)` | 既有消息编码 + **一个新的 mode 码位**，不是一条新指令 |
| `flagId` | `opcode`（1..6） | 把本核归入正确的 align 域，并确定该域在硬件内部预留的 arm lead time。**不携带任何表句柄、不携带 `routeBits`** |
| 返回值 | `void` / `void` | 两条都是发射型/阻塞型（CCE 惯例）。★ 由此 epoch 不能由对齐指令带回 |

**有限自旋诊断壳（非 ISA）**：既有 `try_wait`（取 `sync_mode_t`，返回 `int64_t`）可直接替换 `wait_flag_dev`
做**诊断构建**，定位"某成员永不报到"这类 bug；它**不是**新增 ISA，也**不在正确性路径上**：

```cpp
// 仅诊断构建启用；release 路径必须用 wait_flag_dev（正确性路径不接受有限自旋）
for (uint32_t s = 0; s < maxSpins; ++s)
    if (try_wait(CROSS_CORE, static_cast<uint64_t>(opcode)) != 0) break;
```

### 5.4 CCE 取数：`.rodata` 静态表（+0 条新增）

```cpp
// 不是新增 ISA：keyId 是编译期立即数（折进地址偏移），blockId 是唯一的运行期变量部分。
const uint32_t blockId = static_cast<uint32_t>(get_block_idx());      // 复用既有 SPR getter
const CalendarRouteEntry e = kCalendarRoute[K.keyId][blockId];        // 展开成两条 64 bit 标量 LD
```

通路、命中率、每核占用与一致性性质见 §3.4 与 §3.8；此处只补一条接口层注意事项：

> ⚠️ 传统 alias `get_blockid()` 与新入口 `get_block_idx()` 的 feature gate **不同**：
> 在本机工具链对某些既有目标的最小编译探针中，`get_blockid()` 被报
> `does not support the given target feature`，而 `get_block_idx()` 是各编程模型推荐的稳定入口。
> 因此本文一律写 `get_block_idx()`，并要求在真实 WSE 目标上做最小编译验证（§6.3 待决项 C-7）。

### 5.5 CCE 数据面（发）：`copy_ubuf_to_noc_ubuf_align_b8`【★+1】

**ISA 完整形态**（前 9 个形参逐位沿用既有操作数表，末尾追加 4 个 Calendar 操作数）：

```cpp
#if defined(__DAV_WSE__)
AICORE inline void copy_ubuf_to_noc_ubuf_align_b8(
    // ---- 以下 9 个形参逐位沿用既有 copy_ubuf_to_gm_align_b8 的操作数表 ----
    __ubuf__ void*        dst,          // 既有槽①：目的地址 → ★扩展为「对称 NoC 地址」
    __ubuf__ const void*  src,          // 既有槽②：本核 UB 行起点（32 B 对齐，见 R-1）
    uint8_t               sid,          // 既有槽③：service id；Calendar 方向无意义，恒 0
    uint16_t              nBurst,       // 既有槽④：行数（= rowCount，或 1）
    uint32_t              lenBurst,     // 既有槽⑤：每行有效字节数（= rowBytes）
    uint16_t              srcGap,       // 既有槽⑥：源行间隙
    uint32_t              dstGap,       // 既有槽⑦：目的行间隙
    uint32_t              alignMode0,   // 既有槽⑧：align 模式头/尾处理域；Calendar 恒 0
    uint32_t              alignMode1,   // 既有槽⑨：同上；Calendar 恒 0
    // ---- 以下 4 个是 Calendar 新增的架构操作数 ----
    uint64_t              rbLo,         // ★GPR：routeBits[63:0]
    uint64_t              rbHi,         // ★GPR：硬件只取 [15:0]，高 48 bit 必须忽略
    calendar_op_t         opcode,       // ★3 bit 立即数：索引 CalReg[opcode]
    calendar_red_t        redOp);       // ★3 bit 立即数：归约算子；非归约语义下 dormant
#endif
```

**便捷重载（facade 内联，不是另一条机器指令）**：

```cpp
#if defined(__DAV_WSE__)
// 逐行形态：注意 dst/src 已按 CCE 惯例相对机器层写法交换
AICORE inline void copy_ubuf_to_noc_ubuf(
    __ubuf__ void* dst, __ubuf__ const void* src, uint32_t bytes,
    uint64_t rbLo, uint64_t rbHi, calendar_op_t opcode, calendar_red_t redOp)
{
    copy_ubuf_to_noc_ubuf_align_b8(dst, src, /*sid=*/0, /*nBurst=*/1, /*lenBurst=*/bytes,
                                   /*srcGap=*/0, /*dstGap=*/0, 0, 0,
                                   rbLo, rbHi, opcode, redOp);
}
#endif
```

| 参数 | 类型 | Calendar 语义 |
|---|---|---|
| `dst` | `__ubuf__ void*` | **对称地址** = `recvSymBase + selfOff + r0 * recvRowStride`，写作本核地址；其核内 offset 即各成员接收 slot 的 offset。★这是唯一被扩展的既有槽（§4.4.2） |
| `src` | `__ubuf__ const void*` | 本核发送 tile 的起始行 = `sendUb + r0 * sendRowStride` |
| `sid` | `uint8_t` | 既有的 service / QoS 标识。**Calendar 方向无意义，必须传 0**；硬件应忽略，或对非零值 fault（§6.3 待决项 S-5） |
| `nBurst` | `uint16_t` | 本条指令发送的行数：`rowCount`（整 tile）或 `1`（逐行） |
| `lenBurst` | `uint32_t` | 每行有效字节数 = `rowBytes`。**单位为字节**（`_b8` 粒度） |
| `srcGap` | `uint16_t` | 源相邻行间隙，由 `sendRowStride - rowBytes` 推出（单位见 §6.3 S-1） |
| `dstGap` | `uint32_t` | 目的相邻行间隙，由 `recvRowStride - rowBytes` 推出 |
| `alignMode0/1` | `uint32_t` | 既有 align 模式的头/尾处理域；Calendar 恒 0 |
| `rbLo` / `rbHi` | `uint64_t` | **按值传入的寄存器操作数**，来自 §5.4 的两条 LD。同一 tile 的所有行共用同一份 ⇒ 循环内零额外访存 |
| `opcode` | `calendar_op_t` | 3 bit 立即数。与 `rbLo/rbHi` **必须来自同一个 `CalendarKeyRef`**（不变式 **O1**） |
| `redOp` | `calendar_red_t` | 3 bit 立即数。`CAL_ALLGATHER` 时取 `CAL_RED_NONE` |
| （归约落地时追加）`dtype` | `vtype_t` | 3 bit 立即数，归约的元素类型（§5.2.2，待决项 C-3） |

**备选方案 B 的 facade 形态**：

```cpp
#if defined(__DAV_WSE__)
AICORE inline void set_calendar_send_ctx(
    uint64_t rbLo, uint64_t rbHi, calendar_op_t opcode, calendar_red_t redOp);

// 随后的发包隐式使用当前 send context，参数表退回既有的 9 个槽
AICORE inline void copy_ubuf_to_noc_ubuf_align_b8(
    __ubuf__ void* dst, __ubuf__ const void* src, uint8_t sid,
    uint16_t nBurst, uint32_t lenBurst, uint16_t srcGap, uint32_t dstGap,
    uint32_t alignMode0, uint32_t alignMode1);
#endif
```

方案 B 下包装层在 Step4 之前插一条 `set_calendar_send_ctx` 并补一道栅栏，**调用点签名不变**。

### 5.6 CCE 数据面（收）：`wait_noc_to_ubuf`【★+1】

```cpp
#if defined(__DAV_WSE__)
AICORE inline void wait_noc_to_ubuf(
    __ubuf__ void*  dst,        // 本核接收竞技场基址（对称地址，全组同址）
    uint32_t        capacity,   // 接收范围字节容量 = recvRowCount * recvRowStride
    uint32_t        expVal,     // 本轮期望的远端有效 payload 字节数
    calendar_op_t   opcode,     // ★3 bit 立即数：判别键的一半，并建立本轮上下文（§4.5.1）
    uint32_t        epoch);     // 本轮 collectionEpoch，来自 CalendarNextEpoch(opcode)
#endif
```

语义与七条 `expVal` 契约见 §4.5；返回 `void`（CCE 中发射型指令一律返回 `void`）。

**参数顺序依据**：dst 在前、随后是尺寸/容量类操作数、再是模式操作数（`opcode`）、最后是身份键（`epoch`）——
与既有搬运类惯例及矩形组搬运"表达对端集合的操作数排在模式操作数之后"一致。

**`expVal` 的三方一致性校验（软件侧，零额外访存）**：

```text
用户传入的 expVal  ==  载荷几何推导值 rowCount * rowBytes * landCount  ==  表项 e.expectedRxBytes
```

`landCount` 与 `expectedRxBytes` 都随 `routeBits` 同一次 LD 免费带回（§2.6），因此这项校验零额外访存；
debug 构建下由包装层断言。

### 5.7 CCE 层调用序列

```cpp
// ---- 取数：不是 Calendar 步骤、也不是新增 ISA；可提前到 TSYNC 之前发射 ----
const uint32_t blockId = static_cast<uint32_t>(get_block_idx());
const CalendarRouteEntry e = kCalendarRoute[K.keyId][blockId];   // → 两条普通 64 bit 标量 LD
const uint64_t rbLo   = CalendarRbLo(e);     // 表项字节 0..7   ┐ 纯位运算 helper：
const uint64_t rbHi   = CalendarRbHi(e);     // 表项字节 8..15  ┘ 不是 intrinsic，不产生额外访存
const uint32_t expVal = e.expectedRxBytes;   // 与 routeBits 同一次 LD 免费带回

// ---- Step1：本核前序完成 + 发布 sendTile ----
wait_flag(/*...TSYNC 展开...*/);
pipe_barrier(PIPE_ALL);
dsb(DSB_DDR);

// ---- Step2：全组报到 / 汇合。☆两条都是既有 ISA，机器指令增量 +0 ----
ffts_cross_core_sync(PIPE_MTE3, calendar_ffts_msg(CAL_GROUP_SYNC, K.opcode));
wait_flag_dev(static_cast<uint64_t>(K.opcode));
const uint32_t epoch = CalendarNextEpoch(chan, K.opcode);        // 纯标量自增

// ---- Step3：先建立接收计量（阻塞 MTE4，不阻塞 AICORE 派发）----
wait_noc_to_ubuf(arena, capacity, expVal, K.opcode, epoch);

// ---- Step4：再入队发送；路由与时隙在这里进入数据面 ----
#if CALENDAR_SEND_MULTI_BURST              // nBurst = rowCount：整 tile 一条指令（§4.4.5）
copy_ubuf_to_noc_ubuf_align_b8(
    arena + selfOff, src, /*sid=*/0,
    /*nBurst=*/rowCount, /*lenBurst=*/rowBytes,
    /*srcGap=*/CalendarSrcGap(sendStride, rowBytes),
    /*dstGap=*/CalendarDstGap(recvStride, rowBytes), 0, 0,
    rbLo, rbHi, K.opcode, K.redOp);
#else                                      // nBurst = 1：逐行，支持逐行不同路由
for (uint32_t r = 0; r < rowCount; ++r)
    copy_ubuf_to_noc_ubuf(arena + selfOff + r * recvStride,   // dst 在前
                          src + r * sendStride,               // src 在后
                          rowBytes, rbLo, rbHi, K.opcode, K.redOp);
#endif

// ---- Step4s：本核段补写；由 flags.selfLand 决定归谁写，两者必须一致 ----
if constexpr (!CalendarPlatformTraits::FabricSelfDelivery) {
    for (uint32_t r = 0; r < rowCount; ++r)
        copy_ubuf_to_ubuf(arena + selfOff + r * recvStride, src + r * sendStride, rowBytes);
}

// ---- Step5：先排空发送，再等接收命令退休 ----
pipe_barrier(PIPE_MTE3);
pipe_barrier(PIPE_MTE4);
```

**每轮条数**：`ffts_cross_core_sync` × 1、`wait_flag_dev` × 1、`wait_noc_to_ubuf` × 1、
发包 × `1`（整 tile 形态）或 `rowCount`（逐行形态）、普通 `LD` × 2（同一条 64 B 行，第 2 条必命中）。

### 5.8 PTO 层公共类型

```cpp
namespace pto {

// ---- 集合语义与归约算子：与 CCE 侧 calendar_op_t / calendar_red_t 逐值一一对应、数值相同 ----
enum class CalendarCollective : uint8_t {
    Illegal = 0, AllGather = 1, Reduce = 2, AllReduce = 3,
    ReduceScatter = 4, Scatter = 5, Gather = 6, Reserved = 7,
};
enum class CalendarReduceOp : uint8_t { None = 0, Sum = 1, Max = 2, Min = 3, Prod = 4 };

// ---- .rodata 静态表项：16 B，正好被两条 64 bit LD 装进一个寄存器对 ----
// ★ 布局被寄存器对操作数约束，不得随意调整字段顺序。
struct alignas(16) CalendarRouteEntry {
    uint8_t  routeBits[10];   // 本核作为 source 时的 40 × {P,L}；bit(2i+1)=P_i, bit(2i)=L_i
    uint8_t  landCount;       // popcount(L)，本核这一发的落核数，供交叉校验
    uint8_t  flags;           // bit0 isMember  bit1 isRoot  bit2 selfLand(= FabricSelfDelivery)
    uint32_t expectedRxBytes; // 本核作为 destination 时本轮应收的远端有效字节数（= expVal）
};
static_assert(sizeof(CalendarRouteEntry) == 16, "entry must fit one GPR pair");

inline constexpr uint32_t kNocNodeCount        = 40;
inline constexpr uint32_t kCalendarKeyMax      = 16;
inline constexpr uint32_t kCalendarOpcodeCount = 8;   // opcode 码位数（3 bit；0 与 7 保留）

// ---- 取数结果：按【值】持有的寄存器对，这是发包指令真正吃的操作数形态 ----
// ★ 只做位运算，不再触碰表项地址；landCount / flags / expectedRxBytes 是 rbHi 高 48 bit 的免费搭车。
struct CalendarRouteRegs {
    uint64_t rbLo;   // 表项字节 0..7  → node 00..31 的 32 个位对
    uint64_t rbHi;   // 表项字节 8..15 → [15:0] node 32..39 的位对（硬件只取这 16 bit）
                     //                  [23:16] landCount  [31:24] flags  [63:32] expectedRxBytes

    AICORE uint32_t landCount()       const { return static_cast<uint32_t>((rbHi >> 16) & 0xFFu); }
    AICORE uint32_t flags()           const { return static_cast<uint32_t>((rbHi >> 24) & 0xFFu); }
    AICORE uint32_t expectedRxBytes() const { return static_cast<uint32_t>(rbHi >> 32); }
    AICORE bool     isMember()        const { return (flags() & 0x01u) != 0; }
    AICORE bool     selfLand()        const { return (flags() & 0x04u) != 0; }
};

// ---- 编译期常量对：keyId 与 opcode 必须【一起】传，这是不变式 O1 的类型系统落地 ----
// ★ 只能按值传或作非类型模板参；一旦被取址就掉进 .rodata、opcode 不再是立即数（禁令 F2）。
struct CalendarKeyRef {
    uint16_t           keyId;
    CalendarCollective opcode;       // 索引 CalReg[opcode]，并兼作对齐信号量 flagId
    CalendarReduceOp   redOp;        // AllGather 时为 None
    uint32_t           routeVersion; // 与 calendarVersion / topologyVersion 联合校验
};

// ---- 编译期发射的静态表：40 行【全部】写死，"我是谁"退化成一个运行期下标 blockId ----
// ★ 必须 alignas(64)；程序内不得有任何 ST 指向它（禁令 F3）。
// ★ 表在 DDR 里全片只有一份：40 行与 1 行的 DDR 成本相同，差别只在 D-Cache 驻留，
//   而驻留量由【被触碰的 64 B 行数】决定 —— blockId 是每核常量 ⇒ 每核每 key 只触碰 1 条行。
extern "C" alignas(64) extern const CalendarRouteEntry
    kCalendarRoute[/*keyCount*/][kNocNodeCount];   // ← 落在 .rodata

// ---- CalendarChannel ----
// ★ 必须是 kernel 内的局部对象（栈/寄存器），【不得】声明为全局或静态可写对象：
//   SPMD 下可写全局是全片共享的（禁令 F1），epochCtr 会被 40 核互相踩。
struct CalendarChannel {
    const CalendarRouteEntry (*routeTable)[kNocNodeCount]; // 指向 .rodata 表；链接期符号，非入参
    uint32_t blockId;                          // 本核 NoC 节点号；来自 get_block_idx()
    uint32_t epoch;                            // 本轮 collectionEpoch（由 CalendarNextEpoch 写入）
    uint32_t epochCtr[kCalendarOpcodeCount];   // ★每个 opcode 域一个核内轮次计数器，初值全 0

    static constexpr pipe_t RecvPipe = PIPE_MTE4;   // 只做 expVal 感知，不搬运数据
    static constexpr pipe_t SendPipe = PIPE_MTE3;   // ★既有的核间写流水，不新增流水线
    static constexpr pipe_t Pipe     = PIPE_MTE4;   // 返回时以最终接收完成为准
    static constexpr pipe_t SyncPipe = PIPE_MTE3;   // ★报到消息由哪条流水发出（§5.3）
};

// ---- 对齐与 epoch 的 PTO helper（都不是 intrinsic）----
AICORE inline uint64_t CalendarFftsMsg(CalendarCollective opcode);   // = msg(CAL_GROUP_SYNC, opcode)

AICORE inline uint32_t CalendarNextEpoch(CalendarChannel& chan, CalendarCollective opcode)
{
    return ++chan.epochCtr[static_cast<uint32_t>(opcode)];   // 从 1 起算；0 永不作为合法 epoch
}

// ---- 几何换算 helper：srcGap/dstGap 的单位随 ISA 手册确定，PTO 接口不受影响 ----
AICORE inline uint16_t CalendarSrcGap(uint32_t sendRowStride, uint32_t rowBytes);
AICORE inline uint32_t CalendarDstGap(uint32_t recvRowStride, uint32_t rowBytes);

} // namespace pto
```

### 5.9 PTO 接口：`TCalendar<K>`

```cpp
// 取数 helper：展开成两条普通标量 LD，不是新增 ISA，也不是独立的 Calendar 步骤。
// keyId 是编译期立即数 ⇒ 折进地址偏移；blockId 是唯一的运行期变量部分。
// 命中即返回；miss 由 D-Cache 硬件经 Batcher.mem（再 miss 回 DDR）整行回填，对用户透明。
template <CalendarKeyRef K>
AICORE inline CalendarRouteRegs GRID_TCALENDAR_SU_ROUTE_LOAD(const CalendarChannel& chan)
{
    CheckCalendarBlockIdInRange(chan.blockId, kNocNodeCount);   // 软件预检；硬件仍须自行 fault

    // ★取【值】不取址：两条 64 bit LD 把表项直接落进寄存器对，之后再不触碰表项地址（禁令 F3）。
    const uint64_t* p = reinterpret_cast<const uint64_t*>(&chan.routeTable[K.keyId][chan.blockId]);
    const CalendarRouteRegs rr{ p[0], p[1] };

    CheckCalendarRouteRegs(rr, K);   // 位对合法 / P_self=1 / isMember / routeVersion（debug 构建）
    return rr;
}

template <CalendarKeyRef K, typename TileProd, typename TileCons, typename... WaitEvents>
PTO_INST RecordEvent TCalendar(
    CalendarChannel& chan,
    uint64_t expVal,
    uint32_t selfOff,
    TileProd& sendTile,
    TileCons& recvTile,
    WaitEvents&... events)
{
#if defined(PTO_NPU_ARCH_A2A3)
    // 先把 routeBits 取进 GPR 对，让可能的 D-Cache miss 延迟与下面的"等数据"重叠。
    const CalendarRouteRegs rr = GRID_TCALENDAR_SU_ROUTE_LOAD<K>(chan);
    TSYNC(events...);                       // Step1 前半：本核前置依赖
    GRID_TCALENDAR_IMPL<K, TileProd, TileCons>(chan, rr, expVal, selfOff, sendTile, recvTile);
#else
    static_assert(sizeof(TileProd) == 0,
                  "TCalendar is unsupported on this target; silent GM fallback is forbidden.");
#endif
    return {};
}
```

包装层的固定执行形态是 **`SU_ROUTE_LOAD → TSYNC → IMPL`**；取数位于 `TSYNC` 之前，理由见 §1.7。

> **`K` 为什么是非类型模板参而不是函数形参**：作模板参时，`K.opcode` 在实例化后必然是编译期常量，
> 能直接折成报到消息的 `flagId` 与发包指令的 `opcode` 立即数字段；作普通形参时编译器可能被迫把它物化进
> `.rodata`，`opcode` 就从 I-Cache 通道掉到 D-Cache 通道（禁令 **F2**），`keyId` 与 `opcode` 的同源保证
> （不变式 **O1**）也随之失去类型系统的支撑。

**参数表**：

| 参数 | 方向 | 语义 |
|---|---|---|
| `K`（模板参） | 编译期 | `CalendarKeyRef{keyId, opcode, redOp, routeVersion}`；**不可拆分**，调用点不得分别传入，也不得取址 |
| `chan` | in/out | `.rodata` 表指针（链接期符号）、`blockId`、本轮 `epoch` 与每 `opcode` 域的轮次计数器 |
| `expVal` | in | 本核本轮期望接收的远端有效 payload 字节数；可显式指定，但必须与载荷几何推导值、表项 `expectedRxBytes` 三方一致 |
| `selfOff` | in | 本源 shard 在所有目标竞技场每一行内的字节偏移 |
| `sendTile` | in | 本核发送 UB tile；派生行数、有效行字节与源行跨距。★源行起点须 32 B 对齐（R-1） |
| `recvTile` | out | 本核对称 UB 竞技场；派生目标对称地址与目标行跨距 |
| `events...` | in | 本核前置依赖，由 `TSYNC` 展开 |
| 返回值 | out | `RecordEvent`；跨核完成已在指令体内闭合，返回值只连接本核后继 pipe |

> **调用点的运行期操作数只有两个**（`expVal` 与 `selfOff`），其余全部上移成编译期模板参或链接期符号。

**完成语义**：`TCalendar` 返回即表示本轮外来有效载荷已达到 `expVal` 并对本核后继可读。
指令体内**不发** `set_flag`；`set/wait` 两半都由 `Event<SrcOp, DstOp>` 框架产生，模板参决定 dstPipe、
`EventIdCounter` 决定 token——调用方不手写 flag、不硬编码 `EVENT_ID0`：

```cpp
Event<Op::TCALENDAR, Op::TSTORE_VEC> evColl = TCalendar<K>(/* ... */);
TSTORE(dstGm, recv, evColl);     // 由调用方发 set_flag 的另一半
```

**下沉伪码**：

```cpp
template <CalendarKeyRef K, typename TileProd, typename TileCons>
AICORE void GRID_TCALENDAR_IMPL(
    CalendarChannel& chan,
    const CalendarRouteRegs& rr,     // 已由两条普通 LD 取回，按值持有
    uint64_t expVal, uint32_t selfOff,
    TileProd& sendTile, TileCons& recvTile)
{
    const uint32_t rowCount     = a2a3_grid_payload::TileRowCount<TileProd>();
    const uint32_t recvRowCount = a2a3_grid_payload::TileRowCount<TileCons>();
    const uint32_t rowBytes     = a2a3_grid_payload::TileRowBytes<TileProd>();
    const uint32_t sendStride   = a2a3_grid_payload::TileRowStride<TileProd>();
    const uint32_t recvStride   = a2a3_grid_payload::TileRowStride<TileCons>();
    auto* src   = reinterpret_cast<__ubuf__ uint8_t*>(a2a3_grid_payload::TileUbPtr<TileProd>(sendTile));
    auto* arena = reinterpret_cast<__ubuf__ uint8_t*>(a2a3_grid_payload::TileUbPtr<TileCons>(recvTile));
    const uint32_t capacity = recvRowCount * recvStride;

    CheckCalendarGeometry(K, selfOff, rowCount, recvRowCount, rowBytes, recvStride, rr);
    // expVal 三方一致：用户传入值 == 载荷几何推导值 == .rodata 表项里 NoC 算法的输出值
    CheckCalendarExpVal(expVal,
                        static_cast<uint64_t>(rowCount) * rowBytes * rr.landCount(),
                        rr.expectedRxBytes(), capacity);

    // ---- Step1 后半：前序完成 + 发布 sendTile ----
    // ★ 这道栅栏【不】承担"配置对 router / scheduler / MTE4 ingress 可见"的职责，
    //   routeBits 只活在 GPR 与 flit 头里，没有第二个可见性面。
#ifndef __PTO_AUTO__
    pipe_barrier(PIPE_ALL);
#endif
    dsb(DSB_DDR);

    // ---- Step2：全组报到 / 汇合。★复用既有 ISA，机器指令增量 +0 ----
    // K.opcode 是编译期立即数，直接充当 flagId 编进指令的 3 bit 字段。
    ffts_cross_core_sync(CalendarChannel::SyncPipe, CalendarFftsMsg(K.opcode));
    wait_flag_dev(static_cast<uint64_t>(K.opcode));
    chan.epoch = CalendarNextEpoch(chan, K.opcode);     // 纯标量自增；对齐指令无返回值

    if constexpr (K.opcode == CalendarCollective::AllGather) {
        // ---- Step3：MTE4 建立接收并按实际有效字节阻塞完成（只阻塞 PIPE_MTE4，不变式 O3）----
        wait_noc_to_ubuf(arena, capacity, static_cast<uint32_t>(expVal), K.opcode, chan.epoch);

        // ---- Step4：MTE3 入队；由 NoC 按 CalReg[opcode] 放行写出 ----
        // ★ 本方案的核心：routeBits 与 opcode 在【这里】进入数据面。
#if CALENDAR_SEND_MULTI_BURST
        copy_ubuf_to_noc_ubuf_align_b8(
            arena + selfOff, src, /*sid=*/0,
            /*nBurst=*/static_cast<uint16_t>(rowCount), /*lenBurst=*/rowBytes,
            CalendarSrcGap(sendStride, rowBytes), CalendarDstGap(recvStride, rowBytes), 0, 0,
            rr.rbLo, rr.rbHi, K.opcode, K.redOp);
#else
        for (uint32_t r = 0; r < rowCount; ++r)
            copy_ubuf_to_noc_ubuf(arena + selfOff + r * recvStride, src + r * sendStride,
                                  rowBytes, rr.rbLo, rr.rbHi, K.opcode, K.redOp);
#endif

        // ---- Step4s：本核段。由 rr.selfLand() 决定归谁写，两者必须一致（编译校验 selfLand）----
        if constexpr (!CalendarPlatformTraits::FabricSelfDelivery) {
            for (uint32_t r = 0; r < rowCount; ++r)
                copy_ubuf_to_ubuf(arena + selfOff + r * recvStride,
                                  src + r * sendStride, rowBytes);   // 本地字节不计入 expVal
        }

        // ---- Step5：先排空 MTE3，再等待 Step3 的 MTE4 命令退休 ----
#ifndef __PTO_AUTO__
        pipe_barrier(PIPE_MTE3);
#endif
        pipe_barrier(PIPE_MTE4);
    } else {
        CalendarUnsupportedOpcodeFault(K.opcode);
    }

    // ---- Step6：不发 set_flag；由调用方 Event<Op::TCALENDAR, Op::Y> 产生 ----
}
```

### 5.10 调用示例：32 核 FFN 的两段 AllGather

32 核（4 × 8）SPMD FFN：phase B 沿 ROW 组收齐 8 份 `[8,96]` shard，phase C 沿 COL 组收齐 4 份 `[8,768]` row block。
**两相分开 launch，host 在相间插 barrier。**

```cpp
constexpr int kT        = 8;      // token tile（每 cell 的有效行数）
constexpr int kIShard   = 96;     // 每 cell 的 I shard
constexpr int kRowBlock = 768;    // phase B 收齐宽 = gridCols * kIShard
constexpr int kIfull    = 3072;   // 全 I
constexpr int kGridRows = 4;
constexpr int kGridCols = 8;

using HiddenShardTile = Tile<TileType::Vec, half, kT, kIShard,   BLayout::RowMajor>; // [8,96]
using RowBlockTile    = Tile<TileType::Vec, half, kT, kRowBlock, BLayout::RowMajor>; // [8,768]
using HiddenFullTile  = Tile<TileType::Vec, half, kT, kIfull,    BLayout::RowMajor>; // [8,3072]
```

```cpp
// =====================================================================
// ★★ 编译期发射的 .rodata 静态表 ★★
//   40 节点排成 5×8 mesh，node = r*8 + c；FFN 用前 4 行共 32 个 cell，node 32..39 位对恒为 00。
//   2 个 key × 40 行 × 16 B = 1280 B，随 kernel binary 一次 H2D 到 DDR —— ★全片只有一份，
//   不逐 cell 复制；各核 I$/D$ 由 Batcher.mem 统一供给。
//   ★ blockId 是每核常量 ⇒ 本核只会命名第 blockId 行的地址，另外 39 行永不进 D-Cache；
//     每核每 key 只触碰 1 条 64 B 行 ⇒ 每核 D-Cache 驻留 2 条行 = 128 B。
// =====================================================================
alignas(64) const CalendarRouteEntry kCalendarRoute[2][kNocNodeCount] = {
  // ---- keyId 0 = {AllGather, ROW(my_row)}：ROW 组 = {r*8+0 .. r*8+7} ----
  //   source = node 0：P = {N00..N07}，L = {N01..N07}（自己不落核，本地 UB→UB 补）
  { {{0xFE,0xFF,0,0,0,0,0,0,0,0}, /*landCount=*/7, /*flags=*/0x01, /*expectedRxBytes=*/10752},
    /* node 01..07：同一 ROW 组，L 位换成"除自己外的 7 个" */
    /* node 08..31：各自所在 ROW 组的位图 */
    /* node 32..39：全 00，landCount = 0，flags = 0x00（非成员） */ },

  // ---- keyId 1 = {AllGather, COL(my_col)}：COL 组 = {c, c+8, c+16, c+24} ----
  //   source = node 0：P = {N00,N08,N16,N24}，L = {N08,N16,N24}
  { {{0x02,0x00,0x03,0x00,0x03,0x00,0x03,0x00,0,0}, /*landCount=*/3, /*flags=*/0x01,
     /*expectedRxBytes=*/36864},
    /* node 01..31：各自所在 COL 组的位图；node 32..39：全 00 */ },
};

// ★ 编译期常量对：keyId 与 opcode 一起走，调用点不得拆开、不得取址（不变式 O1 / 禁令 F2）
inline constexpr CalendarKeyRef kRowAllGather{
    0, CalendarCollective::AllGather, CalendarReduceOp::None, ROUTE_VERSION };
inline constexpr CalendarKeyRef kColAllGather{
    1, CalendarCollective::AllGather, CalendarReduceOp::None, ROUTE_VERSION };
// ★ 两个 key 的 opcode 相同 ⇒ 共用 CalReg[AllGather]，也共用同一个对齐信号量域（flagId = 1）。
//   两相分开 launch、任一时刻只有一种流量在飞 ⇒ 同时满足"同 opcode 数据面不并发"与"对齐不并发"。
```

```cpp
__global__ AICORE void FfnCalendarAllGatherKernel(
    /* ... 数据缓冲若干 ... */
    int phase, int row, int col, int gridRows, int gridCols)
{
    using namespace pto;

    // ===== 唯一的 CalendarChannel =====
    //   routeTable 直接指向 .rodata 的链接期符号，【不是】kernel 入参；
    //   blockId 优先取 SPR（0 次访存），退路是随 kernel 入参下发（取数变成两级依赖）；
    //   epochCtr 全 0 初始化 ⇒ 本 launch 第一轮 epoch = 1。
    //   ★ chan 必须是局部对象：SPMD 下可写全局是全片共享的（禁令 F1）。
    CalendarChannel chan{ /*routeTable=*/kCalendarRoute,
                          /*blockId=*/  static_cast<uint32_t>(get_block_idx()),
                          /*epoch=*/    0,
                          /*epochCtr=*/ {} };

    const uint32_t rowSelfOff = static_cast<uint32_t>(col) * kIShard   * sizeof(half); // col * 192 B
    const uint32_t colSelfOff = static_cast<uint32_t>(row) * kRowBlock * sizeof(half); // row * 1536 B

    // ================= phase B：ROW 组 8 路 AllGather =================
    if (phase == 1) {
        if constexpr (DAV_VEC) {                    // ★ 通信相是 vec 相，守卫不可省
            HiddenShardTile send;
            RowBlockTile    recv;
            TASSIGN(send, 0x0000);
            TASSIGN(recv, 0x2000);                  // ★ 全组对称地址（不变式 A1）

            Event<Op::TLOAD, Op::TCALENDAR> evLoad =
                TLOAD(send, GlobalTensor<half, Shape<1,1,1,kT,kIShard>,
                                         Stride<kT*kIShard,kT*kIShard,kT*kIShard,kIShard,1>>(hiddenShard));

            // 取数   两条普通 LD：rr = kCalendarRoute[0][blockId]（非独立步骤）
            //          → rbLo = 0x…FFFE（cell(0,0) 时），landCount = 7, expectedRxBytes = 10752
            //          → 首次 miss：MSHR → BIU → Batcher.mem（再 miss 回 DDR）→ 64 B 整行回填
            // Step1  pipe_barrier(PIPE_ALL) + dsb（不承担"配置对 router 可见"）
            // Step2  ffts_cross_core_sync(PIPE_MTE3, msg(CAL_GROUP_SYNC, AllGather))
            //          + wait_flag_dev(1) + epoch = 1      ← ☆两条既有 ISA，控制面新增 0 条
            //        ★本相有 4 个 ROW 组共 32 核落在同一个 flagId=1 的对齐域上 ⇒ 信号量位宽须 ≥ 6 bit
            // Step3  wait_noc_to_ubuf(arena, capacity = 8×1536 = 12288 B,
            //                         expVal = 8×192×7 = 10752 B, opcode = AllGather, epoch = 1)
            // Step4  copy_ubuf_to_noc_ubuf_align_b8(arena + col*192, send,
            //                         sid=0, nBurst=8, lenBurst=192,
            //                         srcGap=0, dstGap=1536-192=1344, 0, 0,
            //                         rbLo, rbHi, AllGather, None)     ← ★整 tile 一条
            // Step4s copy_ubuf_to_ubuf × 8（fabric 不 self-delivery 时；本地字节不计入 expVal）
            // Step5  pipe_barrier(PIPE_MTE3) → pipe_barrier(PIPE_MTE4)
            Event<Op::TCALENDAR, Op::TSTORE_VEC> evColl =
                TCalendar<kRowAllGather>(
                    chan,
                    /*expVal=*/ static_cast<uint64_t>(kT) * kIShard * sizeof(half) * (kGridCols - 1),
                    /*selfOff=*/rowSelfOff, send, recv, evLoad);

            TSTORE(GlobalTensor<half, Shape<1,1,1,kT,kRowBlock>,
                                Stride<kT*kRowBlock,kT*kRowBlock,kT*kRowBlock,kRowBlock,1>>(rowBlock),
                   recv, evColl);
            pipe_barrier(PIPE_ALL);  dsb(DSB_DDR);
        }
        return;
    }

    // ================= phase C：COL 组 4 路 AllGather =================
    if (phase == 2) {
        if constexpr (DAV_VEC) {
            RowBlockTile   send;
            HiddenFullTile recv;
            TASSIGN(send, 0x0000);
            TASSIGN(recv, 0x4000);                  // 12 KB send 与 48 KB 竞技场不重叠

            Event<Op::TLOAD, Op::TCALENDAR> evLoad =
                TLOAD(send, GlobalTensor<half, Shape<1,1,1,kT,kRowBlock>,
                                         Stride<kT*kRowBlock,kT*kRowBlock,kT*kRowBlock,kRowBlock,1>>(rowBlock));

            // ★ phase C 换的是【keyId】，不是任何硬件配置：
            //     取数 rr = kCalendarRoute[1][blockId] —— 另一条 64 B 行，本相首次访问会 miss 一次
            //     opcode 仍是 AllGather ⇒ 与 phase B 共用同一项 CalReg 与同一个对齐信号量域；
            //     两相分开 launch ⇒ 任一时刻只有一种流量在飞。
            //   ★ epochCtr 随 launch 重置 ⇒ 本相的 epoch 也是 1，与 phase B 相同。
            //     跨相隔离因此【完全依赖】"launch 边界前全片 Calendar 流量已排空"（host 相间 barrier），
            //     而不依赖 epoch 数值区分两相。
            // Step3  wait_noc_to_ubuf(arena, capacity = 8×6144 = 49152 B,
            //                         expVal = 8×1536×3 = 36864 B, AllGather, epoch = 1)
            // Step4  copy_ubuf_to_noc_ubuf_align_b8(arena + row*1536, send,
            //                         0, nBurst=8, lenBurst=1536,
            //                         srcGap=0, dstGap=6144-1536=4608, 0, 0,
            //                         rbLo, rbHi, AllGather, None)
            Event<Op::TCALENDAR, Op::TSTORE_VEC> evColl =
                TCalendar<kColAllGather>(
                    chan,
                    /*expVal=*/ static_cast<uint64_t>(kT) * kRowBlock * sizeof(half) * (kGridRows - 1),
                    /*selfOff=*/colSelfOff, send, recv, evLoad);

            TSTORE(GlobalTensor<half, Shape<1,1,1,kT,kIfull>,
                                Stride<kT*kIfull,kT*kIfull,kT*kIfull,kIfull,1>>(hiddenFull),
                   recv, evColl);
            pipe_barrier(PIPE_ALL);  dsb(DSB_DDR);
        }
        return;
    }
}
```

> ★ **示例里两处与 `PIPE_MTE3` 复用相关的提醒**：① Step2 的报到消息由 `PIPE_MTE3` 发出，会等本核既有
> MTE3 命令排空——本例中 `TCalendar` 之前没有 MTE3 命令，代价为零；② Step5 的 `pipe_barrier(PIPE_MTE3)`
> 与随后的 `TSTORE`（也在 MTE3 上）是程序序，无竞争，但该栅栏会连带排空同相其他 store-out 命令。

### 5.11 复用的既有能力与最终接口面

**复用清单**（每一条都是既有 intrinsic，本文只规定其在 Calendar 协议中的用法）：

| 接口 | 用途 | 出现位置 |
|---|---|---|
| `get_block_idx()` | 取本核 NoC 节点号，索引 `.rodata` 静态表 | 取数 |
| `ffts_cross_core_sync(pipe, msg)` | 向 BSP 报到（机器助记符 `SET_CROSS_CORE`） | Step2 |
| `wait_flag_dev(flagId)` | 阻塞到全组到齐 | Step2 |
| `try_wait(CROSS_CORE, …)` | 对齐的有限自旋诊断壳（**非正确性路径**） | 诊断构建 |
| `pipe_barrier(PIPE_ALL)` | 前序指令完成 + 发布 `sendTile`。★**不**承担"配置可见"这层含义 | Step1 |
| `dsb(DSB_DDR)` | 建立 data-before-ready 顺序 | Step1 |
| `pipe_barrier(PIPE_MTE3)` | 排空本核对外发送（**O3**）；注意会连带排空同相 `TSTORE` | Step5 |
| `pipe_barrier(PIPE_MTE4)` | 等待接收命令退休（即 `wait_mte4_completion()`） | Step5 |
| `set_flag` / `wait_flag` | 本核前置依赖（`TSYNC`）与后继 pipe 依赖；**外加**标量 → `PIPE_MTE3` 对 `rbLo/rbHi` 的 RAW 依赖（**O4**） | Step1 / Step6 |
| `copy_ubuf_to_ubuf` | fabric 不 self-delivery 时补写本核段；**本地字节不计入 `expVal`** | Step4s |
| `dcci` / `dci` | 算子尾部对 `.rodata` 表所在行**只需 invalidate**（表永不 dirty） | 算子边界 |

**最终接口面**：

```text
控制面（0 条新增，全部复用既有 CCE intrinsic）
├── ffts_cross_core_sync(PIPE_MTE3, msg(CAL_GROUP_SYNC, opcode))   // SET_CROSS_CORE，裸名层
│                                                                  //   向 BSP 报到；opcode 充当 flagId
├── wait_flag_dev(opcode)                                          // WAIT_FLAG_DEV，裸名层
└── CalendarNextEpoch(chan, opcode)                                // 纯标量自增，不是 intrinsic

取路由位图（0 条新增）
└── get_block_idx() + 两条普通 64 bit 标量 LD                       // .rodata → D-Cache → GPR 对
                                                                   //   miss：Batcher.mem → DDR → 64 B 行回填
                                                                   //   不经 local DRAM、不经 L1

数据面（2 条新增）
├── wait_noc_to_ubuf(dst, capacity, expVal, opcode, epoch)         // ★MTE4_NOC_RECV_WAIT，裸名层
│                                                                  //   只感知不搬运；判别键 {opcode, epoch, dstRange}
│                                                                  //   并建立本轮 {opcode, epoch} 上下文
└── copy_ubuf_to_noc_ubuf_align_b8(                                // ★扩展既有 UB→核外 align DMA，裸名层
        dst, src, sid, nBurst, lenBurst, srcGap, dstGap,           //   ↑ 9 个既有槽逐位沿用
        alignMode0, alignMode1,                                    //     （槽① 地址域 → 对称 NoC 地址）
        rbLo, rbHi, opcode, redOp)                                 //   ↑ 4 个 Calendar 新增操作数
    ├── 便捷重载 copy_ubuf_to_noc_ubuf(dst, src, bytes, rbLo, rbHi, opcode, redOp)  // nBurst = 1
    └── 归约类 opcode 落地时追加 dtype: vtype_t（待决项 C-3）

备选（方案 B，仅当 SEND 编码槽放不下）
└── set_calendar_send_ctx(rbLo, rbHi, opcode, redOp)               // CALENDAR_SEND_CTX_SET，裸名层

新增枚举
├── calendar_op_t   CAL_ILLEGAL / ALLGATHER / REDUCE / ALLREDUCE / REDUCESCATTER
│                   / SCATTER / GATHER / RESERVED        （3 bit，0 与 7 保留）
├── calendar_red_t  CAL_RED_NONE / SUM / MAX / MIN / PROD（3 bit）
└── vtype_t         复用，零改动；归约类语义的元素类型（待决项 C-3）

需要 CCE 侧改动的既有资产（共三处，均不新增指令）
1) UB→核外 align 字节粒度 DMA → 新增同族成员 copy_ubuf_to_noc_ubuf_align_b8
                                （槽① 地址域扩展 + 末尾 4 个操作数）
2) 跨核同步消息编码            → 新增一个 Calendar 全组 mode 码位（CAL_GROUP_SYNC）
3) 跨核同步信号量位宽/语义      → 必须容得下该 opcode 域的报到核数（★X-3）
   —— pipe_t 零改动（PIPE_MTE3 / PIPE_MTE4 都已存在）
```

### 5.12 使用约束与合规检查

**通用约束**

- `TCalendar<K>` 只在 `PTO_NPU_ARCH_A2A3` 下实现；其他 profile 触发编译期不支持错误，
  **不提供静默的 GM fallback**。
- 组内全部成员必须给出一致的集合语义、载荷几何、对称 `recvTile` 偏移与本轮身份；`selfOff` 必须不重叠地
  铺满竞技场每一行（**A1 / A3**）。
- 同步集合通信要求整组成员都能被调度；host 分 wave 时不能拆开正在同步的组。
- mix kernel 中通信相是 vector 相，必须加 `if constexpr (DAV_VEC)` 守卫。否则同一 cell 的 cube 与 vector
  子核会**各发一次**，发包与接收字节守恒双双破坏。
- `expVal` 由用户传入时仍须与载荷几何推导值、表项 `expectedRxBytes` 三方一致；不一致必须 fault。
- 完成链不使用 `notify` / write-with-notify / `NOTIFY_GROUP` / `WAIT_SPR`；本地自拷贝的字节永远不计入 `expVal`。
- `PIPE_MTE4` 与 `PIPE_MTE3` 必须能独立前进；接收命令的阻塞（= 该流水命令不退休）不得阻止 AICORE 继续
  向发送流水派发。
- `sendTile` 的行起点必须 **32 B 对齐**（R-1，正确性）；每行字节数是链路位宽整数倍仍只是效率建议。

**本方案专有约束**

- `CalendarKeyRef` 必须整体传入、必须按值或作非类型模板参、**不得取址**；`rbLo/rbHi` 必须**按值传**。
- `CalendarChannel` 必须是 kernel 内局部对象，不得是可写全局 / 静态对象（禁令 **F1**）。
- 同一 `opcode` 的集合不得并发在飞，**也不得并发汇合**；不满足时必须重新分相。
- 同一 `opcode` 域内成员核对 `TCalendar<K>` 的调用次数与顺序必须一致（不变式 **E1**）。
- launch 边界前必须保证全片 Calendar 流量已排空——`epochCtr` 随 launch 重置，跨 launch 不靠 epoch 区分。
- `kCalendarRoute` 必须落在 `.rodata`、`alignas(64)`、大小恰为 `keyCount × 40 × 16 B`，且
  `keyCount ≤ kCalendarKeyMax`；程序内不得有任何 ST 指向它（禁令 **F3**）。
- `blockId` 必须满足 `blockId < kNocNodeCount`。
- `routeBits` 的 `{P_i = 1}` 诱导子图必须连通、含 source、无环；`{L_i = 1}` 必须与集合语义的目的核集合一致，
  且 `landCount == popcount(L)`；source 自身的 `L` 位必须与 `CalendarPlatformTraits::FabricSelfDelivery` 一致。
- `CalReg` 的写入只能发生在装载期的停机窗口内、原子完成，且写入期间不得有 Calendar 流量在飞；kernel 期只读。
- **三版本联合校验**：`topologyVersion` / `calendarVersion` / `routeVersion` 必须匹配，任一不匹配在装载期 fault。

**合规检查（对照既有命名与参数惯例）**

| 检查项 | 本方案的落实 |
|---|---|
| **命名**：全小写 `snake_case`、动词打头、地址空间入名、方向用 `_to_` | ✅ `copy_ubuf_to_noc_ubuf_align_b8`、`wait_noc_to_ubuf`、`set_calendar_send_ctx`；复用的两条对齐指令本身就是既有名 |
| **同义异拼不混用** | ✅ 统一 `ubuf`，不混入 `ub`；统一 `noc` 作对端修饰词。若端点改 L1 则整体换 `cbuf`（待决项 S-2） |
| **dtype / 位宽后缀** | ✅ 接收侧是纯字节计量，**不加** dtype 后缀；发送侧 `_b8` 是被复用指令身份的一部分，且只扩展 `_b8`；归约元素类型由独立的 `vtype_t` 操作数承载 |
| **版本变体** | ✅ 不使用 `_v2`：本变体不是代际升级，而是**对端修饰词不同的同代成员**，按构词法换对端修饰词而不是加版本号 |
| **三名对应**与命名层归属 | ✅ §5.1 逐条给出；四条既有指令沿用其既有归属（全部裸名层），两条新增也归裸名层（带 `__<space>__` 指针操作数的流水发射型指令，不是标量握手原语，故**不**进 `__` 前缀层） |
| **返回类型**：发射型 → `void`；标量读回 → `int64_t`/`uint64_t`；仿真壳 → `bool` | ✅ 两条新增均 `void`；复用的 `try_wait` 返回 `int64_t`（既有） |
| **指针限定**：一律带 `__<space>__`，禁用裸 `void*` | ✅ 全部 `__ubuf__` |
| **已解析指针**：拓扑/方向/距离折进指针，facade 不重复传拓扑字段 | ⚠️ **部分例外且有理由**：Calendar 的对端集合是任意转发树，软件算不出对端物理地址，故传"对称地址 + `routeBits`"；这是"NoC 不查表"的必要入口，不是冗余 |
| **编译期 vs 运行期**：结构性量走编译期，运行期变量走函数参数 | ✅ §5.2.5 给出完整切分表；`opcode` / `redOp` / `keyId` / `sid` / align 模式域全部为编译期立即数，并用非类型模板参防止被取址（禁令 F2） |
| **枚举参数**：复用既有枚举，新增值加架构守卫 | ✅ 复用 `pipe_t`（**零改动**）、`vtype_t`、`mem_dsb_t`、`event_t`、`sync_mode_t`；新增 `calendar_op_t` / `calendar_red_t`，全部置于 `#if defined(__DAV_WSE__)` 下 |
| **三态下沉**：native / `__CPU_SIM`（字节循环）/ GM-mock 齐全 | ✅ 发包 facade 的 GM-mock 几乎是对既有 UB→核外 align DMA 的 1:1 转调；mock 直接从 `rbLo/rbHi` 解出落核集合，窗口基址取自通道状态、不进签名 |

---

## 6. 依赖项与待决项

> **级别口径**：**P0** 表示融合式 `AllGather` 不可缺失；**P1** 表示语义必须具备但机器形态可替换或存在软件兜底；
> **P2** 表示优化项。★ 标注的三条是**软件侧无法检测**的死角。

### 6.1 硬件 / NoC 依赖总表

| 编号 | 依赖 / 假设 | 级别 | 对应 Step | 不满足时会怎样 |
|---|---|---|---|---|
| **HW-0** | **NoC 按 flit 自带的 40 × `{P,L}` 被动转发与落核**：`L=1` 落核提交，`out = {P_j=1 邻居} − ingress` 分叉复制；每跳**无路由表存储、无查表通路、无状态** | **P0** | Step4 | 整个方案不成立 |
| **HW-1** | **flit 头可携带 `{opcode(3b), redOp(3b), epochTag, routeBits(80b)}`**（约 12 B），逐 flit 复制同一份 | **P0** | Step4 | 退回 per-packet 头（待决项 C-2）；链路位宽 64 B 时头部开销约 19% |
| **HW-2** | **NoC 节点常驻 Calendar 时隙寄存器 `CalReg`**，由 flit 的 `opcode` 做一次寄存器索引 `CalReg[opcode]` 决定何时放行；装载期一次性**原子**写入、kernel 期只读、不随轮次更换；**每个 `opcode` 独立排队**以避免队头阻塞；非法/未安装码位必须 fault。寄存器内部格式与排时隙方式由 NoC 定义，程序不感知 | **P0** | Step4 | `opcode` 语义不成立 |
| **HW-3** | **发包指令编码能容纳 2 个额外 GPR 源槽（或 1 个偶奇寄存器对槽）+ 6 bit 立即数**；且 `nBurst > 1` 时**同一份 flit 头必须施加到展开出的全部 burst 上** | **P0** | Step4 | 前者不满足退方案 B（§4.4.3）；后者不满足退回逐行形态，**软件接口不变** |
| **HW-4** | **发包指令兑现 §4.4.4 的五条 `routeBits` 契约**（含"`rbHi` 高 48 bit 必须忽略、不得做任何检查"），并接受槽① 的**对称地址** | **P0** | Step4 | `landCount / flags / expectedRxBytes` 的免费搭车失效，须另加一次 LD |
| **HW-5** | ★**接收指令能按 `{opcode, epoch, dstRange}` 累计有效 payload 字节并据此退休**，满足 §4.5.2 的七条契约；其建立的本轮 `{opcode, epoch}` 上下文按 **AICORE / kernel / stream 隔离**，并可被同核 MTE3 的 flit `epochTag` 取用 | **P0** | Step3 / Step5 | **本方案最主要的正确性风险** |
| **HW-6** | **跨核报到指令的作用域**扩展到 Calendar `opcode` 域（全组成员核），**BSP 可观测**该汇合点并据以在硬件内部对齐全组时间原点（不返回给程序） | **P0** | Step2 | 不变式 **O2** 失效 ⇒ 链路冲突、性能塌陷（**不是**错转发） |
| **HW-7** | ★**跨核同步信号量位宽容得下该 `opcode` 域的报到核数**：既有为 4 bit（最多 15），FFN phase B 为 **32**、全片集合为 **40**，均需 **≥ 6 bit**；或改用"按成员位图判定到齐"的 reduce 语义。`flagId` 域须 ⊇ `0..6`；另需新增 `CAL_GROUP_SYNC` mode 码位 | **P0** | Step2 | 计数器回绕 ⇒ 等待指令在全组未到齐时提前放行，**软件不可检测** |
| **HW-8** | `PIPE_MTE3` / `PIPE_MTE4` **独立派发、独立反压、独立 barrier**；`pipe_barrier(PIPE_MTE3)` 必须等实际发出/排空，不得因"尚未被 `CalReg` 放行"提前返回；标量 → `PIPE_MTE3` 的 RAW 依赖覆盖 `rbLo/rbHi` | **P0** | Step3–Step5 | 全组死锁，或不变式 **O3/O4** 失效 |
| **HW-9** | **SU 可用运行期 `blockId` 索引 `.rodata` 静态表**；miss 走标准 D-Cache 通路（MSHR → BIU → **Batcher.mem** → 未命中回 **DDR** → 64 B 整行回填），**不经 local DRAM、不经 L1**，对用户透明 | **P0** | 取数 | 取数成本与 §3.8 的测算不符 |
| **HW-10** | **对称地址模型**：发包指令的目的操作数接受"全组一致地址 + 本核段偏移"；**转发不改落点**；本核段的写入分工由 `flags.selfLand` 明确 | **P0** | Step4 | 落点错乱或本核段漏写/双写 |
| **HW-11** | `get_block_idx()` 在目标上可用并返回本核 NoC 节点号 | P1 | 取数 | 退回 kernel 入参，取数变两级依赖（待决项 C-7） |
| **HW-12** | 既有 UB→核外 align 字节粒度 DMA 在 WSE 目标上的**源侧对齐与长度口径**（R-1 / R-2）以及 `srcGap` / `dstGap` 的单位已确认 | P1 | Step4 | 决定 `rowBytes` / `sendRowStride` 的合法取值（待决项 S-1） |
| **HW-13** | **三版本联合校验**：`topologyVersion` / `calendarVersion` / `routeVersion` 在装载期匹配，任一不匹配必须 fault | **P0** | 装载期 | 这是替代"一条指令原子选中路由/时隙"的一致性锚，缺失则配错无法检出 |
| **HW-14** | **拓扑同构**：目标芯片实例的物理拓扑与 PG 信息与编译期假设一致 | P1 | 编译/装载期 | `.rodata` 静态表降级为 B 类只读 buffer（§3.11 退化路径） |
| **HW-15** | **packet / segment identity 去重机制**，含"多播分叉副本按落核点去重" | **P0** | Step3 | `expVal` 被重复计数满足，集合提前返回（待决项 S-6） |
| **HW-16** | NoC / BSP 内部全域同步的时间基准（**硬件内部依赖，程序不感知**）：漂移界、跨 die / 跨 launch 的重同步 | **P0** | Step2 | 不变式 **O2** 的物理可实现性（待决项 C-4） |
| ★**E1**（软件侧） | 同一 `opcode` 域内成员核的 `TCalendar` 调用次数与顺序一致 | **P0** | 全程 | `epochCtr` 错位 ⇒ 接收侧按错 epoch 丢包 |
| ★**SW-1**（跨 launch） | launch 边界前全片 Calendar 流量已排空（host 相间 barrier） | **P0** | 相边界 | 上一 launch 的滞留副本被本 launch 同 `{opcode, epoch}` 的接收命令误记入 |

### 6.2 软件栈依赖

**编译器（PyPTO）**

| 工作 | 产物 |
|---|---|
| 从 IR 识别 GROUP、集合通信语义与集合间并发关系，结合目标芯片的拓扑信息与 PG 信息，调用 NoC 提供的 C 算法 | 每个逻辑身份的 40 行 `routeBits`、逐目的落核源数、`opcode`，以及全域时隙寄存器内容 `CalReg` |
| 校验位对合法、诱导子图为树、`P_self = 1`、落核完备、收发守恒、`selfLand` 一致 | 编译诊断与 `routeVersion` |
| 向 NoC 算法提供集合间并发关系；接收算法对**同 `opcode` 无冲突**与**同 `opcode` 不并发**（数据面 **与** 对齐面）的保证，不满足时重新分相 | `conflictProof` + 分相方案 |
| **发射 `.rodata` 静态表**（`alignas(64)`，不做 per-core 裁剪），核算 DDR 占用与每核驻留行数 | `kCalendarRoute[keyId][node]` 段字节与符号 |
| 为每个调用点固化 `CalendarKeyRef{keyId, opcode, redOp, routeVersion}` 与 `expVal` / `capacity` / `selfOff` 立即数 | `.text` 里的立即数 |
| 规划对称 `recvTile`，计算并校验 tile 几何与 `selfOff`（铺满性 **A3**、源行 32 B 对齐 **R-1**） | kernel 常量/参数及布局校验记录 |
| 下沉 `LD → TSYNC → align → MTE4 感知 → MTE3 send → join` | kernel plan |

> **不需要做的三件事**：活跃区间分析、按容量装箱、与 GEMM 的 L1 分配器互斥——本方案的表不占 L1，
> 受 D-Cache 预算约束而非 L1 容量约束。

**PTO-ISA 与工具链**：公共类型与 `TCalendar` 包装层；`Op::TCALENDAR` 返回事件以最终 MTE4 `expVal` 感知完成
为准，同时必须能向 `PIPE_MTE3` 建立 `sendTile` 生产依赖。**额外要求**：`CalendarKeyRef` 必须以按值 / 非类型
模板参形式传递，包装层不得取其地址（禁令 **F2**）。

**Host / Runtime / Batcher**

| 工作 | 时机 | 关键点 |
|---|---|---|
| 下发 kernel binary（`.text` + `.rodata` 同批） | 每 kernel 一次 | **与传统算子完全一致的下发过程**；Calendar 表不需要任何特殊处理 |
| **把 `CalReg` 全域镜像原子写入各 NoC 节点时隙寄存器** | 停机窗口内，一次性 | 写入期间不得有 Calendar 流量在飞；写入后 kernel 期只读 |
| **三版本联合校验**（`topologyVersion` / `calendarVersion` / `routeVersion`） | install 时 | 任一不匹配必须 fault |
| **不**预建 `collectionEpoch`、**不**分配 notify counter | — | Host 看不到 kernel 内的轮次；每轮 epoch 由核内计数器给出 |
| 提供 `blockId` | kickstart / launch | 优先用 `block_idx` SPR（0 次访存）；否则随 args 下发（B 类，1 次 LD） |
| 相边界插 barrier，保证全片 Calendar 流量排空 | 每次 launch 之间 | 跨 launch 的 epoch 隔离**完全依赖这一条** |
| 按完整 group 调度 | 每次 launch | 分 wave 时不得拆开正在同步的组 |
| 安装期校验 | install 时 | `selfOff` 铺满性；40 行覆盖完整；`expectedRxBytes == expVal`；成员/几何一致；对称地址、容量与 `opcode` 合法 |

### 6.3 待决项

#### 6.3.1 接口与编码（S 系列）

| 编号 | 待决项 | 归属 | 若结论改变，软件层要改什么 |
|---|---|---|---|
| **S-1** | **操作数位宽与单位冻结**：`expVal` / `capacity` / `epoch` 的编码位宽（本文按 `uint32_t`）；被复用的 9 个既有槽的精确位宽与单位（`srcGap` 以 32 B block 还是 byte 计、槽⑧⑨ 的确切含义）；溢出 / 超时 / 异常语义 | 硬件 ISA | 下沉层的换算与范围检查；`expVal` 若冻结为 32 bit，PTO 签名可同步收窄 |
| **S-2** | **地址空间与命名**：Calendar 竞技场落 UB 还是 L1（决定词根 `ubuf` / `cbuf`）；发包 facade 取 `copy_ubuf_to_noc_ubuf_align_b8`（推荐）还是保留旧词根的拼法；接收 facade 取 `wait_noc_to_ubuf`（推荐）还是更强调"不搬运"的 `wait_noc_recv_ubuf` | 硬件 + 软件栈 | 若落 L1，`recvTile` 的 tile 类型与 `__ubuf__` 限定词整体改为 `__cbuf__` |
| **S-3** | **对称地址的机器编码**：是一个**寻址模式位**，还是**另一个操作数槽**？ | 硬件 ISA | 仅影响下沉，PTO 接口不变 |
| **S-4** | **机器助记符**：沿用短名 `MTE3_NOC_SEND` / `MTE4_NOC_RECV_WAIT`，还是改为与 facade 同构的 `COPY_UBUF_TO_NOC_UBUF_ALIGN_B8` / `WAIT_NOC_TO_UBUF`（既有惯例：机器助记符全大写 `SNAKE_CASE`） | ISA 评审 | 仅文档与汇编可读性 |
| **S-5** | **两处小语义确认**：① 发包指令的 `sid` 在 Calendar 方向无意义，硬件是忽略还是对非零值 fault；② 报到指令 `pipe` 操作数的定序含义（是否等价于 `set_flag` 的 `srcPipe`） | 硬件 ISA | ② 决定 `SyncPipe = PIPE_MTE3` 能否真的免去一道栅栏；否则须补栅栏 |
| **S-6** | **packet / segment identity 去重机制**：§4.5.2 契约 5 的实现形态，含"多播分叉副本按落核点去重" | 硬件 | 无软件侧替代方案；缺失则 `expVal` 可被重复计数满足 |
| **S-7** | **三态下沉的 mock 形态冻结**：GM-mock 的对端窗口布局与计数器位置是否进 facade 签名（本文按"不进签名、取自通道状态"处理） | 软件栈 | mock facade 签名 |

#### 6.3.2 方案专有（C 系列）

| 编号 | 待决项 | 归属 | 若结论改变，软件层要改什么 |
|---|---|---|---|
| **C-1** | **发包的 80 bit 操作数编码**：方案 A（2 个 GPR 源槽或 1 个偶奇寄存器对槽 + 6 bit 立即数，共 2 条新 ISA）vs 方案 B（`set_calendar_send_ctx` 锁存进 SPR，共 3 条新 ISA + 一道栅栏 + 一个上下文隔离状态） | 硬件 ISA | 方案 B 下 Step4 前插一条 `set_calendar_send_ctx` 并补栅栏；**调用点不变** |
| **C-2** | **flit 头承载形态**：(a) 逐 flit 复制（基线，NoC 完全无状态，头部约 12 B/flit，链路位宽 64 B 时约 19%）；(b) 逐 **packet** 头部一次 + 沿途保留每 packet 上下文（省带宽，但把状态还给 NoC） | NoC | 带宽与 NoC 每跳是否有状态 |
| **C-3** | **归约元素类型字段**：`redOp` 单独不足以确定机器运算，需在 flit 头与发包 facade 签名中追加 3 bit 元素类型（建议取 `vtype_t` 子集）。首期只上 `AllGather` 时不阻塞，但**必须在 flit 头格式冻结前决定** | NoC + ISA | `CalendarKeyRef` 增加一个 `dtype` 字段并透传到发包 facade |
| **C-4** | **NoC / BSP 内部时间基准的全域同步精度**（硬件内部依赖，程序不感知）：漂移界、跨 die / 跨 launch 的重同步、BSP 对齐时建立内部时间原点的时延与 arm lead time 的关系 | NoC + BSP | 不变式 **O2** 的物理可实现性；软件侧无接口改动 |
| **C-5** | **`epochTag` 位宽与回绕策略**：对齐指令不再返回 epoch 后，唯一性改由"核内计数器 + SPMD 同构 + 同 `opcode` 不并发 + **E1**"保证 ⇒ 需确认**跨 launch 的计数器初值约定**（本文按"每 launch 从 1 起算 + launch 边界排空"），以及"同 `opcode` 不并发"不满足时是只做编译期硬失败还是另需运行期 fault 兜底 | 硬件 + 编译器 | 若要求 epoch 跨 launch 单调，`epochCtr` 须移到按 `blockId` 索引的每核私有 GM slot，每轮多一次读改写 |
| **C-6** | **`epochTag` 的载体**：由接收指令建立的本轮上下文隐含传给发包（本文基线，省 SEND 编码槽，但引入一个需上下文隔离的隐含状态）vs 发包显式带 `epoch` 操作数（彻底无状态，多占编码槽） | 硬件 ISA | 后者下沉层多传一个操作数；**调用点不变** |
| **C-7** | **`blockId` 获取方式**：`get_block_idx()` 在 WSE 目标上的可用性（传统 alias 在某些既有目标上被报 target feature 不支持） | 硬件 + Runtime | 退回 kernel 入参 ⇒ `CalendarChannel::blockId` 由形参填入，取数成两级依赖、miss 可能串行叠加 |
| **C-8** | **逐行不同路由的表形状**：`ReduceScatter` / `Scatter` 需把静态表扩成 `[keyId][node][shardIdx]` 三维，**DDR 占用乘以 `shardCount`；更要紧的是每核 D-Cache 驻留也随 `shardCount` 线性增长**（`shardIdx` 是循环变量，"下标恒定"的前提被打破，访问模式退化成真正的顺序查表）。是否值得，还是改用"每 shard 一个 `keyId`" | 编译器 | 影响逐行形态的取数 helper 形状与 §3.8 预算 |
| **C-9** | **静态表布局 key-major vs node-major**：转置后每核 D-Cache 驻留与冷 miss 各降 4 倍，而**指令数、寻址形态、表总字节数全不变**（§3.8.3 已给出完整对照）。**不影响任何硬件依赖与调用点**；**建议在 `keyCount > 4` 之前决策** | 编译器 | 只改表声明、取数 helper 的地址算术与编译期形状断言 |
| **C-10** | **归约语义在 `{P,L}` 下的完整定义**：本文的口径是"归约点 = 位对 `11` 且存在下游 `P` 邻居"（§2.2）。需 NoC 确认：中间节点的 merge 缓冲深度、乱序到达时的 merge 时序、`redOp` 的溢出 / 饱和语义 | NoC | 决定 `Reduce` / `AllReduce` / `ReduceScatter` 分支何时能开 |
| **C-11** | **坏核 / 坏链导致每实例拓扑不同时的退化路径**：静态表将从 A 类降级为 B 类只读 buffer（§3.11）。需要确定：是否允许编译期假设"拓扑同构"，还是从一开始就按 B 类设计 | 系统 + 编译器 | 决定 §3.4 / §3.5 的链路形态；取数变两级依赖，每次 launch 多一次下发 |
| **C-12** | **`.rodata` 表的 D-Cache 策略**：是否需要在取数前用 `DataCachePreload` 把行提前拉进来（preload 不改变下发链路，只把 miss 的时间点提前，代价是用一个发射槽换延迟）；`keyCount` 上限 16 是否够 | 编译器 | 取数 helper 内多一条 preload；预算上限 |

### 6.4 验收 checklist

**编译期（PyPTO）**

- [ ] 每个逻辑身份的 40 行 `routeBits` 中无 `01` 位对，且 `P_self == 1`
- [ ] `{P_i=1}` 的诱导子图连通、含 source、无环（是树）
- [ ] `{L_i=1}` 与集合语义的目的核集合一致，且 `landCount == popcount(L)`
- [ ] 对每个目的核 `d`：`Σ_{s: L_d(s)=1} rowCount × rowBytes == expectedRxBytes[d]`
- [ ] source 自身的 `L` 位与 `CalendarPlatformTraits::FabricSelfDelivery` 一致
- [ ] 共用同一 `opcode` 的全部逻辑身份在**同一次** NoC 算法调用中生成，算法返回成功并附 `conflictProof`
- [ ] 集合间并发关系已作为算法输入；同一 `opcode` 的两个集合不会同时在飞、**也不会同时汇合**
- [ ] `keyCount ≤ 16`，`keyCount × 40 × 16 B` 在 DDR 预算内，表 `alignas(64)`
- [ ] `armLeadCycles[opcode]` 覆盖 MTE4 感知命令 + 全部 MTE3 描述符入队的最坏时延
- [ ] `sendTile` 行起点 32 B 对齐（R-1）；`selfOff` 不重叠地铺满竞技场每一行（A3）

**编译产物（§3.10 自检脚本）**

- [ ] `.data` / `.bss` 中无 Calendar 自有符号（禁令 **F1**）
- [ ] `kCalendarRoute` 落在 `.rodata`（`nm` 类型为 `R`/`r`），大小 == `keyCount × 40 × 16`
- [ ] `.rodata` 段 `AddrAlign ≥ 64`（禁令 **F3**）
- [ ] `kRowAllGather` / `kColAllGather` 等 `CalendarKeyRef` 常量**未被物化**（禁令 **F2**）
- [ ] 反汇编中报到指令的 `flagId` 与发包指令的 `opcode` 字段**确实是立即数**
- [ ] 反汇编中**没有**任何 Calendar 专用控制面指令（时隙对齐必须走既有跨核报到 / 汇合指令）
- [ ] `CalendarChannel` 未被声明为全局 / 静态可写对象

**装载期（Host / Runtime）**

- [ ] `CalReg` 全域镜像在停机窗口内原子写入各 NoC 节点完成，且无流量在飞
- [ ] `topologyVersion` / `calendarVersion` / `routeVersion` 三方匹配，否则 fault
- [ ] 目标芯片实例的物理拓扑与编译期假设一致（无坏核 / 坏链重映射），否则走 §3.11 退化路径
- [ ] 核内 `CalendarNextEpoch(opcode)` 给出的 `collectionEpoch` 在其 `opcode` 域内单调、全组取值一致、
      回绕前旧 epoch 已排空——★**本方案的正确性依赖此项，不是优化项**
- [ ] 报到指令的作用域覆盖 Calendar `opcode` 域的全部成员核，BSP 能观测该汇合点以对齐时间原点（X-1 / X-2）
- [ ] ★ 跨核同步信号量位宽 ≥ `⌈log2(报到核数 + 1)⌉`（32 核 / 40 核均需 ≥ 6 bit），或已改用位图判定（X-3）
- [ ] `blockId` 来源已确定（`block_idx` SPR 优先），且 `blockId < 40`
- [ ] 相边界 barrier 已插入，保证 launch 之间全片 Calendar 流量排空
- [ ] 不分配、不清零任何 notify counter

---

## 附录：术语表

| 术语 | 含义 |
|---|---|
| **`routeBits`** | **40 × 2 bit = 80 bit 路径位图**：每个 NoC 节点一个 `{P, L}` 位对；`bit(2i+1)=P_i`、`bit(2i)=L_i`，bit `b` 落在字节 `b/8` 的第 `b%8` 位（LSB 优先）。随每个 flit 走 |
| **`{P, L}` 位对** | `00` 无关 / `10` 经过不落核 / `11` 经过且落核（可续转） / `01` 非法 |
| **诱导子图** | 取 `{P_i = 1}` 的节点集合后，物理拓扑上**集合内任意两个相邻节点之间的链路都算在内**的子图；它必须连通、含 source、无环 |
| **归约点** | 位对为 `11` 且存在下游 `P` 邻居的节点；归约类语义在此 merge 后续转 |
| **`opcode`** | 3 bit 时隙寄存器索引，值由集合语义唯一决定；职责有二——**索引 `CalReg[opcode]`** 与**兼作对齐信号量 `flagId`**；编译期立即数，编进 `.text` |
| **`redOp`** | flit 头的独立 3 bit 归约算子字段（`None/Sum/Max/Min/Prod`）；`AllGather` 时 dormant |
| **`CalReg`** | NoC 节点常驻的 Calendar 时隙寄存器；`CalReg[opcode]` 决定该 `opcode` 的包何时放行；装载期一次性原子写入、kernel 期只读；内部格式由 NoC 硬件定义，程序不感知 |
| **`keyId`** | `.rodata` 静态表的 key 维下标；编译期立即数，折进 LD 的地址偏移 |
| **`blockId`** | 本核的 NoC 节点号；**唯一的运行期变量下标**，也是本方案唯一的 B 类值 |
| **逻辑身份（`RouteKey`）** | `{programId, kernelId, phaseId, collective, groupRole}`，编译期的稳定符号身份 |
| **`CalendarKeyRef`** | 不可拆分的编译期常量对 `{keyId, opcode, redOp, routeVersion}`；不变式 **O1** 的类型系统落地，不可拆分、不可取址 |
| **`CalendarRouteEntry`** | 16 B 的 `.rodata` 静态表项：`routeBits[10] + landCount + flags + expectedRxBytes` |
| **`CalendarRouteRegs`** | 取数结果的**按值**形态 `{rbLo, rbHi}`；`landCount / flags / expectedRxBytes` 是 `rbHi` 高 48 bit 的免费搭车 |
| **`kCalendarRoute[key][node]`** | 40 行全核同构的静态表；`alignas(64)`，随 kernel binary 一次 H2D；DDR 里全片只有一份 |
| **`landCount`** | `popcount(L)`；本核这一发的落核数，与 `expVal` 的推导直接相关 |
| **`selfLand`** | `flags.bit2`：source 自身的 `L` 位是否置 1，必须与 `CalendarPlatformTraits::FabricSelfDelivery` 一致 |
| **对称接收竞技场** | 组内所有成员的 `recvTile` 使用相同的本地 UB 偏移 |
| **`selfOff`** | 本源 shard 在接收竞技场每一行内的段偏移，单位为字节；SPMD 下**唯一逐核不同**的发包操作数 |
| **`expVal`** | 本轮必须从**远端**收到的有效 payload 字节数；达到前接收命令保持未完成。不计 flit 头、CRC、填充、重传副本与本核来源数据 |
| **`capacity`** | 本核接收竞技场 `[dst, dst + capacity)` 的字节容量 = `recvRowCount × recvRowStride` |
| **`collectionEpoch`** | 本轮集合的接收身份编号，隔离旧轮迟到包、重复包与其他流量；由核内 `CalendarNextEpoch(opcode)` 给出，写入 flit 头 `epochTag` 并作为 MTE4 记账键。**本方案里它承担全部区分职责**（`opcode` 判别力弱） |
| **`CalendarNextEpoch`** | 每 `opcode` 域一个的核内轮次计数器，纯标量自增；**不是** intrinsic、**不是**机器指令；每 launch 从 0 重置、第一轮为 1 |
| **`flagId`** | 跨核同步信号量编号（`0..15`）；Calendar 直接用 `opcode`（`1..6`）充当，这是"对齐不新增指令"的前提 |
| **`nBurst` / `lenBurst`** | 被复用的 align DMA 的行数与每行字节数；Calendar 用它们把整个 tile 压成一条发包指令 |
| **`MTE3_NOC_SEND`** | ★扩展既有的 MTE3 核间写（UB → 核外 align 字节粒度 DMA）：9 个既有槽 + `{rbLo, rbHi, opcode, redOp}`；**MTE3 负责把包发到目的地**，路由与时隙在此进入数据面。流水线 `PIPE_MTE3` 为既有，不新增 |
| **`MTE4_NOC_RECV_WAIT`** | ★新增：**只做 `expVal` 感知，不搬运数据**——按 `{opcode, epoch, dstRange}` 累计 NoC 已写入本核的有效字节，达到 `expVal` 后退休 |
| **`Batcher.mem`** | 供给全片各核 I$/D$ 的共享存储层级；D-Cache miss 的回填路径是 `MSHR → BIU → Batcher.mem →（miss）DDR`，**不经 local DRAM、不经 L1** |
| **BSP** | 承担全组时隙对齐与 group 调度的硬件 / 固件实体，即文中的 Batcher；它观测跨核汇合点并据以对齐全组时间原点 |
| **A 类 / B 类 / C 类** | 三种户籍（§3.1）：编译期静态镜像 / 运行期 Host 下发 / 运行期 Device 自产 |
| **立即数通道** | 编译期常量**不需要地址** ⇒ 折进指令立即数 ⇒ 物理上就是 `.text` 的一部分 ⇒ I-Cache（2 KB 块）⇒ 零 D-Cache 流量 |
| **`.rodata` 通道** | 编译期常量被**运行期变量索引** ⇒ 必须物化 ⇒ `.rodata` ⇒ Scalar LD ⇒ D-Cache（64 B 行）⇒ 永不 dirty |
| **arm lead time** | 对齐返回后到首个发包时隙之间的准备窗口，由 BSP / NoC 在该 `opcode` 域内部预留；**不是程序可见操作数**，程序不感知 cycle |
