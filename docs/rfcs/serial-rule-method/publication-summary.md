# 事务接口摘要合同

本文件细化 [MLIR 流程](mlir-pipeline.md)中的独立发布接口，以下字段和 API 是明确的目标合同。
原生摘要、结构验证、body 重验、公共独立发布和硬件执行分别验收；不得将前一层通过当成后一层完成。
接口格式以精确的英文标识列出，字段、作用域、失败边界和保守未知值均是合同的一部分。

后续 import 硬切要求 `semantic_kind` 必填：`hardware` 必须有 `dependency_summary`
且禁止 `transaction_summary`；`transactions` 相反。所有 canonical builtins 都是 hardware。
不得给未知依赖补 `[]`。硬切必须同时更新 producer、accessor、临时签名载体及全部 fixture。
本轮原生接口不提前改变 import 或 source-unit 发布许可。

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
`{name=StringAttr, declaration=Occurrence, function_type=TypeAttr<FunctionType>, argument_names=[StringAttr...], ac.parameters=[existing six-field parameter rows], ac.result_constraints=[existing hardware constraints], ac.return_form="none"|"single", captures=[Capture...], guard=ValueSummary, guard_effects=[EffectRow...], body_effects=[EffectRow...], results=[ValueSummary...]}`.
Use existing shared source constraint and required-param validators. Guard/results/effects callable scope. Result count signature exact. Method inherits caller sampling domain; do not invent one. Result validity is conditional on successful return, not unconditional dataflow authority.

AutonomousRow:
`{name=StringAttr, declaration=Occurrence, registration=Occurrence, domain=Domain, captures=[Capture...], guard=ValueSummary, guard_effects=[EffectRow...], body_effects=[EffectRow...], result_types=[TypeAttr...], results=[ValueSummary...]}`.
Declaration/registration are actual distinct roles. Values zero-formal callable scope, capture inputs module scope. Preserve read-only/zero-write transactions. Do not fabricate logical result constraints not in transaction IR.

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
Guard/body effect arrays preserve exact respective IR order; guard permits reads only per existing op admission. Keep read checks/write intent/call control even unused or known-false. Do not sort, dedup, inline or conjunction-reduce effects.

## Derivation

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
