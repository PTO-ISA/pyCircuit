# Shared functional and timing execution

This local extension is based on main `24fa89804f26bb7618692f8c80bcd451233c47b3`.
It adds a bounded automatic execution projection to the existing source-unit
compiler. A source model contains computation, state/effects and queue timing;
`link --execution functional|timing` selects an execution strategy. `emit --target
cpp|verilog` separately selects a backend. An open frontend proposal is not part
of this contract.

## One source and its owners

The word pipeline example in this candidate is
`examples/timed_word_pipeline/`. Its
`timed_word_pipeline.py` imports the public
`functional_float.word_arithmetic` provider. `WordWorker.process` owns the
accumulator, completed count and rejection count. The wrapper owns acceptance
and two queues. The worker uses ordinary generated scalar calculations and
state writes; no adapter computes answers or restores state.

A literal `@ac.module(execution={...})` dictionary declares roles. It does not
execute Python or define a second functional body:

| Keys | Meaning |
| --- | --- |
| `worker`, `ingress`, `egress` | Names of the one worker and its two exclusively owned FIFO instances. |
| `process`, `snapshot`, `submit`, `take`, `transfer` | Callable roles. `snapshot` is the wrapper's read-only architectural view; `transfer` is its sole automatic Rule. |
| `enabled`, `valid` | Actual request-enable and response-publication Struct field names. These protocol flags are distinct from compiler-owned availability. |
| `accepted` | The wrapper's finite Bits acceptance counter, incremented once in the enabled admission transaction. |
| `interval`, `wait_state` | II 1 with an empty private-state name, or II 2 with the explicitly owned one-bit issue-gap state. |

Queue depth and latency remain in ordinary `ac.queue` allocations. The compiler
checks actual typed SSA, calls, guards, state ownership and effects against these
roles; role strings are not proof of safe projection.

## Admission and automatic derivation

Capture records the literal declaration. Native source import publishes
`ac.execution_model` in both interface and body metadata. Body/interface
snapshot checks retain that authority. After ordinary source-unit authentication
and helper linking, `ExecutionProjection.cpp` checks the closed component and
creates a derived root. The original source definitions and System callers
remain intact. Generated roots record their original owner and timing profile.
The normal transaction analysis, hardware lowering, final verification and
C++/Verilog emitters then operate on the selected root.

The admitted component has a single domain, one deterministic resource-free
worker, two finite static FIFOs, a by-value request/response and a read-only
snapshot. Requests are flat nominal records of finite Bits. The worker's scalar
state has closed known initial values. Its reachable operations must be total
on known data; unsigned division/remainder require a nonzero-denominator proof.
A numeric invalid result is a defined response/status, not a hidden exception.

Submit must push the unchanged enabled request and increment acceptance exactly
once. Transfer must pop, call that worker with the popped snapshot, and publish
the unchanged response only under its declared valid field. The II 2 controller
must implement the private gap contract. Take returns the queue's unmodified
response. Snapshot and other worker accessors are read-only.

Architectural timers, observable wait state, cancellation, additional workers or
arbiters, cross-domain resources, autonomous worker effects, trapping/non-total
worker operations and unsupported topology are rejected before final
publication. This slice does not silently discard exceptions or infer arbitrary
IR timing equivalence. Functional selection of an enclosing System is rejected:
its stimulus timer is not a DUT wait to erase. Routed/saved-anchor selection is
outside this slice.

## Execution relation

Timing retains the original queues, latency, capacity, II state and automatic
transfer. The generated entry offers `request`/`active`, optional `consume`,
`ack`, `completion`, and ordered `snapshot_N` outputs. The input adapter holds a
pending offer until acknowledgement. A committed ingress push captures its
value; later input changes cannot change an accepted request. Queue-full or
result backpressure blocks the entire transfer, including all worker effects.

Functional derivation removes the proved transport allocations and private
issue gap from its derived root. One logical step invokes the **same generated
worker Method and state owner** for one enabled request, increments acceptance,
and exposes its completion/status. The sink is always ready. There is no loop
advancing empty FIFO clocks and no rewrite of queue latency to zero. Serial
request order preserves the worker's state dependency and exactly-once effects.
Intermediate queue occupancies are not architectural observations of this view.

Both strategies use existing Work, whole-system checks, discard and Xfer.
Active unknown required data or unknown control fails before any new effects.
Inactive or business-disabled unknown payload is not demanded and has no
queue/worker/counter effect. The rejection is a simulation failure, not an
implementation of architectural exception recovery. Such recoverable trapping
workers are outside admission.

For Word, eight offered requests mean one disabled request, seven accepted
requests and seven architectural worker completions: six valid results and one
invalid completion. Invalid increments `rejected`, does not update `total`, and
never publishes a valid payload. Final state is accepted 7, total 65, completed
7, rejected 1. The functional entry exposes invalid completion flags; timing
consumption exposes only the six queued valid responses. Compare valid
publications and final architectural state, not those different event counts.

## Public installed use

See `examples/timed_word_pipeline/README.md` for portable commands and checked-in inputs. `pycircuit compile` still publishes one
unit per source; `link` receives an explicit complete closure and selects the
execution model; either backend consumes that strategy's verified final IR.

`pycircuit run EXAMPLE --execution functional --model QUALIFIED_MODEL --inputs
requests.json --target cpp` builds and drives that generated entry. Timing adds
`--cycles` and `--consumer-start`. Functional omits physical cycle limits and
uses one logical step per input row. Each logical/rising step still uses the
normal high/low lifecycle. The adapter is only typed raw-bit I/O, handshake and
sampling. Its success receipt says `execution_only`; tests separately own
numerical, flag, state and timing expectations. C++ uses the standard generated
`pyc_dut` and installed Runtime; RTL uses the generated managed interface and
Icarus, including precheck/commit/discard controls.

The CMake helper's `PROVIDER_UNITS` takes explicit sibling example
`root|package|source` records and compiles them before dependent units.
`MODEL_SOURCES` declares the model's closure independently from its consuming
System bench. No provider is copied. Every definition in a supplied unit is
still admitted, and analysis budgets are unchanged. Build directories isolate
strategy, model and backend to avoid stale-product substitution.

## Validation and limits

`tests/system/test_shared_execution_models.py` owns independent raw/flag/state
and discrete FIFO-time expectations. It checks two Word profiles and a distinct
16-bit state-dependent XOR/add Digest worker with different role names. Each
runs functional/timing on generated C++ workers 1/2 and Icarus from identical
per-strategy final IR. The oracle checks every cycle: acceptance, maturity, II,
capacity, stalls/recovery, output order, snapshot state and valid publication.
After acceptance the driver changes external data to unknown canaries, proving
that later input does not supply queued values.

The Word profiles are depth 2/latency 1 → depth 2/latency 3 at II 1, and depth
1/latency 1 → depth 1/latency 1 at II 2. Digest uses depth 1/latency 2 → depth
2/latency 1 at II 1. Consumer delays deliberately fill the result queues.
Digest's five offers include one disabled and one invalid; four completions,
three valid payloads 7, 10, 18, and final digest 18 demonstrate a true data/state
dependency.

The generated-entry probe also inspects committed state after active-unknown
failure without resetting/restoring it, and checks inactive/disabled unknown
zero effects. Representative invalid interval, architectural timer, duplicate
worker effect and trapping worker cases fail publication and preserve the
previous final artifact. The existing authored System remains a separate
normal timing-flow regression. Historical authored functional source/driver
are not part of the active DUT closure. The pre-split candidate64730b9 retains
their historical source; current examples use generated entries only.

The local candidate checks on this base recorded 4/4 shared-execution pytest
cases, 165/165 affected package unit tests, 25/25 API gate entries, and 1/1
authored timing-System test. Changed-file pre-commit and strict MkDocs passed.
The seven-design example gate selected 13 CTest entries. Its initial run with
Verilator 5.020 passed 9 and failed 4 (`rob`, `struct_transform`,
`enum_payload_pipeline`, `table_rule`). The repository's setup action and
nightly/release workflows pin **5.048**; 5.020 is below the action's minimum
check of 5.024. Reusing the existing 5.048 kit, with its real ccache/mold and an
explicitly corrected CMake executable cache, passed **13/13, rc0** on the same
implementation commit. All four initial failures disappeared without changing
primitive bytes, suppressing diagnostics or relaxing admission/budgets. The
initial result was a test-tool configuration mismatch, not an established
external primitive blocker. The new profiles' actual RTL execution above still
uses Icarus; this gate does not add a Verilator-equivalence claim for them.
Initial raw evidence remains under ignored
`docs/gates/logs/shared-functional-timing-20261101/`; the pinned-version rerun,
selection, XML, environment and process exit are in
`docs/gates/logs/shared-functional-timing-20261102-verilator/`.

These bounded checks do not establish arbitrary timing collapse, all source
operations, multiworker scheduling, cancellation, multiple domains, physical
asynchronous reset, a broad exception model or cycle equivalence for arbitrary
RTL models. Formal independent review and full nightly remain separate gates.
