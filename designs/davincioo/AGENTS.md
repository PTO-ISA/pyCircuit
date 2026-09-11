# DavinciOO contributor design program

Decision 0222 and the maintainer's 2026-09-07 instruction authorize this
in-repository design program. Do not use Decision 0158 to reject DavinciOO
module implementations placed here. Other consumers retain their boundaries.

- Read `README.md`, `MODULE_CHECKLIST.md`, and the module card before claiming
  work. Include contained state, aliases and unresolved ownership in reviews.
- Claim a bounded module or assembly and its source/test paths in the tracking
  issue. A proposed port is an interface design task, not a frozen contract.
- Put ordinary `.py` implementations next to their cards, shared contracts in
  `contracts/`, tests in `tests/`, and tooling in `tools/`. Do not create empty
  Python modules to make the inventory look implemented.
- Order a card so a reader learns what the module does before why it is built
  that way. The functional sections come first: the problem it solves, where it
  sits among its siblings, and how one operation completes end to end. Framework
  limits and deviations belong in a later "Departures from the original proposal"
  section, never interleaved with the functional description -- a constraint
  argument is review material, not an explanation of the hardware.
  `tmu/trn/fre.md` is the reference structure.
- A `_CN.md` card is an independent Chinese rendering, not a sentence-by-sentence
  mirror of the English one. Literal, nominalization-heavy translation is a
  defect to fix, not a style choice.
- Resolve contained-state and alias ownership before creating an independent
  module. Each persistent state value has exactly one owner.
- Keep public Python marker-free. ACIR infers handshake, effects and commit
  groups; retain CycleAwareSignal contracts on PYC authoring paths.
- Reduce a framework/primitive gap to a generic failing test. Link its focused
  issue/PR, implement shared verifier/pass/runtime semantics, then land the
  dependent design. No design-name exceptions in compiler/runtime code.
- Every implementation PR supplies input/output types, state/reset, transaction
  and cancellation rules, expected results, gfsim evidence, and admitted
  PYC/RTL evidence or an explicit unsupported boundary.
- Only promote contract-reviewed, implemented, gfsim-verified or rtl-verified
  status when corresponding evidence exists. Inventory checks prove no runtime.
- Preserve source provenance when porting existing work. Do not edit the
  external DavinciOO checkout incidentally or create an automatic code mirror.
- Generated models, traces and binaries belong in `.pycircuit_out/`.
