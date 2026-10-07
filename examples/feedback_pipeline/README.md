# Feedback pipeline

`FeedbackPipeline(valid, data, take)` returns Result38: ready37, valid36 and
Item36. Three queues have depths2/1/1 and latency1: source, internal feedback
and result. They preserve four payload slots and144 logical payload bits. The
stateless Step module contains only combinational selection and arithmetic and
adds no storage or cycle. Ordinary typed calls and queue connections use the
existing frontend and common IR without a new feedback primitive.

## Timing and priority

Resident feedback has priority over the source head. Continuation advances even
while the result queue is full; only a completed exit waits for output capacity.
A resident feedback exit cannot process a new source head on that same edge.
External offers may still enter free source slots; a full source cannot replace
its head until that head is actually consumed.

With initially empty queues, E0 accepts a token. remaining0 exits at E1 and first
retires at E2. For remaining=n>0, updates occur at E1 through En, exit pushes at
E(n+1), and first retirement is E(n+2). A stalled exit retains its completed value.
All queues explicitly use downstream_pop; the feedback slot consumes/replaces
itself on a continuation. Its input-ready is not fed into its own take decision,
so no combinational ready cycle or extra buffer is introduced.

Held/falling clocks and discarded proposals do not advance the loop. Rising
reset drops every occupied token. Invalid result data is packed zero. The compiler
generates hidden clock/reset pins driven by the host/SystemRunner, and the shared
Runtime manages Work/Xfer and whole-system discard.

## Preserved scope

The old feedback implementation also maintained an iteration count and a1024
guard. For this exact design, initial remaining is0..15 and every accepted
continuation decrements it; the guard is unreachable. This rewrite omits that
unobservable bookkeeping under the reviewed proof.
It does not implement arbitrary Python while loops or claim1024-limit failure
coverage. The original payload capacity and edge timing remain required.

No-update exits preserve all input value/known/Z planes. After an arithmetic
update, unknown value bits produce the accepted computed-X value: masks and
known bits are exact, computed-X latent bits unspecified, and arithmetic emits
no Z. Unknown remaining controls a transfer and fails when effective, with no
partial commit; changing an external offer cannot repair a poisoned queued head.
Reset is required after terminal executor failure. These current failure rules
are not a claim of universal parity with legacy two-state/procedural behavior.

## Build and verification

```sh
cmake -S examples/feedback_pipeline -B /absolute/build/feedback_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/feedback_pipeline --parallel 4
ctest --test-dir /absolute/build/feedback_pipeline --output-on-failure --no-tests=error
```

Independent tests must check all remaining values, wrap, feedback priority,
four-slot saturation, progress despite result blockage, stable blocked exit,
no same-edge exit/source processing, exact edge latency, holds, busy reset,
discard/reprepare and observed token conservation. Native workers1/2, known
Verilator and genuine Icarus execute the generated model. Standalone gates pass
2/2 and the paired targeted aggregate passes4/4. There are7,527 known and7,607
four-state Work rows, including320 known tokens,408 four-state tokens and
directed timing/backpressure histories.

Known history accepts349 tokens, retires341 and reset-drops8; four-state history
is437/429/8. Both drain empty and reach four occupied slots. Every initial
remaining value0..15 has an isolated exact-latency check. Native34 terminal
cases per worker and18 isolated RTL failures cover selected unknown remaining
and protocol controls; queued unknown data waits safely behind resident feedback
until it becomes the selected control. Two owner probes verify whole-system
discard/reprepare and actual retained-token retirement after a discarded Reset.
The finite runner limit is12,000 epochs per successful history.