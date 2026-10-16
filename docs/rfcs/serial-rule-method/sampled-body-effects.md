# 事务 body checks 与 observations

本文扩展[事务采样准入](sampled-admission.md)的既有表示与所有者。它定义明确的
实现合同及验收要求；规格存在不代表实现或测试通过。源语句和发布摘要继续遵循
[事务接口摘要](publication-summary.md)。不增加作者 API、pass、runner 或后端私有事务协议。

后续覆盖关系：[条件调用与 effects 合同](conditional-effects.md)在同一流程中接纳
statement-arm calls/assert/log/report，并以 tagged `controls` 和选中路径取代本直线
切片的 `guards` schema 与无条件路径限制。下文保留此前切片的布局和证明义务，不能
用它作为当前 final IR 的生产格式；语句时刻快照、私有槽、双层证明及提交原子性继续
适用。条件候选仍须完成其独立 checklist，不由本切片的历史证据推断通过。

## Rule 与 Method 的责任

自主 Rule 拥有事务，Method 的状态候选、guard 和效果归入调用者事务。直接 body 的
`assert` 是提交检查，不是 guard；它不改变后续执行、候选版本或 fire。`log`/`report`
按语句顺序保存当场的 SSA operand；后续写入不得改变已经保存的值。

每个效果的执行 path 使用整个事务的最终 prefix。因此早先 false assert 后出现
false guard 时，该事务 hold，assert 与 observations 均不激活。已到达的 X/Z guard
仍按采样准入合同失败，后续 false guard 不能屏蔽它。独立 Rule 继续读取旧 Q。

本切片只接纳 direct、unconditional body effects、同物理域、whole Bits 和既有 scalar observation 的
情况。authored effect path 必须证明 true。嵌套 pure region、guard、纯 helper 和未表示
的效果继续拒绝；branch arm 内 effects、partial state、资源端口与冲突闭包分别验收。

## 六字段 schema 与精确结果布局 {#exact-layout}

本直线切片原有的 `ac.sampled_admission` 精确包含六个必填字段，包括空数组；当前
候选的 hard break、controls/path 与结果 slots 见
[条件合同](conditional-effects.md#final-sampled-schema)。以下旧形状在当前候选中拒绝：

```text
{
  clock_capture: Index, reset_capture: Index, fire_result: Index,
  guards: [{id: CheckID, invocation: Invocation, predecessor: IndexOrMinusOne}],
  body_checks: [{id: CheckID, invocation: Invocation}],
  observations: [{id: ObservationID, value_count: Index}]
}
```

所有 Index 为非负 signless i64；只有 predecessor 可为 -1。四字段旧格式直接拒绝。
CheckID 和五字段 ObservationID 不变。body assertion i 的 obligation 为 G+i，
observation j 的 ordinal 为 j。registration 来自外层 Rule 注册；site/check 来自原
语句；invocation 保存完整 call/receiver 链，owner 为最后 receiver，根 body 为 self。
效果 invocation 必须存在于 guard inventory；同一 invocation 的多个效果合法。

设 O 为普通结果，W 为候选 owner，G 为 guards，A 为 assertions，N 为 observations，
Kj 为 observation j 的 value 数量：

```text
D_i = O + 2i
E_i = O + 2i + 1
F = O + 2W
Pj = F + 1 + sum(t < j)(1 + Kt)
C = F + 1 + sum(j)(1 + Kj)
T = C + 2 * (G + A)

ordinary: [0, O); candidate i: data D_i, enable E_i; fire: F
observation j: path Pj; values [Pj+1, Pj+1+Kj)
guard i: raw C+2i; reached C+2i+1
assertion i: condition C+2(G+i); path C+2(G+i)+1
```

结果与 yield 必须精确有 T 个 operands。effect path 和检查对为 Bits1，payload 沿用
既有 scalar 与 64 位 slot 限制。绑定后的 saved SSA 决定实际 payload/condition 类型；
不能把 clock 的静态类型表达式作为所有 condition 的定义来源。解析后的硬件类型仍由
共同分析验证，不放宽 nominal identity 或 width 合同。

`ac.required_checks` 精确按 G 个 guard_known、A 个 assert 排列，构成 trailing pair
suffix。assert 不获得 samplingBindingIndex 或 samplingOccurrenceIndex；guard_known
才消费 `[clock, reset]`。`transaction_wiring.candidates` 精确使用
`{state,data,enable}`。旧的 `{state,result}` 形状拒绝。candidate data 保存 D；
candidate enable 保存 `bit_and(internal_final_fire, raw_write_intent)` 的 E，不能把
未知 intent 转为 false。外部 fire result 必须零使用；每个 observation/assertion
仍使用自己的私有 result slot，即使该 path 在内部等于 final fire。

## 两层证明与 terminal 私有路由

物化时按原 frame/op 保存效果 operands，并恰好消费一次原事件。guard expects 先
生成，随后 body terminals 按展开后的混合事件顺序生成。assert 保留 message 的缺失
与空字符串区别；所有 terminals 保留原 kind、spec、SourceSpan 和 MLIR Location。

原源认证必须独立 replay 原 plan、调用、读版本和 clones，验证 saved operands、完整
身份、类型、布局、顺序以及 terminals 一一对应。候选 mappings、locators 和 metadata
只定位对象，不授予源历史权威。缺失、多余、交换、重复或共享 terminal 均拒绝。
原源认证成功后再运行 canonical final verifier；两层成功才原子替换输入。

final prefix prover 保留每个 guard 的 reached，并证明 fire 和全部 effect paths 等于
最终 prefix。assert condition 与 observation payload 不参与 admission。完整 private
boundary 包含 candidates、fire、observations 的 path/values 和全部检查对。

私有路由复用 Data、Enable 与 EffectOperand 角色。EffectOperand 必须记录精确
Operation 和 operand ordinal：expect 为 condition0/path1；observe 为 path0/value k+1。
每个 terminal 消费指定的独立结果槽；直接替换为 fire 槽也必须拒绝，即使其内部 yield
相同。payload 不能逃逸到公共输出、无关计算、其他 terminal、child port 或 dead carrier。

共享 source-effect resolver 验证逻辑 owner 与 provenance。WiringProof 将返回的真实
实例路径与当前物理 occurrence 组合，并独立证明 receiver 的 clock/reset 与 writer
相同。逻辑解析不授予物理域证明。Report 冲突继续由既有 dependency owner 根据真实
logical owner 与 authored name 检查；false path 或重复调用不豁免冲突。

## 有界性与验证复用

使用现有 SourceCheckBudget；保持原限额。所有计数在分配、narrowing、索引和乘加前
使用饱和检查。metadata 采用显式、收费的遍历栈，不能递归消耗 host stack。
仅 raw traversal/prepayment 使用嵌套、不可复制/移动的 RawDiagnosticScope：预算失败
在 UnknownLoc 诊断，不 dump op、加 notes 或遍历 source location；退出后恢复普通诊断，
且必须在 native verifier 前退出。

记 E=A+N、Q=G+A+N、K=sum(Kj)。Fframes 计 guards、body_checks 和 observations IDs
中的全部 invocation entries，包括重复 frame；M 为 sampled 与 RequiredCheck metadata
结构成本的饱和和。记 R(T)=1+ceil(sizeof(T)/8)，schema reserve 为：

```text
S = R(View) + G*R(GuardView) + A*R(BodyCheckView)
  + N*R(ObservationView) + K*R(Value) + Fframes*R(DictionaryAttr)
  + (2Q+3)*M + 3 + 2G + 2A + N + 2K
```

此 reserve 不替代实际 parser debts。prefixQueries=G+1+E+andOperands，
queries=prefixQueries+2*comparisons+5G，equalities=G+1+E；其余既有 identity、atom、
copy 与 conjunction 项保持。实际 effect path yields 是 seeds，condition/data 不是。
原源认证同样提前计费 replay 存储、payload vectors、全部 terminal scans 与 frame 链。

成功证明复用只限同一 analysis、budget 调用、immutable package、准确 definition、
完整 bindings、Rule 和完整 boundary policy。每次查询仍解析实际 Rule；每个实际根的
owner、domain、terminal 与 private-use 检查独立进行。IR 改变后相关 plans、prepared
boundaries 和检查/发射 contexts 全部失效。MLIR uniqued Attribute 的相等比较可以使用
实际恒定成本，不借此跳过结构验证或改变 schema reserve。

## 执行与验收 checklist {#acceptance-checklist}

C++ validator 顺序为 snapshots、physical faults、ordered guard-known checks、ordinary/
body assertions、observation precommit，最后才 Xfer/history/publish。RTL 复用既有
PREPARE/COMMIT/DISCARD/RESET 与全系统 barrier。assert 使用普通 reset mask 和四态
失败语义；held/falling epoch 的合法 observations 可以发布。失败保留 state、history、
先前 events/gauges 与 epoch；显式 Reset 不伪造成功 observation epoch。

- [ ] 四字段旧格式、缺字段、错误计数/类型/顺序/索引与巨大 raw counts 有独立反例。
- [ ] guard-only、assert-only、observe-only、无状态、混合、多值和 literal-only 布局通过。
- [ ] 源认证 mutation 与独立 final mutation 分别覆盖完整身份、metadata、saved operands、
  terminal inventory、slot 替换与 private escape；失败保持原对象、文件和 stage sentinels。
- [ ] 保留公共六个恒真、三个动态场景的规模与原预算；低预算原子失败、缓存隔离与变换
  后失效独立验收。
- [ ] provider/consumer 分开 compile，consumer 无 provider Python；完整 closure link 保存
  实际 transaction input/output stages。同一 final 双 emit 并生成完整 pycircuit_sim。
- [ ] C++ workers1/2 与 RTL 对照独立数值、完整混合事件顺序及源派生 IDs/invocations/
  ordinals/logical owners；已提交 observations 后的 assert failure 不发布失败 epoch。
- [ ] 真实四态 RTL 补充 assert false/X/Z、guard X/Z、reset、held/falling 与实际 state/history
  零提交；module-only 探针不能替代上述 generated System。
- [ ] 独立 review、短 API/examples 和文档 checks 绑定最终候选；nightly、综合及时序单列
  NOT-RUN，未执行不得视为通过。

完整基础 Method 仍需[剩余矩阵](extension-roadmap.md)的 conditional calls/effects、
partial state、resources/conflicts/causality 与效果安全优化。whole-Bits 分支的独立合同
见[branch state SSA](branch-state-ssa.md)。这个切片完成不能关闭整个 issue #307。
