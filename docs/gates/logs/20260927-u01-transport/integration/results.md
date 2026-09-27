# Private source transport integration

Candidate commit: `7f96a877573995623b3a49ec48999401f2fe4021`.
Primary integration commit: `8b39f70a`.

The Luna high implementation and independent Sol medium tests were reviewed by
Sol high. Review A requested removal of the Homebrew-only parser dependency
from pure unit tests; review B passed the repaired three-file candidate.
The manifest in `../review-b/content-sha256.txt` binds the reviewed source.

Fresh primary-checkout validation:

- Unit lane: 266 passed with both in-tree Python source packages on PYTHONPATH.
- Configured LLVM 22.1.8 parser lane: 4 passed, no skips.
- Ruff: passed.
- Documentation/repository tests: 16 passed; strict MkDocs build passed.

`commands.json` and adjacent logs retain exact invocation and exit codes.
The initial unit attempts exposed a subprocess import setup gap: plain pytest
did not expose pycircuit to the subprocess; adding only that source package
then exposed the missing semantic-core package. The passing invocation includes
both current-checkout source paths. No source fix or borrowed build was used.

The isolated U01 native candidate separately has an old primitive-catalog unit
failure after schema replacement; it is not included in this transport commit
and remains an integration obligation. This result proves private AST transport,
not source-unit authority, helper execution, final IR, or either backend.

## Filesystem portability follow-up

Test-only candidate `6ac45c51` was independently reviewed by Sol high with
PASS in `../review-c/summary.md`, then integrated as `c96c909b`. Quoted paths
are now synthetic capture metadata over a portable real filename, so tests
do not require creating a Windows-invalid filename. The serializer is unchanged.
Primary command `MLIR_OPT=/opt/homebrew/opt/llvm/bin/mlir-opt pytest
tests/unit/test_source_transport.py tests/system/test_source_transport_mlir.py
-q` passed all 17 tests, exit 0. This is a reviewed portability fix tested on
macOS; no Windows execution result is claimed.
