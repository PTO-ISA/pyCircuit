# Issue #63 OPT-02 nested rule module-state capture

A direct `@ac.rule` inside an `@ac.module` may now declare typed module-state
owners with direct-body Python `nonlocal`. Before ordinary rule parsing, the
frontend assigns a deterministic module-qualified rule identity and rewrites
each capture and call to the existing explicit state-parameter form. All owner,
field/index footprint, committed-read, proposal, conflict, arbitration and
lowering semantics therefore remain on the already verified path; no ACIR or
runtime operation was added.

The admitted surface is intentionally narrow. Captures must be direct typed
`AnnAssign` module states declared before the nested rule and are ordered by
state declaration. Missing `nonlocal`, untyped/unknown/late state, parameter
shadowing, nested-scope declarations, nested-rule calls/recursion, unused rules,
identity alias/escape and collisions with flattened definitions fail closed.
The same module specialization can still be instantiated twice with distinct
instance IDs and independently owned state.

DavinciOO I2 now nests all seven rules. Rule definitions and call sites shrink
from 52 state-plus-payload arguments to four real payload arguments, and source
shrinks from 468 to 342 lines. The exact base explicit form and captured form
both retain seven rules, thirteen states, four sources, seven sinks, 45 state
reads, 30 scalar assignments, two element assignments, 32 frozen state writes,
68 reservations, seven QueueGraph blocks and 1,561 expressions. Removing only
the internal nested-rule prefix and specialization fingerprints produces
identical QueueGraph JSON and byte-identical generated C++; both generated files
are 193,135 bytes with five Table scans and five priority encoders.

The existing I2 executable matrix covers inactive acceptance, ordinary and
speculative operands, load hit/miss/replay, sink retry/accept, release,
external/generated cancellation, tombstone reclaim and selected-output
backpressure. Stateful Table PYC/RTL remains outside the admitted backend
surface.
