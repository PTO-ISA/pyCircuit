# whole-Bits 分支状态与局部值 SSA 规格

状态：有界语义与接口已冻结；实现候选须按本文独立验收。本文不是候选测试通过、
nightly、综合或时序声明。它扩展[源语义](source-semantics.md)、
[事务采样准入](sampled-admission.md)和[事务 body effects](sampled-body-effects.md)，
不增加 Python API、Runtime 协议或第二条编译路线。

后续覆盖关系：[条件调用与 effects 合同](conditional-effects.md)扩展本切片的
statement-arm admission、source-path 有效性与事件序列，并将最终 `guards` schema
hard break 为 tagged `controls`。下文的 arm 内调用/effect 拒绝和 `guards` 布局只描述
此前分支切片，不是当前条件候选的边界。纯分支四态 D/rawIntent join、D/E 候选与原有
验收义务继续保留；条件候选的完整验收状态见[扩展矩阵](extension-roadmap.md)。

## 支持范围

Rule/Method 事务 body 接纳有限嵌套的 statement `if`/`elif`/`else`/`pass`。条件继续使用
既有 Boolean 或有权威的 fixed `u1` 规则。分支可更新 whole fixed-width Bits owner，
也可产生跨 join 使用的 scalar Bits、nominal Enum 和有限无环 nominal Struct local。
Struct 闭包只含 fixed-width Bits、Enum 和嵌套 Struct；改变值的两 arm 必须具有相同
nominal identity，等宽布局不能替代声明身份。含 Table 的改变后 join 仍拒绝；未改变的
Table-containing capture 保留原有接纳。

Method 调用、`assert`、`log` 和 `report` 可在分支前后无条件出现。它们不得出现在任一
arm、branch test 或 branch-local 表达式中，即使该 arm 可静态证明为 false。early return、
loop、动态实例/注册、guard region 内分支、partial/aggregate state、资源操作、跨域和
多 writer 仍拒绝。conditional calls/effects、partial state 和资源是后续独立切片。

新 local 在 join 后使用时必须在两个 arm 都绑定；不能推断零值。已有 local 或 owner 在
某 arm 未赋值时继承分支入口值。普通 SSA 名称仍是不可变快照；后续候选写入不改变已
保存的值。分支前后的无条件 Method 调用继续共享唯一外层事务的候选版本。独立 Rule
分别从 old Q 开始，不按注册顺序前递。

scalar local join 复用现有 numeric preparation、definite-assignment 和 binding-boundary
规则。Boolean、Integer 与 fixed Bits 的逻辑种类、区间及宽度权威继续保留；arm conversion
发生在各自 yield 前，join 不把原 producer 改种类，也不引入新的 constant evaluator。
nominal join 独立应用每个 arm 的绑定边界，不制造 Boolean/Integer 数值事实。

普通单目标 `local.inner.payload = value` 重建不可变局部 Struct，也可替换嵌套 Struct
subtree。root 必须是当前已绑定的普通 payload；captured/returned payload 是快照，
保存的 alias 不随重建改变，producer 和 persistent owner 不被修改。替换按声明字段
类型检查，root 保留原 nominal boundary。含 Table 的 root、persistent/instance/
callable/type 身份、Subscript、slice、annotated field 和 augmented targets 不接纳。

## 状态版本与四态 join

每个 owner 版本表示 `(D, rawIntent)`：

```text
OldQ:  D = Q0;                    rawIntent = 0
Write: D = statement-time RHS;    rawIntent = 1
Join:  D = BitsSelect(c, true.D, false.D)
       rawIntent = BitsSelect(c, true.rawIntent, false.rawIntent)
```

两个 arm 从完全相同的入口 pair 开始。未写路径继承入口 pair，不回退到 Q0；因此分支前
已有写入时，两边都继承该写意图。身份写仍是 Write，所有原始写入都进入 potential-writer
inventory，包括恒假 arm 中的写入。编译器不得根据 D 相等、结果未使用或常量条件删除
raw intent。

`BitsSelect` 保留四态数据和 intent。selector 为 X/Z 且两边 intent 都为 1 时，E 可为 1，
D 保留共同 known/Z 位并在其余位产生 X。入口 intent 为 0 的单边写产生 unknown intent；
有效非 reset 上升沿沿既有 DFFE 合同失败并触发 whole-system zero commit。分支前已有
intent=1 时，未写 arm 仍继承 1，不能凭 selector 制造 enable fault。

每个实际 storage 的最终使能为：

```text
E = bit_and(internalFinalFire, rawIntent)
```

必须保留四态 AND，不能用 KnownOne、case equality 或常量折叠把 unknown intent 变成
false。`finalFire=0` 时按四态 AND 抑制 unknown intent。直接 body effects 使用 finalFire，
不使用任一 owner 的 E；owner 未写不表示事务或其 observations hold。

## 源 IR、发布摘要与 state plan

`ac.branch` 只存在于 source IR。它有一个 Bits1 condition、必填 occurrence 和
`ac.condition_location`、零个或多个 Bits/Enum/Struct results，以及两个 single-block、
zero-argument、以 `ac.yield` 结束的 region。两个 yield 的 arity/type 与 results 精确一致。
该操作声明 recursive memory effects，不能标 Pure；final hardware 不得残留 `ac.branch`。
它只允许直接位于 transaction/Method body 或另一个 `ac.branch` arm；缺少 `else` 时仍
建立真实空 region，并在需要时 yield 入口 local。

branch 发布摘要精确包含七个字段，不含 `path`：

```text
{kind="branch", occurrence, condition_location, condition,
 then_effects, else_effects, results}
```

`results` 每项精确包含 `{type,when_true,when_false}`。摘要保留 arm/event 次序并按既有
深度和 work budget 收费。consumer compile 只读发布接口；完整 link 必须根据 provider
body 重导 branch 摘要，不能读取 provider Python 或信任 header 自证。

`SourceTransactionStateAnalysis` 的 `StateVersion` 分为 OldQ、Write 和 Join。Join 只引用
对应 join record，没有伪造 RHS；branch result、entry/arm/final version、调用 frame、
required prefix 和有序事件均进入只读 plan。结果 locator、IR mapping 和 metadata 只定位
对象，不授予源历史权威。任一事务失败时不得发布部分 plan。

事件顺序固定为 BranchEnter、ThenEnter、ThenExit、ElseEnter、ElseExit，随后按 owner ID
发出 StateJoin、按 result ordinal 发出 BranchResult，最后 BranchExit。即使分支没有结果
或写入，六个边界事件仍存在。version ordinal 按 owner 的实际 Write/Join 创建顺序分配，
不能用 predecessor+1 跨两个 arm 猜测。

## nominal local 的共同 IR join

lowering 按实际声明顺序对 Struct 字段分别投影 true/false 值、递归 join，再构造原
nominal Struct。Bits leaf 使用原 condition 的 `bits.select`；Enum leaf（包括独立
Enum result）使用两个 `enum.to_bits`、同宽 carrier select 和 `enum.from_bits`。
carrier 宽度由实际 Enum 声明提供；生成的 membership result 必须零使用，即使 join
未使用也如此。invalid/X/Z carrier 保留原四态数据含义，不增加 membership fault。

source authentication 从原 branch/yields/types 独立导出完整图，验证 live locator、
字段与 nominal type、carrier 宽度/来源、精确 operands/attributes、块内严格次序、
完整消费和最终 result identity；metadata 不授予权威。七字段 branch summary 和
三字段 result row 不变。完整图认证后仍执行独立 canonical final proof。

## 最终 schema 与结果布局

本切片原有的 `ac.sampled_admission` 精确使用六个必填字段：`clock_capture`、`reset_capture`、
`fire_result`、`guards`、`body_checks` 和 `observations`。空数组仍必须存在。
当前条件候选拒绝此旧形状；以[条件合同](conditional-effects.md#final-sampled-schema)的
`controls`、effect paths 和结果布局为准，下面的 G-based suffix 是此前切片的记录。

`ac.transaction_wiring.writers[*].candidates` hard break 为：

```text
{state: StateID, data: Index, enable: Index}
```

旧 `{state,result}` 形状直接拒绝，无默认值、alias 或 fallback。设 O 为普通结果数、W 为
候选 owner 数、G 为 guards 数、A 为 assertions 数，observation j 有 K_j 个 values：

```text
D_i = O + 2i
E_i = O + 2i + 1
F   = O + 2W
P_j = F + 1 + sum(t < j)(1 + K_t)
C   = F + 1 + sum(j)(1 + K_j)
T   = C + 2(G + A)
```

`D_i` 是候选数据，`E_i` 是 finalFire 与 rawIntent 的 AND，`F` 是 external fire result。
observation j 使用 path `P_j` 及随后 K_j 个值；guard/assertion suffix 从 C 开始，沿用
既有 pair 布局。external fire result 在所有事务中必须零使用，包括 state-free 和
effectful 事务；internal fire 仍按有序 prefix 证明并作为 yield 返回。D/E、guard 和
effect slots 各自走精确 private route，不能用 fire 直接替代 E 或 effect path。

## 两层证明、预算与缓存

原源认证独立 replay branch occurrence、condition location、arm/event 次序、entry 与 arm
版本、每个 data/intent select、未写继承、local yields、保存快照、后续 guard 读取和
最终 E 的 AND 形状。它还核对完整 original write inventory。随后 canonical final
verifier 独立证明 slot、owner、domain、private routes、actual DFFE E 和无 source 残留。
两层成功后才原子替换 detached candidate；失败保持输入 IR、发布文件和 stage outputs。

保持既有 source/preparation 和 final verifier 限额。branch regions、两套 arm environment、
joins、bindings、types、metadata、proof roots 和 cache storage 都必须在分配前收费。
实现按 arm 分析，不能枚举 path 组合。缓存只复用 exact immutable definition/bindings 的
已验证事实；每个 actual Rule query 仍解析该实际 Rule。actual instance 的 inventory、
domain、D/E 和 private route 必须逐一证明。IR mutation 使 plan、prepared boundary 和
cache 全部失效；预算优化不得提高限额、缩小输入或跳过证明。
新增 nominal closure/metadata 遍历、structural containers、materialization 和 proof
成本均须预付。继承的任意精度算术 evaluator capacity policy 仍为 OPEN；本切片不能
据此声明完整 evaluator 已有界或关闭 `METHOD-ANALYSIS-BOUNDS`。

## 验收 checklist {#acceptance-checklist}

- [ ] 数值 oracle 从 Q=5 覆盖 branch 前写、两 arm、join 后写和 saved local：false 得到
  before=5/saved=16/after=6/final=10，true 得到 5/6/8/12；同一事务 false→true 最终为
  17 且 saved 为 16/11，独立 Rule 仍读旧 Q=5。
- [ ] finite nesting、`elif`、empty `else`、`pass`、entry write、identity write、guard-entry
  join、branch 前后无条件 Method 调用及 scalar definite assignment 有独立正反例。
- [ ] arm 中 call/assert/log/report、effectful test、early return、partial/aggregate state、
  changed Table-containing escape/reconstruction、resource、跨域与多 writer 均 fail closed，
  包括恒假 arm；arm call/effects 的当前接纳以条件合同为准。
- [ ] nested/sibling Struct、独立 Enum、field/subtree 重建、captured/returned payload 与
  saved aliases、遗漏 arm 继承和 nominal identity 拒绝有独立正反例；unchanged Table
  captures 保持，cycles、foreign types 和非法 targets 拒绝。
- [ ] Enum invalid/X/Z carrier 与未用 membership 的四态结果，以及 graph attributes、
  membership use、stale/swapped/duplicate locators、同宽错字段和 carrier 来源 mutation
  均独立验证。
- [ ] X/Z selector 覆盖共同 known/Z 数据位、单边 intent、entry intent=1、identity write、
  fire=false、reset、held/falling 和失败后的 state/history/observations/epoch 零提交。
- [ ] source mutations 覆盖错误入口继承、交换 arms、snapshot 改 final candidate、删除身份
  intent、KnownOne intent、E→fire 及 branch provenance；final mutations 覆盖旧 schema、
  D/E 交换、错 owner/domain、private escape、漏 AND 和 residual `ac.branch`。
- [ ] provider/consumer 独立 compile；consumer 无 provider Python/body；完整 closure 重验
  branch summary、occurrence/order/type，并保留 publication/output protection。
- [ ] 同一 final artifact 运行 generated Method `@system` 的 C++ workers 1/2 与 RTL；真实
  四态 RTL 补充 intent/rollback 矩阵，module-only probe 不能替代 System closure。
- [ ] 原保留 straight-line、六个恒真、三个动态 Method 场景和 body-effects regression
  不回归；native/lit、短 API/examples、installed CompilerDev consumer、strict docs 和
  independent whole-candidate review 绑定同一候选。
- [ ] 固定规模公共跨源用例在原预算内完成，极小预算原子失败；每个 actual Rule 仍实际
  parse，cache mutation/identity 隔离有独立证据。nightly、综合和时序未执行时明确记
  `NOT-RUN`，不得由短 gate 推断。

本文规定 whole-Bits statement branch、scalar 与上述 nominal local SSA join，不构成
候选验收通过声明。conditional Method
calls/effects、partial or aggregate state、resources/conflicts/causality 及 effect-safe
optimization 继续由[扩展矩阵](extension-roadmap.md)跟踪；不能据此关闭 issue #307。
