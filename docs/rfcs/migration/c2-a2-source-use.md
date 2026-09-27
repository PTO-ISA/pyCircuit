# C2-A2：原始 next 写入与 helper 返回的源 use 载体

修订 A。状态：待独立设计审阅与用户精确批准。独立 architect
u02_ods_design 给出设计，PM 整理。获批 C1-C/C2-C/C3-C/
C2-N1-C 原文保持冻结；本增补尚不授权实现。目标是按既有
U02-B 工作包编译原样 Packet、Accumulator、Core 三份 Python
源，保留每次源写入和返回的来源、实际值、路径及 target，供
MLIR 在 specialization 后独立提取 C2 RequiredUse；不增加
另一套 Python frontend 或 backend 语义引擎。

## 缺口与前后行为

冻结 C2 已定义 ValueID、UseID、RequiredUse、UseTarget、
ac.value.use、ac.yield_bindings，并规定 specialization 后从
实际 source next-assignment/return-target operations 独立提取
RequiredUse；不能从合并的 final yield 猜测原目标。然而 C2
没有给通用 scalar source assignment 明确的序列化 op/属性。
现有 Rules.cpp 直接生成源 rule yield；私有 C++ assignment
列表在 .ac 序列化/重读后不存在。donor 的 next map 还可能按
最后写入覆盖，与 C1 同一路径重复 next 必须拒绝不符。

原始 Accumulator 的条件 self.total 写入与无条件返回是两次
不同 source use；Core 对 left_request 的完整 Request 写入
是第三次。变更后这三次在 source MLIR 中各有实际 SSA 操作、
独立身份和 target，特殊化/inlining 后才经已批准的 proof/
binding 流程转为 final data,enable 方程。局部变量赋值不是
持久 next use；读 self.total 始终读 committed current。

扩展现有 ac.value.use 的 target/results 会混淆原始 use
权威与派生的 final witness。新增一个 source-only operation
是本增补的全部新 op；不添加 dialect type、Python 装饰器、
CLI 选项、runtime ABI、backend route 或隐式仲裁。

## 精确 operation 与闭合 target

    ac.source.use(value:T, valid:i1, path:i1)
        -> (data:T, enabled:i1)
      id:UseID
      source:ValueID
      target:SourceUseTarget
      loc:原赋值/返回位置

无 region、无 symbol、无状态分配。必需属性均不许缺失或错型；
target 为闭合 DictionaryAttr：

    SourceUseTarget =
        {kind="next_scalar", state:StateRef}
      | {kind="helper_result", ordinal:u32}
      | {kind="helper_return", call:Occurrence, ordinal:u32}

前两者由 source producer 写入；helper_return 由验证过的
inline/expansion 在 linked 阶段生成。UseID、ValueID、
Occurrence、StateRef、u32 沿用 C2；UseID.role 对
next_scalar 为 next，对其余两个为 helper_return。这个
初次增补不声称覆盖 C2 的动态 next_selection，相关 source
carrier/提取门槛仍列入后续能力表，不可从本 op 擅自类推。

唯一值方程是 data=value、enabled=path&&valid。
enabled 仅是 next-enable 或 helper-return-valid，不证明某个
i1 是源 bool。inactive data 不可观察，后续合并仍须产生
C2 允许的类型正确 placeholder。本 op 既不写 Q、不提交、
不吞掉源错误，也不提供 winner。不得标为 Pure/可投机/
可随意丢弃；直到独立提取和受证替换完成才可删除。
这里的保留是编译器证据义务，不是新增硬件 side effect。

## 语境、类型与身份

next_scalar 只能处于已注册 rule 的计算区域及其已验证控制
子区域，state 精确解析为 enclosing module 的 owned DFFE
或 formal port 的 scalar/whole-record StateRef。T 与其
LogicalType 的实际 payload representation 相等；源
math_int 须经获批的边界转换后才成为 finite next 数据。
record field 不是独立可写 state，child 内部 state 不可写。

helper_result 只能在 source-owned constructor/value helper body，
ordinal 是 declared data result，而非 hidden valid result。
helper_return 只能在已验证 helper inlining 后、已注册 rule
内部，call 指向该真实展开 call occurrence。其 T 遵守原
ValueConstraint；数学整数/静态列表只在原 C2 限制内暂存，
不会因本 op 获得 runtime ABI。U02-B 只要求原三源的有限
logical result 子集；其他合法大能力仍须显式 capability
诊断直至后续实施。

每个原始 next assignment、每个数据 return ordinal 各产生
一个 use。UseID.origin 指真实赋值或 Return occurrence，
slot 为左到右 target/result ordinal，scalar 为零。
constructor 隐式 constructed-record return 可用 constructor
声明 occurrence 及 result ordinal，不伪造 Python statement。
不同 use 可共享数据 SSA；共用 ValueID 必须在同一未来
ProofScope 内绑定同一实际 value/valid/path 和兼容 logical
meaning，否则创建不同 ID。loc 仅用于诊断，不成为调度优先级
或身份。rule 注册调用 occurrence 和 rule 方法体 Return
occurrence 独立保留。

source ValueID 必须对应 operand(value,valid,path) 的真实
边界值；只放一个孤立 id 属性不能满足义务。verifier 对
next_scalar 从实际 rule output handle、方向、LogicalType、
StateRef 和模块状态声明重算 target，拒绝同型 handle 交换、
错 owner、未声明状态、写 child 内部/record field、错误绑定。
source use 的 data/enabled results 必须沿真实 SSA 到对应的
candidate merge/rule yield pair；不能流向 current read、
另一个 owner 或悬空不用。helper_result 的数据到对应
func.return 数据 operand，enabled 参与 hidden return
valid；helper_return 的结果取代实际 caller call-result 路径，
与 call/ordinal/返回 SSA 精确匹配。修改任一边、增加一个
无关 good marker 均无效。

## 路径、错误、分支和重复写

path 是 C2 的 threaded source use path，valid 是实际
边界值有效性；其 AND 不替代独立 source error path。
继续使用已批准的

    live_after = live_before && (!evaluation_path || result_valid)

按源顺序求值 actual，只执行选中 runtime 分支，失败抑制同一路
之后的表达式，未选分支的 false valid 不污染选中路径。
一个后续 next enable=false 不能抹去先前 demanded
除零/index/range 错误。

互斥分支的多个原始 use 和 UseID 均保留到提取，即使
最后合并为一个 next pair。C1 的同一路径重复目标静态拒绝、
可交叠目标 precommit 检查、不同已知 owner 独立继续生效；
本增补不引入 last-write-wins、临时表或后端仲裁。
valid=false 或 path=false 不能使原本非法的重复源写入变
合法。每个 output merge 必须由该批 op 的实际 data/
enabled 结果构成；不能从最终 yield 倒推 use。

## helper 展开与静态初始化

参与本 source-use 闭包的 source func.call 必带
ac.origin:Occurrence，标识真实调用而非诊断 loc。
callee/signature 仍由 owning header 唯一授权。verified
inlining 按真实 SSA 映射克隆 helper source use，对
UseID、ValueID、call、nested occurrence 追加一致的 C2
call-expansion frame；把 helper_result(ordinal) 改为
helper_return(expanded_call_occurrence,ordinal)。
原 helper return 数据/valid 与 caller result 的映射须先
验证再丢弃 call 边界。多条 Return 路径不合成一个伪来源；
改 callee、ordinal、frame、返回 SSA 或 caller 连线均应
失败。

StaticExpr evaluator 若遇到已验证的 helper source use，只按
真实 MLIR SSA 把 data 透传、enabled=path&&valid；false
return valid 是静态求值失败。它不能假定 constructor 参数
排列等于 record field 布局。Header/body snapshot 比较包含
该 op、ac.origin、属性、result 路径与 helper body。Owning
header 仍是 declaration authority；body snapshot 只作
已验证副本。此变更要求重新编译受影响的 Packet 等
source/header，不读取旧版本 artifact 作为兼容输入。

## 提取、替换与最终拒绝

按同一条 MLIR 路线严格排序：

1. 每源编译发布实际 source use；source verifier 校验闭合
   记录、调用/target/SSA、body/header snapshot 与 origin。
2. link 先完成 SourceOwner/header authority、namespace、
   signature/effect/helper body 的比较。
3. specialize/static 展开及 helper inline 同步改全部 occurrence、
   ValueID/UseID/target；不得先丢 branch uses。
4. specialization 后、list scalarization/numeric lowering 前，
   仅从源 use op 读 id/source/target，独立建立有序
   ac.required_uses:Array<RequiredUse>。helper_result 此时
   必须全部解析；不能从 yield、私有 sidecar 或静态推测生成。
5. 按当前保留的真实 SSA 边做 source math/数字 lowering，
   产生现有 ac.value.binding、ac.value.use、proof/check、
   candidate contributions 与 ac.yield_bindings。
6. 一对一验证 RequiredUse、ValueID、use 的 data/enabled、
   check path 与实际 yield contribution 后，才用普通 SSA
   方程替换并删除 ac.source.use；每次删除已留存可复查的
   C2 证据。
7. final verifier 和 cpp/verilog 两个真实 emit 入口拒绝任何
   残留 ac.source.use、helper_result 或 source math carrier。
   ac.stage 是诊断标签，改写它不能跳过义务。

RequiredUse 按 UseID 的结构顺序排序，不建立源执行优先级。
source-use 与最终无效果 proof 的责任不同：前者原始执行
路径要保留到提取，后者留在 final 供独立 verifier 检查。

## 原样 fixture 与独立 gate

原三源的三项 state-writing use 固定为：

| Source | 源使用 | 目标 |
| --- | --- | --- |
| accumulator.py 条件 self.total=total | 路径取旧 item.valid | owned total |
| accumulator.py Return total | 每成功返回路径 | formal result |
| core.py self.left_request=Request(...) | 完整 record 写入 | owned left_request |

Packet 构造器有 helper_result anchor；Core 的 runtime
Request 调用 inline 后有与实际调用 occurrence 对应的
helper_return use。Accumulator 两个 next 目标不能合并：
total 只在旧 item.valid 成功分支写，result 每次成功
返回写；Core 读取旧 left_request.valid，再提出完整新
Request。两次 Accumulator child 共用同一 SpecKey 定义，
状态仍属不同实例 OwnerRef。

独立测试应覆盖：op parse/print/reparse、闭合 target/类型/
arity；重复/缺失 UseID、冲突 ValueID、错 StateRef/handle、
同型 data/enable 交换、脱离真实 yield 的孤立 use、错误
path/valid 绕过异常；同一路径重复写拒绝/互斥写分立；
next 后 current 仍读旧值；错误 helper callee/ordinal/
call frame/返回 SSA；提取后丢/改 target、提前删除、final
残留 op、StaticExpr 经新 helper return anchor 求值。
三源必须各有独立 producer，parent 隐藏依赖 Python/body
仍可编译；测试不可只比较 producer 两次输出，预期来自
C1/C2 本合同。

最终 C++ 与 Verilog 的独立执行 oracle 仍为 Reset
(7,19)，Work 后 (7,19)，三次 Xfer (1,2)/(1,4)/(2,6)，
再 Reset/rerun。U02-B source 编译成功不算这些执行证据；
U02-C link/owner 与 U03 final proof、双后端和 C3 SDK
仍需各自验收。

## 实施边界和授权

影响 ODS/注册/verifier、Rule expression/statement 生成、
header helper return、static evaluator、specialization/inline、
RequiredUse 提取、数字 lowering、final proof/merge 和
双 emit 拒绝测试。诊断须给 source location、UseID、
target 和实际不一致的 SSA/owner/path；缺来源锚点是
incomplete source IR，不得从 final yield 回填。

本增补没有新增 Python 语法、reset、仲裁、storage、runtime
或 backend 路线。实施时一次切换新 source/header；
无 schema-version 选择、旧 helper header 兼容、第二
assignment 语言或旧 QueueGraph/ACPy fallback。接口仍需
独立设计审阅并得到用户对精确文本的批准。
