# Resources and transactions: donor-based design gaps

Revision: A. Status: investigation and bounded recommendation; not a complete
resource-interface proposal or implementation approval. Architect:
dut_resource_design, Astra xhigh; PM transcription.

## Finding and reuse boundary

The smallest sound next slice uses donor FIFO behavior authored as ordinary
Pythonic state and equations, together with precise typed external I/O. Do not
invent Queue/Slot/Table APIs just to connect SSM. Ordinary DFFE updates do not
complete resource-transaction migration.

GFSIM b852ed83fa0288d0be7406bba0ed47be4b2c0f63's approved
frontend/ObjectFrontendProposal.md:646–735 describes fixed cells/head/tail/count
and ordinary conditional assignments. The compiler need not recognize the
Mailbox class name; replacement with SimQueue requires proven equivalence.

| Evidence | Reusable fact | Limit |
| --- | --- | --- |
| Donor ObjectFrontendProposal.md:704–731 | Read old head; empty simultaneous put/take has no bypass; full take/put can replace; reply appears after Xfer | Explicit authored equations, not implicit inference |
| Donor docs/IR/proposals/final-current-next-acir.md:82–131 | Capacity/current/next/latency/flush belong in common IR | Future RTL contract, not measured backend parity |
| Donor include/SimQueue.h:332–341 | Readable current head and resident-pop+push<=capacity; counts 0/1 | Self-transfer credit, not general multi-owner preparation |
| Donor include/SimQueue.h:126–223 | Xfer resolves requests and publishes | Capacity throws and survivor-vector allocations at 143–159 prevent direct reuse as no-fail commit |
| Donor compiler/acir/lib/CodeGenLifecyclePhases.cpp:198–241 | Whole-tree checks precede DriveNext | Phase names do not prove all-resource atomicity |
| pyCircuit Decisions 0236, 0262 | Selected effects all-or-none; Reset clears reservations; Slot release/refill distinct | Obligations survive source API retirement |
| C2-C final contract; C3-C step/failure contract | Ordinary state/check/failure semantics already precise | Resource admission remains outside approval |

Donor queue latency 0 and 1 both publish next edge; larger values delay further
(docs/API/SimQueue.md:25–31; include/SimQueue.h:521–524). Do not reinterpret zero
as bypass. Direct FlushIf call order and generated simultaneous equations are
different abstraction levels; neither implies unrestricted atomic transactions.

## Bounded ordinary-state slice

Use generic framework fixtures, fixed state lists and explicit request/reply
records. Donor constructor-dependent range(depth) annotations are not implicitly
approved C1 syntax. Fix capacity in source, or use approved nondependent bounded
index/count aliases and static collection lengths.

```text
take = request.take and count > 0
put  = request.put and (count < capacity or take)
read_value = cells[head] if take else an explicit valid default
if put:  cells[tail] := request.value; tail := (tail + 1) % capacity
if take: head := (head + 1) % capacity
count := count + int(put) - int(take)
reply := Reply(read_value, take, put)
```

All reads are current; writes propose next; reply is committed output. Source
specifies reset, functional enables and coordination. Field names ready/valid/count
or class names never introduce implicit resource semantics.

Independent deque/sequence oracles cover capacity 1 and a non-power-of-two,
empty/full, same-slot replacement, long backpressure, wrap, reset/rerun and both
backends. Source errors/driver conflicts before commit leave all owners unchanged.
Permuting rule/child/declaration traversal cannot change observations.

This proves ordinary C1 algorithms/C2 current-next/C3 lifecycle. It does not prove
implicit admission, Queue topology, Slot release, activation, ordered multilane
prefixes, Table selection or transactions across independent resource owners.

## Required normal commit boundary

A future resource contract must specify the following without relying on C++
call order:

1. Read one committed snapshot; choose the functional branch.
2. Determine all selected input consumptions, output productions, state effects
   and releases before checking availability of the whole selected set.
3. Validate source errors, ownership, ranges and conflicts before publication.
4. Prepare the entire selected set or cancel all temporary preparation.
5. Publish acceptance only after preparation succeeds.
6. Commit accepted physical data/enable updates without allocation, callbacks,
   policy decisions or operations that can normally fail.

A blocked selected branch cannot fall through to another branch. It leaves all
participating owners unchanged. Other legal transactions may proceed under their
approved contract; scheduler order cannot choose a winner.

Public Python cannot expose reserve/prepare/commit/rollback, atomic/check, or
low-level pop/push control merely to implement this protocol (Decision 0236).
Source-authored FIFO equations remain authored algorithms; implicit resource
behavior needs separate exact source design.

No-fail normal resource commit does not silently rewrite C3's catastrophic
internal-Xfer exception clause. Unexpected failures still enter Failed and
require Reset; they cannot be legal modeled outcomes of normal admission.

## Required open design packets

| Packet | Exact unresolved decisions |
| --- | --- |
| Source expression | Ordinary objects versus retained Queue/Slot/Table capabilities; syntax, aliases, ownership, inferred selected effects |
| Admission/conflicts | Same-transaction versus independent pop credit, conditional outputs, overlap, explicit arbitration, activation/fairness; select branch before availability |
| Physical common IR | Resource declarations/equations, complete selected effects, admission proof, commit-target binding; reject omitted/redirected effects without backend reconstruction |
| Slot/multilane | Retained invalid payload, no same-epoch refill after release, unique release owner, ordered valid prefix and whole-prefix commit; Slot is not automatically depth-one FIFO |
| Runtime | Fixed preparation scratch, nonthrowing transfer/destruction, cancellation/reset, owner boundaries, fault injection; adapt donor SimQueue or generate equations |
| Table/list/memory | Rank/masks/selection, whole-entry versus field conflict, banking/ports/timing; no implicit SRAM/resource inference from list length |

Each packet needs independent source-derived oracles, omitted-effect/forged-proof
mutations, same-final-IR dual-backend execution, independent review and user
approval. Full resource/atomicity rows in the capability matrix remain open.

## SSM scope and stop condition

Observed SSM fd0fcf9f21cc5e030af732b4952deed794554dbd:
core/core.py:99–105 calls empty spe/spe.py:1–8; spe/ooo/ooo.py:18–34 and
spe/iex/iex.py:25–37 are separate assemblies. Framework work cannot substitute
for connecting the real canonical scalar hierarchy in SSM.

scripts/pyc_elf_frontend.py:53–98 owns immutable bytes/decode lookup; its baseline
status separates this from modeled PC, operands, execution and retirement.
scripts/generate_pyc_block_elf.py:55–190 defines eight programs and independent
expectations. tests/pyc_model/test_gfsim_pyc_cli_dev.py:103–148 checks exact
commit/block PCs, registers, 1–4 in-flight with one peak>=2, and init+run+drain;
174–210 covers loop-budget/bad-target failures.

Keep these sources, algorithms, schemas and evidence in SSM. A host reporting
dictionary must not provide operands, branches, retirement or architectural state.
Precomputed transactions, fake terminals, old binaries, static-library builds
and skipped ELF cases do not prove execution.

This memo ends at donor-backed guidance and explicit gaps. It claims no completed
resource design/implementation, backend parity or live ELF result. The separate
[typed I/O draft](c3-dut-io.md) also requires independent review and exact approval.
