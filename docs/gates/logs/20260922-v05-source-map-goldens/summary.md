# Item V05: source-map goldens for the reachable constructs

## Item

`#128` V05 — source-map golden tests covering the helper, module, projection,
and specialization constructs.

## What is covered

`tests/python/agentic-circuit/python_frontend/test_source_map_goldens.py` pins
the published source map for the two constructs the native AC bundle flow admits
today, one golden each under `tests/goldens/agentic-circuit/source-map/`:

| construct | golden | artifact |
| --- | --- | --- |
| inlined `@ac.inline` helper | `helper.json` | 1912 B |
| nested-record projection | `projection.json` | 1839 B |

Each test lowers the design with the frontend, freezes it through
`ac-freeze-topology`, runs `acc -c <unit.ac> -emit-cpp-bundle`, validates
`share/generated/source-map.json` against the published
`schemas/agentic-circuit/source-map.schema.json` (Draft 2020-12), and compares
the artifact byte-for-byte with the golden. The helper case additionally asserts
that the frames carry both `statement` (the helper definition site) and
`inline_callsite` (the expansion call site), which is the property inline
expansion has to preserve; the projection case asserts that every frame names
the normalized project-relative source file.

Measured: 2 passed. Tampering with one line number in the helper golden makes the
comparison fail, so the goldens are non-vacuous.

## What is blocked

The **module** and **specialization** constructs are *not* covered. Their frozen
units contain several `ac.module` definitions, and `acc -emit-cpp-bundle` accepts
a multi-definition unit only as a linked AC package directory:

```text
ACLOWER-QUEUE-CXX: acc: structured input requires a directory-backed AC package;
a standalone AC source unit may contain multiple definitions only when they
share one ac.source_file
```

No frontend surface produces that linked package yet — it is the multi-unit flow
tracked by issue #180. A single-definition unit does work, which is why the two
flat constructs above are reachable. Recording V05 as partially satisfied and
naming the dependency, rather than pinning module/specialization goldens that
cannot be regenerated from a repository command.

## Scope

Item V05 only. V01-V03 are recorded in
`docs/gates/logs/20260922-v01-v02-cross-layer-audit/` and
`docs/gates/logs/20260922-v03-pyc-closure/`. V04 and V07 still need a Verilog
toolchain, the simulation lanes, and strict documentation; V08 is a consumer-side
obligation under Decisions 0158 and 0235.
