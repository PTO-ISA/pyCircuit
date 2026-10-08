# Independent historical mailbox and resident issue oracles

This directory owns test models and literal expectations only. It imports no
DUT, compiler, generated code, or retired frontend. Expected data never enters
hardware logic. The implementation owner separately runs the actual FIFO,
slot, table, and two-instance mailbox designs against these expectations.

`generate.py` verifies the six inert historical assets and their SHA-256 hashes
in `tests/compiler/lit/Source/Inputs/history-slot-issue/baseline/manifest.json`.
The algorithms are from revision `8887e6dec7b4cc530a9967c860dc6a224d79a4ab`;
the original executable tests and the flat/nested mailbox fixtures are from
`b3e52fb22061350d61b23089c8bed36913650245`. The original issue test checks
allocation `totalPops`, unchanged full-table residents before wakeup, exactly
one age-0 and one age-99 output, and no remaining valid age-99 entry. All those
checks remain, with complete per-epoch state added independently from the
original algorithms.

## History scopes

The `original` group preserves the six mailbox cycles and thirty issue ticks.
Mailbox rows are paired projections of the separately executed flat and nested
single-mailbox fixture histories. Their input is committed directly before
the first Work phase. Issue starts from the exact four heterogeneous host
`initializeEntry` records. These histories are **reference evidence only**:
they are not claimed as supported current hardware initialization or execution.

The `executable` group starts from reset and uses ordinary physical boundary
operations, with one offered token per port per epoch and no hidden state
injection. The mailbox original body has one offer epoch followed by all six
original cycles. The issue original body has twelve epochs installing all four
records through the real allocation queue/slot, followed by the full original
thirty-tick body. The warmup restores the original table contents, but retained
slot payloads and transfer counters differ from host initialization. Its body
allocation-pop delta is checked separately. This is a distinct reset-reachable
scenario, not an exact match to the original initial host trace.

| Executable case | Epochs | Independent contract |
| --- | ---: | --- |
| `mailbox_original_body` | 7 | Flat/nested original body after ordinary offer |
| `mailbox_backpressure` | 22 | Both instances, full input/slot/output, independent progress, retained payload |
| `issue_original_body` | 42 | Four ordinary allocations plus complete original thirty-tick body |
| `issue_wakeup_flags` | 11 | False wakeup payload flag, both operands, no same-edge selection |
| `issue_allocation_no_forward` | 12 | Wakeup and allocation use old table; no persisted global readiness |
| `issue_full_slots_and_output` | 30 | Full table still captures allocation, depth-two input, blocked output still clears selection |
| `issue_oldest_ties` | 14 | Oldest ready, stable index ties, physical output-pop bubble |
| `issue_independent_fields` | 9 | Both readiness patches, old issue clear and disjoint allocation cofire |
| `issue_full_replacement` | 12 | Full entry replacement, false allocated validity, no stale readiness |

There are 159 executable epochs. The supplemental programs use a finite
256-epoch test bound and bounded waits; that bound is not attributed to the
historical six/thirty-cycle drivers.

## Old-state behavior

Offers commit after Work. `inject` is reserved for original reference rows and
commits before Work. `take` models ordinary physical output consumption;
`sink` models the historical automatic sink after the read source and can stay
asserted on empty outputs. Both use old output occupancy. Mailbox transactional
output preparation rejects a pending host pop. Issue's plain read source has
different behavior for an exceptional prepublished host pop, but its original
automatic sink runs after the read. This packet does not claim support for
that exceptional direct host-pop operation through a closed system.

Slots capture only when old-empty, regardless of table capacity; release
retains their payload and never refills on that edge. Wakeup occupancy enables
patches even when the payload's `valid` field is false. Both operand readiness
fields merge independently. Selection and first-free allocation query old
entries. Issue output copies the entire old selected record, while the separate
valid-clear occurs even with a full output queue. A wakeup does not forward to
new allocation or selection. No global ready bitmap is introduced.

## Literal handoff and observations

`vectors.json` contains source hashes, scope labels, reset state, action rows,
old-state grants, meaningful public controls, and decimal packed snapshots
before Work, at Work, and after commit. Full expected snapshots can be obtained
with `generate.py --output /tmp/history-slot-issue-golden.json`. Queue cells
beyond occupancy normalize to zero. Invalid slot payloads and all invalid
table-entry fields remain observable and are never normalized away.

Packing concatenates fields from most significant to least significant:

- Mailbox, **54 bits**: left then right, each containing input count(1)/head(8),
  slot validity(1)/payload(8), output count(1)/head(8).
- Issue, **250 bits**: wakeup queue count(2)/two Wakeup records(9 each),
  allocation count(2)/two Entry records(27 each), wakeup slot validity(1)/record(9),
  allocation slot validity(1)/record(27), all four Entry records(27 each),
  output count(1)/Entry record(27).
- Entry field order is `valid:1, age:8, src0_tag:8, src0_ready:1,
  src1_tag:8, src1_ready:1`; Wakeup is `tag:8, valid:1`; Event is `value:8`.

`check.py` accepts observed `{"rows": [...]}`. Every row must contain the exact
epoch and either a complete `after` snapshot or `packed_after` plus actual
`after_counters` containing `pops`, `pushes`, and `received` for all queues.
Those counters must come from observed hardware transfers. Expected counters
must never be copied into observations. A final output list, shortened history,
or packed state without transfer observations is rejected. Optional before/Work
observations receive the same checks. If actual `controls` are supplied, every
meaningful expected control is required and checked, including selected-but-
blocked issue data; unused heads/data are omitted. The implementation harness
should supply public controls as well as the mandatory complete state/counters.

```sh
python3 tests/compiler/oracles/history_slot_issue/check.py \
  --group executable --case issue_original_body --observed /tmp/observed.json
```

## Independent validation

```sh
python3 -m unittest discover \
  -s tests/compiler/oracles/history_slot_issue -p test_models.py -v
python3 tests/compiler/oracles/history_slot_issue/rivals.py \
  --output /tmp/history-slot-issue-rivals.json
```

`rivals.json` records the first reset-reachable physical-state counterexample
for all 26 test-model variants: output replacement, release without space,
same-edge refill/forwarding, erased retained payload, coupled mailbox instances,
payload-valid gating, full-table capture gating, readiness overwrite, wrong
oldest/tie selection, blanket serialization, new-state reads, output-gated clear,
forced allocation validity, partial entry replacement, new-entry wakeup
forwarding, and dropped pending allocations. These are deliberately incorrect
independent models, not mutations executed through the compiler or backends.

The independent model tests and literal checks establish this oracle packet.
Compiler-stage artifacts, native execution, RTL execution, real product mutation
runs, and PM integration remain separate acceptance responsibilities. No such
result follows from these selftests.
