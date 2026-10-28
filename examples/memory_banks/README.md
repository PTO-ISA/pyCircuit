# Four independent memory banks

`MemoryBanks` calls the same `Bank` Module four times. Requests enter one shared
Queue and route according to the oldest request's bank field. Responses return
through a fixed-priority merge, with bank0 ahead of bank1, bank2, and bank3.
Different banks can execute concurrently; a busy selected bank blocks the root
head rather than allowing a later request to bypass it.

## Design

Each Bank owns a two-entry request Queue, a 16×u16 synchronous memory, controller
state, and a two-entry response Queue. The root owns an eight-entry request Queue
and a two-entry merged-response Queue. All ten Queues use latency1 and
`local_occupancy` readiness.

Each Bank uses one cohesive `step` Rule for its natural controller state machine:
accept an idle request, capture the read-first memory value, advance the remaining
latency, or publish the completed response. These phases are mutually exclusive
from committed state and preserve the release-edge bubble without multiple
ordered Rules for one FSM.

`route_request` pops the root request and calls exactly one child `enqueue` Method
in the same transaction. `merge_response` calls the first available child
`dequeue` Method and pushes the merged response atomically. Queue availability is
compiler owned; the explicit child-valid conditions define only the authored
priority policy. The service and routing Rules activate from their declarations;
callable enqueue/dequeue operations do not activate autonomously.

A write returns the previous memory value. Each controller saves the old RAM
output before backpressure can outlive the primitive's read lifetime. Responses
appear no earlier than two rising edges after acceptance, and a Bank cannot
accept another request on its release edge.

## Short System scenario

`bench.py` contains 26 direct cycles. It writes41 to bank0 and91 to bank1, then
reads both values and also reads untouched bank2. A second group writes `0x3333`
to bank3 and reads all four banks at the same offset. The output is paused for
two cycles while the bank3 write response is visible, then released.

Literal expectations check input capacity and every response's bank, operation,
old/new data, and tag. The observed response order demonstrates head routing,
bank0-first merge priority, per-bank order, and recovery after output
backpressure. Expected data is independent test data, never derived from DUT
outputs.

One sequential System Rule performs the checks, logs the visible output state,
and advances phase. The 26-cycle System passes generated C++ workers1/2 and
Verilator together with the unchanged 883-sample Module oracle.

The earlier 440-cycle Table bench is preserved with its original stimulus and oracle at
`tests/fixtures/historical_examples/system_benches/memory_banks/bench.py`. It
remains a historical regression fixture and is not counted as a passing short
example. Passing the short System does not claim that the retired 440-cycle
transaction plan now compiles or passes.

## Independent Module verification

The unchanged handwritten Module oracle covers 883 Work samples and 439 rising
edges with native workers1/2 and Verilator. Its independent model owns four RAM
arrays, ten Queue models, deadlines, tag/order tracking, capacity accounting,
parallel service, head-of-line blocking, fixed-priority merging, resets, and
Discard/retry probes. It accepts136 inputs, retires102, and cancels34 by reset.

That full physical oracle remains the owner for saturation, held clocks, reset,
fault, and conservation behavior. The short System is deliberately a readable
functional path instead of a second copy of the same long matrix.

## Build and run

```sh
cmake -S examples/memory_banks -B /absolute/build/memory_banks -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/memory_banks --parallel 4
ctest --test-dir /absolute/build/memory_banks --output-on-failure --no-tests=error
```

The Module and short System tests pass through the public
compile/link/dual-emit route without changing compiler limits.
