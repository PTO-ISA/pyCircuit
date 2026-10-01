# M6-04 selected resource and finite-runtime baseline

Accepted: 2026-10-01. macOS arm64, base `9ff015a2` plus frozen developer-tool/test overlay.
Candidate aggregate `4418364a20201086fd83aa9c085625025705951e88ad81576232b201ebc61cf7`
is in `candidate-manifest.json`. Exact selected report SHA-256:
`8b013fc93bc1309710230ac64525a8725d63026719da86f87ad291915aa2c274`.
Native toolchain was freshly built from this checkout in `.pycircuit_out/m6-04-root`
and installed in separate `.pycircuit_out/m6-04-install`. No foreign or old toolchain
was used; source metadata binds baseline, developer-input hashes bind this overlay.

| Gate | Result | Evidence |
| --- | --- | --- |
| Fresh build/install | exit 0 | build.txt |
| Selected matrix | 4cases/32phase/24CABIrun, all valid | report.json; result-summary.json; measurement.txt |
| Independent unit/system/layout | 39 passed | independent reviewer/author record |
| PM post-format focused | 39 passed, zero skips | tests.txt; tests.xml |
| All-unit marker lane | result recorded separately | unit.txt; unit.xml |
| Hooks/strict docs | passed | precommit.txt; docs.txt |

Sizes are shared-definition 1/16 and distinct-source 1/8. All programs come from
existing explicit Python generator source DAGs, one compile per source, header-only
imports, native link, both target emits, separate CPP translation units and a
thin C ABI consumer of the standard executor. Every receipt file is contained and
regular; source-group implementation paths match their SourceOwner and are unique.
The three-epoch CPP scalar report oracle passes on each case. RTL inventory/emit
is checked, but RTL simulation/throughput is not executed or claimed in this packet.

Each case runs 1,000/10,000/100,000 epochs twice through the existing seven-callback
model API, validates exact termination epoch, report gauges, empty error and
Reset/replay. The consumer checks table version/size and all callbacks before use.
Returned timing separates setup, step callback loop, Reset/replay and total process
wall time. Zero timer duration has null rate; it is not infinite performance.

RSS is `ps`-sampled KiB summed for an owned process group. It is a sampled lower
bound on the time-peak of that sum, not exclusive/unique physical memory: shared
pages duplicate across processes, short-lived members can be missed, detached
processes outside the group are excluded. Requested and observed sample intervals,
unavailable reasons, timeout cleanup and ps overhead are in each record.
Callback rate includes ABI/executor work; process rate includes startup, replay and
sampler overhead. No exact peak, bare kernel, isolated steady state, full size
matrix or performance threshold is promised. Shared-1's initial run overlapped
about 12 seconds of PM test workload; samples are exploratory, not regression limits.

Review exposed real dev-tool defects and the candidate closed them: owned child
left after leader TERM exit, non-finite timers, a negative cleanup sleep, missing
candidate hash binding, inventory false-passes and malformed API tables. Independent
resistant-child, invalid numeric, receipt mutation and compiled fake-table cases
prove the corrected behavior. Earlier smoke/selected reports predated fixes and
are not accepted evidence. No product runtime/IR/public CLI/protocol was changed.

## Reproduction

Use LLVM/MLIR 22.1.8 and a Python with pytest/dev dependencies. Fresh build options
and prefix/version identities are in `report.json` metadata and `build.txt`.

```sh
PYC_BUILD_TESTING=OFF CMAKE_BUILD_PARALLEL_LEVEL=4 bash flows/scripts/pyc build --build-dir "$PWD/.pycircuit_out/m6-04-root" --install-prefix "$PWD/.pycircuit_out/m6-04-install"
python3 flows/tools/measure_m6_resources.py --prefix .pycircuit_out/m6-04-install --output-dir .pycircuit_out/m6-04-selected-accepted --sizes selected --epochs 1000 10000 100000 --repeats 2
PYCIRCUIT_M6_PREFIX="$PWD/.pycircuit_out/m6-04-install" python3 -m pytest tests/unit/test_m6_process_usage.py tests/system/test_m6_resource_runtime.py tests/unit/test_repository_layout.py -q
```

Output must be empty/absent; use new dirs and reconfigure so metadata matches your
HEAD when reproducing another revision. External developer report/input checksums
are gate provenance, not IR/model identity, generated names, formal schema or SDK ABI.
`report.json` is bounded and contains real argv/status/log paths; disposable full
case outputs remain regenerable. Broader fault/platform/SDK, backend selective
rebuild, Unicode host dirs and parallel simulation remain later M6 responsibilities.
