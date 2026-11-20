# FA2-CORE-ARRAY3D-1002：16×16×16 空间 MAC 阵列 {#FA2-CORE-ARRAY3D-1002}

<!-- ndf: kind=mechanism modality=may refinement=L2 status=draft owner=flash_attention2 refines=FA2-CORE-COMP-0001 -->

## 状态与边界

本页是新增 3D 阵列的有界实现合同，M=N=P_K=16。架构方案和独立分解已完成，
源码、默认预算下的完整链接、双目标生成与执行证据分别验收；本文的容量、
空间并行度和流水边界是设计目标，不能当作已经执行的结果。旧 ARRAY-1002/1003、P0S 和冻结
L0 不作改写。旧 2D 阵列连 clear 闭包都曾触发默认 transaction 工作预算，新增
3D 规模不会因为接口设计完成而自动获得编译支持。

本块计算独立 tile，不连接 SRAM、gather、DMA、softmax、online rescale、精度
转换或完整 attention scheduler。输入由调用方提供；供数与存储的独立提案见
[ARRAY3D-BUF-1004](fa2-core-array3d-buf-1004.md)。该 feeder 不属于本块实现。
不提高预算，不识别测试夹具，不以标量复用替代 4096 个空间乘法位置。

## 空间结构与数值顺序

16 个 Row，各有 16 个 Dot16 cell。cell `(m,n)` 含 16 个空间乘法位置：

```text
p[k] = MUL_RTZ_FTZ(A[m,k], B[k,n])              k=0..15
s1[j] = ADD_RTZ_FTZ(p[2*j], p[2*j+1])         j=0..7
s2[j] = ADD_RTZ_FTZ(s1[2*j], s1[2*j+1])       j=0..3
s3[j] = ADD_RTZ_FTZ(s2[2*j], s2[2*j+1])       j=0..1
s4 = ADD_RTZ_FTZ(s3[0], s3[1])
total = ADD_RTZ_FTZ(total, s4)
```

乘积、s1、s2、s3、s4 都有实际持久数据寄存器；total 是第六个寄存边界。
共 4096 个逻辑乘法位置、3840 个树加法位置和 256 个跨组累加位置。不能用一个
组合归约结果再等待若干周期来代替四级树，不能用一个乘法器循环 16 次代替 K 轴。
这是未融合算术；每个乘法、树节点及跨组加法分别舍入。树顺序固定为相邻配对，
跨组按接受顺序累加，因此一般不与旧逐 K 标量累加逐位相等。

复用 `fa2_numeric.py` 唯一的 `fp_mul_value` / `fp_add_value` 实现：有限 binary32
编码，输入/结果 FTZ、RTZ、溢出有限饱和，保留既有 signed-zero 规则。NaN/Inf
在输入域外；本块不新增浮点异常协议。独立期望使用精确有理数和显式编码，不能
调用 DUT helpers 或 host float 计算预期。

乘法零符号为输入符号异或。加法按原始编码的无符号 magnitude 选择 large，
相等时保留第一个操作数；零/下溢结果使用该符号。输入 subnormal 虽在算术值上
FTZ，符号选择仍比较其原始 magnitude，不能先把两输入重编码为 ±0 再选符号。

## 接口与操作数布局

使用无普通输出的静态 `Array16x16x16` Module 实例和 typed callable Methods。
调用者在 Rule 中调用同一实例；参数、availability、返回值与原子控制由编译器
连线。普通 source 不暴露 clock/reset、ready、proposal/commit 或寄存器 q/d。

| 类型 | 字段 |
| --- | --- |
| `Row16` | `c0..c15: ac.u32`，复用既有多字段行类型 |
| `Array3DBegin` | `tag: ac.u32`, `length: ac.u7`, `cell_mask: ac.bits[256]` |
| `Array3DAdvance` | `action: ac.u2`, `tag: ac.u32`, `index: ac.u3`, `left/right: ac.table[16, Row16]`, `row_mask/col_mask/k_mask: ac.u16` |
| `Array3DResult` | `tag: ac.u32`, `rows: ac.table[16, Row16]`, `groups: ac.u3`, `useful_macs: ac.u32` |

`left[m].cK` 表示 A[m,k]；`right[n].cK` 表示 B[k,n]，即右操作数按输出列组织。
每侧 256 个 32-bit 数，共 1024 B；每个满组共 2048 B。作者先绑定 Table 元素
到 local 再读字段，不依赖 Table-of-Table 或二级下标。result 的所有字段一起
交给一个消费者，因此一个 Struct 合理；单个返回值不另包装单字段 Struct。

length 是完整 reduction 长度，合法范围 1..64；组数 `G=ceil(length/16)` 为
1..4。index 表示组号 0..G-1，不是 KV token 标识。每个组按 K 连续覆盖 16 项。

| Method | 合同 |
| --- | --- |
| `begin(command: Array3DBegin) -> None` | IDLE 且 length 合法；原子保存身份/掩码，清零 total、占用和统计，进入 ACTIVE |
| `advance(command: Array3DAdvance) -> ac.u1` | 显式推进一次阵列；返回本次 action 是否被接受，详细规则如下 |
| `take() -> Array3DResult` | RESULT；返回完整结果并原子释放为 IDLE |

每个 rising edge 最多调用上述一个公开 Method。调用者不能同一 Rule 串联 begin、
advance、take 来缩短周期；不同 Rule 也不能同时争用生命周期。tag 是身份比较，
不是防重放计数器；旧请求可能到达时不得复用 tag。

## 显式推进、命令接受与生命周期

状态为 IDLE、ACTIVE、DRAIN、RESULT。`advance` 的 action 编码：

| action | 允许条件 | 动作与返回 |
| --- | --- | --- |
| 0 BUBBLE | ACTIVE/DRAIN 且 tag 匹配 | 推进旧 token，不插入组；返回 1 |
| 1 GROUP | ACTIVE、tag 匹配、index=accepted_groups<G | 推进旧 token，捕获本组乘积，accepted_groups 加 1；返回 1 |
| 2 END | ACTIVE、tag 匹配、accepted_groups=G | 推进一个气泡并封闭输入，进入 DRAIN；返回 1 |
| 3 ABORT | ACTIVE/DRAIN/RESULT 且 tag 匹配 | 优先撤销 tile、清空全部流水占用并进入 IDLE，不做本次算术推进；返回 1 |

ACTIVE/DRAIN 中未满足条件的命令返回 0，但仍以气泡推进已有 token。
例如重复组号、错误 tag、提前 END、DRAIN 中 GROUP，均不新增 group，却不能
阻止旧 group 前进。`actionAccepted=0` **不等于整个调用事务未提交**。
IDLE 中任何 advance 返回 0 且不改变阵列；RESULT 中只有匹配 ABORT 有效，
其余 advance 返回 0、保持结果。begin/take 的生命周期 guard 不满足时则是
普通 callable 不可用，调用者事务不提交；两种机制必须分开检查。

没有 advance 调用时，全部流水数据、占用、total 和统计保持。不存在暗中运行
的 autonomous pipeline。返回 actionAccepted 只是显式命令协议的应答，不替代
编译器提供的 Method availability，也不是“Rule 未提交”的载荷 validity。
如果整个调用者事务因下游资源或检查不能提交，advance 和应答消费都不提交。

每次正常 advance 退休旧 s4 token，再推进旧 s3/s2/s1/product，最后注入新组或
气泡。封闭输入后，已退休组数达到 G 的同一提交边沿进入 RESULT；即使 END
晚于全部组退休，也在该 END 边沿进入 RESULT。全部组已退休而未 END 时保持
ACTIVE，不允许提前 take。非法请求可与最后一次退休同边沿发生，此时返回 0
但 DRAIN 仍能转入 RESULT。

ABORT 优先于正常推进；与最后一个 token 同边沿时仍不生成成功结果。
物理数据寄存器无需在 abort 时清零，但父/子占用必须原子清空，下一 begin
清零 total 后不得泄漏旧组。reset 初始化 IDLE、零占用和 total，四态失败执行
现有 whole-System 零提交规则。不能用 `is_known` 或 `case_eq` 消除功能控制的 X/Z。

## 掩码、尾组和空组

cell_mask 的 bit `16*m+n` 对应 `(m,n)`。GROUP 的有效 K 叶条件是：

```text
cell_mask[16*m+n] & row_mask[m] & col_mask[n]
& k_mask[k] & (16*index+k < length)
```

无效叶写入精确 `+0`，不读取其乘法结果来形成有效叶。有效 cell 的树仍按固定
16→8→4→2→1 归约，包括补零叶。一个 cell 在该组没有任何有效叶时，整个组
对该 cell 的累加器为逐位保持，不能以 `ADD(total,+0)` 替换；这保留 signed-zero
语义。完全空组仍消耗组号并按相同流水时序退休，useful_macs 增加 0。

k_mask 是组内所有行/列共享的 K 掩码，不是 16 个独立 per-row K membership。
不同 query 的 sparse KV 列表不能仅靠 row_mask 合并；必须由外部调度建立共同
token 顺序及兼容 membership，当前接口没有自动对齐或 gather 功能。QK 和 PV
可分别映射 A/B，但其 softmax、重标定和跨 tile 数值顺序不在本合同中。

## 状态与 Pythonic 拓扑

静态 16 Row × 16 Dot16 层级；不依赖动态实例列表或 Method-owner Collection。
每个 Dot16 子树独占 16+8+4+2+1+1=32 个 32-bit 数据状态，整个阵列为 32 KiB。
乘积可由每 cell 的 16 个独立 Product 子 Module 各保存一项；树节点也可按真实
寄存边界划分独立子 Module。它们使用普通有状态算法方法，不暴露 q/d 或 DFF。
拆分只能改变静态定义的粒度，不能共享实例、时间复用乘法或减少寄存级数。
前级 read 必须在同一推进中对应的 load 之前，保持提交前数据的时序。
五级在途数据有真实 token 占用：父级跟踪所有组（包括空组）的五位占用；
cell 跟踪相应组是否参与累加的五位状态，或等价的父级五份 256-bit participation
掩码。它们是跨周期任务状态，不是替作者手写的 proposal/commit 控制。

父级独占 phase、tag、length、cell_mask、accepted/retired_groups 和 useful_macs。
一个公开 advance 调用同一批已有 Row/Dot16 实例，所有流水、累加和计数属于
同一个事务；不是 256 条独立 Rule 偶然同步。内部 cohesive advance 方法按
下游到上游顺序赋值，或先保存旧值，保证每个阶段只消费提交前的前级。
Module 层注解用于持久状态并使用定义良好的初始化；动态 wire 局部量不以状态
注解伪装，必要的 widening 放在 typed pure helper 或 Rule 局部。

状态量之外的操作数广播和结果连线不计入 32 KiB；control bits、暂存、编译器
导出结构和物理综合面积必须分别报告。4096 是逻辑空间乘法位置，不是 DSP 数。

## 流水结构与统计

GROUP 依次经过产品、四级树和 total 的六个持久数据寄存边界；每次成功
advance 推进在途组，没有调用则保持，BUBBLE 只推进旧组。资源须支持连续
GROUP 接受，目标组 II=1。RESULT 必须在已接受的全部组退休且 END 已接受后
才可取；本接口不支持 tile overlap 或 begin/take 同拍旁路。算子时延和
tile cadence 不属于本 NDF。
`groups` 是实际接受组数，`useful_macs` 是有效叶乘法数，最大 64×256=16,384。
空组也占 4096 个容量 slot；不能计成有效运算。非空 cell 每组执行 15 个树加法
和 1 个跨组加法，所以带 mask 时加法数一般不等于 useful_macs。

满组有 4096 个乘法和 4096 个加法位置（8192 运算）；输入为 2 KiB/advance，
不表示片外总线已提供相应带宽。完整 attention 性能必须另计供数、gather、
存储、softmax 和写回。首批验证与未运行项见
[ARRAY3D-1003](../l3/fa2-core-array3d-1003.md)。

## 当前证据边界（2026-11-04）

实际编译、运行和性能状态见 [VALIDATION](../../VALIDATION.md)。
