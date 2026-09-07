# DavinciOO contributor design program

Decision 0222 admits DavinciOO modules under `designs/davincioo/` so that
contributors can build a substantial design while improving reusable
pyCircuit capabilities. This is a maintainer-authorized exception to Decision
0158, not a change to the compiler's semantic boundaries.

NDF L0 means architectural intent, L1 behavior, and L2 microarchitecture.
Hardware H1/H2/H3 are all subdivisions of the L2 microarchitecture, not NDF
refinement levels. Concrete NDF identities and cache names remain unchanged.

The catalog covers seven H1 domains, 31 H2 subsystem groups and all 240 H3
vocabulary candidates. The external catalog calls only three candidates
reviewed leaves and leaves 237 unresolved. Newer packets and typed source
refine some of these; evidence and proposals remain separate.

## Location and ownership

- Implement accepted H3 leaves at `designs/davincioo/<h1>/<h2>/<h3>.py`.
  Contained state and aliases must not create duplicate hardware owners.
- Compose H2 and H1 using `assembly.py` at the corresponding level. Shared
  payload and state records live in `designs/davincioo/contracts/`.
- Keep design-specific tests and integration/reference harnesses under the
  design directory. Reusable compiler, primitive and simulator functionality
  stays in existing framework source roots.
- DavinciOO remains an architectural/NDF reference with exact revision/content
  provenance. New implementations in this program are owned by this design
  tree; there is no automatic bidirectional source mirror.
- A separate DavinciOO checkout is not required to validate the catalog or run
  self-contained design tests.

## Contribution and merge order

1. Claim a module ID and source/test ownership in the design tracking issue.
2. Resolve its disposition, typed ports, identity, state, reset, cancellation
   and parent-owned seams. Proposed interfaces require review before promotion.
3. Define a bounded implementation slice and independent expected results.
4. Reduce framework gaps to generic regressions and merge shared semantic
   fixes before their dependent designs.
5. Verify gfsim behavior and integrate H3 to H2 to H1. Require PYC/RTL parity
   where the shared lowering admits it; report remaining boundaries explicitly.

Issues #46 and #48 supply immediate optional-output and type-expression
capabilities. Issue #50 remains the capability roadmap; its earlier blanket
consumer-placement rule is superseded here by Decision 0222. Independent
modules may proceed when their own prerequisites are verified.

Detailed cards are in the
[design directory](https://github.com/PTO-ISA/pyCircuit/tree/main/designs/davincioo).

## Validation and completion

```bash
python3 designs/davincioo/tools/check_catalog.py
```

Implementation PRs also run design-local tests and the narrowest gates proving
changed framework semantics. Required PR CI stays lightweight. This decision
does not make 240 design workloads mandatory for every framework release;
promoted suites receive explicit gate ownership.

Track inventoried, contract-reviewed, implemented, gfsim-verified, integrated
and rtl-verified separately. Passing catalog checks does not mean that a
processor exists or that its modules passed simulation.
