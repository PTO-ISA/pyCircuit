# Issue #46 closure review

Decision 0223 extends the one-output contract in Decision 0210 to fixed-arity,
heterogeneous selected results. Python uses a typed tuple and initializes each
optional result local to `None`; `None` is eliminated into an SSA presence and
never becomes a payload, Queue wrapper, dummy value or public handshake marker.

The review found and closed the following correctness gaps before sign-off:

- output-capacity checks originally participated too early in candidate
  construction; Work now computes one functional selected set from the current
  tick-start snapshot, while Arbitrate checks the selected resources;
- candidates are cleared at an unsuccessful epoch boundary rather than carrying
  stale state-derived writes into a later committed snapshot;
- read-only scalar state, source-order scalar state overrides and scalar-payload
  bit indexing are distinguished from Queue payload/state-prefix inference;
- business fields named `ready`, `full`, `commit`, `reservation` or `dummy` are
  accepted; only explicit forbidden Queue/control API calls fail closed;
- the official `ac.firing` catalog now describes 0..N inputs and outputs and
  distinguishes stateless PYC support from the stateful Table rejection;
- the release gate resolves C++ and Verilator before use and executes the
  public-Python stateful gfsim plus stateless PYC parity test once;
- PYC emits each firing expression DAG once and shares its named SSA results
  across all output values, presences and the guard. The eight-output regression
  prevents output-count-proportional re-emission.

The final independent code review reports no remaining P0, P1 or P2 finding.
One load-sensitive native performance test failed while four gate lanes ran in
parallel; its isolated rerun passed in 141 ms and the complete native suite
subsequently passed when run without competing heavy jobs. The recorded native
log is the successful complete rerun.

The full release matrix is intentionally outside this PR evidence. Stateful
Table lowering to PYC/RTL remains unsupported and is not represented as parity.
