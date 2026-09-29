# C2-M1 design validation

Date: 2026-09-28. Checkout:
`/Users/zhoubot/linx-isa/tools/pyCircuit`.
Base HEAD: `7e5ffdc22416e8e63bf66cbee9dfa049a7614c41` plus the R1 and
M1 design documentation overlays from this conversation.

Proposal revision A SHA-256:
`7b7aec52144f2f44b0f6c410213faab4454b461c2efd7a6bb3c78bf282478f43`.
Archived `revision-a.txt` matches the proposal. The independent-design-review
status is **unavailable**, because both native dispatch attempts returned
`agent thread limit reached`. This validation is not approval-readiness.

| Check | Observed result |
| --- | --- |
| Four proposed Python examples | AST parse and Python syntax/lexical compilation passed; no model execution or import |
| Independent scalar state equations | Four cases passed: single/pipeline and forward/reversed rule computation |
| Physical reg inventory in those oracles | Single=4; pipeline=5 |
| Work-phase output sequences | Single: 0,2,5,5,5; pipeline: 0,0,3,6,6 |
| Completion publication | Commit epoch 5 for all four cases |
| Changed-file pre-commit | exit 0 |
| Explicit API hygiene | exit 0 |
| Strict MkDocs | exit 0 |
| git diff --check | exit 0 |

The oracle takes a dictionary copy of Q at the start of each epoch, computes
all proposals exclusively from that snapshot, rejects duplicate write keys,
and only then publishes them. It is ordinary Python mathematics, not a run of
the proposed model, pyCircuit, GFSIM or either emitter. Detailed results and
the proposal digest are in `example-validation.json`.

```bash
SKIP=pyc-api-hygiene pre-commit run --files docs/rfcs/migration/c2-m1-module-system.md docs/work-items/m2-m1-module-system.md docs/work-items/m2-r1-unified-register.md docs/development/pycircuit-modernization-plan.md docs/work-items/migration-capabilities.md docs/work-items/single-route-migration.md docs/reviews/20260928-c2-m1-module-system.md docs/reviews/README.md mkdocs.yml
python3 flows/tools/check_api_hygiene.py python/pycircuit/src/pycircuit examples/pycircuit docs README.md
mkdocs build --strict --site-dir .pycircuit_out/gates/20260928-module-system-design/site
git diff --check
```

Raw logs remain in `.pycircuit_out/gates/20260928-module-system-design/` as
`pre-commit.log`, `api-hygiene.log` and `mkdocs.log`. The duplicate pre-commit
API-hygiene invocation was skipped because the explicit full command ran.
Python formatting hooks had no applicable files. MkDocs historical-log
exclusion notices were informational; the strict build succeeded.

No product code changed. No source-to-ACIR, native compiler, GFSIM, RTL,
parallel runtime, installed SDK or release gate is claimed to have passed.
