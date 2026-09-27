# C3-DUT-IO revision B independent design review

Date: 2026-09-28. Verdict: **approval-ready**.

Reviewer: `dut_io_design_review`, independent design validation. Actual dispatch:
`agent_type=default`, `model=gpt-6-astra`, `reasoning_effort=xhigh`,
`fork_turns=none`, confirmed by the PM. This instance is independent of architect
`dut_resource_design` and the PM transcription. No recursive delegation, proposal
edits or implementation changes were performed. The only authored artifact is
this review under the assigned revision-B evidence directory.

This verdict permits the PM to request the user's precise approval of revision
B. It is not user approval, implementation authorization, gate acceptance,
backend-parity evidence or evidence that the consumer ELF goal is complete.

## Exact inputs

Primary checkout: `/Users/zhoubot/linx-isa/tools/pyCircuit`.
Observed HEAD: `8f55c5feab188bc68a3c27abed92812e22f41ea3`.
The primary proposal is 34,427 bytes and 675 lines at review time.

| Input | SHA-256 |
| --- | --- |
| `docs/rfcs/migration/c3-dut-io.md`, revision B | `d8e553b36c4f533892285bce8c200eb8c0d65a037e70dc9be690391d2f421876` |
| `docs/rfcs/migration/c2-resources-transactions.md`, unchanged revision A memo | `7fb279f793865294e97d7be7c9412a5f8a8e1a18dc75a27db8555aa3a0686cd8` |
| Frozen C1-C | `5768e1571e56eb1a5963ff5d40dff1087de38ee997e9c52b5ef91f520da90dfc` |
| Frozen C2-C | `387cf52b129f864b87a1b2a388213a3fe36d81c330d94ced0e6696522b58a322` |
| Frozen C3-C | `0c476ced27519cf93427a89f77b9348d388e96db57fb183790144d24118b1170` |
| Frozen C2-N1-C | `ac7d56a403e21ac03186f17c2f75f9c8eb53a00dcb74677c5286219dc5f1e6c3` |

All hashes were verified with `shasum -a 256`. The four approved texts and the
resource memo match the revision-A review inputs. The complete revision-B draft
was read; affected C2 check/binding/final-IR and C3 lifecycle/SDK sections were
cross-checked again. The named existing packaging scripts and API smoke files
were located. Governance, the design-review skill and review independence rules
remain the authority used in the prior review.

Donor and consumer findings retain their exact pinned evidence from the
[revision-A review](../revision-a/independent-design-review.md): GFSIM
`b852ed83fa0288d0be7406bba0ed47be4b2c0f63` and SSM
`fd0fcf9f21cc5e030af732b4952deed794554dbd`. Neither donor nor SSM implementation
was changed or executed during this re-review.

## Finding closure

| Prior finding | Revision-B resolution | Disposition |
| --- | --- | --- |
| R1: ingress authority and failure contract | Lines 118–122 and 169–252 make `ac.dut` an effectful intrinsic, the sole root invocation and owner of ingress admission. They define complete leaf resolution, safe validation before source evaluation, whole-tree inhibition, exact reset/run/reject alternatives, final and RTL-private checks, diagnostics and continuation. | Closed for design readiness. |
| R2: C header/query linkage | Lines 290–342 give the standalone include, C/C++ guards, query typedef/declaration, size/offset assertions, matching definition linkage, export macro use and platform calling convention. Helpers explicitly remain exported, out-of-line C++ definitions. IO-ABI requires actual compilation, symbol lookup and Windows linking. | Closed for design readiness. |
| R3: unexpected I/O failure lifecycle | Lines 407–449 define one serialized entry guard, helper validation before narrowing, shared diagnostics and buffer invalidation, explicit transition to Failed, preserved data/snapshot/time/destination, first-failure retention, subsequent rejection and Reset recovery. Fallible work precedes a nonthrowing publication suffix. | Closed for design readiness. |
| R4: callers, inventory and acceptance lanes | Lines 504–658 provide a full Runtime-only caller, precise affected surfaces and export inventory, explicit inherited deletion scope, a two-level capability handshake, named execution lanes and current-candidate compile/link/emit, host and four-state RTL commands. | Closed for design readiness. |

No new blocking design finding was identified in the revised packet.

## Cross-contract assessment

The new intrinsic is an explicitly declared exception for external ingress,
not an orphan `ac.expect`. C2's existing rule registration, CheckID, evaluation
paths and source-check semantics remain intact. The intrinsic's closed port and
binding inventory provides the authority for the external sample and boundary
storage; it is not an emitter-selected scheduler. Its final verifier must reject
missing, duplicated, redirected or bypassed authority, and RTL-private verification
must check the materialized evaluation and commit controls. Those are concrete
future implementation and mutation-test obligations.

Reset/run/reject predicates are complete and two-valued. Known asserted reset
wins over invalid data and restores reset images. X/Z reset or invalid inputs
on a non-reset edge reject that edge without evaluation or commit; corrected
pins may run on the next edge without an added failure latch. Source/check
failure after admission retains the frozen C2/C3 failure and Reset contract.
Unknown clocks remain outside this explicitly bounded profile. Detecting X/Z
at ingress does not admit four-state source values or extend internal state
semantics. Real Icarus execution, exact diagnostic accounting and completion
oracles are required rather than substituting two-state simulation.

The distinct C++ error cases are now explicit. Malformed host drive is an API
rejection retaining the old input batch and a healthy lifecycle. An unexpected
I/O exception is fail-stop; a corrupt retained batch discovered by step is an
internal invariant failure. Neither case invents rollback for C3's possible
partial internal Xfer publication. Public output snapshots remain the last
complete reset/commit publication, including sampling permitted in Failed.

The C ABI keeps the 64-byte lifecycle table and existing status/diagnostic
fields. The separate table is fixed at 40 bytes with checked offsets and C
linkage. The two source-qualified helpers have C++ linkage and enter the same
guard before any rejection. Their full-width validation, nominal records,
packing and nonthrowing final copy preserve the stated range and destination
contracts. The direct-link client and generic optional-symbol loader have
separate, accurately described responsibilities.

The example client is internally consistent: Request contains one byte of
Word and one byte of bool; bias adds one byte; result is one byte. The expected
3/1 table sizes, reset result 7, and first accepted step result 3 follow the
source example. The client uses the preserved create/configure/reset/step/
destroy lifecycle and only the generated public include.

Exact exports are now a deliberate extension: four symbols for a typed bundle,
two queries for a new portless bundle, and no helper exports for the latter.
Matching-compiler C++ decoration checks avoid hardcoded cross-platform names.
The SDK/platform/release-index/consumer-lock agreement is distinct from selected
model availability and table compatibility; toolchain capability alone cannot
turn a portless model into a typed DUT. The affected export validator is named
for update rather than having its check removed.

Source ownership and hierarchy are preserved: independent producers, the
declaration-only Packet header, executable source groups, unchanged C2 child
current/next forwarding and one selected system boundary. System/default
classification, no implicit relay register, nominal identity and N1 declaration
authority were not weakened. The source-owned transport headers are the
explicit new public surface; they do not expose arbitrary internal model state.

## Resource memo and remaining acceptance obligations

The unchanged resource memo remains coherent bounded advice. Its donor-first
ordinary FIFO equations do not claim implicit transactions or a complete
resource proposal. Resource admission, selected effects, Slot/multilane behavior,
memory and other listed gaps still need their own exact contracts and approvals.
No resource capability row can be closed by this review.

The proposal keeps ELF/decode transport, architectural state, PC/branch behavior,
ALU/BRU execution, retirement and the actual connected scalar hierarchy in SSM.
It does not authorize a host interpreter or treat typed I/O as proof of the
eight ELF/overlap/init-run-drain outcomes.

All execution lanes are still planned. This review ran no compiler build,
generated client, ABI executable, fault injection, simulator or consumer test.
Implementation acceptance must execute IO-SOURCE, IO-IR, IO-ABI, IO-RUNTIME,
IO-RTL4 and IO-INSTALLED with nonempty assertions and no skipped required cases,
plus the applicable frozen C3 obligations. Current-checkout tool provenance,
independent expectations, exact symbols, complete snapshots and real four-state
edge behavior remain material acceptance risks until those tests pass.

## Disposition

The PM may request precise user approval of the primary revision-B SHA-256
above. Do not present this verdict as approval already obtained. The frozen
C1/C2/C3/N1 files need no change for this review; the extension's explicit deltas
are the object of the new approval request. A material change to revision B
invalidates this verdict and requires another independent review.
