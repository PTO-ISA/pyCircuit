# Host result Queue boundary gate summary

Decision 0194 adds a compiler-selected host result mode without adding any
Python Output, Queue, sink, ready, or dequeue authoring object.

## Evidence

- `--host-results` lowers typed system returns to Top module Queue results and
  omits compiler-generated sink blocks.
- Structured root verification accepts exact Queue results while continuing to
  reject borrowed root inputs.
- QueueGraph records four root interface outputs for the reusable ROB.
- Generated roots expose committed result Queue access plus
  `try_take_result_N(system)` external-Xfer adapters.
- The native one-result MLIR fixture passes freeze, plan extraction, C++
  generation, and C++ syntax compilation.
- Frontend host-result test proves a two-result Top has `ac.return` and no sink.
- Runtime backpressure test holds the first allocation result full. The second
  allocation remains committed at the input boundary until the host dequeue
  commits, then produces value `300` at slot one after value `100` at slot zero.

## Remaining scope

Host results currently require structured module systems. The full ROB
scan/activation gate still needs per-tick Queue/state/timeline comparison rather
than final-value comparison only. Semantic-no-change Table proposals still wake
subscribers conservatively.
