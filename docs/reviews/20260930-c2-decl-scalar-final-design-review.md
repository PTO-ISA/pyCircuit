# C2-DECL scalar final projection — independent design review

Date: 2026-09-30. Reviewer: `/root/declaration_design_review`, architect,
gpt-6-astra/xhigh, independent from PM writer and architecture advisor.
Proposal: [C2-DECL revision A](../rfcs/migration/c2-decl-scalar-final.md).
Final SHA-256: `38dd31d13cff150cf7b778e9c3df469f9ab1e0b55c8b2311f7c8d05d49b736b8`.

## Final verdict

**approval-ready**; no remaining must-fix findings. The reviewer read the full
corrected candidate and checked its hash. Static design review only: no edits,
builds or implementation acceptance. This permits requesting precise user
approval and does not itself authorize implementation. Material changes require
new independent review and a new content binding.

The final review confirms contextual origin-to-symbol binding, canonical
import-module uniqueness across all unit kinds (including empty/facade), and
immediate-parent declaration placement. C++ scalar types and dependency names
are globally qualified. Reserved global std/gfsim/glue scope collisions are
explicitly target-side restrictions; nested source namespaces remain supported.
All have corresponding positive/negative acceptance cases.

Scalar widths, bool/integer-i1 distinction, exact MathInt retention, native C++
range rejection, canonical ownership selection and source-owned header-only
outputs are coherent. The non-scalar admission tightening is disclosed.
SYSTEM/EXPECT revision B and public emit remain outside scope; gates are planned.

## First review and disposition

First reviewed SHA-256:
`c538c719fc702b2ae3f931621ae1d34dc4c68df92563d6bd6d4ff4cdb0d19afb`.
Verdict: revise. Findings and final disposition:

1. Structural Occurrence validation alone does not bind origin to its symbol.
   Final proposal requires exact origin.site.definition equality and a mutation test.
2. Empty owners can collide on canonical import-module identity without duplicate
   symbols. Final proposal checks every owner and tests empty/facade collisions.
3. Local `Std` can shadow standard type references. Final proposal uses ::std
   throughout declaration/module/system dependency references, includes ::gfsim
   qualification and implementation-owned shadowing positives, and explicitly
   rejects occupied global target scopes without changing source/link/RTL.

The implementation watch on declarations nested inside ac.module/rule regions
was made an explicit immediate-parent rule and rejection gate. No product code
was written to make the proposed shape pass prematurely.
