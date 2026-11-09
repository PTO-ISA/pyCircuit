# Structural C++ compatibility baseline

This matrix is the delivery baseline for moving the proven final-ACIR native
behavior into the concrete-plan Structural C++ backend. It compares behavior,
not generated C++ spelling. `supported` requires plan validation plus compiled
execution evidence; `rejected` requires an admission diagnostic before bundle
publication. The numbered migration stages 1–8 are now implemented; remaining
gaps are listed explicitly instead of being hidden behind a planned stage.

| Capability | Structural C++ baseline | Required evidence |
| --- | --- | --- |
| Closed concrete module hierarchy | supported | plan, syntax and runtime tests |
| Stable specialization identity | supported | canonical-key and dedup tests |
| Bits compute and four-state Runtime values | supported | generated execution tests |
| DFF/DFFE/FIFO/memory lifecycle | supported | `SimSystem` commit/discard tests |
| Whole-system source checks | supported | plan and generated `Precheck` tests |
| Source-check occurrence count | supported | bundle contract test |
| Observation descriptors and metadata | supported | plan verifier and runner metadata tests |
| Observation staging/commit/discard | supported | generated runtime tests, including failed-cycle discard |
| Source-owned header/implementation split | supported | multi-TU build and source-map tests |
| Typed DUT drive/sample API | supported | generated DUT and runner tests |
| Host-driven combinational roots | supported | typed DUT keeps the root runnable without making nested combinational modules stateful |
| Fixed static collection lanes | supported | positive collection planning and execution tests |
| Whole-table publication after lane writes | supported | plan-only availability barriers and authored table examples |
| Nested aggregates and dynamic indexing | supported for approved fixed shapes | generated compute tests; unsupported forms fail closed |
| Four-state observation payloads | supported through arbitrary packed width | 130-bit X/Z staging and JSON tests |
| Complete four-state operation matrix | partial, fail-closed | approved Runtime operations have truth-table tests; unmapped operations are rejected |
| Python source-to-run regression | supported | public compile/link/emit/multi-TU build/run tests |

The typed host API is emitted in `design.hpp`; authored drivers do not depend
on a generated compatibility shim. A typed DUT root is marked host-driven so a
purely combinational design can execute one step for each supplied input. That
flag is not inherited by ordinary child modules: their `HasWork()` result still
comes only from stateful descendants. The root also preserves the committed
old-Q sample boundary, while nested modules refresh temporal outputs after
Xfer so their parents can consume newly committed state on the next Work.

Fixed tables use aliasing lane views. The plan records the lane's availability
parent and inserts a plan-only availability barrier after every lane writer has
completed. The barrier publishes the whole table to dependent operations but
emits no C++ statement. Explicit table construction is rendered as concrete
lane assignments; the renderer never rediscovers table semantics from HWIR.

The backend must fail closed for every row not yet supported. A successful
bundle with placeholder counts, empty metadata for retained observations, or a
renderer-time semantic lookup is a contract failure rather than partial
support.
