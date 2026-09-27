# C3-DUT-IO revision A independent design review

Date: 2026-09-28. Verdict: **revise**.

Reviewer: `dut_io_design_review`, independent design validation. Actual dispatch
confirmed by the PM: `agent_type=default`, `model=gpt-6-astra`,
`reasoning_effort=xhigh`, `fork_turns=none`. The reviewer is a separate instance
from architect `dut_resource_design` and the PM who transcribed the proposal.
The reviewer did not edit either proposal or any implementation and did not
delegate. Writable scope was this evidence directory only.

This is a contract review, not product approval or execution evidence. No
compiler, generated DUT, ELF case, runtime failure injection, or backend parity
test was run. `approval-ready` is withheld until the findings below are closed
in a new independently reviewed revision.

## Reviewed identities

Primary checkout: `/Users/zhoubot/linx-isa/tools/pyCircuit`.
Observed HEAD: `a4a43606bac9ea16a45fcb19d9dcee79feda8ee8`.

| Input | SHA-256 |
| --- | --- |
| `docs/rfcs/migration/c3-dut-io.md`, revision A | `8308a814baec7dc36ca634028f6d2f4c4878d2d34b614a6ad2053c2493ecfa52` |
| `docs/rfcs/migration/c2-resources-transactions.md`, revision A memo | `7fb279f793865294e97d7be7c9412a5f8a8e1a18dc75a27db8555aa3a0686cd8` |
| Frozen C1-C | `5768e1571e56eb1a5963ff5d40dff1087de38ee997e9c52b5ef91f520da90dfc` |
| Frozen C2-C | `387cf52b129f864b87a1b2a388213a3fe36d81c330d94ced0e6696522b58a322` |
| Frozen C3-C | `0c476ced27519cf93427a89f77b9348d388e96db57fb183790144d24118b1170` |
| Frozen C2-N1-C | `ac7d56a403e21ac03186f17c2f75f9c8eb53a00dcb74677c5286219dc5f1e6c3` |

All six hashes were read with `shasum -a 256` and matched the supplied proposal
identities and approval records. Both draft files were untracked in the observed
checkout. Existing `.omx-state-locks*` and `examples/davo/` were left untouched.

Read authority included the pyc6 skill, repo-local design-review skill,
project governance, modernization plan, review-record requirements, capability
matrix, the frozen proposals and their approval records, applicable current
decisions including 0236/0262, and current ABI declarations. The migration
approval records take precedence for the proposed new route; existing active
product documentation is not evidence that the migration has already cut over.

Donor evidence was read from Git object
`b852ed83fa0288d0be7406bba0ed47be4b2c0f63` in
`/Users/zhoubot/Documents/GFSIM`, not from its governance/configuration dirty
overlay. SSM evidence was read from Git object
`fd0fcf9f21cc5e030af732b4952deed794554dbd` in
`/Users/zhoubot/Documents/.worktrees/SuperScalarModel-pyc-alu-bru`.

## Findings requiring revision

### R1 — P1: close the ingress-check authority and failure contract

Locations: `c3-dut-io.md:160–165, 318–323`; frozen
`c2-mlir-contract.md:218–226, 352–366`.

The draft requires illegal ranges and X/Z to be diagnosed and to inhibit every
commit, and says shared lowering must represent that behavior. It does not
define the actual common-IR ingress obligation, its scope, or how it binds the
external current values to the global failure condition. This matters because
C2 currently permits runtime `ac.expect` only with a rule owner, and its
CheckID/required-check machinery is rooted in a rule registration. `ac.dut` is
declared regionless and without operands/results, and this revision does not
say that it is a new intrinsic check authority or specify another check carrier.
An emitter cannot safely invent this missing exception to C2 or use the input
LogicalType as proof that unchecked external bits satisfy a mathematical domain.

Freeze either explicit intrinsic ingress semantics for `ac.dut`, including
independently verifiable binding to root currents and global admission, or an
exact additional operation/record contract. State how invalid ingress prevents
unsafe source evaluation as well as publication, how knownness is handled at
the two-state boundary without admitting four-state source values, and the
diagnostic/recovery behavior at a bad RTL edge. Also define reset precedence:
does an asserted reset ignore illegal external values, or reject that stimulus?
Do not silently acquire a new rule owner, failure latch, source check kind, or
backend-only guard.

The C++ invalid-drive case deliberately retains the old batch and leaves a
healthy model usable; an illegal RTL edge is a different observation point.
Specify their separate failure/recovery expectations instead of inferring a
common next-cycle behavior. Required mutations must remove or redirect the
ingress obligation/global gate and fail final verification through both emit
entries. RTL X/Z execution needs a genuine four-state simulator; a two-state
Verilator run or a text scan does not establish this requirement.

### R2 — P2: freeze the actual C linkage of the extension header/query

Locations: `c3-dut-io.md:203–224`; existing
`simulator/gfsim/include/gfsim/model_api.h:6–8, 83–105` and
`compiler/acir/lib/CodeGen/QueueGraphGenerator.cpp:9113–9116`.

The new header is presented as an exact ABI declaration, but the shown query
has only `AGENTIC_MODEL_EXPORT`; that macro supplies visibility, not C linkage.
The snippet does not state the include of `model_api.h` or the C++ `extern "C"`
guard. Copied literally into a C++ header/definition, it does not establish the
unmangled optional symbol that external callers must discover. The existing
lifecycle header and emitted definition explicitly establish that linkage.

State that `gfsim/model_dut_io.h` is a standalone C/C++-compatible installed
header including the lifecycle types, with C linkage for the query and its
definition, the existing export/calling-convention policy, and the fixed table
layout checks. Add C and C++ caller compilation plus actual exported-symbol
lookup/link checks. This is a design closure request, not a claim that a new
header has already been implemented incorrectly.

### R3 — P2: specify the state transition after unexpected I/O failure

Locations: `c3-dut-io.md:277–291`; frozen `c3-driver-runtime.md:232–254`.

The table specifies validation rejection and the prose adds caught unexpected
internal failures returning `RUNTIME_FAILURE`, with no batch/output mutation.
It never states whether such a failure leaves a healthy model Ready/Completed
or transitions it to Failed. C3 explicitly distinguishes ordinary argument
errors, reset failures and execution failures; a status alone does not determine
the lifecycle. Consequently, a subsequent step/sample/reset has no unique
expected result for this newly admitted case.

Choose and state the transition for each I/O operation, while retaining the
specified old input batch, last complete snapshot, time/statistics, destination
atomicity and first-failure diagnostic rules. Include fault-injection tests for
the chosen continuation/reset behavior. State that typed-helper domain
rejection participates in the same serialization, diagnostic and borrowed-buffer
invalidation contract rather than merely returning a locally constructed status.

### R4 — P2: make the external-caller and acceptance packet concrete

Locations: `c3-dut-io.md:242–303, 325–345`; governance interface-packet
requirements; frozen `c3-driver-runtime.md:324–342`.

The draft has a useful new Python example and a planned pytest filename, but
no complete before/after host caller or I/O-specific affected-caller/deletion
inventory. The broad statement that old sink arrays/private access will not
return is not the required mapping of what callers change and what gets
deleted/rejected. The gate description also leaves the new public C linkage,
installed capability handshake, four-state ingress execution and failure
continuation without concrete test lanes.

Add a minimal Runtime-only C++ client using only `generated/dut.h`, showing
create/configure/reset, the exact optional-table compatibility checks,
drive/step/sample, and destruction. Explicitly contrast the previous portless
contract with that caller. List the new/changed runtime header/export,
generated public helpers and source-owned headers, SDK/platform/release-index/
consumer-lock schemas and caller tests. State whether this extension has no
additional deletion beyond the frozen M5 retirement set, and name the rejected
legacy external-I/O route/caller patterns; consumer sink-array migration stays
in SSM.

Bind named acceptance cases to real current-checkout compile/link/emit and
Runtime-only compile/link/run commands, including independent header-only
producers, ABI/C-linkage/layout, absent/null/incompatible query, capability-set
mismatch, source/default/root/binding mutations, and four-state RTL ingress.
Specify nonempty execution and no skipped required cases. Existing C3 publication
and hierarchy obligations may be referenced rather than duplicated. These may
remain planned tests; no execution claim is required to finish this design.

## Positive conclusions and scope checks

- The proposal identifies external system parameters as a pyCircuit extension.
  Donor `ObjectFrontendProposal.md:737–806` describes a closed source system,
  and donor `PythonLower.cpp:568–571` rejects system lowering. It does not
  manufacture donor implementation or approval.
- Constant defaults as boundary reset/idle values are distinguished from
  ordinary module connection defaults and static parameters. Exact transitive
  effects determine direction; root-only systems and the lack of bidirectional
  host/model mutation are coherent with the admitted C1 subset.
- `ac.dut` gives an explicit root binding and storage authority rather than
  permitting private-state access. The defined input identity, one output
  DFFE, forwarding, same-StateID R+W behavior, declaration order and binding
  coverage preserve the source-owned model. The new external-current meaning
  is declared rather than silently implemented as an extra register.
- The legal-stimulus timing and oracle are internally consistent: reset output
  7; invalid-request hold; retained request 3 produces 3 then 6; rejected
  request/bias batch preserves the old input and produces 9 on the next step.
  The model authors handshake and repeated-command behavior.
- Reset/commit snapshots and Failed sampling preserve C3's possible partial
  internal Xfer publication while avoiding exposure of a partial public
  snapshot. No rollback of all internal state is falsely promised.
- The payload layout is clear: ordered padding-free bytes, explicit bool,
  little-endian integer encoding, byte-width sign extension, full declared
  range checking before narrowing and checked sizes. Source-owned nominal
  transport structs and C3 name/collision rules are sound in direction;
  equal-layout records remain distinct and no cross-release C++ layout ABI is
  claimed. The revised external-client gate should make the illustrated
  namespace/entry naming directly testable.
- The separate optional table and capability preserve the frozen lifecycle
  table's size and version. Missing capability is fail-closed, and no runtime
  schema/trace/instruction transport or second model runtime is introduced.
- Fixed `default` clock/reset, no combinational outputs, and explicit exclusions
  of public list/tuple/Enum, general four-state values and resource ports keep
  the proposed scope bounded. Full system, resources and other capability rows
  cannot be closed by this packet.

## Resource memo assessment

The resource memo is **coherent bounded advice**, not a complete resource
proposal and not resource acceptance. No blocking finding is raised solely
because it leaves the enumerated future packets unresolved.

The ordinary FIFO equations match donor source behavior: reads use old state,
empty simultaneous put/take has no bypass, full take/put may replace, and reply
publishes after Xfer. The memo correctly avoids inferring Queue semantics from
class names or lists and catches the unapproved dependent-annotation issue.
The donor `SimQueue` source confirms resident/pop/push self-credit and the
throwing/allocation work in Xfer; those are not a no-fail multi-owner commit.
The memo preserves selected-branch all-or-none obligations from Decisions
0236/0262 and correctly keeps Slot release/refill distinct from a depth-one FIFO.

SSM evidence confirms the claimed separation: the actual paths are under
`model/pyc/davo/`; `core/core.py` invokes the empty `spe/spe.py`, while
`spe/ooo/ooo.py` and `spe/iex/iex.py` are independent assemblies. The ELF test
contains exact PC/register, 1–4 in-flight with peak overlap, init/run/drain and
failure expectations. This review did not execute those tests. Typed I/O is a
necessary integration surface, not evidence that this real modeled scalar
hierarchy or eight ELF cases are complete. ELF/decode transport and all modeled
architectural computation remain in SSM as the memo requires.

## Disposition and invalidation

Return to the design author/PM for R1–R4. Preserve the frozen C1/C2/C3/N1 texts;
put precise delta authority in the revised extension. Re-review the new primary
hash before asking the user to approve it. Any material proposal change
invalidates this review. The resource memo may remain an advisory artifact;
it cannot authorize resource implementation or close resource capability rows.
