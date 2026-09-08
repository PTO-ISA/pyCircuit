# DavinciOO shared SPE contracts

`spe.py` contains the nominal payloads shared by the first in-tree SPE design
modules. The source was ported from `hengliao1972/DavinciOO` commit
`b81ecfc2b634d41886b01fd8724905eeb5bb6551`; the in-tree design program owns
subsequent changes.

## Canonical issue identity

`IssueIdentity` is the one common identity value used by `IssueEntry` and
`IssueAttemptKey`:

```text
IssueIdentity
  epoch: EpochKey
  inst: InstKey
  block: BlockKey
  rob: RobKey
  dispatch: DispatchReservation
  isq_index: u4

IssueEntry.identity: IssueIdentity
IssueAttemptKey.identity: IssueIdentity
IssueAttemptKey.attempt_generation: u16
```

Code compares two attempts with `left == right`. Code that relates an attempt
to its resident entry compares `attempt.identity == entry.identity`. Field-name
or packed-width heuristics are forbidden.

The external baseline also stored `IssueEntry.execution_class` beside
`IssueEntry.dispatch.execution_class`. Decision 0224 removes that parallel
field: the one value now lives in `IssueIdentity.dispatch`, so a mismatch is
unrepresentable rather than checked repeatedly at every consumer.

## Operand source contract

`IssueEntry.src0` and `IssueEntry.src1` directly use
`OperandSourceDescriptor`. `valid_operand_source` is the single named invariant
for these rules:

- constant zero has architectural index zero and no live physical source;
- a live physical source has a nonzero architectural index below 24;
- speculative sources carry a valid producer and nonzero load-stage mask;
- non-speculative sources carry no live producer and a zero stage mask;
- a speculative producer destination matches the operand physical identity;
- the producer epoch, instruction, block and ROB records carry one FlowKey.

I1 and I2 call this invariant explicitly at their transaction boundaries. The
compiler does not assume that construction or `with_fields` preserves it.

## Ownership

I1 owns its retained read-attempt slot. I2 owns operand/dependency join state,
execute transfer, release and generated-cancel state. WBA owns retained terminal
results and cancellation tombstones. These values are NDF L2 microarchitecture
inside hardware H1 SPE / H2 IEX; they do not add architectural state.

## ROB handoff payload

ROB owns per-flow entries and their completion/ordered-handoff lifecycle in H2
OOO. `RobCompletion` carries the writeback result into ROB; `RobEvent` carries
allocation and microcommit messages. Both preserve `result: u64`,
`result_valid: bool`, `status: TerminalStatus`, `fault_code: u32`,
`fault_arg0: u64`, `fault_bi: bool` and `fault_valid: bool`. The earlier imported
eight-bit RobEvent fault code is removed rather than silently truncating W2's
exception payload. `RobHandoff` is CMT's durable release authority. See the
[ROB module contract](../spe/ooo/rob.md) for identity, cancellation and finite-tag
requirements; these messages do not implement the CMT/W2 owners themselves.
