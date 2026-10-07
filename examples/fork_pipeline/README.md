# Decoupled fork pipeline

The original `int` is unsigned 64-bit transport. One depth-one input queue
feeds two depth-two output queues. All have latency one and explicit
`downstream_pop` readiness, preserving old-head full replacement without empty
bypass. Two ordinary one-bit variables remember which branches have received
the held input head. A branch can receive while its peer stalls; a branch that
already received that head must not receive it again. The input retires only
after both branches have received it. Completion clears both flags, including
when a new input replaces the completed head on that edge.

There are five physical queue slots and two metadata bits. The held input is
not another payload copy or an extra capacity reservation. At most three source
identities await delivery to either consumer; physical slots can contain two
copies of one identity. Reset accounting tracks each consumer's outstanding
obligations separately, including partially delivered heads.

`ForkPipeline(valid, data, left_take, right_take)` returns `ForkResult` with
ready at bit 130, left_valid at 129, left_data at 128:65, right_valid at 64,
and right_data at 63:0. Python exposes ordinary queues, variables and a rule;
the existing MLIR flow infers the two storage owners and hidden clock/reset.
The same verified IR feeds both backends. No fork primitive is added.

Outputs are the Work snapshot before Xfer. Rising reset empties all queues and
clears delivery metadata. Held and falling clock levels do not transfer. Empty
initialized data is zero, and copied payloads preserve all value/known/Z planes.
System checking precedes commit; failed systems require host Reset before reuse.

## Build and verification

```sh
cmake -S examples/fork_pipeline -B /absolute/build/fork_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/fork_pipeline --parallel 4
ctest --test-dir /absolute/build/fork_pipeline --output-on-failure --no-tests=error
```

The installed example helper independently compiles the source, links its
explicit closure, emits both backends, and runs the generated SystemRunner with
workers 1 and 2 against an independent token-obligation oracle. RTL checks use
Verilator and genuine Icarus four-state execution. Tests must distinguish partial
delivery from atomic fanout, prevent duplicates under asymmetric stalls, and
exercise replacement, wrap, reset, hold, discard and failure recovery.

The standalone generated-DUT gates pass: workers 1/2, Verilator and Icarus
agree on 395 known Work samples; native and genuine Icarus agree on another
659 four-state samples. Native raw transport checks all three planes, including
latent values. The four-state vectors include 256 single-bit uncertainty/latent
cases and eight dense patterns.

Each consumer's known history accepts 144, retires 137 and reset-drops seven
obligations; the four-state history accepts 276, retires 269 and drops seven.
Mirrored partial-reset cases drop 1/3 and 3/1 obligations, and full reset drops
3/3 at five occupied physical slots. The oracle requires partial deliveries,
duplicate suppression and replacement, with a bounded final drain. Direct-owner
discard/retry preserves metadata; the terminal unknown-control test checks
failure and sample rejection, followed by mandatory host Reset.

The terminal effective-transfer failure occurs before metadata preparation and therefore
does not directly observe metadata after a late system failure. The rollback
proof combines this lifecycle test, the direct-owner test that prepares and
discards all three queues plus two flag owners, the emitted common discard
path, and unchanged accepted Q6 late-parent/sibling failure tests. It does not
resume a failed system to inspect state or claim a new late-sibling execution.