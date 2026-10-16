# 条件 Method 调用与 body effects

状态：实现使用现有单一编译与双后端流程。本文规定条件调用和效果的语义及验收义务，
不替代精确候选的执行证据，也不声明全量 nightly 或综合通过。当前可用范围以
[语言参考](../../reference/language.md#method-execution-and-remaining-capabilities)和精确候选证据为准。

本扩展复用 statement `if/elif/else/pass`、现有 Method、assert/log/report、source units、
`@system` 与同一 MLIR/common IR/双后端流程。Rule 仍是唯一自主原子事务；Method 不创建
事务、周期或隐式队列。纯计算继续使用普通 `def`。

## 源码范围与串行含义

在 Rule/Method body 的分支 arm 中接纳 Method 调用和直接 assert/log/report。仍拒绝
selector、guard、纯 helper、collection callback、条件表达式中的 Method 调用或效果，
以及 early return、效果循环、动态实例/注册、partial/aggregate state、改变后逃出 join 的
Table-containing local、跨域与多个 outer writer。有限无环 nominal Struct（闭包只含
Bits/Enum/Struct）及独立 Enum locals 可以 join；普通局部字段/subtree 赋值重建不可变
payload，保留 root nominal boundary 和 saved aliases，不修改 producer/owner。
含 Table 的重建、indexed/slice/annotated/augmented targets 仍拒绝；未改变的 Table
capture 保留原接纳。精确类型、共同 IR 图与证明义务见
[局部值 SSA 合同](branch-state-ssa.md)。类型、闭包、递归与域检查覆盖恒假 arm。
裸 Rule 中的条件 log/report 无需人为添加 Method、恒真 guard 或虚假状态。

以下是事务内序列示意，counter 为静态 Method-owner 实例，choose 为既有一位条件：

```python
@ac.rule
def work() -> None:
    before = counter.add(1)
    if choose:
        selected = counter.add(2)
    else:
        selected = counter.add(4)
    after = counter.add(3)
    ac.log("info", "result", before, selected, after)
```

若 add 返回更新后的候选，进入 Q=5 时，choose=1 得到 6/8/11；choose=0 得到 6/10/13。
两臂从相同入口候选 6 开始；else 不继承先扫描 then 的结果。独立 Rule 仍读 old Q=5。
已保存普通 SSA 不随后续写入变化。调用 guard 读取各自进入时的候选。

未选 arm 的 false/X/Z guard 不阻塞事务；选中调用的 false guard 使整个外层 Rule hold，
不转入 else，也不保留此前部分写入。较早到达的未知控制不能由后来的 false guard 掩盖。
assert 是提交检查，不改变 candidate、后续调用或准入。

## 效果分支的四态选择

只含纯计算、whole-Bits state 和 admitted scalar/nominal local 操作的分支保留
[B1 四态 data/rawIntent join](branch-state-ssa.md)。任一 arm 直接或嵌套含 Method call、
assert/log/report 时，所有 enclosing branches 均增加 selection-known 义务。所有 Method
调用均计入，包括空体、只读、state-only 与恒真 guard。分类按已验证的原始结构进行；
恒假 arm 中的调用不因此从库存消失。

| 条件与时刻 | 行为 |
| --- | --- |
| 效果 selector 已知 0 或 1 | 均通过 selection 准入，仅选择对应 arm |
| reached selector 为 X/Z，rising 且非 reset | managed fault，整个系统零提交 |
| selector 为 X/Z，held/falling/reset | 不新增 sampled fault；该事务 final fire=0，全部 body effects 不激活 |
| 先前 guard 已知 false | 后续未知控制不激活 |
| 先到达 X/Z、后来 guard false | rising 非 reset 仍失败 |
| selector 已知，选中 assert 为 false/X/Z | 保留普通每 Work reset-mask 检查，包括 low/held/falling |
| selector 已知，选中 observation 合法 | 保留成功 epoch 发布，允许 held/falling |

未知 selector 对整个事务的取消也会抑制此前 body effects，沿用后续 required guard
取消整个事务的原则。clock/reset 的未知值、显式 Reset、Discard、时钟历史与 sticky failure
恢复均保留既有合同。observations 不新增全局 reset mask。

## 值有效性与 totality

源码路径保留每个原 branch arm，包括没有 selection obligation 的纯分支。一个值的
可用点是 `(completedControlPrefix, sourcePath)`，普通使用同时要求 prefix 已建立、
定义路径支配使用路径、原 SSA dominance、lexical anchor 与 frame 合法。

跨 frame 只允许已认证的 formal/actual 与 result/yield 边。actual 在 callee entry 可用；
formal 保留 callee frame。caller 的调用结果必须有独立引用，其有效性一律为 callee exit，
即使返回 literal、capture 或调用前已有值，也不能通过 alias 消除该有效性。
物理 SSA 可以共享，不要求硬件 buffer。

只有真实 branch result/state join 能把各 arm exit 已验证的值提升到父路径。必须验证原
selector、两臂值、候选快照和完整写意图；缺失写入继承 entry。不能仅取最大 prefix 就让
未执行调用的结果逃逸。纯分支保持四态 join；效果分支成功准入时恰好一个 arm 有效。

结构验证与 totality 分离。恒假 arm 保留签名、闭包、递归、类型、域和潜在 writer 检查，
但静态不可达 guard/selection 不阻止 total Rule。静态已知 0 与 1 都通过 selection。
不能证明最终准入恒真时，outer Rule 必须 `-> None`；Method 返回仍可在事务内使用。

静态事实只来自已有 literal/identity/formal/capture 及已认证绑定；不根据初值、区间或测试
输入猜测。发布事实不能表达的 Method-result selector 仍可保守保持 Unknown，不新增
表达式解释器或接口自证。未知不当 false，不用补零、上拍值或隐藏 valid 输出。

## MLIR 所有权与源证明

| 既有 owner | 扩展责任 |
| --- | --- |
| Python importer / Records | 输出现有 typed branch/call/check/observe，区分 arm 与 pure-expression 位置；保留类型、definite assignment 和源坐标 |
| SourceTransactionSummary / SourceMethodAnalysis | 保持七字段 branch rows，遍历全部闭包；按结构路径证明 totality |
| SourceTransactionStateAnalysis | source paths、调用入口候选、有序 controls、join lifting、frame/result 绑定和写库存 |
| LowerTransactions | 同一物化和独立原源认证；成功后才原子替换 |
| HardwareSampledAdmission / SourceChecks / Preflight | 当前 Rule 的 closed schema、路径/准入证明、真实 owner/domain/私有路由和预算 |

`ac.branch` 及摘要仍为七字段：kind、occurrence、condition_location、condition、
then_effects、else_effects、results。源码 authored path 仍须证明 true；结构执行路径由
MLIR 推导。不能把二者混为一个作者接口。

每个 invocation frame 恰好一个 callable guard。controls 是 guard 或 selection 的 tagged
引用，形成精确双射；G=frames 数，C=G+S。control index、GuardBoundaryId 和
SelectionBoundaryId 是不同库存索引。每个 control i 进入 prefix i，退出 i+1。

事件按源 DFS：BranchEnter、可选 SelectionEnter/Exit、then events、else events、
state joins、branch results、BranchExit。prefix 全局单调，两臂 candidates 各自恢复 entry。
原源认证独立重导 classification、paths、原 SSA、spans、frame/bindings、候选与事件顺序；
plan 和 materialized locator 只定位对象，不授予权威。

## 准入与效果方程

```text
K1(x)=case_eq(x,1); K0(x)=case_eq(x,0)
Known(x)=K0(x) OR K1(x)
S(root)=1
S(then b)=S(parent b) AND K1(selector_b)
S(else b)=S(parent b) AND K0(selector_b)
P0=1; Ri=Pi AND S(path_i)
Ai=K1(raw_i)       # callable_guard
Ai=Known(raw_i)    # branch_select
P(i+1)=Pi AND BitsSelect(S(path_i),Ai,1)
F=P(C)
effectPath(e)=F AND S(path_e)
E(owner)=F AND rawIntent(owner)
```

S/P/R/F 是已证明两态控制；rawIntent 仍为四态。未选路径必须 bypass，不能把 `R AND A`
直接当整个事务的新 fire。root 规范特化为 R=P、next=P AND A、effectPath=F。

<a id="final-sampled-schema"></a>

## 最终属性 hard break

`ac.sampled_admission` 精确六字段，删除 guards，改为 controls：

```text
{clock_capture, reset_capture, fire_result,
 controls:[{kind,id,invocation,predecessor,path}],
 body_checks:[{id,invocation,path}],
 observations:[{id,value_count,path}]}
```

kind 精确为 callable_guard 或 branch_select。PathRef 为 `{}` 或
`{selection:Index,when_true:BoolAttr}`。Index 为非负 signless i64，predecessor 仅首项为 -1，
其余精确 i-1。path 仅引用较早 selection；调用链只引用当前或 ancestor invocation，
不能引用 sibling 或逃出 owning callable 的路径。effect invocation 必须对应唯一 callable
control，selection 不能充当 callable。重复源 branch 在不同 invocation 中保持不同身份。

硬件路径投影保留最近 selection-bearing ancestor 自己的 arm polarity，只删除中间纯分支
组件；源码可用性仍使用完整路径。CheckID 三字段不变，control obligation=i；selection
check 是 branch occurrence，location 是 condition span。ObservationID 五字段不变。

设 O 普通结果、W owner、C controls、A assertions，observation j 有 K_j 个 values：

```text
D_i=O+2i; E_i=O+2i+1; Fslot=O+2W
P_j=Fslot+1+sum(t<j)(1+K_t)
B=Fslot+1+sum(j)(1+K_j)
control_i: raw=B+2i, reached=B+2i+1
assert_i: condition=B+2(C+i), path=B+2(C+i)+1
total=B+2(C+A)
```

RequiredChecks 为 C controls 后 A assertions，assertion obligation=C+i。
`{state,data,enable}` 候选、external fire 零使用和独立 effect slots 保持；旧 guards、缺少
kind/path 或错误字段直接拒绝，无 alias/fallback。root effect 保持内部 F 精确 identity；
nonroot effect 要求左输入精确为 F 的 AND，并证明整体等于 F AND S。

## 有界证明与费用

沿用 AND/ACI Prefix 证明；在一次不可变证明内，将认证后的 atom 存为私有整数 ID。
Known table 保存 KnownZero/One/Either 与完整 normalized endpoint；conditional pool 保存
`(selectionPath,acceptance)`。known ID 为 checked `2*index`，conditional ID 为 checked
`2*index+1`，溢出和饱和值在插入前拒绝。ID 不进入 IR、接口或跨变换缓存。
Known interning 保留 test tag、实际 SSA、完整 scope/bindings 和 resolved type；不能将同一
raw 的 One 与 Either 混同。literal reduction 前仍保留 raw 身份。

Prefix 使用 zero-inline、排序去重的 ID 数组。copy 精确预留源长度；conjunction 先检查并
预付两输入长度之和，再线性 merge；equality 比较 impossible flag、长度和 ID。删除原有
endpoint 集合的二次成员扫描和排序。path 只含 Zero/One，acceptance 只为 literal 或一个
One/Either，二者不含 conditional ID，因而 pool 不递归；命中也须先完整验证 children。

解析分 Prefix、SelectionPath、Acceptance 三模式，缓存含 mode、完整 scope/bindings/stops
和 Active/Complete。proof 借用构造时的不可变 prepared scope；parse、normalize 和
knownEither 不接受调用者供应的 scope，首次 miss 也不能进入第二上下文。缓存先筛选
mode/value，命中后仍检查完整 scope/bindings/type；DefinitionLocal 不穿越实例。
OR 仅直接匹配同 raw 的 K0/K1，literal reduction 前保留 raw 身份；
每次 miss 六个额外 identity 请求、最多八个 endpoint pair 比较。select 仅 Prefix 模式，
先完整验证 path/acceptance 与 one fallback，再查 linear pool；命中也不跳过验证。

保持 source/preparation 1M、final 4M 和原公开场景规模。所有字段、variant/tag/padding、
映射、路径访问、原子缓冲、缓存、pool、显式 frame 与容量搬移在分配前计费。
cache/pool/frame 几何增长与 append 工作分开，不重复收已占记录字节。私有 sizeof helper
返回实际 Known、Prefix、cache、pool、frame 和路径记录尺寸，不能以测量样本的常数代替。

费用由实际 mode-specific census 和既定 helper 调用求和：Q=C+1+E+I+2T；
Qid=Q+2X+5C+6O_or+T；Known factory attempts K=X+O_or+G+3S；
conditional attempts B=T+D；实际 Prefix cardinality 上界 AA=X+O_or+T。
I 为 AND child edges，X 为 comparison misses，O_or 为 OR misses，T 为 select misses，
D 为 nonroot controls，E 为 effects。expected prefix P_i 最多 i 个 atom，selection path
最多其 metadata depth，reached/effect 使用 prefix 加实际 path 的界；不把实际与 expected
集合合并成一个未认证的大集合。

令 u=bytes(uint64_t)、h 为 Prefix record 费，线性 helper 上界为
Copy(n)=h+n*(u+1)、Join(l,r)=h+1+(u+3)*(l+r)、Eq(l,r)=1+min(l,r)。Known table
线性 interning 收取全部 endpoint/scope 比较、插入、作用域复制和容量搬移；pool 比较
不超过 choose(B,2)。完整 identity/OR/fallback/frame/path/独立 intent proof 各计一次。

metadata prepayment 按每次真实比较的两个 Attribute 和 prefix frames 求和，不把整份
Rule metadata 乘到每个关系上。相同不可变 Attribute 比较费为 1；否则支付两侧完整遍历。
native metadata walker 与 raw cost walker 使用相同规则：root/push 计访问，字符串和
字典键计字节，stack 由 0 开始增长到 max(2,2*capacity+1)，分配前支付容量差字节和已保留
元素搬移。pop 不重复收节点费用。raw census 只借用 Attribute 与标量计数，不缓存 schema
verdict，也不调用结构比较器或 typed verifier；错误 shape、role、ordinal 和 parent index
在索引前拒绝。row、control pairs、callable candidates、PrefixRelation 与 ancestor visits
分别收费，完整三个 native schema parses 与后续 catalog/wiring/occurrence 检查全部保留。

preflight 数学边界不证明后续完整流程已适配 4M。只有同一候选的默认预算完整 link、
System 执行和独立 debit-to-reservation 审查均通过，才可验收。raw predicate/data/intent
不混入 admission grammar；不得为达到预算而删检查、缩减场景或提高限额。

新增 nominal traversal/metadata、structural containers、materialization 与 proof 成本
在原限额内预付。继承的任意精度算术 evaluator capacity policy 仍为 OPEN；不能由
这些新增费用覆盖推断完整 evaluator 已有界或关闭 `METHOD-ANALYSIS-BOUNDS`。

## 验收 checklist {#acceptance-checklist}

- [ ] Q=5 数值 oracle、两 arm、nested/elif、重复/self/child/跨 owner 调用及独立 old-Q。
- [ ] selected false hold 全事务；unselected false/X/Z guard bypass；早 X 不被晚 false 掩盖。
- [ ] literal/capture 返回值仍有 callee-exit 有效性；frame binding、pure-path 投影和 join lifting。
- [ ] static false/zero totality、reachable dynamic selector 的 None 限制、dead-arm 类型/递归拒绝。
- [ ] selector 0/1/X/Z × rising/low/held/falling/reset；普通 assert false/X/Z 与 observation 行为。
- [ ] state/history/events/gauges/epoch 零提交、前次发布保持、Discard 与 sticky-failure 恢复。
- [ ] source/final mutations 覆盖路径、control kind、raw、bypass、frame、spans、版本、slot、私有路由。
- [ ] provider/consumer 独立 compile，consumer 无 provider Python/body；完整 link 重验 authority。
- [ ] 同 final artifact 的 generated System workers 1/2、Verilator 与真实 Icarus 四态矩阵。
- [ ] 原六个恒真、三个动态及 B1/direct-effects 场景不缩减，默认 4M 与极小预算原子失败。
- [ ] 短 API/examples、installed CompilerDev、strict docs、skills、独立精确候选 review 全部通过。
- [ ] nightly、综合、时序未运行时明确 NOT-RUN，不由局部通过推断。

验收入口复用[现有命令](acceptance.md#commands-and-gates)。same-owner/name 的 report
即使位于互斥 arms 仍按现有规则冲突；可在 join 后一次 report。typed source unit 可先独立
发布，完整 link 闭包确定 ObservationID/逻辑 owner 后拒绝冲突，并保护先前 final 产物。
本扩展不关闭 partial state、resources/conflicts/causality、effect-safe optimization 或整个
issue #307；剩余范围继续见[扩展矩阵](extension-roadmap.md)。
