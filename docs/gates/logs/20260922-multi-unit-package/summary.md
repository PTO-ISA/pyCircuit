# Issue #180: multi-unit AC package linking and the structured bundle

## What was broken

No in-repo test exercised the multi-unit `--header-output` flow, and the flow
did not link. Three frontend properties were wrong or absent:

1. **Import nominal inventory.** `ac.module.import` declared an empty nominal
   declaration inventory while the provider published the payload nominals, so
   `acc` rejected the unit with `module import signature mismatch`.
2. **Declared interface skeleton.** An implementation whose symbol carries a
   declaration entry published a freshly resolved concrete interface instead of
   the declared skeleton, so the import schema and the provider schema differed
   in port names, queue envelope, and provenance.
3. **Nominal ownership.** Nominal declarations carried no `ac.source_file`, so
   the compiler grouped every type into `_compiler/common.py` and `acc` rejected
   the unit with `source interface type is owned by another Python file`. The
   closure capture additionally attributed every dependency declaration to the
   entry file, so a shared nominal appeared in two interface units and the link
   failed with `redefinition of symbol named 'In'`.

## What the test does

`tests/python/agentic-circuit/python_frontend/test_multi_unit_package.py`
builds a three-file workspace in a temporary directory — `source/child_a.py`
(`In -> Mid`), `source/child_b.py` (`Mid -> Out`, importing `Mid` from
child_a), and `source/core.py` (the system that composes both) — then compiles
each unit with `acc.py`, links the directory with `acc -c <package> -verify`, and
emits the structured bundle with `acc -c <package> -emit-cpp-bundle`.

Measured: 3 passed. The emitted bundle contains

```text
CMakeLists.txt
include/generated/dut.h
include/generated/model.h
include/generated/modules/child_a.hpp
include/generated/modules/child_b.hpp
share/generated/cost-report.json
share/generated/source-map.json
src/generated/model.cpp
src/generated/modules/child_a.cpp
src/generated/modules/child_b.cpp
src/generated/queuegraph.cpp
```

The published `share/generated/source-map.json` validates against
`schemas/agentic-circuit/source-map.schema.json` and reports both placements
(`child_a_0 -> @child_a`, `child_b_1 -> @child_b`). The per-source interface
units are disjoint: `child_a.ac` owns `@In` and `@Mid`, `child_b.ac` owns
`@Out`, and each carries its own `ac.module.import` with the provider's nominal
inventory.

Non-vacuity: reverting the frontend changes makes all three tests fail in the
link step (`source interface type is owned by another Python file`).

## Lane results

```text
tests/python/agentic-circuit (pytest, ignore tools/):  489 passed, 1 skipped, 5 failed
tests/unit -m unit:                                    217 passed
check-acir (tests/mlir):                               258 passed, 13 failed
ctest:                                                 6/6 passed
check-contracts.py / check-ir-coverage.py / catalog:   OK
```

The five failures are pre-existing at `091d56b1`: two environmental
`cli/test_discovery_commands.py` schema mismatches and three in the untracked
`test_dual_port_issue_queue.py`. The 13 lit failures are all
`verilator: command not found`.

## What is still open on #180

- No Python/JIT API (`lower_sources`/`lower_cpp`); the flow is driven by
  `acc.py` plus the native `acc`.
- No module/instance manifest beyond the bundle source map.
- Cross-unit placements publish an empty `source_provenance.origins` list in the
  bundle source map, so item V05 cannot yet pin module and specialization
  source-map goldens; that needs the closure capture to carry per-node source
  frames.
