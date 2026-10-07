# Four independent memory banks

One reusable `Bank` module is called four times. Requests enter a shared queue,
route to the bank named in its oldest packet, and return through a fixed-priority
merge. A busy selected bank blocks the root head; later requests do not bypass it.
Different banks can serve transactions concurrently.

The source uses existing module/rule/queue/sync_mem constructs. Bank selection,
capacity, latency and merge priority are source policy. Its ordinary consumer
executable uses the existing public compiler and Runtime; it adds no framework
specialization, executable, IR interface or alternative simulation path.

## Interface

`BankRequest` contains `bank:u2`, `offset:u4`, `write:u1`, `data:u16`, `tag:u8`.
Every response retains all request fields, replacing only `data` with the old
memory word. A write updates the selected bank at acceptance and still returns
the previous value; a read leaves memory unchanged.

`MemoryBanks(valid, request, take)` returns a named `BanksResult` with input ready,
output valid and the full response. Invalid response payload is unspecified.
Pure four-bit observations expose route, acceptance, response enqueue, available
bank responses and selected merge; bit `i` denotes bank `i`. They add no state.

## Storage and timing

The root owns an eight-packet request queue and a two-packet merge queue. Each of
four separate Bank instances owns a two-packet request queue, one 16×u16 RAM,
a 35-bit controller state and a two-packet response queue. There are ten queues
with 26 total slots, plus at most four controller-held transactions. All queues
have latency1 and local-occupancy ready without implicit full replacement.

Each bank enqueues its response no earlier than two rising edges after acceptance.
It saves old RAM data on the first following edge so backpressure cannot outlive
the primitive's read-output lifetime. A bank cannot accept on its release edge.
The merge selects bank0 before bank1, bank2 and bank3; only the selected response
is popped when the merge queue has room. Per-bank order is preserved, while
responses from different banks can reorder. Fixed priority is not a fairness
promise under an infinite high-priority stream.

## Original contract and verification

The current native and RTL tests cover:
883 Work samples and 439 rising edges agree on native workers1/2 and Verilator.
The independent oracle uses four RAM arrays, ten deques and absolute deadlines.
It checks all 22 flags, all 31 valid packet bits, unique tags, per-bank order, every
queue capacity, parallel service, head-of-line blocking and fixed-priority merge.
Of 136 accepted inputs, 102 retire and 34 are canceled by reset.

Three native generated-root probes prepare/discard/retry merge edges with
non-idempotent writes; subsequent old-data responses detect leaked writes.
Selected unknown output-pop guards also reject and retry. These direct
Work/Discard/Xfer probes do not establish automatic SimSystem failure handling,
arbitrary fault sites, native Z-control or RTL fault rollback.

## Build and run

```sh
cmake -S examples/memory_banks -B /absolute/build/memory_banks -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/memory_banks --parallel 4
ctest --test-dir /absolute/build/memory_banks --output-on-failure --no-tests=error
```

The existing shared verifier runs native workers1/2 and RTL from one final IR,
comparing settled precommit Work observations. Finite SystemRunner configuration
and testbench time bounds prevent an unbounded run. Local verification is not a
claim of full-nightly or cross-platform acceptance.
