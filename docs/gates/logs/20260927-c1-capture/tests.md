# Independent C1 capture validation

Test author: baseline_verification, test-engineer, gpt-5.6-sol, medium.
Implementation author: governance_impl, executor, gpt-5.6-sol, medium.
Candidate and checkout: candidate.json in this directory.

- `pytest tests/unit/test_source_capture.py -q`: exit 0, 36 passed.
- `pytest tests/unit -m unit -q`: exit 0, 253 passed.
- `ruff check tests/unit/test_source_capture.py`: exit 0.
- `ruff format --check tests/unit/test_source_capture.py`: exit 0.
- PM: `GIT_WORK_TREE="$PWD" pre-commit run --files python/pycircuit/src/pycircuit/_source_capture.py tests/unit/test_source_capture.py`: exit 0, all applicable hooks passed; file contents unchanged.

The imported implementation path was independently confirmed inside the capture worktree. Five C1 fixtures are parsed only; their backend execution remains UNRUN. These checks do not prove MLIR, runtime, backend equivalence, or full migration completion.

Coverage includes model side effects never executing, source encodings and BOM, Unicode byte/codepoint columns, CRLF/CR, Unicode separators inside strings, multiline spans, malformed positions, type comments, root/symlink confinement, and fixture AST capture.

Git commands in the capture worktree use explicit GIT_WORK_TREE because this repository has a shared core.worktree setting pointing at the primary checkout. No shared configuration was changed and no native binaries were copied.
