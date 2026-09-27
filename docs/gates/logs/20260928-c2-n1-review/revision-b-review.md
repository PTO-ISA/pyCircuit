# C2-N1 revision B independent design review

Verdict: **revise**, validation-packet repair only. No architectural blocker.

Reviewer: namespace_review, independent architect, configured gpt-6-astra/xhigh.
Date: 2026-09-28. Separate from interface_design and PM; no authorship,
implementation, delegation or builds. PM transcribed this report.

Reviewed proposal: revision-b-proposal.md, preserved byte-for-byte.
SHA-256: 2a8ea8ac9c14768ef6f82314c0129500c7d768f41951e97beb6a48340cb46cf2.
Checkout HEAD: 03760118df71d5c231ae28cd05f2b29fb462be36 plus staged proposal/nav.
The reviewer rechecked the proposal hash after inspection.

## R1: complete validation acceptance before approval

The packet needs gate commands and case-to-gate mapping, required by the repo
independent design-review skill and project-governance interface-packet rules.
The existing ACIRSourceContractsTests target does not prove N1 coverage.

Add explicit observable cases for:

- Missing/wrong-typed top-level attributes, nondictionary elements, and all
  missing/extra/wrong-typed nested fields, not only generic closed records.
- A bad unused import or bad import later shadowed by a valid one must fail.
- Positive lowering removes both attributes; injecting either residual into
  final input fails through both cpp and verilog emit before publication.
- Independent expected table contents/order, including Unicode names, numeric
  AST indices and header permutations. Preserve C2 numeric-value acceptance;
  do not require every u64 carrier to be an i64 container.

Minimal repair: one acceptance subsection maps the existing P/N requirements
plus these explicit cases to structural, source/header, link and final/emitter
lanes, with commands, outcomes and nonempty discovery. Label planned tests as
planned. No implementation, U03 completion or resource design is required
before this proposal can be approved.

## Architectural assessment

The two-attribute schema, mandatory arrays, namespace/declaration authority
separation, canonical alias/constant/helper identity, Module-root NamespaceSite,
actual provider/name consumption snapshots, exact header/body mirror, explicit
header-only dependencies and final-IR removal are coherent. Complete consistent
cyclic header sets are distinct from producer startup without required headers.
No CLI/receipt/runtime ABI or alternative route is introduced.

General C1 scoping/rebinding ambiguity does not block this bounded named-import
slice. It defines repeated named-import replacement and retained consumption
history without redefining arbitrary assignment/function scoping. It must not
be implemented as blanket rejection of rebinding, private names, unused imports
or complete cyclic header sets. Nor does it authorize duplicate nominal
identities or a general namespace evaluator.

P09 must retain the HeaderView/FullSourceUnit boundary: no unmanaged projection,
recovery bypass, or compile-time child body/source read is authorized. Existing
source-contract helpers are implementation context, not N1 implementation proof.

C1/C2/C3 hashes were confirmed against their recorded approvals. N1 is not user
approved. Stop condition: repair R1, freeze new text and obtain independent
re-review. The repair should change acceptance details only.
