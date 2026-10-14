# 事务接口摘要合同

本文件细化 [MLIR 流程](mlir-pipeline.md)中的独立发布接口，以下字段和 API 是明确的目标合同。
原生摘要、结构验证、body 重验、公共独立发布和硬件执行分别验收；不得将前一层通过当成后一层完成。
接口格式以精确的英文标识列出，字段、作用域、失败边界和保守未知值均是合同的一部分。

Import 硬切要求 `semantic_kind` 必填：`hardware` 必须有 `dependency_summary`
且禁止 `transaction_summary`；`transactions` 相反。所有 canonical builtins 都是 hardware。
不得给未知依赖补 `[]`。硬切必须同时更新 producer、accessor、临时签名载体及全部 fixture。
原生 tagged import 验证本身不构成独立发布或执行验收。

## 定义与接口的共同解析

`SourceMethodView` 借用实际 Module 及 Method definition 或已验证 row，两者互斥；
签名、参数、结果约束和 declaration identity 使用同一只读接口。
`ResolvedSourceMethod` 携带该 view、实际 receiver bindings 及 occurrence-based OwnerRef。
`lookupSourceMethod` 和两个 `resolveSourceMethodCall` 入口共用 receiver walker；
实际实例和摘要实例 row 都经既有 `HardwareAnalysis::bindCallee` 绑定，不制造临时 IR。

Package analysis 使用本包声明；header closure constructor 同步消费 canonical declarations
及 `SourceModuleLookup`，不保存借用 callback。Registry 在 primitive catalog、snapshot、
namespace 和显式 interface closure 权威验证之后调用它。普通 provider 必须属于调用源的
`ac.interfaces`，不能靠另一个碰巧提供的 header 补足未声明依赖。

所有 Method／实例边都检查，包括未使用和 false-path 调用。实际调用验证完整 SSA 类型；
摘要调用验证 receiver、arity 和已知 predicate fact 对应的 bound formal width。
摘要没有 opaque actual 的完整来源类型，不能伪造该项证明；provider body 重推导负责关闭
剩余权威。Method 与实例递归、超预算闭包及缺失 provider 必须诊断。

发布分类来自实际 typed operations 和实例图，而不是返回值个数。普通 typed-result
transaction module 保留值调用接口；Method owner 才提供符合当前构造合同的静态句柄。
提取先完成所有定义的分类／摘要，再 staged 替换；pair 验证重算分类和摘要，且反向要求
每个 header 模块声明都有实际 owning body。未完成 lowering 前，link 不得赋予事务 final 身份。

## Exact API in existing SourceMethodAnalysis.h/.cpp

The existing isValid(), lookup(SourceMethodOp), and isTotal(SourceTransactionOp) semantics are unchanged.
Member `mlir::FailureOr<mlir::DictionaryAttr> getPublicationSummary(acir::ac::ModuleOp definition)` with independent memoized success/failure. Free functions: `mlir::LogicalResult verifySourceTransactionSummary(mlir::Operation *moduleDeclaration, mlir::DictionaryAttr summary)` and `mlir::LogicalResult verifyPublishedTransactionSummary(acir::ac::ModuleOp definition, mlir::DictionaryAttr published, SourceMethodSummariesAnalysis &analysis)`.
Structural verifier accepts existing ac.module or ac.module.import for signature, source owner, scope; never constructs SourceMethodSummariesAnalysis or calls mlir::verify(package), never consults provider body for proof. Body verifier structure-checks, calls unique derivation, compares exact canonical attributes; no trusted/hash flags. Publication failure never invalidates successful existing totality analysis.

## Closed schema

Every dictionary has precisely the fields below; all fields mandatory, wrong types/extra fields reject. Ordinals are nonnegative signless i64. Type payloads are TypeAttr. Arrays keep stated order.

PredicateFact variants:

- `{kind="constant", value=BoolAttr}`
- `{kind="argument", ordinal=i64}`
- `{kind="unknown"}`
Only physical one-bit values may have nonunknown facts. Argument ordinal scope described below. No new integer interpreter or state-initializer proof.

Dependencies variants:

- `{kind="known", arguments=[i64...], captures=[i64...]}`
- `{kind="unknown"}`
Known arrays strictly increasing, unique, in range. Module scope arguments are physical input ordinals and captures empty; callable scope arguments explicit formals and captures indexed captures; autonomous has zero explicit arguments. Empty known means proven independent, not unknown. These are conservative may-dependencies, NOT executable hardware dependency_summary.

ValueSummary exactly `{fact=PredicateFact, dependencies=Dependencies}`.
Capture exactly `{type=TypeAttr, value=ValueSummary}`; capture value uses MODULE scope, array preserves actual capture operand order including unused captures. Within callable a capture fact may lift only proven constant; never reinterpret module argument as Method argument.

DomainEndpoint variants `{kind="input", ordinal=i64}` or `{kind="constant", value=BoolAttr}`. Input selects module physical one-bit input. Literal uses existing static evaluator. Domain exactly `{clock=DomainEndpoint, reset=DomainEndpoint}`. No unknown endpoint; arbitrary SSA clock/reset remains legal typed IR but fails publication request with precise diagnostic.

Top exactly `{owners=[OwnerRow...], methods=[MethodRow...], autonomous=[AutonomousRow...]}`. Direct summaries only; child references compose through explicit closure, never flatten child trees.

State OwnerRow:
`{kind="state", name=StringAttr, state=StateRef, type=TypeAttr, domain=Domain}`.
StateRef reuses existing `{kind="owned", declaration=Occurrence, element=[]}` for whole actual ac.state. No fabricated element identity.

Instance OwnerRow:
`{kind="instance", name=StringAttr, occurrence=Occurrence, callee=FlatSymbolRefAttr, parameters=[StaticExprAttr...], type_arguments=[TypeAttr...], input_types=[TypeAttr...], arguments=[ValueSummary...], result_types=[TypeAttr...]}`.
Arguments module scope, exact input arity. Preserve ALL direct instances including ordinary hardware/primitive. Names share actual state/instance namespace, occurrences unique; HardwareAnalysis owns actual static/physical binding. Structure verifies row shape; body rederivation and later registry closure verify callee binding. Preserve true op identity, no fabricated instance.

MethodRow:
`{name=StringAttr, declaration=Occurrence, function_type=TypeAttr<FunctionType>, argument_names=[StringAttr...], ac.parameters=[existing six-field parameter rows], ac.result_constraints=[existing hardware constraints], ac.return_form="none"|"single", guard_location=SourceSpan, captures=[Capture...], guard=ValueSummary, guard_effects=[EffectRow...], body_effects=[EffectRow...], results=[ValueSummary...]}`.
Use existing shared source constraint and required-param validators. Guard/results/effects callable scope. Result count signature exact. Method inherits caller sampling domain; do not invent one. Result validity is conditional on successful return, not unconditional dataflow authority.

AutonomousRow:
`{name=StringAttr, declaration=Occurrence, registration=Occurrence, domain=Domain, guard_location=SourceSpan, captures=[Capture...], guard=ValueSummary, guard_effects=[EffectRow...], body_effects=[EffectRow...], result_types=[TypeAttr...], results=[ValueSummary...]}`.
Declaration/registration are actual distinct roles. Values zero-formal callable scope, capture inputs module scope. Preserve read-only/zero-write transactions. Do not fabricate logical result constraints not in transaction IR.

Both callable rows require the existing closed SourceSpan as `guard_location`.
Its path equals the declaring provider Module/ModuleImport SourceOwner path.
It is derived from mandatory `ac.guard_location` on the source callable: the
entire explicit Lambda span, or the entire FunctionDef span for a bare guard.
Use captured coordinates, never reconstruct from MLIR Location or the caller.
SourceMethodView exposes the same span for definitions and interface rows;
provider body rederivation rejects stale or altered published spans. Missing old
fields reject without defaults or aliases. This location authority does not
itself establish dynamic guard execution.

Selectors:

- `{kind="field", name=StringAttr}`
- `{kind="any_element", index_type=TypeAttr<BitsType>, index=ValueSummary}`
ALL Table indices conservatively wildcard, even literals. Preserve full index type and corresponding index value; check actual nominal field/Table axis types. Do not fold false-path effects away.

Read EffectRow:
`{kind="read", occurrence=Occurrence, path=PredicateFact, state=StateRef, selectors=[Selector...]}`.
Write EffectRow same fields plus `value=ValueSummary`, kind="write".
Call EffectRow:
`{kind="call", occurrence=Occurrence, path=PredicateFact, callee=SymbolRefAttr @Module::@method, receiver=OwnerRef, arguments=[ValueSummary...]}`.
OwnerRef existing `{instance_path=[Occurrence...]}`; empty=self. Each receiver occurrence comes from validated real instance path, never string fabrication. Call actuals explicit order. All calls required; no required=true flag.
Check EffectRow:
`{kind="check", occurrence=Occurrence, path=PredicateFact, check_kind="assert", condition=ValueSummary, location=SourceSpan, message=StringAttr|UnitAttr}`.
Observe EffectRow:
`{kind="observe", occurrence=Occurrence, path=PredicateFact, observation_kind="log"|"report", spec=DictionaryAttr, location=SourceSpan, values=[{type=TypeAttr, value=ValueSummary}...]}`.
The check condition has the same implicit one-bit contract as a guard summary;
shape/scope validation does not replace provider body type authority. Absent assert
message is UnitAttr; an authored empty string remains StringAttr. Observation value
rows preserve their actual scalar types and callable argument/capture scope. Both
variants require exact fields and reuse the existing observation spec validator.
Spec item ordinals retain the existing decodeU32 contract; they are distinct from
summary reference ordinals and final ObservationID ordinals.

Guard/body effect arrays preserve exact respective IR order; guard permits reads only per existing op admission. Keep read checks/write intent/call control even unused or known-false. Do not sort, dedup, inline or conjunction-reduce effects.

## Source body checks and observations

`ac.method` 与 `ac.transaction` 的直接 body 使用已有 `ac.expect` / `ac.observe`。
Source assert 只允许 assert kind、condition/path 两个 Bits1 和空 sampling；source
observation 只允许 log/report、Bits1 path 和现有 scalar payload。Guard、纯 helper、
普通纯 Rule 及嵌套 pure region 不因此获得效果权限。

Source form 必须有原语句 `occurrence`，assert 使用既有 `location`，observation 使用
`ac.location`。Occurrence 的 definition 与 lexical Module 一致，SourceSpan 的 path 与
其 SourceOwner 一致。Source body 不得提供 `ac.check_id` 或 `ac.observation_id`；独立
发布时尚未确定 outer registration 与 invocation。Module-level final forms 保持其必需 ID。
Summary 结构验证使用实际 Module/ModuleImport 声明，不构造临时 op 或读取 provider body。

Producer 先验证静态 message/spec，再通过已有 expression owner 按参数顺序求值一次。
Method call 和 state read 形成在消费效果前的显式 op；后续表达式不能改写先前 SSA snapshot。
一条 log 产生一个 observe，literal-only log 保持零 value，report 恰好一个 value。
新事务 assert 的 authored path 为 true，不把 condition 合入后续调用或 guard 的 continuation。
已有普通 behavioral Rule 的 continuation 合同保持。`SourceMethodCallOp`、
`SourceTransactionOp` 和叶 observe 保留 observation resource write；effects 不因结果未用而删除。

Body assert 是后续 whole-system commit check，不能被解释为 Method 的 mayBlock guard。
实际 lowering 必须使用最终 fire，guard-known 检查则保留有序 reached prefix。Report 名字冲突
按实际展开的 logical owner 检查，不能在发布未调用声明时伪造调用次数。

source/publication 通过不等于执行支持。已有显式 state-plan records 的直线效果还须通过
[sampled body-effect 合同](sampled-body-effects.md)的语句时刻 operands、源计划认证、
共同 final proof 与生成 runner 的实际执行。未表示或不支持的效果继续原子拒绝；不得
先丢弃效果以生成可执行 final。完整 [Method System 验收](acceptance.md)使用同一
compile/link/emit 和 C++/Verilog flow。

## Derivation

Publication 的 1,048,576 work units 和 depth 128 上限保持。新 spec、items、message、
provenance 字符串及记录遍历/复制使用同一预算；不得给大字符串只计一个固定节点成本。

Reuse existing verified typed IR, HardwareAnalysis, shared source contracts and existing literal/static fact logic. Do not add AST analysis, a constant interpreter, or a second semantic compiler.

- BitsConstant: existing evaluator; known-empty deps.
- module block argument: module argument dependency.
- callable explicit formal: callable argument dependency; one-bit argument fact.
- callable capture block argument: capture dependency; only constant predicate fact lift.
- regionless admitted pure payload op: union known operand dependencies, any unknown => unknown.
- pure payload op with regions: unknown result dependencies.
- state/read/method.call/instance result: unknown dependencies and unknown facts (no initializer truth).
Predicate facts stay current direct formal/capture/literal proof, no extra algebra.
Module body allowed typed declarations, actual instances, admitted pure graph and yield. Reject publication on any unrepresented effects/structures (Queue/Collection/observe/expect etc), never silently omit. Totality-only analysis unchanged.
Canonical owners sort by state declaration/instance occurrence using compareClosedSourceStructure; methods by name; autonomous by registration occurrence. Dependencies sorted sets. Captures/actuals/results/selectors/effects stay original order.
Independent publication budget 1,048,576 work units and depth128, charge traversal and emitted records. Cache by definition Operation identity; invalidate with analysis.

## Composition and authority

Keep child callee references; no flattened child owners/autonomous copies. Multiple instances of same callee remain distinct occurrences. Later closure instantiates relative paths and binds child input domain through instance argument facts; unknown cannot prove domain. Existing Method recursion remains rejected including unused/dead paths. Native publication rejects recursive instance graph explicitly using visiting/visited, not only budget exhaustion. Structure verifier proves local shape/scope/state/local receiver references; external receiver suffix/callee Method/child summary authority belongs to header closure, not standalone Dictionary validation. Body verifier rederives each provider own summary exactly.

## 验收边界

独立原生测试必须覆盖 canonical roundtrip、每类记录的缺失／额外／错误类型字段、ordinal
和 domain 边界，以及合法形状的 guard、效果顺序、owner、逻辑类型、actual、capture、依赖
篡改被 body 重推导拒绝。两个同 callee 实例必须保留独立 occurrence；子自主 Rule 必须沿
instance 边可达，摘要大小不得按实例子树重复膨胀。

完整索引类型、wildcard、computed domain、失败缓存、foreign definition、递归、深度及预算
耗尽须有独立检查。Publication 失败不能污染已有 totality。修改 body 后的测试必须重建
analysis，不能用失效缓存代替实际候选。现有真实 source 与 transaction 回归保持。

公共独立发布还要求 provider Python 不可用时 consumer 编译成功、完整闭包逐 provider
body 重验，以及失败输出保护。原生检查不能替代这些证据，也不能替代 state SSA、admission、
C++／Verilog 和 `@system` 的执行验收。完整要求见[验收标准](acceptance.md)。
