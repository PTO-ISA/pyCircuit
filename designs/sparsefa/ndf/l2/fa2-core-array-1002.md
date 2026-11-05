# FA2-CORE-ARRAY-1002：P0T 16×16 共享外积阵列 {#FA2-CORE-ARRAY-1002}

<!-- ndf: kind=mechanism modality=may refinement=L2 status=draft owner=flash_attention2 refines=FA2-CORE-BUF-0001,FA2-CORE-COMP-0001 -->

## 状态与本次边界

本页保留二维基线，后续并行设计转向
[16×16×16 三维阵列](fa2-core-array3d-1002.md)。两者使用不同归约次序和接口，
本页的 K+2 周期合同不能用于三维阵列。

本页记录首个有界实现合同。**16×16 阵列源码已实现，四个 source unit compile
通过；完整 System 在默认预算下 link 受阻，尚无 final、双 emit 或生成 C++ 执行
证据**。独立分解、root 合同审查及独立源码语义审查已完成；这些不替代硬件执行。
P0S 已有生成 C++ 功能/周期仿真证据，不是 P0T 性能验收。L0 不变，L1/L2 仍为 draft。

第一交付物是独立 `Array16x16` 计算块、独立算术 oracle 和生成 System 测试。
计算块被 QK 与 PV 分时复用；本次不连接 DMA、SRAM、softmax、指数/归一化、
FP16/BF16 转换、全 attention scheduler 或 P0S 行引擎。P0S 行引擎、scheduler、
bench、输入和 oracle 不变；唯一 numeric 源码重构例外及回归要求见末节。不能据此宣称完整 P0T、外部带宽或端到端加速已经实现。

## 阵列形状和数值合同

阵列含 **16 个 Row，每个 Row 含 16 个独立 PE，共 256 个逻辑 MAC**。
PE `(r,c)` 独占一个 `ac.u32` 编码 FP32 累加器；每个被接受的 reduction step，
一个左操作数 `left[r]` 沿行广播，一个右操作数 `right[c]` 沿列广播：

```text
product = fp_mul(left[r], right[c])
acc[r,c] = fp_add(acc[r,c], product)
```

这是先乘后加的两个独立舍入运算，不是 FMA。复用 `fa2_numeric.py` 的既有
`fp_mul`/`fp_add` 算法（pure helper 绑定见末节），normal 和 signed zero 输入、输入/结果 FTZ、RTZ、有限饱和，
保留既有 signed-zero 语义；不是完整 IEEE-754。调用方提供已校验的有限编码，
NaN/Inf 不属于本块数值输入域，不在本次新增非法数值错误协议。
独立 oracle 必须基于精确有理数计算与显式编码规则，不能调用 DUT 算术给出期望。

**每个 PE 每步只消费一个 reduction 项**。16 是空间行/列数，不是每 PE 的
reduction 展开数；不再使用“256 个 score lane 各自又含 16 维 MAC”的含混说法。
完整有效 step 为 256 次乘法和 256 次加法，即 256 MAC / 512 算术操作；不代表
256 个物理 DSP，也不代表某个时钟频率能够实现。

## 最小接口

采用零普通结果的静态 Module 实例，公开 typed callable methods。调用方在自己的
Rule 内调用同一实例；编译器负责跨层参数、结果、availability 与原子控制。
不手写 ready/valid、proposal/commit、clk/reset 或转发 wrapper。
以下类型/接口已在 `src/fa2_array_interfaces.py` 与 `src/fa2_array.py` 实现并
通过 source-unit compile；完整 link/执行状态见末节。表中的 Table 写作实际源码
`ac.table`，不表示已经获得 final 或运行验收：

| 类型 | 字段与含义 |
| --- | --- |
| `Vector16` | `values: ac.table[16, ac.u32]`；实际源码直接使用 Table 参数/字段，不创建单字段 Struct |
| `Row16` | 结果表的元素类型；实现为含 16 个明确命名 `ac.u32` 字段的 Struct，避免单字段 Table wrapper 与二次下标 |
| `ArrayBegin` | `tag: ac.u32`, `length: ac.u7`, `cell_mask: ac.bits[256]` |
| `ArrayStep` | `tag: ac.u32`, `index: ac.u7`, `left: ac.table[16, ac.u32]`, `right: ac.table[16, ac.u32]`, `row_mask: ac.u16`, `col_mask: ac.u16` |
| `ArrayResult` | `tag: ac.u32`, `rows: ac.table[16, Row16]`, `steps: ac.u7`, `useful_macs: ac.u32` |

`Vector16` 仅是本页对 16 元素 Table 的简称，不引入新 nominal type。
Result 的全部字段一起交给一个消费者，使用一个 Struct；Row 的字段按 `c0..c15`
排列，`rows[r].cN` 对应 `(r,N)`。读 Table 元素先绑定 local，再读 Struct 字段，
不依赖尚不支持的 Table-of-Table 或二级下标。不要把 unrelated 接口封入此结果。

`ArrayStep.index` 是本次 reduction 的序号，不是 KV cache 的 token 索引。
KV 选择仍由外部预生成列表和 gather 模块负责；本阵列接收已选出的操作数，不生成索引。

| Method | 允许状态与动作 |
| --- | --- |
| `begin(command: ArrayBegin) -> None` | IDLE 且 `1 <= length <= 64`；捕获 tag/length/cell_mask，清除所有 PE 为 +0、计数清零，进入 ACTIVE |
| `step(beat: ArrayStep) -> None` | ACTIVE、tag 相同、`index == accepted_steps < length`；原子更新全部有效 PE 与计数 |
| `end(tag: ac.u32) -> None` | ACTIVE、tag 相同、`accepted_steps == length`；进入 RESULT，冻结结果 |
| `take() -> ArrayResult` | RESULT；返回已提交的 256 个累加器及元数据，原子释放为 IDLE |
| `abort(tag: ac.u32) -> None` | ACTIVE 或 RESULT 且 tag 相同；丢弃当前 tile，进入 IDLE，不生成成功结果 |

上述条件是算法/生命周期 guard，保留在源码；Queue 容量、结果 availability 等资源
条件由编译器处理，不重复写入 guard。失败的已知 guard 仅令调用事务不提交，
没有错误 completion；未知已到达控制遵循现有四态/全局零提交合同。
本次不增加错误状态寄存器。非法 length、重复/乱序 index、错误 tag、提前 end、
忙时 begin 的测试必须验证无提交，而不是将其计为 accepted。

合法调用者每个 rising edge 对此实例最多发起一个公开生命周期动作；不能在同一
Rule 中串联 begin/step/end/take 来缩短合同周期，也不能在不同 Rule 并发争用它们。
这是第一版单发射调度边界，不暗示编译器新增仲裁。abort 与 take 不同时发出；
调度器希望取消时选择 abort。tag 是身份比较，不是防重放计数器：不得在旧调用仍
可能到达时复用 tag；本块无外部 outstanding 响应和 drain。

## Ownership、mask 与背压

Array 独占 phase、tag、length、cell_mask、accepted_steps、useful_macs；每个 PE
独占自己的累加器。Row 仅组织固定 16 个 PE，不额外保存镜像 accumulator。
源码声明的累加状态为256×32=8,192 bit（1 KiB）；6个控制字段合计
2+32+7+256+7+32=336 bit。合计8,528 bit是逻辑状态量，不包含输入/结果连线、
测试常量表或后续SRAM，也不表示综合后的寄存器面积。每个Row返回16-bit有效mask的
popcount，Array累加16行计数；当前源码没有为计数或浮点链显式加入流水级。
Array 的一次 step 调用同一批已有 Row/PE 实例；全部 PE 和计数器属于调用方的
同一个事务。任一已到达检查失败或调用方下游资源阻塞，全部更新都不提交。
不能把 256 个独立 autonomous Rule 的偶然同步当作 tile 原子性。

cell_mask bit `16*r+c` 对应 `(r,c)`；step 的有效条件为：

```text
cell_mask[16*r+c] AND row_mask[r] AND col_mask[c]
```

无效 PE 不执行该项算术更新，逐位保持 accumulator；不是先计算再乘零。
空 mask 的 step 仍消费其 index，accepted_steps 加 1，useful_macs 加 0。
全无效 tile 的结果为 +0。固定 cell_mask 支持非矩形尾部/causal 输出集合；逐步
row/col mask 支持 PV membership 与尾维。mask 的意义由调度器提供，PE 不猜测 token。
有限域内无效载荷不能影响有效输出；未知 mask 按普通已到达条件处理，不能用
`is_known`/`case_eq` 消除硬件未知控制。

RESULT 状态不再接受 step/begin；无 take 时 accumulator 和元数据无限保持。
消费者可在同一 Rule 中 `result = array.take()` 后 `output_queue.push(result)`；
输出阻塞必须使整个事务（包括释放状态）保持。没有 Queue 的第一批 bench 可延迟
调用 take 检查保持，但这不自动等于 Queue 资源背压路径已经执行通过。
abort 不必清除物理 PE 内容；它令旧结果不可再取，下一 begin 原子清零。
reset 初始化 IDLE 和零计数/PE；中断 ACTIVE/RESULT 后不得发布旧结果。

## 周期、II 和计数定义

第一版 PE 是组合 `fp_mul -> fp_add` 加一个累加状态，没有隐藏 pipeline 或
浮点流水线 flush；每个被接受 step 在一个 rising edge 提交。目标 step II=1，
成立条件是调用方每拍供数且整个事务可提交。它是周期语义目标，必须由生成 C++
记录证明；没有综合/时序结果时不能转写成 MHz 或物理吞吐。

以 begin 被接受的边沿为 `e0`，无 stall 的 K 项 tile：

| 边沿 | 动作 |
| --- | --- |
| `e0` | begin，清零 |
| `e1 .. eK` | 按 index `0 .. K-1` 依次 step |
| `e(K+1)` | end |
| `e(K+2)` | 最早 take；Work 读取 end 后已提交 RESULT |
| `e(K+3)` | 最早下一个 begin |

因此单 tile 占用 K+3 个动作边沿，begin-to-next-begin 间隔 K+3 周期；
begin 到最早 take 是 K+2 周期间隔。每个无动作/被阻塞的 ACTIVE 边沿及 RESULT
等待边沿分别增加 1 周期。没有 tile overlap、双缓冲或 begin/take 同拍旁路。
观察以成功 Work 和 rising-edge commit 定义；不能把 falling/held sample 重复算作 step。

D64 满 QK：K=64，16,384 MAC，32,768 算术操作；tile cadence 67 周期，
理想均摊 16,384/67 MAC/周期。PV 的一个 16-token×16-output-dim slice：K=16，
4,096 MAC，8,192 操作，cadence 19 周期。D64 输出需要四个独立 PV slice；
其调度和结果存储不在本块实现范围。

`accepted_steps` 每次实际 step commit 加 1；`useful_macs` 每次加有效 cell 数，
最大 64×256=16,384，u32 足够；结果给出的乘法数、加法数都等于 useful_macs。
容量 slot 数为 `256*accepted_steps`，不把 masked slot 计为有效计算。
独立 bench 同时记录 begin/end/take edge、ACTIVE stall、RESULT wait、abort 和
成功 tile 数。计数从实际成功边沿导出，不能把 host wall time、请求次数或固定
结束 epoch 当作已完成运算。模块不增加与功能无关的自由运行性能寄存器。

## QK / PV 调度责任

QK：`r=query lane`，`c=key lane`，step `k=d`，left=Q[:,d]，right=K[:,d]；
按 d 从 0 到 D-1 累加。scale、score 屏蔽表示、max/exp/sum 由后续阶段负责。
PV：`r=query lane`，`c=output-dimension lane`，step `k=token`，left=P[:,k]，
right=V[k,:]；按所提供 token 顺序累加。本块只算零初值的 sum(P*V)。
P0S online rescale 的 `add(mul(acc,a),mul(b,v))` 不能自动改写为 tile reduction；
跨 tile rescale/结合顺序及精度合同需要另行提案与 oracle，不能宣称逐位等价。

indexed 首期只启用一个 query 行，其余行 mask=0；输入列表顺序保持，不能将不同
query 的索引列表直接拼成 dense tile。将来共享不同 query 的 KV，必须由调度器
显式形成 token 对齐与 membership mask，不能只因为 tag 或地址相同就共享。

## 分解与首批验收

1. **结构与 ABI**：独立接口 source unit；显式 16 个 Row 实例，每 Row 显式 16 个
   PE 实例。使用普通 typed Module/Method、局部变量与顺序赋值；Module-scope
   Python loop、动态实例列表、Method-owner Collection 不作为实现依赖。
   PE 只有 clear/accumulate/read 类内部 callable；Row 返回包含 16 个相关值的 Struct。
2. **算术与原子性**：独立 host exact oracle 覆盖非融合舍入区别、消去、正负零、
   FTZ 与边界；测试 256 cell 映射、不同 row/column 操作数、K=1/16/64、非矩形
   mask、全空 mask、逐步 mask、两 tile 隔离、错误 tag/index/length、早 end、
   stall、RESULT 保持、ACTIVE/RESULT abort、reset 和重新开始。报告触发次数。
3. **执行与结构证据**：独立 compile/link，默认预算；同一 verified final 双 emit；
   生成 C++ workers1/2 逐位结果/周期一致，确认恰好 16 Row / 256 独立 PE owner
   及算术链。源码调用计数不能单独证明 final 保留该结构。按用户要求不跑 RTL 仿真。
4. **背压和负例**：在该候选确实支持的标准 Queue 调用路径测试 take+push 原子保持；
   若编译器不支持该组合，记录具体 stage/diagnostic 为实现缺口，不添加手工 ready
   wrapper、不声称此项通过。X/Z 控制失败必须全局零提交；域外 NaN/Inf 不算数值通过。

顺序是：本提案 → 独立分解/root 审查 → 最小 source/compile 证明 → 独立 oracle
与生成 System 验证 → 更新真实证据。完整 16×16 默认预算不足时保留为编译器缺口，
不缩水为少量 PE、提高预算、复用 P0S 标量 loop 或把 oracle 结果放入 DUT。
物理面积/Fmax、SRAM bank/端口、总线带宽、pipeline、DMA tag/drain、softmax 和
全 attention 性能验收均留给后续有界任务，不能由此次通过推断。

## Numeric binding dependency and source-baseline exception

The current frontend rejects a direct `@ac.module` call from inside a Rule body. The
array therefore depends on a bounded owning-source refactor before implementation:
`fa2_numeric.py` may add ordinary typed pure helpers (including widening helpers and
`fp_add_value`/`fp_mul_value`) containing the existing arithmetic expressions;
`@ac.module` `fp_add` and `fp_mul` remain public wrappers over those helpers, with
all other numeric modules unchanged. The array calls the published pure helpers from
its PE callable. This is one implementation of each operation, not a second numeric
route or a compiler special case. If the pure helper form is rejected, the array
implementation stops and records the exact frontend stage/diagnostic.

This is a source-baseline exception, not permission to alter P0S behavior. Before
array acceptance, compare the pre-refactor and post-refactor arithmetic AST/bit
semantics, rerun the existing numeric, numeric_exact and original six-System gates
with the unchanged compiler, and record both source hashes. Any mismatch blocks P0T.
The P0S files remain behaviorally frozen; their new hash is explicitly identified
rather than hidden as an unchanged-source claim.


## 当前执行记录（2026-11-04）

首批完整 16×16 System 使用原始默认预算。numeric、array interfaces、array 和
bench 四个 source unit compile 均返回 0；link 返回失败：

```text
transaction state analysis work budget exceeded
(requested 6144, used 1047468, limit 1048576, phase transaction plans)
```

实际失败记录：`docs/gates/logs/fa2-p0t-array/20261104T013007/manifest.json`。
源文件与 installed toolchain 的前后 hash 保持一致。没有缩小阵列、提高预算或
注入 host 期望；final 中 256 owner 的结构检查、双 emit、C++ workers1/2、实测
II/周期/有效 MAC 统计均 **NOT RUN**。Queue 原子背压、中途 reset、X/Z 零提交
也未执行；Verilog 仿真按用户要求排除。检查方法见
[ARRAY-1003](../l3/fa2-core-array-1003.md)。

numeric pure-helper 重构已完成，候选 SHA256 为
`d1e48bb0c99fbc97e93eede5d9f91a3c6a708b3d38b621c6b3c46a240799ddc7`。
独立 AST 审查确认原算法仅作 helper 名称替换，四个 public Module wrapper 保持
原签名；其余 numeric 算法不变。新的 numeric/RTZ/exp 回归已通过 C++ workers1/2，
32 个 numeric、32 个 exact case、edge 与 4,096 点 exp sweep 均完成，双 emit 保留；
记录为 `docs/gates/logs/fa2-p0t-start/p0s-numeric-regression-20261103T172532.497260Z/manifest.json`。

原 P0S 六个完整 System（tensor/layout/recovery/lifecycle/vectors/faults）、product
静态库、standalone row 和每 worker 21 个 controller case 已全部重跑通过；使用
默认预算、同 final 双 emit、C++ workers1/2。独立复查全部 12 个 System 终态及
事件，原 35 文件 P0S closure 和 installed 工具在执行期间不变；相对旧基线唯一
源码差异是上述 numeric 重构。记录为
`docs/gates/logs/fa2-p0t-start/p0s-regression-20261103T172532.554329Z/result.md`。
这些是 P0S 回归证据，不能将尚未链接的 P0T 标成运行通过。

后续通用入口快照复用已通过302项编译器专项和P0S完整回归，但保留原始阵列闭包时
仍超预算。将测试输入改为14组共享向量、保留全部235条指令后，四个unit再次compile
通过，link仍在transaction plans处失败（requested1024、used1048429、limit1048576）。
最后记录为 `docs/gates/logs/fa2-p0t-array/20261104T014911/manifest.json`。
输入等价审查、工具链身份及当前未运行项统一记录在
[VALIDATION.md](../../VALIDATION.md)；本页的K+2/K+3仍是合同值，尚非阵列实测值。
