# C2-R1 design documentation validation

Date: 2026-09-28. Checkout:
`/Users/zhoubot/linx-isa/tools/pyCircuit`.
Base HEAD: `7e5ffdc22416e8e63bf66cbee9dfa049a7614c41` plus the design,
review, work-item, plan, capability, ledger and navigation edits in this task.

Reviewed proposal revision B SHA-256:
`76283661bffba96aad544e31d2b4996f019135f448c4c8b685ebb02eab8edfba`.
The archived `revision-b.txt` matches the current proposal byte for byte.
Independent design review is recorded in
`docs/reviews/20260928-c2-r1-unified-register.md`.

| Check | Result |
| --- | --- |
| Changed-file pre-commit: merge markers, YAML, EOF, whitespace, Markdown | exit 0 |
| Explicit repository/docs API hygiene | exit 0 |
| Strict MkDocs build | exit 0 |
| git diff --check | exit 0 |
| Donor DFF/DFFE implementation/API/test hashes against pinned b852ed83 | all three match |

```bash
SKIP=pyc-api-hygiene pre-commit run --files docs/rfcs/migration/c2-r1-unified-register.md docs/work-items/m2-r1-unified-register.md docs/development/pycircuit-modernization-plan.md docs/work-items/migration-capabilities.md docs/work-items/single-route-migration.md docs/reviews/20260928-c2-r1-unified-register.md docs/reviews/README.md mkdocs.yml
python3 flows/tools/check_api_hygiene.py python/pycircuit/src/pycircuit examples/pycircuit docs README.md
mkdocs build --strict --site-dir .pycircuit_out/gates/20260928-unified-reg-design/site
git diff --check
```

Raw local logs are under
`.pycircuit_out/gates/20260928-unified-reg-design/` as `pre-commit.log`,
`api-hygiene.log`, and `mkdocs.log`. Python formatting hooks had no applicable
files; the duplicate pre-commit API-hygiene invocation was skipped because
the explicit full check ran separately. MkDocs printed existing informational
messages for excluded historical gate logs and an existing page absent from
navigation; the strict build succeeded.

No product source or runtime implementation changed. No R1 native, semantic,
parallel runtime, C++/RTL or installed-SDK test is claimed. The proposal names
future tests and targets explicitly as acceptance requirements. Historical
passing DFFE tests do not verify the new register contract.
