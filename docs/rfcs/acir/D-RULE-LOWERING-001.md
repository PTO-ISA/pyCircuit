# Rule authoring and staged semantic lowering

## Keep Python rules simple and carry incomplete knowledge as typed markers {#D-RULE-LOWERING-001}
<!-- ndf: kind=decision level=must layer=L2 status=stable depends-on=D-BLOCK-MODEL-001 -->

**Context.** The current prototype exposes `atomic`, `firing`, explicit Queue
effects, and user-authored checks at the Python surface. Those spellings expose
implementation machinery before the compiler has inferred types, effects,
handshake requirements, or rule conflicts. They also force the frontend to
decide too early when information could be refined by later MLIR passes.

**Decision.** The target Agentic Circuit Python surface expresses what a rule
computes, not how the transaction is implemented. `@ac.rule` is the only
explicit schedulable boundary. Queue arguments define input token sources,
function parameters are immutable firing-local payload values, return values
define output payloads, ordinary control flow defines whether the rule is
enabled, and typed Table or Reg assignments define state-update intent.
User-authored Python does not spell `atomic`, `firing`, `peek`, `pop`, `push`,
transport `ready`/`valid`, `check`, `assert`, `commit`, or `rollback` to obtain
rule semantics. This does not reserve ordinary payload field names such as an
architectural Entry's `valid` bit.

A rule path that returns the fixed output payload tuple is a candidate firing
path. For a zero-output rule, reaching terminal function fallthrough after the
rule body or writing explicit `return ()` is a successful empty-output
candidate. An explicit bare `return` disables that rule attempt: it consumes no
input, produces no output, and commits no state proposal. All candidate firing
paths have one statically identical output arity and type signature. A
zero-input, zero-output rule must contain at least one potentially enabled Table
or Reg proposal; an effectless rule is rejected.

Control flow distinguishes rule guard from local effect enable. The disjunction
of paths reaching a successful fixed-signature return forms the functional rule
guard. A conditional Table or Reg assignment contributes its own path predicate
as that proposal's local enable; it does not disable a rule whose successful
return remains reachable. The first implementation accepts only reducible,
statically bounded symbolic conditionals. Symbolic loops, exceptions,
generators, recursion that survives elaboration, and branches that change Queue
topology are rejected before rule lowering.

The compiler lowers a rule through explicit, verifier-owned stages:

1. Capture a high-level `ac.rule` with its Python control flow intact.
2. Normalize the supported CFG into candidate returns and per-operation path
   predicates while preserving source locations.
3. Infer value types, provenance, ownership, Queue signatures, state access
   paths, and read/write/resource/Queue effect sets.
4. Refine or propagate typed markers for facts and obligations that later
   passes can resolve.
5. Materialize static diagnostics and path-qualified dynamic checks from the
   inferred types and provenance.
6. Materialize protocol and ready/valid handshake conditions.
7. Resolve or reject cross-rule conflicts and scheduling requirements.
8. Lower every schedulable rule, after its selection policy and predicate are
   explicit, to an internal `ac.firing` transaction with grouped proposals and
   one commit decision.
9. Eliminate every remaining semantic marker and verify marker-free Frozen
   ACIR before topology freeze, hashing, or serialization.
10. Lower the verified transaction independently to ACSim/gfsim or to verified
    PYC and the pyCircuit 6 backends.

When a pass cannot finish an inference but later stages can, it represents the
incomplete knowledge on the affected SSA variable with a dialect-owned
refinement type, marker op, or explicit result/block-argument metadata. One
generic bag of attributes is forbidden. The marker system has three classes:

- **Type constraints** cover payload type, Queue signature, rate, output arity,
  protocol shape, and time-domain compatibility. Their lattice is
  `unknown -> constrained -> exact`; join intersects compatible constraints,
  and an empty intersection is a diagnostic.
- **Value facts** cover committed-snapshot provenance, rule scope, Table or Reg
  owner, CandidateSet/Table identity, access path, and resolved time domain.
  A CFG join retains only facts proven on every incoming path. Incompatible
  identity-bearing facts require an explicit sum/select representation or a
  diagnostic; they never widen silently to an unowned value.
- **Pending obligations** cover dynamic bounds checks, protocol checks,
  handshake construction, effect classification, conflict resolution, and
  marker elimination. A CFG join takes the union of obligations and retains
  each originating path predicate. An obligation transitions only from
  `pending` to `materialized` and then to `discharged` by its named resolver and
  verifier.

Marker handling is monotonic and fail-closed:

- a pass may refine a marker, replace it with explicit IR, or diagnose it;
- transformations, cloning, inlining, CSE, and canonicalization must preserve
  every still-live fact and obligation through a marker-aware interface;
- a transformation that cannot apply the class-specific join and preservation
  rules is illegal while those markers remain;
- combining values applies the class-specific join above and diagnoses
  conflicts;
- no pass may silently discard a marker or replace missing information with a
  default value;
- only the named resolver may remove an obligation, and it must leave explicit
  checked/handshake/schedule IR or a verifier-consumable discharge record;
- every lowering boundary declares which markers it accepts and which pass
  must resolve them; and
- the pre-freeze verifier rejects every unresolved marker. Frozen ACIR,
  topology digests, caches, canonical ACSim, ACSim execution, generated C++,
  gfsim execution, PYC, Verilog, and release artifacts are marker-free.

Static facts that violate a contract are compile-time errors. Conditions that
remain legitimately dynamic lower to explicit checked IR with stable
diagnostics. Dynamic checks remain under the control-flow predicate that made
their source operation reachable; a disabled path does not speculatively
evaluate them.

The generated runtime diagnostic is enabled only for a provisionally selected
rule: its required inputs are valid, its normalized functional guard is true,
its required outputs and resources are ready, and conflict scheduling selected
it. An input-invalid, guard-false, backpressured, resource-blocked, or
scheduler-rejected rule reports no payload-dependent dynamic diagnostic and
commits nothing. Implementations may compute check predicates speculatively as
combinational values, but their diagnostic effects remain gated by provisional
selection and the originating path predicate. If an enabled diagnostic fails,
the runtime reports the stable error and prevents the complete proposal group
from committing; failure is not ordinary backpressure, a false guard, or a
partial firing. Queue readiness and output capacity are generated by handshake
lowering and are not observable as ordinary Python guard values.

Rule evaluation reads one committed snapshot. It constructs proposals without
publishing side effects and participates in deterministic conflict resolution.
Every rule reads the same start-of-Epoch committed snapshot; there is no
same-Epoch rule-to-rule state forwarding. Rules may be selected together only
when their Queue and state effects are statically disjoint, are accepted by an
existing explicit merge/replace contract, or are proven mutually exclusive.
Python order, MLIR order, and pass traversal order never create priority. A
remaining conflict requires an explicit arbiter or preemption primitive;
otherwise compilation fails. The compiler does not introduce implicit
fairness, round-robin state, urgency, or scheduling state.

Atomicity is per selected rule. One selected rule commits all of its Queue,
Table, and Reg effects together or none; independent selected rules may commit
on the same edge but do not form a global rollback domain. Runtime rollback is
not the implementation model. Specialized operations such as `ac.transform`,
`ac.broadcast`, or an identity Queue transfer may remain canonical lower-level
forms when they preserve the same transaction contract.

Every rule resolves to one exact time domain before freeze. Its Queue, Table,
and Reg effects belong to that domain; cross-domain communication uses an
explicit bridge primitive. An unknown domain never defaults to a global or
`default` domain. Rule evaluation is combinational proposal work within one
Epoch and does not advance time, insert a hidden microstep, or add a scheduler
cycle. New state comes only from explicit Queue latency, Table, Reg, bridge, or
arbiter primitives. In the synthesizable path, one accepted firing maps to one
commit edge of the resolved pyCircuit 6 domain.

The target pre-freeze `ac.rule` is a transient high-level operation with Queue
operands, immutable payload block arguments, ordinary typed state operations,
candidate returns, and source CFG. The target Frozen `ac.firing` has a fixed
list of input Queue operands, output Queue results, immutable input payload
arguments, one normalized guard, fixed yielded output payloads, explicit state
proposals/effect summary, stable rule identity, and exact time domain. It also
carries or symbolically references four distinct compiler-generated contracts:
path-qualified dynamic checks, handshake requirements, scheduler selection or
arbiter predicate, and the functional guard. Verifiers forbid folding any of
these into another because diagnostics, backpressure, scheduling loss, and
functional disable have different semantics. It does not contain user-authored
`peek`, `pop`, `push`, or checks. The current epoch `0.4` queue-effect
`ac.firing` is replaced rather than extended in place.

**Compatibility.** Current source-captured `ac.atomic()` and `.firing()` are
transitional implementation surfaces, not the target authoring contract. Their
removal is a hard break performed with matching frontend diagnostics, spec
updates, and migration examples; no compatibility shim creates a second rule
semantics. The internal `ac.firing` name remains available for the explicit
transaction IR produced after inference, checking, handshake construction, and
scheduling. This decision does not change CycleAwareSignal, PYC, or pyCircuit 6
timing semantics.

The implementation hard break increments the Agentic Circuit serialized
contract epoch from `0.4` to `0.5`. Epoch `0.4` producers continue to describe
only the current prototype until that implementation lands. The implementation
change updates ACPy, raw/Frozen ACIR, schemas, tools, examples, and consumers in
one closure; epoch `0.5` consumers do not accept the old queue-effect
`ac.firing`, and epoch `0.4` consumers do not accept the new rule contract.

**Implementation status.** Accepted direction; implementation is pending.
Until the hard break lands, current examples and tests document the existing
prototype behavior, but new semantic work must not deepen a dependency on the
Python `atomic` or `.firing()` spellings.

**Verification.** Closure requires frontend tests for the simple `@ac.rule`
surface and removed spellings; MLIR verifier tests for marker creation,
propagation, refinement, conflict, and final elimination; pass-boundary tests
for generated checks and handshake; deterministic scheduling and grouped
commit tests; and ACSim/gfsim versus PYC/C++/Verilog refinement evidence for
the supported synthesizable subset.

Coverage includes CFG joins for all marker classes; disabled, input-invalid,
backpressured, resource-blocked, scheduler-rejected, dynamically invalid, and
successfully committed rules; static disjointness, mutual exclusion, explicit
arbitration, and rejected ambiguous conflicts; marker-preserving and rejected
CSE/inlining cases; pre-freeze marker rejection; time-domain mismatch and
explicit bridges; and exact epoch `0.4`/`0.5` rejection in both directions.
It includes zero-input/zero-output state rules, one-input/zero-output consumer
rules, terminal fallthrough, explicit `return ()`, disabling bare `return`, and
rejection of an effectless zero-input/zero-output rule.

**Consequence.** Python remains a compact architectural language while MLIR is
the semantic source of truth. Complexity appears only after enough information
exists to verify it, and insufficient information remains explicit and
actionable instead of becoming backend guesswork.

**Source.** Repository-owner direction (2026-09-04), refining the target
`ac.firing` work tracked by PTO-ISA/pyCircuit issue 13 and the same-field writer
conflict work tracked by issue 25.
