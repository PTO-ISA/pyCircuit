# C2-R1：统一寄存器、rule proposal 与 Xfer

修订 B。日期：2026-09-28。状态：精确设计候选；当前审阅与批准状态见工作包。

用户本轮明确要求：DFF、DFFE 与 RTL 寄存器在 ACIR 中统一为 `ac.reg`；
rule 读取 reg 或 reg collection，产生 next proposal；rule 内部无状态，
可以拆分并行；只有 Xfer 提交状态，未使能的 proposal 在 Xfer 丢弃；
commit/discard 由 C++ 寄存器原语实现，producer 不承担输出反压；
模块之间通过 reg 连接；SimQueue 退役，后续 `ac.queue` 降到由寄存器
构成的 circular-buffer 模块库。

这些方向直接采纳。本文件补全 IR 签名、时序、冲突、reset、并行、
删除顺序与验收细节，不把方向请求记作这些新增细节已经获得精确批准。
原 [C1](c1-pythonic-source.md)、[C2](c2-mlir-contract.md)、
[C3](c3-driver-runtime.md) 和批准记录保持冻结；产品代码尚未切换。

## 来源与修改范围

目标主仓为 `7e5ffdc22416e8e63bf66cbee9dfa049a7614c41`；隔离实现基线为
`82f161ea95336fdd15c238e103f13b9f823e8f28`。当前未提交 source-link
接纳工作不视作本提案的实现。

最新相关 donor 为本地 GFSIM 功能分支
`b852ed83fa0288d0be7406bba0ed47be4b2c0f63`。本轮核对其
`include/SimDFF.h`、`docs/API/SimDFF.md` 和寄存器测试；相关产品文件
无 dirty 修改。远端 main `b9c2ec2704101d91f515a1a7ed69d2e3b02dbdfb`
为更早的 bootstrap，尚无该 DFF/DFFE 实现，不能以远端 main 名称替代
实际较新的组件基线。

| 来源事实 | 本提案采用或适配 |
| --- | --- |
| SimDFF Write 暂存 D；Xfer 要求恰有一次 Write | 只作为已证明恒使能、完整驱动的 C++ 特化 |
| SimDFFE Write 暂存 D/E；Xfer 按 E 提交并清 pending/E | 作为全部 ac.reg 均可使用的基础实现 |
| disabled proposal 不在下一周期重放 | 作为共同硬件语义，不由 producer 手动过滤 |
| donor Reset 直接修改 Q | 改为寄存器原语内部的 reset transfer，保持 Q 只由 Xfer 发布 |
| donor 寄存器方法为单线程 API | 只并行计算独立 proposal，join 后每个 owner 单次驱动 |
| pyc.reg 已有 clk/rst/en/next/init 到 q | 仅复用为 RTL 私有 lowering 目标，不成为第二语义权威 |

精确取代 C2 的 `ac.dffe`/`!ac.dffe<T>` 普通状态合同、rule 的空
readiness/conditions/admission 区域，以及 C3 中普通状态的提交实现。
保留数学整数、源错误、来源、StateRef/StateID、header-only parent、
逐源 producer、双后端和 SDK 边界。

## 一个寄存器语义

每个物理寄存器恰有一个 `StateID`、一个 declaring owner、一个 reset
image 和一个 clock/reset domain。Q 是 committed current；D/E 是当前
transfer 的候选，不能通过读取 D 得到另一条 rule 的中间结果。

在本包的 default domain 有效上升沿，定义：

```text
reset = 1       : Q_after = initial
reset = 0,E = 1 : Q_after = D
reset = 0,E = 0 : Q_after = Q_before
```

reset 为同步、高有效，优先于 enable。正常成功 transfer 后，pending
与 E 无条件清零。没有 proposal 等价于本周期 E=false；discard 表示
本轮候选不可再提交，不表示清零 Q，也不表示将候选延期。

D 在 E=false 时不可观察，可以使用类型正确的 placeholder。计算 D
时已经被求值的 source assert/range/index/division 错误仍按 C1/C2
报告；E=false 不能掩盖这些错误。

本包不接纳派生时钟、异步复位、多域混接或 X/Z 数据。后续多域扩展
必须保持相同 register contract，并定义每个 domain 的 transfer 边界；
本包不得用默认域静默接纳其他时钟。

## ACIR 的五个接口及 SSA 组织

统一 op/type 为 `ac.reg` 和 `!ac.reg<T>`。T 采用 C2 有限 bool、integer
或 nominal record 的 payload representation；没有 queue、pointer、
proposal、clock 或 reset 可以作为 T。

| 接口 | Source/linked/final 中的表示 |
| --- | --- |
| clk | ac.reg 的第 0 个 i1 operand，必须是所属模块的 clock control argument |
| reset | ac.reg 的第 1 个 i1 operand，必须是所属模块的 reset control argument |
| Q | 由 reg handle 绑定到 rule computation 的只读 payload argument |
| D | owning module 的最终 yield 中对应 commit target 的 data operand |
| enable | 同一 pair 的 i1 enable operand，经过 source validity 与冲突检查 |

声明先产生 state handle，rule 再由 Q 计算 D/E，最终 module yield 绑定
D/E。不要求 `%q = ac.reg %d` 形成从 Q 到 D 再回 Q 的非法 SSA 环。
也不增加第二种 storage op、公开 proposal type 或 runtime queue。

`ac.reg` 无 region，operands 恰为 `(clk:i1,reset:i1)`，一个
`!ac.reg<T>` result。所有阶段均必须有 `name:StringAttr`、
`ac.source_owner:SourceOwner`、`ac.declaration:Occurrence`、
`ac.element:Array<u64>`、`ac.domain="default"` 和有效 source `loc`。
state identity 取 declaration/element 与实际实例 owner，不取 name。

| 阶段 | 必需属性 | 禁止/替换 |
| --- | --- | --- |
| source / linked semantic | `ac.logical_element:LogicalType`、`ac.shape:Array<StaticExpr>`、`ac.initial_value:InitialSpec`；shape 恰为 `[]` 或 `[literal(N)]`，N>0；element 恰为 `[]` 或 `[k]` 且 k<N | 禁止 `ac.logical_type` 和 payload-typed final initializer；本包不发布未闭合 shape |
| final hardware | `ac.logical_type:LogicalType`、`ac.initial_value` 为该单个元素的完整 payload-typed reset image；保留 ac.element | 移除 `ac.shape`、`ac.logical_element`、InitialSpec 及其 symbolic expressions |

source 已展平的每个 ac.reg 只拥有一个物理元素；其 source `ac.shape`
明确记录**原逻辑声明**的形状，不表示又分配一个数组。相同 declaration
的元素共享完全相等的 shape、logical_element 和原始 InitialSpec。
source verifier 按 declaration 分组，要求 element 集恰为 `0..N-1`，
无重复/缺失；scalar declaration 恰有 element=[] 的一个 reg。
T 在所有阶段必须精确对应该元素的 LogicalType representation。

initializer 沿 C2 的 scalar/repeat/elements/expression 四类定义。
materialization 按 `(SpecKey,declaration)` 恰求值一次完整 initializer，
验证静态类型、长度、字段与区间，然后由每个 reg 的 element[k] 选择
第 k 个值。repeat 求值一次再复制；elements 按源顺序各求值一次；
expression 必须返回恰 N 项的 list。不能逐 reg 重执行整个 initializer，
也不能从遍历顺序或 reg 的 name 选择初值。scalar 的 element=[] 直接
取得 scalar initializer。转成 final 属性前，转换校验核对每个元素的
实际 image，才移除 source initializer；final 自身继续核对物理类型、
完整 image 与范围，不声称能从被一并篡改的合法 final 恢复原 Python。

模块 control argument 固定在 body argument 0/1，新增必需闭合属性：

```text
ac.control_ports = {clock=0:u32, reset=1:u32}
```

`ac.ports`、`PortBinding.port` 继续只计数据端口；数据端口 j 的实际
block argument 为 2+j。module import 与 implementation 必须同时
声明这个控制前缀。instance operands 为 `(parent_clk,parent_reset,
data_reg_handles...)`；data bindings 不把控制前缀算成两个数据端口。
只有 selected root 从运行环境取得 control pins，子实例必须 identity
forward 相同 domain 的控制值；不允许通过 arithmetic、constant 或
普通 reg Q 伪造 clock/reset。control pins 不属于模块之间的数据连接，
也不能成为 rule 的数据输入或 source bool 的类型证明。

下例是省略 C2 来源/类型/proof 属性的结构图，不是可直接运行的 MLIR
测试输入：

```text
module(clk, reset, input_reg, output_reg):
  local = ac.reg(clk, reset) initial=7
  rule inputs=[input_reg, local], targets=[output_reg, local]:
    body(input_Q, local_Q):
      ... verified value computation ...
      yield(output_D, output_E, local_D, local_E)
  module yield(... pairs bound to exact commit_targets ...)
```

final verifier 从实际 arguments、handles、instance graph 和 yield SSA
核对五个接口，不能仅相信上述属性或 `ac.stage`。

## Rule：只读快照到 proposal

`ac.rule` 只有一个 computation region；删除 readiness、conditions、
admission region。现有 name、registration、source ownership、来源、
proof/check/use 与 input/output binding 属性保留。

其 operands 分成两个明确的 variadic segments：inputs 和 targets，均为
`!ac.reg<T>` handles。targets 仅标识 proposal 的目的地，不授予 rule
执行寄存器 Write/Xfer 的能力。原 output segment 改名 targets；
`ac.output_bindings`、`ac.output_types` 保留 C2 含义作为目标描述。

computation block 每个物理 input 恰有一个 T 类型的 Q argument；
target 数为 w 时，终结 `ac.yield` 恰有 `2*w` 个交错的 `(D,E)`。
source 与 final rule 均产生这 `2*w` 个 SSA results，不再保留 source
rule 零 results、final rule 有 results 的两套 arity。每对结果必须与
同 ordinal 的 target、LogicalType 和 RequiredUse 绑定。

规则如下：

- 所有 runtime 数据输入来自 reg Q 或固定 reg collection 的 Q。
  source 静态参数、literal/default 和纯 helper 常量在编译期绑定，
  不为它们分配虚假 reg，也不产生第二种 runtime 输入接口。
- rule 内只有 SSA 临时量、纯 helper、受保护的控制流和 source checks；
  不分配 persistent state，不访问 next，不隐藏 mutable capture。
- rule computation 可以含 source check 效果，因此整个 `ac.rule` 不
  标为可投机 `Pure`；无 persistent state 不等于错误可被删除。
- 同一 Q 在一个 evaluation epoch 内不可变化；rule 顺序和并行调度
  不影响其读取值。同一 source read 的来源由重基后的 L1 载体保存。
- 缺写分支给出 E=false；没有目标的 rule 可以有零对输出。A2 的原始
  next use、helper-return、源求值路径及目标证明仍须先保存再降低。
- producer 不访问 output Full/Empty/CanTransfer/ready/pending；寄存器
  不返回写入是否被接纳，也不保留等待后续成功的输出队列。

## Collection 与 source interface

本包首个精确可执行 profile 为 scalar 及 source compile 时已知长度的
一维正长度 fixed reg collection。collection 是有序的 reg identity
列表，没有额外容量、读消费、隐藏状态或 SRAM 推断。

source 单元即将该 profile 的 collection 展开为逐元素 `ac.reg` 和
扁平 rule/instance handles。顺序按 C2 declaration/element ordinal；
header 的 Parameter、Connection.elements、PortSlot 与 StateRef.ordinal
保留 source 参数和元素对应关系，不新增 collection runtime ABI。
读 alias 可以共享一个物理 Q argument，但必须保留每次 source read
occurrence；两个源 ordinal 的可写 alias 继续按 C1 拒绝。

例如 `self.cells: list[Word] = [3, 7, 11]` 的三个 source ac.reg 都引用
同一个 declaration D，shape 都是 `[literal(3)]`，InitialSpec 都是
`elements([3,7,11])`，element 分别 `[0]`、`[1]`、`[2]`；最终 image
分别为 3、7、11，reset 后逐项恢复。name 可用 `cells[0]` 等可读标签，
不能以标签代替 element 或改变声明次序。反例从同一未改的权威
initializer 出发，篡改一个 element、交换转换后的 image、重复 element、
缺元素、错误 initializer 长度或重排 initializer 的某个副本，必须在
source/link/materialization 对应边界拒绝；合法修改原 Python 列表
形成新模型，不属于此篡改拒绝主张。

首片静态下标直接选择元素，先检查 integer kind 与 bounds；bool、负数
和越界拒绝。已批准的动态索引目标保留到后续完整 carrier：动态有界
下标先验证 bounds，再由同一次 index SSA 选择 Q，写入产生每个元素的
`E_k = source_E && index == k`，D 来自同一次 replacement 求值。
完整 selection RequiredUse 仍以 C2 的有序 StateRef 列表绑定；A2
尚未定义的动态 source carrier 不由本包的展开说明冒充已冻结接口。

C1 已批准的 constructor 参数依赖 collection shape 仍是必须完成的
能力。它需要能跨逐源编译保存 generic collection 的精确 carrier；
本包不凭一个尚无具体长度的 tuple 假称已经支持。未闭合长度在首片
给出明确 capability rejection，不能将调用方某个实际 N 作为 header
的 generic shape，也不能把该实现限制记为永久退役。

## 模块连接与唯一 owner

所有模块间 runtime 数据连接都是 reg 或上述 reg collection 的引用。
转发必须保持同一 StateID，不新增 relay register、复制 reset image
或增加周期。多个 reader 合法；writer 集合必须完整核验。

子模块接收 current snapshot/只读 view，返回属于所绑定目标的
proposal。borrowed reg 不由子模块分配、Reset、Write 或 Xfer；
它的物理 owner 汇总自己和 descendants 的 proposal，并且恰提交一次。
子模块自己声明的 reg 仍由子模块拥有。

普通纯值 helper 不是 module 连接；它不能被包装成跨 module 的零周期
数据旁路。本轮的 reg-only 模块连接方向取代前轮尚未定案的组合模块
端口建议。memory、外部 typed DUT 与多时钟合同需要据此重基；本包
不将未批准的外部 wire 输入偷偷包装成增加或减少一拍的假 reg。

## 多 proposal、检查与并行边界

多个 source uses 可以到达同一目标，但不建立先到先得或最后写入胜出。
每个 owner 从已验证的贡献集合形成唯一 `(D,E)`：

- 无 active contribution：E=false。
- 恰有一个 active contribution：采用其 D，E=true。
- 可证明互斥的多个 contributions：按相同 enables 选择数据并取 OR。
- 必然重叠的重复写编译拒绝；可能重叠的普通 driver 按 C1 产生
  precommit conflict check。动态冲突为模型错误，不是背压或仲裁。

source/check/driver fatal failure 阻止本轮全树 Drive/Xfer，并进入 C3
Failed。不能将以后 FIFO 的局部满条件放进全树 fatal guard，也不能
因为一个 reg E=false 就阻止其他合法 reg 提交。

执行顺序固定为：

```text
freeze current Q snapshot
  -> evaluate rules/children into independent proposal buffers
  -> join; verify source checks and all target conflicts
  -> each physical owner stages exactly one D/E per register
  -> barrier; Xfer each owned register exactly once
  -> publish observations and successful-cycle counter
```

rule 计算可以并行；每个 worker 只写自己独占的临时输出。不能让多个
worker 并发调用同一 SimDFFE::Write。Xfer 期间不运行 rule、不采样
部分更新的树；初期 Xfer 串行即满足并行语义，真实并行 Xfer 须另证
disjoint owners 与全树观察屏障。拆分 rule 不改变 registration、source
error 顺序、UseID 和冲突集合；无法证明这些不变时不得拆分。

## C++ 寄存器原语

所有 ac.reg 初期可以统一实现为 `SimDFFE<T>`。generated producer
生成 proposal；owner 只调用 `Write(D,E)`，包括 E=false 的 pair。
决定 commit、hold、discard 和清空 pending 的代码位于寄存器原语，
不能由 producer 的 `if (E) Write(D)` 代替该语义。

沿用方法名 Q/Write/Xfer/Reset；增加私有 runtime 清理方法
`DiscardNext() noexcept`，只清 pending/E、不改变 Q，用于失败清理。
不暴露 Python、ACIR 指令或 generated DUT API 的 discard/reserve 接口。

```cpp
// Contract pseudocode; ordinary no-fail payload publication is required.
void Xfer() noexcept {
  if (reset_pending_) q_ = initial_;
  else if (pending_ && enable_) q_ = d_;
  DiscardNext();
  reset_pending_ = false;
}
```

完整 runtime `reset()` 保持 C3 的调用后可观察初值，但内部执行专用
reset transfer：禁止 rule 求值，丢弃旧 proposals，设置 reset_pending，
经各原语 Xfer 恢复 Q；成功后时间/统计为零，不计一次正常 step。
普通 Reset 方法不能另开一条直接赋 Q 的执行路径。construction 建立
初始对象，不属于运行期状态更新。RTL 对应完成一次 asserted-reset
有效边沿后的观察点。

SimDFF 只允许作为同一 ac.reg 的实现特化：final verifier 证明每个
成功、非 reset 周期都完整产生恰一份 E=true proposal。仅看到局部
`true` 或某个已覆盖分支不足以证明。条件写、缺 proposal、enable
可能为假时使用 SimDFFE。两种实现共享相同 reset-transfer 规则；
DFF 在普通无驱动 Xfer 的断言只是内部 invariant tripwire。

寄存器 payload 为不可变逻辑值。普通大 record 可以沿用 donor 的
immutable shared handle，不将 host 指针身份或 nullable queue slot
变成 IR 语义。正常 Xfer 的复制/句柄交换必须无分配且不抛异常；
不满足 no-fail assignment 的类型不进入生成模型。unexpected host
故障仍按 C3 处理，不能当作正常模型分支或用来证明事务原子性。

## RTL 映射与故障边界

verified ac.reg 经单向 RTL-private legalization 映射到现有
`pyc.reg(clk,rst,E,D,initial)->Q` 及 `pyc_reg.v`。legalizer 必须核对
实际 StateID/owner、D/E、clock/reset 和初值，不重新计算驱动或调度。

```verilog
always @(posedge clk) begin
  if (reset) q <= INITIAL;
  else if (enable) q <= d;
end
```

source/check fatal 行为保留 C2/C3：失败边沿不能更新任一 reg，C++
进入 Failed，复用前必须 Reset。final verifier 从全部 RequiredCheck
和 driver-conflict checks 重建本轮 fatal，RTL-private verifier 核对
所有实际 commit enable 都被本轮非 fatal 条件保护；不能仅发 `$error`
却仍更新其他 reg。reset 边沿跳过普通 source 求值并恢复初值。

为保持逐源 RTL module 层级，允许 RTL-private legalization 从已验证
共同 IR 物化以下**无状态 compiler-control 投影**。它不改变 source
module 的两项 control 前缀或 reg-only 数据端口，不进入 Python/SDK：

```text
local_error[m] = OR(每个本模块 check 的 path && !condition,
                    每个本模块 driver-conflict predicate)
subtree_error[m] = local_error[m] OR OR(subtree_error[child])
root_commit_ok = !subtree_error[root]
physical_enable[r] = root_commit_ok && proposal_enable[r]
```

每个 RTL 模块有一个私有 i1 `subtree_error` 输出向 parent 汇总，以及
一个私有 i1 `root_commit_ok` 输入向 descendants identity-forward；
root 的许可输入绑定到自身归约后的 `!subtree_error`。私有端口角色由
legalizer 记录并与实例图核对，不以拼写识别；名字按 C3 名称
legalization 与碰撞拒绝规则生成，任何碰撞在发射前拒绝。
它们只传 source/check 合法性，不能承载模型数据、
容量、ready、admission、winner 或持久状态，也不新增周期。

error 计算必须使用**尚未乘 root_commit_ok 的** source path、valid
和 proposal enables；rule/child error 求值不能依赖许可，否则会形成
组合自依赖并掩盖冲突。root_commit_ok 只参与最后物理 E 门控，不能
反馈到 source demand、check path 或原 proposal 的计算。reset=1
时普通规则不执行，local_error 为 false；各 reg 的 reset 分支直接
优先恢复 initial，不受 root_commit_ok 限制。

共同 final verifier 建立实际每实例 check 和 commit-target 闭包。
RTL-private verifier 对照此闭包验证：每项 local check 恰进入所属
subtree 归约、每条 child 上行边/下行边完整、所有 owner 的实际 E
都受同一个根许可保护且没有旁路。跨域、漏 child、反相许可、遗漏
某个 commit endpoint 或把已门控 E 用来生成 error，均拒绝。不能只
检查存在一个名为 root_commit_ok 的 wire。此投影与 C++ 全树 Check
barrier 实现相同的既有全树 fatal 合同，不引入另一个调度器。

前轮审查发现的“RTL source fault 后再次非 reset 边沿”仍需精确的
共同故障合同。本包不暗中增加 backend-only fault latch 或把它藏在
属性中；需要持久故障状态时也必须在共同 IR 中由 ac.reg 明确表示，
并冻结其控制 owner/身份与观察接口。该补充是完整双后端故障验收的
前置依赖。本包可验证故障当拍和紧接 Reset 的合法序列，但不得据此
宣称任意故障后执行已等价或关闭完整 M2。普通 reg 的无故障轨迹和
enable/discard 行为不依赖新增故障状态。

本包没有 typed DUT ingress；可恢复输入拒绝不能与 source fatal
混同。C++ 内部实现异常也不是需要硬件仿真的一种输入值。

## SimQueue 退役与 FIFO 库边界

最终产品移除 SimQueue runtime、queue pointer 端口、Work 阶段的隐式
CanTransfer/Full/Empty/Pop/Write 调度、QueueGraph 语义引擎，以及
后端绕过 reg 库直接生成私有 FIFO primitive 的路径。

后续 FIFO 库是普通、可独立编译的 Pythonic module：cells、head、tail、
count 及必要 valid/接口状态由 ac.reg 构成。满空、容量、读写时序、
同拍操作与协议是库的显式算法；库内 rule 仍无状态，状态属于 reg。
`ac.queue` 只作为已批准高层语义入口，MLIR 将其 lower 到该库的实例
与 reg 连接；到共同 final IR 时不允许残留 ac.queue/SimQueue 调用。

本包不发明 FIFO 的源 API、ready/valid 协议、bypass、capacity、latency、
Slot/Table 或多 lane 规则。这些需要下一份库合同及独立 deque oracle。
producer 无隐式反压不意味着 FIFO 可以静默丢失数据；是否写入、
如何表示满空/接收结果由库协议明确，不能把 disabled reg proposal
当成自动重试队列或全局事务成功。

迁移顺序为：先 ac.reg/rule 闭环，再库设计及 queue-to-library lowering，
最后同一 hard-break 候选删除旧实现。库未覆盖的旧 queue 能力继续
列为切换阻塞，不能提前删除 oracle，也不能以兼容 fallback 进入新链。

## 受影响合同与调用方

| 对象 | 必需变更 |
| --- | --- |
| C2 普通 storage、rule、module、instance | DFFE 名字/type 改为 reg，控制前缀、单 computation region、统一 result arity、唯一 commit owner |
| A2/L1 待批提案 | 以 reg handles、Q snapshots 和 proposal results 重基；保留完整 read/use 原始事实 |
| C3 lifecycle/generated C++ | 原语内 commit/discard/reset transfer、DFF 特化证明、并行 proposal 屏障 |
| C3-DUT-IO 待批提案 | 重基 reg-only 连接、control 前缀与 source-fault 策略；外部采样语义单独保持精确 |
| Decisions 0236/0237、0238–0241、0262/0263、0280 | 普通 reg rule 不再继承隐式资源 firing；保留能力迁移到明确库算法/后续合同，逐条写 supersession |
| Decisions 0264、0270、0274–0278 | reset、header/owner、逐源与特化继续成立；具体旧 storage/source carrier 在 M5 更新 |
| Candidate ODS/types/verifiers/importer/headers | 同名 schema 一次 cutover；拒绝 ac.dff/ac.dffe 和旧 rule regions，不增加兼容 reader |
| C++/RTL emitter、runtime、CMake、SDK | 一个 ac.reg 语义链；保留 RTL 私有 pyc.reg，删除旧 C++ emitter、SimQueue 与已迁移 queue targets |
| Tests/examples/docs/gates | 逐项继承 Q/时序/reset/冲突/来源断言，旧拼写改负例；不能只搜索替换名称 |

### 分阶段删除与替代清单

下列路径以 repository-relative 表示；R1 指隔离候选的 schema/普通
寄存器切片，M5 指具备全部替代证据的最终产品候选。

| 阶段 | 路径或注册入口 | 替代证据与删除条件 |
| --- | --- | --- |
| R1 | `compiler/acir/include/acir/Dialect/ACIR/ACIRTypes.td` 的 DffeType；`ACIROps.td` 的 DffeOp 与 RuleOp 多区域定义 | RegType/RegOp、单 computation region、controls/result/binding 正反例通过后，一次替换并拒绝旧 assembly |
| R1 | `compiler/acir/lib/Dialect/ACIR/ACIRPacketTypes.cpp`、`ACIRModuleOps.cpp`、`ACIRModuleContracts.cpp`、`ACIRRegistration.cpp` 中 DFFE/type/旧 region 分支 | 新 reg verifier 与注册；旧类/旧 diagnostic 接纳路径无活跃引用 |
| R1 | `compiler/acir/lib/Compiler/PythonImportModuleBody.cpp`、`PythonImportRules.cpp`、`SourceHeaderRegistry.cpp`、`SourceBodySnapshots.cpp`；`ACIRMathOps.cpp` 的 DFFE authority 查询 | 真正逐源 reg/header、element/reset、input-slot 与 source-use/read 来源门槛；无旧类型 fallback |
| R1 | source-unit harness 和 `tests/cpp/agentic-circuit/Dialect/ACIR/` 中旧 DFFE/schema 正例 | 同一语义的新 reg 正例与旧拼写负例；保留所有原始 oracle/边界责任 |
| R1 | 新 donor-derived runtime 内 `SimDFF/SimDFFE` 的直接 Reset 改 Q 路径 | 改为 reset transfer；两个 C++ 类允许作为同一 ac.reg 的内部实现，**不按名字删除 SimDFFE** |
| M5/FIFO | 新 runtime 及 generated code 中 SimQueue、BindInput/BindOutput queue ports、AdvanceInnerQueues/ResetInnerQueues、CanTransfer/Pop/Write/Flush 的 queue 分派 | 具体 FIFO 库与 ac.queue-to-library 全量功能/双后端 oracle 通过；无未分类旧 Queue 能力后删除注册、头文件、链接/安装资产 |
| M5 | `compiler/acir/lib/CodeGen/QueueGraphPlan.cpp`、`QueueGraphGenerator.cpp`、`QueueGraphPyc.cpp`，`compiler/mlir/lib/Emit/CppEmitter.cpp` 及调用/构建目标 | 新共同 IR/backend/SDK 完整闭环后删除；旧路线 passing logs 不作为替代证据 |
| M5/FIFO | `compiler/mlir/include/pyc/Dialect/PYC/PYCOps.td` 的 fifo 分派、`library/verilog/pyc_fifo.v` 及 catalog/build 使用者 | 库 lowering 覆盖且无活跃使用者后退役；本轮不提前删除仍服务旧产品的文件 |
| 保留 | RTL-private `pyc.reg`、`library/verilog/pyc_reg.v` | 只由 verified ac.reg 单向 lowering 使用，D/E/control/initial 对应验证通过；不保留旧 PYC C++ 产品路线 |

R1 实施 owner 必须在冻结候选补充这些入口的精确反向引用、CMake
targets 与 installed inventory；上表不伪称 M5 的全仓删除已经执行。

## 验收与实施包

独立 oracle 不调用被测 emitter 或 SimDFFE 来计算期望。第一批交付
必须贯穿真实 Python source、每源 header/body、link/final、C++ 与 RTL。

| 包 | 交付及独立门槛 |
| --- | --- |
| R1-IR | reg/type/control prefix/rule schema、unit/link/final verifier；反例含错误 owner/clock、旧 DFFE、交换同型 targets、残留 queue |
| R1-RUNTIME | Write 不改变 Q；Xfer commit/discard；reset transfer；DFF presence 证明；普通 commit no-fail |
| R1-RULE | 同一 Q 快照、不同 traversal/worker 顺序、无 hidden state、冲突在任何 Write 前失败、collection ordinal |
| R1-BACKENDS | 同一 program.ac 两后端逐拍一致；孙模块故障时父/兄弟均不提交、紧接 Reset 恢复；断开/取反/漏接 fatal control 与 commit guard 的对应 verifier 拒绝；完整故障后 continuation 单独依赖待补共同合同 |
| R1-CUTOVER | 隔离候选的原样 Accumulator/Core 与静态 concrete collection 通过；R1 当期旧 schema/region 拒绝；不代表 M5 安装资产删除或完整项目切换 |
| FIFO-FOLLOWUP | 精确库提案先审阅；容量 1/3、wrap、满空、同拍操作、reset 与独立 deque；通过后才替代相应 ac.queue 路径 |

寄存器 oracle：reset image=3；Write(99,false) 后 Q=3，Xfer 后 Q=3；
下一无 proposal 周期仍 Q=3；Write(7,true) 在 Xfer 前 Q=3、之后 Q=7；
再一个 disabled 周期 Q=7；带 pending 的 reset transfer 后 Q=3，
旧 D 不重放。没有 proposal 的 source reg 按 hold，不产生 DFF 缺驱动
的 source error。

并行 oracle：两条 rule 均读取旧 Q=3，一条提议 A=4，另一条提议 B=6；
所有调度顺序在 Xfer 前均观察旧值，Xfer 后同时观察 (4,6)。两条规则
同时使能同一 target 时，全树本轮保持旧 Q 并进入 Failed；E=false
的独立输出不阻止其他合法输出。collection N=3 分别静态写元素 1/2，
验证旧 disabled proposal 不进入新元素或下一拍。

层级错误 oracle：root 拥有 A、child 拥有 B、grandchild 触发 source
assert，sibling 拥有 C；A/B/C 各自有合法 enabled proposal。故障边沿
四层全部保持 Q，紧接 reset 边沿恢复各自非零 initial。分别断开
grandchild error、漏接 sibling 许可、取反 root_commit_ok，或以门控后
E 构造 conflict，必须在实际 RTL-private projection verifier 拒绝。
修改共同 IR 的 check/commit 绑定则由两个 emit 入口的共同 verifier
拒绝；只修改 RTL 物理网络不假称由 C++ emitter 检查。

拟新增实现门槛名称在实施包中固定如下；文件/targets 尚未创建，
这些命令是验收合同，不是已运行证据：

```bash
cmake --build "$R1_BUILD" --target ACIRRegContractsTests ACIRRegRuntimeTests
ctest --test-dir "$R1_BUILD" --output-on-failure -R '^ACIRReg(Contracts|Runtime)Tests$'
pytest tests/system/test_unified_register_units.py -q
pytest tests/system/test_unified_register_backends.py -q
```

上面是 R1 有界门槛。M5/release 候选完成 FIFO、剩余能力和删除责任后，
才运行对应新路线的完整 gates、installed inventory 审计，以及：

```bash
python3 flows/tools/check_decision_status.py --status docs/gates/decision_status_v6.md --out "$R1_OUT/decision_status.json" --require-no-deferred --require-all-verified --require-concrete-evidence --require-existing-evidence
```

旧决定账本的通过不能代替 R1 新 schema/行为证据；R1 通过也不把
其他尚未闭合能力标为 implemented-verified。

记录真实测试发现数、零 skip 的目标平台结果、Q/epoch trace、并行
次序、source→AC→TU 图、精确 source/toolchain identity。Windows、
四态或未来多域未执行时分别报告，不用基础单元测试替代。

独立设计审阅针对本文件 SHA-256。实施、独立测试、代码审阅使用
不同实例；ODS、shared verifier、CMake 和集成各只有一个 writer。
回退恢复整个隔离候选并从其源码重建，不在新产品中保留 DFFE/Queue
兼容模式。本文件不授权修改 donor checkout 或发布产品。
