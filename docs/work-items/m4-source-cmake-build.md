# M4 explicit source-unit CMake build

Status: done for this bounded AC build slice; independent review APPROVE. Base: `ffef119c`, clean product checkout
`/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.

## Scope and authority

Implement the explicit source DAG allowed by approved C3-C §42 and §220–224,
and the public compile/link approval. Decisions 0158/0235 preserve the generic
framework boundary. No public project-generator interface, emit cutover, IR,
SDK, runtime or schema change. This is the AC source-build part of M4/V45;
source-owned C++ groups and independent C++ TUs remain outstanding.

## Ownership and lanes

- Implementation: `unit_pair_fix`, gpt-6-luna/high; exclusive
  `tests/integration/agentic-circuit/source-unit-build/` including README.
- Independent tests: `cmake_source_tests`, separate gpt-6-luna/high; exclusive
  `tests/system/test_source_unit_cmake_build.py`.
- Review: `other_agent_code_review`, gpt-5.6-sol/high, read-only frozen candidate.
- PM: integration, milestone ledger, this packet, evidence and commit. Shared
  native output `.pycircuit_out/w10-pm/build` remains PM-owned; no native source
  changes or rebuild are intended in this packet.

Agents share the checkout, preserve others' changes and do not recursively
orchestrate. Planning checkout's six pre-existing PM/runtime files are excluded.

## Acceptance

- Real per-source public compile producers, parent/root separately compiled;
  body/header/unit.json OUTPUT and stem.d DEPFILE, no post-split generation.
- Parent consumes provider header/receipt; full link consumes body/header/receipt.
- Clean parallel Ninja build, no-op rebuild, selective source change, imported
  header/receipt and helper/config invalidation; paths containing spaces.
- Header-only parent compile succeeds while full link without provider body
  refuses and preserves prior linked bytes.
- Archive exact commands, fresh results and independent candidate-bound review.

No C++/RTL execution, installation, release, Windows, performance or real runtime
parallelism claim follows from parallel source compilation.

## Verification and current outcome

Independent real CMake/Ninja test: 1 passed, no failures/errors/skips. Applicable
changed-file pre-commit checks passed. Raw first-failure and final-pass evidence,
exact test commands and eight candidate hashes are archived at
[the gate packet](../gates/logs/20260930-m4-source-cmake/README.md).

Ninja pre-creates empty output parents; the fixture handles this with a small
atomic-rmdir preparer, rejecting symlink destinations/ancestors and preserving
populated directories. The public driver/publication rules are unchanged.
The test demonstrates header-only parent compilation and failed full link that
preserves the existing final IR. Source build parallelism is not simulator
parallelism. M4 remains active pending source-owned C++ groups/TUs and the full
supported build/run workflow.

The final clean/rebuild regression was added by PM (separate from fixture author)
after the independent test instance could not resume due to the tool thread
limit. The reviewer checked the updated eight-file manifest and run-02 evidence;
[final independent review](../gates/logs/20260930-m4-source-cmake/review.md).
