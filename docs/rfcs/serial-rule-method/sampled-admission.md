# 事务采样准入扩展规格

状态：待完整实现的目标合同；本文件不是动态 guard 执行或测试通过声明。
实现检查点为 `3078e75f6a354c6a947a7d31e1a7a930978d9a0a`，该检查点已有
恒真跨实例 Method lowering。本规格承接其原实例接线，定义下一切片的精确共同 IR、
验证责任、Runtime 生命周期和验收出口。源码总合同见[source-semantics](source-semantics.md)，
全体扩展仍按[roadmap](extension-roadmap.md)分层交付。

本合同整理自独立架构与审查确认的 sampled admission v2 及 v3 补充：真实 architect
`gpt-6-astra/xhigh`、独立 critic `gpt-5.6-sol/high`、独立 decomposition
`gpt-6-astra/medium`。历史审查只证明设计经过复核，不证明实现完成。
本文件完整承载合同；实现和测试不依赖本地日志或聊天记录。

## 范围与作者接口

继续限定直线调用、同一实际物理域、whole Bits 状态、每个实际 owner 唯一外层 writer。
事务内 assert/观测与 Method System 闭环的精确扩展合同见
[body effects](sampled-body-effects.md)。分支、部分更新和资源分别验收。
不得为完成本切片偷偷开放这些能力，也不得缩减它们的最终验收要求。

复用 `@module`、`@rule`、`@method` 及现有 `guard=`；不新增作者可见 valid、fire、
时钟历史或 knownness API。Rule 定义自主事务入口，Method 调用归入该外层事务。
普通 success/retry 仍是数据，不按字段名参与准入。

目标源码：只有 guarded Rule、没有 Method/状态的模块也必须进入事务分类。
普通 Module 的 typed 输入保持原接口；这不放宽 Method owner 的源码限制。

```python
from pycircuit import module, rule, u1

@module
def GuardOnly(enabled: u1):
    @rule(guard=lambda: enabled)
    def tick() -> None:
        pass
    tick()
```

此处 Module 省略返回标注，不能改成当前不支持的 Module `-> None`。
嵌套 Rule 注册仍为零参数静态调用；不能插入 dummy Method/状态以绕过分类缺口。

**每个实际 `ac.transaction`** 降低后均有采样 metadata、全部 guard、显式 clock/reset
capture 和 history binding，包括裸 guard、字面 true/false、total 有值返回、只读和
完全无状态事务。totality 只控制普通结果逃逸，不豁免物理采样义务。
不源于 transaction 的普通组合 `ac.rule` 保持原合同。

## 源 guard 位置与发布 hard break

每个 `ac.method` / `ac.transaction` 必须具有 `ac.guard_location: DictionaryAttr`，
复用既有闭合 SourceSpan：

```text
{path: StringAttr, line: IntegerAttr, column: IntegerAttr,
 end_line: IntegerAttr, end_column: IntegerAttr}
```

使用既有坐标表示和 `verifySourceSpan`，不新增 span 类型。显式 guard 保存整个 Lambda
的捕获坐标；裸 guard 保存整个 FunctionDef 坐标，包括未调用 Method。位置必须来自
`sourceSpan(at, source.path, guard ? guard : declaration)`，不得从注册点、MLIR Location
或生成 literal 猜测。共享 `verifySourceGuardLocation` 验证 SourceOwner、SourceSpan
以及两者 path 完全相同。

MethodRow 和 AutonomousRow 均增加必填 `guard_location`，其 owner 是声明 provider 的
Module/ModuleImport。`SourceMethodView::getGuardLocation()` 在定义/导入上读取同一事实；
`GuardBoundary::location` 直接保存原 callable 的属性。link 重新由 body 推导摘要，拒绝
结构合法但已经篡改的位置；哈希不能替代 body authority。

lowering 生成的 RequiredCheck 和 expect 位置必须与原 GuardBoundary 完全一致。
重复调用共享声明 span，但有不同 invocation 身份。生成最终纯 Rule 时移除
`ac.guard_location`，位置由 RequiredCheck/expect 承载。同步更新 producer、getter、
verifier 和所有保留正例；缺失旧字段直接拒绝，无默认值、别名或兼容格式。

## 最终 IR 的闭合 schema

以下属性只允许出现在最终硬件中的相应对象；source body/interface 或其他 op 上必须拒绝。
所有新增 index/count/ordinal 是 signless i64 IntegerAttr，按有符号值检查非负及上下文
边界；只有 predecessor 可为 -1。不能仅调用宽松的 `decodeU64`。数组使用 ArrayAttr，
字典精确包含所列字段。复用 Occurrence、OwnerRef、StateID、CheckID 的既有闭合验证器。

每个降低后的事务 Rule 具有：

```text
ac.sampled_admission = {
  clock_capture: Index, reset_capture: Index, fire_result: Index,
  guards: [{id: CheckID,
            invocation: [{call: Occurrence, receiver: OwnerRef}, ...],
            predecessor: i64}, ...],
  body_checks: [{id: CheckID, invocation: Invocation}, ...],
  observations: [{id: ObservationID, value_count: Index}, ...]
}
```

- clock/reset 索引实际 Bits1 captures；无状态/没有其他使用时也必须插入。
- guards 非空，包含 plan 的所有 guard，包括 literal。第一个 predecessor=-1，其余为 i-1。
- CheckID 保持 `{registration, check, obligation}`。registration 是原外层注册 occurrence，
  check 是实际 Rule/Method 声明 occurrence，obligation 为该 guard 的序号 i。
- invocation 是从外到内的完整调用 frame 链；外层 guard 为 []。每帧使用实际 call occurrence
  和相对事务根的已解析 receiver OwnerRef，不使用源码变量名。链在事务内唯一。
- RequiredCheck 保持 `{id, kind, location}`；该 Rule 的 required_checks 精确包含按顺序
  对应的 guard_known 行，随后是 body assert 行。其他普通 assert/range Rule 的合同不变。
- 六字段全部必填，空数组也必须保留；四字段旧格式直接拒绝。效果身份、完整布局与
  重复 invocation 合同见 [body effects](sampled-body-effects.md#exact-layout)。

受影响 Module 具有：

```text
ac.transaction_wiring = {
  states: [Occurrence, ...],
  incoming: [{state: StateID, data: Index, enable: Index}, ...],
  writers: [{registration: Occurrence, ordinary_results: Index,
             candidates: [{state: StateID, result: Index}, ...]}, ...]
}
```

- states 枚举原直接拥有的 SourceState declaration；每项解析为保留该 occurrence 的一个
  直接 DFFE，不按名称识别状态。StateID 保持 `{owner, declaration, element: []}`。
- 此属性内所有 StateID 都相对**属性所在 Module**。直接状态的 instance_path=[]；
  incoming 从接收 Module 起算，candidate 从 writer 的词法 Module 起算；父级组合时
  只加一次真实 child occurrence 前缀，不能混用 package-root 坐标。
- incoming 第 j 行恰好覆盖 appended 输入 `source_inputs+2j` 的 D 和其后 E；
  覆盖所有 appended 输入且无缺口、重复或额外项。保留 `ac.internal_ports` 原前缀及根
  默认值合同，不改变公共根 ABI。
- writers 每个降低后的事务恰好一行，包括无写事务；registration 解析到一个实际 Rule。
  candidate owner 和 result ordinal 均唯一。

设 O 为普通结果数、W 为 candidates 数、G 为 guards 数、A 为 body assertions，
observation j 有 Kj 个 values，结果布局必须为：

```text
F = O+W; C = F+1+sum(j)(1+Kj)
ordinary = [0, O); candidates = [O, F); fire_result = F
observation_j_path = F+1+sum(t<j)(1+Kt); values follow its path
guard_i_predicate = C+2i; guard_i_reached = C+2i+1
assert_i_condition = C+2(G+i); assert_i_path = C+2(G+i)+1
total = C+2(G+A)
```

candidates 精确覆盖连续 slots；guard 的 ID 等于对应 RequiredCheck ID，恰好一个
expect 消费其 predicate/reached suffix。body terminals 必须消费各自独立的指定结果槽。
non-total 必须 O=0；有值事务继续要求全部
guard 经既有 literal/identity 证明恒真。不得通过隐藏 valid state 放宽返回限制。

## 两层验证 authority

**源到最终的验证**从完整原始、已验证的 state plan 和 summaries 构造预期事实。
`LoweringPreparation` 必须保持原 plan 存活至 detached candidate 验证成功，逐项比较：
predicate suffix 与映射后的原 guard、candidate 与最终版本 RHS、state read 与计划版本、
call result 与计划 yielded value、物理 captures 与原 domain、metadata 与独立构造的身份。
包括 guard span 和完整 inventories，不以候选自己声明的 metadata 证明自己。

所有定义（含未选中定义）的 companion 集合必须精确等于：直接转换 SourceState、直接
降低 SourceTransaction、或存在预备向上/向下路由的 Module 集合。三数组均必填；仅向上
路由的 Module 可三数组为空。缺失、多余 companion 或 inventory 都在这一层拒绝。
先完成 detached graph 和原 plan 对照，再执行 native final/shared hardware 验证，最后
原子替换输入；不得在 SourceState 已擦除的半降低 clone 上调用 source op 验证。

**独立 final verifier**验证留存记录与实际 SSA/接线一致：有 sampled Rule、guard_known
obligation/expect 或 internal_ports 的 Module 必须有 companion；candidate/incoming
指向的终端 state owner 必须有匹配 inventory。每个留存 companion 均验证 placement、
schema、routes 和 writers；没有 sampled/check/internal-port witness 的空 companion 拒绝。
仅实例化受影响 child 不要求本 Module 自带 companion。

final verifier 无法重建已完全擦除的源历史，例如所有标记被移除后的孤立未写 DFFE。
它也不能仅从结构证明某 DFFE 历史上来自 SourceState。精确历史集合由原 plan 验证负责；
不得新增另一个 provenance tag 假装消除了这一边界。两层负例分别验收，互不替代。

## SSA identity、顺序准入与物理接线证明

identity endpoint 包含实际 SSA、实例上下文和解析后的类型。只允许以下透明传递：

1. 同宽、静态 low=0 的 bits.extract。
2. Rule argument 到对应 capture。
3. Rule result 到对应 yield，但在 candidate/fire/check 边界停止。
4. 普通 Instance result 到对应 Module yield。
5. callee Module argument 到实际 Instance operand。

解析真实定义和 ordinals，primitive 输出为终点；不按名字或表达式相似性合并 clock。
select、resize、concat、算术、resource、Collection 不属于 identity route。
memoization key 包含上下文与 stop policy；环或预算超限失败关闭。

从原顺序 guard `g[i]` 独立构造期望前缀：

```text
R0 = true
Ki = case_eq(g[i], one)
Pi = Ri & Ki
R(i+1) = Pi
fire = P(G-1)
```

证明 normalizer 只接纳 identity、已知 Bits1 zero/one、任一参数顺序的
`case_eq(raw_guard, one)` 和 Bits1 AND。表示为 false 或归一化的 KnownOne(raw SSA)
原子集合；只使用 literal、true/false、AND 结合/交换/幂等规则，不调用 SAT 或通用等价
解释器，不用 ordinary eq 替代。逐项检查实际 reached 和 fire；后续 false 不能屏蔽先到达
的未知 guard。old Q/词法值无前缀要求，guard 使用进入前缀，body 值使用当前获准前缀，
call result 使用含内部 guard 的退出前缀；纯消费者保留最晚必需前缀。

物理验证必须覆盖全部 inventoried owners、候选及 unwritten hold：

- 每个实际 DFFE/child incoming pair 只来自对应本地 candidate+fire、对应 incoming pair，
  或 E=0 的合法 hold（本地 D=Q；未使用外部 D=0）。D 与 E 都通过 identity 追踪到规定
  stop slots；正确 fire 不能掩盖错误 D、交换 owner、漏路由或伪造常量 E。
- 按普通 Instance 自底向上组合实际 owner writer；zero pair 不构成 writer。包括子级
  本地写入在内，不同外层 writer 冲突必须拒绝，不从未使用或 identity write 推导无写。
- 每个 writer 的 clock/reset 必须与所有目标 DFFE 的真实 pins identity 一致；DFFE pin
  ordinal 来自已验证 primitive 合同。无状态事务对齐所属 Module 的 domain_inputs，
  不借用任意 DFFE history 或祖先 reset。
- 单独检查 candidate/fire/private routes 的 use；终端必须恰好是对应 D/E，check suffix
  终端是对应 expects。原公共 Module 输出、无关 Rule capture、未声明 primitive pin 等
  私有值逃逸拒绝；已声明并验证的 appended internal routing 是允许的传递路径。
- 继承[跨实例 Collection 限制](cross-instance-lowering.md)，不因采样扩展自动解除。
  已准入 check-only Collection 必须有逐 occurrence/lane 的独立 history。

## 检查表示与共同分析 API

扩展既有 `ac.expect` 为 `condition, path, sampling[]`：assert/range 的 trailing
sampling 为空，guard_known 恰好 `[clock, reset]`，都是 Bits1。condition 是 raw guard，
path 是 reached；sampling 与所属 Rule captures 通过 identity 验证。保持 effectful
Module placement、零结果和 RequiredCheck/expect 精确配对。

`ac.bits.compare` 增加 `case_eq`，等宽输入、已知一位结果，精确比较 0/1/X/Z。
C++ 在现有 `compare<ComparePredicate::CASE_EQ>` 中调用 `parityEquivalent` 并封装
known bool；RTL 使用 `===`。复用已知位/噪声掩码逻辑。普通 eq 不变，X/Z 的 `eq(x,x)`
仍为未知；无新增 Python intrinsic，也不建立第二比较实现。

共同 Dialect/check owner 提供以下责任 API；具体 C++ view/endpoint/budget 类型及错误
返回形式须在派发时冻结，不增加 pass 或编译路线：

```text
parseSampledAdmission(rule, analysis, bindings, budget) -> SampledAdmissionView
parseTransactionWiring(module, analysis, bindings, budget) -> TransactionWiringView
resolveTransactionState(module, StateID, analysis, bindings, budget) -> PhysicalStateEndpoint
normalizeIdentityEndpoint(endpoint, stopBoundaries, analysis, budget) -> NormalizedEndpoint
prepareSampledBoundaries(definition, analysis, bindings, budget) -> optional PreparedSampledBoundaries
proveSampledAdmission(view, prepared, analysis, bindings, budget) -> LogicalResult
verifyTransactionWiring(analysis, scopes, validation, budget) -> LogicalResult
```

views 包含已解析 ops/indices；physical endpoint 包含真实路径/DFFE/绑定 payload。
LowerTransactions 的 `verifyTransactionMaterialization` 比较 original preparation、
final candidate、materialized value mappings，不另造 evaluator。

`PreparedSampledBoundaries` 是一次只读验证中借用 immutable IR 的准备结果，不是缓存的
验证结论。仅 factory 可以构造；结果不可复制，公开只读 scope/boundaries。它必须包含
该定义完整的 candidate、fire、observation path/values、guard、assert/range 边界，不能因本次 Rule 不使用某边界
而省略。普通无 sampled/wiring 定义成功返回空 optional；构造失败与空结果必须区分。

消费前核对精确 definition、definition-local 空路径和完整 bindings。不同 bindings 不得
复用；IR 修改后必须失效。同一准确 scope 内的 definition checks、sampling catalog 和
多个 Rule 可以共享准备结果，每个 Rule/完整 bindings 必须有完整 prefix 证明。实际 occurrence 的物理接线、
private-use 与 clock history 验证继续独立进行，不能按定义名去重实际实例。
旧的无 prepared 参数证明入口必须删除，不保留默认参数或 fallback。

既有 source-check owner 中的 `SampledAdmissionValidation` 在一次只读调用中保有准备
结果与已经成功的 definition-local Rule prefix 证明。它绑定同一 analysis 和 budget，
不允许复制/移动；`verifyInvocation` 拒绝错配。`prepare(definition, bindings)` 只通过
原 factory 构造完整边界，稳定保存指针；普通定义成功返回空指针。
`verify(rule, bindings)` 每次从真实 Rule 重新解析 schema，只有精确 definition、完整
bindings 与 Rule 身份已成功时才复用 prefix 证明。首次仍运行同一个 prover，失败不记录。
调用者不能注入 view 或“已成功”标志；Prepared 对象本身仍不授予证明权威。

Builder 将同一个 validation 贯穿 catalog、绑定检查和 wiring；独立 native 入口各自
新建。相关 IR 修改必须销毁旧 context 后重新验证。不得跨独立 source/final 验证调用
保留缓存。定义局部成功不代表整份 source checks、实际 occurrence 或独立根成功；
所有 expect 配对、实际 stops、D/E、domain、private-use 仍按其原责任验证。
context 查找、完整 bindings 比较、entry/Rule 存储均提前计费；native raw preflight
保留既有上界并覆盖新增记账，不通过减少限额预付来使样本通过。

同一 writer 的候选均已逐项验证目标、类型、域、冲突及其规范化 enable 等于 fire 后，
允许对该 fire 的完整 Enable 扇出只遍历一次；每条 Data 路由仍须验证。该复用不跨 writer
或实际 occurrence，不删 incoming Enable、末端、逃逸、环和 dead-end 检查。

扩展既有 HardwareAnalysis 数据：

```text
HardwareCheckSamplingBinding {ModuleOp definition; Value clock, reset; uint64_t localOrdinal;}
HardwareCheckSamplingOccurrence {uint64_t bindingIndex, ownerOccurrence, ordinal;}
HardwareCheckBinding: optional samplingBindingIndex
HardwareResolvedSourceCheck: optional samplingOccurrenceIndex
HardwareSourceCheckPlan: samplingBindings, samplingOccurrences
```

只去重完全一致、归一化后的 definition-local SSA clock/reset pair；每个实际实例/lane
生成独立 occurrence/history。普通 reset authority 单独保留。getSourceCheckPlan 派生，
verifySourceCheckPlan 重算全部字段；任何相关 IR 修改使旧 plan 失效。
definition check verifier 负责 schema/suffix/prefix，package wiring proof 在 native final
verify 后由共同 hardware validation/check-plan preparation 调用；局部 Module verifier
不得反向请求全包 check plan，避免递归依赖。

复用 `HardwareSourceCheckPreflight.h` 的 `SourceCheckBudget`；在 metadata、路径遍历、
原子集合、cache、frame、occurrence/history/snapshot 分配前付预算。失败不暴露部分 plan，
也不改写原 IR 或已发布输出。不得各 owner 另起互不相干的预算系统。

## 采样与全系统生命周期

从 DFFE 现有 clock 判定提取无 LLVM 依赖的 `evaluateSampledClock(previous, clock, reset)`，
供 `dffe_kernel` 与 checks 共用；返回 nextClock、rising、resetActive、fault
（None / UnknownClock / UnknownReset）。保持原 DFFE 的异常及诊断信息。

| 输入 | 结果 |
| --- | --- |
| clock X/Z | UnknownClock；nextClock 保持 previous |
| 已知 clock，非 rising | nextClock=clock；不检查 reset |
| rising，reset X/Z | UnknownReset；不提交候选 clock |
| rising，reset=1 | resetActive=true，新增 guard fault 不激活 |
| rising，reset=0 | resetActive=false，检查实际到达的 guard |

verification-only history 保存 committed clock、pending clock/validity 和冻结采样事实；
不增加作者可见 Q、端口或可综合功能状态。

| 阶段 | 必须行为 |
| --- | --- |
| Work | 清 completion，冻结 pins/raw/reached，用 committed history 求值；成功物理采样才 stage clock，fault 留给 Precheck |
| Precheck | 先验证整个 closure 快照完成，再检查物理 fault，最后按确定 guard 顺序检查 |
| Xfer | 全系统成功后提交 pending history；false fire、hold 和 falling 也提交合法 history |
| Discard | 保持 committed history，丢弃 pending 与 snapshots |
| 显式 Reset | stage false、清检查，经既有 Reset/Xfer 提交；不重读普通 pins，不要求 guard completion |

guard fault 的精确条件为 `rising && !resetActive && reached && !predicate.isFullyKnown()`。
reached 必须经证明是两态；未知 reached 是完整性错误。已知 false guard 通过 knownness
检查但禁止本 Rule fire。clock X/Z 每个 Work 都是 fault，不能被 reset/false guard 屏蔽；
reset X/Z 仅在 rising fault。Work 后 host 改 pins 不改变冻结的判断。

保留现有 SimSystem barrier、Precommit 观测及 discard 协议。真实硬失败后 SimSystem
仍为 sticky Failed，必须先观测零提交，再按既有 Reset/重建恢复；不能把“修改输入”
描述成失败后的合法重试。单独 Discard 未提交的 prepared rising 再用同一 high 采样，
应仍观察 rising；该测试与 sticky failure 恢复分开。

managed RTL 复用现有 phase 1 PREPARE、phase 2 COMMIT（只在 root commit_ok===1）、
phase 3 DISCARD、phase 4 RESET_PREPARE。检查 history/snapshots 是 nonsynthesis
instrumentation；沿现有 local-error、root discard-before-fatal 和成功观测顺序执行。
不新增 runner protocol，不在后端重新解释 Method admission。

## 有界实现顺序与所有权

此表细化[实现计划](implementation-plan.md)，不要求每行创建新 pass。派发前 PM 冻结
具体文件和 C++ 类型；共享头文件、ODS、CMake、注册、安装及 docs 由 PM 独占。
实现者不得改同一批独立测试；依赖接线完成后再并行运行，不能边安装边启动 consumer。

| 切片 | 文件责任入口 | 依赖与出口 |
| --- | --- | --- |
| 比较前置 | `ACIRBitsOps.cpp`、两个现有 definition emitter、`include/gfsim/four_state.h` | 独立四态比较 oracle；不代表动态执行 |
| guard 位置前置 | `ACIRSourceContracts.cpp`、`ACIRTransactionOps.cpp`、`SourceTransactionSummary.cpp`、`SourceMethodResolution.cpp`、`SourceTransactionStateAnalysis.cpp`、`PythonImportTransactions.inc` | producer/view/summary/body 一致 hard break；不代表采样已实现 |
| 共同 schema/proof | `HardwareSourceChecks.cpp`、`HardwareSourceCheckPreflight.{h,cpp}`、`HardwareAnalysisValidation.cpp`；可按规模拆私有 `HardwareSampledAdmission.{h,cpp}` | 前置完成；冻结上述 shared API，final-only/schema/identity/prefix/wiring 的原生正反例 |
| 源分类与有效性 | `PythonImportTransactions.inc`、`SourceTransactionStateAnalysis.cpp` | GuardOnly 不依赖 dummy；typed plan 保存前缀和正确结果使用；与位置前置串行编辑 |
| 统一物化 | 既有 `LowerTransactions.cpp` | plan + shared proof 完成；全部 transactions 物化，原 plan 对照成功才替换 |
| Runtime/C++ | `include/gfsim/dff.h`、既有 `HardwareEmitCppChecks.{h,cpp}`、C++ definition emitter | 共享验证计划已稳定；LLVM-independent evaluator、原 DFFE 回归和 history 生命周期 |
| RTL | 既有 `HardwareEmitVerilogDefinition.cpp` 检查/phase owner | 同一验证计划；真实四态、managed phases 和全系统零提交 |
| 独立集成 | 既有 transaction native tests、`serial-method-public.test` 及对应外置 oracle | producer→publish→link→相同 final 双 emit；workers 1/2、RTL、输出保护和独立复核 |

除 `include/` 明示路径外，上表 Dialect 文件位于 `compiler/lib/Dialect/ACIR/`，
importer/emitter 位于 `compiler/lib/Compiler/`，LowerTransactions 位于
`compiler/lib/Transforms/`。公共结构放现有 `HardwareAnalysis.h`，不增加转发头或重复
语义层。Runtime helper 若确需新文件，须先冻结精确路径、安装/CMake 所有权及移除范围。

## 验收矩阵与交付 checklist {#acceptance-checklist}

每项同时记录实现状态与 `PASS/FAIL/NOT-RUN`，绑定精确 candidate、命令和输出。
下列要求补充[已有 METHOD-* 验收](acceptance.md)，不是新的一套 gate。

| 场景 | 独立期望与负例 | 对应需求 |
| --- | --- | --- |
| uniform sampling | statefree true/false/dynamic、readonly、valued total 全部有真实域/history；无交易组合 Rule 不变 | `METHOD-RESULT`、`METHOD-FOUR-STATE` |
| guard 顺序 | true→false 本 Rule hold、无关 Rule 提交；X/Z→false 全系统零提交；false→X/Z 不激活下层 guard fault | `METHOD-GUARD-LOCAL-HOLD`、`METHOD-GLOBAL-ZERO-COMMIT` |
| 四态比较 | 全部 16 个符号对、跨 word 宽值/噪声 masks；普通 eq 未知语义保持 | `METHOD-FOUR-STATE` |
| 物理采样 | false rising 后 held-high X guard 不新增 fault；clock X/Z 每 Work fault；reset X/Z rising fault、held/falling 不 fault；显式 Reset | `METHOD-GLOBAL-ZERO-COMMIT`、`METHOD-FOUR-STATE` |
| history 隔离与冻结 | 不同实际实例/lane 独立；Work 后改 pins 无影响；prepared rising Discard 后同 high 仍 rising | `METHOD-INSTANCE-IDENTITY`、`METHOD-GLOBAL-ZERO-COMMIT` |
| 位置 authority | missing/type/malformed/foreign span 拒绝；合法但 stale 发布 span 拒绝；生成 check 不匹配原边界拒绝；Lambda/bare/重复跨源调用坐标准确 | `METHOD-GUARD-ENTRY`、`METHOD-SUMMARY-AUTHORITY` |
| 原 plan 证明 | 原 affected 集合的 missing/extra companion、错误 inventory、raw/candidate/call-result/span 替换拒绝；输入不变 | `METHOD-SUMMARY-AUTHORITY`、`METHOD-OUTPUT-PROTECTION` |
| 独立 final 证明 | wrong D/owner、交换 D/E、漏 route、重复 writer、private escape、错域、漏 witness companion、错误 suffix/order/ordinal、旧 plan 拒绝 | `METHOD-ALIAS-CONFLICT`、`METHOD-CAUSALITY`、`METHOD-EFFECT-RETENTION` |
| 预算与发布 | 路径/cache/occurrence 预算在分配前拒绝；compile/link/emit/stage output 失败保留原发布字节 | `METHOD-OUTPUT-PROTECTION` |
| 公共执行 | interface-only consumer、完整 closure、真实前后 stage、同一 final 的 C++ workers 1/2 与真实四态 RTL，对照独立 oracle | `METHOD-DUAL-FINAL-IR`、`METHOD-NO-ADDED-LATENCY` |

交付前逐项核对：

- [ ] 所有 schema 与 API 在现有 owners 内实现；旧字段格式 hard break，保留 caller 全部更新。
- [ ] 已验证原 plan 对照和 standalone final 两种 authority，各自有独立故障注入用例。
- [ ] 未将已知 false 变全局失败，未将 X/Z 控制变 false；snapshot、旧 Q、原 storage
  ownership、DFFE 数量和周期语义均保持。
- [ ] 全系统失败恢复前的 state、clock history、提交和成功观测均无前进；C++ 与 RTL 一致。
- [ ] 运行[既有命令](acceptance.md#commands-and-gates)：本 checkout build/install 完成后，
  非空 native/lit 选择，再适用短 API/example gate；保留 raw command/status 与完整产物。
- [ ] 独立 review 绑定最终候选内容；文档/原生 schema 通过不冒充公共源或执行通过。
- [ ] 按 [body effects checklist](sampled-body-effects.md#acceptance-checklist)验收 Method `@system`；分支/partial state、resources 和效果安全优化的未完成项继续在
  roadmap 保留；普通 System smoke 不替代 `METHOD-SYSTEM-CLOSURE`。
- [ ] nightly、长期 mutation、综合及时序若未运行明确记 NOT-RUN；不由局部成功推断。

本切片完成仅关闭上述受限 sampled admission。它不代表完整基础 Method、资源扩展，
或 issue #307 全部完成。
