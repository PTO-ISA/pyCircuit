# Independent complete routed dependency graph oracle

This test owner imports no DUT, compiler, generated hardware, or retired
frontend. Expected items never enter hardware logic. `models.py` represents the
entire original graph: all fourteen latency-one queues, all four route branches,
one global eight-entry/four-resource dependency scheduler, round-robin merge,
64-entry reorder, final map, sink, and four original observation points.

The approved semantic choice is the recovered native/specification contract.
The original source and both full-root strong test files remain exact inert
assets under `tests/compiler/lit/Source/Inputs/history-routed/baseline/`, bound
to revisions and SHA-256 hashes. Those tests establish historical deterministic
topology/artifact and build checks; they contain no full-root executable input
sequence. This packet therefore claims **no historical full-root runtime pass**
and invents no original runtime inputs. Its runtime programs are independently
authored new scenarios for that complete original algorithm.

## Selected algorithm and historical differences

Work reads one immutable old-Q image. Host input and every ordinary queue
transfer commit after Work. Queue capacity uses old occupancy; a consumer pop
does not provide future capacity to a producer on the same edge. The original
producer-before-consumer dispatch order supports this choice. Every payload
retains all 106 bits in original declaration order:
`sequence_id:8, opcode:8, route:2, waits_for:8, cycles:16, value:64`.
The frontend adds one, branches add one through four, and output adds 100,
modulo 2^64. Opcode and all other fields remain unchanged.

Scheduler admission selects old-first-free, checks the available head only when
that slot exists, rejects zero cost and duplicate **live** keys before popping,
and stores the complete item. A waiting item is ready when its predecessor is
255 or an old valid Done entry. There is no persistent completed or seen bitmap.
Each resource issues its lowest-key ready waiter. An old executing entry blocks
that resource only while its absolute u64 deadline is greater than the epoch.
Completion and another issue can cofire at that deadline; completion does not
forward readiness to dependent waiters. Retirement selects the earliest old
Done `(deadline, key)` with scheduled-queue capacity. A waiter can use old Done
readiness while its predecessor retires on that same edge. Admission, issue,
completion, retirement and release never reuse state created on that edge.

Merge chooses an old available branch cyclically from its cursor and advances
to the committed winner plus one. Reorder validates stale/duplicate keys only
with an old free slot, admits the complete item, and retires the old entry equal
to `next_key` only with old output space. `next_key` is u64: key 255 is admitted
and retired, then next key becomes 256; byte key zero is stale after that point.

The retired PYC lowering differs in first-slot issue/retirement and projected
key-width reorder advancement. Its exact source is retained inert in
`retired_backend/QueueGraphPyc.cpp.txt`, with revision/path/hash manifest. Its
u16 countdown completes at **remaining == 1** and does not block a resource on
that completing edge (lines 4892 onward). With native ordering, finite u16 costs
and successive unit epochs, that countdown has the same completion timing as
absolute deadlines. The tests explicitly verify this; they do not invent a
countdown precision or off-by-one defect. `retired_pyc_scheduler_policy` applies
the actual at-one countdown and first-slot choices to this same full graph;
its reset-reachable witness is the wrong issue order. It is a bounded policy
comparison, not an emulator or execution of the whole retired backend.
`countdown_at_zero` is a separate deliberately incorrect test model. Absolute
time authority and u64 overflow are separately labeled reference checks.

All valid proposals, host offers, sink effects, counters and observations discard
together on failure. Core epoch does not advance. Failed status stays latched
until an explicit host Reset. The two scheduler fault histories preserve their
first failure and latched attempt, then label a distinct host-reset retry; they
never pretend that a physical reset alone resumes a failed runner.

## Complete runtime programs

| Reset-reachable full-graph case | Attempts | Contract |
| --- | ---: | --- |
| `four_routes_live_dependencies` | 81 | Sixteen complete items, four routes, live Done chain, modulo arithmetic |
| `lowest_key_issue` | 46 | Higher-key earlier slot loses to lower key on same resource |
| `full_topology_backpressure` | 727 | 180 items, full global tables and boundary queues, all four branches, input valid held while blocked, complete drain |
| `late_retired_predecessor` | 47 | Late waiter remains stalled after predecessor retires; no global completion history |
| `scheduler_deferred_zero_cost` | 57 | Full eight-entry scheduler defers invalid cost, then first free capacity fails atomically; explicit reset retry |
| `scheduler_deferred_duplicate` | 57 | Full scheduler defers duplicate live key; explicit first failure/reset retry |
| `live_key_reuse_reorder_stale` | 27 | Dependency admits a retired key again, reorder rejects its stale key |
| `reorder_live_duplicate` | 20 | Duplicate resident reorder key rejects concurrent valid host offer |
| `full64_deferred_stale` | 103 | Full reorder defers validation, output progress frees capacity, then stale head fails |
| `full64_missing_zero_duplicate_stall` | 90 | 64 resident keys waiting for zero, duplicate head remains blocked without premature validation |
| `full_byte_domain_no_reorder_wrap` | 537 | All 256 byte keys retire in order; next key 256 rejects new byte key zero |

These are 1,792 complete attempts. Each case has its own finite 2,048-attempt
bound. Positive scenarios and fresh fault prefixes can drive genuine systems;
the ordinary typed module harness covers complete histories including sticky
failure and explicit host Reset. Those scopes must remain separate in backend
receipts. Four reference-only programs are marked `executable: false`: seeded
u64 deadline crossing, retirement comparator, u64 overflow and host clock-control
discard. They authorize no hidden writes to compiled storage or new clock/error
ports. Clock injection needs an actual existing runner operation before any
runtime acceptance claim. These references are not reset-reachable hardware
traces, and theoretical full-cost/time-domain coverage is not claimed.

The original route is u2; all known well-typed values select one of four outputs.
An invalid route four cannot be manufactured by widening this payload. No such
runtime negative case is claimed.

## DTO, packing and actual observation checks

The canonical full snapshot is:

```text
epoch:u64, cursor:u2, next_key:u64
queues[name]: ordered live WorkItem records for all fourteen queues
scheduler[8]: {valid:u1, state:u2 (Waiting=0, Executing=1, Done=2),
               deadline:u64, item:WorkItem}
reorder[64]: {valid:u1, key:u64, item:WorkItem}
pops[name], pushes[name], received:[WorkItem]
observations[scheduled|route_1_done|route_3_done|completed]:[WorkItem]
observer_last[name]: null | {pops, item:WorkItem}
```

DTO names are test representation; native/RTL inspectors read the actual storage
and ports. Source storage may call `item` `data`, or `valid` `occupied`. No
representation conversion changes algorithm or injects expected values.
Admission writes old-first-free; retirement clears validity only. Invalid table
status, deadlines, keys and full payloads remain observable. Only queue cells
beyond committed occupancy normalize to zero. Inspectors rotate actual queue
storage by the real read pointer to enumerate live logical cells, rather than
compare stale implementation-specific queue cells.

The **21,934-bit** snapshot concatenates, most significant first:

1. epoch(64), cursor(2), next key(64).
2. All fourteen queues in `QUEUE_NAMES` order, each count(`depth.bit_length()`)
   then exactly its declared number of 106-bit item cells. The topology depths
   are `16,4,16,8,8,8,8,1,1,1,1,8,8,1` (89 cells and 42 count bits).
3. All eight scheduler entries, each validity(1), status(2), deadline(64), item(106).
4. All 64 reorder entries, each validity(1), key(64), item(106).

Literal packed values use **5,484 hexadecimal digits**, including leading zero
padding. Hex avoids Python's default large decimal conversion limit.
`vectors.json` includes action frames `{valid,take,data}` plus explicit
`host_reset`/reference-only `clock_error` metadata, public old-Q queue views,
before/after packed state, grants, failure status and semantic failure reasons.
`present` actions hold valid and a complete pending input while old capacity is
full; `offer` actions designate an input accepted with old space. Failure reasons
are semantic model labels; an emitted generic failure does not establish exact
historical named-code or RTL message identity.

To avoid quadratic repetition of observation prefixes, each `after_flow` carries
all fourteen actual pop/push counters, received count and exact newly received
records, all four observation counts and exact new observations, and last
observed `(actual pop count, full head)` records. This checks every observed item
including opcode, repeated heads and held-head deduplication. Host Reset clears
the external ledgers explicitly. Inspectors must derive counters from actual
accepted hardware transfers and committed status; copying golden counters into
observations is forbidden.

`check.py` requires exact attempt count/order, committed and failed status, and
either a full actual after snapshot or packed actual after state plus complete
actual `after_flow`. It checks optional before snapshots and public observations
when supplied. The hardware harness should supply those public and before views
as well. Extra attempts, shortened histories, final-output-only reports, missing
ledgers, lost intermediate observations and changed retained fields are rejected.
The implementation owner can call `check_case(expected_case, observed_rows)`
with independently generated full expectations or directly with the literal
case. A packed-to-full mode switch needs a full actual before snapshot; actual
history must never be reconstructed from expected results.

```sh
python3 tests/compiler/oracles/history_routed/check.py \
  --case full_topology_backpressure --observed /tmp/observed.json
python3 -m unittest discover \
  -s tests/compiler/oracles/history_routed -p test_models.py -v
```

## Evidence scope

`rivals.json` provides reset-reachable witnesses for 27 independent hostile
variants, covering the retired scheduler policies, countdown off-by-one,
byte-key wrap, persistent completion/seen history, forward admission/completion,
old-free reuse, early full-capacity validation, wrong routing/merge selection,
cursor updates without transfer, future pop capacity, opcode loss, arithmetic
changes, repeated held-head observation, missing fault checks and partial commit.
Most witnesses differ in complete physical state; repeated-observation variants
also expose actual trace differences. They are test-model counterexamples,
not product mutations executed by the compiler.

Selftests bind all literal rows and original assets to independent models and
check handwritten thirteen-edge single-item timing, full global capacities,
complete payloads, fault boundaries and checker refusal of adversarial reports.
Compiler stages, native/RTL runs, workers, genuine system simulation, product
mutation executions, candidate acceptance and shared gate registration belong
to separate owners. Nothing in this directory claims those runs passed.
